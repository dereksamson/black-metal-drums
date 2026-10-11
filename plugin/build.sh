#!/bin/sh
# Build, install, sign and validate the Black Metal Drums AU (MIDI FX).
# Fails if any test or the auval validation fails (also used by CI).
set -e
cd "$(dirname "$0")"
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target BlackMetalDrums_AU EngineTest ProcessorTest ExportPatterns
./build/EngineTest_artefacts/Release/EngineTest
./build/ProcessorTest_artefacts/Release/ProcessorTest
COMPONENT="$HOME/Library/Audio/Plug-Ins/Components/Black Metal Drums.component"
codesign --force --deep -s - "$COMPONENT"
killall -9 AudioComponentRegistrar 2>/dev/null || true
LOG=$(mktemp)
if auval -v aumi BmDr Dsmn > "$LOG" 2>&1; then STATUS=0; else STATUS=$?; fi
tail -3 "$LOG"
rm -f "$LOG"
exit $STATUS
