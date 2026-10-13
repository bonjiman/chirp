#include "utils/phase_accumulator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <limits>
#include <numbers>

#include "utils/math.h"

using chirp::detail::PhaseAccumulator;
using chirp::detail::two_pi;

namespace {

/// An accumulator at `cycles`, which must lie in [0, 1).
PhaseAccumulator at_cycles(double cycles) {
    PhaseAccumulator phase;
    phase.advance(cycles);
    return phase;
}

}  // namespace

TEST_CASE("PhaseAccumulator starts at zero") {
    const PhaseAccumulator phase;
    CHECK(phase.cycles() == 0.0);
    CHECK(phase.radians() == 0.0);
}

TEST_CASE("PhaseAccumulator::from_radians wraps to [0, 2pi)") {
    constexpr double pi = std::numbers::pi;
    CHECK(PhaseAccumulator::from_radians(pi / 2).cycles() == 0.25);
    CHECK(PhaseAccumulator::from_radians(-pi / 2).cycles() == 0.75);
    CHECK(PhaseAccumulator::from_radians(two_pi).cycles() == 0.0);
    CHECK(PhaseAccumulator::from_radians(-two_pi).cycles() == 0.0);
    CHECK(PhaseAccumulator::from_radians(-pi).cycles() == 0.5);
    CHECK(PhaseAccumulator::from_radians(8 * pi).cycles() == 0.0);
}

TEST_CASE("PhaseAccumulator wraps exactly at one cycle") {
    PhaseAccumulator phase;
    for (int i = 0; i < 4; ++i) {
        phase.advance(0.25);
    }
    CHECK(phase.cycles() == 0.0);
}

TEST_CASE("PhaseAccumulator accepts negative increments") {
    PhaseAccumulator phase;
    phase.advance(-0.25);
    CHECK(phase.cycles() == 0.75);
    phase.advance(-0.5);
    CHECK(phase.cycles() == 0.25);
}

TEST_CASE("PhaseAccumulator accepts increments over one cycle") {
    PhaseAccumulator phase;
    phase.advance(2.5);
    CHECK(phase.cycles() == 0.5);
    phase.advance(-3.25);
    CHECK(phase.cycles() == 0.25);
}

TEST_CASE("PhaseAccumulator wraps a tiny negative phase to zero") {
    // -1e-20 - floor(-1e-20) rounds to 1.0, which must become 0.
    PhaseAccumulator phase;
    phase.advance(-1e-20);
    CHECK(phase.cycles() == 0.0);
    CHECK(phase.radians() == 0.0);
}

TEST_CASE("PhaseAccumulator::radians stays below 2pi") {
    const double below_one = std::nextafter(1.0, 0.0);
    const PhaseAccumulator phase = at_cycles(below_one);
    CHECK(phase.cycles() == below_one);
    CHECK(phase.radians() >= 0.0);
    CHECK(phase.radians() < two_pi);
}

TEST_CASE("PhaseAccumulator propagates NaN and turns infinity into NaN") {
    const double step = GENERATE(std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::infinity(),
                                 -std::numeric_limits<double>::infinity());
    PhaseAccumulator phase = at_cycles(0.5);
    phase.advance(step);
    CHECK(std::isnan(phase.cycles()));
    CHECK(std::isnan(phase.radians()));

    // A later finite step does not recover.
    phase.advance(0.25);
    CHECK(std::isnan(phase.cycles()));
}

TEST_CASE("PhaseAccumulator::from_radians gives NaN for non-finite input") {
    const double radians = GENERATE(std::numeric_limits<double>::quiet_NaN(),
                                    std::numeric_limits<double>::infinity());
    CHECK(std::isnan(PhaseAccumulator::from_radians(radians).cycles()));
}
