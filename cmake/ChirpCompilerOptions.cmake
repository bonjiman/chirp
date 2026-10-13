# Compiler flags for chirp's own targets.
#
# Edit the lists below to change flags. chirp_configure_target() applies
# them to every chirp target: the library, the tests, and later the Python
# module and benchmarks. They are added PRIVATE, so they never reach
# projects that link against chirp.
#
# Each setting has a GNU list (GCC, Clang, AppleClang) and an MSVC list
# (MSVC, clang-cl).

# Warnings, always on.
set(CHIRP_WARNINGS_GNU
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
)
set(CHIRP_WARNINGS_MSVC
    /W4
    /permissive-
)

# Added when CHIRP_WARNINGS_AS_ERRORS is ON.
set(CHIRP_WERROR_GNU -Werror)
set(CHIRP_WERROR_MSVC /WX)

# Floating point, always on.
#
# -ffp-contract=off stops GCC and Clang from fusing a * b + c into one FMA
# instruction, so results do not depend on compiler defaults or on whether
# the target has FMA. MSVC contracts only under /fp:contract or /fp:fast,
# so it needs no flag.
#
# Never add -ffast-math or /fp:fast: chirp relies on IEEE NaN propagation
# and on std::isnan and std::isfinite.
set(CHIRP_FP_FLAGS_GNU -ffp-contract=off)
set(CHIRP_FP_FLAGS_MSVC)

# Added when CHIRP_SANITIZE is ON, to both compiling and linking.
set(CHIRP_SANITIZE_FLAGS_GNU
    -fsanitize=address,undefined
    -fno-sanitize-recover=undefined
    -fno-omit-frame-pointer
)
set(CHIRP_SANITIZE_FLAGS_MSVC /fsanitize=address)

# Applies the flags above, and C++20 without compiler extensions, to
# `target`.
function(chirp_configure_target target)
    if(MSVC)
        set(style MSVC)
    else()
        set(style GNU)
    endif()

    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    target_compile_options(${target} PRIVATE
        ${CHIRP_WARNINGS_${style}}
        ${CHIRP_FP_FLAGS_${style}}
    )
    if(CHIRP_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE ${CHIRP_WERROR_${style}})
    endif()
    if(CHIRP_SANITIZE)
        target_compile_options(${target} PRIVATE
            ${CHIRP_SANITIZE_FLAGS_${style}})
        if(NOT MSVC)
            target_link_options(${target} PRIVATE
                ${CHIRP_SANITIZE_FLAGS_${style}})
        endif()
    endif()
endfunction()
