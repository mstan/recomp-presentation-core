#pragma once
#include "gl.hpp"
namespace retro::presentation::detail {
class Image {
public:
 Gl g_;
 std::uint32_t limit_=16384;
 Style style_;
 void upload(const void*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
 bool supports_pixel_pairs() const;
 bool upload_pixel_pairs(const void*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
 bool copy_texture(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
 std::vector<std::uint8_t> snapshot(std::uint32_t&,std::uint32_t&);
 Prepared prepare(Size,Size);
 void release();
private:
 bool copy_texture_sized(unsigned,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
 void set_minified(bool);
 unsigned prescaled(std::uint32_t,std::uint32_t);
 unsigned tex_=0,pair_source_=0,pre_=0,read_fb_=0,draw_fb_=0;
 std::uint32_t pair_w_=0,pair_h_=0,w_=0,h_=0,aspect_num_=0,aspect_den_=0,pre_w_=0,pre_h_=0;
 std::uint64_t rev_=0,pre_rev_=0;
 int mag_=0;bool mipmapped_=false;
};
}
