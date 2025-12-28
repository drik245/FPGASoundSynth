#pragma once
/**
 * @file voice.hpp
 * @brief Complete synthesizer voice
 *
 * Combines all components into a single synth voice:
 * - 2 MixingOscillators with waveform blending
 * - Mixer
 * - Filter with drive
 * - 2 ADSR envelopes (amp + filter)
 */

#include "envelope.hpp"
#include "filter.hpp"
#include "oscillator.hpp"
#include "types.hpp"

namespace synth {

// Forward declaration
struct SynthPreset;

/**
 * @class Voice
 * @brief Single polyphonic voice with wave mixing and full ADSR control
 */
class Voice {
public:
  Voice() : active_(false), note_(0), velocity_(0.0) {
    // Default to saw wave
    osc1_.setMix(0.0, 0.0, 1.0, 0.0, 0.0);
    osc2_.setMix(0.0, 0.0, 1.0, 0.0, 0.0);
  }

  /**
   * @brief Trigger note on
   * @param note MIDI note number
   * @param velocity Note velocity (0.0 to 1.0)
   */
  void noteOn(int note, double velocity = 1.0) {
    note_ = note;
    velocity_ = velocity;
    active_ = true;
    Frequency baseFreq = midiToFrequency(note);
    osc1_.setFrequency(baseFreq);
    osc2_.setFrequency(baseFreq * 1.002); // Slight detune for richness
    multi_.setFrequency(baseFreq);
    ampEnv_.noteOn();
    filterEnv_.noteOn();
    filter_.reset();
  }

  /**
   * @brief Trigger note off
   */
  void noteOff() {
    ampEnv_.noteOff();
    filterEnv_.noteOff();
  }

  /**
   * @brief Check if voice is still active
   * @return true if voice is playing
   */
  bool isActive() const { return active_ && ampEnv_.isActive(); }

  /**
   * @brief Get the MIDI note this voice is playing
   * @return MIDI note number
   */
  int getNote() const { return note_; }

  /**
   * @brief Force stop voice
   */
  void kill() {
    active_ = false;
    ampEnv_.reset();
    filterEnv_.reset();
  }

  // ==================== Wave Mix Setters ====================

  /**
   * @brief Set waveform mix for both oscillators
   */
  void setWaveMix(const WaveMix &mix) {
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setWaveMix(Parameter sine, Parameter tri, Parameter saw, Parameter sqr,
                  Parameter noise = 0.0) {
    osc1_.setMix(sine, tri, saw, sqr, noise);
    osc2_.setMix(sine, tri, saw, sqr, noise);
  }

  void setSineMix(Parameter level) {
    WaveMix mix = osc1_.getMix();
    mix.sine = level;
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setTriangleMix(Parameter level) {
    WaveMix mix = osc1_.getMix();
    mix.triangle = level;
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setSawtoothMix(Parameter level) {
    WaveMix mix = osc1_.getMix();
    mix.sawtooth = level;
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setSquareMix(Parameter level) {
    WaveMix mix = osc1_.getMix();
    mix.square = level;
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setNoiseMix(Parameter level) {
    WaveMix mix = osc1_.getMix();
    mix.noise = level;
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  // Legacy waveform setters (sets single waveform only)
  void setOsc1Waveform(Waveform wf) {
    WaveMix mix = {0, 0, 0, 0, 0};
    switch (wf) {
    case Waveform::SINE:
      mix.sine = 1.0;
      break;
    case Waveform::TRIANGLE:
      mix.triangle = 1.0;
      break;
    case Waveform::SAW:
      mix.sawtooth = 1.0;
      break;
    case Waveform::SQUARE:
      mix.square = 1.0;
      break;
    case Waveform::NOISE:
      mix.noise = 1.0;
      break;
    }
    osc1_.setMix(mix);
    osc2_.setMix(mix);
  }

  void setOsc2Waveform(Waveform wf) {
    WaveMix mix = {0, 0, 0, 0, 0};
    switch (wf) {
    case Waveform::SINE:
      mix.sine = 1.0;
      break;
    case Waveform::TRIANGLE:
      mix.triangle = 1.0;
      break;
    case Waveform::SAW:
      mix.sawtooth = 1.0;
      break;
    case Waveform::SQUARE:
      mix.square = 1.0;
      break;
    case Waveform::NOISE:
      mix.noise = 1.0;
      break;
    }
    osc2_.setMix(mix);
  }

  // ==================== Filter Setters ====================

  void setFilterCutoff(Frequency freq) { baseCutoff_ = freq; }
  void setFilterResonance(Parameter res) { filter_.setResonance(res); }
  void setFilterDrive(Parameter drive) { filter_.setDrive(drive); }

  // ==================== Envelope Setters ====================

  /**
   * @brief Set amplitude envelope ADSR
   */
  void setAmpADSR(double a, double d, Parameter s, double r) {
    ampEnv_.setAttack(a);
    ampEnv_.setDecay(d);
    ampEnv_.setSustain(s);
    ampEnv_.setRelease(r);
  }

  void setAmpAttack(double a) { ampEnv_.setAttack(a); }
  void setAmpDecay(double d) { ampEnv_.setDecay(d); }
  void setAmpSustain(Parameter s) { ampEnv_.setSustain(s); }
  void setAmpRelease(double r) { ampEnv_.setRelease(r); }

  /**
   * @brief Set filter envelope ADSR
   */
  void setFilterADSR(double a, double d, Parameter s, double r) {
    filterEnv_.setAttack(a);
    filterEnv_.setDecay(d);
    filterEnv_.setSustain(s);
    filterEnv_.setRelease(r);
  }

  void setFilterEnvDepth(Parameter depth) { filterEnvDepth_ = depth; }
  void setOscMix(Parameter mix) { oscMix_ = mix; }

  // ==================== Getters ====================

  const WaveMix &getWaveMix() const { return osc1_.getMix(); }
  Frequency getFilterCutoff() const { return baseCutoff_; }
  Parameter getFilterEnvDepth() const { return filterEnvDepth_; }

  /**
   * @brief Process one sample
   * @param lfoValue External LFO value for modulation
   * @return Audio sample
   */
  Sample process(Sample lfoValue = 0.0) {
    if (!isActive()) {
      active_ = false;
      return 0.0;
    }

    Sample ampEnvVal = ampEnv_.process();
    Sample filterEnvVal = filterEnv_.process();

    // Mix both oscillators
    Sample osc1Out = osc1_.process();
    Sample osc2Out = osc2_.process();
    Sample mix = osc1Out * (1.0 - oscMix_) + osc2Out * oscMix_;

    // Apply filter envelope modulation
    Frequency cutoff =
        baseCutoff_ * std::pow(2.0, filterEnvVal * filterEnvDepth_ * 4.0);
    cutoff += lfoValue * 1000.0;
    filter_.setCutoff(std::clamp(cutoff, 20.0, 20000.0));

    Sample filtered = filter_.process(mix);
    return filtered * ampEnvVal * velocity_;
  }

private:
  bool active_;
  int note_;
  double velocity_;
  MixingOscillator osc1_, osc2_; // Now using MixingOscillator!
  MultiEngine multi_;
  StateVariableFilter filter_;
  ADSR ampEnv_, filterEnv_;
  Frequency baseCutoff_ = 2000.0;
  Parameter filterEnvDepth_ = 0.5;
  Parameter oscMix_ = 0.5;
};

} // namespace synth
