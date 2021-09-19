#pragma once

#include "system.hpp"

#include "components.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <map>
#include <list>
#include <vector>
#include <utility>
#include <deque>

namespace systems {
    // position system
    struct Position {
        void update(Context &c, float dt);
    };
    // velocity system
    struct Velocity {
        void update(Context &c, float dt);
    };
    // acceleration system
    struct Acceleration {
        void update(Context &c);
    };
    struct Force {
        void update(Context &c, float dt);
    };
    // player input system (use velocity to update)
    struct Input {
        bool debugToggle = false;
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
    // rendering systems
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
    // organize entities by depth & layer, request draws
    struct Sprite {
        void update(Context &c);
    };
    extern Sprite spr;
    // request UI draws
    struct UI {
        TTF_Font* font;
        void update(Context& c, SDL_Renderer& r);
    };
    extern UI ui;
    // draws the current rendering queue
    struct Graphics {
        std::deque<LRenderable> renderQueue;
        void update(SDL_Renderer& r);
    };
    extern Graphics graphics;
    struct Collision {
        // Update collision components, if applicable
        void update(Context& c);
        // Collision Resolution
        void resolve(Context& c, float dt);
    };
    // CombatAI
    //     needs to register interest in collision events
    struct CombatAI {
        std::map<entity,entity> targets;
        void update(Context& c);
    };
    // bullet spawner
    struct Bullet {
        std::deque<std::pair<entity,vec2f>> shotsToFire;
        std::list<entity> bullets;
        void update(Context& c);
    };
    extern Bullet bul;
    // Debug
    struct Debug {
        bool showCollision{ false };
        void update(Context& c);
    };
    extern Debug dbg;
    // Debug Graphics
    /*
    struct DebugGraphics {
        void update(Context& c, SDL_Renderer& r);
    };
    */
}