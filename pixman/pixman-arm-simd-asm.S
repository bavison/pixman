/*
 * Copyright © 2012 Raspberry Pi Foundation
 * Copyright © 2012 RISC OS Open Ltd
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
#include "pixman-arm-simd-asm.h"

/* A head macro should do all processing which results in an output of up to
 * 16 bytes, as far as the final load instruction. The corresponding tail macro
 * should complete the processing of the up-to-16 bytes. The calling macro will
 * sometimes choose to insert a preload or a decrement of X between them.
 *   cond           ARM condition code for code block
 *   numbytes       Number of output bytes that should be generated this time
 *   firstreg       First WK register in which to place output
 *   unaligned_src  Whether to use non-wordaligned loads of source image
 *   unaligned_mask Whether to use non-wordaligned loads of mask image
 *   preload        If outputting 16 bytes causes 64 bytes to be read, whether an extra preload should be output
 */

.macro blit_init
        line_saved_regs STRIDE_D, STRIDE_S
.endm

.macro blit_process_head   cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   cond, numbytes, firstreg, SRC, unaligned_src
.endm

.macro blit_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, dst_alignment
    WK4     .req    STRIDE_D
    WK5     .req    STRIDE_S
    WK6     .req    MASK
    WK7     .req    STRIDE_M
110:    pixld   , 16, 0, SRC, unaligned_src
        pixld   , 16, 4, SRC, unaligned_src
        pld     [SRC, SCRATCH]
        pixst   , 16, 0, DST
        pixst   , 16, 4, DST
        subs    X, X, #32*8/src_bpp
        bhs     110b
    .unreq  WK4
    .unreq  WK5
    .unreq  WK6
    .unreq  WK7
.endm

generate_composite_function \
    pixman_composite_src_8888_8888_asm_armv6, 32, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_SPILL_LINE_VARS_WIDE | FLAG_PROCESS_PRESERVES_SCRATCH, \
    4, /* prefetch distance */ \
    blit_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    blit_process_head, \
    nop_macro, /* process tail */ \
    blit_inner_loop

generate_composite_function \
    pixman_composite_src_0565_0565_asm_armv6, 16, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_SPILL_LINE_VARS_WIDE | FLAG_PROCESS_PRESERVES_SCRATCH, \
    4, /* prefetch distance */ \
    blit_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    blit_process_head, \
    nop_macro, /* process tail */ \
    blit_inner_loop

generate_composite_function \
    pixman_composite_src_8_8_asm_armv6, 8, 0, 8, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_SPILL_LINE_VARS_WIDE | FLAG_PROCESS_PRESERVES_SCRATCH, \
    3, /* prefetch distance */ \
    blit_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    blit_process_head, \
    nop_macro, /* process tail */ \
    blit_inner_loop

/******************************************************************************/

.macro src_n_8888_init
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        mov     STRIDE_S, SRC
        mov     MASK, SRC
        mov     STRIDE_M, SRC
.endm

.macro src_n_0565_init
        ldrh    SRC, [sp, #ARGS_STACK_OFFSET]
        orr     SRC, SRC, lsl #16
        mov     STRIDE_S, SRC
        mov     MASK, SRC
        mov     STRIDE_M, SRC
.endm

.macro src_n_8_init
        ldrb    SRC, [sp, #ARGS_STACK_OFFSET]
        orr     SRC, SRC, lsl #8
        orr     SRC, SRC, lsl #16
        mov     STRIDE_S, SRC
        mov     MASK, SRC
        mov     STRIDE_M, SRC
.endm

.macro fill_process_tail  cond, numbytes, firstreg
    WK4     .req    SRC
    WK5     .req    STRIDE_S
    WK6     .req    MASK
    WK7     .req    STRIDE_M
        pixst   cond, numbytes, 4, DST
    .unreq  WK4
    .unreq  WK5
    .unreq  WK6
    .unreq  WK7
.endm

generate_composite_function \
    pixman_composite_src_n_8888_asm_armv6, 0, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_PROCESS_PRESERVES_PSR | FLAG_PROCESS_DOES_STORE | FLAG_PROCESS_PRESERVES_SCRATCH \
    0, /* prefetch distance doesn't apply */ \
    src_n_8888_init \
    nop_macro, /* newline */ \
    nop_macro /* cleanup */ \
    nop_macro /* process head */ \
    fill_process_tail

generate_composite_function \
    pixman_composite_src_n_0565_asm_armv6, 0, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_PROCESS_PRESERVES_PSR | FLAG_PROCESS_DOES_STORE | FLAG_PROCESS_PRESERVES_SCRATCH \
    0, /* prefetch distance doesn't apply */ \
    src_n_0565_init \
    nop_macro, /* newline */ \
    nop_macro /* cleanup */ \
    nop_macro /* process head */ \
    fill_process_tail

generate_composite_function \
    pixman_composite_src_n_8_asm_armv6, 0, 0, 8, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_PROCESS_PRESERVES_PSR | FLAG_PROCESS_DOES_STORE | FLAG_PROCESS_PRESERVES_SCRATCH \
    0, /* prefetch distance doesn't apply */ \
    src_n_8_init \
    nop_macro, /* newline */ \
    nop_macro /* cleanup */ \
    nop_macro /* process head */ \
    fill_process_tail

/******************************************************************************/

.macro src_x888_8888_pixel, cond, reg
        orr&cond WK&reg, WK&reg, #0xFF000000
.endm

.macro pixman_composite_src_x888_8888_process_head   cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   cond, numbytes, firstreg, SRC, unaligned_src
.endm

.macro pixman_composite_src_x888_8888_process_tail   cond, numbytes, firstreg
        src_x888_8888_pixel cond, %(firstreg+0)
 .if numbytes >= 8
        src_x888_8888_pixel cond, %(firstreg+1)
  .if numbytes == 16
        src_x888_8888_pixel cond, %(firstreg+2)
        src_x888_8888_pixel cond, %(firstreg+3)
  .endif
 .endif
.endm

generate_composite_function \
    pixman_composite_src_x888_8888_asm_armv6, 32, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_PROCESS_PRESERVES_SCRATCH, \
    3, /* prefetch distance */ \
    nop_macro, /* init */ \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    pixman_composite_src_x888_8888_process_head, \
    pixman_composite_src_x888_8888_process_tail

/******************************************************************************/

.macro src_0565_8888_init
        /* Hold loop invariants in MASK and STRIDE_M */
        ldr     MASK, =0x07E007E0
        mov     STRIDE_M, #0xFF000000
        /* Set GE[3:0] to 1010 so SEL instructions do what we want */
        ldr     SCRATCH, =0x80008000
        uadd8   SCRATCH, SCRATCH, SCRATCH
.endm

.macro src_0565_8888_2pixels, reg1, reg2
        and     SCRATCH, WK&reg1, MASK             @ 00000GGGGGG0000000000gggggg00000
        bic     WK&reg2, WK&reg1, MASK             @ RRRRR000000BBBBBrrrrr000000bbbbb
        orr     SCRATCH, SCRATCH, SCRATCH, lsr #6  @ 00000GGGGGGGGGGGG0000ggggggggggg
        mov     WK&reg1, WK&reg2, lsl #16          @ rrrrr000000bbbbb0000000000000000
        mov     SCRATCH, SCRATCH, ror #19          @ GGGG0000ggggggggggg00000GGGGGGGG
        bic     WK&reg2, WK&reg2, WK&reg1, lsr #16 @ RRRRR000000BBBBB0000000000000000
        orr     WK&reg1, WK&reg1, WK&reg1, lsr #5  @ rrrrrrrrrr0bbbbbbbbbb00000000000
        orr     WK&reg2, WK&reg2, WK&reg2, lsr #5  @ RRRRRRRRRR0BBBBBBBBBB00000000000
        pkhtb   WK&reg1, WK&reg1, WK&reg1, asr #5  @ rrrrrrrr--------bbbbbbbb--------
        sel     WK&reg1, WK&reg1, SCRATCH          @ rrrrrrrrggggggggbbbbbbbb--------
        mov     SCRATCH, SCRATCH, ror #16          @ ggg00000GGGGGGGGGGGG0000gggggggg
        pkhtb   WK&reg2, WK&reg2, WK&reg2, asr #5  @ RRRRRRRR--------BBBBBBBB--------
        sel     WK&reg2, WK&reg2, SCRATCH          @ RRRRRRRRGGGGGGGGBBBBBBBB--------
        orr     WK&reg1, STRIDE_M, WK&reg1, lsr #8 @ 11111111rrrrrrrrggggggggbbbbbbbb
        orr     WK&reg2, STRIDE_M, WK&reg2, lsr #8 @ 11111111RRRRRRRRGGGGGGGGBBBBBBBB
.endm

/* This version doesn't need STRIDE_M, but is one instruction longer.
   It would however be preferable for an XRGB target, since we could knock off the last 2 instructions, but is that a common case?
        and     SCRATCH, WK&reg1, MASK             @ 00000GGGGGG0000000000gggggg00000
        bic     WK&reg1, WK&reg1, MASK             @ RRRRR000000BBBBBrrrrr000000bbbbb
        orr     SCRATCH, SCRATCH, SCRATCH, lsr #6  @ 00000GGGGGGGGGGGG0000ggggggggggg
        mov     WK&reg2, WK&reg1, lsr #16          @ 0000000000000000RRRRR000000BBBBB
        mov     SCRATCH, SCRATCH, ror #27          @ GGGGGGGGGGGG0000ggggggggggg00000
        bic     WK&reg1, WK&reg1, WK&reg2, lsl #16 @ 0000000000000000rrrrr000000bbbbb
        mov     WK&reg2, WK&reg2, lsl #3           @ 0000000000000RRRRR000000BBBBB000
        mov     WK&reg1, WK&reg1, lsl #3           @ 0000000000000rrrrr000000bbbbb000
        orr     WK&reg2, WK&reg2, WK&reg2, lsr #5  @ 0000000000000RRRRRRRRRR0BBBBBBBB
        orr     WK&reg1, WK&reg1, WK&reg1, lsr #5  @ 0000000000000rrrrrrrrrr0bbbbbbbb
        pkhbt   WK&reg2, WK&reg2, WK&reg2, lsl #5  @ --------RRRRRRRR--------BBBBBBBB
        pkhbt   WK&reg1, WK&reg1, WK&reg1, lsl #5  @ --------rrrrrrrr--------bbbbbbbb
        sel     WK&reg2, SCRATCH, WK&reg2          @ --------RRRRRRRRGGGGGGGGBBBBBBBB
        sel     WK&reg1, SCRATCH, WK&reg1          @ --------rrrrrrrrggggggggbbbbbbbb
        orr     WK&reg2, WK&reg2, #0xFF000000      @ 11111111RRRRRRRRGGGGGGGGBBBBBBBB
        orr     WK&reg1, WK&reg1, #0xFF000000      @ 11111111rrrrrrrrggggggggbbbbbbbb
*/

.macro src_0565_8888_1pixel, reg
        bic     SCRATCH, WK&reg, MASK              @ 0000000000000000rrrrr000000bbbbb
        and     WK&reg, WK&reg, MASK               @ 000000000000000000000gggggg00000
        mov     SCRATCH, SCRATCH, lsl #3           @ 0000000000000rrrrr000000bbbbb000
        mov     WK&reg, WK&reg, lsl #5             @ 0000000000000000gggggg0000000000
        orr     SCRATCH, SCRATCH, SCRATCH, lsr #5  @ 0000000000000rrrrrrrrrr0bbbbbbbb
        orr     WK&reg, WK&reg, WK&reg, lsr #6     @ 000000000000000gggggggggggg00000
        pkhbt   SCRATCH, SCRATCH, SCRATCH, lsl #5  @ --------rrrrrrrr--------bbbbbbbb
        sel     WK&reg, WK&reg, SCRATCH            @ --------rrrrrrrrggggggggbbbbbbbb
        orr     WK&reg, WK&reg, #0xFF000000        @ 11111111rrrrrrrrggggggggbbbbbbbb
.endm

.macro src_0565_8888_process_head   cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        pixldst ld,, 8, firstreg, %(firstreg+2),,, SRC, unaligned_src
 .elseif numbytes == 8
        pixld   , 4, firstreg, SRC, unaligned_src
 .elseif numbytes == 4
        pixld   , 2, firstreg, SRC, unaligned_src
 .endif
.endm

.macro src_0565_8888_process_tail   cond, numbytes, firstreg
 .if numbytes == 16
        src_0565_8888_2pixels firstreg, %(firstreg+1)
        src_0565_8888_2pixels %(firstreg+2), %(firstreg+3)
 .elseif numbytes == 8
        src_0565_8888_2pixels firstreg, %(firstreg+1)
 .else
        src_0565_8888_1pixel firstreg
 .endif
.endm

generate_composite_function \
    pixman_composite_src_0565_8888_asm_armv6, 16, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_BRANCH_OVER, \
    3, /* prefetch distance */ \
    src_0565_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    src_0565_8888_process_head, \
    src_0565_8888_process_tail

generate_composite_function_single_scanline \
    pixman_get_scanline_0565_asm_armv6, 16, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_BRANCH_OVER, \
    3, /* prefetch distance */ \
    src_0565_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    src_0565_8888_process_head, \
    src_0565_8888_process_tail

/******************************************************************************/

.macro src_x888_0565_init
        /* Hold loop invariant in MASK */
        ldr     MASK, =0x001F001F
        line_saved_regs  STRIDE_S, ORIG_W
.endm

.macro src_x888_0565_1pixel  s, d
        and     WK&d, MASK, WK&s, lsr #3           @ 00000000000rrrrr00000000000bbbbb
        and     STRIDE_S, WK&s, #0xFC00            @ 0000000000000000gggggg0000000000
        orr     WK&d, WK&d, WK&d, lsr #5           @ 00000000000-----rrrrr000000bbbbb
        orr     WK&d, WK&d, STRIDE_S, lsr #5       @ 00000000000-----rrrrrggggggbbbbb
        /* Top 16 bits are discarded during the following STRH */
.endm

.macro src_x888_0565_2pixels  slo, shi, d, tmp
        and     SCRATCH, WK&shi, #0xFC00           @ 0000000000000000GGGGGG0000000000
        and     WK&tmp, MASK, WK&shi, lsr #3       @ 00000000000RRRRR00000000000BBBBB
        and     WK&shi, MASK, WK&slo, lsr #3       @ 00000000000rrrrr00000000000bbbbb
        orr     WK&tmp, WK&tmp, WK&tmp, lsr #5     @ 00000000000-----RRRRR000000BBBBB
        orr     WK&tmp, WK&tmp, SCRATCH, lsr #5    @ 00000000000-----RRRRRGGGGGGBBBBB
        and     SCRATCH, WK&slo, #0xFC00           @ 0000000000000000gggggg0000000000
        orr     WK&shi, WK&shi, WK&shi, lsr #5     @ 00000000000-----rrrrr000000bbbbb
        orr     WK&shi, WK&shi, SCRATCH, lsr #5    @ 00000000000-----rrrrrggggggbbbbb
        pkhbt   WK&d, WK&shi, WK&tmp, lsl #16      @ RRRRRGGGGGGBBBBBrrrrrggggggbbbbb
.endm

.macro src_x888_0565_process_head   cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        WK4     .req    STRIDE_S
        WK5     .req    STRIDE_M
        WK6     .req    WK3
        WK7     .req    ORIG_W
 .if numbytes == 16
        pixld   , 16, 4, SRC, 0
        src_x888_0565_2pixels  4, 5, 0, 0
        pixld   , 8, 4, SRC, 0
        src_x888_0565_2pixels  6, 7, 1, 1
        pixld   , 8, 6, SRC, 0
 .else
        pixld   , numbytes*2, 4, SRC, 0
 .endif
.endm

.macro src_x888_0565_process_tail   cond, numbytes, firstreg
 .if numbytes == 16
        src_x888_0565_2pixels  4, 5, 2, 2
        src_x888_0565_2pixels  6, 7, 3, 4
 .elseif numbytes == 8
        src_x888_0565_2pixels  4, 5, 1, 1
        src_x888_0565_2pixels  6, 7, 2, 2
 .elseif numbytes == 4
        src_x888_0565_2pixels  4, 5, 1, 1
 .else
        src_x888_0565_1pixel  4, 1
 .endif
 .if numbytes == 16
        pixst   , numbytes, 0, DST
 .else
        pixst   , numbytes, 1, DST
 .endif
        .unreq  WK4
        .unreq  WK5
        .unreq  WK6
        .unreq  WK7
.endm

generate_composite_function \
    pixman_composite_src_x888_0565_asm_armv6, 32, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH, \
    3, /* prefetch distance */ \
    src_x888_0565_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    src_x888_0565_process_head, \
    src_x888_0565_process_tail

generate_composite_function_single_scanline \
    pixman_write_back_0565_asm_armv6, 32, 0, 16, \
    FLAG_DST_WRITEONLY | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH, \
    3, /* prefetch distance */ \
    src_x888_0565_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    src_x888_0565_process_head, \
    src_x888_0565_process_tail

/******************************************************************************/

.macro src_8_8888_init
        mov     MASK, #0xff000000
.endm

.macro src_8_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   cond, numbytes/4, firstreg, SRC, unaligned_src
.endm

.macro src_8_8888_1pixel  cond, d0
        mov&cond WK&d0, WK&d0, lsl #24
.endm

.macro src_8_8888_2pixels  cond, d0, d1
        and&cond WK&d1, MASK, WK&d0, lsl #16
        mov&cond WK&d0, WK&d0, lsl #24
.endm

.macro src_8_8888_4pixels  cond, d0, d1, d2, d3
        and&cond WK&d3, MASK, WK&d0
        and&cond WK&d2, MASK, WK&d0, lsl #8
        and&cond WK&d1, MASK, WK&d0, lsl #16
        mov&cond WK&d0, WK&d0, lsl #24
.endm

.macro src_8_8888_process_tail  cond, numbytes, firstreg
 .if numbytes == 16
        src_8_8888_4pixels  cond, %(firstreg+0), %(firstreg+1), %(firstreg+2), %(firstreg+3)
 .elseif numbytes == 8
        src_8_8888_2pixels  cond, %(firstreg+0), %(firstreg+1)
 .else // numbytes == 4
        src_8_8888_1pixel  cond, %(firstreg+0)
 .endif
.endm

.macro src_8_8888_inner_loop  process_head, process_tail, unaligned_src, unaligned_mask, dst_alignment
110: /* Length of inner loop is set to allow one preload per 32 source pixels */
        ldr     STRIDE_M, [SRC], #4
        and     WK3, MASK, STRIDE_M
        and     WK2, MASK, STRIDE_M, lsl #8
        and     WK1, MASK, STRIDE_M, lsl #16
        mov     WK0, STRIDE_M, lsl #24
        ldr     STRIDE_M, [SRC], #4
 .rept 6
        pixst   , 16, 0, DST
        and     WK3, MASK, STRIDE_M
        and     WK2, MASK, STRIDE_M, lsl #8
        and     WK1, MASK, STRIDE_M, lsl #16
        mov     WK0, STRIDE_M, lsl #24
        ldr     STRIDE_M, [SRC], #4
 .endr
        pld     [SRC, SCRATCH]
        pixst   , 16, 0, DST
        and     WK3, MASK, STRIDE_M
        and     WK2, MASK, STRIDE_M, lsl #8
        and     WK1, MASK, STRIDE_M, lsl #16
        mov     WK0, STRIDE_M, lsl #24
        pixst   , 16, 0, DST
        subs    X, X, #32
        bhs     110b
.endm

generate_composite_function_single_scanline \
    pixman_get_scanline_8_asm_armv6, 8, 0, 32, \
    FLAG_DST_WRITEONLY | FLAG_COND_EXEC | FLAG_PROCESS_PRESERVES_SCRATCH, \
    2, /* prefetch distance */ \
    src_8_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    src_8_8888_process_head, \
    src_8_8888_process_tail, \
    src_8_8888_inner_loop

/******************************************************************************/

.macro add_8_8_8pixels  cond, dst1, dst2
        uqadd8&cond  WK&dst1, WK&dst1, MASK
        uqadd8&cond  WK&dst2, WK&dst2, STRIDE_M
.endm

.macro add_8_8_4pixels  cond, dst
        uqadd8&cond  WK&dst, WK&dst, MASK
.endm

.macro add_8_8_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
    WK4     .req    MASK
    WK5     .req    STRIDE_M
 .if numbytes == 16
        pixld   cond, 8, 4, SRC, unaligned_src
        pixld   cond, 16, firstreg, DST, 0
        add_8_8_8pixels cond, firstreg, %(firstreg+1)
        pixld   cond, 8, 4, SRC, unaligned_src
 .else
        pixld   cond, numbytes, 4, SRC, unaligned_src
        pixld   cond, numbytes, firstreg, DST, 0
 .endif
    .unreq  WK4
    .unreq  WK5
.endm

.macro add_8_8_process_tail  cond, numbytes, firstreg
 .if numbytes == 16
        add_8_8_8pixels cond, %(firstreg+2), %(firstreg+3)
 .elseif numbytes == 8
        add_8_8_8pixels cond, firstreg, %(firstreg+1)
 .else
        add_8_8_4pixels cond, firstreg
 .endif
.endm

generate_composite_function \
    pixman_composite_add_8_8_asm_armv6, 8, 0, 8, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_PRESERVES_SCRATCH, \
    2, /* prefetch distance */ \
    nop_macro, /* init */ \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    add_8_8_process_head, \
    add_8_8_process_tail

generate_composite_function_single_scanline \
    pixman_composite_scanline_add_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_PRESERVES_SCRATCH, \
    2, /* prefetch distance */ \
    nop_macro, /* init */ \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    add_8_8_process_head, \
    add_8_8_process_tail

/******************************************************************************/

.macro over_8888_8888_check_transparent  numbytes, reg0, reg1, reg2, reg3
        /* Since these colours a premultiplied by alpha, only 0 indicates transparent (any other colour with 0 in the alpha byte is luminous) */
        teq     WK&reg0, #0
 .if numbytes > 4
        teqeq   WK&reg1, #0
  .if numbytes > 8
        teqeq   WK&reg2, #0
        teqeq   WK&reg3, #0
  .endif
 .endif
.endm

.macro over_8888_8888_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000
        line_saved_regs Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro over_8888_8888_newline
        ldr     Y, =0x00800080
        mov     STRIDE_D, #0xff
.endm

.macro over_8888_8888_1pixel  s, m, d, tmp, half, ff, offset
        sub     m, ff, s, lsr #24
        uxtb16  tmp, d
        uxtb16  d, d, ror #8
        mla     tmp, tmp, m, half
        mla     d, d, m, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 d, d, d, ror #8
        mov     tmp, tmp, ror #8
        sel     d, tmp, d
        uqadd8  d, d, s
        str     d, [DST, #offset]
.endm

.macro over_8888_8888_2pixels  s0, s1, m0, m1, d0, d1, tmp0, tmp1, half, ff, offset
        sub     m0, ff, s0, lsr #24
        sub     m1, ff, s1, lsr #24
        uxtb16  tmp0, d0
        uxtb16  d0, d0, ror #8
        uxtb16  tmp1, d1
        uxtb16  d1, d1, ror #8
        mla     tmp0, tmp0, m0, half
        mla     d0, d0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     d1, d1, m1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 d0, d0, d0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 d1, d1, d1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     d0, tmp0, d0
        sel     d1, tmp1, d1
        uqadd8  d0, d0, s0
        uqadd8  d1, d1, s1
        strd    d0, d1, [DST, #offset]
.endm

.macro over_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        ldm     SRC!, {WK0, WK1}
        ldm     SRC!, {STRIDE_S, STRIDE_M}
        ldrd    WK2, WK3, [DST], #16
        orr     SCRATCH, WK0, WK1
        orr     SCRATCH, SCRATCH, STRIDE_S
        orrs    SCRATCH, SCRATCH, STRIDE_M
        beq     20f
        over_8888_8888_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -16
        ldrd    WK0, WK1, [SRC, #-8]
        ldrd    WK2, WK3, [DST, #-8]
 .elseif numbytes == 8
        ldrd    WK0, WK1, [SRC], #8
        ldrd    WK2, WK3, [DST], #8
        orrs    SCRATCH, WK0, WK1
        beq     20f
 .else // numbytes == 4
        ldr     WK0, [SRC], #4
        ldr     WK2, [DST], #4
        teq     WK0, #0
        beq     20f
 .endif
.endm

.macro over_8888_8888_process_tail  cond, numbytes, firstreg
 .if numbytes >= 8
        over_8888_8888_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -8
 .else // numbytes == 4
        over_8888_8888_1pixel WK0, STRIDE_S, WK2, SCRATCH, Y, STRIDE_D, -4
 .endif
20:
.endm

generate_composite_function \
    pixman_composite_over_8888_8888_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_8888_8888_init, \
    over_8888_8888_newline, \
    nop_macro, /* cleanup */ \
    over_8888_8888_process_head, \
    over_8888_8888_process_tail

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_8888_8888_init, \
    over_8888_8888_newline \
    nop_macro, /* cleanup */ \
    over_8888_8888_process_head, \
    over_8888_8888_process_tail

/******************************************************************************/

.macro over_8888_8888_8888_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000
        /* Point at alpha byte in mask */
        add     MASK, MASK, #3
        line_saved_regs Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro over_8888_8888_8888_newline
        ldr     Y, =0x00800080
        mov     STRIDE_D, #0xff
.endm

.macro over_8888_8888_8888_1pixel  s, m, d, tmp, half, ff, offset
        uxtb16  tmp, s
        uxtb16  s, s, ror #8
        mla     tmp, tmp, m, half
        mla     s, s, m, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 s, s, s, ror #8
        mov     tmp, tmp, ror #8
        sub     m, ff, s, lsr #24
        sel     s, tmp, s
        uxtb16  tmp, d
        uxtb16  d, d, ror #8
        mla     tmp, tmp, m, half
        mla     d, d, m, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 d, d, d, ror #8
        mov     tmp, tmp, ror #8
        sel     d, tmp, d
        uqadd8  d, d, s
        str     d, [DST, #offset]
.endm

.macro over_8888_8888_8888_2pixels  s0, s1, m0, m1, d0, d1, tmp0, tmp1, half, ff, offset
        uxtb16  tmp0, s0
        uxtb16  s0, s0, ror #8
        uxtb16  tmp1, s1
        uxtb16  s1, s1, ror #8
        mla     tmp0, tmp0, m0, half
        mla     s0, s0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     s1, s1, m1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sub     m0, ff, s0, lsr #24
        sub     m1, ff, s1, lsr #24
        sel     s0, tmp0, s0
        sel     s1, tmp1, s1
        uxtb16  tmp0, d0
        uxtb16  d0, d0, ror #8
        uxtb16  tmp1, d1
        uxtb16  d1, d1, ror #8
        mla     tmp0, tmp0, m0, half
        mla     d0, d0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     d1, d1, m1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 d0, d0, d0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 d1, d1, d1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     d0, tmp0, d0
        sel     d1, tmp1, d1
        uqadd8  d0, d0, s0
        uqadd8  d1, d1, s1
        strd    d0, d1, [DST, #offset]
.endm

.macro over_8888_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        ldm     SRC!, {WK0, WK1, SCRATCH, ORIG_W}
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
        orr     WK2, WK0, WK1
        orr     WK2, WK2, SCRATCH
        orrs    WK2, WK2, ORIG_W
        ldrb    SCRATCH, [MASK], #4
        ldrb    ORIG_W, [MASK], #4
        ldrd    WK2, WK3, [DST], #16
        bne     10f
        orr     SCRATCH, SCRATCH, STRIDE_S
        orr     SCRATCH, SCRATCH, STRIDE_M
        orrs    SCRATCH, SCRATCH, ORIG_W
        beq     20f
10:
        over_8888_8888_8888_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -16
        ldrd    WK0, WK1, [SRC, #-8]
        ldrb    STRIDE_S, [MASK, #-8]
        ldrb    STRIDE_M, [MASK, #-4]
        ldrd    WK2, WK3, [DST, #-8]
 .elseif numbytes == 8
        ldrd    WK0, WK1, [SRC], #8
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
        ldrd    WK2, WK3, [DST], #8
 .else // numbytes == 4
        ldr     WK0, [SRC], #4
        ldrb    STRIDE_S, [MASK], #4
        ldr     WK2, [DST], #4
 .endif
.endm

.macro over_8888_8888_8888_process_tail  cond, numbytes, firstreg
 .if numbytes >= 8
        over_8888_8888_8888_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -8
 .else // numbytes == 4
        over_8888_8888_8888_1pixel WK0, STRIDE_S, WK2, SCRATCH, Y, STRIDE_D, -4
 .endif
20:
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_8888_8888_8888_init, \
    over_8888_8888_8888_newline, \
    nop_macro, /* cleanup */ \
    over_8888_8888_8888_process_head, \
    over_8888_8888_8888_process_tail

/******************************************************************************/

.macro over_reverse_8888_8888_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000

        /* Keep WK0,WK1 where they are so they remain valid for LDRD */
        .unreq  WK2
        .unreq  WK3
        WK2     .req    STRIDE_S
        WK3     .req    STRIDE_M
        WK4     .req    r10
        WK5     .req    r11

        line_saved_regs STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro over_reverse_8888_8888_newline
        ldr     MASK, =0x00800080
        mov     STRIDE_D, #0xff
.endm

.macro over_reverse_8888_8888_cleanup
        .unreq  WK2
        .unreq  WK3
        .unreq  WK4
        .unreq  WK5
        WK2     .req    r10
        WK3     .req    r11
.endm

.macro over_reverse_8888_8888_1pixel  s0, d0, tmp0, tmp1, half, ff, offset
        uxtb16  tmp0, s0
        sub     tmp1, ff, d0, lsr #24
        uxtb16  s0, s0, ror #8
        mla     tmp0, tmp0, tmp1, half
        mla     s0, s0, tmp1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        mov     tmp0, tmp0, ror #8
        sel     s0, tmp0, s0
        uqadd8  WK0, d0, s0
        str     WK0, [DST, #offset]
.endm

.macro over_reverse_8888_8888_2pixels  s0, s1, d0, d1, tmp0, tmp1, tmp2, tmp3, half, ff, offset
        uxtb16  tmp0, s0
        uxtb16  s0, s0, ror #8
        sub     tmp2, ff, d0, lsr #24
        sub     tmp3, ff, d1, lsr #24
        uxtb16  tmp1, s1
        uxtb16  s1, s1, ror #8
        mla     tmp0, tmp0, tmp2, half
        mla     s0, s0, tmp2, half
        mla     tmp1, tmp1, tmp3, half
        mla     s1, s1, tmp3, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     s0, tmp0, s0
        sel     s1, tmp1, s1
        uqadd8  WK0, d0, s0
        uqadd8  WK1, d1, s1
        strd    WK0, WK1, [DST, #offset]
.endm

.macro over_reverse_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        pixld   , 16, 2, DST, 0
        ldrd    WK0, WK1, [SRC], #16
        and     SCRATCH, WK2, WK3
        and     SCRATCH, SCRATCH, WK4
        and     SCRATCH, SCRATCH, WK5
        teq     STRIDE_D, SCRATCH, lsr #24
 .elseif numbytes == 8
        pixld   , 8, 2, DST, 0
        pixld   , 8, 0, SRC, unaligned_src
        and     SCRATCH, WK2, WK3
        teq     STRIDE_D, SCRATCH, lsr #24
 .else // numbytes == 4
        pixld   , 4, 2, DST, 0
        pixld   , 4, 0, SRC, unaligned_src
        teq     STRIDE_D, WK2, lsr #24
 .endif
.endm

.macro over_reverse_8888_8888_process_tail  cond, numbytes, firstreg
        beq     10f // all destination pixels are opaque
 .if numbytes == 16
        over_reverse_8888_8888_2pixels WK0, WK1, WK2, WK3, WK4, WK5, ORIG_W, SCRATCH, MASK, STRIDE_D, -16
        ldmdb   SRC, {WK2, WK3}
        ldmdb   DST, {WK0, WK1}
        over_reverse_8888_8888_2pixels WK2, WK3, WK0, WK1, WK4, WK5, ORIG_W, SCRATCH, MASK, STRIDE_D, -8
 .elseif numbytes == 8
        over_reverse_8888_8888_2pixels WK0, WK1, WK2, WK3, WK4, WK5, ORIG_W, SCRATCH, MASK, STRIDE_D, -8
 .else // numbytes == 4
        over_reverse_8888_8888_1pixel WK0, WK2, WK4, ORIG_W, MASK, STRIDE_D, -4
 .endif
10:
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_reverse_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_reverse_8888_8888_init, \
    over_reverse_8888_8888_newline, \
    over_reverse_8888_8888_cleanup, \
    over_reverse_8888_8888_process_head, \
    over_reverse_8888_8888_process_tail

/******************************************************************************/

.macro over_reverse_8888_8888_8888_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000
        /* Point at alpha byte in mask */
        add     MASK, MASK, #3

        /* Keep WK0,WK1 where they are so they remain valid for LDRD */
        .unreq  WK2
        .unreq  WK3
        WK2     .req    STRIDE_S
        WK3     .req    STRIDE_M
        WK4     .req    r10
        WK5     .req    r11

        line_saved_regs Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro over_reverse_8888_8888_8888_newline
        ldr     Y, =0x00800080
        mov     STRIDE_D, #0xff
.endm

.macro over_reverse_8888_8888_8888_cleanup
        .unreq  WK2
        .unreq  WK3
        .unreq  WK4
        .unreq  WK5
        WK2     .req    r10
        WK3     .req    r11
.endm

.macro over_reverse_8888_8888_8888_1pixel  s, m, d, tmp, half, ff, offset
        uxtb16  tmp, s
        uxtb16  s, s, ror #8
        mla     tmp, tmp, m, half
        mla     s, s, m, half
        sub     m, ff, d, lsr #24
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 s, s, s, ror #8
        uxtb16  tmp, tmp, ror #8
        uxtb16  s, s, ror #8
        mla     tmp, tmp, m, half
        mla     s, s, m, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 s, s, s, ror #8
        mov     tmp, tmp, ror #8
        sel     s, tmp, s
        uqadd8  d, d, s
        str     d, [DST, #offset]
.endm

.macro over_reverse_8888_8888_8888_2pixels  s0, s1, m0, m1, d0, d1, tmp0, tmp1, half, ff, offset
        uxtb16  tmp0, s0
        uxtb16  s0, s0, ror #8
        uxtb16  tmp1, s1
        uxtb16  s1, s1, ror #8
        mla     tmp0, tmp0, m0, half
        mla     s0, s0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     s1, s1, m1, half
        sub     m0, ff, d0, lsr #24
        sub     m1, ff, d1, lsr #24
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        uxtb16  tmp0, tmp0, ror #8
        uxtb16  s0, s0, ror #8
        uxtb16  tmp1, tmp1, ror #8
        uxtb16  s1, s1, ror #8
        mla     tmp0, tmp0, m0, half
        mla     s0, s0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     s1, s1, m1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     s0, tmp0, s0
        sel     s1, tmp1, s1
        uqadd8  WK0, d0, s0
        uqadd8  WK1, d1, s1
        strd    WK0, WK1, [DST, #offset]
.endm

.macro over_reverse_8888_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        pixld   , 16, 2, DST, 0
        ldrd    WK0, WK1, [SRC], #16
        ldrb    ORIG_W, [MASK], #16
        and     SCRATCH, WK2, WK3
        and     SCRATCH, SCRATCH, WK4
        and     SCRATCH, SCRATCH, WK5
        teq     STRIDE_D, SCRATCH, lsr #24
 .elseif numbytes == 8
        pixld   , 8, 2, DST, 0
        pixld   , 8, 0, SRC, unaligned_src
        ldrb    ORIG_W, [MASK], #8
        and     SCRATCH, WK2, WK3
        teq     STRIDE_D, SCRATCH, lsr #24
 .else // numbytes == 4
        pixld   , 4, 2, DST, 0
        pixld   , 4, 0, SRC, unaligned_src
        ldrb    ORIG_W, [MASK], #4
        teq     STRIDE_D, WK2, lsr #24
 .endif
.endm

.macro over_reverse_8888_8888_8888_process_tail  cond, numbytes, firstreg
        beq     10f // all destination pixels are opaque
 .if numbytes == 16
        ldrb    SCRATCH, [MASK, #-12]
        over_reverse_8888_8888_8888_2pixels WK0, WK1, ORIG_W, SCRATCH, WK2, WK3, WK4, WK5, Y, STRIDE_D, -16
        ldmdb   SRC, {WK2, WK3}
        ldrb    ORIG_W, [MASK, #-8]
        ldrb    SCRATCH, [MASK, #-4]
        ldmdb   DST, {WK0, WK1}
        over_reverse_8888_8888_8888_2pixels WK2, WK3, ORIG_W, SCRATCH, WK0, WK1, WK4, WK5, Y, STRIDE_D, -8
 .elseif numbytes == 8
        ldrb    SCRATCH, [MASK, #-4]
        over_reverse_8888_8888_8888_2pixels WK0, WK1, ORIG_W, SCRATCH, WK2, WK3, WK4, WK5, Y, STRIDE_D, -8
 .else // numbytes == 4
        over_reverse_8888_8888_8888_1pixel WK0, ORIG_W, WK2, WK4, Y, STRIDE_D, -4
 .endif
10:
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_over_reverse_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_reverse_8888_8888_8888_init, \
    over_reverse_8888_8888_8888_newline, \
    over_reverse_8888_8888_8888_cleanup, \
    over_reverse_8888_8888_8888_process_head, \
    over_reverse_8888_8888_8888_process_tail

/******************************************************************************/

/* Multiply each byte of a word by a byte.
 * Useful when there aren't any obvious ways to fill the stalls with other instructions.
 * word  Register containing 4 bytes
 * byte  Register containing byte multiplier (bits 8-31 must be 0)
 * tmp   Scratch register
 * half  Register containing the constant 0x00800080
 * GE[3:0] bits must contain 0101
 */
.macro mul_8888_8  word, byte, tmp, half
        /* Split even/odd bytes of word apart */
        uxtb16  tmp, word
        uxtb16  word, word, ror #8
        /* Multiply bytes together with rounding, then by 257/256 */
        mla     tmp, tmp, byte, half
        mla     word, word, byte, half /* 1 stall follows */
        uxtab16 tmp, tmp, tmp, ror #8  /* 1 stall follows */
        uxtab16 word, word, word, ror #8
        /* Recombine bytes */
        mov     tmp, tmp, ror #8
        sel     word, tmp, word
.endm

/******************************************************************************/

.macro over_8888_n_8888_init
        /* Mask is constant */
        ldr     MASK, [sp, #ARGS_STACK_OFFSET+8]
        /* Hold loop invariant in STRIDE_M */
        ldr     STRIDE_M, =0x00800080
        /* We only want the alpha bits of the constant mask */
        mov     MASK, MASK, lsr #24
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, STRIDE_M, STRIDE_M
        line_saved_regs Y, STRIDE_D, STRIDE_S, ORIG_W
.endm

.macro over_8888_n_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
    WK4     .req    Y
    WK5     .req    STRIDE_D
    WK6     .req    STRIDE_S
    WK7     .req    ORIG_W
        pixld   , numbytes, %(4+(firstreg%2)), SRC, unaligned_src
        pixld   , numbytes, firstreg, DST, 0
    .unreq  WK4
    .unreq  WK5
    .unreq  WK6
    .unreq  WK7
.endm

.macro over_8888_n_8888_1pixel src, dst
        mul_8888_8  WK&src, MASK, SCRATCH, STRIDE_M
        sub     WK7, WK6, WK&src, lsr #24
        mul_8888_8  WK&dst, WK7, SCRATCH, STRIDE_M
        uqadd8  WK&dst, WK&dst, WK&src
.endm

.macro over_8888_n_8888_process_tail  cond, numbytes, firstreg
    WK4     .req    Y
    WK5     .req    STRIDE_D
    WK6     .req    STRIDE_S
    WK7     .req    ORIG_W
        over_8888_8888_check_transparent numbytes, %(4+(firstreg%2)), %(5+(firstreg%2)), %(6+firstreg), %(7+firstreg)
        beq     10f
        mov     WK6, #255
 .set PROCESS_REG, firstreg
 .rept numbytes / 4
  .if numbytes == 16 && PROCESS_REG == 2
        /* We're using WK6 and WK7 as temporaries, so half way through
         * 4 pixels, reload the second two source pixels but this time
         * into WK4 and WK5 */
        ldmdb   SRC, {WK4, WK5}
  .endif
        over_8888_n_8888_1pixel  %(4+(PROCESS_REG%2)), %(PROCESS_REG)
  .set PROCESS_REG, PROCESS_REG+1
 .endr
        pixst   , numbytes, firstreg, DST
10:
    .unreq  WK4
    .unreq  WK5
    .unreq  WK6
    .unreq  WK7
.endm

generate_composite_function \
    pixman_composite_over_8888_n_8888_asm_armv6, 32, 0, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS \
    2, /* prefetch distance */ \
    over_8888_n_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    over_8888_n_8888_process_head, \
    over_8888_n_8888_process_tail

/******************************************************************************/

.macro over_n_8_8888_init
        /* Source is constant, but splitting it into even/odd bytes is a loop invariant */
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        /* Not enough registers to hold this constant, but we still use it here to set GE[3:0] */
        ldr     SCRATCH, =0x00800080
        uxtb16  STRIDE_S, SRC
        uxtb16  SRC, SRC, ror #8
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, SCRATCH, SCRATCH
        line_saved_regs Y, STRIDE_D, STRIDE_M, ORIG_W
.endm

.macro over_n_8_8888_newline
        ldr     STRIDE_D, =0x00800080
        b       1f
 .ltorg
1:
.endm

.macro over_n_8_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
    WK4     .req    STRIDE_M
        pixld   , numbytes/4, 4, MASK, unaligned_mask
        pixld   , numbytes, firstreg, DST, 0
    .unreq  WK4
.endm

.macro over_n_8_8888_1pixel src, dst
        uxtb    Y, WK4, ror #src*8
        /* Trailing part of multiplication of source */
        mla     SCRATCH, STRIDE_S, Y, STRIDE_D
        mla     Y, SRC, Y, STRIDE_D
        mov     ORIG_W, #255
        uxtab16 SCRATCH, SCRATCH, SCRATCH, ror #8
        uxtab16 Y, Y, Y, ror #8
        mov     SCRATCH, SCRATCH, ror #8
        sub     ORIG_W, ORIG_W, Y, lsr #24
        sel     Y, SCRATCH, Y
        /* Then multiply the destination */
        mul_8888_8  WK&dst, ORIG_W, SCRATCH, STRIDE_D
        uqadd8  WK&dst, WK&dst, Y
.endm

.macro over_n_8_8888_process_tail  cond, numbytes, firstreg
    WK4     .req    STRIDE_M
        teq     WK4, #0
        beq     10f
 .set PROCESS_REG, firstreg
 .rept numbytes / 4
        over_n_8_8888_1pixel  %(PROCESS_REG-firstreg), %(PROCESS_REG)
  .set PROCESS_REG, PROCESS_REG+1
 .endr
        pixst   , numbytes, firstreg, DST
10:
    .unreq  WK4
.endm

generate_composite_function \
    pixman_composite_over_n_8_8888_asm_armv6, 0, 8, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS \
    2, /* prefetch distance */ \
    over_n_8_8888_init, \
    over_n_8_8888_newline, \
    nop_macro, /* cleanup */ \
    over_n_8_8888_process_head, \
    over_n_8_8888_process_tail

/******************************************************************************/

.macro over_reverse_n_8888_init
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        ldr     MASK, =0x00800080
        /* Split source pixel into RB/AG parts */
        uxtb16  STRIDE_S, SRC
        uxtb16  STRIDE_M, SRC, ror #8
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, MASK, MASK
        line_saved_regs  STRIDE_D, ORIG_W
.endm

.macro over_reverse_n_8888_newline
        mov     STRIDE_D, #0xFF
.endm

.macro over_reverse_n_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   , numbytes, firstreg, DST, 0
.endm

.macro over_reverse_n_8888_1pixel  d, is_only
        teq     WK&d, #0
        beq     8f       /* replace with source */
        bics    ORIG_W, STRIDE_D, WK&d, lsr #24
 .if is_only == 1
        beq     49f      /* skip store */
 .else
        beq     9f       /* write same value back */
 .endif
        mla     SCRATCH, STRIDE_S, ORIG_W, MASK /* red/blue */
        mla     ORIG_W, STRIDE_M, ORIG_W, MASK  /* alpha/green */
        uxtab16 SCRATCH, SCRATCH, SCRATCH, ror #8
        uxtab16 ORIG_W, ORIG_W, ORIG_W, ror #8
        mov     SCRATCH, SCRATCH, ror #8
        sel     ORIG_W, SCRATCH, ORIG_W
        uqadd8  WK&d, WK&d, ORIG_W
        b       9f
8:      mov     WK&d, SRC
9:
.endm

.macro over_reverse_n_8888_tail  numbytes, reg1, reg2, reg3, reg4
 .if numbytes == 4
        over_reverse_n_8888_1pixel  reg1, 1
 .else
        and     SCRATCH, WK&reg1, WK&reg2
  .if numbytes == 16
        and     SCRATCH, SCRATCH, WK&reg3
        and     SCRATCH, SCRATCH, WK&reg4
  .endif
        mvns    SCRATCH, SCRATCH, asr #24
        beq     49f /* skip store if all opaque */
        over_reverse_n_8888_1pixel  reg1, 0
        over_reverse_n_8888_1pixel  reg2, 0
  .if numbytes == 16
        over_reverse_n_8888_1pixel  reg3, 0
        over_reverse_n_8888_1pixel  reg4, 0
  .endif
 .endif
        pixst   , numbytes, reg1, DST
49:
.endm

.macro over_reverse_n_8888_process_tail  cond, numbytes, firstreg
        over_reverse_n_8888_tail  numbytes, firstreg, %(firstreg+1), %(firstreg+2), %(firstreg+3)
.endm

generate_composite_function \
    pixman_composite_over_reverse_n_8888_asm_armv6, 0, 0, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH, \
    3, /* prefetch distance */ \
    over_reverse_n_8888_init, \
    over_reverse_n_8888_newline, \
    nop_macro, /* cleanup */ \
    over_reverse_n_8888_process_head, \
    over_reverse_n_8888_process_tail

/******************************************************************************/

.macro over_white_8888_8888_ca_init
        HALF    .req    SRC
        TMP0    .req    STRIDE_D
        TMP1    .req    STRIDE_S
        TMP2    .req    STRIDE_M
        TMP3    .req    ORIG_W
        WK4     .req    SCRATCH
        line_saved_regs STRIDE_D, STRIDE_M, ORIG_W
        ldr     SCRATCH, =0x800080
        mov     HALF, #0x80
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, SCRATCH, SCRATCH
        .set DST_PRELOAD_BIAS, 8
.endm

.macro over_white_8888_8888_ca_cleanup
        .set DST_PRELOAD_BIAS, 0
        .unreq  HALF
        .unreq  TMP0
        .unreq  TMP1
        .unreq  TMP2
        .unreq  TMP3
        .unreq  WK4
.endm

.macro over_white_8888_8888_ca_combine  m, d
        uxtb16  TMP1, TMP0                /* rb_notmask */
        uxtb16  TMP2, d                   /* rb_dest; 1 stall follows */
        smlatt  TMP3, TMP2, TMP1, HALF    /* red */
        smlabb  TMP2, TMP2, TMP1, HALF    /* blue */
        uxtb16  TMP0, TMP0, ror #8        /* ag_notmask */
        uxtb16  TMP1, d, ror #8           /* ag_dest; 1 stall follows */
        smlatt  d, TMP1, TMP0, HALF       /* alpha */
        smlabb  TMP1, TMP1, TMP0, HALF    /* green */
        pkhbt   TMP0, TMP2, TMP3, lsl #16 /* rb; 1 stall follows */
        pkhbt   TMP1, TMP1, d, lsl #16    /* ag */
        uxtab16 TMP0, TMP0, TMP0, ror #8
        uxtab16 TMP1, TMP1, TMP1, ror #8
        mov     TMP0, TMP0, ror #8
        sel     d, TMP0, TMP1
        uqadd8  d, d, m                   /* d is a late result */
.endm

.macro over_white_8888_8888_ca_1pixel_head
        pixld   , 4, 1, MASK, 0
        pixld   , 4, 3, DST, 0
.endm

.macro over_white_8888_8888_ca_1pixel_tail
        mvn     TMP0, WK1
        teq     WK1, WK1, asr #32
        bne     01f
        bcc     03f
        mov     WK3, WK1
        b       02f
01:     over_white_8888_8888_ca_combine WK1, WK3
02:     pixst   , 4, 3, DST
03:
.endm

.macro over_white_8888_8888_ca_2pixels_head
        pixld   , 8, 1, MASK, 0
.endm

.macro over_white_8888_8888_ca_2pixels_tail
        pixld   , 8, 3, DST
        mvn     TMP0, WK1
        teq     WK1, WK1, asr #32
        bne     01f
        movcs   WK3, WK1
        bcs     02f
        teq     WK2, #0
        beq     05f
        b       02f
01:     over_white_8888_8888_ca_combine WK1, WK3
02:     mvn     TMP0, WK2
        teq     WK2, WK2, asr #32
        bne     03f
        movcs   WK4, WK2
        b       04f
03:     over_white_8888_8888_ca_combine WK2, WK4
04:     pixst   , 8, 3, DST
05:
.endm

.macro over_white_8888_8888_ca_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 4
        over_white_8888_8888_ca_1pixel_head
 .else
  .if numbytes == 16
        over_white_8888_8888_ca_2pixels_head
        over_white_8888_8888_ca_2pixels_tail
  .endif
        over_white_8888_8888_ca_2pixels_head
 .endif
.endm

.macro over_white_8888_8888_ca_process_tail  cond, numbytes, firstreg
 .if numbytes == 4
        over_white_8888_8888_ca_1pixel_tail
 .else
        over_white_8888_8888_ca_2pixels_tail
 .endif
.endm

generate_composite_function \
    pixman_composite_over_white_8888_8888_ca_asm_armv6, 0, 32, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH \
    2, /* prefetch distance */ \
    over_white_8888_8888_ca_init, \
    nop_macro, /* newline */ \
    over_white_8888_8888_ca_cleanup, \
    over_white_8888_8888_ca_process_head, \
    over_white_8888_8888_ca_process_tail


.macro over_n_8888_8888_ca_init
        /* Set up constants. RB_SRC and AG_SRC are in registers;
         * RB_FLDS, A_SRC, and the two HALF values need to go on the
         * stack (and the ful SRC value is already there) */
        ldr     SCRATCH, [sp, #ARGS_STACK_OFFSET]
        mov     WK0, #0x00FF0000
        orr     WK0, WK0, #0xFF        /* RB_FLDS (0x00FF00FF) */
        mov     WK1, #0x80             /* HALF default value */
        mov     WK2, SCRATCH, lsr #24  /* A_SRC */
        orr     WK3, WK1, WK1, lsl #16 /* HALF alternate value (0x00800080) */
        push    {WK0-WK3}
 .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET+16
        uxtb16  SRC, SCRATCH
        uxtb16  STRIDE_S, SCRATCH, ror #8

        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, WK3, WK3

        .unreq  WK0
        .unreq  WK1
        .unreq  WK2
        .unreq  WK3
        WK0     .req    Y
        WK1     .req    STRIDE_D
        RB_SRC  .req    SRC
        AG_SRC  .req    STRIDE_S
        WK2     .req    STRIDE_M
        RB_FLDS .req    r8       /* the reloaded constants have to be at consecutive registers starting at an even one */
        A_SRC   .req    r8
        HALF    .req    r9
        WK3     .req    r10
        WK4     .req    r11
        WK5     .req    SCRATCH
        WK6     .req    ORIG_W

        line_saved_regs Y, STRIDE_D, STRIDE_M, ORIG_W
.endm

.macro over_n_8888_8888_ca_cleanup
        add     sp, sp, #16
 .set ARGS_STACK_OFFSET, ARGS_STACK_OFFSET-16

        .unreq  WK0
        .unreq  WK1
        .unreq  RB_SRC
        .unreq  AG_SRC
        .unreq  WK2
        .unreq  RB_FLDS
        .unreq  A_SRC
        .unreq  HALF
        .unreq  WK3
        .unreq  WK4
        .unreq  WK5
        .unreq  WK6
        WK0     .req    r8
        WK1     .req    r9
        WK2     .req    r10
        WK3     .req    r11
.endm

.macro over_n_8888_8888_ca_1pixel_head
        pixld   , 4, 6, MASK, 0
        pixld   , 4, 0, DST, 0
.endm

.macro over_n_8888_8888_ca_1pixel_tail
        ldrd    A_SRC, HALF, [sp, #LOCALS_STACK_OFFSET+8]
        uxtb16  WK1, WK6                 /* rb_mask (first step of hard case placed in what would otherwise be a stall) */
        teq     WK6, WK6, asr #32        /* Zc if transparent, ZC if opaque */
        bne     20f
        bcc     40f
        /* Mask is fully opaque (all channels) */
        ldr     WK6, [sp, #ARGS_STACK_OFFSET] /* get SRC back */
        eors    A_SRC, A_SRC, #0xFF
        bne     10f
        /* Source is also opaque - same as src_8888_8888 */
        mov     WK0, WK6
        b       30f
10:     /* Same as over_8888_8888 */
        mul_8888_8 WK0, A_SRC, WK5, HALF
        uqadd8  WK0, WK0, WK6
        b       30f
20:     /* No simplifications possible - do it the hard way */
        uxtb16  WK2, WK6, ror #8         /* ag_mask */
        mla     WK3, WK1, A_SRC, HALF    /* rb_mul; 2 cycles */
        mla     WK4, WK2, A_SRC, HALF    /* ag_mul; 2 cycles */
        ldrd    RB_FLDS, HALF, [sp, #LOCALS_STACK_OFFSET]
        uxtb16  WK5, WK0                 /* rb_dest */
        uxtab16 WK3, WK3, WK3, ror #8
        uxtb16  WK6, WK0, ror #8         /* ag_dest */
        uxtab16 WK4, WK4, WK4, ror #8
        smlatt  WK0, RB_SRC, WK1, HALF   /* red1 */
        smlabb  WK1, RB_SRC, WK1, HALF   /* blue1 */
        bic     WK3, RB_FLDS, WK3, lsr #8
        bic     WK4, RB_FLDS, WK4, lsr #8
        pkhbt   WK1, WK1, WK0, lsl #16   /* rb1 */
        smlatt  WK0, WK5, WK3, HALF      /* red2 */
        smlabb  WK3, WK5, WK3, HALF      /* blue2 */
        uxtab16 WK1, WK1, WK1, ror #8
        smlatt  WK5, AG_SRC, WK2, HALF   /* alpha1 */
        pkhbt   WK3, WK3, WK0, lsl #16   /* rb2 */
        smlabb  WK0, AG_SRC, WK2, HALF   /* green1 */
        smlatt  WK2, WK6, WK4, HALF      /* alpha2 */
        smlabb  WK4, WK6, WK4, HALF      /* green2 */
        pkhbt   WK0, WK0, WK5, lsl #16   /* ag1 */
        uxtab16 WK3, WK3, WK3, ror #8
        pkhbt   WK4, WK4, WK2, lsl #16   /* ag2 */
        uxtab16 WK0, WK0, WK0, ror #8
        uxtab16 WK4, WK4, WK4, ror #8
        mov     WK1, WK1, ror #8
        mov     WK3, WK3, ror #8
        sel     WK2, WK1, WK0            /* recombine source*mask */
        sel     WK1, WK3, WK4            /* recombine dest*(1-source_alpha*mask) */
        uqadd8  WK0, WK1, WK2            /* followed by 1 stall */
30:     /* The destination buffer is already in the L1 cache, so
         * there's little point in amalgamating writes */
        pixst   , 4, 0, DST
40:
.endm

.macro over_n_8888_8888_ca_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .rept (numbytes / 4) - 1
        over_n_8888_8888_ca_1pixel_head
        over_n_8888_8888_ca_1pixel_tail
 .endr
        over_n_8888_8888_ca_1pixel_head
.endm

.macro over_n_8888_8888_ca_process_tail  cond, numbytes, firstreg
        over_n_8888_8888_ca_1pixel_tail
.endm

pixman_asm_function pixman_composite_over_n_8888_8888_ca_asm_armv6
        ldr     ip, [sp]
        cmp     ip, #-1
        beq     pixman_composite_over_white_8888_8888_ca_asm_armv6
        /* else drop through... */
 .endfunc
generate_composite_function \
    pixman_composite_over_n_8888_8888_ca_asm_armv6_helper, 0, 32, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH | FLAG_PROCESS_CORRUPTS_WK0 \
    2, /* prefetch distance */ \
    over_n_8888_8888_ca_init, \
    nop_macro, /* newline */ \
    over_n_8888_8888_ca_cleanup, \
    over_n_8888_8888_ca_process_head, \
    over_n_8888_8888_ca_process_tail

/******************************************************************************/

.macro in_reverse_8888_8888_init
        /* Hold loop invariant in MASK */
        ldr     MASK, =0x00800080
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, MASK, MASK
        /* Offset the source pointer: we only need the alpha bytes */
        add     SRC, SRC, #3
        line_saved_regs  ORIG_W
.endm

.macro in_reverse_8888_8888_head  numbytes, reg1, reg2, reg3
        ldrb    ORIG_W, [SRC], #4
 .if numbytes >= 8
        ldrb    WK&reg1, [SRC], #4
  .if numbytes == 16
        ldrb    WK&reg2, [SRC], #4
        ldrb    WK&reg3, [SRC], #4
  .endif
 .endif
        add     DST, DST, #numbytes
.endm

.macro in_reverse_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        in_reverse_8888_8888_head  numbytes, firstreg, %(firstreg+1), %(firstreg+2)
.endm

.macro in_reverse_8888_8888_1pixel  s, d, offset, is_only
 .if is_only != 1
        movs    s, ORIG_W
  .if offset != 0
        ldrb    ORIG_W, [SRC, #offset]
  .endif
        beq     01f
        teq     STRIDE_M, #0xFF
        beq     02f
 .endif
        uxtb16  SCRATCH, d                 /* rb_dest */
        uxtb16  d, d, ror #8               /* ag_dest */
        mla     SCRATCH, SCRATCH, s, MASK
        mla     d, d, s, MASK
        uxtab16 SCRATCH, SCRATCH, SCRATCH, ror #8
        uxtab16 d, d, d, ror #8
        mov     SCRATCH, SCRATCH, ror #8
        sel     d, SCRATCH, d
        b       02f
 .if offset == 0
48:     /* Last mov d,#0 of the set - used as part of shortcut for
         * source values all 0 */
 .endif
01:     mov     d, #0
02:
.endm

.macro in_reverse_8888_8888_tail  numbytes, reg1, reg2, reg3, reg4
 .if numbytes == 4
        teq     ORIG_W, ORIG_W, asr #32
        ldrne   WK&reg1, [DST, #-4]
 .elseif numbytes == 8
        teq     ORIG_W, WK&reg1
        teqeq   ORIG_W, ORIG_W, asr #32  /* all 0 or all -1? */
        ldmnedb DST, {WK&reg1-WK&reg2}
 .else
        teq     ORIG_W, WK&reg1
        teqeq   ORIG_W, WK&reg2
        teqeq   ORIG_W, WK&reg3
        teqeq   ORIG_W, ORIG_W, asr #32  /* all 0 or all -1? */
        ldmnedb DST, {WK&reg1-WK&reg4}
 .endif
        cmnne   DST, #0   /* clear C if NE */
        bcs     49f       /* no writes to dest if source all -1 */
        beq     48f       /* set dest to all 0 if source all 0 */
 .if numbytes == 4
        in_reverse_8888_8888_1pixel  ORIG_W, WK&reg1, 0, 1
        str     WK&reg1, [DST, #-4]
 .elseif numbytes == 8
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg1, -4, 0
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg2, 0, 0
        stmdb   DST, {WK&reg1-WK&reg2}
 .else
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg1, -12, 0
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg2, -8, 0
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg3, -4, 0
        in_reverse_8888_8888_1pixel  STRIDE_M, WK&reg4, 0, 0
        stmdb   DST, {WK&reg1-WK&reg4}
 .endif
49:
.endm

.macro in_reverse_8888_8888_process_tail  cond, numbytes, firstreg
        in_reverse_8888_8888_tail  numbytes, firstreg, %(firstreg+1), %(firstreg+2), %(firstreg+3)
.endm

generate_composite_function \
    pixman_composite_in_reverse_8888_8888_asm_armv6, 32, 0, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_SCRATCH | FLAG_NO_PRELOAD_DST \
    2, /* prefetch distance */ \
    in_reverse_8888_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    in_reverse_8888_8888_process_head, \
    in_reverse_8888_8888_process_tail

/******************************************************************************/

.macro over_n_8888_init
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        /* Hold loop invariant in MASK */
        ldr     MASK, =0x00800080
        /* Hold multiplier for destination in STRIDE_M */
        mov     STRIDE_M, #255
        sub     STRIDE_M, STRIDE_M, SRC, lsr #24
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, MASK, MASK
.endm

.macro over_n_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   , numbytes, firstreg, DST, 0
.endm

.macro over_n_8888_1pixel dst
        mul_8888_8  WK&dst, STRIDE_M, SCRATCH, MASK
        uqadd8  WK&dst, WK&dst, SRC
.endm

.macro over_n_8888_process_tail  cond, numbytes, firstreg
 .set PROCESS_REG, firstreg
 .rept numbytes / 4
        over_n_8888_1pixel %(PROCESS_REG)
  .set PROCESS_REG, PROCESS_REG+1
 .endr
        pixst   , numbytes, firstreg, DST
.endm

generate_composite_function \
    pixman_composite_over_n_8888_asm_armv6, 0, 0, 32 \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE \
    2, /* prefetch distance */ \
    over_n_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    over_n_8888_process_head, \
    over_n_8888_process_tail

/******************************************************************************/

.macro over_n_0565_init
        BITMSK5 .req    Y
        BITMSK6 .req    STRIDE_D
        SRCRB   .req    SRC
        SRCG    .req    STRIDE_S
        HALF    .req    MASK
        ALPHA   .req    STRIDE_M
        TMP0    .req    SCRATCH
        TMP1    .req    ORIG_W
        line_saved_regs  Y, STRIDE_D, ORIG_W
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        mov     ALPHA, #255
        pkhbt   SRCG, SRC, SRC, lsl #16             @ GGGGGGGGxxxxxxxxGGGGGGGGxxxxxxxx
        sub     ALPHA, ALPHA, SRC, lsr #24
        mov     SRCRB, SRC, lsl #8                  @ RRRRRRRRxxxxxxxxBBBBBBBBxxxxxxxx
        ldr     HALF, =0x00800080
.endm

.macro over_n_0565_newline
        ldr     BITMSK5, =0x001f001f
        ldr     BITMSK6, =0xfc00fc00
.endm

.macro over_n_0565_cleanup
        .unreq  BITMSK5
        .unreq  BITMSK6
        .unreq  SRCRB
        .unreq  SRCG
        .unreq  HALF
        .unreq  ALPHA
        .unreq  TMP0
        .unreq  TMP1
.endm

.macro over_n_0565_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
        pixld   , numbytes, firstreg, DST, 0
.endm

.macro over_n_0565_1pixel dst
        mov     TMP1, WK&dst, lsl #16               @ rrrrrggggggbbbbb0000000000000000
        bic     TMP1, TMP1, BITMSK6, lsr #5         @ rrrrr000000bbbbb0000000000000000
        and     TMP0, BITMSK6, WK&dst, lsl #5       @ 0000000000000000gggggg0000000000
        orr     WK&dst, TMP1, TMP1, lsr #5          @ rrrrrrrrrr0bbbbbbbbbb00000000000
        orr     TMP0, TMP0, lsr #6                  @ 0000000000000000gggggggggggg0000
        pkhtb   WK&dst, WK&dst, WK&dst, asr #5      @ rrrrrrrrrr0xxxxxbbbbbbbbbb000000
        uxtb    TMP0, TMP0, ror #8                  @ 000000000000000000000000gggggggg
        uxtb16  WK&dst, WK&dst, ror #8              @ 00000000rrrrrrrr00000000bbbbbbbb
        mla     TMP0, TMP0, ALPHA, HALF             @ xxxxxxxxxxxxxxxxgggggggggggggggg
        mla     WK&dst, WK&dst, ALPHA, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        uxtab   TMP0, TMP0, TMP0, ror #8            @ xxxxxxxxxxxxxxxxgggggggggggggggg
        uxtab16 WK&dst, WK&dst, WK&dst, ror #8      @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        uqadd8  TMP0, TMP0, SRCG                    @ xxxxxxxxxxxxxxxxggggggggxxxxxxxx
        uqadd8  WK&dst, WK&dst, SRCRB               @ rrrrrrrrxxxxxxxxbbbbbbbbxxxxxxxx
        and     TMP0, TMP0, BITMSK6                 @ xxxxxx0000000000gggggg0000000000
        and     WK&dst, BITMSK5, WK&dst, lsr #11    @ 00000000000rrrrr00000000000bbbbb
        orr     WK&dst, WK&dst, WK&dst, lsr #5      @ 00000000000xxxxxrrrrr000000bbbbb
        orr     WK&dst, WK&dst, TMP0, lsr #5        @ 00000xxxxxxxxxxxrrrrrggggggbbbbb
.endm

.macro over_n_0565_2pixels dst
        bic     TMP1, WK&dst, BITMSK6, lsr #5       @ RRRRR000000BBBBBrrrrr000000bbbbb
        and     TMP0, BITMSK6, WK&dst, lsl #5       @ GGGGGG0000000000gggggg0000000000
        mov     WK&dst, TMP1, lsl #16               @ rrrrr000000bbbbb0000000000000000
        orr     TMP0, TMP0, lsr #6                  @ GGGGGGGGGGGG0000gggggggggggg0000
        bic     TMP1, TMP1, WK&dst, lsr #16         @ RRRRR000000BBBBB0000000000000000
        orr     WK&dst, WK&dst, WK&dst, lsr #5      @ rrrrrrrrrr0bbbbbbbbbb00000000000
        orr     TMP1, TMP1, TMP1, lsr #5            @ RRRRRRRRRR0BBBBBBBBBB00000000000
        pkhtb   WK&dst, WK&dst, WK&dst, asr #5      @ rrrrrrrrrr0xxxxxbbbbbbbbbb000000
        pkhtb   TMP1, TMP1, TMP1, asr #5            @ RRRRRRRRRR0xxxxxBBBBBBBBBB000000
        uxtb16  TMP0, TMP0, ror #8                  @ 00000000GGGGGGGG00000000gggggggg
        uxtb16  WK&dst, WK&dst, ror #8              @ 00000000rrrrrrrr00000000bbbbbbbb
        uxtb16  TMP1, TMP1, ror #8                  @ 00000000RRRRRRRR00000000BBBBBBBB
        mla     TMP0, TMP0, ALPHA, HALF             @ GGGGGGGGGGGGGGGGgggggggggggggggg
        mla     WK&dst, WK&dst, ALPHA, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        mla     TMP1, TMP1, ALPHA, HALF             @ RRRRRRRRRRRRRRRRBBBBBBBBBBBBBBBB
        uxtab16 TMP0, TMP0, TMP0, ror #8            @ GGGGGGGGGGGGGGGGgggggggggggggggg
        uxtab16 WK&dst, WK&dst, WK&dst, ror #8      @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        uxtab16 TMP1, TMP1, TMP1, ror #8            @ RRRRRRRRRRRRRRRRBBBBBBBBBBBBBBBB
        uqadd8  TMP0, TMP0, SRCG                    @ GGGGGGGGxxxxxxxxggggggggxxxxxxxx
        uqadd8  TMP1, TMP1, SRCRB                   @ RRRRRRRRxxxxxxxxBBBBBBBBxxxxxxxx
        uqadd8  WK&dst, WK&dst, SRCRB               @ rrrrrrrrxxxxxxxxbbbbbbbbxxxxxxxx
        and     TMP0, TMP0, BITMSK6                 @ GGGGGG0000000000gggggg0000000000
        and     TMP1, BITMSK5, TMP1, lsr #11        @ 00000000000RRRRR00000000000BBBBB
        and     WK&dst, BITMSK5, WK&dst, lsr #11    @ 00000000000rrrrr00000000000bbbbb
        orr     TMP1, TMP1, TMP1, lsr #5            @ 00000000000xxxxxRRRRR000000BBBBB
        orr     WK&dst, WK&dst, WK&dst, lsr #5      @ 00000000000xxxxxrrrrr000000bbbbb
        pkhbt   TMP1, WK&dst, TMP1, LSL #16         @ RRRRR000000BBBBBrrrrr000000bbbbb
        orr     WK&dst, TMP1, TMP0, lsr #5          @ RRRRRGGGGGGBBBBBrrrrrggggggbbbbb
.endm

.macro over_n_0565_process_tail  cond, numbytes, firstreg
 .if numbytes == 2
        over_n_0565_1pixel firstreg
 .else
  .set PROCESS_REG, firstreg
  .rept numbytes / 4
        over_n_0565_2pixels %(PROCESS_REG)
   .set PROCESS_REG, PROCESS_REG+1
  .endr
 .endif
.endm

generate_composite_function \
    pixman_composite_over_n_0565_asm_armv6, 0, 0, 16, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_SPILL_LINE_VARS, \
    2, /* prefetch distance */ \
    over_n_0565_init, \
    over_n_0565_newline, \
    over_n_0565_cleanup, \
    over_n_0565_process_head, \
    over_n_0565_process_tail

/******************************************************************************/

.macro in_8888_8_init
        SRC0    .req   Y
        SRC1    .req   STRIDE_D
        SRC2    .req   STRIDE_S
        SRC3    .req   MASK
        HALF    .req   STRIDE_M
        TMP     .req   ORIG_W
        line_saved_regs  Y, STRIDE_D, STRIDE_S, ORIG_W
        ldr     SCRATCH, =0x00800080
        mov     HALF, #0x80
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, SCRATCH, SCRATCH
        /* Offset the source pointer: we only need the alpha bytes */
        add     SRC, SRC, #3
.endm

.macro in_8888_8_cleanup
        .unreq  SRC0
        .unreq  SRC1
        .unreq  SRC2
        .unreq  SRC3
        .unreq  HALF
        .unreq  TMP
.endm

.macro in_8888_8_4pixels_head dst
        ldr     TMP, [DST], #4
        ldrb    SRC0, [SRC], #12
        ldrb    SRC3, [SRC], #-4
        ldrb    SRC2, [SRC], #-4
        uxtb16  WK&dst, TMP
        uxtb16  TMP, TMP, ror #8
        ldrb    SRC1, [SRC], #12
        smlabb  SRC0, SRC0, WK&dst, HALF
        smlabt  SRC3, SRC3, TMP, HALF
        smlabt  SRC2, SRC2, WK&dst, HALF
        smlabb  SRC1, SRC1, TMP, HALF
        orr     WK&dst, SRC0, SRC2, lsl #16
        /* There'd be a stall here if immediately followed by orr, so
         * fill it with something like a preload if possible */
.endm

.macro in_8888_8_4pixels_tail dst
        orr     TMP, SRC1, SRC3, lsl #16
        uxtab16 WK&dst, WK&dst, WK&dst, ror #8
        uxtab16 TMP, TMP, TMP, ror #8
        mov     WK&dst, WK&dst, ror #8
        sel     WK&dst, WK&dst, TMP
.endm

.macro in_8888_8_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 1
        ldrb    WK3, [DST], #1
        ldrb    SRC0, [SRC], #4
 .elseif numbytes == 2
        ldrb    WK3, [DST], #1
        ldrb    SRC0, [SRC], #4
        ldrb    TMP, [DST], #1
        ldrb    SRC1, [SRC], #4
 .else
  .if numbytes >= 8
   .if numbytes == 16
        in_8888_8_4pixels_head 0
        in_8888_8_4pixels_tail 0
        in_8888_8_4pixels_head 1
    .if preload
        PF  bic,    SCRATCH, SRC, #31
        PF  pld,    [SCRATCH, #32*prefetch_distance]
    .endif
        in_8888_8_4pixels_tail 1
   .endif
        in_8888_8_4pixels_head 2
        in_8888_8_4pixels_tail 2
  .endif
        in_8888_8_4pixels_head 3
 .endif
.endm

.macro in_8888_8_process_tail  cond, numbytes, firstreg
 .if numbytes == 1
        smlabb  WK3, SRC0, WK3, HALF
        add     WK3, WK3, WK3, lsr #8
        mov     WK3, WK3, lsr #8
        strb    WK3, [DST, #-1]
 .elseif numbytes == 2
        smlabb  WK3, SRC0, WK3, HALF
        smlabb  TMP, SRC1, TMP, HALF
        add     WK3, WK3, WK3, lsr #8
        add     TMP, TMP, TMP, lsr #8
        mov     WK3, WK3, lsr #8
        mov     TMP, TMP, lsr #8
        strb    WK3, [DST, #-2]
        strb    TMP, [DST, #-1]
 .else
        in_8888_8_4pixels_tail 3
        pixst   , numbytes, (4-numbytes/4), DST
 .endif
.endm

generate_composite_function \
    pixman_composite_in_8888_8_asm_armv6, 32, 0, 8, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS, \
    3, /* prefetch distance */ \
    in_8888_8_init, \
    nop_macro, /* newline */ \
    in_8888_8_cleanup, \
    in_8888_8_process_head, \
    in_8888_8_process_tail

/******************************************************************************/

.macro over_8888_x_0565_init
        .unreq  WK2
        .unreq  WK3
        HALF    .req    Y
        WK2     .req    STRIDE_D
        WK3     .req    STRIDE_S
        WK4     .req    STRIDE_M
        WK5     .req    SCRATCH
        WK6     .req    ORIG_W
        WK7     .req    r10
        WK8     .req    r11
        BITMSK5 .req    WK7
        BITMSK6 .req    WK8
        line_saved_regs  Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro over_8888_x_0565_newline
        ldr     HALF, =0x00800080
        ldr     BITMSK5, =0x001f001f
        ldr     BITMSK6, =0xfc00fc00
.endm

.macro over_8888_x_0565_cleanup
        .unreq  HALF
        .unreq  WK2
        .unreq  WK3
        .unreq  WK4
        .unreq  WK5
        .unreq  WK6
        .unreq  WK7
        .unreq  WK8
        .unreq  BITMSK5
        .unreq  BITMSK6
        WK2     .req    r10
        WK3     .req    r11
.endm

.macro over_8888_x_0565_2pixels_head
        ldmia   SRC!, {WK0, WK1}
 .if OVER_8888_X_0565_SOLID
        ldrb    WK2, [sp, #ARGS_STACK_OFFSET + 8 + 3]
 .else
        ldrb    WK2, [MASK], #1
        ldrb    WK3, [MASK], #1
 .endif
        @ Because we'll be writing the destination in sub-cacheline
        @ chunks either way, it needs to be preloaded, so there's no
        @ penalty in loading its existing value even if it's unused
        ldr     WK4, [DST], #4
.endm

.set COUNTER, 0

.macro over_8888_x_0565_2pixels_tail
        uxtb16  WK5, WK0                    @ 00000000rrrrrrrr00000000bbbbbbbb
        uxtb16  WK0, WK0, ror #8            @ 00000000aaaaaaaa00000000gggggggg
        uxtb16  WK6, WK1                    @ 00000000RRRRRRRR00000000BBBBBBBB
        uxtb16  WK1, WK1, ror #8            @ 00000000AAAAAAAA00000000GGGGGGGG
        mla     WK5, WK5, WK2, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        mla     WK0, WK0, WK2, HALF         @ aaaaaaaaaaaaaaaagggggggggggggggg
 .if OVER_8888_X_0565_SOLID
        mla     WK6, WK6, WK2, HALF         @ RRRRRRRRRRRRRRRRBBBBBBBBBBBBBBBB
        mla     WK1, WK1, WK2, HALF         @ AAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG
 .else
        mla     WK6, WK6, WK3, HALF         @ RRRRRRRRRRRRRRRRBBBBBBBBBBBBBBBB
        mla     WK1, WK1, WK3, HALF         @ AAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG
 .endif
        uxtab16 WK5, WK5, WK5, ror #8
        uxtab16 WK0, WK0, WK0, ror #8
        uxtab16 WK6, WK6, WK6, ror #8
        uxtab16 WK1, WK1, WK1, ror #8
        mov     WK3, #0xFF
        eor     WK2, WK3, WK0, lsr #24      @ 000000000000000000000000aaaaaaaa
        eors    WK3, WK3, WK1, lsr #24      @ 000000000000000000000000AAAAAAAA
        pkhbt   WK0, WK0, WK1, lsl #16      @ GGGGGGGGGGGGGGGGgggggggggggggggg
        it      eq
        teqeq   WK2, #0
        beq     1f  @ skip processing of existing dest buffer if both pixels opaque
        bic     WK1, WK4, BITMSK6, lsr #5   @ RRRRR000000BBBBBrrrrr000000bbbbb
        and     WK7, BITMSK6, WK4, lsl #5   @ GGGGGG0000000000gggggg0000000000
        mov     WK4, WK1, lsl #16           @ rrrrr000000bbbbb0000000000000000
        orr     WK7, WK7, WK7, lsr #6       @ GGGGGGGGGGGG0000gggggggggggg0000
        bic     WK1, WK1, WK4, lsr #16      @ RRRRR000000BBBBB0000000000000000
        orr     WK4, WK4, WK4, lsr #5       @ rrrrrrrrrr0bbbbbbbbbb00000000000
        orr     WK1, WK1, WK1, lsr #5       @ RRRRRRRRRR0BBBBBBBBBB00000000000
        pkhtb   WK4, WK4, WK4, asr #5       @ rrrrrrrrrr0xxxxxbbbbbbbbbb000000
        pkhtb   WK1, WK1, WK1, asr #5       @ RRRRRRRRRR0xxxxxBBBBBBBBBB000000
        uxtb16  WK7, WK7, ror #8            @ 00000000GGGGGGGG00000000gggggggg
        uxtb16  WK4, WK4, ror #8            @ 00000000rrrrrrrr00000000bbbbbbbb
        uxtb16  WK1, WK1, ror #8            @ 00000000RRRRRRRR00000000BBBBBBBB
        smlabb  WK8, WK7, WK2, HALF         @ 00000000x0000000gggggggggggggggg
        smlatb  WK7, WK7, WK3, HALF         @ 00000000x0000000GGGGGGGGGGGGGGGG
        mla     WK4, WK4, WK2, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        mla     WK1, WK1, WK3, HALF         @ RRRRRRRRRRRRRRRRBBBBBBBBBBBBBBBB
        pkhbt   WK3, WK8, WK7, lsl #16      @ GGGGGGGGGGGGGGGGgggggggggggggggg
        ldr     BITMSK5, =0x001f001f
        ldr     BITMSK6, =0xfc00fc00
 .if COUNTER == 16
  .set COUNTER, 0
        b       0f
        .ltorg
0:
 .else
  .set COUNTER, COUNTER + 1
 .endif
        uxtab16 WK4, WK4, WK4, ror #8
        uxtab16 WK1, WK1, WK1, ror #8
        uxtab16 WK3, WK3, WK3, ror #8
        @ We could have used 16-bit saturating adds here for greater
        @ precision, but then it wouldn't be binary identical to the C version
        uqadd8  WK5, WK4, WK5               @ rrrrrrrrxxxxxxxxbbbbbbbbxxxxxxxx
        uqadd8  WK6, WK1, WK6               @ RRRRRRRRxxxxxxxxBBBBBBBBxxxxxxxx
        uqadd8  WK0, WK3, WK0               @ GGGGGGGGxxxxxxxxggggggggxxxxxxxx
1:      and     WK5, BITMSK5, WK5, lsr #11  @ 00000000000rrrrr00000000000bbbbb
        and     WK6, BITMSK5, WK6, lsr #11  @ 00000000000RRRRR00000000000BBBBB
        orr     WK5, WK5, WK5, lsr #5       @ 00000000000xxxxxrrrrr000000bbbbb
        orr     WK6, WK6, WK6, lsr #5       @ 00000000000xxxxxRRRRR000000BBBBB
        and     WK0, BITMSK6, WK0           @ GGGGGG0000000000gggggg0000000000
        pkhbt   WK3, WK5, WK6, lsl #16      @ RRRRR000000BBBBBrrrrr000000bbbbb
        orr     WK0, WK3, WK0, lsr #5       @ RRRRRGGGGGGBBBBBrrrrrggggggbbbbb
        str     WK0, [DST, #-4]
.endm

.macro over_8888_x_0565_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 2
        ldr     WK0, [SRC], #4
  .if OVER_8888_X_0565_SOLID
        ldrb    WK1, [sp, #ARGS_STACK_OFFSET + 8 + 3]
  .else
        ldrb    WK1, [MASK], #1
  .endif
        ldrh    WK2, [DST], #2
 .else
  .if numbytes >= 8
   .if numbytes == 16
        over_8888_x_0565_2pixels_head
        over_8888_x_0565_2pixels_tail
        over_8888_x_0565_2pixels_head
        over_8888_x_0565_2pixels_tail
   .endif
        over_8888_x_0565_2pixels_head
        over_8888_x_0565_2pixels_tail
  .endif
        over_8888_x_0565_2pixels_head
 .endif
.endm

.macro over_8888_x_0565_process_tail  cond, numbytes, firstreg
 .if numbytes == 2
        uxtb16  WK3, WK0                    @ 00000000rrrrrrrr00000000bbbbbbbb
        uxtb16  WK0, WK0, ror #8            @ 00000000aaaaaaaa00000000gggggggg
        mla     WK3, WK3, WK1, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        mla     WK0, WK0, WK1, HALF         @ aaaaaaaaaaaaaaaagggggggggggggggg
        mov     WK1, #0xFF
        uxtab16 WK3, WK3, WK3, ror #8
        uxtab16 WK0, WK0, WK0, ror #8
        eors    WK1, WK1, WK0, lsr #24      @ 000000000000000000000000aaaaaaaa
        beq     1f  @ skip processing of existing dest buffer if pixel opaque
        bic     WK4, WK2, BITMSK6, lsr #5   @ 0000000000000000rrrrr000000bbbbb
        and     WK2, BITMSK6, WK2, lsl #5   @ 0000000000000000gggggg0000000000
        mov     WK4, WK4, lsl #16           @ rrrrr000000bbbbb0000000000000000
        orr     WK4, WK4, WK4, lsr #5       @ rrrrrrrrrr0bbbbbbbbbb00000000000
        pkhtb   WK4, WK4, WK4, asr #5       @ rrrrrrrrrr0xxxxxbbbbbbbbbb000000
        orr     WK2, WK2, WK2, lsr #6       @ 0000000000000000gggggggggggg0000
        uxtb16  WK4, WK4, ror #8            @ 00000000rrrrrrrr00000000bbbbbbbb
        uxtb    WK2, WK2, ror #8            @ 000000000000000000000000gggggggg
        mla     WK4, WK4, WK1, HALF         @ rrrrrrrrrrrrrrrrbbbbbbbbbbbbbbbb
        smlabb  WK2, WK2, WK1, HALF         @ 00000000x0000000gggggggggggggggg
        uxtab16 WK4, WK4, WK4, ror #8
        uxtab   WK2, WK2, WK2, ror #8
        uqadd8  WK3, WK4, WK3               @ rrrrrrrrxxxxxxxxbbbbbbbbxxxxxxxx
        uqadd8  WK0, WK2, WK0               @ xxxxxxxxxxxxxxxxggggggggxxxxxxxx
1:      and     WK3, BITMSK5, WK3, lsr #11  @ 00000000000rrrrr00000000000bbbbb
        and     WK0, BITMSK6, WK0           @ xxxxxx0000000000gggggg0000000000
        orr     WK3, WK3, WK3, lsr #5       @ 00000000000xxxxxrrrrr000000bbbbb
        orr     WK0, WK3, WK0, lsr #5       @ 000000xxxxxxxxxxrrrrrggggggbbbbb
        strh    WK0, [DST, #-2]
 .else
        over_8888_x_0565_2pixels_tail
 .endif
.endm

.set OVER_8888_X_0565_SOLID, 0

generate_composite_function \
    pixman_composite_over_8888_8_0565_asm_armv6, 32, 8, 16, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_8888_x_0565_init, \
    over_8888_x_0565_newline, \
    over_8888_x_0565_cleanup, \
    over_8888_x_0565_process_head, \
    over_8888_x_0565_process_tail

/******************************************************************************/

.set OVER_8888_X_0565_SOLID, 1

generate_composite_function \
    pixman_composite_over_8888_n_0565_asm_armv6, 32, 0, 16, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_CORRUPTS_PSR | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    over_8888_x_0565_init, \
    over_8888_x_0565_newline, \
    over_8888_x_0565_cleanup, \
    over_8888_x_0565_process_head, \
    over_8888_x_0565_process_tail

/******************************************************************************/

.macro in_n_8888_init
        /* Source components and constant half are loop invariants */
        ldr     SRC, [sp, #ARGS_STACK_OFFSET]
        ldr     MASK, =0x00800080
        uxtb16  STRIDE_S, SRC    @ rb
        uxtb16  SRC, SRC, ror #8 @ ag
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        uadd8   SCRATCH, MASK, MASK
.endm

.macro in_n_8888_1pixel  dst, rb, ag, a
        mla     dst, rb, a, MASK
        mla     a, ag, a, MASK
        uxtab16 dst, dst, dst, ror #8
        uxtab16 a, a, a, ror #8
        mov     dst, dst, ror #8
        sel     dst, dst, a
.endm

.macro in_n_8888_2pixels  dst0, dst1, rb, ag, a0, a1
        mla     dst0, rb, a0, MASK
        mla     a0, ag, a0, MASK
        mla     dst1, rb, a1, MASK
        mla     a1, ag, a1, MASK
        uxtab16 dst0, dst0, dst0, ror #8
        uxtab16 a0, a0, a0, ror #8
        uxtab16 dst1, dst1, dst1, ror #8
        uxtab16 a1, a1, a1, ror #8
        mov     dst0, dst0, ror #8
        mov     dst1, dst1, ror #8
        sel     dst0, dst0, a0
        sel     dst1, dst1, a1
.endm

.macro in_n_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 4
        ldrb    SCRATCH, [DST, #3]
        add     DST, DST, #4
 .elseif numbytes == 8
        ldrb    SCRATCH, [DST, #3]
        ldrb    STRIDE_M, [DST, #7]
        add     DST, DST, #8
 .else // numbytes == 16
        ldrb    SCRATCH, [DST, #3] @ it's OK, SCRATCH isn't used for prefetch of dest buffer
        ldrb    STRIDE_M, [DST, #7]
        pixld   , 16, 0, DST, 0
 .endif
.endm

.macro in_n_8888_process_tail  cond, numbytes, firstreg
 .if numbytes == 4
        in_n_8888_1pixel WK3, STRIDE_S, SRC, SCRATCH
        pixst   , 4, 3, DST
 .elseif numbytes == 8
        in_n_8888_2pixels WK2, WK3, STRIDE_S, SRC, SCRATCH, STRIDE_M
        pixst   , 8, 2, DST
 .else // numbytes == 16
        in_n_8888_2pixels WK0, WK1, STRIDE_S, SRC, SCRATCH, STRIDE_M
        uxtb    SCRATCH, WK2, ror #24
        uxtb    STRIDE_M, WK3, ror #24
        in_n_8888_2pixels WK2, WK3, STRIDE_S, SRC, SCRATCH, STRIDE_M
        pixst   , 16, 0, DST
 .endif
.endm

generate_composite_function \
    pixman_composite_in_n_8888_asm_armv6, 0, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE, \
    2, /* prefetch distance */ \
    in_n_8888_init, \
    nop_macro, /* newline */ \
    nop_macro, /* cleanup */ \
    in_n_8888_process_head, \
    in_n_8888_process_tail

/******************************************************************************/

.macro add_8888_8888_8888_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000
        /* Point at alpha bytes in mask */
        add     MASK, MASK, #3
        line_saved_regs Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro add_8888_8888_8888_newline
        ldr     Y, =0x00800080
.endm

.macro add_8888_8888_8888_1pixel_head  d, s, m
        ldr     s, [SRC], #4
        ldrb    m, [MASK], #4
        ldr     WK&d, [DST], #4
.endm

.macro add_8888_8888_8888_1pixel_tail  d, s, m, tmp, half
        mul_8888_8 s, m, tmp, half
        uqadd8  WK&d, WK&d, s
.endm

.macro add_8888_8888_8888_2pixels_head  d0, d1, s0, s1, tmp0, tmp1, half
        ldm     SRC!, {s0, s1}
        ldrb    WK&d0, [MASK], #4
        ldrb    WK&d1, [MASK], #4
        uxtb16  tmp0, s0
        uxtb16  s0, s0, ror #8
        uxtb16  tmp1, s1
        uxtb16  s1, s1, ror #8
        mla     tmp0, tmp0, WK&d0, half
        mla     s0, s0, WK&d0, half
        mla     tmp1, tmp1, WK&d1, half
        mla     s1, s1, WK&d1, half
        ldm     DST!, {WK&d0, WK&d1}
.endm

.macro add_8888_8888_8888_2pixels_tail  d0, d1, s0, s1, tmp0, tmp1
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     s0, tmp0, s0
        sel     s1, tmp1, s1
        uqadd8  WK&d0, WK&d0, s0
        uqadd8  WK&d1, WK&d1, s1
.endm

.macro add_8888_8888_8888_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        add_8888_8888_8888_2pixels_head %(firstreg+0), %(firstreg+1), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W, Y
        add_8888_8888_8888_2pixels_tail %(firstreg+0), %(firstreg+1), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
        add_8888_8888_8888_2pixels_head %(firstreg+2), %(firstreg+3), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W, Y
 .elseif numbytes == 8
        add_8888_8888_8888_2pixels_head %(firstreg+0), %(firstreg+1), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W, Y
 .else // numbytes == 4
        add_8888_8888_8888_1pixel_head firstreg, STRIDE_S, STRIDE_M
 .endif
.endm

.macro add_8888_8888_8888_process_tail  cond, numbytes, firstreg
 .if numbytes == 16
        add_8888_8888_8888_2pixels_tail %(firstreg+2), %(firstreg+3), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
 .elseif numbytes == 8
        add_8888_8888_8888_2pixels_tail %(firstreg+0), %(firstreg+1), STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
 .else // numbytes == 4
        add_8888_8888_8888_1pixel_tail firstreg, STRIDE_S, STRIDE_M, ORIG_W, Y
 .endif
.endm

generate_composite_function_single_scanline \
    pixman_composite_scanline_add_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER, \
    2, /* prefetch distance */ \
    add_8888_8888_8888_init, \
    add_8888_8888_8888_newline, \
    nop_macro, /* cleanup */ \
    add_8888_8888_8888_process_head, \
    add_8888_8888_8888_process_tail

/******************************************************************************/

.macro inout_init
        /* Set GE[3:0] to 0101 so SEL instructions do what we want */
        msr     CPSR_s, #0x50000
        /* Point at alpha byte in source / mask / dest */
 .if REVERSE
        add     SRC, SRC, #3
 .endif
 .if WITH_MASK
        add     MASK, MASK, #3
 .endif
 .if !REVERSE
        add     DST, DST, #3
 .endif
        line_saved_regs Y, STRIDE_D, STRIDE_S, STRIDE_M, ORIG_W
.endm

.macro inout_newline
        ldr     Y, =0x00800080
        mov     STRIDE_D, #0xff
.endm

.macro inout_1pixel  s, m, d, tmp, half, ff, offset
        uxtb16  tmp, s
 .if !IN_NOT_OUT
        bic     d, ff, d
 .endif
        uxtb16  s, s, ror #8
 .if WITH_MASK
        mla     tmp, tmp, m, half
        mla     s, s, m, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 s, s, s, ror #8
        uxtb16  tmp, tmp, ror #8
        uxtb16  s, s, ror #8
 .endif
        mla     tmp, tmp, d, half
        mla     s, s, d, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 s, s, s, ror #8
        mov     tmp, tmp, ror #8
        sel     s, tmp, s
        str     s, [DST, #offset]
.endm

.macro inout_2pixels  s0, s1, m0, m1, d0, d1, tmp0, tmp1, half, ff, offset
        uxtb16  tmp0, s0
        uxtb16  s0, s0, ror #8
 .if !IN_NOT_OUT
        bic     d0, ff, d0
        bic     d1, ff, d1
 .endif
        uxtb16  tmp1, s1
        uxtb16  s1, s1, ror #8
 .if WITH_MASK
        mla     tmp0, tmp0, m0, half
        mla     s0, s0, m0, half
        mla     tmp1, tmp1, m1, half
        mla     s1, s1, m1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        uxtb16  tmp0, tmp0, ror #8
        uxtb16  s0, s0, ror #8
        uxtb16  tmp1, tmp1, ror #8
        uxtb16  s1, s1, ror #8
 .endif
        mla     tmp0, tmp0, d0, half
        mla     s0, s0, d0, half
        mla     tmp1, tmp1, d1, half
        mla     s1, s1, d1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 s0, s0, s0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 s1, s1, s1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     s0, tmp0, s0
        sel     s1, tmp1, s1
        strd    s0, s1, [DST, #offset]
.endm

.macro inout_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes >= 8
        pixld   , 8, 0, SRC, unaligned_src
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
  .endif
        ldrb    WK2, [DST], #4
        ldrb    WK3, [DST], #4
  .if numbytes == 16
        inout_2pixels  WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -11
        pixld   , 8, 0, SRC, unaligned_src
   .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
   .endif
        ldrb    WK2, [DST], #4
        ldrb    WK3, [DST], #4
  .endif
 .else // numbytes == 4
        pixld   , 4, 0, SRC, unaligned_src
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
  .endif
        ldrb    WK2, [DST], #4
 .endif
.endm

.macro inout_process_tail  cond, numbytes, firstreg
 .if numbytes >= 8
        inout_2pixels  WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -11
 .else // numbytes == 4
        inout_1pixel  WK0, STRIDE_S, WK2, SCRATCH, Y, STRIDE_D, -7
 .endif
.endm

.macro inout_reverse_1pixel  s, m, d, tmp, half, ff, offset
 .if WITH_MASK
        smlabb  s, s, m, half
        uxtb16  tmp, d
        uxtb16  d, d, ror #8
        uxtab   s, s, s, ror #8
  .if IN_NOT_OUT
        and     s, ff, s, lsr #8
  .else
        bic     s, ff, s, lsr #8
  .endif
 .else
  .if !IN_NOT_OUT
        bic     s, ff, s
  .endif
        uxtb16  tmp, d
        uxtb16  d, d, ror #8
 .endif
        mla     tmp, tmp, s, half
        mla     d, d, s, half
        uxtab16 tmp, tmp, tmp, ror #8
        uxtab16 d, d, d, ror #8
        mov     tmp, tmp, ror #8
        sel     d, tmp, d
        str     d, [DST, #offset]
.endm

.macro inout_reverse_2pixels  s0, s1, m0, m1, d0, d1, tmp0, tmp1, half, ff, offset
 .if WITH_MASK
        smlabb  s0, s0, m0, half
        smlabb  s1, s1, m1, half
        uxtb16  tmp0, d0
        uxtb16  d0, d0, ror #8
        uxtab   s0, s0, s0, ror #8
        uxtab   s1, s1, s1, ror #8
  .if IN_NOT_OUT
        and     s0, ff, s0, lsr #8
        and     s1, ff, s1, lsr #8
  .else
        bic     s0, ff, s0, lsr #8
        bic     s1, ff, s1, lsr #8
  .endif
 .else
  .if !IN_NOT_OUT
        bic     s0, ff, s0
        bic     s1, ff, s1
  .endif
        uxtb16  tmp0, d0
        uxtb16  d0, d0, ror #8
 .endif
        uxtb16  tmp1, d1
        uxtb16  d1, d1, ror #8
        mla     tmp0, tmp0, s0, half
        mla     d0, d0, s0, half
        mla     tmp1, tmp1, s1, half
        mla     d1, d1, s1, half
        uxtab16 tmp0, tmp0, tmp0, ror #8
        uxtab16 d0, d0, d0, ror #8
        uxtab16 tmp1, tmp1, tmp1, ror #8
        uxtab16 d1, d1, d1, ror #8
        mov     tmp0, tmp0, ror #8
        mov     tmp1, tmp1, ror #8
        sel     d0, tmp0, d0
        sel     d1, tmp1, d1
        strd    d0, d1, [DST, #offset]
.endm

.macro inout_reverse_process_head  cond, numbytes, firstreg, unaligned_src, unaligned_mask, preload
 .if numbytes == 16
        ldrb    WK0, [SRC], #4
        ldrb    WK1, [SRC], #4
        ldrb    SCRATCH, [SRC], #4
        ldrb    ORIG_W, [SRC], #4
  .if IN_NOT_OUT
        and     WK2, WK0, WK1
        and     WK2, WK2, SCRATCH
        and     WK2, WK2, ORIG_W
        bics    WK2, STRIDE_D, WK2
  .else
        orr     WK2, WK0, WK1
        orr     WK2, WK2, SCRATCH
        orrs    WK2, WK2, ORIG_W
  .endif
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
        ldrb    SCRATCH, [MASK], #4
        ldrb    ORIG_W, [MASK], #4
  .endif
        ldrd    WK2, WK3, [DST], #16
  .if WITH_MASK
        bne     10f
   .if IN_NOT_OUT
        and     SCRATCH, SCRATCH, ORIG_W
        and     SCRATCH, SCRATCH, STRIDE_S
        and     SCRATCH, SCRATCH, STRIDE_M
        bics    SCRATCH, STRIDE_D, SCRATCH
   .else
        orr     SCRATCH, SCRATCH, ORIG_W
        orr     SCRATCH, SCRATCH, STRIDE_S
        orrs    SCRATCH, SCRATCH, STRIDE_M
   .endif
  .endif
        beq     20f
10:
        inout_reverse_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -16
  .if IN_NOT_OUT && !WITH_MASK
        ldrd    WK2, WK3, [DST, #-8]
  .endif
        ldrb    WK0, [SRC, #-8]
        ldrb    WK1, [SRC, #-4]
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK, #-8]
        ldrb    STRIDE_M, [MASK, #-4]
  .endif
  .if !(IN_NOT_OUT && !WITH_MASK)
        ldrd    WK2, WK3, [DST, #-8]
  .endif
 .elseif numbytes == 8
  .if IN_NOT_OUT && !WITH_MASK
        ldrd    WK2, WK3, [DST], #8
  .endif
        ldrb    WK0, [SRC], #4
        ldrb    WK1, [SRC], #4
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
        ldrb    STRIDE_M, [MASK], #4
  .endif
  .if !(IN_NOT_OUT && !WITH_MASK)
        ldrd    WK2, WK3, [DST], #8
  .endif
 .else // numbytes == 4
  .if IN_NOT_OUT && !WITH_MASK
        ldr     WK2, [DST], #4
  .endif
        ldrb    WK0, [SRC], #4
  .if WITH_MASK
        ldrb    STRIDE_S, [MASK], #4
  .endif
  .if !(IN_NOT_OUT && !WITH_MASK)
        ldr     WK2, [DST], #4
  .endif
 .endif
.endm

.macro inout_reverse_process_tail  cond, numbytes, firstreg
 .if numbytes >= 8
        inout_reverse_2pixels WK0, WK1, STRIDE_S, STRIDE_M, WK2, WK3, SCRATCH, ORIG_W, Y, STRIDE_D, -8
 .else // numbytes == 4
        inout_reverse_1pixel WK0, STRIDE_S, WK2, SCRATCH, Y, STRIDE_D, -4
 .endif
20:
.endm

.set REVERSE, 0
.set IN_NOT_OUT, 0
.set WITH_MASK, 0

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_process_head, \
    inout_process_tail

.set WITH_MASK, 1

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_process_head, \
    inout_process_tail

.set IN_NOT_OUT, 1
.set WITH_MASK, 0

generate_composite_function_single_scanline \
    pixman_composite_scanline_in_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_process_head, \
    inout_process_tail

.set WITH_MASK, 1

generate_composite_function_single_scanline \
    pixman_composite_scanline_in_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_process_head, \
    inout_process_tail

.set REVERSE, 1
.set IN_NOT_OUT, 0
.set WITH_MASK, 0

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_reverse_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_reverse_process_head, \
    inout_reverse_process_tail

.set WITH_MASK, 1

generate_composite_function_single_scanline \
    pixman_composite_scanline_out_reverse_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_reverse_process_head, \
    inout_reverse_process_tail

.set IN_NOT_OUT, 1
.set WITH_MASK, 0

generate_composite_function_single_scanline \
    pixman_composite_scanline_in_reverse_asm_armv6, 32, 0, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_reverse_process_head, \
    inout_reverse_process_tail

.set WITH_MASK, 1

generate_composite_function_single_scanline \
    pixman_composite_scanline_in_reverse_mask_asm_armv6, 32, 32, 32, \
    FLAG_DST_READWRITE | FLAG_BRANCH_OVER | FLAG_PROCESS_DOES_STORE | FLAG_SPILL_LINE_VARS | FLAG_PROCESS_CORRUPTS_WK0, \
    2, /* prefetch distance */ \
    inout_init, \
    inout_newline, \
    nop_macro, /* cleanup */ \
    inout_reverse_process_head, \
    inout_reverse_process_tail

/******************************************************************************/
