#pragma once

#include "component.hpp"
#include "utility.hpp"

#include <SDL.h>
#include "sdl_util.hpp"

#include <vector>
#include <string>

struct name : component<name> {
    std::string str;
    name(std::string str) : str(str) {}
};
struct input : component<input> {
    bool pressing = false;
    int ticks;
    input(int ticks) : ticks(ticks) {}
};
// physics components
struct position : component<position> {
    float x, y;
    position(float x, float y) : x(x), y(y) {}
};
struct velocity : component<velocity> {
    float x, y;
    velocity(float x, float y) : x(x), y(y) {}
};
struct acceleration : component<acceleration> {
    float x, y;
    acceleration(float x, float y) : x(x), y(y) {}
};
struct friction : component<friction> {
    float coeff;
    friction(float coeff) : coeff(coeff) {}
};
struct force : component<force> {
    float x, y;
    force(float x, float y) : x(x), y(y) {}
};
struct mass : component<mass> {
    float m;
    mass(float m) : m(m) {}
};
struct shoots : component<shoots> {
    unsigned ammo{ 0 };
    tilesetMetaPtr pTS;
    shoots(unsigned ammo, tilesetMetaPtr pTS) 
        : ammo(ammo), pTS(pTS) {}
};
// despawning struct
struct bullet : component<bullet> {
    entity shooter{ 0 };
    bool hit{ false };
    bullet(entity shooter, bool hit) : shooter(shooter), hit(hit) {}
};
// entity state specific
struct direction : component<direction> {
    enum class facing {left, right, up, down} dir;
    direction(facing dir) : dir(dir) {}
};
// ui component
struct cursor : component<cursor> { 
    float x, y;
    cursor(float x, float y) : x(x), y(y) {}
};
// renderable
struct sprite : component<sprite> {
    // parent texture (sprite sheet)
    tilesetMetaPtr pTS;
    unsigned row;
    unsigned col;
    // z ordering, determines what sprites are draw first
    unsigned z;
    sprite(tilesetMetaPtr sourceTilesetMetaPtr, unsigned rowIndex, unsigned colIndex, 
        const unsigned& zOrdering) 
        : pTS(sourceTilesetMetaPtr), row(rowIndex), col(colIndex), z(zOrdering) {}
};
struct layer : component<layer> { };
// static collision box
struct volume : component<volume> {
    rectf box;
    volume(rectf box) : box(box) {}
};
// dynamic (indexed) collision box
struct collide : component<collide> {
    rectf box;
    collide(rectf box) : box(box) {}
};
// should change this to be a component of the player entity
struct camera : component<camera> {
    entity target;
    float zoom;
    camera(entity target, float zoomLevel) : target(target), zoom(zoomLevel) {}
};
struct combat : component<combat> {
    unsigned health;
    combat(unsigned health) : health(health) {}
};
// should have a table from enemy entity -> target entitiy
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