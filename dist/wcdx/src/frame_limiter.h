//*****************************************frame_limiter.h*****************************************
// Frame Limiter that can be used to limit the framerate of the main thread.
//
// Usage:
// Frame_Limiter_Set_Target(fps) - Sets the target framerate (default 30 fps)
// Frame_Limiter()               - Call at the end of each frame to enforce the framerate limit.
//
//*************************************************************************************************

#pragma once
#include <atomic>
#include <cmath>
#include <functional>

#include <Windows.h>
#include <unordered_set>
#include <mutex>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

// Default value for frame limiter target fps.
inline int FRAME_MAX = 30;

//============================================ Frame Limiter ============================================

struct FrameLimiter
{
        std::atomic<int> targetFps{ FRAME_MAX };
        LARGE_INTEGER freq{};
        LONGLONG lastTick{ 0 };

        FrameLimiter()
        {
            timeBeginPeriod(1);
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTick));
        }

        ~FrameLimiter()
        {
            timeEndPeriod(1);
        }

        void onFrame(std::function<void(DWORD)> sleeper)
        {
            if (targetFps.load() <= 0)
            {
                QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTick));
                return;
            }

            LARGE_INTEGER nowLI;
            QueryPerformanceCounter(&nowLI);
            double elapsedMs = double(nowLI.QuadPart - lastTick) * 1000.0 / double(freq.QuadPart);
            double targetMs = 1000.0 / double(targetFps.load());
            if (elapsedMs < targetMs)
            {
                double remaining = targetMs - elapsedMs;

                if (remaining > 2.0)
                {
                    DWORD sleepMs = static_cast<DWORD>(std::floor(remaining)) - 1;
                    if (sleepMs > 0)
                        sleeper(sleepMs);
                }

                do
                {
                    QueryPerformanceCounter(&nowLI);
                    elapsedMs = double(nowLI.QuadPart - lastTick) * 1000.0 / double(freq.QuadPart);
                } while (elapsedMs < targetMs);
            }

            lastTick = nowLI.QuadPart;
        }
    };

inline FrameLimiter g_frameLimiter;

inline void Frame_Limiter_Set_Target(int fps) { 
    g_frameLimiter.targetFps.store(fps); 
}

inline void Frame_Limiter(){
    g_frameLimiter.onFrame([](DWORD ms){ ::Sleep(ms); });
}