#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int drumChannel = 10;
    constexpr int exportPpq = 960;
    constexpr int maxTriggersPerBlock = 64;
}

BlackMetalDrumsProcessor::BlackMetalDrumsProcessor()
    : AudioProcessor (BusesProperties()),
      params (*this, nullptr, "BlackMetalDrums", createLayout (library))
{
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        noteMap[(size_t) lane] = bm::defaultNotes[lane];

    pGroove      = params.getRawParameterValue ("groove");
    pFill        = params.getRawParameterValue ("fill");
    pRandomFill  = params.getRawParameterValue ("randomFill");
    pFillEvery   = params.getRawParameterValue ("fillEvery");
    pHumanize    = params.getRawParameterValue ("humanize");
    pAccent      = params.getRawParameterValue ("accent");
    pVelocity    = params.getRawParameterValue ("velocity");
    pTrigger     = params.getRawParameterValue ("trigger");
    pCymbal      = params.getRawParameterValue ("cymbal");
    pCrashAfter  = params.getRawParameterValue ("crashAfterFill");
    pChance      = params.getRawParameterValue ("chance");
    pKickBalance = params.getRawParameterValue ("kickBalance");
    pTiming      = params.getRawParameterValue ("timing");

    // Every groove needs a trigger key below the drum range.
    jassert (library.grooves.size() <= heldOrder.size());
    jassert (fillKeyBase + (int) library.fills.size() <= 128);
    pendingOff.fill (-1);
}

juce::AudioProcessorValueTreeState::ParameterLayout BlackMetalDrumsProcessor::createLayout (const bm::Library& lib)
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "groove", 1 }, "Groove", lib.grooveNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "fill", 1 }, "Fill", lib.fillNames(), 0));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "randomFill", 1 }, "Random fill", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "fillEvery", 1 }, "Fill every",
                                                        StringArray { "Off", "1 bar", "2 bars", "4 bars", "8 bars" }, 3));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "humanize", 1 }, "Humanize", 0, 20, 6));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "accent", 1 }, "Beat accent", 0, 20, 5));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "velocity", 1 }, "Velocity",
                                                        NormalisableRange<float> (50.0f, 150.0f, 1.0f), 100.0f,
                                                        AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "trigger", 2 }, "Note trigger",
                                                        StringArray { "Off", "Latch", "Hold" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "cymbal", 3 }, "Cymbal",
                                                        cymbalChoiceNames(), 0));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "crashAfterFill", 3 }, "Crash after fill", true));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "chance", 3 }, "Chance",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
                                                        AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "kickBalance", 3 }, "Kick L/R balance", 0, 20, 6));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "timing", 3 }, "Timing humanize",
                                                        NormalisableRange<float> (0.0f, 15.0f, 0.1f), 0.0f,
                                                        AudioParameterFloatAttributes().withLabel ("ms")));
    return layout;
}

//==============================================================================
bm::Settings BlackMetalDrumsProcessor::currentSettings() const
{
    bm::Settings s;
    s.groove        = juce::jlimit (0, (int) library.grooves.size() - 1, (int) pGroove->load());
    s.fill          = juce::jlimit (0, (int) library.fills.size() - 1, (int) pFill->load());
    s.randomFill    = pRandomFill->load() > 0.5f;
    s.fillEvery     = fillEveryBars[juce::jlimit (0, 4, (int) pFillEvery->load())];
    s.humanize      = (int) pHumanize->load();
    s.accent        = (int) pAccent->load();
    s.velocityScale = pVelocity->load() / 100.0f;
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        s.notes[(size_t) lane] = noteMap[(size_t) lane].load();
    s.cymbal         = (bm::CymbalSwap) juce::jlimit (0, (int) bm::CymbalSwap::NumChoices - 1, (int) pCymbal->load());
    s.crashAfterFill = pCrashAfter->load() > 0.5f;
    s.chanceAmount   = pChance->load() / 100.0f;
    s.kickBalance    = (int) pKickBalance->load();
    s.timingMs       = pTiming->load();
    return s;
}

juce::StringArray BlackMetalDrumsProcessor::cymbalChoiceNames()
{
    return { "As written", "Ride", "Ride bell", "Crash", "China", "Hi-hat", "Hi-hat open" };
}

BlackMetalDrumsProcessor::TriggerMode BlackMetalDrumsProcessor::triggerMode() const
{
    return (TriggerMode) juce::jlimit (0, 2, (int) pTrigger->load());
}

bm::Settings BlackMetalDrumsProcessor::playbackSettings() const
{
    auto s = currentSettings();
    const auto mode = triggerMode();
    if (mode == TriggerOff)
        return s;

    if (mode == TriggerLatch)
    {
        if (latchedGroove >= 0)
            s.groove = latchedGroove;
    }
    else
    {
        int best = -1, bestOrder = 0;
        for (int i = 0; i < (int) library.grooves.size(); ++i)
            if (heldOrder[(size_t) i] > bestOrder)
            {
                best = i;
                bestOrder = heldOrder[(size_t) i];
            }
        if (best >= 0)
            s.groove = best;
        else
            s.grooveMuted = true;
    }

    s.origin = origin;
    s.forcedFillBar = forcedFillBar;
    s.forcedFill = forcedFill;
    return s;
}

bm::Settings BlackMetalDrumsProcessor::displaySettings() const
{
    auto s = currentSettings();
    if (triggerMode() == TriggerOff || uiGroove.load() < 0)
        return s;

    s.groove = uiGroove.load();
    s.grooveMuted = uiMuted.load();
    s.origin = uiOrigin.load();
    s.forcedFillBar = uiForcedFillBar.load();
    s.forcedFill = uiForcedFill.load();
    return s;
}

void BlackMetalDrumsProcessor::publishForEditor()
{
    const auto s = playbackSettings();
    uiGroove = s.groove;
    uiMuted = s.grooveMuted;
    uiOrigin = s.origin;
    uiForcedFillBar = s.forcedFillBar;
    uiForcedFill = s.forcedFill;
}

void BlackMetalDrumsProcessor::resetTriggers()
{
    latchedGroove = -1;
    heldOrder.fill (0);
    pressCounter = 0;
    origin = 0.0;
    forcedFillBar = bm::noBar;
}

// Groove keys select a groove and restart it on the bar line at (or just after) the key.
// Fill keys put that fill at the end of the bar the key is in.
void BlackMetalDrumsProcessor::handleTrigger (bool noteOn, int note, double ppq,
                                              const juce::AudioPlayHead::PositionInfo* pos)
{
    const int index = note - grooveKeyBase;
    const int numGrooves = (int) library.grooves.size();

    if (juce::isPositiveAndBelow (index, numGrooves))
    {
        if (! noteOn)
        {
            heldOrder[(size_t) index] = 0;
            return;
        }

        latchedGroove = index;
        heldOrder[(size_t) index] = ++pressCounter;

        if (pos == nullptr)   // stopped: just select it
            return;

        // Snap to the host's bar grid; a key up to a 16th early still counts for the next bar.
        const double tolerance = 0.25;
        const auto barStart = pos->getPpqPositionOfLastBarStart();
        const auto sig = pos->getTimeSignature();
        if (barStart.hasValue() && sig.hasValue() && sig->numerator > 0 && sig->denominator > 0)
        {
            const double barLen = sig->numerator * 4.0 / sig->denominator;
            origin = *barStart + std::floor ((ppq + tolerance - *barStart) / barLen) * barLen;
        }
        else
        {
            origin = std::floor (ppq + tolerance);
        }
        forcedFillBar = bm::noBar;
        return;
    }

    const int fillIndex = note - fillKeyBase;
    if (noteOn && juce::isPositiveAndBelow (fillIndex, (int) library.fills.size()) && pos != nullptr)
    {
        const auto s = playbackSettings();
        forcedFill = fillIndex;
        forcedFillBar = bm::barAt (s, library.grooves[(size_t) s.groove]->layout(), ppq + 1.0e-6);
    }
}

//==============================================================================
void BlackMetalDrumsProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    pendingOff.fill (-1);
    wasPlaying = false;
    passThrough.ensureSize (8192);
}

void BlackMetalDrumsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    juce::Optional<juce::AudioPlayHead::PositionInfo> pos;
    if (auto* playHead = getPlayHead())
        pos = playHead->getPosition();

    const bool playing = pos.hasValue() && pos->getIsPlaying()
                         && pos->getPpqPosition().hasValue() && pos->getBpm().hasValue();

    if (pos.hasValue())
        if (const auto sig = pos->getTimeSignature(); sig.hasValue())
        {
            hostNumerator = sig->numerator;
            hostDenominator = sig->denominator;
        }

    // Split incoming MIDI: trigger keys are consumed, everything else passes through.
    const auto mode = triggerMode();
    if (mode != lastTriggerMode)
    {
        resetTriggers();
        lastTriggerMode = mode;
    }

    struct Trigger { int sample; bool on; int note; };
    std::array<Trigger, maxTriggersPerBlock> triggers;
    int numTriggers = 0;

    passThrough.clear();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const bool isNote = msg.isNoteOn() || msg.isNoteOff();
        const int note = msg.getNoteNumber();
        const bool isTriggerKey = isNote && mode != TriggerOff
                                  && (juce::isPositiveAndBelow (note - grooveKeyBase, (int) library.grooves.size())
                                      || juce::isPositiveAndBelow (note - fillKeyBase, (int) library.fills.size()));
        if (isTriggerKey)
        {
            if (numTriggers < maxTriggersPerBlock)
                triggers[(size_t) numTriggers++] = { meta.samplePosition, msg.isNoteOn(), msg.getNoteNumber() };
        }
        else
        {
            passThrough.addEvent (msg, meta.samplePosition);
        }
    }
    midi.swapWith (passThrough);

    auto noteOff = [&] (int note, int sample)
    {
        midi.addEvent (juce::MidiMessage::noteOff (drumChannel, note), sample);
        pendingOff[(size_t) note] = -1;
    };

    const bool previewing = ! playing && previewOn.load();

    if (! playing && ! previewing)
    {
        wasPreviewing = false;
        for (int i = 0; i < numTriggers; ++i)
            handleTrigger (triggers[(size_t) i].on, triggers[(size_t) i].note, 0.0, nullptr);
        for (int note = 0; note < 128; ++note)
            if (pendingOff[(size_t) note] >= 0)
                noteOff (note, 0);
        heldOrder.fill (0);
        wasPlaying = false;
        playheadPpq = -1.0;
        publishForEditor();
        return;
    }

    // Switching between host playback and preview: end held notes and restart cleanly.
    if (previewing != wasPreviewing)
    {
        for (int note = 0; note < 128; ++note)
            if (pendingOff[(size_t) note] >= 0)
                noteOff (note, 0);
        previewPpq = 0.0;
        wasPlaying = false;
    }
    if (playing && ! wasPlaying)
        forcedFillBar = bm::noBar;

    const double bpm = pos.hasValue() && pos->getBpm().hasValue() ? *pos->getBpm() : 120.0;
    const double ppqPerSample = bpm / (60.0 * sampleRate);
    double start = playing ? *pos->getPpqPosition() : previewPpq;

    // Hosts report positions with tiny rounding drift; snap to the previous block's end
    // so hits on a block boundary are neither doubled nor dropped.
    if (wasPlaying && std::abs (start - lastEnd) < 0.01)
        start = lastEnd;
    const double end = start + numSamples * ppqPerSample;

    for (int note = 0; note < 128; ++note)
        if (pendingOff[(size_t) note] >= 0 && pendingOff[(size_t) note] < numSamples)
            noteOff (note, pendingOff[(size_t) note]);

    const int noteLength = juce::jmax (1, (int) (0.5 * (60.0 / bpm / 8.0) * sampleRate));   // half a 32nd

    auto renderSegment = [&] (double from, double to)
    {
        if (to <= from)
            return;
        // Timing humanize can move a hit across a block boundary: look a little either side
        // and keep only hits whose shifted time falls in this segment.
        auto s = previewing ? currentSettings() : playbackSettings();
        if (previewing && previewFillEveryBar.load())
        {
            s.fillEvery = 1;
            s.randomFill = false;
        }
        const double ppqPerMs = bpm / 60'000.0;
        const double reach = s.timingMs * ppqPerMs;
        bm::renderHits (library, s, from - reach, to + reach, [&] (const bm::Hit& hit)
        {
            const double t = hit.ppq + hit.offsetMs * ppqPerMs;
            if (t < from || t >= to)
                return;
            const int sample = juce::jlimit (0, numSamples - 1, (int) ((t - start) / ppqPerSample));
            if (pendingOff[(size_t) hit.note] >= 0)
                noteOff (hit.note, sample);
            midi.addEvent (juce::MidiMessage::noteOn (drumChannel, hit.note, (juce::uint8) hit.velocity), sample);
            pendingOff[(size_t) hit.note] = sample + noteLength;
        });
    };

    // Trigger keys take effect at their exact position inside the block.
    double segmentStart = start;
    for (int i = 0; i < numTriggers; ++i)
    {
        const auto& t = triggers[(size_t) i];
        const double at = juce::jlimit (start, end, start + t.sample * ppqPerSample);
        renderSegment (segmentStart, at);
        handleTrigger (t.on, t.note, at, playing ? &*pos : nullptr);
        segmentStart = at;
    }
    renderSegment (segmentStart, end);

    for (int note = 0; note < 128; ++note)
    {
        auto& off = pendingOff[(size_t) note];
        if (off < 0)
            continue;
        if (off < numSamples)
            noteOff (note, off);
        else
            off -= numSamples;
    }

    lastEnd = end;
    wasPlaying = playing;
    wasPreviewing = previewing;
    if (previewing)
        previewPpq = end;
    playheadPpq = start;
    hostBpm = bpm;
    publishForEditor();
}

//==============================================================================
juce::File BlackMetalDrumsProcessor::exportMidiFile() const
{
    const auto s = currentSettings();
    const auto& groove = *library.grooves[(size_t) s.groove];
    const auto gl = groove.layout();
    const double bpm = hostBpm.load() > 0.0 ? hostBpm.load() : 180.0;

    // Long enough to contain one full groove cycle and one fill cycle.
    const int bars = s.fillEvery > 0 ? juce::jmax (gl.bars, s.fillEvery) : gl.bars;
    const double length = bars * gl.barQuarters();

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, groove.name));
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60'000'000.0 / bpm)));
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (gl.num, gl.den));

    bm::renderHits (library, s, 0.0, length, [&] (const bm::Hit& hit)
    {
        const double tick = juce::jmax (0.0, (hit.ppq + hit.offsetMs * bpm / 60'000.0) * exportPpq);
        seq.addEvent (juce::MidiMessage::noteOn (drumChannel, hit.note, (juce::uint8) hit.velocity), tick);
        seq.addEvent (juce::MidiMessage::noteOff (drumChannel, hit.note), tick + exportPpq / 16.0);
    });
    seq.updateMatchedPairs();

    auto endOfTrack = juce::MidiMessage::endOfTrack();
    endOfTrack.setTimeStamp (length * exportPpq);
    seq.addEvent (endOfTrack);

    juce::MidiFile file;
    file.setTicksPerQuarterNote (exportPpq);
    file.addTrack (seq);

    juce::String name = groove.name;
    if (s.fillEvery > 0)
        name << " + " << (s.randomFill ? juce::String ("random fills") : library.fills[(size_t) s.fill]->name);
    name << " " << juce::roundToInt (bpm) << "bpm";

    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BlackMetalDrums");
    dir.createDirectory();
    auto out = dir.getChildFile (juce::File::createLegalFileName (name) + ".mid");
    out.deleteFile();

    juce::FileOutputStream stream (out);
    if (! stream.openedOk() || ! file.writeTo (stream, 1))
        return {};
    stream.flush();
    return out;
}

bm::NoteMap BlackMetalDrumsProcessor::getNoteMap() const
{
    bm::NoteMap notes {};
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        notes[(size_t) lane] = noteMap[(size_t) lane].load();
    return notes;
}

void BlackMetalDrumsProcessor::setNoteMap (const bm::NoteMap& notes)
{
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        noteMap[(size_t) lane] = notes[(size_t) lane];
    patternsEdited();
}

void BlackMetalDrumsProcessor::patternsEdited()
{
    updateHostDisplay (ChangeDetails().withNonParameterStateChanged (true));
}

//==============================================================================
void BlackMetalDrumsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = params.copyState();

    for (auto name : { "Patterns", "Notes" })
        state.removeChild (state.getChildWithName (name), nullptr);

    juce::ValueTree patterns ("Patterns");
    for (auto* list : { &library.grooves, &library.fills })
        for (auto& p : *list)
            patterns.appendChild (p->toValueTree(), nullptr);
    state.appendChild (patterns, nullptr);

    juce::ValueTree notes ("Notes");
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        notes.setProperty (bm::laneIds[lane], noteMap[(size_t) lane].load(), nullptr);
    state.appendChild (notes, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void BlackMetalDrumsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (params.state.getType()))
        return;

    params.replaceState (state);
    history.clear();

    const auto patterns = state.getChildWithName ("Patterns");
    for (auto* list : { &library.grooves, &library.fills })
        for (auto& p : *list)
        {
            const auto saved = patterns.getChildWithProperty ("name", p->name);
            if (saved.isValid())
                p->fromValueTree (saved);
        }

    const auto notes = state.getChildWithName ("Notes");
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        noteMap[(size_t) lane] = (int) notes.getProperty (bm::laneIds[lane], bm::defaultNotes[lane]);
}

juce::AudioProcessorEditor* BlackMetalDrumsProcessor::createEditor()
{
    return new BlackMetalDrumsEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlackMetalDrumsProcessor();
}
