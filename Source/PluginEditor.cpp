#include "PluginEditor.h"
#include <cmath>

using namespace strata;

namespace
{
    static void setChoiceParam (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& paramId, int index)
    {
        if (auto* p = apvts.getParameter (paramId))
        {
            const float norm = apvts.getParameterRange (paramId).convertTo0to1 ((float) index);
            if (std::abs (p->getValue() - norm) < 1.0e-6f)
                return;

            p->beginChangeGesture();
            p->setValueNotifyingHost (norm);
            p->endChangeGesture();
        }
    }

    static int getChoiceIndex (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
    {
        if (auto* raw = apvts.getRawParameterValue (paramId))
            return juce::roundToInt (raw->load());
        return 0;
    }
}

StrataEditor::StrataEditor (StrataProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    guiScaleBox.addItem ("75%", 1);
    guiScaleBox.addItem ("100%", 2);
    guiScaleBox.addItem ("125%", 3);
    guiScaleBox.addItem ("150%", 4);
    guiScaleBox.addItem ("200%", 5);
    guiScaleBox.setSelectedId (2, juce::dontSendNotification);
    guiScaleBox.onChange = [this]
    {
        static constexpr float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
        setUiScale (scales[(size_t) juce::jlimit (0, 4, guiScaleBox.getSelectedItemIndex())]);
    };
    content.addAndMakeVisible (guiScaleBox);

    auto setupKnob = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        s.setTextValueSuffix (" dB");
        s.setDoubleClickReturnValue (true, 0.0);
        content.addAndMakeVisible (s);
    };
    setupKnob (inGain);
    inGainAtt = std::make_unique<SliderAttach> (proc.apvts, params::inputGain, inGain);

    for (auto* b : { &bypassBtn, &muteBtn, &monoBtn, &phaseBtn })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::accent));
        content.addAndMakeVisible (*b);
    }
    bypassBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::warn));
    muteBtn  .setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::clip));
    bypassAtt = std::make_unique<ButtonAttach> (proc.apvts, params::bypass,      bypassBtn);
    muteAtt   = std::make_unique<ButtonAttach> (proc.apvts, params::mute,        muteBtn);
    monoAtt   = std::make_unique<ButtonAttach> (proc.apvts, params::monoSum,     monoBtn);
    phaseAtt  = std::make_unique<ButtonAttach> (proc.apvts, params::phaseInvert, phaseBtn);

    meterTypePills.onChange = [this] (int index) { setChoiceParam (proc.apvts, params::meterType, index); };
    monitorPills.onChange   = [this] (int index) { setChoiceParam (proc.apvts, params::monitorMode, index); };
    content.addAndMakeVisible (meterTypePills);
    content.addAndMakeVisible (monitorPills);

    groupLabel.setText ("GROUP", juce::dontSendNotification);
    groupLabel.setJustificationType (juce::Justification::centredLeft);
    groupLabel.setColour (juce::Label::textColourId, juce::Colour (ui::Theme::textDim));
    groupLabel.setInterceptsMouseClicks (false, false);
    groupLabel.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
    content.addAndMakeVisible (groupLabel);

    groupDots.onChange = [this] (int group) { proc.setGroup (group); };
    content.addAndMakeVisible (groupDots);

    meterTypePills.setSelectedIndex (getChoiceIndex (proc.apvts, params::meterType));
    monitorPills.setSelectedIndex (getChoiceIndex (proc.apvts, params::monitorMode));
    groupDots.setSelectedGroup (proc.getGroup());

    content.addAndMakeVisible (meter);
    meter.onClear = [this] { proc.clearMeterClip(); };
    meter.onCalibrate = [this] (float refDb)
    {
        if (params::meterTypeFixedRef (proc.getMeterType())) return; // K-scales fixed
        if (auto* cal = proc.apvts.getParameter (params::vuCal))
            cal->setValueNotifyingHost (proc.apvts.getParameterRange (params::vuCal).convertTo0to1 (refDb));
    };


    // ---- Channel name (editable) ----
    nameLabel.setText (proc.getInstanceName(), juce::dontSendNotification);
    nameLabel.setEditable (true);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::backgroundColourId, juce::Colour (ui::Theme::panel));
    nameLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    nameLabel.onTextChange = [this] { proc.setInstanceName (nameLabel.getText()); };
    content.addAndMakeVisible (nameLabel);

    // ---- Channel View toggle (control-surface overview) ----
    chViewBtn.setClickingTogglesState (true);
    chViewBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::panel));
    chViewBtn.onClick = [this] { setChannelViewMode (chViewBtn.getToggleState()); };
    content.addAndMakeVisible (chViewBtn);

    content.addChildComponent (channelView); // hidden until channel-view mode

    // ---- Live registry updates ----
    proc.onRegistryChangedCallback = [this] { refreshLinkPanel(); };
    refreshLinkPanel(); // reflect any existing bucket names right away

    setResizable (false, false);
    setSize (360, 520);
    updateWindowSize();

    startTimerHz (30);
}

StrataEditor::~StrataEditor()
{
    proc.onRegistryChangedCallback = nullptr;
    setLookAndFeel (nullptr);
}

void StrataEditor::timerCallback()
{
    meter.setScale (params::meterTypeLabel (proc.getMeterType()),
                    proc.getVuReferenceDb(), proc.getMeterOffsetDb());

    auto& s = proc.getOutputMeters(); // the meter shows what leaves the plugin
    meter.setValues (s.rmsDb.load(), s.peakDb.load(), s.peakHold.load(), s.clip.load());

    meterTypePills.setSelectedIndex (getChoiceIndex (proc.apvts, params::meterType));
    monitorPills.setSelectedIndex (getChoiceIndex (proc.apvts, params::monitorMode));
    groupDots.setSelectedGroup (proc.getGroup());
}

void StrataEditor::refreshLinkPanel()
{
    nameLabel.setText (proc.getInstanceName(), juce::dontSendNotification);
    groupDots.setSelectedGroup (proc.getGroup());
}

void StrataEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (ui::Theme::bg));

    const int baseW = channelViewMode ? 760 : 360;
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::scale (uiScale));

    auto top = juce::Rectangle<float> (0.0f, 0.0f, (float) baseW, 30.0f);
    const juce::Font titleFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.setFont (titleFont);
    g.setColour (juce::Colour (ui::Theme::accent));
    const auto titleArea = top.withTrimmedLeft (10.0f).withWidth ((float) titleFont.getStringWidth ("STRATA") + 2.0f);
    g.drawText ("STRATA", titleArea, juce::Justification::centredLeft);

    if (channelViewMode)
    {
        const float badgeX = titleArea.getRight() + 8.0f;
        const auto badge = juce::Rectangle<float> (badgeX, top.getCentreY() - 9.0f, 84.0f, 18.0f);
        g.setColour (juce::Colour (ui::Theme::accent));
        g.drawRoundedRectangle (badge, 3.0f, 1.5f);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.drawText ("CH VIEW", badge.toNearestInt(), juce::Justification::centred, true);
    }

    if (proc.getGroup() != 0)
    {
        const auto col = strata::link::InstanceRegistry::getInstance().getBucketColour (proc.getGroup());
        g.setColour (juce::Colour (col));
        g.fillEllipse (top.removeFromRight (28.0f).withSizeKeepingCentre (10.0f, 10.0f));
    }
}

void StrataEditor::setChannelViewMode (bool on)
{
    channelViewMode = on;
    chViewBtn.setToggleState (on, juce::dontSendNotification);
    chViewBtn.setButtonText (on ? "STRIP VIEW" : "CHANNEL VIEW");

    const bool strip = ! on;
    nameLabel.setVisible (strip); inGain.setVisible (strip);
    bypassBtn.setVisible (strip); muteBtn.setVisible (strip);
    monoBtn.setVisible (strip);   phaseBtn.setVisible (strip);
    meterTypePills.setVisible (strip); monitorPills.setVisible (strip);
    groupLabel.setVisible (strip); groupDots.setVisible (strip);
    meter.setVisible (strip);
    channelView.setVisible (on);

    updateWindowSize();
    resized();
}

void StrataEditor::setUiScale (float scale)
{
    uiScale = juce::jlimit (0.75f, 2.0f, scale);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    updateWindowSize();
}

void StrataEditor::updateWindowSize()
{
    const int baseW = channelViewMode ? 760 : 360;
    const int baseH = channelViewMode ? 420 : 520;
    setSize (juce::roundToInt ((float) baseW * uiScale),
             juce::roundToInt ((float) baseH * uiScale));
}

void StrataEditor::resized()
{
    content.setBounds (0, 0, channelViewMode ? 760 : 360, channelViewMode ? 420 : 520);
    auto r = content.getLocalBounds();
    auto header = r.removeFromTop (30);
    guiScaleBox.setBounds (header.removeFromRight (58).reduced (0, 4));
    header.removeFromRight (6);
    header.removeFromRight (28);                 // reserved for the group-link dot
    chViewBtn.setBounds (header.removeFromRight (130).reduced (4));

    if (channelViewMode)
        channelView.setBounds (r.reduced (8));
    else
        layoutStrip (r.reduced (10));
}

void StrataEditor::layoutStrip (juce::Rectangle<int> r)
{
    nameLabel.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);

    // single meter (plugin output)
    meter.setBounds (r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.42f)).reduced (3));

    r.removeFromTop (8);
    meterTypePills.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);
    monitorPills.setBounds (r.removeFromTop (24));
    r.removeFromTop (8);

    // single INPUT trim knob (centred)
    auto knobs = r.removeFromTop (110);
    inGain.setBounds (knobs.withSizeKeepingCentre (juce::jmin (knobs.getWidth(), 120), knobs.getHeight()).reduced (4));

    r.removeFromTop (8);
    auto btns = r.removeFromTop (28); // BYPASS | MUTE | MONO | Ø
    const int bw = btns.getWidth() / 4;
    bypassBtn.setBounds (btns.removeFromLeft (bw).reduced (2));
    muteBtn  .setBounds (btns.removeFromLeft (bw).reduced (2));
    monoBtn  .setBounds (btns.removeFromLeft (bw).reduced (2));
    phaseBtn .setBounds (btns.reduced (2));

    r.removeFromTop (8);
    auto groupRow = r.removeFromTop (22);
    groupLabel.setBounds (groupRow.removeFromLeft (46).reduced (2, 0));
    groupDots.setBounds (groupRow.reduced (0, 2));
}
