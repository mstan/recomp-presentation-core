#include "retro_presentation/presentation.hpp"
#include "image.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace retro::presentation {
struct Presenter::Impl {
    explicit Impl(Host h):host(h){}
    Host host;bool opened=false,gpu=false;
    std::unique_ptr<detail::Image> image;
    Style style;
    std::vector<std::uint8_t> cpu;
    std::uint32_t width=0,height=0,aspect_num=0,aspect_den=0,limit=16384;
    bool current(){return opened&&(!host.make_current||host.make_current(host.user));}
    bool extent(std::uint32_t w,std::uint32_t h)const{return w&&h&&w<=limit&&h<=limit&&std::uint64_t(w)*h<=64ull*1024*1024;}
};
Presenter::Presenter(Host h):impl_(std::make_unique<Impl>(h)){}
Presenter::~Presenter(){close();}
bool Presenter::open(){
    auto& s=*impl_;if(s.opened)return true;
    if(s.host.open&&!s.host.open(s.host.user))return false;
    s.opened=true;
    if(!s.current()){close();return false;}
    auto image=std::make_unique<detail::Image>();image->g_.load(s.host);
    if(image->g_.complete&&s.host.draw_texture){
        detail::GLint limit=0;image->g_.GetIntegerv(0x0D33,&limit);
        if(limit>0){s.limit=std::min<std::uint32_t>(16384,static_cast<std::uint32_t>(limit));image->limit_=s.limit;
            image->style_=s.style;s.image=std::move(image);s.gpu=true;return true;}
    }
    if(!s.host.draw_cpu){close();return false;}
    s.gpu=false;s.limit=16384;return true;
}
void Presenter::close(){
    auto& s=*impl_;if(!s.opened)return;
    if(s.image){if(s.current())s.image->release();s.image.reset();}
    s.cpu.clear();s.width=s.height=0;s.gpu=false;s.opened=false;
    if(s.host.close)s.host.close(s.host.user);
}
bool Presenter::is_open()const{return impl_->opened;}
bool Presenter::uses_gpu()const{return impl_->gpu;}
bool Presenter::supports_pixel_pairs()const{return impl_->opened&&(!impl_->gpu||impl_->image->supports_pixel_pairs());}
void Presenter::set_style(Style style){impl_->style=style;if(impl_->image)impl_->image->style_=style;}
bool Presenter::upload(const void* rgba,std::uint32_t w,std::uint32_t h,std::uint32_t an,std::uint32_t ad){
    auto& s=*impl_;if(!rgba||!s.extent(w,h)||!s.current())return false;
    if(s.gpu)s.image->upload(rgba,w,h,an,ad);
    else {const auto* begin=static_cast<const std::uint8_t*>(rgba);s.cpu.assign(begin,begin+std::size_t(w)*h*4);}
    s.width=w;s.height=h;s.aspect_num=an;s.aspect_den=ad;return true;
}
bool Presenter::upload_pixel_pairs(const void* rgba,std::uint32_t w,std::uint32_t h,std::uint32_t an,std::uint32_t ad){
    auto& s=*impl_;if(!rgba||w>UINT32_MAX/2||!s.extent(w*2,h)||!s.current())return false;
    if(s.gpu){if(!s.image->upload_pixel_pairs(rgba,w,h,an,ad))return false;}
    else {std::vector<std::uint8_t> pixels(std::size_t(w)*2*h*4);const auto* source=static_cast<const std::uint8_t*>(rgba);
        for(std::size_t i=0;i<std::size_t(w)*h;++i){std::memcpy(pixels.data()+i*8,source+i*4,4);std::memcpy(pixels.data()+i*8+4,source+i*4,4);}s.cpu=std::move(pixels);}
    s.width=w*2;s.height=h;s.aspect_num=an;s.aspect_den=ad;return true;
}
bool Presenter::copy_texture(unsigned texture,std::uint32_t w,std::uint32_t h,std::uint32_t an,std::uint32_t ad){
    auto& s=*impl_;if(!s.gpu||!s.extent(w,h)||!s.current()||!s.image->copy_texture(texture,w,h,an,ad))return false;
    s.width=w;s.height=h;s.aspect_num=an;s.aspect_den=ad;return true;
}
std::vector<std::uint8_t> Presenter::snapshot(std::uint32_t& w,std::uint32_t& h){
    auto& s=*impl_;w=h=0;if(!s.current()||!s.width||!s.height)return {};
    if(s.gpu){if(!s.image->g_.blit)return {};return s.image->snapshot(w,h);}
    w=s.width;h=s.height;return s.cpu;
}
Prepared Presenter::prepare(Size display,Size scale){
    auto& s=*impl_;if(!s.current()||!s.width||!s.height||display.x>1000000||display.y>1000000||scale.x>64||scale.y>64)return {};
    const auto rectangle=fit_picture(display,scale,s.width,s.height,s.aspect_num,s.aspect_den,s.style.integer_scale);
    if(rectangle.w<=0||rectangle.h<=0)return {};
    if(s.gpu)return s.image->prepare(display,scale);
    return {0,rectangle,s.cpu.data(),s.width,s.height,s.style.filter==Filter::SharpBilinear?Filter::Bilinear:s.style.filter};
}
bool Presenter::present(Size display,Size scale){
    auto& s=*impl_;const auto frame=prepare(display,scale);if(!frame.width||!frame.height)return false;
    if(s.gpu)s.host.draw_texture(s.host.user,frame.texture,frame.rectangle);
    else s.host.draw_cpu(s.host.user,frame.rgba,frame.width,frame.height,frame.rectangle,frame.filter);
    if(s.host.swap)s.host.swap(s.host.user);
    return true;
}
}
