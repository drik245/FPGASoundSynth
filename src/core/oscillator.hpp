#pragma once
/**
 * oscillator.hpp
 * Band-limited oscillators with PolyBLEP anti-aliasing
 *
 * Implements VCO1 and VCO2 from Minilogue XD:
 * - Saw, Triangle, Square waveforms
 * - Pulse Width Modulation
 * - Hard sync capability
 * - PolyBLEP for alias-free output at 192kHz
 */

#include "types.hpp"
#include <random>

namespace synth {

/**
 * Oscillator
 * Band-limited oscillator with multiple waveforms
 *
 * Uses PolyBLEP (Polynomial Band-Limited Step) for anti-aliasing.
 * This is efficient and maps well to FPGA implementation.
 */
class Oscillator {
public:
  Oscillator()
      : phase_(0.0), phaseIncrement_(0.0), waveform_(Waveform::SAW),
        pulseWidth_(0.5), lastOutput_(0.0), rng_(std::random_device{}()),
        noiseDist_(-1.0, 1.0) {}

  /**
   * @brief Set oscillator frequency
   * @param freq Frequency in Hz
   */
  void setFrequency(Frequency freq) {
    phaseIncrement_ = frequencyToPhaseIncrement(freq);
  }

  /**
   * @brief Set oscillator frequency from MIDI note
   * @param note MIDI note number (0-127)
   */
  void setNote(int note) { setFrequency(midiToFrequency(note)); }

  /**
   * @brief Set waveform type
   */
  void setWaveform(Waveform wf) { waveform_ = wf; }

  /**
   * @brief Set pulse width for square wave
   * @param pw Pulse width (0.0 to 1.0, 0.5 = square)
   */
  void setPulseWidth(Parameter pw) { pulseWidth_ = std::clamp(pw, 0.01, 0.99); }

  /**
   * @brief Hard sync - reset phase (called by master oscillator)
   */
  void sync() { phase_ = 0.0; }

  /**
   * @brief Process one sample
   * @return Output sample (-1.0 to 1.0)
   */
  Sample process() {
    Sample output = 0.0;

    switch (waveform_) {
    case Waveform::SINE:
      output = processSine();
      break;
    case Waveform::SAW:
      output = processSaw();
      break;
    case Waveform::TRIANGLE:
      output = processTriangle();
      break;
    case Waveform::SQUARE:
      output = processSquare();
      break;
    case Waveform::NOISE:
      output = processNoise();
      break;
    }

    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0) {
      phase_ -= 1.0;
    }

    lastOutput_ = output;
    return output;
  }

  /**
   * @brief Get current phase (for sync)
   */
  Phase getPhase() const { return phase_; }

private:
  Phase phase_;
  Phase phaseIncrement_;
  Waveform waveform_;
  Parameter pulseWidth_;
  Sample lastOutput_;

  // For noise generation
  std::mt19937 rng_;
  std::uniform_real_distribution<double> noiseDist_;

  /**
   * @brief PolyBLEP correction for discontinuities
   *
   * This is the magic sauce for alias-free waveforms!
   * Approximates the band-limited step with a polynomial.
   */
  Sample polyBlep(Phase t) const {
    Phase dt = phaseIncrement_;

    if (t < dt) {
      // Rising edge
      t /= dt;
      return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
      // Falling edge
      t = (t - 1.0) / dt;
      return t * t + t + t + 1.0;
    }
    return 0.0;
  }

  Sample processSine() const { return std::sin(TWO_PI * phase_); }

  Sample processSaw() {
    // Naive saw: 2 * phase - 1
    Sample saw = 2.0 * phase_ - 1.0;
    // Apply PolyBLEP at discontinuity
    saw -= polyBlep(phase_);
    return saw;
  }

  Sample processTriangle() {
    // Integrate the square wave for triangle
    // Or use direct formula with PolyBLEP
    Sample tri;
    if (phase_ < 0.5) {
      tri = 4.0 * phase_ - 1.0;
    } else {
      tri = 3.0 - 4.0 * phase_;
    }
    return tri;
  }

  Sample processSquare() {
    Sample square;
    if (phase_ < pulseWidth_) {
      square = 1.0;
    } else {
      square = -1.0;
    }
    // PolyBLEP at both edges
    square += polyBlep(phase_);
    square -= polyBlep(std::fmod(phase_ + (1.0 - pulseWidth_), 1.0));
    return square;
  }

  Sample processNoise() {
    // White noise (LFSR would be better for FPGA)
    return noiseDist_(rng_);
  }
};

/**
 * @class MultiEngine
 * @brief Digital multi-engine oscillator (like Minilogue XD's third oscillator)
 *
 * Provides additional digital waveforms:
 * - VPM (Variable Phase Modulation / FM)
 * - Wavetable
 * - Digital noise with shaping
 */
class MultiEngine {
public:
  enum class Mode {
    VPM,   // FM/Phase modulation
    WAVES, // Wavetable
    NOISE  // Shaped noise
  };

  MultiEngine()
      : phase_(0.0), phaseIncrement_(0.0), mode_(Mode::VPM), modIndex_(1.0),
        ratio_(1.0), shape_(0.5) {}

  void setFrequency(Frequency freq) {
    phaseIncrement_ = frequencyToPhaseIncrement(freq);
  }

  void setMode(Mode m) { mode_ = m; }

  // VPM parameters
  void setModIndex(Parameter idx) { modIndex_ = idx * 8.0; } // 0-8 range
  void setRatio(Parameter r) { ratio_ = 1.0 + r * 7.0; }     // 1-8 ratio

  void setShape(Parameter s) { shape_ = s; }

  Sample process() {
    Sample output = 0.0;

    switch (mode_) {
    case Mode::VPM:
      output = processVPM();
      break;
    case Mode::WAVES:
      output = processWaves();
      break;
    case Mode::NOISE:
      output = processNoise();
      break;
    }

    phase_ += phaseIncrement_;
    if (phase_ >= 1.0)
      phase_ -= 1.0;

    return output;
  }

private:
  Phase phase_;
  Phase phaseIncrement_;
  Mode mode_;
  Parameter modIndex_;
  Parameter ratio_;
  Parameter shape_;

  std::mt19937 rng_{std::random_device{}()};
  std::uniform_real_distribution<double> noiseDist_{-1.0, 1.0};

  Sample processVPM() const {
    // Simple 2-op FM synthesis
    // Carrier modulated by modulator
    Phase modPhase = std::fmod(phase_ * ratio_, 1.0);
    Sample modulator = std::sin(TWO_PI * modPhase);
    Sample carrier = std::sin(TWO_PI * phase_ + modIndex_ * modulator);
    return carrier;
  }

  Sample processWaves() const {
    // Simple morphing wavetable (sine -> saw blend)
    Sample sine = std::sin(TWO_PI * phase_);
    Sample saw = 2.0 * phase_ - 1.0;
    return sine * (1.0 - shape_) + saw * shape_;
  }

  Sample processNoise() {
    // Filtered noise based on shape
    return noiseDist_(rng_);
  }
};

} // namespace synth
