//*****************************************frame_limiter.h*****************************************
// Frame Limiter that can be used to limit the framerate of the main thread.
// Also a push byte to the game to modify the space framerate for Wing Commander 2.
// Games math (# * 1000) / 60;
//*************************************************************************************************

#pragma once
#include <atomic>
#include <cmath>
#include <functional>

#include <Windows.h>
#include <unordered_set>
#include <mutex>
#include <mmsystem.h>

#include "se_/push_byte.h"

#pragma comment(lib, "winmm.lib")

// Defauly value for frame limiter target fps.
inline int FRAME_MAX = 30;
inline int SPACE_MAX = 3;

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

inline void Frame_Limiter_Set_Target(int fps) { g_frameLimiter.targetFps.store(fps); }

inline void Frame_Limiter(){
    g_frameLimiter.onFrame([](DWORD ms){ ::Sleep(ms); });
}

//============================================ Space Limiter ============================================

inline void Frame_Limiter_Space(int modifer)
{
    if (modifer >= 0 && modifer <= 4)
    {
        if (Process_Has_Title(L"Wing Commander II: Vengeance of the Kilrathi")) {
                if (modifer == 0) Push_Byte(1, 0x006944F, {0x00}); // Frame Rate Unlimited
                else if (modifer == 1) Push_Byte(1, 0x006944F, {0x01}); // Frame Rate 60
                else if (modifer == 2) Push_Byte(1, 0x006944F, {0x02}); // Frame Rate 30
                else if (modifer == 3) Push_Byte(1, 0x006944F, {0x03}); // Frame Rate 20
                else if (modifer == 4) Push_Byte(1, 0x006944F, {0x04}); // Frame Rate 15
        }
        else if (Process_Has_Title(L"Wing Commander II: Special Operations II")) {
            
                if (modifer == 0) Push_Byte(1, 0x002EEDE, {0x00}); // Frame Rate Unlimited
                else if (modifer == 1) Push_Byte(1, 0x002EEDE, {0x01}); // Frame Rate 60
                else if (modifer == 2) Push_Byte(1, 0x002EEDE, {0x02}); // Frame Rate 30
                else if (modifer == 3) Push_Byte(1, 0x002EEDE, {0x03}); // Frame Rate 20
                else if (modifer == 4) Push_Byte(1, 0x002EEDE, {0x04}); // Frame Rate 15
        }
        else if (Process_Has_Title(L"Wing Commander II: Special Operations I")) {
                if (modifer == 0) Push_Byte(1, 0x003C86E, {0x00}); // Frame Rate Unlimited
                else if (modifer == 1) Push_Byte(1, 0x003C86E, {0x01}); // Frame Rate 60
                else if (modifer == 2) Push_Byte(1, 0x003C86E, {0x02}); // Frame Rate 30
                else if (modifer == 3) Push_Byte(1, 0x003C86E, {0x03}); // Frame Rate 20
                else if (modifer == 4) Push_Byte(1, 0x003C86E, {0x04}); // Frame Rate 15
        }
        SPACE_MAX = modifer;
    }
    else {
        // Invalid modifier, reset to 0
        if (Process_Has_Title(L"Wing Commander II: Vengeance of the Kilrathi")) {
        Push_Byte(1, 0x006944F, {0x00}); // Frame Rate Unlimited
        }
        else if (Process_Has_Title(L"Wing Commander II: Special Operations II")) {
        Push_Byte(1, 0x002EEDE, {0x00}); // Frame Rate Unlimited
        }  
        else if (Process_Has_Title(L"Wing Commander II: Special Operations I")) {
        Push_Byte(1, 0x003C86E, {0x00}); // Frame Rate Unlimited
        }   
        SPACE_MAX = 0;
    }
}

inline void Frame_Limiter_Space_Cycle()
{
    Frame_Limiter_Space(SPACE_MAX + 1);
}