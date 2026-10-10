#pragma once

#include "Patterns.h"

namespace bm
{

enum class FillStyle : int { SnareRoll, TomCascade, SnareToToms, Unison, NumStyles };

const char* fillStyleName (FillStyle) noexcept;

// Writes a new fill into the pattern, keeping its length and resolution.
// Each call gives a different variation of the style.
void generateFill (Pattern&, FillStyle, juce::Random&);

} // namespace bm
