// Headless checks for the pattern engine.
#include "../Source/Engine.h"
#include "../Source/FillGenerator.h"
#include "../Source/EditTools.h"
#include "../Source/Song.h"
#include <cstdio>

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        if (! ok)
        {
            std::printf ("FAIL %s\n", what.toRawUTF8());
            ++failures;
        }
    }

    std::vector<std::pair<double, int>> render (const bm::Library& lib, const bm::Settings& s, double from, double to)
    {
        std::vector<std::pair<double, int>> hits;
        bm::renderHits (lib, s, from, to, [&] (const bm::Hit& h) { hits.push_back ({ h.ppq, h.note }); });
        return hits;
    }

    // Render in random audio-block-sized chunks, as processBlock does.
    std::vector<std::pair<double, int>> renderChunked (const bm::Library& lib, const bm::Settings& s, double to, juce::Random& rng)
    {
        std::vector<std::pair<double, int>> hits;
        const double ppqPerSample = 200.0 / (60.0 * 48000.0);
        for (double t = 0.0; t < to;)
        {
            const double end = std::min (to, t + (16 + rng.nextInt (1024)) * ppqPerSample);
            bm::renderHits (lib, s, t, end, [&] (const bm::Hit& h) { hits.push_back ({ h.ppq, h.note }); });
            t = end;
        }
        return hits;
    }
}

int main()
{
    bm::Library lib;
    bm::Settings s;
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        s.notes[(size_t) lane] = bm::defaultNotes[lane];

    // 1. Each factory groove (no fills) has the expected number of hits over its length.
    //    The 4/4 counts match the files written by generate.py.
    const int expected[] = { 48, 48, 48, 48, 48, 56, 32, 44, 44, 28, 25,   // 4/4
                             26, 36, 30,                                   // 6/8
                             36, 19,                                       // 3/4
                             42, 48, 56, 50, 32, 32,                       // more 4/4
                             33,                                           // 12/8
                             26, 32, 32, 47, 69,                           // iconic / polymeter
                             36, 28, 26, 60,                               // ternary 4/4
                             26, 24, 36,                                   // ternary 12/8
                             32, 48, 56,                                   // second wave
                             64, 48, 33, 15, 17,                           // atmospheric / DSBM
                             64, 32,                                       // war metal
                             31, 37,                                       // death
                             15, 13 };                                     // doom
    for (auto& e : bm::factoryPatternErrors())
        check (false, e);
    check (lib.grooves.size() == std::size (expected), "groove count");
    s.fillEvery = 0;
    s.chanceAmount = 0.0f;   // every hit plays
    for (size_t g = 0; g < lib.grooves.size() && g < std::size (expected); ++g)
    {
        s.groove = (int) g;
        const auto n = (int) render (lib, s, 0.0, lib.grooves[g]->layout().lengthQuarters()).size();
        check (n == expected[g], lib.grooves[g]->name + ": " + juce::String (n) + " hits, expected " + juce::String (expected[g]));
    }

    // 2. Chunked rendering equals one-shot rendering for every groove/fill combination.
    juce::Random rng (42);
    for (size_t g = 0; g < lib.grooves.size(); ++g)
        for (int every : { 1, 2, 4 })
        {
            s.groove = (int) g;
            s.fill = (int) ((g * 5 + (size_t) every) % lib.fills.size());
            s.fillEvery = every;
            s.randomFill = (g % 3 == 0);
            const double len = 16.0 * lib.grooves[g]->layout().barQuarters();
            check (render (lib, s, 0.0, len) == renderChunked (lib, s, len, rng),
                   "chunking " + lib.grooves[g]->name + " every " + juce::String (every));
        }
    s.randomFill = false;

    // 3. In 6/8, a 3/8 fill takes the last dotted quarter of the bar and nothing else.
    {
        s.groove = 11;   // 6/8 tremolo groove
        s.fill = 9;      // Toms 3/8
        s.fillEvery = 1;
        const auto hits = render (lib, s, 0.0, 3.0);
        bool ok = std::count (hits.begin(), hits.end(), std::pair<double, int> { 1.5, 38 }) == 1;
        for (auto& [t, note] : hits)
        {
            const bool isTom = note == 50 || note == 48 || note == 43;
            if (isTom && t < 1.5) ok = false;          // toms only in the fill
            if (note == 51 && t >= 1.5) ok = false;    // no ride under the fill
        }
        check (ok, "6/8 fill placement");
    }

    // 4. Changing resolution keeps hits; adding bars repeats them; reset restores.
    {
        auto& p = *lib.grooves[0];
        int before = 0;
        for (int lane = 0; lane < bm::NumLanes; ++lane)
            for (int st = 0; st < p.layout().numSteps(); ++st)
                before += p.get (lane, st) > 0;

        auto l = p.layout();
        l.res = bm::Resolution::ThirtySecond;
        l.bars = 4;
        p.setLayout (l);
        int after = 0;
        for (int lane = 0; lane < bm::NumLanes; ++lane)
            for (int st = 0; st < p.layout().numSteps(); ++st)
                after += p.get (lane, st) > 0;
        check (p.layout().numSteps() == 128 && after == before * 2, "setLayout to 1/32 x 4 bars");
        check (p.get (bm::Kick, 0) > 0 && p.get (bm::Kick, 2) == 0 && p.get (bm::Kick, 4) > 0, "1/32 remap positions");

        p.reset();
        check (p.layout().numSteps() == 32, "reset restores layout");

        auto bad = p.layout();
        bad.num = 7; bad.den = 8; bad.res = bm::Resolution::EighthTriplet;
        check (! bad.isValid(), "7/8 with 1/8T is rejected");
    }

    // 5. Forced fill plays in its bar; hold mode with no key plays only that fill.
    {
        s.groove = 0;
        s.fillEvery = 0;
        s.forcedFillBar = 2;
        s.forcedFill = 3;   // Toms 16ths (one beat)
        s.grooveMuted = true;
        const auto hits = render (lib, s, 0.0, 16.0);
        bool ok = hits.size() == 4;
        for (auto& h : hits)
            ok = ok && h.first >= 11.0 && h.first < 12.0;
        check (ok, "forced fill in hold mode");

        s.grooveMuted = false;
        s.origin = 2.0;     // groove restarted at beat 2
        const auto shifted = render (lib, s, 2.0, 2.01);
        check (! shifted.empty() && shifted.front().first == 2.0, "origin shifts the groove");
    }

    s = bm::Settings {};
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        s.notes[(size_t) lane] = bm::defaultNotes[lane];
    s.fillEvery = 0;
    s.humanize = 0;
    s.accent = 0;

    auto countNote = [] (const std::vector<std::pair<double, int>>& hits, int note, double at = -1.0)
    {
        int n = 0;
        for (auto& h : hits)
            if (h.second == note && (at < 0.0 || std::abs (h.first - at) < 1.0e-9)) ++n;
        return n;
    };

    // 6. Cymbal swap: ride part moves to china, the downbeat crash stays.
    {
        s.groove = 0;   // traditional (ride)
        s.cymbal = bm::CymbalSwap::China;
        const auto hits = render (lib, s, 0.0, 8.0);
        check (countNote (hits, 51) == 0 && countNote (hits, 52) == 15 && countNote (hits, 49, 0.0) == 1, "cymbal swap");
        s.cymbal = bm::CymbalSwap::AsWritten;
    }

    // 7. Crash after fill lands on the next downbeat only when enabled.
    {
        s.groove = 0;
        s.fill = 0;
        s.fillEvery = 1;
        s.crashAfterFill = true;
        check (countNote (render (lib, s, 0.0, 8.0), 49, 4.0) == 1, "crash after fill on");
        s.crashAfterFill = false;
        check (countNote (render (lib, s, 0.0, 8.0), 49, 4.0) == 0, "crash after fill off");
        s.fillEvery = 0;
    }

    // 8. Chance: '?' kicks play about half the time, and always at chance amount 0.
    {
        s.groove = 21;   // Mid-tempo - Variable kicks: 16 sure + 4 '?' kicks per 2 bars
        check (lib.grooves[21]->name.contains ("Variable"), "variable kicks index");
        s.chanceAmount = 1.0f;
        const int kicks = countNote (render (lib, s, 0.0, 800.0), 36);   // 100 cycles
        check (kicks > 1600 + 120 && kicks < 1600 + 280, "chance ~50% (" + juce::String (kicks) + ")");
        s.chanceAmount = 0.0f;
        check (countNote (render (lib, s, 0.0, 800.0), 36) == 2000, "chance amount 0 plays all");
        s.chanceAmount = 1.0f;
    }

    // 9. Kick balance: in a 16th run every second kick is weaker.
    {
        s.groove = 7;    // Double kick - 16ths
        s.kickBalance = 10;
        std::vector<int> vels;
        bm::renderHits (lib, s, 0.0, 1.0, [&] (const bm::Hit& h) { if (h.note == 36) vels.push_back (h.velocity); });
        check (vels == std::vector<int> { 100, 90, 100, 90 }, "kick balance");
        s.kickBalance = 0;
    }

    // 10. Timing humanize stays within range.
    {
        s.groove = 0;
        s.timingMs = 8.0f;
        double maxOffset = 0.0;
        bm::renderHits (lib, s, 0.0, 64.0, [&] (const bm::Hit& h) { maxOffset = std::max (maxOffset, std::abs (h.offsetMs)); });
        check (maxOffset > 4.0 && maxOffset <= 8.0, "timing range");
        s.timingMs = 0.0f;
    }

    // 11. Fill generator: every style works for every fill length/resolution.
    {
        juce::Random rng (7);
        for (auto& f : lib.fills)
            for (int style = 0; style < (int) bm::FillStyle::NumStyles; ++style)
            {
                bm::generateFill (*f, (bm::FillStyle) style, rng);
                int hits = 0, outside = 0;
                for (int lane = 0; lane < bm::NumLanes; ++lane)
                    for (int st = 0; st < bm::maxSteps; ++st)
                        if (f->get (lane, st) > 0)
                            (st < f->layout().numSteps() ? hits : outside)++;
                check (hits > 0 && outside == 0, "generate " + juce::String (bm::fillStyleName ((bm::FillStyle) style)) + " into " + f->name);
                f->reset();
            }
    }

    // 12. Every pattern lands in a menu category.
    {
        const auto order = bm::grooveCategoryOrder();
        for (auto& g : lib.grooves)
            check (order.contains (bm::grooveCategory (g->name)), "groove category for " + g->name);
        const auto fillOrder = bm::fillCategoryOrder();
        for (auto& f : lib.fills)
            check (fillOrder.contains (bm::fillCategory (f->factoryLayout())), "fill category for " + f->name);
        check (bm::grooveCategory ("Doom shuffle - Sabbath-style") == "Ternary - 4/4 triplets"
               && bm::grooveCategory ("Blast - Bomb") == "Blast beats", "groove categories");
    }

    // 13. Undo/redo restores cells and layout.
    {
        bm::EditHistory history;
        auto& p = *lib.grooves[0];
        history.beginEdit (p);
        p.set (bm::Tom1, 3, 77);
        history.beginEdit (p);
        auto l = p.layout();
        l.res = bm::Resolution::ThirtySecond;
        p.setLayout (l);
        check (history.undo().pattern == &p && p.layout().numSteps() == 32 && p.get (bm::Tom1, 3) == 77, "undo layout");
        check (history.undo().pattern == &p && p.get (bm::Tom1, 3) == 0, "undo cell");
        check (! history.undo(), "nothing more to undo");
        check (history.redo().pattern == &p && p.get (bm::Tom1, 3) == 77, "redo cell");
        check (history.redo().pattern == &p && p.layout().numSteps() == 64, "redo layout");
        p.reset();
    }

    // 14. Copy/paste across grids keeps time positions.
    {
        auto& src = *lib.grooves[0];   // 4/4, 1/16: kick on every 8th
        auto& dst = **std::find_if (lib.grooves.begin(), lib.grooves.end(),
                                    [] (auto& g) { return g->name.startsWith ("3/4 - Epic"); });   // 3/4, 1/16
        auto clip = bm::copyBar (src, 0);
        bm::pasteBar (dst, 1, clip);   // 4/4 bar into a 3/4 bar: last beat dropped
        int kicks = 0;
        for (int st = 12; st < 24; ++st) kicks += dst.get (bm::Kick, st) > 0;
        check (kicks == 6 && dst.get (bm::Kick, 12) > 0 && dst.get (bm::Kick, 14) > 0, "paste bar 4/4 -> 3/4");
        dst.reset();

        auto& fill = *lib.fills[1];    // 1/32, one beat: kick on 0 and 4
        auto laneClip = bm::copyLane (fill, bm::Kick);
        auto& groove = *lib.grooves[9];   // mid-tempo march, 2 bars of 1/16
        bm::pasteLane (groove, bm::Snare, laneClip);   // one-beat lane repeats over 8 beats
        int snares = 0;
        for (int st = 0; st < 32; ++st) snares += groove.get (bm::Snare, st) > 0;
        check (snares == 16 && groove.get (bm::Snare, 2) > 0, "paste lane repeats");
        groove.reset();

        bm::copyBarToAllBars (groove, 1);
        check (groove.get (bm::Crash, 0) == groove.get (bm::Crash, 16), "copy bar to all bars");
        groove.reset();
    }

    // 15. Song: sections chain in time (mixed meters), end fills, crash after fill, loop.
    {
        auto indexOf = [&] (const char* prefix)
        {
            for (size_t i = 0; i < lib.grooves.size(); ++i)
                if (lib.grooves[i]->name.startsWith (prefix)) return (int) i;
            return -1;
        };
        bm::Song song;
        song.assign ({ { 0, 2, 0, bm::Section::noFill },                        // trad blast 4/4: 0 - 8
                       { indexOf ("3/4 - Hammer"), 1, 0, 3 },                    // 3/4 + toms fill: 8 - 11
                       { indexOf ("Skank"), 1, 0, bm::Section::noFill } });      // 4/4: 11 - 15
        const auto placed = bm::placeSections (lib, song);
        check (placed.count == 3 && std::abs (placed.length - 15.0) < 1.0e-9, "song placement");

        bm::Settings base = s;
        base.crashAfterFill = true;
        std::vector<std::pair<double, int>> hits;
        auto collect = [&] (const bm::Hit& h) { hits.push_back ({ h.ppq, h.note }); };
        bm::renderSong (lib, base, placed, false, 0.0, 30.0, collect);

        check (countNote (hits, 38, 8.0) == 1 && countNote (hits, 36, 8.0) == 1, "song: hammer starts at 8");
        check (countNote (hits, 50, 10.0) == 1 && countNote (hits, 43, 10.75) == 1, "song: end fill on last beat of the 3/4 bar");
        check (countNote (hits, 49, 11.0) == 1, "song: crash after the section's end fill");
        bool pastEnd = false;
        for (auto& h : hits) pastEnd = pastEnd || h.first >= 15.0;
        check (! pastEnd, "song: silent after the end");

        hits.clear();
        bm::renderSong (lib, base, placed, true, 0.0, 30.0, collect);
        check (countNote (hits, 36, 15.0) == 1 && countNote (hits, 38, 23.0) == 1, "song: loops");

        // Chunked rendering matches one-shot rendering.
        std::vector<std::pair<double, int>> whole, chunked;
        bm::renderSong (lib, base, placed, true, 0.0, 45.0, [&] (const bm::Hit& h) { whole.push_back ({ h.ppq, h.note }); });
        juce::Random r2 (3);
        for (double t = 0.0; t < 45.0;)
        {
            const double e = std::min (45.0, t + (16 + r2.nextInt (1024)) * (200.0 / (60.0 * 48000.0)));
            bm::renderSong (lib, base, placed, true, t, e, [&] (const bm::Hit& h) { chunked.push_back ({ h.ppq, h.note }); });
            t = e;
        }
        check (whole == chunked, "song: chunked rendering");

        // Song edits undo and redo.
        bm::EditHistory songHistory;
        songHistory.beginSongEdit (song);
        song.assign ({ { 5, 8, 2, bm::Section::randomFill } });
        check (songHistory.undo().song && song.size() == 3 && song.get (1).endFill == 3, "song undo");
        check (songHistory.redo().song && song.size() == 1 && song.get (0).bars == 8, "song redo");
        songHistory.undo();

        // Saved and restored.
        bm::Song copy;
        copy.fromValueTree (song.toValueTree(), (int) lib.grooves.size(), (int) lib.fills.size());
        check (copy.size() == 3 && copy.get (1).endFill == 3 && copy.get (1).bars == 1, "song: state round trip");
    }

    // 16. Swing: off-16ths move toward the triplet position; triplet grids and 12/8 stay put.
    {
        s = bm::Settings {};
        for (int lane = 0; lane < bm::NumLanes; ++lane) s.notes[(size_t) lane] = bm::defaultNotes[lane];
        s.fillEvery = 0;
        auto kickTimes = [&] (int groove)
        {
            s.groove = groove;
            std::vector<double> t;
            bm::renderHits (lib, s, 0.0, 1.0, [&] (const bm::Hit& h) { if (h.note == 36) t.push_back (h.ppq + h.offsetPpq); });
            return t;
        };
        s.swing = 1.0f;
        const auto dk = kickTimes (7);   // double kick 16ths
        check (dk.size() == 4 && std::abs (dk[0]) < 1e-9 && std::abs (dk[1] - 1.0 / 3.0) < 1e-9
               && std::abs (dk[2] - 0.5) < 1e-9 && std::abs (dk[3] - 0.5 - 1.0 / 3.0) < 1e-9, "full swing on 16ths");
        s.swing = 0.5f;
        const auto half = kickTimes (7);
        check (half.size() == 4 && std::abs (half[1] - (0.25 + 1.0 / 24.0)) < 1e-9, "half swing");
        const auto mgla = kickTimes (34);   // 12/8 triplet double kick: compound meter, no swing
        check (mgla.size() == 2 && std::abs (mgla[1] - 0.5) < 1e-9, "no swing in 12/8");
        s.swing = 0.0f;
    }

    std::printf (failures == 0 ? "All engine tests passed\n" : "%d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
