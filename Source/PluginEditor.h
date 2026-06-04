#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/ChoiceControls.h"
#include "ui/VuMeter.h"
#include "ui/ChannelView.h"

class StrataEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit StrataEditor (StrataProcessor&);
    ~StrataEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;          // 30 Hz UI refresh
    void refreshLinkPanel();                 // registry changed
    void setChannelViewMode (bool on);       // toggle strip <-> overview
    void layoutStrip (juce::Rectangle<int>); // normal single-channel layout

    StrataProcessor& proc;
    strata::ui::StrataLookAndFeel lnf;

    juce::TextButton chViewBtn { "CHANNEL VIEW" };
    strata::ui::ChannelView channelView { proc };
    bool channelViewMode = false;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttach = APVTS::SliderAttachment;
    using ButtonAttach = APVTS::ButtonAttachment;

    juce::Slider   inGain;                 // single INPUT trim knob (for now)
    std::unique_ptr<SliderAttach> inGainAtt;

    juce::TextButton bypassBtn { "BYPASS" }, muteBtn { "MUTE" }, monoBtn { "MONO" },
                     phaseBtn { juce::String::fromUTF8 ("\xC3\x98") }; // Ø polarity
    std::unique_ptr<ButtonAttach> bypassAtt, muteAtt, monoAtt, phaseAtt;

    strata::ui::PillBar   meterTypePills { strata::params::meterTypeChoices };
    strata::ui::PillBar   monitorPills   { strata::params::monitorChoices };
    juce::Label           groupLabel;
    strata::ui::BucketDots groupDots;

    strata::ui::VuMeter meter;              // single meter — shows plugin output

    juce::Label    nameLabel;       // editable channel name

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrataEditor)
};
