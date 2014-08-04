/*
 * Copyright © 2008 Mozilla Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"
#include "pixman-arm-common.h"
#include "pixman-inlines.h"

PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_8888_8888,
		                   uint32_t, 1, uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_x888_8888,
                                   uint32_t, 1, uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_0565_0565,
                                   uint16_t, 1, uint16_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_8_8,
                                   uint8_t, 1, uint8_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_0565_8888,
                                   uint16_t, 1, uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_x888_0565,
                                   uint32_t, 1, uint16_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, src_1555_8888,
                                   uint16_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, add_8_8,
                                   uint8_t, 1, uint8_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, over_8888_8888,
                                   uint32_t, 1, uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, in_8888_8,
                                   uint32_t, 1, uint8_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_DST (armv6, in_reverse_8888_8888,
                                   uint32_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_N_DST (SKIP_ZERO_SRC, armv6, over_n_8888,
                                 uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST (SKIP_ZERO_SRC, armv6, over_n_0565,
                                 uint16_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST (0, armv6, over_reverse_n_8888,
                                 uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_N_DST (0, armv6, in_n_8888,
                                 uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, armv6, over_8888_n_8888,
                                     uint32_t, 1, uint32_t, 1)
PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST (SKIP_ZERO_MASK, armv6, over_8888_n_0565,
                                     uint32_t, 1, uint16_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, armv6, over_n_8_8888,
                                      uint8_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST (SKIP_ZERO_SRC, armv6, over_n_8888_8888_ca,
                                      uint32_t, 1, uint32_t, 1)

PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(armv6, over_8888_8_0565,
                                       uint32_t, 1, uint8_t, 1, uint16_t, 1)

PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST (armv6, 0565_0565, SRC,
                                        uint16_t, uint16_t)
PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST (armv6, 8888_8888, SRC,
                                        uint32_t, uint32_t)

void
pixman_composite_scanline_src_mask_asm_armv6 (int32_t         w,
                                              uint32_t       *dst,
                                              const uint32_t *src,
                                              const uint32_t *mask);

static void
armv6_combine_src_u (pixman_implementation_t *imp,
                     pixman_op_t              op,
                     uint32_t *               dest,
                     const uint32_t *         src,
                     const uint32_t *         mask,
                     int                      width)
{
    if (mask)
        pixman_composite_scanline_src_mask_asm_armv6 (width, dest, src, mask);
    else
        memcpy (dest, src, width * sizeof (uint32_t));
}

PIXMAN_ARM_BIND_COMBINE_U (armv6, over)
PIXMAN_ARM_BIND_COMBINE_U (armv6, over_reverse)
PIXMAN_ARM_BIND_COMBINE_U (armv6, in)
PIXMAN_ARM_BIND_COMBINE_U (armv6, in_reverse)
PIXMAN_ARM_BIND_COMBINE_U (armv6, out)
PIXMAN_ARM_BIND_COMBINE_U (armv6, out_reverse)
PIXMAN_ARM_BIND_COMBINE_U (armv6, add)

PIXMAN_ARM_BIND_GET_SCANLINE (armv6, x8r8g8b8)
PIXMAN_ARM_BIND_GET_SCANLINE (armv6, r5g6b5)
PIXMAN_ARM_BIND_WRITE_BACK   (armv6, r5g6b5)
PIXMAN_ARM_BIND_GET_SCANLINE (armv6, a1r5g5b5)
PIXMAN_ARM_BIND_GET_SCANLINE (armv6, a8)

static uint32_t *
fast_dest_fetch_noop (pixman_iter_t *iter, const uint32_t *mask)
{
    iter->bits += iter->stride;
    return iter->buffer;
}

#define DECLARE_NEAREST_SCALED_SCANLINE_FUNCTION(cputype, name, alias, type)   \
void                                                                           \
pixman_get_scanline_nearest_scaled_cover_##name##_asm_##cputype (              \
                                                 int32_t         width,        \
                                                 pixman_fixed_t  x,            \
                                                 pixman_fixed_t  ux,           \
                                                 uint32_t       *dest,         \
                                                 const type     *source,       \
                                                 const uint32_t *mask);

#define CALL_NEAREST_SCALED_SCANLINE_FUNCTION(                                 \
        cputype, name, alias, width, x, ux, dest, source, mask, source_width)  \
    pixman_get_scanline_nearest_scaled_cover_##name##_asm_##cputype (          \
        width, x, ux, dest, source, mask);

PIXMAN_ARM_BIND_GET_SCANLINE_NEAREST_SCALED_COVER (armv6, a8r8g8b8, 8888, uint32_t)
PIXMAN_ARM_BIND_GET_SCANLINE_NEAREST_SCALED_COVER (armv6, r5g6b5,   0565, uint16_t)

void
pixman_composite_src_n_8888_asm_armv6 (int32_t   w,
                                       int32_t   h,
                                       uint32_t *dst,
                                       int32_t   dst_stride,
                                       uint32_t  src);

void
pixman_composite_src_n_0565_asm_armv6 (int32_t   w,
                                       int32_t   h,
                                       uint16_t *dst,
                                       int32_t   dst_stride,
                                       uint16_t  src);

void
pixman_composite_src_n_8_asm_armv6 (int32_t   w,
                                    int32_t   h,
                                    uint8_t  *dst,
                                    int32_t   dst_stride,
                                    uint8_t  src);

static pixman_bool_t
arm_simd_fill (pixman_implementation_t *imp,
               uint32_t *               bits,
               int                      stride, /* in 32-bit words */
               int                      bpp,
               int                      x,
               int                      y,
               int                      width,
               int                      height,
               uint32_t                 _xor)
{
    /* stride is always multiple of 32bit units in pixman */
    uint32_t byte_stride = stride * sizeof(uint32_t);

    switch (bpp)
    {
    case 8:
	pixman_composite_src_n_8_asm_armv6 (
		width,
		height,
		(uint8_t *)(((char *) bits) + y * byte_stride + x),
		byte_stride,
		_xor & 0xff);
	return TRUE;
    case 16:
	pixman_composite_src_n_0565_asm_armv6 (
		width,
		height,
		(uint16_t *)(((char *) bits) + y * byte_stride + x * 2),
		byte_stride / 2,
		_xor & 0xffff);
	return TRUE;
    case 32:
	pixman_composite_src_n_8888_asm_armv6 (
		width,
		height,
		(uint32_t *)(((char *) bits) + y * byte_stride + x * 4),
		byte_stride / 4,
		_xor);
	return TRUE;
    default:
	return FALSE;
    }
}

static pixman_bool_t
arm_simd_blt (pixman_implementation_t *imp,
              uint32_t *               src_bits,
              uint32_t *               dst_bits,
              int                      src_stride, /* in 32-bit words */
              int                      dst_stride, /* in 32-bit words */
              int                      src_bpp,
              int                      dst_bpp,
              int                      src_x,
              int                      src_y,
              int                      dest_x,
              int                      dest_y,
              int                      width,
              int                      height)
{
    if (src_bpp != dst_bpp)
	return FALSE;

    switch (src_bpp)
    {
    case 8:
        pixman_composite_src_8_8_asm_armv6 (
                width, height,
                (uint8_t *)(((char *) dst_bits) +
                dest_y * dst_stride * 4 + dest_x * 1), dst_stride * 4,
                (uint8_t *)(((char *) src_bits) +
                src_y * src_stride * 4 + src_x * 1), src_stride * 4);
        return TRUE;
    case 16:
	pixman_composite_src_0565_0565_asm_armv6 (
		width, height,
		(uint16_t *)(((char *) dst_bits) +
		dest_y * dst_stride * 4 + dest_x * 2), dst_stride * 2,
		(uint16_t *)(((char *) src_bits) +
		src_y * src_stride * 4 + src_x * 2), src_stride * 2);
	return TRUE;
    case 32:
	pixman_composite_src_8888_8888_asm_armv6 (
		width, height,
		(uint32_t *)(((char *) dst_bits) +
		dest_y * dst_stride * 4 + dest_x * 4), dst_stride,
		(uint32_t *)(((char *) src_bits) +
		src_y * src_stride * 4 + src_x * 4), src_stride);
	return TRUE;
    default:
	return FALSE;
    }
}

static const pixman_fast_path_t arm_simd_fast_paths[] =
{
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, a8r8g8b8, armv6_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, a8b8g8r8, armv6_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, x8r8g8b8, armv6_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, x8b8g8r8, armv6_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, x8r8g8b8, armv6_composite_src_8888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, x8b8g8r8, armv6_composite_src_8888_8888),

    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, a8b8g8r8, armv6_composite_src_x888_8888),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, a8r8g8b8, armv6_composite_src_x888_8888),

    PIXMAN_STD_FAST_PATH (SRC, r5g6b5, null, r5g6b5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5, null, b5g6r5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a1r5g5b5, null, a1r5g5b5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a1b5g5r5, null, a1b5g5r5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a1r5g5b5, null, x1r5g5b5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a1b5g5r5, null, x1b5g5r5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, x1r5g5b5, null, x1r5g5b5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, x1b5g5r5, null, x1b5g5r5, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a4r4g4b4, null, a4r4g4b4, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a4b4g4r4, null, a4b4g4r4, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a4r4g4b4, null, x4r4g4b4, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, a4b4g4r4, null, x4b4g4r4, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, x4r4g4b4, null, x4r4g4b4, armv6_composite_src_0565_0565),
    PIXMAN_STD_FAST_PATH (SRC, x4b4g4r4, null, x4b4g4r4, armv6_composite_src_0565_0565),

    PIXMAN_STD_FAST_PATH (SRC, a8, null, a8, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, r3g3b2, null, r3g3b2, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, b2g3r3, null, b2g3r3, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, a2r2g2b2, null, a2r2g2b2, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, a2b2g2r2, null, a2b2g2r2, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, c8, null, c8, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, g8, null, g8, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, x4a4, null, x4a4, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, x4c4, null, x4c4, armv6_composite_src_8_8),
    PIXMAN_STD_FAST_PATH (SRC, x4g4, null, x4g4, armv6_composite_src_8_8),

    PIXMAN_STD_FAST_PATH (SRC, r5g6b5, null, a8r8g8b8, armv6_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, r5g6b5, null, x8r8g8b8, armv6_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5, null, a8b8g8r8, armv6_composite_src_0565_8888),
    PIXMAN_STD_FAST_PATH (SRC, b5g6r5, null, x8b8g8r8, armv6_composite_src_0565_8888),

    PIXMAN_STD_FAST_PATH (SRC, a8r8g8b8, null, r5g6b5, armv6_composite_src_x888_0565),
    PIXMAN_STD_FAST_PATH (SRC, x8r8g8b8, null, r5g6b5, armv6_composite_src_x888_0565),
    PIXMAN_STD_FAST_PATH (SRC, a8b8g8r8, null, b5g6r5, armv6_composite_src_x888_0565),
    PIXMAN_STD_FAST_PATH (SRC, x8b8g8r8, null, b5g6r5, armv6_composite_src_x888_0565),

    PIXMAN_STD_FAST_PATH (SRC, x1r5g5b5, null, x8r8g8b8, armv6_composite_src_1555_8888),
    PIXMAN_STD_FAST_PATH (SRC, a1r5g5b5, null, x8r8g8b8, armv6_composite_src_1555_8888),
    PIXMAN_STD_FAST_PATH (SRC, x1b5g5r5, null, x8b8g8r8, armv6_composite_src_1555_8888),
    PIXMAN_STD_FAST_PATH (SRC, a1b5g5r5, null, x8b8g8r8, armv6_composite_src_1555_8888),
    PIXMAN_STD_FAST_PATH (SRC, a1r5g5b5, null, a8r8g8b8, armv6_composite_src_1555_8888),
    PIXMAN_STD_FAST_PATH (SRC, a1b5g5r5, null, a8b8g8r8, armv6_composite_src_1555_8888),

    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, a8r8g8b8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, null, x8r8g8b8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, a8b8g8r8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, null, x8b8g8r8, armv6_composite_over_8888_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid, a8r8g8b8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid, x8r8g8b8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid, a8b8g8r8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid, x8b8g8r8, armv6_composite_over_8888_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, a8, r5g6b5, armv6_composite_over_8888_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, a8, b5g6r5, armv6_composite_over_8888_8_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8r8g8b8, solid, r5g6b5, armv6_composite_over_8888_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, a8b8g8r8, solid, b5g6r5, armv6_composite_over_8888_n_0565),

    PIXMAN_STD_FAST_PATH (OVER, solid, null, a8r8g8b8, armv6_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, null, x8r8g8b8, armv6_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, null, a8b8g8r8, armv6_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, null, x8b8g8r8, armv6_composite_over_n_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, null, r5g6b5, armv6_composite_over_n_0565),
    PIXMAN_STD_FAST_PATH (OVER, solid, null, b5g6r5, armv6_composite_over_n_0565),
    PIXMAN_STD_FAST_PATH (OVER_REVERSE, solid, null, a8r8g8b8, armv6_composite_over_reverse_n_8888),
    PIXMAN_STD_FAST_PATH (OVER_REVERSE, solid, null, a8b8g8r8, armv6_composite_over_reverse_n_8888),

    PIXMAN_STD_FAST_PATH (ADD, a8, null, a8, armv6_composite_add_8_8),

    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8r8g8b8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8r8g8b8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, a8b8g8r8, armv6_composite_over_n_8_8888),
    PIXMAN_STD_FAST_PATH (OVER, solid, a8, x8b8g8r8, armv6_composite_over_n_8_8888),

    PIXMAN_STD_FAST_PATH (IN, a8r8g8b8, null, a8, armv6_composite_in_8888_8),
    PIXMAN_STD_FAST_PATH (IN, a8b8g8r8, null, a8, armv6_composite_in_8888_8),
    PIXMAN_STD_FAST_PATH (IN, a8r8g8b8_sRGB, null, a8, armv6_composite_in_8888_8),
    PIXMAN_STD_FAST_PATH (IN, solid, null, a8r8g8b8, armv6_composite_in_n_8888),
    PIXMAN_STD_FAST_PATH (IN, solid, null, a8b8g8r8, armv6_composite_in_n_8888),
    PIXMAN_STD_FAST_PATH (IN_REVERSE, a8r8g8b8, null, a8r8g8b8, armv6_composite_in_reverse_8888_8888),
    PIXMAN_STD_FAST_PATH (IN_REVERSE, a8r8g8b8, null, x8r8g8b8, armv6_composite_in_reverse_8888_8888),
    PIXMAN_STD_FAST_PATH (IN_REVERSE, a8b8g8r8, null, a8b8g8r8, armv6_composite_in_reverse_8888_8888),
    PIXMAN_STD_FAST_PATH (IN_REVERSE, a8b8g8r8, null, x8b8g8r8, armv6_composite_in_reverse_8888_8888),

    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, a8r8g8b8, armv6_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8r8g8b8, x8r8g8b8, armv6_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, a8b8g8r8, armv6_composite_over_n_8888_8888_ca),
    PIXMAN_STD_FAST_PATH_CA (OVER, solid, a8b8g8r8, x8b8g8r8, armv6_composite_over_n_8888_8888_ca),

    SIMPLE_NEAREST_FAST_PATH (SRC, r5g6b5, r5g6b5, armv6_0565_0565),
    SIMPLE_NEAREST_FAST_PATH (SRC, b5g6r5, b5g6r5, armv6_0565_0565),

    SIMPLE_NEAREST_FAST_PATH (SRC, a8r8g8b8, a8r8g8b8, armv6_8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8r8g8b8, x8r8g8b8, armv6_8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, x8r8g8b8, x8r8g8b8, armv6_8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8b8g8r8, a8b8g8r8, armv6_8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, a8b8g8r8, x8b8g8r8, armv6_8888_8888),
    SIMPLE_NEAREST_FAST_PATH (SRC, x8b8g8r8, x8b8g8r8, armv6_8888_8888),

    { PIXMAN_OP_NONE },
};

static const pixman_iter_info_t arm_simd_iters[] =
{
    PIXMAN_ARM_NEAREST_SCALED_COVER_FETCHER (armv6, a8r8g8b8),

    PIXMAN_ARM_UNTRANSFORMED_COVER_FETCHER (armv6, x8r8g8b8),

    PIXMAN_ARM_UNTRANSFORMED_COVER_FETCHER (armv6, r5g6b5),
    PIXMAN_ARM_NEAREST_SCALED_COVER_FETCHER (armv6, r5g6b5),
    PIXMAN_ARM_WRITEBACK (armv6, r5g6b5),

    PIXMAN_ARM_UNTRANSFORMED_COVER_FETCHER (armv6, a1r5g5b5),

    PIXMAN_ARM_UNTRANSFORMED_COVER_FETCHER (armv6, a8),

    { PIXMAN_null },
};

pixman_implementation_t *
_pixman_implementation_create_arm_simd (pixman_implementation_t *fallback)
{
    pixman_implementation_t *imp = _pixman_implementation_create (fallback, arm_simd_fast_paths);

    imp->combine_32[PIXMAN_OP_SRC] = armv6_combine_src_u;
    imp->combine_32[PIXMAN_OP_OVER] = armv6_combine_over_u;
    imp->combine_32[PIXMAN_OP_OVER_REVERSE] = armv6_combine_over_reverse_u;
    imp->combine_32[PIXMAN_OP_IN] = armv6_combine_in_u;
    imp->combine_32[PIXMAN_OP_IN_REVERSE] = armv6_combine_in_reverse_u;
    imp->combine_32[PIXMAN_OP_OUT] = armv6_combine_out_u;
    imp->combine_32[PIXMAN_OP_OUT_REVERSE] = armv6_combine_out_reverse_u;
    imp->combine_32[PIXMAN_OP_ADD] = armv6_combine_add_u;

    imp->iter_info = arm_simd_iters;
    imp->blt = arm_simd_blt;
    imp->fill = arm_simd_fill;

    return imp;
}
