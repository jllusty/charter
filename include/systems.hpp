#pragma once

#include "system.hpp"

#include "components.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <vector>
#include <utility>
#include <deque>

namespace systems {
    // position system
    struct Position {
        void update(Context &c);
    };
    // velocity system
    struct Velocity {
        void update(Context &c);
    };
    // player input system (use velocity to update)
    struct Input {
        bool Wd = false, Ad = false, Sd = false, Dd = false;
        float mouseX{ 0.0f }, mouseY { 0.0f };
        bool mouseLeftd = false, mouseRightd = false;
        bool mousePressing = false;
        bool upArr = false, downArr = false;
        void update(Context &c); 
    };
    struct Direction {
        void update(Context &c);
    };
    struct Camera {
        unsigned vw{0};
        unsigned vh{0};
        float cx{0.0f};
        float cy{0.0f};
        float zoom{0.0f};
        void update(Context &c, SDL_Renderer& r);
        std::pair<float,float> getWorldCoordinates(float x, float y);
        std::pair<float,float> getCameraCoordinates(float x, float y);
    };
    extern Camera cam;
    struct Sprite {
        void update(Context &c, SDL_Renderer& r);
    };
    // Collision
    struct Collision {
        void update(Context& c);
    };
    // Combat
    struct Combat {
        void update(Context& c);
    };
    // spawner / despawner
    // track the lifetime and/or position of entities in deciding whether or
    // not to remove them from the given context, needs some better components
    // to accomplish this
    struct Spawner {
        void update(Context& c);
    };
    // User Interface
    struct UI {
        TTF_Font* font;
        void update(Context& c, SDL_Renderer& r);
    };
    // Debug Graphics
    /*
    struct DebugGraphics {
        void update(Context& c, SDL_Renderer& r);
    };
    */
}