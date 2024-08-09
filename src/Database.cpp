#include "Database.h"

namespace myecs {

	Database::Database() {
		mGens = new uint8_t * [RAW_INDEX_COUNT / MIN_VER_COUNT];
		std::fill_n(mGens, RAW_INDEX_COUNT / MIN_VER_COUNT, nullptr);
		mGens[0] = new uint8_t[MIN_VER_COUNT];
		std::fill_n(mGens[0], MIN_VER_COUNT, 0);
	}

	Database::~Database() {
		for (int i = 0; i < RAW_INDEX_COUNT / MIN_VER_COUNT; i++) {
			if (mGens[i]) {
				delete mGens[i];
			}
		}

		delete mGens;
	}

	void Database::create(size_t n, Entity* entities) {
		Entity::Type index{};
		auto& freeList = mFreeList;

		// this must be thread-safe, acquire the free-list mutex
		std::lock_guard<std::mutex> const lock(mFreeListLock);
		Entity::Type currentIndex = mCurrentIndex;
		for (size_t i = 0; i < n; i++) {
			// If we have more than a certain number of freed indices, get one from the list.
			// this is a trade-off between how often we recycle indices and how large the free list
			// can grow.
			if (freeList.size() > 0) {
				index = freeList.front();
				freeList.pop_front();
			}
			else {
				if (currentIndex >= RAW_INDEX_COUNT) {
					entities[i] = {};
					continue;
				}
				// In the common case, we just grab the next index.
				// This works only until all indices have been used once, at which point
				// we're always in the slower case above. The idea is that we have enough indices
				// that it doesn't happen in practice.
				index = currentIndex++;

				if (mGens[index >> MIN_VER_SHIFT] == nullptr) {
					mGens[index >> MIN_VER_SHIFT] = new uint8_t[MIN_VER_COUNT];
					std::fill_n(mGens[index >> MIN_VER_SHIFT], MIN_VER_COUNT, 0);
				}

			}

			entities[i] = Entity{ index, getGen(index) };
#
		}
		mCurrentIndex = currentIndex;
	}
	
	void Database::destroy(size_t n, Entity* entities) noexcept {
		auto& freeList = mFreeList;

		std::unique_lock<std::mutex> lock(mFreeListLock);
		for (size_t i = 0; i < n; i++) {
			if (!entities[i]) {
				// behave like free(), ok to free null Entity.
				continue;
			}

			// it's an error to delete an Entity twice...
			assert(isAlive(entities[i]));

			// ... deleting a dead Entity will corrupt the internal state, so we protect ourselves
			// against it. We don't guarantee anything about external state -- e.g. the listeners
			// will be called.
			if (isAlive(entities[i])) {
				Entity::Type const index = entities[i].getId();
				freeList.push_back(index);

				// The generation update doesn't require the lock because it's only used for isAlive()
				// and entities work as weak references -- it just means that isAlive() could return
				// true a little longer than expected in some other threads.
				// We do need a memory fence though, it is provided by the mFreeListLock.unlock() below.
				getGen(index)++;

			}
		}
		lock.unlock();

		// notify our listeners that some entities are being destroyed
		auto listeners = getListeners();
		for (auto const& l : listeners) {
			l->onEntitiesDestroyed(n, entities);
		}
	}

	void Database::registerListener(Database::Listener* l) noexcept {
		std::lock_guard<std::mutex> const lock(mListenerLock);
		mListeners.insert(l);
	}

	void Database::unregisterListener(Database::Listener* l) noexcept {
		std::lock_guard<std::mutex> const lock(mListenerLock);
		mListeners.erase(l);
	}


}