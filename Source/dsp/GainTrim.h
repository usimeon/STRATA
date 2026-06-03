#pragma once
#include <juce_dsp/juce_dsp.h>

/*  Sample-accurate, click-free gain with smoothed ramp.
    Zero latency. No allocation in process(). */
namespace strata::dsp
{
    class GainTrim
    {
    public:
        void prepare (double sampleRate, int /*blockSize*/, int numChannels) noexcept
        {
            sr = sampleRate;
            gain.reset (sampleRate, 0.02); // 20 ms smoothing
            numCh = numChannels;
        }

        void setGainDb (float db) noexcept
        {
            gain.setTargetValue (juce::Decibels::decibelsToGain (db, -100.0f));
        }

        void setPhaseInvert (bool inv) noexcept { sign = inv ? -1.0f : 1.0f; }

        // buffer: interleaved-by-channel JUCE AudioBuffer view
        void process (float* const* channels, int numChannels, int numSamples) noexcept
        {
            for (int n = 0; n < numSamples; ++n)
            {
                const float g = gain.getNextValue() * sign;
                for (int ch = 0; ch < numChannels; ++ch)
                    channels[ch][n] *= g;
            }
        }

    private:
        double sr = 48000.0;
        int numCh = 2;
        float sign = 1.0f;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain { 1.0f };
    };
}
