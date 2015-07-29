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

.macro bilinear_enlarge_label  fname, width_mod_8, leading, in_remain, out_remain
.L&fname&_&width_mod_8&_&leading&_&in_remain&_&out_remain:
.endm

.macro bilinear_enlarge_jump_table_offset  fname, width_mod_8, leading, in_remain, out_remain
        .word       .L&fname&_&width_mod_8&_&leading&_&in_remain&_&out_remain - (0b + 4)
.endm

.macro bilinear_enlarge_branch  cond, fname, width_mod_8, leading, in_remain, out_remain
        b&cond      .L&fname&_&width_mod_8&_&leading&_&in_remain&_&out_remain
.endm

.macro bilinear_enlarge_adr  cond, r, fname, width_mod_8, leading, in_remain, out_remain
        adr&cond&l  r, .L&fname&_&width_mod_8&_&leading&_&in_remain&_&out_remain
.endm

.macro my_vmovd  dd, dm
        vmov        d&dd, d&dm
.endm

.macro my_vext8d  dd, dn, dm, imm
        vext.8      d&dd, d&dn, d&dm, #imm
.endm

.macro my_vstmd  base, dlo, dhi
 .if dlo == dhi
        vstmia      base, {d&dlo}
 .else
        vstmia      base, {d&dlo-d&dhi}
 .endif
.endm

.macro bilinear_enlarge_main_loop  fname, max_out_remain, width_mod_8, leading
 .set in_remain, 9
 .rept 8

  .set out_remain, max_out_remain
  .rept max_out_remain

        bilinear_enlarge_label fname, %(width_mod_8), %(leading), %(in_remain), %(out_remain)
   .if ((in_remain) & 1) == 0
        my_vmovd    %(32-out_remain), %(6-in_remain/2)
   .else
        my_vext8d   %(32-out_remain), %(5-in_remain/2), %(6-in_remain/2), 4
   .endif
   .if out_remain > 1
        subs        ACCUM, ACCUM, UX, lsl #16
    .if in_remain == 2
        bilinear_enlarge_adr  cc, lr, fname, %(width_mod_8), %(leading), 9, %(out_remain - 1)
        bcc         80f
    .else
        bilinear_enlarge_branch  cc, fname, %(width_mod_8), %(leading), %(in_remain - 1), %(out_remain - 1)
    .endif
   .endif

   .set out_remain, out_remain - 1
  .endr

  .if (max_out_remain >= 6)
        vuzp.32     q12, q13
  .elseif (max_out_remain == 5)
        vuzp.32     d25, d27
  .endif
  .if (max_out_remain >= 2)
        vuzp.32     q14, q15
  .elseif (max_out_remain == 1)
        vuzp.32     d29, d31
  .endif
  .if max_out_remain == 8
        interpolate_horizontal  4, 24, 25, 26, 27, 1
   .if width_mod_8 < 4
        my_vstmd    DST!, 16, 19
   .else
    .if width_mod_8 != 4
        add         ip, DST, #96 - (width_mod_8 - 4) * 8
    .endif
        my_vstmd    DST, 16, %(19 - (width_mod_8 - 4))
        add         DST, DST, #96
    .if width_mod_8 != 4
        my_vstmd    ip, %(20 - (width_mod_8 - 4)), 19
    .endif
   .endif
        interpolate_horizontal  4, 28, 29, 30, 31, 1
   .if width_mod_8 >= 4
        my_vstmd    DST!, 16, 19
   .else
    .if width_mod_8 != 0
        add         ip, DST, #96 - width_mod_8 * 8
    .endif
        my_vstmd    DST, 16, %(19 - width_mod_8)
        add         DST, DST, #96
    .if width_mod_8 != 0
        my_vstmd    ip, %(20 - width_mod_8), 19
    .endif
   .endif
  .elseif max_out_remain >= 5
        interpolate_horizontal  (max_out_remain - 4), 24, 25, 26, 27, 1
        my_vstmd    DST!, %(24 - max_out_remain), 19
        interpolate_horizontal  4, 28, 29, 30, 31, 1
        my_vstmd    DST!, 16, 19
  .else
        interpolate_horizontal  max_out_remain, 28, 29, 30, 31, 1
        my_vstmd    DST!, %(20 - max_out_remain), 19
  .endif
        subs        COUNT, COUNT, #8
        bcc         99f
        subs        ACCUM, ACCUM, UX, lsl #16
        bilinear_enlarge_branch  cs, fname, %(width_mod_8), 0, %(in_remain), 8
  .if in_remain == 2
        bilinear_enlarge_adr  , lr, fname, %(width_mod_8), 0, 9, 8
        b           80f
  .elseif max_out_remain < 8
        bilinear_enlarge_branch  , fname, %(width_mod_8), 0, %(in_remain - 1), 8
  .else
        @ Drop through to next iteration of the in_remain loop
  .endif

  .set in_remain, in_remain - 1
 .endr
.endm

.macro generate_bilinear_scaled_cover_function_enlarge fname, \
                                                       bpp_, \
                                                       format, \
                                                       factor, \
                                                       prefetch_distance_, \
                                                       convert, \
                                                       pack, \
                                                       init, \
                                                       final

0:      .word       0xfffeffff
        .word       0xfffcfffd

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
SRC     .req    v1
D4321   .req    d0  @ needs to be in range d0-d7 for vmla.s16
@ second half of d1 and d2-d5 contain 9x32bpp source pixel circular buffer
DDIST   .req    d6  @ needs to be in range d0-d7 for vmla.s16
DACCUM  .req    d7
DUX     .req    d8

        push        {v1,lr}
        vpush       {d8}

        ldr         SRC, [sp, #8 + 2*4]
        add         SRC, SRC, ACCUM, lsr #16 - (log2_\bpp_ - 3)
 .if bpp > 8
        bic         SRC, SRC, #(bpp - 1) / 8
 .endif
 .set off, 0
 .rept prefetch_distance + 1
        pld         [SRC, #off * 64]
  .set off, off + 1
 .endr

        init

        @ Each destination pixel is interpolated from two source pixels, even if
        @ the weight for one of them is zero (due to the fractional part of the
        @ position being 0 within the precision of interpolation). The use of a
        @ negative accumulator means that the source pixel with weight 0 is the
        @ one to the left.
        @ When the leftmost destination pixel is one of this type, we take
        @ special measures to ensure we don't load the leftmost source pixel to
        @ ensure we can't stray beyond array bounds. We need to calculate the
        @ total number of source pixels required, both including and excluding
        @ any such leftmost pixel.
        sub         COUNT, COUNT, #1
        sub         lr, ACCUM, #1 << (16 - BILINEAR_INTERPOLATION_BITS)
        mla         ip, COUNT, UX, lr
        mov         lr, lr, lsr #16
        rsb         lr, lr, ip, lsr #16  @ = number of source pixels (incl) - 2
        mov         ip, #(1 << (32 - BILINEAR_INTERPOLATION_BITS)) - 1
        subs        ACCUM, ip, ACCUM, lsl #16
        @ Now carry is set if we need to load one fewer pixels
        add         ip, ACCUM, UX, lsl #16
        vmov.i32    DACCUM, ip, UX
        vldr.i64    D4321, 0b
        vdup.i16    DUX, DACCUM[2]
        vdup.i16    DACCUM, DACCUM[1]
        and         lr, lr, #7
        rsc         ip, lr, #8
        vmla.i16    DACCUM, D4321, DUX

        ldrb        ip, [pc, ip]
        add         pc, pc, ip
0:      .byte       9f - (0b + 4)
        .byte       8f - (0b + 4)
        .byte       7f - (0b + 4)
        .byte       6f - (0b + 4)
        .byte       5f - (0b + 4)
        .byte       4f - (0b + 4)
        .byte       3f - (0b + 4)
        .byte       2f - (0b + 4)
        .byte       1f - (0b + 4)
        .align
9:
 .ifnc "\convert",""
        scatter_load_one  16, 7*bpp, 1, SRC, !
        \convert    2, 16
        \pack       2
        vmov        d1, d5
 .else
        scatter_load_one  1, 1*bpp, 1, SRC, !
 .endif
        @ drop through
8:
 .ifnc "\convert",""
        scatter_load_one  16, 0*bpp, 8, SRC, !
 .else
        scatter_load_one  2, 0*bpp, 8, SRC, !
 .endif
        b           10f
7:
 .ifnc "\convert",""
        scatter_load_one  16, 1*bpp, 1, SRC, !
 .else
        scatter_load_one  2, 1*bpp, 1, SRC, !
 .endif
        @ drop through
6:
 .ifnc "\convert",""
        scatter_load_one  16, 2*bpp, 2, SRC, !
 .else
        scatter_load_one  2, 2*bpp, 2, SRC, !
 .endif
        @ drop through
4:
 .ifnc "\convert",""
        scatter_load_one  16, 4*bpp, 4, SRC, !
 .else
        scatter_load_one  2, 4*bpp, 4, SRC, !
 .endif
        b           10f
5:
 .ifnc "\convert",""
        scatter_load_one  16, 3*bpp, 1, SRC, !
        scatter_load_one  16, 4*bpp, 4, SRC, !
 .else
        scatter_load_one  2, 3*bpp, 1, SRC, !
        scatter_load_one  2, 4*bpp, 4, SRC, !
 .endif
        b           10f
3:
 .ifnc "\convert",""
        scatter_load_one  16, 5*bpp, 1, SRC, !
 .else
        scatter_load_one  2, 5*bpp, 1, SRC, !
 .endif
        @ drop through
2:
 .ifnc "\convert",""
        scatter_load_one  16, 6*bpp, 2, SRC, !
 .else
        scatter_load_one  2, 6*bpp, 2, SRC, !
 .endif
        b           10f
1:
 .ifnc "\convert",""
        scatter_load_one  16, 7*bpp, 1, SRC, !
 .else
        scatter_load_one  2, 7*bpp, 1, SRC, !
 .endif
        b           10f

10:
 .ifnc "\convert",""
        \convert    2, 16
        \pack       2
 .endif
        orr         lr, lr, COUNT, lsl #3
        and         lr, lr, #0x3f
        vshr.u16    DDIST, DACCUM, #16 - BILINEAR_INTERPOLATION_BITS
        ldr         ip, [pc, lr, lsl #2]
        add         pc, pc, ip
0:
 .set width_mod_8, 1
 .rept 8
  .set in_remain, 2
  .rept 8
   .if width_mod_8 > 0
        bilinear_enlarge_jump_table_offset fname, %(width_mod_8), 1, %(in_remain), %(width_mod_8)
   .else
        bilinear_enlarge_jump_table_offset fname, %(width_mod_8), 0, %(in_remain), 8
   .endif
   .set in_remain, in_remain + 1
  .endr
  .set width_mod_8, (width_mod_8 + 1) % 8
 .endr
 .align

 .set width_mod_8, 0
 .rept 8
  .if width_mod_8 != 0
        @ First handle the leading <8 output pixels
        bilinear_enlarge_main_loop  fname, width_mod_8, width_mod_8, 1
  .endif
        @ Then the remaining blocks of 8 output pixels
        bilinear_enlarge_main_loop  fname, 8, width_mod_8, 0
  .set width_mod_8, width_mod_8 + 1
 .endr

80:     @ Load next 8 source pixels
        pld         [SRC, #prefetch_distance * 64]
        vmov        d1, d5
 .ifnc "\convert",""
        scatter_load_one  16, 0*bpp, 8, SRC, !
        \convert    2, 16
        \pack       2
 .else
        scatter_load_one  2, 0*bpp, 8, SRC, !
 .endif
        bx          lr

99:     @ Clean up and exit
        final
        vpop        {d8}
        pop         {v1,pc}

.unreq   COUNT
.unreq   ACCUM
.unreq   UX
.unreq   DST
.unreq   SRC
.unreq   D4321
.unreq   DACCUM
.unreq   DUX
.unreq   DDIST
.endfunc
.endm

.macro generate_bilinear_scaled_cover_functions bpp, \
                                                format, \
                                                alternate_enlarge_method, \
                                                pd0, pd1, pd2, pd3, pd4, pd5, pd6, pd7, \
                                                convert, \
                                                pack, \
                                                init, \
                                                final
 .if alternate_enlarge_method
generate_bilinear_scaled_cover_function_enlarge \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor0_asm_neon, \
    \bpp, \format, 0, \pd0, \convert, \pack, \init, \final
 .else
generate_bilinear_scaled_cover_function_reduce \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor0_asm_neon, \
    \bpp, \format, 0, \pd0, \convert, \pack, \init, \final
 .endif
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
