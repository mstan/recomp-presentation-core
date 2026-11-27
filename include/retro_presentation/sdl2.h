/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Matthew Stanley */
#ifndef RETRO_PRESENTATION_SDL2_H
#define RETRO_PRESENTATION_SDL2_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Borrows one renderer. Owns only upload/sharp textures; it never creates a
 * window, presents, clears, changes pacing, or copies the caller's pixels.
 * Serialize calls on the renderer's owner thread and destroy before renderer.
 * Resident GPU textures passed to copy remain borrowed. */
typedef struct RpSdl2 RpSdl2;
/* linear=-1 preserves SDL's existing scale-quality hint (legacy SMS profile). */
RpSdl2 *rp_sdl2_create(SDL_Renderer *, Uint32 format, int w, int h, int linear);
void rp_sdl2_destroy(RpSdl2 *);
SDL_Texture *rp_sdl2_texture(RpSdl2 *);
int rp_sdl2_resize(RpSdl2 *, int w, int h);
/* Reuses the last successful upload geometry for identical source dimensions.
 * Optional halving preserves the complete source raster on limited devices. */
int rp_sdl2_prepare_source(RpSdl2 *, unsigned w, unsigned h, int allow_halving,
                           int *upload_w, int *upload_h);
void rp_sdl2_set_linear(RpSdl2 *, int linear);
void rp_sdl2_reset_sharp(RpSdl2 *);
int rp_sdl2_upload(RpSdl2 *, const void *pixels, int pitch);
/* factor >=2 uses nearest source -> linear intermediate -> destination.
 * Unsupported target/allocation paths retain the ordinary source copy. */
int rp_sdl2_copy(RpSdl2 *, SDL_Texture *source, const SDL_Rect *crop,
                  const SDL_Rect *destination, int logical_w, int logical_h,
                  int sharp_factor);
#ifdef __cplusplus
}
#endif
#endif
