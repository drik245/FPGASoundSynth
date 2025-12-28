#pragma once
/**
 * @file synth_engine.hpp
 * @brief Polyphonic synth engine with voice management
 *
 * Manages 4-voice polyphony with voice allocation and stealing.
 * Now includes wave mixing, preset support, and full ADSR control.
 */

#include "../core/lfo.hpp"
#include "../core/presets.hpp"
#include "../core/types.hpp"
#include "../core/voice.hpp"
#include <array>

namespace synth {

/**
 * @class SynthEngine
 * @brief 4-voice polyphonic synthesizer engine with wave mixing and presets
 */
class SynthEngine {
public:
  static constexpr int MAX_VOICES = 4;

  SynthEngine() {
    // Load init preset
    loadPreset(0);
    lfo_.setRate(2.0);
    lfo_.setShape(LFO::Shape::TRIANGLE);
  }

  // ==================== Note Control ====================

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

  // ==================== Preset System ====================

  /**
   * @brief Load a preset by index
   */
  void loadPreset(int index) {
    if (index < 0 || index >= PresetBank::NUM_PRESETS)
      return;

    currentPreset_ = index;
    SynthPreset preset = PresetBank::getPreset(index);
    applyPreset(preset);
  }

  /**
   * @brief Apply a preset to all voices
   */
  void applyPreset(const SynthPreset &preset) {
    for (auto &v : voices_) {
      v.setWaveMix(preset.waveMix);
      v.setFilterCutoff(preset.filterCutoff);
      v.setFilterResonance(preset.filterResonance);
      v.setFilterDrive(preset.filterDrive);
      v.setAmpADSR(preset.ampAttack, preset.ampDecay, preset.ampSustain,
                   preset.ampRelease);
      v.setFilterADSR(preset.filterAttack, preset.filterDecay,
                      preset.filterSustain, preset.filterRelease);
      v.setFilterEnvDepth(preset.filterEnvDepth);
    }
    masterVolume_ = preset.masterVolume;
  }

  int getCurrentPreset() const { return currentPreset_; }
  const char *getCurrentPresetName() const {
    return PresetBank::getPresetName(currentPreset_);
  }

  // ==================== Wave Mixing ====================

  /**
   * @brief Set waveform mix for all voices
   */
  void setWaveMix(const WaveMix &mix) {
    for (auto &v : voices_)
      v.setWaveMix(mix);
  }

  void setWaveMix(Parameter sine, Parameter tri, Parameter saw, Parameter sqr,
                  Parameter noise = 0.0) {
    for (auto &v : voices_)
      v.setWaveMix(sine, tri, saw, sqr, noise);
  }

  void setSineMix(Parameter level) {
    for (auto &v : voices_)
      v.setSineMix(level);
  }

  void setTriangleMix(Parameter level) {
    for (auto &v : voices_)
      v.setTriangleMix(level);
  }

  void setSawtoothMix(Parameter level) {
    for (auto &v : voices_)
      v.setSawtoothMix(level);
  }

  void setSquareMix(Parameter level) {
    for (auto &v : voices_)
      v.setSquareMix(level);
  }

  void setNoiseMix(Parameter level) {
    for (auto &v : voices_)
      v.setNoiseMix(level);
  }

  // Legacy waveform setters (for backward compatibility)
  void setOsc1Waveform(Waveform wf) {
    for (auto &v : voices_)
      v.setOsc1Waveform(wf);
  }

  void setOsc2Waveform(Waveform wf) {
    for (auto &v : voices_)
      v.setOsc2Waveform(wf);
  }

  // ==================== Filter Control ====================

  void setFilterCutoff(Frequency f) {
    for (auto &v : voices_)
      v.setFilterCutoff(f);
  }

  void setFilterResonance(Parameter r) {
    for (auto &v : voices_)
      v.setFilterResonance(r);
  }

  void setFilterDrive(Parameter d) {
    for (auto &v : voices_)
      v.setFilterDrive(d);
  }

  // ==================== ADSR Control ====================

  /**
   * @brief Set amplitude envelope for all voices
   */
  void setAmpADSR(double a, double d, Parameter s, double r) {
    for (auto &v : voices_)
      v.setAmpADSR(a, d, s, r);
  }

  void setAmpAttack(double a) {
    for (auto &v : voices_)
      v.setAmpAttack(a);
  }

  void setAmpDecay(double d) {
    for (auto &v : voices_)
      v.setAmpDecay(d);
  }

  void setAmpSustain(Parameter s) {
    for (auto &v : voices_)
      v.setAmpSustain(s);
  }

  void setAmpRelease(double r) {
    for (auto &v : voices_)
      v.setAmpRelease(r);
  }

  /**
   * @brief Set filter envelope for all voices
   */
  void setFilterADSR(double a, double d, Parameter s, double r) {
    for (auto &v : voices_)
      v.setFilterADSR(a, d, s, r);
  }

  void setFilterEnvDepth(Parameter depth) {
    for (auto &v : voices_)
      v.setFilterEnvDepth(depth);
  }

  // ==================== LFO Control ====================

  void setLfoRate(Frequency hz) { lfo_.setRate(hz); }
  void setLfoShape(LFO::Shape s) { lfo_.setShape(s); }
  void setLfoDepth(Parameter depth) { lfoDepth_ = depth; }

  // ==================== Master Control ====================

  void setMasterVolume(Parameter vol) { masterVolume_ = vol; }

  // ==================== Audio Processing ====================

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
  int currentPreset_ = 0;
};

} // namespace synth
