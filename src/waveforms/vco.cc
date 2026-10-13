#include <chirp/waveforms/vco.h>

#include <cmath>
#include <cstddef>
#include <stdexcept>

#include "utils/math.h"
#include "utils/phase_accumulator.h"
#include "utils/spans.h"

namespace chirp::waveforms {
namespace {

using detail::two_pi;

/// Shapes behind the public vco_* functions.
enum class Shape { cexp, sine, cosine, square };

/// Throws std::invalid_argument for mismatched lengths or bad parameters.
void validate(std::size_t control_size, std::size_t out_size,
              const VcoParams& p, double initial_phase) {
    if (out_size != control_size) {
        throw std::invalid_argument(
            "vco: output length must equal control length");
    }
    if (!std::isfinite(p.sample_rate) || p.sample_rate <= 0.0) {
        throw std::invalid_argument(
            "vco: sample_rate must be finite and positive");
    }
    if (!std::isfinite(p.sensitivity)) {
        throw std::invalid_argument("vco: sensitivity must be finite");
    }
    if (!std::isfinite(p.carrier)) {
        throw std::invalid_argument("vco: carrier must be finite");
    }
    if (!std::isfinite(initial_phase)) {
        throw std::invalid_argument("vco: initial_phase must be finite");
    }
}

/// Computes shape S at phase `cycles` in double; the caller narrows it.
template <Shape S>
auto shape_sample(double cycles) {
    if constexpr (S == Shape::cexp) {
        const double r = two_pi * cycles;
        return std::complex<double>{std::cos(r), std::sin(r)};
    } else if constexpr (S == Shape::sine) {
        return std::sin(two_pi * cycles);
    } else if constexpr (S == Shape::cosine) {
        return std::cos(two_pi * cycles);
    } else {
        // Decided on cycles directly so zero crossings (0.25, 0.75) are exact.
        if (std::isnan(cycles)) {
            return cycles;
        }
        return (cycles <= 0.25 || cycles >= 0.75) ? 1.0 : -1.0;
    }
}

/**
 * The one implementation behind every public vco_* function. `In` and
 * `Out` let future precisions reuse it; phase accumulates in double.
 */
template <Shape S, class In, class Out>
double vco_impl(std::span<const In> control, std::span<Out> out,
                const VcoParams& p, double initial_phase) {
    validate(control.size(), out.size(), p, initial_phase);
    if (detail::overlaps(control, out)) {
        throw std::invalid_argument("vco: control and output must not overlap");
    }

    const double carrier_step = p.carrier / p.sample_rate;
    const double k = p.sensitivity / p.sample_rate;
    auto phase = detail::PhaseAccumulator::from_radians(initial_phase);

    for (std::size_t n = 0; n < control.size(); ++n) {
        // Emit before advancing: control[n] first affects out[n + 1].
        out[n] = static_cast<Out>(shape_sample<S>(phase.cycles()));
        phase.advance(carrier_step + k * static_cast<double>(control[n]));
    }
    return phase.radians();
}

/// Allocating form: fills a new vector through vco_impl, dropping the phase.
template <Shape S, class Out, class In>
std::vector<Out> vco_alloc(std::span<const In> control, const VcoParams& p,
                           double initial_phase) {
    std::vector<Out> out(control.size());
    vco_impl<S>(control, std::span<Out>(out), p, initial_phase);
    return out;
}

}  // namespace

double vco_cexp(std::span<const double> control,
                std::span<std::complex<double>> out, const VcoParams& params,
                double initial_phase) {
    return vco_impl<Shape::cexp>(control, out, params, initial_phase);
}

std::vector<std::complex<double>> vco_cexp(std::span<const double> control,
                                           const VcoParams& params,
                                           double initial_phase) {
    return vco_alloc<Shape::cexp, std::complex<double>>(
        control, params, initial_phase);
}

double vco_sin(std::span<const double> control, std::span<double> out,
               const VcoParams& params, double initial_phase) {
    return vco_impl<Shape::sine>(control, out, params, initial_phase);
}

std::vector<double> vco_sin(std::span<const double> control,
                            const VcoParams& params, double initial_phase) {
    return vco_alloc<Shape::sine, double>(control, params, initial_phase);
}

double vco_cos(std::span<const double> control, std::span<double> out,
               const VcoParams& params, double initial_phase) {
    return vco_impl<Shape::cosine>(control, out, params, initial_phase);
}

std::vector<double> vco_cos(std::span<const double> control,
                            const VcoParams& params, double initial_phase) {
    return vco_alloc<Shape::cosine, double>(control, params, initial_phase);
}

double vco_square(std::span<const double> control, std::span<double> out,
                  const VcoParams& params, double initial_phase) {
    return vco_impl<Shape::square>(control, out, params, initial_phase);
}

std::vector<double> vco_square(std::span<const double> control,
                               const VcoParams& params, double initial_phase) {
    return vco_alloc<Shape::square, double>(control, params, initial_phase);
}

}  // namespace chirp::waveforms
