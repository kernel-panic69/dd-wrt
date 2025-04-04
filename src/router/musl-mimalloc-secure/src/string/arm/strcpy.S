#if __ARM_ARCH >=7 

/*
 * Copyright (C) 2013 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2013 ARM Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define STRCPY

#if !defined(STPCPY) && !defined(STRCPY)
#error "Either STPCPY or STRCPY must be defined."
#endif

#include "asmdefs.h"

    .syntax unified

    .thumb
    .thumb_func

    // To avoid warning about deprecated instructions, add an explicit
    // arch. The code generated is exactly the same.
    .arch armv7-a

#if defined(STPCPY)
    .macro m_push
    push    {r4, r5, lr}
    .cfi_def_cfa_offset 12
    .cfi_rel_offset r4, 0
    .cfi_rel_offset r5, 4
    .cfi_rel_offset lr, 8
    .endm // m_push
#else
    .macro m_push
    push    {r0, r4, r5, lr}
    .cfi_def_cfa_offset 16
    .cfi_rel_offset r0, 0
    .cfi_rel_offset r4, 4
    .cfi_rel_offset r5, 8
    .cfi_rel_offset lr, 12
    .endm // m_push
#endif

#if defined(STPCPY)
    .macro m_ret inst
    \inst   {r4, r5, pc}
    .endm // m_ret
#else
    .macro m_ret inst
    \inst   {r0, r4, r5, pc}
    .endm // m_ret
#endif

    .macro m_copy_byte reg, cmd, label
    ldrb    \reg, [r1], #1
    strb    \reg, [r0], #1
    \cmd    \reg, \label
    .endm // m_copy_byte

#if defined(STPCPY)
ENTRY(stpcpy)
#else
ENTRY(strcpy)
#endif
    // Unroll the first 8 bytes that will be copied.
    m_push
    m_copy_byte reg=r2, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r3, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r4, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r5, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r2, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r3, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r4, cmd=cbz, label=.Lstringcopy_finish
    m_copy_byte reg=r5, cmd=cbnz, label=.Lstringcopy_continue

.Lstringcopy_finish:
#if defined(STPCPY)
    sub     r0, r0, #1
#endif
    m_ret   inst=pop

.Lstringcopy_continue:
    pld     [r1, #0]
    ands    r3, r0, #7
    bne     .Lstringcopy_align_dst

.Lstringcopy_check_src_align:
    // At this point dst is aligned to a double word, check if src
    // is also aligned to a double word.
    ands    r3, r1, #7
    bne     .Lstringcopy_unaligned_copy

    .p2align 2
.Lstringcopy_mainloop:
    ldmia   r1!, {r2, r3}

    pld     [r1, #64]

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_mainloop

.Lstringcopy_zero_in_first_register:
    lsls    lr, ip, #17
    itt     ne
    strbne  r2, [r0]
    m_ret   inst=popne
    itt     cs
#if defined(STPCPY)
    strhcs  r2, [r0], #1
#else
    strhcs  r2, [r0]
#endif
    m_ret   inst=popcs
    lsls    ip, ip, #1
    itt     eq
#if defined(STPCPY)
    streq   r2, [r0], #3
#else
    streq   r2, [r0]
#endif
    m_ret   inst=popeq
    strh    r2, [r0], #2
    lsr     r3, r2, #16
    strb    r3, [r0]
    m_ret   inst=pop

.Lstringcopy_zero_in_second_register:
    lsls    lr, ip, #17
    ittt    ne
    stmiane r0!, {r2}
    strbne  r3, [r0]
    m_ret   inst=popne
    ittt    cs
    strcs   r2, [r0], #4
#if defined(STPCPY)
    strhcs  r3, [r0], #1
#else
    strhcs  r3, [r0]
#endif
    m_ret   inst=popcs
    lsls    ip, ip, #1
#if defined(STPCPY)
    ittt    eq
#else
    itt     eq
#endif
    stmiaeq r0, {r2, r3}
#if defined(STPCPY)
    addeq   r0, r0, #7
#endif
    m_ret   inst=popeq
    stmia   r0!, {r2}
    strh    r3, [r0], #2
    lsr     r4, r3, #16
    strb    r4, [r0]
    m_ret   inst=pop

.Lstringcopy_align_dst:
    // Align to a double word (64 bits).
    rsb     r3, r3, #8
    lsls    ip, r3, #31
    beq     .Lstringcopy_align_to_32

    ldrb    r2, [r1], #1
    strb    r2, [r0], #1
    cbz     r2, .Lstringcopy_complete

.Lstringcopy_align_to_32:
    bcc     .Lstringcopy_align_to_64

    ldrb    r4, [r1], #1
    strb    r4, [r0], #1
    cmp     r4, #0
#if defined(STPCPY)
    itt     eq
    subeq   r0, r0, #1
#else
    it      eq
#endif
    m_ret   inst=popeq
    ldrb    r5, [r1], #1
    strb    r5, [r0], #1
    cmp     r5, #0
#if defined(STPCPY)
    itt     eq
    subeq   r0, r0, #1
#else
    it      eq
#endif
    m_ret   inst=popeq

.Lstringcopy_align_to_64:
    tst     r3, #4
    beq     .Lstringcopy_check_src_align
    // Read one byte at a time since we don't have any idea about the alignment
    // of the source and we don't want to read into a different page.
    ldrb    r2, [r1], #1
    strb    r2, [r0], #1
    cbz     r2, .Lstringcopy_complete
    ldrb    r2, [r1], #1
    strb    r2, [r0], #1
    cbz     r2, .Lstringcopy_complete
    ldrb    r2, [r1], #1
    strb    r2, [r0], #1
    cbz     r2, .Lstringcopy_complete
    ldrb    r2, [r1], #1
    strb    r2, [r0], #1
    cbz     r2, .Lstringcopy_complete
    b       .Lstringcopy_check_src_align

.Lstringcopy_complete:
#if defined(STPCPY)
    sub     r0, r0, #1
#endif
    m_ret   inst=pop

.Lstringcopy_unaligned_copy:
    // Dst is aligned to a double word, while src is at an unknown alignment.
    // There are 7 different versions of the unaligned copy code
    // to prevent overreading the src. The mainloop of every single version
    // will store 64 bits per loop. The difference is how much of src can
    // be read without potentially crossing a page boundary.
    tbb     [pc, r3]
.Lstringcopy_unaligned_branchtable:
    .byte 0
    .byte ((.Lstringcopy_unalign7 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign6 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign5 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign4 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign3 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign2 - .Lstringcopy_unaligned_branchtable)/2)
    .byte ((.Lstringcopy_unalign1 - .Lstringcopy_unaligned_branchtable)/2)

    .p2align 2
    // Can read 7 bytes before possibly crossing a page.
.Lstringcopy_unalign7:
    ldr     r2, [r1], #4

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    ldrb    r3, [r1]
    cbz     r3, .Lstringcopy_unalign7_copy5bytes
    ldrb    r4, [r1, #1]
    cbz     r4, .Lstringcopy_unalign7_copy6bytes
    ldrb    r5, [r1, #2]
    cbz     r5, .Lstringcopy_unalign7_copy7bytes

    ldr     r3, [r1], #4
    pld     [r1, #64]

    lsrs    ip, r3, #24
    stmia   r0!, {r2, r3}
#if defined(STPCPY)
    beq     .Lstringcopy_finish
#else
    beq     .Lstringcopy_unalign_return
#endif
    b       .Lstringcopy_unalign7

.Lstringcopy_unalign7_copy5bytes:
    stmia   r0!, {r2}
    strb    r3, [r0]
.Lstringcopy_unalign_return:
    m_ret   inst=pop

.Lstringcopy_unalign7_copy6bytes:
    stmia   r0!, {r2}
    strb    r3, [r0], #1
    strb    r4, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign7_copy7bytes:
    stmia   r0!, {r2}
    strb    r3, [r0], #1
    strb    r4, [r0], #1
    strb    r5, [r0]
    m_ret   inst=pop

    .p2align 2
    // Can read 6 bytes before possibly crossing a page.
.Lstringcopy_unalign6:
    ldr     r2, [r1], #4

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    ldrb    r4, [r1]
    cbz     r4, .Lstringcopy_unalign_copy5bytes
    ldrb    r5, [r1, #1]
    cbz     r5, .Lstringcopy_unalign_copy6bytes

    ldr     r3, [r1], #4
    pld     [r1, #64]

    tst     r3, #0xff0000
    beq     .Lstringcopy_unalign6_copy7bytes
    lsrs    ip, r3, #24
    stmia   r0!, {r2, r3}
#if defined(STPCPY)
    beq     .Lstringcopy_finish
#else
    beq     .Lstringcopy_unalign_return
#endif
    b       .Lstringcopy_unalign6

.Lstringcopy_unalign6_copy7bytes:
    stmia   r0!, {r2}
    strh    r3, [r0], #2
    lsr     r3, #16
    strb    r3, [r0]
    m_ret   inst=pop

    .p2align 2
    // Can read 5 bytes before possibly crossing a page.
.Lstringcopy_unalign5:
    ldr     r2, [r1], #4

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    ldrb    r4, [r1]
    cbz     r4, .Lstringcopy_unalign_copy5bytes

    ldr     r3, [r1], #4

    pld     [r1, #64]

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_unalign5

.Lstringcopy_unalign_copy5bytes:
    stmia   r0!, {r2}
    strb    r4, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign_copy6bytes:
    stmia   r0!, {r2}
    strb    r4, [r0], #1
    strb    r5, [r0]
    m_ret   inst=pop

    .p2align 2
    // Can read 4 bytes before possibly crossing a page.
.Lstringcopy_unalign4:
    ldmia   r1!, {r2}

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    ldmia   r1!, {r3}
    pld     [r1, #64]

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_unalign4

    .p2align 2
    // Can read 3 bytes before possibly crossing a page.
.Lstringcopy_unalign3:
    ldrb    r2, [r1]
    cbz     r2, .Lstringcopy_unalign3_copy1byte
    ldrb    r3, [r1, #1]
    cbz     r3, .Lstringcopy_unalign3_copy2bytes
    ldrb    r4, [r1, #2]
    cbz     r4, .Lstringcopy_unalign3_copy3bytes

    ldr     r2, [r1], #4
    ldr     r3, [r1], #4

    pld     [r1, #64]

    lsrs    lr, r2, #24
    beq     .Lstringcopy_unalign_copy4bytes

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_unalign3

.Lstringcopy_unalign3_copy1byte:
    strb    r2, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign3_copy2bytes:
    strb    r2, [r0], #1
    strb    r3, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign3_copy3bytes:
    strb    r2, [r0], #1
    strb    r3, [r0], #1
    strb    r4, [r0]
    m_ret   inst=pop

    .p2align 2
    // Can read 2 bytes before possibly crossing a page.
.Lstringcopy_unalign2:
    ldrb    r2, [r1]
    cbz     r2, .Lstringcopy_unalign_copy1byte
    ldrb    r3, [r1, #1]
    cbz     r3, .Lstringcopy_unalign_copy2bytes

    ldr     r2, [r1], #4
    ldr     r3, [r1], #4
    pld     [r1, #64]

    tst     r2, #0xff0000
    beq     .Lstringcopy_unalign_copy3bytes
    lsrs    ip, r2, #24
    beq     .Lstringcopy_unalign_copy4bytes

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_unalign2

    .p2align 2
    // Can read 1 byte before possibly crossing a page.
.Lstringcopy_unalign1:
    ldrb    r2, [r1]
    cbz     r2, .Lstringcopy_unalign_copy1byte

    ldr     r2, [r1], #4
    ldr     r3, [r1], #4

    pld     [r1, #64]

    sub     ip, r2, #0x01010101
    bic     ip, ip, r2
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_first_register

    sub     ip, r3, #0x01010101
    bic     ip, ip, r3
    ands    ip, ip, #0x80808080
    bne     .Lstringcopy_zero_in_second_register

    stmia   r0!, {r2, r3}
    b       .Lstringcopy_unalign1

.Lstringcopy_unalign_copy1byte:
    strb    r2, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign_copy2bytes:
    strb    r2, [r0], #1
    strb    r3, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign_copy3bytes:
    strh    r2, [r0], #2
    lsr     r2, #16
    strb    r2, [r0]
    m_ret   inst=pop

.Lstringcopy_unalign_copy4bytes:
    stmia   r0, {r2}
#if defined(STPCPY)
    add     r0, r0, #3
#endif
    m_ret   inst=pop
#if defined(STPCPY)
END(stpcpy)
#else
END(strcpy)
#endif
#endif