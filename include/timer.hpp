#pragma once

#include <SDL.h>

class Timer {
    Uint64 previousTicks{ 0 };
    float elapsedSeconds{ 0.0f };

public:
    Timer();
    // clock actions
    void tick();
    float elapsed();
};