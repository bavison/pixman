/*
 * Copyright © 2015 RISC OS Open Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:  Ben Avison (bavison@riscosopen.org)
 */

/* Prevent the stack from becoming executable for no reason... */
#if defined(__linux__) && defined (__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

.text
.fpu neon
.arch armv7a
.object_arch armv4
.eabi_attribute 10, 0
.eabi_attribute 12, 0
.arm
.altmacro
.p2align 2

#include "pixman-private.h"
#include "pixman-arm-asm.h"
#include "pixman-arm-neon-asm.h"
#include "pixman-arm-neon-asm-bilinear.h"

.macro init_x8r8g8b8_a8r8g8b8
        vpush       {d14-d15}
        vmov.u32    q7, #0xff000000
.endm

.macro final_x8r8g8b8_a8r8g8b8
        vpop        {d14-d15}
.endm

.macro convert_x8r8g8b8_a8r8g8b8_8pix_internal  out, in
        vorr.i32    q&out, q&in, q7
.endm

.macro convert_x8r8g8b8_a8r8g8b8_8pix  out, in
        convert_x8r8g8b8_a8r8g8b8_8pix_internal  %((out) / 2), %((in) / 2)
        convert_x8r8g8b8_a8r8g8b8_8pix_internal  %((out) / 2 + 1), %((in) / 2 + 1)
.endm

.macro convert_r5g6b5_a8r8g8b8_8pix_internal  b, g, r, a, in
        vshrn.i16   d&r, q&in, #8    @ rrrrrggg
        vshrn.i16   d&g, q&in, #3    @ ggggggbb
        vsli.16     q&in, q&in, #5   @ ggggggbbbbbbbbbb
        vsri.8      d&r, d&r, #5     @ rrrrrrrr
        vsri.8      d&g, d&g, #6     @ gggggggg
        vmov.i8     d&a, #255
        vshrn.i16   d&b, q&in, #2    @ bbbbbbbb
.endm

.macro convert_r5g6b5_a8r8g8b8_8pix  out, in
        convert_r5g6b5_a8r8g8b8_8pix_internal  %(out), %((out)+1), %((out)+2), %((out)+3), %((in)/2)
.endm

.macro my_vzip8q  a, b
        vzip.8      q&a, q&b
.endm

.macro pack_planar  base0, base1
 .rept 2
        my_vzip8q   %((base0)/2), %((base0)/2 + 1)
  .ifnc "base1",""
        my_vzip8q   %((base1)/2), %((base1)/2 + 1)
  .endif
 .endr
.endm

.macro my_vshll  qd, dm, imm
        vshll.i&imm  q&qd, d&dm, #imm
.endm

.macro convert_a8_a8r8g8b8_8pix_step1  out, in
        my_vshll  %((out)/2+1), %(in), 8
.endm

.macro convert_a8_a8r8g8b8_8pix_step2  base0, base1
        my_vshll  %((base0)/2), %((base0)+2), 16
        my_vshll  %((base0)/2+1), %((base0)+3), 16
 .ifnc "base1",""
        my_vshll  %((base1)/2), %((base1)+2), 16
        my_vshll  %((base1)/2+1), %((base1)+3), 16
 .endif
.endm

generate_bilinear_scaled_cover_functions \
    32, a8r8g8b8, 0, 4, 4, 4, 4, 4, 4, 4, 4

generate_bilinear_scaled_cover_functions \
    32, x8r8g8b8, 0, 4, 4, 4, 4, 4, 4, 4, 4, convert_x8r8g8b8_a8r8g8b8_8pix, \
    nop_macro, init_x8r8g8b8_a8r8g8b8, final_x8r8g8b8_a8r8g8b8

generate_bilinear_scaled_cover_functions \
    16, r5g6b5, 1, 4, 4, 4, 4, 4, 4, 4, 4, convert_r5g6b5_a8r8g8b8_8pix, pack_planar

generate_bilinear_scaled_cover_functions \
    8, a8, 0, 4, 4, 4, 4, 4, 4, 4, 4, convert_a8_a8r8g8b8_8pix_step1, convert_a8_a8r8g8b8_8pix_step2

/* void
 * pixman_get_scanline_bilinear_scaled_cover_pass2_asm_neon (
 *                                               uint32_t  width,
 *                                               uint32_t *dest,
 *                                               int16_t  *source,
 *                                               int16_t   dist_y)
 *
 * This version is used when the output scanline falls between two
 * different input scanlines.
 *
 * Unlike the ARMv6 version, no preload is needed here because ARMv7 CPUs can
 * be expected to do write-allocate in pass 1.
 */
pixman_asm_function pixman_get_scanline_bilinear_scaled_cover_pass2_asm_neon
COUNT   .req    a1
DST     .req    a2
SRC     .req    a3
SRCB    .req    a4
        @ Key for fixed-point representation in comments:
        @ s = sign bit
        @ u = unit bit
        @ f = fractional bit
        @ Examples given for BILINEAR_INTERPOLATION_BITS = 7 or 4
        vmov.16     d0[0], a4       @ 0fffffff00000000 or 0ffff00000000000
        subs        COUNT, COUNT, #8
        bcc         40f

81:     /* Block of 8 pixels */
        vldmia      SRC!, {d16-d31} @ 0uuuuuuuufffffff or 0000uuuuuuuuffff
        vsub.i16    q12, q12, q8    @ suuuuuuuufffffff or ssssuuuuuuuuffff
        vsub.i16    q13, q13, q9
        vsub.i16    q14, q14, q10
        vsub.i16    q15, q15, q11
        vqdmulh.s16 q12, q12, d0[0] @ suuuuuuuufffffff or ssssuuuuuuuuffff
        vqdmulh.s16 q13, q13, d0[0] @ note that the discarded lower bits
        vqdmulh.s16 q14, q14, d0[0] @ can't cause a carry into the unit bits
        vqdmulh.s16 q15, q15, d0[0] @ during the following addition
        vadd.i16    q8, q8, q12     @ 0uuuuuuuufffffff or 0000uuuuuuuuffff
        vadd.i16    q9, q9, q13
        vadd.i16    q10, q10, q14
        vadd.i16    q11, q11, q15
        vshrn.i16   d16, q8, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d17, q9, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d18, q10, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d19, q11, #BILINEAR_INTERPOLATION_BITS
        vstmia      DST!, {d16-d19}

        subs        COUNT, COUNT, #8
        bcs         81b

40:     add         SRCB, SRC, #8*4*2
        movs        COUNT, COUNT, lsl #30
        bcc         20f

        /* Block of 4 pixels */
        vldmia      SRC!, {d16-d19}
        vldmia      SRCB!, {d24-d27}
        vsub.i16    q12, q12, q8
        vsub.i16    q13, q13, q9
        vqdmulh.s16 q12, q12, d0[0]
        vqdmulh.s16 q13, q13, d0[0]
        vadd.i16    q8, q8, q12
        vadd.i16    q9, q9, q13
        vshrn.i16   d16, q8, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d17, q9, #BILINEAR_INTERPOLATION_BITS
        vstmia      DST!, {d16-d17}

20:     bxeq        lr
        movs        COUNT, COUNT, lsl #1

        /* Do blocks of 2 and 1 pixels together due to read-after-write hazards */
        vldmiacs    SRC!, {d20-d21}
        vldmiacs    SRCB!, {d28-d29}
        vldmiami    SRC!, {d22}
        vldmiami    SRCB!, {d30}
        vsub.i16    q14, q14, q10
        vsub.i16    q15, q15, q11
        vqdmulh.s16 q14, q14, d0[0]
        vqdmulh.s16 q15, q15, d0[0]
        vadd.i16    q10, q10, q14
        vadd.i16    q11, q11, q15
        vshrn.i16   d18, q10, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d19, q11, #BILINEAR_INTERPOLATION_BITS
        vstmiacs    DST!, {d18}
        bxpl        lr
        vst1.32     {d19[0]}, [DST]!
        bx          lr

.unreq  COUNT
.unreq  SRCB
.unreq  DST
.unreq  SRC
.endfunc

/* void
 * pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_neon (
 *                                               uint32_t  width,
 *                                               uint32_t *dest,
 *                                               int16_t  *source)
 *
 * This version is used when the output scanline coincides
 * exactly with an input scanline.
 *
 * Unlike the ARMv6 version, no preload is needed here because ARMv7 CPUs can
 * be expected to do write-allocate in pass 1.
 */
pixman_asm_function pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_neon
COUNT   .req    a1
DST     .req    a2
SRC     .req    a3
        subs        COUNT, COUNT, #8
        bcc         40f

81:     /* Block of 8 pixels */
        vldmia      SRC!, {d16-d23}
        vshrn.i16   d16, q8, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d17, q9, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d18, q10, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d19, q11, #BILINEAR_INTERPOLATION_BITS
        add         SRC, SRC, #8*4*2
        vstmia      DST!, {d16-d19}

        subs        COUNT, COUNT, #8
        bcs         81b

40:     movs        COUNT, COUNT, lsl #30
        bcc         20f

        /* Block of 4 pixels */
        vldmia      SRC!, {d16-d19}
        vshrn.i16   d16, q8, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d17, q9, #BILINEAR_INTERPOLATION_BITS
        vstmia      DST!, {d16-d17}

20:     bxeq        lr
        movs        COUNT, COUNT, lsl #1

        /* Do blocks of 2 and 1 pixels together due to read-after-write hazards */
        vldmiacs    SRC!, {d20-d21}
        vldmiami    SRC!, {d22}
        vshrn.i16   d18, q10, #BILINEAR_INTERPOLATION_BITS
        vshrn.i16   d19, q11, #BILINEAR_INTERPOLATION_BITS
        vstmiacs    DST!, {d18}
        bxpl        lr
        vst1.32     {d19[0]}, [DST]!
        bx          lr

.unreq  COUNT
.unreq  DST
.unreq  SRC
.endfunc
