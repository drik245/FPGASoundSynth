#pragma once
/**
 * @file synth_engine.hpp
 * @brief Polyphonic synth engine with voice management
 *
 * Manages 4-voice polyphony with voice allocation and stealing.
 */

#include "../core/lfo.hpp"
#include "../core/types.hpp"
#include "../core/voice.hpp"
#include <array>


namespace synth {

/**
 * @class SynthEngine
 * @brief 4-voice polyphonic synthesizer engine
 */
class SynthEngine {
public:
  static constexpr int MAX_VOICES = 4;

  SynthEngine() {
    for (auto &voice : voices_) {
      voice.setOsc1Waveform(Waveform::SAW);
      voice.setOsc2Waveform(Waveform::SAW);
      voice.setFilterCutoff(2000.0);
      voice.setFilterResonance(0.3);
      voice.setAmpADSR(0.01, 0.3, 0.6, 0.5);
      voice.setFilterADSR(0.01, 0.2, 0.3, 0.3);
      voice.setFilterEnvDepth(0.5);
    }
    lfo_.setRate(2.0);
    lfo_.setShape(LFO::Shape::TRIANGLE);
  }

  /**
   * @brief Trigger note on
   * @param note MIDI note number
   * @param velocity Note velocity (0.0 to 1.0)
   */
  void noteOn(int note, double velocity = 1.0) {
    Voice *target = nullptr;
    for (auto &voice : voices_) {
      if (!voice.isActive()) {
        target = &voice;
        break;
      }
    }
    if (!target)
      target = &voices_[0]; // Simple steal
    target->noteOn(note, velocity);
  }

  /**
   * @brief Trigger note off
   * @param note MIDI note number
   */
  void noteOff(int note) {
    for (auto &voice : voices_) {
      if (voice.isActive() && voice.getNote() == note) {
        voice.noteOff();
      }
    }
  }

  /**
   * @brief Release all notes
   */
  void allNotesOff() {
    for (auto &voice : voices_)
      voice.noteOff();
  }

  // Global parameter setters
  void setOsc1Waveform(Waveform wf) {
    for (auto &v : voices_)
      v.setOsc1Waveform(wf);
  }
  void setOsc2Waveform(Waveform wf) {
    for (auto &v : voices_)
      v.setOsc2Waveform(wf);
  }
  void setFilterCutoff(Frequency f) {
    for (auto &v : voices_)
      v.setFilterCutoff(f);
  }
  void setFilterResonance(Parameter r) {
    for (auto &v : voices_)
      v.setFilterResonance(r);
  }
  void setLfoRate(Frequency hz) { lfo_.setRate(hz); }
  void setLfoShape(LFO::Shape s) { lfo_.setShape(s); }
  void setMasterVolume(Parameter vol) { masterVolume_ = vol; }

  /**
   * @brief Process one mono sample
   * @return Mixed audio sample
   */
  Sample process() {
    Sample lfoVal = lfo_.process();
    Sample output = 0.0;

    for (auto &voice : voices_) {
      if (voice.isActive()) {
        output += voice.process(lfoVal * lfoDepth_);
      }
    }

    return output * masterVolume_ * 0.5;
  }

  /**
   * @brief Process one stereo sample
   * @param left Left channel output
   * @param right Right channel output
   */
  void processStereo(Sample &left, Sample &right) {
    Sample mono = process();
    left = mono;
    right = mono;
  }

private:
  std::array<Voice, MAX_VOICES> voices_;
  LFO lfo_;
  Parameter lfoDepth_ = 0.2;
  Parameter masterVolume_ = 0.8;
};

} // namespace synth
