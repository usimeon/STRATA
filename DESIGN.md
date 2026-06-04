---
name: STRATA
theme:
  colorMode: DARK
  customColor: "#36c1a6"
  headlineFont: INTER
  bodyFont: INTER
  roundness: ROUND_FOUR
tokens:
  colors:
    bg: "#14171c"
    panel: "#1d222a"
    stroke: "#2c333d"
    text: "#e7ecf2"
    textDim: "#8a94a3"
    accent: "#36c1a6"
    link: "#5b8cff"
    warn: "#e2b341"
    clip: "#e5484d"
    vuNeedle: "#f2e9d8"
    vu:
      bezel: "#0d0d0f"
      creamHi: "#f3ecd9"
      creamLo: "#d8cdb0"
      ink: "#2c2822"
      inkDim: "#6b6354"
      red: "#b23a2c"
      needle: "#1b1814"
      hold: "#e08a1e"
      ledOff: "#3a2420"
      ledOn: "#e5483d"
    health:
      red: "#e5484d"
      amber: "#e2b341"
      green: "#3fb86b"
      blue: "#4178d6"
      grey: "#5b5f6b"
  roundness:
    accentStrip: 1.5px
    panelColumn: 5.0px
    headerSwatch: 3.0px
    vuBezel: 7.0px
    vuBezelCompact: 4.0px
    vuFace: 5.0px
    vuFaceCompact: 3.0px
    faderWash: 4.0px
    faderCap: 3.0px
  spacing:
    xs: 4px
    sm: 8px
    md: 10px
    lg: 12px
    xl: 16px
    xxl: 24px
    xxxl: 30px
---

# STRATA Design System & Specification

This design document serves as the single source of truth for the visual theme, component layout, and user experience of **STRATA**, a live-safe, zero-latency gain-staging channel strip. It is intended to guide development, ensure consistency across components, and assist AI-assisted design and generation tools (such as Stitch).

---

## 1. Design Philosophy & Overview

STRATA is engineered for high-pressure live environments (streaming, broadcast, live sound). 
The core design principles are:
- **High Readability:** Flat, dark interfaces with bold, easily readable markings. Essential information (clipping, linking, active channels) should stand out from across the room.
- **Visual Feedback over Numerical Precise-Input:** Colors tell the story. The gain-staging "health wash" behind channel faders gives volunteers immediate visual warning if signal chains are running too hot.
- **Physical Heritage:** Critical elements like the VU meter and fader caps draw inspiration from premium physical consoles (brushed aluminium, analog dials, engraved index lines) while maintaining a flat, high-contrast digital layer.

---

## 2. Color Palette & Semantic Tokens

### Main Application Colors
The core frame utilizes a dark, low-fatigue slate grey palette to reduce screen glare in dim control rooms:
- **Background (`bg` / `#14171c`):** Main workspace background.
- **Panel (`panel` / `#1d222a`):** Used to isolate individual channel strips, buckets, or navigation frames.
- **Stroke (`stroke` / `#2c333d`):** Fine outline color for widgets, boxes, and inactive rotary tracks.
- **Text Primary (`text` / `#e7ecf2`):** Primary text and numeric markings.
- **Text Dim (`textDim` / `#8a94a3`):** Muted metadata, inactive markers, and sub-labels.
- **Accent Teal (`accent` / `#36c1a6`):** Indicates active states, primary selections, and the active path of the rotary slider.
- **Link Blue (`link` / `#5b8cff`):** Highlight color used for mirror-linked groups.
- **Warning Amber (`warn` / `#e2b341`):** Applied to caution states, such as active bypass ("OUT").
- **Clip Red (`clip` / `#e5484d`):** Warns of digital overload, clipping, or active mute.

### Analog VU Meter Color Palette
The VU meter replicates physical meter aesthetics:
- **Bezel (`#0d0d0f`):** Hard charcoal outer frame.
- **Cream Dial Gradient (`#f3ecd9` to `#d8cdb0`):** Soft, warm-white to shadowed cream face to resemble aged parchment/plastic dials.
- **Ink (`#2c2822`):** Low-contrast, high-readability dark ink for numbers, arc lines, and ticks.
- **Ink Dim (`#6b6354`):** Subdued ink for secondary markings and type labels.
- **Red Zone (`#b23a2c`):** Rust red arc representing the unsafe zone (0 VU to +3 VU).
- **Needle (`#1b1814`):** Slate black pointer needle (turns solid bright red during output clipping).
- **Hold Needle (`#e08a1e`):** Muted orange peak-hold needle indicator.
- **LED Indicators:** Reddish-black (`#3a2420`) for inactive clip LED; bright neon red (`#e5483d`) with a matching translucent glow (`rgba(229, 72, 61, 0.4)`) for active clipping.

### Gain-Staging Health Wash
Fader backgrounds are dynamically tinted based on the peak-held output level. The levels are:
- **Clip / Too Hot (`#e5484d`):** Signal levels $\ge$ 0 dBFS (or latched clipping).
- **Hot Zone (`#e2b341`):** Signal levels between -3 dBFS and 0 dBFS.
- **Healthy Zone (`#3fb86b`):** Signal levels between -9 dBFS and -3 dBFS.
- **Low Level (`#4178d6`):** Signal levels between -24 dBFS and -9 dBFS.
- **Silence / Inactive (`#5b5f6b`):** Signal levels below -24 dBFS.

---

## 3. Typography

All typography relies on the modern, highly legible sans-serif typeface **Inter**.

### Sizing and Hierarchy
- **Header / Logo Title:** `16.0px` (Bold)
- **Bucket/Group Tab Numbers:** `13.0px` (Bold)
- **Status / Empty State Alerts:** `13.0px` (Medium)
- **Meter Type Selector / Main Labels:** `11.0px` (Bold)
- **Bucket/Group Tab Names:** `10.5px` (Regular)
- **Meter Scale Labels:** `10.0px` (Compact: `7.5px`, Regular)
- **Numeric Readouts / dB Scale:** `9.0px` (Regular)

---

## 4. Layout & Grid System

### Single Channel Strip (Compact View)
- **Window Size:** Fixed bounds of `360 x 520 px`.
- **Vertical Stack Order:**
  - **Header Row (30px):** Includes "STRATA" title (left-aligned) and the "CHANNEL VIEW" toggle button (130px wide, right-aligned).
  - **Channel Label (24px):** Displays the editable channel name, sitting on a `panel` background.
  - **VU Meter Arc:** Occupies `42%` of the remaining vertical space.
  - **Calibration & Routing selectors (24px):** Two side-by-side dropdowns for meter calibration presets and monitor routing.
  - **Gain Knob (110px):** Large centered rotary controller.
  - **Utility Buttons (28px):** Row of four toggle buttons (BYPASS, MUTE, MONO, Ø Polarity) divided equally.
  - **Group Selector (26px):** Lowers the channel strip into one of the 8 available linkage buckets.

### Channel View (Mixer Console Grid)
- **Minimum Size:** `760 x 420 px`.
- **Navigation Row (42px):** Row of 8 tabs spanning the width, representing buckets. Active tab is highlighted with a 3px top border colored to represent the bucket.
- **Bucket Label Row (22px):** Shows a small color swatch (14px wide) and the editable bucket name.
- **Channel Columns (Dynamic Width):** Displays individual columns representing other STRATA instances. Columns have a minimum width of `60px` and display:
  - **Drag Handle Name Header (20px):** Displays instance name and 6 drag-grip dots.
  - **Compact VU Meter (60 to 96px depending on window scaling).**
  - **Console Fader (Vertical):** Linear vertical fader superimposed on the dynamic **Health Wash** background.
  - **Link Button (20px):** Cycles mirror set from 0 (off) to 1–4.
  - **In/Out Toggle Button (22px):** Standard bypass selector.

---

## 5. Shape, Borders & Shadows

- **Border Radius:**
  - Accent strip: `1.5px`
  - Fader background wash & metal cap corner: `3.0px`
  - Window panels and channel column wraps: `5.0px`
  - VU Meter bezel: `7.0px` (Compact: `4.0px`)
  - VU Meter cream face: `5.0px` (Compact: `3.0px`)
- **Fader Groove:** Thin vertical line (`2.0px`) recessed with a dark edge (`rgba(0,0,0,0.55)`) and a light highlight (`rgba(255,255,255,0.06)`).
- **Metal Fader Cap (3D Detail):**
  - Drop shadow: `rgba(0,0,0,0.45)` offset by 2px down.
  - Vertical brushed-metal gradient: `#e7e7ea` (top) to `#7e7e85` (bottom) with horizontal highlights at 10%, 48%, 52%, and 90%.
  - Top bevel line: `rgba(255,255,255,0.5)` (1px).
  - Bottom bevel line: `rgba(0,0,0,0.4)` (1px).
  - Central index groove: Dark grey (`#3c3c40`) slot (6px high) with black top border and a bright, solid-white line at the exact center.

---

## 6. Do's and Don'ts

### Do's:
- **Do** keep spacing clean and aligned to the grid. Always use the defined `Theme` color codes.
- **Do** ensure that the gain-staging health wash behaves predictably: it should snap up instantly to signal peaks but decay slowly (~1.25s hold, ~12dB/s decay) to prevent flickering.
- **Do** ensure any new processing parameters mapped to the UI (e.g., output faders) fit the same dark, stage-ready theme.

### Don'ts:
- **Don't** use standard saturated HTML colors. Avoid bright pure reds or greens. Use the specific, curated HSL-tailored hex values.
- **Don't** add shiny gradients or glossy highlights to buttons or panel backdrops; keep control widgets flat and text highly legible.
- **Don't** allow parameters to fight the user's manual mouse drag: disable live metering pushes to the GUI controls while the user is actively dragging.
