#pragma once
/**
 * @file filter.hpp
 * @brief State Variable Filter (SVF) with resonance and drive
 *
 * Implements the Minilogue XD's 2-pole 12dB/oct filter:
 * - Low-pass, high-pass, band-pass modes
 * - Resonance (Q) control
 * - Filter drive/saturation
 * - Cutoff frequency modulation
 *
 * Uses the Chamberlin SVF topology, well-suited for FPGA.
 */

#include "types.hpp"
#include <algorithm>

namespace synth {

/**
 * @class StateVariableFilter
 * @brief 2-pole resonant filter with multiple outputs
 *
 * The SVF provides simultaneous LP, HP, BP, and Notch outputs.
 * This topology is numerically stable and maps well to fixed-point.
 */
class StateVariableFilter {
public:
  StateVariableFilter()
      : cutoff_(1000.0), resonance_(0.0), drive_(0.0),
        mode_(FilterMode::LOWPASS), lowpass_(0.0), highpass_(0.0),
        bandpass_(0.0), notch_(0.0) {
    updateCoefficients();
  }

  /**
   * @brief Set cutoff frequency
   * @param freq Cutoff frequency in Hz (20 - 20000)
   */
  void setCutoff(Frequency freq) {
    cutoff_ = std::clamp(freq, 20.0, NYQUIST * 0.9);
    updateCoefficients();
  }

  /**
   * @brief Set resonance (Q)
   * @param res Resonance amount (0.0 = none, 1.0 = self-oscillation)
   */
  void setResonance(Parameter res) {
    resonance_ = std::clamp(res, 0.0, 0.99);
    updateCoefficients();
  }

  /**
   * @brief Set filter drive (saturation)
   * @param drv Drive amount (0.0 = clean, 1.0 = heavy saturation)
   */
  void setDrive(Parameter drv) { drive_ = std::clamp(drv, 0.0, 1.0); }

  /**
   * @brief Set filter mode
   * @param m Filter mode (LP, HP, BP, Notch)
   */
  void setMode(FilterMode m) { mode_ = m; }

  /**
   * @brief Process one sample
   * @param input Input sample
   * @return Filtered output sample
   */
  Sample process(Sample input) {
    if (drive_ > 0.0) {
      input = softClip(input * (1.0 + drive_ * 3.0));
    }

    for (int i = 0; i < 2; ++i) {
      lowpass_ += f_ * bandpass_;
      highpass_ = input - lowpass_ - q_ * bandpass_;
      bandpass_ += f_ * highpass_;
      notch_ = highpass_ + lowpass_;
    }

    Sample output;
    switch (mode_) {
    case FilterMode::LOWPASS:
      output = lowpass_;
      break;
    case FilterMode::HIGHPASS:
      output = highpass_;
      break;
    case FilterMode::BANDPASS:
      output = bandpass_;
      break;
    case FilterMode::NOTCH:
      output = notch_;
      break;
    default:
      output = lowpass_;
    }

    if (drive_ > 0.5) {
      output = softClip(output);
    }

    return output;
  }

  /**
   * @brief Get all filter outputs simultaneously
   * @param input Input sample
   * @param lp Low-pass output
   * @param hp High-pass output
   * @param bp Band-pass output
   * @param notch Notch output
   */
  void processMultiMode(Sample input, Sample &lp, Sample &hp, Sample &bp,
                        Sample &notch) {
    if (drive_ > 0.0) {
      input = softClip(input * (1.0 + drive_ * 3.0));
    }

    for (int i = 0; i < 2; ++i) {
      lowpass_ += f_ * bandpass_;
      highpass_ = input - lowpass_ - q_ * bandpass_;
      bandpass_ += f_ * highpass_;
      notch_ = highpass_ + lowpass_;
    }

    lp = lowpass_;
    hp = highpass_;
    bp = bandpass_;
    notch = notch_;
  }

  /**
   * @brief Reset filter state (on note-on to prevent clicks)
   */
  void reset() {
    lowpass_ = 0.0;
    highpass_ = 0.0;
    bandpass_ = 0.0;
    notch_ = 0.0;
  }

private:
  Frequency cutoff_;
  Parameter resonance_;
  Parameter drive_;
  FilterMode mode_;

  Sample lowpass_;
  Sample highpass_;
  Sample bandpass_;
  Sample notch_;

  Sample f_;
  Sample q_;

  /**
   * @brief Update filter coefficients when parameters change
   */
  void updateCoefficients() {
    f_ = 2.0 * std::sin(PI * cutoff_ / SAMPLE_RATE);
    q_ = 2.0 - 2.0 * resonance_;
  }

  /**
   * @brief Soft clipping saturation (tanh approximation)
   * @param x Input value
   * @return Clipped value
   */
  static Sample softClip(Sample x) {
    if (x > 3.0)
      return 1.0;
    if (x < -3.0)
      return -1.0;
    Sample x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
  }
};

/**
 * @class LadderFilter
 * @brief 4-pole 24dB/oct Moog-style ladder filter
 *
 * More computationally expensive but provides that classic ladder sound.
 * Good for FPGA since it's just a cascade of 1-pole filters.
 */
class LadderFilter {
public:
  LadderFilter() : cutoff_(1000.0), resonance_(0.0) {
    reset();
    updateCoefficients();
  }

  /**
   * @brief Set cutoff frequency
   * @param freq Cutoff frequency in Hz
   */
  void setCutoff(Frequency freq) {
    cutoff_ = std::clamp(freq, 20.0, NYQUIST * 0.45);
    updateCoefficients();
  }

  /**
   * @brief Set resonance
   * @param res Resonance amount (0.0 to 1.0)
   */
  void setResonance(Parameter res) {
    resonance_ = std::clamp(res, 0.0, 1.0);
    k_ = 4.0 * resonance_;
  }

  /**
   * @brief Process one sample
   * @param input Input sample
   * @return Filtered output
   */
  Sample process(Sample input) {
    Sample feedback = stage_[3] * k_;
    input = softClip(input - feedback);

    for (int i = 0; i < 4; ++i) {
      Sample prev = (i == 0) ? input : stage_[i - 1];
      stage_[i] += g_ * (softClip(prev) - stage_[i]);
    }

    return stage_[3];
  }

  /**
   * @brief Reset filter state
   */
  void reset() {
    for (int i = 0; i < 4; ++i) {
      stage_[i] = 0.0;
    }
  }

private:
  Frequency cutoff_;
  Parameter resonance_;
  Sample stage_[4];
  Sample g_;
  Sample k_;

  void updateCoefficients() {
    Sample wc = 2.0 * std::tan(PI * cutoff_ / SAMPLE_RATE);
    g_ = wc / (1.0 + wc);
  }

  static Sample softClip(Sample x) { return std::tanh(x); }
};

} // namespace synth
