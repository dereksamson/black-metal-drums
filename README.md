# Black Metal Drums

Black metal (and wider metal) drum patterns, as MIDI files and as a Logic Pro plugin.

- **[plugin/](plugin/)** — *Black Metal Drums*, an Audio Unit MIDI FX for Logic Pro. It plays
  35 grooves and 23 fills in time with Logic's transport. Patterns can be edited on a step grid,
  arranged with trigger notes, and dragged out as MIDI regions. It includes:
  - blast beats, plus thrash and polymeter grooves
  - ternary grooves: triplet 4/4, 6/8 and 12/8
  - kit maps, so it works with any drum plugin
  - a per-step chance of playing, cymbal swap, and velocity and timing humanize
  - a fill generator

  See the [plugin README](plugin/README.md).
- **[generate.py](generate.py)** — a standalone Python script (standard library only) that writes
  General MIDI drum grooves and one-beat fills to [midi/](midi/). It covers only the first, smaller
  set of black metal patterns.

## Quick start (plugin)

    git clone --recurse-submodules https://github.com/dereksamson/black-metal-drums.git
    cd black-metal-drums/plugin
    brew install cmake ninja
    ./build.sh

Requires macOS with Apple's Command Line Tools. `build.sh` builds, tests, installs, signs and
validates the plugin. Then, in Logic, choose **MIDI FX > Audio Units > Grosbug > Black Metal Drums**
on a drum track.

## License

[GNU AGPLv3](LICENSE). The plugin is built on [JUCE](https://juce.com), used under its AGPLv3 licence.
