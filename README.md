# 🎹 FPGA Sound Synthesizer - Korg Minilogue XD Clone

A pure FPGA synthesizer modeled after the **Korg Minilogue XD**, developed using a C++ prototype for testing before HDL synthesis.

## 🎯 Project Goals

- Create a **4-voice polyphonic synthesizer** on FPGA
- Model the sound characteristics of the Korg Minilogue XD
- Target **24-bit / 192 kHz** audiophile-grade audio quality
- Use **Simulink + HDL Coder** workflow for Verilog generation

## 📁 Project Structure

```
Sound Synth/
├── minilogue_synth.exe     ← Compiled Windows executable
├── README.md               ← This file
├── CMakeLists.txt          ← CMake build configuration
├── build.bat               ← CMake build script
├── compile.bat             ← Quick g++ compile script
│
├── include/
│   └── miniaudio.h         ← Single-header audio library
│
├── src/
│   ├── main.cpp            ← Entry point with keyboard UI
│   │
│   ├── core/               ← Core DSP modules (FPGA-portable)
│   │   ├── types.hpp       ← Type definitions & fixed-point helpers
│   │   ├── oscillator.hpp  ← VCO with PolyBLEP anti-aliasing
│   │   ├── filter.hpp      ← 2-pole SVF & Moog ladder filter
│   │   ├── envelope.hpp    ← ADSR envelope generator
│   │   └── lfo.hpp         ← Low-frequency oscillator
│   │
│   ├── effects/            ← Effects processing
│   │   ├── chorus.hpp      ← Modulated delay chorus/flanger
│   │   ├── delay.hpp       ← Stereo delay with feedback
│   │   └── reverb.hpp      ← Schroeder reverb algorithm
│   │
│   └── engine/
│       └── synth_engine.hpp ← 4-voice polyphonic engine
│
├── simulink/               ← Simulink models (TODO)
└── hdl/                    ← Generated Verilog output (TODO)
```

## 🎛️ Synthesizer Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         VOICE (×4 Polyphony)                            │
│                                                                         │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌──────────┐  │
│  │  VCO 1  │ + │  VCO 2  │ + │  Multi  │ → │  MIXER  │ → │  FILTER  │──┼─→
│  │  (Saw,  │   │  (Saw,  │   │ Engine  │   │         │   │  (SVF)   │  │
│  │  Tri,   │   │  Tri,   │   │  (FM)   │   │         │   │  12dB/oct│  │
│  │  Sqr,   │   │  Sqr,   │   │         │   │         │   │  + Drive │  │
│  │  Sin)   │   │  Sin)   │   │         │   │         │   │          │  │
│  └────┬────┘   └────┬────┘   └─────────┘   └─────────┘   └────┬─────┘  │
│       │             │                                          │        │
│       └─────────────┴──────────────────────────────────────────┘        │
│                              Modulation                                 │
│  ┌─────────┐   ┌─────────┐                                             │
│  │   LFO   │ → │  EG 1   │  (Filter Envelope)                          │
│  │         │   │  ADSR   │                                             │
│  └─────────┘   └─────────┘                                             │
│                                                                         │
│                ┌─────────┐                                             │
│                │  EG 2   │  (Amp Envelope) ──────────→ VCA ──→ OUT     │
│                │  ADSR   │                                             │
│                └─────────┘                                             │
└─────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
                    ┌────────────────────────────────────┐
                    │          VOICE MIXER (×4)          │
                    └────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         EFFECTS SECTION                                 │
│                                                                         │
│   ┌──────────┐      ┌──────────┐      ┌──────────┐                     │
│   │  CHORUS  │  →   │  DELAY   │  →   │  REVERB  │  →  STEREO OUT     │
│   │ /Flanger │      │          │      │          │                     │
│   └──────────┘      └──────────┘      └──────────┘                     │
└─────────────────────────────────────────────────────────────────────────┘
```

## 🎹 Keyboard Controls

### Piano Keys (QWERTY Layout)
```
    │  2  │  3  │     │  5  │  6  │  7  │
    │ C#  │ D#  │     │ F#  │ G#  │ A#  │
┌───┴─┬───┴─┬───┴─┬───┴─┬───┴─┬───┴─┬───┴─┬───┐
│  Q  │  W  │  E  │  R  │  T  │  Y  │  U  │  I  │
│  C  │  D  │  E  │  F  │  G  │  A  │  B  │  C  │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

### Control Keys

| Key | Function |
|-----|----------|
| **Z** | Octave down |
| **X** | Octave up |
| **A** | Waveform: Saw |
| **S** | Waveform: Triangle |
| **D** | Waveform: Square |
| **F** | Waveform: Sine |
| **[** | Filter cutoff down |
| **]** | Filter cutoff up |
| **-** | Resonance down |
| **=** | Resonance up |
| **SPACE** | All notes off |
| **ESC** | Quit |

## 🎚️ Target Specifications

| Parameter | Value |
|-----------|-------|
| Sample Rate | 192 kHz |
| Bit Depth | 24-bit |
| Polyphony | 4 voices |
| Oscillators | 2 VCO + Multi-engine per voice |
| Filter | 2-pole 12dB/oct State Variable |
| Envelopes | 2× ADSR (Filter + Amp) |
| LFO | Sine, Tri, Saw, Square, S&H |
| Effects | Chorus, Delay, Reverb |

## 🔧 Build Instructions

### Prerequisites
- **Windows 10/11**
- **MinGW-w64** with g++ (or Visual Studio)
- **CMake 3.20+** (optional, for CMake build)

### Quick Build (g++)
```cmd
cd "C:\Users\drikp\Desktop\Sound Synth"
.\compile.bat
```

Or manually:
```cmd
g++ -std=c++14 -O2 -I./src -I./include src/main.cpp -o minilogue_synth.exe -lole32 -lwinmm
```

### CMake Build
```cmd
.\build.bat
```

### Run
```cmd
.\minilogue_synth.exe
```

## 🛠️ Development Phases

### ✅ Phase 1: C++ Prototype
- [x] Core DSP modules (Oscillator, Filter, Envelope, LFO)
- [x] 4-voice polyphonic engine
- [x] Real-time audio playback (miniaudio)
- [x] Keyboard input UI
- [x] Basic effects (Chorus, Delay, Reverb)

### 🔄 Phase 2: Sound Design
- [ ] Parameter tuning to match Minilogue XD
- [ ] Wave shaping and sync
- [ ] Voice detune and unison mode
- [ ] Velocity and modulation wheel support

### 📐 Phase 3: Simulink Models
- [ ] Oscillator model with PolyBLEP
- [ ] State Variable Filter model
- [ ] ADSR Envelope model
- [ ] Fixed-point conversion

### 🔌 Phase 4: HDL Generation
- [ ] HDL Coder Verilog output
- [ ] FPGA resource optimization
- [ ] I2S audio interface
- [ ] MIDI input module

### 🎛️ Phase 5: Hardware Integration
- [ ] FPGA synthesis and testing
- [ ] DAC integration (24-bit I2S)
- [ ] Front panel controls
- [ ] Full MIDI implementation

## 📚 DSP Algorithms Used

| Component | Algorithm |
|-----------|-----------|
| Oscillators | Phase Accumulator + PolyBLEP anti-aliasing |
| Filter | Chamberlin State Variable Filter |
| Envelopes | Exponential segment ADSR |
| Chorus | Modulated delay line with LFO |
| Delay | Circular buffer with interpolation |
| Reverb | Schroeder reverb (4 comb + 2 allpass) |
| FM Synth | 2-operator Phase Modulation |

## 🎯 FPGA Targets

Recommended boards for this project:

| Board | FPGA | Resources | Price |
|-------|------|-----------|-------|
| Digilent Arty A7-35T | Xilinx Artix-7 | 33K LUTs, 90 DSP | ~$130 |
| Terasic DE10-Nano | Intel Cyclone V | 40K LEs, 112 DSP | ~$130 |
| Digilent Zybo Z7-20 | Xilinx Zynq | ARM + FPGA | ~$200 |

## 📖 References

- [Korg Minilogue XD Manual](https://www.korg.com/us/products/synthesizers/minilogue_xd/)
- [PolyBLEP Oscillators](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/)
- [The Art of VA Filter Design](https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf)
- [miniaudio - Single-header Audio Library](https://miniaud.io/)

## 📄 License

MIT License - Feel free to use for your own synthesizer projects!

---

*Created with ❤️ for the love of synthesizers and FPGAs*
