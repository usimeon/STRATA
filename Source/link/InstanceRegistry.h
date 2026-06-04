#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <vector>
#include <map>

/*  ============================================================================
    InstanceRegistry — process-wide directory of all STRATA instances.

    KEY FACTS / ASSUMPTIONS
    -----------------------
    * In every major DAW (Logic, Pro Tools, Live, Cubase, Reaper, Studio One)
      all instances of a given plugin load into the SAME host process, so a
      static singleton is a reliable shared meeting point. NO IPC needed.
    * One known exception class: out-of-process / sandboxed scanning or some
      AUv3 hosts. If that happens, each instance simply sees only itself in the
      registry and STRATA degrades to a perfectly good standalone strip. We do
      NOT depend on cross-process visibility.
    * ALL registry access happens on the MESSAGE THREAD ONLY. The audio thread
      never sees this class. Linking is a UI/control concern, not a DSP concern.

    THREADING
    ---------
    Guarded by a JUCE CriticalSection, but it is only ever locked on the message
    thread (register/unregister/broadcast/UI enumeration), never the audio
    thread, so there is zero realtime risk.
    ============================================================================ */
namespace strata::link
{
    // What the registry needs from each plugin instance.
    // NOTE: gains are NOT ganged. This interface exposes each instance's live
    // state so ANY instance can *display* and *remotely control* the others
    // individually (control-surface model), without sharing parameter values.
    struct InstanceClient
    {
        virtual ~InstanceClient() = default;

        // Stable identity (persisted in plugin state).
        virtual juce::Uuid   getUuid()        const = 0;
        virtual juce::String getDisplayName() const = 0;  // user-editable label
        virtual int          getGroupId()     const = 0;  // 0 == no bucket
        virtual void         setGroupId (int group) = 0;  // (re)assign to a bucket

        // --- Remote control (message thread only) -------------------------
        // Set ONE of this instance's own parameters because a controlling
        // instance asked. This does NOT propagate anywhere else.
        virtual void setRemoteParameter (const juce::String& paramId, float value) = 0;

        // --- Live state read by a controlling instance's Channel View -----
        // All lock-free atomic reads; safe from the message thread.
        virtual float getParamValue   (const juce::String& paramId) const = 0;
        virtual float getOutputPeakDb () const = 0;   // the meter = plugin output
        virtual float getOutputRmsDb  () const = 0;
        virtual float getVuReferenceDb() const = 0;   // effective 0 VU point (dBFS)
        virtual int   getMeterType    () const = 0;   // params::MeterType index
        virtual float getMeterOffsetDb() const = 0;   // e.g. +3 for RMS+3
        virtual bool  getOutputClip   () const = 0;
        virtual void  clearMeterClip  () = 0;
        virtual bool  isBypassed      () const = 0;

        // --- Mirror linking (stereo pairs etc.) ---------------------------
        virtual int  getLinkId() const = 0;             // 0 == not linked
        virtual void setLinkId (int id) = 0;
        // Apply a mirrored value WITHOUT re-broadcasting (re-entrancy guarded).
        virtual void applyMirror (const juce::String& paramId, float value) = 0;

        // --- Display ordering within the bucket ---------------------------
        virtual int  getOrderIndex() const = 0;
        virtual void setOrderIndex (int idx) = 0;

        // Called when registry membership/metadata changes so the UI refreshes.
        virtual void onRegistryChanged() = 0;
    };

    class InstanceRegistry
    {
    public:
        static InstanceRegistry& getInstance();

        void add    (InstanceClient* c);
        void remove (InstanceClient* c);

        // Notify the registry that an instance's name/group changed.
        void notifyMetadataChanged();

        // Remotely set one parameter on a single target instance (by UUID).
        // Returns true if the target was found. Message thread only.
        bool setRemoteParameter (const juce::Uuid& target,
                                 const juce::String& paramId, float value);

        // Remotely set a target instance's mirror link set (by UUID).
        bool setRemoteLinkId (const juce::Uuid& target, int linkId);

        // Remotely (re)assign a target instance to a bucket (by UUID).
        bool setRemoteGroupId (const juce::Uuid& target, int group);

        // Remotely set a target instance's display order within its bucket.
        bool setRemoteOrderIndex (const juce::Uuid& target, int orderIndex);

        // Remotely clear a target instance's latched peak/clip indicators.
        bool clearRemoteClip (const juce::Uuid& target);

        // ---- Bucket (group) identity: shared name + colour ----------------
        // Stored process-wide; persisted by member instances in their state.
        juce::String   getBucketName   (int group) const;
        void           setBucketName   (int group, const juce::String& name);
        juce::uint32   getBucketColour (int group) const;
        void           setBucketColour (int group, juce::uint32 argb);
        static juce::uint32 defaultBucketColour (int group);

        // Push a mirrored parameter from `sender` to every other instance that
        // shares its (non-zero) link set AND bucket. Returns # of partners.
        int broadcastToLinked (InstanceClient* sender,
                               const juce::String& paramId, float value);

        // Live pointers to ALL instances in `group` (including the caller's own
        // instance), valid only for the current message-thread callback. Used by
        // the Channel View to read meters / push remote changes. Order is stable.
        std::vector<InstanceClient*> clientsInGroup (int group) const;

        // Snapshot for UI (copied, safe to use after returning).
        struct Entry { juce::Uuid uuid; juce::String name; int groupId; bool isSelf; };
        std::vector<Entry> snapshot (InstanceClient* self) const;

        int  count() const;

    private:
        InstanceRegistry() = default;
        void fireRegistryChanged();

        juce::CriticalSection lock;
        std::vector<InstanceClient*> clients;

        struct BucketInfo { juce::String name; juce::uint32 colour = 0; };
        std::map<int, BucketInfo> buckets; // group -> name/colour overrides

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstanceRegistry)
    };
}
