# STRATA — Live-Safe Gain-Staging Channel Strip

Zero-latency VST3/AU/Standalone utility strip with calibrated VU + peak metering
and process-wide instance linking. Built on JUCE 8.

## Prerequisites
- CMake ≥ 3.22
- A C++17 compiler (Xcode 15+ on macOS, MSVC 2022 on Windows)
- JUCE 8 as a submodule at `./JUCE` (or enable the FetchContent block in `CMakeLists.txt`)

```bash
git clone https://github.com/juce-framework/JUCE.git --branch 8.0.4 JUCE
```

## Build — macOS (VST3 + AU + Standalone)
```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
```
Artifacts are copied to:
- AU:  `~/Library/Audio/Plug-Ins/Components/STRATA.component`
- VST3:`~/Library/Audio/Plug-Ins/VST3/STRATA.vst3`

Validate AU: `auval -v aufx Strt Mygn`

### Signing / notarization (distribution)
```bash
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: <YOU>" build/.../STRATA.component
xcrun notarytool submit STRATA.zip --keychain-profile <profile> --wait
```
AU/VST3 plug-ins do **not** need entitlements themselves; the host process owns
the sandbox. Only the Standalone build needs hardened-runtime + mic entitlement
if you enable audio input.

## Build — Windows (VST3 + Standalone; AU is macOS-only)
```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
VST3 lands in `build\STRATA_artefacts\Release\VST3\STRATA.vst3`. Copy to
`C:\Program Files\Common Files\VST3\`.

## Layout
See the architecture/spec doc. Core: `Source/PluginProcessor.*`,
`Source/link/InstanceRegistry.*`, `Source/dsp/*`, `Source/ui/*`.
