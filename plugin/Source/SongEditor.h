#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

// Grouped pattern menus, shared by the main controls and the song editor.
// Item IDs are pattern index + 1.
void addGrooveItems (juce::ComboBox&, const bm::Library&);
void addFillItems (juce::ComboBox&, const bm::Library&);

// The song arrangement: one row per section (groove, bars, fills), plus song mode controls.
class SongEditor : public juce::Component
{
public:
    explicit SongEditor (BlackMetalDrumsProcessor&);
    ~SongEditor() override;

    void refreshIfChanged();            // rebuild rows if the song changed elsewhere (e.g. project load)
    void setPlayingSection (int index);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Row;
    static constexpr int rowHeight = 32;
    static constexpr int endFillNone = 1000, endFillRandom = 1001;

    void rebuild();
    void commit (std::vector<bm::Section>);
    std::vector<bm::Section> withRowEdited (int index, const bm::Section&) const;
    void addSection();
    void moveSection (int index, int delta);
    void duplicateSection (int index);
    void removeSection (int index);
    juce::String summary() const;

    BlackMetalDrumsProcessor& bmProcessor;

    juce::ToggleButton songModeButton { "Song mode (plays the arrangement)" }, loopButton { "Loop song" };
    juce::TextButton addButton { "+ Add section" };
    juce::Label summaryLabel, emptyLabel;
    std::array<juce::Label, 4> headers;
    juce::Viewport viewport;
    juce::Component rowHolder;
    juce::OwnedArray<Row> rows;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> songModeAttachment, loopAttachment;
    std::vector<uint32_t> shown;   // packed sections the rows were built from
    int playing = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditor)
};
