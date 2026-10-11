#include "PatternFiles.h"

namespace bm
{

namespace
{
    constexpr int drumChannel = 10;
    constexpr int ppq = 960;
}

bool writePatternMidi (const Settings& s, const Pattern& p, bool isFill, double bpm, const juce::File& file)
{
    const auto l = p.layout();
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, p.name));
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60'000'000.0 / bpm)));
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (isFill ? 4 : l.num, isFill ? 4 : l.den));

    renderPatternAlone (s, p, isFill, [&] (const Hit& hit)
    {
        const double tick = juce::jmax (0.0, (hit.ppq + hit.offsetMs * bpm / 60'000.0) * ppq);
        seq.addEvent (juce::MidiMessage::noteOn (drumChannel, hit.note, (juce::uint8) hit.velocity), tick);
        seq.addEvent (juce::MidiMessage::noteOff (drumChannel, hit.note), tick + ppq / 16.0);
    });
    seq.updateMatchedPairs();

    auto endOfTrack = juce::MidiMessage::endOfTrack();
    endOfTrack.setTimeStamp (l.lengthQuarters() * ppq);
    seq.addEvent (endOfTrack);

    juce::MidiFile midi;
    midi.setTicksPerQuarterNote (ppq);
    midi.addTrack (seq);

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream stream (file);
    return stream.openedOk() && midi.writeTo (stream, 1);
}

int suggestedTempo (const Pattern& p, bool isFill)
{
    if (isFill)
        return 180;

    const auto& n = p.name;
    if (n.contains ("Doom"))                     return 70;
    if (n.contains ("Half-time atmospheric"))    return 90;
    if (n.contains ("Mid-tempo"))                return 130;
    if (n.startsWith ("12/8"))                   return 110;   // dotted quarter ~ 73

    const auto category = grooveCategory (n);
    if (category == "Blast beats")               return 200;
    if (category == "Polymeter")                 return 120;
    if (category == "Ternary - 4/4 triplets")    return 140;
    if (category == "Ternary - 6/8 & 12/8")      return 180;   // 6/8: dotted quarter = 120
    if (category == "Thrash, heavy & double kick") return 170;
    return 180;
}

int exportAllPatterns (const Library& lib, const Settings& s, double bpm, const juce::File& folder)
{
    int written = 0;
    for (const bool fills : { false, true })
    {
        const auto& list = fills ? lib.fills : lib.grooves;
        const auto dir = folder.getChildFile (fills ? "Fills" : "Grooves");
        for (size_t i = 0; i < list.size(); ++i)
        {
            const auto& p = *list[i];
            const int tempo = bpm > 0.0 ? juce::roundToInt (bpm) : suggestedTempo (p, fills);
            const auto name = juce::String (i + 1).paddedLeft ('0', 2) + " " + p.name + " - " + juce::String (tempo) + "bpm";
            if (writePatternMidi (s, p, fills, tempo, dir.getChildFile (juce::File::createLegalFileName (name) + ".mid")))
                ++written;
        }
    }
    return written;
}

//==============================================================================
juce::File presetFolder (bool fills)
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support/BlackMetalDrums/Patterns")
        .getChildFile (fills ? "Fills" : "Grooves");
}

juce::Array<juce::File> listPresets (bool fills)
{
    auto files = presetFolder (fills).findChildFiles (juce::File::findFiles, false, "*.bmpat");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
    return files;
}

bool savePreset (const Pattern& p, const juce::File& file)
{
    juce::ValueTree root ("BlackMetalDrumsPattern");
    root.setProperty ("version", 1, nullptr);
    root.appendChild (p.toValueTree(), nullptr);
    file.getParentDirectory().createDirectory();
    if (auto xml = root.createXml())
        return xml->writeTo (file);
    return false;
}

bool loadPreset (Pattern& p, const juce::File& file)
{
    const auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName ("BlackMetalDrumsPattern"))
        return false;
    const auto pattern = juce::ValueTree::fromXml (*xml).getChildWithName ("Pattern");
    if (! pattern.isValid())
        return false;
    p.fromValueTree (pattern);
    return true;
}

} // namespace bm
