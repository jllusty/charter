#pragma once
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
using std::shared_ptr;
using std::unordered_set;
using std::unordered_map;

#include "entity.hpp"
#include "component.hpp"

class Context {
    // entities
    unordered_set<entity> entities;
    // components
    template<typename T>
    inline static unordered_map<entity, typename component<T>::ptr> m{};
public:
    // add / remove entities
    entity addEntity();
    void removeEntity(entity e);
    // get entities
    unordered_set<entity> getEntities();
    // add component to entity
    template<typename T, typename ... Args>
    void addComponent(entity e, Args ... args) {
        static_assert(std::is_base_of<component<T>, T>::value, "component<T> is undefined\n");
        //assert(m<T>.count(e) > 0);
        m<T>[e] = std::make_shared<T>(args...);
    }
    // get component of entity
    template<typename T>
    typename component<T>::ptr getComponent(entity e) {
        static_assert(std::is_base_of<component<T>, T>::value, "component<T> is undefined\n");
        //assert(m<T>.count(e) > 0);
        return m<T>[e];
    }
    // remove component from entity
    void removeComponent(entity e);
    // query if entity has component(s)
    template<typename T, typename... Args>
    bool hasComponents(entity e) {
        static_assert(std::is_base_of<component<T>, T>::value, "component<T> is undefined\n");
        if constexpr(sizeof...(Args) > 0) {
            return (m<T>.count(e) > 0) && hasComponents<Args...>(e);
        }
        else {
            return(m<T>.count(e) > 0);
        }
    }
};
