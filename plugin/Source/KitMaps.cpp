#include "KitMaps.h"

namespace bm
{

std::vector<KitMap> builtInKitMaps()
{
    NoteMap gm {};
    for (int lane = 0; lane < NumLanes; ++lane)
        gm[(size_t) lane] = defaultNotes[lane];

    // Same, but with the toms one step lower (48/45/43/41), as many 4-tom kits are laid out.
    auto lowToms = gm;
    lowToms[Tom1] = 48;
    lowToms[Tom2] = 45;
    lowToms[Tom3] = 43;
    lowToms[Floor] = 41;

    return { { "General MIDI", gm, {} }, { "General MIDI - low toms", lowToms, {} } };
}

juce::File userKitMapFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support/BlackMetalDrums/KitMaps");
}

std::vector<KitMap> userKitMaps()
{
    std::vector<KitMap> maps;
    for (const auto& file : userKitMapFolder().findChildFiles (juce::File::findFiles, false, "*.bmkit"))
    {
        const auto xml = juce::parseXML (file);
        if (xml == nullptr || ! xml->hasTagName ("KitMap"))
            continue;

        KitMap map { file.getFileNameWithoutExtension(), {}, file };
        for (int lane = 0; lane < NumLanes; ++lane)
            map.notes[(size_t) lane] = juce::jlimit (0, 127, xml->getIntAttribute (laneIds[lane], defaultNotes[lane]));
        maps.push_back (std::move (map));
    }

    std::sort (maps.begin(), maps.end(), [] (const KitMap& a, const KitMap& b) { return a.name.compareIgnoreCase (b.name) < 0; });
    return maps;
}

bool saveKitMap (const juce::File& file, const NoteMap& notes)
{
    juce::XmlElement xml ("KitMap");
    for (int lane = 0; lane < NumLanes; ++lane)
        xml.setAttribute (laneIds[lane], notes[(size_t) lane]);
    file.getParentDirectory().createDirectory();
    return xml.writeTo (file);
}

} // namespace bm
