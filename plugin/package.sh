#!/bin/sh
# Universal (Apple Silicon + Intel) build of the AU, packaged as a .pkg installer and a .zip
# in ../dist. Installs into /Library/Audio/Plug-Ins/Components (all users).
set -e
cd "$(dirname "$0")"
VERSION=$(sed -n 's/^project(BlackMetalDrums VERSION \([0-9.]*\))/\1/p' CMakeLists.txt)

cmake -B build-universal -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DBMD_COPY_PLUGIN=OFF
cmake --build build-universal --target BlackMetalDrums_AU

AU_DIR="build-universal/BlackMetalDrums_artefacts/Release/AU"
COMPONENT="$AU_DIR/Black Metal Drums.component"
lipo -info "$COMPONENT/Contents/MacOS/Black Metal Drums"
codesign --force --deep -s - "$COMPONENT"

DIST="$(cd .. && pwd)/dist"
mkdir -p "$DIST"
STAGE=$(mktemp -d)
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components"
ditto "$COMPONENT" "$STAGE/Library/Audio/Plug-Ins/Components/Black Metal Drums.component"

# Always install into /Library, even if a copy of the plugin exists elsewhere (pkgbuild would
# otherwise "relocate" the update to that copy).
PLIST="$STAGE.plist"
pkgbuild --analyze --root "$STAGE" "$PLIST"
plutil -replace 0.BundleIsRelocatable -bool NO "$PLIST"
pkgbuild --root "$STAGE" --component-plist "$PLIST" --identifier com.grosbug.blackmetaldrums.pkg \
         --version "$VERSION" --install-location / "$DIST/BlackMetalDrums-$VERSION.pkg"
rm -f "$PLIST"
(cd "$AU_DIR" && ditto -c -k --keepParent "Black Metal Drums.component" "$DIST/BlackMetalDrums-$VERSION-component.zip")
rm -rf "$STAGE"
ls -lh "$DIST"
