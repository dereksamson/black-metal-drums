#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "Engine.h"

namespace bm
{

// Writes one pattern on its own as a type-1 MIDI file (channel 10, with tempo and meter).
bool writePatternMidi (const Settings&, const Pattern&, bool isFill, double bpm, const juce::File&);

// Writes every groove and fill into folder/Grooves and folder/Fills. bpm <= 0 uses each
// pattern's suggested tempo (also added to the file name). Returns the number of files written.
int exportAllPatterns (const Library&, const Settings&, double bpm, const juce::File& folder);

// A tempo that suits the pattern, for files written without a host tempo.
int suggestedTempo (const Pattern&, bool isFill);

// Personal pattern presets: .bmpat files in
// ~/Library/Application Support/BlackMetalDrums/Patterns/{Grooves,Fills}.
juce::File presetFolder (bool fills);
juce::Array<juce::File> listPresets (bool fills);
bool savePreset (const Pattern&, const juce::File&);
bool loadPreset (Pattern&, const juce::File&);   // keeps the slot's name; layout and cells come from the file

} // namespace bm
