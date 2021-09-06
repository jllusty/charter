#pragma once

#include "component.hpp"
#include "utility.hpp"

#include <SDL.h>

#include <string>

//using TexturePtr = std::shared_ptr<sf::Texture>;

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
    float x, y;
    velocity(float x, float y) : x(x), y(y) {}
};
struct sprite : component<sprite> {
    // parent texture (sprite sheet)
    SDL_Texture* tex;
    // selected rectangle
    rectu src;
    // rendered z-ordering
    unsigned z;
    sprite(SDL_Texture* pTexture, const rectu& sourceRectangle, const unsigned& zOrdering) 
        : tex(pTexture), src(sourceRectangle), z(zOrdering) {}
};
/*
struct graphics : component<graphics> {
    // shared ownership of a texture
    std::shared_ptr<sf::Texture> t;
    // sub-rectangle of that texture to draw
    sf::IntRect r;
    // z-ordering
    unsigned z;
    graphics(std::shared_ptr<sf::Texture> t, const sf::IntRect& r, const unsigned& z) 
        : t(t), r(r), z(z) {}
};
*/
struct direction : component<direction> {
    enum class facing {down, right, left, up};
    facing dir;
    direction(std::string direction) {
        if(direction == "down") dir = facing::down;
        else if(direction == "right") dir = facing::right;
        else if(direction == "left") dir = facing::left;
        else dir = facing::up;
    }
};
struct volume : component<volume> {
    float w, h;
    volume(float w, float h) : w(w), h(h) {}
};
//struct grid : component<grid> {
//    vector<vector<int>> g;
//    grid(vector<vector<int>>& g) : g(g) {}
//};
struct view : component<view> {
    entity target;
    float zoom;
    view(entity target) : target(target) {}
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