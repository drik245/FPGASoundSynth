#pragma once
/**
 * @file delay.hpp
 * @brief Stereo delay effect with feedback
 */

#include "../core/types.hpp"
#include <algorithm>
#include <vector>

namespace synth {

/**
 * @class Delay
 * @brief Stereo delay line with feedback and mix control
 */
class Delay {
public:
  /**
   * @brief Construct delay with maximum delay time
   * @param maxDelayMs Maximum delay time in milliseconds
   */
  Delay(double maxDelayMs = 2000.0)
      : writePos_(0), delayTime_(500.0), feedback_(0.5), mix_(0.5) {
    size_t maxSamples = static_cast<size_t>(maxDelayMs * SAMPLE_RATE / 1000.0);
    bufferL_.resize(maxSamples, 0.0);
    bufferR_.resize(maxSamples, 0.0);
    updateDelaySamples();
  }

  /**
   * @brief Set delay time
   * @param ms Delay time in milliseconds
   */
  void setDelayTime(double ms) {
    delayTime_ = std::clamp(ms, 1.0, 2000.0);
    updateDelaySamples();
  }

  /**
   * @brief Set feedback amount
   * @param fb Feedback (0.0 to 0.95)
   */
  void setFeedback(Parameter fb) { feedback_ = std::clamp(fb, 0.0, 0.95); }

  /**
   * @brief Set wet/dry mix
   * @param m Mix amount (0.0 = dry, 1.0 = wet)
   */
  void setMix(Parameter m) { mix_ = std::clamp(m, 0.0, 1.0); }

  /**
   * @brief Process stereo sample
   * @param left Left channel (in/out)
   * @param right Right channel (in/out)
   */
  void process(Sample &left, Sample &right) {
    size_t readPos =
        (writePos_ + bufferL_.size() - delaySamples_) % bufferL_.size();

    Sample delayedL = bufferL_[readPos];
    Sample delayedR = bufferR_[readPos];

    bufferL_[writePos_] = left + delayedL * feedback_;
    bufferR_[writePos_] = right + delayedR * feedback_;

    left = left * (1.0 - mix_) + delayedL * mix_;
    right = right * (1.0 - mix_) + delayedR * mix_;

    writePos_ = (writePos_ + 1) % bufferL_.size();
  }

  /**
   * @brief Clear delay buffers
   */
  void clear() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0);
  }

private:
  std::vector<Sample> bufferL_, bufferR_;
  size_t writePos_;
  size_t delaySamples_;
  double delayTime_;
  Parameter feedback_;
  Parameter mix_;

  void updateDelaySamples() {
    delaySamples_ = static_cast<size_t>(delayTime_ * SAMPLE_RATE / 1000.0);
    delaySamples_ = std::min(delaySamples_, bufferL_.size() - 1);
  }
};

} // namespace synth
