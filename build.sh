#!/usr/bin/env bash
# Build STRATA (VST3 + AU) and force-install to the macOS user plug-in folders
# so they can be tested immediately. Usage: ./build.sh [Debug|Release]
set -euo pipefail
cd "$(dirname "$0")"

CONFIG="${1:-Debug}"
VST3_DST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components"

echo "==> Building STRATA ($CONFIG)"
cmake --build build --config "$CONFIG" --target STRATA_VST3 STRATA_AU -j8

SRC="build/STRATA_artefacts/$CONFIG"
mkdir -p "$VST3_DST" "$AU_DST"

echo "==> Installing to user plug-in folders"
rsync -a --delete "$SRC/VST3/STRATA.vst3/"      "$VST3_DST/STRATA.vst3/"
rsync -a --delete "$SRC/AU/STRATA.component/"    "$AU_DST/STRATA.component/"

# Re-sign the AU and flush the AU cache so hosts pick up the new build.
codesign --force --sign - "$AU_DST/STRATA.component" >/dev/null 2>&1 || true
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true

echo "==> Installed:"
echo "    $VST3_DST/STRATA.vst3"
echo "    $AU_DST/STRATA.component"
