#include "systems.hpp"

#include <cassert>
#include <queue>
#include <cmath>
#include "sdl_util.hpp"

#include "logger.hpp"
extern Logger glog;

namespace systems {
    Camera cam{ };
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
                        glog.get() << "applying frictional force F = (" << rx << "," << ry << ")\n";
                    }
                }
                c.getComponent<acceleration>(e)->x = rx / m;
                c.getComponent<acceleration>(e)->y = ry / m;
            }
        }
    }
    // input system
    //   (input,velocity,position) ? (velocity)
    void Input::update(Context &c) {
        int count = ((Wd)?1:0) + ((Ad)?1:0) + ((Sd)?1:0) + ((Dd)?1:0);
        bool only = (count == 1);
        for(auto e : c.getEntities()) {
            if(c.hasComponents<input,velocity>(e)) {
                glog.get() << "[systems::Input]: found an entity!\n";
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
            if(c.hasComponents<position,cursor,sprite<idle>,sprite<cursor>,shoots>(e)) {
                c.getComponent<cursor>(e)->x = mouseX;
                c.getComponent<cursor>(e)->y = mouseY;
                if(mouseLeftd && !mousePressing) {
                    mousePressing = true;
                    entity spawned = c.addEntity();
                    // entity location in world coordinates
                    float x = c.getComponent<position>(e)->x;
                    float y = c.getComponent<position>(e)->y;
                    // needs to be spawned in world coordinates, not screen coordinates
                    auto [wX, wY] = cam.getWorldCoordinates(mouseX, mouseY);
                    float dx = wX-x, dy = wY-y;
                    float len = sqrtf(dx*dx + dy*dy);
                    dx = dx / len;
                    dy = dy / len;
                    // galilean relativity, baby
                    if(c.hasComponents<velocity>(e)) {
                        
                    }
                    c.addComponent<position>(spawned, x+dx*30.f, y+dy*30.f);
                    auto pTS = c.getComponent<shoots>(e)->pTS;
                    float w = pTS->tileMetas[0]->boxes.back().w;
                    float h = pTS->tileMetas[0]->boxes.back().h;

                    float speed = 60.0f;
                    dx *= speed;
                    dy *= speed;
                    // galilean relativity, baby
                    if(c.hasComponents<velocity>(e)) {
                        dx += c.getComponent<velocity>(e)->x; 
                        dy += c.getComponent<velocity>(e)->y; 
                    }
                    c.addComponent<velocity>(spawned, dx, dy);
                    //c.addComponent<volume>(spawned, w, h);
                    c.addComponent<collide>(spawned);
                    c.addComponent<mass>(spawned, 1.f);
                    c.addComponent<bullet>(spawned, false);
                    c.addComponent<sprite<idle>>(spawned,pTS,0,0,1,idle(0));
                    glog.get() << "[systems::Input]: spawned an entity!\n";
                    // needs spawner systems to despawn entities
                }
                else if(!mouseLeftd) {
                    mousePressing = false;
                }
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
                    cx = c.getComponent<position>(target)->x + c.getComponent<volume>(target)->w/2.0f;
                    cy = c.getComponent<position>(target)->y + c.getComponent<volume>(target)->h/2.0f;
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
    // sprite system: do way too many things
    void Sprite::update(Context& c, SDL_Renderer& r) {
        // order sprites by z
        using ep = std::pair<entity,float>;
        std::vector<ep> eps;
        auto cmp = [](ep ep1, ep ep2) -> bool
        {
            return(ep1.second > ep2.second);
        };
        std::priority_queue pq(cmp,eps);   
        std::priority_queue tops(cmp,eps); // draw last by request
        std::vector<entity> boxes;
        std::vector<entity> cursors;
        for(entity e : c.getEntities()) {
            // idle sprites
            if(c.hasComponents<cursor,sprite<cursor>>(e)) {
                cursors.push_back(e);
            }
            if(c.hasComponents<position,sprite<idle>>(e)) {
                if(c.hasComponents<layer>(e)) {
                    tops.emplace(e,c.getComponent<sprite<idle>>(e)->z + c.getComponent<position>(e)->y);
                }
                else pq.emplace(e,c.getComponent<sprite<idle>>(e)->z + c.getComponent<position>(e)->y);
                if(dbg.showCollision) {
                    boxes.push_back(e);
                }
            }
        }
        // render according to z, applies a camera transform
        while(!pq.empty()) {
            auto [e, zl] = pq.top(); pq.pop();
            auto s = c.getComponent<sprite<idle>>(e);
            float x = c.getComponent<position>(e)->x;
            float y = c.getComponent<position>(e)->y;
            SDL_Texture* tex = s->pTS->tex;
            unsigned& ticks = ++s->s.ticks;
            unsigned row = s->row;
            unsigned& col = s->col;
            if(ticks > 60) {
                col = (col + 1) % s->pTS->numCols;
                ticks = 0;
            }
            SDL_Rect src = s->pTS->get(row,col);
            // transform to camera coordinates (maybe make a method for a full rect)
            SDL_Rect dest;
            auto [cx, cy] = cam.getCameraCoordinates(x,y);
            dest.x = cx;
            dest.y = cy;
            dest.w = src.w*cam.zoom; dest.h = src.h*cam.zoom;
            glog.get() << "[systems::Sprite]: calling SDL_RenderCopy on \n";
            glog.get() << "\tsource = [x = " << src.x << ",y = " << src.y 
                      << ",w = " << src.w << ",h = " << src.h << "]\n";
            glog.get() << "\tdest = [x = " << dest.x << ",y = " << dest.y 
                      << ",w = " << dest.w << ",h = " << dest.h << "]\n";
            SDL_RenderCopy(&r, tex, &src, &dest);
        }
        // always drawn on top of all entities, with the exception of cursors
        while(!tops.empty()) {
            auto [e, zl] = tops.top(); tops.pop();
            auto s = c.getComponent<sprite<idle>>(e);
            float x = c.getComponent<position>(e)->x;
            float y = c.getComponent<position>(e)->y;
            SDL_Texture* tex = s->pTS->tex;
            unsigned& ticks = ++s->s.ticks;
            unsigned row = s->row;
            unsigned& col = s->col;
            if(ticks > 60) {
                col = (col + 1) % s->pTS->numCols;
                ticks = 0;
            }
            SDL_Rect src = s->pTS->get(row,col);
            // transform to camera coordinates
            SDL_Rect dest;
            auto [cx, cy] = cam.getCameraCoordinates(x,y);
            dest.x = cx;
            dest.y = cy;
            dest.w = src.w*cam.zoom; dest.h = src.h*cam.zoom;
            glog.get() << "[systems::Sprite]: calling SDL_RenderCopy on \n";
            glog.get() << "\tsource = [x = " << src.x << ",y = " << src.y 
                      << ",w = " << src.w << ",h = " << src.h << "]\n";
            glog.get() << "\tdest = [x = " << dest.x << ",y = " << dest.y 
                      << ",w = " << dest.w << ",h = " << dest.h << "]\n";
            SDL_RenderCopy(&r, tex, &src, &dest);
        }
        // cursors
        for(entity e : cursors) {
            auto s = c.getComponent<sprite<cursor>>(e);
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
        }
        for(entity e : boxes) {
            float x = c.getComponent<position>(e)->x;
            float y = c.getComponent<position>(e)->y;
            float w = 0.f, h = 0.f;
            if(c.hasComponents<collide>(e)) {
                auto tms = c.getComponent<sprite<idle>>(e)->pTS->tileMetas;
                unsigned row = c.getComponent<sprite<idle>>(e)->row;
                unsigned col = c.getComponent<sprite<idle>>(e)->col;
                //unsigned nRows = c.getComponent<sprite<idle>>(e)->pTS->numRows;
                unsigned nCols = c.getComponent<sprite<idle>>(e)->pTS->numCols;
                // get tile id from row & col
                unsigned id = col + row*nCols;
                assert(tms.count(id) > 0);
                rectf& cr = tms[id]->boxes.back();
                x += cr.x;
                y += cr.y;
                w = cr.w, h = cr.h;
            }
            else if(c.hasComponents<volume>(e)) {
                w = c.getComponent<volume>(e)->w;
                h = c.getComponent<volume>(e)->h;
            }
            SDL_Rect dest;
            auto [cx, cy] = cam.getCameraCoordinates(x,y);
            dest.x = cx;
            dest.y = cy;
            dest.w = w*cam.zoom; dest.h = h*cam.zoom;
            glog.get() << "[systems::Sprite]: calling SDL_RenderCopy on \n";
            glog.get() << "\tdest = [x = " << dest.x << ",y = " << dest.y 
                      << ",w = " << dest.w << ",h = " << dest.h << "]\n";
            SDL_SetRenderDrawColor(&r, 255, 0, 0, 255);
            SDL_RenderDrawRect(&r, &dest);
        }
    }
    // direction system
    //   (velocity) ? (direction)
    void Direction::update(Context& c) {
        for(auto e : c.getEntities()) {
            if(c.hasComponents<direction>(e)) {
                if(c.hasComponents<sprite<idle>>(e)) {
                    auto dir = c.getComponent<direction>(e)->dir;
                    if(dir == direction::facing::left) {
                        c.getComponent<sprite<idle>>(e)->row = 1;
                    }
                    else if(dir == direction::facing::right) {
                        c.getComponent<sprite<idle>>(e)->row = 0;
                    }
                    else if(dir == direction::facing::down) {
                        c.getComponent<sprite<idle>>(e)->row = 3;
                    }
                    else if(dir == direction::facing::up) {
                        c.getComponent<sprite<idle>>(e)->row = 2;
                    }
                }
            }
        }
    }
    // Collision
    void Collision::update(Context& c, float dt) {
        std::vector<std::pair<entity,entity>> collisions;
        // accumulate pairwise collisions
        for(entity e : c.getEntities()) {
            if(c.hasComponents<position,collide,velocity,sprite<idle>>(e)) {
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                float dx = dt * c.getComponent<velocity>(e)->x;
                float dy = dt * c.getComponent<velocity>(e)->y;
                auto tms = c.getComponent<sprite<idle>>(e)->pTS->tileMetas;
                unsigned row = c.getComponent<sprite<idle>>(e)->row;
                unsigned col = c.getComponent<sprite<idle>>(e)->col;
                //unsigned nRows = c.getComponent<sprite<idle>>(e)->pTS->numRows;
                unsigned nCols = c.getComponent<sprite<idle>>(e)->pTS->numCols;
                // get tile id from row & col
                unsigned id = col + row*nCols;
                if(tms.count(id) != 0) {
                    rectf& r = tms[id]->boxes.back();
                    for(entity t : c.getEntities()) {
                        // don't detect a collision with ourselves
                        if(e == t) continue;
                        // has an indexed collision box
                        if(c.hasComponents<position,collide,sprite<idle>>(t)) {
                            float x2 = c.getComponent<position>(t)->x;
                            float y2 = c.getComponent<position>(t)->y;
                            unsigned row2 = c.getComponent<sprite<idle>>(t)->row;
                            unsigned col2 = c.getComponent<sprite<idle>>(t)->col;
                            unsigned nCols2 = c.getComponent<sprite<idle>>(t)->pTS->numCols;
                            unsigned id2 = col2 + row2*nCols2;
                            auto tms2 = c.getComponent<sprite<idle>>(t)->pTS->tileMetas;
                            rectf& r2 = tms2[id2]->boxes.back();
                            if(collision(rectf(x+r.x+dx,y+r.y+dy,r.w,r.h),rectf(x2+r2.x,y2+r2.y,r2.w,r2.h))) {
                                collisions.emplace_back(e,t);
                            }
                        }
                        // has a static collision box (should be just a version of the first case ...)
                        else if(c.hasComponents<position,volume>(t)) {
                            float x2 = c.getComponent<position>(t)->x;
                            float y2 = c.getComponent<position>(t)->y;
                            float w2 = c.getComponent<volume>(t)->w;
                            float h2 = c.getComponent<volume>(t)->h;
                            if(collision(rectf(x+r.x+dx,y+r.y+dy,r.w,r.h),rectf(x2,y2,w2,h2))) {
                                collisions.emplace_back(e,t);
                            }
                        }
                    }
                }
            }
            // can this case be more easily included?
            else if(c.hasComponents<position,volume,velocity>(e)) {
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                float dx = dt * c.getComponent<velocity>(e)->x;
                float dy = dt * c.getComponent<velocity>(e)->y;
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
                            collisions.emplace_back(e,t);
                        }
                    }
                }
            }
        }
        // handle pairwise collisions (should be moved to an appropriate system?)
        for(auto [e1, e2] : collisions) {
            bool moving1 = c.hasComponents<velocity>(e1);
            bool moving2 = c.hasComponents<velocity>(e2);
            bool dragging1 = c.hasComponents<friction>(e1);
            bool dragging2 = c.hasComponents<friction>(e2);
            bool massy1 = c.hasComponents<mass>(e1);
            bool massy2 = c.hasComponents<mass>(e2);
            // bullet
            if(c.hasComponents<bullet>(e1)) c.getComponent<bullet>(e1)->hit = true;
            if(c.hasComponents<bullet>(e2)) c.getComponent<bullet>(e2)->hit = true;
            if(c.hasComponents<bullet>(e1) && c.hasComponents<combat>(e2)) {
                c.getComponent<combat>(e2)->health -= 25;
            }
            else if(c.hasComponents<bullet>(e2) && c.hasComponents<combat>(e1)) {
                c.getComponent<combat>(e1)->health -= 25;
            }
            // combat (just a test)
            if(c.hasComponents<combat>(e1) && c.hasComponents<combat>(e2)) {
                c.getComponent<combat>(e1)->health--;
                c.getComponent<combat>(e2)->health--;
            }
            // elastic collision: at least one is not dragging and both are massy
            if((!dragging1 || !dragging2) && (massy1 && massy2)) {
                float m1 = c.getComponent<mass>(e1)->m;
                float m2 = c.getComponent<mass>(e2)->m;
                auto pVel1 = c.getComponent<velocity>(e1); 
                auto pVel2 = c.getComponent<velocity>(e2); 
                //
                float u1 = pVel1->x*((m1-m2)/(m1+m2)) + pVel2->x*(2.f*m2/(m1+m2));
                float v1 = pVel1->y*((m1-m2)/(m1+m2)) + pVel2->y*(2.f*m2/(m1+m2));
                float u2 = pVel2->x*((m2-m1)/(m1+m2)) + pVel1->x*(2.f*m1/(m1+m2));
                float v2 = pVel2->y*((m2-m1)/(m1+m2)) + pVel1->y*(2.f*m1/(m1+m2));
                //
                pVel1->x = u1; pVel1->y = v1;
                pVel2->x = u2; pVel2->y = v2;
            }
            // just stop the moving entities
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
    void Combat::update(Context& c) {
        for(entity e : c.getEntities()) {
            // bullet hits something? just despawn for now
            if(c.hasComponents<bullet>(e) && c.getComponent<bullet>(e)->hit) c.removeEntity(e);
            // enemies - attack entities that have a combat component
            if(c.hasComponents<position,velocity,enemy>(e)) {
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
                            if(!c.hasComponents<force>(e)) c.addComponent<force>(e,0.f,0.f);
                            c.getComponent<enemy>(e)->s = enemy::state::aggressive;
                            c.getComponent<force>(e)->x = dx/dr/2.0f;
                            c.getComponent<force>(e)->y = dy/dr/2.0f;
                        }
                        else {
                            if(!c.hasComponents<force>(e)) c.addComponent<force>(e,0.f,0.f);
                            c.getComponent<force>(e)->x = 0.f;
                            c.getComponent<force>(e)->y = 0.f;
                            c.getComponent<enemy>(e)->s = enemy::state::passive;
                        }
                    }
                }
            }
        }
    }
    // UI
    void UI::update(Context& c, SDL_Renderer& r) {
        SDL_Color fg = {125,0,0};
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
                // render to screen (needs camera coords, biiiiiiitch)
                float x = c.getComponent<position>(e)->x;
                float y = c.getComponent<position>(e)->y;
                float dx = w/2;
                float dy = h;
                if(c.hasComponents<volume>(e)) {
                    dx -= c.getComponent<volume>(e)->w;
                }
                SDL_Rect src = {0, 0, w, h};
                SDL_Rect dest;
                auto [cX, cY] = cam.getCameraCoordinates(x,y);
                dest.x = cX-dx;
                dest.y = cY-dy;
                dest.w = src.w; dest.h = src.h;
                SDL_RenderCopy(&r,textTexture,&src,&dest);
            }
        }
    }
    // Debug Graphics
    /*
    void DebugGraphics::update(Context& c, SDL_Renderer& r) {
        for(entity e : c.getEntities()) {
            if(c.hasComponents<sprite<debug>>(e)) {
                    float x = c.getComponent<position>(e)->x;
                    float y = c.getComponent<position>(e)->y;
                    SDL_Texture* tex = c.getComponent<sprite<debug>>(e)->pTS->tex;
                    SDL_Rect src = c.getComponent<sprite<debug>>(e)->pTS->get(0,0);
                    // transform to camera coordinates
                    glog.get() << "[systems::DebugGraphics]: calling SDL_RenderCopy on \n";
                    glog.get() << "\tsource & dest = [x = " << src.x << ",y = " << src.y 
                              << ",w = " << src.w << ",h = " << src.h << "]\n";
                    
                    SDL_RenderCopy(&r, tex, &src, &src);
            }
        }
    }
    */
}
