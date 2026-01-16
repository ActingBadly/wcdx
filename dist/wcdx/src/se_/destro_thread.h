//*****************************************destro_thread.h*****************************************
// Custom thread for SE_ code to run on that does not interfere with main thread.
// Used for input remapping and other features that need to run independently of wcdx.
// Still in testing phase.
// Once controls are completed this thread will be tidied up.
//*************************************************************************************************

#pragma once

// Ensure Win32 APIs introduced in Vista (GetTickCount64) are declared.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#if !defined(GetTickCount64)
    // For older SDKs that may not declare GetTickCount64, declare it manually.
    #if defined(__cplusplus)
    extern "C" {
    #endif
        typedef unsigned __int64 ULONGLONG;
        typedef ULONGLONG(WINAPI* GetTickCount64_t)(void);
        static ULONGLONG WINAPI GetTickCount64(void)
        {
            // Fallback implementation for pre-Vista systems
            static DWORD lastTick = 0;
            static ULONGLONG tick64 = 0;
            DWORD now = GetTickCount();
            if (now < lastTick)
                tick64 += (1ULL << 32); // handle wrap
            lastTick = now;
            return (tick64 & 0xFFFFFFFF00000000ULL) | now;
        }
    #if defined(__cplusplus)
    }
    #endif
#endif

#include "wcdx.h"
#include "SE_Controls.h"
#include "key_jacker/KJ_Main.h"
#include "frame_limiter.h"

#include "se_/defines.h"
#include "se_/push_byte.h"
#include "se_/widescreen.h"
static HANDLE DESTRO_Thread;
static volatile LONG DESTRO_Thread_ShouldStop = 0;

inline bool DESTRO_Thread_ShouldTerminate() {
    return InterlockedCompareExchange(&DESTRO_Thread_ShouldStop, 0, 0) != 0;
}

inline void DESTRO_Thread_RequestStop() {
    InterlockedExchange(&DESTRO_Thread_ShouldStop, 1);
}

// File-scope ramp start so other functions can reset it.
static unsigned long long g_movementRampStart = 0;
static unsigned long long g_rollRampStart = 0;
// Per-axis joystick ramp timers to keep X/Y ramps independent
static unsigned long long g_joystickRampStartX = 0;
static unsigned long long g_joystickRampStartY = 0;

// Used for the Mouse Ramping
static bool Up_Down_WasPressed = false;
static bool Up_Key = false;
static bool Down_Key = false;
static bool Left_Right_WasPressed = false;
static bool Left_Key = false;
static bool Right_Key = false;
static bool Comma_Period_WasPressed = false;
static bool Comma_key = false;
static bool Period_key = false;

//DEBUG
static int MAX_ROLL_SPEED = 8;

// value 0x00 causes broken movement so we use 0x01 for positive and 0xFF for negitive
// Value needs to be 0xFF for negitive modifer and 0x01 for positive modifier
uint8_t Modifier(uint8_t value, int modifier) {
    // Treat the stored byte as a signed 8-bit value when applying a
    // positive or negative modifier, then return the resulting raw byte.
    int signedVal = static_cast<int>(static_cast<uint8_t>(value));
    signedVal += modifier;
    // Wrap to 8 bits (preserve two's complement representation)
    return static_cast<uint8_t>(signedVal & 0xFF);
}

static inline int GetRampedRollSpeed()
{
    // If no movement keys are down, reset and return 0
    if (!(Comma_key || Period_key)) {
        g_rollRampStart = 0;
        return 0;
    }

    unsigned long long now = GetTickCount64();
    if (g_rollRampStart == 0) g_rollRampStart = now;

    unsigned long long elapsed = (now > g_rollRampStart) ? (now - g_rollRampStart) : 0;

    // Ensure sensible behavior for small or non-positive configured max
    if (MAX_ROLL_SPEED <= 1) return MAX_ROLL_SPEED > 0 ? MAX_ROLL_SPEED : 1;

    if (elapsed >= MOVEMENT_RAMP_TIME_MS) return MAX_ROLL_SPEED;

    double frac = (double)elapsed / (double)MOVEMENT_RAMP_TIME_MS;
    int val = 1 + static_cast<int>((MAX_ROLL_SPEED - 1) * frac + 0.5);
    if (val < 1) val = 1;
    return val;
}

// Returns a fractional ramp 0.0 .. 1.0 for joystick axes based on how long
// any directional key has been held. Resets to 0.0 on release.
// Get ramp 0..1 for Y axis (Up/Down)
static inline float GetRampedJoystickSpeedY()
{
    if (!(Up_Key || Down_Key)) {
        g_joystickRampStartY = 0;
        return 0.0f;
    }

    unsigned long long now = GetTickCount64();
    if (g_joystickRampStartY == 0) g_joystickRampStartY = now;

    unsigned long long elapsed = (now > g_joystickRampStartY) ? (now - g_joystickRampStartY) : 0;

    double minR = static_cast<double>(MOVEMENT_MIN_EFFECTIVE);
    double maxR = static_cast<double>(MOVEMENT_MAX_SPEED);

    // Keep values in sensible 0..1 range for joystick fractions
    if (maxR > 1.0) maxR = 1.0;
    if (maxR < 0.0) maxR = 0.0;
    if (minR < 0.0) minR = 0.0;
    if (minR > 1.0) minR = 1.0;
    if (maxR < minR) maxR = minR;

    // If ramp time is non-positive, immediately return the maximum value
    double rampTime = static_cast<double>(MOVEMENT_RAMP_TIME_MS);
    if (rampTime <= 0.0) return static_cast<float>(maxR);

    double frac = static_cast<double>(elapsed) / rampTime;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;

    double value = minR + frac * (maxR - minR);
    return static_cast<float>(value);
}

// Get ramp 0..1 for X axis (Left/Right)
static inline float GetRampedJoystickSpeedX()
{
    if (!(Left_Key || Right_Key)) {
        g_joystickRampStartX = 0;
        return 0.0f;
    }

    unsigned long long now = GetTickCount64();
    if (g_joystickRampStartX == 0) g_joystickRampStartX = now;

    unsigned long long elapsed = (now > g_joystickRampStartX) ? (now - g_joystickRampStartX) : 0;

    double minR = static_cast<double>(MOVEMENT_MIN_EFFECTIVE);
    double maxR = static_cast<double>(MOVEMENT_MAX_SPEED);

    if (maxR > 1.0) maxR = 1.0;
    if (maxR < 0.0) maxR = 0.0;
    if (minR < 0.0) minR = 0.0;
    if (minR > 1.0) minR = 1.0;
    if (maxR < minR) maxR = minR;

    double rampTime = static_cast<double>(MOVEMENT_RAMP_TIME_MS);
    if (rampTime <= 0.0) return static_cast<float>(maxR);

    double frac = static_cast<double>(elapsed) / rampTime;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;

    double value = minR + frac * (maxR - minR);
    return static_cast<float>(value);
}

static inline void Reset_RampedSpeedIfNeeded()
{
    if (!(Up_Key || Down_Key)) {
        g_joystickRampStartY = 0;
    }

        if (!(Left_Key || Right_Key)) {
        g_joystickRampStartX = 0;
    }


    if (!(Comma_key || Period_key))
        g_rollRampStart = 0;

}

int Roll_Override() {

    SIZE_T GameAddress1;
    if (Process_Has_Title(L"Wing Commander II: Vengeance of the Kilrathi")) {
        GameAddress1 = 0x931AC;
    }
    else {
        return 0; // Not Wing Commander 2, skip
    }

    // Comma and Period movement
    if (Comma_key && Period_key) {
        Comma_Period_WasPressed = true;
        Push_Byte(2, GameAddress1, { 0x00, 0x00 });
    }
    else if (Comma_key) {
        Comma_Period_WasPressed = true;
        Push_Byte(2, GameAddress1, {(Modifier(0x01, GetRampedRollSpeed())), 0x00}); // Left strafe
    }
    else if (Period_key) {
        Comma_Period_WasPressed = true;
        Push_Byte(2, GameAddress1, {(Modifier(0xFF, -GetRampedRollSpeed())), 0xFF}); // Right strafe
    }
    else {
        if (Comma_Period_WasPressed) {
            // Reset to neutral only if we previously sent movement
            Push_Byte(2, GameAddress1, { 0x00, 0x00 });
            Comma_Period_WasPressed = false;
        }
    }
    return 0;
}


bool Key_Shift = false;

// Main Thread loop for adding in SE_ code
static DWORD WINAPI DestroThread(LPVOID param) {

    // Initialize termination flag
    InterlockedExchange(&DESTRO_Thread_ShouldStop, 0);

    // Reset any mapped controls to null state.
    SE_Controls_Reset();
    // Ensure joystick axes start centered and ramp timers cleared to avoid
    // spurious initial movement. Use dynamic center depending on real
    // joystick presence.

    SE_Joystick_ResetAxesToCenter();
    g_joystickRampStartX = 0;
    g_joystickRampStartY = 0;

// Fire Remapped to Joystick Button 1
// Spacebar on release causes issues with with controls studdering.
    SE_Controls_Map("space",
                    []{
                    },
                    []{
                        KJ_VJoy_SetButton(0, true); // Joystick Button 1 down
                    },
                    []{
                        KJ_VJoy_SetButton(0, false); // Joystick Button 1 up
                    });


// Test as mouse movements still cause issues with C and other keys such as 1,2,3,4,etc.
    SE_Controls_Map("c",
                    []{
                        KJ_Input_InjectKey(0x43, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x43, false); // Joystick Button 1 up
                    });

    SE_Controls_Map("1",
                    []{
                        KJ_Input_InjectKey(0x31, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x31, false); // Joystick Button 1 up
                    });

    SE_Controls_Map("2",
                    []{
                        KJ_Input_InjectKey(0x32, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x32, false); // Joystick Button 1 up
                    });


    SE_Controls_Map("3",
                    []{
                        KJ_Input_InjectKey(0x33, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x33, false); // Joystick Button 1 up
                    });

    SE_Controls_Map("4",
                    []{
                        KJ_Input_InjectKey(0x34, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x34, false); // Joystick Button 1 up
                    });


    SE_Controls_Map("5",
                    []{
                        KJ_Input_InjectKey(0x35, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x35, false); // Joystick Button 1 up
                    });


    SE_Controls_Map("6",
                    []{
                        KJ_Input_InjectKey(0x36, true); // Joystick Button 1 down
                    },
                    []{
                        nullptr;
                    },
                    []{
                        KJ_Input_InjectKey(0x36, false); // Joystick Button 1 up
                    });







    SE_Controls_Map("XboxRT",
                    []{
                        if (Key_Shift) KJ_Input_InjectKey(0x47, true); // G Pressed
                    },
                    []{
                        if (!Key_Shift) KJ_VJoy_SetButton(0, true); // Button 1 down (held)
                    },
                    []{
                        KJ_VJoy_SetButton(0, false); // Button 1 up
                        KJ_Input_InjectKey(0x47, false); // G Released
                    });

// Speed Up
    SE_Controls_Map("minus",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_OEM_MINUS);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_OEM_MINUS, false);
                    });

    SE_Controls_Map("XboxRSDown",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_OEM_MINUS);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_OEM_MINUS, false);
                    });

// Speed Down
    SE_Controls_Map("equals",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_OEM_PLUS);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_OEM_PLUS, false);
                    });
    SE_Controls_Map("XboxRSUp",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_OEM_PLUS);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_OEM_PLUS, false);
                    });


// Roll remappings with ramping.
    SE_Controls_Map("comma",
                    []{
                        Comma_key = true;
                    },
                    []{
                        Comma_key = true;
                    },
                    []{
                        Comma_key = false;
                    });

    SE_Controls_Map("XboxRSLeft",
                    []{
                        Comma_key = true;
                    },
                    []{
                        Comma_key = true;
                    },
                    []{
                        Comma_key = false;
                    });

// Roll remappings with ramping.
    SE_Controls_Map("period",
                    []{
                        Period_key = true;
                    },
                    []{
                        Period_key = true;
                    },
                    []{
                        Period_key = false;
                    });
                    
    SE_Controls_Map("XboxRSRight",
                    []{
                        Period_key = true;
                    },
                    []{
                        Period_key = true;
                    },
                    []{
                        Period_key = false;
                    });

// Afterburner and stearing!
    SE_Controls_Map("tab",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_TAB);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_TAB, false);
                    });

    SE_Controls_Map("XboxLT",
                    []{
                        KJ_Input_InjectKeyFastRepeat(VK_TAB);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_TAB, false);
                    });

// Arrow keys
    // Map arrow keys directly to the virtual joystick axes so games that
    // poll WinMM joystick APIs observe joystick movement when arrow keys
    // are pressed. Pressed => extreme, release => centered.
    SE_Controls_Map("up",
                    []{
                        Up_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter() - (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedY())));
                    },
                    []{
                        Up_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter() - (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedY())));
                    },
                    []{
                        Up_Key = false;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter()));
                    });

    SE_Controls_Map("down",
                    []{
                        Down_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter() + (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedY())));
                    },
                    []{
                        Down_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter() + (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedY())));
                    },
                    []{
                        Down_Key = false;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::Y, static_cast<DWORD>(KJ_GetVirtualJoyCenter()));
                    });

    SE_Controls_Map("left",
                    []{
                        Left_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter() - (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedX())));
                    },
                    []{
                        Left_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter() - (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedX())));
                    },
                    []{
                        Left_Key = false;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter()));
                    });

    SE_Controls_Map("right",
                    []{
                        Right_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter() + (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedX())));
                    },
                    []{
                        Right_Key = true;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter() + (KJ_GetVirtualJoyCenter() * GetRampedJoystickSpeedX())));
                    },
                    []{
                        Right_Key = false;
                        KJ_VJoy_SetAxis(KJ_VJoyAxis::X, static_cast<DWORD>(KJ_GetVirtualJoyCenter()));
                    });




    SE_Controls_Map("XboxStart",
                    []{
                        KJ_Input_InjectKey(VK_ESCAPE, true);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_ESCAPE, false);
                    });

// Shift Modifer
    SE_Controls_Map("XboxBack",
                    []{
                        Key_Shift = true;
                    },
                    []{
                        Key_Shift = true;
                    },
                    []{
                        Key_Shift = false;
                    });

    SE_Controls_Map("XboxA",
                    []{
                        if (Key_Shift) KJ_Input_InjectKey(0x41, true);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x41, false);
                    });



    SE_Controls_Map("XboxB",
                    []{
                        KJ_Input_InjectKey(VK_RETURN, true);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_RETURN, false);
                    });

    SE_Controls_Map("XboxX",
                    []{
                        KJ_Input_InjectKey(0x43, true);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x43, false);
                    });

    SE_Controls_Map("XboxY",
                    []{
                        KJ_Input_InjectKey(0x4E, true);
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x4E, false);
                    });

                    




                    
    SE_Controls_Map("XboxRB",
                    []{
                        if (Key_Shift) KJ_Input_InjectKey(0x57, true); // W (Change Missels)
                        else KJ_Input_InjectKey(VK_RETURN, true); // Fire Missels
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(VK_RETURN, false); // Enter Release
                        KJ_Input_InjectKey(0x57, false); // W Release
                    });

    SE_Controls_Map("XboxLB",
                    []{
                        KJ_Input_InjectKey(0x54, true); // T (Cycle Target)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x54, false); // T Release
                    });

    SE_Controls_Map("XboxRSClick",
                    []{
                        KJ_Input_InjectKey(0x4C, true); // L (Lock Target)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x4C, false); // L Release
                    });

    SE_Controls_Map("XboxDPadUp",
                    []{
                        KJ_Input_InjectKey(0x31, true); // 1 (Coms Option 1)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x31, false); // 1 Release
                    });

    SE_Controls_Map("XboxDPadRight",
                    []{
                        KJ_Input_InjectKey(0x32, true); // 2 (Coms Option 2)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x32, false); // 2 Release
                    });

    SE_Controls_Map("XboxDPadDown",
                    []{
                        KJ_Input_InjectKey(0x33, true); // 3 (Coms Option 3)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x33, false); // 3 Release
                    });

    SE_Controls_Map("XboxDPadLeft",
                    []{
                        KJ_Input_InjectKey(0x34, true); // 4 (Coms Option 4)
                    },
                    nullptr,
                    []{
                        KJ_Input_InjectKey(0x34, false); // 4 Release
                    });

    


// Mouse movement mapping — inject relative mouse movement into the game
    // SE_Controls_Map("mouse_move",
    //                 [](int dx, int dy){
    //                     KJ_Input_InjectMouseMove(dx, dy);
    //                 });

// Mouse Click mappings — use dedicated mouse injection to avoid keyboard/mouse conflicts
    // SE_Controls_Map("mouse_left",
    //                 []{
    //                     Inject_Mouse_Button(0, true); // left down (send once on press)
    //                 },
    //                 []{
    //                     nullptr;
    //                     // No repeated DOWN while held — avoid sending multiple
    //                     // LEFTDOWN events which can confuse the OS/game and
    //                     // lead to stuck/unresponsive mouse state.
    //                 },
    //                 []{
    //                     Inject_Mouse_Button(0, false); // left up
    //                 });

    // SE_Controls_Map("mouse_right",
    //                 []{
    //                     Inject_Mouse_Button(1, true); // right down (send once on press)
    //                 },
    //                 []{
    //                     nullptr;
    //                     // No repeated RIGHTDOWN while held for same reasons
    //                     // as left button.
    //                 },
    //                 []{
    //                     Inject_Mouse_Button(1, false); // right up
    //                 });




// Cycle Space Frame rates
    SE_Controls_Map("f11", 
                    []{
                        Frame_Limiter_Space_Cycle();
                    },
                    []{
                        nullptr;
                    },
                    []{
                        nullptr;
                    });


// Aspect toggle
    SE_Controls_Map("f12", 
                    []{
                        AspectRatioSwitch();
                    },
                    []{
                        nullptr;
                    },
                    []{
                        nullptr;
                    });




// Initial delay to allow game to start up properly before injecting inputs.
// Otherwise the keypesses get stuck down.
Sleep(10);


// Main Thread Loop
    for (;;)
    {
        // Check if we should terminate
        if (DESTRO_Thread_ShouldTerminate())
            break;

        SE_Controls_Read(); // Read the controls for all the keys pressed this loop and process the functions mapped.
        Reset_RampedSpeedIfNeeded();
        Roll_Override(); // Apply roll override based on current key states.
    }
   
   SE_Controls_Reset(); //Cleanup control mappings on thread exit
   return 0;
}
