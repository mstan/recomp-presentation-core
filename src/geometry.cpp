#include "retro_presentation/presentation.hpp"
#include <cmath>
namespace retro::presentation {
Rect fit_picture(Size disp, Size fb_scale, std::uint32_t tex_w, std::uint32_t tex_h,
                        std::uint32_t aspect_num, std::uint32_t aspect_den, bool integer_scale) {
    Rect r;
    if (!tex_w || !tex_h) return r;
    // The hub's letterbox, exactly as it always was: the default draws the
    // same pixels it always did.
    const float aspect = (aspect_num && aspect_den) ? float(aspect_num) / float(aspect_den)
                                                    : float(tex_w) / float(tex_h);
    float w = disp.x, h = disp.x / aspect;
    if (h > disp.y) {
        h = disp.y;
        w = disp.y * aspect;
    }
    if(!tex_w||!tex_h||disp.x<=0||disp.y<=0||!std::isfinite(disp.x)||!std::isfinite(disp.y)||!std::isfinite(fb_scale.x)||!std::isfinite(fb_scale.y))return {};
    const float sx = fb_scale.x > 0.f ? fb_scale.x : 1.f;
    const float sy = fb_scale.y > 0.f ? fb_scale.y : 1.f;
    if (integer_scale) {
        // In framebuffer pixels, where "whole" means something.
        const float n = std::floor(h * sy / float(tex_h) + 1e-3f);
        if (n >= 1.f) {
            const float ph = n * float(tex_h);
            const float pw = std::round(ph * aspect * sx / sy);
            r.w = pw / sx;
            r.h = ph / sy;
            r.x = std::floor((disp.x * sx - pw) * 0.5f) / sx;
            r.y = std::floor((disp.y * sy - ph) * 0.5f) / sy;
            return r;
        }
    }
    r.x = (disp.x - w) * 0.5f;
    r.y = (disp.y - h) * 0.5f;
    r.w = w;
    r.h = h;
    return r;
}

}
