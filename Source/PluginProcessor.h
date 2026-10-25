#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "ChordGen.h"
#include <atomic>
#include <memory>
#include <vector>

struct PadSound : juce::SynthesiserSound {
  bool appliesToNote(int) override { return true; }
  bool appliesToChannel(int) override { return true; }
};

struct PadVoice : juce::SynthesiserVoice {
  bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<PadSound*>(s) != nullptr; }
  void startNote(int note, float vel, juce::SynthesiserSound*, int) override {
    inc = juce::MathConstants<double>::twoPi * juce::MidiMessage::getMidiNoteInHertz(note) / getSampleRate();
    phase = 0; level = vel * 0.1f;
    env.setSampleRate(getSampleRate());
    env.setParameters(juce::ADSR::Parameters{0.25f, 0.7f, 0.85f, 1.4f});
    env.noteOn();
  }
  void stopNote(float, bool allow) override { if (allow) env.noteOff(); else { env.reset(); clearCurrentNote(); } }
  void pitchWheelMoved(int) override {}
  void controllerMoved(int, int) override {}
  void renderNextBlock(juce::AudioBuffer<float>& b, int start, int num) override {
    if (!env.isActive()) return;
    while (--num >= 0) {
      float e = env.getNextSample();
      float s = (float)(std::sin(phase) + 0.35 * std::sin(2 * phase) + 0.15 * std::sin(3 * phase)) * level * e;
      for (int c = 0; c < b.getNumChannels(); ++c) b.addSample(c, start, s);
      phase += inc; ++start;
      if (!env.isActive()) { clearCurrentNote(); break; }
    }
  }
  juce::ADSR env; double phase = 0, inc = 0; float level = 0;
};

class VibeSparkProcessor : public juce::AudioProcessor {
public:
  VibeSparkProcessor();
  ~VibeSparkProcessor() override = default;

  const juce::String getName() const override { return "VibeSpark"; }
  void prepareToPlay(double sr, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 1.5; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor* createEditor() override;

  void getStateInformation(juce::MemoryBlock& d) override {
    juce::MemoryOutputStream o(d, true);
    o.writeInt(key); o.writeInt(open); o.writeFloat(dark); o.writeFloat(tense);
    o.writeInt(octaveShift); o.writeBool(noBass); o.writeBool(strum);
    o.writeFloat(vibeX); o.writeFloat(vibeY);
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 16) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 12, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    dark = juce::jlimit(0.0f, 1.0f, in.readFloat()); tense = juce::jlimit(0.0f, 1.0f, in.readFloat());
    octaveShift = juce::jlimit(-1, 1, in.readInt()); noBass = in.readBool(); strum = in.readBool();
    if (in.getNumBytesRemaining() >= 8) {
      vibeX = juce::jlimit(0.0f, 1.0f, in.readFloat());
      vibeY = juce::jlimit(0.0f, 1.0f, in.readFloat());
    }
    lastKey = key;
    generate();
  }

  double getCurrentBpm() const {
    if (auto* ph = const_cast<VibeSparkProcessor*>(this)->getPlayHead())
      if (auto pos = ph->getPosition())
        if (auto b = pos->getBpm())
          if (*b >= 20.0 && *b <= 300.0)
            return *b;
    return 120.0;
  }

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

  void generate(bool newSeed = true) {
    if (newSeed) seed = uiRng.nextInt();
    juce::Random r(seed);
    juce::SpinLock::ScopedLockType l(lock);
    state.generate(r, key, dark, tense, open != 0, getCurrentBpm(), newSeed);
    updateChordNames();
    ++version;
  }

  void setDarkAndTense(float x, float y) {
    vibeX = x; vibeY = y;
    dark = x;
    tense = 1.0f - y; // Invert Y so top is high tension, bottom is calm
    generate(false);
  }

  void setKey(int nk) {
    lastKey = key; key = nk;
    if (nk == 12) {
      generate(true);
    } else {
      const int d = (((nk - lastKey) % 12) + 12) % 12;
      juce::SpinLock::ScopedLockType l(lock);
      state.transpose(d, open != 0, uiRng);
      updateChordNames();
      ++version;
    }
  }

  void previewChord(int chordIndex, juce::MidiBuffer& midiOut) {
    juce::SpinLock::ScopedLockType l(lock);
    if (chordIndex < 0 || chordIndex >= 4) return;
    const auto& c = state.voiced[chordIndex];
    for (int note : c) {
      int n = note + octaveShift.load() * 12;
      if (n >= 0 && n <= 127) {
        midiOut.addEvent(juce::MidiMessage::noteOn(1, (juce::uint8) n, (juce::uint8) 80), 0);
        midiOut.addEvent(juce::MidiMessage::noteOff(1, (juce::uint8) n), 480);
      }
    }
  }

  juce::File writeMidiFile();
  juce::File writeSingleMidiFile(int chordIndex);

  void updateChordNames() {
    chordNames.clear();
    for (int i = 0; i < 4; ++i) {
      juce::String n = juce::String(NOTE_NAMES[state.root[i]]) + SUF[state.type[i]];
      if (state.bass[i] != state.root[i]) n += "/" + juce::String(NOTE_NAMES[state.bass[i]]);
      chordNames.add(n);
    }
  }

  juce::AudioProcessorValueTreeState apvts;
  std::atomic<float> vibeX{0.5f}, vibeY{0.5f};
  std::atomic<float> dark{0.6f}, tense{0.4f};
  std::atomic<int> key{0}, open{0}, octaveShift{0};
  std::atomic<bool> noBass{false}, strum{false}, loop{true};
  std::atomic<int> currentBar{-1};
  std::atomic<float> beatInBar{0.0f};
  std::atomic<bool> hostPlaying{false};
  std::atomic<double> hostBpm{120.0};
  juce::StringArray chordNames;

private:
  struct Act { int note; double end; };
  struct Pend { double t; int note; juce::uint8 vel; };
  std::vector<Pend> pending;
  juce::Synthesiser synth;
  juce::SpinLock lock;
  ChordGen state, snap;
  std::atomic<int> version{0};
  int snapVersion = -1, seed = 1, lastKey = 0;
  juce::Random uiRng, audioRng;
  std::vector<Act> active;
  double sampleRate = 44100.0, internalPpq = 0.0, expectedPpq = 0.0;
  bool wasRunning = false;
  int fileCounter = 0;
};
