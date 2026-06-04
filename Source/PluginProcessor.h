#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/GainTrim.h"
#include "dsp/Meters.h"
#include "link/InstanceRegistry.h"

/*  STRATA — live-safe gain-staging / utility channel strip.
    Zero added latency. Realtime-safe audio thread. */
class StrataProcessor : public juce::AudioProcessor,
                        public strata::link::InstanceClient,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    StrataProcessor();
    ~StrataProcessor() override;

    //==== AudioProcessor =====================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "STRATA"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms() override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int sizeInBytes) override;

    // Host pushes the track name/colour here; we use it as the default label.
    void updateTrackProperties (const TrackProperties& props) override;

    //==== InstanceClient (message thread only) ===============================
    juce::Uuid   getUuid()        const override { return uuid; }
    juce::String getDisplayName() const override { return instanceName; }
    int          getGroupId()     const override { return groupId.load(); }
    void         setGroupId (int g) override     { setGroup (g); }
    void  setRemoteParameter (const juce::String& paramId, float value) override;
    float getParamValue   (const juce::String& paramId) const override;
    float getOutputPeakDb () const override { return outMeters.peakDb.load(); }
    float getOutputRmsDb  () const override { return outMeters.rmsDb.load(); }
    int   getMeterType    () const override;
    float getMeterOffsetDb() const override;
    bool  getOutputClip   () const override { return outMeters.clip.load(); }
    void  clearMeterClip  () override       { outMeters.clear(); }
    bool  isBypassed      () const override { return pBypass != nullptr && pBypass->load() >= 0.5f; }
    int   getLinkId       () const override { return linkId.load(); }
    void  setLinkId       (int id) override;                         // mirror set (0 == off)
    void  applyMirror     (const juce::String& paramId, float value) override;
    int   getOrderIndex   () const override { return orderIndex.load(); }
    void  setOrderIndex   (int idx) override { orderIndex.store (idx); } // bucket sort key
    void onRegistryChanged() override;

    //==== Public API used by the editor ======================================
    juce::AudioProcessorValueTreeState apvts;
    const strata::dsp::MeterSnapshot& getOutputMeters() const { return outMeters; }

    juce::String getInstanceName() const            { return instanceName; }
    void         setInstanceName (juce::String n);
    int          getGroup() const                   { return groupId.load(); }
    void         setGroup (int g);
    float        getVuReferenceDb() const override;

    std::function<void()> onRegistryChangedCallback; // editor subscribes

    // Access to the process-wide directory for the editor's Channel View.
    strata::link::InstanceClient* asClient() { return this; }

private:
    //==== APVTS listener — drives mirror linking =============================
    void parameterChanged (const juce::String& paramId, float newValue) override;
    void scheduleMirror   (const juce::String& paramId, float value);

    //==== Identity / bucket state ============================================
    juce::Uuid       uuid;                 // persisted (re-minted on restore)
    juce::String     instanceName { "STRATA" };
    bool             customName = false;   // true once the user types a name
    std::atomic<int> groupId { 0 };        // 0 == no bucket
    std::atomic<int> linkId  { 0 };        // mirror set; same id+bucket -> ganged
    std::atomic<int> orderIndex { 0 };     // display order within the bucket
    std::atomic<bool> applyingMirror { false }; // re-entrancy guard for mirroring

    //==== DSP ================================================================
    strata::dsp::GainTrim   inputTrim, outputTrim;
    strata::dsp::PeakDetector outPeakL, outPeakR;
    strata::dsp::VuDetector   outVu;
    strata::dsp::MeterSnapshot outMeters;

    // cached atomic param pointers (set in ctor, read on audio thread)
    std::atomic<float>* pInGain   = nullptr;
    std::atomic<float>* pOutGain  = nullptr;
    std::atomic<float>* pBypass   = nullptr;
    std::atomic<float>* pMonoSum  = nullptr;
    std::atomic<float>* pPhase    = nullptr;
    std::atomic<float>* pMute     = nullptr;
    std::atomic<float>* pMonitor  = nullptr;

    double currentSampleRate = 48000.0;
    int    clipHoldSamples = 120000; // ~2.5 s; set in prepareToPlay

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrataProcessor)
};
