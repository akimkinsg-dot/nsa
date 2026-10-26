#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

enum { MAJ7, MAJ9, M7, M9, M11, D9, D13, MAJ7S11, D7B9, SUS9, MMAJ7, SIX9, ADD9, M7B5 };
inline const std::vector<int> IV[] = {
  {0,4,7,11},{0,4,7,11,14},{0,3,7,10},{0,3,7,10,14},{0,3,7,10,14,17},
  {0,4,7,10,14},{0,4,10,14,21},{0,4,7,11,18},{0,4,7,10,13},{0,5,7,10,14},
  {0,3,7,11},{0,4,7,9,14},{0,4,7,14},{0,3,6,10}};
inline const char* SUF[] = {"maj7","maj9","m7","m9","m11","9","13","maj7#11","7b9","9sus4","m(maj7)","6/9","add9","m7b5"};
inline const char* NOTE_NAMES[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

inline double tensOf(int t) {
  switch (t) {
    case MAJ7: case MAJ9: case ADD9: case SIX9: return 0.0;
    case M7: case M9: return 0.2;
    case M11: return 0.3;
    case MAJ7S11: case SUS9: return 0.5;
    case D9: return 0.6;
    case D13: return 0.7;
    case MMAJ7: case M7B5: return 0.8;
    case D7B9: return 1.0;
    default: return 0.3;
  }
}

struct Ch { int r, t, b; };
struct Prog { Ch c[4]; float dk, tn; };
inline Ch mk(int r, int t, int b = -1) { return Ch{r, t, b}; }
inline Prog mkP(Ch a, Ch b, Ch c, Ch d, float dk, float tn) { return Prog{{a, b, c, d}, dk, tn}; }

// ЭЛИТАРНЫЙ БАНК R&B / TRAPSOUL ПРОГРЕССИЙ (ТОЛЬКО ВЫСШИЙ КЛАСС, ИДЕАЛЬНО ВЫВЕРЕННЫЕ ЦЕПОЧКИ)
inline const std::vector<Prog> BANK = {
  // --- Candlelight & Dates (Левый нижний квадрант: Теплый соул, мажорные переливы, интимные 90-е) ---
  mkP(mk(0,MAJ9), mk(9,M9), mk(5,MAJ9), mk(7,SUS9),              0.15f, 0.15f),
  mkP(mk(0,SIX9), mk(5,MAJ9), mk(2,M9), mk(7,ADD9),              0.20f, 0.10f),
  mkP(mk(5,MAJ9), mk(4,M9), mk(9,M9), mk(0,MAJ9),               0.10f, 0.15f),
  mkP(mk(0,ADD9), mk(7,SUS9), mk(9,M9), mk(5,MAJ7S11),          0.25f, 0.20f),
  mkP(mk(2,M9), mk(7,SUS9), mk(0,MAJ9), mk(9,M9),               0.20f, 0.20f),
  mkP(mk(0,MAJ7), mk(3,MAJ9), mk(5,MAJ9), mk(7,SUS9),           0.20f, 0.25f),
  mkP(mk(9,M9), mk(5,MAJ9), mk(0,MAJ9), mk(7,SUS9),             0.30f, 0.15f),
  mkP(mk(5,MAJ7S11), mk(4,M7), mk(2,M9), mk(0,MAJ9),            0.25f, 0.25f),

  // --- 4 AM Intimacy (Правый нижний квадрант: Глубокие ночные миноры, бархат, m11, m9) ---
  mkP(mk(0,M11), mk(8,MAJ9), mk(5,M9), mk(7,SUS9),              0.80f, 0.35f),
  mkP(mk(0,M9), mk(8,MAJ9), mk(5,M9), mk(7,D9),                 0.75f, 0.40f),
  mkP(mk(9,M11), mk(2,M9), mk(7,D13), mk(0,MAJ9),               0.65f, 0.35f),
  mkP(mk(0,M9), mk(3,MAJ9), mk(8,MAJ9), mk(7,D9),                0.70f, 0.30f),
  mkP(mk(5,M9), mk(0,M11), mk(8,MAJ9), mk(7,D7B9),              0.75f, 0.45f),
  mkP(mk(0,M11), mk(5,M9), mk(10,D9), mk(8,MAJ9),               0.70f, 0.40f),
  mkP(mk(0,M9), mk(10,D13), mk(8,MAJ9), mk(7,D7B9),             0.70f, 0.50f),

  // --- Ethereal Drama (Левый верхний квадрант: Воздушная драма, мажорные надстройки #11, чувственный трепет) ---
  mkP(mk(0,MAJ9), mk(9,M9), mk(5,MAJ7S11), mk(5,M9),            0.30f, 0.55f),
  mkP(mk(0,SIX9), mk(8,MAJ7S11), mk(5,MAJ9), mk(7,SUS9),        0.25f, 0.60f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(9,M11), mk(7,SUS9),           0.30f, 0.50f),
  mkP(mk(0,MAJ7S11), mk(2,D9), mk(4,M9), mk(5,MAJ7S11),         0.35f, 0.65f),
  mkP(mk(0,MAJ9), mk(3,D9), mk(5,MAJ7S11), mk(7,SUS9),          0.30f, 0.60f),

  // --- Toxic TrapSoul (Правый верхний квадрант: Мрачный ночной мегаполис, tension, d7b9, m7b5, mmaj7) ---
  mkP(mk(0,MMAJ7), mk(8,MAJ7S11), mk(5,M11), mk(7,D7B9),        0.95f, 0.85f),
  mkP(mk(0,M9), mk(2,M7B5), mk(7,D7B9), mk(0,M11),              0.85f, 0.80f),
  mkP(mk(0,MMAJ7), mk(0,M7,10), mk(8,MAJ9), mk(7,D7B9),         0.90f, 0.80f),
  mkP(mk(0,M11), mk(8,MAJ7S11), mk(1,MAJ9), mk(7,D7B9),         0.90f, 0.75f),
  mkP(mk(0,MMAJ7), mk(5,M9), mk(2,M7B5), mk(7,D7B9),            0.85f, 0.85f),
  mkP(mk(2,M7B5), mk(7,D7B9), mk(0,M11), mk(9,M9),              0.80f, 0.80f),
  mkP(mk(9,M11), mk(5,MAJ7S11), mk(2,M9), mk(7,D7B9),           0.75f, 0.75f),
  mkP(mk(0,M9), mk(8,MAJ9), mk(3,MAJ9), mk(7,D7B9),             0.80f, 0.70f),

  // ===== РАСШИРЕНИЕ: Candlelight & Dates (90s soul, Kaleem Taylor, Nobu Woods) =====
  mkP(mk(0,MAJ9), mk(4,M7), mk(9,M9), mk(2,D9),                  0.15f, 0.20f),
  mkP(mk(5,MAJ9), mk(0,MAJ9,4), mk(2,M9), mk(7,SUS9),            0.15f, 0.15f),
  mkP(mk(4,M9), mk(9,M9), mk(2,M9), mk(7,SUS9),                  0.25f, 0.15f),
  mkP(mk(0,MAJ9), mk(5,MAJ9,0), mk(4,M9), mk(2,M9),              0.15f, 0.10f),
  mkP(mk(0,MAJ9), mk(10,D9), mk(5,MAJ9), mk(0,SIX9),             0.30f, 0.30f),
  mkP(mk(9,M9), mk(2,M9), mk(7,SUS9), mk(0,MAJ9),                0.30f, 0.15f),
  mkP(mk(0,MAJ9), mk(6,M7B5), mk(5,MAJ9), mk(4,M9),              0.20f, 0.30f),
  mkP(mk(0,SIX9), mk(9,M7), mk(2,M9), mk(7,D13),                 0.20f, 0.25f),

  // ===== РАСШИРЕНИЕ: 4 AM Intimacy (PARTYNEXTDOOR, Drake, The Code) =====
  mkP(mk(0,M9), mk(8,MAJ9), mk(3,MAJ9), mk(10,SUS9),             0.70f, 0.30f),
  mkP(mk(0,M11), mk(5,M9), mk(8,MAJ7S11), mk(3,MAJ9),            0.75f, 0.40f),
  mkP(mk(8,MAJ9), mk(7,D9), mk(0,M9), mk(5,M11),                 0.70f, 0.40f),
  mkP(mk(0,M9), mk(3,MAJ9), mk(10,D9), mk(8,MAJ9),               0.65f, 0.35f),
  mkP(mk(5,M9), mk(3,MAJ9), mk(8,MAJ9), mk(7,SUS9),              0.75f, 0.35f),
  mkP(mk(0,M11), mk(0,M9,10), mk(8,MAJ9), mk(5,M9),              0.80f, 0.40f),
  mkP(mk(8,MAJ9), mk(5,M9), mk(10,D9), mk(3,MAJ9),               0.60f, 0.30f),
  mkP(mk(0,M9), mk(5,M9), mk(8,MAJ7S11), mk(7,D9),               0.80f, 0.45f),

  // ===== РАСШИРЕНИЕ: Ethereal Drama (Kaleem Taylor, Nobu Woods, Che Ecru) =====
  mkP(mk(0,MAJ9), mk(10,MAJ7S11), mk(5,MAJ9), mk(9,M9),          0.30f, 0.55f),
  mkP(mk(5,MAJ7S11), mk(10,MAJ7S11), mk(0,MAJ9), mk(9,M9),       0.30f, 0.60f),
  mkP(mk(0,MAJ9), mk(4,D7B9), mk(9,M11), mk(5,MAJ7S11),          0.30f, 0.65f),
  mkP(mk(2,M9), mk(8,MAJ7S11), mk(0,MAJ9), mk(5,MAJ7S11),        0.30f, 0.60f),
  mkP(mk(0,SIX9), mk(3,MAJ7S11), mk(5,MAJ9), mk(7,D13),          0.35f, 0.60f),
  mkP(mk(9,M11), mk(5,MAJ7S11), mk(0,MAJ9), mk(7,D13),           0.35f, 0.55f),
  mkP(mk(0,MAJ7S11), mk(6,M7B5), mk(5,MAJ9), mk(4,M9),           0.30f, 0.65f),
  mkP(mk(0,MAJ9), mk(2,D13), mk(5,MAJ7S11), mk(7,SUS9),          0.25f, 0.65f),

  // ===== РАСШИРЕНИЕ: Toxic TrapSoul (PARTYNEXTDOOR, Drake, Artie J) =====
  mkP(mk(0,M9), mk(1,MAJ7S11), mk(8,MAJ9), mk(7,D7B9),           0.90f, 0.80f),
  mkP(mk(0,MMAJ7), mk(5,M11), mk(1,MAJ9), mk(7,D7B9),            0.95f, 0.85f),
  mkP(mk(0,M11), mk(10,D13), mk(8,MAJ7S11), mk(7,D7B9),          0.85f, 0.80f),
  mkP(mk(8,MAJ7S11), mk(7,D7B9), mk(0,MMAJ7), mk(5,M9),          0.90f, 0.80f),
  mkP(mk(0,M9), mk(3,MAJ7S11), mk(2,M7B5), mk(7,D7B9),           0.85f, 0.75f),
  mkP(mk(5,M11), mk(1,D9), mk(0,MMAJ7), mk(8,MAJ9),              0.85f, 0.85f),
  mkP(mk(0,MMAJ7), mk(0,M9,10), mk(8,MAJ7S11), mk(2,M7B5),       0.95f, 0.80f),
  mkP(mk(1,MAJ9), mk(0,M11), mk(8,MAJ7S11), mk(7,D13),           0.80f, 0.70f)
};

// Настроение сцены: ностальгия / воздух в ритме / «слеза» топлайна / разрешение петли
struct Mood { float nost = 0.3f, air = 0.4f, tear = 0.3f, res = 0.5f; };

struct Hit { double start, len; int chord; };
struct PadPattern { int n; double s[4], l[4]; int cMap[4]; };

inline const PadPattern PATTERNS_SLOW[3] = {
  {2, {0.0, 8.0, 0, 0}, {8.0, 8.0, 0, 0}, {0, 2, 0, 0}},
  {3, {0.0, 8.0, 12.0, 0}, {8.0, 4.0, 4.0, 0}, {0, 1, 2, 0}},
  {2, {0.0, 12.0, 0, 0}, {12.0, 4.0, 0, 0}, {0, 3, 0, 0}}
};

inline const PadPattern PATTERNS_MID[3] = {
  {2, {0.0, 8.0, 0, 0}, {8.0, 8.0, 0, 0}, {0, 2, 0, 0}},
  {3, {0.0, 4.0, 8.0, 0}, {4.0, 4.0, 8.0, 0}, {0, 1, 2, 0}},
  {4, {0.0, 4.0, 8.0, 12.0}, {4.0, 4.0, 4.0, 4.0}, {0, 1, 2, 3}}
};

inline const PadPattern PATTERNS_FAST[3] = {
  {2, {0.0, 8.0, 0, 0}, {8.0, 8.0, 0, 0}, {0, 2, 0, 0}},
  {2, {0.0, 8.0, 0, 0}, {8.0, 8.0, 0, 0}, {0, 1, 0, 0}},
  {3, {0.0, 8.0, 12.0, 0}, {8.0, 4.0, 4.0, 0}, {0, 2, 3, 0}}
};

struct ChordGen {
  int root[4] = {0,0,0,0}, type[4] = {MAJ9,MAJ9,MAJ9,MAJ9}, bass[4] = {0,0,0,0};
  std::vector<int> voiced[4];
  std::vector<Hit> hits;
  Prog base{};
  int baseIdx = -1;
  int topLine[4] = {72,72,72,72};
  float topCenter = 72.0f, tenseV = 0.4f;
  juce::String names;
  Mood mood;

  static int pickIdx(juce::Random& rng, float dk, float tn, float sigma = 0.25f) {
    const size_t n = BANK.size();
    std::vector<double> w(n); double sum = 0;
    for (size_t i = 0; i < n; ++i) {
      const double dx = BANK[i].dk - dk, dy = BANK[i].tn - tn;
      w[i] = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma)) + 0.005;
      sum += w[i];
    }
    double r = rng.nextDouble() * sum;
    for (size_t i = 0; i < n; ++i) { r -= w[i]; if (r <= 0.0) return (int) i; }
    return (int) n - 1;
  }

  void makeRhythm(juce::Random& rng, double bpm = 120.0) {
    hits.clear();
    const PadPattern* selectedPool = PATTERNS_MID;
    if (bpm < 85.0) selectedPool = PATTERNS_SLOW;
    else if (bpm > 115.0) selectedPool = PATTERNS_FAST;

    int pi = rng.nextInt(3);
    const PadPattern& p = selectedPool[pi];
    for (int k = 0; k < p.n; ++k) {
      int chordIndex = p.cMap[k] % 4;
      hits.push_back(Hit{ p.s[k], p.l[k], chordIndex });
    }
    // AIR: опережение смены аккорда на 1/8 такта + «вдох» перед следующим аккордом
    const double air = (double) mood.air;
    for (size_t k = 1; k < hits.size(); ++k)
      if (rng.nextDouble() < air * 0.6) hits[k].start -= 0.5;
    for (size_t k = 0; k < hits.size(); ++k) {
      const double end = (k + 1 < hits.size()) ? hits[k + 1].start : 16.0;
      hits[k].len = std::max(1.0, end - hits[k].start - air * 0.75);
    }
  }

  void apply(juce::Random& rng, int key, float tense) {
    tenseV = tense;
    const int effectiveKey = (key == 12) ? rng.nextInt(12) : key;

    int t[4];
    for (int i = 0; i < 4; ++i) {
      t[i] = base.c[i].t; const float r = rng.nextFloat();
      if (t[i] == D7B9 && r < 0.4f * (1.0f - tense)) t[i] = D9;
      else if (t[i] == MAJ7S11 && r < 0.3f * (1.0f - tense)) t[i] = MAJ9;
      else if (t[i] == M11 && r < 0.3f * (1.0f - tense)) t[i] = M9;
      // ностальгия: «пыльная» сладость (6/9, add9, m7 вместо глянцевых maj9/m9)
      if (rng.nextFloat() < mood.nost * 0.3f) {
        if (t[i] == MAJ9) t[i] = rng.nextBool() ? SIX9 : ADD9;
        else if (t[i] == M9 && tense < 0.5f) t[i] = M7;
      }
    }
    for (int i = 0; i < 4; ++i) {
      root[i] = (base.c[i].r + effectiveKey) % 12;
      bass[i] = ((base.c[i].b < 0 ? base.c[i].r : base.c[i].b) + effectiveKey) % 12;
      type[i] = t[i];
    }
    // ностальгия: педаль тоники под 2-м и 3-м аккордами (slash-аккорды, «тоска по дому»)
    if (mood.nost > 0.45f && rng.nextFloat() < mood.nost) {
      const int p0 = base.c[0].b < 0 ? base.c[0].r : base.c[0].b;
      for (int i = 1; i < 3; ++i) {
        if (base.c[i].b >= 0) continue;
        const int rel = ((p0 - base.c[i].r) % 12 + 12) % 12;
        bool ct = false;
        for (int v : IV[t[i]]) if (v % 12 == rel) ct = true;
        if (ct && rel != 0) bass[i] = (p0 + effectiveKey) % 12;
      }
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
      int mx = v[0], cntMax = 0, argmax = 0, second = -999; double mean = 0;
      for (int i = 0; i < 4; ++i) { mx = std::max(mx, v[i]); mean += v[i]; }
      mean /= 4.0;
      for (int i = 0; i < 4; ++i) { if (v[i] == mx) { ++cntMax; argmax = i; } else second = std::max(second, v[i]); }
      for (int i = 0; i < 4; ++i) {
        const int leap = std::abs(v[i] - v[(i + 1) % 4]);
        s += leap * 1.2; if (leap > 3) s += (leap - 3) * 2.5;
      }
      s += std::abs(mean - topCenter) * 0.3;
      // TEAR: верхний голос тянется к расширениям (9/11/#11/maj7), особенно ближе к концу фразы
      static const double wExt[4] = {0.4, 0.8, 1.5, 1.0};
      for (int i = 0; i < 4; ++i) {
        const int rel = ((v[i] - root[i]) % 12 + 12) % 12;
        if (rel == 2 || rel == 5 || rel == 6 || rel == 11) s -= (double) mood.tear * 4.0 * wExt[i];
      }
      if (s < bestS) { bestS = s; for (int i = 0; i < 4; ++i) bv[i] = v[i]; }
    }
    for (int i = 0; i < 4; ++i) topLine[i] = bv[i];
  }

  double quality() const {
    static const double ideal[4] = {0.25, 0.5, 1.0, 0.5};
    double q = 0;
    for (int i = 0; i < 4; ++i) {
      const auto& a = voiced[i]; const auto& b = voiced[(i + 1) % 4];
      if (a.size() < 2 || b.size() < 2) return 1e9;
      // RESOLVE: насколько строго последний аккорд «замыкается» в начало петли
      const double w = (i == 3) ? 0.3 + 1.7 * (double) mood.res : 1.0;
      const int bl = std::abs(a[0] - b[0]); if (bl > 5) q += w * (bl - 5) * 3;
      const int tl = std::abs(a.back() - b.back()); if (tl > 4) q += w * (tl - 4) * 2;
      q += 3.0 * std::abs(tensOf(type[i]) - ideal[i] * (0.4 + 0.6 * (double) tenseV));
    }
    return q;
  }

  void generate(juce::Random& rng, int key, float dark, float tense, bool open, double bpm = 120.0, bool differ = false, const Mood& m = Mood{}) {
    const int avoid = differ ? baseIdx : -1;
    ChordGen best; double bq = 1e18;
    for (int a = 0; a < 30; ++a) {
      ChordGen t;
      t.mood = m;
      t.baseIdx = pickIdx(rng, dark, tense); t.base = BANK[(size_t) t.baseIdx];
      // уникальность: иногда начинаем историю с другого аккорда (чем выше resolve, тем реже)
      if (rng.nextFloat() < 0.15f + 0.35f * (1.0f - m.res))
        std::rotate(t.base.c, t.base.c + 1 + rng.nextInt(3), t.base.c + 4);
      t.topCenter = 74.0f - 4.0f * dark;
      t.apply(rng, key, tense); t.revoice(open, rng, true); t.makeRhythm(rng, bpm);
      double q = t.quality();
      if (t.baseIdx == avoid) q += 25;
      if (q < bq) { bq = q; best = t; }
    }
    *this = best;
  }

  void evolve(juce::Random& rng, int key, float tense, bool open, double bpm = 120.0, const Mood& m = Mood{}) {
    ChordGen best; double bq = 1e18; bool found = false;
    for (int a = 0; a < 12; ++a) {
      Prog np = base; bool changed = false;
      const int changes = 1;
      for (int m = 0; m < changes; ++m) {
        const int k = 1 + rng.nextInt(3);
        const Prog& q = BANK[(size_t) pickIdx(rng, base.dk, base.tn, 0.2f)];
        const Ch nc = q.c[k]; const Ch old = np.c[k];
        if (nc.r == old.r && nc.t == old.t && nc.b == old.b) continue;
        np.c[k] = nc; changed = true;
      }
      if (!changed) continue;
      ChordGen t = *this; t.base = np; t.mood = m;
      t.apply(rng, key, tense); t.revoice(open, rng, true); t.makeRhythm(rng, bpm);
      const double q = t.quality();
      if (q < bq) { bq = q; best = t; found = true; }
    }
    if (found) *this = best;
  }

  void transpose(int d, bool open, juce::Random& rng) {
    for (int i = 0; i < 4; ++i) {
      root[i] = (root[i] + d) % 12; bass[i] = (bass[i] + d) % 12;
      topLine[i] += d; if (topLine[i] > 77) topLine[i] -= 12;
    }
    revoice(open, rng);
  }

  static std::vector<int> pick(int rt, int t, const std::vector<int>& prev, juce::Random& rng, int top = -1) {
    const auto& iv = IV[t]; std::vector<int> pcs;
    for (size_t i = 1; i < iv.size(); ++i) { if (iv[i] % 12 == 7 && iv.size() >= 5) continue; pcs.push_back((rt + iv[i]) % 12); }
    std::vector<std::vector<int>> cand;
    for (int pc : pcs) { std::vector<int> a; for (int m = 59; m <= 78; ++m) if (m % 12 == pc) a.push_back(m); cand.push_back(a); }
    std::vector<int> best, cur; double bestCost = 1e9;
    std::function<void(size_t)> rec = [&](size_t k) {
      if (k == cand.size()) {
        auto s = cur; std::sort(s.begin(), s.end());
        double mean = 0; for (int v : s) mean += v; mean /= (double) s.size();
        double cost = rng.nextFloat() * 0.5;
        if (s.back() - s.front() > 14) cost += 100;
        if (top >= 0 && s.back() != top) cost += 400;
        for (size_t i = 0; i + 1 < s.size(); ++i) {
          const int gap = s[i + 1] - s[i];
          if (gap < 3 && s[i] < 66) cost += 25; else if (gap == 1) cost += 2;
        }
        if (!prev.empty()) {
          double pm = 0; for (int v : prev) pm += v; pm /= (double) prev.size();
          for (int v : s) { int d = 99; for (int q : prev) d = std::min(d, std::abs(v - q)); cost += d; }
          cost += std::abs(mean - pm) * 0.5;
        } else cost += std::abs(mean - 68) * 1.5;
        if (cost < bestCost) { bestCost = cost; best = s; }
        return;
      }
      for (int n : cand[k]) { cur.push_back(n); rec(k + 1); cur.pop_back(); }
    };
    rec(0);
    return best;
  }

  void revoice(bool open, juce::Random& rng, bool replan = false) {
    if (replan) planTop(rng);
    std::vector<int> prev; juce::StringArray nm; int prevBass = -1;
    for (int i = 0; i < 4; ++i) {
      auto up = pick(root[i], type[i], prev, rng, topLine[i]); prev = up;
      int bassNote = 31 + (((bass[i] - 31) % 12) + 12) % 12;
      if (prevBass >= 0) {
        int bestD = 99;
        for (int m = 31; m <= 43; ++m)
          if (m % 12 == bass[i] && std::abs(m - prevBass) < bestD) { bestD = std::abs(m - prevBass); bassNote = m; }
      }
      prevBass = bassNote;
      if (open && up.size() >= 4) {
        const int dn = up[up.size() - 2] - 12;
        if (dn >= 55) up[up.size() - 2] = dn;
      }
      std::sort(up.begin(), up.end());
      voiced[i].clear(); voiced[i].push_back(bassNote);
      for (int v : up) voiced[i].push_back(v);
      juce::String n = juce::String(NOTE_NAMES[root[i]]) + SUF[type[i]];
      if (bass[i] != root[i]) n += juce::String("/") + NOTE_NAMES[bass[i]];
      nm.add(n);
    }
    names = nm.joinIntoString("   |   ");
  }
};
