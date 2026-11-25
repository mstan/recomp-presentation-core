#pragma once
#include "retro_presentation/presentation.hpp"
#if defined(_WIN32)
#define RGL_CALL __stdcall
#else
#define RGL_CALL
#endif
namespace retro::presentation::detail {
using GLenum=unsigned;using GLuint=unsigned;using GLint=int;using GLsizei=int;using GLboolean=unsigned char;using GLbitfield=unsigned;
enum : unsigned {GL_TEXTURE_2D=0x0de1,GL_TEXTURE_MIN_FILTER=0x2801,GL_TEXTURE_MAG_FILTER=0x2800,GL_TEXTURE_WRAP_S=0x2802,GL_TEXTURE_WRAP_T=0x2803,GL_CLAMP_TO_EDGE=0x812f,GL_NEAREST=0x2600,GL_LINEAR=0x2601,GL_LINEAR_MIPMAP_LINEAR=0x2703,GL_RGBA=0x1908,GL_RGBA8=0x8058,GL_UNSIGNED_BYTE=0x1401,GL_UNPACK_ALIGNMENT=0x0cf5,GL_PACK_ALIGNMENT=0x0d05,GL_READ_FRAMEBUFFER=0x8ca8,GL_DRAW_FRAMEBUFFER=0x8ca9,GL_READ_FRAMEBUFFER_BINDING=0x8caa,GL_DRAW_FRAMEBUFFER_BINDING=0x8ca6,GL_COLOR_ATTACHMENT0=0x8ce0,GL_FRAMEBUFFER_COMPLETE=0x8cd5,GL_COLOR_BUFFER_BIT=0x4000,GL_SCISSOR_TEST=0x0c11,GL_MAX_TEXTURE_SIZE=0x0d33};
struct Gl {
#define RGL_FN(ret,name,...) ret (RGL_CALL *name)(__VA_ARGS__)=nullptr
 RGL_FN(void,GenTextures,GLsizei,GLuint*);RGL_FN(void,DeleteTextures,GLsizei,const GLuint*);
 RGL_FN(void,BindTexture,GLenum,GLuint);RGL_FN(void,TexParameteri,GLenum,GLenum,GLint);
 RGL_FN(void,PixelStorei,GLenum,GLint);RGL_FN(void,TexImage2D,GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
 RGL_FN(void,TexSubImage2D,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
 RGL_FN(void,GetIntegerv,GLenum,GLint*);RGL_FN(GLboolean,IsEnabled,GLenum);
 RGL_FN(void,Disable,GLenum);RGL_FN(void,Enable,GLenum);
 RGL_FN(void,ReadPixels,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
 RGL_FN(void,GenFramebuffers,GLsizei,GLuint*);RGL_FN(void,DeleteFramebuffers,GLsizei,const GLuint*);
 RGL_FN(void,BindFramebuffer,GLenum,GLuint);RGL_FN(void,FramebufferTexture2D,GLenum,GLenum,GLenum,GLuint,GLint);
 RGL_FN(GLenum,CheckFramebufferStatus,GLenum);
 RGL_FN(void,BlitFramebuffer,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
 RGL_FN(void,GenerateMipmap,GLenum);
#undef RGL_FN
 bool blit=false,complete=false;
 void load(const Host& host) {
#define RGL_LOAD(name) name=reinterpret_cast<decltype(name)>(host.get_proc?host.get_proc(host.user,"gl" #name):nullptr)
 RGL_LOAD(GenTextures);RGL_LOAD(DeleteTextures);RGL_LOAD(BindTexture);RGL_LOAD(TexParameteri);RGL_LOAD(PixelStorei);
 RGL_LOAD(TexImage2D);RGL_LOAD(TexSubImage2D);RGL_LOAD(GetIntegerv);RGL_LOAD(IsEnabled);RGL_LOAD(Disable);RGL_LOAD(Enable);RGL_LOAD(ReadPixels);
 RGL_LOAD(GenFramebuffers);RGL_LOAD(DeleteFramebuffers);RGL_LOAD(BindFramebuffer);RGL_LOAD(FramebufferTexture2D);RGL_LOAD(CheckFramebufferStatus);RGL_LOAD(BlitFramebuffer);RGL_LOAD(GenerateMipmap);
#undef RGL_LOAD
 complete=GenTextures&&DeleteTextures&&BindTexture&&TexParameteri&&PixelStorei&&TexImage2D&&TexSubImage2D&&GetIntegerv&&IsEnabled&&Disable&&Enable&&ReadPixels;
 blit=GenFramebuffers&&DeleteFramebuffers&&BindFramebuffer&&FramebufferTexture2D&&CheckFramebufferStatus&&BlitFramebuffer;
 }
};
}
