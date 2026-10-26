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
    env.setParameters(juce::ADSR::Parameters{0.25f, 0.7f, 0.85f, 1.4f}); // Еще более мягкая, обволакивающая атака и релиз
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

// ===== МЕТАДАННЫЕ ЧУВСТВ: каждое значение сдвигает d=dark t=tense n=nostalgia a=air e=tear r=resolve h=human =====
struct Meta { float d, t, n, a, e, r, h; };
inline const Meta M_SEASON[4]  = { {-.10f,0,.10f,.10f,0,.10f,0}, {-.25f,-.05f,.25f,.05f,.05f,.10f,.10f}, {.05f,.05f,.30f,.10f,.20f,-.10f,0}, {.25f,.10f,.05f,.25f,.15f,-.10f,-.10f} };
inline const Meta M_TIME[4]    = { {-.15f,0,.20f,.05f,.10f,.10f,.10f}, {.15f,.10f,0,.10f,.10f,0,0}, {.25f,.10f,.05f,.25f,.20f,-.20f,-.05f}, {-.05f,-.05f,.10f,.30f,.10f,.20f,0} };
inline const Meta M_WEATHER[4] = { {0,0,0,0,0,0,0}, {.10f,.05f,.15f,.10f,.20f,-.05f,.05f}, {.05f,.10f,.05f,.30f,.10f,-.15f,-.05f}, {.10f,-.05f,.10f,.25f,.05f,.10f,-.10f} };
inline const Meta M_CTX[5]     = { {-.15f,.05f,.05f,0,.05f,.20f,.20f}, {.10f,0,.15f,.15f,.15f,-.10f,-.10f}, {.15f,.20f,-.10f,-.15f,.05f,0,.10f}, {.05f,0,.10f,0,.05f,.10f,.05f}, {.20f,.25f,.20f,.10f,.35f,-.30f,-.05f} };
struct SceneDef { const char* name; int se, ti, we, cx; };
inline const SceneDef SCENES[] = {
  {"Warm summer evening, a date", 1,0,0,0}, {"Driving home at dawn", 0,3,2,3},
  {"First rain after the breakup", 2,1,1,4}, {"4 AM, club afterglow", 2,2,0,2},
  {"Snowed in, thinking of her", 3,0,3,1}, {"Toxic night, city lights", 3,1,1,2},
  {"Last night of summer, alone", 1,1,0,1}, {"Foggy morning, still awake", 3,3,2,1} };
inline const int NUM_SCENES = (int) (sizeof(SCENES) / sizeof(SCENES[0]));

class NSAProcessor : public juce::AudioProcessor {
public:
  NSAProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    synth.addSound(new PadSound());
    for (int i = 0; i < 16; ++i) synth.addVoice(new PadVoice());
    applyMeta(); generate();
  }
  const juce::String getName() const override { return "NSA Chords"; }
  void prepareToPlay(double sr, int) override { sampleRate = sr; synth.setCurrentPlaybackSampleRate(sr); }
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
    o.writeInt(season); o.writeInt(timeOfDay); o.writeInt(weather); o.writeInt(context); o.writeInt(scene);
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 20) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 12, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    dark = juce::jlimit(0.0f, 1.0f, in.readFloat()); tense = juce::jlimit(0.0f, 1.0f, in.readFloat());
    octaveShift = juce::jlimit(-1, 1, in.readInt()); noBass = in.readBool(); strum = in.readBool();
    lastKey = key;
    if (in.getNumBytesRemaining() >= 20) {
      season = juce::jlimit(0, 3, in.readInt()); timeOfDay = juce::jlimit(0, 3, in.readInt());
      weather = juce::jlimit(0, 3, in.readInt()); context = juce::jlimit(0, 4, in.readInt());
      scene = juce::jlimit(0, NUM_SCENES, in.readInt());
    }
    applyMeta(false);
    generate();
  }

  double getCurrentBpm() const {
    if (auto* ph = const_cast<NSAProcessor*>(this)->getPlayHead())
      if (auto pos = ph->getPosition())
        if (auto b = pos->getBpm())
          if (*b >= 20.0 && *b <= 300.0)
            return *b;
    return 120.0;
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
    else if (preview.load()) { ppq = internalPpq; bpm = getCurrentBpm(); internalPpq += (double) n / sampleRate * (bpm / 60.0); run = true; }

    if (version.load() != snapVersion) {
      juce::SpinLock::ScopedTryLockType tl(lock);
      if (tl.isLocked()) { snap = state; snapVersion = version.load(); }
    }

    juce::MidiBuffer ev;
    if (run) {
      const double pps = bpm / 60.0 / sampleRate;
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
        active.clear(); pending.clear();
      };
      // отложенные ноты (живой строум / микро-тайминг), попавшие в этот блок
      for (size_t i = 0; i < pending.size();) {
        const juce::int64 rel = pending[i].at - clock;
        if (rel < n) {
          ev.addEvent(juce::MidiMessage::noteOn(1, pending[i].note, (juce::uint8) pending[i].vel), (int) juce::jmax<juce::int64>(0, rel));
          active.push_back(Act{pending[i].note, pending[i].end});
          pending.erase(pending.begin() + (long) i);
        } else ++i;
      }
      auto play = [&](const Hit& h, double t, int at) {
        cutAll(at);
        if (h.chord < 0 || h.chord > 3) return;
        const auto& c = snap.voiced[h.chord];
        const int octOffset = octaveShift.load() * 12;
        const bool skipB = noBass.load();
        const bool doStrum = strum.load();
        const float hu = human.load();
        const size_t top = c.size() - 1;
        for (size_t i = 0; i < c.size(); ++i) {
          if (skipB && i == 0) continue;
          const int note = c[i] + octOffset;
          if (note < 0 || note > 127) continue;
          // HUMAN: настоящий микро-тайминг в миллисекундах + разброс velocity, верхний голос чуть громче
          const double ms = (doStrum ? 16.0 * (double) i : 0.0) + (double) hu * 16.0 * audioRng.nextDouble();
          const int at2 = at + (int) (ms * 0.001 * sampleRate);
          int vel = (i == 0 ? 76 : 60 + (i == top ? 8 : 0)) + (int) ((audioRng.nextFloat() - 0.5f) * 22.0f * hu);
          vel = juce::jlimit(40, 100, vel);
          const double endP = t + h.len;
          if (at2 < n) { ev.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), at2); active.push_back(Act{note, endP}); }
          else pending.push_back(Pend{clock + at2, note, vel, endP});
        }
      };

      const bool jump = !wasRunning || std::abs(p0 - expectedPpq) > 0.25;
      const double base = std::floor(p0 / 16.0) * 16.0;
      if (jump) {
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
      active.clear(); pending.clear(); wasRunning = false;
    }
    clock += n;
    synth.renderNextBlock(buffer, ev, 0, n);
    midi.swapWith(ev);
  }

  void generate(bool newSeed = true) {
    if (newSeed) seed = uiRng.nextInt();
    juce::Random r(seed);
    juce::SpinLock::ScopedLockType l(lock);
    state.generate(r, key, dark, tense, open != 0, getCurrentBpm(), newSeed, mood()); ++version;
  }
  void padChanged() { generate(false); }
  void evolve() {
    juce::SpinLock::ScopedLockType l(lock);
    state.evolve(uiRng, key, tense, open != 0, getCurrentBpm(), mood()); ++version;
  }
  void setKey(int nk) {
    lastKey = key; key = nk;
    if (nk == 12) {
      generate(true);
    } else {
      const int d = (((nk - lastKey) % 12) + 12) % 12;
      juce::SpinLock::ScopedLockType l(lock);
      state.transpose(d, open != 0, uiRng); ++version;
    }
  }
  void revoice() { juce::SpinLock::ScopedLockType l(lock); state.revoice(open != 0, uiRng); ++version; }
  Mood mood() const { return Mood{nost.load(), air.load(), tear.load(), res.load()}; }
  // метаданные -> позиция на паде + параметры движка
  void applyMeta(bool setPad = true) {
    float d = .5f, t = .4f, n = .3f, a = .4f, e = .3f, r = .5f, h = .4f;
    const Meta* ms[4] = { &M_SEASON[season.load()], &M_TIME[timeOfDay.load()], &M_WEATHER[weather.load()], &M_CTX[context.load()] };
    for (const Meta* m : ms) { d += m->d; t += m->t; n += m->n; a += m->a; e += m->e; r += m->r; h += m->h; }
    auto c = [](float v) { return juce::jlimit(0.0f, 1.0f, v); };
    if (setPad) { dark = c(d); tense = c(t); }
    nost = c(n); air = c(a); tear = c(e); res = c(r); human = c(h);
  }
  void setMeta(int which, int idx) {
    switch (which) { case 0: season = idx; break; case 1: timeOfDay = idx; break; case 2: weather = idx; break; default: context = idx; break; }
    scene = 0; applyMeta(); generate(true);
  }
  void setScene(int s) {
    scene = s;
    if (s < 1 || s > NUM_SCENES) return;
    const auto& d = SCENES[s - 1];
    season = d.se; timeOfDay = d.ti; weather = d.we; context = d.cx;
    applyMeta(); generate(true);
  }
  void setPreview(bool on) { if (on) internalPpq = 0; preview = on; }
  juce::String getNames() { juce::SpinLock::ScopedLockType l(lock); return state.names; }

  juce::File writeMidiFile() {
    ChordGen copy; { juce::SpinLock::ScopedLockType l(lock); copy = state; }
    juce::MidiMessageSequence seq;
    const int octOffset = octaveShift.load() * 12;
    const bool skipB = noBass.load();
    const bool doStrum = strum.load();
    const float hu = human.load(); juce::Random rr;

    for (const auto& h : copy.hits) {
      if (h.chord < 0 || h.chord > 3) continue;
      const double on = h.start * 480.0, offT = (h.start + h.len) * 480.0 - 10.0;
      const auto& c = copy.voiced[h.chord];
      for (size_t j = 0; j < c.size(); ++j) {
        if (skipB && j == 0) continue;
        int note = c[j] + octOffset;
        if (note < 0 || note > 127) continue;
        int tickOffset = (doStrum ? (int)(j * 15) : 0) + (int)(hu * 12.0f * rr.nextFloat());
        int vel = juce::jlimit(40, 100, (j == 0 ? 76 : 60 + (j + 1 == c.size() ? 8 : 0)) + (int)((rr.nextFloat() - 0.5f) * 22.0f * hu));
        seq.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), on + tickOffset);
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

  std::atomic<int> key{0}, open{0}, octaveShift{0};
  std::atomic<float> dark{0.6f}, tense{0.4f};
  std::atomic<bool> preview{false}, noBass{false}, strum{false};
  std::atomic<int> season{1}, timeOfDay{0}, weather{0}, context{0}, scene{1};
  std::atomic<float> nost{0.3f}, air{0.4f}, tear{0.3f}, res{0.5f}, human{0.4f};

private:
  struct Act { int note; double end; };
  struct Pend { juce::int64 at; int note, vel; double end; };
  std::vector<Pend> pending; juce::int64 clock = 0;
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
    g.setColour(juce::Colours::white); g.setFont(16.0f);
    g.drawText("DRAG MIDI  >>  drop on DAW track", getLocalBounds(), juce::Justification::centred);
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

struct PadBox : juce::Component {
  explicit PadBox(NSAProcessor& p) : proc(p) {}
  void paint(juce::Graphics& g) override {
    auto b = getLocalBounds().toFloat().reduced(1);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8a7be0), b.getX(), 0.0f, juce::Colour(0xff0b0818), b.getRight(), 0.0f, false));
    g.fillRoundedRectangle(b, 10.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0x66ff4d6d), 0.0f, b.getY(), juce::Colour(0x00ff4d6d), 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle(b, 10.0f);
    
    // Подписываем 4 обновленных интимных/ночных квадранта R&B вайба
    g.setColour(juce::Colours::white.withAlpha(0.85f)); g.setFont(11.0f);
    g.drawText("Candlelight & Dates", 8, 6, 120, 16, juce::Justification::left);
    g.drawText("Ethereal Drama", 8, getHeight() - 22, 120, 16, juce::Justification::left);
    g.drawText("Toxic TrapSoul", getWidth() - 120, 6, 112, 16, juce::Justification::right);
    g.drawText("4 AM Intimacy", getWidth() - 120, getHeight() - 22, 112, 16, juce::Justification::right);

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
    setSize(400, 510);
    keyBox.addItemList(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B","Free (Random Key)"}, 1);
    voiBox.addItemList(juce::StringArray{"Close Voicing","Open Voicing"}, 1);
    octBox.addItemList(juce::StringArray{"Oct: -1","Oct: 0","Oct: +1"}, 1);
    
    keyBox.setSelectedItemIndex(proc.key, juce::dontSendNotification);
    voiBox.setSelectedItemIndex(proc.open, juce::dontSendNotification);
    octBox.setSelectedItemIndex(proc.octaveShift + 1, juce::dontSendNotification);

    keyBox.onChange = [this] { proc.setKey(keyBox.getSelectedItemIndex()); };
    voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.revoice(); };
    octBox.onChange = [this] { proc.octaveShift = octBox.getSelectedItemIndex() - 1; };

    sceneBox.addItem("Custom (meta)", 1);
    for (int i = 0; i < NUM_SCENES; ++i) sceneBox.addItem(SCENES[i].name, i + 2);
    seaBox.addItemList(juce::StringArray{"Spring","Summer","Autumn","Winter"}, 1);
    timBox.addItemList(juce::StringArray{"Evening","Night","4 AM","Dawn"}, 1);
    weaBox.addItemList(juce::StringArray{"Clear","Rain","Fog","Snow"}, 1);
    ctxBox.addItemList(juce::StringArray{"Date","Alone","Club","Drive","Breakup"}, 1);
    syncMeta();
    sceneBox.onChange = [this] { const int i = sceneBox.getSelectedItemIndex(); if (i > 0) { proc.setScene(i); syncMeta(); pad.repaint(); } else proc.scene = 0; };
    seaBox.onChange = [this] { metaChanged(0, seaBox); };
    timBox.onChange = [this] { metaChanged(1, timBox); };
    weaBox.onChange = [this] { metaChanged(2, weaBox); };
    ctxBox.onChange = [this] { metaChanged(3, ctxBox); };

    genBtn.setButtonText("Generate Vibe"); genBtn.onClick = [this] { proc.generate(); };
    contBtn.setButtonText("Evolve Loop"); contBtn.onClick = [this] { proc.evolve(); };
    
    preBtn.setClickingTogglesState(true);
    preBtn.setToggleState(proc.preview.load(), juce::dontSendNotification);
    preBtn.setButtonText(proc.preview.load() ? "Stop" : "Preview");
    preBtn.onClick = [this] {
      bool on = preBtn.getToggleState();
      proc.setPreview(on);
      preBtn.setButtonText(on ? "Stop" : "Preview");
    };

    nobassBtn.setButtonText("No Bass (808 Safe)");
    nobassBtn.setClickingTogglesState(true);
    nobassBtn.setToggleState(proc.noBass.load(), juce::dontSendNotification);
    nobassBtn.onClick = [this] { proc.noBass = nobassBtn.getToggleState(); };

    strumBtn.setButtonText("Strum");
    strumBtn.setClickingTogglesState(true);
    strumBtn.setToggleState(proc.strum.load(), juce::dontSendNotification);
    strumBtn.onClick = [this] { proc.strum = strumBtn.getToggleState(); };

    label.setJustificationType(juce::Justification::centred); label.setFont(14.0f);
    label.setColour(juce::Label::textColourId, juce::Colours::white);

    for (juce::Component* c : std::initializer_list<juce::Component*>{&sceneBox, &seaBox, &timBox, &weaBox, &ctxBox, &keyBox, &voiBox, &octBox, &pad, &genBtn, &contBtn, &preBtn, &nobassBtn, &strumBtn, &label, &drag})
      addAndMakeVisible(c);
    startTimerHz(4); timerCallback();
  }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff14101f));
    g.setColour(juce::Colour(0xffb08cff)); g.setFont(18.0f);
    g.drawText("NSA CHORDS // TrapSoul & R&B", 14, 8, 300, 24, juce::Justification::left);
  }
  void resized() override {
    auto r = getLocalBounds().reduced(14); r.removeFromTop(28);
    auto row = r.removeFromTop(28); int w3 = row.getWidth() / 3;
    keyBox.setBounds(row.removeFromLeft(w3).reduced(2, 0)); 
    voiBox.setBounds(row.removeFromLeft(w3).reduced(2, 0)); 
    octBox.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); sceneBox.setBounds(r.removeFromTop(28).reduced(2, 0));
    r.removeFromTop(6); row = r.removeFromTop(28); const int w4 = row.getWidth() / 4;
    seaBox.setBounds(row.removeFromLeft(w4).reduced(2, 0)); timBox.setBounds(row.removeFromLeft(w4).reduced(2, 0));
    weaBox.setBounds(row.removeFromLeft(w4).reduced(2, 0)); ctxBox.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); pad.setBounds(r.removeFromTop(140).reduced(2, 0));
    
    r.removeFromTop(6); auto rowToggles = r.removeFromTop(26); int wt = rowToggles.getWidth() / 2;
    nobassBtn.setBounds(rowToggles.removeFromLeft(wt).reduced(2, 0));
    strumBtn.setBounds(rowToggles.reduced(2, 0));

    r.removeFromTop(6); row = r.removeFromTop(30); int bw = row.getWidth() / 3;
    genBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0)); 
    contBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0)); 
    preBtn.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); label.setBounds(r.removeFromTop(36));
    r.removeFromTop(6); drag.setBounds(r);
  }
  void timerCallback() override { label.setText(proc.getNames(), juce::dontSendNotification); pad.repaint(); }
private:
  void syncMeta() {
    seaBox.setSelectedItemIndex(proc.season, juce::dontSendNotification);
    timBox.setSelectedItemIndex(proc.timeOfDay, juce::dontSendNotification);
    weaBox.setSelectedItemIndex(proc.weather, juce::dontSendNotification);
    ctxBox.setSelectedItemIndex(proc.context, juce::dontSendNotification);
    sceneBox.setSelectedItemIndex(proc.scene, juce::dontSendNotification);
  }
  void metaChanged(int which, juce::ComboBox& b) {
    proc.setMeta(which, b.getSelectedItemIndex());
    sceneBox.setSelectedItemIndex(0, juce::dontSendNotification); pad.repaint();
  }
  NSAProcessor& proc;
  juce::ComboBox keyBox, voiBox, octBox, sceneBox, seaBox, timBox, weaBox, ctxBox;
  juce::TextButton genBtn, contBtn, preBtn, nobassBtn, strumBtn; 
  juce::Label label; PadBox pad; DragBox drag;
};

juce::AudioProcessorEditor* NSAProcessor::createEditor() { return new NSAEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NSAProcessor(); }
