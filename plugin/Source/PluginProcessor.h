#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "Engine.h"
#include "KitMaps.h"

class BlackMetalDrumsProcessor : public juce::AudioProcessor
{
public:
    BlackMetalDrumsProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override    { return true; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                                     { return true; }

    const juce::String getName() const override                         { return JucePlugin_Name; }
    bool acceptsMidi() const override                                   { return true; }
    bool producesMidi() const override                                  { return true; }
    bool isMidiEffect() const override                                  { return true; }
    double getTailLengthSeconds() const override                        { return 0.0; }

    int getNumPrograms() override                                       { return 1; }
    int getCurrentProgram() override                                    { return 0; }
    void setCurrentProgram (int) override                               {}
    const juce::String getProgramName (int) override                    { return {}; }
    void changeProgramName (int, const juce::String&) override          {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    enum TriggerMode { TriggerOff, TriggerLatch, TriggerHold };
    static constexpr int fillEveryBars[] = { 0, 1, 2, 4, 8 };
    // Trigger keys sit outside the General MIDI drum range (35-81), so drum notes still pass through:
    // grooves from C-2 (note 0) up to A#0, fills from C6 (note 96) up.
    static constexpr int grooveKeyBase = 0, maxGrooveKeys = 35, fillKeyBase = 96;

    // Settings from the parameters alone (what the controls say).
    bm::Settings currentSettings() const;
    // Settings as last played, including note-trigger overrides (for the editor).
    bm::Settings displaySettings() const;

    static juce::StringArray cymbalChoiceNames();

    TriggerMode triggerMode() const;

    // Renders the selected groove + fills into a temp .mid file.
    juce::File exportMidiFile() const;

    bm::NoteMap getNoteMap() const;
    void setNoteMap (const bm::NoteMap&);

    // Call after editing patterns or the note map, so the host marks the project dirty.
    void patternsEdited();

    bm::Library library;
    std::array<std::atomic<int>, bm::NumLanes> noteMap;
    juce::AudioProcessorValueTreeState params;

    std::atomic<double> playheadPpq { -1.0 };   // -1 when stopped
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<int> hostNumerator { 0 }, hostDenominator { 0 };   // 0 = unknown

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const bm::Library&);

    bm::Settings playbackSettings() const;   // audio thread
    void handleTrigger (bool noteOn, int note, double ppq, const juce::AudioPlayHead::PositionInfo*);
    void resetTriggers();
    void publishForEditor();

    std::atomic<float>* pGroove = nullptr;
    std::atomic<float>* pFill = nullptr;
    std::atomic<float>* pRandomFill = nullptr;
    std::atomic<float>* pFillEvery = nullptr;
    std::atomic<float>* pHumanize = nullptr;
    std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pVelocity = nullptr;
    std::atomic<float>* pTrigger = nullptr;
    std::atomic<float>* pCymbal = nullptr;
    std::atomic<float>* pCrashAfter = nullptr;
    std::atomic<float>* pChance = nullptr;
    std::atomic<float>* pKickBalance = nullptr;
    std::atomic<float>* pTiming = nullptr;

    double sampleRate = 44100.0;
    std::array<int, 128> pendingOff {};   // samples until note-off, -1 = none
    double lastEnd = 0.0;
    bool wasPlaying = false;
    juce::MidiBuffer passThrough;

    // Note-trigger state (audio thread only).
    int lastTriggerMode = TriggerOff;
    int latchedGroove = -1;
    std::array<int, maxGrooveKeys> heldOrder {};      // per groove key: 0 = up, else press order
    int pressCounter = 0;
    double origin = 0.0;
    long long forcedFillBar = bm::noBar;
    int forcedFill = 0;

    // Copies of the trigger state for the editor.
    std::atomic<int> uiGroove { -1 };
    std::atomic<bool> uiMuted { false };
    std::atomic<double> uiOrigin { 0.0 };
    std::atomic<long long> uiForcedFillBar { bm::noBar };
    std::atomic<int> uiForcedFill { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlackMetalDrumsProcessor)
};
