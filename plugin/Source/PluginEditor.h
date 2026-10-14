#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "SongEditor.h"

// "Bone & blood": monochrome bone on black; blood is kept for what is live or emphasised
// (playhead, accents, toggled-on state, warnings), never for decoration.
namespace bm::colours
{
    const juce::Colour background { 0xff070707 };
    const juce::Colour panel      { 0xff0e0e0f };
    const juce::Colour cellEmpty  { 0xff19181a };
    const juce::Colour outline    { 0xff2c2a29 };
    const juce::Colour text       { 0xffe8e2d6 };   // bone
    const juce::Colour dimText    { 0xff8c877e };
    const juce::Colour blood      { 0xffb3121b };
}

// Embedded typefaces: a blackletter for titles and headings, a condensed sans for everything else.
namespace bm::fonts
{
    juce::FontOptions blackletter (float height);
    juce::FontOptions ui (float height, bool bold = false);
}

class BlackMetalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlackMetalLookAndFeel();

    juce::Font getLabelFont (juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool vertical,
                        int thumbStart, int thumbSize, bool mouseOver, bool mouseDown) override;

    // TextButtons carrying this property are drawn as headings (blackletter), e.g. the editor tabs.
    static constexpr const char* headingProperty = "bmHeading";
};

// Lane-by-step editor for one pattern; lives in a Viewport, lane names stay pinned left.
//   click: add / remove   drag up-down: velocity   alt-click: ghost   cmd-click: accent
//   shift-drag: chance (shift-click toggles 100% / 50%)   right-click: copy/paste bars and lanes
//   click a lane name: choose its MIDI note
class StepGrid : public juce::Component
{
public:
    static constexpr int labelWidth = 130;
    static constexpr int minCellWidth = 14;

    explicit StepGrid (BlackMetalDrumsProcessor&);

    void setPattern (bm::Pattern*);
    bm::Pattern* getPattern() const noexcept    { return pattern; }
    void setPlayStep (int step);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    float scrollX() const;
    juce::Rectangle<float> cellArea() const;
    bool cellAt (juce::Point<float>, int& lane, int& step) const;
    int laneAt (float y) const;
    void showNoteMenu (int lane);
    void showCellMenu (int lane, int step);
    void beginEdit();
    void edited();

    BlackMetalDrumsProcessor& bmProcessor;
    bm::Pattern* pattern = nullptr;
    int playStep = -1;

    int dragLane = -1, dragStep = -1, dragStartVelocity = 0;
    bool dragCreated = false, dragMoved = false, dragChance = false;
};

class GridViewport : public juce::Viewport
{
public:
    std::function<void()> onScroll;
    void visibleAreaChanged (const juce::Rectangle<int>&) override    { if (onScroll) onScroll(); }
};

// Drag this into Logic's tracks area to drop the current groove (+ fills) as a MIDI region.
class MidiDragTile : public juce::Component
{
public:
    explicit MidiDragTile (BlackMetalDrumsProcessor& p) : bmProcessor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override    { dragging = false; }

private:
    BlackMetalDrumsProcessor& bmProcessor;
    bool dragging = false;
};

// Offline quick reference shown over the editor; click outside the panel or Close to dismiss.
class HelpOverlay : public juce::Component
{
public:
    explicit HelpOverlay (const juce::String& text);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override   { if (! panel.contains (e.getPosition())) setVisible (false); }

private:
    juce::TextEditor body;
    juce::TextButton closeButton { "Close" };
    juce::Rectangle<int> panel;
};

class BlackMetalDrumsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BlackMetalDrumsEditor (BlackMetalDrumsProcessor&);
    ~BlackMetalDrumsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void syncGridPattern();
    void updateGridSize();
    void refreshLayoutControls();
    void applyLayoutFromControls();
    void updateStatus();
    void showKitMapMenu();
    void saveKitMapAs();
    juce::String currentKitMapName() const;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String& text);
    void setupCaption (juce::Label&, const juce::String& text);
    void saveMidiAs();
    void showGenerateMenu();
    void buildPatternMenus();
    void updateTabs();
    void syncPatternMenus();
    void setChoiceParam (const char* id, int index);
    void undoOrRedo (bool undo);
    void showPresetMenu();
    void showHelpMenu();
    juce::String quickReference() const;
    void savePresetAs();
    void exportAll();

    BlackMetalLookAndFeel lookAndFeel;
    BlackMetalDrumsProcessor& bmProcessor;

    // Row 1: playback
    juce::ComboBox grooveBox, fillBox, fillEveryBox;
    juce::ToggleButton randomFillButton { "Random fill" };
    juce::ComboBox cymbalBox;
    juce::ToggleButton crashAfterButton { "Crash after fill" };
    juce::Slider humanizeKnob, accentKnob, velocityKnob, timingKnob, kickKnob, chanceKnob;
    juce::Label grooveCaption, fillCaption, fillEveryCaption, humanizeCaption, accentCaption, velocityCaption,
                timingCaption, kickCaption, chanceCaption;

    // Row 2: kit map + note triggers
    juce::TextButton kitMapButton { "Kit map" };
    juce::Label kitMapLabel, triggerCaption, triggerInfo;
    juce::ComboBox triggerBox, timeFeelBox;
    juce::Slider swingSlider;
    juce::Label timeFeelCaption, swingCaption;

    // Grid toolbar
    juce::TextButton grooveTab { "Edit groove" }, fillTab { "Edit fill" }, songTab { "Song" };
    juce::ComboBox meterBox, barsBox, lengthBox, resolutionBox;
    juce::Label statusLabel;
    juce::TextButton generateButton { "Generate..." }, presetsButton { "Presets..." }, resetButton { "Reset" }, clearButton { "Clear" };
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" };

    GridViewport viewport;
    StepGrid grid;
    SongEditor songEditor;

    MidiDragTile dragTile;
    juce::TextButton saveButton { "Save .mid..." }, exportAllButton { "Export all..." }, previewButton { "Preview" };
    juce::Label hintLabel;
    juce::TextButton helpButton { "?" };
    std::unique_ptr<HelpOverlay> helpOverlay;

    std::unique_ptr<ComboAttachment> fillEveryAttachment, triggerAttachment,
                                     cymbalAttachment, timeFeelAttachment;
    std::unique_ptr<ButtonAttachment> randomFillAttachment, crashAfterAttachment;
    std::unique_ptr<SliderAttachment> humanizeAttachment, accentAttachment, velocityAttachment, timingAttachment,
                                      kickAttachment, chanceAttachment, swingAttachment;
    juce::Random fillRandom;
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<bm::KitMap> knownKitMaps;
    uint32_t shownLayout = 0;
    bm::Pattern* shownPattern = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlackMetalDrumsEditor)
};
