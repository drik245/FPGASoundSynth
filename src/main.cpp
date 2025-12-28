/**
 * @file main.cpp
 * @brief FPGA Synth - Korg Minilogue XD Clone
 *
 * Real-time audio synthesis using miniaudio.
 * Target: 24-bit / 192 kHz
 */

// C++ standard library headers MUST come before miniaudio
// to avoid std::char_traits conflicts with older GCC versions
#include <conio.h>
#include <iostream>
#include <windows.h>

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

void printUI() {
  std::cout << "\n";
  std::cout << "========================================\n";
  std::cout << "   FPGA SYNTH - Minilogue XD Clone\n";
  std::cout << "   24-bit / 192 kHz\n";
  std::cout << "========================================\n\n";
  std::cout << "  Keyboard (QWERTY piano):\n";
  std::cout << "  |  2  |  3  |     |  5  |  6  |  7  |\n";
  std::cout << "  | C#  | D#  |     | F#  | G#  | A#  |\n";
  std::cout << "  |  Q  |  W  |  E  |  R  |  T  |  Y  |  U  |  I  |\n";
  std::cout << "  |  C  |  D  |  E  |  F  |  G  |  A  |  B  |  C  |\n\n";
  std::cout << "  Controls:\n";
  std::cout << "  Z/X     - Octave down/up (current: " << g_octave << ")\n";
  std::cout << "  A/S/D/F - Waveform: Saw/Tri/Sqr/Sin\n";
  std::cout << "  [/]     - Filter cutoff down/up\n";
  std::cout << "  -/=     - Resonance down/up\n";
  std::cout << "  SPACE   - Note off (release)\n";
  std::cout << "  ESC     - Quit\n\n";
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

  printUI();

  Frequency filterCutoff = 2000.0;
  Parameter filterRes = 0.3;

  while (g_running) {
    // Auto note-off after 300ms
    if (g_lastNote >= 0 && (GetTickCount() - g_noteOnTime) > 300) {
      g_synth.noteOff(g_lastNote);
      g_lastNote = -1;
    }

    if (_kbhit()) {
      char key = _getch();

      if (key == 27) {
        g_running = false;
        break;
      }

      if (key == ' ') {
        g_synth.allNotesOff();
        g_lastNote = -1;
        std::cout << "Notes OFF          \r" << std::flush;
      } else if (key == 'z' || key == 'Z') {
        g_octave = (g_octave > 1) ? g_octave - 1 : 1;
        std::cout << "Octave: " << g_octave << "           \r" << std::flush;
      } else if (key == 'x' || key == 'X') {
        g_octave = (g_octave < 7) ? g_octave + 1 : 7;
        std::cout << "Octave: " << g_octave << "           \r" << std::flush;
      } else if (key == 'a' || key == 'A') {
        g_synth.setOsc1Waveform(Waveform::SAW);
        std::cout << "Waveform: SAW      \r" << std::flush;
      } else if (key == 's' || key == 'S') {
        g_synth.setOsc1Waveform(Waveform::TRIANGLE);
        std::cout << "Waveform: TRIANGLE \r" << std::flush;
      } else if (key == 'd' || key == 'D') {
        g_synth.setOsc1Waveform(Waveform::SQUARE);
        std::cout << "Waveform: SQUARE   \r" << std::flush;
      } else if (key == 'f' || key == 'F') {
        g_synth.setOsc1Waveform(Waveform::SINE);
        std::cout << "Waveform: SINE     \r" << std::flush;
      } else if (key == '[') {
        filterCutoff = (filterCutoff > 100) ? filterCutoff * 0.8 : 100;
        g_synth.setFilterCutoff(filterCutoff);
        std::cout << "Cutoff: " << (int)filterCutoff << " Hz    \r"
                  << std::flush;
      } else if (key == ']') {
        filterCutoff = (filterCutoff < 15000) ? filterCutoff * 1.25 : 15000;
        g_synth.setFilterCutoff(filterCutoff);
        std::cout << "Cutoff: " << (int)filterCutoff << " Hz    \r"
                  << std::flush;
      } else if (key == '-') {
        filterRes = (filterRes > 0.1) ? filterRes - 0.1 : 0.0;
        g_synth.setFilterResonance(filterRes);
        std::cout << "Resonance: " << filterRes << "    \r" << std::flush;
      } else if (key == '=') {
        filterRes = (filterRes < 0.9) ? filterRes + 0.1 : 0.95;
        g_synth.setFilterResonance(filterRes);
        std::cout << "Resonance: " << filterRes << "    \r" << std::flush;
      } else {
        int note = keyToNote(key);
        if (note >= 0) {
          if (g_lastNote >= 0)
            g_synth.noteOff(g_lastNote);
          g_synth.noteOn(note, 0.8);
          g_lastNote = note;
          g_noteOnTime = GetTickCount();
          std::cout << "Note: " << note << "          \r" << std::flush;
        }
      }
    }

    Sleep(10);
  }

  std::cout << "\nShutting down...\n";
  g_synth.allNotesOff();
  ma_device_uninit(&device);

  return 0;
}
