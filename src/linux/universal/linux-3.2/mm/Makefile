#
# Makefile for the linux memory manager.
#

mmu-y			:= nommu.o
mmu-$(CONFIG_MMU)	:= fremap.o highmem.o madvise.o memory.o mincore.o \
			   mlock.o mmap.o mprotect.o mremap.o msync.o rmap.o \
			   vmalloc.o pagewalk.o pgtable-generic.o \
			   process_vm_access.o

obj-y			:= filemap.o mempool.o oom_kill.o fadvise.o \
			   maccess.o page_alloc.o page-writeback.o \
			   readahead.o swap.o truncate.o vmscan.o shmem.o \
			   interval_tree.o util.o mmzone.o vmstat.o backing-dev.o \
			   page_isolation.o mm_init.o mmu_context.o percpu.o \
			   $(mmu-y)
obj-y += init-mm.o

ifdef CONFIG_NO_BOOTMEM
	obj-y		+= nobootmem.o
else
	obj-y		+= bootmem.o
endif

obj-$(CONFIG_HAVE_MEMBLOCK) += memblock.o

obj-$(CONFIG_BOUNCE)	+= bounce.o
obj-$(CONFIG_SWAP)	+= page_io.o swap_state.o swapfile.o thrash.o
obj-$(CONFIG_HAS_DMA)	+= dmapool.o
obj-$(CONFIG_HUGETLBFS)	+= hugetlb.o
obj-$(CONFIG_NUMA) 	+= mempolicy.o
obj-$(CONFIG_SPARSEMEM)	+= sparse.o
obj-$(CONFIG_SPARSEMEM_VMEMMAP) += sparse-vmemmap.o
obj-$(CONFIG_SLOB) += slob.o
obj-$(CONFIG_COMPACTION) += compaction.o
obj-$(CONFIG_MMU_NOTIFIER) += mmu_notifier.o
obj-$(CONFIG_KSM) += ksm.o
obj-$(CONFIG_PAGE_POISONING) += debug-pagealloc.o
obj-$(CONFIG_SLAB) += slab.o
obj-$(CONFIG_SLUB) += slub.o
obj-$(CONFIG_KMEMCHECK) += kmemcheck.o
obj-$(CONFIG_FAILSLAB) += failslab.o
obj-$(CONFIG_MEMORY_HOTPLUG) += memory_hotplug.o
obj-$(CONFIG_FS_XIP) += filemap_xip.o
obj-$(CONFIG_MIGRATION) += migrate.o
obj-$(CONFIG_QUICKLIST) += quicklist.o
obj-$(CONFIG_TRANSPARENT_HUGEPAGE) += huge_memory.o
obj-$(CONFIG_CGROUP_MEM_RES_CTLR) += memcontrol.o page_cgroup.o
obj-$(CONFIG_MEMORY_FAILURE) += memory-failure.o
obj-$(CONFIG_HWPOISON_INJECT) += hwpoison-inject.o
obj-$(CONFIG_DEBUG_KMEMLEAK) += kmemleak.o
obj-$(CONFIG_DEBUG_KMEMLEAK_TEST) += kmemleak-test.o
obj-$(CONFIG_CLEANCACHE) += cleancache.o
