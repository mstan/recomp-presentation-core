/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Matthew Stanley */
#include "retro_presentation/sdl2.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct RpSdl2 {
    SDL_Renderer *renderer;
    SDL_Texture *texture, *sharp;
    Uint32 format;
    int width, height, sharp_w, sharp_h, linear, sharp_target_failed;
    unsigned source_w, source_h;
};
void rp_sdl2_reset_sharp(RpSdl2 *p) {
    if (!p) return;
    if (p->sharp) SDL_DestroyTexture(p->sharp);
    p->sharp = NULL;
    p->sharp_w = p->sharp_h = 0;
}
static void scale_mode(SDL_Texture *t, int linear) {
#if SDL_VERSION_ATLEAST(2,0,12)
    if (linear >= 0) SDL_SetTextureScaleMode(t, linear ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
#else
    (void)t; (void)linear;
#endif
}
int rp_sdl2_resize(RpSdl2 *p, int w, int h) {
    SDL_Texture *replacement;
    if (!p || w <= 0 || h <= 0) return 0;
    if (p->texture && w == p->width && h == p->height) return 1;
    replacement = SDL_CreateTexture(p->renderer, p->format,
                                   SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!replacement) return 0;
    SDL_SetTextureBlendMode(replacement, SDL_BLENDMODE_NONE);
    scale_mode(replacement, p->linear);
    if (p->texture) SDL_DestroyTexture(p->texture);
    rp_sdl2_reset_sharp(p);
    p->texture = replacement; p->width = w; p->height = h;
    p->source_w = p->source_h = 0;
    return 1;
}
RpSdl2 *rp_sdl2_create(SDL_Renderer *renderer, Uint32 format, int w, int h, int linear) {
    RpSdl2 *p;
    if (!renderer) return NULL;
    p = (RpSdl2 *)calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->renderer = renderer; p->format = format; p->linear = linear;
    if (!rp_sdl2_resize(p, w, h)) { free(p); return NULL; }
    return p;
}
void rp_sdl2_destroy(RpSdl2 *p) {
    if (!p) return;
    rp_sdl2_reset_sharp(p);
    if (p->texture) SDL_DestroyTexture(p->texture);
    free(p);
}
SDL_Texture *rp_sdl2_texture(RpSdl2 *p) { return p ? p->texture : NULL; }
void rp_sdl2_set_linear(RpSdl2 *p, int linear) {
    if (!p) return;
    p->linear = linear; scale_mode(p->texture, linear);
}
int rp_sdl2_prepare_source(RpSdl2 *p, unsigned w, unsigned h, int halve,
                           int *upload_w, int *upload_h) {
    unsigned tw = w, th = h;
    SDL_RendererInfo info;
    if (!p || !w || !h || w > INT_MAX || h > INT_MAX) return 0;
    if (p->source_w != w || p->source_h != h) {
        SDL_zero(info); SDL_GetRendererInfo(p->renderer, &info);
        if (halve) {
            while ((info.max_texture_width > 0 && tw > (unsigned)info.max_texture_width) ||
                   (info.max_texture_height > 0 && th > (unsigned)info.max_texture_height)) {
                tw = tw > 1 ? tw / 2 : 1; th = th > 1 ? th / 2 : 1;
            }
        }
        while (!rp_sdl2_resize(p, (int)tw, (int)th)) {
            if (!halve || (tw == 1 && th == 1)) return 0;
            tw = tw > 1 ? tw / 2 : 1; th = th > 1 ? th / 2 : 1;
        }
        p->source_w = w; p->source_h = h;
    }
    if (upload_w) *upload_w = p->width;
    if (upload_h) *upload_h = p->height;
    return 1;
}
int rp_sdl2_upload(RpSdl2 *p, const void *pixels, int pitch) {
    return p && pixels && pitch > 0 && SDL_UpdateTexture(p->texture, NULL, pixels, pitch) == 0;
}
static int ensure_sharp(RpSdl2 *p, int w, int h) {
    SDL_RendererInfo info;
    if (p->sharp && p->sharp_w == w && p->sharp_h == h) return 1;
    SDL_zero(info);
    if (SDL_GetRendererInfo(p->renderer, &info) == 0 &&
        ((info.max_texture_width > 0 && w > info.max_texture_width) ||
         (info.max_texture_height > 0 && h > info.max_texture_height))) {
        if (!p->sharp_target_failed)
            fprintf(stderr, "recomp-presentation: sharp target %dx%d exceeds renderer limit %dx%d; using nearest fallback\n",
                    w,h,info.max_texture_width,info.max_texture_height);
        p->sharp_target_failed = 1;
        rp_sdl2_reset_sharp(p); return 0;
    }
    rp_sdl2_reset_sharp(p);
    p->sharp = SDL_CreateTexture(p->renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_TARGET, w, h);
    if (!p->sharp) {
        if (!p->sharp_target_failed)
            fprintf(stderr, "recomp-presentation: sharp target creation failed: %s; using nearest fallback\n",SDL_GetError());
        p->sharp_target_failed = 1; return 0;
    }
    p->sharp_target_failed = 0;
    p->sharp_w = w; p->sharp_h = h;
    SDL_SetTextureBlendMode(p->sharp, SDL_BLENDMODE_NONE);
    scale_mode(p->sharp, 1); return 1;
}
int rp_sdl2_copy(RpSdl2 *p, SDL_Texture *source, const SDL_Rect *crop,
                  const SDL_Rect *destination, int w, int h, int factor) {
    if (!p || !source) return 0;
    if (factor >= 2 && w > 0 && h > 0 && w <= INT_MAX/factor && h <= INT_MAX/factor &&
        ensure_sharp(p, w*factor, h*factor) &&
        SDL_SetRenderTarget(p->renderer, p->sharp) == 0) {
        int copied = SDL_RenderCopy(p->renderer, source, crop, NULL) == 0;
        int restored = SDL_SetRenderTarget(p->renderer, NULL) == 0;
        if (copied && restored && SDL_RenderCopy(p->renderer, p->sharp, NULL, destination) == 0)
            return 1;
    }
    if (factor >= 2) SDL_SetRenderTarget(p->renderer, NULL);
    return SDL_RenderCopy(p->renderer, source, crop, destination) == 0;
}
