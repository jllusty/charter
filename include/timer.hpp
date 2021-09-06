#pragma once

#include <SDL.h>

class Timer {
    Uint32 startTicks;
    Uint32 pausedTicks;
    bool paused;
    bool started;
public:
    Timer();
    // clock actions
    void start();
    void stop();
    void pause();
    void unpause();
    // timer's time
    Uint32 getTicks();
    // check status of timer
    bool isStarted();
    bool isPaused();
};