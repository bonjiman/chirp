#include <chirp/waveforms/vco.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <numbers>
#include <span>
#include <stdexcept>
#include <vector>

using namespace chirp::waveforms;
using Catch::Matchers::WithinAbs;

namespace {

constexpr double two_pi = 2.0 * std::numbers::pi;
constexpr double quiet_nan = std::numeric_limits<double>::quiet_NaN();
constexpr double infinity = std::numeric_limits<double>::infinity();

/// A deterministic, non-constant control signal of length `n`.
std::vector<double> wobble(std::size_t n) {
    std::vector<double> x(n);
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = std::sin(0.01 * static_cast<double>(i)) + 0.3;
    }
    return x;
}

/// Distance between two angles in radians, ignoring whole turns.
double angle_distance(double a, double b) {
    return std::abs(std::remainder(a - b, two_pi));
}

}  // namespace

TEST_CASE("vco_cexp: constant control gives the closed-form tone") {
    const VcoParams p{.sensitivity = 50.0, .sample_rate = 48e3,
                      .carrier = 1e3};
    const double level = 2.0;
    const double f = p.carrier + p.sensitivity * level;
    const double phase0 = GENERATE(0.0, 0.5, -2.0);
    const std::vector<double> x(4096, level);

    const auto out = vco_cexp(x, p, phase0);

    REQUIRE(out.size() == x.size());
    for (std::size_t n = 0; n < out.size(); ++n) {
        const double phi =
            two_pi * f * static_cast<double>(n) / p.sample_rate + phase0;
        CHECK_THAT(out[n].real(), WithinAbs(std::cos(phi), 1e-11));
        CHECK_THAT(out[n].imag(), WithinAbs(std::sin(phi), 1e-11));
    }
}

TEST_CASE("vco: the first sample is at initial_phase, before any control") {
    // control[0] first affects out[1].
    const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0};
    const std::vector<double> x{0.25, 0.0};
    const auto out = vco_cos(x, p, std::numbers::pi);
    CHECK(out[0] == -1.0);
    CHECK_THAT(out[1], WithinAbs(std::cos(std::numbers::pi * 1.5), 1e-15));
}

TEST_CASE("vco_sin and vco_cos are the parts of vco_cexp") {
    const VcoParams p{.sensitivity = 0.05, .sample_rate = 1.0,
                      .carrier = 0.1};
    const auto x = wobble(1000);
    const auto z = vco_cexp(x, p, 1.0);
    const auto s = vco_sin(x, p, 1.0);
    const auto c = vco_cos(x, p, 1.0);
    for (std::size_t n = 0; n < x.size(); ++n) {
        CHECK_THAT(c[n], WithinAbs(z[n].real(), 1e-15));
        CHECK_THAT(s[n], WithinAbs(z[n].imag(), 1e-15));
    }
}

TEST_CASE("vco: empty control gives empty output and the wrapped phase") {
    const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0};
    const std::vector<double> x;

    CHECK(vco_cexp(x, p).empty());
    CHECK(vco_sin(x, p).empty());
    CHECK(vco_cos(x, p).empty());
    CHECK(vco_square(x, p).empty());

    std::vector<std::complex<double>> out;
    const double phase0 = GENERATE(0.0, 1.0, -std::numbers::pi / 2, 7.0);
    const double next = vco_cexp(x, out, p, phase0);
    CHECK(next >= 0.0);
    CHECK(next < two_pi);
    CHECK_THAT(angle_distance(next, phase0), WithinAbs(0.0, 1e-14));
}

TEST_CASE("vco_cexp: generating in two pieces matches one call") {
    const VcoParams p{.sensitivity = 0.05, .sample_rate = 1.0,
                      .carrier = 0.1};
    const auto x = wobble(1000);
    const std::span<const double> xs(x);
    const double phase0 = 0.7;

    std::vector<std::complex<double>> whole(x.size());
    const double whole_next = vco_cexp(xs, whole, p, phase0);

    const std::size_t split = GENERATE(0u, 1u, 17u, 500u, 999u, 1000u);
    std::vector<std::complex<double>> pieces(x.size());
    const std::span<std::complex<double>> ps(pieces);
    const double mid = vco_cexp(xs.first(split), ps.first(split), p, phase0);
    const double pieces_next =
        vco_cexp(xs.subspan(split), ps.subspan(split), p, mid);

    for (std::size_t n = 0; n < x.size(); ++n) {
        CHECK_THAT(pieces[n].real(), WithinAbs(whole[n].real(), 1e-12));
        CHECK_THAT(pieces[n].imag(), WithinAbs(whole[n].imag(), 1e-12));
    }
    CHECK_THAT(angle_distance(pieces_next, whole_next),
               WithinAbs(0.0, 1e-12));
}

TEST_CASE("vco: the returned phase lies in [0, 2pi)") {
    const double carrier = GENERATE(0.0, 0.3, -0.3, 0.5, 2.75, -7.1);
    const double phase0 = GENERATE(0.0, -100.0, 100.0);
    const VcoParams p{.sensitivity = 0.2, .sample_rate = 1.0,
                      .carrier = carrier};
    const auto x = wobble(257);
    std::vector<double> out(x.size());

    const double next = vco_sin(x, out, p, phase0);
    CHECK(next >= 0.0);
    CHECK(next < two_pi);
}

TEST_CASE("vco: span and allocating forms give identical samples") {
    const VcoParams p{.sensitivity = 0.05, .sample_rate = 1.0,
                      .carrier = 0.1};
    const auto x = wobble(1000);
    const double phase0 = 0.3;
    std::vector<std::complex<double>> z(x.size());
    std::vector<double> r(x.size());

    vco_cexp(x, z, p, phase0);
    CHECK(vco_cexp(x, p, phase0) == z);

    vco_sin(x, r, p, phase0);
    CHECK(vco_sin(x, p, phase0) == r);

    vco_cos(x, r, p, phase0);
    CHECK(vco_cos(x, p, phase0) == r);

    vco_square(x, r, p, phase0);
    CHECK(vco_square(x, p, phase0) == r);
}

TEST_CASE("vco is vco_cexp") {
    const VcoParams p{.sensitivity = 0.05, .sample_rate = 1.0,
                      .carrier = 0.1};
    const auto x = wobble(100);
    std::vector<std::complex<double>> a(x.size());
    std::vector<std::complex<double>> b(x.size());

    CHECK(vco(x, a, p, 0.3) == vco_cexp(x, b, p, 0.3));
    CHECK(a == b);
    CHECK(vco(x, p, 0.3) == vco_cexp(x, p, 0.3));
}

TEST_CASE("vco_square: edges fall exactly at 0.25 and 0.75 cycles") {
    const std::vector<double> x(10, 0.0);

    SECTION("quarter-cycle steps hit the zero crossings") {
        // Phases 0, .25, .5, .75, 0, ...; +1 at zero crossings.
        const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0,
                          .carrier = 0.25};
        const std::vector<double> expected{1, 1, -1, 1, 1, 1, -1, 1, 1, 1};
        CHECK(vco_square(x, p) == expected);
    }
    SECTION("eighth-cycle steps") {
        const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0,
                          .carrier = 0.125};
        const std::vector<double> expected{1, 1, 1, -1, -1, -1, 1, 1, 1, 1};
        CHECK(vco_square(x, p) == expected);
    }
}

TEST_CASE("vco: invalid parameters throw std::invalid_argument") {
    const std::vector<double> x(4, 0.0);
    const VcoParams ok{.sensitivity = 1.0, .sample_rate = 1.0};

    VcoParams bad = ok;
    SECTION("sample_rate") {
        bad.sample_rate = GENERATE(0.0, -1.0, quiet_nan, infinity);
    }
    SECTION("sensitivity") {
        bad.sensitivity = GENERATE(quiet_nan, infinity, -infinity);
    }
    SECTION("carrier") {
        bad.carrier = GENERATE(quiet_nan, infinity, -infinity);
    }
    CHECK_THROWS_AS(vco_cexp(x, bad), std::invalid_argument);
    CHECK_THROWS_AS(vco_sin(x, bad), std::invalid_argument);
    CHECK_THROWS_AS(vco_cos(x, bad), std::invalid_argument);
    CHECK_THROWS_AS(vco_square(x, bad), std::invalid_argument);
    CHECK_THROWS_AS(vco(x, bad), std::invalid_argument);
}

TEST_CASE("vco: a non-finite initial_phase throws std::invalid_argument") {
    const std::vector<double> x(4, 0.0);
    const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0};
    const double phase0 = GENERATE(quiet_nan, infinity, -infinity);
    std::vector<double> out(x.size());

    CHECK_THROWS_AS(vco_cexp(x, p, phase0), std::invalid_argument);
    CHECK_THROWS_AS(vco_square(x, out, p, phase0), std::invalid_argument);
}

TEST_CASE("vco: mismatched lengths throw std::invalid_argument") {
    const std::vector<double> x(4, 0.0);
    const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0};
    const std::size_t size = GENERATE(0u, 3u, 5u);
    std::vector<double> r(size);
    std::vector<std::complex<double>> z(size);

    CHECK_THROWS_AS(vco_cexp(x, z, p), std::invalid_argument);
    CHECK_THROWS_AS(vco_sin(x, r, p), std::invalid_argument);
    CHECK_THROWS_AS(vco_cos(x, r, p), std::invalid_argument);
    CHECK_THROWS_AS(vco_square(x, r, p), std::invalid_argument);
}

TEST_CASE("vco: overlapping control and output throw") {
    const VcoParams p{.sensitivity = 1.0, .sample_rate = 1.0};
    std::vector<double> buf(10, 0.0);
    const std::span<double> all(buf);

    SECTION("exact aliasing") {
        CHECK_THROWS_AS(vco_sin(all, all, p), std::invalid_argument);
    }
    SECTION("partial overlap") {
        CHECK_THROWS_AS(vco_cos(all.first(6), all.last(6), p),
                        std::invalid_argument);
    }
    SECTION("complex output over real control") {
        // std::complex<double> is layout-compatible with double[2].
        std::vector<std::complex<double>> zbuf(4);
        const std::span<const double> control(
            reinterpret_cast<const double*>(zbuf.data()), zbuf.size());
        CHECK_THROWS_AS(vco_cexp(control, zbuf, p), std::invalid_argument);
    }
    SECTION("adjacent spans do not overlap") {
        CHECK_NOTHROW(vco_sin(all.first(5), all.last(5), p));
    }
}

TEST_CASE("vco: a non-finite control value makes later samples NaN") {
    const double bad = GENERATE(quiet_nan, infinity, -infinity);
    const VcoParams p{.sensitivity = 0.05, .sample_rate = 1.0,
                      .carrier = 0.1};
    auto x = wobble(8);
    x[3] = bad;

    std::vector<std::complex<double>> z(x.size());
    const double next = vco_cexp(x, z, p);
    // control[3] first affects out[4].
    for (std::size_t n = 0; n <= 3; ++n) {
        CHECK(std::isfinite(z[n].real()));
        CHECK(std::isfinite(z[n].imag()));
    }
    for (std::size_t n = 4; n < z.size(); ++n) {
        CHECK(std::isnan(z[n].real()));
        CHECK(std::isnan(z[n].imag()));
    }
    CHECK(std::isnan(next));

    std::vector<double> s(x.size());
    vco_square(x, s, p);
    CHECK(std::isfinite(s[3]));
    CHECK(std::isnan(s[4]));

    // The NaN phase cannot be passed back.
    CHECK_THROWS_AS(vco_cexp(x, z, p, next), std::invalid_argument);
}
