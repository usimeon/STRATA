#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/*  Dark, stage-readable theme. High contrast, large hit targets, no gloss. */
namespace strata::ui
{
    struct Theme
    {
        static constexpr juce::uint32 bg      = 0xff14171c;
        static constexpr juce::uint32 panel   = 0xff1d222a;
        static constexpr juce::uint32 stroke  = 0xff2c333d;
        static constexpr juce::uint32 text    = 0xffe7ecf2;
        static constexpr juce::uint32 textDim = 0xff8a94a3;
        static constexpr juce::uint32 accent  = 0xff36c1a6; // teal
        static constexpr juce::uint32 link    = 0xff5b8cff; // blue (linked)
        static constexpr juce::uint32 warn     = 0xffe2b341; // amber
        static constexpr juce::uint32 clip    = 0xffe5484d; // red
        static constexpr juce::uint32 vuNeedle = 0xfff2e9d8;
    };

    class StrataLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        StrataLookAndFeel()
        {
            setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (Theme::bg));
            setColour (juce::Slider::textBoxTextColourId,         juce::Colour (Theme::text));
            setColour (juce::Slider::textBoxOutlineColourId,      juce::Colour (Theme::stroke));
            setColour (juce::Label::textColourId,                 juce::Colour (Theme::text));
            setDefaultSansSerifTypefaceName ("Inter");
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle,
                               juce::Slider&) override
        {
            auto bodyBounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
            const float diameter = juce::jmin (bodyBounds.getWidth(), bodyBounds.getHeight());
            bodyBounds = bodyBounds.withSizeKeepingCentre (diameter, diameter);
            const auto centre = bodyBounds.getCentre();
            const float radius = diameter * 0.5f;
            const float angle  = startAngle + pos * (endAngle - startAngle);

            juce::ColourGradient bodyGrad (juce::Colour (0xff272e38), centre.x - radius * 0.25f,
                                           centre.y - radius * 0.25f, juce::Colour (0xff1a1f27),
                                           centre.x, centre.y + radius, false);
            bodyGrad.addColour (0.45, juce::Colour (0xff232935));
            bodyGrad.addColour (1.0, juce::Colour (0xff1a1f27));
            g.setGradientFill (bodyGrad);
            g.fillEllipse (bodyBounds);

            // self-ring: the strip's only knob is the "current instance" control.
            g.setColour (juce::Colour (Theme::accent));
            g.drawEllipse (bodyBounds.expanded (2.5f), 1.5f);

            // track
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, radius - 8.0f, radius - 8.0f,
                                 0.0f, startAngle, endAngle, true);
            g.setColour (juce::Colour (Theme::stroke));
            g.strokePath (track, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            // value arc
            juce::Path val;
            val.addCentredArc (centre.x, centre.y, radius - 8.0f, radius - 8.0f,
                               0.0f, startAngle, angle, true);
            g.setColour (juce::Colour (Theme::accent));
            g.strokePath (val, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            // pointer
            const auto tip = centre.getPointOnCircumference (radius - 13.0f, angle);
            g.setColour (juce::Colour (Theme::text));
            g.drawLine ({ centre, tip }, 2.0f);

            // hub
            g.setColour (juce::Colour (Theme::stroke));
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (centre));
        }
    };

    //========================================================================
    //  Large console-style channel fader with a printed dB scale (for the
    //  Channel View). Draws a light fader cap on a groove, plus numeric marks
    //  aligned to the slider's real value->pixel mapping.
    //========================================================================
    class FaderLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        FaderLookAndFeel() { setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                               float sliderPos, float minSP, float maxSP,
                               juce::Slider::SliderStyle style, juce::Slider& s) override
        {
            if (style != juce::Slider::LinearVertical)
            {
                LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minSP, maxSP, style, s);
                return;
            }

            auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
            auto scaleCol = area.removeFromRight (18.0f);     // right column for numbers
            const float cx = area.getCentreX();

            // ---- thin recessed track (dark line + subtle highlight) --------
            const float top = area.getY() + 8.0f, bot = area.getBottom() - 8.0f;
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (juce::Rectangle<float> (cx - 2.5f, top, 5.0f, bot - top), 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRect (cx + 1.2f, top, 1.0f, bot - top);

            // ---- dB scale numbers (aligned to JUCE's value->pixel map) -----
            g.setFont (juce::Font (juce::FontOptions (8.5f)));
            const auto lo = (float) s.getMinimum(), hi = (float) s.getMaximum();
            for (int db = (int) hi; db >= (int) lo; db -= 6)
            {
                const float yy = juce::jmap ((float) db, lo, hi, minSP, maxSP);
                g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.95f : 0.55f));
                g.drawText (juce::String (db > 0 ? "+" : "") + juce::String (db),
                            juce::Rectangle<float> (scaleCol.getX(), yy - 5.5f, scaleCol.getWidth() - 1.0f, 11.0f),
                            juce::Justification::centredRight);
            }

            drawMetalCap (g, cx, sliderPos, juce::jmax (18.0f, juce::jmin (46.0f, area.getWidth() - 2.0f)));
        }

    private:
        // Brushed-aluminium console fader cap with a central index groove.
        static void drawMetalCap (juce::Graphics& g, float cx, float cy, float capW)
        {
            const float capH = 52.0f;
            juce::Rectangle<float> cap (cx - capW * 0.5f, cy - capH * 0.5f, capW, capH);
            const float r = 3.0f;

            // drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (cap.translated (0.0f, 2.0f).expanded (0.5f), r);

            // body: brushed-aluminium gradient with ribbed bands.
            juce::ColourGradient grad (juce::Colour (0xffc8c8cc), cap.getX(), cap.getY(),
                                       juce::Colour (0xffb0b0b4), cap.getX(), cap.getBottom(), false);
            grad.addColour (0.10, juce::Colour (0xffd6d6da));
            grad.addColour (0.48, juce::Colour (0xffa8a8ac));
            grad.addColour (0.52, juce::Colour (0xff989899));
            grad.addColour (0.90, juce::Colour (0xffb0b0b4));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (cap, r);

            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (cap.toNearestInt());

            for (float yy = cap.getY(); yy < cap.getBottom(); yy += 10.0f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.38f));
                g.fillRect (cap.getX(), yy, cap.getWidth(), 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.28f));
                g.fillRect (cap.getX(), yy + 8.0f, cap.getWidth(), 2.0f);
            }

            // edge bevel: light top/left, dark bottom/right
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawLine (cap.getX() + 2.0f, cap.getY() + 1.0f, cap.getRight() - 2.0f, cap.getY() + 1.0f, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawLine (cap.getX() + 2.0f, cap.getBottom() - 1.0f, cap.getRight() - 2.0f, cap.getBottom() - 1.0f, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawRoundedRectangle (cap, r, 1.0f);

            // central index groove (the read line)
            juce::Rectangle<float> slot (cap.getX() + 2.0f, cy - 3.0f, cap.getWidth() - 4.0f, 6.0f);
            g.setColour (juce::Colour (0xff3c3c40));
            g.fillRoundedRectangle (slot, 1.5f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawLine (slot.getX(), slot.getY(), slot.getRight(), slot.getY(), 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.85f)); // bright index line = exact value
            g.drawLine (slot.getX() + 1.0f, cy, slot.getRight() - 1.0f, cy, 1.0f);
        }
    };
}
