#pragma once
#include <stdint.h>


namespace myecs {

	class ComponentSet {
	public:
		using Type = uint32_t;

		static int getComponentSetType() {
			static int type = 0;
			return type++;
		}
	};
}

