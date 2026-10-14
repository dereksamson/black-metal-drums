# Black Metal Drums: plugin source

An Audio Unit MIDI FX for Logic Pro, built with JUCE 8. **How to use it is covered in the
[user guide](../docs/USER_GUIDE.md).** This page covers building and the code.

## Build

Requires macOS with Apple's Command Line Tools, plus `brew install cmake ninja`.
JUCE 8.0.10 is a git submodule in `external/JUCE`; clone with `--recurse-submodules`.

| Command | What it does |
|---|---|
| `./build.sh` | Builds the plugin, runs the engine and processor tests, installs to `~/Library/Audio/Plug-Ins/Components`, ad-hoc signs, runs `auval` |
| `./package.sh` | Universal build (arm64 + x86_64): a non-relocatable `.pkg` installer and a `.zip` of the component in `../dist`. The plugin installed by `build.sh` is left alone |
| `./build/ExportPatterns_artefacts/Release/ExportPatterns ../midi` | Regenerates the repository's `midi/` folder from the factory patterns |
| `./build/ExportPatterns_artefacts/Release/ExportPatterns --key-table` | Prints the user guide's trigger key tables. Paste them between the `key-table` markers |

The version is set in `CMakeLists.txt` (`project(... VERSION ...)`) and shown in the plugin's
**? > About** menu.

## CI and releases

- **CI** ([.github/workflows/ci.yml](../.github/workflows/ci.yml)) runs on every push to `main`
  and on every pull request, on a macOS runner:
  - `build.sh`: build, all tests, and `auval`
  - checks that `midi/` and the user guide's key tables match the factory patterns. After
    changing a pattern, run `ExportPatterns ../midi` and `ExportPatterns --key-table`, and commit
    the results.
- **Releasing** ([.github/workflows/release.yml](../.github/workflows/release.yml)):
  1. Bump `project(BlackMetalDrums VERSION x.y.z)` in `CMakeLists.txt`.
  2. Merge to `main`.
  3. Push a tag: `git tag vx.y.z && git push origin vx.y.z`.

  The workflow checks that the tag matches the version, builds and tests, runs `package.sh`, and
  publishes a GitHub release with the `.pkg`, the `.zip`, checksums and generated notes.

## Code map

| File | Role |
|---|---|
| `Source/Patterns.*` | Pattern model: lanes, `Layout` (meter, bars, resolution), atomic cells and chances, factory grooves and fills, menu categories |
| `Source/Engine.h` | Rendering: which pattern and step plays at a PPQ position, fills, accents, chance, cymbal swap, kick balance, humanize |
| `Source/Song.h` | Song sections (one atomic word each) and song rendering |
| `Source/PluginProcessor.*` | Host sync, note triggers, preview, note-offs, MIDI export, state |
| `Source/PluginEditor.*` | Main window, step grid, menus, help |
| `Source/SongEditor.*` | Song tab |
| `Source/EditTools.*` | Undo/redo history, bar and lane clipboards |
| `Source/PatternFiles.*` | Per-pattern MIDI export, export all, presets |
| `Source/KitMaps.*` | Built-in and saved kit maps |
| `Source/FillGenerator.*` | Generated fills |
| `Tests/EngineTest.cpp` | Engine, layout, clipboard, song and generator tests |
| `Tests/ProcessorTest.cpp` | The real processor against a simulated host transport |
| `Tools/ExportPatterns.cpp` | Command-line exporter |

### Grid notation for factory patterns

In `Patterns.cpp`, each lane is a string with one character per step:

| Character | Meaning |
|---|---|
| `.` | rest |
| `x` | normal hit |
| `X` | accent |
| `g` | ghost note |
| `1`–`9` | velocity level |
| `?` | normal hit with a 50 % chance |

Spaces and `|` are ignored. New patterns go at the end of their list, so saved projects keep
their selections. Groove keys follow list order:
- In the default trigger modes, only the first 35 grooves have keys, so the keys stay below the
  General MIDI drum range.
- The "ch 16" modes give every groove a key, up to 96 grooves. `factoryPatternErrors()`, which the
tests check, catches grids of the wrong length.

### Threading

Patterns, the kit map and the song are edited on the message thread and read on the audio
thread through atomics. The audio thread doesn't lock or allocate. Undo history and clipboards
are used only on the message thread.

## License

GNU AGPLv3. See [LICENSE](../LICENSE).
