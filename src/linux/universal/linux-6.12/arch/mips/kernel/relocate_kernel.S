/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * relocate_kernel.S for kexec
 * Created by <nschichan@corp.free.fr> on Thu Oct 12 17:49:57 2006
 */

#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/addrspace.h>
#include "machine_kexec.h"

#include <kernel-entry-init.h>

LEAF(kexec_relocate_new_kernel)
	PTR_L a0,	arg0
	PTR_L a1,	arg1
	PTR_L a2,	arg2
	PTR_L a3,	arg3

	PTR_L		s0, kexec_indirection_page
	PTR_L		s1, kexec_start_address

process_entry:
	PTR_L		s2, (s0)
	PTR_ADDIU	s0, s0, SZREG

	/*
	 * In case of a kdump/crash kernel, the indirection page is not
	 * populated as the kernel is directly copied to a reserved location
	 */
	beqz		s2, done

	/* destination page */
	and		s3, s2, 0x1
	beq		s3, zero, 1f
	and		s4, s2, ~0x1	/* store destination addr in s4 */
	b		process_entry

1:
	/* indirection page, update s0	*/
	and		s3, s2, 0x2
	beq		s3, zero, 1f
	and		s0, s2, ~0x2
	b		process_entry

1:
	/* done page */
	and		s3, s2, 0x4
	beq		s3, zero, 1f
	b		done
1:
	/* source page */
	and		s3, s2, 0x8
	beq		s3, zero, process_entry
	and		s2, s2, ~0x8
	li		s6, (1 << _PAGE_SHIFT) / SZREG

copy_word:
	/* copy page word by word */
	REG_L		s5, (s2)
	REG_S		s5, (s4)
	PTR_ADDIU	s4, s4, SZREG
	PTR_ADDIU	s2, s2, SZREG
	LONG_ADDIU	s6, s6, -1
	beq		s6, zero, process_entry
	b		copy_word

done:
#ifdef CONFIG_SMP
	/* kexec_flag reset is signal to other CPUs what kernel
	   was moved to its location. Note - we need relocated address
	   of kexec_flag.  */

	bal		1f
 1:	move		t1,ra;
	PTR_LA		t2,1b
	PTR_LA		t0,kexec_flag
	PTR_SUB		t0,t0,t2;
	PTR_ADD		t0,t1,t0;
	LONG_S		zero,(t0)
#endif

#ifdef CONFIG_CPU_CAVIUM_OCTEON
	/* We need to flush I-cache before jumping to new kernel.
	 * Unfortunately, this code is cpu-specific.
	 */
	.set push
	.set noreorder
	syncw
	syncw
	synci		0($0)
	.set pop
#else
	sync
#endif
	/* jump to kexec_start_address */
	j		s1
	END(kexec_relocate_new_kernel)

#ifdef CONFIG_SMP
/*
 * Other CPUs should wait until code is relocated and
 * then start at entry (?) point.
 */
LEAF(kexec_smp_wait)
	PTR_L		a0, s_arg0
	PTR_L		a1, s_arg1
	PTR_L		a2, s_arg2
	PTR_L		a3, s_arg3
	PTR_L		s1, kexec_start_address

	/* Non-relocated address works for args and kexec_start_address ( old
	 * kernel is not overwritten). But we need relocated address of
	 * kexec_flag.
	 */

	bal		1f
1:	move		t1,ra;
	PTR_LA		t2,1b
	PTR_LA		t0,kexec_flag
	PTR_SUB		t0,t0,t2;
	PTR_ADD		t0,t1,t0;

1:	LONG_L		s0, (t0)
	bne		s0, zero,1b

#ifdef USE_KEXEC_SMP_WAIT_FINAL
	kexec_smp_wait_final
#else
	sync
#endif
	j		s1
	END(kexec_smp_wait)
#endif

#ifdef __mips64
       /* all PTR's must be aligned to 8 byte in 64-bit mode */
       .align  3
#endif

/* All parameters to new kernel are passed in registers a0-a3.
 * kexec_args[0..3] are used to prepare register values.
 */

EXPORT(kexec_args)
arg0:	PTR_WD		0x0
arg1:	PTR_WD		0x0
arg2:	PTR_WD		0x0
arg3:	PTR_WD		0x0
	.size	kexec_args,PTRSIZE*4

#ifdef CONFIG_SMP
/*
 * Secondary CPUs may have different kernel parameters in
 * their registers a0-a3. secondary_kexec_args[0..3] are used
 * to prepare register values.
 */
EXPORT(secondary_kexec_args)
s_arg0: PTR_WD		0x0
s_arg1: PTR_WD		0x0
s_arg2: PTR_WD		0x0
s_arg3: PTR_WD		0x0
	.size	secondary_kexec_args,PTRSIZE*4
kexec_flag:
	LONG		0x1

#endif

EXPORT(kexec_start_address)
	PTR_WD		0x0
	.size		kexec_start_address, PTRSIZE

EXPORT(kexec_indirection_page)
	PTR_WD		0
	.size		kexec_indirection_page, PTRSIZE

kexec_argv_buf:
	EXPORT(kexec_argv_buf)
	.skip		KEXEC_COMMAND_LINE_SIZE
	.size		kexec_argv_buf, KEXEC_COMMAND_LINE_SIZE

kexec_argv:
	EXPORT(kexec_argv)
	.skip		KEXEC_ARGV_SIZE
	.size		kexec_argv, KEXEC_ARGV_SIZE

kexec_relocate_new_kernel_end:
	EXPORT(kexec_relocate_new_kernel_end)
