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

class NSAProcessor : public juce::AudioProcessor {
public:
  NSAProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    synth.addSound(new PadSound());
    for (int i = 0; i < 16; ++i) synth.addVoice(new PadVoice());
    generate();
  }
  const juce::String getName() const override { return "NSA Chords"; }
  void prepareToPlay(double sr, int) override { sampleRate = sr; synth.setCurrentPlaybackSampleRate(sr); active.reserve(64); pending.reserve(64); }
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
    o.writeFloat(nost.load()); o.writeFloat(air.load()); o.writeFloat(tear.load()); o.writeFloat(human.load());
    o.writeInt(res.load()); o.writeFloat(reg.load());
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 20) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 12, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    dark = juce::jlimit(0.0f, 1.0f, in.readFloat()); tense = juce::jlimit(0.0f, 1.0f, in.readFloat());
    octaveShift = juce::jlimit(-1, 1, in.readInt()); noBass = in.readBool(); strum = in.readBool();
    if (in.getNumBytesRemaining() >= 20) {
      nost = juce::jlimit(0.0f, 1.0f, in.readFloat()); air = juce::jlimit(0.0f, 1.0f, in.readFloat());
      tear = juce::jlimit(0.0f, 1.0f, in.readFloat()); human = juce::jlimit(0.0f, 1.0f, in.readFloat());
      res = juce::jlimit(0, 2, in.readInt());
      if (in.getNumBytesRemaining() >= 4) reg = juce::jlimit(-1.0f, 1.0f, in.readFloat());
    }
    lastKey = key;
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
      auto play = [&](const Hit& h, double t, int at) {
        if (h.chord < 0 || h.chord > 3) return;
        if (!h.topOnly) cutAll(at);
        const auto& c = snap.voiced[h.chord];
        const int sz = (int) c.size();
        if (sz < 2) return;
        const int octOffset = octaveShift.load() * 12;
        const bool skipB = noBass.load();
        const bool doStrum = strum.load();
        const float hm = human.load();
        const double msToPpq = bpm / 60000.0;

        for (int i = 0; i < sz; ++i) {
          const bool isTop = (i == sz - 1);
          if (skipB && i == 0) continue;
          if (h.topOnly && !isTop) continue;
          int note = c[(size_t) i];
          double endT = t + h.len;
          if (isTop && h.sus >= 0) { note = h.sus; endT = t + h.susLen; }
          else if (i > 0 && !isTop && !h.topOnly) endT -= (double) hm * 0.1 * audioRng.nextFloat();
          note += octOffset;
          if (note < 0 || note > 127) continue;

          double ms = h.topOnly ? (double) hm * 4.0 * audioRng.nextFloat()
                                : (doStrum ? 22.0 * i : 0.0) + (double) hm * (6.0 * i + 8.0 * audioRng.nextFloat());
          const int vel = calcVel(i, isTop, note, h.chord, hm, audioRng.nextFloat(), h.topOnly);

          active.push_back(Act{note, endT});
          if (ms < 0.5) ev.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), at);
          else pending.push_back(Pend{t + ms * msToPpq, note, (juce::uint8) vel});
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
      for (size_t i = 0; i < pending.size();) {
        if (pending[i].t < p1) {
          ev.addEvent(juce::MidiMessage::noteOn(1, pending[i].note, pending[i].vel), off(pending[i].t));
          pending.erase(pending.begin() + (long) i);
        } else ++i;
      }
      release(p1);
      expectedPpq = p1; wasRunning = true;
    } else if (wasRunning) {
      for (auto& a : active) ev.addEvent(juce::MidiMessage::noteOff(1, a.note), 0);
      active.clear(); pending.clear(); wasRunning = false;
    }
    synth.renderNextBlock(buffer, ev, 0, n);
    midi.swapWith(ev);
  }

  void generate(bool newSeed = true) {
    if (newSeed) seed = uiRng.nextInt();
    juce::Random r(seed);
    juce::SpinLock::ScopedLockType l(lock);
    state.feel = currentFeel();
    state.generate(r, key, dark, tense, open != 0, getCurrentBpm(), newSeed); ++version;
  }
  void padChanged() { generate(false); }
  Feel currentFeel() const {
    Feel f; f.nost = nost.load(); f.res = res.load(); f.air = air.load(); f.tear = tear.load();
    f.human = human.load(); f.reg = reg.load();
    return f;
  }

  // Mood pad: (cx, cy) is the dot position inside a corner (680x440 view space).
  // The dot is never snapped to a shade: darkness / tension follow the dot continuously,
  // and every colour parameter is a soft blend of the neighbouring shades.
  void setMood(int corner, float cx, float cy, bool newSeed) {
    if (corner < 0 || corner > 3) return;
    const Corner& c = CORNERS[corner];
    float w[8]; moodWeights(corner, cx, cy, w);
    float nn = 0, aa = 0, tt = 0, hh = 0, rr = 0, st = 0, rw[3] = {0, 0, 0};
    for (int j = 0; j < 8; ++j) {
      const Shade& s = SHADES[corner * 8 + j];
      nn += w[j] * s.f.nost; aa += w[j] * s.f.air; tt += w[j] * s.f.tear;
      hh += w[j] * s.f.human; rr += w[j] * s.f.reg;
      rw[juce::jlimit(0, 2, s.f.res)] += w[j];
      if (s.strum) st += w[j];
    }
    if (newSeed) resDraw = uiRng.nextFloat();
    int rs = RES_HOME; float acc = 0.0f;
    for (int k = 0; k < 3; ++k) { acc += rw[k]; rs = k; if (resDraw <= acc) break; }

    const float fx = juce::jlimit(0.0f, 1.0f, (cx - 70.0f) / 520.0f);
    const float fy = juce::jlimit(0.0f, 1.0f, (375.0f - cy) / 270.0f);
    dark = c.dk0 + fx * (c.dk1 - c.dk0);
    tense = c.tn0 + fy * (c.tn1 - c.tn0);
    nost = nn; air = aa; tear = tt; human = hh; reg = rr; res = rs; strum = st >= 0.5f;
    moodDirty = true;
    generate(newSeed);
  }

  static int calcVel(int i, bool isTop, int note, int chord, float hm, float rnd, bool soft) {
    static const int arc[4] = {0, 3, 7, -3};
    int v = (i == 0) ? 76 : ((isTop ? 52 : 60) + (note % 7));
    v += (int) std::lround(arc[chord] * hm) + (int) std::lround((rnd - 0.5f) * 10.0f * hm);
    if (soft) v -= 6;
    return juce::jlimit(36, 100, v);
  }
  void evolve() {
    juce::SpinLock::ScopedLockType l(lock);
    state.feel = currentFeel();
    state.evolve(uiRng, key, tense, open != 0, getCurrentBpm()); ++version;
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
  void setPreview(bool on) { if (on) internalPpq = 0; preview = on; }
  juce::String getNames() { juce::SpinLock::ScopedLockType l(lock); return state.names; }

  juce::File writeMidiFile() {
    ChordGen copy; { juce::SpinLock::ScopedLockType l(lock); copy = state; }
    juce::MidiMessageSequence seq;
    const int octOffset = octaveShift.load() * 12;
    const bool skipB = noBass.load();
    const bool doStrum = strum.load();
    const float hm = human.load();

    for (const auto& h : copy.hits) {
      if (h.chord < 0 || h.chord > 3) continue;
      const auto& c = copy.voiced[h.chord];
      const int sz = (int) c.size();
      for (int j = 0; j < sz; ++j) {
        const bool isTop = (j == sz - 1);
        if (skipB && j == 0) continue;
        if (h.topOnly && !isTop) continue;
        int note = c[(size_t) j];
        double endBeat = h.start + h.len;
        if (isTop && h.sus >= 0) { note = h.sus; endBeat = h.start + h.susLen; }
        note += octOffset;
        if (note < 0 || note > 127) continue;
        const int tickOffset = (!h.topOnly && doStrum) ? j * 15 : 0;
        const int vel = calcVel(j, isTop, note, h.chord, hm, 0.5f, h.topOnly);
        seq.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), h.start * 480.0 + tickOffset);
        seq.addEvent(juce::MidiMessage::noteOff(1, note), endBeat * 480.0 - 10.0);
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
  std::atomic<bool> preview{false}, noBass{false}, strum{false}, moodDirty{false};
  std::atomic<float> nost{0.0f}, air{0.0f}, tear{0.0f}, human{0.5f}, reg{0.0f};
  std::atomic<int> res{RES_HOME};

private:
  struct Act { int note; double end; };
  struct Pend { double t; int note; juce::uint8 vel; };
  std::vector<Pend> pending;
  juce::Synthesiser synth;
  juce::SpinLock lock;
  ChordGen state, snap;
  std::atomic<int> version{0};
  int snapVersion = -1, seed = 1, lastKey = 0;
  float resDraw = 0.5f;
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

// ============================================================================
//  MOOD PAD: four corner moods -> click a circle, the pad zooms in -> eight shades.
//  The dot floats freely; shades blend smoothly around it.
// ============================================================================
struct MoodPad : juce::Component, private juce::Timer {
  enum Mode { MapMode, AnimMode, InnerMode };
  static constexpr float VX = 14.0f, VY = 14.0f, VW = 652.0f, VH = 412.0f, INFO_H = 58.0f;

  static int heightFor(int w) { return (int) std::lround((float) w * VH / VW + INFO_H); }

  explicit MoodPad(NSAProcessor& p) : proc(p) {
    back.setButtonText("< Back to map");
    back.onClick = [this] { leave(); };
    addChildComponent(back);
    startTimerHz(60);
  }

  void resized() override { back.setBounds(getWidth() - 116, getHeight() - (int) INFO_H + 4, 116, 24); }

  // ---- geometry --------------------------------------------------------------
  juce::AffineTransform viewT() const {
    auto r = getLocalBounds().toFloat(); r.removeFromBottom(INFO_H);
    const float k = std::min(r.getWidth() / VW, r.getHeight() / VH);
    return juce::AffineTransform::translation(-VX, -VY).scaled(k)
        .translated(r.getX() + (r.getWidth() - VW * k) * 0.5f, r.getY() + (r.getHeight() - VH * k) * 0.5f);
  }
  juce::Point<float> toView(juce::Point<float> p) const { return p.transformedBy(viewT().inverted()); }

  static float sm(float t) { t = juce::jlimit(0.0f, 1.0f, t); return t * t * (3.0f - 2.0f * t); }
  static void setF(juce::Graphics& g, float h, bool bold) { g.setFont(h); if (bold) g.setFont(g.getCurrentFont().boldened()); }
  static juce::String U(const char* s) { return juce::String::fromUTF8(s); }
  static juce::String dot() { return U(" \xc2\xb7 "); }

  // ---- state transitions -----------------------------------------------------
  void enter(int i) {
    if (mode != MapMode) return;
    active = i; mode = AnimMode; animFrom = 0.0f; animTo = 1.0f; animStart = juce::Time::getMillisecondCounterHiRes();
    cur = tg = { 340.0f, 225.0f }; sent = cur;
    proc.setMood(i, cur.x, cur.y, true);
    computeWeights();
    setMouseCursor(juce::MouseCursor::NormalCursor);
  }
  void leave() {
    if (mode != InnerMode) return;
    mode = AnimMode; animFrom = 1.0f; animTo = 0.0f; animStart = juce::Time::getMillisecondCounterHiRes();
    back.setVisible(false);
  }

  void timerCallback() override {
    if (mode == AnimMode) {
      const float k = juce::jlimit(0.0f, 1.0f, (float) ((juce::Time::getMillisecondCounterHiRes() - animStart) / 700.0));
      t = animFrom + (animTo - animFrom) * sm(k);
      if (k >= 1.0f) {
        t = animTo;
        if (animTo > 0.5f) { mode = InnerMode; back.setVisible(true); }
        else { mode = MapMode; active = -1; }
      }
      repaint();
    } else if (mode == InnerMode) {
      const float dx = tg.x - cur.x, dy = tg.y - cur.y;
      if (std::abs(dx) + std::abs(dy) > 0.05f) {
        cur.x += dx * 0.22f; cur.y += dy * 0.22f;
        computeWeights();
        if (std::hypot(cur.x - sent.x, cur.y - sent.y) > 1.5f) { sent = cur; proc.setMood(active, cur.x, cur.y, false); }
        repaint();
      }
    }
  }

  // ---- input -----------------------------------------------------------------
  int cornerAt(juce::Point<float> v) const {
    for (int i = 0; i < 4; ++i)
      if (std::hypot(v.x - CORNERS[i].cx, v.y - CORNERS[i].cy) <= 54.0f) return i;
    return -1;
  }
  void setTarget(juce::Point<float> v) { tg = { juce::jlimit(32.0f, 648.0f, v.x), juce::jlimit(32.0f, 408.0f, v.y) }; }
  void mouseMove(const juce::MouseEvent& e) override {
    setMouseCursor(mode == MapMode && cornerAt(toView(e.position)) >= 0 ? juce::MouseCursor::PointingHandCursor
                                                                         : juce::MouseCursor::NormalCursor);
  }
  void mouseDown(const juce::MouseEvent& e) override {
    const auto v = toView(e.position);
    if (mode == MapMode) { const int i = cornerAt(v); if (i >= 0) enter(i); }
    else if (mode == InnerMode) setTarget(v);
  }
  void mouseDrag(const juce::MouseEvent& e) override { if (mode == InnerMode) setTarget(toView(e.position)); }

  void computeWeights() {
    if (active < 0) return;
    moodWeights(active, cur.x, cur.y, w);
    nearest = 0; second = -1;
    for (int j = 1; j < 8; ++j) if (w[j] > w[nearest]) nearest = j;
    for (int j = 0; j < 8; ++j) if (j != nearest && (second < 0 || w[j] > w[second])) second = j;
  }

  // ---- painting --------------------------------------------------------------
  void paint(juce::Graphics& g) override {
    {
      juce::Graphics::ScopedSaveState sss(g);
      g.addTransform(viewT());
      juce::Path frame; frame.addRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f);
      g.setColour(juce::Colour(0xff1a1530)); g.fillPath(frame);
      g.reduceClipRegion(frame);

      if (mode == MapMode) drawMap(g, 1.0f);
      else if (mode == InnerMode) drawInner(g, 1.0f);
      else {
        const Corner& c = CORNERS[active];
        const float mapA = 1.0f - sm(t / 0.7f), inA = sm((t - 0.3f) / 0.7f);
        if (mapA > 0.001f) {
          juce::Graphics::ScopedSaveState s2(g);
          const float s = 1.0f + 5.0f * t;
          g.addTransform(juce::AffineTransform::scale(s, s, c.cx, c.cy));
          drawMap(g, mapA);
        }
        if (inA > 0.001f) {
          juce::Graphics::ScopedSaveState s3(g);
          const float s = 0.2f + 0.8f * t;
          g.addTransform(juce::AffineTransform::scale(s, s, c.cx, c.cy));
          drawInner(g, inA);
        }
      }
    }
    {
      juce::Graphics::ScopedSaveState sss(g);
      g.addTransform(viewT());
      g.setColour(juce::Colours::white.withAlpha(0.14f));
      g.drawRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f, 1.0f);
    }
    paintInfo(g);
  }

  void drawMap(juce::Graphics& g, float alpha) {
    juce::Graphics::ScopedSaveState sss(g);
    if (alpha < 0.999f) g.beginTransparencyLayer(alpha);

    setF(g, 14.5f, false); g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("warm", juce::Rectangle<float>(34.0f, 215.0f, 120.0f, 18.0f), juce::Justification::left);
    g.drawText("cold", juce::Rectangle<float>(526.0f, 215.0f, 120.0f, 18.0f), juce::Justification::right);
    g.drawText("tap a circle", juce::Rectangle<float>(240.0f, 215.0f, 200.0f, 18.0f), juce::Justification::centred);

    for (int i = 0; i < 4; ++i) {
      const Corner& c = CORNERS[i]; const juce::Colour col(c.col);
      g.setColour(col.withAlpha(0.20f)); g.fillEllipse(c.cx - 54.0f, c.cy - 54.0f, 108.0f, 108.0f);
      g.setColour(col.withAlpha(0.95f)); g.drawEllipse(c.cx - 54.0f, c.cy - 54.0f, 108.0f, 108.0f, 1.3f);
      for (int j = 0; j < 8; ++j) {
        const Shade& s = SHADES[i * 8 + j];
        const float px = c.cx + (s.x - 340.0f) / 270.0f * 34.0f, py = c.cy + (s.y - 225.0f) / 135.0f * 30.0f;
        g.setColour(col.withAlpha(0.8f)); g.fillEllipse(px - 2.6f, py - 2.6f, 5.2f, 5.2f);
      }
      const bool top = c.cy < 220.0f;
      const float ty = top ? c.cy - 54.0f - 26.0f : c.cy + 54.0f + 22.0f;
      setF(g, 16.5f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
      g.drawText(juce::String(c.name) + dot() + c.tag, juce::Rectangle<float>(c.cx - 160.0f, ty - 14.0f, 320.0f, 20.0f), juce::Justification::centred);
      setF(g, 14.0f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
      g.drawText(c.desc, juce::Rectangle<float>(c.cx - 160.0f, ty + 3.0f, 320.0f, 18.0f), juce::Justification::centred);
    }
    if (alpha < 0.999f) g.endTransparencyLayer();
  }

  void drawInner(juce::Graphics& g, float alpha) {
    if (active < 0) return;
    juce::Graphics::ScopedSaveState sss(g);
    if (alpha < 0.999f) g.beginTransparencyLayer(alpha);
    const Corner& c = CORNERS[active]; const juce::Colour col(c.col);

    g.setColour(juce::Colour(0xff1a1530)); g.fillRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f);
    g.setColour(col.withAlpha(0.9f)); g.drawRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f, 1.3f);

    setF(g, 17.0f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawText(juce::String(c.name) + dot() + c.tag, juce::Rectangle<float>(36.0f, 32.0f, 420.0f, 20.0f), juce::Justification::left);
    setF(g, 14.0f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(c.desc, juce::Rectangle<float>(36.0f, 53.0f, 420.0f, 18.0f), juce::Justification::left);

    const juce::String aL = U("\xe2\x86\x90"), aR = U("\xe2\x86\x92"), aU = U("\xe2\x86\x91"), aD = U("\xe2\x86\x93");
    g.drawText(aL + " " + c.axL, juce::Rectangle<float>(34.0f, 398.0f, 260.0f, 18.0f), juce::Justification::left);
    g.drawText(juce::String(c.axR) + " " + aR, juce::Rectangle<float>(386.0f, 398.0f, 260.0f, 18.0f), juce::Justification::right);
    g.drawText(juce::String(c.axU) + " " + aU, juce::Rectangle<float>(386.0f, 31.0f, 260.0f, 18.0f), juce::Justification::right);
    g.drawText(juce::String(c.axD) + " " + aD, juce::Rectangle<float>(386.0f, 380.0f, 260.0f, 18.0f), juce::Justification::right);

    const Shade& ns = SHADES[active * 8 + nearest];
    const float dashes[2] = {3.0f, 3.0f};
    g.setColour(col.withAlpha(0.85f));
    g.drawDashedLine(juce::Line<float>(cur.x, cur.y, ns.x, ns.y), dashes, 2, 1.2f);

    for (int j = 0; j < 8; ++j) {
      const Shade& s = SHADES[active * 8 + j];
      const float dd = std::hypot(cur.x - s.x, cur.y - s.y);
      const float r = 8.0f + 7.0f * sm((130.0f - dd) / 110.0f);
      g.setColour(col.withAlpha(0.28f)); g.fillEllipse(s.x - r, s.y - r, 2 * r, 2 * r);
      g.setColour(col); g.drawEllipse(s.x - r, s.y - r, 2 * r, 2 * r, 1.2f);
      setF(g, 14.0f, j == nearest);
      g.setColour(juce::Colours::white.withAlpha(j == nearest ? 1.0f : 0.7f));
      g.drawText(s.name, juce::Rectangle<float>(s.x - 90.0f, s.y + 16.0f, 180.0f, 18.0f), juce::Justification::centred);
    }
    g.setColour(juce::Colours::white); g.fillEllipse(cur.x - 6.0f, cur.y - 6.0f, 12.0f, 12.0f);
    g.setColour(juce::Colour(0xff1a1530)); g.drawEllipse(cur.x - 6.0f, cur.y - 6.0f, 12.0f, 12.0f, 2.0f);
    if (alpha < 0.999f) g.endTransparencyLayer();
  }

  juce::StringArray chips() const {
    juce::StringArray c;
    if (proc.nost.load() >= 0.35f) c.add("borrowed chord");
    const int r = proc.res.load();
    c.add(r == RES_HOME ? "ends home" : (r == RES_HANG ? "ends hanging" : "ends twisted"));
    if (proc.air.load() >= 0.3f) c.add("anticipation");
    if (proc.tear.load() >= 0.4f) c.add("suspension");
    if (proc.reg.load() >= 0.35f) c.add("high topline");
    else if (proc.reg.load() <= -0.3f) c.add("low register");
    if (proc.strum.load()) c.add("soft roll");
    return c;
  }

  void paintInfo(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    auto info = b.removeFromBottom(INFO_H).reduced(2.0f, 0.0f);

    juce::String name, axes;
    if (active < 0) {
      name = "Pick a corner";
      axes = "Each corner is a core mood. Inside, it opens into shades.";
    } else {
      const Corner& c = CORNERS[active];
      name = juce::String(c.name);
      if (mode == InnerMode) {
        name += " > " + juce::String(SHADES[active * 8 + nearest].name);
        if (second >= 0 && w[second] >= 0.35f * w[nearest]) name += "  +  " + juce::String(SHADES[active * 8 + second].name);
        const int px = (int) std::lround(juce::jlimit(0.0f, 1.0f, (cur.x - 70.0f) / 520.0f) * 100.0f);
        const int py = (int) std::lround(juce::jlimit(0.0f, 1.0f, (375.0f - cur.y) / 270.0f) * 100.0f);
        const juce::String lr = U("\xe2\x86\x94");
        axes = juce::String(c.axL) + " " + lr + " " + c.axR + ": " + juce::String(px) + "%" + dot()
             + c.axD + " " + lr + " " + c.axU + ": " + juce::String(py) + "%";
      } else axes = "Zooming in...";
    }
    const float textW = info.getWidth() - 120.0f;
    setF(g, 15.5f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawText(name, juce::Rectangle<float>(info.getX(), info.getY() + 4.0f, textW, 19.0f), juce::Justification::left);
    setF(g, 12.5f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(axes, juce::Rectangle<float>(info.getX(), info.getY() + 24.0f, info.getWidth(), 16.0f), juce::Justification::left);

    if (active >= 0 && mode == InnerMode) {
      setF(g, 11.0f, false);
      float x = info.getX();
      for (const auto& s : chips()) {
        const float cw = (float) s.length() * 6.0f + 18.0f;
        if (x + cw > info.getRight()) break;
        const auto pill = juce::Rectangle<float>(x, info.getY() + 41.0f, cw, 17.0f);
        g.setColour(juce::Colours::white.withAlpha(0.07f)); g.fillRoundedRectangle(pill, 8.5f);
        g.setColour(juce::Colours::white.withAlpha(0.22f)); g.drawRoundedRectangle(pill, 8.5f, 0.8f);
        g.setColour(juce::Colours::white.withAlpha(0.75f)); g.drawText(s, pill, juce::Justification::centred);
        x += cw + 6.0f;
      }
    }
  }

private:
  NSAProcessor& proc;
  juce::TextButton back;
  Mode mode = MapMode;
  int active = -1, nearest = 0, second = -1;
  float t = 0.0f, animFrom = 0.0f, animTo = 0.0f, w[8] = {};
  double animStart = 0.0;
  juce::Point<float> cur{340.0f, 225.0f}, tg{340.0f, 225.0f}, sent{340.0f, 225.0f};
};

class NSAEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit NSAEditor(NSAProcessor& p) : AudioProcessorEditor(&p), proc(p), pad(p), drag(p) {
    setSize(520, 672);
    keyBox.addItemList(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B","Free (Random Key)"}, 1);
    voiBox.addItemList(juce::StringArray{"Close Voicing","Open Voicing"}, 1);
    octBox.addItemList(juce::StringArray{"Oct: -1","Oct: 0","Oct: +1"}, 1);

    keyBox.setSelectedItemIndex(proc.key, juce::dontSendNotification);
    voiBox.setSelectedItemIndex(proc.open, juce::dontSendNotification);
    octBox.setSelectedItemIndex(proc.octaveShift + 1, juce::dontSendNotification);

    keyBox.onChange = [this] { proc.setKey(keyBox.getSelectedItemIndex()); };
    voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.revoice(); };
    octBox.onChange = [this] { proc.octaveShift = octBox.getSelectedItemIndex() - 1; };

    resBox.addItemList(juce::StringArray{"End: Home","End: Hang","End: Twist"}, 1);
    resBox.setSelectedItemIndex(proc.res.load(), juce::dontSendNotification);
    resBox.onChange = [this] { proc.res = resBox.getSelectedItemIndex(); proc.padChanged(); };
    initCtl(cNost, "Nostalgia", proc.nost.load(), [this](float v) { proc.nost = v; proc.padChanged(); });
    initCtl(cAir, "Air", proc.air.load(), [this](float v) { proc.air = v; proc.padChanged(); });
    initCtl(cTear, "Tear", proc.tear.load(), [this](float v) { proc.tear = v; proc.padChanged(); });
    initCtl(cHuman, "Human", proc.human.load(), [this](float v) { proc.human = v; });

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

    for (juce::Component* c : std::initializer_list<juce::Component*>{&keyBox, &voiBox, &octBox, &resBox, &pad, &genBtn, &contBtn, &preBtn, &nobassBtn, &strumBtn, &label, &drag})
      addAndMakeVisible(c);
    startTimerHz(4); timerCallback();
  }
  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff14101f));
    g.setColour(juce::Colour(0xffb08cff)); g.setFont(18.0f);
    g.drawText("NSA CHORDS // Mood Pad", 14, 8, 360, 24, juce::Justification::left);
  }
  void resized() override {
    auto r = getLocalBounds().reduced(14); r.removeFromTop(28);
    auto row = r.removeFromTop(28); int w3 = row.getWidth() / 3;
    keyBox.setBounds(row.removeFromLeft(w3).reduced(2, 0));
    voiBox.setBounds(row.removeFromLeft(w3).reduced(2, 0));
    octBox.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); pad.setBounds(r.removeFromTop(MoodPad::heightFor(r.getWidth())));

    r.removeFromTop(6); row = r.removeFromTop(26); int wt = row.getWidth() / 3;
    nobassBtn.setBounds(row.removeFromLeft(wt).reduced(2, 0));
    strumBtn.setBounds(row.removeFromLeft(wt).reduced(2, 0));
    resBox.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); row = r.removeFromTop(22); placeCtl(cNost, row.removeFromLeft(row.getWidth() / 2)); placeCtl(cAir, row);
    r.removeFromTop(4); row = r.removeFromTop(22); placeCtl(cTear, row.removeFromLeft(row.getWidth() / 2)); placeCtl(cHuman, row);

    r.removeFromTop(6); row = r.removeFromTop(30); int bw = row.getWidth() / 3;
    genBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0));
    contBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0));
    preBtn.setBounds(row.reduced(2, 0));

    r.removeFromTop(6); label.setBounds(r.removeFromTop(32));
    r.removeFromTop(6); drag.setBounds(r);
  }
  struct Ctl { juce::Label name; juce::Slider s; };
  void initCtl(Ctl& c, const char* nm, float v, std::function<void(float)> cb) {
    c.name.setText(nm, juce::dontSendNotification); c.name.setFont(12.0f);
    c.name.setColour(juce::Label::textColourId, juce::Colour(0xffb08cff));
    c.s.setSliderStyle(juce::Slider::LinearHorizontal);
    c.s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    c.s.setRange(0.0, 100.0, 1.0); c.s.setTextValueSuffix("%");
    c.s.setValue((double) v * 100.0, juce::dontSendNotification);
    c.s.onValueChange = [&c, cb] { cb((float) (c.s.getValue() / 100.0)); };
    addAndMakeVisible(c.name); addAndMakeVisible(c.s);
  }
  static void placeCtl(Ctl& c, juce::Rectangle<int> a) { a.reduce(2, 0); c.name.setBounds(a.removeFromLeft(58)); c.s.setBounds(a); }
  void syncFromProc() {
    cNost.s.setValue(proc.nost.load() * 100.0, juce::dontSendNotification);
    cAir.s.setValue(proc.air.load() * 100.0, juce::dontSendNotification);
    cTear.s.setValue(proc.tear.load() * 100.0, juce::dontSendNotification);
    cHuman.s.setValue(proc.human.load() * 100.0, juce::dontSendNotification);
    resBox.setSelectedItemIndex(proc.res.load(), juce::dontSendNotification);
    strumBtn.setToggleState(proc.strum.load(), juce::dontSendNotification);
  }
  void timerCallback() override {
    label.setText(proc.getNames(), juce::dontSendNotification);
    if (proc.moodDirty.exchange(false)) syncFromProc();
    pad.repaint();
  }
private:
  NSAProcessor& proc;
  juce::ComboBox keyBox, voiBox, octBox, resBox;
  Ctl cNost, cAir, cTear, cHuman;
  juce::TextButton genBtn, contBtn, preBtn, nobassBtn, strumBtn;
  juce::Label label; MoodPad pad; DragBox drag;
};

juce::AudioProcessorEditor* NSAProcessor::createEditor() { return new NSAEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NSAProcessor(); }
