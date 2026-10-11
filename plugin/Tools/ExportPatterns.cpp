// Writes every factory groove and fill as a MIDI file (General MIDI map, plugin defaults).
//   ExportPatterns <output folder>
#include "../Source/PatternFiles.h"
#include <cstdio>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf ("usage: ExportPatterns <output folder>\n");
        return 1;
    }

    const bm::Library lib;
    bm::Settings s;   // plugin defaults
    for (int lane = 0; lane < bm::NumLanes; ++lane)
        s.notes[(size_t) lane] = bm::defaultNotes[lane];
    s.humanize = 6;
    s.accent = 5;
    s.kickBalance = 6;

    const auto folder = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    const int written = bm::exportAllPatterns (lib, s, 0.0, folder);
    const int expected = (int) (lib.grooves.size() + lib.fills.size());
    std::printf ("Wrote %d of %d files to %s\n", written, expected, folder.getFullPathName().toRawUTF8());
    return written == expected ? 0 : 1;
}
