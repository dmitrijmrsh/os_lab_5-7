#pragma once

#include <chrono>
#include <string>

std::chrono::system_clock::time_point start;
std::chrono::system_clock::time_point stop;
int countStopMilliseconds = -1;
bool stopped = false;
bool started = false;

typedef const char* Error;

std::string HandleStart();

Error StartTimer() {
    if (started && !stopped) {
        return "Timer is already running";
    }
    if (started && stopped) {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> elapsed = now - stop;
        countStopMilliseconds += int(elapsed.count());
    }
    if (!started) {
        start = std::chrono::system_clock::now();
        countStopMilliseconds = -1;
    }
    stopped = false;
    started = true;

    return nullptr;
}

Error StopTimer() {
    if (!started) {
        return "Timer has not been starter yet";
    }

    if (stopped) {
        return "Timer is already stopped";
    }

    if (countStopMilliseconds == -1) {
        countStopMilliseconds = 0;
    }
    stop = std::chrono::system_clock::now();
    stopped = true;

    return nullptr;
}

int GetTime() {
    int result;
    if (countStopMilliseconds == -1) {
        if (started) {
            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> elapsed = now - start;
            result = int(elapsed.count());
        } else {
            result = 0;
        }
    } else {
        if (stopped) {
            std::chrono::duration<double, std::milli> elapsed = stop - start;
            result = int(elapsed.count());
            result -= countStopMilliseconds;
        } else {
            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> elapsed = now - start;
            result = int(elapsed.count());
            result -= countStopMilliseconds;
        }
    }
    return result;
}