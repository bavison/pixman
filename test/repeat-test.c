#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define WIDTH 3
#define HEIGHT 5
#define BORDER 6

void
check (pixman_repeat_t repeat_type, uint32_t expected_crc)
{
    uint32_t *src, *dest;
    size_t i;
    uint32_t crc;

    prng_srand (0);

    src = fence_malloc (WIDTH * HEIGHT * sizeof *src);
    for (i = 0; i < WIDTH * HEIGHT; i++)
        src[i] = 0xFF000000 | ((i & 0xFF) << 16) | ((i & 0xFF) << 8) | ((i & 0xFF) << 0);
    dest = (uint32_t *) make_random_bytes ((WIDTH + 2 * BORDER) * (HEIGHT + 2 * BORDER) * sizeof *dest);

    pixman_image_t *s = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, src, WIDTH * sizeof *src);
    pixman_image_set_repeat (s, repeat_type);
    pixman_image_t *d = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH + 2 * BORDER, HEIGHT + 2 * BORDER, dest, (WIDTH + 2 * BORDER) * sizeof *dest);
    pixman_image_composite (PIXMAN_OP_OVER, s, NULL, d, -BORDER, -BORDER, 0, 0, 0, 0, WIDTH + BORDER * 2, HEIGHT + BORDER * 2);

#if 0
    size_t x, y;
    for (y = 0; y < HEIGHT + 2 * BORDER; y++)
    {
        for (x = 0; x < WIDTH + 2 * BORDER; x++)
            printf (" %08X", dest[y * (WIDTH + 2 * BORDER) + x]);
        printf ("\n");
    }
    printf ("\n");
#endif

    if ((crc = compute_crc32_for_image (0, d)) != expected_crc)
    {
        const char *name[] = { "NONE", "NORMAL", "PAD", "REFLECT" };
        printf ("Failed CRC for repeat type '%s': got %08X, expected %08X\n",
                name[repeat_type], crc, expected_crc);
        exit (EXIT_FAILURE);
    }

    fence_free (src);
    fence_free (dest);
}

int
main (void)
{
    check (PIXMAN_REPEAT_NONE,    0xCB6B2680);
    check (PIXMAN_REPEAT_NORMAL,  0x0254C0A1);
    check (PIXMAN_REPEAT_PAD,     0x29D09CDB);
    check (PIXMAN_REPEAT_REFLECT, 0x85F2E260);
    return EXIT_SUCCESS;
}
