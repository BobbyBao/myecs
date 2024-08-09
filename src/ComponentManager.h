#pragma once

#include <utils/StructureOfArrays.h>
#include <utils/Slice.h>

#pragma warning(push)
#pragma warning(disable:4819)
#include "robin_hood.h"
#pragma warning(pop)

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "Entity.h"
#include "ComponentSet.h"

namespace myecs {

template <typename ... Elements>
class UTILS_PUBLIC TComponentManager : public ComponentSet {
protected:
    static constexpr size_t ENTITY_INDEX = sizeof ... (Elements);

public:
    using SoA = utils::StructureOfArrays<Elements ..., Entity>;

    using Structure = typename SoA::Structure;

    using Instance = ComponentSet::Type;
	inline static int TypeID = getComponentSetType();

    TComponentManager() noexcept {
        // We always start with a dummy entry because index=0 is reserved. The component
        // at index = 0, is guaranteed to be default-initialized.
        // Sub-classes can use this to their advantage.
        mData.push_back(Structure{});
    }

    TComponentManager(TComponentManager&&) noexcept {/* = default */}
    TComponentManager& operator=(TComponentManager&&) noexcept {/* = default */}
    ~TComponentManager() noexcept = default;

    // not copyable
    TComponentManager(TComponentManager const& rhs) = delete;
    TComponentManager& operator=(TComponentManager const& rhs) = delete;


    // returns true if the given Entity has a component of this Manager
    bool hasComponent(Entity e) const noexcept {
        return getInstance(e) != 0;
    }

    // Get instance of this Entity to be used to retrieve components
    Instance getInstance(Entity e) const noexcept {
        auto const& map = mInstanceMap;
        // find() generates quite a bit of code
        auto pos = map.find(e);
        return pos != map.end() ? pos->second : 0;
    }

    // Returns the number of components (i.e. size of each array)
    size_t getComponentCount() const noexcept {
        // The array as an extra dummy component at index 0, so the visible count is 1 less.
        return mData.size() - 1;
    }

    bool empty() const noexcept {
        return getComponentCount() == 0;
    }

    Entity const* getEntities() const noexcept {
        return data<ENTITY_INDEX>() + 1;
    }

    Entity getEntity(Instance i) const noexcept {
        return elementAt<ENTITY_INDEX>(i);
    }

    // Add a component to the given Entity. If the entity already has a component from this
    // manager, this function is a no-op.
    // This invalidates all pointers components.
    inline Instance addComponent(Entity e);

    // Removes a component from the given entity.
    // This invalidates all pointers components.
    inline Instance removeComponent(Entity e);

    // return the first instance
    Instance begin() const noexcept { return 1u; }

    // return the past-the-last instance
    Instance end() const noexcept { return Instance(begin() + getComponentCount()); }

    // return a pointer to the first element of the ElementIndex'th array
    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex>* begin() noexcept {
        return mData.template data<ElementIndex>() + 1;
    }

    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex> const* begin() const noexcept {
        return mData.template data<ElementIndex>() + 1;
    }

    // return a pointer to the past-the-end element of the ElementIndex'th array
    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex>* end() noexcept {
        return begin<ElementIndex>() + getComponentCount();
    }

    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex> const* end() const noexcept {
        return begin<ElementIndex>() + getComponentCount();
    }

    // return a Slice<>
    template<size_t ElementIndex>
    utils::Slice<typename SoA::template TypeAt<ElementIndex>> slice() noexcept {
        return { begin<ElementIndex>(), end<ElementIndex>() };
    }

    template<size_t ElementIndex>
    utils::Slice<const typename SoA::template TypeAt<ElementIndex>> slice() const noexcept {
        return { begin<ElementIndex>(), end<ElementIndex>() };
    }

    // return a reference to the index'th element of the ElementIndex'th array
    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex>& elementAt(Instance index) noexcept {
        assert(index);
        return data<ElementIndex>()[index];
    }

    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex> const& elementAt(Instance index) const noexcept {
        assert(index);
        return data<ElementIndex>()[index];
    }

    // returns a pointer to the RAW ARRAY of components including the first dummy component
    // Use with caution.
    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex> const* raw_array() const noexcept {
        return data<ElementIndex>();
    }

    // We need our own version of Field because mData is private
    template<size_t E>
    struct Field : public SoA::template Field<E> {
        Field(TComponentManager& soa, ComponentSet::Type i) noexcept
                : SoA::template Field<E>{ soa.mData, i } {
        }
        using SoA::template Field<E>::operator =;
    };

protected:
    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex>* data() noexcept {
        return mData.template data<ElementIndex>();
    }

    template<size_t ElementIndex>
    typename SoA::template TypeAt<ElementIndex> const* data() const noexcept {
        return mData.template data<ElementIndex>();
    }

    // swap only internals
    void swap(Instance i, Instance j) noexcept {
        assert(i);
        assert(j);
        if (i && j) {
            // update the index map
            auto& map = mInstanceMap;
            Entity& ei = elementAt<ENTITY_INDEX>(i);
            Entity& ej = elementAt<ENTITY_INDEX>(j);
            std::swap(ei, ej);
            if (ei) {
                map[ei] = i;
            }
            if (ej) {
                map[ej] = j;
            }
        }
    }

protected:
    SoA mData;

private:
    // maps an entity to an instance index
	robin_hood::unordered_map<Entity, Instance, Entity::Hasher> mInstanceMap;
	std::vector<Instance> mFreeList;
};

// Keep these outside of the class because CLion has trouble parsing them
template<typename ... Elements>
typename TComponentManager<Elements ...>::Instance
TComponentManager<Elements ...>::addComponent(Entity e) {
    Instance ci = 0;
    if (!e.isNull()) {
        if (!hasComponent(e)) {

            if (!mFreeList.empty()) {
                auto ci = mFreeList.back();
                mFreeList.pop_back();
                mData.emplace(ci, Structure{}).template back<ENTITY_INDEX>() = e;

            } else {

                // this is like a push_back(e);
                mData.push_back(Structure{}).template back<ENTITY_INDEX>() = e;
                // index 0 is used when the component doesn't exist
                ci = Instance(mData.size() - 1);
            }

            mInstanceMap[e] = ci;
        } else {
            // if the entity already has this component, just return its instance
            ci = mInstanceMap[e];
        }
    }
    assert(ci != 0);
    return ci;
}

// Keep these outside of the class because CLion has trouble parsing them
template <typename ... Elements>
typename TComponentManager<Elements ...>::Instance
TComponentManager<Elements ... >::removeComponent(Entity e) {
    auto& map = mInstanceMap;
    auto pos = map.find(e);
    if (UTILS_LIKELY(pos != map.end())) {
        auto index = pos->second;
        assert(index != 0);
        mFreeList.push_back(index);
        map.erase(pos);
        return index;
    }
    return 0;
}


}