// Runs the real processor against a simulated host transport to check note triggers.
#include "../Source/PluginProcessor.h"
#include "../Source/PatternFiles.h"
#include <cstdio>

namespace
{
    int failures = 0;
    void check (bool ok, const char* what) { if (! ok) { std::printf ("FAIL %s\n", what); ++failures; } }

    struct FakeHost : juce::AudioPlayHead
    {
        double ppq = 0.0, bpm = 120.0;
        bool playing = true;
        int num = 4, den = 4;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setIsPlaying (playing);
            info.setBpm (bpm);
            info.setPpqPosition (ppq);
            info.setTimeSignature (TimeSignature { num, den });
            const double barLen = num * 4.0 / den;
            info.setPpqPositionOfLastBarStart (std::floor (ppq / barLen) * barLen);
            return info;
        }
    };

    struct Rig
    {
        static constexpr double sampleRate = 48000.0;
        static constexpr int block = 512;
        BlackMetalDrumsProcessor proc;
        FakeHost host;
        juce::AudioBuffer<float> buffer { 0, block };
        int inputChannel = 1;

        Rig()
        {
            proc.setPlayHead (&host);
            proc.prepareToPlay (sampleRate, block);
        }

        void setParam (const char* id, float index)
        {
            auto* p = proc.params.getParameter (id);
            p->setValueNotifyingHost (p->convertTo0to1 (index));
        }

        // Plays from 'from' to 'to' (PPQ). 'input' maps PPQ -> (note, on). Returns note-ons as (ppq, note).
        std::vector<std::pair<double, int>> play (double from, double to, std::vector<std::tuple<double, int, bool>> input = {})
        {
            std::vector<std::pair<double, int>> out;
            const double ppqPerSample = host.bpm / (60.0 * sampleRate);
            for (host.ppq = from; host.ppq < to - 1.0e-9; host.ppq += block * ppqPerSample)
            {
                juce::MidiBuffer midi;
                for (auto& [t, note, on] : input)
                    if (t >= host.ppq && t < host.ppq + block * ppqPerSample)
                    {
                        const int s = (int) ((t - host.ppq) / ppqPerSample);
                        midi.addEvent (on ? juce::MidiMessage::noteOn (inputChannel, note, (juce::uint8) 100)
                                          : juce::MidiMessage::noteOff (inputChannel, note), s);
                    }
                proc.processBlock (buffer, midi);
                for (const auto m : midi)
                    if (m.getMessage().isNoteOn())
                        out.push_back ({ host.ppq + m.samplePosition * ppqPerSample, m.getMessage().getNoteNumber() });
            }
            return out;
        }
    };

    bool hasNear (const std::vector<std::pair<double, int>>& hits, double t, int note)
    {
        for (auto& h : hits)
            if (h.second == note && std::abs (h.first - t) < 0.02) return true;
        return false;
    }
    bool anyIn (const std::vector<std::pair<double, int>>& hits, double from, double to, int note = -1)
    {
        for (auto& h : hits)
            if (h.first >= from && h.first < to && (note < 0 || h.second == note)) return true;
        return false;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    // Trigger off: default groove (traditional blast) plays, kick on beat 1, input notes pass through.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        const auto hits = r.play (0.0, 4.0, { { 1.0, 60, true } });
        check (hasNear (hits, 0.0, 36) && hasNear (hits, 0.0, 49), "off: groove plays");
        check (hasNear (hits, 1.0, 60), "off: input passes through");
    }

    // Latch: key for groove 3 (hammer crash -> note 3) a 16th before bar 2 starts it on bar 2.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 1);
        const auto hits = r.play (0.0, 8.0, { { 3.8, 3, true }, { 4.5, 3, false } });
        // Hammer blast: snare + kick together on beat 1 of bar 2 (traditional has no snare on the beat).
        check (hasNear (hits, 4.0, 38) && hasNear (hits, 4.0, 36), "latch: hammer starts on the bar");
        check (! hasNear (hits, 0.0, 38), "latch: traditional before the key");
        check (hasNear (hits, 6.0, 38), "latch: stays after key release");
        check (! anyIn (hits, 0.0, 8.0, 3), "latch: trigger key not passed through");
    }

    // Hold: silent until a key is held, stops when released.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 2);
        const auto hits = r.play (0.0, 12.0, { { 4.0, 0, true }, { 8.0, 0, false } });
        check (! anyIn (hits, 0.0, 3.99), "hold: silent before key");
        check (anyIn (hits, 4.0, 7.99), "hold: plays while held");
        check (! anyIn (hits, 8.01, 12.0), "hold: silent after release");
    }

    // Fill key: fill 3 (toms 16ths, note 96 + 3 = 99) in bar 2 plays on beat 4 of bar 2.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 1);
        const auto hits = r.play (0.0, 12.0, { { 4.0, 99, true }, { 4.2, 99, false } });
        check (hasNear (hits, 7.0, 50) && hasNear (hits, 7.75, 43), "fill key: toms on beat 4 of that bar");
        check (! anyIn (hits, 0.0, 6.9, 50) && ! anyIn (hits, 8.0, 12.0, 50), "fill key: only once");
    }

    // 6/8 host meter: groove key snaps to the 6/8 bar line (3 quarters).
    {
        Rig r;
        r.host.num = 6; r.host.den = 8;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 1);
        const auto hits = r.play (0.0, 9.0, { { 3.1, 11, true } });   // 6/8 tremolo groove, key slightly late
        check (hasNear (hits, 3.0, 49) || hasNear (hits, 3.0, 36), "6/8: snapped to bar at 3.0");
        check (hasNear (hits, 4.5, 38), "6/8: snare on the second dotted quarter");
    }

    // Trigger keys stay outside the drum range: drawn kick notes pass through, the last groove key works.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 1);
        const auto hits = r.play (0.0, 8.0, { { 1.25, 36, true }, { 1.3, 36, false }, { 3.9, 34, true } });
        check (hasNear (hits, 1.25, 36), "trigger on: drum notes pass through");
        // Groove 34 = 12/8 triplet double kick: kick on every 8th (half a quarter) from bar 2, snare on the second dotted quarter.
        check (hasNear (hits, 4.5, 36) && hasNear (hits, 5.0, 36) && hasNear (hits, 5.5, 38), "last groove key");
    }

    // Preview: plays while the host is stopped, silent once it is switched off.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.host.playing = false;
        check (r.play (0.0, 4.0).empty(), "stopped: silent");
        r.proc.previewOn = true;
        const auto hits = r.play (0.0, 4.0);
        check (hasNear (hits, 0.0, 36) && hasNear (hits, 0.5, 36), "preview plays from bar 1");
        r.proc.previewOn = false;
        check (r.play (4.0, 8.0).empty(), "preview off: silent");
    }

    // Song mode: plays the arrangement from bar 1, silent after the end.
    {
        Rig r;
        r.proc.song.assign ({ { 3, 1, 0, bm::Section::noFill } });   // one bar of hammer blast
        r.setParam ("songMode", 1);
        const auto hits = r.play (0.0, 8.0);
        check (hasNear (hits, 0.0, 38) && hasNear (hits, 3.5, 38), "song mode: hammer blast in bar 1");
        check (! anyIn (hits, 4.01, 8.0), "song mode: silent after the song");
    }

    // Presets round-trip layout and cells; export-all writes every pattern as a readable MIDI file.
    {
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("bmd-test-" + juce::String (juce::Random::getSystemRandom().nextInt()));
        bm::Library lib;
        auto& g = *lib.grooves[0];
        auto l = g.layout();
        l.res = bm::Resolution::ThirtySecond;
        g.setLayout (l);
        g.set (bm::Tom2, 5, 88);
        g.setChance (bm::Tom2, 5, 40);
        const auto preset = dir.getChildFile ("mine.bmpat");
        check (bm::savePreset (g, preset), "preset saved");
        g.reset();
        check (bm::loadPreset (g, preset) && g.layout().numSteps() == 64 && g.get (bm::Tom2, 5) == 88
               && g.getChance (bm::Tom2, 5) == 40, "preset loaded");
        g.reset();

        bm::Settings s;
        for (int lane = 0; lane < bm::NumLanes; ++lane) s.notes[(size_t) lane] = bm::defaultNotes[lane];
        s.humanize = 0;
        const int written = bm::exportAllPatterns (lib, s, 0.0, dir);
        check (written == (int) (lib.grooves.size() + lib.fills.size()), "export all: file count");

        const auto files = dir.getChildFile ("Grooves").findChildFiles (juce::File::findFiles, false, "01 *.mid");
        int notes = 0;
        if (files.size() == 1)
        {
            juce::FileInputStream in (files[0]);
            juce::MidiFile midi;
            if (midi.readFrom (in))
                for (int t = 0; t < midi.getNumTracks(); ++t)
                    for (auto* e : *midi.getTrack (t))
                        notes += e->message.isNoteOn();
        }
        check (files.size() == 1 && files[0].getFileName().contains ("200bpm") && notes == 48, "export all: traditional blast file");
        check (bm::fileNameFor ("12/8 - Atmospheric") == "12-8 - Atmospheric"
               && dir.getChildFile ("Grooves").findChildFiles (juce::File::findFiles, false, "* 12-8 - Atmospheric *").size() == 1,
               "file names keep meters readable");
        dir.deleteRecursively();
    }

    // Half-time / double-time: traditional blast kicks every 8th at normal speed.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("timeFeel", 1);   // half-time: kicks every quarter
        auto hits = r.play (0.0, 4.0);
        check (hasNear (hits, 1.0, 36) && ! hasNear (hits, 0.5, 36), "half-time");

        Rig d;
        d.setParam ("fillEvery", 0);
        d.setParam ("timeFeel", 2);   // double-time: kicks every 16th
        hits = d.play (0.0, 4.0);
        check (hasNear (hits, 0.25, 36) && hasNear (hits, 0.5, 36), "double-time");

        // Half-time + note trigger: the groove restarts on the host bar line.
        Rig t;
        t.setParam ("fillEvery", 0);
        t.setParam ("timeFeel", 1);
        t.setParam ("trigger", 1);
        hits = t.play (0.0, 8.0, { { 3.9, 3, true } });   // hammer blast from bar 2
        check (hasNear (hits, 4.0, 38) && hasNear (hits, 5.0, 38) && ! hasNear (hits, 4.5, 38), "half-time trigger");
    }

    // Swing across block boundaries: every kick plays exactly once.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("groove", 7);     // double kick 16ths: 4 kicks per beat
        r.setParam ("swing", 100.0f);
        r.setParam ("timing", 5.0f);
        const auto hits = r.play (0.0, 16.0);
        int kicks = 0;
        for (auto& h : hits) kicks += h.second == 36 && h.first > 0.9 && h.first < 15.9;
        check (kicks == 60 && hasNear (hits, 2.0 + 1.0 / 3.0, 36), "swing: no lost or doubled hits");
    }

    // Grooves past the first 35: not reachable in the default key mode (the note is a drum note and
    // passes through), reachable on channel 16 in the "ch 16" modes.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("trigger", 1);    // Latch, default keys
        auto hits = r.play (0.0, 8.0, { { 3.9, 46, true } });
        check (hasNear (hits, 3.9, 46) && ! hasNear (hits, 4.125, 38), "default keys: note 46 is a drum note");

        Rig c;
        c.setParam ("fillEvery", 0);
        c.setParam ("trigger", 3);    // Latch (ch 16)
        c.inputChannel = 16;
        hits = c.play (0.0, 8.0, { { 3.9, 46, true } });   // groove 46: hyperblast, snare 32nds
        check (! hasNear (hits, 3.9, 46) && hasNear (hits, 4.125, 38), "ch 16: note 46 triggers groove 46");

        Rig o;
        o.setParam ("fillEvery", 0);
        o.setParam ("trigger", 3);
        hits = o.play (0.0, 8.0, { { 3.9, 3, true } });    // channel 1 in a ch 16 mode: passes through
        check (hasNear (hits, 3.9, 3) && ! hasNear (hits, 4.0, 38), "ch 16: other channels pass through");
    }

    // Timing humanize across block boundaries: every kick still plays exactly once.
    {
        Rig r;
        r.setParam ("fillEvery", 0);
        r.setParam ("timing", 10.0f);
        const auto hits = r.play (0.0, 16.0);
        // Away from the rendered range's edges (where shifted hits can fall outside): kicks at 1.0 ... 15.0.
        int kicks = 0;
        for (auto& h : hits) kicks += h.second == 36 && h.first > 0.9 && h.first < 15.1;
        check (kicks == 29, "timing: no lost or doubled hits");
    }

    std::printf (failures == 0 ? "All processor tests passed\n" : "%d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
