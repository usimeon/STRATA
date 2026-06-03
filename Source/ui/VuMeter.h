#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"

/*  Classic analog-style VU meter (cream dial, ink scale, black needle, red
    zone, peak/clip LED, orange hold needle, numeric readout) — inspired by the
    look of hardware VU meters. Reads atomic snapshots set by the audio thread;
    NO audio-thread coupling. */
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
            static constexpr juce::uint32 ink     = 0xff2c2822;
            static constexpr juce::uint32 inkDim  = 0xff6b6354;
            static constexpr juce::uint32 red     = 0xffb23a2c;
            static constexpr juce::uint32 needle  = 0xff1b1814;
            static constexpr juce::uint32 hold    = 0xffe08a1e;
            static constexpr juce::uint32 ledOff  = 0xff3a2420;
            static constexpr juce::uint32 ledOn   = 0xffe5483d;
        };

        // values pushed by the editor timer
        void setValues (float rmsDb, float peakDb, float peakHoldDb, bool clip) noexcept
        {
            const float vuDb = (rmsDb + scaleOffset) - referenceDb; // 0 VU at refDb
            needle = juce::jlimit (0.0f, 1.0f, (vuDb + 20.0f) / 23.0f);
            if (needle > holdNeedle) holdNeedle = needle;           // latched VU hold
            peak = peakDb; hold = peakHoldDb; clipped = clip;
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
            if (! onCalibrate) return;             // disabled for fixed (K) scales
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
            auto full = getLocalBounds().toFloat();

            // bezel + cream dial face
            g.setColour (juce::Colour (Face::bezel));
            g.fillRoundedRectangle (full, compact ? 4.0f : 7.0f);
            auto face = full.reduced (compact ? 2.0f : 4.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (Face::creamHi), face.getCentreX(), face.getY(),
                                                     juce::Colour (Face::creamLo), face.getCentreX(), face.getBottom(), false));
            g.fillRoundedRectangle (face, compact ? 3.0f : 5.0f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawRoundedRectangle (face.reduced (0.6f), compact ? 3.0f : 5.0f, 1.0f);

            auto area = face.reduced (compact ? 4.0f : 8.0f);

            // Carve distinct rows so nothing overlaps:
            //   [ type label ][ ......... arc ......... ][ peak bar ][ readouts ]
            auto typeRow = area.removeFromTop (compact ? 10.0f : 13.0f);
            juce::Rectangle<float> readoutRow;
            if (! compact) readoutRow = area.removeFromBottom (12.0f);
            auto peakRow = area.removeFromBottom (compact ? 5.0f : 7.0f);
            area.removeFromBottom (compact ? 1.0f : 3.0f);

            drawArcScale (g, area);
            drawHoldNeedle (g, area);
            drawNeedle (g, area);
            drawPeakBar (g, peakRow);
            drawLed (g, face);

            // type label (VU / RMS / K-20 …)
            g.setColour (juce::Colour (Face::inkDim));
            g.setFont (juce::Font (juce::FontOptions (compact ? 8.0f : 11.0f, juce::Font::bold)));
            g.drawText (label, typeRow, juce::Justification::centred);

            // numeric readouts (full size only)
            if (! compact)
            {
                g.setColour (juce::Colour (Face::ink));
                g.setFont (juce::Font (juce::FontOptions (9.0f)));
                g.drawText (peak <= -99.0f ? "-inf" : juce::String (peak, 1) + " dB",
                            readoutRow.removeFromLeft (readoutRow.getWidth() * 0.5f),
                            juce::Justification::centredLeft);
                g.drawText ("0VU=" + juce::String (referenceDb, 0),
                            readoutRow, juce::Justification::centredRight);
            }
        }

    private:
        // Clockwise from 12 o'clock. Rest = far left, full deflection = right.
        static constexpr float kA0 = -0.92f; // ~ -53 deg : -20 VU
        static constexpr float kA1 =  0.92f; // ~ +53 deg : +3 VU
        static float angleFor (float t) noexcept { return kA0 + t * (kA1 - kA0); }
        static float tForVu (float vu) noexcept  { return (vu + 20.0f) / 23.0f; }

        juce::Point<float> pivot (juce::Rectangle<float> r, float& radiusOut) const noexcept
        {
            const float numMargin = compact ? 9.0f : 13.0f;
            radiusOut = juce::jmin (r.getWidth() * 0.46f, r.getHeight() * 0.92f) - numMargin;
            return { r.getCentreX(), r.getBottom() - r.getHeight() * 0.08f };
        }

        void drawArcScale (juce::Graphics& g, juce::Rectangle<float> r)
        {
            float radius; const auto c = pivot (r, radius);

            // main ink arc
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, radius, radius, 0.0f, kA0, kA1, true);
            g.setColour (juce::Colour (Face::ink));
            g.strokePath (arc, juce::PathStrokeType (compact ? 1.5f : 2.0f));

            // red zone arc (0 VU .. +3)
            juce::Path red;
            red.addCentredArc (c.x, c.y, radius, radius, 0.0f, angleFor (tForVu (0.0f)), kA1, true);
            g.setColour (juce::Colour (Face::red));
            g.strokePath (red, juce::PathStrokeType (compact ? 2.5f : 3.5f));

            // ticks
            for (int i = 0; i <= 10; ++i)
            {
                const float t    = (float) i / 10.0f;
                const float vuDb = t * 23.0f - 20.0f;
                const float ang  = angleFor (t);
                const float len  = (i % 5 == 0 ? 9.0f : 5.0f) * (compact ? 0.7f : 1.0f);
                g.setColour (juce::Colour (vuDb >= 0.0f ? Face::red : Face::ink));
                g.drawLine ({ c.getPointOnCircumference (radius - 1.0f, ang),
                              c.getPointOnCircumference (radius - 1.0f - len, ang) },
                            vuDb >= 0.0f ? 1.8f : 1.0f);
            }

            // scale numbers
            g.setFont (juce::Font (juce::FontOptions (compact ? 7.5f : 10.0f)));
            for (int vu : { -20, -10, -7, -5, -3, 0, 3 })
            {
                const float ang = angleFor (tForVu ((float) vu));
                const auto  p   = c.getPointOnCircumference (radius + (compact ? 6.0f : 8.0f), ang);
                g.setColour (juce::Colour (vu >= 0 ? Face::red : Face::ink));
                g.drawText (vu > 0 ? "+" + juce::String (vu) : juce::String (vu),
                            juce::Rectangle<float> (p.x - 10.0f, p.y - 6.0f, 20.0f, 12.0f),
                            juce::Justification::centred);
            }
        }

        void drawNeedle (juce::Graphics& g, juce::Rectangle<float> r)
        {
            float radius; const auto c = pivot (r, radius);
            const auto tip = c.getPointOnCircumference (radius - 2.0f, angleFor (needle));
            // needle shadow then needle
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.drawLine ({ c.translated (0.8f, 0.8f), tip.translated (0.8f, 0.8f) }, compact ? 1.6f : 2.2f);
            g.setColour (juce::Colour (clipped ? Face::red : Face::needle));
            g.drawLine ({ c, tip }, compact ? 1.6f : 2.2f);
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
        float referenceDb = -18.0f, scaleOffset = 0.0f, dragStartRef = -18.0f;
        float needle = 0.0f, holdNeedle = 0.0f, peak = -100.0f, hold = -100.0f;
        bool  clipped = false, compact = false, dragged = false;
    };
}
