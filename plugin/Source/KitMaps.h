#pragma once

#include "Patterns.h"

namespace bm
{

using NoteMap = std::array<int, NumLanes>;

// A named lane -> MIDI note assignment. Built-ins ship with the plugin; user maps are
// .bmkit files in ~/Library/Application Support/BlackMetalDrums/KitMaps.
struct KitMap
{
    juce::String name;
    NoteMap notes;
    juce::File file;   // empty for built-ins
};

std::vector<KitMap> builtInKitMaps();
std::vector<KitMap> userKitMaps();
juce::File userKitMapFolder();
bool saveKitMap (const juce::File&, const NoteMap&);

} // namespace bm
