#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include "retro_presentation/presentation.hpp"
#include <array>
#include <cstdio>
#include <cstring>
using namespace retro::presentation;
#define REQUIRE(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Native {HWND window=nullptr;HDC dc=nullptr;HGLRC context=nullptr;int reads=0,draws=0;};
static Native* observer=nullptr;
static void APIENTRY observed_read(GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum t,void* out){++observer->reads;::glReadPixels(x,y,w,h,f,t,out);}
static void close_host(void* ptr){auto& n=*static_cast<Native*>(ptr);wglMakeCurrent(nullptr,nullptr);if(n.context)wglDeleteContext(n.context);if(n.dc)ReleaseDC(n.window,n.dc);if(n.window)DestroyWindow(n.window);n.context=nullptr;n.dc=nullptr;n.window=nullptr;}
static bool open_host(void* ptr){auto& n=*static_cast<Native*>(ptr);WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"RetroPresentationHiddenTest";
    if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return false;
    n.window=CreateWindowExW(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,wc.lpszClassName,L"",WS_POPUP,0,0,32,32,nullptr,nullptr,wc.hInstance,nullptr);
    if(!n.window)return false;n.dc=GetDC(n.window);
    PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=32;pf.cAlphaBits=8;
    int chosen=ChoosePixelFormat(n.dc,&pf);if(!chosen||!SetPixelFormat(n.dc,chosen,&pf)){close_host(ptr);return false;}
    n.context=wglCreateContext(n.dc);if(!n.context||!wglMakeCurrent(n.dc,n.context)){close_host(ptr);return false;}observer=&n;return true;
}
static bool current_host(void* ptr){auto& n=*static_cast<Native*>(ptr);return wglMakeCurrent(n.dc,n.context)!=FALSE;}
static void* get_proc(void*,const char* name){if(!std::strcmp(name,"glReadPixels"))return reinterpret_cast<void*>(observed_read);
    auto p=wglGetProcAddress(name);if(!p||p==reinterpret_cast<PROC>(1)||p==reinterpret_cast<PROC>(2)||p==reinterpret_cast<PROC>(3)||p==reinterpret_cast<PROC>(-1))p=GetProcAddress(GetModuleHandleW(L"opengl32.dll"),name);return reinterpret_cast<void*>(p);
}
static void draw_host(void* ptr,unsigned,const Rect&){++static_cast<Native*>(ptr)->draws;}
static void swap_host(void* ptr){SwapBuffers(static_cast<Native*>(ptr)->dc);}
int main(){
    Native native;Host host{&native,open_host,close_host,current_host,get_proc,draw_host,nullptr,swap_host};Presenter p(host);
    if(!p.open()){std::puts("native WGL context unavailable");return 77;}
    REQUIRE(p.uses_gpu());if(!p.supports_pixel_pairs()){std::puts("native framebuffer blit unavailable");return 77;}
    std::printf("renderer: %s\n",reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    const std::array<std::uint8_t,16> original{{255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}};
    REQUIRE(p.upload(original.data(),2,2));p.set_style({Filter::Nearest,false});const auto baseline=p.prepare({6,6});REQUIRE(baseline.texture);
    REQUIRE(p.present({6,6})&&native.draws==1&&native.reads==0);
    std::uint32_t w,h;auto snapshot=p.snapshot(w,h);REQUIRE(w==2&&h==2&&snapshot==std::vector<std::uint8_t>(original.begin(),original.end()));
    p.set_style({Filter::SharpBilinear,false});auto sharp=p.prepare({6,6});REQUIRE(sharp.texture&&sharp.texture!=baseline.texture);
    glBindTexture(GL_TEXTURE_2D,sharp.texture);GLint width=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);REQUIRE(width==6);
    REQUIRE(p.prepare({6,6}).texture==sharp.texture);REQUIRE(native.reads==1);
    (void)p.prepare({1,1});REQUIRE(native.reads==1);REQUIRE(p.snapshot(w,h)==snapshot);
    Host borrowed=host;borrowed.open=nullptr;borrowed.close=nullptr;Presenter copy(borrowed);REQUIRE(copy.open());
    REQUIRE(copy.copy_texture(baseline.texture,2,2));REQUIRE(copy.snapshot(w,h)==snapshot);copy.close();
    REQUIRE(p.upload_pixel_pairs(original.data(),2,2));auto pairs=p.snapshot(w,h);REQUIRE(w==4&&h==2&&pairs.size()==32);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)REQUIRE(pairs[i*8+j]==original[i*4+j]&&pairs[i*8+j+4]==original[i*4+j]);
    REQUIRE(!p.upload(original.data(),UINT32_MAX,2));REQUIRE(p.snapshot(w,h)==pairs);
    p.close();REQUIRE(!p.is_open());std::puts("native GL upload/reuse, filtering, GPU copy, pixel-pair reconstruction and explicit capture passed");
}
