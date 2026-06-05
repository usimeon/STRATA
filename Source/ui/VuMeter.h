#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>
#include "LookAndFeel.h"

/*  Classic analog-style VU meter (cream dial, ink scale, black needle, red
    zone, peak/clip LED, orange hold needle, numeric readout).

    Scale is non-linear — ANSI/IEC standard VU spacing, matching a real
    hardware meter face.  The "VU" / meter-type label sits centered on the
    dial face, not at the top, exactly as printed on a physical meter.

    Reads atomic snapshots set by the audio thread; NO audio-thread coupling. */
namespace strata::ui
{
    class VuMeter : public juce::Component
    {
    public:
        // analog face palette
        struct Face
        {
            static constexpr juce::uint32 bezel   = 0xff0d0d0f;
            static constexpr juce::uint32 creamHi = 0xfff3ecd9;
            static constexpr juce::uint32 creamLo = 0xffd8cdb0;
            static constexpr juce::uint32 ink     = 0xff1b1814;
            static constexpr juce::uint32 inkDim  = 0xff6b6354;
            static constexpr juce::uint32 red     = 0xffb23a2c;
            static constexpr juce::uint32 needle  = 0xff1b1814;
            static constexpr juce::uint32 hold    = 0xffe08a1e;
            static constexpr juce::uint32 ledOff  = 0xff3a2420;
            static constexpr juce::uint32 ledOn   = 0xffe5483d;
        };

        // Non-linear ANSI VU scale: each entry maps a VU value → normalised
        // position t ∈ [0,1] along the arc from kA0 to kA1.
        struct VuMark { float vu; float t; };
        static constexpr VuMark kScale[] = {
            { -20.0f, 0.000f }, { -10.0f, 0.218f }, {  -7.0f, 0.324f },
            {  -5.0f, 0.410f }, {  -3.0f, 0.508f }, {  -2.0f, 0.570f },
            {  -1.0f, 0.636f }, {   0.0f, 0.710f }, {  +1.0f, 0.793f },
            {  +2.0f, 0.870f }, {  +3.0f, 1.000f }
        };
        static constexpr int kScaleN = 11;

        // Map a VU value to normalised arc position using piecewise linear
        // interpolation between the ANSI scale marks.
        static float tForVu (float vu) noexcept
        {
            vu = juce::jlimit (-20.0f, 3.0f, vu);
            for (int i = 0; i < kScaleN - 1; ++i)
            {
                if (vu >= kScale[i].vu && vu <= kScale[i + 1].vu)
                {
                    const float frac = (vu - kScale[i].vu) / (kScale[i + 1].vu - kScale[i].vu);
                    return kScale[i].t + frac * (kScale[i + 1].t - kScale[i].t);
                }
            }
            return vu <= -20.0f ? 0.0f : 1.0f;
        }

        // values pushed by the editor timer
        void setValues (float rmsDb, float peakDb, float peakHoldDb, bool clip) noexcept
        {
            const float vuDb = (rmsDb + scaleOffset) - referenceDb; // 0 VU at refDb
            needle = tForVu (juce::jlimit (-20.0f, 3.0f, vuDb));
            if (needle > holdNeedle) holdNeedle = needle;            // latched VU hold
            peak = peakDb; hold = peakHoldDb; clipped = clip; rms = rmsDb;
            repaint();
        }

        void setReferenceDb (float db) noexcept { referenceDb = db; }
        float getReferenceDb() const noexcept   { return referenceDb; }
        void setCompact (bool c) noexcept       { compact = c; }

        // Meter type label + scale offset (e.g. "RMS+3" with +3 dB).
        void setScale (const juce::String& lbl, float refDb, float offsetDb) noexcept
        {
            label = lbl; referenceDb = refDb; scaleOffset = offsetDb;
        }

        // Click clears latched clip/hold; vertical drag sets the calibration.
        std::function<void()>      onClear;
        std::function<void(float)> onCalibrate; // new 0 VU reference in dBFS

        void mouseDown (const juce::MouseEvent&) override { dragStartRef = referenceDb; dragged = false; }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! onCalibrate) return;
            dragged = true;
            const float ref = juce::jlimit (-30.0f, -6.0f,
                                            dragStartRef + (float) e.getDistanceFromDragStartY() * 0.1f);
            onCalibrate (ref);
        }
        void mouseUp (const juce::MouseEvent&) override
        {
            if (! dragged) { holdNeedle = 0.0f; if (onClear) onClear(); }
        }

        void paint (juce::Graphics& g) override
        {
            // Load the VU face photo once (JUCE ImageCache keeps it alive)
            static const juce::Image faceImg = juce::ImageCache::getFromMemory (
                BinaryData::vu_face_png, BinaryData::vu_face_pngSize);

            auto full = getLocalBounds().toFloat();

            // ── Bezel ─────────────────────────────────────────────────────────
            g.setColour (juce::Colour (Face::bezel));
            g.fillRoundedRectangle (full, compact ? 4.0f : 7.0f);

            auto face = full.reduced (compact ? 2.0f : 4.0f);

            if (! compact && faceImg.isValid())
            {
                // ── IMAGE PATH — full-size meter with photo face ───────────────
                // Draw the photo face stretched to fill (aspect ratio of face
                // ~1.87 vs image 1.57 — slight horizontal stretch is acceptable).
                {
                    juce::Path clip;
                    clip.addRoundedRectangle (face, 5.0f);
                    juce::Graphics::ScopedSaveState ss (g);
                    g.reduceClipRegion (clip);
                    g.drawImage (faceImg, face, juce::RectanglePlacement::stretchToFit);
                }

                // Subtle inner shadow at bezel edge (depth)
                g.setColour (juce::Colours::black.withAlpha (0.18f));
                g.drawRoundedRectangle (face.reduced (0.6f), 5.0f, 1.5f);

                // Needle, hold-needle, LED — all calibrated to image geometry
                drawNeedleOnImage (g, face);
                drawLed (g, face);

                // Peak bar pinned to the image face bottom
                auto peakRow = face.withTrimmedTop (face.getHeight() - 7.0f);
                drawPeakBar (g, peakRow);

                // Readouts — drawn in the lower region over the image
                {
                    auto rb = face.reduced (12.0f, 6.0f)
                                  .withHeight (20.0f)
                                  .withY (face.getBottom() - 28.0f);
                    g.setColour (juce::Colour (Face::ink));
                    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
                    g.drawText (peak <= -99.0f ? "-inf" : juce::String (peak, 1) + " dB",
                                rb, juce::Justification::bottomLeft);
                    g.drawText (rms  <= -99.0f ? "-inf" : juce::String (rms,  1) + " dB",
                                rb, juce::Justification::bottomRight);
                }
            }
            else
            {
                // ── PROGRAMMATIC PATH — compact mode or image unavailable ─────
                g.setGradientFill (juce::ColourGradient (
                    juce::Colour (Face::creamHi), face.getCentreX(), face.getY(),
                    juce::Colour (Face::creamLo), face.getCentreX(), face.getBottom(), false));
                g.fillRoundedRectangle (face, compact ? 3.0f : 5.0f);
                g.setColour (juce::Colours::black.withAlpha (0.22f));
                g.drawRoundedRectangle (face.reduced (0.6f), compact ? 3.0f : 5.0f, 1.0f);

                auto area = face.reduced (compact ? 4.0f : 8.0f);

                juce::Rectangle<float> readoutRow;
                if (! compact) readoutRow = area.removeFromBottom (12.0f);
                auto peakRow = area.removeFromBottom (compact ? 5.0f : 7.0f);
                area.removeFromBottom (compact ? 1.0f : 3.0f);

                drawArcScale     (g, area);
                drawHoldNeedle   (g, area);
                drawNeedle       (g, area);
                drawPeakBar      (g, peakRow);
                drawLed          (g, face);

                if (! compact)
                {
                    g.setColour (juce::Colour (Face::ink));
                    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
                    auto rb = face.reduced (12.0f, 8.0f)
                                  .withHeight (24.0f)
                                  .withY (face.getBottom() - 32.0f);
                    g.drawText (peak <= -99.0f ? "-inf" : juce::String (peak, 1),
                                rb, juce::Justification::bottomLeft);
                    g.drawText (rms  <= -99.0f ? "-inf" : juce::String (rms,  1),
                                rb, juce::Justification::bottomRight);
                }
            }
        }

    private:
        // Arc angles, clockwise from 12 o'clock (JUCE convention).
        static constexpr float kA0 = -0.92f; // −53° : −20 VU (far left)
        static constexpr float kA1 =  0.92f; // +53° : +3 VU  (far right)
        static float angleFor (float t) noexcept { return kA0 + t * (kA1 - kA0); }

        juce::Point<float> pivot (juce::Rectangle<float> r, float& radiusOut) const noexcept
        {
            // Leave room for outer scale numbers above and sides.
            const float numMargin = compact ? 9.0f : 16.0f;
            radiusOut = juce::jmin (r.getWidth() * 0.46f, r.getHeight() * 0.92f) - numMargin;
            return { r.getCentreX(), r.getBottom() - r.getHeight() * 0.08f };
        }

        // ── Image-calibrated needle ───────────────────────────────────────────
        // Geometry derived by measuring the 606×386 vu_face.png:
        //   pivot (px):  x = 303 (image centre),  y = 401 (15 px below image bottom)
        //   arc radius:  332 px in image space
        // Scale factors are applied so the needle always tracks the printed
        // scale regardless of the rendered component size.
        void drawNeedleOnImage (juce::Graphics& g, juce::Rectangle<float> face)
        {
            static constexpr float kImgW = 606.0f, kImgH = 386.0f;
            static constexpr float kPivX = 303.0f, kPivY = 401.0f;
            static constexpr float kR    = 332.0f;

            const float sx = face.getWidth()  / kImgW;
            const float sy = face.getHeight() / kImgH;

            // Pivot and needle tip in face coordinates
            const float ang  = angleFor (needle);
            const float pivX = face.getX() + kPivX * sx;
            const float pivY = face.getY() + kPivY * sy;   // may be slightly below face
            const float tipX = face.getX() + (kPivX + kR * std::sin (ang)) * sx;
            const float tipY = face.getY() + (kPivY - kR * std::cos (ang)) * sy;

            // Tapered needle path (wide at pivot, fine at tip)
            const float dx = tipX - pivX, dy = tipY - pivY;
            const float len = std::hypot (dx, dy);
            if (len < 1.0f) return;
            const float px = -dy / len, py = dx / len;   // perpendicular unit
            const float bw = 4.0f, tw = 0.7f;

            juce::Path np;
            np.startNewSubPath (pivX - px * bw, pivY - py * bw);
            np.lineTo (tipX  - px * tw, tipY  - py * tw);
            np.lineTo (tipX  + px * tw, tipY  + py * tw);
            np.lineTo (pivX  + px * bw, pivY  + py * bw);
            np.closeSubPath();

            g.setColour (juce::Colours::black.withAlpha (0.22f));
            g.fillPath (np, juce::AffineTransform::translation (0.8f, 1.0f));
            g.setColour (juce::Colour (clipped ? Face::ledOn : Face::needle));
            g.fillPath (np);

            // Peak-hold needle (orange thin line)
            if (holdNeedle > 0.001f)
            {
                const float ha  = angleFor (holdNeedle);
                const float htX = face.getX() + (kPivX + kR * std::sin (ha)) * sx;
                const float htY = face.getY() + (kPivY - kR * std::cos (ha)) * sy;
                // draw only the outer 45 % of the needle length
                const float t = 0.55f;
                g.setColour (juce::Colour (Face::hold));
                g.drawLine (pivX + (htX - pivX) * t, pivY + (htY - pivY) * t,
                            htX, htY, 1.8f);
            }

            // Hub — clamped so it's always just inside the face bottom edge
            const float hub  = 9.0f;
            const float hubY = juce::jmin (pivY, face.getBottom() - hub * 0.55f);
            g.setColour (juce::Colour (Face::ink));
            g.fillEllipse (juce::Rectangle<float> (hub, hub).withCentre ({ pivX, hubY }));
            g.setColour (juce::Colour (0xffd4c8aa));
            g.fillEllipse (juce::Rectangle<float> (hub * 0.42f, hub * 0.42f)
                               .withCentre ({ pivX - hub * 0.12f, hubY - hub * 0.12f }));
        }

        void drawArcScale (juce::Graphics& g, juce::Rectangle<float> r)
        {
            float radius; const auto c = pivot (r, radius);

            // ── Arcs ─────────────────────────────────────────────────────────
            // Black arc: −20 VU → 0 VU
            const float t0 = kScale[7].t; // t at 0 VU
            juce::Path arcBlk;
            arcBlk.addCentredArc (c.x, c.y, radius, radius, 0.0f, kA0, angleFor (t0), true);
            g.setColour (juce::Colour (Face::ink));
            g.strokePath (arcBlk, juce::PathStrokeType (compact ? 1.5f : 2.0f));

            // Red zone arc: 0 VU → +3 VU — thicker, like the reference meter
            juce::Path arcRed;
            arcRed.addCentredArc (c.x, c.y, radius, radius, 0.0f, angleFor (t0), kA1, true);
            g.setColour (juce::Colour (Face::red));
            g.strokePath (arcRed, juce::PathStrokeType (compact ? 3.0f : 5.0f));

            // ── Ticks at every ANSI scale mark ───────────────────────────────
            // Major: −20, −10, −5, 0, +3  |  minor: everything else
            const auto isMajor = [](float vu) {
                return vu == -20.0f || vu == -10.0f || vu == -5.0f
                    || vu ==   0.0f || vu ==   3.0f;
            };
            for (int i = 0; i < kScaleN; ++i)
            {
                const float ang  = angleFor (kScale[i].t);
                const bool  isRed = kScale[i].vu >= 0.0f;
                const float len  = (isMajor (kScale[i].vu) ? 9.0f : 5.5f) * (compact ? 0.7f : 1.0f);
                g.setColour (juce::Colour (isRed ? Face::red : Face::ink));
                g.drawLine ({ c.getPointOnCircumference (radius - 0.5f, ang),
                              c.getPointOnCircumference (radius - 0.5f - len, ang) },
                            isRed ? 2.0f : 1.2f);
            }

            if (compact) return; // compact mode: no text labels on the arc

            // ── Outer scale numbers (outside the arc) ────────────────────────
            // Convention matching real VU meters: −20 shown with minus sign,
            // all others shown as absolute value; positive values use "+".
            struct OuterLbl { float vu; const char* txt; };
            static constexpr OuterLbl kOuter[] = {
                { -20.0f, "-20" }, { -10.0f, "10" }, { -7.0f, "7" },
                {  -5.0f, "5"   }, {  -3.0f, "3"  }, { 0.0f,  "0" },
                {  +3.0f, "+3"  }
            };
            const float fs = juce::jmax (8.0f, radius * 0.112f);
            g.setFont (juce::Font (juce::FontOptions (fs, juce::Font::bold)));
            for (const auto& lbl : kOuter)
            {
                const float ang = angleFor (tForVu (lbl.vu));
                const auto  p   = c.getPointOnCircumference (radius + 9.0f, ang);
                g.setColour (juce::Colour (lbl.vu >= 0.0f ? Face::red : Face::ink));
                g.drawText (lbl.txt,
                    juce::Rectangle<float> (p.x - 12.0f, p.y - 7.0f, 24.0f, 14.0f),
                    juce::Justification::centred);
            }

            // ── Inner scale numbers (inside the arc, dimmer, smaller) ────────
            // Mirrors the second row visible on the reference hardware meter.
            struct InnerLbl { float vu; const char* txt; };
            static constexpr InnerLbl kInner[] = {
                { -20.0f, "-20" }, { -10.0f, "-10" }, { -5.0f, "-5" }, { 0.0f, "0" }
            };
            const float fsI = juce::jmax (7.0f, radius * 0.085f);
            g.setFont (juce::Font (juce::FontOptions (fsI)));
            g.setColour (juce::Colour (Face::inkDim));
            for (const auto& il : kInner)
            {
                const float ang = angleFor (tForVu (il.vu));
                const auto  p   = c.getPointOnCircumference (radius - 14.0f, ang);
                g.drawText (il.txt,
                    juce::Rectangle<float> (p.x - 10.0f, p.y - 5.0f, 20.0f, 10.0f),
                    juce::Justification::centred);
            }

            // ── Meter type label centered on the face ────────────────────────
            // Sits in the open space below the arc, above the needle pivot —
            // exactly where "VU" is printed on a real hardware dial.
            const float labelY = c.y - radius * 0.38f;
            g.setColour (juce::Colour (Face::inkDim));
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText (label,
                juce::Rectangle<float> (c.x - 28.0f, labelY - 9.0f, 56.0f, 18.0f),
                juce::Justification::centred);
        }

        void drawNeedle (juce::Graphics& g, juce::Rectangle<float> r)
        {
            float radius; const auto c = pivot (r, radius);
            const auto tipC = c.getPointOnCircumference (radius - 2.0f, angleFor (needle));
            
            juce::Path nPath;
            const float baseW = compact ? 2.5f : 4.0f;
            const float tipW  = compact ? 0.8f : 1.2f;
            const float dx = tipC.x - c.x;
            const float dy = tipC.y - c.y;
            const float len = std::hypot (dx, dy);
            const float px = -dy / len;
            const float py = dx / len;
            
            nPath.startNewSubPath (c.x - px * baseW, c.y - py * baseW);
            nPath.lineTo (tipC.x - px * tipW, tipC.y - py * tipW);
            nPath.lineTo (tipC.x + px * tipW, tipC.y + py * tipW);
            nPath.lineTo (c.x + px * baseW, c.y + py * baseW);
            nPath.closeSubPath();

            // subtle drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillPath (nPath, juce::AffineTransform::translation (0.8f, 0.8f));
            
            // needle (turns red while clip is latched)
            g.setColour (juce::Colour (clipped ? Face::red : Face::needle));
            g.fillPath (nPath);
            
            // pivot hub
            const float hub = compact ? 5.0f : 8.0f;
            g.setColour (juce::Colour (Face::ink));
            g.fillEllipse (juce::Rectangle<float> (hub, hub).withCentre (c));
            g.setColour (juce::Colour (Face::inkDim));
            g.fillEllipse (juce::Rectangle<float> (hub * 0.45f, hub * 0.45f).withCentre (c));
        }

        void drawHoldNeedle (juce::Graphics& g, juce::Rectangle<float> r)
        {
            if (holdNeedle <= 0.001f) return;
            float radius; const auto c = pivot (r, radius);
            const auto a = c.getPointOnCircumference (radius * 0.62f, angleFor (holdNeedle));
            const auto b = c.getPointOnCircumference (radius - 2.0f,   angleFor (holdNeedle));
            g.setColour (juce::Colour (Face::hold));
            g.drawLine ({ a, b }, compact ? 1.2f : 1.6f);
        }

        void drawPeakBar (juce::Graphics& g, juce::Rectangle<float> r)
        {
            g.setColour (juce::Colour (Face::ink).withAlpha (0.35f));
            g.fillRoundedRectangle (r, 2.0f);
            const float norm = juce::jlimit (0.0f, 1.0f, (peak + 60.0f) / 60.0f);
            auto fill = r.withWidth (r.getWidth() * norm);
            g.setColour (juce::Colour (peak > -1.0f ? Face::red
                                       : peak > -6.0f ? Face::hold : Face::ink));
            g.fillRoundedRectangle (fill, 2.0f);
            const float hN = juce::jlimit (0.0f, 1.0f, (hold + 60.0f) / 60.0f);
            g.setColour (juce::Colour (Face::ink));
            g.fillRect (r.getX() + r.getWidth() * hN - 1.0f, r.getY(), 1.5f, r.getHeight());
        }

        void drawLed (juce::Graphics& g, juce::Rectangle<float> face)
        {
            const float d = compact ? 5.0f : 7.0f;
            auto led = juce::Rectangle<float> (d, d).withCentre ({ face.getRight() - d, face.getY() + d });
            g.setColour (juce::Colour (clipped ? Face::ledOn : Face::ledOff));
            g.fillEllipse (led);
            if (clipped)
            {
                g.setColour (juce::Colour (Face::ledOn).withAlpha (0.4f));
                g.fillEllipse (led.expanded (2.0f));
            }
        }

        juce::String label = "VU";
        float referenceDb = -24.0f, scaleOffset = 0.0f, dragStartRef = -24.0f;
        float needle = 0.0f, holdNeedle = 0.0f, peak = -100.0f, hold = -100.0f, rms = -100.0f;
        bool  clipped = false, compact = false, dragged = false;
    };
}
