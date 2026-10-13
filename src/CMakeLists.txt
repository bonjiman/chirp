# The chirp C++ library.
#
# Static by default; -DBUILD_SHARED_LIBS=ON builds a shared library. The
# code is position-independent either way, so the static library can be
# linked into the Python extension module.

add_library(chirp
    waveforms/vco.cc
)
add_library(chirp::chirp ALIAS chirp)

# Public headers, installed with the library. List approved headers only;
# drafts stay out until they are reviewed.
target_sources(chirp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES
            ${PROJECT_SOURCE_DIR}/include/chirp/waveforms/vco.h
)

# Private headers under src/, included as "utils/...". Never installed.
target_include_directories(chirp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(chirp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    # A Windows DLL exports every function without per-declaration macros.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
chirp_configure_target(chirp)
