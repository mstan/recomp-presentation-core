#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
static int allocation_fails;
static void *test_alloc(size_t size){return allocation_fails?NULL:malloc(size);}
#define RP_PNG_ALLOC test_alloc
#define RP_PNG_FREE free
#include "retro_presentation/png.h"
typedef struct Output{unsigned char bytes[1024];size_t used;int fail;} Output;
static int sink(void *user,const void *bytes,size_t size){
    Output *out=(Output*)user;if(out->fail||size>sizeof out->bytes-out->used)return 0;
    memcpy(out->bytes+out->used,bytes,size);out->used+=size;return 1;
}
int main(void){
    const uint32_t padded[]={0xff123456,0xffabcdef,0xff000000,0xff102030,0xff405060,0xff000000};
    const uint32_t tight[]={0xff123456,0xffabcdef,0xff102030,0xff405060};
    Output a={{0},0,0},b={{0},0,0};
    assert(!rp_png_write_argb_sink(sink,&a,padded,2,2,3));
    assert(!rp_png_write_argb_sink(sink,&b,tight,2,2,2));
    assert(a.used==b.used&&!memcmp(a.bytes,b.bytes,a.used));
    assert(a.used==82&&!memcmp(a.bytes,"\x89PNG\r\n\x1a\n",8));
    assert(a.bytes[16]==0&&a.bytes[19]==2&&a.bytes[23]==2);
    const unsigned char rgb[]={0,0x12,0x34,0x56,0xab,0xcd,0xef,0,0x10,0x20,0x30,0x40,0x50,0x60};
    assert(!memcmp(a.bytes+48,rgb,sizeof rgb));
    b.fail=1;assert(rp_png_write_argb_sink(sink,&b,tight,2,2,2)==-1);
    allocation_fails=1;assert(rp_png_write_argb_sink(sink,&b,tight,2,2,2)==-1);allocation_fails=0;
    assert(rp_png_write_argb_sink(sink,&b,NULL,2,2,2)==-1);
    assert(rp_png_write_argb_sink(sink,&b,tight,0,2,2)==-1);
    assert(rp_png_write_argb_sink(sink,&b,tight,2,2,1)==-1);
    assert(rp_png_write_argb_sink(sink,&b,tight,INT_MAX,INT_MAX,INT_MAX)==-1);
    assert(rp_png_write_argb("missing-dir/capture.png",tight,2,2,2)==-1);
    return 0;
}
