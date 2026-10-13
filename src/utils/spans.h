#pragma once

#include <cstdint>
#include <span>

namespace chirp::detail {

/**
 * Reports whether two spans share any memory. Empty spans never overlap.
 * @param a First span.
 * @param b Second span; its element type may differ from `a`'s.
 * @return True if any byte of `a` is also a byte of `b`.
 */
template <class A, class B>
bool overlaps(std::span<A> a, std::span<B> b) {
    if (a.empty() || b.empty()) {
        return false;
    }
    const auto a0 = reinterpret_cast<std::uintptr_t>(a.data());
    const auto b0 = reinterpret_cast<std::uintptr_t>(b.data());
    return a0 < b0 + b.size_bytes() && b0 < a0 + a.size_bytes();
}

}  // namespace chirp::detail
