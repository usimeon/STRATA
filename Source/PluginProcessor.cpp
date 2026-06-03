#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace strata;

//============================================================================
StrataProcessor::StrataProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", params::createLayout())
{
    pInGain  = apvts.getRawParameterValue (params::inputGain);
    pOutGain = apvts.getRawParameterValue (params::outputGain);
    pBypass  = apvts.getRawParameterValue (params::bypass);
    pMonoSum = apvts.getRawParameterValue (params::monoSum);
    pPhase   = apvts.getRawParameterValue (params::phaseInvert);
    pMute    = apvts.getRawParameterValue (params::mute);
    pMonitor = apvts.getRawParameterValue (params::monitorMode);

    // Listen to the mirror-able params so linked instances can stay in sync.
    for (auto id : { params::inputGain, params::outputGain, params::bypass, params::mute })
        apvts.addParameterListener (id, this);

    // Register in the process-wide directory (we are on the message thread here).
    link::InstanceRegistry::getInstance().add (this);
}

StrataProcessor::~StrataProcessor()
{
    for (auto id : { params::inputGain, params::outputGain, params::bypass, params::mute })
        apvts.removeParameterListener (id, this);
    link::InstanceRegistry::getInstance().remove (this);
}

//============================================================================
bool StrataProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out; // in == out
}

void StrataProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int ch = getTotalNumOutputChannels();

    inputTrim .prepare (sampleRate, samplesPerBlock, ch);
    outputTrim.prepare (sampleRate, samplesPerBlock, ch);

    for (auto* p : { &inPeakL, &inPeakR, &outPeakL, &outPeakR })
        p->prepare (sampleRate, samplesPerBlock);
    inVu.prepare (sampleRate);
    outVu.prepare (sampleRate);
}

//============================================================================
void StrataProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();
    auto* const* ch = buffer.getArrayOfWritePointers();

    // ---- Bypass = clean passthrough, still meters raw ------------------
    if (pBypass->load() >= 0.5f)
    {
        if (numCh > 0) inPeakL.processBlock (ch[0], n);
        if (numCh > 1) inPeakR.processBlock (ch[1], n);
        inVu.processBlock (ch[0], n);
        const float pk = juce::jmax (inPeakL.getEnvelope(),
                                     numCh > 1 ? inPeakR.getEnvelope() : 0.0f);
        inMeters.publish (pk, inVu.getRms());
        outMeters.publish (pk, inVu.getRms());
        return;
    }

    // ---- Input trim -----------------------------------------------------
    inputTrim.setGainDb (pInGain->load());
    inputTrim.setPhaseInvert (pPhase->load() >= 0.5f);
    inputTrim.process (ch, numCh, n);

    // ---- IN metering (POST input gain, pre everything else) ------------
    if (numCh > 0) inPeakL.processBlock (ch[0], n);
    if (numCh > 1) inPeakR.processBlock (ch[1], n);
    inVu.processBlock (ch[0], n);
    {
        const float pk = juce::jmax (inPeakL.getEnvelope(),
                                     numCh > 1 ? inPeakR.getEnvelope() : 0.0f);
        inMeters.publish (pk, inVu.getRms());
    }

    // ---- Mono sum (utility, zero latency) ------------------------------
    if (pMonoSum->load() >= 0.5f && numCh > 1)
    {
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (ch[0][i] + ch[1][i]);
            ch[0][i] = m; ch[1][i] = m;
        }
    }

    // ---- Output trim ----------------------------------------------------
    outputTrim.setGainDb (pOutGain->load());
    outputTrim.process (ch, numCh, n);

    // ---- Monitor routing (utility, zero latency) -----------------------
    const int mon = (int) (pMonitor->load() + 0.5f);
    if (numCh > 1 && mon != params::kStereo)
    {
        for (int i = 0; i < n; ++i)
        {
            const float L = ch[0][i], R = ch[1][i];
            float a = L, b = R;
            switch (mon)
            {
                case params::kLeft:  a = b = L;               break;
                case params::kRight: a = b = R;               break;
                case params::kMono:  a = b = 0.5f * (L + R);  break;
                case params::kSide:  a = b = 0.5f * (L - R);  break;
                default: break;
            }
            ch[0][i] = a; ch[1][i] = b;
        }
    }

    // ---- Mute (after output, still meters the silence) -----------------
    if (pMute->load() >= 0.5f)
        buffer.clear();

    // ---- Output metering (post-gain) -----------------------------------
    if (numCh > 0) outPeakL.processBlock (ch[0], n);
    if (numCh > 1) outPeakR.processBlock (ch[1], n);
    outVu.processBlock (ch[0], n);
    {
        const float pk = juce::jmax (outPeakL.getEnvelope(),
                                     numCh > 1 ? outPeakR.getEnvelope() : 0.0f);
        outMeters.publish (pk, outVu.getRms());
    }
}

//============================================================================
//  Remote control — all on the MESSAGE THREAD. Gains are NOT ganged: this only
//  sets THIS instance's own parameter when a controlling instance asks.
//============================================================================
void StrataProcessor::setRemoteParameter (const juce::String& paramId, float value)
{
    jassert (juce::MessageManager::existsAndIsCurrentThread());

    auto* p = apvts.getParameter (paramId);
    if (p == nullptr) return;

    const float norm = apvts.getParameterRange (paramId).convertTo0to1 (value);
    if (std::abs (p->getValue() - norm) < 1.0e-6f)
        return; // no-op -> nothing to notify

    p->setValueNotifyingHost (norm); // keeps host automation / generic editor in sync
}

float StrataProcessor::getParamValue (const juce::String& paramId) const
{
    if (auto* raw = apvts.getRawParameterValue (paramId))
        return raw->load();
    return 0.0f;
}

//----------------------------------------------------------------------------
//  Mirror linking. parameterChanged fires for ANY source (own UI, channel-view
//  remote set, host automation). If we are linked and the change did NOT come
//  from a mirror application, propagate it to our link partners.
//----------------------------------------------------------------------------
void StrataProcessor::parameterChanged (const juce::String& paramId, float newValue)
{
    if (applyingMirror.load()) return;   // came from a partner -> don't echo back
    if (linkId.load() == 0)     return;   // not linked
    scheduleMirror (paramId, newValue);
}

void StrataProcessor::scheduleMirror (const juce::String& paramId, float value)
{
    // parameterChanged can arrive on the audio thread (automation); the registry
    // is message-thread only, so marshal there.
    auto* self = this;
    auto fn = [self, paramId, value]
    {
        link::InstanceRegistry::getInstance().broadcastToLinked (self, paramId, value);
    };
    if (juce::MessageManager::existsAndIsCurrentThread()) fn();
    else juce::MessageManager::callAsync (std::move (fn));
}

void StrataProcessor::applyMirror (const juce::String& paramId, float value)
{
    jassert (juce::MessageManager::existsAndIsCurrentThread());
    auto* p = apvts.getParameter (paramId);
    if (p == nullptr) return;

    const float norm = apvts.getParameterRange (paramId).convertTo0to1 (value);
    if (std::abs (p->getValue() - norm) < 1.0e-6f) return; // already there

    applyingMirror.store (true);          // suppress our own re-broadcast
    p->setValueNotifyingHost (norm);      // synchronous -> parameterChanged sees the guard
    applyingMirror.store (false);
}

void StrataProcessor::setLinkId (int id)
{
    linkId.store (id);
    link::InstanceRegistry::getInstance().notifyMetadataChanged();
    if (onRegistryChangedCallback)
        juce::MessageManager::callAsync ([cb = onRegistryChangedCallback] { cb(); });
}

void StrataProcessor::onRegistryChanged()
{
    if (onRegistryChangedCallback)
        juce::MessageManager::callAsync ([cb = onRegistryChangedCallback] { cb(); });
}

//============================================================================
void StrataProcessor::setInstanceName (juce::String n)
{
    instanceName = std::move (n);
    customName = instanceName.isNotEmpty();
    link::InstanceRegistry::getInstance().notifyMetadataChanged();
}

void StrataProcessor::updateTrackProperties (const TrackProperties& props)
{
    // Adopt the host track name as our label unless the user set a custom one.
    if (! customName && props.name.isNotEmpty() && props.name != instanceName)
    {
        instanceName = props.name;
        link::InstanceRegistry::getInstance().notifyMetadataChanged();
        if (onRegistryChangedCallback)
            juce::MessageManager::callAsync ([cb = onRegistryChangedCallback] { cb(); });
    }
}

void StrataProcessor::setGroup (int g)
{
    groupId.store (g);
    link::InstanceRegistry::getInstance().notifyMetadataChanged();
}

int StrataProcessor::getMeterType() const
{
    if (auto* raw = apvts.getRawParameterValue (params::meterType))
        return (int) (raw->load() + 0.5f);
    return params::kVU;
}

float StrataProcessor::getMeterOffsetDb() const
{
    return params::meterTypeOffsetDb (getMeterType());
}

float StrataProcessor::getVuReferenceDb() const
{
    const int t = getMeterType();
    if (params::meterTypeFixedRef (t))       // K-scales fix the 0 VU point
        return params::meterTypeFixedRefDb (t);
    if (auto* raw = apvts.getRawParameterValue (params::vuCal))
        return raw->load();                  // user calibration (drag the meter)
    return -24.0f;
}

//============================================================================
//  State — store APVTS + identity/linking metadata.
//============================================================================
void StrataProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto meta  = state.getOrCreateChildWithName ("strataMeta", nullptr);
    meta.setProperty ("uuid",   uuid.toString(), nullptr);
    meta.setProperty ("name",   instanceName,    nullptr);
    meta.setProperty ("custom", customName,      nullptr);
    meta.setProperty ("group",  groupId.load(),  nullptr);
    meta.setProperty ("link",   linkId.load(),   nullptr);
    meta.setProperty ("order",  orderIndex.load(), nullptr);

    // Carry the bucket's shared identity so it survives session recall.
    auto& reg = link::InstanceRegistry::getInstance();
    meta.setProperty ("bucketName",   reg.getBucketName (groupId.load()), nullptr);
    meta.setProperty ("bucketColour", (juce::int64) reg.getBucketColour (groupId.load()), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void StrataProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid()) return;

    apvts.replaceState (state);

    auto meta = state.getChildWithName ("strataMeta");
    if (meta.isValid())
    {
        instanceName = meta.getProperty ("name", "STRATA").toString();
        customName   = (bool) meta.getProperty ("custom", false);
        groupId.store ((int) meta.getProperty ("group", 0));
        linkId.store  ((int) meta.getProperty ("link", 0));
        orderIndex.store ((int) meta.getProperty ("order", 0));

        // IMPORTANT: when a host DUPLICATES a track it copies this state, which
        // would clone the UUID. Re-mint a fresh UUID per live instance so two
        // instances never share identity. Grouping survives via groupId.
        uuid = juce::Uuid();

        // Restore the bucket's shared identity (only if a real override exists).
        auto& reg = link::InstanceRegistry::getInstance();
        const int g = groupId.load();
        const auto bn = meta.getProperty ("bucketName", "").toString();
        if (g != 0 && bn.isNotEmpty() && bn != ("Group " + juce::String (g)))
            reg.setBucketName (g, bn);
        const auto bc = (juce::uint32) (juce::int64) meta.getProperty ("bucketColour", (juce::int64) 0);
        if (g != 0 && bc != 0 && bc != link::InstanceRegistry::defaultBucketColour (g))
            reg.setBucketColour (g, bc);
    }

    link::InstanceRegistry::getInstance().notifyMetadataChanged();
    if (onRegistryChangedCallback) onRegistryChangedCallback();
}

//============================================================================
juce::AudioProcessorEditor* StrataProcessor::createEditor()
{
    return new StrataEditor (*this);
}

//============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StrataProcessor();
}
