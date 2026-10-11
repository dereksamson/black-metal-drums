#include "SongEditor.h"
#include "PluginEditor.h"

using namespace bm;

void addGrooveItems (juce::ComboBox& box, const Library& lib)
{
    for (const auto& category : grooveCategoryOrder())
    {
        bool first = true;
        for (size_t i = 0; i < lib.grooves.size(); ++i)
            if (grooveCategory (lib.grooves[i]->name) == category)
            {
                if (std::exchange (first, false))
                    box.addSectionHeading (category);
                box.addItem (lib.grooves[i]->name, (int) i + 1);
            }
    }
}

void addFillItems (juce::ComboBox& box, const Library& lib)
{
    for (const auto& category : fillCategoryOrder())
    {
        bool first = true;
        for (size_t i = 0; i < lib.fills.size(); ++i)
            if (fillCategory (lib.fills[i]->factoryLayout()) == category)
            {
                if (std::exchange (first, false))
                    box.addSectionHeading (category);
                box.addItem (lib.fills[i]->name, (int) i + 1);
            }
    }
}

//==============================================================================
class SongEditor::Row : public juce::Component
{
public:
    Row (SongEditor& o, int i, const Section& s) : owner (o), index (i)
    {
        const auto& lib = owner.bmProcessor.library;

        number.setText (juce::String (i + 1), juce::dontSendNotification);
        number.setJustificationType (juce::Justification::centred);
        number.setColour (juce::Label::textColourId, colours::dimText);
        addAndMakeVisible (number);

        addGrooveItems (groove, lib);
        groove.setSelectedId (s.groove + 1, juce::dontSendNotification);

        for (int b : { 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32 })
            bars.addItem (juce::String (b) + (b == 1 ? " bar" : " bars"), b);
        if (bars.indexOfItemId (s.bars) < 0)
            bars.addItem (juce::String (s.bars) + " bars", s.bars);
        bars.setSelectedId (s.bars, juce::dontSendNotification);

        fillEvery.addItem ("No fills", 1);
        for (int b : { 1, 2, 4, 8 })
            fillEvery.addItem ("Fill every " + juce::String (b), b + 1);
        fillEvery.setSelectedId (s.fillEvery + 1, juce::dontSendNotification);

        endFill.addItem ("No end fill", endFillNone);
        endFill.addItem ("Random end fill", endFillRandom);
        addFillItems (endFill, lib);
        endFill.setSelectedId (s.endFill == Section::noFill ? endFillNone
                               : s.endFill == Section::randomFill ? endFillRandom : s.endFill + 1,
                               juce::dontSendNotification);

        groove.setTooltip ("Groove for this section");
        endFill.setTooltip ("Fill on the last bar of the section");
        for (auto* box : { &groove, &bars, &fillEvery, &endFill })
        {
            box->onChange = [this] { owner.commit (owner.withRowEdited (index, read())); };
            addAndMakeVisible (*box);
        }

        up.setTooltip ("Move up");
        down.setTooltip ("Move down");
        duplicate.setTooltip ("Duplicate");
        remove.setTooltip ("Delete");
        up.onClick        = [this] { owner.moveSection (index, -1); };
        down.onClick      = [this] { owner.moveSection (index, +1); };
        duplicate.onClick = [this] { owner.duplicateSection (index); };
        remove.onClick    = [this] { owner.removeSection (index); };
        for (auto* b : { &up, &down, &duplicate, &remove })
            addAndMakeVisible (*b);
    }

    Section read() const
    {
        Section s;
        s.groove = juce::jmax (0, groove.getSelectedId() - 1);
        s.bars = juce::jmax (1, bars.getSelectedId());
        s.fillEvery = juce::jmax (0, fillEvery.getSelectedId() - 1);
        const int f = endFill.getSelectedId();
        s.endFill = f == endFillNone ? Section::noFill : f == endFillRandom ? Section::randomFill : f - 1;
        return s;
    }

    void paint (juce::Graphics& g) override
    {
        if (highlighted)
        {
            g.setColour (colours::blood.withAlpha (0.25f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (0, 3);
        number.setBounds (r.removeFromLeft (34));
        groove.setBounds (r.removeFromLeft (300));
        r.removeFromLeft (8);
        bars.setBounds (r.removeFromLeft (90));
        r.removeFromLeft (8);
        fillEvery.setBounds (r.removeFromLeft (120));
        r.removeFromLeft (8);
        endFill.setBounds (r.removeFromLeft (260));
        r.removeFromLeft (12);
        for (auto* b : { &up, &down, &duplicate, &remove })
        {
            b->setBounds (r.removeFromLeft (34));
            r.removeFromLeft (4);
        }
    }

    bool highlighted = false;

private:
    SongEditor& owner;
    const int index;
    juce::Label number;
    juce::ComboBox groove, bars, fillEvery, endFill;
    juce::TextButton up { "^" }, down { "v" }, duplicate { "+" }, remove { "x" };
};

//==============================================================================
SongEditor::SongEditor (BlackMetalDrumsProcessor& p) : bmProcessor (p)
{
    songModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (bmProcessor.params, "songMode", songModeButton);
    loopAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (bmProcessor.params, "songLoop", loopButton);
    songModeButton.setTooltip ("When on, playback follows this arrangement from bar 1 instead of the Groove/Fill menus");
    addAndMakeVisible (songModeButton);
    addAndMakeVisible (loopButton);

    addButton.setTooltip ("Add a section with the current Groove and Fill");
    addButton.onClick = [this] { addSection(); };
    addAndMakeVisible (addButton);

    summaryLabel.setColour (juce::Label::textColourId, colours::dimText);
    summaryLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (summaryLabel);

    const char* headerNames[] = { "GROOVE", "LENGTH", "FILLS", "END OF SECTION" };
    for (size_t i = 0; i < headers.size(); ++i)
    {
        headers[i].setText (headerNames[i], juce::dontSendNotification);
        headers[i].setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        headers[i].setColour (juce::Label::textColourId, colours::dimText);
        addAndMakeVisible (headers[i]);
    }

    emptyLabel.setText ("No sections yet. Use \"+ Add section\" to start an arrangement, then turn on Song mode.",
                        juce::dontSendNotification);
    emptyLabel.setJustificationType (juce::Justification::centred);
    emptyLabel.setColour (juce::Label::textColourId, colours::dimText);
    addAndMakeVisible (emptyLabel);

    viewport.setViewedComponent (&rowHolder, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    rebuild();
}

SongEditor::~SongEditor() = default;

std::vector<Section> SongEditor::withRowEdited (int index, const Section& s) const
{
    auto list = bmProcessor.song.all();
    if (juce::isPositiveAndBelow (index, (int) list.size()))
        list[(size_t) index] = s;
    return list;
}

void SongEditor::moveSection (int index, int delta)
{
    auto list = bmProcessor.song.all();
    const int to = index + delta;
    if (! juce::isPositiveAndBelow (index, (int) list.size()) || ! juce::isPositiveAndBelow (to, (int) list.size()))
        return;
    std::swap (list[(size_t) index], list[(size_t) to]);
    commit (list);
}

void SongEditor::duplicateSection (int index)
{
    auto list = bmProcessor.song.all();
    if (juce::isPositiveAndBelow (index, (int) list.size()) && (int) list.size() < Song::maxSections)
        list.insert (list.begin() + index + 1, list[(size_t) index]);
    commit (list);
}

void SongEditor::removeSection (int index)
{
    auto list = bmProcessor.song.all();
    if (juce::isPositiveAndBelow (index, (int) list.size()))
        list.erase (list.begin() + index);
    commit (list);
}

void SongEditor::addSection()
{
    auto list = bmProcessor.song.all();
    if ((int) list.size() >= Song::maxSections)
        return;
    const auto s = bmProcessor.currentSettings();
    list.push_back ({ s.groove, 4, 0, s.fill });
    commit (list);
}

// Rows are rebuilt asynchronously: commit() can be called from inside a row's own callback.
void SongEditor::commit (std::vector<Section> list)
{
    bmProcessor.history.beginSongEdit (bmProcessor.song);
    bmProcessor.song.assign (list);
    bmProcessor.songEdited();
    juce::Component::SafePointer<SongEditor> safe (this);
    juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->rebuild(); });
}

void SongEditor::rebuild()
{
    rows.clear();
    shown.clear();
    const auto list = bmProcessor.song.all();
    for (size_t i = 0; i < list.size(); ++i)
    {
        auto* row = rows.add (new Row (*this, (int) i, list[i]));
        row->highlighted = (int) i == playing;
        rowHolder.addAndMakeVisible (row);
        shown.push_back (list[i].pack());
    }
    emptyLabel.setVisible (list.empty());
    for (auto& h : headers)
        h.setVisible (! list.empty());
    addButton.setEnabled ((int) list.size() < Song::maxSections);
    summaryLabel.setText (summary(), juce::dontSendNotification);
    resized();
}

void SongEditor::refreshIfChanged()
{
    const auto list = bmProcessor.song.all();
    bool changed = list.size() != shown.size();
    for (size_t i = 0; ! changed && i < list.size(); ++i)
        changed = list[i].pack() != shown[i];
    if (changed)
        rebuild();
}

void SongEditor::setPlayingSection (int index)
{
    if (index == playing)
        return;
    playing = index;
    for (int i = 0; i < rows.size(); ++i)
    {
        rows[i]->highlighted = i == playing;
        rows[i]->repaint();
    }
}

juce::String SongEditor::summary() const
{
    const auto& lib = bmProcessor.library;
    const auto placed = placeSections (lib, bmProcessor.song);
    int bars = 0;
    juce::StringArray meters;
    for (int i = 0; i < placed.count; ++i)
    {
        bars += placed[i].section.bars;
        const auto l = lib.grooves[(size_t) placed[i].section.groove]->layout();
        meters.addIfNotAlreadyThere (juce::String (l.num) + "/" + juce::String (l.den));
    }
    if (placed.count == 0)
        return {};

    auto text = juce::String (placed.count) + (placed.count == 1 ? " section, " : " sections, ") + juce::String (bars) + " bars";
    if (meters.size() > 1)
        text << "  -  meters change (" << meters.joinIntoString (", ") << "): set matching time signatures in Logic";
    return text;
}

void SongEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);
}

void SongEditor::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    auto top = r.removeFromTop (30);
    addButton.setBounds (top.removeFromLeft (130));
    top.removeFromLeft (16);
    songModeButton.setBounds (top.removeFromLeft (260));
    loopButton.setBounds (top.removeFromLeft (110));
    summaryLabel.setBounds (top);

    r.removeFromTop (6);
    // Header columns line up with Row::resized().
    auto header = r.removeFromTop (18);
    header.removeFromLeft (34);
    for (auto [label, width] : { std::pair { 0, 300 }, std::pair { 1, 90 }, std::pair { 2, 120 }, std::pair { 3, 260 } })
    {
        headers[(size_t) label].setBounds (header.removeFromLeft (width));
        header.removeFromLeft (8);
    }
    emptyLabel.setBounds (r);
    viewport.setBounds (r);

    const int width = juce::jmax (1, viewport.getWidth() - viewport.getScrollBarThickness());
    rowHolder.setSize (width, rows.size() * rowHeight);
    for (int i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, i * rowHeight, width, rowHeight);
}
