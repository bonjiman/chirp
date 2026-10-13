# vco — voltage-controlled oscillator

A VCO turns a control signal into an oscillator whose instantaneous
frequency follows that signal. Typical uses are frequency modulation at
complex baseband, swept tones, and modeling frequency noise.

## Signal definition

Instantaneous frequency, in hertz:

    f[n] = fc + sensitivity * x[n]

Phase uses exclusive rectangular integration:

    φ[n] = φ₀ + 2π Σ_{m<n} f[m] / fs

- `φ₀` (`initial_phase`) is the phase of the first output sample, in
  radians. With a constant input, `cexp` output is
  `exp(j(2π f n / fs + φ₀))` up to floating-point rounding.
- Input sample `x[n]` first affects output `n + 1`. This is a deliberate
  one-sample difference from the inclusive-cumsum form used by MATLAB
  `fmmod` and most textbook FM code.

## C++ API

Each output shape has its own function, in `chirp::waveforms`
(`<chirp/waveforms/vco.h>`):

| Function     | Output                   | Sample type            |
|--------------|--------------------------|------------------------|
| `vco_cexp`   | `exp(jφ[n])`             | `std::complex<double>` |
| `vco_sin`    | `sin(φ[n])`              | `double`               |
| `vco_cos`    | `cos(φ[n])`              | `double`               |
| `vco_square` | `sign(cos φ[n])`         | `double`               |
| `vco`        | alias for `vco_cexp`     | `std::complex<double>` |

Every function comes in two forms:

- **Span form:** writes into a caller-owned output span and allocates
  nothing. It returns φ[N], the phase of the sample *following* the last
  output, wrapped to [0, 2π).
- **Allocating form:** returns a new `std::vector` holding only the
  samples. It produces exactly the same samples as the span form.

```cpp
using namespace chirp::waveforms;
VcoParams p{.sensitivity = 50.0, .sample_rate = 48e3, .carrier = 1e3};

std::vector<std::complex<double>> z = vco_cexp(x, p);  // or vco(x, p)
std::vector<double> s = vco_square(x, p);

std::vector<double> out(x.size());
double next_phase = vco_sin(x, out, p, /*initial_phase=*/0.5);
```

### Parameters

Scalar parameters are grouped in `VcoParams`, meant for C++20 designated
initializers:

| Field         | Unit       | Constraint                                |
|---------------|------------|-------------------------------------------|
| `sensitivity` | Hz / unit  | finite; any sign                          |
| `sample_rate` | Hz         | finite and > 0; 1.0 for cycles per sample |
| `carrier`     | Hz         | finite; any sign; default 0; may alias    |

`initial_phase` is in radians, must be finite, and defaults to 0. Any
finite value is accepted and wrapped.

Inputs and outputs are contiguous 1-D spans. The output must have the
same length as the control signal and must not overlap it.

### Errors

`std::invalid_argument` is thrown for:

- an output whose length differs from the control signal's
- an output that overlaps the control signal
- a non-finite or non-positive sample rate
- a non-finite sensitivity or carrier
- a non-finite `initial_phase`

## Waveforms

- `cos` and `sin` give the real and imaginary parts of `cexp`:
  `cos(φ[n]) = Re{exp(jφ[n])}` and `sin(φ[n]) = Im{exp(jφ[n])}`.
- The square wave is `sign(cos φ)`: amplitude ±1, and +1 at zero
  crossings. Its edges are decided on the phase in cycles (0.25 and 0.75),
  so they fall exactly where intended. It is a naive sampled square wave,
  so harmonics above Nyquist alias.

## Generating a signal in pieces

The functions are stateless. To generate a long signal in pieces, pass the
phase returned by one span-form call as the next call's `initial_phase`:

```cpp
double phase = 0.0;
for (auto [x_block, out_block] : blocks) {
    phase = vco_cexp(x_block, out_block, p, phase);
}
```

The allocating form returns only the samples, so continuing a signal needs
the span form.

## Numerical behavior

Phase is accumulated internally in cycles, wrapped to [0, 1). Wrapping in
cycles is exact, and keeping the value small keeps its precision near
1e-16 cycles however long the signal runs; a phase left to grow would
lose precision as its integer part used up significant digits.

Converting to radians at the boundary can round by about 1e-16 rad, so a
signal generated in pieces matches a single call within a small
tolerance, not bit-for-bit.

`cexp`, `sin` and `cos` are not exactly zero at quarter cycles (for
example, cos(2π · 0.25) ≈ 6e-17), because 2π is not exactly representable.
The square wave has no such error.

A NaN or infinite control value makes every later output sample NaN, and
the returned phase NaN as well. Passing a NaN phase back as
`initial_phase` throws `std::invalid_argument`.
