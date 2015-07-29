/*
 * Copyright © 2009 Nokia Corporation
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
 * Author:  Siarhei Siamashka (siarhei.siamashka@nokia.com)
 */

/*
 * Bilinear scaling support code which tries to provide pixel fetching, color
 * format conversion, and interpolation as separate macros which can be used
 * as the basic building blocks for constructing bilinear scanline functions.
 */

.macro bilinear_load_8888 reg1, reg2, tmp
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    vld1.32   {reg1}, [TMP1], STRIDE
    vld1.32   {reg2}, [TMP1]
.endm

.macro bilinear_load_0565 reg1, reg2, tmp
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    vld1.32   {reg2[0]}, [TMP1], STRIDE
    vld1.32   {reg2[1]}, [TMP1]
    convert_four_0565_to_x888_packed reg2, reg1, reg2, tmp
.endm

.macro bilinear_load_and_vertical_interpolate_two_8888 \
                    acc1, acc2, reg1, reg2, reg3, reg4, tmp1, tmp2

    bilinear_load_8888 reg1, reg2, tmp1
    vmull.u8  acc1, reg1, d28
    vmlal.u8  acc1, reg2, d29
    bilinear_load_8888 reg3, reg4, tmp2
    vmull.u8  acc2, reg3, d28
    vmlal.u8  acc2, reg4, d29
.endm

.macro bilinear_load_and_vertical_interpolate_four_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    bilinear_load_and_vertical_interpolate_two_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi
    bilinear_load_and_vertical_interpolate_two_8888 \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi
.endm

.macro bilinear_load_and_vertical_interpolate_two_0565 \
                acc1, acc2, reg1, reg2, reg3, reg4, acc2lo, acc2hi

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {acc2lo[0]}, [TMP1], STRIDE
    vld1.32   {acc2hi[0]}, [TMP2], STRIDE
    vld1.32   {acc2lo[1]}, [TMP1]
    vld1.32   {acc2hi[1]}, [TMP2]
    convert_0565_to_x888 acc2, reg3, reg2, reg1
    vzip.u8   reg1, reg3
    vzip.u8   reg2, reg4
    vzip.u8   reg3, reg4
    vzip.u8   reg1, reg2
    vmull.u8  acc1, reg1, d28
    vmlal.u8  acc1, reg2, d29
    vmull.u8  acc2, reg3, d28
    vmlal.u8  acc2, reg4, d29
.endm

.macro bilinear_load_and_vertical_interpolate_four_0565 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {xacc2lo[0]}, [TMP1], STRIDE
    vld1.32   {xacc2hi[0]}, [TMP2], STRIDE
    vld1.32   {xacc2lo[1]}, [TMP1]
    vld1.32   {xacc2hi[1]}, [TMP2]
    convert_0565_to_x888 xacc2, xreg3, xreg2, xreg1
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #1
    mov       TMP2, X, asr #16
    add       X, X, UX
    add       TMP2, TOP, TMP2, asl #1
    vld1.32   {yacc2lo[0]}, [TMP1], STRIDE
    vzip.u8   xreg1, xreg3
    vld1.32   {yacc2hi[0]}, [TMP2], STRIDE
    vzip.u8   xreg2, xreg4
    vld1.32   {yacc2lo[1]}, [TMP1]
    vzip.u8   xreg3, xreg4
    vld1.32   {yacc2hi[1]}, [TMP2]
    vzip.u8   xreg1, xreg2
    convert_0565_to_x888 yacc2, yreg3, yreg2, yreg1
    vmull.u8  xacc1, xreg1, d28
    vzip.u8   yreg1, yreg3
    vmlal.u8  xacc1, xreg2, d29
    vzip.u8   yreg2, yreg4
    vmull.u8  xacc2, xreg3, d28
    vzip.u8   yreg3, yreg4
    vmlal.u8  xacc2, xreg4, d29
    vzip.u8   yreg1, yreg2
    vmull.u8  yacc1, yreg1, d28
    vmlal.u8  yacc1, yreg2, d29
    vmull.u8  yacc2, yreg3, d28
    vmlal.u8  yacc2, yreg4, d29
.endm

.macro bilinear_store_8888 numpix, tmp1, tmp2
.if numpix == 4
    vst1.32   {d0, d1}, [OUT, :128]!
.elseif numpix == 2
    vst1.32   {d0}, [OUT, :64]!
.elseif numpix == 1
    vst1.32   {d0[0]}, [OUT, :32]!
.else
    .error bilinear_store_8888 numpix is unsupported
.endif
.endm

.macro bilinear_store_0565 numpix, tmp1, tmp2
    vuzp.u8 d0, d1
    vuzp.u8 d2, d3
    vuzp.u8 d1, d3
    vuzp.u8 d0, d2
    convert_8888_to_0565 d2, d1, d0, q1, tmp1, tmp2
.if numpix == 4
    vst1.16   {d2}, [OUT, :64]!
.elseif numpix == 2
    vst1.32   {d2[0]}, [OUT, :32]!
.elseif numpix == 1
    vst1.16   {d2[0]}, [OUT, :16]!
.else
    .error bilinear_store_0565 numpix is unsupported
.endif
.endm

/******************************************************************************/

.set log2_32, 5
.set log2_16, 4
.set log2_8,  3
.set log2_4,  2
.set log2_2,  1
.set log2_1,  0

.macro my_vld1  esize, bpp_, start_reg, end_reg, index, base, writeback
 .if esize > 64
        vld1.&bpp_ {d&start_reg-d&end_reg}, [base] writeback
 .elseif esize == 64
        vld1.&bpp_ {d&start_reg}, [base] writeback
 .else
        vld1.&esize {d&start_reg[index]}, [base] writeback
 .endif
.endm

.macro scatter_load_one  start_reg, bit_offset, count, ptr, writeback
 .set start_bit, \start_reg * 64 + \bit_offset
 .set esize, \count * bpp
 .set end_bit, start_bit + esize - 1
        my_vld1     %(esize), %(bpp), %(start_bit / 64), %(end_bit / 64), %(start_bit % 64 / esize), \ptr, \writeback
.endm

.macro scatter_load  start_reg, start_offset, out_pix, ptr1, ptr2, factor, may_be_final
 .if out_pix == 1
        teq         ACCUM, #0
        add         \ptr2, \ptr1, #bpp / 8
        scatter_load_one  \start_reg, \start_offset, 1, \ptr1
        beq         0f
        scatter_load_one  \start_reg, \start_offset + bpp, 1, \ptr2
0:
 .else
  .set pix, 0
  .rept \out_pix / 2
        adds        ACCUM, ACCUM, UX, lsl #16
        addcc       \ptr2, \ptr1, #\factor * bpp / 8
   .if ((pix % 8) == 0) || (((bpp == 32) || ((factor >= 4) && (bpp == 16))) && ((pix % 4) == 0)) || ((factor >= 4) && (bpp == 32) && ((pix % 2) == 0))
        pld         [\ptr1, #prefetch_distance * 64]
   .endif
        addcs       \ptr2, \ptr1, #(\factor + 1) * bpp / 8
        scatter_load_one  \start_reg, \start_offset + 2 * pix * bpp, 2, \ptr1
   .set pix, pix + 1
   .if (pix + 1 == \out_pix) && \may_be_final
        add         \ptr1, \ptr2, #bpp / 8
        scatter_load_one  \start_reg, \start_offset + 2 * pix * bpp, 1, \ptr2
        beq         0f
        scatter_load_one  \start_reg, \start_offset + (2 * pix + 1) * bpp, 1, \ptr1
0:
        adds        ACCUM, ACCUM, UX, lsl #16
        addcc       \ptr1, \ptr2, #\factor * bpp / 8
        addcs       \ptr1, \ptr2, #(\factor + 1) * bpp / 8
   .else
        adds        ACCUM, ACCUM, UX, lsl #16
        addcc       \ptr1, \ptr2, #\factor * bpp / 8
        addcs       \ptr1, \ptr2, #(\factor + 1) * bpp / 8
        scatter_load_one  \start_reg, \start_offset + 2 * pix * bpp, 2, \ptr2
   .endif
   .set pix, pix + 1
  .endr
 .endif
.endm

.macro my_vmla_ddist  dest, vect, idx
        vmla.s16    d&dest, d&vect, DDIST[idx]
.endm

.macro interpolate_horizontal  pix, llo, lhi, rlo, rhi, hi_aligned
 .if hi_aligned
  .if (pix > 2)
        vshll.u8    q8, d&rlo, #BILINEAR_INTERPOLATION_BITS
        vsubl.u8    q10, d&llo, d&rlo
  .endif
        vshll.u8    q9, d&rhi, #BILINEAR_INTERPOLATION_BITS
        vsubl.u8    q11, d&lhi, d&rhi
 .else
        vshll.u8    q8, d&llo, #BILINEAR_INTERPOLATION_BITS
        vsubl.u8    q10, d&rlo, d&llo
  .if (pix > 2)
        vshll.u8    q9, d&lhi, #BILINEAR_INTERPOLATION_BITS
        vsubl.u8    q11, d&rhi, d&lhi
  .endif
 .endif
 .if pix == 4
        vmla.i16    DACCUM, DUX, D4321[3]
 .elseif (pix == 3) && hi_aligned
        vmla.i16    DACCUM, DUX, D4321[2]
 .elseif (pix == 2) && hi_aligned
        vmla.i16    DACCUM, DUX, D4321[1]
 .elseif (pix == 1) && hi_aligned
        vmla.i16    DACCUM, DUX, D4321[0]
 .endif
 .set N, 0
 .rept pix
 .set M, N + hi_aligned * (4 - pix)
        my_vmla_ddist  %(16+M), %(20+M), N
  .set N, N+1
 .endr
 .if (pix == 4) || hi_aligned
        vshr.u16    DDIST, DACCUM, #16 - BILINEAR_INTERPOLATION_BITS
 .endif
.endm

.macro generate_bilinear_scaled_cover_function_reduce fname, \
                                                      bpp_, \
                                                      format, \
                                                      factor, \
                                                      prefetch_distance_, \
                                                      convert, \
                                                      pack, \
                                                      init, \
                                                      final

0:      .word       0x20001
        .word       0x40003

/* void fname(uint32_t       width,
 *            pixman_fixed_t x,
 *            pixman_fixed_t ux,
 *            uint32_t      *dest,
 *            const void    *source);
 */
pixman_asm_function fname

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
 .set bpp, \bpp_
 .set prefetch_distance, \prefetch_distance_

/*
 * Assign symbolic names to registers
 */
COUNT   .req    a1
ACCUM   .req    a2
UX      .req    a3
DST     .req    a4
SRCA    .req    v1
SRCB    .req    ip
D4321   .req    d0
DACCUM  .req    d1
DUX     .req    d2
DDIST   .req    d3

        push        {v1,lr}

        ldr         SRCA, [sp, #2*4]
        add         SRCA, SRCA, ACCUM, lsr #16 - (log2_\bpp_ - 3)
 .if bpp > 8
        bic         SRCA, SRCA, #(bpp - 1) / 8
 .endif
 .set off, 0
 .rept prefetch_distance
        pld         [SRCA, #off * 64]
  .set off, off + 1
 .endr

        init

        sub         lr, ACCUM, UX
        vldr.i64    D4321, 0b
        vdup.i16    DUX, UX
        vdup.i16    DACCUM, lr
        vmla.i16    DACCUM, D4321, DUX
        mov         ACCUM, ACCUM, lsl #16
        subs        COUNT, COUNT, #16
        vshr.u16    DDIST, DACCUM, #16 - BILINEAR_INTERPOLATION_BITS
        bcc         20f
10:
 .ifnc "\convert",""
        scatter_load  16, 0, 8, SRCA, SRCB, \factor, 0
        \convert    24, 16
        \convert    28, 16 + bpp / 8
        \pack       24, 28
 .else
        scatter_load  24, 0, 8, SRCA, SRCB, \factor, 0
 .endif
        vuzp.32     q12, q13
        vuzp.32     q14, q15
        interpolate_horizontal  4, 24, 25, 26, 27, 0
        vstmia      DST!, {d16-d19}
        interpolate_horizontal  4, 28, 29, 30, 31, 0
        vstmia      DST, {d16-d19}
        add         DST, DST, #96

        subs        COUNT, COUNT, #8
        bcs         10b
20:
        adds        COUNT, COUNT, #8
        bcc         30f

 .ifnc "\convert",""
        scatter_load  16, 0, 8, SRCA, SRCB, \factor, 1
        \convert    24, 16
        \convert    28, 16 + bpp / 8
        \pack       24, 28
 .else
        scatter_load  24, 0, 8, SRCA, SRCB, \factor, 1
 .endif
        vuzp.32     q12, q13
        vuzp.32     q14, q15
        interpolate_horizontal  4, 24, 25, 26, 27, 0
        vstmia      DST!, {d16-d19}
        interpolate_horizontal  4, 28, 29, 30, 31, 0
        vstmia      DST, {d16-d19}
        add         DST, DST, #96
30:
        tst         COUNT, #4
        beq         40f
 .ifnc "\convert",""
        scatter_load  16, 0, 4, SRCA, SRCB, \factor, 1
        \convert    24, 16
        \pack       24
 .else
        scatter_load  24, 0, 4, SRCA, SRCB, \factor, 1
 .endif
        vuzp.32     q12, q13
        interpolate_horizontal  4, 24, 25, 26, 27, 0
        vstmia      DST!, {d16-d19}
40:
        ands        COUNT, COUNT, #3
 .ifnc "\final",""
        beq         99f
 .else
        popeq       {v1, pc}
 .endif
        tst         COUNT, #2
        beq         50f
 .ifnc "\convert",""
        scatter_load  16, 4*2*bpp, 2, SRCA, SRCB, \factor, 1
 .else
        scatter_load  24, 4*2*bpp, 2, SRCA, SRCB, \factor, 1
 .endif
50:
        tst         COUNT, #1
        beq         60f
 .ifnc "\convert",""
        scatter_load  16, 6*2*bpp, 1, SRCA, SRCB, \factor, 1
 .else
        scatter_load  24, 6*2*bpp, 1, SRCA, SRCB, \factor, 1
 .endif
60:
 .ifnc "\convert",""
        \convert    28, 16 + bpp / 8
        \pack       28
 .endif
        vuzp.32     q14, q15
        cmp         COUNT, #2
        bhi         80f
        beq         70f
        interpolate_horizontal  1, 29,, 31,, 0
        vstmia      DST, {d16}
        final
        pop         {v1, pc}
70:     interpolate_horizontal  2, 28,, 30,, 0
        vstmia      DST, {d16-d17}
        final
        pop         {v1, pc}
80:     interpolate_horizontal  3, 28, 29, 30, 31, 0
        vstmia      DST, {d16-d18}
99:     final
        pop         {v1, pc}

.unreq   COUNT
.unreq   ACCUM
.unreq   UX
.unreq   DST
.unreq   SRCA
.unreq   SRCB
.unreq   D4321
.unreq   DACCUM
.unreq   DUX
.unreq   DDIST
.endfunc
.endm

.macro generate_bilinear_scaled_cover_functions bpp, \
                                                format, \
                                                pd0, pd1, pd2, pd3, pd4, pd5, pd6, pd7, \
                                                convert, \
                                                pack, \
                                                init, \
                                                final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor0_asm_neon, \
    \bpp, \format, 0, \pd0, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor1_asm_neon, \
    \bpp, \format, 1, \pd1, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor2_asm_neon, \
    \bpp, \format, 2, \pd2, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor3_asm_neon, \
    \bpp, \format, 3, \pd3, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor4_asm_neon, \
    \bpp, \format, 4, \pd4, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor5_asm_neon, \
    \bpp, \format, 5, \pd5, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor6_asm_neon, \
    \bpp, \format, 6, \pd6, \convert, \pack, \init, \final
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor7_asm_neon, \
    \bpp, \format, 7, \pd7, \convert, \pack, \init, \final
.endm
