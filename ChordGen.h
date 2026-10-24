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

inline const std::vector<Prog> BANK = {
  mkP(mk(0,MAJ9), mk(9,M9), mk(5,MAJ7S11), mk(5,M9),            0.30f, 0.25f),
  mkP(mk(0,MAJ9), mk(9,M9), mk(2,M9), mk(7,D13),                0.15f, 0.45f),
  mkP(mk(5,MAJ9), mk(4,M7), mk(9,M9), mk(2,M11),                0.20f, 0.15f),
  mkP(mk(0,SIX9), mk(8,MAJ7S11), mk(5,MAJ9), mk(7,SUS9),        0.20f, 0.30f),
  mkP(mk(0,ADD9), mk(7,ADD9,11), mk(9,M9), mk(5,MAJ9),          0.10f, 0.10f),
  mkP(mk(0,M9), mk(8,MAJ9), mk(5,M9), mk(7,D7B9),               0.80f, 0.65f),
  mkP(mk(0,M11), mk(5,M9), mk(10,D9), mk(8,MAJ7S11),            0.75f, 0.50f),
  mkP(mk(0,MMAJ7), mk(0,M7,10), mk(8,MAJ9), mk(7,D7B9),         0.90f, 0.80f),
  mkP(mk(0,M9), mk(10,D9), mk(3,MAJ9), mk(5,M11),               0.70f, 0.35f),
  mkP(mk(0,M11), mk(8,MAJ7S11), mk(1,MAJ9), mk(7,D7B9),         0.90f, 0.75f),
  mkP(mk(0,MAJ9), mk(8,MAJ7S11), mk(5,MAJ9), mk(10,D9),         0.25f, 0.20f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(9,M11), mk(7,SUS9),           0.30f, 0.30f),
  mkP(mk(0,SIX9), mk(2,D9), mk(4,M9), mk(5,MAJ7S11),            0.20f, 0.25f),
  mkP(mk(9,M9), mk(5,MAJ9), mk(0,MAJ9), mk(7,SUS9),             0.40f, 0.15f),
  mkP(mk(0,MAJ7), mk(3,MAJ9), mk(5,MAJ7S11), mk(7,SUS9),        0.30f, 0.30f),
  mkP(mk(0,M9), mk(8,MAJ9), mk(3,MAJ9), mk(10,D9),              0.65f, 0.15f),
  mkP(mk(0,M11), mk(5,M9), mk(8,MAJ7S11), mk(7,D7B9),           0.70f, 0.45f),
  mkP(mk(5,M9), mk(0,M11), mk(8,MAJ9), mk(7,D7B9),              0.75f, 0.50f),
  mkP(mk(0,M9), mk(10,D13), mk(8,MAJ9), mk(7,D7B9),             0.70f, 0.60f),
  mkP(mk(0,MMAJ7), mk(5,M9), mk(2,M7B5), mk(7,D7B9),            0.85f, 0.85f),
  mkP(mk(5,MAJ9), mk(7,D13), mk(4,M9), mk(9,M11),               0.30f, 0.40f),
  mkP(mk(0,MAJ9), mk(3,D9), mk(5,MAJ7S11), mk(7,SUS9),          0.25f, 0.45f),
  mkP(mk(2,M9), mk(7,D13), mk(4,M9), mk(9,D7B9),                0.50f, 0.70f),
  mkP(mk(0,MAJ7), mk(2,D9), mk(4,M7), mk(9,M9),                 0.30f, 0.35f),
  mkP(mk(0,M9), mk(5,D9), mk(3,MAJ9), mk(10,MAJ9),              0.50f, 0.30f)
};

struct Hit { double start, len; int chord; };
struct Pat { int n; double s[3], l[3]; };
inline const Pat PATS[6] = {
  {1, {0,0,0},     {4,0,0}},
  {2, {0,2.5,0},   {2.5,1.5,0}},
  {3, {0,2,3},     {2,1,1}},
  {3, {0,1.5,3},   {1.5,1.5,1}},
  {2, {0,1.5,0},   {1,2.5,0}},
  {1, {0.5,0,0},   {3.5,0,0}}
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

  static int pickIdx(juce::Random& rng, float dk, float tn, float sigma = 0.28f) {
    const size_t n = BANK.size();
    std::vector<double> w(n); double sum = 0;
    for (size_t i = 0; i < n; ++i) {
      const double dx = BANK[i].dk - dk, dy = BANK[i].tn - tn;
      w[i] = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma)) + 0.01;
      sum += w[i];
    }
    double r = rng.nextDouble() * sum;
    for (size_t i = 0; i < n; ++i) { r -= w[i]; if (r <= 0.0) return (int) i; }
    return (int) n - 1;
  }

  void makeRhythm(juce::Random& rng) {
    hits.clear(); int prevPat = -1;
    for (int bar = 0; bar < 4; ++bar) {
      const int m = (bar == 0 ? 5 : 6);
      int pi = rng.nextInt(m);
      if (pi == prevPat) pi = (pi + 1 + rng.nextInt(4)) % m;
      prevPat = pi;
      const Pat& p = PATS[pi];
      for (int k = 0; k < p.n; ++k) hits.push_back(Hit{ bar * 4.0 + p.s[k], p.l[k], bar });
      if (bar > 0 && p.s[0] == 0.0 && rng.nextFloat() < 0.35f) {
        Hit& f = hits[hits.size() - (size_t) p.n];
        f.start -= 0.5; f.len += 0.5;
      }
    }
    for (size_t i = 0; i + 1 < hits.size(); ++i)
      hits[i].len = std::min(hits[i].len, hits[i + 1].start - hits[i].start);
  }

  void apply(juce::Random& rng, int key, float tense) {
    tenseV = tense;
    int t[4];
    for (int i = 0; i < 4; ++i) {
      t[i] = base.c[i].t; const float r = rng.nextFloat();
      if (t[i] == D7B9 && r < 0.5f * (1.0f - tense)) t[i] = D9;
      else if (t[i] == MAJ7S11 && r < 0.4f * (1.0f - tense)) t[i] = MAJ9;
      else if (t[i] == M11 && r < 0.3f * (1.0f - tense)) t[i] = M9;
      else if ((t[i] == M7 || t[i] == MAJ7) && r < 0.4f) t[i] = (t[i] == M7 ? M9 : MAJ9);
    }
    auto isSurp = [](int x) { return x == MAJ7S11 || x == MMAJ7; };
    int cnt = 0, keep = -1;
    for (int i = 0; i < 4; ++i)
      if (isSurp(t[i])) { ++cnt; if (keep < 0 || std::abs(i - 2) <= std::abs(keep - 2)) keep = i; }
    if (cnt > 1) {
      for (int i = 0; i < 4; ++i)
        if (i != keep && isSurp(t[i])) t[i] = (t[i] == MMAJ7 ? M9 : MAJ9);
    } else if (cnt == 0 && rng.nextFloat() < 0.6f + 0.4f * tense) {
      const int k = rng.nextFloat() < 0.7f ? 2 : 1;
      const int x = t[k];
      if (x == MAJ9 || x == MAJ7 || x == ADD9 || x == SIX9) t[k] = MAJ7S11;
      else if ((x == M9 || x == M7 || x == M11) && base.c[k].r == 0) t[k] = MMAJ7;
      else if (x == M9 || x == M7) t[k] = M11;
      else if (x == D9) t[k] = D13;
    }
    for (int i = 0; i < 4; ++i) {
      root[i] = (base.c[i].r + key) % 12;
      bass[i] = ((base.c[i].b < 0 ? base.c[i].r : base.c[i].b) + key) % 12;
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
      for (int m = (int) topCenter - 5; m <= (int) topCenter + 4; ++m)
        for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
      if (cands[i].empty())
        for (int m = 62; m <= 78; ++m) for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
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
        s += leap * 1.5; if (leap > 4) s += (leap - 4) * 3;
      }
      if (a == b && b == c && c == d) s += 10;
      if (cntMax == 1) { s -= 3; if (argmax == 2) s -= 3; else if (argmax == 1) s -= 1; if (mx - second >= 2) s -= 2; }
      s += std::abs(mean - topCenter) * 0.4;
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
      const int bl = std::abs(a[0] - b[0]); if (bl > 5) q += (bl - 5) * 3;
      const int tl = std::abs(a.back() - b.back()); if (tl > 4) q += (tl - 4) * 2;
      const int j = (i + 1) % 4;
      if (root[i] == root[j] && bass[i] == bass[j]) q += 15;
      std::vector<int> pa, pb;
      for (size_t k = 1; k < a.size(); ++k) pa.push_back(a[k] % 12);
      for (size_t k = 1; k < b.size(); ++k) pb.push_back(b[k] % 12);
      std::sort(pa.begin(), pa.end()); std::sort(pb.begin(), pb.end());
      if (pa == pb) q += 15;
      for (size_t k = 0; k + 1 < a.size(); ++k) { const int gap = a[k + 1] - a[k]; if (gap < 3 && a[k] < 66) q += 10; }
      if (a.back() > 79 || a.back() < 64) q += 5;
      q += 3.0 * std::abs(tensOf(type[i]) - ideal[i] * (0.4 + 0.6 * (double) tenseV));
    }
    return q;
  }

  void generate(juce::Random& rng, int key, float dark, float tense, bool open, bool differ = false) {
    const int avoid = differ ? baseIdx : -1;
    ChordGen best; double bq = 1e18;
    for (int a = 0; a < 24; ++a) {
      ChordGen t;
      t.baseIdx = pickIdx(rng, dark, tense); t.base = BANK[(size_t) t.baseIdx];
      t.topCenter = 74.0f - 4.0f * dark;
      t.apply(rng, key, tense); t.revoice(open, rng, true); t.makeRhythm(rng);
      double q = t.quality();
      if (t.baseIdx == avoid) q += 25;
      if (q < bq) { bq = q; best = t; }
    }
    *this = best;
  }

  void evolve(juce::Random& rng, int key, float tense, bool open) {
    ChordGen best; double bq = 1e18; bool found = false;
    for (int a = 0; a < 12; ++a) {
      Prog np = base; bool changed = false;
      const int changes = rng.nextFloat() < 0.4f ? 2 : 1;
      for (int m = 0; m < changes; ++m) {
        const int k = 1 + rng.nextInt(3);
        const Prog& q = BANK[(size_t) pickIdx(rng, base.dk, base.tn, 0.2f)];
        const Ch nc = q.c[k]; const Ch old = np.c[k];
        if (nc.r == old.r && nc.t == old.t && nc.b == old.b) continue;
        if (nc.r == np.c[k - 1].r || nc.r == np.c[(k + 1) % 4].r) continue;
        np.c[k] = nc; changed = true;
      }
      if (!changed) continue;
      ChordGen t = *this; t.base = np;
      t.apply(rng, key, tense); t.revoice(open, rng, true);
      if (rng.nextFloat() < 0.4f) t.makeRhythm(rng);
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
        double cost = rng.nextFloat() * 0.8;
        if (s.back() - s.front() > 15) cost += 100;
        if (top >= 0 && s.back() != top) cost += 500;
        for (size_t i = 0; i + 1 < s.size(); ++i) {
          const int gap = s[i + 1] - s[i];
          if (gap < 3 && s[i] < 66) cost += 30; else if (gap == 1) cost += 3;
        }
        if (!prev.empty()) {
          double pm = 0; for (int v : prev) pm += v; pm /= (double) prev.size();
          for (int v : s) { int d = 99; for (int q : prev) d = std::min(d, std::abs(v - q)); cost += d; }
          cost += std::abs(mean - pm) * 0.7;
        } else cost += std::abs(mean - 68) * 2;
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
