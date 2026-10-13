#pragma once

#include <cmath>

#include "utils/math.h"

namespace chirp::detail {

/**
 * A phase that advances in cycles and stays wrapped to [0, 1).
 *
 * Wrapping in cycles is exact, since 2π is not representable in double.
 * Keeping the value below 1 holds its precision near 1e-16 cycles however
 * long it runs; a phase left to grow loses precision as its integer part
 * takes up significant digits.
 *
 * Defined in the header because advance() runs once per sample and must
 * inline.
 */
class PhaseAccumulator {
public:
    /// Constructs an accumulator at zero phase.
    constexpr PhaseAccumulator() = default;

    /**
     * Constructs an accumulator at a phase given in radians.
     * @param radians Initial phase. Callers check that it is finite; a
     *     non-finite value gives a NaN phase.
     * @return An accumulator at `radians`, wrapped to [0, 2π).
     */
    static PhaseAccumulator from_radians(double radians) {
        PhaseAccumulator phase;
        phase.cycles_ = wrap(radians / two_pi);
        return phase;
    }

    /**
     * Advances the phase by one step and wraps it to [0, 1).
     * @param increment_cycles Step in cycles, f / fs for one sample. NaN
     *     propagates; an infinite step gives NaN.
     */
    void advance(double increment_cycles) {
        const double next = cycles_ + increment_cycles;
        // For next in [0, 1), wrap(next) == next exactly, so skipping the
        // wrap there changes nothing.
        cycles_ = (next >= 0.0 && next < 1.0) ? next : wrap(next);
    }

    /**
     * Current phase in cycles.
     * @return Phase in [0, 1), or NaN after a NaN or infinite step.
     */
    double cycles() const { return cycles_; }

    /**
     * Current phase in radians.
     * @return Phase in [0, 2π), or NaN after a NaN or infinite step.
     */
    double radians() const {
        const double r = two_pi * cycles_;
        return r >= two_pi ? 0.0 : r;  // cycles near 1 can round up to 2π
    }

private:
    /// Wraps a phase in cycles to [0, 1). NaN propagates.
    static double wrap(double cycles) {
        const double w = cycles - std::floor(cycles);
        return w == 1.0 ? 0.0 : w;  // tiny negatives round up to 1.0
    }

    /// Phase in cycles: [0, 1), or NaN.
    double cycles_ = 0.0;
};

}  // namespace chirp::detail
