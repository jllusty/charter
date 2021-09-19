// SDL layer
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

// generates context using tilemap & tileset files
#include "loader.hpp"
// ECS systems, instantiated here
#include "systems.hpp"

// game timer
#include "timer.hpp"
// game logging stream instance (extern-ed to loader and system updates)
#include "logger.hpp"
Logger glog("log.txt");

// const screen settings
const unsigned screenWidth = 1280;
const unsigned screenHeight = 720;

// handles a single event
bool handleInput(systems::Input& iSys, SDL_Event event);

// ENTRY POINT
int main(int argc, char* argv[]) {
    // set resource directory
    const std::string resourceDirectory = "resources";
    // Load Game
    Loader loader("resources");
    loader.loadTilemap("forest.tmx", "testmap");

    // Load Window + Graphics
    SDL_Init(SDL_INIT_VIDEO);
    // turn off cursor
    // SDL_ShowCursor(SDL_DISABLE);
    // initialize SDL_ttf
    if(TTF_Init()==-1) {
        glog.get() << "TTF_Init: " << TTF_GetError() << "\n";
    }
    // initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) && imgFlags)) {
        glog.get() << "[main thread]: SDL_image could not initialize loading PNG: " << IMG_GetError() << "\n";
    }
    // create SDL window & SDL renderer
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

    // Game: Load tilesets into SDL Textures
    loader.populateTilemap("testmap", renderer);
    // Game: Instantiate Room Context for selected Tilemap
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
    systems::CombatAI combatSystem;

    // TTF tests
    TTF_Font* font = TTF_OpenFont("resources\\Azeret_Mono\\static\\AzeretMono-Black.ttf",26);
    if(font == nullptr) {
        glog.get() << "[main thread]: Could not load font. TTF_OpenFont: " << TTF_GetError() << "\n";
    }
    systems::ui.font = font;

    // Main Loop: cap framerate to 60 FPS
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

        // do not throw exception on floating-point comparison for timer
        if(std::isgreater(accumulatedSeconds,cycleTime))
        {
            // reset accumulator
            accumulatedSeconds = 0.f;

            // update systems
            static Timer physicsTimer;
            physicsTimer.tick();
            float dt = physicsTimer.elapsed();
            glog.get() << "[main thread]: dt = " << dt << "\n";
            //  these update velocity components, which the collision system uses for resolution
            inputSystem.update(*cxt);
            accelerationSystem.update(*cxt);
            velocitySystem.update(*cxt,dt);
            //  TODO(jllusty): when entities have velocities set, be careful about the order of operations
            //                 so that they are not decellarated to a velocity below their maximal velocity
            directionSystem.update(*cxt);
            systems::bul.update(*cxt);
            combatSystem.update(*cxt);
            //  updates velocity components based on results of collision resolution
            collisionSystem.update(*cxt);
            collisionSystem.resolve(*cxt,dt);
            //  updates position unconditionally on velocity
            positionSystem.update(*cxt,dt);

            // update camera
            systems::cam.update(*cxt,*renderer);
            // draw systems
            SDL_RenderClear(renderer);
            systems::spr.update(*cxt);
            systems::ui.update(*cxt, *renderer);
            systems::graphics.update(*renderer);

            // draw game
            SDL_RenderPresent(renderer);
            // newline in log
            glog.get() << "\n";
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
        case SDLK_BACKQUOTE:
            iSys.debugToggle = true;
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
        case SDLK_BACKQUOTE:
            iSys.debugToggle = false;
            break;
        }
    }
    return running;
}