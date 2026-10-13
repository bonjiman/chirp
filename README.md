# chirp

chirp is a C++ library for RF DSP simulation and waveform development.
A Python API backed by the C++ core is planned.

chirp is at an early stage. Its functions are added one at a time and
reviewed before the API grows. Today it provides a voltage-controlled
oscillator (`<chirp/waveforms/vco.h>`). Python bindings are not available
yet.

## Requirements

- A C++20 compiler. Development and testing use GCC 15 on Linux.
- CMake 3.25 or later. Ninja is optional but recommended.
- For the tests only, [Catch2](https://github.com/catchorg/Catch2) v3.
  CMake uses an installed Catch2 3 if it finds one. Otherwise it fetches
  Catch2 with Git and builds it from source in each build directory, which
  adds about 40 seconds to the first build. The library itself has no
  third-party dependencies, and projects that use chirp never build
  Catch2.

On Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build git catch2
```

## Building

The repository includes CMake presets:

| Preset           | Build type | Notes                                       |
|------------------|------------|---------------------------------------------|
| `dev`            | Debug      | AddressSanitizer, UBSan, warnings as errors |
| `release`        | Release    | Static library                              |
| `release-shared` | Release    | Shared library                              |

Each preset builds into `build/<preset>/`. To configure, build and run the
tests:

```sh
export CMAKE_GENERATOR=Ninja    # optional; the default is Makefiles
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Without presets, the usual CMake commands work:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

### Options

| Option                     | Default           | Effect                   |
|----------------------------|-------------------|--------------------------|
| `BUILD_SHARED_LIBS`        | `OFF`             | Build a shared library   |
| `CHIRP_BUILD_TESTS`        | `ON` if top-level | Build the C++ tests      |
| `CHIRP_INSTALL`            | `ON` if top-level | Generate install rules   |
| `CHIRP_SANITIZE`           | `OFF`             | ASan and UBSan           |
| `CHIRP_WARNINGS_AS_ERRORS` | `OFF`             | Treat warnings as errors |

"Top-level" means chirp is the project being built, not a dependency of
another project. Compiler flags, including warnings and floating-point
settings, are kept in `cmake/ChirpCompilerOptions.cmake`.

## Using chirp from another CMake project

Every method below provides the target `chirp::chirp`, which carries
chirp's include path and its C++20 requirement.

### From an installed copy

Install chirp to a prefix:

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix "$HOME/.local"
```

Then find it from your project:

```cmake
find_package(chirp 0.1 REQUIRED)
target_link_libraries(my_app PRIVATE chirp::chirp)
```

If the prefix is not a standard location, tell CMake where it is with
`-DCMAKE_PREFIX_PATH="$HOME/.local"`.

### As a subproject

Build chirp as part of your project with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(chirp
    GIT_REPOSITORY <chirp repository URL>
    GIT_TAG <commit or tag>
)
FetchContent_MakeAvailable(chirp)
target_link_libraries(my_app PRIVATE chirp::chirp)
```

or, with a copy of the source tree, `add_subdirectory(path/to/chirp)`.
chirp's tests and install rules are off by default in this case.

## Example

```cpp
#include <chirp/waveforms/vco.h>

#include <complex>
#include <vector>

int main() {
    using namespace chirp::waveforms;

    // 1 kHz carrier, 50 Hz of deviation per unit of control, at 48 kHz.
    const VcoParams p{.sensitivity = 50.0, .sample_rate = 48e3,
                      .carrier = 1e3};
    const std::vector<double> x(4800, 1.0);  // control signal

    // Allocating form: returns the samples.
    std::vector<std::complex<double>> z = vco_cexp(x, p);

    // Span form: fills a caller-owned buffer and returns the phase of the
    // next sample, for continuing the signal in a later call.
    std::vector<double> out(x.size());
    double next_phase = vco_sin(x, out, p, /*initial_phase=*/0.5);
    vco_sin(x, out, p, next_phase);
}
```

All functions are in the `chirp` namespace and take contiguous 1-D inputs
as `std::span`. Invalid arguments throw `std::invalid_argument`.

## Documentation

Function behavior is described in `docs/specs/`, which mirrors
`include/chirp/`. For example, the VCO is documented in
[`docs/specs/waveforms/vco.md`](docs/specs/waveforms/vco.md).

## Repository layout

```
include/chirp/   public headers
src/             implementation and private headers
tests/cpp/       C++ tests (Catch2), mirroring src/
docs/specs/      function documentation
cmake/           CMake modules: compiler flags, install rules
python/          Python package and bindings (planned)
```
