#include "InstanceRegistry.h"

namespace strata::link
{
    InstanceRegistry& InstanceRegistry::getInstance()
    {
        // Meyers singleton: thread-safe init, lives for process lifetime.
        static InstanceRegistry instance;
        return instance;
    }

    void InstanceRegistry::add (InstanceClient* c)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        {
            const juce::ScopedLock sl (lock);
            clients.push_back (c);
        }
        fireRegistryChanged();
    }

    void InstanceRegistry::remove (InstanceClient* c)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        {
            const juce::ScopedLock sl (lock);
            clients.erase (std::remove (clients.begin(), clients.end(), c), clients.end());
        }
        fireRegistryChanged();
    }

    void InstanceRegistry::notifyMetadataChanged()
    {
        fireRegistryChanged();
    }

    bool InstanceRegistry::setRemoteParameter (const juce::Uuid& target,
                                               const juce::String& paramId, float value)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());

        InstanceClient* found = nullptr;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c->getUuid() == target) { found = c; break; }
        }
        if (found == nullptr) return false; // target gone -> degrade safely

        found->setRemoteParameter (paramId, value); // called outside the lock
        return true;
    }

    bool InstanceRegistry::setRemoteLinkId (const juce::Uuid& target, int linkId)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        InstanceClient* found = nullptr;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c->getUuid() == target) { found = c; break; }
        }
        if (found == nullptr) return false;
        found->setLinkId (linkId);
        return true;
    }

    bool InstanceRegistry::setRemoteGroupId (const juce::Uuid& target, int group)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        InstanceClient* found = nullptr;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c->getUuid() == target) { found = c; break; }
        }
        if (found == nullptr) return false;
        found->setGroupId (group);
        return true;
    }

    bool InstanceRegistry::setRemoteOrderIndex (const juce::Uuid& target, int orderIndex)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        InstanceClient* found = nullptr;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c->getUuid() == target) { found = c; break; }
        }
        if (found == nullptr) return false;
        found->setOrderIndex (orderIndex);
        return true;
    }

    bool InstanceRegistry::clearRemoteClip (const juce::Uuid& target)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        InstanceClient* found = nullptr;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c->getUuid() == target) { found = c; break; }
        }
        if (found == nullptr) return false;
        found->clearMeterClip();
        return true;
    }

    //---- Bucket identity --------------------------------------------------
    juce::uint32 InstanceRegistry::defaultBucketColour (int group)
    {
        // 8 distinct, stage-readable hues; group 0 (none) is neutral grey.
        static const juce::uint32 palette[8] = {
            0xff5b8cff, 0xff36c1a6, 0xffe2b341, 0xffe5684d,
            0xffa56bff, 0xff4db6e5, 0xff8bc34a, 0xffff8a65 };
        if (group <= 0) return 0xff8a94a3;
        return palette[(group - 1) % 8];
    }

    juce::String InstanceRegistry::getBucketName (int group) const
    {
        const juce::ScopedLock sl (lock);
        auto it = buckets.find (group);
        if (it != buckets.end() && it->second.name.isNotEmpty())
            return it->second.name;
        return group == 0 ? juce::String ("No Group") : "Group " + juce::String (group);
    }

    void InstanceRegistry::setBucketName (int group, const juce::String& name)
    {
        { const juce::ScopedLock sl (lock); buckets[group].name = name; }
        fireRegistryChanged();
    }

    juce::uint32 InstanceRegistry::getBucketColour (int group) const
    {
        const juce::ScopedLock sl (lock);
        auto it = buckets.find (group);
        if (it != buckets.end() && it->second.colour != 0)
            return it->second.colour;
        return defaultBucketColour (group);
    }

    void InstanceRegistry::setBucketColour (int group, juce::uint32 argb)
    {
        { const juce::ScopedLock sl (lock); buckets[group].colour = argb; }
        fireRegistryChanged();
    }

    int InstanceRegistry::broadcastToLinked (InstanceClient* sender,
                                             const juce::String& paramId, float value)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread());
        const int link  = sender->getLinkId();
        const int group = sender->getGroupId();
        if (link == 0) return 0;

        std::vector<InstanceClient*> targets;
        {
            const juce::ScopedLock sl (lock);
            for (auto* c : clients)
                if (c != sender && c->getLinkId() == link && c->getGroupId() == group)
                    targets.push_back (c);
        }
        for (auto* t : targets)
            t->applyMirror (paramId, value); // each guards its own re-entrancy
        return (int) targets.size();
    }

    std::vector<InstanceClient*> InstanceRegistry::clientsInGroup (int group) const
    {
        const juce::ScopedLock sl (lock);
        std::vector<InstanceClient*> out;
        if (group == 0) return out;            // no bucket -> nothing to show
        for (auto* c : clients)
            if (c->getGroupId() == group)
                out.push_back (c);             // includes the caller's own instance

        // Sort by user-defined order; tie-break on UUID for a stable layout.
        std::sort (out.begin(), out.end(), [] (InstanceClient* a, InstanceClient* b)
        {
            if (a->getOrderIndex() != b->getOrderIndex())
                return a->getOrderIndex() < b->getOrderIndex();
            return a->getUuid().toString() < b->getUuid().toString();
        });
        return out;
    }

    std::vector<InstanceRegistry::Entry> InstanceRegistry::snapshot (InstanceClient* self) const
    {
        const juce::ScopedLock sl (lock);
        std::vector<Entry> out;
        out.reserve (clients.size());
        for (auto* c : clients)
            out.push_back ({ c->getUuid(), c->getDisplayName(), c->getGroupId(), c == self });
        return out;
    }

    int InstanceRegistry::count() const
    {
        const juce::ScopedLock sl (lock);
        return (int) clients.size();
    }

    void InstanceRegistry::fireRegistryChanged()
    {
        // Take a snapshot of clients under lock, notify outside lock on msg thread.
        std::vector<InstanceClient*> copy;
        {
            const juce::ScopedLock sl (lock);
            copy = clients;
        }
        for (auto* c : copy)
            c->onRegistryChanged();
    }
}
