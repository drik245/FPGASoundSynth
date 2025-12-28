#pragma once
/**
 * @file chorus.hpp
 * @brief Chorus/Flanger effect with modulated delay
 */

#include "../core/lfo.hpp"
#include "../core/types.hpp"
#include <algorithm>
#include <cmath>
#include <vector>


namespace synth {

/**
 * @class Chorus
 * @brief Stereo chorus effect with LFO-modulated delay
 */
class Chorus {
public:
  Chorus() : writePos_(0), rate_(0.5), depth_(0.5), mix_(0.5), baseDelay_(7.0) {
    size_t bufSize = static_cast<size_t>(50.0 * SAMPLE_RATE / 1000.0);
    bufferL_.resize(bufSize, 0.0);
    bufferR_.resize(bufSize, 0.0);

    lfoL_.setRate(rate_);
    lfoR_.setRate(rate_);
    lfoL_.setShape(LFO::Shape::SINE);
    lfoR_.setShape(LFO::Shape::SINE);
  }

  /**
   * @brief Set LFO rate
   * @param hz Rate in Hz (0.1 to 5.0)
   */
  void setRate(double hz) {
    rate_ = std::clamp(hz, 0.1, 5.0);
    lfoL_.setRate(rate_);
    lfoR_.setRate(rate_ * 1.1);
  }

  /**
   * @brief Set modulation depth
   * @param d Depth (0.0 to 1.0)
   */
  void setDepth(Parameter d) { depth_ = std::clamp(d, 0.0, 1.0); }

  /**
   * @brief Set wet/dry mix
   * @param m Mix (0.0 = dry, 1.0 = wet)
   */
  void setMix(Parameter m) { mix_ = std::clamp(m, 0.0, 1.0); }

  /**
   * @brief Process stereo sample
   * @param left Left channel (in/out)
   * @param right Right channel (in/out)
   */
  void process(Sample &left, Sample &right) {
    bufferL_[writePos_] = left;
    bufferR_[writePos_] = right;

    double modL = lfoL_.process() * depth_ * 3.0;
    double modR = lfoR_.process() * depth_ * 3.0;

    double delayL = baseDelay_ + modL;
    double delayR = baseDelay_ + modR;

    Sample chorusL = readInterpolated(bufferL_, delayL);
    Sample chorusR = readInterpolated(bufferR_, delayR);

    left = left * (1.0 - mix_) + chorusL * mix_;
    right = right * (1.0 - mix_) + chorusR * mix_;

    writePos_ = (writePos_ + 1) % bufferL_.size();
  }

private:
  std::vector<Sample> bufferL_, bufferR_;
  size_t writePos_;
  LFO lfoL_, lfoR_;
  double rate_;
  Parameter depth_;
  Parameter mix_;
  double baseDelay_;

  Sample readInterpolated(const std::vector<Sample> &buffer, double delayMs) {
    double delaySamples = delayMs * SAMPLE_RATE / 1000.0;
    double readPosF = static_cast<double>(writePos_) - delaySamples;
    if (readPosF < 0)
      readPosF += buffer.size();

    size_t idx0 = static_cast<size_t>(readPosF) % buffer.size();
    size_t idx1 = (idx0 + 1) % buffer.size();
    double frac = readPosF - std::floor(readPosF);

    return buffer[idx0] * (1.0 - frac) + buffer[idx1] * frac;
  }
};

} // namespace synth
