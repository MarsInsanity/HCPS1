/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../src/video.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t reference(unsigned p)
{
    return (uint16_t)(((p & 31) << 11) | ((p & 992) << 1) | ((p >> 10) & 31));
}
int main(void)
{
    unsigned char src[4096], dst[4096];
    unsigned value, so, d, n, i;
    /* All 65536 inputs, including mask bit; packed pair lane independence. */
    for (value = 0; value < 65536; ++value) {
        uint16_t actual;
        src[0] = value; src[1] = value >> 8;
        src[2] = ~value; src[3] = (~value) >> 8;
        bgr555_to_rgb565(dst, src, 2);
        memcpy(&actual, dst, 2); assert(actual == reference(value));
        memcpy(&actual, dst + 2, 2); assert(actual == reference((~value) & 65535));
    }
    for (i = 0; i < sizeof(src); ++i) src[i] = (i * 73u + 17u) & 255;
    /* Odd widths, tiny spans and deliberately unaligned source/destination. */
    for (so = 0; so < 16; ++so)
        for (d = 0; d < 16; ++d)
            for (n = 0; n <= 1024; n = n < 33 ? n + 1 : n + 31) {
                memset(dst, 0xa5, sizeof(dst));
                bgr555_to_rgb565(dst + d, src + so, (int)n);
                for (i = 0; i < n; ++i) {
                    uint16_t actual;
                    unsigned p = src[so + 2*i] | (src[so + 2*i + 1] << 8);
                    memcpy(&actual, dst + d + 2*i, 2);
                    assert(actual == reference(p));
                }
                for (i = 0; i < d; ++i) assert(dst[i] == 0xa5);
                for (i = d + 2*n; i < sizeof(dst); ++i) assert(dst[i] == 0xa5);
            }
    /* ASan catches accidental short-buffer vector reads as well as writes. */
    for (n = 1; n < 20; ++n) {
        unsigned char *s = malloc(n * 2), *t = malloc(n * 2);
        memset(s, 0xff, n * 2); bgr555_to_rgb565(t, s, n);
        free(s); free(t);
    }
    puts("PASS: exhaustive color conversion, alignments, tails, bounds");
    return 0;
}
