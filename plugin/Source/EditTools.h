#pragma once

#include "Patterns.h"

namespace bm
{

// Undo/redo over pattern edits (message thread only). Call beginEdit() just before
// changing a pattern; undo() puts back what it was then.
class EditHistory
{
public:
    void beginEdit (Pattern&);
    void clear()                            { undoStack.clear(); redoStack.clear(); }

    bool canUndo() const noexcept           { return ! undoStack.empty(); }
    bool canRedo() const noexcept           { return ! redoStack.empty(); }

    // Return the pattern that changed, or nullptr if there was nothing to do.
    Pattern* undo();
    Pattern* redo();

private:
    struct Entry { Pattern* pattern; PatternSnapshot state; };
    static constexpr size_t maxEntries = 200;
    std::vector<Entry> undoStack, redoStack;
};

// One bar of all lanes, or one lane of a whole pattern. Pasting moves each hit to the
// nearest step of the target's grid, so clips work across meters and resolutions.
struct BarClip
{
    bool valid = false;
    Layout layout;
    PatternSnapshot cells;   // the bar's steps, starting at index 0
};

struct LaneClip
{
    bool valid = false;
    Layout layout;
    std::array<uint8_t, maxSteps> cells {}, chances {};
};

BarClip copyBar (const Pattern&, int bar);
void pasteBar (Pattern&, int bar, const BarClip&);
void clearBar (Pattern&, int bar);
void copyBarToAllBars (Pattern&, int bar);

LaneClip copyLane (const Pattern&, int lane);
void pasteLane (Pattern&, int lane, const LaneClip&);
void clearLane (Pattern&, int lane);

} // namespace bm
