# Black Metal Drums (Logic Pro MIDI FX)

An Audio Unit MIDI FX plugin. It plays black metal grooves and fills in time with Logic's
transport, and you can edit them on a step grid. Notes go out on channel 10. A kit map sets
which note each drum sends, so it works in front of EZdrummer, Superior Drummer, GGD,
Drum Kit Designer, and similar.

## Build and install

Requires: Apple Command Line Tools, `brew install cmake ninja`. JUCE 8.0.10 is in `external/JUCE`.

    ./build.sh

This builds the plugin, runs the engine and processor tests, installs the plugin to
`~/Library/Audio/Plug-Ins/Components`, ad-hoc signs it, and runs `auval`.

## Use in Logic

1. Create a Software Instrument track with your drum plugin.
2. In the track's **MIDI FX** slot, choose **Audio Units > Grosbug > Black Metal Drums**.
3. Press play.

- **Groove / Fill / Fill every / Random fill**: the fill replaces the end of every Nth bar.
- **Cymbal**: moves the groove's cymbal part to the ride, ride bell, crash, china or hi-hat.
  The downbeat crash stays a crash, and fills are left alone.
- **Crash after fill**: adds a crash on the downbeat after each fill, unless the groove
  already has one there.
- **Knobs**:
  - **Humanize**: velocity variation.
  - **Accent**: extra velocity on the beat.
  - **Velocity**: overall velocity scale.
  - **Timing**: random timing variation, +/- ms.
  - **Kick L/R**: makes every second kick in a double-kick run weaker.
  - **Chance**: how much the per-step chances apply. At 0% every hit plays.
- All the random variation is the same on every playback, so a bar sounds the same on
  every pass. Different bars vary.
- Every control can be automated.

### Kit map

**Kit map...** chooses which MIDI note each lane sends.
- Built-in maps: **General MIDI**, and **General MIDI - low toms** (toms on 48/45/43/41).
- To map your own kit, click each lane name in the grid and pick its note.
  Then use **Save current map as...** and it appears in the menu in every project.
- Saved maps are `.bmkit` files in `~/Library/Application Support/BlackMetalDrums/KitMaps`.

### Note triggers (arranging with MIDI regions)

Set **Note trigger** to **Latch** or **Hold**. Then draw notes on the track to arrange:

| Keys | Effect |
|---|---|
| C-2 (note 0) upward, one key per groove, in Groove-menu order | Selects that groove and restarts it on the bar line |
| C6 (note 96) upward, one key per fill, in Fill-menu order | Plays that fill at the end of the bar the note is in |

- **Latch**: a groove plays until the next groove key.
- **Hold**: a groove plays only while its key is held. Draw one long note per section, and
  gaps are silent.
- A groove key up to a 16th before a bar line counts for that bar.
- The keys sit below and above the General MIDI drum range (35-81). Trigger keys are not
  passed on to the drum plugin; drum notes you draw are.
- The exact key ranges are shown next to the trigger control.

### Meter, length and resolution

Above the grid:
- **Grooves**: meter (2/4, 3/4, 4/4, 5/4, 5/8, 6/8, 7/8, 9/8, 12/8), 1 to 5 bars, and
  resolution (1/8, 1/8 T, 1/16, 1/16 T, 1/32).
- **Fills**: length from 1/8 to a full 4/4 bar, plus resolution.
- Changing these keeps your hits where they fall in time. Adding bars repeats the existing ones.
- Set Logic's time signature to match the groove. The plugin warns when they differ.

### Grid

- Lanes: crash, crash 2, china, ride, ride bell, hi-hat, open hi-hat, snare, 3 toms, floor tom, kick.
- Click to add or remove a hit.
- Drag up or down to set velocity.
- Alt-click for a ghost note, Cmd-click for an accent.
- **Shift-drag** a hit to set its chance of playing. **Shift-click** switches it between 100% and 50%.
  Chance steps show a dashed outline.
- **Generate...** (on the fill tab) replaces the fill with a new snare roll, tom cascade,
  snare-into-toms or kick+snare stab fill. It keeps the fill's length and resolution, and each
  click gives a new variation.
- When note triggers are on, the groove tab follows the groove that's playing.
- Edits are saved with the Logic project. **Reset** restores the factory pattern.

### Export

Drag the **Drag MIDI into Logic** tile onto the tracks area to get a region of the selected
groove and its fills. **Save .mid...** writes the same thing to a file.

If you hear nothing, Logic may be skipping an empty track. Select the track (live mode) or
put an empty MIDI region on it.

## Factory patterns

The "-style" patterns are in the spirit of a band or drummer. They are not transcriptions.

**Grooves (35)**
- **Blast beats**:
  - traditional (ride, crash, hi-hat)
  - hammer (crash, china)
  - bomb (crash, china)
  - gravity (approximation)
  - ride bell
- **Black metal**:
  - skank beat
  - d-beat
  - mid-tempo march
  - half-time atmospheric
  - half-time double kick
  - mid-tempo with variable kicks
- **Double kick**: 16ths, gallop.
- **Thrash and heavy**:
  - thrash beat (Slayer-style)
  - gallop (Maiden-style)
  - tribal toms (Sepultura-style)
- **Polymeter**: the hands stay in 4/4 while the kicks cycle across the bar line.
  - kicks in groups of 3 (Meshuggah-style), 3-bar cycle
  - 5/16 over 4/4, 5-bar cycle
- **Ternary, 4/4 on a triplet grid**:
  - double-bass shuffle (Hot for Teacher-style)
  - doom shuffle (Sabbath-style)
  - triplet breakdown
  - sextuplet double kick (Hoglan-style)
- **Ternary, 6/8 and 12/8**:
  - 6/8 tremolo, blast and double kick
  - 12/8 atmospheric
  - 12/8 tom shuffle (Running Free-style)
  - 12/8 half-time shuffle (Pantera-style)
  - 12/8 triplet double kick (Mgla-style)
- **3/4**: hammer blast, epic (Bathory-style).

**Fills (23)**
- **One beat**:
  - snare 16ths
  - snare 32nds
  - snare + kick
  - toms 16ths and 32nds
  - snare to floor
  - triplet toms
  - sextuplets
  - unison stabs
  - gravity roll
  - 32nd blast burst
  - sextuplet burst (Hoglan-style)
- **3/8** (one 6/8 or 12/8 beat): 16th toms, triplet toms.
- **Half bar**:
  - snare roll
  - stabs
  - linear fill
  - triplet tom run
  - triplet unison stabs
- **Full bar**:
  - tom cascade
  - hand-hand-foot triplets
  - 32nd roll around the kit
  - groups of five
