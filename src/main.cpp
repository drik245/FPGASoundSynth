/**
 * @file main.cpp
 * @brief FPGA Synth - Korg Minilogue XD Clone
 *
 * Real-time audio synthesis using miniaudio.
 * Target: 24-bit / 192 kHz
 *
 * Features:
 * - Wave mixing (sine + triangle + saw + square + noise)
 * - Full ADSR envelope control
 * - Preset system with drum sounds
 * - Low-pass filter with cutoff and resonance
 */

// C++ standard library headers MUST come before miniaudio
// to avoid std::char_traits conflicts with older GCC versions
#include <conio.h>
#include <iomanip>
#include <iostream>
#include <windows.h>

#include "core/presets.hpp"
#include "engine/synth_engine.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"

using namespace synth;

// Global synth engine
SynthEngine g_synth;
bool g_running = true;
int g_octave = 4;
int g_lastNote = -1;
DWORD g_noteOnTime = 0;

// Current parameter values for display
double g_attack = 0.01;
double g_decay = 0.1;
double g_sustain = 0.7;
double g_release = 0.3;
double g_filterCutoff = 2000.0;
double g_filterRes = 0.3;

// Wave mix levels
double g_sineMix = 0.0;
double g_triMix = 0.0;
double g_sawMix = 1.0;
double g_sqrMix = 0.0;
double g_noiseMix = 0.0;

// Keyboard to MIDI note mapping (QWERTY layout)
int keyToNote(char key) {
  int base = 12 * g_octave;
  switch (key) {
  case 'q':
  case 'Q':
    return base + 0; // C
  case '2':
    return base + 1; // C#
  case 'w':
  case 'W':
    return base + 2; // D
  case '3':
    return base + 3; // D#
  case 'e':
  case 'E':
    return base + 4; // E
  case 'r':
  case 'R':
    return base + 5; // F
  case '5':
    return base + 6; // F#
  case 't':
  case 'T':
    return base + 7; // G
  case '6':
    return base + 8; // G#
  case 'y':
  case 'Y':
    return base + 9; // A
  case '7':
    return base + 10; // A#
  case 'u':
  case 'U':
    return base + 11; // B
  case 'i':
  case 'I':
    return base + 12; // C (next octave)
  default:
    return -1;
  }
}

// Audio callback
void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  float *output = static_cast<float *>(pOutput);

  for (ma_uint32 i = 0; i < frameCount; ++i) {
    Sample left, right;
    g_synth.processStereo(left, right);
    output[i * 2 + 0] = static_cast<float>(left);
    output[i * 2 + 1] = static_cast<float>(right);
  }

  (void)pDevice;
  (void)pInput;
}

void clearScreen() { system("cls"); }

void printUI() {
  clearScreen();
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "                    FPGA SYNTH - Korg Minilogue XD Clone\n";
  std::cout << "                           24-bit / 192 kHz\n";
  std::cout << "==============================================================="
               "=================\n\n";

  std::cout << "  PRESET: " << g_synth.getCurrentPresetName() << " ["
            << g_synth.getCurrentPreset() << "]\n\n";

  std::cout << "  .------------------.    .------------------.\n";
  std::cout << "  |  WAVE MIX        |    |  FILTER          |\n";
  std::cout << "  |------------------|    |------------------|\n";
  std::cout << "  |  Sine:     " << std::fixed << std::setprecision(1)
            << std::setw(4) << g_sineMix * 100
            << "% |    |  Cutoff:  " << std::setw(5) << (int)g_filterCutoff
            << " Hz |\n";
  std::cout << "  |  Triangle: " << std::setw(4) << g_triMix * 100
            << "% |    |  Resonance: " << std::setw(4) << g_filterRes << " |\n";
  std::cout << "  |  Sawtooth: " << std::setw(4) << g_sawMix * 100
            << "% |    '------------------'\n";
  std::cout << "  |  Square:   " << std::setw(4) << g_sqrMix * 100 << "% |\n";
  std::cout << "  |  Noise:    " << std::setw(4) << g_noiseMix * 100 << "% |\n";
  std::cout << "  '------------------'\n\n";

  std::cout << "  .------------------------------------------.\n";
  std::cout << "  |  ADSR ENVELOPE                           |\n";
  std::cout << "  |------------------------------------------|\n";
  std::cout << "  |  Attack: " << std::setw(5) << (int)(g_attack * 1000)
            << " ms    Decay: " << std::setw(5) << (int)(g_decay * 1000)
            << " ms  |\n";
  std::cout << "  |  Sustain:  " << std::setw(3) << (int)(g_sustain * 100)
            << " %      Release: " << std::setw(4) << (int)(g_release * 1000)
            << " ms |\n";
  std::cout << "  '------------------------------------------'\n\n";

  std::cout << "  Keyboard (QWERTY piano):\n";
  std::cout << "  |  2  |  3  |     |  5  |  6  |  7  |       Octave: "
            << g_octave << "\n";
  std::cout << "  | C#  | D#  |     | F#  | G#  | A#  |\n";
  std::cout << "  |  Q  |  W  |  E  |  R  |  T  |  Y  |  U  |  I  |\n";
  std::cout << "  |  C  |  D  |  E  |  F  |  G  |  A  |  B  |  C  |\n\n";

  std::cout
      << "  .---------------------------------------------------------.\n";
  std::cout
      << "  |  CONTROLS                                               |\n";
  std::cout
      << "  |---------------------------------------------------------|\n";
  std::cout
      << "  |  PRESETS:  , . = Previous/Next preset                   |\n";
  std::cout
      << "  |                                                         |\n";
  std::cout
      << "  |  WAVE MIX: A/S/D/F/G = Toggle Sine/Tri/Saw/Sqr/Noise    |\n";
  std::cout
      << "  |                                                         |\n";
  std::cout
      << "  |  FILTER:   [ ] = Cutoff -/+     - = = Resonance -/+     |\n";
  std::cout
      << "  |                                                         |\n";
  std::cout
      << "  |  ADSR:     ! @ = Attack -/+    # $ = Decay -/+          |\n";
  std::cout
      << "  |            % ^ = Sustain -/+   & * = Release -/+        |\n";
  std::cout
      << "  |            (Shift + 1-8)                                |\n";
  std::cout
      << "  |                                                         |\n";
  std::cout
      << "  |  OCTAVE:   Z/X = Down/Up       SPACE = All notes off    |\n";
  std::cout
      << "  |            ESC = Quit                                   |\n";
  std::cout
      << "  '---------------------------------------------------------'\n\n";
}

void updateDisplay(const char *message) {
  // Move cursor to status line and print message
  std::cout << "\r  >> " << std::left << std::setw(50) << message << std::flush;
}

void updateWaveMix() {
  g_synth.setWaveMix(g_sineMix, g_triMix, g_sawMix, g_sqrMix, g_noiseMix);
}

int main() {
  std::cout << "Initializing audio at 192kHz...\n";

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = 192000;
  config.dataCallback = audioCallback;
  config.periodSizeInFrames = 512;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    std::cerr << "Failed at 192kHz, trying 48kHz...\n";
    config.sampleRate = 48000;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
      std::cerr << "Audio initialization failed!\n";
      return -1;
    }
  }

  std::cout << "Audio initialized: " << device.sampleRate << " Hz\n";

  if (ma_device_start(&device) != MA_SUCCESS) {
    std::cerr << "Failed to start audio device!\n";
    ma_device_uninit(&device);
    return -1;
  }

  // Load initial preset
  g_synth.loadPreset(0);

  printUI();

  char statusMsg[64];

  while (g_running) {
    // Auto note-off after 300ms
    if (g_lastNote >= 0 && (GetTickCount() - g_noteOnTime) > 300) {
      g_synth.noteOff(g_lastNote);
      g_lastNote = -1;
    }

    if (_kbhit()) {
      char key = _getch();

      // ESC to quit
      if (key == 27) {
        g_running = false;
        break;
      }

      // Space = all notes off
      if (key == ' ') {
        g_synth.allNotesOff();
        g_lastNote = -1;
        updateDisplay("All notes OFF");
        continue;
      }

      // Preset selection (comma = previous, period = next)
      if (key == ',' || key == '<') {
        int presetNum = g_synth.getCurrentPreset();
        presetNum =
            (presetNum > 0) ? presetNum - 1 : PresetBank::NUM_PRESETS - 1;
        g_synth.loadPreset(presetNum);
        SynthPreset p = PresetBank::getPreset(presetNum);
        g_sineMix = p.waveMix.sine;
        g_triMix = p.waveMix.triangle;
        g_sawMix = p.waveMix.sawtooth;
        g_sqrMix = p.waveMix.square;
        g_noiseMix = p.waveMix.noise;
        g_filterCutoff = p.filterCutoff;
        g_filterRes = p.filterResonance;
        g_attack = p.ampAttack;
        g_decay = p.ampDecay;
        g_sustain = p.ampSustain;
        g_release = p.ampRelease;
        printUI();
        snprintf(statusMsg, sizeof(statusMsg), "Preset: %s", p.name.c_str());
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '.' || key == '>') {
        int presetNum = g_synth.getCurrentPreset();
        presetNum =
            (presetNum < PresetBank::NUM_PRESETS - 1) ? presetNum + 1 : 0;
        g_synth.loadPreset(presetNum);
        SynthPreset p = PresetBank::getPreset(presetNum);
        g_sineMix = p.waveMix.sine;
        g_triMix = p.waveMix.triangle;
        g_sawMix = p.waveMix.sawtooth;
        g_sqrMix = p.waveMix.square;
        g_noiseMix = p.waveMix.noise;
        g_filterCutoff = p.filterCutoff;
        g_filterRes = p.filterResonance;
        g_attack = p.ampAttack;
        g_decay = p.ampDecay;
        g_sustain = p.ampSustain;
        g_release = p.ampRelease;
        printUI();
        snprintf(statusMsg, sizeof(statusMsg), "Preset: %s", p.name.c_str());
        updateDisplay(statusMsg);
        continue;
      }

      // Octave controls
      if (key == 'z' || key == 'Z') {
        g_octave = (g_octave > 1) ? g_octave - 1 : 1;
        snprintf(statusMsg, sizeof(statusMsg), "Octave: %d", g_octave);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == 'x' || key == 'X') {
        g_octave = (g_octave < 7) ? g_octave + 1 : 7;
        snprintf(statusMsg, sizeof(statusMsg), "Octave: %d", g_octave);
        updateDisplay(statusMsg);
        continue;
      }

      // Wave mix toggles
      if (key == 'a' || key == 'A') {
        g_sineMix = (g_sineMix > 0.5) ? 0.0 : 1.0;
        updateWaveMix();
        snprintf(statusMsg, sizeof(statusMsg), "Sine: %.0f%%", g_sineMix * 100);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == 's' || key == 'S') {
        g_triMix = (g_triMix > 0.5) ? 0.0 : 1.0;
        updateWaveMix();
        snprintf(statusMsg, sizeof(statusMsg), "Triangle: %.0f%%",
                 g_triMix * 100);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == 'd' || key == 'D') {
        g_sawMix = (g_sawMix > 0.5) ? 0.0 : 1.0;
        updateWaveMix();
        snprintf(statusMsg, sizeof(statusMsg), "Sawtooth: %.0f%%",
                 g_sawMix * 100);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == 'f' || key == 'F') {
        g_sqrMix = (g_sqrMix > 0.5) ? 0.0 : 1.0;
        updateWaveMix();
        snprintf(statusMsg, sizeof(statusMsg), "Square: %.0f%%",
                 g_sqrMix * 100);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == 'g' || key == 'G') {
        g_noiseMix = (g_noiseMix > 0.5) ? 0.0 : 1.0;
        updateWaveMix();
        snprintf(statusMsg, sizeof(statusMsg), "Noise: %.0f%%",
                 g_noiseMix * 100);
        updateDisplay(statusMsg);
        continue;
      }

      // Filter controls
      if (key == '[') {
        g_filterCutoff = (g_filterCutoff > 100) ? g_filterCutoff * 0.8 : 100;
        g_synth.setFilterCutoff(g_filterCutoff);
        snprintf(statusMsg, sizeof(statusMsg), "Cutoff: %d Hz",
                 (int)g_filterCutoff);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == ']') {
        g_filterCutoff =
            (g_filterCutoff < 15000) ? g_filterCutoff * 1.25 : 15000;
        g_synth.setFilterCutoff(g_filterCutoff);
        snprintf(statusMsg, sizeof(statusMsg), "Cutoff: %d Hz",
                 (int)g_filterCutoff);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '-') {
        g_filterRes = (g_filterRes > 0.1) ? g_filterRes - 0.1 : 0.0;
        g_synth.setFilterResonance(g_filterRes);
        snprintf(statusMsg, sizeof(statusMsg), "Resonance: %.1f", g_filterRes);
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '=') {
        g_filterRes = (g_filterRes < 0.9) ? g_filterRes + 0.1 : 0.95;
        g_synth.setFilterResonance(g_filterRes);
        snprintf(statusMsg, sizeof(statusMsg), "Resonance: %.1f", g_filterRes);
        updateDisplay(statusMsg);
        continue;
      }

      // ADSR controls (Shift + number keys)
      // Attack: ! (Shift+1) and @ (Shift+2)
      if (key == '!') {
        g_attack = (g_attack > 0.01) ? g_attack * 0.7 : 0.001;
        g_synth.setAmpAttack(g_attack);
        snprintf(statusMsg, sizeof(statusMsg), "Attack: %d ms",
                 (int)(g_attack * 1000));
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '@') {
        g_attack = (g_attack < 1.5) ? g_attack * 1.4 : 2.0;
        g_synth.setAmpAttack(g_attack);
        snprintf(statusMsg, sizeof(statusMsg), "Attack: %d ms",
                 (int)(g_attack * 1000));
        updateDisplay(statusMsg);
        continue;
      }

      // Decay: # (Shift+3) and $ (Shift+4)
      if (key == '#') {
        g_decay = (g_decay > 0.01) ? g_decay * 0.7 : 0.001;
        g_synth.setAmpDecay(g_decay);
        snprintf(statusMsg, sizeof(statusMsg), "Decay: %d ms",
                 (int)(g_decay * 1000));
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '$') {
        g_decay = (g_decay < 1.5) ? g_decay * 1.4 : 2.0;
        g_synth.setAmpDecay(g_decay);
        snprintf(statusMsg, sizeof(statusMsg), "Decay: %d ms",
                 (int)(g_decay * 1000));
        updateDisplay(statusMsg);
        continue;
      }

      // Sustain: % (Shift+5) and ^ (Shift+6)
      if (key == '%') {
        g_sustain = (g_sustain > 0.1) ? g_sustain - 0.1 : 0.0;
        g_synth.setAmpSustain(g_sustain);
        snprintf(statusMsg, sizeof(statusMsg), "Sustain: %d%%",
                 (int)(g_sustain * 100));
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '^') {
        g_sustain = (g_sustain < 0.9) ? g_sustain + 0.1 : 1.0;
        g_synth.setAmpSustain(g_sustain);
        snprintf(statusMsg, sizeof(statusMsg), "Sustain: %d%%",
                 (int)(g_sustain * 100));
        updateDisplay(statusMsg);
        continue;
      }

      // Release: & (Shift+7) and * (Shift+8)
      if (key == '&') {
        g_release = (g_release > 0.05) ? g_release * 0.7 : 0.01;
        g_synth.setAmpRelease(g_release);
        snprintf(statusMsg, sizeof(statusMsg), "Release: %d ms",
                 (int)(g_release * 1000));
        updateDisplay(statusMsg);
        continue;
      }
      if (key == '*') {
        g_release = (g_release < 2.5) ? g_release * 1.4 : 3.0;
        g_synth.setAmpRelease(g_release);
        snprintf(statusMsg, sizeof(statusMsg), "Release: %d ms",
                 (int)(g_release * 1000));
        updateDisplay(statusMsg);
        continue;
      }

      // Play notes
      int note = keyToNote(key);
      if (note >= 0) {
        if (g_lastNote >= 0)
          g_synth.noteOff(g_lastNote);
        g_synth.noteOn(note, 0.8);
        g_lastNote = note;
        g_noteOnTime = GetTickCount();
        snprintf(statusMsg, sizeof(statusMsg), "Note: %d", note);
        updateDisplay(statusMsg);
      }
    }

    Sleep(10);
  }

  std::cout << "\nShutting down...\n";
  g_synth.allNotesOff();
  ma_device_uninit(&device);

  return 0;
}
