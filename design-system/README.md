# STRATA — Design System

> Dark, stage-readable interface language for **STRATA**, a live-safe gain-staging
> & utility channel-strip audio plugin (VST3 / AU / Standalone, built on JUCE 8 / C++17).

This design system is reverse-engineered directly from STRATA's UI source code —
the JUCE `LookAndFeel`, the analog VU meter, and the multi-instance control
surface. Every color, radius, and type size here is lifted from the real plugin,
not approximated.

---

## Source materials

| Source | URL / path | What it gave us |
|---|---|---|
| **GitHub — usimeon/STRATA** | https://github.com/usimeon/STRATA (branch `main`) | The entire system. Read the files below to go deeper. |
| `Source/ui/LookAndFeel.h` | the dark theme, rotary knob, brushed-metal fader | Core `Theme` palette + component drawing |
| `Source/ui/VuMeter.h` | the signature analog VU meter | Cream-dial `Face` palette, scale, needle |
| `Source/ui/ChannelView.h` | the multi-instance control surface | Bucket tabs, channel columns, **health-wash** color logic |
| `Source/link/InstanceRegistry.cpp` | inter-instance linking | The 8-hue **bucket palette** |
| `Source/PluginEditor.cpp` | the single channel strip | Layout, wordmark, button on-state colors |
| `Source/Parameters.h` | host params | Exact control labels & copy (meter types, monitor modes) |
| `CONTEXT.md`, `README.md` | repo docs | Product intent, feature state, conventions |

**Reader: you can explore https://github.com/usimeon/STRATA further** to build
higher-fidelity designs — the UI `.h` files are short and contain the exact
drawing math (arc angles, gradients, tick spacing) behind every component here.

---

## What STRATA is

STRATA is a **zero-latency utility channel strip** for live sound and streaming.
The thesis: a volunteer running front-of-house or a stream needs to set healthy
input gain *fast*, read it at a glance from across a room, and manage many
channels from one window. So the whole product is built around legibility under
pressure.

It has **one input gain knob**, **one analog VU meter** (showing the plugin's
output), and a process-wide **Channel View** — a control surface that mixes every
STRATA instance in the session from a single panel, grouped into colored
"buckets." There is no EQ, no compressor; STRATA is deliberately a clean gain &
metering utility. The output-gain knob and a former GR meter are intentional
placeholders for if/when processing is ever added.

### Two surfaces (these are the "products")
1. **Channel Strip** — the default single-instance plugin window (≈360×520): name,
   VU meter, meter-type + monitor selectors, the INPUT knob, BYPASS/MUTE/MONO/Ø
   buttons, and a group selector.
2. **Channel View** — a control-surface overview (≈760×420+): bucket tabs across
   the top, then one column per instance in the bucket, each with its own compact
   VU, a brushed-metal fader, IN/OUT, and LINK — plus a **gain-staging health
   wash** behind every fader that turns green/amber/red by level.

---

## CONTENT FUNDAMENTALS

STRATA's copy is **terse, technical, and imperative** — the voice of pro audio
hardware, not a consumer app. It assumes the user knows audio.

- **Casing:** Control captions are **ALL-CAPS, single words**: `BYPASS`, `MUTE`,
  `MONO`, `IN` / `OUT`, `LINK`, `ASSIGN`. The brand mark is all-caps: **STRATA**.
  Menu/section labels use Title Case: "Input Gain", "Meter Type", "No Group",
  "Group 1". Choice values are terse tokens: `VU`, `RMS`, `RMS+3`, `K-12`,
  `K-14`, `K-20`, `Stereo / Left / Right / Mono / Side`.
- **Units are explicit and always present:** `" dB"` suffix on gains, `dBFS` for
  calibration, `"0VU=-24"` readout, `"-inf"` for silence. Numbers carry signs
  (`+3`, `-20`). This is non-negotiable in the visual language — a value without
  a unit looks broken here.
- **Voice / person:** Effectively no prose. The one full sentence in the UI is an
  empty-state instruction, written in plain second-person imperative:
  *"No channels in this bucket yet. Use ASSIGN to add STRATA instances."* That's
  the tone for any help text — short, directive, names the exact control to press.
- **Symbols over words where pros expect them:** polarity is the **Ø** glyph, not
  "PHASE". But metering modes are spelled (`K-20`, not an icon).
- **No emoji. No marketing adjectives. No exclamation.** Nothing is "amazing." The
  product never addresses the user with personality; it reports state.
- **Naming is functional:** instances auto-name from their track; buckets are
  "Group N" until renamed; mirror sets are "LINK 1..4". Defaults are descriptive,
  never cute.

**Rule of thumb:** if you're writing STRATA copy, write what a mixing console
silkscreen would say. One or two uppercase words, a unit, a number with a sign.

---

## VISUAL FOUNDATIONS

The brief, verbatim from the source: *"Dark, stage-readable theme. High contrast,
large hit targets, no gloss."* Everything below serves reading a level from across
a dark room.

### Color & vibe
- **Near-black, cool, desaturated ground.** Background `#14171c`, panels `#1d222a`,
  hairlines `#2c333d`. The chrome recedes; only signal-bearing elements have color.
- **One brand accent: teal `#36c1a6`.** Used for the wordmark, the knob's value
  arc, and the "this is the instance you're on" highlight. Used sparingly.
- **State colors are a traffic system, not decoration:** blue `#5b8cff` = linked,
  amber `#e2b341` = bypass / hot, red `#e5484d` = mute / clip / over. These map
  1:1 to the **health wash** (red→amber→green→blue→grey by level) so color *means
  the same thing everywhere*: warmer = hotter signal.
- **The VU meter is the one warm object** in a cool UI — a cream dial
  (`#f3ecd9`→`#d8cdb0`) with ink-brown scale (`#2c2822`), a red zone, a near-black
  needle, and an orange peak-hold needle (`#e08a1e`). It reads as analog hardware
  dropped into a dark rack.
- **Imagery:** none. There are no photographs, no illustrations, no gradients-as-
  background. The only gradients are *physically motivated*: the cream dial's
  top-to-bottom light falloff, the brushed-aluminium fader cap, and the health
  wash. Decorative gradients are off-brand.

### Typography
- **One typeface: Inter**, weights 400/600/700. Sans only. (JUCE:
  `setDefaultSansSerifTypefaceName("Inter")`.)
- **Tiny by web standards, because it's a plugin:** wordmark 16px/700, labels
  ~11px/700, tab numbers 13px/700, VU scale 9–10px, readouts 9px. When you build a
  plugin-faithful mockup, *don't* inflate these — the density is the point. (For
  slides or marketing-scale work, keep the proportions and the all-caps label
  treatment but scale up.)
- **Numbers use tabular figures** and always show sign + unit.

### Spacing, borders, radii
- **Compact, console-like density.** Gaps of 4–8px between control rows; columns
  reduced by 3–4px insets.
- **Radii are small and consistent:** VU bezel 7px (4px compact), channel column
  card 5px, health wash 4px, fader cap / swatches / tab accent 3px, peak bar 2px.
  Nothing is pill-shaped; nothing is sharp-cornered.
- **Borders are hairlines** (`#2c333d`) or **black dividers at 40% alpha** between
  tabs and columns. The "self" instance gets a 1.5px teal outline.

### Elevation, light & material
- **No drop shadows on panels.** Depth comes from value steps (bg → panel →
  stroke), not blur. The *only* shadows are object-level and physical: the fader
  cap's `0 2px 3px rgba(0,0,0,.45)` and the cap's bevel (light top edge, dark
  bottom edge).
- **Recessed vs raised is rendered, not faked:** the fader groove is a dark line
  (`black @55%`) with a 6%-white highlight beside it; the metal cap sits *above*
  it with engraved grip ridges. This skeuomorphic-but-restrained material is core
  to the brand — it says "real console," without gloss or photoreal chrome.
- **The clip LED blooms** (`#e5483d` core + 40%-alpha halo) only when latched
  over. That's the single "glow" in the system; use it only for over/alarm.

### Motion
- **Functional, fast, no flourish.** Meters poll at 24–30 Hz. The health wash has
  a deliberate **peak-hold then ease-down** (~1.25s hold, ~12 dB/s release) so
  color doesn't flicker. Button state changes are instant. There are no entrance
  animations, no bounces, no parallax. If you animate, animate a *meter* or a
  *level*, never the chrome.

### Hover / press / active states
- **Toggle buttons:** off = panel fill + hairline; **on = a solid state-color fill**
  (BYPASS→amber, MUTE→red, CH VIEW/LINK→blue, generic→teal). Hover lifts the panel
  fill one step (`#232a33`).
- **Active tab:** panel background **plus** a 22%-alpha tint of the bucket color,
  with a 3px bucket-color accent strip on top.
- **Drag affordances are drawn, not hinted:** channel headers show a 2×3 grid of
  grip dots; the fader cap has engraved ridges; the VU meter responds to
  click-to-clear and vertical drag-to-calibrate.

### Layout rules
- Header bar is fixed at the top (30px) carrying the wordmark + a group-color dot.
- Single-strip layout is a **vertical stack**: name → meter → selectors → knob →
  button row → group. Channel View is **tabs on top, equal-width columns below**,
  reflowing as instances are added/removed/reordered.
- Windows are resizable within clamps (strip 320×420 → 1100×900); Channel View
  forces a wider minimum (≈760) so columns stay legible.

---

## ICONOGRAPHY

**STRATA has no icon font and ships no image/SVG/PNG icon assets.** This is a
deliberate, important fact about the brand: the UI is **text-label + code-drawn**.

- **Controls are labeled with words**, set in Inter all-caps: `BYPASS`, `MUTE`,
  `MONO`, `IN`/`OUT`, `LINK` (`LINK 1`–`LINK 4`), `ASSIGN`. No glyph stands in for
  a labeled function.
- **The one symbolic glyph is `Ø`** (U+00D8) for **polarity/phase invert** — the
  standard pro-audio convention. If you need it, it's a Unicode character, not an
  asset.
- **Everything "graphical" is procedurally drawn**, not an icon: the VU needle &
  arc, the rotary knob's arc + pointer, the brushed-metal fader cap, the clip LED,
  the bucket color swatch/dot, and the 2×3 **grip-dot** drag handle. Recreate these
  with `<canvas>` or CSS, as the UI kit does — **do not** swap in a generic icon
  set.
- **The logo is purely typographic:** the word **STRATA** in Inter Bold, teal
  `#36c1a6`, ~0.06em tracking. There is no logomark. A small filled **color dot**
  next to it indicates the active group/bucket.
- **No emoji, ever.** No decorative unicode beyond `Ø`.

**If you must add an icon** (e.g. for a marketing surface that doesn't exist in
the plugin), match the spirit: thin, monochrome, functional — closer to a console
silkscreen than a friendly app glyph. Flag any such addition as non-canonical.

> **Substitution note:** Inter is loaded from Google Fonts CDN in these files
> (it *is* the real plugin typeface, so this is faithful, not a substitution). No
> binary font files are bundled — if you want them in `fonts/`, ask and I'll add
> the Inter webfont files.

---

## Index / manifest

### Root files

| File | Purpose |
|---|---|
| `README.md` | This file — product context, content & visual foundations, iconography, manifest |
| `colors_and_type.css` | All design tokens as CSS variables: core theme, VU face, health wash, bucket palette, metal-cap gradient, type ramp, radii, button states |
| `SKILL.md` | Agent-Skill front-matter — makes this folder a downloadable Claude skill |

### Design System cards (`preview/`)

Each is a self-contained HTML specimen (~700 × 100–200 px):

| Card | Group | Content |
|---|---|---|
| `colors-core.html` | Colors | bg · panel · stroke · fg · fg-dim |
| `colors-state.html` | Colors | accent teal · link blue · warn amber · clip red |
| `colors-health-wash.html` | Colors | Level-driven fader wash: grey→blue→green→amber→red |
| `colors-buckets.html` | Colors | 9-hue stage-readable bucket palette |
| `colors-vu-face.html` | Colors | Cream dial, ink, red zone, needles, LED, bezel |
| `type-wordmark.html` | Type | STRATA wordmark specimen |
| `type-ramp.html` | Type | Full type scale (wordmark → readout) |
| `type-copy-tokens.html` | Type | Exact copy: control labels, meter types, units |
| `spacing-radii-elevation.html` | Spacing | Radius scale, hairlines, shadows, depth system |
| `comp-buttons.html` | Components | Toggle button states (off/bypass/mute/link/assign) |
| `comp-rotary-knob.html` | Components | Rotary knob: min / self-instance / max |
| `comp-fader.html` | Components | Brushed-metal fader + health-wash variants |
| `comp-bucket-tabs.html` | Components | Bucket tab: active vs inactive |
| `comp-channel-column.html` | Components | Channel column: self / peer / bypassed |

### UI Kit (`ui_kits/strata-plugin/`)

High-fidelity interactive React recreation of both plugin surfaces.
Start at **`ui_kits/strata-plugin/index.html`** — click CH VIEW to toggle surfaces.

| Component file | Exports | What it does |
|---|---|---|
| `VuMeter.jsx` | `VuMeter` | Animated analog VU meter with spring-physics needle, dual-row scale, peak hold |
| `Knob.jsx` | `Knob` | Draggable rotary knob with teal value arc |
| `ChannelStrip.jsx` | `ChannelStrip` | Full single-channel plugin window (360×520) |
| `ChannelView.jsx` | `ChannelView`, `Fader` | Multi-instance control surface with bucket tabs + channel columns |

See `ui_kits/strata-plugin/README.md` for full component API and interactive behaviour.

There are **no slides** (no deck template was provided) and **no `assets/` images**
(the product ships none — its visuals are entirely code-drawn).

---

*Built from the STRATA source at https://github.com/usimeon/STRATA. Explore the
`Source/ui/*.h` files there for the exact drawing math behind any component.*
