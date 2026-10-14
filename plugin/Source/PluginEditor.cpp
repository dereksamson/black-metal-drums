#include "PluginEditor.h"
#include "FillGenerator.h"
#include "PatternFiles.h"

using namespace bm;

namespace
{
    const char* const guideUrl  = "https://github.com/dereksamson/black-metal-drums/blob/main/docs/USER_GUIDE.md";
    const char* const issuesUrl = "https://github.com/dereksamson/black-metal-drums/issues";
    const char* const repoUrl   = "https://github.com/dereksamson/black-metal-drums";

    struct Meter { int num, den; };
    const Meter meters[]      = { { 2, 4 }, { 3, 4 }, { 4, 4 }, { 5, 4 }, { 5, 8 }, { 6, 8 }, { 7, 8 }, { 9, 8 }, { 12, 8 } };
    const int barChoices[]    = { 1, 2, 3, 4, 5 };
    const int fillEighths[]   = { 1, 2, 3, 4, 6, 8 };
    const char* fillLengthNames[] = { "1/8", "1/4", "3/8", "1/2", "3/4", "4/4" };

    juce::String noteName (int note)    { return juce::MidiMessage::getMidiNoteName (note, true, true, 3); }
}

//==============================================================================
BlackMetalLookAndFeel::BlackMetalLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::background);
    setColour (juce::ComboBox::backgroundColourId, colours::panel);
    setColour (juce::ComboBox::outlineColourId, colours::outline);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::arrowColourId, colours::blood);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::headerTextColourId, colours::dimText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::blood.darker (0.4f));
    setColour (juce::TextButton::buttonColourId, colours::panel);
    setColour (juce::TextButton::buttonOnColourId, colours::blood.darker (0.3f));
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ToggleButton::textColourId, colours::text);
    setColour (juce::ToggleButton::tickColourId, colours::blood);
    setColour (juce::ToggleButton::tickDisabledColourId, colours::dimText);
    setColour (juce::Slider::rotarySliderFillColourId, colours::blood);
    setColour (juce::Slider::rotarySliderOutlineColourId, colours::outline);
    setColour (juce::Slider::thumbColourId, colours::text);
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::ScrollBar::thumbColourId, colours::outline);
}

//==============================================================================
StepGrid::StepGrid (BlackMetalDrumsProcessor& p) : bmProcessor (p) {}

void StepGrid::setPattern (Pattern* newPattern)
{
    pattern = newPattern;
    playStep = -1;
    repaint();
}

void StepGrid::setPlayStep (int step)
{
    if (step != playStep)
    {
        playStep = step;
        repaint();
    }
}

float StepGrid::scrollX() const
{
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        return (float) vp->getViewPositionX();
    return 0.0f;
}

juce::Rectangle<float> StepGrid::cellArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft ((float) labelWidth);
}

int StepGrid::laneAt (float y) const
{
    const int lane = (int) (y / ((float) getHeight() / NumLanes));
    return juce::isPositiveAndBelow (lane, (int) NumLanes) ? lane : -1;
}

bool StepGrid::cellAt (juce::Point<float> pos, int& lane, int& step) const
{
    if (pattern == nullptr)
        return false;
    const auto area = cellArea();
    const int numSteps = pattern->layout().numSteps();
    if (! area.contains (pos))
        return false;
    lane = laneAt (pos.y);
    step = (int) ((pos.x - area.getX()) / (area.getWidth() / (float) numSteps));
    return lane >= 0 && juce::isPositiveAndBelow (step, numSteps);
}

void StepGrid::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);
    if (pattern == nullptr)
        return;

    const auto layout = pattern->layout();
    const int numSteps = layout.numSteps();
    const int perBar = layout.stepsPerBar();
    const double beat = layout.beatQuarters();

    const auto area = cellArea();
    const float rowH = (float) getHeight() / NumLanes;
    const float colW = area.getWidth() / (float) numSteps;
    const float h = (float) getHeight();
    auto stepX = [&] (int step) { return area.getX() + colW * (float) step; };

    auto beatIndex = [&] (int step) { return (int) std::floor ((step % perBar) * layout.stepLength() / beat + 1.0e-6); };
    auto isBeatStart = [&] (int step)
    {
        const double pos = (step % perBar) * layout.stepLength();
        const double off = std::fmod (pos, beat);
        return off < 1.0e-6 || beat - off < 1.0e-6;
    };

    // Shade alternate beats so the grid reads in beats (dotted quarters in 6/8, 9/8, 12/8).
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    for (int step = 0; step < numSteps; ++step)
        if (beatIndex (step) % 2 == 1)
            g.fillRect (stepX (step), 0.0f, colW, h);

    if (juce::isPositiveAndBelow (playStep, numSteps))
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (stepX (playStep), 0.0f, colW, h);
    }

    for (int lane = 0; lane < NumLanes; ++lane)
    {
        const float y = rowH * (float) lane;
        for (int step = 0; step < numSteps; ++step)
        {
            const auto cell = juce::Rectangle<float> (stepX (step), y, colW, rowH).reduced (colW > 8.0f ? 1.5f : 0.5f);
            const int v = pattern->get (lane, step);

            g.setColour (colours::cellEmpty);
            g.fillRoundedRectangle (cell, 2.0f);

            if (v > 0)
            {
                const float level = (float) v / 127.0f;
                const int chance = pattern->getChance (lane, step);
                const float alpha = chance < 100 ? 0.35f + 0.55f * (float) chance / 100.0f : 1.0f;
                g.setColour (colours::blood.withMultipliedBrightness (0.45f + 0.75f * level).withAlpha (alpha));
                g.fillRoundedRectangle (cell, 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.35f));   // velocity bar
                const float barH = (cell.getHeight() - 4.0f) * level;
                g.fillRect (cell.getX() + 2.0f, cell.getBottom() - 2.0f - barH, 2.0f, barH);

                if (chance < 100)   // dashed outline + percentage marks a chance step
                {
                    juce::Path outline, dashed;
                    outline.addRoundedRectangle (cell.reduced (0.5f), 2.0f);
                    const float dashes[] = { 2.0f, 2.0f };
                    juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
                    g.setColour (juce::Colours::white.withAlpha (0.7f));
                    g.fillPath (dashed);
                    if (colW >= 22.0f)
                    {
                        g.setFont (juce::FontOptions (9.0f));
                        g.drawText (juce::String (chance), cell, juce::Justification::centred);
                    }
                }
            }
        }
    }

    // Beat and bar lines.
    for (int step = 0; step <= numSteps; ++step)
    {
        const bool barLine = step % perBar == 0;
        if (! barLine && ! isBeatStart (step))
            continue;
        g.setColour (barLine ? colours::blood.withAlpha (0.8f) : colours::outline);
        g.fillRect (stepX (step) - (barLine ? 1.0f : 0.5f), 0.0f, barLine ? 2.0f : 1.0f, h);
    }

    // Lane names, pinned to the left edge of the visible area.
    const float lx = scrollX();
    g.setColour (colours::panel);
    g.fillRect (lx, 0.0f, (float) labelWidth, h);
    for (int lane = 0; lane < NumLanes; ++lane)
    {
        const float y = rowH * (float) lane;
        const int note = bmProcessor.noteMap[(size_t) lane].load();
        g.setColour (colours::text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (laneNames[lane], juce::Rectangle<float> (lx + 8.0f, y, (float) labelWidth - 60.0f, rowH),
                    juce::Justification::centredLeft);
        g.setColour (colours::dimText);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String (note) + " " + noteName (note),
                    juce::Rectangle<float> (lx + (float) labelWidth - 62.0f, y, 56.0f, rowH),
                    juce::Justification::centredRight);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    dragLane = -1;

    if (e.position.x < scrollX() + (float) labelWidth)
    {
        if (const int lane = laneAt (e.position.y); lane >= 0)
            showNoteMenu (lane);
        return;
    }

    int lane = 0, step = 0;
    if (! cellAt (e.position, lane, step))
        return;

    if (e.mods.isPopupMenu())
    {
        showCellMenu (lane, step);
        return;
    }

    const int v = pattern->get (lane, step);
    beginEdit();   // one undo step per click or drag

    if (e.mods.isAltDown() || e.mods.isCommandDown())
    {
        const int target = e.mods.isAltDown() ? 45 : 120;
        pattern->set (lane, step, v == target ? 0 : target);
        edited();
        return;
    }

    dragLane = lane;
    dragStep = step;
    dragMoved = false;
    dragChance = e.mods.isShiftDown() && v > 0;
    if (dragChance)
    {
        dragCreated = false;
        dragStartVelocity = pattern->getChance (lane, step);
        return;
    }
    dragCreated = (v == 0);
    if (dragCreated)
    {
        pattern->set (lane, step, 100);
        edited();
    }
    dragStartVelocity = pattern->get (lane, step);
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLane < 0)
        return;
    const int dy = e.getDistanceFromDragStartY();
    if (std::abs (dy) > 3)
        dragMoved = true;
    if (dragMoved)
    {
        if (dragChance)
            pattern->setChance (dragLane, dragStep, juce::jlimit (5, 100, dragStartVelocity - dy / 2));
        else
            pattern->set (dragLane, dragStep, juce::jlimit (1, 127, dragStartVelocity - dy / 2));
        edited();
    }
}

void StepGrid::mouseUp (const juce::MouseEvent&)
{
    if (dragLane >= 0 && ! dragMoved && dragChance)
    {
        pattern->setChance (dragLane, dragStep, pattern->getChance (dragLane, dragStep) == 100 ? 50 : 100);
        edited();
    }
    else if (dragLane >= 0 && ! dragMoved && ! dragCreated)
    {
        pattern->set (dragLane, dragStep, 0);
        pattern->setChance (dragLane, dragStep, 100);
        edited();
    }
    dragLane = -1;
}

void StepGrid::showNoteMenu (int lane)
{
    juce::PopupMenu menu;
    menu.addSectionHeader (juce::String (laneNames[lane]) + " sends note:");
    const int current = bmProcessor.noteMap[(size_t) lane].load();

    juce::PopupMenu below, gm, above;
    for (int note = 0; note < 35; ++note)
        below.addItem (note + 1, juce::String (note) + "  (" + noteName (note) + ")", true, note == current);
    for (int note = 35; note <= 81; ++note)
        gm.addItem (note + 1, juce::String (note) + "  " + juce::MidiMessage::getRhythmInstrumentName (note)
                                  + "  (" + noteName (note) + ")", true, note == current);
    for (int note = 82; note < 128; ++note)
        above.addItem (note + 1, juce::String (note) + "  (" + noteName (note) + ")", true, note == current);

    menu.addSubMenu ("0 - 34", below, true, nullptr, current < 35);
    menu.addSubMenu ("35 - 81 (General MIDI drums)", gm, true, nullptr, current >= 35 && current <= 81);
    menu.addSubMenu ("82 - 127", above, true, nullptr, current > 81);
    menu.addSeparator();
    menu.addItem (1000, "Reset to General MIDI (" + juce::String (defaultNotes[lane]) + ")");

    juce::Component::SafePointer<StepGrid> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safe, lane] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;
                            safe->bmProcessor.noteMap[(size_t) lane] = result == 1000 ? defaultNotes[lane] : result - 1;
                            safe->edited();
                        });
}

void StepGrid::showCellMenu (int lane, int step)
{
    const auto l = pattern->layout();
    const int bar = step / l.stepsPerBar();
    const juce::String barName = l.bars > 1 ? "bar " + juce::String (bar + 1) : juce::String ("bar");
    const juce::String laneName = juce::String (laneNames[lane]).toLowerCase();
    auto& proc = bmProcessor;

    juce::PopupMenu menu;
    menu.addItem (1, "Copy " + barName);
    menu.addItem (2, "Paste into " + barName, proc.barClipboard.valid);
    menu.addItem (3, "Clear " + barName);
    menu.addItem (4, "Copy " + barName + " to all bars", l.bars > 1);
    menu.addSeparator();
    menu.addItem (10, "Copy " + laneName + " lane");
    menu.addItem (11, "Paste into " + laneName + " lane", proc.laneClipboard.valid);
    menu.addItem (12, "Clear " + laneName + " lane");

    const int chance = pattern->getChance (lane, step);
    if (pattern->get (lane, step) > 0)
    {
        menu.addSeparator();
        menu.addSectionHeader ("Chance of this hit");
        for (int i = 0; i < 4; ++i)
            menu.addItem (20 + i, juce::String (100 - 25 * i) + "%", true, chance == 100 - 25 * i);
    }

    juce::Component::SafePointer<StepGrid> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safe, lane, step, bar] (int result)
                        {
                            if (safe == nullptr || result == 0 || safe->pattern == nullptr)
                                return;
                            auto& self = *safe;
                            auto& p = *self.pattern;
                            auto& pr = self.bmProcessor;

                            if (result == 1)  { pr.barClipboard = bm::copyBar (p, bar); return; }
                            if (result == 10) { pr.laneClipboard = bm::copyLane (p, lane); return; }

                            self.beginEdit();
                            switch (result)
                            {
                                case 2:  bm::pasteBar (p, bar, pr.barClipboard); break;
                                case 3:  bm::clearBar (p, bar); break;
                                case 4:  bm::copyBarToAllBars (p, bar); break;
                                case 11: bm::pasteLane (p, lane, pr.laneClipboard); break;
                                case 12: bm::clearLane (p, lane); break;
                                default: p.setChance (lane, step, 100 - 25 * (result - 20)); break;
                            }
                            self.edited();
                        });
}

void StepGrid::beginEdit()
{
    if (pattern != nullptr)
        bmProcessor.history.beginEdit (*pattern);
}

void StepGrid::edited()
{
    bmProcessor.patternsEdited();
    repaint();
}

//==============================================================================
void MidiDragTile::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (isMouseOver() ? colours::blood.darker (0.5f) : colours::panel);
    g.fillRoundedRectangle (r, 6.0f);

    juce::Path border;
    border.addRoundedRectangle (r, 6.0f);
    juce::Path dashed;
    const float dashes[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.5f).createDashedStroke (dashed, border, dashes, 2);
    g.setColour (colours::blood);
    g.fillPath (dashed);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
    g.drawText ("DRAG MIDI INTO LOGIC", r.withTrimmedBottom (r.getHeight() * 0.45f), juce::Justification::centredBottom);
    g.setColour (colours::dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("selected groove + fills", r.withTrimmedTop (r.getHeight() * 0.58f), juce::Justification::centredTop);
}

void MidiDragTile::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6)
        return;
    dragging = true;
    const auto file = bmProcessor.exportMidiFile();
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
HelpOverlay::HelpOverlay (const juce::String& text)
{
    body.setMultiLine (true);
    body.setReadOnly (true);
    body.setScrollbarsShown (true);
    body.setCaretVisible (false);
    body.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    body.setColour (juce::TextEditor::backgroundColourId, colours::panel);
    body.setColour (juce::TextEditor::textColourId, colours::text);
    body.setColour (juce::TextEditor::outlineColourId, colours::outline);
    body.setText (text, false);
    addAndMakeVisible (body);

    closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closeButton);
}

void HelpOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));
    g.setColour (colours::background);
    g.fillRoundedRectangle (panel.toFloat(), 8.0f);
    g.setColour (colours::blood);
    g.drawRoundedRectangle (panel.toFloat(), 8.0f, 1.5f);
    g.setColour (colours::text);
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText ("QUICK REFERENCE", panel.withHeight (40).reduced (16, 0), juce::Justification::centredLeft);
}

void HelpOverlay::resized()
{
    panel = getLocalBounds().reduced (90, 50);
    auto r = panel.reduced (16);
    auto top = r.removeFromTop (28);
    closeButton.setBounds (top.removeFromRight (80));
    r.removeFromTop (8);
    body.setBounds (r);
}

//==============================================================================
BlackMetalDrumsEditor::BlackMetalDrumsEditor (BlackMetalDrumsProcessor& p)
    : AudioProcessorEditor (&p), bmProcessor (p), grid (p), songEditor (p), dragTile (p)
{
    setLookAndFeel (&lookAndFeel);

    // Row 1
    buildPatternMenus();
    grooveBox.onChange = [this] { setChoiceParam ("groove", grooveBox.getSelectedId() - 1); };
    fillBox.onChange = [this] { setChoiceParam ("fill", fillBox.getSelectedId() - 1); };
    fillEveryBox.addItemList ({ "Off", "1 bar", "2 bars", "4 bars", "8 bars" }, 1);
    for (auto* box : { &grooveBox, &fillBox, &fillEveryBox })
        addAndMakeVisible (*box);
    addAndMakeVisible (randomFillButton);

    setupCaption (grooveCaption, "GROOVE");
    setupCaption (fillCaption, "FILL");
    setupCaption (fillEveryCaption, "FILL EVERY");
    setupKnob (humanizeKnob, humanizeCaption, "HUMANIZE");
    setupKnob (accentKnob, accentCaption, "ACCENT");
    setupKnob (velocityKnob, velocityCaption, "VELOCITY");
    setupKnob (timingKnob, timingCaption, "TIMING");
    setupKnob (kickKnob, kickCaption, "KICK L/R");
    setupKnob (chanceKnob, chanceCaption, "CHANCE");
    humanizeKnob.setTooltip ("Random velocity variation");
    accentKnob.setTooltip ("Extra velocity on the beat");
    timingKnob.setTooltip ("Random timing variation (ms)");
    kickKnob.setTooltip ("Velocity taken off the second foot in double-kick runs");
    chanceKnob.setTooltip ("How much per-step chances apply (0% = every hit always plays)");

    for (const auto& name : BlackMetalDrumsProcessor::cymbalChoiceNames())
        cymbalBox.addItem ("Cymbal: " + name, cymbalBox.getNumItems() + 1);
    cymbalBox.setTooltip ("Move the groove's cymbal part to another cymbal (the downbeat crash stays)");
    addAndMakeVisible (cymbalBox);
    crashAfterButton.setTooltip ("Crash on the downbeat after each fill");
    addAndMakeVisible (crashAfterButton);

    // Row 2
    kitMapButton.setButtonText ("Kit map...");
    kitMapButton.onClick = [this] { showKitMapMenu(); };
    addAndMakeVisible (kitMapButton);
    kitMapLabel.setColour (juce::Label::textColourId, colours::text);
    addAndMakeVisible (kitMapLabel);

    triggerBox.addItemList ({ "Off", "Latch", "Hold", "Latch (ch 16)", "Hold (ch 16)" }, 1);
    triggerBox.setTooltip ("Latch/Hold: keys below and above the drum range (first 35 grooves). "
                           "Ch 16: only notes on MIDI channel 16 trigger, and every groove has a key");
    addAndMakeVisible (triggerBox);
    setupCaption (triggerCaption, "NOTE TRIGGER");
    triggerCaption.setJustificationType (juce::Justification::centredRight);
    triggerInfo.setColour (juce::Label::textColourId, colours::dimText);
    triggerInfo.setFont (juce::FontOptions (11.5f));
    addAndMakeVisible (triggerInfo);

    timeFeelBox.addItemList ({ "Normal", "Half-time", "Double-time" }, 1);
    timeFeelBox.setTooltip ("Play every pattern at half or twice the speed, without editing it");
    addAndMakeVisible (timeFeelBox);
    setupCaption (timeFeelCaption, "TIME FEEL");
    timeFeelCaption.setJustificationType (juce::Justification::centredRight);
    swingSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
    swingSlider.setTooltip ("Delays every second 8th/16th/32nd towards the triplet position (straight grids only)");
    addAndMakeVisible (swingSlider);
    setupCaption (swingCaption, "SWING");
    swingCaption.setJustificationType (juce::Justification::centredRight);

    auto& apvts = bmProcessor.params;
    timeFeelAttachment = std::make_unique<ComboAttachment> (apvts, "timeFeel", timeFeelBox);
    swingAttachment    = std::make_unique<SliderAttachment> (apvts, "swing", swingSlider);
    fillEveryAttachment   = std::make_unique<ComboAttachment> (apvts, "fillEvery", fillEveryBox);
    triggerAttachment     = std::make_unique<ComboAttachment> (apvts, "trigger", triggerBox);
    randomFillAttachment  = std::make_unique<ButtonAttachment> (apvts, "randomFill", randomFillButton);
    humanizeAttachment    = std::make_unique<SliderAttachment> (apvts, "humanize", humanizeKnob);
    accentAttachment      = std::make_unique<SliderAttachment> (apvts, "accent", accentKnob);
    velocityAttachment    = std::make_unique<SliderAttachment> (apvts, "velocity", velocityKnob);
    timingAttachment      = std::make_unique<SliderAttachment> (apvts, "timing", timingKnob);
    kickAttachment        = std::make_unique<SliderAttachment> (apvts, "kickBalance", kickKnob);
    chanceAttachment      = std::make_unique<SliderAttachment> (apvts, "chance", chanceKnob);
    cymbalAttachment      = std::make_unique<ComboAttachment> (apvts, "cymbal", cymbalBox);
    crashAfterAttachment  = std::make_unique<ButtonAttachment> (apvts, "crashAfterFill", crashAfterButton);

    // Grid toolbar
    songTab.setTooltip ("Arrange sections into a song");
    for (auto* tab : { &grooveTab, &fillTab, &songTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (1);
        tab->onClick = [this] { syncGridPattern(); updateTabs(); };
        addAndMakeVisible (*tab);
    }
    addChildComponent (songEditor);
    grooveTab.setToggleState (true, juce::dontSendNotification);

    for (int i = 0; i < (int) std::size (meters); ++i)
        meterBox.addItem (juce::String (meters[i].num) + "/" + juce::String (meters[i].den), i + 1);
    for (int i = 0; i < (int) std::size (barChoices); ++i)
        barsBox.addItem (juce::String (barChoices[i]) + (barChoices[i] == 1 ? " bar" : " bars"), i + 1);
    for (int i = 0; i < (int) std::size (fillEighths); ++i)
        lengthBox.addItem (juce::String ("Length ") + fillLengthNames[i], i + 1);
    for (int i = 0; i < (int) Resolution::NumResolutions; ++i)
        resolutionBox.addItem (resolutionName ((Resolution) i), i + 1);

    meterBox.setTooltip ("Time signature of this groove");
    resolutionBox.setTooltip ("Grid resolution (T = triplet)");
    for (auto* box : { &meterBox, &barsBox, &lengthBox, &resolutionBox })
    {
        box->onChange = [this] { applyLayoutFromControls(); };
        addAndMakeVisible (*box);
    }

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    resetButton.onClick = [this]
    {
        if (auto* pat = grid.getPattern())
        {
            bmProcessor.history.beginEdit (*pat);
            pat->reset();
            bmProcessor.patternsEdited();
            syncGridPattern();
            grid.repaint();
        }
    };
    clearButton.onClick = [this]
    {
        if (auto* pat = grid.getPattern())
        {
            bmProcessor.history.beginEdit (*pat);
            pat->clear();
            bmProcessor.patternsEdited();
            grid.repaint();
        }
    };
    undoButton.onClick = [this] { undoOrRedo (true); };
    redoButton.onClick = [this] { undoOrRedo (false); };
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);

    generateButton.setTooltip ("Replace this fill with a generated one (keeps its length and resolution)");
    generateButton.onClick = [this] { showGenerateMenu(); };
    addAndMakeVisible (generateButton);
    presetsButton.setTooltip ("Save this pattern to your library, or load one of your saved patterns into this slot");
    presetsButton.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetsButton);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (clearButton);

    viewport.setViewedComponent (&grid, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (10);
    viewport.onScroll = [this] { grid.repaint(); };
    addAndMakeVisible (viewport);

    // Bottom
    saveButton.onClick = [this] { saveMidiAs(); };
    addAndMakeVisible (saveButton);
    exportAllButton.setTooltip ("Write every groove and fill as separate MIDI files into a folder");
    exportAllButton.onClick = [this] { exportAll(); };
    addAndMakeVisible (exportAllButton);
    previewButton.setClickingTogglesState (true);
    previewButton.setTooltip ("Play the selected groove while Logic is stopped (the fill tab plays its fill every bar)");
    previewButton.onClick = [this] { bmProcessor.previewOn = previewButton.getToggleState(); };
    addAndMakeVisible (previewButton);
    hintLabel.setText ("Click: add/remove  -  Drag: velocity  -  Alt: ghost  -  Cmd: accent  -  Shift-drag: chance  -  "
                       "Right-click: copy/paste  -  Lane name: MIDI note",
                       juce::dontSendNotification);
    hintLabel.setColour (juce::Label::textColourId, colours::dimText);
    hintLabel.setFont (juce::FontOptions (11.5f));
    addAndMakeVisible (hintLabel);
    addAndMakeVisible (dragTile);

    helpButton.setTooltip ("User guide, quick reference and support");
    helpButton.onClick = [this] { showHelpMenu(); };
    addAndMakeVisible (helpButton);

    knownKitMaps = builtInKitMaps();
    for (auto& m : userKitMaps())
        knownKitMaps.push_back (m);

    setSize (1180, 720);
    syncGridPattern();
    updateStatus();
    startTimerHz (30);
}

BlackMetalDrumsEditor::~BlackMetalDrumsEditor()
{
    stopTimer();
    bmProcessor.previewOn = false;
    setLookAndFeel (nullptr);
}

void BlackMetalDrumsEditor::setupCaption (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    label.setColour (juce::Label::textColourId, colours::dimText);
    addAndMakeVisible (label);
}

void BlackMetalDrumsEditor::setupKnob (juce::Slider& knob, juce::Label& caption, const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible (knob);
    setupCaption (caption, text);
    caption.setJustificationType (juce::Justification::centred);
}

//==============================================================================
// The groove tab follows what is playing (a triggered groove when note triggers are on).
void BlackMetalDrumsEditor::syncGridPattern()
{
    const auto s = bmProcessor.displaySettings();
    auto* wanted = fillTab.getToggleState() ? bmProcessor.library.fills[(size_t) s.fill].get()
                                            : bmProcessor.library.grooves[(size_t) s.groove].get();
    const auto layout = wanted->layout().pack();

    if (wanted != shownPattern || layout != shownLayout)
    {
        shownPattern = wanted;
        shownLayout = layout;
        grid.setPattern (wanted);
        refreshLayoutControls();
        updateGridSize();
    }
}

void BlackMetalDrumsEditor::updateGridSize()
{
    if (shownPattern == nullptr)
        return;
    const int needed = StepGrid::labelWidth + shownPattern->layout().numSteps() * StepGrid::minCellWidth;
    const bool scrolls = needed > viewport.getWidth();
    grid.setSize (juce::jmax (viewport.getWidth(), needed),
                  viewport.getHeight() - (scrolls ? viewport.getScrollBarThickness() : 0));
}

void BlackMetalDrumsEditor::refreshLayoutControls()
{
    if (shownPattern == nullptr)
        return;

    const auto l = shownPattern->layout();
    const bool isFill = fillTab.getToggleState();

    meterBox.setVisible (! isFill);
    barsBox.setVisible (! isFill);
    lengthBox.setVisible (isFill);
    generateButton.setVisible (isFill);
    updateTabs();

    for (int i = 0; i < (int) std::size (meters); ++i)
        if (meters[i].num == l.num && meters[i].den == l.den)
            meterBox.setSelectedId (i + 1, juce::dontSendNotification);
    for (int i = 0; i < (int) std::size (barChoices); ++i)
        if (barChoices[i] == l.bars)
            barsBox.setSelectedId (i + 1, juce::dontSendNotification);
    for (int i = 0; i < (int) std::size (fillEighths); ++i)
        if (std::abs (l.lengthQuarters() - fillEighths[i] * 0.5) < 1.0e-9)
            lengthBox.setSelectedId (i + 1, juce::dontSendNotification);

    resolutionBox.setSelectedId ((int) l.res + 1, juce::dontSendNotification);
    for (int i = 0; i < (int) Resolution::NumResolutions; ++i)
    {
        auto candidate = l;
        candidate.res = (Resolution) i;
        resolutionBox.setItemEnabled (i + 1, candidate.isValid());
    }
}

void BlackMetalDrumsEditor::buildPatternMenus()
{
    grooveBox.clear (juce::dontSendNotification);
    addGrooveItems (grooveBox, bmProcessor.library);
    fillBox.clear (juce::dontSendNotification);
    addFillItems (fillBox, bmProcessor.library);
    syncPatternMenus();
}

// Menu item IDs are pattern index + 1, so the grouped order doesn't affect the parameters.
void BlackMetalDrumsEditor::syncPatternMenus()
{
    const auto s = bmProcessor.currentSettings();
    if (grooveBox.getSelectedId() != s.groove + 1)
        grooveBox.setSelectedId (s.groove + 1, juce::dontSendNotification);
    if (fillBox.getSelectedId() != s.fill + 1)
        fillBox.setSelectedId (s.fill + 1, juce::dontSendNotification);
}

void BlackMetalDrumsEditor::setChoiceParam (const char* id, int index)
{
    if (index < 0)
        return;
    if (auto* param = bmProcessor.params.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
}

// Undo/redo, then show the pattern that changed so the edit is visible.
void BlackMetalDrumsEditor::undoOrRedo (bool undo)
{
    const auto target = undo ? bmProcessor.history.undo() : bmProcessor.history.redo();
    if (! target)
        return;
    if (target.song)
    {
        songTab.setToggleState (true, juce::sendNotification);
        bmProcessor.songEdited();
        songEditor.refreshIfChanged();
        return;
    }
    auto* changed = target.pattern;

    const auto& lib = bmProcessor.library;
    for (size_t i = 0; i < lib.fills.size(); ++i)
        if (lib.fills[i].get() == changed)
        {
            setChoiceParam ("fill", (int) i);
            fillTab.setToggleState (true, juce::sendNotification);
        }
    for (size_t i = 0; i < lib.grooves.size(); ++i)
        if (lib.grooves[i].get() == changed)
        {
            setChoiceParam ("groove", (int) i);
            grooveTab.setToggleState (true, juce::sendNotification);
        }

    bmProcessor.patternsEdited();
    syncPatternMenus();
    syncGridPattern();
    grid.repaint();
}

// The Song tab swaps the grid and its tools for the song editor.
void BlackMetalDrumsEditor::updateTabs()
{
    const bool song = songTab.getToggleState();
    const bool isFill = fillTab.getToggleState();
    songEditor.setVisible (song);
    viewport.setVisible (! song);
    meterBox.setVisible (! song && ! isFill);
    barsBox.setVisible (! song && ! isFill);
    lengthBox.setVisible (! song && isFill);
    generateButton.setVisible (! song && isFill);
    for (auto* c : std::initializer_list<juce::Component*> { &resolutionBox, &presetsButton, &resetButton, &clearButton })
        c->setVisible (! song);
}

void BlackMetalDrumsEditor::showHelpMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "User guide (opens in your browser)");
    menu.addItem (2, "Quick reference");
    menu.addSeparator();
    menu.addItem (3, "Report a problem or suggest a pattern...");
    menu.addItem (4, "Project page on GitHub");
    menu.addSeparator();
    menu.addItem (5, juce::String ("Black Metal Drums ") + JucePlugin_VersionString + "  -  GNU AGPLv3", false, false);

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&helpButton),
                        [safe] (int result)
                        {
                            if (safe == nullptr)
                                return;
                            switch (result)
                            {
                                case 1: juce::URL (guideUrl).launchInDefaultBrowser(); break;
                                case 3: juce::URL (issuesUrl).launchInDefaultBrowser(); break;
                                case 4: juce::URL (repoUrl).launchInDefaultBrowser(); break;
                                case 2:
                                    safe->helpOverlay = std::make_unique<HelpOverlay> (safe->quickReference());
                                    safe->addAndMakeVisible (*safe->helpOverlay);
                                    safe->helpOverlay->setBounds (safe->getLocalBounds());
                                    break;
                                default: break;
                            }
                        });
}

juce::String BlackMetalDrumsEditor::quickReference() const
{
    const auto& lib = bmProcessor.library;
    juce::String t;
    auto line = [&t] (const juce::String& s) { t << s << "\n"; };

    line ("Full guide: ? > User guide (online).");
    line ({});
    line ("GRID");
    line ("  Click              add / remove a hit");
    line ("  Drag up / down     velocity");
    line ("  Alt-click          ghost note        Cmd-click   accent");
    line ("  Shift-drag         chance of playing (Shift-click: 100% <-> 50%)");
    line ("  Right-click        copy / paste / clear a bar or a lane, set chance");
    line ("  Click a drum name  choose the MIDI note it sends (Kit map... saves it)");
    line ({});
    line ("TOOLBAR");
    line ("  Meter, bars or length, resolution (T = triplet). Undo / Redo cover grid and song edits.");
    line ("  Presets: your own library.  Generate: new fill.  Reset: factory pattern.");
    line ("  Preview: plays while Logic is stopped (keep the track selected).");
    line ({});
    line ("FEEL");
    line ("  Humanize: velocity variation   Accent: on the beat   Velocity: overall level");
    line ("  Timing: +/- ms                 Kick L/R: weaker second foot");
    line ("  Chance: how much per-step chances apply (0% = every hit plays)");
    line ("  Time feel: half-time / double-time.  Swing: off-steps toward the triplet (straight grids)");
    line ({});
    line ("ARRANGING");
    line ("  Automation: Groove, Fill and Fill every are automatable.");
    line ("  Note trigger Latch / Hold: draw groove keys (from C-2) and fill keys (from C6).");
    line ("    The 'ch 16' modes listen to MIDI channel 16 only and give every groove a key.");
    line ("    Latch: until the next groove key.  Hold: only while held (gaps are silent).");
    line ("  Song tab: sections of groove x bars with fills; turn on Song mode, play from bar 1.");
    line ({});
    line ("EXPORT");
    line ("  Drag MIDI tile -> Logic tracks area.  Save .mid.  Export all: every pattern as files.");
    line ({});
    line ("NO SOUND?");
    line ("  Select the track; Logic doesn't always run plugins on an empty track that isn't selected.");
    line ("  Wrong drum sounds: click the drum's name in the grid and pick the note your kit uses.");
    line ({});

    auto keyName = [] (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 3).paddedRight (' ', 6); };
    line ("GROOVE KEYS (note trigger; grooves after the first 35 need a 'ch 16' mode)");
    for (size_t i = 0; i < lib.grooves.size(); ++i)
        line ("  " + keyName (BlackMetalDrumsProcessor::grooveKeyBase + (int) i) + lib.grooves[i]->name
              + ((int) i >= BlackMetalDrumsProcessor::maxGrooveKeys ? "   (ch 16)" : ""));
    line ({});
    line ("FILL KEYS (note trigger)");
    for (size_t i = 0; i < lib.fills.size(); ++i)
        line ("  " + keyName (BlackMetalDrumsProcessor::fillKeyBase + (int) i) + lib.fills[i]->name);
    return t;
}

void BlackMetalDrumsEditor::showPresetMenu()
{
    if (shownPattern == nullptr)
        return;
    const bool fills = fillTab.getToggleState();
    const auto files = listPresets (fills);

    juce::PopupMenu menu, deleteMenu;
    menu.addItem (500, "Save this " + juce::String (fills ? "fill" : "groove") + " as...");
    if (! files.isEmpty())
    {
        menu.addSectionHeader ("Load into \"" + shownPattern->name + "\"");
        for (int i = 0; i < files.size(); ++i)
        {
            menu.addItem (1 + i, files[i].getFileNameWithoutExtension());
            deleteMenu.addItem (300 + i, files[i].getFileNameWithoutExtension());
        }
        menu.addSeparator();
        menu.addSubMenu ("Delete saved " + juce::String (fills ? "fill" : "groove"), deleteMenu);
    }
    menu.addItem (501, "Show preset folder");

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetsButton),
                        [safe, files, fills] (int result)
                        {
                            if (safe == nullptr || result == 0 || safe->shownPattern == nullptr)
                                return;
                            auto& self = *safe;
                            if (result == 500)
                                self.savePresetAs();
                            else if (result == 501)
                            {
                                presetFolder (fills).createDirectory();
                                presetFolder (fills).revealToUser();
                            }
                            else if (juce::isPositiveAndBelow (result - 300, files.size()))
                                files[result - 300].moveToTrash();
                            else if (juce::isPositiveAndBelow (result - 1, files.size()))
                            {
                                self.bmProcessor.history.beginEdit (*self.shownPattern);
                                loadPreset (*self.shownPattern, files[result - 1]);
                                self.bmProcessor.patternsEdited();
                                self.syncGridPattern();
                                self.refreshLayoutControls();
                                self.grid.repaint();
                            }
                        });
}

void BlackMetalDrumsEditor::savePresetAs()
{
    const bool fills = fillTab.getToggleState();
    presetFolder (fills).createDirectory();
    fileChooser = std::make_unique<juce::FileChooser> (
        "Save pattern", presetFolder (fills).getChildFile (fileNameFor (shownPattern->name) + ".bmpat"), "*.bmpat");

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    auto* pattern = shownPattern;
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe, pattern] (const juce::FileChooser& chooser)
                              {
                                  const auto target = chooser.getResult();
                                  if (safe != nullptr && target != juce::File())
                                      savePreset (*pattern, target.withFileExtension ("bmpat"));
                              });
}

void BlackMetalDrumsEditor::exportAll()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Choose a folder for all patterns",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Black Metal Drums MIDI"));

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& chooser)
                              {
                                  auto folder = chooser.getResult();
                                  if (safe == nullptr || folder == juce::File())
                                      return;
                                  if (folder.existsAsFile())
                                      folder = folder.getParentDirectory();
                                  // Uses the current kit map and humanize settings, and each pattern's suggested tempo.
                                  exportAllPatterns (safe->bmProcessor.library, safe->bmProcessor.currentSettings(), 0.0, folder);
                                  folder.revealToUser();
                              });
}

void BlackMetalDrumsEditor::showGenerateMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Generate fill");
    for (int i = 0; i < (int) FillStyle::NumStyles; ++i)
        menu.addItem (i + 1, fillStyleName ((FillStyle) i));

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&generateButton),
                        [safe] (int result)
                        {
                            if (safe == nullptr || result == 0 || safe->shownPattern == nullptr || ! safe->fillTab.getToggleState())
                                return;
                            safe->bmProcessor.history.beginEdit (*safe->shownPattern);
                            generateFill (*safe->shownPattern, (FillStyle) (result - 1), safe->fillRandom);
                            safe->bmProcessor.patternsEdited();
                            safe->grid.repaint();
                        });
}

void BlackMetalDrumsEditor::applyLayoutFromControls()
{
    if (shownPattern == nullptr)
        return;

    auto l = shownPattern->layout();
    if (fillTab.getToggleState())
    {
        if (const int i = lengthBox.getSelectedId() - 1; juce::isPositiveAndBelow (i, (int) std::size (fillEighths)))
        {
            l.num = fillEighths[i];
            l.den = 8;
            l.bars = 1;
        }
    }
    else
    {
        if (const int i = meterBox.getSelectedId() - 1; juce::isPositiveAndBelow (i, (int) std::size (meters)))
        {
            l.num = meters[i].num;
            l.den = meters[i].den;
        }
        if (const int i = barsBox.getSelectedId() - 1; juce::isPositiveAndBelow (i, (int) std::size (barChoices)))
            l.bars = barChoices[i];
    }
    if (const int i = resolutionBox.getSelectedId() - 1; juce::isPositiveAndBelow (i, (int) Resolution::NumResolutions))
        l.res = (Resolution) i;

    // If the resolution doesn't divide the new meter (e.g. 1/8 T in 7/8), use the first one that does.
    for (int i = (int) Resolution::Sixteenth; ! l.isValid() && i >= 0; --i)
        l.res = (Resolution) i;
    for (int i = 0; ! l.isValid() && i < (int) Resolution::NumResolutions; ++i)
        l.res = (Resolution) i;

    if (l != shownPattern->layout())
        bmProcessor.history.beginEdit (*shownPattern);
    shownPattern->setLayout (l);
    bmProcessor.patternsEdited();
    syncGridPattern();
    refreshLayoutControls();
}

juce::String BlackMetalDrumsEditor::currentKitMapName() const
{
    const auto current = bmProcessor.getNoteMap();
    for (auto& m : knownKitMaps)
        if (m.notes == current)
            return m.name;
    return "Custom (unsaved)";
}

void BlackMetalDrumsEditor::showKitMapMenu()
{
    knownKitMaps = builtInKitMaps();
    const auto builtIns = (int) knownKitMaps.size();
    const auto users = userKitMaps();
    for (auto& m : users)
        knownKitMaps.push_back (m);

    const auto current = bmProcessor.getNoteMap();
    juce::PopupMenu menu;
    menu.addSectionHeader ("Built-in");
    for (int i = 0; i < builtIns; ++i)
        menu.addItem (i + 1, knownKitMaps[(size_t) i].name, true, knownKitMaps[(size_t) i].notes == current);

    juce::PopupMenu deleteMenu;
    if (! users.empty())
    {
        menu.addSectionHeader ("Saved");
        for (int i = 0; i < (int) users.size(); ++i)
        {
            menu.addItem (100 + i, users[(size_t) i].name, true, users[(size_t) i].notes == current);
            deleteMenu.addItem (300 + i, users[(size_t) i].name);
        }
    }
    menu.addSeparator();
    menu.addItem (500, "Save current map as...");
    if (! users.empty())
        menu.addSubMenu ("Delete saved map", deleteMenu);
    menu.addItem (501, "Show kit map folder");

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&kitMapButton),
                        [safe, builtIns, users] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;
                            auto& self = *safe;
                            if (result <= builtIns)
                                self.bmProcessor.setNoteMap (self.knownKitMaps[(size_t) result - 1].notes);
                            else if (juce::isPositiveAndBelow (result - 100, (int) users.size()))
                                self.bmProcessor.setNoteMap (users[(size_t) result - 100].notes);
                            else if (juce::isPositiveAndBelow (result - 300, (int) users.size()))
                                users[(size_t) result - 300].file.moveToTrash();
                            else if (result == 500)
                                self.saveKitMapAs();
                            else if (result == 501)
                            {
                                userKitMapFolder().createDirectory();
                                userKitMapFolder().revealToUser();
                            }

                            self.knownKitMaps = builtInKitMaps();
                            for (auto& m : userKitMaps())
                                self.knownKitMaps.push_back (m);
                            self.grid.repaint();
                        });
}

void BlackMetalDrumsEditor::saveKitMapAs()
{
    userKitMapFolder().createDirectory();
    fileChooser = std::make_unique<juce::FileChooser> ("Save kit map", userKitMapFolder().getChildFile ("My kit.bmkit"), "*.bmkit");

    juce::Component::SafePointer<BlackMetalDrumsEditor> safe (this);
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& chooser)
                              {
                                  const auto target = chooser.getResult();
                                  if (safe == nullptr || target == juce::File())
                                      return;
                                  saveKitMap (target.withFileExtension ("bmkit"), safe->bmProcessor.getNoteMap());
                                  safe->knownKitMaps = builtInKitMaps();
                                  for (auto& m : userKitMaps())
                                      safe->knownKitMaps.push_back (m);
                              });
}

void BlackMetalDrumsEditor::updateStatus()
{
    kitMapLabel.setText (currentKitMapName(), juce::dontSendNotification);

    const auto mode = bmProcessor.triggerMode();
    const int base = BlackMetalDrumsProcessor::grooveKeyBase;
    const int numGrooves = bmProcessor.numGrooveKeys();
    const int numFills = (int) bmProcessor.library.fills.size();
    const int fillBase = BlackMetalDrumsProcessor::fillKeyBase;

    triggerInfo.setText (mode == BlackMetalDrumsProcessor::TriggerOff
                             ? juce::String ("Groove and fill follow the menus")
                             : juce::String (bmProcessor.triggersOnChannel16() ? "Ch 16: " : "Keys: ")
                                   + "grooves " + noteName (base) + "-" + noteName (base + numGrooves - 1) + ", fills "
                                   + noteName (fillBase) + "-" + noteName (fillBase + numFills - 1),
                         juce::dontSendNotification);

    // Status: what is playing, and whether Logic's meter matches the groove.
    const auto s = bmProcessor.displaySettings();
    const auto& groove = *bmProcessor.library.grooves[(size_t) s.groove];
    const auto gl = groove.layout();
    const int hostNum = bmProcessor.hostNumerator.load(), hostDen = bmProcessor.hostDenominator.load();

    juce::String text;
    auto colour = colours::dimText;
    if (hostNum > 0 && hostDen > 0 && (hostNum != gl.num || hostDen != gl.den))
    {
        text = "Logic is in " + juce::String (hostNum) + "/" + juce::String (hostDen) + " but this groove is "
               + juce::String (gl.num) + "/" + juce::String (gl.den) + " - match them so bars line up";
        colour = colours::blood.brighter (0.6f);
    }
    else if (bmProcessor.songMode())
    {
        const int section = bmProcessor.currentSection();
        text = bmProcessor.song.size() == 0 ? juce::String ("Song mode: no sections yet (Song tab)")
               : section < 0 ? juce::String ("Song mode: past the end of the song")
               : "Song mode: section " + juce::String (section + 1) + " of " + juce::String (bmProcessor.song.size())
                     + "  -  " + groove.name;
    }
    else if (mode != BlackMetalDrumsProcessor::TriggerOff)
    {
        text = s.grooveMuted ? juce::String ("Hold: no groove key down") : "Playing: " + groove.name;
    }
    statusLabel.setText (text, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, colour);
}

void BlackMetalDrumsEditor::timerCallback()
{
    syncPatternMenus();
    syncGridPattern();
    undoButton.setEnabled (bmProcessor.history.canUndo());
    redoButton.setEnabled (bmProcessor.history.canRedo());
    bmProcessor.previewFillEveryBar = fillTab.getToggleState();
    songEditor.refreshIfChanged();
    songEditor.setPlayingSection (bmProcessor.songMode() && bmProcessor.playheadPpq.load() >= 0.0
                                      ? bmProcessor.currentSection() : -1);
    updateStatus();

    const double ppq = bmProcessor.patternPlayhead();
    if (ppq < 0.0)
    {
        grid.setPlayStep (-1);
        return;
    }

    const auto loc = bm::locate (bmProcessor.library, bmProcessor.displaySettings(), ppq);
    grid.setPlayStep (loc.pattern == grid.getPattern() ? loc.step : -1);
}

void BlackMetalDrumsEditor::saveMidiAs()
{
    const auto temp = bmProcessor.exportMidiFile();
    if (! temp.existsAsFile())
        return;

    fileChooser = std::make_unique<juce::FileChooser> (
        "Save MIDI file",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (temp.getFileName()), "*.mid");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [temp] (const juce::FileChooser& chooser)
                              {
                                  const auto target = chooser.getResult();
                                  if (target != juce::File())
                                      temp.copyFileTo (target.withFileExtension ("mid"));
                              });
}

//==============================================================================
void BlackMetalDrumsEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    auto header = getLocalBounds().removeFromTop (48).reduced (20, 0);
    g.setColour (colours::text);
    g.setFont (juce::FontOptions (22.0f).withStyle ("Bold"));
    g.drawText ("BLACK METAL DRUMS", header, juce::Justification::centredLeft);
    g.setColour (colours::blood);
    g.fillRect (20, 46, 60, 2);

    g.setColour (colours::dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("MIDI FX  -  channel 10", header.withTrimmedRight (44), juce::Justification::centredRight);

    // Separator above the kit / trigger row.
    g.setColour (colours::outline);
    g.fillRect (20, 157, getWidth() - 40, 1);
}

void BlackMetalDrumsEditor::resized()
{
    helpButton.setBounds (getWidth() - 20 - 32, 10, 32, 28);
    if (helpOverlay != nullptr)
        helpOverlay->setBounds (getLocalBounds());

    auto area = getLocalBounds().reduced (20, 0);
    area.removeFromTop (58);

    // Row 1: playback.
    auto row1 = area.removeFromTop (92);
    auto column = [&] (int width) { auto c = row1.removeFromLeft (width); row1.removeFromLeft (12); return c; };

    auto c = column (250);
    grooveCaption.setBounds (c.removeFromTop (18));
    grooveBox.setBounds (c.removeFromTop (30));
    c.removeFromTop (6);
    cymbalBox.setBounds (c.removeFromTop (26));

    c = column (220);
    fillCaption.setBounds (c.removeFromTop (18));
    fillBox.setBounds (c.removeFromTop (30));
    randomFillButton.setBounds (c.removeFromTop (30));

    c = column (130);
    fillEveryCaption.setBounds (c.removeFromTop (18));
    fillEveryBox.setBounds (c.removeFromTop (30));
    crashAfterButton.setBounds (c.removeFromTop (30));

    for (auto [knob, caption] : { std::pair { &humanizeKnob, &humanizeCaption },
                                  std::pair { &accentKnob, &accentCaption },
                                  std::pair { &velocityKnob, &velocityCaption },
                                  std::pair { &timingKnob, &timingCaption },
                                  std::pair { &kickKnob, &kickCaption },
                                  std::pair { &chanceKnob, &chanceCaption } })
    {
        c = column (64);
        caption->setBounds (c.removeFromTop (18));
        knob->setBounds (c);
    }

    // Row 2: kit map + note triggers.
    area.removeFromTop (14);
    auto row2 = area.removeFromTop (30);
    kitMapButton.setBounds (row2.removeFromLeft (100));
    row2.removeFromLeft (8);
    kitMapLabel.setBounds (row2.removeFromLeft (190));
    triggerCaption.setBounds (row2.removeFromLeft (100));
    row2.removeFromLeft (6);
    triggerBox.setBounds (row2.removeFromLeft (90));
    row2.removeFromLeft (12);
    swingSlider.setBounds (row2.removeFromRight (170));
    swingCaption.setBounds (row2.removeFromRight (56));
    row2.removeFromRight (12);
    timeFeelBox.setBounds (row2.removeFromRight (120));
    row2.removeFromRight (6);
    timeFeelCaption.setBounds (row2.removeFromRight (76));
    row2.removeFromRight (12);
    triggerInfo.setBounds (row2);

    // Grid toolbar.
    area.removeFromTop (14);
    auto tools = area.removeFromTop (30);
    grooveTab.setBounds (tools.removeFromLeft (110));
    tools.removeFromLeft (6);
    fillTab.setBounds (tools.removeFromLeft (110));
    tools.removeFromLeft (6);
    songTab.setBounds (tools.removeFromLeft (80));
    tools.removeFromLeft (16);
    auto layoutControls = tools.removeFromLeft (80 + 6 + 90);
    meterBox.setBounds (layoutControls.removeFromLeft (80));
    layoutControls.removeFromLeft (6);
    barsBox.setBounds (layoutControls);
    lengthBox.setBounds (meterBox.getBounds().getUnion (barsBox.getBounds()));
    tools.removeFromLeft (6);
    resolutionBox.setBounds (tools.removeFromLeft (80));
    tools.removeFromLeft (16);
    undoButton.setBounds (tools.removeFromLeft (60));
    tools.removeFromLeft (6);
    redoButton.setBounds (tools.removeFromLeft (60));
    clearButton.setBounds (tools.removeFromRight (70));
    tools.removeFromRight (6);
    resetButton.setBounds (tools.removeFromRight (70));
    tools.removeFromRight (6);
    generateButton.setBounds (tools.removeFromRight (96));
    tools.removeFromRight (6);
    presetsButton.setBounds (tools.removeFromRight (86));

    // Bottom bar.
    auto bottom = area.removeFromBottom (76).withTrimmedTop (12).withTrimmedBottom (14);
    dragTile.setBounds (bottom.removeFromLeft (240));
    bottom.removeFromLeft (12);
    saveButton.setBounds (bottom.removeFromLeft (110).withSizeKeepingCentre (110, 30));
    bottom.removeFromLeft (8);
    exportAllButton.setBounds (bottom.removeFromLeft (100).withSizeKeepingCentre (100, 30));
    bottom.removeFromLeft (8);
    previewButton.setBounds (bottom.removeFromLeft (90).withSizeKeepingCentre (90, 30));
    bottom.removeFromLeft (16);
    statusLabel.setBounds (bottom.removeFromTop (bottom.getHeight() / 2));
    hintLabel.setBounds (bottom);

    area.removeFromTop (8);
    viewport.setBounds (area);
    songEditor.setBounds (area);
    updateGridSize();
}
