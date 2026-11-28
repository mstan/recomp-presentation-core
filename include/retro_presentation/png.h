/* Optional dependency-free ARGB8888 -> RGB PNG capture encoder.
 * Stored deflate, filter 0, one IDAT: deterministic legacy Sega capture bytes.
 * Pixels are borrowed; stride is in pixels. Geometry/crop/timing stay with the
 * caller. A sink returns nonzero only after accepting the entire byte range.
 * Returns 0 on success, -1 on invalid input, allocation, or output failure.
 */
#ifndef RETRO_PRESENTATION_PNG_H
#define RETRO_PRESENTATION_PNG_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef RP_PNG_ALLOC
#define RP_PNG_ALLOC malloc
#define RP_PNG_FREE free
#endif
typedef int (*rp_png_sink)(void *user,const void *data,size_t size);
static void rp_png_be32(uint8_t *p,uint32_t value) {
    p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);
    p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;
}
static uint32_t rp_png_crc(uint32_t crc,const uint8_t *p,size_t size) {
    uint32_t table[256];
    for(uint32_t i=0;i<256;i++){
        uint32_t c=i;for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));table[i]=c;
    }
    while(size--)crc=table[(crc^*p++)&255u]^(crc>>8);
    return crc;
}
static int rp_png_chunk(rp_png_sink sink,void *user,const char *type,const uint8_t *data,uint32_t size) {
    uint8_t header[8],tail[4];rp_png_be32(header,size);memcpy(header+4,type,4);
    uint32_t crc=rp_png_crc(0xffffffffu,(const uint8_t*)type,4);
    if(size)crc=rp_png_crc(crc,data,size);rp_png_be32(tail,~crc);
    return sink(user,header,8)&&(!size||sink(user,data,size))&&sink(user,tail,4);
}
static int rp_png_write_argb_sink(rp_png_sink sink,void *user,const uint32_t *pixels,
                                  int width,int height,int stride_pixels) {
    if(!sink||!pixels||width<=0||height<=0||stride_pixels<width)return -1;
    size_t w=(size_t)width,h=(size_t)height,stride=(size_t)stride_pixels;
    if(w>(SIZE_MAX-1)/3 || h>SIZE_MAX/(1+3*w) ||
       (h-1)>(SIZE_MAX/sizeof(uint32_t)-w)/stride)return -1;
    size_t row=1+3*w,raw_size=row*h;
    size_t blocks=raw_size/65535u+(raw_size%65535u!=0);
    if(raw_size>UINT32_MAX-6u || blocks>(UINT32_MAX-6u-raw_size)/5u)return -1;
    size_t packed_size=raw_size+blocks*5u+6u;
    uint8_t *raw=(uint8_t*)RP_PNG_ALLOC(raw_size);
    if(!raw)return -1;
    for(size_t y=0;y<h;y++){
        uint8_t *out=raw+y*row;out[0]=0;
        for(size_t x=0;x<w;x++){
            uint32_t pixel=pixels[y*stride+x];
            out[1+x*3]=(uint8_t)(pixel>>16);out[2+x*3]=(uint8_t)(pixel>>8);out[3+x*3]=(uint8_t)pixel;
        }
    }
    uint8_t *packed=(uint8_t*)RP_PNG_ALLOC(packed_size);
    if(!packed){RP_PNG_FREE(raw);return -1;}
    packed[0]=0x78;packed[1]=1;
    size_t pos=2,index=0;uint32_t a=1,b=0;
    while(index<raw_size){
        size_t count=raw_size-index;if(count>65535u)count=65535u;
        packed[pos++]=(uint8_t)(index+count==raw_size);
        packed[pos++]=(uint8_t)count;packed[pos++]=(uint8_t)(count>>8);
        packed[pos++]=(uint8_t)~count;packed[pos++]=(uint8_t)(~count>>8);
        for(size_t i=0;i<count;i++,index++){
            uint8_t value=raw[index];
            packed[pos++]=value;a=(a+value)%65521u;b=(b+a)%65521u;
        }
    }
    rp_png_be32(packed+pos,(b<<16)|a);
    static const uint8_t signature[]={137,80,78,71,13,10,26,10};
    uint8_t header[13];rp_png_be32(header,(uint32_t)width);rp_png_be32(header+4,(uint32_t)height);
    header[8]=8;header[9]=2;header[10]=header[11]=header[12]=0;
    int ok=sink(user,signature,8)&&rp_png_chunk(sink,user,"IHDR",header,13)&&
        rp_png_chunk(sink,user,"IDAT",packed,(uint32_t)packed_size)&&rp_png_chunk(sink,user,"IEND",NULL,0);
    RP_PNG_FREE(packed);RP_PNG_FREE(raw);return ok?0:-1;
}
static int rp_png_file_sink(void *file,const void *data,size_t size) {
    return fwrite(data,1,size,(FILE*)file)==size;
}
static int rp_png_write_argb(const char *path,const uint32_t *pixels,int width,int height,int stride_pixels) {
    if(!path)return -1;FILE *file=fopen(path,"wb");if(!file)return -1;
    int result=rp_png_write_argb_sink(rp_png_file_sink,file,pixels,width,height,stride_pixels);
    if(fclose(file))result=-1;return result;
}
#endif
