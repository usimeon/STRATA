#include "PluginEditor.h"

using namespace strata;

StrataEditor::StrataEditor (StrataProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    auto setupKnob = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        s.setTextValueSuffix (" dB");
        s.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (s);
    };
    setupKnob (inGain);
    inGainAtt = std::make_unique<SliderAttach> (proc.apvts, params::inputGain, inGain);

    for (auto* b : { &bypassBtn, &muteBtn, &monoBtn, &phaseBtn })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::accent));
        addAndMakeVisible (*b);
    }
    bypassBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::warn));
    muteBtn  .setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::clip));
    bypassAtt = std::make_unique<ButtonAttach> (proc.apvts, params::bypass,      bypassBtn);
    muteAtt   = std::make_unique<ButtonAttach> (proc.apvts, params::mute,        muteBtn);
    monoAtt   = std::make_unique<ButtonAttach> (proc.apvts, params::monoSum,     monoBtn);
    phaseAtt  = std::make_unique<ButtonAttach> (proc.apvts, params::phaseInvert, phaseBtn);

    meterTypeBox.addItemList (params::meterTypeChoices, 1);
    addAndMakeVisible (meterTypeBox);
    meterTypeAtt = std::make_unique<ComboAttach> (proc.apvts, params::meterType, meterTypeBox);

    monitorBox.addItemList (params::monitorChoices, 1);
    addAndMakeVisible (monitorBox);
    monitorAtt = std::make_unique<ComboAttach> (proc.apvts, params::monitorMode, monitorBox);

    addAndMakeVisible (meter);
    meter.onClear = [this] { proc.clearMeterClip(); };
    meter.onCalibrate = [this] (float refDb)
    {
        if (params::meterTypeFixedRef (proc.getMeterType())) return; // K-scales fixed
        if (auto* cal = proc.apvts.getParameter (params::vuCal))
            cal->setValueNotifyingHost (proc.apvts.getParameterRange (params::vuCal).convertTo0to1 (refDb));
    };

    // IN / GR / OUT meter-source buttons (radio).
    for (auto* b : { &inBtn, &grBtn, &outBtn })
    {
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::accent));
        addAndMakeVisible (*b);
    }
    inBtn .onClick = [this] { setMeterSource (0); };
    grBtn .onClick = [this] { setMeterSource (1); };
    outBtn.onClick = [this] { setMeterSource (2); };
    grBtn.setEnabled (false); // enabled once compression exists
    setMeterSource (0);

    // ---- Channel name (editable) ----
    nameLabel.setText (proc.getInstanceName(), juce::dontSendNotification);
    nameLabel.setEditable (true);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::backgroundColourId, juce::Colour (ui::Theme::panel));
    nameLabel.onTextChange = [this] { proc.setInstanceName (nameLabel.getText()); };
    addAndMakeVisible (nameLabel);

    // ---- Group selector ----
    groupBox.addItem ("No Group", 1);
    for (int i = 1; i <= 8; ++i) groupBox.addItem ("Group " + juce::String (i), i + 1);
    groupBox.setSelectedId (proc.getGroup() + 1, juce::dontSendNotification);
    groupBox.onChange = [this] { proc.setGroup (groupBox.getSelectedId() - 1); };
    addAndMakeVisible (groupBox);

    // ---- Channel View toggle (control-surface overview) ----
    chViewBtn.setClickingTogglesState (true);
    chViewBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (ui::Theme::link));
    chViewBtn.onClick = [this] { setChannelViewMode (chViewBtn.getToggleState()); };
    addAndMakeVisible (chViewBtn);

    addChildComponent (channelView); // hidden until channel-view mode

    // ---- Live registry updates ----
    proc.onRegistryChangedCallback = [this] { refreshLinkPanel(); };
    refreshLinkPanel(); // reflect any existing bucket names right away

    setResizable (true, true);
    setResizeLimits (320, 420, 1100, 900);
    setSize (360, 520);

    startTimerHz (30);
}

StrataEditor::~StrataEditor()
{
    proc.onRegistryChangedCallback = nullptr;
    setLookAndFeel (nullptr);
}

void StrataEditor::timerCallback()
{
    const float ref = proc.getVuReferenceDb();
    const float off = proc.getMeterOffsetDb();
    const auto  lbl = meterSource == 1 ? juce::String ("GR")
                                       : params::meterTypeLabel (proc.getMeterType());
    meter.setScale (lbl, ref, off);

    if (meterSource == 1) // GR — no compressor yet, show no reduction
    {
        meter.setValues (-100.0f, -100.0f, -100.0f, false);
        return;
    }

    auto& s = (meterSource == 2) ? proc.getOutputMeters() : proc.getInputMeters();
    meter.setValues (s.rmsDb.load(), s.peakDb.load(), s.peakHold.load(), s.clip.load());
}

void StrataEditor::setMeterSource (int s)
{
    meterSource = s;
    inBtn .setToggleState (s == 0, juce::dontSendNotification);
    grBtn .setToggleState (s == 1, juce::dontSendNotification);
    outBtn.setToggleState (s == 2, juce::dontSendNotification);
}

void StrataEditor::refreshLinkPanel()
{
    nameLabel.setText (proc.getInstanceName(), juce::dontSendNotification);

    // Reflect custom bucket names in the group selector (id = group + 1).
    auto& reg = strata::link::InstanceRegistry::getInstance();
    for (int g = 1; g <= 8; ++g)
        groupBox.changeItemText (g + 1, reg.getBucketName (g));

    groupBox.setSelectedId (proc.getGroup() + 1, juce::dontSendNotification);
}

void StrataEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (ui::Theme::bg));

    auto top = getLocalBounds().removeFromTop (30).toFloat();
    g.setColour (juce::Colour (ui::Theme::accent));
    g.setFont (juce::Font (juce::FontOptions (16.0f).withStyle ("Bold")));
    g.drawText ("STRATA", top.reduced (10, 0), juce::Justification::centredLeft);

    if (proc.getGroup() != 0)
    {
        const auto col = strata::link::InstanceRegistry::getInstance().getBucketColour (proc.getGroup());
        g.setColour (juce::Colour (col));
        g.fillEllipse (top.removeFromRight (28).withSizeKeepingCentre (10, 10));
    }
}

void StrataEditor::setChannelViewMode (bool on)
{
    channelViewMode = on;
    chViewBtn.setToggleState (on, juce::dontSendNotification);

    const bool strip = ! on;
    nameLabel.setVisible (strip); inGain.setVisible (strip);
    bypassBtn.setVisible (strip); muteBtn.setVisible (strip);
    monoBtn.setVisible (strip);   phaseBtn.setVisible (strip);
    meterTypeBox.setVisible (strip); monitorBox.setVisible (strip); groupBox.setVisible (strip);
    meter.setVisible (strip);
    inBtn.setVisible (strip); grBtn.setVisible (strip); outBtn.setVisible (strip);
    channelView.setVisible (on);

    if (on && getWidth() < 640)
        setSize (760, juce::jmax (getHeight(), 420));

    resized();
}

void StrataEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (30);
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

    // single meter
    meter.setBounds (r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.40f)).reduced (3));

    // IN / GR / OUT source buttons
    r.removeFromTop (6);
    auto srcRow = r.removeFromTop (22);
    const int sw = srcRow.getWidth() / 3;
    inBtn .setBounds (srcRow.removeFromLeft (sw).reduced (2, 0));
    grBtn .setBounds (srcRow.removeFromLeft (sw).reduced (2, 0));
    outBtn.setBounds (srcRow.reduced (2, 0));

    r.removeFromTop (6);
    auto meterRow = r.removeFromTop (24); // meter type + monitor selectors
    meterTypeBox.setBounds (meterRow.removeFromLeft (meterRow.getWidth() / 2).reduced (2, 0));
    monitorBox  .setBounds (meterRow.reduced (2, 0));
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
    groupBox.setBounds (r.removeFromTop (26));
}
