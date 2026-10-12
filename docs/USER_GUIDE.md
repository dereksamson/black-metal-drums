# Black Metal Drums: user guide

*Black Metal Drums* is a MIDI FX plugin for Logic Pro. It plays metal drum grooves and fills in
time with Logic, and sends them to your drum plugin, such as EZdrummer, Superior Drummer,
GetGood Drums or Logic's Drum Kit Designer. You can:
- play its patterns as they are
- edit them on a step grid
- arrange them into a song
- drag the result into Logic as MIDI

## Contents

1. [Install](#1-install)
2. [Quick start](#2-quick-start)
3. [The window](#3-the-window)
4. [Grooves and fills](#4-grooves-and-fills)
5. [Shaping the feel](#5-shaping-the-feel)
6. [Kit maps: making it work with your drum plugin](#6-kit-maps-making-it-work-with-your-drum-plugin)
7. [Editing patterns](#7-editing-patterns)
8. [Arranging a song](#8-arranging-a-song)
9. [Getting MIDI out](#9-getting-midi-out)
10. [Pattern library and trigger keys](#10-pattern-library-and-trigger-keys)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Install

**Installer:**
1. Download `BlackMetalDrums-<version>.pkg` from
   [Releases](https://github.com/dereksamson/black-metal-drums/releases).
2. Open it. The installer isn't notarised, so the first time, right-click the file and choose
   **Open**.
3. The plugin is installed to `/Library/Audio/Plug-Ins/Components`. It's a universal build, so
   it runs on both Apple Silicon and Intel Macs.

**From source:** see the [repository README](../README.md#build-from-source-plugin).

Restart Logic after installing. To check that Logic sees the plugin, open
**Logic Pro > Settings > Plug-in Manager** and look under **Grosbug**.

## 2. Quick start

1. Create a **Software Instrument** track and load your drum plugin in its Instrument slot.
2. In the **MIDI FX** slot above it, choose **Audio Units > Grosbug > Black Metal Drums**.
3. Set Logic's tempo (blast beats sound right at 180–220 BPM) and press **Play**.
4. Pick a groove from the **Groove** menu and a fill from the **Fill** menu.
5. If a drum sounds wrong (say, a tom where the china should be), set the right note for it.
   See [Kit maps](#6-kit-maps-making-it-work-with-your-drum-plugin).

If the track stays silent, select it. Logic doesn't always run plugins on an empty track that
isn't selected.

## 3. The window

| Area | What it does |
|---|---|
| **Top row** | Groove, cymbal swap, fill, fill frequency, crash after fill, and the feel knobs |
| **Second row** | Kit map, note-trigger mode and key ranges, time feel, swing |
| **Tabs** | **Edit groove** and **Edit fill** show the step grid; **Song** shows the arrangement |
| **Grid toolbar** | Meter, bars or length, resolution, Undo/Redo, Presets, Generate, Reset, Clear |
| **Grid** | One row per drum, one column per step. The playing step is highlighted |
| **Bottom bar** | Drag MIDI, Save .mid, Export all, Preview, the status line, and a reminder of the mouse controls |
| **? button** (top right) | This guide, a quick reference, and a link to report a problem |

## 4. Grooves and fills

- **Groove**: the beat that loops. The menu is grouped by style:
  - blast beats, black metal, second wave
  - atmospheric, DSBM and blackgaze
  - war metal, thrash and heavy, death metal, doom
  - polymeter, ternary (triplets, 6/8 and 12/8), and 3/4
- **Fill**: grouped by length, from 1/8 of a bar to a full bar.
- **Fill every**: plays the fill at the end of every 1, 2, 4 or 8 bars, replacing the end of
  that bar. **Off** means no fills.
- **Random fill**: picks a different fill each time. The same bar always gets the same fill.
- **Crash after fill**: adds a crash on the next downbeat, unless the groove already has one there.
- **Cymbal**: moves the groove's whole cymbal part to the ride, ride bell, crash, china, hi-hat
  or open hi-hat. The downbeat crash stays a crash, and fills are left alone.

Every control can be automated in Logic. For example, automating **Groove** switches the beat
at the chorus.

## 5. Shaping the feel

| Knob | Effect |
|---|---|
| **Humanize** | Random velocity variation |
| **Accent** | Extra velocity on the beat |
| **Velocity** | Overall level, 50–150 % |
| **Timing** | Random timing variation, ± ms. Off by default, which keeps the tight, "triggered" sound of modern black metal |
| **Kick L/R** | Makes every second kick in a double-kick run weaker, like a real weaker foot |
| **Chance** | How much the per-step chances apply (see [Chance](#chance)). At 0 % every hit always plays |

Two more controls on the second row change the feel of every pattern without editing it:

- **Time feel**:
  - **Half-time** plays patterns at half speed. A blast becomes a half-time blast over the
    same riff, with each pattern bar spanning two of Logic's bars.
  - **Double-time** plays them twice as fast.
  - Fills, song sections and note triggers follow the new speed, and exports contain what
    you hear.
- **Swing**: delays every second step towards the triplet position. 100 % gives a full
  shuffle, so a straight d-beat or blast slides towards a ternary feel. It works on straight
  grids (1/8, 1/16, 1/32). Triplet grids and 6/8, 9/8 and 12/8 grooves are already ternary and
  stay as they are.

None of this variation is random from one playback to the next. A given bar sounds the same
every time you play it and in every export, but different bars vary.

## 6. Kit maps: making it work with your drum plugin

Each row of the grid (crash, crash 2, china, ride, ride bell, hi-hat, open hi-hat, snare, three
toms, floor tom, kick) sends one MIDI note. By default these are General MIDI notes, which most
drum plugins understand.

- **Change a drum's note:** click the drum's name on the left of the grid and pick a note.
- **Kit map...** has two built-in maps: **General MIDI**, and **General MIDI - low toms**
  (toms on 48/45/43/41).
- **Save your kit's mapping:** once each drum sounds right, choose
  **Kit map... > Save current map as...**. The map then appears in this menu in every project.
- The label next to the button shows which map is active, or **Custom (unsaved)**.

Saved maps are `.bmkit` files in `~/Library/Application Support/BlackMetalDrums/KitMaps`.

## 7. Editing patterns

Choose **Edit groove** or **Edit fill**. The grid shows the selected groove or fill; edits are
heard on the next pass and saved with your Logic project.

### Mouse controls

| Action | Result |
|---|---|
| Click | Add or remove a hit |
| Drag up/down on a hit | Velocity (the white bar shows it) |
| Alt-click | Ghost note |
| Cmd-click | Accent |
| Shift-drag on a hit | Chance of playing; Shift-click switches between 100 % and 50 % |
| Right-click | Copy/paste a bar or a lane, set chance |
| Click a drum name | Choose the MIDI note it sends |

### Meter, length and resolution

- **Grooves:**
  - meters: 2/4, 3/4, 4/4, 5/4, 5/8, 6/8, 7/8, 9/8 or 12/8
  - length: 1 to 5 bars
- **Fills:** length from 1/8 of a bar to a full 4/4 bar.
- **Resolution:** 1/8, 1/8 T, 1/16, 1/16 T or 1/32. T means triplet: 1/8 T puts three steps on
  each beat, and 1/16 T puts six.
- Changing any of these keeps your hits where they fall in time. Adding bars repeats the
  existing ones.
- Set Logic's time signature to match the groove. The status line warns when they differ.

### Chance

A hit with a chance below 100 % sometimes doesn't play. Chance hits have a dashed outline.
Use chance for variable kicks, occasional ghost notes, or a cymbal that comes and goes. The
**Chance** knob scales every chance in the plugin at once, from fully applied down to off.

### Copy and paste (right-click)

- **Bars:** copy, paste, clear, or copy a bar to all bars.
- **Lanes:** copy, paste or clear.
- The clipboard works between patterns.
- Pasting keeps time positions:
  - a 4/4 bar pasted into a 3/4 bar loses its last beat
  - a one-beat lane pasted into a 2-bar groove repeats to fill it

### Generate, Reset, Clear

- **Generate...** (fill tab only) writes a new fill in one of four styles: snare roll, tom
  cascade, snare into toms, or kick and snare stabs. It keeps the fill's length and resolution,
  and each click gives a new variation.
- **Reset** restores the factory pattern.
- **Clear** empties the grid.

### Presets

**Presets...** saves the pattern to your personal library, or loads a saved pattern into this
slot. Loading replaces the slot's content; **Reset** brings back the factory version. Presets
are `.bmpat` files in `~/Library/Application Support/BlackMetalDrums/Patterns`, available in
every project.

### Undo and Redo

**Undo** and **Redo** cover every edit: grid clicks and drags, paste, Generate, Reset, Clear,
presets, layout changes, and song sections. Undo switches to the pattern or song that changed.

### Preview

**Preview** plays the selected groove while Logic is stopped. On the fill tab, it plays the
fill every bar so you can hear it in context. Keep the track selected.

## 8. Arranging a song

There are three ways to change grooves over a song.

### a. Automation

Automate the **Groove**, **Fill** and **Fill every** parameters in Logic. This is simple, but
the groove keeps its own phase.

### b. Note triggers

Set **Note trigger** to one of four modes:

| Mode | Which notes trigger | Grooves with a key |
|---|---|---|
| **Latch** / **Hold** | Notes on any channel, below and above the drum range | The first 35 |
| **Latch (ch 16)** / **Hold (ch 16)** | Only notes on MIDI channel 16 | All of them |

In the channel-16 modes, set the trigger notes' channel to 16 in Logic's Piano Roll. Notes on
other channels, including drum notes you draw, pass straight through to the drum plugin.

Then draw notes on the track:
- **Groove keys** start at **C-2** (note 0), one key per groove in the order of the key table. A groove key
  selects that groove and restarts it on the bar line. A key up to a 16th early still counts for
  that bar.
- **Fill keys** start at **C6** (note 96). A fill key plays that fill at the end of the bar the
  note is in.
- **Latch**: a groove keeps playing until the next groove key.
- **Hold**: a groove plays only while its key is held. Draw one long note per section, and the
  gaps are silent.
- In the default modes, trigger keys sit outside the drum range, so drum notes you draw
  alongside them still reach your drum plugin.

See the [key table](#10-pattern-library-and-trigger-keys).

### c. Song mode

Open the **Song** tab:
1. **+ Add section** adds a section with the current groove and fill. Each section has:
   - a groove
   - a length (1–32 bars)
   - fills every N bars
   - an **end of section** fill: a chosen fill, random, or none
2. Use the row buttons to move a section up or down (`^` `v`), duplicate it (`+`) or delete it (`x`).
3. Turn on **Song mode**. Playback now follows the arrangement from bar 1 of the project:
   - the playing section is highlighted
   - **Loop song** repeats the song; otherwise it goes silent after the last section
   - the Groove and Fill menus and note triggers are ignored in this mode, but the feel knobs,
     cymbal swap and kit map still apply
   - sections can use different meters; set the same time signature changes in Logic

## 9. Getting MIDI out

- **Drag MIDI into Logic:** drag the tile onto the tracks area to get a MIDI region. It contains
  the selected groove with its fills, or the whole song in song mode, with time signature
  changes. You can then edit the region like any other.
- **Save .mid...** writes the same thing to a file.
- **Export all...** writes all 49 grooves and 26 fills as separate files into a folder you
  choose. The export uses your kit map and humanize settings, and each file name includes a
  suggested tempo.
- The repository's [midi/](../midi/) folder has every pattern as a General MIDI file, ready
  for any DAW.

## 10. Pattern library and trigger keys

The "-style" patterns are in the spirit of a band or drummer, not transcriptions. Key names
use Logic's convention, where middle C (note 60) is C3.

| Key | Groove | Suggested tempo | Trigger modes |
|---|---|---|---|
| C-2 (0) | Blast - Traditional (ride) | 200 | any channel |
| C#-2 (1) | Blast - Traditional (crash) | 200 | any channel |
| D-2 (2) | Blast - Traditional (hi-hat) | 200 | any channel |
| D#-2 (3) | Blast - Hammer (crash) | 200 | any channel |
| E-2 (4) | Blast - Hammer (china) | 200 | any channel |
| F-2 (5) | Blast - Bomb | 200 | any channel |
| F#-2 (6) | Skank beat | 180 | any channel |
| G-2 (7) | Double kick - 16ths | 170 | any channel |
| G#-2 (8) | Double kick - Gallop | 170 | any channel |
| A-2 (9) | Mid-tempo march | 130 | any channel |
| A#-2 (10) | Half-time atmospheric | 90 | any channel |
| B-2 (11) | 68 - Tremolo groove | 180 | any channel |
| C-1 (12) | 68 - Blast | 180 | any channel |
| C#-1 (13) | 68 - Double kick | 180 | any channel |
| D-1 (14) | 34 - Hammer blast | 180 | any channel |
| D#-1 (15) | 34 - Epic (Bathory-style) | 180 | any channel |
| E-1 (16) | Blast - Gravity (approx.) | 200 | any channel |
| F-1 (17) | Blast - Ride bell | 200 | any channel |
| F#-1 (18) | Blast - Bomb (china) | 200 | any channel |
| G-1 (19) | Half-time double kick | 180 | any channel |
| G#-1 (20) | D-beat | 180 | any channel |
| A-1 (21) | Mid-tempo - Variable kicks | 130 | any channel |
| A#-1 (22) | 128 - Atmospheric | 110 | any channel |
| B-1 (23) | Thrash beat - Slayer-style | 170 | any channel |
| C0 (24) | Gallop - Maiden-style | 170 | any channel |
| C#0 (25) | Tribal toms - Sepultura-style | 170 | any channel |
| D0 (26) | Polymeter - kicks in 3s (Meshuggah-style) | 120 | any channel |
| D#0 (27) | Polymeter - 516 over 44 | 120 | any channel |
| E0 (28) | Shuffle double bass - Hot for Teacher-style | 140 | any channel |
| F0 (29) | Doom shuffle - Sabbath-style | 70 | any channel |
| F#0 (30) | Triplet breakdown | 140 | any channel |
| G0 (31) | Sextuplet double kick - Hoglan-style | 140 | any channel |
| G#0 (32) | 128 - Tom shuffle (Running Free-style) | 110 | any channel |
| A0 (33) | 128 - Half-time groove shuffle (Pantera-style) | 110 | any channel |
| A#0 (34) | 128 - Triplet double kick (Mgla-style) | 110 | any channel |
| B0 (35) | 2nd wave - Fast polka (Darkthrone-style) | 200 | ch 16 only |
| C1 (36) | 2nd wave - Blast crash on 1 and 3 (Emperor-style) | 200 | ch 16 only |
| C#1 (37) | 2nd wave - Bomb with ride bell (Hellhammer-style) | 200 | ch 16 only |
| D1 (38) | Atmospheric - Washed blast (ride 16ths) | 180 | ch 16 only |
| D#1 (39) | Atmospheric - Tom-wash blast (blackgaze) | 180 | ch 16 only |
| E1 (40) | Atmospheric - Driving post-black 8ths | 180 | ch 16 only |
| F1 (41) | DSBM - Half-time dirge | 80 | ch 16 only |
| F#1 (42) | DSBM - 68 slow waltz (Burzum-style) | 80 | ch 16 only |
| G1 (43) | War metal - Chaotic blast (Blasphemy-style) | 200 | ch 16 only |
| G#1 (44) | War metal - Crash-wash d-beat | 200 | ch 16 only |
| A1 (45) | Death - Double-kick groove bursts | 200 | ch 16 only |
| A#1 (46) | Death - Hyperblast (32nd gravity) | 200 | ch 16 only |
| B1 (47) | Doom - Slow crush | 70 | ch 16 only |
| C2 (48) | Doom - 128 funeral march | 70 | ch 16 only |

| Key | Fill | Suggested tempo | Trigger modes |
|---|---|---|---|
| C6 (96) | Snare 16ths | 180 | any channel |
| C#6 (97) | Snare 32nds crescendo | 180 | any channel |
| D6 (98) | Snare + kick unison | 180 | any channel |
| D#6 (99) | Toms 16ths | 180 | any channel |
| E6 (100) | Toms 32nds | 180 | any channel |
| F6 (101) | Snare to floor | 180 | any channel |
| F#6 (102) | Triplet toms | 180 | any channel |
| G6 (103) | Sextuplet (hand-hand-foot) | 180 | any channel |
| G#6 (104) | Unison stabs | 180 | any channel |
| A6 (105) | Toms 38 (68 beat) | 180 | any channel |
| A#6 (106) | Snare roll half bar | 180 | any channel |
| B6 (107) | Tom cascade full bar | 180 | any channel |
| C7 (108) | Gravity roll | 180 | any channel |
| C#7 (109) | Blast burst 32nds | 180 | any channel |
| D7 (110) | Stabs half bar | 180 | any channel |
| D#7 (111) | Hand-hand-foot triplets - full bar | 180 | any channel |
| E7 (112) | Triplet tom run - half bar | 180 | any channel |
| F7 (113) | Linear fill - half bar | 180 | any channel |
| F#7 (114) | 32nd roll around the kit - full bar | 180 | any channel |
| G7 (115) | Groups of five - full bar | 180 | any channel |
| G#7 (116) | Triplet unison stabs - half bar | 180 | any channel |
| A7 (117) | Triplet toms 38 (128 beat) | 180 | any channel |
| A#7 (118) | Sextuplet burst - Hoglan-style | 180 | any channel |
| B7 (119) | Chaotic snare-kick burst (war metal) | 180 | any channel |
| C8 (120) | Floor-tom wash crescendo - half bar | 180 | any channel |
| C#8 (121) | Slow tom fall (doom) - full bar | 180 | any channel |

## 11. Troubleshooting

| Problem | Fix |
|---|---|
| No sound | Select the track (or record-enable it); check the drum plugin is in the Instrument slot and the plugin is in **MIDI FX**, not Audio FX |
| Wrong drums sound (toms instead of china, etc.) | Your kit uses other notes: click the drum's name in the grid and pick the right note, then save a kit map |
| Plugin missing in Logic | Restart Logic; check **Settings > Plug-in Manager**; reset and rescan the plugin there |
| Fills land in the wrong place with note triggers | Start playback at the section's first bar: the groove restarts at the bar where playback starts |
| Bars don't line up | The groove's meter differs from Logic's time signature (the status line says so) |
| Drag MIDI doesn't create a region | Use **Save .mid...** and import the file |
| A drawn note is swallowed | With note triggers on, notes from C-2 to A#0 and from C6 up are trigger keys. Use a "ch 16" mode to keep them apart |

To report a problem or suggest a pattern, use [GitHub issues](https://github.com/dereksamson/black-metal-drums/issues).
