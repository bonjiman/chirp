#pragma once

#include <complex>
#include <span>
#include <vector>

namespace chirp::waveforms {

/// Scalar parameters for the vco_* family. Use designated initializers.
struct VcoParams {
    double sensitivity;    ///< Hz per unit of control signal. Finite; any sign.
    double sample_rate;    ///< Hz. Finite and > 0. Use 1.0 for cycles/sample.
    double carrier = 0.0;  ///< Hz. Finite; any sign; aliases above Nyquist.
};

// Every vco_* function shares this definition:
//
//     f[n] = carrier + sensitivity * control[n]              (Hz)
//     φ[n] = initial_phase + 2π Σ_{m<n} f[m] / sample_rate   (radians)
//
// control[n] first affects out[n + 1], a deliberate one-sample difference
// from inclusive-cumsum FM. Span forms write every element of `out` and
// return φ[N], the phase of the sample after the last output; pass it as
// the next call's initial_phase to continue the signal. Allocating forms
// return the samples only.
//
// A NaN or infinite control value makes every later sample, and the
// returned phase, NaN. Throws std::invalid_argument for mismatched lengths,
// overlapping spans, or invalid parameters or initial_phase.

/**
 * Complex exponential VCO: out[n] = exp(jφ[n]).
 * @param control Control signal, one value per output sample.
 * @param out Output. Same length as `control`; must not overlap it.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return φ[N] in radians, in [0, 2π).
 */
double vco_cexp(std::span<const double> control,
                std::span<std::complex<double>> out, const VcoParams& params,
                double initial_phase = 0.0);

/**
 * Complex exponential VCO, allocating form.
 * @param control Control signal, one value per output sample.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return exp(jφ[n]) for each control sample.
 */
std::vector<std::complex<double>> vco_cexp(std::span<const double> control,
                                           const VcoParams& params,
                                           double initial_phase = 0.0);

/**
 * Sine VCO: out[n] = sin(φ[n]).
 * @param control Control signal, one value per output sample.
 * @param out Output. Same length as `control`; must not overlap it.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return φ[N] in radians, in [0, 2π).
 */
double vco_sin(std::span<const double> control, std::span<double> out,
               const VcoParams& params, double initial_phase = 0.0);

/**
 * Sine VCO, allocating form.
 * @param control Control signal, one value per output sample.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return sin(φ[n]) for each control sample.
 */
std::vector<double> vco_sin(std::span<const double> control,
                            const VcoParams& params,
                            double initial_phase = 0.0);

/**
 * Cosine VCO: out[n] = cos(φ[n]).
 * @param control Control signal, one value per output sample.
 * @param out Output. Same length as `control`; must not overlap it.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return φ[N] in radians, in [0, 2π).
 */
double vco_cos(std::span<const double> control, std::span<double> out,
               const VcoParams& params, double initial_phase = 0.0);

/**
 * Cosine VCO, allocating form.
 * @param control Control signal, one value per output sample.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return cos(φ[n]) for each control sample.
 */
std::vector<double> vco_cos(std::span<const double> control,
                            const VcoParams& params,
                            double initial_phase = 0.0);

/**
 * Square-wave VCO: out[n] = sign(cos φ[n]), with +1 at zero crossings.
 * A naive sampled square wave: harmonics above Nyquist alias.
 * @param control Control signal, one value per output sample.
 * @param out Output. Same length as `control`; must not overlap it.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return φ[N] in radians, in [0, 2π).
 */
double vco_square(std::span<const double> control, std::span<double> out,
                  const VcoParams& params, double initial_phase = 0.0);

/**
 * Square-wave VCO, allocating form.
 * @param control Control signal, one value per output sample.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return sign(cos φ[n]) for each control sample, ±1.
 */
std::vector<double> vco_square(std::span<const double> control,
                               const VcoParams& params,
                               double initial_phase = 0.0);

/**
 * Alias for vco_cexp().  VCO complex exponential
 * @param control Control signal, one value per output sample.
 * @param out Output. Same length as `control`; must not overlap it.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return φ[N] in radians, in [0, 2π).
 */
inline double vco(std::span<const double> control,
                  std::span<std::complex<double>> out, const VcoParams& params,
                  double initial_phase = 0.0) {
    return vco_cexp(control, out, params, initial_phase);
}

/**
 * Alias for vco_cexp(), allocating form.  VCO complex exponential
 * @param control Control signal, one value per output sample.
 * @param params Sensitivity, sample rate and carrier.
 * @param initial_phase φ[0] in radians. Must be finite.
 * @return exp(jφ[n]) for each control sample.
 */
inline std::vector<std::complex<double>> vco(std::span<const double> control,
                                             const VcoParams& params,
                                             double initial_phase = 0.0) {
    return vco_cexp(control, params, initial_phase);
}
}  // namespace chirp::waveforms
