#pragma once
/**
 * @file reverb.hpp
 * @brief Schroeder reverb with allpass and comb filters
 */

#include "../core/types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace synth {

/**
 * @class Reverb
 * @brief Schroeder reverb algorithm (4 comb + 2 allpass filters)
 */
class Reverb {
public:
  Reverb() : mix_(0.3), decay_(0.5) {
    const std::array<size_t, 4> combDelays = {2999, 3407, 3701, 4003};
    for (size_t i = 0; i < 4; ++i) {
      combBuffers_[i].resize(combDelays[i] * 4, 0.0);
      combPos_[i] = 0;
    }

    const std::array<size_t, 2> apDelays = {521, 337};
    for (size_t i = 0; i < 2; ++i) {
      apBuffers_[i].resize(apDelays[i] * 4, 0.0);
      apPos_[i] = 0;
    }

    updateDecay();
  }

  /**
   * @brief Set wet/dry mix
   * @param m Mix (0.0 = dry, 1.0 = wet)
   */
  void setMix(Parameter m) { mix_ = std::clamp(m, 0.0, 1.0); }

  /**
   * @brief Set decay time
   * @param d Decay amount (0.0 to 0.99)
   */
  void setDecay(Parameter d) {
    decay_ = std::clamp(d, 0.0, 0.99);
    updateDecay();
  }

  /**
   * @brief Process stereo sample
   * @param left Left channel (in/out)
   * @param right Right channel (in/out)
   */
  void process(Sample &left, Sample &right) {
    Sample input = (left + right) * 0.5;

    Sample combOut = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      combOut += processComb(i, input);
    }
    combOut *= 0.25;

    Sample apOut = combOut;
    for (size_t i = 0; i < 2; ++i) {
      apOut = processAllpass(i, apOut);
    }

    left = left * (1.0 - mix_) + apOut * mix_;
    right = right * (1.0 - mix_) + apOut * mix_;
  }

  /**
   * @brief Clear all buffers
   */
  void clear() {
    for (auto &buf : combBuffers_)
      std::fill(buf.begin(), buf.end(), 0.0);
    for (auto &buf : apBuffers_)
      std::fill(buf.begin(), buf.end(), 0.0);
  }

private:
  std::array<std::vector<Sample>, 4> combBuffers_;
  std::array<size_t, 4> combPos_;
  std::array<Sample, 4> combFeedback_;

  std::array<std::vector<Sample>, 2> apBuffers_;
  std::array<size_t, 2> apPos_;

  Parameter mix_;
  Parameter decay_;

  void updateDecay() {
    combFeedback_[0] = 0.805 * decay_;
    combFeedback_[1] = 0.827 * decay_;
    combFeedback_[2] = 0.783 * decay_;
    combFeedback_[3] = 0.764 * decay_;
  }

  Sample processComb(size_t idx, Sample input) {
    auto &buffer = combBuffers_[idx];
    auto &pos = combPos_[idx];

    Sample output = buffer[pos];
    buffer[pos] = input + output * combFeedback_[idx];
    pos = (pos + 1) % buffer.size();

    return output;
  }

  Sample processAllpass(size_t idx, Sample input) {
    auto &buffer = apBuffers_[idx];
    auto &pos = apPos_[idx];
    const Sample g = 0.7;

    Sample delayed = buffer[pos];
    Sample output = -g * input + delayed;
    buffer[pos] = input + g * delayed;
    pos = (pos + 1) % buffer.size();

    return output;
  }
};

} // namespace synth
