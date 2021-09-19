#include "systems.hpp"

#include <cassert>
#include <queue>
#include <cmath>
#include "sdl_util.hpp"

#include "logger.hpp"
extern Logger glog;

namespace systems {
    // instances
    //  entity lifetime managers
    Bullet bul{ };
    //  rendering managers
    Sprite spr{ };
    UI ui{ };
    Graphics graphics{ };

    // camera instance
    Camera cam{ };
    // debug system
    Debug dbg{ };
    // position system
    //   (position,velocity) ? (position)
    void Position::update(Context &c, float dt) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<position,velocity>(e)) {
                c.getComponent<position>(e)->x += dt * c.getComponent<velocity>(e)->x;
                c.getComponent<position>(e)->y += dt * c.getComponent<velocity>(e)->y;
            }
        }
    }
    // velocity system
    //     TODO(jllusty): maybe track whether entities are or are not moving
    void Velocity::update(Context& c, float dt) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<velocity,acceleration>(e)) {
                c.getComponent<velocity>(e)->x += dt * c.getComponent<acceleration>(e)->x;
                c.getComponent<velocity>(e)->y += dt * c.getComponent<acceleration>(e)->y;
            }
        }
    }
    // acceleration system
    void Acceleration::update(Context& c) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<acceleration,mass>(e)) {
                float m = c.getComponent<mass>(e)->m;
                float rx = 0.f, ry = 0.f;       // resultant force
                // body force
                if(c.hasComponents<force>(e)) {
                    rx += c.getComponent<force>(e)->x;
                    ry += c.getComponent<force>(e)->y;
                }
                // kinetic friction
                /*
                if(c.hasComponents<velocity,friction>(e)) {
                    float fK = c.getComponent<friction>(e)->coeff;
                    // get direction of velocity
                    float& ux = c.getComponent<velocity>(e)->x;
                    float& uy = c.getComponent<velocity>(e)->y;
                    float mag = sqrtf(ux*ux + uy*uy);
                    // needs sufficiency condition for application, otherwise, will cause annoying shaking
                    if(mag > 0.000001) {
                        float fKx = ux / mag * fK * m;
                        float fKy = uy / mag * fK * m;
                        rx -= fKx;
                        ry -= fKy;
                        glog.get() << "[systems::Acceleration]: applying frictional force F = (" << rx << "," << ry << ")\n";
                    }
                }
                */
                c.getComponent<acceleration>(e)->x = rx / m;
                c.getComponent<acceleration>(e)->y = ry / m;
            }
        }
    }
    // input system
    void Input::update(Context &c) {
        // debug toggle
        dbg.showCollision = debugToggle;
        int count = ((Wd)?1:0) + ((Ad)?1:0) + ((Sd)?1:0) + ((Dd)?1:0);
        bool only = (count == 1);
        for(auto e : c.getEntities()) {
            if(c.hasComponents<input,velocity>(e)) {
                bool canFace = c.hasComponents<direction>(e);
                auto& fx = c.getComponent<velocity>(e)->x;
                auto& fy = c.getComponent<velocity>(e)->y;
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
                fx = sx*30.f; fy = sy*30.f;
            }
            // camera zoom debug
            else if(c.hasComponents<camera>(e)) {
                if(upArr) c.getComponent<camera>(e)->zoom += 1.0f/60.f;
                else if(downArr) c.getComponent<camera>(e)->zoom -= 1.0f/60.f;
            }
            // mouse position debug
            if(c.hasComponents<position,cursor,sprite,shoots>(e)) {
                c.getComponent<cursor>(e)->x = mouseX;
                c.getComponent<cursor>(e)->y = mouseY;
                if(mouseLeftd && !mousePressing) {
                    mousePressing = true;
                    auto [wX,wY] = cam.getWorldCoordinates(mouseX,mouseY);
                    float x = c.getComponent<position>(e)->x;
                    float y = c.getComponent<position>(e)->y;
                    vec2f dir = vec2f(wX,wY)-vec2f(x,y);
                    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
                    dir = dir/len;
                    bul.shotsToFire.emplace_front(e,dir);
                }
                else if(!mouseLeftd) {
                    mousePressing = false;
                }
            }
        }
    }
    // bullet system
    void Bullet::update(Context &c) {
        // shoot bullets requested by other systems
        while(!shotsToFire.empty()) {
            auto[e, dir] = shotsToFire.front(); shotsToFire.pop_front();

            if(!c.hasComponents<position,shoots>(e)) continue;
            // entity location in world coordinates
            float x = c.getComponent<position>(e)->x;
            float y = c.getComponent<position>(e)->y;
            // needs to be spawned in world coordinates, not screen coordinates
            auto pTS = c.getComponent<shoots>(e)->pTS;
            
            entity spawned = c.addEntity();
            bullets.push_back(spawned);
            float dx = dir.x, dy = dir.y;
            c.addComponent<position>(spawned, x, y);
            // galilean relativity, baby
            float speed = 60.0f;
            dx *= speed;
            dy *= speed;
            if(c.hasComponents<velocity>(e)) {
                dx += c.getComponent<velocity>(e)->x; 
                dy += c.getComponent<velocity>(e)->y; 
            }
            c.addComponent<velocity>(spawned, dx, dy);
            rectf vbox = pTS->tileMetas[0]->boxes[0];
            c.addComponent<volume>(spawned, vbox);
            //c.addComponent<collide>(spawned,0.f,0.f);
            c.addComponent<mass>(spawned, 1.f);
            c.addComponent<bullet>(spawned, e, false);
            c.addComponent<sprite>(spawned,pTS,0,0,1);
            glog.get() << "[systems::Bullet]: spawned an entity! id = " << spawned << "\n";
        }
        // delete bullets that hit shit
        for(auto it = bullets.begin(); it != bullets.end();) {
            if(c.getComponent<bullet>(*it)->hit) {
                c.removeEntity(*it);
                glog.get() << "[systems::Bullet]: despawned an entity! id = " << *it << "\n";
                it = bullets.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    // camera system
    void Camera::update(Context &c, SDL_Renderer& r) {
        // get viewport dimensions
        SDL_Rect vr;
        SDL_RenderGetViewport(&r, &vr);
        vw = vr.w;
        vh = vr.h;
        // get camera
        for(entity e: c.getEntities()) {
            if(c.hasComponents<camera>(e)) {
                entity target = c.getComponent<camera>(e)->target;
                if(c.hasComponents<position,volume>(target)) {
                    // center of screen
                    cx = c.getComponent<position>(target)->x + c.getComponent<volume>(target)->box.w/2.0f;
                    cy = c.getComponent<position>(target)->y + c.getComponent<volume>(target)->box.h/2.0f;
                    glog.get() << "[systems::Camera]: world position of center of screen: (" << cx << "," << cy << ")\n";
                }
                else {
                    glog.get() << "[systems::Camera]: camera targeted to non-positional entity\n";
                }
                zoom = c.getComponent<camera>(e)->zoom;
                break;
            }
        }
    }
    std::pair<float,float> Camera::getWorldCoordinates(float x, float y) {
        float wx = (x - float(vw)/2.f)/zoom + cx;
        float wy = (y - float(vh)/2.f)/zoom + cy;
        return {wx, wy};
    }
    std::pair<float,float> Camera::getCameraCoordinates(float x, float y) {
        return { (x-cx)*zoom+float(vw)/2.f, (y-cy)*zoom+float(vh)/2.f };
    }
    // ui system
    //     currently needs a renderer as it makes textures actively using
    //     a rendering context
    void UI::update(Context& c, SDL_Renderer& r) {
        std::vector<entity> cursors;
        // text color
        SDL_Color fg = {125,0,0};
        // combat info
        for(entity e : c.getEntities()) {
            if(c.hasComponents<position,combat>(e)) {
                // render health string into texture
                std::string textStr = "HEALTH: " + std::to_string(c.getComponent<combat>(e)->health);
                SDL_Surface* textSurface = TTF_RenderText_Solid(font, textStr.c_str(), fg);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(&r, textSurface);
                SDL_FreeSurface(textSurface);
                // query texture for width & height
                int w, h;
                int access;
                Uint32 format;
                SDL_QueryTexture(textTexture, &format, &access, &w, &h);
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                float dx = w/2;
                float dy = h;
                // center text above entity
                if(c.hasComponents<volume>(e)) {
                    dx -= c.getComponent<volume>(e)->box.w;
                }
                SDL_Rect src = {0, 0, w, h};
                SDL_Rect dst;
                auto [cX, cY] = cam.getCameraCoordinates(x,y);
                dst.x = cX-dx;
                dst.y = cY-dy;
                dst.w = src.w; dst.h = src.h;
                // push renderable to graphics system
                graphics.renderQueue.emplace_back(textTexture,src,dst);
            }
            // other UI things to be draw in camera coords
            /*for(entity e : cursors) {
                auto s = c.getComponent<sprite>(e);
                float x = c.getComponent<cursor>(e)->x;
                float y = c.getComponent<cursor>(e)->y;
                SDL_Texture* tex = s->pTS->tex;
                unsigned row = s->row;
                unsigned col = s->col;
                SDL_Rect src = s->pTS->get(row,col);
                // transform to camera coordinates
                SDL_Rect dest;
                dest.x = x;
                dest.y = y;
                dest.w = src.w; dest.h = src.h;
                glog.get() << "[systems::Sprite]: calling SDL_RenderCopy on \n";
                glog.get() << "\tsource = [x = " << src.x << ",y = " << src.y 
                          << ",w = " << src.w << ",h = " << src.h << "]\n";
                glog.get() << "\tdest = [x = " << dest.x << ",y = " << dest.y 
                          << ",w = " << dest.w << ",h = " << dest.h << "]\n";
                SDL_RenderCopy(&r, tex, &src, &dest);
            }*/
        }
    }
    // sprite system
    void Sprite::update(Context& c) {
        // order sprites by z
        using ep = std::pair<entity,float>;
        std::vector<ep> eps;
        auto cmp = [](ep ep1, ep ep2) -> bool
        {
            return(ep1.second > ep2.second);
        };
        std::priority_queue pq(cmp,eps);   
        std::priority_queue tops(cmp,eps); // draw last by request
        for(entity e : c.getEntities()) {
            // sprites
            if(c.hasComponents<position,sprite>(e)) {
                pq.emplace(e,c.getComponent<sprite>(e)->z + c.getComponent<position>(e)->y);
            }
        }
        // apply camera transform
        while(!pq.empty()) {
            auto [e, zl] = pq.top(); pq.pop();
            auto s = c.getComponent<sprite>(e);
            float x = c.getComponent<position>(e)->x;
            float y = c.getComponent<position>(e)->y;
            unsigned row = s->row;
            unsigned col = s->col;
            SDL_Rect src = s->pTS->get(row,col);
            // transform to camera coordinates
            SDL_Rect dst;
            auto [cx, cy] = cam.getCameraCoordinates(x,y);
            dst.x = cx;
            dst.y = cy;
            dst.w = src.w*cam.zoom; dst.h = src.h*cam.zoom;
            graphics.renderQueue.emplace_back(s->pTS->tex, src, dst);
        }
    }
    void Graphics::update(SDL_Renderer& r) {
        while(!renderQueue.empty()) {
            auto rParams = renderQueue.front(); renderQueue.pop_front();
            SDL_Texture* tex = rParams.texPtr;
            SDL_Rect& src = rParams.src; SDL_Rect& dst = rParams.dst;
            glog.get() << "[systems::Graphics]: calling SDL_RenderCopy on \n";
            glog.get() << "\tsource = [x = " << src.x << ",y = " << src.y 
                  << ",w = " << src.w << ",h = " << src.h << "]\n";
            glog.get() << "\tdest = [x = " << dst.x << ",y = " << dst.y 
                  << ",w = " << dst.w << ",h = " << dst.h << "]\n";
            SDL_RenderCopy(&r, tex, &src, &dst);
        }
    }
    // direction system
    //   (velocity) ? (direction)
    void Direction::update(Context& c) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<direction>(e)) {
                if(c.hasComponents<sprite>(e)) {
                    auto dir = c.getComponent<direction>(e)->dir;
                    if(dir == direction::facing::left) {
                        c.getComponent<sprite>(e)->row = 1;
                    }
                    else if(dir == direction::facing::right) {
                        c.getComponent<sprite>(e)->row = 0;
                    }
                    else if(dir == direction::facing::down) {
                        c.getComponent<sprite>(e)->row = 3;
                    }
                    else if(dir == direction::facing::up) {
                        c.getComponent<sprite>(e)->row = 2;
                    }
                }
            }
        }
    }
    // for entities with indexed collision boxes, update their local boxes
    void Collision::update(Context& c) {
        for(entity e : c.getEntities()) {
            if(c.hasComponents<position,collide,sprite>(e)) {
                //float x = c.getComponent<position>(e)->x;
                //float y = c.getComponent<position>(e)->y;
                auto tms = c.getComponent<sprite>(e)->pTS->tileMetas;
                unsigned nCols = c.getComponent<sprite>(e)->pTS->numCols;
                unsigned row = c.getComponent<sprite>(e)->row;
                unsigned col = c.getComponent<sprite>(e)->col;
                unsigned id = col + row*nCols;
                if(tms.count(id) != 0) {
                    c.getComponent<collide>(e)->box = tms[id]->boxes.back();
                }
            }
        }
    }
    // Collision
    void Collision::resolve(Context& c, float dt) {
        // accumulate pairwise collisions
        std::vector<std::pair<entity,entity>> collisions;
        // accumulate all future rectfs in world coordinates
        std::vector<std::pair<entity,rectf>> eRects;
        for(entity e : c.getEntities()) {
            if(c.hasComponents<collide>(e) || c.hasComponents<volume>(e)) {
                // has indexed collision component
                rectf r(0.f,0.f,0.f,0.f);
                // get entity-local collision box
                if(c.hasComponents<collide>(e)) {
                    r = c.getComponent<collide>(e)->box;
                }
                else if(c.hasComponents<volume>(e)){
                    r = c.getComponent<volume>(e)->box;
                }
                // get current entity world position
                if(c.hasComponents<position>(e)) {
                    r.x += c.getComponent<position>(e)->x;
                    r.y += c.getComponent<position>(e)->y;
                }
                // has future position: move forward 1 timestep
                if(c.hasComponents<velocity>(e)) {
                    r.x += dt * c.getComponent<velocity>(e)->x;
                    r.y += dt * c.getComponent<velocity>(e)->y;
                }
                eRects.emplace_back(e,r);
            }
        }
        // do collision check
        for(size_t i = 0; i < eRects.size(); ++i) {
            auto& [e1,r1] = eRects[i];
            for(size_t j = i+1; j < eRects.size(); ++j) {
                auto& [e2,r2] = eRects[j];
                if(collision(r1,r2)) collisions.emplace_back(e1,e2);
            }
        }
        // handle pairwise collisions (should be eventually moved to a proper system)
        for(auto [e1, e2] : collisions) {
            bool moving1 = c.hasComponents<velocity>(e1);
            bool moving2 = c.hasComponents<velocity>(e2);
            bool dragging1 = c.hasComponents<friction>(e1);
            bool dragging2 = c.hasComponents<friction>(e2);
            bool massy1 = c.hasComponents<mass>(e1);
            bool massy2 = c.hasComponents<mass>(e2);
            bool bullet1 = c.hasComponents<bullet>(e1);
            bool bullet2 = c.hasComponents<bullet>(e2);
            
            // bullet: mark for despawn and damage combatant (move damaging to combat system)
            if(bullet1 && (e2 != c.getComponent<bullet>(e1)->shooter)) {
                c.getComponent<bullet>(e1)->hit = true;
                if(c.hasComponents<combat>(e2)) c.getComponent<combat>(e2)->health -= 25;
            }
            else if(bullet2 && (e1 != c.getComponent<bullet>(e2)->shooter)) {
                c.getComponent<bullet>(e2)->hit = true;
                if(c.hasComponents<combat>(e1)) c.getComponent<combat>(e1)->health -= 25;
            }
            // elastic collision: at least one is not dragging and both are massy
            //     should be moved to a physics system - just enque an impulse
            if((!dragging1 || !dragging2) && (massy1 && massy2)) {
                float m1 = c.getComponent<mass>(e1)->m;
                float m2 = c.getComponent<mass>(e2)->m;
                auto pVel1 = c.getComponent<velocity>(e1); 
                auto pVel2 = c.getComponent<velocity>(e2); 
                // new velocities assuming conservation of momentum and kinetic energy
                float u1 = pVel1->x*((m1-m2)/(m1+m2)) + pVel2->x*(2.f*m2/(m1+m2));
                float v1 = pVel1->y*((m1-m2)/(m1+m2)) + pVel2->y*(2.f*m2/(m1+m2));
                float u2 = pVel2->x*((m2-m1)/(m1+m2)) + pVel1->x*(2.f*m1/(m1+m2));
                float v2 = pVel2->y*((m2-m1)/(m1+m2)) + pVel1->y*(2.f*m1/(m1+m2));
                // set post-collision velocities
                pVel1->x = u1; pVel1->y = v1;
                pVel2->x = u2; pVel2->y = v2;
            }
            // stop whichever one (or both) if they are moving)
            // TODO: this leads to moving entities getting stuck together
            else {
                if(moving1) {
                    c.getComponent<velocity>(e1)->x = 0;
                    c.getComponent<velocity>(e1)->y = 0;
                }
                if(moving2) {
                    c.getComponent<velocity>(e2)->x = 0;
                    c.getComponent<velocity>(e2)->y = 0;
                }
            }
        }
    }
    // Combat
    void CombatAI::update(Context& c) {
        for(entity e : c.getEntities()) {
            // enemies - attack entities that have a combat component
            if(c.hasComponents<position,velocity,enemy>(e)) {
                auto x = c.getComponent<position>(e)->x;
                auto y = c.getComponent<position>(e)->y;
                // if passive, roam about
                if(targets.count(e) == 0) {
                    for(entity t : c.getEntities()) {
                        if(e == t) continue;
                        if(c.hasComponents<position,velocity,combat>(t)) {
                            auto x2 = c.getComponent<position>(t)->x;
                            auto y2 = c.getComponent<position>(t)->y;
                            float dx = x2-x, dy = y2-y;
                            float dr = sqrt(dx*dx+dy*dy);
                            // set target to combatant
                            if(dr < 16.0f*5.0f) {
                                targets[e] = t;
                                glog.get() << "[systems::CombatAI]: registering target!\n";
                            }
                            // play doom music
                        }
                    }
                }
                else {
                    // steer towards target
                    entity t = targets[e];
                    const float maxSpeed = 30.0f;
                    // displacement vector
                    vec2f vE = vec2f(c.getComponent<position>(e)->x,c.getComponent<position>(e)->y);
                    vec2f vT = vec2f(c.getComponent<position>(t)->x,c.getComponent<position>(t)->y);
                    vec2f vET = vE-vT;
                    float len = sqrtf(vET.x*vET.x + vET.y*vET.y);
                    vec2f nvET = vET / len;
                    if(c.hasComponents<velocity,acceleration>(e)) {
                        float& u = c.getComponent<velocity>(e)->x;
                        float& v = c.getComponent<velocity>(e)->y;
                        float speed = sqrtf(u*u + v*v);
                        if(speed < maxSpeed) {
                            glog.get() << "[systems::CombatAI]: swiggity swooty\n";
                            c.getComponent<velocity>(e)->x -= 0.3f * nvET.x;
                            c.getComponent<velocity>(e)->y -= 0.3f * nvET.y;
                        }
                        else {
                            // cap speed
                            u = maxSpeed * nvET.x;
                            v = maxSpeed * nvET.y;
                        }
                    }
                }
            }
        }
    }
}