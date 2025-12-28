#pragma once
/**
 * @file voice.hpp
 * @brief Complete synthesizer voice
 *
 * Combines all components into a single synth voice:
 * - 2 VCOs + Multi-engine
 * - Mixer
 * - Filter
 * - 2 ADSR envelopes
 */

#include "envelope.hpp"
#include "filter.hpp"
#include "oscillator.hpp"
#include "types.hpp"


namespace synth {

/**
 * @class Voice
 * @brief Single polyphonic voice with all synth components
 */
class Voice {
public:
  Voice() : active_(false), note_(0), velocity_(0.0) {}

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
    osc2_.setFrequency(baseFreq * 1.002); // Slight detune
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

  // Parameter setters
  void setOsc1Waveform(Waveform wf) { osc1_.setWaveform(wf); }
  void setOsc2Waveform(Waveform wf) { osc2_.setWaveform(wf); }
  void setFilterCutoff(Frequency freq) { baseCutoff_ = freq; }
  void setFilterResonance(Parameter res) { filter_.setResonance(res); }
  void setFilterDrive(Parameter drive) { filter_.setDrive(drive); }

  /**
   * @brief Set amplitude envelope ADSR
   * @param a Attack time (seconds)
   * @param d Decay time (seconds)
   * @param s Sustain level (0.0 to 1.0)
   * @param r Release time (seconds)
   */
  void setAmpADSR(double a, double d, Parameter s, double r) {
    ampEnv_.setAttack(a);
    ampEnv_.setDecay(d);
    ampEnv_.setSustain(s);
    ampEnv_.setRelease(r);
  }

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

    Sample osc1Out = osc1_.process();
    Sample osc2Out = osc2_.process();
    Sample mix = osc1Out * (1.0 - oscMix_) + osc2Out * oscMix_;

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
  Oscillator osc1_, osc2_;
  MultiEngine multi_;
  StateVariableFilter filter_;
  ADSR ampEnv_, filterEnv_;
  Frequency baseCutoff_ = 2000.0;
  Parameter filterEnvDepth_ = 0.5;
  Parameter oscMix_ = 0.5;
};

} // namespace synth
