// MIT: extracted from Retro-Launcher Picture; see PROVENANCE.json.
#pragma once
#include <cstdint>
#include <memory>
#include <vector>
namespace retro::presentation {
struct Size {float x=0,y=0;};
struct Rect {float x=0,y=0,w=0,h=0;};
enum class Filter {Bilinear,Nearest,SharpBilinear};
struct Style {Filter filter=Filter::Bilinear;bool integer_scale=false;};
Rect fit_picture(Size display,Size framebuffer_scale,std::uint32_t width,std::uint32_t height,
                 std::uint32_t aspect_num,std::uint32_t aspect_den,bool integer_scale);
struct Host {
    void* user=nullptr;
    // A session owns the successful open/close pair. The host decides whether
    // this creates a window/context or borrows one; no UI framework is required.
    bool (*open)(void*)=nullptr;
    void (*close)(void*)=nullptr;
    bool (*make_current)(void*)=nullptr;
    void* (*get_proc)(void*,const char*)=nullptr;
    void (*draw_texture)(void*,unsigned texture,const Rect&)=nullptr;
    void (*draw_cpu)(void*,const std::uint8_t*,std::uint32_t,std::uint32_t,const Rect&,Filter)=nullptr;
    void (*swap)(void*)=nullptr;
};
struct Prepared {
    unsigned texture=0;
    Rect rectangle{};
    const std::uint8_t* rgba=nullptr;
    std::uint32_t width=0,height=0;
    Filter filter=Filter::Bilinear;
};
class Presenter {
public:
    explicit Presenter(Host host);
    ~Presenter();
    Presenter(const Presenter&)=delete;
    Presenter& operator=(const Presenter&)=delete;
    bool open();
    void close();
    bool is_open() const;
    bool uses_gpu() const;
    bool supports_pixel_pairs() const;
    void set_style(Style);
    // Tightly-packed RGBA8; input is borrowed only until return. Geometry and
    // allocations are checked before replacing the last successfully held frame.
    bool upload(const void*,std::uint32_t width,std::uint32_t height,
                std::uint32_t aspect_num=0,std::uint32_t aspect_den=0);
    bool upload_pixel_pairs(const void*,std::uint32_t width,std::uint32_t height,
                            std::uint32_t aspect_num=0,std::uint32_t aspect_den=0);
    // Same-context texture only. No process-local texture name crosses a link.
    bool copy_texture(unsigned,std::uint32_t width,std::uint32_t height,
                       std::uint32_t aspect_num=0,std::uint32_t aspect_den=0);
    // Explicit observer only; steady GPU presentation never reads pixels back.
    std::vector<std::uint8_t> snapshot(std::uint32_t& width,std::uint32_t& height);
    // Returned CPU pixels remain valid until upload/close. The texture remains
    // owned by this session and is used only with its current host context.
    Prepared prepare(Size display,Size framebuffer_scale={1,1});
    bool present(Size display,Size framebuffer_scale={1,1});
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
