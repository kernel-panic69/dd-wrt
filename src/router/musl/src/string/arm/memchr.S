/*
 * memchr - scan memory for a character
 *
 * Copyright (c) 2010-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/*
   Written by Dave Gilbert <david.gilbert@linaro.org>

   This __memchr_arm routine is optimised on a Cortex-A9 and should work on
   all ARMv7 processors.   It has a fast past for short sizes, and has
   an optimised path for large data sets; the worst case is finding the
   match early in a large data set.

 */
#if __ARM_ARCH > 6

@ 2011-02-07 david.gilbert@linaro.org
@    Extracted from local git a5b438d861
@ 2011-07-14 david.gilbert@linaro.org
@    Import endianness fix from local git ea786f1b
@ 2011-12-07 david.gilbert@linaro.org
@    Removed unneeded cbz from align loop

	.syntax unified
#if __ARM_ARCH >= 8 && __ARM_ARCH_PROFILE == 'M'
    /* keep config inherited from -march= */
#else
	.arch armv7-a
#endif

@ this lets us check a flag in a 00/ff byte easily in either endianness
#ifdef __ARMEB__
#define CHARTSTMASK(c) 1<<(31-(c*8))
#else
#define CHARTSTMASK(c) 1<<(c*8)
#endif
	.thumb
#include "asmdefs.h"


@ ---------------------------------------------------------------------------
	.thumb_func
	.align 2
	.p2align 4,,15
	.global memchr
	.type memchr,%function
	.fnstart
	.cfi_startproc
memchr:
	@ r0 = start of memory to scan
	@ r1 = character to look for
	@ r2 = length
	@ returns r0 = pointer to character or NULL if not found
	prologue
	and	r1,r1,#0xff	@ Don't think we can trust the caller to actually pass a char

	cmp	r2,#16		@ If it's short don't bother with anything clever
	blt	20f 

	tst	r0, #7		@ If it's already aligned skip the next bit
	beq	10f

	@ Work up to an aligned point
5:
	ldrb	r3, [r0],#1
	subs	r2, r2, #1
	cmp	r3, r1
	beq	50f		@ If it matches exit found
	tst	r0, #7
	bne	5b		@ If not aligned yet then do next byte
	
10:
	@ At this point, we are aligned, we know we have at least 8 bytes to work with
	push	{r4,r5,r6,r7}
	.cfi_adjust_cfa_offset 16
	.cfi_rel_offset 4, 0
	.cfi_rel_offset 5, 4
	.cfi_rel_offset 6, 8
	.cfi_rel_offset 7, 12
	orr	r1, r1, r1, lsl #8	@ expand the match word across to all bytes
	orr	r1, r1, r1, lsl #16
	bic	r4, r2, #7	@ Number of double words to work with
	mvns	r7, #0		@ all F's
	movs	r3, #0
	
15:
	ldmia	r0!,{r5,r6}
	subs	r4, r4, #8
	eor	r5,r5, r1	@ Get it so that r5,r6 have 00's where the bytes match the target
	eor	r6,r6, r1
	uadd8	r5, r5, r7	@ Parallel add 0xff - sets the GE bits for anything that wasn't 0
	sel	r5, r3, r7	@ bytes are 00 for none-00 bytes, or ff for 00 bytes - NOTE INVERSION
	uadd8	r6, r6, r7	@ Parallel add 0xff - sets the GE bits for anything that wasn't 0
	sel	r6, r5, r7	@ chained....bytes are 00 for none-00 bytes, or ff for 00 bytes - NOTE INVERSION
	cbnz	r6, 60f
	bne	15b		@ (Flags from the subs above) If not run out of bytes then go around again

	pop	{r4,r5,r6,r7}
	.cfi_restore 7
	.cfi_restore 6
	.cfi_restore 5
	.cfi_restore 4
	.cfi_adjust_cfa_offset -16
	and	r1,r1,#0xff	@ Get r1 back to a single character from the expansion above
	and	r2,r2,#7	@ Leave the count remaining as the number after the double words have been done
 
20:
	cbz	r2, 40f		@ 0 length or hit the end already then not found

21:  @ Post aligned section, or just a short call
	ldrb	r3,[r0],#1
	subs	r2,r2,#1
	eor	r3,r3,r1	@ r3 = 0 if match - doesn't break flags from sub
	cbz	r3, 50f
	bne	21b		@ on r2 flags

40:
	.cfi_remember_state
	movs	r0,#0		@ not found
	epilogue

50:
	.cfi_restore_state
	.cfi_remember_state
	subs	r0,r0,#1	@ found
	epilogue

60:  @ We're here because the fast path found a hit - now we have to track down exactly which word it was
	@ r0 points to the start of the double word after the one that was tested
	@ r5 has the 00/ff pattern for the first word, r6 has the chained value
	.cfi_restore_state	@ Standard post-prologue state
	.cfi_adjust_cfa_offset 16
	.cfi_rel_offset	4, 0
	.cfi_rel_offset 5, 4
	.cfi_rel_offset 6, 8
	.cfi_rel_offset 7, 12
	cmp	r5, #0
	itte	eq
	moveq	r5, r6		@ the end is in the 2nd word
	subeq	r0,r0,#3	@ Points to 2nd byte of 2nd word
	subne	r0,r0,#7	@ or 2nd byte of 1st word

	@ r0 currently points to the 3rd byte of the word containing the hit
	tst	r5, # CHARTSTMASK(0)	@ 1st character
	bne	61f
	adds	r0,r0,#1
	tst	r5, # CHARTSTMASK(1)	@ 2nd character
	ittt	eq
	addeq	r0,r0,#1
	tsteq	r5, # (3<<15)		@ 2nd & 3rd character
	@ If not the 3rd must be the last one
	addeq	r0,r0,#1

61:
	pop	{r4,r5,r6,r7}
	.cfi_restore 7
	.cfi_restore 6
	.cfi_restore 5
	.cfi_restore 4
	.cfi_adjust_cfa_offset -16
	subs	r0,r0,#1
	epilogue
	.cfi_endproc
	.cantunwind
	.fnend

	.size	memchr, . - memchr
#endif