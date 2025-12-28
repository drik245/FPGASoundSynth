/**
 * FPGA Sound Synth - Web Audio Synthesizer
 * Interactive demo with wave mixing, ADSR, and filters
 */

class FPGASynth {
    constructor() {
        this.audioContext = null;
        this.masterGain = null;
        this.filter = null;
        this.analyser = null;
        this.activeOscillators = new Map();

        // Synth parameters
        this.params = {
            waveMix: {
                sine: 1.0,
                triangle: 0.0,
                sawtooth: 0.0,
                square: 0.0,
                noise: 0.0
            },
            filter: {
                cutoff: 2000,
                resonance: 1.0
            },
            envelope: {
                attack: 0.01,
                decay: 0.1,
                sustain: 0.7,
                release: 0.3
            }
        };

        // Note frequencies
        this.noteFrequencies = {
            'C4': 261.63, 'C#4': 277.18, 'D4': 293.66, 'D#4': 311.13,
            'E4': 329.63, 'F4': 349.23, 'F#4': 369.99, 'G4': 392.00,
            'G#4': 415.30, 'A4': 440.00, 'A#4': 466.16, 'B4': 493.88,
            'C5': 523.25, 'C#5': 554.37, 'D5': 587.33, 'D#5': 622.25,
            'E5': 659.25
        };

        // Keyboard mapping
        this.keyMap = {
            'q': 'C4', '2': 'C#4', 'w': 'D4', '3': 'D#4', 'e': 'E4',
            'r': 'F4', '5': 'F#4', 't': 'G4', '6': 'G#4', 'y': 'A4',
            '7': 'A#4', 'u': 'B4', 'i': 'C5', '9': 'C#5', 'o': 'D5',
            '0': 'D#5', 'p': 'E5'
        };

        this.pressedKeys = new Set();
        this.initialized = false;

        this.init();
    }

    async initAudio() {
        if (this.audioContext) return;

        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();

        // Create master gain
        this.masterGain = this.audioContext.createGain();
        this.masterGain.gain.value = 0.3;

        // Create filter
        this.filter = this.audioContext.createBiquadFilter();
        this.filter.type = 'lowpass';
        this.filter.frequency.value = this.params.filter.cutoff;
        this.filter.Q.value = this.params.filter.resonance;

        // Create analyser for waveform display
        this.analyser = this.audioContext.createAnalyser();
        this.analyser.fftSize = 2048;

        // Connect: filter -> analyser -> masterGain -> output
        this.filter.connect(this.analyser);
        this.analyser.connect(this.masterGain);
        this.masterGain.connect(this.audioContext.destination);

        this.initialized = true;
        this.startWaveformAnimation();
    }

    init() {
        this.setupEventListeners();
        this.setupKnobs();
        this.setupSliders();
        this.setupKeyboard();
        this.setupPresets();
        this.drawADSR();
        this.drawHeroWaveform();
    }

    setupEventListeners() {
        // Keyboard events
        document.addEventListener('keydown', (e) => this.handleKeyDown(e));
        document.addEventListener('keyup', (e) => this.handleKeyUp(e));

        // First interaction to enable audio
        document.addEventListener('click', () => this.initAudio(), { once: true });
        document.addEventListener('keydown', () => this.initAudio(), { once: true });
    }

    setupKnobs() {
        const knobs = document.querySelectorAll('.knob');

        knobs.forEach(knob => {
            let isDragging = false;
            let startY = 0;
            let startValue = 0;

            const updateKnob = (value) => {
                value = Math.max(0, Math.min(100, value));
                knob.dataset.value = value;

                // Update visual
                const indicator = knob.querySelector('.knob-indicator');
                const progress = knob.querySelector('.knob-progress');
                const valueDisplay = knob.parentElement.querySelector('.knob-value');

                // Rotate indicator (from -135 to 135 degrees)
                const rotation = -135 + (value / 100) * 270;
                knob.querySelector('.knob-inner').style.transform = `rotate(${rotation}deg)`;

                // Update progress ring
                const circumference = 2 * Math.PI * 45;
                const arcLength = (270 / 360) * circumference;
                const offset = arcLength - (value / 100) * arcLength;
                progress.style.strokeDashoffset = 70 + offset;
                progress.style.stroke = `url(#knobGradient)`;
                progress.style.stroke = value > 0 ? '#00d4ff' : 'transparent';

                // Update value display
                valueDisplay.textContent = `${Math.round(value)}%`;

                // Update synth parameter
                const wave = knob.dataset.wave;
                if (wave) {
                    this.params.waveMix[wave] = value / 100;
                }
            };

            knob.addEventListener('mousedown', (e) => {
                isDragging = true;
                startY = e.clientY;
                startValue = parseFloat(knob.dataset.value) || 0;
                document.body.style.cursor = 'grabbing';
            });

            document.addEventListener('mousemove', (e) => {
                if (!isDragging) return;
                const deltaY = startY - e.clientY;
                const newValue = startValue + deltaY * 0.5;
                updateKnob(newValue);
            });

            document.addEventListener('mouseup', () => {
                isDragging = false;
                document.body.style.cursor = '';
            });

            // Initialize knob
            updateKnob(parseFloat(knob.dataset.value) || 0);
        });
    }

    setupSliders() {
        // Cutoff slider
        const cutoffSlider = document.getElementById('cutoffSlider');
        const cutoffValue = document.getElementById('cutoffValue');
        cutoffSlider.addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            this.params.filter.cutoff = value;
            cutoffValue.textContent = value >= 1000 ? `${(value / 1000).toFixed(1)}k Hz` : `${Math.round(value)} Hz`;
            if (this.filter) {
                this.filter.frequency.setValueAtTime(value, this.audioContext.currentTime);
            }
        });

        // Resonance slider
        const resonanceSlider = document.getElementById('resonanceSlider');
        const resonanceValue = document.getElementById('resonanceValue');
        resonanceSlider.addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            this.params.filter.resonance = value;
            resonanceValue.textContent = value.toFixed(1);
            if (this.filter) {
                this.filter.Q.setValueAtTime(value, this.audioContext.currentTime);
            }
        });

        // ADSR sliders
        const adsrSliders = {
            attack: { slider: 'attackSlider', value: 'attackValue', format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` },
            decay: { slider: 'decaySlider', value: 'decayValue', format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` },
            sustain: { slider: 'sustainSlider', value: 'sustainValue', format: (v) => `${Math.round(v * 100)}%` },
            release: { slider: 'releaseSlider', value: 'releaseValue', format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` }
        };

        Object.entries(adsrSliders).forEach(([param, config]) => {
            const slider = document.getElementById(config.slider);
            const valueDisplay = document.getElementById(config.value);

            slider.addEventListener('input', (e) => {
                const value = parseFloat(e.target.value);
                this.params.envelope[param] = value;
                valueDisplay.textContent = config.format(value);
                this.drawADSR();
            });
        });
    }

    setupKeyboard() {
        const keys = document.querySelectorAll('.key');

        keys.forEach(key => {
            key.addEventListener('mousedown', (e) => {
                e.preventDefault();
                this.initAudio();
                const note = key.dataset.note;
                this.noteOn(note);
                key.classList.add('active');
            });

            key.addEventListener('mouseup', () => {
                const note = key.dataset.note;
                this.noteOff(note);
                key.classList.remove('active');
            });

            key.addEventListener('mouseleave', () => {
                const note = key.dataset.note;
                if (this.activeOscillators.has(note)) {
                    this.noteOff(note);
                    key.classList.remove('active');
                }
            });
        });
    }

    setupPresets() {
        const presets = {
            init: {
                waveMix: { sine: 1.0, triangle: 0, sawtooth: 0, square: 0, noise: 0 },
                filter: { cutoff: 2000, resonance: 1 },
                envelope: { attack: 0.01, decay: 0.1, sustain: 0.7, release: 0.3 }
            },
            bass: {
                waveMix: { sine: 0.3, triangle: 0, sawtooth: 0.7, square: 0, noise: 0 },
                filter: { cutoff: 400, resonance: 5 },
                envelope: { attack: 0.01, decay: 0.3, sustain: 0.8, release: 0.2 }
            },
            lead: {
                waveMix: { sine: 0, triangle: 0, sawtooth: 0.6, square: 0.4, noise: 0 },
                filter: { cutoff: 3000, resonance: 3 },
                envelope: { attack: 0.01, decay: 0.1, sustain: 0.6, release: 0.4 }
            },
            pad: {
                waveMix: { sine: 0.5, triangle: 0.5, sawtooth: 0, square: 0, noise: 0 },
                filter: { cutoff: 1500, resonance: 1 },
                envelope: { attack: 0.5, decay: 0.3, sustain: 0.8, release: 1.0 }
            },
            pluck: {
                waveMix: { sine: 0, triangle: 0.3, sawtooth: 0.7, square: 0, noise: 0 },
                filter: { cutoff: 5000, resonance: 3 },
                envelope: { attack: 0.001, decay: 0.3, sustain: 0, release: 0.2 }
            },
            strings: {
                waveMix: { sine: 0, triangle: 0, sawtooth: 1.0, square: 0, noise: 0 },
                filter: { cutoff: 2500, resonance: 2 },
                envelope: { attack: 0.3, decay: 0.2, sustain: 0.7, release: 0.5 }
            },
            fmbell: {
                waveMix: { sine: 1.0, triangle: 0, sawtooth: 0, square: 0, noise: 0 },
                filter: { cutoff: 8000, resonance: 1 },
                envelope: { attack: 0.001, decay: 1.5, sustain: 0, release: 1.0 }
            },
            kick: {
                waveMix: { sine: 1.0, triangle: 0, sawtooth: 0, square: 0, noise: 0 },
                filter: { cutoff: 200, resonance: 8 },
                envelope: { attack: 0.001, decay: 0.2, sustain: 0, release: 0.1 }
            },
            snare: {
                waveMix: { sine: 0.3, triangle: 0.2, sawtooth: 0.2, square: 0, noise: 0.3 },
                filter: { cutoff: 5000, resonance: 2 },
                envelope: { attack: 0.001, decay: 0.15, sustain: 0.1, release: 0.15 }
            },
            hihat: {
                waveMix: { sine: 0, triangle: 0, sawtooth: 0.3, square: 0.3, noise: 0.4 },
                filter: { cutoff: 10000, resonance: 1 },
                envelope: { attack: 0.001, decay: 0.05, sustain: 0, release: 0.05 }
            }
        };

        const presetButtons = document.querySelectorAll('.preset-btn');

        presetButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                const presetName = btn.dataset.preset;
                const preset = presets[presetName];
                if (!preset) return;

                // Update active button
                presetButtons.forEach(b => b.classList.remove('active'));
                btn.classList.add('active');

                // Apply preset
                this.applyPreset(preset);
            });
        });
    }

    applyPreset(preset) {
        // Update wave mix
        Object.entries(preset.waveMix).forEach(([wave, value]) => {
            this.params.waveMix[wave] = value;
            const knob = document.querySelector(`[data-wave="${wave}"]`);
            if (knob) {
                knob.dataset.value = value * 100;
                const valueDisplay = knob.parentElement.querySelector('.knob-value');
                valueDisplay.textContent = `${Math.round(value * 100)}%`;
                const rotation = -135 + value * 270;
                knob.querySelector('.knob-inner').style.transform = `rotate(${rotation}deg)`;
            }
        });

        // Update filter
        document.getElementById('cutoffSlider').value = preset.filter.cutoff;
        document.getElementById('cutoffValue').textContent =
            preset.filter.cutoff >= 1000 ? `${(preset.filter.cutoff / 1000).toFixed(1)}k Hz` : `${preset.filter.cutoff} Hz`;
        document.getElementById('resonanceSlider').value = preset.filter.resonance;
        document.getElementById('resonanceValue').textContent = preset.filter.resonance.toFixed(1);
        this.params.filter = { ...preset.filter };

        if (this.filter) {
            this.filter.frequency.value = preset.filter.cutoff;
            this.filter.Q.value = preset.filter.resonance;
        }

        // Update ADSR
        const adsrConfig = {
            attack: { format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` },
            decay: { format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` },
            sustain: { format: (v) => `${Math.round(v * 100)}%` },
            release: { format: (v) => v >= 1 ? `${v.toFixed(1)}s` : `${Math.round(v * 1000)}ms` }
        };

        Object.entries(preset.envelope).forEach(([param, value]) => {
            document.getElementById(`${param}Slider`).value = value;
            document.getElementById(`${param}Value`).textContent = adsrConfig[param].format(value);
            this.params.envelope[param] = value;
        });

        this.drawADSR();
    }

    handleKeyDown(e) {
        if (e.repeat) return;

        const note = this.keyMap[e.key.toLowerCase()];
        if (!note) return;

        if (this.pressedKeys.has(note)) return;
        this.pressedKeys.add(note);

        this.initAudio();
        this.noteOn(note);

        // Update visual
        const keyElement = document.querySelector(`[data-note="${note}"]`);
        if (keyElement) keyElement.classList.add('active');
    }

    handleKeyUp(e) {
        const note = this.keyMap[e.key.toLowerCase()];
        if (!note) return;

        this.pressedKeys.delete(note);
        this.noteOff(note);

        // Update visual
        const keyElement = document.querySelector(`[data-note="${note}"]`);
        if (keyElement) keyElement.classList.remove('active');
    }

    noteOn(note) {
        if (!this.audioContext || !this.initialized) return;

        const frequency = this.noteFrequencies[note];
        if (!frequency) return;

        // Stop existing oscillators for this note
        this.noteOff(note);

        const oscillators = [];
        const gains = [];
        const oscGain = this.audioContext.createGain();
        oscGain.gain.value = 0;
        oscGain.connect(this.filter);

        // Get total mix level
        const totalMix = Object.values(this.params.waveMix).reduce((a, b) => a + b, 0);
        if (totalMix === 0) return;

        // Create oscillators for each wave type with mix level
        Object.entries(this.params.waveMix).forEach(([type, level]) => {
            if (level <= 0) return;

            // Handle noise separately (it's not a standard oscillator type)
            if (type === 'noise') {
                // Create noise using a buffer source
                const bufferSize = this.audioContext.sampleRate * 2;
                const noiseBuffer = this.audioContext.createBuffer(1, bufferSize, this.audioContext.sampleRate);
                const output = noiseBuffer.getChannelData(0);
                for (let i = 0; i < bufferSize; i++) {
                    output[i] = Math.random() * 2 - 1;
                }

                const noiseSource = this.audioContext.createBufferSource();
                noiseSource.buffer = noiseBuffer;
                noiseSource.loop = true;

                const gain = this.audioContext.createGain();
                gain.gain.value = level / totalMix;

                noiseSource.connect(gain);
                gain.connect(oscGain);
                noiseSource.start();

                oscillators.push(noiseSource);
                gains.push(gain);
                return;
            }

            const osc = this.audioContext.createOscillator();
            osc.type = type;
            osc.frequency.value = frequency;

            const gain = this.audioContext.createGain();
            gain.gain.value = level / totalMix;

            osc.connect(gain);
            gain.connect(oscGain);
            osc.start();

            oscillators.push(osc);
            gains.push(gain);
        });

        // Apply ADSR envelope
        const now = this.audioContext.currentTime;
        const { attack, decay, sustain } = this.params.envelope;

        oscGain.gain.setValueAtTime(0, now);
        oscGain.gain.linearRampToValueAtTime(0.8, now + attack);
        oscGain.gain.linearRampToValueAtTime(sustain * 0.8, now + attack + decay);

        this.activeOscillators.set(note, { oscillators, gains, oscGain });
    }

    noteOff(note) {
        const active = this.activeOscillators.get(note);
        if (!active) return;

        const { oscillators, oscGain } = active;
        const now = this.audioContext.currentTime;
        const release = this.params.envelope.release;

        // Apply release
        oscGain.gain.cancelScheduledValues(now);
        oscGain.gain.setValueAtTime(oscGain.gain.value, now);
        oscGain.gain.linearRampToValueAtTime(0, now + release);

        // Stop oscillators after release
        oscillators.forEach(osc => {
            osc.stop(now + release + 0.1);
        });

        this.activeOscillators.delete(note);
    }

    drawADSR() {
        const canvas = document.getElementById('adsrCanvas');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const rect = canvas.parentElement.getBoundingClientRect();
        canvas.width = rect.width * 2;
        canvas.height = rect.height * 2;
        ctx.scale(2, 2);

        const width = rect.width;
        const height = rect.height;
        const padding = 20;

        // Clear
        ctx.fillStyle = '#0a0a0f';
        ctx.fillRect(0, 0, width, height);

        // Draw grid
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
        ctx.lineWidth = 1;
        for (let i = 0; i <= 4; i++) {
            const y = padding + (height - 2 * padding) * i / 4;
            ctx.beginPath();
            ctx.moveTo(padding, y);
            ctx.lineTo(width - padding, y);
            ctx.stroke();
        }

        const { attack, decay, sustain, release } = this.params.envelope;
        const totalTime = attack + decay + 0.3 + release;
        const xScale = (width - 2 * padding) / totalTime;
        const yScale = height - 2 * padding;

        // Draw envelope
        const gradient = ctx.createLinearGradient(0, 0, width, 0);
        gradient.addColorStop(0, '#00d4ff');
        gradient.addColorStop(0.5, '#7b2cbf');
        gradient.addColorStop(1, '#ff006e');

        ctx.strokeStyle = gradient;
        ctx.lineWidth = 3;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        ctx.beginPath();

        // Start point
        let x = padding;
        let y = height - padding;
        ctx.moveTo(x, y);

        // Attack
        x = padding + attack * xScale;
        y = padding;
        ctx.lineTo(x, y);

        // Decay
        x = padding + (attack + decay) * xScale;
        y = padding + (1 - sustain) * yScale;
        ctx.lineTo(x, y);

        // Sustain
        x = padding + (attack + decay + 0.3) * xScale;
        ctx.lineTo(x, y);

        // Release
        x = padding + totalTime * xScale;
        y = height - padding;
        ctx.lineTo(x, y);

        ctx.stroke();

        // Draw glow
        ctx.shadowColor = '#00d4ff';
        ctx.shadowBlur = 10;
        ctx.stroke();
        ctx.shadowBlur = 0;

        // Draw labels
        ctx.fillStyle = 'rgba(255, 255, 255, 0.3)';
        ctx.font = '10px Orbitron';
        ctx.textAlign = 'center';

        const labelX = [
            padding + attack * xScale / 2,
            padding + attack * xScale + decay * xScale / 2,
            padding + (attack + decay + 0.15) * xScale,
            padding + (attack + decay + 0.3 + release / 2) * xScale
        ];

        ['A', 'D', 'S', 'R'].forEach((label, i) => {
            ctx.fillText(label, labelX[i], height - 5);
        });
    }

    startWaveformAnimation() {
        const canvas = document.getElementById('waveformCanvas');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const dataArray = new Uint8Array(this.analyser.frequencyBinCount);

        const draw = () => {
            requestAnimationFrame(draw);

            const rect = canvas.parentElement.getBoundingClientRect();
            canvas.width = rect.width * 2;
            canvas.height = rect.height * 2;
            ctx.scale(2, 2);

            const width = rect.width;
            const height = rect.height;

            // Clear
            ctx.fillStyle = '#0a0a0f';
            ctx.fillRect(0, 0, width, height);

            // Draw grid
            ctx.strokeStyle = 'rgba(0, 212, 255, 0.1)';
            ctx.lineWidth = 1;
            const gridSize = 20;
            for (let x = 0; x < width; x += gridSize) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, height);
                ctx.stroke();
            }
            for (let y = 0; y < height; y += gridSize) {
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(width, y);
                ctx.stroke();
            }

            // Center line
            ctx.strokeStyle = 'rgba(0, 212, 255, 0.3)';
            ctx.beginPath();
            ctx.moveTo(0, height / 2);
            ctx.lineTo(width, height / 2);
            ctx.stroke();

            // Get waveform data
            this.analyser.getByteTimeDomainData(dataArray);

            // Draw waveform
            const gradient = ctx.createLinearGradient(0, 0, width, 0);
            gradient.addColorStop(0, '#00d4ff');
            gradient.addColorStop(0.5, '#7b2cbf');
            gradient.addColorStop(1, '#ff006e');

            ctx.strokeStyle = gradient;
            ctx.lineWidth = 2;
            ctx.beginPath();

            const sliceWidth = width / dataArray.length;
            let x = 0;

            for (let i = 0; i < dataArray.length; i++) {
                const v = dataArray[i] / 128.0;
                const y = (v * height) / 2;

                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }

                x += sliceWidth;
            }

            ctx.stroke();

            // Glow effect
            ctx.shadowColor = '#00d4ff';
            ctx.shadowBlur = 10;
            ctx.stroke();
            ctx.shadowBlur = 0;
        };

        draw();
    }

    drawHeroWaveform() {
        const container = document.getElementById('heroWaveform');
        if (!container) return;

        const canvas = document.createElement('canvas');
        container.appendChild(canvas);

        const ctx = canvas.getContext('2d');
        let time = 0;

        const draw = () => {
            requestAnimationFrame(draw);

            const rect = container.getBoundingClientRect();
            canvas.width = rect.width * 2;
            canvas.height = rect.height * 2;
            ctx.scale(2, 2);

            const width = rect.width;
            const height = rect.height;

            // Clear
            ctx.fillStyle = 'rgba(20, 20, 30, 0.1)';
            ctx.fillRect(0, 0, width, height);

            // Draw multiple waves
            const waves = [
                { freq: 0.02, amp: 0.3, phase: 0, color: 'rgba(0, 212, 255, 0.5)' },
                { freq: 0.015, amp: 0.25, phase: 2, color: 'rgba(123, 44, 191, 0.5)' },
                { freq: 0.025, amp: 0.2, phase: 4, color: 'rgba(255, 0, 110, 0.5)' }
            ];

            waves.forEach(wave => {
                ctx.strokeStyle = wave.color;
                ctx.lineWidth = 2;
                ctx.beginPath();

                for (let x = 0; x < width; x++) {
                    const y = height / 2 +
                        Math.sin(x * wave.freq + time + wave.phase) * height * wave.amp +
                        Math.sin(x * wave.freq * 2 + time * 1.5 + wave.phase) * height * wave.amp * 0.3;

                    if (x === 0) {
                        ctx.moveTo(x, y);
                    } else {
                        ctx.lineTo(x, y);
                    }
                }

                ctx.stroke();
            });

            time += 0.03;
        };

        draw();
    }
}

// Initialize synth when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.synth = new FPGASynth();
});

// Add SVG gradient definition for knobs
document.addEventListener('DOMContentLoaded', () => {
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.style.position = 'absolute';
    svg.style.width = '0';
    svg.style.height = '0';
    svg.innerHTML = `
        <defs>
            <linearGradient id="knobGradient" x1="0%" y1="0%" x2="100%" y2="100%">
                <stop offset="0%" style="stop-color:#00d4ff"/>
                <stop offset="100%" style="stop-color:#7b2cbf"/>
            </linearGradient>
        </defs>
    `;
    document.body.insertBefore(svg, document.body.firstChild);
});
