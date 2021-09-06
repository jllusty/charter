#include <iostream>
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>

#include "loader.hpp"

#include "systems.hpp"

#include "timer.hpp"
#include "logger.hpp"
Logger glog("log.txt");

// const screen settings
const unsigned screenWidth = 1280;
const unsigned screenHeight = 720;
const unsigned screenFPS = 60;
const unsigned screenTickPerFrame = 1000 / screenFPS;

bool handleInput(systems::Input& iSys, SDL_Event event);

int main(int argc, char* argv[]) {
    const std::string resourceDirectory = "resources";
    // Load Game
    Loader loader("resources");
    loader.loadTilemap("forest.tmx", "testmap");

    // Load Window + Graphics
    SDL_Init(SDL_INIT_VIDEO);
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
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    // Load SDL Textures
    loader.populateTilemap("testmap", renderer);
    // Instantiate Game Context
    auto cxt = loader.getTilemapContext("testmap");

    // Init Systems
    systems::Input inputSystem;
    systems::Position positionSystem;
    systems::Sprite spriteSystem;
    systems::Collision collisionSystem;
    systems::Combat combatSystem;

    // Main Loop: cap framerate to 60 FPS
    Timer fpsTimer;
    Timer capTimer;
    int countedFrames = 0;
    fpsTimer.start();
    bool running = true;
    while(running) {
        // start timer to measure how long the "work" takes
        capTimer.start();

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

        // calculate fps
        float avgFPS = countedFrames / (fpsTimer.getTicks() / 1000.f);
        if(avgFPS > 2000000) avgFPS = 0;
        glog.get() << "[main thread]: " << "fps = " << avgFPS << "\n";

        // update systems
        //  these update velocity components, which the collision system uses for resolution
        inputSystem.update(*cxt);
        combatSystem.update(*cxt);
        //  updates velocity components based on results of collision resolution
        collisionSystem.update(*cxt);
        //  updates position unconditionally on velocity
        positionSystem.update(*cxt);

        // draw systems
        SDL_RenderClear(renderer);
        spriteSystem.update(*cxt, *renderer);
        
        // draw game
        SDL_RenderPresent(renderer);
        // update counted frames
        ++countedFrames;
        // frame finished early? wait it out, but don't give the CPU a chance to breath
        while(capTimer.getTicks() < screenTickPerFrame) {}
        glog.get() << "\n";
    }

    // Clear Engine-Requested SDL_Texture memory
    loader.destroySDLTextures();

    // Clear Window + Graphics
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

// pass input to input system (move into systems::Input)
bool handleInput(systems::Input& iSys, SDL_Event event) {
    bool running = true;
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
        case SDLK_ESCAPE:
            running = false;
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
        }
    }
    return running;
}