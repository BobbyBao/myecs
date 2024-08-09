#pragma once
#include "ComponentSet.h"

namespace myecs {

	struct Entity {
		// this can be used to create an array of to-be-filled entities (see create())
		Entity() noexcept { } // NOLINT(modernize-use-equals-default), Ubuntu compiler bug

		// Entities can be copied
		Entity(const Entity& e) noexcept = default;
		Entity(Entity&& e) noexcept = default;
		Entity& operator=(const Entity& e) noexcept = default;
		Entity& operator=(Entity&& e) noexcept = default;

		// Entities can be compared
		bool operator==(Entity e) const { return e.mId == mId; }
		bool operator!=(Entity e) const { return e.mId != mId; }

		// Entities can be sorted
		bool operator<(Entity e) const { return e.mId < mId; }

		bool isNull() const noexcept {
			return mId == 0;
		}

		// an id that can be used for debugging/printing
		uint32_t getId() const noexcept {
			return mId;
		}

		explicit operator bool() const noexcept { return !isNull(); }

		void clear() noexcept { mValue = 0; }

		struct Hasher {
			typedef Entity argument_type;
			typedef size_t result_type;
			result_type operator()(argument_type const& e) const {
				return e.mValue;
			}
		};

	private:
		friend class Database;

		using Type = uint32_t;

		explicit Entity(Type identity, Type ver) noexcept : mId(identity), mVersion(ver) { }
		union {
			Type mValue = 0;
			struct {
				Type mId : 24;
				Type mVersion : 8;
			};
		};
	};
}