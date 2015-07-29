/*
 * Copyright © 2010 Nokia Corporation
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

#ifndef PIXMAN_ARM_COMMON_H
#define PIXMAN_ARM_COMMON_H

#include <stdlib.h>
#include "pixman-inlines.h"

/* Define some macros which can expand into proxy functions between
 * ARM assembly optimized functions and the rest of pixman fast path API.
 *
 * All the low level ARM assembly functions have to use ARM EABI
 * calling convention and take up to 8 arguments:
 *    width, height, dst, dst_stride, src, src_stride, mask, mask_stride
 *
 * The arguments are ordered with the most important coming first (the
 * first 4 arguments are passed to function in registers, the rest are
 * on stack). The last arguments are optional, for example if the
 * function is not using mask, then 'mask' and 'mask_stride' can be
 * omitted when doing a function call.
 *
 * Arguments 'src' and 'mask' contain either a pointer to the top left
 * pixel of the composited rectangle or a pixel color value depending
 * on the function type. In the case of just a color value (solid source
 * or mask), the corresponding stride argument is unused.
 */

#define SKIP_ZERO_SRC  1
#define SKIP_ZERO_MASK 2

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(cputype, name,                \
                                          src_type, src_cnt,            \
                                          dst_type, dst_cnt)            \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t   w,                   \
                                         int32_t   h,                   \
                                         dst_type *dst,                 \
                                         int32_t   dst_stride,          \
                                         src_type *src,                 \
                                         int32_t   src_stride);         \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type *dst_line;							\
    src_type *src_line;                                                 \
    int32_t dst_stride, src_stride;                                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride);     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_DST(flags, cputype, name,           \
                                        dst_type, dst_cnt)              \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src);               \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
			    pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);					\
    dst_type  *dst_line;                                                \
    int32_t    dst_stride;                                              \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (					\
	imp, src_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                            \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src);                      \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(flags, cputype, name,      \
                                             mask_type, mask_cnt,       \
                                             dst_type, dst_cnt)         \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src,                \
                                         int32_t    unused,             \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    mask_type *mask_line;                                               \
    int32_t    dst_stride, mask_stride;                                 \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (					\
	imp, src_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_SRC) && src == 0)                            \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src, 0,                    \
                                             mask_line, mask_stride);   \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(flags, cputype, name,       \
                                            src_type, src_cnt,          \
                                            dst_type, dst_cnt)          \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         uint32_t   mask);              \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    src_type  *src_line;                                                \
    int32_t    dst_stride, src_stride;                                  \
    uint32_t   mask;                                                    \
                                                                        \
    mask = _pixman_image_get_solid (					\
	imp, mask_image, dest_image->bits.format);			\
                                                                        \
    if ((flags & SKIP_ZERO_MASK) && mask == 0)                          \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask);                     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(cputype, name,           \
                                               src_type, src_cnt,       \
                                               mask_type, mask_cnt,     \
                                               dst_type, dst_cnt)       \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_composite_info_t *info)              \
{                                                                       \
    PIXMAN_COMPOSITE_ARGS (info);                                       \
    dst_type  *dst_line;						\
    src_type  *src_line;                                                \
    mask_type *mask_line;                                               \
    int32_t    dst_stride, src_stride, mask_stride;                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type,        \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask_line, mask_stride);   \
}

#define PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_DST(cputype, name, op,             \
                                               src_type, dst_type)            \
void                                                                          \
pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (                \
                                                   int32_t          w,        \
                                                   dst_type *       dst,      \
                                                   const src_type * src,      \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx);  \
                                                                              \
static force_inline void                                                      \
scaled_nearest_scanline_##cputype##_##name##_##op (dst_type *       pd,       \
                                                   const src_type * ps,       \
                                                   int32_t          w,        \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   pixman_bool_t    zero_src) \
{                                                                             \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (w, pd, ps,  \
                                                                  vx, unit_x, \
                                                                  max_vx);    \
}                                                                             \
                                                                              \
FAST_NEAREST_MAINLOOP (cputype##_##name##_cover_##op,                         \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, COVER)                             \
FAST_NEAREST_MAINLOOP (cputype##_##name##_none_##op,                          \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, NONE)                              \
FAST_NEAREST_MAINLOOP (cputype##_##name##_pad_##op,                           \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, PAD)                               \
FAST_NEAREST_MAINLOOP (cputype##_##name##_normal_##op,                        \
                       scaled_nearest_scanline_##cputype##_##name##_##op,     \
                       src_type, dst_type, NORMAL)

#define PIXMAN_ARM_BIND_SCALED_NEAREST_SRC_A8_DST(flags, cputype, name, op,   \
                                                  src_type, dst_type)         \
void                                                                          \
pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (                \
                                                   int32_t          w,        \
                                                   dst_type *       dst,      \
                                                   const src_type * src,      \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   const uint8_t *  mask);    \
                                                                              \
static force_inline void                                                      \
scaled_nearest_scanline_##cputype##_##name##_##op (const uint8_t *  mask,     \
                                                   dst_type *       pd,       \
                                                   const src_type * ps,       \
                                                   int32_t          w,        \
                                                   pixman_fixed_t   vx,       \
                                                   pixman_fixed_t   unit_x,   \
                                                   pixman_fixed_t   max_vx,   \
                                                   pixman_bool_t    zero_src) \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                               \
    pixman_scaled_nearest_scanline_##name##_##op##_asm_##cputype (w, pd, ps,  \
                                                                  vx, unit_x, \
                                                                  max_vx,     \
                                                                  mask);      \
}                                                                             \
                                                                              \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                  \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, COVER, TRUE, FALSE)\
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_none_##op,                   \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, NONE, TRUE, FALSE) \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                    \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, PAD, TRUE, FALSE)  \
FAST_NEAREST_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                 \
                              scaled_nearest_scanline_##cputype##_##name##_##op,\
                              src_type, uint8_t, dst_type, NORMAL, TRUE, FALSE)

/* Provide entries for the fast path table */
#define PIXMAN_ARM_SIMPLE_NEAREST_A8_MASK_FAST_PATH(op,s,d,func)              \
    SIMPLE_NEAREST_A8_MASK_FAST_PATH (op,s,d,func),                           \
    SIMPLE_NEAREST_A8_MASK_FAST_PATH_NORMAL (op,s,d,func)

/*****************************************************************************/

#define PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_DST(flags, cputype, name, op,     \
                                                src_type, dst_type)           \
void                                                                          \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (               \
                                                dst_type *       dst,         \
                                                const src_type * top,         \
                                                const src_type * bottom,      \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   x,           \
                                                pixman_fixed_t   ux,          \
                                                int              width);      \
                                                                              \
static force_inline void                                                      \
scaled_bilinear_scanline_##cputype##_##name##_##op (                          \
                                                dst_type *       dst,         \
                                                const uint32_t * mask,        \
                                                const src_type * src_top,     \
                                                const src_type * src_bottom,  \
                                                int32_t          w,           \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   vx,          \
                                                pixman_fixed_t   unit_x,      \
                                                pixman_fixed_t   max_vx,      \
                                                pixman_bool_t    zero_src)    \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                               \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (           \
                            dst, src_top, src_bottom, wt, wb, vx, unit_x, w); \
}                                                                             \
                                                                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                 \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, COVER, FLAG_NONE)        \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_none_##op,                  \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, NONE, FLAG_NONE)         \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                   \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, PAD, FLAG_NONE)          \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint32_t, dst_type, NORMAL,                  \
                       FLAG_NONE)


#define PIXMAN_ARM_BIND_SCALED_BILINEAR_SRC_A8_DST(flags, cputype, name, op,  \
                                                src_type, dst_type)           \
void                                                                          \
pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (               \
                                                dst_type *       dst,         \
                                                const uint8_t *  mask,        \
                                                const src_type * top,         \
                                                const src_type * bottom,      \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   x,           \
                                                pixman_fixed_t   ux,          \
                                                int              width);      \
                                                                              \
static force_inline void                                                      \
scaled_bilinear_scanline_##cputype##_##name##_##op (                          \
                                                dst_type *       dst,         \
                                                const uint8_t *  mask,        \
                                                const src_type * src_top,     \
                                                const src_type * src_bottom,  \
                                                int32_t          w,           \
                                                int              wt,          \
                                                int              wb,          \
                                                pixman_fixed_t   vx,          \
                                                pixman_fixed_t   unit_x,      \
                                                pixman_fixed_t   max_vx,      \
                                                pixman_bool_t    zero_src)    \
{                                                                             \
    if ((flags & SKIP_ZERO_SRC) && zero_src)                                  \
	return;                                                                   \
    pixman_scaled_bilinear_scanline_##name##_##op##_asm_##cputype (           \
                      dst, mask, src_top, src_bottom, wt, wb, vx, unit_x, w); \
}                                                                             \
                                                                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_cover_##op,                 \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, COVER,                    \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_none_##op,                  \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, NONE,                     \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_pad_##op,                   \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, PAD,                      \
                       FLAG_HAVE_NON_SOLID_MASK)                              \
FAST_BILINEAR_MAINLOOP_COMMON (cputype##_##name##_normal_##op,                \
                       scaled_bilinear_scanline_##cputype##_##name##_##op,    \
                       src_type, uint8_t, dst_type, NORMAL,                   \
                       FLAG_HAVE_NON_SOLID_MASK)

/*****************************************************************************/

#define PIXMAN_ARM_BIND_COMBINE_U(cputype, name)                              \
void                                                                          \
pixman_composite_scanline_##name##_mask_asm_##cputype (int32_t         w,     \
                                                       uint32_t       *dst,   \
                                                       const uint32_t *src,   \
                                                       const uint32_t *mask); \
                                                                              \
void                                                                          \
pixman_composite_scanline_##name##_asm_##cputype (int32_t         w,          \
                                                  uint32_t       *dst,        \
                                                  const uint32_t *src);       \
                                                                              \
static void                                                                   \
cputype##_combine_##name##_u (pixman_implementation_t *imp,                   \
                              pixman_op_t              op,                    \
                              uint32_t *               dest,                  \
                              const uint32_t *         src,                   \
                              const uint32_t *         mask,                  \
                              int                      width)                 \
{                                                                             \
    if (mask)                                                                 \
        pixman_composite_scanline_##name##_mask_asm_##cputype (width, dest,   \
                                                               src, mask);    \
    else                                                                      \
        pixman_composite_scanline_##name##_asm_##cputype (width, dest, src);  \
}

/*****************************************************************************/

/* Support for nearest scaled fetchers and fast paths */

/* PIXMAN_ARM_IMAGE_GET_SCALED is also used for bilinear fetchers */
#define PIXMAN_ARM_IMAGE_GET_SCALED(image, unscaled_x, unscaled_y, type, stride, out_bits, scaled_x, scaled_y, uxx, uxy, uyy) \
    do                                                                                           \
    {                                                                                            \
        pixman_image_t  *__image__      = (image);                                               \
        pixman_fixed_t   __offset__     = pixman_int_to_fixed (unscaled_x) + pixman_fixed_1 / 2; \
        pixman_fixed_t   __line__       = pixman_int_to_fixed (unscaled_y) + pixman_fixed_1 / 2; \
        pixman_fixed_t   __x__, __y__;                                                           \
        int64_t          __x64__, __y64__;                                                       \
        pixman_fixed_t (*__matrix__)[3] = __image__->common.transform->matrix;                   \
                                                                                                 \
        __x64__  = (int64_t) __matrix__[0][0] * (__offset__ & 0xFFFF);                           \
        __x64__ += (int64_t) __matrix__[0][1] * (__line__ & 0xFFFF);                             \
        __x__    = (__x64__ + 0x8000) >> 16;                                                     \
        __x__   += __matrix__[0][0] * (__offset__ >> 16);                                        \
        __x__   += __matrix__[0][1] * (__line__ >> 16);                                          \
        __x__   += __matrix__[0][2];                                                             \
        __y64__  = (int64_t) __matrix__[1][1] * (__line__ & 0xFFFF);                             \
        __y__    = (__y64__ + 0x8000) >> 16;                                                     \
        __y__   += __matrix__[1][1] * (__line__ >> 16);                                          \
        __y__   += __matrix__[1][2];                                                             \
                                                                                                 \
        (stride)   = __image__->bits.rowstride * (int) sizeof (uint32_t) / (int) sizeof (type);  \
        (out_bits) = (type *)__image__->bits.bits;                                               \
        (scaled_x) = __x__;                                                                      \
        (scaled_y) = __y__;                                                                      \
        (uxx)      = __matrix__[0][0];                                                           \
        (uxy)      = __matrix__[0][1];                                                           \
        (uyy)      = __matrix__[1][1];                                                           \
    } while (0)

#define PIXMAN_ARM_BIND_GET_SCANLINE_NEAREST_SCALED_COVER(cputype, name, alias, type)  \
                                                                            \
DECLARE_NEAREST_SCALED_SCANLINE_FUNCTION (cputype, name, alias, type)       \
                                                                            \
static uint32_t *                                                           \
cputype##_get_scanline_nearest_scaled_cover_##name (pixman_iter_t  *iter,   \
                                                    const uint32_t *mask)   \
{                                                                           \
    int            stride;                                                  \
    type          *bits, *source;                                           \
    pixman_fixed_t x, y, uxx, uxy, uyy;                                     \
                                                                            \
    PIXMAN_ARM_IMAGE_GET_SCALED (iter->image, iter->x, iter->y++, type,     \
                                 stride, bits, x, y, uxx, uxy, uyy);        \
                                                                            \
    (void) uxy;                                                             \
    (void) uyy;                                                             \
    source = bits + stride * pixman_fixed_to_int (y - pixman_fixed_e);      \
                                                                            \
    CALL_NEAREST_SCALED_SCANLINE_FUNCTION (                                 \
            cputype, name, alias,                                           \
            iter->width, x - pixman_fixed_e, uxx,                           \
            iter->buffer, source, mask, iter->image->bits.width);           \
                                                                            \
    return iter->buffer;                                                    \
}

#define PIXMAN_ARM_NEAREST_AFFINE_FLAGS                                 \
    (FAST_PATH_NO_ALPHA_MAP             |                               \
     FAST_PATH_NO_ACCESSORS             |                               \
     FAST_PATH_NARROW_FORMAT            |                               \
     FAST_PATH_NEAREST_FILTER           |                               \
     FAST_PATH_HAS_TRANSFORM            |                               \
     FAST_PATH_AFFINE_TRANSFORM)

#define PIXMAN_ARM_NEAREST_SCALED_COVER_FLAGS                           \
    (PIXMAN_ARM_NEAREST_AFFINE_FLAGS      |                             \
     FAST_PATH_SAMPLES_COVER_CLIP_NEAREST |                             \
     FAST_PATH_X_UNIT_POSITIVE            |                             \
     FAST_PATH_Y_UNIT_ZERO)

#define PIXMAN_ARM_NEAREST_SCALED_COVER_FETCHER(cputype, format)               \
    { PIXMAN_ ## format,                                                       \
      PIXMAN_ARM_NEAREST_SCALED_COVER_FLAGS,                                   \
      ITER_NARROW | ITER_SRC,                                                  \
      NULL,                                                                    \
      cputype ## _get_scanline_nearest_scaled_cover_ ## format,                \
      NULL                                                                     \
    }

#define PIXMAN_ARM_NEAREST_SCALED_COVER_SRC_DST_FAST_PATH(cputype,op,s,d,func) \
    {   PIXMAN_OP_ ## op,                                                      \
        PIXMAN_ ## s,                                                          \
        PIXMAN_ARM_NEAREST_SCALED_COVER_FLAGS,                                 \
        PIXMAN_null, 0,                                                        \
        PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,                                \
        cputype ## _composite_nearest_scaled_cover_ ## func                    \
    }

/*****************************************************************************/

/* Support for scaled bilinear fetchers */

#define PIXMAN_ARM_BILINEAR_AFFINE_FLAGS                                \
    (FAST_PATH_NO_ALPHA_MAP             |                               \
     FAST_PATH_NO_ACCESSORS             |                               \
     FAST_PATH_NARROW_FORMAT            |                               \
     FAST_PATH_BILINEAR_FILTER          |                               \
     FAST_PATH_HAS_TRANSFORM            |                               \
     FAST_PATH_AFFINE_TRANSFORM)

#define PIXMAN_ARM_BILINEAR_SCALED_COVER_FLAGS                          \
    (PIXMAN_ARM_BILINEAR_AFFINE_FLAGS            |                      \
     FAST_PATH_SAMPLES_COVER_CLIP_TIGHT_BILINEAR |                      \
     FAST_PATH_X_UNIT_POSITIVE                   |                      \
     FAST_PATH_SCALE_TRANSFORM) // implies FAST_PATH_Y_UNIT_ZERO

typedef void (*pixman_arm_bilinear_pass1_t) (uint32_t       width,
                                             pixman_fixed_t x,
                                             pixman_fixed_t ux,
                                             int16_t       *dest,
                                             const void    *source);

#ifndef __STDC_VERSION__
#define FLEXIBLE 1
#else
#if __STDC_VERSION__ >= 199901 // struct hack is illegal in C99, use flexible array member
#define FLEXIBLE
#else
#define FLEXIBLE 1
#endif
#endif

typedef struct
{
    pixman_arm_bilinear_pass1_t pass1;
    int                         line_y[2];
    int16_t                    *line_buffer;
    pixman_fixed_t              x;
    pixman_fixed_t              y;
    int                         stride;
    uint8_t                     data[FLEXIBLE];
} pixman_arm_bilinear_info_t;

#define ROUNDUP(x, n) (((x) + (n) - 1) &~ ((n) - 1))

#define ALIGNPTR(p, n) ((void *) ROUNDUP((uintptr_t) (p), (n)))

#define PIXMAN_ARM_DECLARE_BILINEAR_SCALED_SUPPORT(cputype)      \
                                                                 \
void                                                             \
pixman_get_scanline_bilinear_scaled_cover_pass2_asm_##cputype (  \
                                              uint32_t  width,   \
                                              uint32_t *dest,    \
                                              int16_t  *source,  \
                                              int16_t   dist_y); \
                                                                 \
void                                                             \
pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_##cputype ( \
                                              uint32_t  width,   \
                                              uint32_t *dest,    \
                                              int16_t  *source,  \
                                              int16_t   unused); \
                                                                 \
static void                                                      \
cputype##_get_scanline_bilinear_fini (pixman_iter_t *iter)       \
{                                                                \
    free (iter->data);                                           \
}                                                                \
                                                                 \
static inline void                                               \
cputype##_convert_adjacent_a8r8g8b8 (const void *void_source,    \
                                      int         x,             \
                                      uint32_t   *lag,           \
                                      uint32_t   *rag,           \
                                      uint32_t   *lrb,           \
                                      uint32_t   *rrb)           \
{                                                                \
    const uint32_t *source = void_source;                        \
    uint32_t left  = source[pixman_fixed_to_int (x)];            \
    uint32_t right;                                              \
    if (pixman_fixed_fraction (x) != 0)                          \
        right = source[pixman_fixed_to_int (x) + 1];             \
    *lag = (left & 0xff00ff00) >> 8;                             \
    *rag = (right & 0xff00ff00) >> 8;                            \
    *lrb = (left & 0x00ff00ff);                                  \
    *rrb = (right & 0x00ff00ff);                                 \
}                                                                \
                                                                 \
static inline void                                               \
cputype##_convert_adjacent_x8r8g8b8 (const void *void_source,    \
                                     int         x,              \
                                     uint32_t   *lag,            \
                                     uint32_t   *rag,            \
                                     uint32_t   *lrb,            \
                                     uint32_t   *rrb)            \
{                                                                \
    const uint32_t *source = void_source;                        \
    uint32_t left  = source[pixman_fixed_to_int (x)];            \
    uint32_t right;                                              \
    if (pixman_fixed_fraction (x) != 0)                          \
        right = source[pixman_fixed_to_int (x) + 1];             \
    *lag = ((left & 0xff00) >> 8) | 0x00ff0000;                  \
    *rag = ((right & 0xff00) >> 8) | 0x00ff0000;                 \
    *lrb = (left & 0x00ff00ff);                                  \
    *rrb = (right & 0x00ff00ff);                                 \
}                                                                \
                                                                 \
static inline void                                               \
cputype##_convert_adjacent_r5g6b5 (const void *void_source,      \
                                   int         x,                \
                                   uint32_t   *lag,              \
                                   uint32_t   *rag,              \
                                   uint32_t   *lrb,              \
                                   uint32_t   *rrb)              \
{                                                                \
    const uint16_t *source = void_source;                        \
    uint32_t r, g, b;                                            \
    uint32_t left  = source[pixman_fixed_to_int (x)];            \
    uint32_t right;                                              \
    if (pixman_fixed_fraction (x) != 0)                          \
        right = source[pixman_fixed_to_int (x) + 1];             \
    r = (left >> 8) & 0xf8;                                      \
    g = (left >> 3) & 0xfc;                                      \
    b = (left << 3) & 0xf8;                                      \
    r |= r >> 5;                                                 \
    g |= g >> 6;                                                 \
    b |= b >> 5;                                                 \
    *lag = 0xff0000 | g;                                         \
    *lrb = (r << 16) | b;                                        \
    r = (right >> 8) & 0xf8;                                     \
    g = (right >> 3) & 0xfc;                                     \
    b = (right << 3) & 0xf8;                                     \
    r |= r >> 5;                                                 \
    g |= g >> 6;                                                 \
    b |= b >> 5;                                                 \
    *rag = 0xff0000 | g;                                         \
    *rrb = (r << 16) | b;                                        \
}                                                                \
                                                                 \
static inline void                                               \
cputype##_convert_adjacent_a8 (const void *void_source,          \
                               int         x,                    \
                               uint32_t   *lag,                  \
                               uint32_t   *rag,                  \
                               uint32_t   *lrb,                  \
                               uint32_t   *rrb)                  \
{                                                                \
    const uint8_t *source = void_source;                         \
    uint32_t left  = source[pixman_fixed_to_int (x)];            \
    uint32_t right;                                              \
    if (pixman_fixed_fraction (x) != 0)                          \
        right = source[pixman_fixed_to_int (x) + 1];             \
    *lag = left << 16;                                           \
    *rag = right << 16;                                          \
    *lrb = 0;                                                    \
    *rrb = 0;                                                    \
}

#define PIXMAN_ARM_BIND_GET_SCANLINE_BILINEAR_SCALED_COMMON(cputype, name, type)            \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor0_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor1_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor2_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor3_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor4_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor5_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor6_asm_##cputype ();     \
void pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor7_asm_##cputype ();     \
                                                                                            \
static void                                                                                 \
cputype##_get_scanline_bilinear_scaled_cover_pass1_##name##_factor8 (                       \
                                uint32_t       width,                                       \
                                pixman_fixed_t x,                                           \
                                pixman_fixed_t ux,                                          \
                                int16_t       *dest,                                        \
                                const void    *source)                                      \
{                                                                                           \
    /* The preload scheme used by the assembly version relies on the                        \
     * reduction factor being less than 8x. Fall back to C. */                              \
    while (width--)                                                                         \
    {                                                                                       \
        uint32_t lag, rag, lrb, rrb, dist_x, ag, rb;                                        \
        cputype##_convert_adjacent_##name (source, x,  &lag, &rag, &lrb, &rrb);             \
        dist_x = (x & 0xFFFF) >> (16 - BILINEAR_INTERPOLATION_BITS);                        \
        ag     = (lag << BILINEAR_INTERPOLATION_BITS) + dist_x * (rag - lag);               \
        rb     = (lrb << BILINEAR_INTERPOLATION_BITS) + dist_x * (rrb - lrb);               \
        cputype##_store_part_interpolated (dest, ag, rb);                                   \
        dest += 4;                                                                          \
        if (((uintptr_t) dest & (PIXMAN_ARM_BILINEAR_GRANULE * 4 * 2 - 1)) == 0)            \
            dest += PIXMAN_ARM_BILINEAR_GRANULE * 4;                                        \
        x += ux;                                                                            \
    }                                                                                       \
}                                                                                           \

#define PIXMAN_ARM_BIND_GET_SCANLINE_BILINEAR_SCALED(cputype, name, type, repeat_mode)      \
static void                                                                                 \
cputype##_get_scanline_bilinear_scaled_##repeat_mode##_##name##_init (                      \
                                                       pixman_iter_t *iter,                 \
                                                       const pixman_iter_info_t *iter_info) \
{                                                                                           \
    int                         width = iter->width;                                        \
    pixman_arm_bilinear_info_t *info;                                                       \
    int                         stride;                                                     \
    type                       *bits;                                                       \
    pixman_fixed_t              x, y, uxx, uxy, uyy;                                        \
                                                                                            \
    PIXMAN_ARM_IMAGE_GET_SCALED (iter->image, iter->x, iter->y, type,                       \
                                 stride, bits, x, y, uxx, uxy, uyy);                        \
    (void) bits;                                                                            \
    (void) uxy;                                                                             \
    (void) uyy;                                                                             \
                                                                                            \
    info = malloc (offsetof (pixman_arm_bilinear_info_t, data) +                            \
                   PIXMAN_ARM_BILINEAR_GRANULE * 4 * 2 - 1 +                                \
                   2 * ROUNDUP (width, PIXMAN_ARM_BILINEAR_GRANULE) * 4 * 2);               \
    if (!info)                                                                              \
    {                                                                                       \
        /* In this case, we don't guarantee any particular rendering. */                    \
        _pixman_log_error (                                                                 \
            FUNC, "Allocation failure, skipping rendering\n");                              \
                                                                                            \
        iter->get_scanline = _pixman_iter_get_scanline_noop;                                \
        iter->fini = NULL;                                                                  \
        iter->data = NULL;                                                                  \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        static const pixman_arm_bilinear_pass1_t routines[9] = {                            \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor0_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor1_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor2_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor3_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor4_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor5_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor6_asm_##cputype, \
            pixman_get_scanline_bilinear_scaled_cover_pass1_##name##_factor7_asm_##cputype, \
            cputype##_get_scanline_bilinear_scaled_cover_pass1_##name##_factor8             \
        };                                                                                  \
        uxx >>= 16;                                                                         \
        if (uxx >= 8)                                                                       \
            uxx = 8;                                                                        \
        info->pass1 = routines[uxx];                                                        \
                                                                                            \
        /* It is safe to set the y coordinates to -1 initially                              \
         * because COVER_CLIP_BILINEAR ensures that we will only                            \
         * be asked to fetch lines in the [0, height) interval                              \
         */                                                                                 \
        info->line_y[0] = -1;                                                               \
        info->line_y[1] = -1;                                                               \
                                                                                            \
        info->line_buffer = ALIGNPTR (info->data, PIXMAN_ARM_BILINEAR_GRANULE * 4 * 2);     \
        info->x = x - pixman_fixed_1 / 2;                                                   \
        info->y = y - pixman_fixed_1 / 2;                                                   \
        info->stride = stride;                                                              \
                                                                                            \
        iter->fini = cputype##_get_scanline_bilinear_fini;                                  \
        iter->data = info;                                                                  \
    }                                                                                       \
}                                                                                           \
                                                                                            \
static uint32_t *                                                                           \
cputype##_get_scanline_bilinear_scaled_##repeat_mode##_##name (pixman_iter_t  *iter,        \
                                                               const uint32_t *mask)        \
{                                                                                           \
    pixman_arm_bilinear_info_t *info   = iter->data;                                        \
    int                         y0     = pixman_fixed_to_int (info->y);                     \
    int                         y1     = y0 + 1;                                            \
    int                         i;                                                          \
    int                         width  = iter->width;                                       \
    pixman_fixed_t              fx     = info->x;                                           \
    pixman_fixed_t              ux     = iter->image->common.transform->matrix[0][0];       \
    int16_t                    *buffer = info->line_buffer;                                 \
    type                       *bits   = (type *)iter->image->bits.bits;                    \
    int                         stride = info->stride;                                      \
    uint32_t                   *out    = iter->buffer;                                      \
    int32_t                     dist_y;                                                     \
    unsigned                    frac_bits;                                                  \
                                                                                            \
    frac_bits = BILINEAR_INTERPOLATION_BITS + PIXMAN_ARM_BILINEAR_PADDING_BITS;             \
    dist_y = (info->y >> (16 - frac_bits)) &                                                \
              ((1 << frac_bits) - (1 << PIXMAN_ARM_BILINEAR_PADDING_BITS));                 \
    info->y += iter->image->common.transform->matrix[1][1];                                 \
                                                                                            \
    i = y0 & 1;                                                                             \
    if (i)                                                                                  \
    {                                                                                       \
        /* Invert weight if upper scanline is in second buffer */                           \
        dist_y = (1 << frac_bits) - dist_y;                                                 \
    }                                                                                       \
                                                                                            \
    if (info->line_y[i] != y0)                                                              \
    {                                                                                       \
        info->pass1 (width, fx, ux,                                                         \
                     buffer + PIXMAN_ARM_BILINEAR_GRANULE * 4 * i,                          \
                     bits + stride * y0);                                                   \
        info->line_y[i] = y0;                                                               \
    }                                                                                       \
                                                                                            \
    if (dist_y & ((1 << frac_bits) - (1 << PIXMAN_ARM_BILINEAR_PADDING_BITS)))              \
    {                                                                                       \
        if (info->line_y[!i] != y1)                                                         \
        {                                                                                   \
            info->pass1 (width, fx, ux,                                                     \
                         buffer + PIXMAN_ARM_BILINEAR_GRANULE * 4 * !i,                     \
                         bits + stride * y1);                                               \
            info->line_y[!i] = y1;                                                          \
        }                                                                                   \
                                                                                            \
        pixman_get_scanline_bilinear_scaled_cover_pass2_asm_##cputype (                     \
            width, out, buffer, dist_y);                                                    \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        pixman_get_scanline_bilinear_scaled_cover_pass2a_asm_##cputype (                    \
            width, out, buffer + PIXMAN_ARM_BILINEAR_GRANULE * 4 * i, 0);                   \
    }                                                                                       \
                                                                                            \
    return out;                                                                             \
}

#define PIXMAN_ARM_BIND_GET_SCANLINE_BILINEAR_SCALED_COVER(cputype, name, type)             \
    PIXMAN_ARM_BIND_GET_SCANLINE_BILINEAR_SCALED_COMMON(cputype, name, type)                \
    PIXMAN_ARM_BIND_GET_SCANLINE_BILINEAR_SCALED(cputype, name, type, COVER)                \

#define PIXMAN_ARM_BILINEAR_SCALED_COVER_FETCHER(cputype, format)               \
    { PIXMAN_ ## format,                                                        \
      PIXMAN_ARM_BILINEAR_SCALED_COVER_FLAGS,                                   \
      ITER_NARROW | ITER_SRC,                                                   \
      cputype ## _get_scanline_bilinear_scaled_COVER_ ## format ## _init,       \
      cputype ## _get_scanline_bilinear_scaled_COVER_ ## format,                \
      NULL                                                                      \
    }

#define PIXMAN_ARM_BILINEAR_SCALED_FETCHER(cputype, format)                     \
     PIXMAN_ARM_BILINEAR_SCALED_COVER_FETCHER (cputype, format)

/*****************************************************************************/

/* Support for untransformed fetchers and writeback */

#define PIXMAN_ARM_BIND_GET_SCANLINE(cputype, name)                         \
void                                                                        \
pixman_get_scanline_##name##_asm_##cputype (int32_t        w,               \
                                            uint32_t       *dst,            \
                                            const uint32_t *src);           \
                                                                            \
uint32_t *                                                                  \
cputype##_get_scanline_##name (pixman_iter_t *iter, const uint32_t *mask)   \
{                                                                           \
    pixman_get_scanline_##name##_asm_##cputype (iter->width, iter->buffer,  \
                                                (uint32_t *) iter->bits);   \
    iter->bits += iter->stride;                                             \
    return iter->buffer;                                                    \
}

#define PIXMAN_ARM_BIND_WRITE_BACK(cputype, name)                                      \
void                                                                                   \
pixman_write_back_##name##_asm_##cputype (int32_t        w,                            \
                                          uint32_t       *dst,                         \
                                          const uint32_t *src);                        \
                                                                                       \
void                                                                                   \
cputype##_write_back_##name (pixman_iter_t *iter)                                      \
{                                                                                      \
    pixman_write_back_##name##_asm_##cputype (iter->width,                             \
                                              (uint32_t *)(iter->bits - iter->stride), \
                                              iter->buffer);                           \
}

#define PIXMAN_ARM_UNTRANSFORMED_COVER_FETCHER(cputype, format)      \
    { PIXMAN_ ## format,                                             \
      (FAST_PATH_STANDARD_FLAGS             |                        \
       FAST_PATH_ID_TRANSFORM               |                        \
       FAST_PATH_SAMPLES_COVER_CLIP_NEAREST |                        \
       FAST_PATH_BITS_IMAGE),                                        \
      ITER_NARROW | ITER_SRC,                                        \
      _pixman_iter_init_bits_stride,                                 \
      cputype ## _get_scanline_ ## format,                           \
      NULL                                                           \
    }

#define PIXMAN_ARM_WRITEBACK(cputype, format)                        \
    { PIXMAN_ ## format,                                             \
      FAST_PATH_STD_DEST_FLAGS,                                      \
      ITER_NARROW | ITER_DEST | ITER_IGNORE_RGB | ITER_IGNORE_ALPHA, \
      _pixman_iter_init_bits_stride,                                 \
      fast_dest_fetch_noop,                                          \
      cputype ## _write_back_ ## format                              \
    },                                                               \
                                                                     \
    { PIXMAN_ ## format,                                             \
      FAST_PATH_STD_DEST_FLAGS,                                      \
      ITER_NARROW | ITER_DEST,                                       \
      _pixman_iter_init_bits_stride,                                 \
      cputype ## _get_scanline_ ## format,                           \
      cputype ## _write_back_ ## format                              \
    }

#endif
