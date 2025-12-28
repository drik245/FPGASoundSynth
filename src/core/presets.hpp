#pragma once
/**
 * @file presets.hpp
 * @brief Synthesizer preset system
 *
 * Contains preset definitions for various sounds including
 * synth patches and drum sounds.
 */

#include "oscillator.hpp"
#include "types.hpp"
#include <string>

namespace synth {

/**
 * @struct SynthPreset
 * @brief Complete synthesizer preset containing all parameters
 */
struct SynthPreset {
  std::string name;

  // Oscillator wave mix
  WaveMix waveMix;

  // Filter parameters
  Frequency filterCutoff = 2000.0;
  Parameter filterResonance = 0.3;
  Parameter filterDrive = 0.0;

  // Amplitude envelope
  double ampAttack = 0.01;
  double ampDecay = 0.1;
  Parameter ampSustain = 0.7;
  double ampRelease = 0.3;

  // Filter envelope
  double filterAttack = 0.01;
  double filterDecay = 0.1;
  Parameter filterSustain = 0.3;
  double filterRelease = 0.3;
  Parameter filterEnvDepth = 0.5;

  // Master
  Parameter masterVolume = 0.8;
};

/**
 * @class PresetBank
 * @brief Collection of factory presets
 */
class PresetBank {
public:
  static constexpr int NUM_PRESETS = 10;

  static SynthPreset getPreset(int index) {
    switch (index) {
    case 0:
      return initPreset();
    case 1:
      return bassPreset();
    case 2:
      return leadPreset();
    case 3:
      return padPreset();
    case 4:
      return kickPreset();
    case 5:
      return snarePreset();
    case 6:
      return hihatPreset();
    case 7:
      return pluckPreset();
    case 8:
      return stringsPreset();
    case 9:
      return fmBellPreset();
    default:
      return initPreset();
    }
  }

  static const char *getPresetName(int index) {
    static const char *names[] = {"Init",    "Bass",   "Lead",   "Pad",
                                  "Kick",    "Snare",  "Hi-Hat", "Pluck",
                                  "Strings", "FM Bell"};
    if (index >= 0 && index < NUM_PRESETS) {
      return names[index];
    }
    return "Unknown";
  }

private:
  // ==================== SYNTH PRESETS ====================

  static SynthPreset initPreset() {
    SynthPreset p;
    p.name = "Init";
    p.waveMix = {1.0, 0.0, 0.0, 0.0, 0.0}; // Pure sine
    p.filterCutoff = 2000.0;
    p.filterResonance = 0.3;
    p.ampAttack = 0.01;
    p.ampDecay = 0.1;
    p.ampSustain = 0.7;
    p.ampRelease = 0.3;
    p.filterEnvDepth = 0.3;
    return p;
  }

  static SynthPreset bassPreset() {
    SynthPreset p;
    p.name = "Bass";
    p.waveMix = {0.3, 0.0, 0.7, 0.0, 0.0}; // Sine + Saw
    p.filterCutoff = 400.0;
    p.filterResonance = 0.5;
    p.filterDrive = 0.2;
    p.ampAttack = 0.01;
    p.ampDecay = 0.3;
    p.ampSustain = 0.8;
    p.ampRelease = 0.2;
    p.filterAttack = 0.01;
    p.filterDecay = 0.2;
    p.filterSustain = 0.2;
    p.filterEnvDepth = 0.6;
    return p;
  }

  static SynthPreset leadPreset() {
    SynthPreset p;
    p.name = "Lead";
    p.waveMix = {0.0, 0.0, 0.6, 0.4, 0.0}; // Saw + Square
    p.filterCutoff = 3000.0;
    p.filterResonance = 0.4;
    p.ampAttack = 0.01;
    p.ampDecay = 0.1;
    p.ampSustain = 0.6;
    p.ampRelease = 0.4;
    p.filterAttack = 0.01;
    p.filterDecay = 0.15;
    p.filterSustain = 0.4;
    p.filterEnvDepth = 0.5;
    return p;
  }

  static SynthPreset padPreset() {
    SynthPreset p;
    p.name = "Pad";
    p.waveMix = {0.5, 0.5, 0.0, 0.0, 0.0}; // Sine + Triangle
    p.filterCutoff = 1500.0;
    p.filterResonance = 0.2;
    p.ampAttack = 0.5;
    p.ampDecay = 0.3;
    p.ampSustain = 0.8;
    p.ampRelease = 1.0;
    p.filterAttack = 0.4;
    p.filterDecay = 0.3;
    p.filterSustain = 0.5;
    p.filterEnvDepth = 0.3;
    return p;
  }

  static SynthPreset pluckPreset() {
    SynthPreset p;
    p.name = "Pluck";
    p.waveMix = {0.0, 0.3, 0.7, 0.0, 0.0}; // Triangle + Saw
    p.filterCutoff = 5000.0;
    p.filterResonance = 0.3;
    p.ampAttack = 0.001;
    p.ampDecay = 0.3;
    p.ampSustain = 0.0;
    p.ampRelease = 0.2;
    p.filterAttack = 0.001;
    p.filterDecay = 0.2;
    p.filterSustain = 0.1;
    p.filterEnvDepth = 0.7;
    return p;
  }

  static SynthPreset stringsPreset() {
    SynthPreset p;
    p.name = "Strings";
    p.waveMix = {0.0, 0.0, 1.0, 0.0, 0.0}; // Pure Saw (detuned in engine)
    p.filterCutoff = 2500.0;
    p.filterResonance = 0.2;
    p.ampAttack = 0.3;
    p.ampDecay = 0.2;
    p.ampSustain = 0.7;
    p.ampRelease = 0.5;
    p.filterAttack = 0.2;
    p.filterDecay = 0.2;
    p.filterSustain = 0.5;
    p.filterEnvDepth = 0.2;
    return p;
  }

  static SynthPreset fmBellPreset() {
    SynthPreset p;
    p.name = "FM Bell";
    p.waveMix = {1.0, 0.0, 0.0, 0.0, 0.0}; // Sine (FM in MultiEngine)
    p.filterCutoff = 8000.0;
    p.filterResonance = 0.1;
    p.ampAttack = 0.001;
    p.ampDecay = 1.5;
    p.ampSustain = 0.0;
    p.ampRelease = 1.0;
    p.filterEnvDepth = 0.1;
    return p;
  }

  // ==================== DRUM PRESETS ====================

  static SynthPreset kickPreset() {
    SynthPreset p;
    p.name = "Kick";
    p.waveMix = {1.0, 0.0, 0.0, 0.0, 0.0}; // Pure sine for thump
    p.filterCutoff = 200.0;
    p.filterResonance = 0.8;
    p.filterDrive = 0.3;
    p.ampAttack = 0.001;
    p.ampDecay = 0.2;
    p.ampSustain = 0.0;
    p.ampRelease = 0.1;
    p.filterAttack = 0.001;
    p.filterDecay = 0.15;
    p.filterSustain = 0.0;
    p.filterEnvDepth = 0.9; // High env depth for pitch drop
    p.masterVolume = 1.0;
    return p;
  }

  static SynthPreset snarePreset() {
    SynthPreset p;
    p.name = "Snare";
    p.waveMix = {0.3, 0.2, 0.2, 0.0, 0.3}; // Mix with noise for snare
    p.filterCutoff = 5000.0;
    p.filterResonance = 0.3;
    p.ampAttack = 0.001;
    p.ampDecay = 0.15;
    p.ampSustain = 0.1;
    p.ampRelease = 0.15;
    p.filterAttack = 0.001;
    p.filterDecay = 0.1;
    p.filterSustain = 0.2;
    p.filterEnvDepth = 0.5;
    p.masterVolume = 0.9;
    return p;
  }

  static SynthPreset hihatPreset() {
    SynthPreset p;
    p.name = "Hi-Hat";
    p.waveMix = {0.0, 0.0, 0.3, 0.3, 0.4}; // Saw + Square + Noise
    p.filterCutoff = 10000.0;
    p.filterResonance = 0.2;
    p.ampAttack = 0.001;
    p.ampDecay = 0.05;
    p.ampSustain = 0.0;
    p.ampRelease = 0.05;
    p.filterEnvDepth = 0.1;
    p.masterVolume = 0.7;
    return p;
  }
};

} // namespace synth
