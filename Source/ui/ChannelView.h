#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "LookAndFeel.h"
#include "VuMeter.h"

/*  Control-surface overview. Shows one column per OTHER instance in the same
    bucket. Reads each instance's live meters/values (lock-free atomics) and can
    remotely set an individual instance's parameters. Gains are NOT ganged —
    every column drives exactly one target instance.

    Everything here runs on the MESSAGE THREAD (component + timer). No audio
    coupling whatsoever. */
namespace strata::ui
{
    // A MixHub-style bucket tab: number on top, bucket name underneath,
    // a bucket-colour accent strip, and an active highlight.
    class BucketTab : public juce::Component
    {
    public:
        int          group  = 1;
        bool         active = false;
        juce::uint32 colour = 0xff8a94a3;
        juce::String name;                 // empty when the bucket has no custom name
        std::function<void()> onSelect;

        void mouseUp (const juce::MouseEvent&) override { if (onSelect) onSelect(); }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour (juce::Colour (active ? Theme::panel : Theme::bg));
            g.fillRect (b);
            if (active) { g.setColour (juce::Colour (colour).withAlpha (0.22f)); g.fillRect (b); }

            g.setColour (juce::Colour (colour));               // top accent strip
            g.fillRect (b.removeFromTop (3.0f));
            g.setColour (juce::Colours::black.withAlpha (0.4f)); // divider
            g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());

            auto numRow = b.removeFromTop (16.0f);
            g.setColour (juce::Colour (active ? Theme::text : Theme::textDim));
            g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
            g.drawText (juce::String (group), numRow, juce::Justification::centred);

            g.setFont (juce::Font (juce::FontOptions (10.5f)));
            g.setColour (juce::Colour (active ? Theme::text : Theme::textDim));
            g.drawText (name, b.reduced (2.0f, 0.0f), juce::Justification::centredTop, true);
        }
    };

    class ChannelView : public juce::Component,
                        private juce::Timer
    {
    public:
        explicit ChannelView (StrataProcessor& p) : proc (p)
        {
            viewedGroup = proc.getGroup() != 0 ? proc.getGroup() : 1;

            // Bucket tabs: switch which group/bucket is displayed.
            for (int g = 1; g <= 8; ++g)
            {
                auto* t = tabs.add (new BucketTab());
                t->group = g;
                t->onSelect = [this, g] { viewedGroup = g; resized(); repaint(); };
                addAndMakeVisible (*t);
            }

            // Editable bucket name (double-click to rename).
            bucketName.setJustificationType (juce::Justification::centredLeft);
            bucketName.setColour (juce::Label::textColourId, juce::Colour (Theme::text));
            bucketName.setEditable (false, true, false);
            bucketName.onTextChange = [this]
            {
                link::InstanceRegistry::getInstance().setBucketName (viewedGroup, bucketName.getText());
            };
            addAndMakeVisible (bucketName);

            // ASSIGN: add/remove any instance to/from the viewed bucket.
            assignBtn.onClick = [this] { showAssignMenu(); };
            addAndMakeVisible (assignBtn);

            startTimerHz (24);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colour (Theme::bg));

            // header: viewed-bucket colour swatch
            g.setColour (juce::Colour (link::InstanceRegistry::getInstance().getBucketColour (viewedGroup)));
            g.fillRoundedRectangle (headerSwatch, 3.0f);

            if (columns.empty())
            {
                g.setColour (juce::Colour (Theme::textDim));
                g.setFont (juce::Font (juce::FontOptions (13.0f)));
                g.drawText ("No channels in this bucket yet. Use ASSIGN to add STRATA instances.",
                            getLocalBounds().reduced (20).withTrimmedTop (kHeaderH),
                            juce::Justification::centredTop, true);
            }
        }

        static constexpr int kTabRow = 42, kNameRow = 22, kHeaderH = kTabRow + kNameRow;

        void resized() override
        {
            auto r = getLocalBounds();

            // row 1: bucket tabs (number + name) | ASSIGN + meter source on the right
            auto row1 = r.removeFromTop (kTabRow);
            auto rightCol = row1.removeFromRight (66).reduced (4, 3);
            assignBtn.setBounds (rightCol.removeFromTop (16));

            const int n8 = (int) tabs.size();
            const int tw = n8 > 0 ? row1.getWidth() / n8 : 0;
            for (auto* t : tabs) t->setBounds (row1.removeFromLeft (tw));

            // row 2: swatch | editable bucket name (renames the viewed bucket)
            auto row2 = r.removeFromTop (kNameRow).reduced (4, 2);
            headerSwatch = row2.removeFromLeft (14).reduced (0, 1).toFloat();
            row2.removeFromLeft (6);
            bucketName.setBounds (row2);

            const int n = (int) columns.size();
            if (n == 0) return;
            const int w = juce::jmax (72, r.getWidth() / n);
            for (auto& c : columns)
                c->setBounds (r.removeFromLeft (w).reduced (3));
        }

    private:
        // ---- One remote channel strip -----------------------------------
        struct Column : public juce::Component
        {
            Column (StrataProcessor& owner, juce::Uuid target, bool self)
                : proc (owner), uuid (target), isSelf (self)
            {
                name.setJustificationType (juce::Justification::centred);
                name.setColour (juce::Label::textColourId,
                                juce::Colour (isSelf ? Theme::accent : Theme::text));
                name.setInterceptsMouseClicks (false, false);
                addAndMakeVisible (name);

                // Per-channel analog VU needle. Click clears; drag calibrates.
                vu.setCompact (true);
                vu.onClear = [this] { link::InstanceRegistry::getInstance().clearRemoteClip (uuid); };
                vu.onCalibrate = [this] (float refDb)
                {
                    if (params::meterTypeFixedRef (targetMeterType)) return; // K-scale fixed
                    link::InstanceRegistry::getInstance().setRemoteParameter (uuid, params::vuCal, refDb);
                };
                addAndMakeVisible (vu);

                gain.setSliderStyle (juce::Slider::LinearVertical);
                gain.setRange (-24.0, 24.0, 0.01);
                gain.setLookAndFeel (&faderLnf);
                gain.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                gain.setTextValueSuffix (" dB");
                gain.setDoubleClickReturnValue (true, 0.0);
                // Only push to the target on real user interaction, and remember
                // we are editing so the 24 Hz refresh never fights the drag.
                gain.onDragStart   = [this] { editing = true; };
                gain.onDragEnd     = [this] { editing = false; };
                // refresh() uses dontSendNotification, so this only fires on real
                // user input (drag, scroll, double-click, keyboard) -> always send.
                gain.onValueChange = [this]
                {
                    link::InstanceRegistry::getInstance().setRemoteParameter (
                        uuid, params::inputGain, (float) gain.getValue());
                };
                addAndMakeVisible (gain);

                inOut.setClickingTogglesState (true);
                inOut.setColour (juce::TextButton::buttonOnColourId, juce::Colour (Theme::warn));
                inOut.onClick = [this]
                {
                    link::InstanceRegistry::getInstance().setRemoteParameter (
                        uuid, params::bypass, inOut.getToggleState() ? 1.0f : 0.0f);
                };
                addAndMakeVisible (inOut);

                // LINK: cycles mirror sets 0(off) -> 1..4. Channels in the same
                // bucket sharing a set mirror each other's gain & bypass.
                linkBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (Theme::link));
                linkBtn.onClick = [this]
                {
                    const int next = (linkId + 1) % 5;
                    link::InstanceRegistry::getInstance().setRemoteLinkId (uuid, next);
                };
                addAndMakeVisible (linkBtn);
            }

            ~Column() override { gain.setLookAndFeel (nullptr); }

            // The header (name) row is a drag handle for reordering columns.
            static constexpr int headerHeight = 26;
            void mouseDown (const juce::MouseEvent& e) override
            {
                if (e.y < headerHeight && onHeaderDown) { headerDragging = true; onHeaderDown (e); }
            }
            void mouseDrag (const juce::MouseEvent& e) override
            {
                if (headerDragging && onHeaderDrag) onHeaderDrag (e);
            }
            void mouseUp (const juce::MouseEvent& e) override
            {
                if (headerDragging && onHeaderUp) { onHeaderUp (e); headerDragging = false; }
            }

            void resized() override
            {
                auto r = getLocalBounds().reduced (4);
                name.setBounds (r.removeFromTop (20));
                inOut.setBounds (r.removeFromBottom (22).reduced (2, 2));
                linkBtn.setBounds (r.removeFromBottom (20).reduced (2, 1));
                r.removeFromTop (3);
                // analog VU across the top, large fader fills the rest
                vu.setBounds (r.removeFromTop (juce::jlimit (60, 96, r.getHeight() * 2 / 5)));
                r.removeFromTop (4);
                gain.setBounds (r);
                faderBg = r.toFloat(); // tinted by input-gain health
            }

            // Gain-staging "health" of the INPUT level feeding downstream plugins.
            // Ranges are deliberately conservative for live streaming; tweak freely.
            static juce::Colour healthColour (float inputPeakDb) noexcept
            {
                if (inputPeakDb >= 0.0f)  return juce::Colour (0xffe5484d); // RED  : clipping / too hot
                if (inputPeakDb >= -3.0f) return juce::Colour (0xffe2b341); // AMBER: hot, watch it
                if (inputPeakDb >= -9.0f) return juce::Colour (0xff3fb86b); // GREEN: healthy
                if (inputPeakDb >= -24.0f)return juce::Colour (0xff4178d6); // BLUE : a bit low
                return juce::Colour (0xff5b5f6b);                           // GREY : little/no signal
            }

            void paint (juce::Graphics& g) override
            {
                auto bounds = getLocalBounds().toFloat();
                g.setColour (juce::Colour (Theme::panel));
                g.fillRoundedRectangle (bounds, 5.0f);
                if (isSelf) // highlight the instance you're sitting on
                {
                    g.setColour (juce::Colour (Theme::accent).withAlpha (0.9f));
                    g.drawRoundedRectangle (bounds.reduced (1.0f), 5.0f, 1.5f);
                }

                // bucket colour strip across the very top
                g.setColour (juce::Colour (accent));
                g.fillRoundedRectangle (bounds.getX() + 4.0f, bounds.getY() + 2.0f,
                                        bounds.getWidth() - 8.0f, 3.0f, 1.5f);

                // drag-handle grip dots in the header (reorder hint)
                g.setColour (juce::Colour (Theme::textDim));
                for (int gx = 0; gx < 2; ++gx)
                    for (int gy = 0; gy < 3; ++gy)
                        g.fillEllipse (6.0f + (float) gx * 4.0f, 10.0f + (float) gy * 4.0f, 2.0f, 2.0f);

                // gain-staging health wash behind the fader (volunteer-friendly:
                // green = good input level, amber/red = too hot, blue/grey = low)
                const auto hc = healthClip ? juce::Colour (0xffe5484d) // latched clip -> red
                                           : healthColour (healthHeldDb);
                g.setGradientFill (juce::ColourGradient (hc.withAlpha (0.42f), faderBg.getCentreX(), faderBg.getY(),
                                                         hc.withAlpha (0.14f), faderBg.getCentreX(), faderBg.getBottom(), false));
                g.fillRoundedRectangle (faderBg, 4.0f);
                g.setColour (hc.withAlpha (0.7f));
                g.drawRoundedRectangle (faderBg.reduced (0.5f), 4.0f, 1.0f);
            }

            // Pulled from the live instance each tick (message thread).
            void refresh (link::InstanceClient* c, juce::uint32 bucketColour)
            {
                accent = bucketColour;
                name.setText (c->getDisplayName(), juce::dontSendNotification);

                // Meter shows the plugin OUTPUT, using the target's calibration.
                targetMeterType = c->getMeterType();
                vu.setScale (params::meterTypeLabel (targetMeterType),
                             c->getVuReferenceDb(), c->getMeterOffsetDb());
                const float rms = c->getOutputRmsDb();
                const float pk  = c->getOutputPeakDb();
                vu.setValues (rms, pk, pk, c->getOutputClip());

                // A clip (auto-releasing) forces the wash red.
                healthClip = c->getOutputClip();

                // Health wash tracks the output level, with a peak-hold so the
                // colour doesn't flicker: snaps up, holds ~1.25 s, eases down.
                healthDb = c->getOutputPeakDb();
                if (healthDb >= healthHeldDb)   { healthHeldDb = healthDb; healthHold = kHealthHoldFrames; }
                else if (healthHold > 0)        { --healthHold; }
                else                            { healthHeldDb = juce::jmax (healthDb, healthHeldDb - kHealthReleaseDb); }

                // Never write to the fader while the user is dragging/editing it.
                if (! editing)
                {
                    const float gv = c->getParamValue (params::inputGain);
                    if (std::abs ((float) gain.getValue() - gv) > 0.001f)
                        gain.setValue (gv, juce::dontSendNotification);
                }
                inOut.setToggleState (c->isBypassed(), juce::dontSendNotification);
                inOut.setButtonText (c->isBypassed() ? "OUT" : "IN");

                linkId = c->getLinkId();
                linkBtn.setButtonText (linkId == 0 ? "LINK" : "LINK " + juce::String (linkId));
                linkBtn.setToggleState (linkId != 0, juce::dontSendNotification);
                repaint();
            }

            StrataProcessor& proc;
            juce::Uuid       uuid;
            juce::Label      name;
            juce::Slider     gain;
            juce::TextButton inOut { "IN" };
            juce::TextButton linkBtn { "LINK" };
            VuMeter          vu;
            int              linkId = 0;
            int              targetMeterType = 0;
            float            healthDb = -100.0f;     // instantaneous input peak
            float            healthHeldDb = -100.0f; // peak-held value used for colour
            bool             healthClip = false;     // latched clip -> red until cleared
            int              healthHold = 0;         // frames remaining at the held peak
            static constexpr int   kHealthHoldFrames = 30;  // ~1.25 s at 24 Hz
            static constexpr float kHealthReleaseDb  = 0.5f; // dB/frame after hold (~12 dB/s)
            juce::Rectangle<float> faderBg;          // tinted background behind fader
            juce::uint32 accent = 0xff8a94a3; // bucket colour
            bool  editing = false; // true while the user drags this fader
            bool  isSelf  = false; // the instance whose window is open
            bool  headerDragging = false;
            std::function<void (const juce::MouseEvent&)> onHeaderDown, onHeaderDrag, onHeaderUp;
        };

        inline static FaderLookAndFeel faderLnf; // shared by all columns

        void timerCallback() override
        {
            if (reordering) return; // don't touch layout/values mid-drag

            auto live = link::InstanceRegistry::getInstance().clientsInGroup (viewedGroup);
            const auto selfUuid = proc.getUuid();

            // Rebuild columns when the membership set OR the viewed bucket changes.
            if (viewedGroup != builtGroup || membershipChanged (live))
            {
                builtGroup = viewedGroup;
                columns.clear();
                uuids.clear();
                for (auto* c : live)
                {
                    const auto u = c->getUuid();
                    uuids.push_back (u);
                    auto col = std::make_unique<Column> (proc, u, u == selfUuid);
                    auto* cp = col.get();
                    col->onHeaderDown = [this, cp] (const juce::MouseEvent& e) { beginReorder (cp, e); };
                    col->onHeaderDrag = [this, cp] (const juce::MouseEvent& e) { dragReorder  (cp, e); };
                    col->onHeaderUp   = [this, cp] (const juce::MouseEvent&)   { endReorder   (cp); };
                    addAndMakeVisible (*col);
                    columns.push_back (std::move (col));
                }
                resized();
                repaint();
            }

            // Keep the header (tabs / bucket name / meter source) in sync.
            auto& reg = link::InstanceRegistry::getInstance();
            if (! bucketName.isBeingEdited())
                bucketName.setText (reg.getBucketName (viewedGroup), juce::dontSendNotification);
            for (int g = 1; g <= 8; ++g)
            {
                auto* t = tabs[g - 1];
                const auto nm = reg.getBucketName (g);
                const bool custom = nm != ("Group " + juce::String (g)); // show name only if set
                const bool active = (g == viewedGroup);
                if (t->active != active || t->name != (custom ? nm : juce::String())
                    || t->colour != reg.getBucketColour (g))
                {
                    t->active = active;
                    t->name   = custom ? nm : juce::String();
                    t->colour = reg.getBucketColour (g);
                    t->repaint();
                }
            }

            const auto colour = reg.getBucketColour (viewedGroup);

            // Update each column from its live client.
            for (size_t i = 0; i < columns.size() && i < live.size(); ++i)
                columns[i]->refresh (live[i], colour);
        }

        //---- Drag-to-reorder columns ------------------------------------
        int indexOf (Column* c) const
        {
            for (int i = 0; i < (int) columns.size(); ++i)
                if (columns[(size_t) i].get() == c) return i;
            return -1;
        }

        juce::Rectangle<int> columnsArea() const { return getLocalBounds().withTrimmedTop (kHeaderH); }

        void layoutExcept (Column* skip)
        {
            const int n = (int) columns.size();
            if (n == 0) return;
            auto r = columnsArea();
            const int w = juce::jmax (72, r.getWidth() / n);
            for (auto& cc : columns)
            {
                auto slot = r.removeFromLeft (w).reduced (3);
                if (cc.get() != skip) cc->setBounds (slot);
            }
        }

        void beginReorder (Column* c, const juce::MouseEvent& e)
        {
            dragCol = c; reordering = true;
            grabOffsetX = e.x;
            c->toFront (false);
        }

        void dragReorder (Column* c, const juce::MouseEvent& e)
        {
            if (dragCol != c) return;
            const int n = (int) columns.size();
            if (n <= 1) return;

            const int w = c->getWidth();
            const int newX = juce::jlimit (0, getWidth() - w,
                                           e.getEventRelativeTo (this).x - grabOffsetX);
            c->setTopLeftPosition (newX, c->getY());

            const int slotW = juce::jmax (1, getWidth() / n);
            const int target = juce::jlimit (0, n - 1, (newX + w / 2) / slotW);
            const int current = indexOf (c);
            if (target != current && current >= 0)
            {
                auto moved = std::move (columns[(size_t) current]);
                columns.erase (columns.begin() + current);
                columns.insert (columns.begin() + target, std::move (moved));
                uuids.clear();
                for (auto& cc : columns) uuids.push_back (cc->uuid);
                layoutExcept (c); // reflow the others; dragged one follows the mouse
            }
        }

        void endReorder (Column* c)
        {
            reordering = false; dragCol = nullptr;
            auto& reg = link::InstanceRegistry::getInstance();
            for (int i = 0; i < (int) columns.size(); ++i)
                reg.setRemoteOrderIndex (columns[(size_t) i]->uuid, i); // persist new order
            reg.notifyMetadataChanged(); // refresh any other open Channel Views once
            resized();                   // snap everything to its slot
            juce::ignoreUnused (c);
        }

        bool membershipChanged (const std::vector<link::InstanceClient*>& live) const
        {
            if (live.size() != uuids.size()) return true;
            for (size_t i = 0; i < live.size(); ++i)
                if (live[i]->getUuid() != uuids[i]) return true;
            return false;
        }

        // Add/remove any instance in the session to/from the viewed bucket.
        void showAssignMenu()
        {
            auto& reg = link::InstanceRegistry::getInstance();
            auto all = reg.snapshot (nullptr);
            const int vg = viewedGroup;

            juce::PopupMenu menu;
            menu.addSectionHeader ("Assign to " + reg.getBucketName (vg));
            if (all.empty())
                menu.addItem ("(no STRATA instances)", false, false, [] {});
            for (auto& e : all)
            {
                const bool in = (e.groupId == vg);
                juce::String label = e.name;
                if (! in && e.groupId != 0) label += "   [" + reg.getBucketName (e.groupId) + "]";
                const auto id = e.uuid;
                menu.addItem (label, true, in, [id, vg, in]
                {
                    link::InstanceRegistry::getInstance().setRemoteGroupId (id, in ? 0 : vg);
                });
            }
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&assignBtn));
        }

        StrataProcessor& proc;
        std::vector<std::unique_ptr<Column>> columns;
        std::vector<juce::Uuid> uuids;
        Column* dragCol = nullptr;
        int  grabOffsetX = 0;
        bool reordering = false;

        juce::OwnedArray<BucketTab> tabs;
        juce::TextButton assignBtn { "ASSIGN" };
        juce::Label      bucketName;
        juce::Rectangle<float> headerSwatch;
        int  viewedGroup = 1;
        int  builtGroup = -1;
    };
}
