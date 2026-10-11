#!/bin/sh
# Build, install, sign and validate the Black Metal Drums AU (MIDI FX).
set -e
cd "$(dirname "$0")"
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target BlackMetalDrums_AU EngineTest ProcessorTest ExportPatterns
./build/EngineTest_artefacts/Release/EngineTest
./build/ProcessorTest_artefacts/Release/ProcessorTest
COMPONENT="$HOME/Library/Audio/Plug-Ins/Components/Black Metal Drums.component"
codesign --force --deep -s - "$COMPONENT"
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aumi BmDr Dsmn | tail -3
