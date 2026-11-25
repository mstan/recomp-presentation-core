#include "retro_presentation/presentation.hpp"
#include "retro_presentation/pacing.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace retro::presentation;
#define REQUIRE(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct HostState {int opened=0,closed=0,draws=0,swaps=0;bool current=true;std::vector<std::uint8_t> pixels;};
static bool open_host(void* p){++static_cast<HostState*>(p)->opened;return true;}
static void close_host(void* p){++static_cast<HostState*>(p)->closed;}
static bool current_host(void* p){return static_cast<HostState*>(p)->current;}
static void draw_cpu(void* p,const std::uint8_t* data,std::uint32_t w,std::uint32_t h,const Rect&,Filter){auto* host=static_cast<HostState*>(p);++host->draws;host->pixels.assign(data,data+std::size_t(w)*h*4);}
static void swap_host(void* p){++static_cast<HostState*>(p)->swaps;}
int main(){
    auto r=fit_picture({1920,1080},{1,1},320,240,4,3,false);REQUIRE(r.x==240&&r.y==0&&r.w==1440&&r.h==1080);
    r=fit_picture({800,600},{1,1},320,240,0,0,true);REQUIRE(r.x==80&&r.y==60&&r.w==640&&r.h==480);
    r=fit_picture({800,600},{2,2},320,240,0,0,true);REQUIRE(r.x==0&&r.y==0&&r.w==800&&r.h==600);
    r=fit_picture({100,50},{1,1},320,240,0,0,true);REQUIRE(std::abs(r.w-66.6667f)<0.001f&&r.h==50);
    REQUIRE(fit_picture({0,600},{1,1},320,240,0,0,false).w==0);
    REQUIRE(fit_picture({std::numeric_limits<float>::infinity(),600},{1,1},320,240,0,0,false).w==0);
    HostState state;Host host{&state,open_host,close_host,current_host,nullptr,nullptr,draw_cpu,swap_host};
    Presenter p(host);REQUIRE(p.open()&&!p.uses_gpu()&&p.supports_pixel_pairs());REQUIRE(p.open()&&state.opened==1);
    REQUIRE(!p.present({640,480}));
    std::array<std::uint8_t,16> pixels{{255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}};
    REQUIRE(p.upload(pixels.data(),2,2));const auto original=pixels;pixels.fill(0);
    REQUIRE(p.present({640,480})&&state.draws==1&&state.swaps==1);
    REQUIRE(state.pixels==std::vector<std::uint8_t>(original.begin(),original.end()));
    std::uint32_t w=0,h=0;REQUIRE(p.snapshot(w,h)==state.pixels&&w==2&&h==2);
    REQUIRE(!p.upload(original.data(),UINT32_MAX,2));REQUIRE(p.snapshot(w,h)==state.pixels);
    REQUIRE(!p.copy_texture(1,2,2));
    p.set_style({Filter::SharpBilinear,true});REQUIRE(p.prepare({640,480}).filter==Filter::Bilinear);
    REQUIRE(p.upload_pixel_pairs(original.data(),2,2));auto pair=p.snapshot(w,h);REQUIRE(w==4&&h==2&&pair.size()==32);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)REQUIRE(pair[i*8+j]==original[i*4+j]&&pair[i*8+j+4]==original[i*4+j]);
    HostState independent;host.user=&independent;Presenter second(host);REQUIRE(second.open()&&!second.present({640,480}));
    REQUIRE(p.present({640,480})&&state.draws==2&&independent.draws==0);
    state.current=false;REQUIRE(!p.present({640,480}));p.close();p.close();REQUIRE(state.closed==1);
    state.current=true;REQUIRE(p.open()&&!p.present({640,480}));p.close();REQUIRE(state.closed==2);
    Presenter absent({});REQUIRE(!absent.open());
    RetroFramePacer pacer{};REQUIRE(retro_frame_pacer_init_ratio(&pacer,1000000,1001,60000));
    for(unsigned i=0;i<60000;++i)(void)retro_frame_pacer_deadline(&pacer,pacer.next);
    REQUIRE(pacer.next==1001000000ull);
    retro_frame_pacer_reset(&pacer);REQUIRE(retro_frame_pacer_deadline(&pacer,12345)==12345);
    REQUIRE(retro_frame_pacer_deadline(&pacer,1000000)==1000000);
    REQUIRE(!retro_frame_pacer_init_ratio(&pacer,UINT64_MAX,UINT32_MAX,1));
    RetroPresentationClock display{};retro_presentation_clock_begin(&display,1000000,144,0);
    int presentations=0;std::uint64_t deadline;double phase;
    while(retro_presentation_clock_next(&display,0,1000000.0/60,&deadline,&phase)){REQUIRE(phase>=0&&phase<1);++presentations;}
    REQUIRE(presentations==3);
    retro_presentation_clock_begin(&display,1000000,0,0);REQUIRE(!retro_presentation_clock_next(&display,0,1000,&deadline,&phase));
    retro_frame_pacer_init(&pacer,1000000,0);REQUIRE(!pacer.period&&retro_frame_pacer_deadline(&pacer,7)==7);
    std::puts("geometry, portable delivery/capture, ownership, lifecycle and rational pacing passed");
}
