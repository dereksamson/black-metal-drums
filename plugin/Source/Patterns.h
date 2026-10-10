#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace bm
{

// Drum lanes, top to bottom as shown in the grid.
enum Lane { Crash, Crash2, China, Ride, Bell, HiHat, HiHatOpen, Snare, Tom1, Tom2, Tom3, Floor, Kick, NumLanes };

inline bool isCymbalLane (int lane) noexcept    { return lane <= HiHatOpen; }

constexpr int maxSteps = 192;   // 4 bars of 12/8 at 1/32

extern const char* const laneNames[NumLanes];
extern const char* const laneIds[NumLanes];
extern const int defaultNotes[NumLanes];   // General MIDI

enum class Resolution : int { Eighth, EighthTriplet, Sixteenth, SixteenthTriplet, ThirtySecond, NumResolutions };

double stepQuarters (Resolution) noexcept;
const char* resolutionName (Resolution) noexcept;

// Meter, length and grid resolution of a pattern. All times are in quarter notes (host PPQ).
// A fill is a single "bar" whose meter is its length, e.g. 2/8 = one quarter note.
struct Layout
{
    int num = 4, den = 4, bars = 2;
    Resolution res = Resolution::Sixteenth;

    double barQuarters() const noexcept     { return num * 4.0 / den; }
    double stepLength() const noexcept      { return stepQuarters (res); }
    double lengthQuarters() const noexcept  { return barQuarters() * bars; }
    int stepsPerBar() const noexcept        { return (int) std::floor (barQuarters() / stepLength() + 1.0e-6); }
    int numSteps() const noexcept           { return stepsPerBar() * bars; }

    // Pulse used for beat accents and grid shading: dotted quarter in compound meters.
    double beatQuarters() const noexcept    { return (den == 8 && num % 3 == 0) ? 1.5 : 1.0; }

    // The resolution must divide the bar exactly and everything must fit in maxSteps.
    bool isValid() const noexcept
    {
        const double steps = barQuarters() / stepLength();
        return num > 0 && bars > 0 && std::abs (steps - std::round (steps)) < 1.0e-6 && numSteps() <= maxSteps;
    }

    uint32_t pack() const noexcept          { return (uint32_t) num | ((uint32_t) den << 8) | ((uint32_t) bars << 16) | ((uint32_t) res << 24); }
    static Layout unpack (uint32_t v) noexcept
    {
        return { (int) (v & 0xff), (int) ((v >> 8) & 0xff), (int) ((v >> 16) & 0xff), (Resolution) ((v >> 24) & 0xff) };
    }
    bool operator== (const Layout& o) const noexcept { return pack() == o.pack(); }
    bool operator!= (const Layout& o) const noexcept { return pack() != o.pack(); }
};

// Grid notation (same as generate.py):
//   .  rest   x  normal   X  accent   g  ghost   1-9  velocity level   ?  normal, 50% chance
// Spaces and '|' are ignored. Each lane has layout.numSteps() cells.
struct PatternDef
{
    const char* name;
    Layout layout;
    std::vector<std::pair<Lane, const char*>> lanes;
};

// A pattern whose cells and layout are edited on the message thread and read on the
// audio thread. Everything is atomic, so no locking is needed; a reader may briefly see a
// half-applied edit, which at worst changes one block of output.
class Pattern
{
public:
    explicit Pattern (const PatternDef&);

    const juce::String name;

    Layout layout() const noexcept                  { return Layout::unpack (packedLayout.load (std::memory_order_acquire)); }

    // Changes meter/length/resolution, moving existing hits to the nearest new step.
    // Extra bars are filled by repeating the existing ones.
    void setLayout (const Layout&);

    int get (int lane, int step) const noexcept
    {
        return juce::isPositiveAndBelow (step, maxSteps) ? cells[(size_t) lane][(size_t) step].load (std::memory_order_relaxed) : 0;
    }
    void set (int lane, int step, int velocity) noexcept
    {
        if (juce::isPositiveAndBelow (step, maxSteps))
            cells[(size_t) lane][(size_t) step].store ((uint8_t) juce::jlimit (0, 127, velocity), std::memory_order_relaxed);
    }

    // Chance (0-100 %) that a hit plays; 100 for every cell unless edited.
    int getChance (int lane, int step) const noexcept
    {
        return juce::isPositiveAndBelow (step, maxSteps) ? chances[(size_t) lane][(size_t) step].load (std::memory_order_relaxed) : 100;
    }
    void setChance (int lane, int step, int percent) noexcept
    {
        if (juce::isPositiveAndBelow (step, maxSteps))
            chances[(size_t) lane][(size_t) step].store ((uint8_t) juce::jlimit (0, 100, percent), std::memory_order_relaxed);
    }

    void reset();   // back to the factory pattern and layout
    void clear();

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);

private:
    const PatternDef& def;
    std::atomic<uint32_t> packedLayout;
    std::array<std::array<std::atomic<uint8_t>, maxSteps>, NumLanes> cells, chances;
};

struct Library
{
    Library();

    std::vector<std::unique_ptr<Pattern>> grooves, fills;

    juce::StringArray grooveNames() const;
    juce::StringArray fillNames() const;
};

// Problems in the factory pattern definitions (wrong grid lengths etc.); empty when all is well.
juce::StringArray factoryPatternErrors();

} // namespace bm
