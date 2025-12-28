#pragma once
/**
 * @file types.hpp
 * @brief Common types and constants for the synthesizer
 *
 * These types are designed to be easily portable to fixed-point
 * for FPGA synthesis via HDL Coder.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>


// Compatibility for older C++ (pre C++17)
#if __cplusplus < 201703L
namespace std {
template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
  return (v < lo) ? lo : (hi < v) ? hi : v;
}
} // namespace std
#endif

namespace synth {

// =============================================================================
// Configuration Constants
// =============================================================================

constexpr double SAMPLE_RATE = 192000.0; // 192 kHz
constexpr double NYQUIST = SAMPLE_RATE / 2.0;
constexpr double SAMPLE_PERIOD = 1.0 / SAMPLE_RATE;

constexpr int NUM_VOICES = 4;
constexpr int OVERSAMPLING = 1; // Can increase for anti-aliasing

// =============================================================================
// Type Aliases (Easy to swap for fixed-point later)
// =============================================================================

// For PC testing: use double precision
// For FPGA: replace with fixed-point types
using Sample = double;    // Audio sample (-1.0 to 1.0)
using Phase = double;     // Phase accumulator (0.0 to 1.0)
using Frequency = double; // Frequency in Hz
using Parameter = double; // Control parameter (0.0 to 1.0)

// Fixed-point compatible integer types
using SampleInt = int32_t; // 24-bit audio in 32-bit container
using PhaseAcc = uint32_t; // 32-bit phase accumulator

// =============================================================================
// Mathematical Constants
// =============================================================================

constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

// =============================================================================
// MIDI Utilities
// =============================================================================

/**
 * @brief Convert MIDI note number to frequency
 * @param note MIDI note (0-127, 69 = A4 = 440Hz)
 * @return Frequency in Hz
 */
inline Frequency midiToFrequency(int note) {
  return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

/**
 * @brief Convert frequency to phase increment
 * @param freq Frequency in Hz
 * @return Phase increment per sample (0.0 to 1.0 range)
 */
inline Phase frequencyToPhaseIncrement(Frequency freq) {
  return freq / SAMPLE_RATE;
}

// =============================================================================
// Fixed-Point Conversion Helpers (for FPGA porting)
// =============================================================================

constexpr int FRAC_BITS = 24; // Q8.24 format for most calculations
constexpr int32_t FRAC_SCALE = 1 << FRAC_BITS;

inline int32_t toFixed(double value) {
  return static_cast<int32_t>(value * FRAC_SCALE);
}

inline double fromFixed(int32_t value) {
  return static_cast<double>(value) / FRAC_SCALE;
}

// =============================================================================
// Waveform Types
// =============================================================================

enum class Waveform { SINE, TRIANGLE, SAW, SQUARE, NOISE };

// =============================================================================
// Filter Types
// =============================================================================

enum class FilterMode { LOWPASS, HIGHPASS, BANDPASS, NOTCH };

} // namespace synth
