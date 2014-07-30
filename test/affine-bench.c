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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "utils.h"

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#else
#include <time.h>
#endif

#define WIDTH  1920
#define HEIGHT 1080
#define L2CACHE_SIZE (128 * 1024)

#define CAT(x,y) x##y
#define FORMAT_ENTRY(prefix,name) { #name, CAT(prefix,name) }

typedef struct
{
    pixman_fixed_48_16_t        x1;
    pixman_fixed_48_16_t        y1;
    pixman_fixed_48_16_t        x2;
    pixman_fixed_48_16_t        y2;
} box_48_16_t;

struct name_to_number { const char *name; uint32_t number; };

static const struct name_to_number combine_type[] = {
    FORMAT_ENTRY(PIXMAN_OP_,CLEAR),
    FORMAT_ENTRY(PIXMAN_OP_,SRC),
    FORMAT_ENTRY(PIXMAN_OP_,DST),
    FORMAT_ENTRY(PIXMAN_OP_,OVER),
    FORMAT_ENTRY(PIXMAN_OP_,OVER_REVERSE),
    FORMAT_ENTRY(PIXMAN_OP_,IN),
    FORMAT_ENTRY(PIXMAN_OP_,IN_REVERSE),
    FORMAT_ENTRY(PIXMAN_OP_,OUT),
    FORMAT_ENTRY(PIXMAN_OP_,OUT_REVERSE),
    FORMAT_ENTRY(PIXMAN_OP_,ATOP),
    FORMAT_ENTRY(PIXMAN_OP_,ATOP_REVERSE),
    FORMAT_ENTRY(PIXMAN_OP_,XOR),
    FORMAT_ENTRY(PIXMAN_OP_,ADD)
};

static const struct name_to_number format[] = {
    FORMAT_ENTRY(PIXMAN_,a8r8g8b8),
    FORMAT_ENTRY(PIXMAN_,x8r8g8b8),
    FORMAT_ENTRY(PIXMAN_,a8b8g8r8),
    FORMAT_ENTRY(PIXMAN_,x8b8g8r8),
    FORMAT_ENTRY(PIXMAN_,b8g8r8a8),
    FORMAT_ENTRY(PIXMAN_,b8g8r8x8),
    FORMAT_ENTRY(PIXMAN_,r8g8b8a8),
    FORMAT_ENTRY(PIXMAN_,r8g8b8x8),
    FORMAT_ENTRY(PIXMAN_,x14r6g6b6),
    FORMAT_ENTRY(PIXMAN_,x2r10g10b10),
    FORMAT_ENTRY(PIXMAN_,a2r10g10b10),
    FORMAT_ENTRY(PIXMAN_,x2b10g10r10),
    FORMAT_ENTRY(PIXMAN_,a2b10g10r10),
    FORMAT_ENTRY(PIXMAN_,a8r8g8b8_sRGB),
    FORMAT_ENTRY(PIXMAN_,r8g8b8),
    FORMAT_ENTRY(PIXMAN_,b8g8r8),
    FORMAT_ENTRY(PIXMAN_,r5g6b5),
    FORMAT_ENTRY(PIXMAN_,b5g6r5),
    FORMAT_ENTRY(PIXMAN_,a1r5g5b5),
    FORMAT_ENTRY(PIXMAN_,x1r5g5b5),
    FORMAT_ENTRY(PIXMAN_,a1b5g5r5),
    FORMAT_ENTRY(PIXMAN_,x1b5g5r5),
    FORMAT_ENTRY(PIXMAN_,a4r4g4b4),
    FORMAT_ENTRY(PIXMAN_,x4r4g4b4),
    FORMAT_ENTRY(PIXMAN_,a4b4g4r4),
    FORMAT_ENTRY(PIXMAN_,x4b4g4r4),
    FORMAT_ENTRY(PIXMAN_,a8),
    FORMAT_ENTRY(PIXMAN_,r3g3b2),
    FORMAT_ENTRY(PIXMAN_,b2g3r3),
    FORMAT_ENTRY(PIXMAN_,a2r2g2b2),
    FORMAT_ENTRY(PIXMAN_,a2b2g2r2),
    FORMAT_ENTRY(PIXMAN_,c8),
    FORMAT_ENTRY(PIXMAN_,g8),
    FORMAT_ENTRY(PIXMAN_,x4a4),
    FORMAT_ENTRY(PIXMAN_,x4c4),
    FORMAT_ENTRY(PIXMAN_,x4g4),
    FORMAT_ENTRY(PIXMAN_,a4),
    FORMAT_ENTRY(PIXMAN_,r1g2b1),
    FORMAT_ENTRY(PIXMAN_,b1g2r1),
    FORMAT_ENTRY(PIXMAN_,a1r1g1b1),
    FORMAT_ENTRY(PIXMAN_,a1b1g1r1),
    FORMAT_ENTRY(PIXMAN_,c4),
    FORMAT_ENTRY(PIXMAN_,g4),
    FORMAT_ENTRY(PIXMAN_,a1),
    FORMAT_ENTRY(PIXMAN_,g1),
    FORMAT_ENTRY(PIXMAN_,yuy2),
    FORMAT_ENTRY(PIXMAN_,yv12)
};

static uint32_t
lookup_name_to_number (const char *string, const struct name_to_number *array, size_t entries)
{
    size_t entry, i;
    for (entry = 0; entry < entries; entry++)
    {
        for (i = 0; tolower (string[i]) == tolower (array[entry].name[i]); i++)
            if (string[i] == 0)
                return array[entry].number;
    }
    return 0;
}

static pixman_bool_t
compute_transformed_extents (pixman_transform_t *transform,
                             const pixman_box32_t *extents,
                             box_48_16_t *transformed)
{
    pixman_fixed_48_16_t tx1, ty1, tx2, ty2;
    pixman_fixed_t x1, y1, x2, y2;
    int i;

    x1 = pixman_int_to_fixed (extents->x1) + pixman_fixed_1 / 2;
    y1 = pixman_int_to_fixed (extents->y1) + pixman_fixed_1 / 2;
    x2 = pixman_int_to_fixed (extents->x2) - pixman_fixed_1 / 2;
    y2 = pixman_int_to_fixed (extents->y2) - pixman_fixed_1 / 2;

    if (!transform)
    {
        transformed->x1 = x1;
        transformed->y1 = y1;
        transformed->x2 = x2;
        transformed->y2 = y2;

        return TRUE;
    }

    tx1 = ty1 = INT64_MAX;
    tx2 = ty2 = INT64_MIN;

    for (i = 0; i < 4; ++i)
    {
        pixman_fixed_48_16_t tx, ty;
        pixman_vector_t v;

        v.vector[0] = (i & 0x01)? x1 : x2;
        v.vector[1] = (i & 0x02)? y1 : y2;
        v.vector[2] = pixman_fixed_1;

        if (!pixman_transform_point (transform, &v))
            return FALSE;

        tx = (pixman_fixed_48_16_t)v.vector[0];
        ty = (pixman_fixed_48_16_t)v.vector[1];

        if (tx < tx1)
            tx1 = tx;
        if (ty < ty1)
            ty1 = ty;
        if (tx > tx2)
            tx2 = tx;
        if (ty > ty2)
            ty2 = ty;
    }

    transformed->x1 = tx1;
    transformed->y1 = ty1;
    transformed->x2 = tx2;
    transformed->y2 = ty2;

    return TRUE;
}

static void
create_image (uint32_t width, uint32_t height, pixman_format_code_t format, pixman_filter_t filter, const pixman_transform_t *t, uint32_t **bits, pixman_image_t **image)
{
    uint32_t stride = (width * PIXMAN_FORMAT_BPP(format) + 31) / 32 * 4;
    *bits = aligned_malloc (4096, stride * height);
    memset (*bits, 0xCC, stride * height);
    *image = pixman_image_create_bits (format, width, height, *bits, stride);
    pixman_image_set_repeat (*image, PIXMAN_REPEAT_NORMAL);
    pixman_image_set_filter (*image, filter, NULL, 0);
}

static void
flush_cache (void)
{
    static const char clean_space[L2CACHE_SIZE];
    volatile const char *x = clean_space;
    const char *clean_end = clean_space + sizeof clean_space;
    while (x < clean_end)
    {
        (void) *x;
        x += 32;
    }
}

/* Obtain current time in microseconds modulo 2^32 */
uint32_t
gettimei (void)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;

    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#else
    return (double) clock () / (double) CLOCKS_PER_SEC;
#endif
}

static void
pixman_image_composite_wrapper (pixman_composite_info_t *info)
{
    pixman_image_composite (info->op,
                            info->src_image, info->mask_image, info->dest_image,
                            info->src_x, info->src_y,
                            info->mask_x, info->mask_y,
                            info->dest_x, info->dest_y,
                            info->width, info->height);
}

static void
pixman_image_composite_empty (pixman_composite_info_t *info)
{
    pixman_image_composite (info->op,
                            info->src_image, info->mask_image, info->dest_image,
                            info->src_x, info->src_y,
                            info->mask_x, info->mask_y,
                            info->dest_x, info->dest_y,
                            1, 1);
}

static void
bench (pixman_op_t         op,
       pixman_transform_t *t,
       pixman_image_t     *src_image,
       pixman_image_t     *mask_image,
       pixman_image_t     *dest_image,
       int32_t             src_x,
       int32_t             src_y,
       uint32_t            max_n,
       uint32_t            max_time,
       uint32_t           *ret_n,
       uint32_t           *ret_time,
       void              (*func) (pixman_composite_info_t *info))
{
    uint32_t n = 0;
    uint32_t t0 = gettimei ();
    uint32_t t1;
    uint32_t x = 0;
    do
    {
        pixman_composite_info_t info;
        if (++x >= 64)
            x = 0;
        info.op = op;
        info.src_image = src_image;
        info.mask_image = mask_image;
        info.dest_image = dest_image;
        info.src_x = 0;
        info.src_y = 0;
        info.mask_x = 0;
        info.mask_y = 0;
        info.dest_x = 63 - x;
        info.dest_y = 0;
        info.width = WIDTH;
        info.height = HEIGHT;
        t->matrix[0][2] = pixman_int_to_fixed (src_x + x);
        t->matrix[1][2] = pixman_int_to_fixed (src_y);
        pixman_image_set_transform (src_image, t);
        if (mask_image)
            pixman_image_set_transform (mask_image, t);
        func (&info);
        t1 = gettimei ();
    }
    while (++n < max_n && (t1 - t0) < max_time);
    if (ret_n)
        *ret_n = n;
    *ret_time = t1 - t0;
}

int
main (int argc, char *argv[])
{
    pixman_filter_t      filter      = PIXMAN_FILTER_NEAREST;
    pixman_transform_t   t;
    pixman_op_t          op          = PIXMAN_OP_SRC;
    pixman_format_code_t src_format  = PIXMAN_a8r8g8b8;
    pixman_format_code_t mask_format = 0;
    pixman_format_code_t dest_format = PIXMAN_a8r8g8b8;

    uint32_t *src, *mask, *dest;
    pixman_image_t *src_image, *mask_image = NULL, *dest_image;
    pixman_box32_t dest_box = { 0, 0, WIDTH, HEIGHT };
    box_48_16_t transformed = { 0 };
    int32_t xmin, ymin, xmax, ymax;
    int32_t src_x, src_y;

    pixman_transform_init_identity (&t);

    ++argv;
    if (*argv && (*argv)[0] == '-' && (*argv)[1] == 'n')
    {
        filter = PIXMAN_FILTER_NEAREST;
        ++argv;
        --argc;
    }
    if (*argv && (*argv)[0] == '-' && (*argv)[1] == 'b')
    {
        filter = PIXMAN_FILTER_BILINEAR;
        ++argv;
        --argc;
    }
    if (argc == 1)
    {
        printf ("Usage: affine-bench [-n] [-b] axx [axy] [ayx] [ayy] [combine type]\n");
        printf ("                    [src format] [mask format] [dest format]\n");
        printf ("  -n : nearest scaling (default)\n");
        printf ("  -b : bilinear scaling\n");
        printf ("  axx : x_out:x_in factor\n");
        printf ("  axy : x_out:y_in factor (default 0)\n");
        printf ("  ayx : y_out:x_in factor (default 0)\n");
        printf ("  ayy : y_out:y_in factor (default 1)\n");
        printf ("  combine type : src, over, in etc (default src)\n");
        printf ("  src format : a8r8g8b8, r5g6b6 etc (default a8r8g8b8)\n");
        printf ("  mask format : as for src format, but no mask used if omitted\n");
        printf ("  dest format : as for src format (default a8r8g8b8)\n");
        return EXIT_FAILURE;
    }
    else
    {
        t.matrix[0][0] = pixman_double_to_fixed (strtod (*argv, NULL));
        if (*++argv)
        {
            t.matrix[0][1] = pixman_double_to_fixed (strtod (*argv, NULL));
            if (*++argv)
            {
                t.matrix[1][0] = pixman_double_to_fixed (strtod (*argv, NULL));
                if (*++argv)
                {
                    t.matrix[1][1] = pixman_double_to_fixed (strtod (*argv, NULL));
                    if (*++argv)
                    {
                        op = lookup_name_to_number (*argv, combine_type, sizeof combine_type / sizeof *combine_type);
                        if (*++argv)
                        {
                            src_format = lookup_name_to_number (*argv, format, sizeof format / sizeof *format);
                            ++argv;
                            if (argv[0] && argv[1])
                            {
                                mask_format = lookup_name_to_number (*argv, format, sizeof format / sizeof *format);
                                ++argv;
                            }
                            if (*argv)
                            {
                                dest_format = lookup_name_to_number (*argv, format, sizeof format / sizeof *format);
                            }
                        }
                    }
                }
            }
        }
    }

    compute_transformed_extents (&t, &dest_box, &transformed);
    xmin = pixman_fixed_to_int (transformed.x1 - 8 * pixman_fixed_e - pixman_fixed_1 / 2);
    ymin = pixman_fixed_to_int (transformed.y1 - 8 * pixman_fixed_e - pixman_fixed_1 / 2);
    xmax = pixman_fixed_to_int (transformed.x2 + 8 * pixman_fixed_e + pixman_fixed_1 / 2);
    ymax = pixman_fixed_to_int (transformed.y2 + 8 * pixman_fixed_e + pixman_fixed_1 / 2);
    src_x = -xmin;
    src_y = -ymin;

    create_image (xmax - xmin + 64, ymax - ymin + 1, src_format, filter, &t, &src, &src_image);
    if (mask_format)
    {
        create_image (xmax - xmin + 64, ymax - ymin + 1, mask_format, filter, &t, &mask, &mask_image);
        if ((PIXMAN_FORMAT_R(mask_format) || PIXMAN_FORMAT_G(mask_format) || PIXMAN_FORMAT_B(mask_format)))
            pixman_image_set_component_alpha (mask_image, 1);
    }
    create_image (WIDTH + 64, HEIGHT, dest_format, filter, &t, &dest, &dest_image);

    {
        uint32_t n;  /* number of iterations in at least 5 seconds */
        uint32_t t1; /* time taken to do n iterations, microseconds */
        uint32_t t2; /* calling overhead for n iterations, microseconds */
        flush_cache ();
        bench (op, &t, src_image, mask_image, dest_image, src_x, src_y, UINT32_MAX, 5000000, &n, &t1, pixman_image_composite_wrapper);
        bench (op, &t, src_image, mask_image, dest_image, src_x, src_y, n, UINT32_MAX, NULL, &t2, pixman_image_composite_empty);
        /* The result indicates the output rate in megapixels/second */
        printf ("%6.2f\n", (double) n * WIDTH * HEIGHT / (t1 - t2));
    }

    return EXIT_SUCCESS;
}
