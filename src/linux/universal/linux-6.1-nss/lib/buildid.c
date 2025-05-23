// SPDX-License-Identifier: GPL-2.0

#include <linux/buildid.h>
#include <linux/cache.h>
#include <linux/elf.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/secretmem.h>

#define BUILD_ID 3

/*
 * Parse build id from the note segment. This logic can be shared between
 * 32-bit and 64-bit system, because Elf32_Nhdr and Elf64_Nhdr are
 * identical.
 */
static int parse_build_id_buf(unsigned char *build_id,
			      __u32 *size,
			      const void *note_start,
			      Elf32_Word note_size)
{
	const char note_name[] = "GNU";
	const size_t note_name_sz = sizeof(note_name);
	u64 note_off = 0, new_off, name_sz, desc_sz;
	const char *data;

	while (note_off + sizeof(Elf32_Nhdr) < note_size &&
	       note_off + sizeof(Elf32_Nhdr) > note_off /* overflow */) {
		Elf32_Nhdr *nhdr = (Elf32_Nhdr *)(note_start + note_off);

		name_sz = READ_ONCE(nhdr->n_namesz);
		desc_sz = READ_ONCE(nhdr->n_descsz);

		new_off = note_off + sizeof(Elf32_Nhdr);
		if (check_add_overflow(new_off, ALIGN(name_sz, 4), &new_off) ||
		    check_add_overflow(new_off, ALIGN(desc_sz, 4), &new_off) ||
		    new_off > note_size)
			break;

		if (nhdr->n_type == BUILD_ID &&
		    name_sz == note_name_sz &&
		    memcmp(nhdr + 1, note_name, note_name_sz) == 0 &&
		    desc_sz > 0 && desc_sz <= BUILD_ID_SIZE_MAX) {
			data = note_start + note_off + sizeof(Elf32_Nhdr) + ALIGN(note_name_sz, 4);
			memcpy(build_id, data, desc_sz);
			memset(build_id + desc_sz, 0, BUILD_ID_SIZE_MAX - desc_sz);
			if (size)
				*size = desc_sz;
			return 0;
		}

		note_off = new_off;
	}

	return -EINVAL;
}

static inline int parse_build_id(const void *page_addr,
				 unsigned char *build_id,
				 __u32 *size,
				 const void *note_start,
				 Elf32_Word note_size)
{
	/* check for overflow */
	if (note_start < page_addr || note_start + note_size < note_start)
		return -EINVAL;

	/* only supports note that fits in the first page */
	if (note_start + note_size > page_addr + PAGE_SIZE)
		return -EINVAL;

	return parse_build_id_buf(build_id, size, note_start, note_size);
}

/* Parse build ID from 32-bit ELF */
static int get_build_id_32(const void *page_addr, unsigned char *build_id,
			   __u32 *size)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)page_addr;
	Elf32_Phdr *phdr;
	__u32 i, phnum;

	/*
	 * FIXME
	 * Neither ELF spec nor ELF loader require that program headers
	 * start immediately after ELF header.
	 */
	if (ehdr->e_phoff != sizeof(Elf32_Ehdr))
		return -EINVAL;

	phnum = READ_ONCE(ehdr->e_phnum);
	/* only supports phdr that fits in one page */
	if (phnum > (PAGE_SIZE - sizeof(Elf32_Ehdr)) / sizeof(Elf32_Phdr))
		return -EINVAL;

	phdr = (Elf32_Phdr *)(page_addr + sizeof(Elf32_Ehdr));

	for (i = 0; i < phnum; ++i) {
		if (phdr[i].p_type == PT_NOTE &&
		    !parse_build_id(page_addr, build_id, size,
				    page_addr + READ_ONCE(phdr[i].p_offset),
				    READ_ONCE(phdr[i].p_filesz)))
			return 0;
	}
	return -EINVAL;
}

/* Parse build ID from 64-bit ELF */
static int get_build_id_64(const void *page_addr, unsigned char *build_id,
			   __u32 *size)
{
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)page_addr;
	Elf64_Phdr *phdr;
	__u32 i, phnum;

	/*
	 * FIXME
	 * Neither ELF spec nor ELF loader require that program headers
	 * start immediately after ELF header.
	 */
	if (ehdr->e_phoff != sizeof(Elf64_Ehdr))
		return -EINVAL;

	phnum = READ_ONCE(ehdr->e_phnum);
	/* only supports phdr that fits in one page */
	if (phnum > (PAGE_SIZE - sizeof(Elf64_Ehdr)) / sizeof(Elf64_Phdr))
		return -EINVAL;

	phdr = (Elf64_Phdr *)(page_addr + sizeof(Elf64_Ehdr));

	for (i = 0; i < phnum; ++i) {
		if (phdr[i].p_type == PT_NOTE &&
		    !parse_build_id(page_addr, build_id, size,
				    page_addr + READ_ONCE(phdr[i].p_offset),
				    READ_ONCE(phdr[i].p_filesz)))
			return 0;
	}
	return -EINVAL;
}

/*
 * Parse build ID of ELF file mapped to vma
 * @vma:      vma object
 * @build_id: buffer to store build id, at least BUILD_ID_SIZE long
 * @size:     returns actual build id size in case of success
 *
 * Return: 0 on success, -EINVAL otherwise
 */
int build_id_parse(struct vm_area_struct *vma, unsigned char *build_id,
		   __u32 *size)
{
	Elf32_Ehdr *ehdr;
	struct page *page;
	void *page_addr;
	int ret;

	/* only works for page backed storage  */
	if (!vma->vm_file)
		return -EINVAL;

	/* reject secretmem folios created with memfd_secret() */
	if (vma_is_secretmem(vma))
		return -EFAULT;

	page = find_get_page(vma->vm_file->f_mapping, 0);
	if (!page)
		return -EFAULT;	/* page not mapped */
	if (!PageUptodate(page)) {
		put_page(page);
		return -EFAULT;
	}

	ret = -EINVAL;
	page_addr = kmap_atomic(page);
	ehdr = (Elf32_Ehdr *)page_addr;

	/* compare magic x7f "ELF" */
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
		goto out;

	/* only support executable file and shared object file */
	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
		goto out;

	if (ehdr->e_ident[EI_CLASS] == ELFCLASS32)
		ret = get_build_id_32(page_addr, build_id, size);
	else if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
		ret = get_build_id_64(page_addr, build_id, size);
out:
	kunmap_atomic(page_addr);
	put_page(page);
	return ret;
}

/**
 * build_id_parse_buf - Get build ID from a buffer
 * @buf:      Elf note section(s) to parse
 * @buf_size: Size of @buf in bytes
 * @build_id: Build ID parsed from @buf, at least BUILD_ID_SIZE_MAX long
 *
 * Return: 0 on success, -EINVAL otherwise
 */
int build_id_parse_buf(const void *buf, unsigned char *build_id, u32 buf_size)
{
	return parse_build_id_buf(build_id, NULL, buf, buf_size);
}

#if IS_ENABLED(CONFIG_STACKTRACE_BUILD_ID) || IS_ENABLED(CONFIG_CRASH_CORE)
unsigned char vmlinux_build_id[BUILD_ID_SIZE_MAX] __ro_after_init;

/**
 * init_vmlinux_build_id - Compute and stash the running kernel's build ID
 */
void __init init_vmlinux_build_id(void)
{
	extern const void __start_notes __weak;
	extern const void __stop_notes __weak;
	unsigned int size = &__stop_notes - &__start_notes;

	build_id_parse_buf(&__start_notes, vmlinux_build_id, size);
}
#endif
