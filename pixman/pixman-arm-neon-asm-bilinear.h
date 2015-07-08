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
