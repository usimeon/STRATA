#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <utility>
#include "../link/InstanceRegistry.h"
#include "LookAndFeel.h"

namespace strata::ui
{
    class PillBar : public juce::Component
    {
    public:
        PillBar (juce::StringArray pillLabels,
                 juce::Colour fillOn = juce::Colour (Theme::accent),
                 juce::Colour textOn = juce::Colour (0xff11140f))
            : labels (std::move (pillLabels)), activeFill (fillOn), activeText (textOn)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        std::function<void (int)> onChange;

        void setSelectedIndex (int idx)
        {
            idx = juce::jlimit (0, juce::jmax (0, labels.size() - 1), idx);
            if (selectedIndex == idx)
                return;

            selectedIndex = idx;
            repaint();
        }

        int getSelectedIndex() const noexcept { return selectedIndex; }

        void mouseMove (const juce::MouseEvent& e) override
        {
            updateHover (e.position);
        }

        void mouseExit (const juce::MouseEvent&) override
        {
            if (hoveredIndex == -1)
                return;

            hoveredIndex = -1;
            repaint();
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            selectAt (e.position);
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            selectAt (e.position);
        }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const int n = labels.size();
            if (n <= 0)
                return;

            const float gap = 4.0f;
            const float pillW = juce::jmax (1.0f, (bounds.getWidth() - gap * (float) juce::jmax (0, n - 1)) / (float) n);
            const float pillH = bounds.getHeight();
            const auto baseFill = juce::Colour (Theme::panel);
            const auto hoverFill = juce::Colour (0xff232a33);
            const auto border = juce::Colour (Theme::stroke);

            g.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));

            for (int i = 0; i < n; ++i)
            {
                const float x = bounds.getX() + (float) i * (pillW + gap);
                const auto pill = juce::Rectangle<float> (x, bounds.getY(), pillW, pillH);
                const bool active = (i == selectedIndex);
                const bool hover  = (i == hoveredIndex);

                g.setColour (active ? activeFill : (hover ? hoverFill : baseFill));
                g.fillRoundedRectangle (pill, 3.0f);

                g.setColour (active ? juce::Colours::transparentBlack : border);
                g.drawRoundedRectangle (pill, 3.0f, 1.0f);

                g.setColour (active ? activeText : juce::Colour (Theme::textDim));
                g.drawText (labels[i], pill.toNearestInt(), juce::Justification::centred, true);
            }
        }

    private:
        void updateHover (juce::Point<float> position)
        {
            const int next = indexAt (position);
            if (next == hoveredIndex)
                return;

            hoveredIndex = next;
            repaint();
        }

        void selectAt (juce::Point<float> position)
        {
            const int next = indexAt (position);
            if (next < 0 || next == selectedIndex)
                return;

            selectedIndex = next;
            repaint();

            if (onChange != nullptr)
                onChange (selectedIndex);
        }

        int indexAt (juce::Point<float> position) const
        {
            const int n = labels.size();
            if (n <= 0)
                return -1;

            const auto bounds = getLocalBounds().toFloat();
            const float gap = 4.0f;
            const float pillW = juce::jmax (1.0f, (bounds.getWidth() - gap * (float) juce::jmax (0, n - 1)) / (float) n);

            for (int i = 0; i < n; ++i)
            {
                const float x = bounds.getX() + (float) i * (pillW + gap);
                const auto pill = juce::Rectangle<float> (x, bounds.getY(), pillW, bounds.getHeight());
                if (pill.contains (position))
                    return i;
            }

            return -1;
        }

        juce::StringArray labels;
        int selectedIndex = 0;
        int hoveredIndex = -1;
        juce::Colour activeFill;
        juce::Colour activeText;
    };

    class BucketDots : public juce::Component
    {
    public:
        BucketDots()
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        std::function<void (int)> onChange;

        void setSelectedGroup (int group)
        {
            group = juce::jlimit (0, 8, group);
            if (selectedGroup == group)
                return;

            selectedGroup = group;
            repaint();
        }

        int getSelectedGroup() const noexcept { return selectedGroup; }

        void mouseDown (const juce::MouseEvent& e) override
        {
            selectAt (e.position);
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            selectAt (e.position);
        }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            const int count = 9;
            if (bounds.isEmpty())
                return;

            const float slot = bounds.getWidth() / (float) count;
            const float baseSize = juce::jlimit (5.0f, 9.0f, slot - 2.0f);
            const float selectedSize = juce::jlimit (8.0f, 12.0f, slot - 1.0f);
            const float cy = bounds.getCentreY();

            for (int i = 0; i < count; ++i)
            {
                const auto colour = juce::Colour (link::InstanceRegistry::getInstance().getBucketColour (i));
                const bool selected = (i == selectedGroup);
                const float size = selected ? selectedSize : baseSize;
                const float cx = bounds.getX() + slot * ((float) i + 0.5f);
                const auto dot = juce::Rectangle<float> (cx - size * 0.5f, cy - size * 0.5f, size, size);

                g.setColour (colour.withAlpha (selected ? 1.0f : 0.55f));
                g.fillEllipse (dot);

                if (selected)
                {
                    g.setColour (colour);
                    g.drawEllipse (dot.expanded (2.0f), 1.5f);
                }
            }
        }

    private:
        void selectAt (juce::Point<float> position)
        {
            const int next = indexAt (position);
            if (next < 0 || next == selectedGroup)
                return;

            selectedGroup = next;
            repaint();

            if (onChange != nullptr)
                onChange (selectedGroup);
        }

        int indexAt (juce::Point<float> position) const
        {
            const auto bounds = getLocalBounds().toFloat();
            if (bounds.isEmpty())
                return -1;

            const int count = 9;
            const float slot = bounds.getWidth() / (float) count;
            const float localX = juce::jlimit (0.0f, bounds.getWidth(), position.x - bounds.getX());
            return juce::jlimit (0, count - 1, (int) (localX / slot));
        }

        int selectedGroup = 0;
    };
}
