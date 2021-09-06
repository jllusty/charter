#include "systems.hpp"

#include <cmath>

#include "logger.hpp"
extern Logger glog;

namespace systems {
    // position system
    void Position::update(Context &c) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<position,velocity>(e)) {
                c.getComponent<position>(e)->x += c.getComponent<velocity>(e)->x;
                c.getComponent<position>(e)->y += c.getComponent<velocity>(e)->y;
            }
        }
    }
    // input system
    void Input::update(Context &c) {
        int count = ((Wd)?1:0) + ((Ad)?1:0) + ((Sd)?1:0) + ((Dd)?1:0);
        bool only = (count == 1);
        for(auto e : c.getEntities()) {
            if(c.hasComponents<input,velocity,position>(e)) {
                glog.get() << "[systems::Input]: found an entity!\n";
                int Ws = (Wd)?1:0;
                int As = (Ad)?1:0;
                int Ss = (Sd)?1:0;
                int Ds = (Dd)?1:0;
                glog.get() << "[systems::Input]: state = " << Ws << As << Ss << Ds << "\n";
                bool canFace = c.hasComponents<direction>(e);
                auto& vx = c.getComponent<velocity>(e)->x;
                auto& vy = c.getComponent<velocity>(e)->y;
                int sx = 0, sy = 0;
                // only 1 key being pressed?
                if(Wd) {
                    sy += -1;
                    if(canFace && only) c.getComponent<direction>(e)->dir = direction::facing::up;
                }
                if(Ad) {
                    sx += -1; 
                    if(canFace && only) c.getComponent<direction>(e)->dir = direction::facing::left;
                }
                if(Sd) {
                    sy +=  1;
                    if(canFace && only) c.getComponent<direction>(e)->dir = direction::facing::down;
                }
                if(Dd) {
                    sx +=  1; 
                    if(canFace && only) c.getComponent<direction>(e)->dir = direction::facing::right;
                }
                vx = sx; vy = sy;
            }
        }
    }
    void Sprite::update(Context& c, SDL_Renderer& r) {
        for(entity e : c.getEntities()) {
            if(c.hasComponents<position,sprite>(e)) {
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                SDL_Texture* tex = c.getComponent<sprite>(e)->tex;
                rectu src = c.getComponent<sprite>(e)->src;
                SDL_Rect source;
                source.x = src.x; source.y = src.y;
                source.w = src.w; source.h = src.h;
                SDL_Rect dest;
                dest.x = x; dest.y = y;
                dest.w = src.w; dest.h = src.h;
                glog.get() << "[systems::Sprite]: calling SDL_RenderCopy on \n";
                glog.get() << "\tsource = [x = " << source.x << ",y = " << source.y 
                          << ",w = " << source.w << ",h = " << source.h << "]\n";
                glog.get() << "\tdest = [x = " << dest.x << ",y = " << dest.y 
                          << ",w = " << dest.w << ",h = " << dest.h << "]\n";
                
                SDL_RenderCopy(&r, tex, &source, &dest);
            }
        }
    }
    /*
    // player graphics system
    void Graphics::update(Context& c, sf::RenderTarget& rt) {
        // sort z-ordering of encountered sprites in realtime (for now)
        using zs = std::pair<unsigned,entity>;
        std::vector<zs> sprites;
        for(auto e : c.getEntities()) {
            // static sprites
            if(c.hasComponents<graphics,position>(e)) {
                auto z = c.getComponent<graphics>(e)->z;
                sprites.emplace_back(z,e);
            }
            // directioned
            else if(c.hasComponents<sprite,direction,position>(e)) {
                auto z = c.getComponent<sprite>(e)->z;
                sprites.emplace_back(z,e);
            }
            // volume rendering (agnostic of z-ordering)
            if(c.hasComponents<position,volume>(e)) {
                unsigned z = UINT_MAX;
                sprites.emplace_back(z,e);
            }
        }
        auto cmp = [](zs a, zs b) { return a.first < b.first; };
        std::sort(sprites.begin(),sprites.end(),cmp);
        // draw in z-order, back (z less) first
        for(auto [z, e] : sprites) {
            if(c.hasComponents<graphics,position>(e)) {
                auto t = c.getComponent<graphics>(e)->t;
                auto r = c.getComponent<graphics>(e)->r;
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                sf::Sprite s(*t,r);
                s.setPosition(sf::Vector2f(x,y));
                rt.draw(s);
            }
            else if(c.hasComponents<sprite,direction,position>(e)) {
                auto ts = c.getComponent<sprite>(e)->ts;
                auto dir = c.getComponent<direction>(e)->dir;
                // do directioning
                TexturePtr t;
                switch(dir) {
                case direction::facing::down:
                    t = ts.at(0);
                    break;
                case direction::facing::right:
                    t = ts.at(1);
                    break;
                case direction::facing::left:
                    t = ts.at(2);
                    break;
                case direction::facing::up:
                    t = ts.at(3);
                default:
                    t = ts.back();
                }
                auto x = c.getComponent<position>(e)->x;
                auto y = c.getComponent<position>(e)->y;
                sf::IntRect r(0,0,t->getSize().x,t->getSize().y);
                sf::Sprite s(*t,r);
                s.setPosition(sf::Vector2f(x,y));
                rt.draw(s);
            }
            // these entities are added twice, potentially
            if(c.hasComponents<position,volume>(e)) {
                auto x = c.getComponent<position>(e)->x;
                auto y = c.getComponent<position>(e)->y;
                auto w = c.getComponent<volume>(e)->w;
                auto h = c.getComponent<volume>(e)->h;
                sf::RectangleShape rs(sf::Vector2f(w,h));
                rs.setPosition(x,y);
                rs.setFillColor(sf::Color(128,0,0,64));
                rs.setOutlineThickness(-1.0f);
                rs.setOutlineColor(sf::Color::Black);
                rt.draw(rs);
            }
        }
    }
    */
    // Direction?
    void Direction::update(Context& c) {
            /*
        for(auto e : c.getEntities()) {
            if(c.hasComponents<graphics,direction>(e)) {
                auto g = c.getComponent<graphics>(e);
                auto d = c.getComponent<direction>(e);
                // set g = graphics(direction)
                // map of direction -> graphics, for each e
                // map of state -> graphics, for each e
            }
        }
            */
    }
    // View
    /*
    void View::update(Context& c, sf::RenderTarget& rt) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<view>(e)) {
                entity target = c.getComponent<view>(e)->target;
                float zoom = c.getComponent<view>(e)->zoom;
                // viewport dimensions (in world)
                auto vw = windowWidth;
                auto vh = windowHeight;
                // center view on target
                if(c.hasComponents<position,volume>(target)) {
                    auto x = c.getComponent<position>(target)->x;
                    auto y = c.getComponent<position>(target)->y;
                    auto w = c.getComponent<volume>(target)->w;
                    auto h = c.getComponent<volume>(target)->h;
                    x += w/2.f; y += h/2.f;
                    sf::View v(sf::FloatRect(x-vw/(2.f*zoom),y-vh/(2.f*zoom),vw/zoom,vh/zoom));
                    rt.setView(v);
                }
            }
        }
    }
    */
    // Collision
    void Collision::update(Context& c) {
        for(entity e : c.getEntities()) {
            if(c.hasComponents<position,volume,velocity>(e)) {
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                float dx = c.getComponent<velocity>(e)->x;
                float dy = c.getComponent<velocity>(e)->y;
                float w = c.getComponent<volume>(e)->w;
                float h = c.getComponent<volume>(e)->h;
                for(entity t : c.getEntities()) {
                    if(e == t) continue;
                    //
                    if(c.hasComponents<position,volume>(t)) {
                        float x2 = c.getComponent<position>(t)->x;
                        float y2 = c.getComponent<position>(t)->y;
                        float w2 = c.getComponent<volume>(t)->w;
                        float h2 = c.getComponent<volume>(t)->h;
                        if(collision(rectu(x+dx,y+dy,w,h),rectu(x2,y2,w2,h2))) {
                            c.getComponent<velocity>(e)->x = 0;
                            c.getComponent<velocity>(e)->y = 0;
                        }
                    }
                }
            }
        }
    }
    // Combat
    void Combat::update(Context& c) {
        for(entity e : c.getEntities()) {
            // enemies - attack entities that have a combat component
            if(c.hasComponents<position,velocity,enemy,direction>(e)) {
                auto x = c.getComponent<position>(e)->x;
                auto y = c.getComponent<position>(e)->y;
                for(entity t : c.getEntities()) {
                    if(e == t) continue;
                    if(c.hasComponents<position,velocity,combat>(t)) {
                        auto x2 = c.getComponent<position>(t)->x;
                        auto y2 = c.getComponent<position>(t)->y;
                        float dx = x2-x, dy = y2-y;
                        float dr = sqrt(dx*dx+dy*dy);
                        // 3 16x16 tilewidths away
                        if(dr < 16.0f*3.0f) {
                            c.getComponent<enemy>(e)->s = enemy::state::aggressive;
                            c.getComponent<direction>(e)->dir = direction::facing::right;
                            c.getComponent<velocity>(e)->x = dx/dr/2.0f;
                            c.getComponent<velocity>(e)->y = dy/dr/2.0f;
                        }
                        else {
                            c.getComponent<enemy>(e)->s = enemy::state::passive;
                            c.getComponent<direction>(e)->dir = direction::facing::down;
                            c.getComponent<velocity>(e)->x = 0;
                            c.getComponent<velocity>(e)->y = 0;
                        }
                    }
                }
            }
        }
    }
}
