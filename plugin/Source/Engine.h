#pragma once

#include "Patterns.h"

#include <cmath>
#include <limits>

namespace bm
{

constexpr long long noBar = std::numeric_limits<long long>::min();

// Where the groove's cymbal part goes (the downbeat crash always stays a crash).
enum class CymbalSwap : int { AsWritten, Ride, Bell, Crash, China, HiHat, HiHatOpen, NumChoices };

inline int cymbalSwapLane (CymbalSwap c) noexcept
{
    switch (c)
    {
        case CymbalSwap::Ride:      return Ride;
        case CymbalSwap::Bell:      return Bell;
        case CymbalSwap::Crash:     return Crash;
        case CymbalSwap::China:     return China;
        case CymbalSwap::HiHat:     return HiHat;
        case CymbalSwap::HiHatOpen: return HiHatOpen;
        case CymbalSwap::AsWritten:
        case CymbalSwap::NumChoices: break;
    }
    return -1;
}

// Snapshot of everything that shapes the output; taken per audio block (or per
// segment of a block, when trigger notes change it mid-block).
struct Settings
{
    int groove = 0;
    int fill = 0;
    bool randomFill = false;
    int fillEvery = 4;          // bars; 0 = no automatic fills
    int humanize = 6;           // +/- velocity spread
    int accent = 5;             // extra velocity on the beat
    float velocityScale = 1.0f;
    std::array<int, NumLanes> notes {};

    CymbalSwap cymbal = CymbalSwap::AsWritten;
    bool crashAfterFill = false;   // add a crash on the downbeat after each fill
    float chanceAmount = 1.0f;     // 0 = ignore per-step chances, 1 = apply them fully
    int kickBalance = 0;           // velocity taken off the second foot in kick runs
    float timingMs = 0.0f;         // +/- timing humanize

    double origin = 0.0;        // PPQ where the groove's bar 1 starts
    bool grooveMuted = false;   // hold mode with no key down: only forced fills play
    long long forcedFillBar = noBar;   // bar (relative to origin) that gets a fill from a fill key
    int forcedFill = 0;
};

inline long long floorDiv (long long a, long long b) noexcept
{
    const long long q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

inline long long floorMod (long long a, long long b) noexcept   { return a - floorDiv (a, b) * b; }

// splitmix64: deterministic "randomness" so playback, re-playback and export all match.
inline uint64_t mix (uint64_t x) noexcept
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Which fill (if any) ends the given bar; -1 = none.
inline int fillForBar (const Library& lib, const Settings& s, long long bar) noexcept
{
    if (bar == s.forcedFillBar)
        return juce::jlimit (0, (int) lib.fills.size() - 1, s.forcedFill);
    if (s.grooveMuted || s.fillEvery <= 0 || floorMod (bar, s.fillEvery) != s.fillEvery - 1)
        return -1;
    if (! s.randomFill)
        return s.fill;
    return (int) (mix ((uint64_t) bar ^ 0xF111ULL) % lib.fills.size());
}

// Timing of one groove bar, and where its fill (if any) takes over.
struct BarInfo
{
    long long bar;
    double start, fillStart, end;
    const Pattern* fill;
    Layout fillLayout;
};

inline BarInfo barInfo (const Library& lib, const Settings& s, const Layout& groove, long long bar) noexcept
{
    BarInfo b;
    b.bar = bar;
    b.start = s.origin + (double) bar * groove.barQuarters();
    b.end = b.start + groove.barQuarters();
    b.fillStart = b.end;
    b.fill = nullptr;

    if (const int f = fillForBar (lib, s, bar); f >= 0)
    {
        b.fill = lib.fills[(size_t) f].get();
        b.fillLayout = b.fill->layout();
        b.fillStart = b.end - juce::jmin (b.fillLayout.lengthQuarters(), groove.barQuarters());
    }
    return b;
}

inline long long barAt (const Settings& s, const Layout& groove, double ppq) noexcept
{
    return (long long) std::floor ((ppq - s.origin) / groove.barQuarters());
}

// The pattern and step sounding at a PPQ position (for the editor's playhead).
struct Location
{
    const Pattern* pattern = nullptr;
    int step = -1;
};

inline Location locate (const Library& lib, const Settings& s, double ppq) noexcept
{
    const auto& groove = *lib.grooves[(size_t) s.groove];
    const auto gl = groove.layout();
    const auto b = barInfo (lib, s, gl, barAt (s, gl, ppq));

    if (b.fill != nullptr && ppq >= b.fillStart)
        return { b.fill, (int) ((ppq - b.fillStart) / b.fillLayout.stepLength()) };
    if (s.grooveMuted)
        return {};
    const int stepInBar = (int) ((ppq - b.start) / gl.stepLength());
    return { &groove, (int) floorMod (b.bar, gl.bars) * gl.stepsPerBar() + stepInBar };
}

struct Hit
{
    double ppq;          // position on the grid
    int note;
    int velocity;
    double offsetMs;     // timing humanize, applied by the caller (depends on tempo)
};

// Calls emit(Hit) for every hit whose grid time (PPQ) lies in [start, end).
template <typename Emit>
void renderHits (const Library& lib, const Settings& s, double start, double end, Emit&& emit)
{
    const auto& groove = *lib.grooves[(size_t) s.groove];
    const auto gl = groove.layout();
    const double beat = gl.beatQuarters();
    const int swapLane = cymbalSwapLane (s.cymbal);

    auto random = [] (uint64_t key, int range)   // deterministic 0 .. range-1
    {
        return (int) (mix (key) % (uint64_t) juce::jmax (1, range));
    };

    auto playStep = [&] (const Pattern& p, bool isFill, int step, double t, double posInBar, uint64_t key, bool addCrash)
    {
        const double offBeat = std::fmod (posInBar, beat);
        const bool onBeat = offBeat < 1.0e-6 || beat - offBeat < 1.0e-6;
        const bool downbeat = posInBar < 1.0e-6;
        std::array<bool, 128> used {};   // two lanes mapped to the same note play it once

        auto hit = [&] (int lane, int v, uint64_t laneKey)
        {
            float vel = (float) v;
            if (onBeat && v >= 90)
                vel += (float) s.accent;

            // Second foot of a kick run is a little weaker.
            if (lane == Kick && s.kickBalance > 0)
            {
                int run = 0;
                while (run < 64 && step - run - 1 >= 0 && p.get (Kick, step - run - 1) > 0)
                    ++run;
                if (run % 2 == 1)
                    vel -= (float) s.kickBalance;
            }

            vel *= s.velocityScale;
            if (s.humanize > 0)
                vel += (float) (random (laneKey, 2 * s.humanize + 1) - s.humanize);

            double offsetMs = 0.0;
            if (s.timingMs > 0.0f)
            {
                const int range = juce::roundToInt (s.timingMs * 10.0f);
                offsetMs = (random (laneKey ^ 0x7111ULL, 2 * range + 1) - range) / 10.0;
            }

            const int note = s.notes[(size_t) lane];
            if (! juce::isPositiveAndBelow (note, 128) || used[(size_t) note])
                return;
            used[(size_t) note] = true;
            emit (Hit { t, note, juce::jlimit (1, 127, juce::roundToInt (vel)), offsetMs });
        };

        if (addCrash)
            hit (Crash, 115, key ^ 0xC4A5ULL);

        for (int lane = 0; lane < NumLanes; ++lane)
        {
            const int v = p.get (lane, step);
            if (v == 0)
                continue;

            const auto laneKey = key ^ ((uint64_t) lane << 56);
            if (const int chance = p.getChance (lane, step); chance < 100 && s.chanceAmount > 0.0f)
            {
                const float playProbability = 1.0f - (1.0f - (float) chance / 100.0f) * s.chanceAmount;
                if ((float) random (laneKey ^ 0xCA11ULL, 1000) >= playProbability * 1000.0f)
                    continue;
            }

            int outLane = lane;
            if (! isFill && swapLane >= 0 && isCymbalLane (lane) && ! ((lane == Crash || lane == Crash2) && downbeat))
                outLane = swapLane;

            hit (outLane, v, laneKey);
        }
    };

    const double eps = 1.0e-9;

    for (long long bar = barAt (s, gl, start) - 1; bar <= barAt (s, gl, end); ++bar)
    {
        const auto b = barInfo (lib, s, gl, bar);
        if (b.end <= start || b.start >= end)
            continue;

        if (! s.grooveMuted)
        {
            const int perBar = gl.stepsPerBar();
            const int first = (int) floorMod (bar, gl.bars) * perBar;
            const bool crashHere = s.crashAfterFill && groove.get (Crash, first) == 0
                                   && fillForBar (lib, s, bar - 1) >= 0;
            for (int k = 0; k < perBar; ++k)
            {
                const double t = b.start + k * gl.stepLength();
                if (t >= b.fillStart - eps || t >= end)
                    break;
                if (t >= start)
                    playStep (groove, false, first + k, t, t - b.start,
                              mix ((uint64_t) bar * 1315423911ULL ^ (uint64_t) k), k == 0 && crashHere);
            }
        }

        if (b.fill != nullptr)
        {
            for (int k = 0; k < b.fillLayout.numSteps(); ++k)
            {
                const double t = b.fillStart + k * b.fillLayout.stepLength();
                if (t >= b.end - eps || t >= end)
                    break;
                if (t >= start)
                    playStep (*b.fill, true, k, t, t - b.start,
                              mix ((uint64_t) bar * 2654435761ULL ^ ((uint64_t) k << 32) ^ 0xF1ULL), false);
            }
        }
    }
}

} // namespace bm
