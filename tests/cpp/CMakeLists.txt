# C++ tests, in one Catch2 executable. Test files mirror src/.

include(FetchContent)

# Uses an installed Catch2 3 if find_package() finds one; otherwise fetches
# the pinned tag. Build Catch2 as C++20 too: its compiled library must use
# the same standard as the tests.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS 3
)
FetchContent_MakeAvailable(Catch2)

add_executable(chirp_tests
    utils/phase_accumulator_test.cc
    waveforms/vco_test.cc
)
target_link_libraries(chirp_tests PRIVATE chirp::chirp Catch2::Catch2WithMain)
# Tests may include private headers under src/, as "utils/...".
target_include_directories(chirp_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
chirp_configure_target(chirp_tests)

# Registers each TEST_CASE with ctest. A fetched Catch2 keeps Catch.cmake
# in its source tree; an installed one puts it on the module path.
if(catch2_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()
include(Catch)
catch_discover_tests(chirp_tests)
