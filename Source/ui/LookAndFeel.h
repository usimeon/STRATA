#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

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
        static constexpr juce::uint32 warn    = 0xffe2b341; // amber
        static constexpr juce::uint32 clip    = 0xffe5484d; // red
        static constexpr juce::uint32 vuNeedle= 0xff1b1814;
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

            // Load the knob filmstrip once (64×1088, 17 frames of 64×64)
            knobStrip = juce::ImageCache::getFromMemory (
                BinaryData::knob_png, BinaryData::knob_pngSize);
        }

        // ─────────────────────────────────────────────────────────────────────
        // Rotary knob — filmstrip + teal value arc
        // Strip is 64×1088 px: 17 frames × 64 px, min→max rotation top→bottom.
        // ─────────────────────────────────────────────────────────────────────
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle,
                               juce::Slider&) override
        {
            auto bounds = juce::Rectangle<float> ((float)x, (float)y, (float)w, (float)h)
                              .reduced (4.0f);
            const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
            bounds = bounds.withSizeKeepingCentre (diameter, diameter);
            const auto  centre = bounds.getCentre();
            const float radius = diameter * 0.5f;

            // ── Value track (full range, dark) ────────────────────────────────
            const float trackR = radius - 2.0f;
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f,
                                 startAngle, endAngle, true);
            g.setColour (juce::Colour (0xff1a1c21));
            g.strokePath (track, juce::PathStrokeType (4.5f,
                                  juce::PathStrokeType::curved,
                                  juce::PathStrokeType::rounded));

            // ── Teal value arc ────────────────────────────────────────────────
            if (pos > 0.005f)
            {
                const float angle = startAngle + pos * (endAngle - startAngle);
                juce::Path varc;
                varc.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f,
                                    startAngle, angle, true);
                g.setColour (juce::Colour (Theme::accent));
                g.strokePath (varc, juce::PathStrokeType (4.5f,
                                    juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));
            }

            // ── Filmstrip knob body ───────────────────────────────────────────
            if (knobStrip.isValid())
            {
                const int frameW    = knobStrip.getWidth();            // 64
                const int numFrames = knobStrip.getHeight() / frameW;  // 17
                const int frame     = juce::roundToInt (pos * (float)(numFrames - 1));
                const int srcY      = frame * frameW;

                // Scale strip frame to fill the rotary bounds
                g.drawImage (knobStrip,
                             (int) bounds.getX(),  (int) bounds.getY(),
                             (int) bounds.getWidth(), (int) bounds.getHeight(),
                             0, srcY, frameW, frameW);
            }
            else
            {
                // Fallback: rendered gradient knob
                auto body = bounds.reduced (8.0f);
                juce::ColourGradient metal (
                    juce::Colour (0xffe0e0e4), body.getX() + body.getWidth() * 0.25f,
                                               body.getY() + body.getHeight() * 0.2f,
                    juce::Colour (0xff606068), body.getCentreX(), body.getBottom(), false);
                g.setGradientFill (metal);
                g.fillEllipse (body);
                g.setColour (juce::Colours::black.withAlpha (0.5f));
                g.drawEllipse (body, 1.0f);
                const float angle = startAngle + pos * (endAngle - startAngle);
                const auto  tip   = centre.getPointOnCircumference (body.getWidth() * 0.5f - 5.0f, angle);
                const auto  base  = centre.getPointOnCircumference (body.getWidth() * 0.15f, angle);
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.drawLine (base.x, base.y, tip.x, tip.y, 2.5f);
            }
        }

    private:
        juce::Image knobStrip;
    };
}
