#include "context.hpp"

#include <type_traits>

// create new entity
entity Context::addEntity() {
    static entity created = 0;
    entity id = ++created;
    entities.insert(id);
    return id;
}
unordered_set<entity> Context::getEntities() { return entities; }
// add component to entity
// entity has components?
// needs to be last to know template specializations
void Context::removeEntity(entity e) {
    entities.erase(e);
    // figure out later
}