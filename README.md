# Black Metal Drums

[![CI](https://github.com/dereksamson/black-metal-drums/actions/workflows/ci.yml/badge.svg)](https://github.com/dereksamson/black-metal-drums/actions/workflows/ci.yml)

Black metal (and wider metal) drum patterns, as MIDI files and as a Logic Pro plugin.

- **[plugin/](plugin/)** — *Black Metal Drums*, an Audio Unit MIDI FX for Logic Pro. It plays
  49 grooves and 26 fills in time with Logic's transport. Patterns can be edited on a step grid,
  arranged with trigger notes, and dragged out as MIDI regions. It includes:
  - blast beats, second wave, atmospheric/DSBM, war metal, thrash, death, doom and polymeter grooves
  - ternary grooves: triplet 4/4, 6/8 and 12/8
  - kit maps, so it works with any drum plugin
  - a per-step chance of playing, cymbal swap, and velocity and timing humanize
  - a fill generator
  - a song mode, presets, undo/redo and copy/paste

  **[Read the user guide](docs/USER_GUIDE.md).** Developers should see [plugin/README.md](plugin/README.md).
- **[midi/](midi/)** — every factory groove (49) and fill (26) as a standalone General MIDI file,
  ready to drag into any DAW. Each file name includes a suggested tempo. The files come from the
  plugin's own pattern engine; run `plugin/build/ExportPatterns_artefacts/Release/ExportPatterns midi`
  after a build to regenerate them.

## Install

Download the `.pkg` from [Releases](https://github.com/dereksamson/black-metal-drums/releases).
It's a universal build for Apple Silicon and Intel, and it installs into
`/Library/Audio/Plug-Ins/Components`. The installer isn't notarised: right-click it and choose
**Open** the first time.

## Build from source (plugin)

    git clone --recurse-submodules https://github.com/dereksamson/black-metal-drums.git
    cd black-metal-drums/plugin
    brew install cmake ninja
    ./build.sh

Requires macOS with Apple's Command Line Tools. `build.sh` builds, tests, installs, signs and
validates the plugin. Then, in Logic, choose **MIDI FX > Audio Units > Grosbug > Black Metal Drums**
on a drum track, and follow the [quick start](docs/USER_GUIDE.md#2-quick-start).

## License

[GNU AGPLv3](LICENSE). The plugin is built on [JUCE](https://juce.com), used under its AGPLv3 licence.
