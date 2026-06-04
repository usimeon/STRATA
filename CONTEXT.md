# STRATA — Project Context / Handoff

A live-safe, **zero-latency** VST3/AU gain-staging & utility channel strip built
with **JUCE 8 / C++17**. One input gain knob, one analog VU meter (shows the
plugin's output), and a process-wide **Channel View** control surface for mixing
many instances from a single window. Designed for live sound / streaming where a
volunteer needs to set healthy input gain fast.

- **Repo:** https://github.com/usimeon/STRATA  (branch `main`)
- **Plugin IDs:** AU type `aufx`, code `Strt`, manufacturer `Mygn`, name `STRATA`

---

## Build & run on another machine

```bash
# 1. Clone
git clone https://github.com/usimeon/STRATA.git
cd STRATA

# 2. Get the JUCE dependency (NOT committed — gitignored)
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git JUCE

# 3a. macOS — build + auto-install VST3/AU to user plug-in folders
./build.sh                 # Debug build; rsyncs to ~/Library/Audio/Plug-Ins/{VST3,Components}
auval -v aufx Strt Mygn    # validate the AU

# 3b. Windows
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# copy build\STRATA_artefacts\Release\VST3\STRATA.vst3  ->  C:\Program Files\Common Files\VST3\
```

**Prerequisites:** CMake ≥ 3.22, a C++17 compiler (Xcode 15+ / MSVC 2022).

**No-manual-JUCE option:** `CMakeLists.txt` has a commented `FetchContent` block
that downloads JUCE automatically — uncomment it and remove `add_subdirectory(JUCE)`.

**macOS host caching:** DAWs cache a loaded plug-in binary. After rebuilding,
remove & re-insert the plug-in (or reopen the session) to pick up changes.

---

## Layout

| Path | Role |
|---|---|
| `CMakeLists.txt` | JUCE plug-in target (VST3 / AU / Standalone), `COPY_PLUGIN_AFTER_BUILD`. |
| `build.sh` | Build both formats + force-install to user folders + re-sign AU. |
| `Source/Parameters.h` | All APVTS params + meter-type / monitor enums + helpers. |
| `Source/PluginProcessor.*` | DSP, params, state, registry client. |
| `Source/dsp/GainTrim.h` | Smoothed, click-free gain + polarity. |
| `Source/dsp/Meters.h` | Peak/VU detectors + lock-free `MeterSnapshot` (clip auto-release). |
| `Source/link/InstanceRegistry.*` | Process-wide singleton: discovery, buckets, mirror-linking, remote set. Message-thread only. |
| `Source/PluginEditor.*` | Strip UI + Channel View toggle. |
| `Source/ui/ChannelView.h` | Control surface: bucket tabs, ASSIGN, drag-reorder, per-channel VU + fader + LINK, health wash. |
| `Source/ui/VuMeter.h` | Analog VU dial (scale, red zone, clip LED, hold needle, drag-to-calibrate). |
| `Source/ui/LookAndFeel.h` | Dark theme + brushed-metal fader. |

---

## Architecture notes

**Audio path (zero latency, realtime-safe — no alloc/locks/IPC on audio thread):**
```
input → [INPUT gain] → mono sum → output gain(unity) → monitor → mute → (OUTPUT meter)
```
- `setLatencySamples` is never called; tail length 0.
- Metering: audio thread writes `std::atomic` snapshots; UI polls at 24–30 Hz.

**Inter-instance communication:** a static in-process `InstanceRegistry`
singleton (all instances of a plug-in load into one host process). **No sockets,
no shared memory.** If a host sandboxes instances, each sees only itself and
STRATA degrades to a standalone strip. All registry access is message-thread only.

**Identity / state:** each instance persists a `juce::Uuid` (re-minted on track
duplication so clones stay unique), group/bucket id, mirror link id, order index,
and the bucket's shared name/colour.

**Gain model (current):** the **working gain is INPUT gain** — both the strip
knob and the Channel View fader drive it. `outputGain` exists but is reserved for
later (output-compensation once EQ/comp are added). Default meter calibration is
**0 VU = −24 dBFS**.

**Mirror linking:** opt-in (LINK sets 1–4 per bucket). Same link id + same bucket
→ gains/bypass/mute mirror. Re-entrancy guarded; safe from automation; no
feedback loops. NOT default ganging.

**Channel health wash (volunteer aid):** each Channel View fader's background is
tinted by the channel's output level with peak-hold so it doesn't flicker:
red ≥ 0 dBFS, amber −3…0, green −9…−3, blue −24…−9, grey below. A latched clip
forces red until the ~2.5 s clip-hold auto-releases (or you click the meter).
Thresholds live in `ChannelView::Column::healthColour()`.

---

## Feature state

**Done:** input gain trim · analog VU / RMS / RMS+3 / K-12/14/20 meter ·
drag-to-calibrate · clip auto-release · buckets (named, coloured, tabs) · assign
from Channel View · drag-reorder columns · mirror linking · auto track names ·
gain-staging health-colour faders · monitor (Stereo/L/R/Mono/Side) · mute ·
polarity · session recall.

**Intentionally NOT built:** EQ, compressor, dynamics. The output-gain knob and
the (removed) GR meter are placeholders for when processing is added.

---

## Conventions
- After any change: run `./build.sh` so both formats install to the user folders.
- Commit messages end with a `Co-Authored-By: Claude ...` trailer.
