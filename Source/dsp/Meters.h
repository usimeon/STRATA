#pragma once
#include <atomic>
#include <cmath>
#include <algorithm>

/*  All meter ballistics are computed on the AUDIO thread and published into
    std::atomic<float> so the UI timer can read them lock-free, allocation-free.

    No locks, no allocation, no denormals issues (we clamp tiny values).  */
namespace strata::dsp
{
    // ---------------------------------------------------------------------
    //  Peak meter — instantaneous sample peak with fast attack / slow release
    //  plus a separate, latched "true peak hold" (digital peak, NOT ITU TP).
    // ---------------------------------------------------------------------
    class PeakDetector
    {
    public:
        void prepare (double sampleRate, int blockSize) noexcept
        {
            sr = sampleRate;
            // release ~ 11.8 dB/s typical PPM-ish fall back
            releaseCoeff = std::exp (-1.0f / (float) (0.300 * sr)); // 300 ms time const
            reset();
            (void) blockSize;
        }

        void reset() noexcept { env = 0.0f; holdPeak = 0.0f; }

        // process one channel block; call per channel then publish max.
        void processBlock (const float* x, int n) noexcept
        {
            float e = env;
            float h = holdPeak;
            for (int i = 0; i < n; ++i)
            {
                const float a = std::abs (x[i]);
                if (a > e) e = a;                 // instant attack
                else       e *= releaseCoeff;     // exponential release
                if (a > h) h = a;                 // peak hold (clears on UI read)
            }
            env = e;
            holdPeak = h;
        }

        float getEnvelope() const noexcept { return env; }
        float getAndClearHold() noexcept { float h = holdPeak; holdPeak = 0.0f; return h; }

    private:
        double sr = 48000.0;
        float releaseCoeff = 0.0f;
        float env = 0.0f, holdPeak = 0.0f;
    };

    // ---------------------------------------------------------------------
    //  VU meter — RMS-based, classic 300 ms integration ballistics
    //  (ANSI C16.5 / IEC 60268-17 style: ~300 ms to 99%, ~1-1.5% overshoot).
    //  We model it as a single-pole averager on the squared signal.
    // ---------------------------------------------------------------------
    class VuDetector
    {
    public:
        void prepare (double sampleRate) noexcept
        {
            sr = sampleRate;
            // 300 ms rise/fall time constant for the mean-square smoother.
            const float tc = 0.300f;
            coeff = std::exp (-1.0f / (float) (tc * sr));
            reset();
        }

        void reset() noexcept { meanSq = 0.0f; }

        void processBlock (const float* x, int n) noexcept
        {
            float m = meanSq;
            const float oneMinus = 1.0f - coeff;
            for (int i = 0; i < n; ++i)
                m = coeff * m + oneMinus * (x[i] * x[i]);
            meanSq = m;
        }

        // RMS in linear (0..~1+). Caller converts to dBFS then to VU via reference.
        float getRms() const noexcept { return std::sqrt (std::max (meanSq, 1.0e-12f)); }

    private:
        double sr = 48000.0;
        float coeff = 0.0f, meanSq = 0.0f;
    };

    // ---------------------------------------------------------------------
    //  Lock-free publish target read by the editor timer.
    // ---------------------------------------------------------------------
    struct MeterSnapshot
    {
        std::atomic<float> peakDb   { -100.0f };
        std::atomic<float> peakHold { -100.0f };
        std::atomic<float> rmsDb    { -100.0f }; // full-scale RMS dBFS (UI maps to VU)
        std::atomic<bool>  clip     { false };

        std::atomic<int> clipHoldCtr { 0 }; // samples remaining before clip auto-clears

        void publish (float peakLin, float rmsLin, int numSamples, int clipHoldSamples) noexcept
        {
            const float pdb = toDb (peakLin);
            peakDb.store (pdb,            std::memory_order_relaxed);
            rmsDb.store  (toDb (rmsLin),  std::memory_order_relaxed);
            if (pdb > peakHold.load (std::memory_order_relaxed))
                peakHold.store (pdb, std::memory_order_relaxed);

            // Clip lights instantly, holds, then auto-clears if no new clip.
            if (peakLin >= 1.0f)
            {
                clip.store (true, std::memory_order_relaxed);
                clipHoldCtr.store (clipHoldSamples, std::memory_order_relaxed);
            }
            else
            {
                int c = clipHoldCtr.load (std::memory_order_relaxed);
                if (c > 0)
                {
                    c -= numSamples;
                    clipHoldCtr.store (c, std::memory_order_relaxed);
                    if (c <= 0) clip.store (false, std::memory_order_relaxed);
                }
            }
        }

        // Reset indicators (message thread, lock-free) — manual click-to-clear.
        void clear() noexcept
        {
            clip.store (false, std::memory_order_relaxed);
            clipHoldCtr.store (0, std::memory_order_relaxed);
            peakHold.store (-100.0f, std::memory_order_relaxed);
        }

        static float toDb (float lin) noexcept
        {
            return lin > 1.0e-6f ? 20.0f * std::log10 (lin) : -100.0f;
        }
    };
}
