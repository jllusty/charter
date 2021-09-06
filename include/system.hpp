#pragma once
#include "context.hpp"

template<typename Derived>
class System {
public:
    void update(Context& c) {
        static_cast<Derived*>(this)->update(c);
    }
};