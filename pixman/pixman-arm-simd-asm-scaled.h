/*
 * Copyright © 2014 RISC OS Open Ltd
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Ben Avison (bavison@riscosopen.org)
 */

#include "pixman-private.h" // for BILINEAR_INTERPOLATION_BITS

.set log2_32, 5
.set log2_16, 4
.set log2_8,  3
.set log2_4,  2
.set log2_2,  1
.set log2_1,  0

.macro ldrx  bpp, cond, tail
 .if \bpp == 32
        ldr\cond \tail
 .elseif \bpp == 16
        ldr\cond\()h \tail
 .elseif \bpp == 8
        ldr\cond\()b \tail
 .else
        .error  "Input bits per pixel not supported"
 .endif
.endm

.macro branch  cond, label1, label2
 .ifnc "\label1", ""
        b\cond  \label1
 .else
        b\cond  \label2
 .endif
.endm

.macro nearest_scaled_cover_enlarge_mask_innerloop  bpp, reg, convert, mask_hint, may_be_final, exit_label, store
 .ifnc \mask_hint, mask_is_0
        teq     VALID, #1
  .ifc \convert, nop_macro
   .ifnc \mask_hint, mask_is_non_0
        ittt    ne
        teqne   \reg, #0
   .else
        itt     ne
   .endif
        ldrx    \bpp, ne, <PIXEL, [SRC]>
        movne   VALID, #1
  .else
   .ifnc \mask_hint, mask_is_non_0
        it      ne
        teqne   \reg, #0
   .endif
        beq     1101f
        ldrx    \bpp,, <PIXEL, [SRC]>
        mov     VALID, #1
        \convert PIXEL, TMP
1101:
  .endif
 .endif
        adds    ACCUM, ACCUM, UX
 .ifnc \mask_hint, mask_is_0
        mov     \reg, PIXEL
 .endif
        \store
        branch  cc, \exit_label, 1103f
        add     SRC, SRC, #\bpp/8
        mov     VALID, #0
        tst     SRC, #31
        branch  ne, \exit_label, 1103f
        subs    PLDS, PLDS, #32
        branch  lt, \exit_label, 1103f
        pld     [SRC, #prefetch_distance_src*32]
1103:
.endm

.macro nearest_scaled_cover_enlarge_nomask_innerloop  bpp, reg, convert, mask_hint, may_be_final, exit_label, store
        adds    ACCUM, ACCUM, UX
 .if PIXEL_MERGE_OFFSET == 0
        mov     \reg, PIXEL
 .else
        orr     \reg, \reg, PIXEL, lsl #PIXEL_MERGE_OFFSET
 .endif
 .set PIXEL_MERGE_OFFSET, (PIXEL_MERGE_OFFSET + out_bpp) & 31
        \store
        branch  cc, \exit_label, 1203f
 .ifnc "\may_be_final",""
        teq     COUNT, #0
        ldrx    \bpp, ne, <PIXEL, [SRC, #\bpp/8]!!>
 .else
        ldrx    \bpp,, <PIXEL, [SRC, #\bpp/8]!!>
 .endif
        tst     SRC, #31
        \convert PIXEL, TMP
        branch  ne, \exit_label, 1203f
        subs    PLDS, PLDS, #32
        branch  lt, \exit_label, 1203f
        pld     [SRC, #prefetch_distance_src*32]
1203:
.endm

.macro nearest_scaled_cover_reduce_mask_innerloop  bpp, reg, convert, mask_hint, may_be_final, exit_label, store
        add     PTR, SRC, XHI, lsl #log2_\bpp - 3
        mov     TMP, XHI
        adds    XLO, XLO, UX, lsl #16
        adc     XHI, XHI, UX, lsr #16
 .ifc "\mask_hint",""
        teq     \reg, #0
  .ifnc \convert, nop_macro
        beq     1301f
        ldrx    \bpp,, <\reg, [PTR]>
  .else
        ldrx    \bpp, ne, <\reg, [PTR]>
  .endif
        eor     TMP, TMP, XHI
        bics    TMP, TMP, #255/\bpp
        \convert \reg, TMP
  .ifnc \convert, nop_macro
        b       1302f
1301:   eor     TMP, TMP, XHI
        bics    TMP, TMP, #255/\bpp
1302:
  .endif
 .else
  .ifc \mask_hint, mask_is_non_0
        ldrx    \bpp,, <\reg, [PTR]>
  .endif
        eor     TMP, TMP, XHI
        bics    TMP, TMP, #255/\bpp
  .ifc \mask_hint, mask_is_non_0
        \convert \reg, TMP
  .endif
 .endif
        \store
        branch  eq, \exit_label, 1303f
        subs    PLDS, PLDS, #32
        branch  lt, \exit_label, 1303f
        bic     PTR, PTR, #31   @ base of *previous* cacheline
        pld     [PTR, #(prefetch_distance_src+1)*32]
1303:
.endm

.macro nearest_scaled_cover_reduce_nomask_innerloop  bpp, reg, convert, mask_hint, may_be_final, exit_label, store
        add     PTR, SRC, XHI, lsl #log2_\bpp - 3
        mov     TMP, XHI
        adds    XLO, XLO, UX, lsl #16
        adc     XHI, XHI, UX, lsr #16
 .if PIXEL_MERGE_OFFSET == 0
        ldrx    \bpp,, <\reg, [PTR]>
 .else
        ldrx    \bpp,, <PIXEL2, [PTR]>
 .endif
        eor     TMP, TMP, XHI
        bics    TMP, TMP, #255/\bpp
 .if PIXEL_MERGE_OFFSET == 0
        \convert \reg, TMP
 .else
        \convert PIXEL2, TMP
        orr     \reg, \reg, PIXEL2, lsl #PIXEL_MERGE_OFFSET
 .endif
 .set PIXEL_MERGE_OFFSET, (PIXEL_MERGE_OFFSET + out_bpp) & 31
        \store
        branch  eq, \exit_label, 1403f
        subs    PLDS, PLDS, #32
        branch  lt, \exit_label, 1403f
        bic     PTR, PTR, #31   @ base of *previous* cacheline
        pld     [PTR, #(prefetch_distance_src+1)*32]
1403:
.endm

.macro process1  bpp, has_mask, disable_prefetch, inner_loop, convert
 .if \has_mask
        ldr     WK0, [MASK], #4
  .if !\disable_prefetch
        tst     MASK, #31
        bne     1501f
        pld     [MASK, #prefetch_distance_mask*32]
1501:
  .endif
        teq     WK0, #0
        bne     1502f
        \inner_loop  \bpp, WK0, \convert, mask_is_0, 1, 1503f, <add DST, DST, #4>
        b       1503f
 .endif
 .set PIXEL_MERGE_OFFSET, 0
 .if out_bpp == 32
1502:   \inner_loop  \bpp, WK0, \convert, mask_is_non_0, 1,, <str WK0, [DST], #4>
 .elseif out_bpp == 16
1502:   \inner_loop  \bpp, WK0, \convert, mask_is_non_0, 1,, <strh WK0, [DST], #2>
 .else
        .error  "Output bits per pixel not supported"
 .endif
1503:
.endm

.macro process4  bpp, has_mask, disable_mask_prefetch, inner_loop, convert
 .if \has_mask
        ldmia   MASK!, {WK0-WK3}
  .if !\disable_mask_prefetch
        bic     TMP, MASK, #31
        pld     [TMP, #prefetch_distance_mask*32]
  .endif
        orr     WK0, WK0, WK1
        orr     WK2, WK2, WK3
        orrs    WK0, WK0, WK2
        bne     1601f
        \inner_loop  \bpp, WK0, \convert, mask_is_0
        \inner_loop  \bpp, WK1, \convert, mask_is_0
        \inner_loop  \bpp, WK2, \convert, mask_is_0
        \inner_loop  \bpp, WK3, \convert, mask_is_0, 1, 1602f, <add DST, DST, #4*4>
        b       1602f
 .endif
1601:
 .set PIXEL_MERGE_OFFSET, 0
 .rept 32 / out_bpp
        \inner_loop  \bpp, WK0, \convert
 .endr
 .rept 32 / out_bpp
        \inner_loop  \bpp, WK1, \convert
 .endr
 .rept 32 / out_bpp
        \inner_loop  \bpp, WK2, \convert
 .endr
 .rept 32 / out_bpp - 1
        \inner_loop  \bpp, WK3, \convert
 .endr
        \inner_loop  \bpp, WK3, \convert,, 1,, <stmia DST!!, {WK0,WK1,WK2,WK3}>
1602:
.endm

.macro process  bpp, has_mask, inner_loop, convert
        cmp     COUNT, #2 * 128 / out_bpp - 1 - 1   @ guaranteed at least one aligned half-cacheline output?
        blo     1706f
        tst     DST, #15
        beq     1702f
1701:   process1 \bpp, \has_mask, 0, \inner_loop, \convert
        sub     COUNT, COUNT, #1
        tst     DST, #15
        bne     1701b
1702:   sub     COUNT, COUNT, #128 / out_bpp - 1
 .if \has_mask
        tst     MASK, #16
        beq     1704f
 .endif
1703:
.if \has_mask
        process4  \bpp, \has_mask, 0, \inner_loop, \convert
        subs    COUNT, COUNT, #128 / out_bpp
        bcc     1705f
 .endif
1704:   process4  \bpp, \has_mask, 1, \inner_loop, \convert
        subs    COUNT, COUNT, #128 / out_bpp
        bcs     1703b
1705:   adds    COUNT, COUNT, #128 / out_bpp - 1
        bcc     1707f
        @ drop through...
1706:   process1 \bpp, \has_mask, 1, \inner_loop, \convert
        subs    COUNT, COUNT, #1
        bcs     1706b
1707:   pop     {r4-r11, pc}
.endm

.macro generate_nearest_scaled_cover_function fname, \
                                              bpp, \
                                              prefetch_distance_src_, \
                                              prefetch_distance_mask_, \
                                              init, \
                                              convert, \
                                              out_bpp_

/* void fname(uint32_t width,
 *            pixman_fixed_t x,
 *            pixman_fixed_t ux,
 *            uint32_t *dest,
 *            const uint32_t *source,
 *            const uint32_t *mask);
 */
pixman_asm_function fname

/*
 * Make some macro arguments globally visible and accessible
 * from other macros
 */
 .set prefetch_distance_src,  prefetch_distance_src_
 .set prefetch_distance_mask, prefetch_distance_mask_
 .ifc "out_bpp_",""
  .set out_bpp, 32
 .else
  .set out_bpp, out_bpp_
 .endif

/*
 * Assign symbolic names to registers
 */
COUNT   .req    a1
X       .req    a2
ACCUM   .req    a2  @ enlarge only
XLO     .req    a2  @ reduce only
UX      .req    a3
DST     .req    a4
SRC     .req    v1
MASK    .req    v2  @ only when outputing 32bpp
PIXEL2  .req    v2  @ only when outputing <32bpp and reducing
PLDS    .req    v3
PIXEL   .req    v4  @ enlarge only
XHI     .req    v4  @ reduce only
WK0     .req    v5
WK1     .req    v6
WK2     .req    sl
WK3     .req    fp
VALID   .req    ip  @ enlarge-with-mask only
PTR     .req    ip  @ reduce only
TMP     .req    lr

        mov     ip, sp
        push    {r4-r11, lr}        /* save all registers */
        ldmia   ip, {SRC, MASK}
        subs    COUNT, COUNT, #1
        blo     1807f-4
        \init
        mla     WK2, COUNT, UX, X
 .if out_bpp == 32
        bics    WK0, MASK, #31
        beq     1801f
        @ Use a simplified preload process for the mask,
        @ without a braking distance.
  .set OFFSET, 0
  .rept prefetch_distance_mask + 1
        pld     [WK0, #OFFSET]
   .set OFFSET, OFFSET + 32
  .endr
1801:
 .endif
        add     WK0, SRC, X, lsr #16 - (log2_\bpp - 3)
        bic     WK0, WK0, #31
        pld     [WK0]
        add     WK2, SRC, WK2, lsr #16 - (log2_\bpp - 3)
        bic     WK2, WK2, #31
        add     WK1, WK0, #prefetch_distance_src*32
        subs    PLDS, WK2, WK1
        movcc   WK1, WK2
1802:   add     WK0, WK0, #32
        cmp     WK0, WK1
        bhi     1803f
        pld     [WK0]
        b       1802b
1803:
        cmp     UX, #0x10000
        bhs     1805f
        @ Enlarge
        add     SRC, X, lsr #16 - (log2_\bpp - 3)
        mov     ACCUM, X, lsl #16
        mov     UX, UX, lsl #16
        bic     SRC, SRC, #(\bpp-1)/8
 .if out_bpp == 32
        teq     MASK, #0
        beq     1804f
        mov     VALID, #0
        process \bpp, 1, nearest_scaled_cover_enlarge_mask_innerloop, \convert
1804:
 .endif
        ldrx    \bpp,, <PIXEL, [SRC]>
        \convert PIXEL, TMP
        process \bpp, 0, nearest_scaled_cover_enlarge_nomask_innerloop, \convert

1805:   @ Reduce
        and     TMP, SRC, #31
        bic     SRC, SRC, #31
        mov     XHI, X, lsr #16
        mov     XLO, X, lsl #16
        add     XHI, XHI, TMP, lsr #log2_\bpp - 3
 .if out_bpp == 32
        teq     MASK, #0
        beq     1806f
        process \bpp, 1, nearest_scaled_cover_reduce_mask_innerloop, \convert
 .endif
1806:   process \bpp, 0, nearest_scaled_cover_reduce_nomask_innerloop, \convert
1807:

        .unreq  COUNT
        .unreq  X
        .unreq  ACCUM
        .unreq  XLO
        .unreq  UX
        .unreq  DST
        .unreq  SRC
        .unreq  MASK
        .unreq  PLDS
        .unreq  PIXEL
        .unreq  XHI
        .unreq  WK0
        .unreq  WK1
        .unreq  WK2
        .unreq  WK3
        .unreq  VALID
        .unreq  PTR
        .unreq  TMP
.endfunc
.endm

/******************************************************************************/

.macro bilinear_scaled_cover_process_horizontal  format, factor, in, nin, out, size, exit
        mov     DIST, ACCUM, lsr #32 - BILINEAR_INTERPOLATION_BITS
        sub     AG_OUT\out, AG_IN\nin, AG_IN\in
        sub     RB_OUT\out, RB_IN\nin, RB_IN\in
        mul     AG_OUT\out, AG_OUT\out, DIST
        mul     RB_OUT\out, RB_OUT\out, DIST
        ldr     UX, [sp]
        add     AG_OUT\out, AG_OUT\out, AG_IN\in, lsl #BILINEAR_INTERPOLATION_BITS
        add     RB_OUT\out, RB_OUT\out, RB_IN\in, lsl #BILINEAR_INTERPOLATION_BITS
 .if \size == 1
        stmia   DST!, {AG_OUT0, RB_OUT0}
        subs    COUNT, COUNT, #1
        bmi     .L\format\()_factor\factor\()_\exit
 .elseif \out
        tst     DST, #16
        stmia   DST!, {AG_OUT0, RB_OUT0, AG_OUT1, RB_OUT1}
        addne   DST, DST, #32
        subnes  COUNT, COUNT, #4
        bmi     .L\format\()_factor\factor\()_\exit
 .endif
 .if factor >= 2
        adds    ACCUM, ACCUM, UX
 .else
        subs    ACCUM, ACCUM, UX
 .endif
.endm

.macro bilinear_scaled_cover_innerloop  format, factor, convert, in, nin, out, nout, size, exit, dropthrough
 .if \factor == 0

  .L\format\()_factor\factor\()_\in\out\()_cc:
  .if bpp == 32
        ldr     AG_IN\nin, [SRC, #4]!
  .elseif bpp == 16
        ldrh    AG_IN\nin, [SRC, #2]!
  .else // bpp == 8
        ldrb    AG_IN\nin, [SRC, #1]!
  .endif
        tst     SRC, #31
        bne     .L\format\()_factor\factor\()_\in\out\()_cc_skip
        subs    PLDS, PLDS, #32
        ble     .L\format\()_factor\factor\()_\in\out\()_cc_skip
        pld     [SRC, #prefetch_distance*32]
  .L\format\()_factor\factor\()_\in\out\()_cc_skip:
        \convert  AG_IN\nin, RB_IN\nin
  .L\format\()_factor\factor\()_\in\out\()_cs:
        @ Pass in/nin the opposite way round to allow for negative accumulator
        bilinear_scaled_cover_process_horizontal  \format, \factor, \nin, \in, \out, \size, \exit
        bcs     .L\format\()_factor\factor\()_\in\nout\()_cs
  .ifc "\dropthrough",""
        b       .L\format\()_factor\factor\()_\nin\nout\()_cc
  .endif

 .elseif \factor == 1

  .L\format\()_factor\factor\()_\in\out\()_cs:
  .if bpp == 32
        ldr     AG_IN\nin, [SRC, #4]!
  .elseif bpp == 16
        ldrh    AG_IN\nin, [SRC, #2]!
  .else // bpp == 8
        ldrb    AG_IN\nin, [SRC, #1]!
  .endif
        tst     SRC, #31
  .if \in == 1
        bne     .L\format\()_factor\factor\()_1\out\()_cs_skip
        subs    PLDS, PLDS, #32
        ble     .L\format\()_factor\factor\()_1\out\()_cs_skip
        pld     [SRC, #prefetch_distance*32]
   .L\format\()_factor\factor\()_1\out\()_cs_skip:
        \convert  AG_IN0, RB_IN0
  .else
        bne     .L\format\()_factor\factor\()_0\out\()_converge
        subs    PLDS, PLDS, #32
        ble     .L\format\()_factor\factor\()_0\out\()_converge
        pld     [SRC, #prefetch_distance*32]
        b       .L\format\()_factor\factor\()_0\out\()_converge
   .L\format\()_factor\factor\()_0\out\()_cc:
        orr     TMP, SRC, #31
   .if bpp == 32
        ldmib   SRC!, {AG_IN0, AG_IN1}
   .elseif bpp == 16
        ldrh    AG_IN0, [SRC, #2]!
        ldrh    AG_IN1, [SRC, #2]!
   .else // bpp == 8
        ldrb    AG_IN0, [SRC, #1]!
        ldrb    AG_IN1, [SRC, #1]!
   .endif
        cmp     SRC, TMP
        subgts  PLDS, PLDS, #32
        ble     .L\format\()_factor\factor\()_0\out\()_cc_skip
        pld     [TMP, #1+prefetch_distance*32]
   .L\format\()_factor\factor\()_0\out\()_cc_skip:
        \convert  AG_IN0, RB_IN0
        
   .L\format\()_factor\factor\()_0\out\()_converge:
        \convert  AG_IN1, RB_IN1
  .endif
        @ Pass in/nin the opposite way round to allow for negative accumulator
        bilinear_scaled_cover_process_horizontal  \format, \factor, \nin, \in, \out, \size, \exit
        bcc     .L\format\()_factor\factor\()_0\nout\()_cc
  .ifc "\dropthrough",""
        b       .L\format\()_factor\factor\()_\nin\nout\()_cs
  .endif

 .else // \factor >= 2

  .L\format\()_factor\factor\()_0\out\():
        orr     TMP, SRC, #31
  .if bpp == 32
        ldrcs   AG_IN0, [SRC, #(factor+1)*4]!
        ldrcc   AG_IN0, [SRC, #factor*4]!
        ldrne   AG_IN1, [SRC, #4]
  .elseif bpp == 16
        ldrcsh  AG_IN0, [SRC, #(factor+1)*2]!
        ldrcch  AG_IN0, [SRC, #factor*2]!
        ldrneh  AG_IN1, [SRC, #2]
  .else // bpp == 8
        ldrcsb  AG_IN0, [SRC, #factor+1]!
        ldrccb  AG_IN0, [SRC, #factor]!
        ldrneb  AG_IN1, [SRC, #1]
  .endif
        cmp     SRC, TMP
        subgts  PLDS, PLDS, #32
        ble     .L\format\()_factor\factor\()_0\out\()_skip
        pld     [TMP, #1+prefetch_distance*32]
  .L\format\()_factor\factor\()_0\out\()_skip:
        \convert  AG_IN0, RB_IN0
        \convert  AG_IN1, RB_IN1
        bilinear_scaled_cover_process_horizontal  \format, \factor, \in, \nin, \out, \size, \exit
  .ifc "\dropthrough",""
        b       .L\format\()_factor\factor\()_0\nout\()
  .endif

 .endif
.endm

.macro generate_bilinear_scaled_cover_function fname, \
                                               bpp_, \
                                               format, \
                                               factor, \
                                               prefetch_distance_, \
                                               init, \
                                               convert

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
X       .req    a2
ACCUM   .req    a2
UX      .req    a3
DIST    .req    a3
TMP     .req    a3
DST     .req    a4
AG_IN0  .req    v1
RB_IN0  .req    v2
AG_IN1  .req    v3
RB_IN1  .req    v4
AG_OUT  .req    v5
RB_OUT  .req    v6
AG_OUT0 .req    v5
RB_OUT0 .req    v6
AG_OUT1 .req    v7
RB_OUT1 .req    v8
SRC     .req    ip
PLDS    .req    lr

        push    {v1-v8,lr}
        subs    COUNT, COUNT, #1
        bmi     99f

        mla     v1, UX, COUNT, X        @ v1 = X for final pixel (memory pipeline still busy with push)
        ldr     v2, [sp, #9*4]          @ get source from stack
        add     SRC, v2, X, lsr #16 - (log2_\bpp_ - 3)
        bic     v3, SRC, #31
        add     v2, v2, v1, lsr #16 - (log2_\bpp_ - 3)
        pld     [v3]
        bic     SRC, SRC, #bpp/8-1
 .if \factor >= 2
        @ In these cases we point at the 2nd input sample when we enter the main loop
        add     v1, SRC, #bpp/8
        bic     v1, v1, #31
        add     v1, v1, #prefetch_distance*32
 .else
        add     v1, v3, #prefetch_distance*32
 .endif
        add     v2, v2, #bpp/8           @ v2 -> second input sample for final pixel
        subs    PLDS, v2, v1
        movcc   v1, v2
2:      add     v3, v3, #32
        cmp     v3, v1
        bhi     3f
        pld     [v3]
        b       2b
3:      @ Add 1 to PLDS so that subs PLDS,PLDS,#32 sets GT whenever a preload is to be done
        add     PLDS, PLDS, #1
        mov     UX, UX, lsl #16
        movs    ACCUM, X, lsl #16
        push    {UX}
        \init

 .if \factor >= 2
        @ The accumulator runs the "obvious" way when 1 or 2 unique input samples
        @ are required for each output sample. Here, we can use a free test of
        @ whether the fractional part of the accumulator is 0 to decide if the
        @ second input sample isn't actually used, to avoid array bounds errors.
  .if bpp == 32
        ldr     AG_IN0, [SRC]
        ldrne   AG_IN1, [SRC, #4]
  .elseif bpp == 16
        ldrh    AG_IN0, [SRC]
        ldrneh  AG_IN1, [SRC, #2]
  .else // bpp == 8
        ldrb    AG_IN0, [SRC]
        ldrneb  AG_IN1, [SRC, #1]
  .endif
        \convert  AG_IN0, RB_IN0
        \convert  AG_IN1, RB_IN1
        subs    COUNT, COUNT, #4-1
        bmi     .L\format\()_factor\factor\()_narrow
        bilinear_scaled_cover_process_horizontal  \format, \factor, 0, 1, 0, 4, unused
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, 1, 0, 4, trailing, dropthrough
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, 0, 1, 4, trailing
  .L\format\()_factor\factor\()_narrow:
        add     COUNT, COUNT, #4-1
        bilinear_scaled_cover_process_horizontal  \format, \factor, 0, 1, , 1, done
        b       .L\format\()_factor\factor\()_0
  .L\format\()_factor\factor\()_trailing:
        adds    COUNT, COUNT, #4-1
        bmi     98f
        adds    ACCUM, ACCUM, UX
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, , , 1, done
 .else
        @ When the second input sample may be needed for a subsequent output
        @ sample, we can't use the fractional part of the current accumulator to
        @ filter out unnecessary loads. So run the accumulator in reverse (with
        @ an offset to allow for rounding working in the opposite direction).
        @ This means that when only one input sample is used (because the
        @ fractional accumulator is zero), the unused input buffer represents
        @ the pixel to its left rather than its right; it's simpler to avoid
        @ loading this for the first pixel on each row than inside the inner loop.
        mov     AG_IN0, #-1 >> BILINEAR_INTERPOLATION_BITS
        subs    ACCUM, AG_IN0, ACCUM
        subcs   SRC, SRC, #bpp/8
        bcs     .L\format\()_factor\factor\()_skip_first
  .if bpp == 32
        ldr     AG_IN0, [SRC]
  .elseif bpp == 16
        ldrh    AG_IN0, [SRC]
  .else // bpp == 8
        ldrb    AG_IN0, [SRC]
  .endif
        \convert  AG_IN0, RB_IN0
  .L\format\()_factor\factor\()_skip_first:
        subs    COUNT, COUNT, #4-1
        addmi   COUNT, COUNT, #4-1
  .if \factor == 0
        bmi     .L\format\()_factor\factor\()_0_cc
  .else @ \factor == 1
        bmi     .L\format\()_factor\factor\()_0_cs
  .endif
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, 0, 1, 4, 0_trailing, dropthrough
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 1, 0, 1, 0, 4, 1_trailing
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, 1, 0, 4, 0_trailing, dropthrough
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 1, 0, 0, 1, 4, 1_trailing
  .L\format\()_factor\factor\()_0_trailing:
        adds    COUNT, COUNT, #4-1
        bmi     98f
        subs    ACCUM, ACCUM, UX
  .if \factor == 0
        bcs     .L\format\()_factor\factor\()_0_cs
  .else @ \factor == 1
        bcc     .L\format\()_factor\factor\()_0_cc
  .endif
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 1, 0, , , 1, done

  .L\format\()_factor\factor\()_1_trailing:
        adds    COUNT, COUNT, #4-1
        bmi     98f
        subs    ACCUM, ACCUM, UX
  .if \factor == 0
        bcs     .L\format\()_factor\factor\()_1_cs
  .else @ \factor == 1
        bcc     .L\format\()_factor\factor\()_0_cc
  .endif
        bilinear_scaled_cover_innerloop  \format, \factor, \convert, 0, 1, , , 1, done
 .endif
 .L\format\()_factor\factor\()_done:
98:     pop     {UX,v1-v8,pc}
99:     pop     {v1-v8,pc}

.unreq  COUNT
.unreq  X
.unreq  ACCUM
.unreq  UX
.unreq  DIST
.unreq  TMP
.unreq  DST
.unreq  AG_IN0
.unreq  RB_IN0
.unreq  AG_IN1
.unreq  RB_IN1
.unreq  AG_OUT0
.unreq  RB_OUT0
.unreq  AG_OUT1
.unreq  RB_OUT1
.unreq  SRC
.unreq  PLDS
.endfunc
.endm

.macro generate_bilinear_scaled_cover_functions bpp, \
                                                format, \
                                                pd0, pd1, pd2, pd3, pd4, pd5, pd6, pd7, \
                                                init, \
                                                convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor0_asm_armv6, \
    \bpp, \format, 0, \pd0, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor1_asm_armv6, \
    \bpp, \format, 1, \pd1, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor2_asm_armv6, \
    \bpp, \format, 2, \pd2, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor3_asm_armv6, \
    \bpp, \format, 3, \pd3, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor4_asm_armv6, \
    \bpp, \format, 4, \pd4, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor5_asm_armv6, \
    \bpp, \format, 5, \pd5, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor6_asm_armv6, \
    \bpp, \format, 6, \pd6, \init, \convert
generate_bilinear_scaled_cover_function \
    pixman_get_scanline_bilinear_scaled_cover_pass1_\format\()_factor7_asm_armv6, \
    \bpp, \format, 7, \pd7, \init, \convert
.endm

/******************************************************************************/

.macro nop_macro x:vararg
.endm
