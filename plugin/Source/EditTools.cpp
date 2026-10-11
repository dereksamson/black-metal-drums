#include "EditTools.h"

namespace bm
{

EditHistory::Entry EditHistory::capture (Pattern* pattern, Song* song)
{
    Entry e;
    e.pattern = pattern;
    e.song = song;
    if (pattern != nullptr)
        e.state = pattern->snapshot();
    if (song != nullptr)
        e.sections = song->all();
    return e;
}

EditTarget EditHistory::apply (const Entry& e)
{
    if (e.pattern != nullptr)
        e.pattern->restore (e.state);
    if (e.song != nullptr)
        e.song->assign (e.sections);
    return { e.pattern, e.song != nullptr };
}

void EditHistory::push (Entry e)
{
    undoStack.push_back (std::move (e));
    if (undoStack.size() > maxEntries)
        undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void EditHistory::beginEdit (Pattern& p)        { push (capture (&p, nullptr)); }
void EditHistory::beginSongEdit (Song& song)    { push (capture (nullptr, &song)); }

EditTarget EditHistory::undo()
{
    if (undoStack.empty())
        return {};
    auto entry = std::move (undoStack.back());
    undoStack.pop_back();
    redoStack.push_back (capture (entry.pattern, entry.song));
    return apply (entry);
}

EditTarget EditHistory::redo()
{
    if (redoStack.empty())
        return {};
    auto entry = std::move (redoStack.back());
    redoStack.pop_back();
    undoStack.push_back (capture (entry.pattern, entry.song));
    return apply (entry);
}

//==============================================================================
BarClip copyBar (const Pattern& p, int bar)
{
    BarClip clip;
    clip.layout = p.layout();
    const int perBar = clip.layout.stepsPerBar();
    if (! juce::isPositiveAndBelow (bar, clip.layout.bars))
        return clip;

    for (int lane = 0; lane < NumLanes; ++lane)
        for (int k = 0; k < perBar; ++k)
        {
            clip.cells.cells[(size_t) lane][(size_t) k] = (uint8_t) p.get (lane, bar * perBar + k);
            clip.cells.chances[(size_t) lane][(size_t) k] = (uint8_t) p.getChance (lane, bar * perBar + k);
        }
    clip.valid = true;
    return clip;
}

void clearBar (Pattern& p, int bar)
{
    const auto l = p.layout();
    const int perBar = l.stepsPerBar();
    for (int lane = 0; lane < NumLanes; ++lane)
        for (int k = 0; k < perBar; ++k)
        {
            p.set (lane, bar * perBar + k, 0);
            p.setChance (lane, bar * perBar + k, 100);
        }
}

void pasteBar (Pattern& p, int bar, const BarClip& clip)
{
    const auto l = p.layout();
    if (! clip.valid || ! juce::isPositiveAndBelow (bar, l.bars))
        return;

    clearBar (p, bar);
    const int perBar = l.stepsPerBar();
    for (int lane = 0; lane < NumLanes; ++lane)
        for (int k = 0; k < clip.layout.stepsPerBar(); ++k)
        {
            const int v = clip.cells.cells[(size_t) lane][(size_t) k];
            const double pos = k * clip.layout.stepLength();
            if (v == 0 || pos >= l.barQuarters() - 1.0e-9)
                continue;
            const int d = (int) std::round (pos / l.stepLength());
            if (d >= perBar)
                continue;
            const int step = bar * perBar + d;
            if (v > p.get (lane, step))
            {
                p.set (lane, step, v);
                p.setChance (lane, step, clip.cells.chances[(size_t) lane][(size_t) k]);
            }
        }
}

void copyBarToAllBars (Pattern& p, int bar)
{
    const auto clip = copyBar (p, bar);
    for (int b = 0; b < p.layout().bars; ++b)
        if (b != bar)
            pasteBar (p, b, clip);
}

//==============================================================================
LaneClip copyLane (const Pattern& p, int lane)
{
    LaneClip clip;
    clip.layout = p.layout();
    for (int step = 0; step < clip.layout.numSteps(); ++step)
    {
        clip.cells[(size_t) step] = (uint8_t) p.get (lane, step);
        clip.chances[(size_t) step] = (uint8_t) p.getChance (lane, step);
    }
    clip.valid = true;
    return clip;
}

void clearLane (Pattern& p, int lane)
{
    for (int step = 0; step < maxSteps; ++step)
    {
        p.set (lane, step, 0);
        p.setChance (lane, step, 100);
    }
}

// Hits keep their time position; a shorter clip repeats to fill a longer pattern.
void pasteLane (Pattern& p, int lane, const LaneClip& clip)
{
    if (! clip.valid)
        return;

    const auto l = p.layout();
    const double srcLength = clip.layout.lengthQuarters();
    clearLane (p, lane);

    for (int k = 0; k < clip.layout.numSteps(); ++k)
    {
        const int v = clip.cells[(size_t) k];
        if (v == 0)
            continue;
        for (double t = k * clip.layout.stepLength(); t < l.lengthQuarters() - 1.0e-9; t += srcLength)
        {
            const int d = (int) std::round (t / l.stepLength());
            if (d < l.numSteps() && v > p.get (lane, d))
            {
                p.set (lane, d, v);
                p.setChance (lane, d, clip.chances[(size_t) k]);
            }
        }
    }
}

} // namespace bm
