// Extracted Retro-Launcher image ownership, filtering, GPU copy and snapshot.
#include "image.hpp"
#include <algorithm>
#include <cmath>
namespace retro::presentation::detail {
static std::uint32_t whole_multiple(float pixels,std::uint32_t size){return std::max<std::uint32_t>(1,static_cast<std::uint32_t>(pixels/float(size)+1e-3f));}
#include "image_body.inc"
}
