#include "timer.hpp"

Timer::Timer() {
    previousTicks = 0;
    elapsedSeconds = 0.f;
}

void Timer::tick() {
    const Uint64 currentTicks{ SDL_GetPerformanceCounter() };
    const Uint64 delta{ currentTicks - previousTicks};
    previousTicks = currentTicks;
    static const Uint64 ticksPerSecond{ SDL_GetPerformanceFrequency() };
    elapsedSeconds = delta / static_cast<float>(ticksPerSecond);
}

float Timer::elapsed() {
    return elapsedSeconds;
}