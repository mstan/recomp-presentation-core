# recomp-presentation-core

Current licensing and the grant for existing owner contributions are documented in [LICENSE-SCOPE.md](LICENSE-SCOPE.md).

Shared host-presentation provider. The implementation includes Retro-Launcher's GL image pipeline:
uploads and reuse, same-context GPU copies, exact pixel-pair reconstruction,
aspect/integer/DPI geometry, sharp-bilinear prescaling, mipmap reuse and explicit
snapshot observers. The rational host pacing header comes from Genesis; its
clocks never advance guest simulation.

`layout.hpp` provides exact reduced-aspect, rounded adaptive fit and integer fit
with proportional downscale, inverse pointer mapping and sharp prescale selection.
These retain the GBA adapter's existing geometry policies as explicit functions.
`capture.h` provides SMS/Game Gear's nearest ARGB capture fit, including crop
stride, opaque alpha and centered black bars.

The optional `sdl2.h`/`src/sdl2.c` backend borrows an existing SDL2 renderer and
owns upload texture reuse, bounded allocation halving and a cached sharp target.
Compile that C source into an existing SDL2 host and use the public include
directory; the base library acquires no SDL dependency. The host retains windows,
renderer/context lifetime, pixel production/color grading, UI, clear/present and
pacing. Resident game textures stay borrowed. Destroy the backend before its
renderer. Filtering may be explicit or inherit the existing SDL quality hint.
The GBA and SMS/Game Gear adapters call this same backend. Its software-surface
fixture compares real pixels across three upload formats, cropped and sharp
copies, filtering and resize lifetime without creating a desktop window.

`Presenter` owns one session. The frontend supplies its existing window/context
open/close/current, GL procedure resolver, draw and swap callbacks. This keeps
recomp-ui and the frontend in control of windows and menus. There is no SDL or
ImGui dependency and the library creates no extra player window. GL procedures
are resolved per session rather than cached across unrelated contexts. The GL
host supplies ordinary pixel-transfer state: no bound pack/unpack buffers, zero
row lengths and row/pixel skips. The image pipeline changes texture bindings and
alignment; hosts isolate that state at their existing presentation boundary.

Hosts without compatible GL delivery can supply the portable CPU RGBA callback
under the same interface. That path supports snapshots and exact pixel pairs;
optional sharp-bilinear filtering uses ordinary bilinear presentation there.
The GPU path never reads pixels back during steady prepare/present. Snapshot is
an explicit observer and requires framebuffer support on the GL backend; an
empty result with zero dimensions reports unavailable capture.

Prepared CPU pixels are borrowed until upload/close. Textures remain owned by
their session and may only be used in its current host context. Calls on one
session must be serialized and callbacks must outlive it. On context loss, the
host owner must reclaim the context during teardown. Same-context GPU copies
never transfer process-local texture names across a transport.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --timeout 0
```

Use `add_subdirectory` and link `retro-runtime::presentation`. The implementation
requires C++17. `retro_presentation/pacing.h` is also usable from C; the host frame
API is `retro_presentation/presentation.hpp`. This source package is intended for
the existing Retro-Runtime/recomp-ui host boundary. Guest GPU commands, game
shaders, device timing and simulation scheduling remain engine-owned.

The tests cover geometry, frame ownership, independent sessions, reset/reopen,
capture, portable fallback, rational pacing and invalid rates. The Windows test
creates an unseen, nonactivating WGL window and checks real upload, framebuffer
copy, cached sharp-bilinear textures, minification and pixel-pair readback. A
readback counter also verifies that steady presentation has no readback calls.
These checks do not establish gameplay acceptance, production window integration,
external-memory portability or a measured runtime improvement.

The image pipeline retains its MIT notice (`LICENSE`). Matthew Stanley's pacing,
layout, capture and SDL2 backend contributions are also offered under MIT; see
`LICENSE-SCOPE.md`. The historical Genesis notice remains in `LICENSES/Genesis.txt`
for provenance. Exact original donor identities are recorded in `PROVENANCE.json`.
