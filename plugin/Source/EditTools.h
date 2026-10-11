#pragma once

#include "Song.h"

namespace bm
{

// What an undo or redo changed: a pattern, the song, or nothing.
struct EditTarget
{
    Pattern* pattern = nullptr;
    bool song = false;
    explicit operator bool() const noexcept { return pattern != nullptr || song; }
};

// Undo/redo over pattern and song edits (message thread only). Call beginEdit() or
// beginSongEdit() just before changing something; undo() puts back what it was then.
class EditHistory
{
public:
    void beginEdit (Pattern&);
    void beginSongEdit (Song&);
    void clear()                            { undoStack.clear(); redoStack.clear(); }

    bool canUndo() const noexcept           { return ! undoStack.empty(); }
    bool canRedo() const noexcept           { return ! redoStack.empty(); }

    EditTarget undo();
    EditTarget redo();

private:
    struct Entry
    {
        Pattern* pattern = nullptr;
        Song* song = nullptr;
        PatternSnapshot state;
        std::vector<Section> sections;
    };
    static Entry capture (Pattern*, Song*);
    static EditTarget apply (const Entry&);
    void push (Entry);
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
