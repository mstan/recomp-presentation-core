void Image::upload(const void* rgba, std::uint32_t w, std::uint32_t h, std::uint32_t aspect_num,
                     std::uint32_t aspect_den) {
    if (!tex_) {
        g_.GenTextures(1, &tex_);
        g_.BindTexture(GL_TEXTURE_2D, tex_);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        mag_ = GL_LINEAR;
        mipmapped_ = false;
    }
    g_.BindTexture(GL_TEXTURE_2D, tex_);
    g_.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (w != w_ || h != h_) {
        g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GLsizei(w), GLsizei(h), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, rgba);
        w_ = w;
        h_ = h;
    } else {
        g_.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GLsizei(w), GLsizei(h), GL_RGBA, GL_UNSIGNED_BYTE,
                        rgba);
    }
    aspect_num_ = aspect_num;
    aspect_den_ = aspect_den;
    ++rev_;
    // The chain follows the frame (and a resized level 0 leaves it incomplete).
    if (mipmapped_) g_.GenerateMipmap(GL_TEXTURE_2D);
}

void Image::set_minified(bool on) {
    if (on == mipmapped_ || (on && !g_.GenerateMipmap)) return;
    g_.BindTexture(GL_TEXTURE_2D, tex_);
    if (on) g_.GenerateMipmap(GL_TEXTURE_2D);
    g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, on ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    mipmapped_ = on;
}

bool Image::supports_pixel_pairs() const { return g_.blit; }

bool Image::upload_pixel_pairs(const void* rgba, std::uint32_t w, std::uint32_t h,
                                 std::uint32_t aspect_num, std::uint32_t aspect_den) {
    if (!rgba || !w || !h || w > UINT32_MAX / 2 || !supports_pixel_pairs()) return false;
    if (!pair_source_) {
        g_.GenTextures(1, &pair_source_);
        g_.BindTexture(GL_TEXTURE_2D, pair_source_);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    g_.BindTexture(GL_TEXTURE_2D, pair_source_);
    g_.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (w != pair_w_ || h != pair_h_) {
        g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GLsizei(w), GLsizei(h), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        pair_w_ = w; pair_h_ = h;
    } else {
        g_.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GLsizei(w), GLsizei(h), GL_RGBA,
                        GL_UNSIGNED_BYTE, rgba);
    }
    // Canonical tex_ stays 2*w wide, so mipmaps, snapshots, sharp-bilinear
    // scaling and integer-scale decisions see exactly the legacy image.
    return copy_texture_sized(pair_source_, w, h, w * 2, h, aspect_num, aspect_den);
}

bool Image::copy_texture(unsigned int texture, std::uint32_t w, std::uint32_t h,
                           std::uint32_t aspect_num, std::uint32_t aspect_den) {
    return copy_texture_sized(texture, w, h, w, h, aspect_num, aspect_den);
}

bool Image::copy_texture_sized(unsigned int texture, std::uint32_t sw, std::uint32_t sh,
                                std::uint32_t w, std::uint32_t h,
                                std::uint32_t aspect_num, std::uint32_t aspect_den) {
    const auto& g = g_;
    if (!g.blit || !texture || !w || !h) return false;
    if (!tex_ || w != w_ || h != h_) upload(nullptr, w, h, aspect_num, aspect_den);
    if (!read_fb_) g.GenFramebuffers(1, &read_fb_);
    if (!draw_fb_) g.GenFramebuffers(1, &draw_fb_);
    GLint read = 0, draw = 0;
    g_.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    g_.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    const bool scissor = g_.IsEnabled(GL_SCISSOR_TEST);
    if (scissor) g_.Disable(GL_SCISSOR_TEST);
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, read_fb_);
    g.FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    g.BindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb_);
    g.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
    const bool ok = g.CheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
                    g.CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (ok) g.BlitFramebuffer(0, 0, GLint(sw), GLint(sh), 0, 0, GLint(w), GLint(h),
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read));
    g.BindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(draw));
    if (scissor) g_.Enable(GL_SCISSOR_TEST);
    if (!ok) return false;
    aspect_num_ = aspect_num;
    aspect_den_ = aspect_den;
    ++rev_;
    if (mipmapped_) {
        g_.BindTexture(GL_TEXTURE_2D, tex_);
        g.GenerateMipmap(GL_TEXTURE_2D);
    }
    return true;
}

std::vector<std::uint8_t> Image::snapshot(std::uint32_t& width, std::uint32_t& height) {
    width = height = 0;
    const auto& g = g_;
    if (!tex_ || !g.blit) return {};
    if (!read_fb_) g.GenFramebuffers(1, &read_fb_);
    GLint read = 0, alignment = 0;
    g_.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    g_.GetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, read_fb_);
    g.FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
    std::vector<std::uint8_t> pixels;
    if (g.CheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        pixels.resize(std::size_t(w_) * h_ * 4);
        g_.PixelStorei(GL_PACK_ALIGNMENT, 4);
        g_.ReadPixels(0, 0, GLsizei(w_), GLsizei(h_), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        width = w_; height = h_;
    }
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read));
    g_.PixelStorei(GL_PACK_ALIGNMENT, alignment);
    return pixels;
}

// tex_ blown up kx by ky with nearest, into pre_: redone per new frame or size.
// 0 when it cannot be (the caller draws tex_ instead).
unsigned int Image::prescaled(std::uint32_t kx, std::uint32_t ky) {
    const Gl& g = g_;
    if (!g.blit) return 0;
    if(!w_||!h_||kx>limit_/w_||ky>limit_/h_)return 0;
    const std::uint32_t pw = w_ * kx, ph = h_ * ky;
    if (!pre_) {
        g_.GenTextures(1, &pre_);
        g_.BindTexture(GL_TEXTURE_2D, pre_);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (!read_fb_) g.GenFramebuffers(1, &read_fb_);
        if (!draw_fb_) g.GenFramebuffers(1, &draw_fb_);
    }
    if (pw != pre_w_ || ph != pre_h_) {
        g_.BindTexture(GL_TEXTURE_2D, pre_);
        g_.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GLsizei(pw), GLsizei(ph), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        pre_w_ = pw;
        pre_h_ = ph;
        pre_rev_ = 0; // rev_ is at least 1 once a frame is up
    }
    if (pre_rev_ == rev_) return pre_;

    // A blit bypasses the fragment pipeline but not the scissor test; the
    // bindings go back to what ImGui's frame had.
    GLint was_read = 0, was_draw = 0;
    g_.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &was_read);
    g_.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &was_draw);
    const GLboolean scissor = g_.IsEnabled(GL_SCISSOR_TEST);
    if (scissor) g_.Disable(GL_SCISSOR_TEST);
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, read_fb_);
    g.FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
    g.BindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb_);
    g.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pre_, 0);
    const bool ok = g.CheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
                    g.CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (ok)
        g.BlitFramebuffer(0, 0, GLint(w_), GLint(h_), 0, 0, GLint(pw), GLint(ph),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    g.BindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(was_read));
    g.BindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(was_draw));
    if (scissor) g_.Enable(GL_SCISSOR_TEST);
    if (!ok) {
        return 0;
    }
    pre_rev_ = rev_;
    return pre_;
}

Prepared Image::prepare(Size display,Size scale) {
    if (!tex_ || !w_ || !h_) return {};
    const Rect r = fit_picture(display, scale, w_, h_,
                                      aspect_num_, aspect_den_, style_.integer_scale);
    // What the frame is scaled by, in framebuffer pixels.
    const float px_w = r.w * (scale.x > 0.f ? scale.x : 1.f);
    const float px_h = r.h * (scale.y > 0.f ? scale.y : 1.f);
    set_minified(px_w < float(w_) || px_h < float(h_));
    const int mag = style_.filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR;
    if (mag != mag_) {
        g_.BindTexture(GL_TEXTURE_2D, tex_);
        g_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
        mag_ = mag;
    }
    unsigned int tex = tex_;
    if (style_.filter == Filter::SharpBilinear && !mipmapped_) {
        const std::uint32_t kx = whole_multiple(px_w, w_), ky = whole_multiple(px_h, h_);
        if (kx > 1 || ky > 1)
            if (const unsigned int p = prescaled(kx, ky)) tex = p;
    }
    return {tex,r,nullptr,w_,h_,style_.filter};
}

void Image::release() {
    if (tex_) g_.DeleteTextures(1, &tex_);
    if (pair_source_) g_.DeleteTextures(1, &pair_source_);
    if (pre_) g_.DeleteTextures(1, &pre_);
    if (read_fb_ || draw_fb_) {
        const Gl& g = g_;
        if (read_fb_) g.DeleteFramebuffers(1, &read_fb_);
        if (draw_fb_) g.DeleteFramebuffers(1, &draw_fb_);
    }
    tex_ = pre_ = pair_source_ = read_fb_ = draw_fb_ = 0;
    pair_w_ = pair_h_ = 0;
    w_ = h_ = pre_w_ = pre_h_ = 0;
    mag_ = 0;
    mipmapped_ = false;
}
