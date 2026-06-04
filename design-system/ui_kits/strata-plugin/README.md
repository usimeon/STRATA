# STRATA Plugin UI Kit

High-fidelity, interactive React recreation of both STRATA plugin surfaces.

## Surfaces covered

| Surface | Description | Size |
|---|---|---|
| **Channel Strip** (default) | Single-instance plugin window | 360×520 |
| **Channel View** | Multi-instance control surface | fluid (tabs + columns) |

Click **CH VIEW** in the plugin header to toggle between surfaces.

## Components

| File | Exports | Description |
|---|---|---|
| `VuMeter.jsx` | `VuMeter` | Animated analog VU meter with spring-physics needle, non-linear VU scale, dual-row labels (VU + %), peak-hold orange needle. Props: `value`, `peakHold`, `width`, `height`, `compact`, `simulate`. |
| `Knob.jsx` | `Knob` | Rotary INPUT GAIN knob with teal value arc. Drag vertically to adjust; double-click to reset to 0. Props: `value`, `min`, `max`, `onChange`, `label`, `size`, `self`. |
| `ChannelStrip.jsx` | `ChannelStrip`, `BUCKET_COLORS` | Full single-channel plugin strip. BYPASS/MUTE/MONO/Ø toggles with state colors; meter-type selector (VU/RMS/K-scale); monitor selector; group dot row. Props: `onOpenChannelView`. |
| `ChannelView.jsx` | `ChannelView`, `Fader` | Multi-instance control surface. Bucket tabs (with tint + 3px accent), draggable brushed-metal faders, health-wash color, mini VU meters, IN/LINK buttons. Props: `onClose`, `selfId`. |

## Interactive behaviour

- **BYPASS** → amber button + strip dims to 82% opacity + VU needle drops to –∞
- **MUTE** → red button + VU drops to –∞
- **MONO / Ø** → teal on state
- **INPUT GAIN knob** → drag up/down; double-click resets to 0 dB; VU target adjusts in real time
- **Meter type / Monitor selectors** → pill radio buttons, active = teal
- **Group dots** → click to assign; active dot pulses with outline ring
- **CH VIEW button** → switches to Channel View surface
- **Bucket tabs** → filter visible channels; active tab gets bucket-color tint
- **Channel faders** → draggable; health-wash color updates based on gain (simulated)
- **Channel names** → editable inline

## Usage

```html
<!-- Minimal: just the channel strip -->
<script src="VuMeter.jsx" type="text/babel"></script>
<script src="Knob.jsx" type="text/babel"></script>
<script src="ChannelStrip.jsx" type="text/babel"></script>

<!-- Or render the VU meter standalone -->
<VuMeter value={-9} simulate={true} width={300} height={180} />
```

## Design references

All values match `Source/ui/LookAndFeel.h` (theme, knob, fader) and `Source/ui/ChannelView.h`
(health wash, bucket tabs, column layout) in the STRATA source repo at
https://github.com/usimeon/STRATA.
