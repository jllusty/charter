#include "system.hpp"

#include "components.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <vector>
#include <utility>

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
        bool upArr = false, downArr = false;
        void update(Context &c); 
    };
    struct Direction {
        void update(Context &c);
    };
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