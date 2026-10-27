/* SPDX-License-Identifier: GPL-2.0-or-later
 * HCPS1: two 16-bit pixels per integer operation, without synthetic SIMD.
 * Source pixels are PSX little-endian; destination is host-endian RGB565.
 * memcpy keeps unaligned MIPS accesses and C aliasing rules well-defined.
 */
#include <stdint.h>
#include <string.h>
#include "video.h"

static uint32_t convert_pair(uint32_t p)
{
    return ((p & UINT32_C(0x001f001f)) << 11) |
           ((p & UINT32_C(0x03e003e0)) << 1) |
           ((p & UINT32_C(0x7c007c00)) >> 10);
}

void bgr555_to_rgb565(void *dst_, const void *src_, int pixels)
{
    const unsigned char *src = src_;
    unsigned char *dst = dst_;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    for (; pixels >= 8; pixels -= 8, src += 16, dst += 16) {
        uint32_t a, b, c, d;
        memcpy(&a, src, 4); memcpy(&b, src + 4, 4);
        memcpy(&c, src + 8, 4); memcpy(&d, src + 12, 4);
        a = convert_pair(a); b = convert_pair(b);
        c = convert_pair(c); d = convert_pair(d);
        memcpy(dst, &a, 4); memcpy(dst + 4, &b, 4);
        memcpy(dst + 8, &c, 4); memcpy(dst + 12, &d, 4);
    }
    for (; pixels >= 2; pixels -= 2, src += 4, dst += 4) {
        uint32_t p;
        memcpy(&p, src, 4);
        p = convert_pair(p);
        memcpy(dst, &p, 4);
    }
#endif
    for (; pixels > 0; --pixels, src += 2, dst += 2) {
        uint16_t p = (uint16_t)(src[0] | ((uint16_t)src[1] << 8));
        uint16_t q = (uint16_t)convert_pair(p);
        memcpy(dst, &q, 2);
    }
}
