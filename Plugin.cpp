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
    juce::MemoryOutputStream o(d, true); o.writeInt(key); o.writeInt(mood); o.writeInt(open);
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 12) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 11, in.readInt()); mood = juce::jlimit(0, 4, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    generate();
  }

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
    juce::ScopedNoDenormals nd;
    buffer.clear(); midi.clear();
    const int n = buffer.getNumSamples();
    bool hostPlaying = false, havePpq = false; double ppq = 0;
    if (auto* ph = getPlayHead())
      if (auto pos = ph->getPosition()) {
        hostPlaying = pos->getIsPlaying();
        if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
      }
    bool run = false;
    if (hostPlaying && havePpq) run = true;
    else if (preview.load()) { internalPpq += (double) n / sampleRate * 1.5; ppq = internalPpq; run = true; }
    juce::MidiBuffer ev;
    if (run) {
      int bar = (int) std::floor(ppq / 4.0);
      if (bar != lastBar) {
        for (int note : held) ev.addEvent(juce::MidiMessage::noteOff(1, note), 0);
        std::vector<int> c;
        { juce::SpinLock::ScopedLockType l(lock); c = state.voiced[((bar % 4) + 4) % 4]; }
        for (size_t i = 0; i < c.size(); ++i)
          ev.addEvent(juce::MidiMessage::noteOn(1, c[i], (juce::uint8)(i == 0 ? 85 : 66 + audioRng.nextInt(18))), 0);
        held = c; lastBar = bar;
      }
    } else if (lastBar != INT_MIN) {
      for (int note : held) ev.addEvent(juce::MidiMessage::noteOff(1, note), 0);
      held.clear(); lastBar = INT_MIN;
    }
    synth.renderNextBlock(buffer, ev, 0, n);
    midi.swapWith(ev);
  }

  void generate() {
    juce::SpinLock::ScopedLockType l(lock);
    state.generate(uiRng, key, mood); state.revoice(open != 0, uiRng);
  }
  void revoice() { juce::SpinLock::ScopedLockType l(lock); state.revoice(open != 0, uiRng); }
  void setPreview(bool on) { internalPpq = 0; lastBar = INT_MIN; preview = on; }
  juce::String getNames() { juce::SpinLock::ScopedLockType l(lock); return state.names; }

  juce::File writeMidiFile() {
    ChordGen copy; { juce::SpinLock::ScopedLockType l(lock); copy = state; }
    juce::MidiMessageSequence seq;
    for (int i = 0; i < 4; ++i)
      for (size_t j = 0; j < copy.voiced[i].size(); ++j) {
        int note = copy.voiced[i][j], vel = j == 0 ? 85 : 66 + uiRng.nextInt(18);
        seq.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), i * 1920.0);
        seq.addEvent(juce::MidiMessage::noteOff(1, note), i * 1920.0 + 1900.0);
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

  std::atomic<int> key{0}, mood{0}, open{0};
  std::atomic<bool> preview{false};

private:
  juce::Synthesiser synth;
  juce::SpinLock lock;
  ChordGen state;
  juce::Random uiRng, audioRng;
  std::vector<int> held;
  double sampleRate = 44100.0, internalPpq = 0.0;
  int lastBar = INT_MIN, fileCounter = 0;
};

struct DragBox : juce::Component {
  explicit DragBox(NSAProcessor& p) : proc(p) { setMouseCursor(juce::MouseCursor::DraggingHandCursor); }
  void paint(juce::Graphics& g) override {
    g.setColour(juce::Colour(0xff40267a)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 10.0f);
    g.setColour(juce::Colours::white); g.setFont(juce::Font(17.0f));
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

class NSAEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit NSAEditor(NSAProcessor& p) : AudioProcessorEditor(&p), proc(p), drag(p) {
    setSize(380, 250);
    keyBox.addItemList(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    moodBox.addItemList(juce::StringArray{"Warm","Dark","Dreamy","Sad","Sensual"}, 1);
    voiBox.addItemList(juce::StringArray{"Close","Open"}, 1);
    keyBox.setSelectedItemIndex(proc.key, juce::dontSendNotification);
    moodBox.setSelectedItemIndex(proc.mood, juce::dontSendNotification);
    voiBox.setSelectedItemIndex(proc.open, juce::dontSendNotification);
    keyBox.onChange = [this] { proc.key = keyBox.getSelectedItemIndex(); proc.generate(); };
    moodBox.onChange = [this] { proc.mood = moodBox.getSelectedItemIndex(); proc.generate(); };
    voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.revoice(); };
    genBtn.setButtonText("Generate"); genBtn.onClick = [this] { proc.generate(); };
    preBtn.setButtonText("Preview"); preBtn.setClickingTogglesState(true);
    preBtn.onClick = [this] { proc.setPreview(preBtn.getToggleState()); };
    label.setJustificationType(juce::Justification::centred); label.setFont(juce::Font(15.0f));
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    for (juce::Component* c : std::initializer_list<juce::Component*>{&keyBox, &moodBox, &voiBox, &genBtn, &preBtn, &label, &drag})
      addAndMakeVisible(c);
    startTimerHz(4); timerCallback();
  }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff14101f));
    g.setColour(juce::Colour(0xffb08cff)); g.setFont(juce::Font(18.0f));
    g.drawText("NSA CHORDS", 14, 6, 200, 24, juce::Justification::left);
  }
  void resized() override {
    auto r = getLocalBounds().reduced(14); r.removeFromTop(26);
    auto row = r.removeFromTop(28); int w = row.getWidth() / 3;
    keyBox.setBounds(row.removeFromLeft(w).reduced(3, 0)); moodBox.setBounds(row.removeFromLeft(w).reduced(3, 0)); voiBox.setBounds(row.reduced(3, 0));
    r.removeFromTop(8); row = r.removeFromTop(30);
    genBtn.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(3, 0)); preBtn.setBounds(row.reduced(3, 0));
    r.removeFromTop(8); label.setBounds(r.removeFromTop(40));
    r.removeFromTop(6); drag.setBounds(r);
  }
  void timerCallback() override { label.setText(proc.getNames(), juce::dontSendNotification); }
private:
  NSAProcessor& proc;
  juce::ComboBox keyBox, moodBox, voiBox; juce::TextButton genBtn, preBtn; juce::Label label; DragBox drag;
};

juce::AudioProcessorEditor* NSAProcessor::createEditor() { return new NSAEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NSAProcessor(); }
