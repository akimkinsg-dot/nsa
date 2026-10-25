#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

// ============================================================================
//  CHORD TYPES
// ============================================================================
enum { MAJ7, MAJ9, M7, M9, M11, D9, D13, MAJ7S11, D7B9, SUS9, MMAJ7, SIX9, ADD9, M7B5,
       D7S11, M69, MAJ7SUS2 };
inline const std::vector<int> IV[] = {
  {0,4,7,11},{0,4,7,11,14},{0,3,7,10},{0,3,7,10,14},{0,3,7,10,14,17},
  {0,4,7,10,14},{0,4,10,14,21},{0,4,7,11,18},{0,4,7,10,13},{0,5,7,10,14},
  {0,3,7,11},{0,4,7,9,14},{0,4,7,14},{0,3,6,10},
  {0,4,7,10,18},{0,3,7,9,14},{0,2,7,11}};
inline const char* SUF[] = {"maj7","maj9","m7","m9","m11","9","13","maj7#11","7b9","9sus4","m(maj7)","6/9","add9","m7b5",
                            "7#11","m6/9","maj7sus2"};
inline const char* NOTE_NAMES[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

inline double tensOf(int t) {
  switch (t) {
    case MAJ7: case MAJ9: case ADD9: case SIX9: return 0.0;
    case M7: case M9: case M69: return 0.2;
    case MAJ7SUS2: return 0.25;
    case M11: return 0.3;
    case MAJ7S11: case SUS9: return 0.5;
    case D9: case D7S11: return 0.6;
    case D13: return 0.7;
    case MMAJ7: case M7B5: return 0.8;
    case D7B9: return 1.0;
    default: return 0.3;
  }
}

// ============================================================================
//  PROGRESSION BANK
// ============================================================================
struct Ch { int r, t, b; };
struct Prog { Ch c[4]; float dk, tn; bool mn; };
inline Ch mk(int r, int t, int b = -1) { return Ch{r, t, b}; }
inline Prog mkP(Ch a, Ch b, Ch c, Ch d, float dk, float tn, bool mn = false) { return Prog{{a, b, c, d}, dk, tn, mn}; }

inline const std::vector<Prog> BANK = {
  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ9),  mk(7,SUS9),            0.12f, 0.10f),
  mkP(mk(0,MAJ7),  mk(4,M7),   mk(9,M9),    mk(5,MAJ9),            0.10f, 0.12f),
  mkP(mk(5,MAJ9),  mk(4,M9),   mk(2,M9),    mk(0,MAJ9),            0.12f, 0.08f),
  mkP(mk(0,SIX9),  mk(5,MAJ9), mk(4,M7),    mk(2,M9),              0.16f, 0.12f),
  mkP(mk(0,MAJ9),  mk(5,MAJ9,9), mk(2,M9),  mk(7,SUS9),            0.18f, 0.15f),
  mkP(mk(0,MAJ7),  mk(9,M7),   mk(2,M9),    mk(7,D13),             0.20f, 0.20f),
  mkP(mk(2,M9),    mk(7,D13),  mk(0,MAJ9),  mk(5,MAJ9),            0.22f, 0.22f),
  mkP(mk(0,MAJ9),  mk(10,D9),  mk(5,MAJ9),  mk(0,SIX9),            0.30f, 0.28f),
  mkP(mk(4,M9),    mk(9,M9),   mk(2,M9),    mk(7,SUS9),            0.25f, 0.15f),
  mkP(mk(9,M9),    mk(5,MAJ9), mk(0,MAJ9),  mk(7,SUS9),            0.30f, 0.15f),
  mkP(mk(5,MAJ7S11), mk(4,M7), mk(2,M9),    mk(0,MAJ9),            0.22f, 0.25f),
  mkP(mk(0,MAJ9),  mk(6,M7B5), mk(5,MAJ9),  mk(4,M9),              0.26f, 0.30f),
  mkP(mk(5,MAJ9),  mk(0,MAJ9,4), mk(2,M9),  mk(7,SUS9),            0.08f, 0.08f),
  mkP(mk(0,ADD9),  mk(7,SUS9), mk(9,M9),    mk(5,MAJ7S11),         0.25f, 0.20f),
  mkP(mk(0,MAJ9),  mk(8,MAJ9), mk(5,MAJ9),  mk(7,SUS9),            0.32f, 0.25f),
  mkP(mk(0,MAJ9),  mk(5,M9),   mk(4,M7),    mk(9,M9),              0.33f, 0.32f),
  mkP(mk(2,M9),    mk(4,M7),   mk(5,MAJ9),  mk(7,SUS9),            0.18f, 0.22f),
  mkP(mk(9,M9),    mk(2,M9),   mk(7,D13),   mk(0,MAJ9),            0.28f, 0.30f),
  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ9),  mk(5,M9),              0.35f, 0.28f),
  mkP(mk(0,MAJ9),  mk(5,MAJ9,0), mk(2,M9,0), mk(7,SUS9,0),         0.10f, 0.15f),

  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ7S11), mk(5,M9),            0.30f, 0.55f),
  mkP(mk(0,SIX9),  mk(8,MAJ7S11), mk(5,MAJ9), mk(7,SUS9),          0.25f, 0.60f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(9,M11),   mk(7,SUS9),            0.30f, 0.50f),
  mkP(mk(0,MAJ7S11), mk(2,D7S11), mk(4,M9), mk(5,MAJ7S11),         0.33f, 0.65f),
  mkP(mk(0,MAJ9),  mk(3,MAJ9), mk(5,MAJ7S11), mk(7,SUS9),          0.28f, 0.58f),
  mkP(mk(2,M9),    mk(1,D7S11), mk(0,MAJ9), mk(5,MAJ7S11),         0.22f, 0.62f),
  mkP(mk(0,MAJ7S11), mk(6,M7B5), mk(5,MAJ9), mk(4,M9),             0.30f, 0.66f),
  mkP(mk(0,MAJ9),  mk(2,D13),  mk(5,MAJ7S11), mk(7,SUS9),          0.25f, 0.64f),
  mkP(mk(2,M9),    mk(7,D13),  mk(0,MAJ9),  mk(10,MAJ7S11),        0.30f, 0.55f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(3,MAJ7S11), mk(2,M11),           0.35f, 0.62f),
  mkP(mk(9,M9),    mk(5,MAJ7S11), mk(0,MAJ9), mk(7,SUS9),          0.30f, 0.50f),
  mkP(mk(0,MAJ7SUS2), mk(2,M11), mk(5,MAJ7SUS2), mk(7,SUS9),       0.15f, 0.48f),
  mkP(mk(0,MAJ7SUS2), mk(5,MAJ7S11,0), mk(9,M9,0), mk(7,SUS9,0),   0.15f, 0.45f),
  mkP(mk(0,M9),    mk(8,MAJ7S11), mk(3,MAJ9), mk(10,D13),          0.42f, 0.55f, true),
  mkP(mk(5,MAJ7S11), mk(7,SUS9), mk(4,M9),  mk(9,M9),              0.28f, 0.48f),
  mkP(mk(10,MAJ7S11), mk(9,M9), mk(8,MAJ7S11), mk(7,SUS9),         0.38f, 0.72f),
  mkP(mk(0,MAJ7S11), mk(3,MAJ7S11), mk(5,MAJ7S11), mk(7,SUS9),     0.30f, 0.76f),

  mkP(mk(0,MMAJ7), mk(8,MAJ7S11), mk(5,M11), mk(7,D7B9),           0.92f, 0.80f, true),
  mkP(mk(0,M9),    mk(2,M7B5), mk(7,D7B9),  mk(0,M11),             0.85f, 0.75f, true),
  mkP(mk(2,M7B5),  mk(7,D7B9), mk(0,M11),   mk(8,MAJ9),            0.80f, 0.75f, true),
  mkP(mk(0,M9),    mk(8,MAJ9), mk(3,MAJ9),  mk(7,D7B9),            0.80f, 0.68f, true),
  mkP(mk(0,MMAJ7), mk(5,M9),   mk(2,M7B5),  mk(7,D7B9),            0.88f, 0.82f, true),
  mkP(mk(0,M11),   mk(10,D13), mk(8,MAJ7S11), mk(7,D7B9),          0.85f, 0.78f, true),
  mkP(mk(9,M11),   mk(5,MAJ7S11), mk(2,M9), mk(7,D7B9),            0.70f, 0.66f),
  mkP(mk(1,MAJ9),  mk(0,M11),  mk(8,MAJ7S11), mk(7,D7B9),          0.80f, 0.72f, true),
  mkP(mk(5,M11),   mk(1,D9),   mk(0,MMAJ7), mk(8,MAJ9),            0.85f, 0.82f, true),
  mkP(mk(0,MMAJ7), mk(0,M7,10), mk(8,MAJ7S11), mk(7,D7B9),         0.90f, 0.78f, true),
  mkP(mk(0,M11),   mk(5,M9),   mk(10,D13),  mk(3,MAJ9),            0.78f, 0.58f, true),
  mkP(mk(0,M9),    mk(0,MMAJ7,11), mk(0,M7,10), mk(0,M69,9),       0.75f, 0.60f, true),
  mkP(mk(8,MAJ7S11), mk(7,D7B9), mk(0,MMAJ7), mk(5,M9),            0.88f, 0.85f, true),
  mkP(mk(0,M9),    mk(1,MAJ9), mk(8,MAJ9),  mk(7,D7B9),            0.82f, 0.70f, true),
  mkP(mk(2,M7B5),  mk(1,D9),   mk(0,M11),   mk(5,M9),              0.78f, 0.80f, true),
  mkP(mk(0,M9),    mk(5,M11),  mk(2,M7B5),  mk(7,D7B9),            0.74f, 0.68f, true),
  mkP(mk(0,M11),   mk(3,MAJ7S11), mk(8,MAJ7S11), mk(7,D7B9),       0.72f, 0.62f, true),

  mkP(mk(0,M11),   mk(8,MAJ9), mk(5,M9),    mk(7,SUS9),            0.80f, 0.35f, true),
  mkP(mk(0,M9),    mk(8,MAJ9), mk(5,M9),    mk(7,D9),              0.75f, 0.40f, true),
  mkP(mk(9,M11),   mk(2,M9),   mk(7,D13),   mk(0,MAJ9),            0.62f, 0.35f),
  mkP(mk(0,M9),    mk(3,MAJ9), mk(8,MAJ9),  mk(7,D9),              0.70f, 0.30f, true),
  mkP(mk(5,M9),    mk(0,M11),  mk(8,MAJ9),  mk(7,D7B9),            0.75f, 0.45f, true),
  mkP(mk(0,M11),   mk(5,M9),   mk(10,D9),   mk(8,MAJ9),            0.70f, 0.40f, true),
  mkP(mk(0,M9),    mk(10,D13), mk(8,MAJ9),  mk(7,D9),              0.68f, 0.46f, true),
  mkP(mk(0,M11),   mk(0,M9,10), mk(8,MAJ9), mk(5,M9),              0.80f, 0.40f, true),
  mkP(mk(8,MAJ9),  mk(7,D9),   mk(0,M9),    mk(5,M11),             0.70f, 0.40f, true),
  mkP(mk(8,MAJ9),  mk(5,M9),   mk(10,D9),   mk(3,MAJ9),            0.62f, 0.30f, true),
  mkP(mk(0,M9),    mk(5,D9),   mk(0,M11),   mk(10,MAJ9),           0.70f, 0.25f, true),
  mkP(mk(9,M9),    mk(5,MAJ9), mk(4,M9),    mk(7,SUS9),            0.62f, 0.25f),
  mkP(mk(0,M11),   mk(8,MAJ9,0), mk(5,M11,0), mk(7,SUS9,0),        0.75f, 0.12f, true),
  mkP(mk(5,M9),    mk(3,MAJ9), mk(8,MAJ9),  mk(10,D9),             0.72f, 0.35f, true),
  mkP(mk(0,M9),    mk(5,M9),   mk(0,M11),   mk(8,MAJ9),            0.85f, 0.15f, true),
  mkP(mk(2,M9),    mk(4,M7),   mk(9,M11),   mk(7,SUS9),            0.65f, 0.20f)
};

struct Hit { double start = 0, len = 0; int chord = 0; int sus = -1; double susLen = 0; bool topOnly = false; };

enum { RES_HOME, RES_HANG, RES_TWIST };
struct Feel {
  float nost = 0.0f;
  int   res  = RES_HOME;
  float air  = 0.0f;
  float tear = 0.0f;
  float human = 0.5f;
  float reg = 0.0f;
};

struct ChordGen {
  int root[4] = {0,0,0,0}, type[4] = {MAJ9,MAJ9,MAJ9,MAJ9}, bass[4] = {0,0,0,0};
  std::vector<int> voiced[4];
  std::vector<Hit> hits;
  Prog base{};
  int baseIdx = -1, effKey = 0;
  int topLine[4] = {72,72,72,72};
  float topCenter = 72.0f, tenseV = 0.4f;
  juce::String names;
  Feel feel;
  bool tearAt[4] = {false, false, false, false};

  static int pickIdx(juce::Random& rng, float dk, float tn, float sigma = 0.17f) {
    const size_t n = BANK.size();
    std::vector<double> w(n); double sum = 0;
    for (size_t i = 0; i < n; ++i) {
      const double dx = BANK[i].dk - dk, dy = BANK[i].tn - tn;
      w[i] = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma)) + 0.002;
      sum += w[i];
    }
    double r = rng.nextDouble() * sum;
    for (size_t i = 0; i < n; ++i) { r -= w[i]; if (r <= 0.0) return (int) i; }
    return (int) n - 1;
  }

  void makeRhythm(juce::Random& rng, double bpm = 120.0) {
    hits.clear();
    hits.push_back(Hit{0.0, 4.0, 0});
    hits.push_back(Hit{4.0, 4.0, 1});
    hits.push_back(Hit{8.0, 4.0, 2});
    hits.push_back(Hit{12.0, 4.0, 3});
  }

  void tearify() {
    hits.erase(std::remove_if(hits.begin(), hits.end(), [](const Hit& h) { return h.topOnly; }), hits.end());
    std::vector<Hit> extra;
    for (auto& h : hits) {
      h.sus = -1; h.susLen = 0;
      if (h.chord < 0 || h.chord > 3 || !tearAt[h.chord] || h.len < 4.0) continue;
      const auto& v = voiced[h.chord];
      if (v.size() < 4) continue;
      const int top = v.back();
      int sus = -1;
      for (int add : {2, 1}) {
        const int cand = top + add; bool ok = cand <= 90;
        for (size_t i = 0; i + 1 < v.size(); ++i) {
          if (v[i] % 12 == cand % 12) ok = false;
          if (i > 0 && std::abs(v[i] - cand) == 1) ok = false;
        }
        if (ok) { sus = cand; break; }
      }
      if (sus < 0) continue;
      h.sus = sus; h.susLen = 2.0;
      Hit e; e.start = h.start + h.susLen; e.len = h.len - h.susLen; e.chord = h.chord; e.topOnly = true;
      extra.push_back(e);
    }
    for (auto& e : extra) hits.push_back(e);
    std::stable_sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.start < b.start; });
  }

  void apply(juce::Random& rng, int ek, float tense) {
    tenseV = tense;
    int r[4], t[4], b[4];
    for (int i = 0; i < 4; ++i) {
      r[i] = base.c[i].r; b[i] = base.c[i].b; t[i] = base.c[i].t;
    }
    for (int i = 0; i < 4; ++i) {
      root[i] = (r[i] + ek) % 12;
      bass[i] = ((b[i] < 0 ? r[i] : b[i]) + ek) % 12;
      type[i] = t[i];
    }
  }

  void planTop(juce::Random& rng) {
    std::vector<int> cands[4];
    for (int i = 0; i < 4; ++i) {
      const auto& iv = IV[type[i]];
      std::vector<int> pcs;
      for (size_t k = 1; k < iv.size(); ++k) {
        if (iv[k] % 12 == 7 && iv.size() >= 5) continue;
        if (iv[k] % 12 == 1) continue;
        pcs.push_back((root[i] + iv[k]) % 12);
      }
      for (int m = (int) topCenter - 4; m <= (int) topCenter + 4; ++m)
        for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
      if (cands[i].empty())
        for (int m = 64; m <= 76; ++m) for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
      if (cands[i].empty()) cands[i].push_back(72);
    }
    double bestS = 1e18; int bv[4] = {72, 72, 72, 72};
    for (int a : cands[0]) for (int b : cands[1]) for (int c : cands[2]) for (int d : cands[3]) {
      const int v[4] = {a, b, c, d};
      double s = rng.nextFloat();
      for (int i = 0; i < 4; ++i) {
        const int leap = std::abs(v[i] - v[(i + 1) % 4]);
        if (leap == 0) s += 0.8;
        else if (leap <= 2) s += 0.4;
        else s += 2.0;
      }
      if (s < bestS) { bestS = s; for (int i = 0; i < 4; ++i) bv[i] = v[i]; }
    }
    for (int i = 0; i < 4; ++i) topLine[i] = bv[i];
  }

  double quality() const {
    double q = 0;
    for (int i = 0; i < 4; ++i) {
      const auto& a = voiced[i]; const auto& b = voiced[(i + 1) % 4];
      if (a.size() < 2 || b.size() < 2) return 1e9;
      const int bl = std::abs(a[0] - b[0]); if (bl > 5) q += (bl - 5) * 3;
    }
    return q;
  }

  void generate(juce::Random& rng, int key, float dark, float tense, bool open, double bpm = 120.0, bool differ = false) {
    const int ek = (key == 12) ? rng.nextInt(12) : key;
    ChordGen best; double bq = 1e18;
    for (int a = 0; a < 20; ++a) {
      ChordGen t; t.feel = feel; t.effKey = ek;
      t.baseIdx = pickIdx(rng, dark, tense); t.base = BANK[(size_t) t.baseIdx];
      t.topCenter = juce::jlimit(66.0f, 74.0f, 74.0f - 4.0f * dark + 2.5f * feel.reg);
      t.apply(rng, ek, tense); t.revoice(open, rng, true); t.makeRhythm(rng, bpm);
      double q = t.quality();
      if (q < bq) { bq = q; best = t; }
    }
    *this = best;
  }

  void transpose(int d, bool open, juce::Random& rng) {
    effKey = (effKey + d) % 12;
    for (int i = 0; i < 4; ++i) {
      root[i] = (root[i] + d) % 12; bass[i] = (bass[i] + d) % 12;
      topLine[i] += d; if (topLine[i] > 77) topLine[i] -= 12;
    }
    revoice(open, rng);
  }

  void revoice(bool open, juce::Random& rng, bool replan = false) {
    if (replan) planTop(rng);
    std::vector<int> prev; juce::StringArray nm; int prevBass = -1;
    for (int i = 0; i < 4; ++i) {
      const auto& iv = IV[type[i]]; std::vector<int> pcs;
      for (size_t k = 1; k < iv.size(); ++k) pcs.push_back((root[i] + iv[k]) % 12);
      std::vector<int> up;
      for (int pc : pcs) {
        int m = 60;
        while (m % 12 != pc) m++;
        up.push_back(m);
      }
      int bassNote = 36 + (bass[i] % 12);
      voiced[i].clear(); voiced[i].push_back(bassNote);
      for (int v : up) voiced[i].push_back(v);
      juce::String n = juce::String(NOTE_NAMES[root[i]]) + SUF[type[i]];
      if (bass[i] != root[i]) n += juce::String("/") + NOTE_NAMES[bass[i]];
      nm.add(n);
    }
    names = nm.joinIntoString("   |   ");
    tearify();
  }
};
