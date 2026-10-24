#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

enum { MAJ7, MAJ9, M7, M9, M11, D9, D13, MAJ7S11, D7B9, SUS9 };
inline const std::vector<int> IV[] = {
  {0,4,7,11},{0,4,7,11,14},{0,3,7,10},{0,3,7,10,14},{0,3,7,10,14,17},
  {0,4,7,10,14},{0,4,10,14,21},{0,4,7,11,18},{0,4,7,10,13},{0,5,7,10,14}};
inline const char* SUF[] = {"maj7","maj9","m7","m9","m11","9","13","maj7#11","7b9","9sus4"};
inline const char* NOTE_NAMES[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

struct Prog { int r[4]; int t[4]; };
inline const std::vector<Prog> MOODS[5] = {
 { {{0,4,9,5},{MAJ9,M9,M9,MAJ9}}, {{0,5,4,9},{MAJ9,MAJ9,M7,M11}}, {{5,4,2,7},{MAJ9,M9,M9,D13}},
   {{2,7,0,5},{M9,D13,MAJ9,MAJ7S11}}, {{0,9,2,7},{MAJ7,M9,M9,SUS9}} },
 { {{0,8,3,10},{M9,MAJ9,MAJ9,D9}}, {{0,5,8,7},{M11,M9,MAJ7S11,D7B9}}, {{0,3,10,8},{M9,MAJ7,D9,MAJ9}},
   {{5,0,8,7},{M9,M11,MAJ9,D7B9}}, {{0,10,8,7},{M9,D13,MAJ9,D7B9}} },
 { {{0,8,5,10},{MAJ9,MAJ7S11,MAJ9,D9}}, {{5,4,9,2},{MAJ7S11,M9,M11,SUS9}},
   {{0,4,5,7},{MAJ7S11,M9,MAJ9,SUS9}}, {{3,0,5,10},{MAJ9,MAJ7S11,MAJ9,MAJ9}} },
 { {{0,8,5,7},{M9,MAJ9,M9,D7B9}}, {{0,3,8,10},{M11,MAJ9,MAJ7,D9}}, {{0,5,10,3},{M9,M11,D9,MAJ9}},
   {{8,7,0,5},{MAJ9,D7B9,M11,M9}} },
 { {{5,7,4,9},{MAJ9,D13,M9,M11}}, {{0,3,5,7},{MAJ9,D9,MAJ9,SUS9}}, {{2,7,4,9},{M9,D13,M9,D7B9}},
   {{0,2,4,9},{MAJ7,D9,M7,M9}} }
};

struct ChordGen {
  int root[4] = {0,0,0,0}, type[4] = {MAJ9,MAJ9,MAJ9,MAJ9};
  std::vector<int> voiced[4];
  juce::String names;

  void generate(juce::Random& rng, int key, int mood) {
    const auto& list = MOODS[juce::jlimit(0, 4, mood)];
    const Prog& p = list[(size_t) rng.nextInt((int) list.size())];
    int rot = rng.nextInt(4);
    for (int i = 0; i < 4; ++i) {
      int t = p.t[(i + rot) % 4]; float r = rng.nextFloat();
      if (t == MAJ9 && r < 0.2f) t = MAJ7; else if (t == M9 && r < 0.25f) t = M11; else if (t == M7 && r < 0.4f) t = M9;
      root[i] = (p.r[(i + rot) % 4] + key) % 12; type[i] = t;
    }
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
      if (open && up.size() >= 4) up[up.size() - 2] -= 12;
      std::sort(up.begin(), up.end());
      voiced[i].clear(); voiced[i].push_back(36 + root[i]);
      for (int v : up) voiced[i].push_back(v);
      nm.add(juce::String(NOTE_NAMES[root[i]]) + SUF[type[i]]);
    }
    names = nm.joinIntoString("   |   ");
  }
};
