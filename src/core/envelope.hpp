#pragma once
/**
 * @file envelope.hpp
 * @brief ADSR Envelope Generator
 *
 * Implements the Minilogue XD's envelope generators:
 * - EG1: Controls filter cutoff
 * - EG2: Controls VCA (amplitude)
 *
 * Features:
 * - Exponential curves for natural sound
 * - Re-trigger support
 * - Legato mode
 */

#include "types.hpp"
#include <algorithm>
#include <vector>

namespace synth {

/**
 * @class ADSR
 * @brief Classic ADSR envelope generator with exponential curves
 */
class ADSR {
public:
  enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

  ADSR()
      : stage_(Stage::IDLE), output_(0.0), attackTime_(0.01), decayTime_(0.1),
        sustainLevel_(0.7), releaseTime_(0.3) {
    updateCoefficients();
  }

  /**
   * @brief Set attack time
   * @param time Attack time in seconds (0.001 to 10.0)
   */
  void setAttack(double time) {
    attackTime_ = std::clamp(time, 0.001, 10.0);
    updateCoefficients();
  }

  /**
   * @brief Set decay time
   * @param time Decay time in seconds (0.001 to 10.0)
   */
  void setDecay(double time) {
    decayTime_ = std::clamp(time, 0.001, 10.0);
    updateCoefficients();
  }

  /**
   * @brief Set sustain level
   * @param level Sustain level (0.0 to 1.0)
   */
  void setSustain(Parameter level) {
    sustainLevel_ = std::clamp(level, 0.0, 1.0);
  }

  /**
   * @brief Set release time
   * @param time Release time in seconds (0.001 to 10.0)
   */
  void setRelease(double time) {
    releaseTime_ = std::clamp(time, 0.001, 10.0);
    updateCoefficients();
  }

  /**
   * @brief Trigger the envelope (note on)
   */
  void noteOn() { stage_ = Stage::ATTACK; }

  /**
   * @brief Release the envelope (note off)
   */
  void noteOff() {
    if (stage_ != Stage::IDLE) {
      stage_ = Stage::RELEASE;
      releaseLevel_ = output_;
    }
  }

  /**
   * @brief Hard reset envelope to idle
   */
  void reset() {
    stage_ = Stage::IDLE;
    output_ = 0.0;
  }

  /**
   * @brief Process one sample
   * @return Envelope output (0.0 to 1.0)
   */
  Sample process() {
    switch (stage_) {
    case Stage::IDLE:
      output_ = 0.0;
      break;
    case Stage::ATTACK:
      output_ += attackCoef_ * (1.3 - output_);
      if (output_ >= 1.0) {
        output_ = 1.0;
        stage_ = Stage::DECAY;
      }
      break;
    case Stage::DECAY:
      output_ += decayCoef_ * (sustainLevel_ - output_);
      if (output_ <= sustainLevel_ + 0.001) {
        output_ = sustainLevel_;
        stage_ = Stage::SUSTAIN;
      }
      break;
    case Stage::SUSTAIN:
      output_ = sustainLevel_;
      break;
    case Stage::RELEASE:
      output_ += releaseCoef_ * (0.0 - output_);
      if (output_ <= 0.001) {
        output_ = 0.0;
        stage_ = Stage::IDLE;
      }
      break;
    }
    return output_;
  }

  /**
   * @brief Check if envelope is active
   * @return true if envelope is not idle
   */
  bool isActive() const { return stage_ != Stage::IDLE; }

  /**
   * @brief Get current stage
   * @return Current envelope stage
   */
  Stage getStage() const { return stage_; }

  /**
   * @brief Get current output without processing
   * @return Current envelope value
   */
  Sample getOutput() const { return output_; }

private:
  Stage stage_;
  Sample output_;
  Sample releaseLevel_;

  double attackTime_;
  double decayTime_;
  Parameter sustainLevel_;
  double releaseTime_;

  Sample attackCoef_;
  Sample decayCoef_;
  Sample releaseCoef_;

  /**
   * @brief Calculate exponential coefficients from times
   */
  void updateCoefficients() {
    double samplesAttack = attackTime_ * SAMPLE_RATE;
    double samplesDecay = decayTime_ * SAMPLE_RATE;
    double samplesRelease = releaseTime_ * SAMPLE_RATE;

    attackCoef_ = 1.0 - std::exp(-2.2 / samplesAttack);
    decayCoef_ = 1.0 - std::exp(-2.2 / samplesDecay);
    releaseCoef_ = 1.0 - std::exp(-2.2 / samplesRelease);
  }
};

} // namespace synth
