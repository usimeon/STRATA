#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
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
    using ComboAttach  = APVTS::ComboBoxAttachment;

    juce::Slider   inGain;                 // single INPUT trim knob (for now)
    std::unique_ptr<SliderAttach> inGainAtt;

    juce::TextButton bypassBtn { "BYPASS" }, muteBtn { "MUTE" }, monoBtn { "MONO" },
                     phaseBtn { juce::String::fromUTF8 ("\xC3\x98") }; // Ø polarity
    std::unique_ptr<ButtonAttach> bypassAtt, muteAtt, monoAtt, phaseAtt;

    juce::ComboBox meterTypeBox, monitorBox;
    std::unique_ptr<ComboAttach> meterTypeAtt, monitorAtt;

    strata::ui::VuMeter meter;              // single meter — shows plugin output

    juce::Label    nameLabel;       // editable channel name
    juce::ComboBox groupBox;        // group selector (0 = none, 1..8)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrataEditor)
};
