/*
 * Copyright © 2008 Mozilla Corporation
 * Copyright © 2010 Nokia Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Mozilla Corporation not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Mozilla Corporation makes no
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
 * Author:  Jeff Muizelaar (jeff@infidigm.net)
 *
 */

/* Prevent the stack from becoming executable */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

	.text
	.arch armv6
	.object_arch armv4
	.arm
	.altmacro
	.p2align 2

#include "pixman-arm-asm.h"
#include "pixman-arm-simd-asm-scaled.h"

/*
 * Note: This code is only using armv5te instructions (not even armv6),
 *       but is scheduled for ARM Cortex-A8 pipeline. So it might need to
 *       be split into a few variants, tuned for each microarchitecture.
 *
 * TODO: In order to get good performance on ARM9/ARM11 cores (which don't
 * have efficient write combining), it needs to be changed to use 16-byte
 * aligned writes using STM instruction.
 *
 * Nearest scanline scaler macro template uses the following arguments:
 *  fname                     - name of the function to generate
 *  bpp_shift                 - (1 << bpp_shift) is the size of pixel in bytes
 *  t                         - type suffix for LDR/STR instructions
 *  prefetch_distance         - prefetch in the source image by that many
 *                              pixels ahead
 *  prefetch_braking_distance - stop prefetching when that many pixels are
 *                              remaining before the end of scanline
 */

.macro generate_nearest_scanline_func fname, bpp_shift, t,      \
                                      prefetch_distance,        \
                                      prefetch_braking_distance

pixman_asm_function fname
	W		.req	r0
	DST		.req	r1
	SRC		.req	r2
	VX		.req	r3
	UNIT_X		.req	ip
	TMP1		.req	r4
	TMP2		.req	r5
	VXMASK		.req	r6
	PF_OFFS		.req	r7
	SRC_WIDTH_FIXED	.req	r8

	ldr	UNIT_X, [sp]
	push	{r4, r5, r6, r7, r8, r10}
	mvn	VXMASK, #((1 << bpp_shift) - 1)
	ldr	SRC_WIDTH_FIXED, [sp, #28]

	/* define helper macro */
	.macro	scale_2_pixels
		ldr&t	TMP1, [SRC, TMP1]
		and	TMP2, VXMASK, VX, asr #(16 - bpp_shift)
		adds	VX, VX, UNIT_X
		str&t	TMP1, [DST], #(1 << bpp_shift)
9:		subpls	VX, VX, SRC_WIDTH_FIXED
		bpl	9b

		ldr&t	TMP2, [SRC, TMP2]
		and	TMP1, VXMASK, VX, asr #(16 - bpp_shift)
		adds	VX, VX, UNIT_X
		str&t	TMP2, [DST], #(1 << bpp_shift)
9:		subpls	VX, VX, SRC_WIDTH_FIXED
		bpl	9b
	.endm

	/* now do the scaling */
	and	TMP1, VXMASK, VX, asr #(16 - bpp_shift)
	adds	VX, VX, UNIT_X
9:	subpls	VX, VX, SRC_WIDTH_FIXED
	bpl	9b
	subs	W, W, #(8 + prefetch_braking_distance)
	blt	2f
	/* calculate prefetch offset */
	mov	PF_OFFS, #prefetch_distance
	mla	PF_OFFS, UNIT_X, PF_OFFS, VX
1:	/* main loop, process 8 pixels per iteration with prefetch */
	pld	[SRC, PF_OFFS, asr #(16 - bpp_shift)]
	add	PF_OFFS, UNIT_X, lsl #3
	scale_2_pixels
	scale_2_pixels
	scale_2_pixels
	scale_2_pixels
	subs	W, W, #8
	bge	1b
2:
	subs	W, W, #(4 - 8 - prefetch_braking_distance)
	blt	2f
1:	/* process the remaining pixels */
	scale_2_pixels
	scale_2_pixels
	subs	W, W, #4
	bge	1b
2:
	tst	W, #2
	beq	2f
	scale_2_pixels
2:
	tst	W, #1
	ldrne&t	TMP1, [SRC, TMP1]
	strne&t	TMP1, [DST]
	/* cleanup helper macro */
	.purgem	scale_2_pixels
	.unreq	DST
	.unreq	SRC
	.unreq	W
	.unreq	VX
	.unreq	UNIT_X
	.unreq	TMP1
	.unreq	TMP2
	.unreq	VXMASK
	.unreq	PF_OFFS
	.unreq  SRC_WIDTH_FIXED
	/* return */
	pop	{r4, r5, r6, r7, r8, r10}
	bx	lr
.endfunc
.endm

generate_nearest_scanline_func \
    pixman_scaled_nearest_scanline_0565_0565_SRC_asm_armv6, 1, h, 80, 32

generate_nearest_scanline_func \
    pixman_scaled_nearest_scanline_8888_8888_SRC_asm_armv6, 2,  , 48, 32

/******************************************************************************/

generate_nearest_scaled_cover_function \
    pixman_get_scanline_nearest_scaled_cover_a8r8g8b8_asm_armv6, 32, \
    3, 3 /* prefetch distances */, nop_macro, nop_macro

.macro convert_x888_8888, reg, tmp
        orr     reg, reg, #0xFF000000
.endm

generate_nearest_scaled_cover_function \
    pixman_get_scanline_nearest_scaled_cover_x8r8g8b8_asm_armv6, 32, \
    2, 3 /* prefetch distances */, nop_macro, convert_x888_8888

generate_nearest_scaled_cover_function \
    pixman_get_scanline_r5g6b5_nearest_scaled_cover_r5g6b5_asm_armv6, 16, \
    2, 0 /* prefetch distances */, nop_macro, nop_macro, 16

.macro init_ge
        msr     CPSR_s, #0x50000
.endm

.macro convert_0565_8888, reg, tmp
        bic     tmp, reg, #0x07e0                  @ 0000000000000000rrrrr000000bbbbb
        and     reg, reg, #0x07e0                  @ 000000000000000000000gggggg00000
        mov     tmp, tmp, lsl #3                   @ 0000000000000rrrrr000000bbbbb000
        mov     reg, reg, lsl #5                   @ 0000000000000000gggggg0000000000
        orr     tmp, tmp, tmp, lsr #5              @ 0000000000000rrrrrrrrrr0bbbbbbbb
        orr     reg, reg, reg, lsr #6              @ 000000000000000gggggggggggg00000
        pkhbt   tmp, tmp, tmp, lsl #5              @ --------rrrrrrrr--------bbbbbbbb
        sel     reg, tmp, reg                      @ --------rrrrrrrrggggggggbbbbbbbb
        orr     reg, reg, #0xFF000000              @ 11111111rrrrrrrrggggggggbbbbbbbb
.endm

generate_nearest_scaled_cover_function \
    pixman_get_scanline_nearest_scaled_cover_r5g6b5_asm_armv6, 16, \
    2, 3 /* prefetch distances */, init_ge, convert_0565_8888

.macro convert_8_8888, reg, tmp
        mov     reg, reg, lsl #24
.endm

generate_nearest_scaled_cover_function \
    pixman_get_scanline_nearest_scaled_cover_a8_asm_armv6, 8, \
    2, 3 /* prefetch distances */, nop_macro, convert_8_8888

/******************************************************************************/

.macro convert_8888_08080808  in_ag, rb
        uxtb16  \rb, \in_ag
        uxtb16  \in_ag, \in_ag, ror #8
.endm

generate_bilinear_scaled_cover_functions  32, a8r8g8b8, 3, 3, 3, 3, 3, 3, 3, 3, nop_macro, convert_8888_08080808

/******************************************************************************/

.macro pass2_1pixel_internal  t0, t1, b0, b1, tmp, mul, d
        pkhtb   \tmp, \t0, \b0, asr #16
        pkhbt   \t0, \b0, \t0, lsl #16
        pkhtb   \b0, \t1, \b1, asr #16
        pkhbt   \t1, \b1, \t1, lsl #16
        smuad   \t0, \t0, \mul
        smuad   \tmp, \tmp, \mul
        smuad   \t1, \t1, \mul
        smuad   \b0, \b0, \mul
        mov     \tmp, \tmp, lsl #8
        pkhtb   \t0, \tmp, \t0, asr #8
        pkhtb   \t1, \b0, \t1, asr #16
        sel     \d, \t1, \t0
.endm

.macro pass2_1pixel  check_src_thresh
        ldrd    v3, v4, [SRC, #32]
        ldrd    v1, v2, [SRC], #8
 .if \check_src_thresh
        tst     SRC, #31
        addeq   SRC, SRC, #32
 .endif
        pass2_1pixel_internal  v1, v2, v3, v4, ip, DIST, v1
        str     v1, [DST], #4
.endm

.macro pass2_preload  n, do_preload, pos
 .ifnc "do_preload",""
  /* Minimum distance is 32 because we read the bottom-row pixels from
   * the cacheline after the one being pointed at */
  .set dist, 32 + prefetch_distance*64
  .if \n == 1 || \n == 2
   /* Stretch one cacheline further ahead in these cases to ensure
    * we stop preloading at boundary between groups of 4 output pixels */
   .set dist, dist + 32
  .endif
  .if \pos == \n
        /* Source pointer has just skipped "bottom" cacheline */
        pld     [SRC, #dist]
  .elseif ((\pos) & 1) == ((\n) & 1)
        /* Time-equidistant between skips, but pointer is half-way through cacheline */
        pld     [SRC, #16 + dist]
  .endif
 .endif
.endm

.macro pass2_4pixels  n, do_preload
        ldrd    v3, v4, [SRC, #32]
        ldrd    v1, v2, [SRC], #8 + 32*-(\n==1)
        pass2_preload  \n, \do_preload, 1
        ldrd    v7, v8, [SRC, #32]
        ldrd    v5, v6, [SRC], #8 + 32*-(\n==2)
        pass2_preload  \n, \do_preload, 2
        pass2_1pixel_internal  v1, v2, v3, v4, ip, DIST, OUT0
        pass2_1pixel_internal  v5, v6, v7, v8, ip, DIST, OUT1
        ldrd    v5, v6, [SRC, #32]
        ldrd    v3, v4, [SRC], #8 + 32*-(\n==3)
        pass2_preload  \n, \do_preload, 3
        ldrd    v7, v8, [SRC, #32]
        ldr     lr, [SRC], #4
        pass2_1pixel_internal  v3, v4, v5, v6, ip, DIST, OUT2
        ldr     v6, [SRC], #4 + 32*-(\n==0)
        pass2_preload  \n, \do_preload, 0
        pass2_1pixel_internal  lr, v6, v7, v8, ip, DIST, OUT3
        stmia   DST!, {OUT0, OUT1, OUT2, OUT3}
.endm

.macro pass2  n
 .if \n == 1 || \n == 2
        cmp     ip, #(prefetch_distance+1)*4
        movcs   ip, #(prefetch_distance+1)*4
 .else
        cmp     ip, #prefetch_distance*4
        movcs   ip, #prefetch_distance*4
 .endif
        subs    ip, ip, #4
        bcc     2f
1:      pld     [v1]
        pld     [v1, #32]
        add     v1, v1, #64
        subs    ip, ip, #4
        bcs     1b
2:      sub     DIST, DIST, DIST, lsl #16
        msr     cpsr_s, #0x50000
        add     DIST, DIST, #1 << (32-BILINEAR_INTERPOLATION_BITS)
        @ top half of DIST now holds complementary weight
 .if \n != 0
3:      pass2_1pixel  0
        subs    COUNT, COUNT, #1
        bmi     99f
        tst     DST, #15
        bne     3b
 .endif
        subs    COUNT, COUNT, #4-1
        bmi     8f
        @ pixels_remaining_minus_4 = COUNT
        @ pixels_done = (4-n)&3
        @ preloads_done = at most, (prefetch_distance + 1 + (n==2 | n==3)) * 2
        @ total_preloads = (1 + (pixels - 1) / 4) * 2
        @ so if n is 0 or 3, then
        @ preloads_to_do_minus_1 = (COUNT + pixels_done - 1 - prefetch_distance*4) / 4 * 2
        @ or if n is 1 or 2, then 2 fewer than that
 .set adjust, ((4-\n)&3) - 1 - (prefetch_distance - (\n==1) - (\n==2)) * 4
        adds    COUNT, COUNT, #adjust
        bmi     6f
5:      pass2_4pixels  \n, do_preload
        subs    COUNT, COUNT, #4
        bpl     5b
6:
 .if adjust > -4
        subs    COUNT, COUNT, #adjust
        bmi     8f  // have to handle the possibility there are no groups of 4 without preloads
 .else
        sub     COUNT, COUNT, #adjust
 .endif
7:      pass2_4pixels  \n
        subs    COUNT, COUNT, #4
        bpl     7b
8:      adds    COUNT, COUNT, #4-1
        bmi     99f
9:      pass2_1pixel  (\n != 0)
        subs    COUNT, COUNT, #1
        bpl     9b
99:     pop     {v1-v8,pc}
.endm

/* void
 * pixman_get_scanline_bilinear_scaled_cover_pass2_asm_armv6 (
 *                                               uint32_t  width,
 *                                               uint32_t *dest,
 *                                               int16_t  *source,
 *                                               int16_t   dist_y)
 *
 * This version is used when the output scanline falls between two
 * different input scanlines
 */
pixman_asm_function pixman_get_scanline_bilinear_scaled_cover_pass2_asm_armv6
COUNT   .req    a1
DST     .req    a2
SRC     .req    a3
DIST    .req    a4
OUT0    .req    v1
OUT1    .req    v2
OUT2    .req    v3
OUT3    .req    v5 @ avoid register-lock of last STM register against following LDM
.set prefetch_distance, 2
        push    {v1-v8,lr}
        subs    COUNT, COUNT, #1
        bmi     99f
        movs    ip, DST, lsl #29
        add     v1, SRC, #64
        mov     ip, COUNT
        pld     [SRC]   @ already cacheline-aligned
        pld     [SRC, #32]
        @ total_preloads = (1 + (pixels - 1) / 4) * 2
        @ initial_preloads = at most, (prefetch_distance + 1 + (n==2 | n==3)) * 2
        bhi     13f
        bcs     12f
        bne     11f
10:     pass2   0
11:     pass2   1
12:     pass2   2
13:     pass2   3

.unreq  COUNT
.unreq  DST
.unreq  DIST
.unreq  SRC
.unreq  OUT0
.unreq  OUT1
.unreq  OUT2
.unreq  OUT3
.endfunc

/******************************************************************************/

.macro pass2a_1pixel  check_src_thresh
        ldmia   SRC!, {AG0, RB0}
 .if \check_src_thresh
        tst     SRC, #31
  .if prefetch_distance == 0
        bne     20f
        teq     COUNT, #0
        beq     20f
        add     SRC, SRC, #32
        pld     [SRC]
20:
  .else
        addeq   SRC, SRC, #32
  .endif
 .endif
        mov     AG0, AG0, lsl #8-BILINEAR_INTERPOLATION_BITS
        mov     RB0, RB0, lsr #BILINEAR_INTERPOLATION_BITS
        sel     OUT0, RB0, AG0
        str     OUT0, [DST], #4
.endm

.macro pass2a_4pixels  n, do_preload
 .if \n == 0
        ldmia   SRC!, {AG0, RB0, AG1, RB1, AG2, RB2, AG3, RB3}
 .elseif \n == 1
        ldmia   SRC!, {AG0, RB0}
 .elseif \n == 2
        ldmia   SRC!, {AG0, RB0, AG1, RB1}
 .else // \n == 3
        ldmia   SRC!, {AG0, RB0, AG1, RB1, AG2, RB2}
 .endif
        add     SRC, SRC, #32
 .ifnc "\do_preload",""
        pld     [SRC, #prefetch_distance*64]
 .endif
 .if \n == 1
        ldmia   SRC!, {AG1, RB1, AG2, RB2, AG3, RB3}
 .elseif \n == 2
        ldmia   SRC!, {AG2, RB2, AG3, RB3}
 .elseif \n == 3
        ldmia   SRC!, {AG3, RB3}
 .endif
        mov     AG0, AG0, lsl #8-BILINEAR_INTERPOLATION_BITS
        mov     RB0, RB0, lsr #BILINEAR_INTERPOLATION_BITS
        mov     AG1, AG1, lsl #8-BILINEAR_INTERPOLATION_BITS
        mov     RB1, RB1, lsr #BILINEAR_INTERPOLATION_BITS
        mov     AG2, AG2, lsl #8-BILINEAR_INTERPOLATION_BITS
        mov     RB2, RB2, lsr #BILINEAR_INTERPOLATION_BITS
        mov     AG3, AG3, lsl #8-BILINEAR_INTERPOLATION_BITS
        mov     RB3, RB3, lsr #BILINEAR_INTERPOLATION_BITS
        sel     OUT0, RB0, AG0
        sel     OUT1, RB1, AG1
        sel     OUT2, RB2, AG2
        sel     OUT3, RB3, AG3
        stmia   DST!, {OUT0, OUT1, OUT2, OUT3}
.endm

.macro pass2a  n
 .if \n != 0
3:      pass2a_1pixel  0
        subs    COUNT, COUNT, #1
        bmi     99f
        tst     DST, #15
        bne     3b
 .endif
        subs    COUNT, COUNT, #4-1
        bmi     8f
        @ pixels_remaining_minus_4 = COUNT
        @ pixels_done = (4-n)&3
        @ preloads_done = at most, prefetch_distance + 1
        @ total_preloads = 1 + (pixels - 1) / 4
        @ so preloads_to_do_minus_1 = (COUNT + pixels_done - 1 - prefetch_distance*4) / 4
 .set adjust, ((4-\n)&3) - 1 - prefetch_distance*4
        adds    COUNT, COUNT, #adjust
        bmi     6f
5:      pass2a_4pixels  \n, do_preload
        subs    COUNT, COUNT, #4
        bpl     5b
6:
 .if adjust > -4
        subs    COUNT, COUNT, #adjust
        bmi     8f  // have to handle the possibility there are no groups of 4 without preloads
 .else
        sub     COUNT, COUNT, #adjust
 .endif
7:      pass2a_4pixels  \n
        subs    COUNT, COUNT, #4
        bpl     7b
8:      adds    COUNT, COUNT, #4-1
        bmi     99f
9:      pass2a_1pixel  (\n != 0)
        subs    COUNT, COUNT, #1
        bpl     9b
99:     pop     {v1-v6,pc}
.endm

/* void
 * pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_armv6 (
 *                                               uint32_t  width,
 *                                               uint32_t *dest,
 *                                               int16_t  *source)
 *
 * This version is used when the output scanline coincides
 * exactly with an input scanline
 */
pixman_asm_function pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_armv6
COUNT   .req    a1
DST     .req    a2
SRC     .req    a3
AG0     .req    a4
RB0     .req    v1
AG1     .req    v2
RB1     .req    v3
AG2     .req    v4
RB2     .req    v5
AG3     .req    v6
RB3     .req    ip
OUT0    .req    a4
OUT1    .req    v2
OUT2    .req    v4
OUT3    .req    lr @ avoid register-lock of last STM register against following LDM
.set prefetch_distance, 2
        push    {v1-v6,lr}
        subs    COUNT, COUNT, #1
        bcc     99f
        @ total_preloads = 1 + (pixels - 1) / 4
        @ initial_preloads = at most, prefetch_distance + 1
        mov     ip, #prefetch_distance*4
        cmp     COUNT, #prefetch_distance*4
        pld     [SRC]   @ already cacheline-aligned
        movcc   ip, COUNT
        add     a4, SRC, #64
        subs    ip, ip, #4
        bcc     2f
1:      pld     [a4]
        add     a4, a4, #64
        subs    ip, ip, #4
        bcs     1b
2:      msr     cpsr_s, #0x50000
        movs    ip, DST, lsl #29
        bhi     13f
        bcs     12f
        bne     11f
10:     pass2a  0
11:     pass2a  1
12:     pass2a  2
13:     pass2a  3

.unreq  COUNT
.unreq  DST
.unreq  SRC
.unreq  AG0
.unreq  RB0
.unreq  AG1
.unreq  RB1
.unreq  AG2
.unreq  RB2
.unreq  AG3
.unreq  RB3
.unreq  OUT0
.unreq  OUT1
.unreq  OUT2
.unreq  OUT3
.endfunc

/******************************************************************************/
