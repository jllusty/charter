#pragma once
#include <cstddef>
#include <memory>

template<typename T>
class component {
public:
    using ptr = std::shared_ptr<T>;
};