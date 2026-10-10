#include "Patterns.h"

namespace bm
{

const char* const laneNames[NumLanes] = { "Crash", "Crash 2", "China", "Ride", "Ride bell", "Hi-hat", "Hi-hat open",
                                          "Snare", "Tom 1", "Tom 2", "Tom 3", "Floor tom", "Kick" };
const char* const laneIds[NumLanes]   = { "crash", "crash2", "china", "ride", "bell", "hh", "hh_open",
                                          "snare", "tom1", "tom2", "tom3", "floor", "kick" };
const int defaultNotes[NumLanes]      = { 49, 57, 52, 51, 53, 42, 46, 38, 50, 48, 47, 43, 36 };

double stepQuarters (Resolution r) noexcept
{
    switch (r)
    {
        case Resolution::Eighth:           return 0.5;
        case Resolution::EighthTriplet:    return 1.0 / 3.0;
        case Resolution::Sixteenth:        return 0.25;
        case Resolution::SixteenthTriplet: return 1.0 / 6.0;
        case Resolution::ThirtySecond:     return 0.125;
        case Resolution::NumResolutions:   break;
    }
    return 0.25;
}

const char* resolutionName (Resolution r) noexcept
{
    switch (r)
    {
        case Resolution::Eighth:           return "1/8";
        case Resolution::EighthTriplet:    return "1/8 T";
        case Resolution::Sixteenth:        return "1/16";
        case Resolution::SixteenthTriplet: return "1/16 T";
        case Resolution::ThirtySecond:     return "1/32";
        case Resolution::NumResolutions:   break;
    }
    return "?";
}

namespace
{
    constexpr auto r8   = Resolution::Eighth;
    constexpr auto r8t  = Resolution::EighthTriplet;
    constexpr auto r16  = Resolution::Sixteenth;
    constexpr auto r16t = Resolution::SixteenthTriplet;
    constexpr auto r32  = Resolution::ThirtySecond;

    // 4/4 grooves are ported from generate.py. Grooves are 2 bars.
    const std::vector<PatternDef>& grooveDefs()
    {
        static const std::vector<PatternDef> defs {
            { "Blast - Traditional (ride)", { 4, 4, 2, r16 }, {
                { Crash, "x... .... .... .... | .... .... .... ...." },
                { Ride,  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x" } } },
            { "Blast - Traditional (crash)", { 4, 4, 2, r16 }, {
                { Crash, "X.x. x.x. x.x. x.x. | X.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x" } } },
            { "Blast - Traditional (hi-hat)", { 4, 4, 2, r16 }, {
                { Crash, "x... .... .... .... | .... .... .... ...." },
                { HiHat, "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x" } } },
            { "Blast - Hammer (crash)", { 4, 4, 2, r16 }, {
                { Crash, "X.x. x.x. x.x. x.x. | X.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." } } },
            { "Blast - Hammer (china)", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { China, "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." } } },
            { "Blast - Bomb", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | X... x... x... x..." },
                { Kick,  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx" },
                { Snare, "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." } } },
            { "Skank beat", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { HiHat, "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x... x... x... x... | x... x... x... x..." },
                { Snare, "..x. ..x. ..x. ..x. | ..x. ..x. ..x. ..x." } } },
            { "Double kick - 16ths", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | X... x... x... x..." },
                { Kick,  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx" },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },
            { "Double kick - Gallop", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { Ride,  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.xx x.xx x.xx x.xx | x.xx x.xx x.xx x.xx" },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },
            { "Mid-tempo march", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | X... x... x... x..." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },
            { "Half-time atmospheric", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { Ride,  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x... ..x. .... .... | x... ..x. x... ..x." },
                { Snare, ".... .... X... .... | .... .... X... ...g" } } },

            // 6/8: grouped by dotted quarter (6 sixteenths).
            { "6/8 - Tremolo groove", { 6, 8, 2, r16 }, {
                { Crash, "X..... ...... | ...... ......" },
                { Ride,  "..x.x. x.x.x. | x.x.x. x.x.x." },
                { Kick,  "x.x.x. x.x.x. | x.x.x. x.x.x." },
                { Snare, "...... X..... | ...... X....." } } },
            { "6/8 - Blast", { 6, 8, 2, r16 }, {
                { Crash, "X..... ...... | ...... ......" },
                { Ride,  "..x.x. x.x.x. | x.x.x. x.x.x." },
                { Kick,  "x.x.x. x.x.x. | x.x.x. x.x.x." },
                { Snare, ".x.x.x .x.x.x | .x.x.x .x.x.x" } } },
            { "6/8 - Double kick", { 6, 8, 2, r16 }, {
                { Crash, "X..... x..... | X..... x....." },
                { Kick,  "xxxxxx xxxxxx | xxxxxx xxxxxx" },
                { Snare, "...... X..... | ...... X....." } } },

            // 3/4
            { "3/4 - Hammer blast", { 3, 4, 2, r16 }, {
                { Crash, "X.x. x.x. x.x. | X.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. | x.x. x.x. x.x." },
                { Snare, "x.x. x.x. x.x. | x.x. x.x. x.x." } } },
            { "3/4 - Epic (Bathory-style)", { 3, 4, 2, r16 }, {
                { Crash, "X... .... .... | x... .... ...." },
                { Ride,  "..x. x.x. x.x. | ..x. x.x. x.x." },
                { Kick,  "x... .... x... | x... .... x.x." },
                { Snare, ".... X... .... | .... X... ...." } } },

            // More 4/4 black metal grooves.
            { "Blast - Gravity (approx.)", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | x... .... .... ...." },
                { Kick,  "x... x... x... x... | x... x... x... x..." },
                { Snare, "X6X6 X6X6 X6X6 X6X6 | X6X6 X6X6 X6X6 X6X6" } } },
            { "Blast - Ride bell", { 4, 4, 2, r16 }, {
                { Crash, "x... .... .... .... | .... .... .... ...." },
                { Bell,  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Snare, ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x" } } },
            { "Blast - Bomb (china)", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { China, ".... x... x... x... | x... x... x... x..." },
                { Kick,  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx" },
                { Snare, "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." } } },
            { "Half-time double kick", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { Ride,  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx" },
                { Snare, ".... .... X... .... | .... .... X... ...." } } },
            { "D-beat", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { HiHat, "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x." },
                { Kick,  "x.x. ..x. x.x. ..x. | x.x. ..x. x.x. ..x." },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },
            { "Mid-tempo - Variable kicks", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | X... x... x... x..." },
                { Kick,  "x.x? x.x. x.x? x.x. | x.x? x.x. x.x? x.?." },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },

            // 12/8: grouped by dotted quarter.
            { "12/8 - Atmospheric", { 12, 8, 2, r16 }, {
                { Crash, "X..... ...... ...... ...... | ...... ...... ...... ......" },
                { Ride,  "..x.x. x.x.x. x.x.x. x.x.x. | x.x.x. x.x.x. x.x.x. x.x.x." },
                { Kick,  "x..... ...... x..... ....x. | x..... ...... x..... ......" },
                { Snare, "...... X..... ...... X..... | ...... X..... ...... X....." } } },

            // Iconic and unusual metal grooves (in the style of; not transcriptions).
            { "Thrash beat - Slayer-style", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | x... x... x... x..." },
                { Kick, "x... x... x... x... | x... x... x..x x..x" },
                { Snare, "..x. ..x. ..x. ..x. | ..x. ..x. ..x. ..x." } } },
            { "Gallop - Maiden-style", { 4, 4, 2, r16 }, {
                { Crash, "X... x... x... x... | x... x... x... x..." },
                { Kick, "x.xx ..xx x.xx ..xx | x.xx ..xx x.xx ..xx" },
                { Snare, ".... X... .... X... | .... X... .... X..." } } },
            { "Tribal toms - Sepultura-style", { 4, 4, 2, r16 }, {
                { Crash, "X... .... .... .... | .... .... .... ...." },
                { Floor, "X.xx x.X. X.xx x.X. | X.xx x.X. X.xx x.xx" },
                { Snare, ".... .... X... .... | .... .... X... ...." },
                { Kick, "x... ..x. x... ..x. | x... ..x. x... ..x." } } },
            { "Polymeter - kicks in 3s (Meshuggah-style)", { 4, 4, 3, r16 }, {
                { Crash, "X..............................................." },
                { China, "....x...x...x...x...x...x...x...x...x...x...x..." },
                { Kick, "xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx." },
                { Snare, "........X...............X...............X......." } } },
            { "Polymeter - 5/16 over 4/4", { 4, 4, 5, r16 }, {
                { Crash, "X..............................................................................." },
                { China, ".....x....x....x....x....x....x....x....x....x....x....x....x....x....x....x...." },
                { Kick, "x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx.x.xx." },
                { Snare, "........X...............X...............X...............X...............X......." } } },

            // Ternary: triplet grids in 4/4, and 12/8.
            { "Shuffle double bass - Hot for Teacher-style", { 4, 4, 2, r8t }, {
                { Crash, "x.. ... ... ... | ... ... ... ..." },
                { Bell, ".x. xx. xx. xx. | xx. xx. xx. xx." },
                { Kick, "x.x x.x x.x x.x | x.x x.x x.x x.x" },
                { Snare, "... x.. ... x.. | ... x.. ... x.." } } },
            { "Doom shuffle - Sabbath-style", { 4, 4, 2, r8t }, {
                { Crash, "X.. ... ... ... | ... ... ... ..." },
                { Ride, "..x x.x x.x x.x | x.x x.x x.x x.x" },
                { Kick, "x.. ..x x.. ... | x.. ..x x.. ..x" },
                { Snare, "... X.. ... X.. | ... X.. ... X.g" } } },
            { "Triplet breakdown", { 4, 4, 2, r8t }, {
                { Crash, "X.. ... ... ... | ... ... ... ..." },
                { China, "... x.. x.. x.. | x.. x.. x.. x.." },
                { Kick, "x.x .xx x.. xx. | x.x .xx x.x xxx" },
                { Snare, "... ... X.. ... | ... ... X.. ..." } } },
            { "Sextuplet double kick - Hoglan-style", { 4, 4, 2, r16t }, {
                { Crash, "X..... x..... x..... x..... | x..... x..... x..... x....." },
                { Kick, "xxxxxx xxxxxx xxxxxx xxxxxx | xxxxxx xxxxxx xxxxxx xxxxxx" },
                { Snare, "...... X..... ...... X..... | ...... X..... ...... X....." } } },
            { "12/8 - Tom shuffle (Running Free-style)", { 12, 8, 2, r8 }, {
                { Crash, "X.. ... ... ... | ... ... ... ..." },
                { Floor, "x.x x.x x.x x.x | x.x x.x x.x x.x" },
                { Kick, "x.. ... x.. ... | x.. ... x.. ..x" },
                { Snare, "... X.. ... X.. | ... X.. ... X.." } } },
            { "12/8 - Half-time groove shuffle (Pantera-style)", { 12, 8, 2, r8 }, {
                { Crash, "X.. ... ... ... | ... ... ... ..." },
                { HiHat, "..x x.x x.x x.x | x.x x.x x.x x.x" },
                { Kick, "x.. ..x ... ..x | x.. ..x x.. ..." },
                { Snare, "... ... X.. ... | ... ... X.. ..." } } },
            { "12/8 - Triplet double kick (Mgla-style)", { 12, 8, 2, r8 }, {
                { Crash, "X.. ... ... ... | ... ... ... ..." },
                { Ride, "... x.. x.. x.. | x.. x.. x.. x.." },
                { Kick, "xxx xxx xxx xxx | xxx xxx xxx xxx" },
                { Snare, "... X.. ... X.. | ... X.. ... X.." } } },
        };
        return defs;
    }

    // Fills are one "bar" whose meter is the fill length (2/8 = one beat).
    const std::vector<PatternDef>& fillDefs()
    {
        static const std::vector<PatternDef> defs {
            { "Snare 16ths",                { 2, 8, 1, r16 },  { { Snare, "579X" } } },
            { "Snare 32nds crescendo",      { 2, 8, 1, r32 },  { { Snare, "45566789" }, { Kick, "x...x..." } } },
            { "Snare + kick unison",        { 2, 8, 1, r16 },  { { Snare, "xxxX" }, { Kick, "xxxx" } } },
            { "Toms 16ths",                 { 2, 8, 1, r16 },  { { Tom1, "x..." }, { Tom2, ".x.." }, { Tom3, "..x." }, { Floor, "...X" } } },
            { "Toms 32nds",                 { 2, 8, 1, r32 },  { { Tom1, "xx......" }, { Tom2, "..xx...." }, { Tom3, "....xx.." },
                                                                 { Floor, "......xX" }, { Kick, "x.x.x.x." } } },
            { "Snare to floor",             { 2, 8, 1, r32 },  { { Snare, "xxxx...." }, { Tom2, "....xx.." }, { Floor, "......xX" },
                                                                 { Kick, "x...x..." } } },
            { "Triplet toms",               { 2, 8, 1, r8t },  { { Snare, "x.." }, { Tom2, ".x." }, { Floor, "..X" }, { Kick, "x.." } } },
            { "Sextuplet (hand-hand-foot)", { 2, 8, 1, r16t }, { { Snare, "x....." }, { Tom1, ".x...." }, { Tom3, "...x.." },
                                                                 { Floor, "....X." }, { Kick, "..x..x" } } },
            { "Unison stabs",               { 2, 8, 1, r16 },  { { Crash, "x.xX" }, { Kick, "x.xx" }, { Snare, "x.xX" } } },
            { "Toms 3/8 (6/8 beat)",        { 3, 8, 1, r16 },  { { Snare, "xx...." }, { Tom1, "..x..." }, { Tom2, "...x.." },
                                                                 { Floor, "....xX" }, { Kick, "x.x.x." } } },
            { "Snare roll half bar",        { 4, 8, 1, r16 },  { { Snare, "3455 6789" }, { Kick, "x... x..." } } },
            { "Tom cascade full bar",       { 8, 8, 1, r16 },  { { Snare, "xxxx .... .... ...." }, { Tom1, ".... xxxx .... ...." },
                                                                 { Tom2, ".... .... xxxx ...." }, { Floor, ".... .... .... xxxX" },
                                                                 { Kick, "x.x. x.x. x.x. x.x." } } },
            { "Gravity roll",               { 2, 8, 1, r32 },  { { Snare, "X6X6X7X9" } } },
            { "Blast burst 32nds",          { 2, 8, 1, r32 },  { { Snare, ".x.x.x.X" }, { Kick, "x.x.x.x." } } },
            { "Stabs half bar",             { 4, 8, 1, r16 },  { { Crash, "x.x.X..." }, { Kick, "x.x.X..." }, { Snare, "x.x.X..." } } },

            // Classic metal fills, triplet fills and odd groupings.
            { "Hand-hand-foot triplets - full bar", { 8, 8, 1, r8t }, {
                { Snare, "x.. ... ... ..." },
                { Tom1, ".x. x.. ... ..." },
                { Tom2, "... .x. x.. ..." },
                { Tom3, "... ... .x. ..." },
                { Floor, "... ... ... xX." },
                { Kick, "..x ..x ..x ..x" } } },
            { "Triplet tom run - half bar", { 4, 8, 1, r8t }, {
                { Snare, "x.. ..." },
                { Tom1, ".x. ..." },
                { Tom2, "..x ..." },
                { Tom3, "... x.." },
                { Floor, "... .xX" },
                { Kick, "x.. x.." } } },
            { "Linear fill - half bar", { 4, 8, 1, r16 }, {
                { Snare, "x..x ...." },
                { Tom1, ".x.. ...." },
                { Tom2, ".... x..." },
                { Floor, ".... ..X." },
                { Kick, "..x. .x.x" } } },
            { "32nd roll around the kit - full bar", { 8, 8, 1, r32 }, {
                { Snare, "xxxxxxxx........................" },
                { Tom1, "........xxxxxxxx................" },
                { Tom2, "................xxxxxxxx........" },
                { Floor, "........................xxxxxxxX" },
                { Kick, "x...x...x...x...x...x...x...x..." } } },
            { "Groups of five - full bar", { 8, 8, 1, r16 }, {
                { Snare, "xx... xx... xx... ." },
                { Tom1, "..xx. ..... ..... ." },
                { Tom2, "..... ..xx. ..... ." },
                { Floor, "..... ..... ..xx. X" },
                { Kick, "....x ....x ....x x" } } },
            { "Triplet unison stabs - half bar", { 4, 8, 1, r8t }, {
                { Crash, "x.x x.X" },
                { Kick, "x.x x.X" },
                { Snare, "x.x x.X" } } },
            { "Triplet toms 3/8 (12/8 beat)", { 3, 8, 1, r8 }, {
                { Snare, "x.." },
                { Tom2, ".x." },
                { Floor, "..X" },
                { Kick, "x.x" } } },
            { "Sextuplet burst - Hoglan-style", { 2, 8, 1, r16t }, {
                { Snare, "x.x.x." },
                { Kick, ".x.x.X" } } },
        };
        return defs;
    }

    juce::String stripGrid (const char* grid)
    {
        return juce::String (grid).removeCharacters (" |");
    }

    int velocityFor (juce::juce_wchar c)
    {
        switch (c)
        {
            case 'x':
            case '?': return 100;
            case 'X': return 120;
            case 'g': return 45;
            default:  break;
        }
        if (c >= '1' && c <= '9')
            return 30 + (int) (c - '1') * 12;   // 1 = 30 ... 9 = 126
        return 0;
    }

    using CellSnapshot = std::array<std::array<uint8_t, maxSteps>, NumLanes>;

    juce::String toHex (const std::vector<uint8_t>& bytes)
    {
        return juce::String::toHexString (bytes.data(), (int) bytes.size(), 0);
    }
} // namespace

juce::StringArray factoryPatternErrors()
{
    juce::StringArray errors;
    for (auto* defs : { &grooveDefs(), &fillDefs() })
        for (auto& d : *defs)
        {
            if (! d.layout.isValid())
                errors.add (juce::String (d.name) + ": invalid layout");
            for (auto& [lane, grid] : d.lanes)
                if (stripGrid (grid).length() != d.layout.numSteps())
                    errors.add (juce::String (d.name) + " / " + laneNames[lane] + ": " + juce::String (stripGrid (grid).length())
                                + " steps, expected " + juce::String (d.layout.numSteps()));
        }
    return errors;
}

Pattern::Pattern (const PatternDef& d)
    : name (d.name), def (d), packedLayout (d.layout.pack())
{
    jassert (d.layout.isValid());
    for ([[maybe_unused]] auto& lane : d.lanes)
        jassert (stripGrid (lane.second).length() == d.layout.numSteps());
    reset();
}

void Pattern::clear()
{
    for (int lane = 0; lane < NumLanes; ++lane)
        for (int step = 0; step < maxSteps; ++step)
        {
            set (lane, step, 0);
            setChance (lane, step, 100);
        }
}

void Pattern::reset()
{
    clear();
    packedLayout.store (def.layout.pack(), std::memory_order_release);
    for (auto& [lane, grid] : def.lanes)
    {
        const auto text = stripGrid (grid);
        for (int step = 0; step < juce::jmin (maxSteps, text.length()); ++step)
        {
            set (lane, step, velocityFor (text[step]));
            if (text[step] == '?')
                setChance (lane, step, 50);
        }
    }
}

void Pattern::setLayout (const Layout& newLayout)
{
    const auto old = layout();
    if (newLayout == old || ! newLayout.isValid())
        return;

    CellSnapshot result {}, resultChance;
    for (auto& lane : resultChance)
        lane.fill (100);
    const int oldPerBar = old.stepsPerBar(), newPerBar = newLayout.stepsPerBar();
    const int keptBars = juce::jmin (old.bars, newLayout.bars);

    for (int lane = 0; lane < NumLanes; ++lane)
    {
        for (int step = 0; step < old.numSteps(); ++step)
        {
            const int v = get (lane, step);
            const int bar = step / oldPerBar;
            if (v == 0 || bar >= keptBars)
                continue;

            const double posInBar = (step % oldPerBar) * old.stepLength();
            const int newStep = (int) std::round (posInBar / newLayout.stepLength());
            if (newStep >= newPerBar)
                continue;

            const auto dest = (size_t) (bar * newPerBar + newStep);
            auto& cell = result[(size_t) lane][dest];
            if (v > cell)
            {
                cell = (uint8_t) v;
                resultChance[(size_t) lane][dest] = (uint8_t) getChance (lane, step);
            }
        }

        for (int bar = keptBars; bar < newLayout.bars; ++bar)   // repeat existing bars
            for (int s = 0; s < newPerBar; ++s)
            {
                const auto from = (size_t) ((bar % keptBars) * newPerBar + s), to = (size_t) (bar * newPerBar + s);
                result[(size_t) lane][to] = result[(size_t) lane][from];
                resultChance[(size_t) lane][to] = resultChance[(size_t) lane][from];
            }
    }

    for (int lane = 0; lane < NumLanes; ++lane)
        for (int step = 0; step < maxSteps; ++step)
        {
            set (lane, step, result[(size_t) lane][(size_t) step]);
            setChance (lane, step, resultChance[(size_t) lane][(size_t) step]);
        }
    packedLayout.store (newLayout.pack(), std::memory_order_release);
}

juce::ValueTree Pattern::toValueTree() const
{
    const auto l = layout();
    juce::ValueTree tree ("Pattern");
    tree.setProperty ("name", name, nullptr);
    tree.setProperty ("num", l.num, nullptr);
    tree.setProperty ("den", l.den, nullptr);
    tree.setProperty ("bars", l.bars, nullptr);
    tree.setProperty ("res", (int) l.res, nullptr);

    for (int lane = 0; lane < NumLanes; ++lane)
    {
        std::vector<uint8_t> bytes ((size_t) l.numSteps()), chanceBytes ((size_t) l.numSteps());
        bool usesChance = false;
        for (int step = 0; step < l.numSteps(); ++step)
        {
            bytes[(size_t) step] = (uint8_t) get (lane, step);
            chanceBytes[(size_t) step] = (uint8_t) getChance (lane, step);
            usesChance = usesChance || chanceBytes[(size_t) step] != 100;
        }
        tree.setProperty (laneIds[lane], toHex (bytes), nullptr);
        if (usesChance)
            tree.setProperty (juce::String (laneIds[lane]) + "_chance", toHex (chanceBytes), nullptr);
    }
    return tree;
}

void Pattern::fromValueTree (const juce::ValueTree& tree)
{
    // Older saves have no layout properties: they used the factory layout.
    Layout l { (int) tree.getProperty ("num", def.layout.num), (int) tree.getProperty ("den", def.layout.den),
               (int) tree.getProperty ("bars", def.layout.bars),
               (Resolution) (int) tree.getProperty ("res", (int) def.layout.res) };
    if (! juce::isPositiveAndBelow ((int) l.res, (int) Resolution::NumResolutions) || ! l.isValid())
        l = def.layout;

    clear();
    for (int lane = 0; lane < NumLanes; ++lane)
    {
        juce::MemoryBlock bytes;
        bytes.loadFromHexString (tree.getProperty (laneIds[lane]).toString());
        for (int step = 0; step < juce::jmin (l.numSteps(), (int) bytes.getSize()); ++step)
            set (lane, step, (int) (uint8_t) bytes[(size_t) step]);

        juce::MemoryBlock chanceBytes;
        chanceBytes.loadFromHexString (tree.getProperty (juce::String (laneIds[lane]) + "_chance").toString());
        for (int step = 0; step < juce::jmin (l.numSteps(), (int) chanceBytes.getSize()); ++step)
            setChance (lane, step, (int) (uint8_t) chanceBytes[(size_t) step]);
    }
    packedLayout.store (l.pack(), std::memory_order_release);
}

Library::Library()
{
    for (auto& d : grooveDefs()) grooves.push_back (std::make_unique<Pattern> (d));
    for (auto& d : fillDefs())   fills.push_back (std::make_unique<Pattern> (d));
}

juce::StringArray Library::grooveNames() const
{
    juce::StringArray names;
    for (auto& p : grooves) names.add (p->name);
    return names;
}

juce::StringArray Library::fillNames() const
{
    juce::StringArray names;
    for (auto& p : fills) names.add (p->name);
    return names;
}

} // namespace bm
