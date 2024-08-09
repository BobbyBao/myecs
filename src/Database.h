#pragma once
#include "ComponentSet.h"

#pragma warning(push)
#pragma warning(disable:4819)
#include "robin_hood.h"
#pragma warning(pop)

#include "Entity.h"

#include <deque>
#include <mutex>
#include <vector>
#include <assert.h>

namespace myecs {

	class Database {
	public:

		class Listener {
		public:
			virtual void onEntitiesDestroyed(size_t n, Entity const* entities) noexcept = 0;
		protected:
			virtual ~Listener() noexcept;
		};

		static constexpr const int GENERATION_SHIFT = 24;
		static constexpr const size_t RAW_INDEX_COUNT = (1 << GENERATION_SHIFT);
		static constexpr const Entity::Type INDEX_MASK = (1 << GENERATION_SHIFT) - 1u;

		static constexpr const int MIN_VER_SHIFT = 16;
		static constexpr const size_t MIN_VER_COUNT = (1 << 16);
		static constexpr const size_t MIN_VER_MASK = 0xffff;
		
		static_assert(MIN_VER_SHIFT <= GENERATION_SHIFT);

		Database();
		~Database();

		// maximum number of entities that can exist at the same time
		static size_t getMaxEntityCount() noexcept {
			// because index 0 is reserved, we only have 2^GENERATION_SHIFT - 1 valid indices
			return RAW_INDEX_COUNT - 1;
		}

		size_t getEntityCount() const noexcept {
			std::lock_guard<std::mutex> const lock(mFreeListLock);
			if (mCurrentIndex < RAW_INDEX_COUNT) {
				return (mCurrentIndex - 1) - mFreeList.size();
			}
			else {
				return getMaxEntityCount() - mFreeList.size();
			}
		}
		// Create a new Entity. Thread safe.
		// Return Entity.isNull() if the entity cannot be allocated.
		Entity create() {
			Entity e;
			create(1, &e);
			return e;
		}

		// Destroys an Entity. Thread safe.
		void destroy(Entity e) noexcept {
			destroy(1, &e);
		}

		bool isAlive(Entity e) const noexcept {
			assert(e.getId() < RAW_INDEX_COUNT);
			return (!e.isNull()) && (e.mVersion == getGen(e.getId()));
		}
		
		void create(size_t n, Entity* entities);
		
		void destroy(size_t n, Entity* entities) noexcept;

		void registerListener(Database::Listener* l) noexcept;
		void unregisterListener(Database::Listener* l) noexcept;
	
		template<typename T>
		T& get() {
			return *(T*)mComponentSets[T::TypeID].get();
		}

		template<typename T>
		T* getPtr() {
			auto set = (T*)mComponentSets[T::TypeID].get();
			if (set == nullptr) {
				mComponentSets[T::TypeID] = make_unique<T>();
				return (T*)mComponentSets[T::TypeID].get();
			}
			return set;
		}

		template<typename T>
		void addComSet() {
			auto set = (T*)mComponentSets[T::TypeID].get();
			assert(set == nullptr);
			mComponentSets[T::TypeID] = make_unique<T>();
		}

	private:

		uint8_t getGen(uint32_t index) const {
			return mGens[index >> MIN_VER_SHIFT][index & MIN_VER_MASK];
		}

		uint8_t& getGen(uint32_t index) {
			return mGens[index >> MIN_VER_SHIFT][index & MIN_VER_MASK];
		}

		std::vector<Listener*> getListeners() const noexcept {
			std::lock_guard<std::mutex> const lock(mListenerLock);
			auto const& listeners = mListeners;
			std::vector<Listener*> result((uint32_t)listeners.size());
			result.resize(result.capacity()); // unfortunately this memset()
			std::copy(listeners.begin(), listeners.end(), result.begin());
			return result; // the c++ standard guarantees a move
		}

		uint32_t mCurrentIndex = 1;

		// stores indices that got freed
		mutable std::mutex mFreeListLock;
		std::deque<Entity::Type> mFreeList;

		mutable std::mutex  mListenerLock;
		robin_hood::unordered_set<Listener*> mListeners;

		// stores the generation of each index.
		uint8_t** mGens = nullptr;

		robin_hood::unordered_map<uint32_t, std::unique_ptr<ComponentSet>> mComponentSets;
	};
}