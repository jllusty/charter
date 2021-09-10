#pragma once

#include "component.hpp"
#include "utility.hpp"

#include <SDL.h>
#include "sdl_util.hpp"

#include <vector>
#include <string>

//using TexturePtr = std::shared_ptr<sf::Texture>;
struct name : component<name> {
    std::string str;
    name(std::string str) : str(str) {}
};
struct input : component<input> {
    bool pressing = false;
    int ticks;
    input(int ticks) : ticks(ticks) {}
};
struct position : component<position> {
    float x, y;
    position(float x, float y) : x(x), y(y) {}
};
struct velocity : component<velocity> {
    bool inMotion { false };
    float x, y;
    velocity(float x, float y) : x(x), y(y) {}
};
struct direction : component<direction> {
    enum class facing {left, right, up, down} dir;
    direction(facing dir) : dir(dir) {}
};
// sprite compositional components
struct idle : component<idle> {
    unsigned ticks;
    idle(unsigned startingTicks) : ticks(startingTicks) {}
};
struct walk : component<walk> {
    unsigned ticks;
    walk(unsigned startingTicks) : ticks(startingTicks) {}
};
template<typename T>
struct sprite : component<sprite<T>> {
    // parent texture (sprite sheet)
    tilesetMetaPtr pTS;
    unsigned row;
    unsigned col;
    // z ordering
    unsigned z;
    // switching state - another component
    T s;
    sprite(tilesetMetaPtr sourceTilesetMetaPtr, unsigned rowIndex, unsigned colIndex, 
        const unsigned& zOrdering, const T& t) 
        : pTS(sourceTilesetMetaPtr), row(rowIndex), col(colIndex), z(zOrdering), s(t) {}
};
struct volume : component<volume> {
    float w, h;
    volume(float w, float h) : w(w), h(h) {}
};
struct collide : component<collide> {
    collide() {}
};
struct camera : component<camera> {
    entity target;
    float zoom;
    camera(entity target, float zoomLevel) : target(target), zoom(zoomLevel) {}
};
struct combat : component<combat> {
    unsigned health;
    combat(unsigned health) : health(health) {}
};
struct enemy : component<enemy> {
    enum class state { passive, aggressive };
    state s;
    entity target = 0;
    enemy(state s) : s(s) {}
};
struct debug : component<debug> {
    int val;
    debug(int val) : val(val) {}
};