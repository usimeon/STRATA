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

            // Load the brushed-metal knob cap texture once
            capImage = juce::ImageCache::getFromMemory (
                BinaryData::knob_cap_png, BinaryData::knob_cap_pngSize);
        }

        // ─────────────────────────────────────────────────────────────────────
        // Rotary knob — photorealistic brushed-metal body + teal value arc
        // ─────────────────────────────────────────────────────────────────────
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle,
                               juce::Slider&) override
        {
            // Leave a margin so the value arc doesn't clip the text box area
            auto bounds = juce::Rectangle<float> ((float)x, (float)y, (float)w, (float)h)
                              .reduced (4.0f);
            const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
            bounds = bounds.withSizeKeepingCentre (diameter, diameter);
            const auto  centre = bounds.getCentre();
            const float radius = diameter * 0.5f;
            const float angle  = startAngle + pos * (endAngle - startAngle);

            // ── Value track (full range, dark) ────────────────────────────────
            const float trackR = radius - 4.0f;
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f,
                                 startAngle, endAngle, true);
            g.setColour (juce::Colour (0xff1a1c21));
            g.strokePath (track, juce::PathStrokeType (4.5f,
                                  juce::PathStrokeType::curved,
                                  juce::PathStrokeType::rounded));

            // ── Value arc (teal, filled from start to current) ─────────────────
            if (pos > 0.005f)
            {
                juce::Path varc;
                varc.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f,
                                    startAngle, angle, true);
                g.setColour (juce::Colour (Theme::accent));
                g.strokePath (varc, juce::PathStrokeType (4.5f,
                                    juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));
            }

            // ── Knob body ─────────────────────────────────────────────────────
            auto body = bounds.reduced (10.0f);

            // Soft drop shadow (layered translucent ellipses offset down)
            for (float s = 4.0f; s >= 1.0f; s -= 1.0f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.12f));
                g.fillEllipse (body.translated (0.0f, s));
            }

            {
                juce::Path circle;
                circle.addEllipse (body);
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (circle);

                // Brushed-metal cap texture (non-rotating — looks like machined aluminium)
                if (capImage.isValid())
                {
                    g.drawImage (capImage, body, juce::RectanglePlacement::fillDestination);
                }
                else
                {
                    // Fallback: premium diagonal gradient
                    juce::ColourGradient metal (
                        juce::Colour (0xffe4e4e8),
                        body.getX()   + body.getWidth()  * 0.25f,
                        body.getY()   + body.getHeight() * 0.15f,
                        juce::Colour (0xff6a6a72),
                        body.getRight() - body.getWidth()  * 0.2f,
                        body.getBottom() - body.getHeight() * 0.15f, false);
                    g.setGradientFill (metal);
                    g.fillEllipse (body);
                }

                // Specular highlight — bright soft spot top-left (light source)
                juce::ColourGradient spec (
                    juce::Colours::white.withAlpha (0.45f),
                    body.getX()   + body.getWidth()  * 0.28f,
                    body.getY()   + body.getHeight() * 0.18f,
                    juce::Colours::transparentBlack,
                    body.getCentreX(), body.getCentreY(), true);
                g.setGradientFill (spec);
                g.fillEllipse (body);

                // Bottom darkening — ambient occlusion / curvature shadow
                juce::ColourGradient dark (
                    juce::Colours::transparentBlack,
                    body.getCentreX(), body.getCentreY() - body.getHeight() * 0.1f,
                    juce::Colours::black.withAlpha (0.32f),
                    body.getCentreX(), body.getBottom(), false);
                g.setGradientFill (dark);
                g.fillEllipse (body);
            }

            // Rim: bright top bevel + dark bottom bevel (one-pixel each)
            g.setColour (juce::Colours::white.withAlpha (0.28f));
            g.drawEllipse (body.reduced (0.5f), 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawEllipse (body.expanded (0.5f), 1.0f);

            // ── Pointer ───────────────────────────────────────────────────────
            const float pRadius = body.getWidth() * 0.5f;
            const auto  pTip    = centre.getPointOnCircumference (pRadius - 5.0f,  angle);
            const auto  pBase   = centre.getPointOnCircumference (pRadius * 0.28f, angle);

            // Pointer shadow
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawLine (pBase.x + 0.7f, pBase.y + 1.2f,
                        pTip.x  + 0.7f, pTip.y  + 1.2f, 3.0f);
            // Pointer line
            g.setColour (juce::Colour (0xff1b1814));
            g.drawLine (pBase.x, pBase.y, pTip.x, pTip.y, 2.5f);
            // Bright tip dot
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (juce::Rectangle<float> (4.5f, 4.5f).withCentre (pTip));

            // ── Centre hub cap ─────────────────────────────────────────────────
            const float hubR = body.getWidth() * 0.11f;
            g.setColour (juce::Colour (0xff26262c));
            g.fillEllipse (juce::Rectangle<float> (hubR * 2, hubR * 2).withCentre (centre));
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.fillEllipse (juce::Rectangle<float> (hubR, hubR)
                               .withCentre (centre.translated (-hubR * 0.15f, -hubR * 0.18f)));
        }

    private:
        juce::Image capImage;
    };
}
