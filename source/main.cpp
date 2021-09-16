// entry code through SDL layer

#include <iostream>
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "loader.hpp"

#include "systems.hpp"

#include "timer.hpp"
#include "logger.hpp"
Logger glog("log.txt");

// const screen settings
const unsigned screenWidth = 1280;
const unsigned screenHeight = 720;

// handles a single event
bool handleInput(systems::Input& iSys, SDL_Event event);

int main(int argc, char* argv[]) {
    const std::string resourceDirectory = "resources";
    // Load Game
    Loader loader("resources");
    loader.loadTilemap("forest.tmx", "testmap");

    // Load Window + Graphics
    SDL_Init(SDL_INIT_VIDEO);
    // initialize SDL_ttf
    if(TTF_Init()==-1) {
        glog.get() << "TTF_Init: " << TTF_GetError() << "\n";
    }
    // initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) && imgFlags)) {
        glog.get() << "[main thread]: SDL_image could not initialize loading PNG: " << IMG_GetError() << "\n";
    }
    SDL_Window* window = nullptr;
    window = SDL_CreateWindow("E M B A R K",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                     screenWidth,screenHeight, SDL_WINDOW_SHOWN);
    if(window == nullptr) {
        glog.get() << "SDL failed SDL_CreateWindow()\n";
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(renderer == nullptr) {
        glog.get() << "SDL failed SDL_CreateRenderer()\n";
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    // Load SDL Textures
    loader.populateTilemap("testmap", renderer);
    // Instantiate Game Context
    auto cxt = loader.getTilemapContext("testmap");

    // Init Systems
    // input
    systems::Input inputSystem;
    // physics
    systems::Position positionSystem;
    systems::Velocity velocitySystem;
    systems::Acceleration accelerationSystem;
    systems::Collision collisionSystem;
    // entity state
    systems::Direction directionSystem;
    systems::Combat combatSystem;
    // rendering
    systems::Sprite spriteSystem;
    systems::UI uiSystem;

    //systems::DebugGraphics dgSystem;

    // TTF tests
    TTF_Font* font = TTF_OpenFont("resources\\Azeret_Mono\\static\\AzeretMono-Black.ttf",26);
    if(font == nullptr) {
        glog.get() << "[main thread]: Could not load font. TTF_OpenFont: " << TTF_GetError() << "\n";
    }
    uiSystem.font = font;

    // Main Loop: cap framerate to 60 FPS
    Timer fpsTimer;
    Timer capTimer;
    bool running = true;
    float accumulatedSeconds = 0.f;
    const int updateFrequency{ 60 };
    const float cycleTime{ 1.0f/updateFrequency };
    while(running) {
        // start timer to measure how long the "work" takes
        capTimer.tick();
        accumulatedSeconds += capTimer.elapsed();

        SDL_Event event;
        // poll until all events are handled
        while(SDL_PollEvent(&event) != 0) {
            // decide what to do with this event
            if(event.type == SDL_QUIT) {
                running = false;
            }
            else {
                if(handleInput(inputSystem,event) == false) {
                    running = false;
                }
            }
        }

        if(std::isgreater(accumulatedSeconds,cycleTime))
        {
            accumulatedSeconds = -cycleTime;
            // calculate fps
            //float avgFPS = countedFrames / (fpsTimer.getTicks() / 1000.f);
            //if(avgFPS > 2000000) avgFPS = 0;
            // report fps
            //glog.get() << "[main thread]: " << "fps = " << avgFPS << "\n";

            // calculate timestep for physics
            //Uint32 currentTicks = fpsTimer.getTicks();
            //dt = (currentTicks - lastTicks) / 1000.f;
            //lastTicks = currentTicks;

            // update systems
            static Timer physicsTimer;
            physicsTimer.tick();
            float dt = physicsTimer.elapsed();
            //  these update velocity components, which the collision system uses for resolution
            inputSystem.update(*cxt);
            accelerationSystem.update(*cxt);
            velocitySystem.update(*cxt,dt);
            directionSystem.update(*cxt);
            combatSystem.update(*cxt);
            //  updates velocity components based on results of collision resolution
            collisionSystem.update(*cxt,dt);
            //  updates position unconditionally on velocity
            positionSystem.update(*cxt,dt);

            // turn on debug on
            systems::dbg.showCollision = true;

            // update camera
            systems::cam.update(*cxt,*renderer);

            // draw systems
            SDL_RenderClear(renderer);
            spriteSystem.update(*cxt, *renderer);
            uiSystem.update(*cxt, *renderer);
            //dgSystem.update(*cxt,*renderer);

            // draw game
            SDL_RenderPresent(renderer);
            // update counted frames
            //++countedFrames;
            //glog.get() << "\n";
            // wait until exactly 1/60 seconds has passed
            //while(capTimer.getTicks() < screenTickPerFrame) {}
        } 
    }

    // Clear Engine-Requested SDL_Texture memory
    loader.destroySDLTextures();

    // clear fonts
    TTF_CloseFont(font);

    // Clear Window + Graphics
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}

// pass input to input system (move into systems::Input)
bool handleInput(systems::Input& iSys, SDL_Event event) {
    bool running = true;
    if(event.type == SDL_MOUSEMOTION) {
        iSys.mouseX = event.motion.x;
        iSys.mouseY = event.motion.y;
    }
    if(event.type == SDL_MOUSEBUTTONDOWN) {
        if(event.button.button == SDL_BUTTON_LEFT) {
            iSys.mouseLeftd = true;
        }
        else if(event.button.button == SDL_BUTTON_RIGHT) {
            iSys.mouseRightd = true;
        }
    }
    if(event.type == SDL_MOUSEBUTTONUP) {
        if(event.button.button == SDL_BUTTON_LEFT) {
            iSys.mouseLeftd = false;
        }
        else if(event.button.button == SDL_BUTTON_RIGHT) {
            iSys.mouseRightd = false;
        }
    }
    if(event.type == SDL_KEYDOWN) {
        switch(event.key.keysym.sym) {
        case SDLK_w:
            iSys.Wd = true;
            break;
        case SDLK_a:
            iSys.Ad = true;
            break;
        case SDLK_s:
            iSys.Sd = true;
            break;
        case SDLK_d:
            iSys.Dd = true;
            break;
        case SDLK_UP:
            iSys.upArr = true;
            break;
        case SDLK_DOWN:
            iSys.downArr = true;
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
        }
    }
    if(event.type == SDL_KEYUP) {
        switch(event.key.keysym.sym) {
        case SDLK_w:
            iSys.Wd = false;
            break;
        case SDLK_a:
            iSys.Ad = false;
            break;
        case SDLK_s:
            iSys.Sd = false;
            break;
        case SDLK_d:
            iSys.Dd = false;
            break;
        case SDLK_UP:
            iSys.upArr = false;
            break;
        case SDLK_DOWN:
            iSys.downArr = false;
            break;
        }
    }
    return running;
}