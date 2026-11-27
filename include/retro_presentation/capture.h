/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Matthew Stanley */
#ifndef RETRO_PRESENTATION_CAPTURE_H
#define RETRO_PRESENTATION_CAPTURE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Borrowed ARGB rows; destination and geometry are caller-owned. Positive
 * dimensions and stride >= width are required. No guest frame is modified. */
static inline void rp_capture_argb(const uint32_t *src, int sw, int sh, int sstride, uint32_t *dst, int dw, int dh){
    memset(dst, 0, (size_t)dw * dh * 4);
    for (int i = 0; i < dw * dh; i++) dst[i] = 0xFF000000u;
    int scale_n = dw * sh < dh * sw ? dw : dh, scale_d = dw * sh < dh * sw ? sw : sh;
    int ow = (int)((int64_t)sw * scale_n / scale_d), oh = (int)((int64_t)sh * scale_n / scale_d);
    if (ow > dw) ow = dw;
    if (oh > dh) oh = dh;
    int ox = (dw - ow) / 2, oy = (dh - oh) / 2;
    for (int y = 0; y < oh; y++){
        int sy = (int)((int64_t)y * sh / oh);
        for (int x = 0; x < ow; x++){
            int sx = (int)((int64_t)x * sw / ow);
            dst[(size_t)(oy + y) * dw + ox + x] = src[(size_t)sy * sstride + sx] | 0xFF000000u;
        }
    }
}
#endif
