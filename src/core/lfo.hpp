#pragma once
/**
 * @file lfo.hpp
 * @brief Low Frequency Oscillator for modulation
 *
 * Implements the Minilogue XD's LFO:
 * - Multiple waveforms
 * - Rate control (0.1 Hz to 100 Hz)
 * - Sync to note-on
 * - Shape control
 */

#include "types.hpp"
#include <algorithm>
#include <random>

namespace synth {

/**
 * @class LFO
 * @brief Low-frequency oscillator for modulation
 */
class LFO {
public:
  enum class Shape { SINE, TRIANGLE, SAW_UP, SAW_DOWN, SQUARE, SAMPLE_HOLD };

  LFO()
      : phase_(0.0), rate_(1.0), shape_(Shape::TRIANGLE),
        phaseIncrement_(1.0 / SAMPLE_RATE), lastOutput_(0.0),
        sampleHoldValue_(0.0), rng_(std::random_device{}()),
        randomDist_(-1.0, 1.0) {}

  /**
   * @brief Set LFO rate
   * @param hz Rate in Hz (0.01 to 100)
   */
  void setRate(Frequency hz) {
    rate_ = std::clamp(hz, 0.01, 100.0);
    phaseIncrement_ = rate_ / SAMPLE_RATE;
  }

  /**
   * @brief Set rate from normalized parameter
   * @param param Normalized parameter (0.0 to 1.0)
   */
  void setRateNormalized(Parameter param) {
    Frequency hz = 0.1 * std::pow(500.0, param);
    setRate(hz);
  }

  /**
   * @brief Set LFO shape
   * @param s Shape type
   */
  void setShape(Shape s) { shape_ = s; }

  /**
   * @brief Reset phase (for sync to note-on)
   */
  void sync() { phase_ = 0.0; }

  /**
   * @brief Process one sample
   * @return LFO output (-1.0 to 1.0)
   */
  Sample process() {
    Sample output = 0.0;
    Phase prevPhase = phase_;

    phase_ += phaseIncrement_;
    if (phase_ >= 1.0)
      phase_ -= 1.0;

    switch (shape_) {
    case Shape::SINE:
      output = std::sin(TWO_PI * phase_);
      break;
    case Shape::TRIANGLE:
      output = (phase_ < 0.5) ? (4.0 * phase_ - 1.0) : (3.0 - 4.0 * phase_);
      break;
    case Shape::SAW_UP:
      output = 2.0 * phase_ - 1.0;
      break;
    case Shape::SAW_DOWN:
      output = 1.0 - 2.0 * phase_;
      break;
    case Shape::SQUARE:
      output = (phase_ < 0.5) ? 1.0 : -1.0;
      break;
    case Shape::SAMPLE_HOLD:
      if (phase_ < prevPhase)
        sampleHoldValue_ = randomDist_(rng_);
      output = sampleHoldValue_;
      break;
    }

    lastOutput_ = output;
    return output;
  }

  /**
   * @brief Get unipolar output
   * @return LFO output (0.0 to 1.0)
   */
  Sample processUnipolar() { return (process() + 1.0) * 0.5; }

private:
  Phase phase_;
  Frequency rate_;
  Shape shape_;
  Phase phaseIncrement_;
  Sample lastOutput_;
  Sample sampleHoldValue_;
  std::mt19937 rng_;
  std::uniform_real_distribution<double> randomDist_;
};

} // namespace synth
