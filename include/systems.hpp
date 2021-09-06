#include "system.hpp"

//#include "SFML/Graphics.hpp"

#include "components.hpp"

#include <vector>
#include <utility>

namespace systems {
    // position system
    struct Position {
        void update(Context &c);
    };
    // player input system (use velocity to update)
    struct Input {
        bool Wd = false, Ad = false;
        bool Sd = false, Dd = false;
        void update(Context &c); 
    };
    struct Direction {
        void update(Context &c);
    };
    struct Sprite {
        void update(Context &c, SDL_Renderer& r);
    };
    /*
    // player graphics system (sprite with directionality)
    struct Graphics {
        void update(Context& c, sf::RenderTarget& rt);
    };
    // View (viewport / scrolling world scene)
    struct View {
        float windowWidth = 0, windowHeight = 0;
        void update(Context& c, sf::RenderTarget& rt);
        View(float windowWidth, float windowHeight) : 
            windowWidth(windowWidth), windowHeight(windowHeight) {}
    };
    */
    // Collision
    struct Collision {
        void update(Context& c);
    };
    // Combat
    struct Combat {
        void update(Context& c);
    };
}