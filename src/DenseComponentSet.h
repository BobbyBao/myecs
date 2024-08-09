#pragma once
#include <utils/StructureOfArrays.h>
#include "ComponentSet.h"

namespace myecs {
	
    
	class Indexable {
	public:
		using Instance = uint32_t;
		Instance index = 0;
	};

	template <typename ... Elements>
	class DenseComponentSet : public ComponentSet {
	protected:
		static constexpr size_t ENTITY_INDEX = sizeof ... (Elements);
	public:
		inline static int TypeID = getComponentSetType();

		using SoA = utils::StructureOfArrays<Elements ..., Indexable*>;

		using Structure = typename SoA::Structure;

		using Instance = Indexable::Instance;

		DenseComponentSet() noexcept {
			// We always start with a dummy entry because index=0 is reserved. The component
			// at index = 0, is guaranteed to be default-initialized.
			// Sub-classes can use this to their advantage.
			mData.push_back(Structure{});
		}

		DenseComponentSet(DenseComponentSet&&) noexcept {/* = default */ }
		DenseComponentSet& operator=(DenseComponentSet&&) noexcept {/* = default */ }
		~DenseComponentSet() noexcept = default;

		// not copyable
		DenseComponentSet(DenseComponentSet const& rhs) = delete;
		DenseComponentSet& operator=(DenseComponentSet const& rhs) = delete;


		// returns true if the given Entity has a component of this Manager
		bool hasComponent(Indexable* e) const noexcept {
			return getInstance(e) != 0;
		}

		// Get instance of this Entity to be used to retrieve components
		UTILS_NOINLINE
		Instance getInstance(Indexable* e) const noexcept {
			return e->index;
		}

		// Returns the number of components (i.e. size of each array)
		size_t getComponentCount() const noexcept {
			// The array as an extra dummy component at index 0, so the visible count is 1 less.
			return mData.size() - 1;
		}

		bool empty() const noexcept {
			return getComponentCount() == 0;
		}

		Indexable* const* getEntities() const noexcept {
			return data<ENTITY_INDEX>() + 1;
		}

		Indexable* getEntity(Instance i) const noexcept {
			return elementAt<ENTITY_INDEX>(i);
		}

		// Add a component to the given Entity. If the entity already has a component from this
		// manager, this function is a no-op.
		// This invalidates all pointers components.
		inline Instance addComponent(Indexable* e);

		// Removes a component from the given entity.
		// This invalidates all pointers components.
		inline Instance removeComponent(Indexable* e);

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
			Field(DenseComponentSet& soa, Indexable::Instance i) noexcept
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
				Indexable*& ei = elementAt<ENTITY_INDEX>(i);
				Indexable*& ej = elementAt<ENTITY_INDEX>(j);
				std::swap(ei, ej);
				if (ei) {
					ei->index = i;
				}
				if (ej) {
					ej->index = j;
				}
			}
		}

	protected:
		SoA mData;
	};
	
	// Keep these outside of the class because CLion has trouble parsing them
	template<typename ... Elements>
	typename DenseComponentSet<Elements ...>::Instance
		DenseComponentSet<Elements ...>::addComponent(Indexable* e) {
		Instance ci = 0;
		if (e) {
			if (!hasComponent(e)) {
				// this is like a push_back(e);
				mData.push_back(Structure{}).template back<ENTITY_INDEX>() = e;
				// index 0 is used when the component doesn't exist
				ci = Instance(mData.size() - 1);
				e->index = ci;
			}
			else {
				// if the entity already has this component, just return its instance
				ci = e->index;
			}
		}
		assert(ci != 0);
		return ci;
	}

	// Keep these outside of the class because CLion has trouble parsing them
	template <typename ... Elements>
	typename DenseComponentSet<Elements ...>::Instance
		DenseComponentSet<Elements ... >::removeComponent(Indexable* e) {
		size_t index = e->index;
		if (index !=0 ) {// pos->second;
			size_t last = mData.size() - 1;
			if (last != index) {
				// move the last item to where we removed this component, as to keep
				// the array tightly packed.
				mData.forEach([index, last](auto* p) {
					p[index] = std::move(p[last]);
					});

				auto lastEntity = mData.template elementAt<ENTITY_INDEX>(index);
				lastEntity->index = (Instance)index;
			}
			mData.pop_back();
			return (Instance)last;
		}
		return 0;
	}

}
