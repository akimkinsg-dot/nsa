#include <juce_audio_utils/juce_audio_utils.h>
#include <climits>
#include "ChordGen.h"

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
    env.setParameters(juce::ADSR::Parameters{0.12f, 0.5f, 0.7f, 0.9f});
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

class NSAProcessor : public juce::AudioProcessor {
public:
  NSAProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    synth.addSound(new PadSound());
    for (int i = 0; i < 16; ++i) synth.addVoice(new PadVoice());
    generate();
  }
  const juce::String getName() const override { return "NSA Chords"; }
  void prepareToPlay(double sr, int) override { sampleRate = sr; synth.setCurrentPlaybackSampleRate(sr); }
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 1.0; }
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
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 16) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 11, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    dark = juce::jlimit(0.0f, 1.0f, in.readFloat()); tense = juce::jlimit(0.0f, 1.0f, in.readFloat());
    lastKey = key;
    generate();
  }

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
    juce::ScopedNoDenormals nd;
    buffer.clear(); midi.clear();
    const int n = buffer.getNumSamples();
    bool hostPlaying = false, havePpq = false; double ppq = 0, bpm = 120.0;
    if (auto* ph = getPlayHead())
      if (auto pos = ph->getPosition()) {
        hostPlaying = pos->getIsPlaying();
        if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
        if (auto b = pos->getBpm()) bpm = *b;
      }
    if (bpm < 20.0) bpm = 120.0;
    bool run = false;
    if (hostPlaying && havePpq) run = true;
    else if (preview.load()) { ppq = internalPpq; bpm = 90.0; internalPpq += (double) n / sampleRate * 1.5; run = true; }

    // take a fresh copy of the chords only when they changed
    if (version.load() != snapVersion) {
      juce::SpinLock::ScopedTryLockType tl(lock);
      if (tl.isLocked()) { snap = state; snapVersion = version.load(); }
    }

    juce::MidiBuffer ev;
    if (run) {
      const double pps = bpm / 60.0 / sampleRate;          // beats per sample
      const double p0 = ppq, p1 = p0 + (double) n * pps;
      auto off = [&](double t) { return juce::jlimit(0, n - 1, (int) std::floor((t - p0) / pps)); };
      auto release = [&](double upTo) {
        for (size_t i = 0; i < active.size();) {
          if (active[i].end <= upTo) {
            ev.addEvent(juce::MidiMessage::noteOff(1, active[i].note), off(active[i].end));
            active.erase(active.begin() + (long) i);
          } else ++i;
        }
      };
      auto cutAll = [&](int at) {
        for (auto& a : active) ev.addEvent(juce::MidiMessage::noteOff(1, a.note), at);
        active.clear();
      };
      auto play = [&](const Hit& h, double t, int at) {
        cutAll(at);
        if (h.chord < 0 || h.chord > 3) return;
        const auto& c = snap.voiced[h.chord];
        for (size_t i = 0; i < c.size(); ++i) {
          ev.addEvent(juce::MidiMessage::noteOn(1, c[i], (juce::uint8)(i == 0 ? 85 : 66 + audioRng.nextInt(18))), at);
          active.push_back(Act{c[i], t + h.len});
        }
      };

      const bool jump = !wasRunning || std::abs(p0 - expectedPpq) > 0.25;
      const double base = std::floor(p0 / 16.0) * 16.0;
      if (jump) {
        // start or jump: pick up the chord that should already be sounding
        cutAll(0);
        for (const auto& h : snap.hits) {
          const double t = base + h.start;
          if (t < p0 - 1e-9 && t + h.len > p0 + 1e-9) play(h, t, 0);
        }
      }
      const double w0 = jump ? p0 : expectedPpq;
      for (int cyc = 0; cyc < 2; ++cyc)
        for (const auto& h : snap.hits) {
          const double t = base + cyc * 16.0 + h.start;
          if (t >= w0 && t < p1) { release(t); play(h, t, off(t)); }
        }
      release(p1);
      expectedPpq = p1; wasRunning = true;
    } else if (wasRunning) {
      for (auto& a : active) ev.addEvent(juce::MidiMessage::noteOff(1, a.note), 0);
      active.clear(); wasRunning = false;
    }
    synth.renderNextBlock(buffer, ev, 0, n);
    midi.swapWith(ev);
  }

  // new progression. newSeed=false: same "dice", so moving the pad changes the result smoothly
  void generate(bool newSeed = true) {
    if (newSeed) seed = uiRng.nextInt();
    juce::Random r(seed);
    juce::SpinLock::ScopedLockType l(lock);
    state.generate(r, key, dark, tense, open != 0, newSeed); ++version;
  }
  void padChanged() { generate(false); }
  void evolve() {
    juce::SpinLock::ScopedLockType l(lock);
    state.evolve(uiRng, key, tense, open != 0); ++version;
  }
  void setKey(int nk) {
    const int d = (((nk - lastKey) % 12) + 12) % 12;
    lastKey = nk; key = nk;
    juce::SpinLock::ScopedLockType l(lock);
    state.transpose(d, open != 0, uiRng); ++version;
  }
  void revoice() { juce::SpinLock::ScopedLockType l(lock); state.revoice(open != 0, uiRng); ++version; }
  void setPreview(bool on) { if (on) internalPpq = 0; preview = on; }
  juce::String getNames() { juce::SpinLock::ScopedLockType l(lock); return state.names; }

  juce::File writeMidiFile() {
    ChordGen copy; { juce::SpinLock::ScopedLockType l(lock); copy = state; }
    juce::MidiMessageSequence seq;
    for (const auto& h : copy.hits) {
      if (h.chord < 0 || h.chord > 3) continue;
      const double on = h.start * 480.0, offT = (h.start + h.len) * 480.0 - 10.0;
      const auto& c = copy.voiced[h.chord];
      for (size_t j = 0; j < c.size(); ++j) {
        int note = c[j], vel = j == 0 ? 85 : 66 + uiRng.nextInt(18);
        seq.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), on);
        seq.addEvent(juce::MidiMessage::noteOff(1, note), offT);
      }
    }
    seq.addEvent(juce::MidiMessage::endOfTrack(), 7680.0);
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote(480); mf.addTrack(seq);
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory)
                 .getChildFile("NSA_Chords_" + juce::String(++fileCounter) + ".mid");
    f.deleteFile();
    juce::FileOutputStream out(f);
    if (out.openedOk()) mf.writeTo(out);
    return f;
  }

  std::atomic<int> key{0}, open{0};
  std::atomic<float> dark{0.6f}, tense{0.4f};
  std::atomic<bool> preview{false};

private:
  struct Act { int note; double end; };
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

struct DragBox : juce::Component {
  explicit DragBox(NSAProcessor& p) : proc(p) { setMouseCursor(juce::MouseCursor::DraggingHandCursor); }
  void paint(juce::Graphics& g) override {
    g.setColour(juce::Colour(0xff40267a)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 10.0f);
    g.setColour(juce::Colours::white); g.setFont(17.0f);
    g.drawText("DRAG MIDI  >>  drop on a track", getLocalBounds(), juce::Justification::centred);
  }
  void mouseDrag(const juce::MouseEvent& e) override {
    if (!dragging && e.getDistanceFromDragStart() > 4) {
      dragging = true;
      auto f = proc.writeMidiFile();
      if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles(juce::StringArray(f.getFullPathName()), false, this, [this] { dragging = false; });
      else dragging = false;
    }
  }
  void mouseUp(const juce::MouseEvent&) override { dragging = false; }
  NSAProcessor& proc; bool dragging = false;
};

// 2D pad: left = light, right = dark, bottom = calm, top = tense
struct PadBox : juce::Component {
  explicit PadBox(NSAProcessor& p) : proc(p) {}
  void paint(juce::Graphics& g) override {
    auto b = getLocalBounds().toFloat().reduced(1);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8a7be0), b.getX(), 0.0f, juce::Colour(0xff0b0818), b.getRight(), 0.0f, false));
    g.fillRoundedRectangle(b, 10.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0x66ff4d6d), 0.0f, b.getY(), juce::Colour(0x00ff4d6d), 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.85f)); g.setFont(12.0f);
    g.drawText("light", 10, getHeight() / 2 - 8, 60, 16, juce::Justification::left);
    g.drawText("dark", getWidth() - 70, getHeight() / 2 - 8, 60, 16, juce::Justification::right);
    g.drawText("tense", 0, 5, getWidth(), 16, juce::Justification::centred);
    g.drawText("calm", 0, getHeight() - 21, getWidth(), 16, juce::Justification::centred);
    const float x = b.getX() + proc.dark.load() * b.getWidth();
    const float y = b.getBottom() - proc.tense.load() * b.getHeight();
    g.setColour(juce::Colours::white); g.fillEllipse(x - 9.0f, y - 9.0f, 18.0f, 18.0f);
    g.setColour(juce::Colour(0xff40267a)); g.drawEllipse(x - 9.0f, y - 9.0f, 18.0f, 18.0f, 2.0f);
  }
  void mouseDown(const juce::MouseEvent& e) override { move(e); }
  void mouseDrag(const juce::MouseEvent& e) override { move(e); }
  void move(const juce::MouseEvent& e) {
    auto b = getLocalBounds().toFloat().reduced(1);
    proc.dark = juce::jlimit(0.0f, 1.0f, (e.position.x - b.getX()) / b.getWidth());
    proc.tense = juce::jlimit(0.0f, 1.0f, (b.getBottom() - e.position.y) / b.getHeight());
    proc.padChanged();
    repaint();
  }
  NSAProcessor& proc;
};

class NSAEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit NSAEditor(NSAProcessor& p) : AudioProcessorEditor(&p), proc(p), pad(p), drag(p) {
    setSize(380, 390);
    keyBox.addItemList(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    voiBox.addItemList(juce::StringArray{"Close","Open"}, 1);
    keyBox.setSelectedItemIndex(proc.key, juce::dontSendNotification);
    voiBox.setSelectedItemIndex(proc.open, juce::dontSendNotification);
    keyBox.onChange = [this] { proc.setKey(keyBox.getSelectedItemIndex()); };
    voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.revoice(); };
    genBtn.setButtonText("Generate"); genBtn.onClick = [this] { proc.generate(); };
    contBtn.setButtonText("Continue"); contBtn.onClick = [this] { proc.evolve(); };
    preBtn.setClickingTogglesState(true);
    preBtn.setToggleState(proc.preview.load(), juce::dontSendNotification);
    preBtn.setButtonText(proc.preview.load() ? "Stop" : "Preview");
    preBtn.onClick = [this] {
      bool on = preBtn.getToggleState();
      proc.setPreview(on);
      preBtn.setButtonText(on ? "Stop" : "Preview");
    };
    label.setJustificationType(juce::Justification::centred); label.setFont(15.0f);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    for (juce::Component* c : std::initializer_list<juce::Component*>{&keyBox, &voiBox, &pad, &genBtn, &contBtn, &preBtn, &label, &drag})
      addAndMakeVisible(c);
    startTimerHz(4); timerCallback();
  }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff14101f));
    g.setColour(juce::Colour(0xffb08cff)); g.setFont(18.0f);
    g.drawText("NSA CHORDS", 14, 6, 200, 24, juce::Justification::left);
  }
  void resized() override {
    auto r = getLocalBounds().reduced(14); r.removeFromTop(26);
    auto row = r.removeFromTop(28); int w = row.getWidth() / 2;
    keyBox.setBounds(row.removeFromLeft(w).reduced(3, 0)); voiBox.setBounds(row.reduced(3, 0));
    r.removeFromTop(8); pad.setBounds(r.removeFromTop(150).reduced(3, 0));
    r.removeFromTop(8); row = r.removeFromTop(30); int bw = row.getWidth() / 3;
    genBtn.setBounds(row.removeFromLeft(bw).reduced(3, 0)); contBtn.setBounds(row.removeFromLeft(bw).reduced(3, 0)); preBtn.setBounds(row.reduced(3, 0));
    r.removeFromTop(8); label.setBounds(r.removeFromTop(40));
    r.removeFromTop(6); drag.setBounds(r);
  }
  void timerCallback() override { label.setText(proc.getNames(), juce::dontSendNotification); }
private:
  NSAProcessor& proc;
  juce::ComboBox keyBox, voiBox; juce::TextButton genBtn, contBtn, preBtn; juce::Label label; PadBox pad; DragBox drag;
};

juce::AudioProcessorEditor* NSAProcessor::createEditor() { return new NSAEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NSAProcessor(); }
