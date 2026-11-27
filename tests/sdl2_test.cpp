/* SPDX-License-Identifier: MIT */
#define SDL_MAIN_HANDLED
#include <retro_presentation/sdl2.h>
#include <retro_presentation/capture.h>
#include <retro_presentation/layout.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"%d: %s: %s\n",__LINE__,#x,SDL_GetError());std::exit(1);}}while(0)
static std::vector<unsigned char> pixels(SDL_Renderer *r) {
    std::vector<unsigned char> out(73*51*4);
    CHECK(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_RGBA32,out.data(),73*4)==0);return out;
}
static void clear(SDL_Renderer *r) {
    CHECK(SDL_SetRenderDrawColor(r,7,11,20,255)==0);CHECK(SDL_RenderClear(r)==0);
}
int main() {
    auto *surface=SDL_CreateRGBSurfaceWithFormat(0,73,51,32,SDL_PIXELFORMAT_RGBA32);CHECK(surface);
    auto *r=SDL_CreateSoftwareRenderer(surface);CHECK(r);
    const int w=11,h=7;
    std::vector<unsigned char> rgb(w*h*3);
    for(size_t i=0;i<rgb.size();++i)rgb[i]=static_cast<unsigned char>(i*73+(i>>4)*19);
    unsigned copies=0;
    for(Uint32 format:{SDL_PIXELFORMAT_RGB24,SDL_PIXELFORMAT_ARGB8888,SDL_PIXELFORMAT_ABGR8888}) {
      std::vector<unsigned char> packed(w*h*4);
      CHECK(SDL_ConvertPixels(w,h,SDL_PIXELFORMAT_RGB24,rgb.data(),w*3,format,packed.data(),w*(format==SDL_PIXELFORMAT_RGB24?3:4))==0);
      for(int linear:{0,1}) {
        auto *p=rp_sdl2_create(r,format,w,h,linear);CHECK(p);
        CHECK(rp_sdl2_upload(p,packed.data(),w*(format==SDL_PIXELFORMAT_RGB24?3:4)));
        auto *reference=SDL_CreateTexture(r,format,SDL_TEXTUREACCESS_STREAMING,w,h);CHECK(reference);
        CHECK(SDL_SetTextureBlendMode(reference,SDL_BLENDMODE_NONE)==0);
        CHECK(SDL_SetTextureScaleMode(reference,linear?SDL_ScaleModeLinear:SDL_ScaleModeNearest)==0);
        CHECK(SDL_UpdateTexture(reference,nullptr,packed.data(),w*(format==SDL_PIXELFORMAT_RGB24?3:4))==0);
        SDL_Rect crop{2,1,7,5}, destination{3,4,61,39};
        for(int cropped:{0,1}) for(int sharp:{0,3}) {
          clear(r);
          if(sharp) {
            auto *intermediate=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,w*sharp,h*sharp);CHECK(intermediate);
            CHECK(SDL_SetTextureBlendMode(intermediate,SDL_BLENDMODE_NONE)==0);
            CHECK(SDL_SetTextureScaleMode(intermediate,SDL_ScaleModeLinear)==0);
            CHECK(SDL_SetRenderTarget(r,intermediate)==0);
            CHECK(SDL_RenderCopy(r,reference,cropped?&crop:nullptr,nullptr)==0);
            CHECK(SDL_SetRenderTarget(r,nullptr)==0);
            CHECK(SDL_RenderCopy(r,intermediate,nullptr,&destination)==0);
            SDL_DestroyTexture(intermediate);
          } else CHECK(SDL_RenderCopy(r,reference,cropped?&crop:nullptr,&destination)==0);
          auto expected=pixels(r); clear(r);
          CHECK(rp_sdl2_copy(p,rp_sdl2_texture(p),cropped?&crop:nullptr,&destination,w,h,sharp));
          CHECK(pixels(r)==expected);++copies;
        }
        auto *held=rp_sdl2_texture(p);int uw=0,uh=0;
        CHECK(rp_sdl2_prepare_source(p,w,h,1,&uw,&uh));CHECK(uw==w&&uh==h&&rp_sdl2_texture(p)==held);
        CHECK(rp_sdl2_prepare_source(p,w,h,1,&uw,&uh));CHECK(rp_sdl2_texture(p)==held);
        CHECK(!rp_sdl2_resize(p,0,h));CHECK(rp_sdl2_texture(p)==held);
        CHECK(!rp_sdl2_prepare_source(p,0,h,1,&uw,&uh));CHECK(rp_sdl2_texture(p)==held);
        rp_sdl2_set_linear(p,!linear);CHECK(rp_sdl2_resize(p,13,9));
        SDL_ScaleMode mode;CHECK(SDL_GetTextureScaleMode(rp_sdl2_texture(p),&mode)==0);
        CHECK(mode==(!linear?SDL_ScaleModeLinear:SDL_ScaleModeNearest));
        rp_sdl2_reset_sharp(p);rp_sdl2_destroy(p);SDL_DestroyTexture(reference);
      }
    }
    // SMS/GG capture crop/stride, bars, alpha and fractional downscale.
    const uint32_t source[]={1,2,3,999,4,5,6,999};uint32_t captured[12];
    rp_capture_argb(source,3,2,4,captured,4,3);
    const uint32_t expected[]={0xff000001,0xff000001,0xff000002,0xff000003,
                              0xff000004,0xff000004,0xff000005,0xff000006,
                              0xff000000,0xff000000,0xff000000,0xff000000};
    for(unsigned i=0;i<12;i++)CHECK(captured[i]==expected[i]);
    SDL_DestroyRenderer(r);SDL_FreeSurface(surface);
    std::printf("SDL2: %u matched real pixel copies, formats, filtering, sharp/crop, reuse and lifetime; capture stride/alpha/bars pass\n",copies);
}
