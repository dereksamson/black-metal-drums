#pragma once

#include "Engine.h"

namespace bm
{

// One section of a song: a groove for some bars, with optional fills.
struct Section
{
    static constexpr int noFill = -1, randomFill = -2;

    int groove = 0;
    int bars = 4;
    int fillEvery = 0;        // bars, 0 = none (inside the section)
    int endFill = noFill;     // fill on the section's last bar: fill index, noFill or randomFill

    uint32_t pack() const noexcept
    {
        return (uint32_t) groove | ((uint32_t) bars << 8) | ((uint32_t) fillEvery << 16) | ((uint32_t) (endFill + 2) << 24);
    }
    static Section unpack (uint32_t v) noexcept
    {
        return { (int) (v & 0xff), (int) ((v >> 8) & 0xff), (int) ((v >> 16) & 0xff), (int) ((v >> 24) & 0xff) - 2 };
    }
};

// Sections are edited on the message thread and read on the audio thread; each one is a
// single atomic word, so a reader never sees a half-written section.
class Song
{
public:
    static constexpr int maxSections = 64;

    int size() const noexcept                   { return numSections.load (std::memory_order_acquire); }
    Section get (int i) const noexcept          { return Section::unpack (sections[(size_t) i].load (std::memory_order_relaxed)); }
    void set (int i, const Section& s) noexcept { sections[(size_t) i].store (s.pack(), std::memory_order_relaxed); }

    std::vector<Section> all() const
    {
        std::vector<Section> out;
        for (int i = 0; i < size(); ++i)
            out.push_back (get (i));
        return out;
    }

    void assign (const std::vector<Section>& list) noexcept
    {
        const int n = juce::jmin ((int) list.size(), maxSections);
        numSections.store (juce::jmin (n, size()), std::memory_order_release);   // shrink first
        for (int i = 0; i < n; ++i)
            set (i, list[(size_t) i]);
        numSections.store (n, std::memory_order_release);
    }

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("Song");
        for (const auto& s : all())
        {
            juce::ValueTree section ("Section");
            section.setProperty ("groove", s.groove, nullptr);
            section.setProperty ("bars", s.bars, nullptr);
            section.setProperty ("fillEvery", s.fillEvery, nullptr);
            section.setProperty ("endFill", s.endFill, nullptr);
            tree.appendChild (section, nullptr);
        }
        return tree;
    }

    void fromValueTree (const juce::ValueTree& tree, int numGrooves, int numFills)
    {
        std::vector<Section> list;
        for (const auto& section : tree)
            list.push_back ({ juce::jlimit (0, numGrooves - 1, (int) section.getProperty ("groove", 0)),
                              juce::jlimit (1, 255, (int) section.getProperty ("bars", 4)),
                              juce::jlimit (0, 8, (int) section.getProperty ("fillEvery", 0)),
                              juce::jlimit (Section::randomFill, numFills - 1, (int) section.getProperty ("endFill", Section::noFill)) });
        assign (list);
    }

private:
    std::array<std::atomic<uint32_t>, maxSections> sections {};
    std::atomic<int> numSections { 0 };
};

// Where each section sits in time. Fixed capacity: built on the audio thread without allocating.
struct PlacedSection
{
    Section section;
    double start = 0.0, end = 0.0;
};

struct Placement
{
    std::array<PlacedSection, Song::maxSections> items {};
    int count = 0;
    double length = 0.0;

    const PlacedSection& operator[] (int i) const noexcept { return items[(size_t) i]; }
};

inline Placement placeSections (const Library& lib, const Song& song)
{
    Placement p;
    double t = 0.0;
    for (int i = 0; i < song.size(); ++i)
    {
        const auto s = song.get (i);
        const auto& groove = *lib.grooves[(size_t) juce::jlimit (0, (int) lib.grooves.size() - 1, s.groove)];
        const double len = juce::jmax (1, s.bars) * groove.layout().barQuarters();
        p.items[(size_t) i] = { s, t, t + len };
        t += len;
        ++p.count;
    }
    p.length = t;
    return p;
}

// Settings for playing one section: the base settings with its groove, fills and origin.
inline Settings sectionSettings (const Library& lib, const Settings& base, const Placement& placed, int i, double cycleOffset)
{
    const auto& p = placed[i];
    Settings s = base;
    s.groove = juce::jlimit (0, (int) lib.grooves.size() - 1, p.section.groove);
    s.origin = p.start + cycleOffset;
    s.fillEvery = p.section.fillEvery;
    s.randomFill = false;
    s.grooveMuted = false;
    s.forcedFillBar = noBar;
    s.crashOnFirstBar = false;

    if (p.section.endFill != Section::noFill)
    {
        s.forcedFillBar = p.section.bars - 1;
        s.forcedFill = p.section.endFill == Section::randomFill
                           ? (int) (mix ((uint64_t) i * 7919ULL ^ 0x50ULL) % lib.fills.size())
                           : juce::jlimit (0, (int) lib.fills.size() - 1, p.section.endFill);
    }

    // The crash after a fill also follows a fill that ends the previous section.
    if (i > 0)
    {
        const auto& prev = placed[i - 1].section;
        s.crashOnFirstBar = prev.endFill != Section::noFill || (prev.fillEvery > 0 && prev.bars % prev.fillEvery == 0);
    }
    return s;
}

// Index of the section playing at a PPQ position, or -1 (before the start, or past the end
// of a song that doesn't loop). cycleOffset receives the start of the current loop cycle.
inline int sectionAt (const Placement& placed, double ppq, bool loop, double& cycleOffset)
{
    cycleOffset = 0.0;
    if (placed.length <= 0.0 || ppq < 0.0)
        return -1;
    if (loop)
    {
        cycleOffset = std::floor (ppq / placed.length) * placed.length;
        ppq -= cycleOffset;
    }
    for (int i = 0; i < placed.count; ++i)
        if (ppq >= placed[i].start && ppq < placed[i].end)
            return i;
    return -1;
}

// Calls emit(Hit) for every hit of the song whose grid time lies in [start, end).
template <typename Emit>
void renderSong (const Library& lib, const Settings& base, const Placement& placed, bool loop,
                 double start, double end, Emit&& emit)
{
    if (placed.length <= 0.0 || end <= 0.0)
        return;

    const auto firstCycle = loop ? (long long) std::floor (juce::jmax (0.0, start) / placed.length) : 0LL;
    const auto lastCycle = loop ? (long long) std::floor (end / placed.length) : 0LL;

    for (auto cycle = firstCycle; cycle <= lastCycle; ++cycle)
    {
        const double offset = (double) cycle * placed.length;
        for (int i = 0; i < placed.count; ++i)
        {
            const double from = juce::jmax (start, placed[i].start + offset);
            const double to = juce::jmin (end, placed[i].end + offset);
            if (to > from)
                renderHits (lib, sectionSettings (lib, base, placed, i, offset), from, to, emit);
        }
    }
}

} // namespace bm
