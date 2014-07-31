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

.macro nop_macro x:vararg
.endm
