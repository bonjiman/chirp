#pragma once

#include <numbers>

namespace chirp::detail {

/// 2π. The standard library has std::numbers::pi but no 2π.
inline constexpr double two_pi = 2.0 * std::numbers::pi;

}  // namespace chirp::detail
