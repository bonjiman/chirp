cmake_minimum_required(VERSION 3.25)

project(chirp
    VERSION 0.1.0
    DESCRIPTION "RF DSP simulation and waveform development"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Build options
#
# Set with -D<NAME>=ON|OFF, or per configuration in CMakePresets.json.
# The standard BUILD_SHARED_LIBS option selects a shared chirp library;
# chirp is static by default. Compiler flags live in
# cmake/ChirpCompilerOptions.cmake.
# ---------------------------------------------------------------------------
option(CHIRP_BUILD_TESTS
    "Build the C++ tests" ${PROJECT_IS_TOP_LEVEL})
option(CHIRP_INSTALL
    "Generate install rules and the chirp CMake package"
    ${PROJECT_IS_TOP_LEVEL})
option(CHIRP_SANITIZE
    "Build chirp targets with AddressSanitizer and UBSan" OFF)
option(CHIRP_WARNINGS_AS_ERRORS
    "Treat compiler warnings as errors in chirp targets" OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(ChirpCompilerOptions)

add_subdirectory(src)

if(CHIRP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()

if(CHIRP_INSTALL)
    include(ChirpInstall)
endif()
