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

// One chord: root offset from the key, chord type, bass offset (-1 = bass is the root)
struct Ch { int r, t, b; };
struct Prog { Ch c[4]; };
inline Ch mk(int r, int t, int b = -1) { return Ch{r, t, b}; }
inline Prog mkP(Ch a, Ch b, Ch c, Ch d) { return Prog{{a, b, c, d}}; }

// Starter bank: 5 moods x 5 progressions. Order of each: home -> departure -> color/tension -> soft return
// Moods: 0 Warm, 1 Dark, 2 Dreamy, 3 Sad, 4 Sensual
inline const std::vector<Prog> MOODS[5] = {
  // Warm
  { mkP(mk(0,MAJ9), mk(9,M9), mk(5,MAJ7S11), mk(5,M9)),
    mkP(mk(0,MAJ9), mk(9,M9), mk(2,M9), mk(7,D13)),
    mkP(mk(5,MAJ9), mk(4,M7), mk(9,M9), mk(2,M11)),
    mkP(mk(0,SIX9), mk(8,MAJ7S11), mk(5,MAJ9), mk(7,SUS9)),
    mkP(mk(0,ADD9), mk(7,ADD9,11), mk(9,M9), mk(5,MAJ9)) },
  // Dark
  { mkP(mk(0,M9), mk(8,MAJ9), mk(5,M9), mk(7,D7B9)),
    mkP(mk(0,M11), mk(5,M9), mk(10,D9), mk(8,MAJ7S11)),
    mkP(mk(0,MMAJ7), mk(0,M7,10), mk(8,MAJ9), mk(7,D7B9)),
    mkP(mk(0,M9), mk(10,D9), mk(3,MAJ9), mk(5,M11)),
    mkP(mk(0,M11), mk(8,MAJ7S11), mk(1,MAJ9), mk(7,D7B9)) },
  // Dreamy
  { mkP(mk(0,MAJ9), mk(8,MAJ7S11), mk(5,MAJ9), mk(10,D9)),
    mkP(mk(5,MAJ7S11), mk(4,M9), mk(9,M11), mk(7,SUS9)),
    mkP(mk(0,SIX9), mk(2,D9), mk(4,M9), mk(5,MAJ7S11)),
    mkP(mk(9,M9), mk(5,MAJ9), mk(0,MAJ9), mk(7,SUS9)),
    mkP(mk(0,MAJ7), mk(3,MAJ9), mk(5,MAJ7S11), mk(7,SUS9)) },
  // Sad
  { mkP(mk(0,M9), mk(8,MAJ9), mk(3,MAJ9), mk(10,D9)),
    mkP(mk(0,M11), mk(5,M9), mk(8,MAJ7S11), mk(7,D7B9)),
    mkP(mk(5,M9), mk(0,M11), mk(8,MAJ9), mk(7,D7B9)),
    mkP(mk(0,M9), mk(10,D13), mk(8,MAJ9), mk(7,D7B9)),
    mkP(mk(0,MMAJ7), mk(5,M9), mk(2,M7B5), mk(7,D7B9)) },
  // Sensual
  { mkP(mk(5,MAJ9), mk(7,D13), mk(4,M9), mk(9,M11)),
    mkP(mk(0,MAJ9), mk(3,D9), mk(5,MAJ7S11), mk(7,SUS9)),
    mkP(mk(2,M9), mk(7,D13), mk(4,M9), mk(9,D7B9)),
    mkP(mk(0,MAJ7), mk(2,D9), mk(4,M7), mk(9,M9)),
    mkP(mk(0,M9), mk(5,D9), mk(3,MAJ9), mk(10,MAJ9)) }
};

// One chord hit in the 4-bar loop (16 beats): start/length in beats, chord = 0..3
struct Hit { double start, len; int chord; };
// Rhythm patterns inside one bar (start, length in beats)
struct Pat { int n; double s[3], l[3]; };
inline const Pat PATS[6] = {
  {1, {0,0,0},     {4,0,0}},        // hold the whole bar
  {2, {0,2.5,0},   {2.5,1.5,0}},    // long + short
  {3, {0,2,3},     {2,1,1}},        // 2 + 1 + 1
  {3, {0,1.5,3},   {1.5,1.5,1}},    // dotted
  {2, {0,1.5,0},   {1,2.5,0}},      // hit, rest, late hit
  {1, {0.5,0,0},   {3.5,0,0}}       // enter off the beat
};

struct ChordGen {
  int root[4] = {0,0,0,0}, type[4] = {MAJ9,MAJ9,MAJ9,MAJ9}, bass[4] = {0,0,0,0};
  std::vector<int> voiced[4];
  std::vector<Hit> hits;
  juce::String names;

  void makeRhythm(juce::Random& rng) {
    hits.clear(); int prevPat = -1;
    for (int bar = 0; bar < 4; ++bar) {
      const int m = (bar == 0 ? 5 : 6);
      int pi = rng.nextInt(m);
      if (pi == prevPat) pi = (pi + 1 + rng.nextInt(4)) % m;
      prevPat = pi;
      const Pat& p = PATS[pi];
      for (int k = 0; k < p.n; ++k) hits.push_back(Hit{ bar * 4.0 + p.s[k], p.l[k], bar });
      // anticipation: chord arrives half a beat early
      if (bar > 0 && p.s[0] == 0.0 && rng.nextFloat() < 0.35f) {
        Hit& f = hits[hits.size() - (size_t) p.n];
        f.start -= 0.5; f.len += 0.5;
      }
    }
    for (size_t i = 0; i + 1 < hits.size(); ++i)
      hits[i].len = std::min(hits[i].len, hits[i + 1].start - hits[i].start);
  }

  void generate(juce::Random& rng, int key, int mood) {
    mood = juce::jlimit(0, 4, mood);
    const auto& list = MOODS[mood];
    const Prog& p = list[(size_t) rng.nextInt((int) list.size())];
    const bool minorish = (mood == 1 || mood == 3);
    for (int i = 0; i < 4; ++i) {
      const Ch c = p.c[i];
      int t = c.t; float r = rng.nextFloat();
      if (i == 0 && c.r == 0 && minorish && (t == M9 || t == M11) && r < 0.2f) t = MMAJ7;
      else if (i == 0 && !minorish && t == MAJ9 && r < 0.25f) t = SIX9;
      else if (i > 0 && t == MAJ9 && r < 0.2f) t = MAJ7S11;
      else if (t == M9 && r < 0.25f) t = M11;
      else if ((t == M7 || t == MAJ7) && r < 0.4f) t = (t == M7 ? M9 : MAJ9);
      root[i] = (c.r + key) % 12;
      bass[i] = ((c.b < 0 ? c.r : c.b) + key) % 12;
      type[i] = t;
    }
    makeRhythm(rng);
  }

  static std::vector<int> pick(int rt, int t, const std::vector<int>& prev, juce::Random& rng) {
    const auto& iv = IV[t]; std::vector<int> pcs;
    for (size_t i = 1; i < iv.size(); ++i) { if (iv[i] % 12 == 7 && iv.size() >= 5) continue; pcs.push_back((rt + iv[i]) % 12); }
    std::vector<std::vector<int>> cand;
    for (int pc : pcs) { std::vector<int> a; for (int m = 54; m <= 78; ++m) if (m % 12 == pc) a.push_back(m); cand.push_back(a); }
    std::vector<int> best, cur; double bestCost = 1e9;
    std::function<void(size_t)> rec = [&](size_t k) {
      if (k == cand.size()) {
        auto s = cur; std::sort(s.begin(), s.end());
        double mean = 0; for (int v : s) mean += v; mean /= (double) s.size();
        double cost = rng.nextFloat() * 0.8;
        if (s.back() - s.front() > 15) cost += 100;
        // no muddy clusters in the low register
        for (size_t i = 0; i + 1 < s.size(); ++i) {
          int gap = s[i + 1] - s[i];
          if (gap < 3 && s[i] < 66) cost += 30; else if (gap == 1) cost += 3;
        }
        if (!prev.empty()) {
          double pm = 0; for (int v : prev) pm += v; pm /= (double) prev.size();
          for (int v : s) { int d = 99; for (int q : prev) d = std::min(d, std::abs(v - q)); cost += d; }
          cost += std::abs(mean - pm) * 0.7;
        } else cost += std::abs(mean - 64) * 2;
        if (cost < bestCost) { bestCost = cost; best = s; }
        return;
      }
      for (int n : cand[k]) { cur.push_back(n); rec(k + 1); cur.pop_back(); }
    };
    rec(0);
    return best;
  }

  void revoice(bool open, juce::Random& rng) {
    std::vector<int> prev; juce::StringArray nm;
    for (int i = 0; i < 4; ++i) {
      auto up = pick(root[i], type[i], prev, rng); prev = up;
      const int bassNote = 36 + bass[i];
      if (open && up.size() >= 4) {
        int dn = up[up.size() - 2] - 12;
        if (dn - bassNote >= 7) up[up.size() - 2] = dn;
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
