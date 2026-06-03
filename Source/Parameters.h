#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/*  Central definition of every host-visible parameter.
    Keep IDs stable forever — they are the contract for session recall & automation. */
namespace strata::params
{
    // ---- Parameter IDs (string keys, version-stable) -----------------------
    static constexpr auto inputGain   = "inputGain";    // dB
    static constexpr auto outputGain  = "outputGain";   // dB
    static constexpr auto bypass      = "bypass";       // bool
    static constexpr auto monoSum     = "monoSum";      // bool (utility, no latency)
    static constexpr auto phaseInvert = "phaseInvert";  // bool (polarity)
    static constexpr auto mute        = "mute";         // bool
    static constexpr auto vuCal       = "vuCal";        // float dBFS = 0 VU (drag-calibrate)
    static constexpr auto meterType   = "meterType";    // choice: VU/RMS/RMS+3/K-12/K-14/K-20
    static constexpr auto monitorMode = "monitorMode";  // choice: Stereo/L/R/Mono/Side

    // ---- Non-automatable / state-only (handled outside APVTS) ---------------
    // instanceName, uuid, groupId, linkId, orderIndex  -> stored in plugin state XML.

    // ---- Meter types --------------------------------------------------------
    enum MeterType { kVU = 0, kRMS, kRMSp3, kK12, kK14, kK20 };
    static const juce::StringArray meterTypeChoices { "VU", "RMS", "RMS+3", "K-12", "K-14", "K-20" };

    inline juce::String meterTypeLabel  (int t) { return meterTypeChoices[juce::jlimit (0, 5, t)]; }
    inline bool         meterTypeFixedRef (int t) { return t >= kK12; }            // K-scales fixed
    inline float        meterTypeFixedRefDb (int t) { return t == kK12 ? -12.0f : t == kK14 ? -14.0f : -20.0f; }
    inline float        meterTypeOffsetDb (int t) { return t == kRMSp3 ? 3.0f : 0.0f; }

    // ---- Monitor / output routing ------------------------------------------
    enum MonitorMode { kStereo = 0, kLeft, kRight, kMono, kSide };
    static const juce::StringArray monitorChoices { "Stereo", "Left", "Right", "Mono", "Side" };

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        auto gainRange = NormalisableRange<float> (-24.0f, 24.0f, 0.01f);
        gainRange.setSkewForCentre (0.0f);

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { inputGain, 1 }, "Input Gain", gainRange, 0.0f,
            AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { outputGain, 1 }, "Output Gain", gainRange, 0.0f,
            AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { bypass, 1 }, "Bypass", false));

        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { mute, 1 }, "Mute", false));

        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { monoSum, 1 }, "Mono Sum", false));

        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { phaseInvert, 1 }, "Polarity", false));

        // 0 VU calibration point in dBFS — drag the meter to set it.
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { vuCal, 1 }, "VU Calibration",
            NormalisableRange<float> (-30.0f, -6.0f, 0.5f), -18.0f,
            AudioParameterFloatAttributes().withLabel ("dBFS")));

        layout.add (std::make_unique<AudioParameterChoice> (
            ParameterID { meterType, 1 }, "Meter Type", meterTypeChoices, kVU));

        layout.add (std::make_unique<AudioParameterChoice> (
            ParameterID { monitorMode, 1 }, "Monitor", monitorChoices, kStereo));

        return layout;
    }
}
