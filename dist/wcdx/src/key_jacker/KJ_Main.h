//*******************************************KJ_Main.h********************************************
// Main header for Key_Jacker input injection and hooking system.
// Hooks keyboard, mouse, and joystick input to allow for remapping and virtual device injection.
// Added XInput hooking to support Xbox controller remapping.
// 
// Install Function:
// #include "key_jacker/KJ_Main.h"
// 
// KJ_Input_Install(HINSTANCE hInstance)
//
// Uninstall Function:
// KJ_Input_Uninstall()
//*************************************************************************************************

#pragma once

#include <windows.h>
#include "wcdx.h"
#include "se_/SE_Controls.h"
#include "key_jacker/KJ_Keyboard.h"
#include "key_jacker/KJ_Mouse.h"
#include "key_jacker/KJ_Joystick.h"
#include "key_jacker/KJ_Xinput.h"

// Compatibility for missing SDK flags
#ifndef LLKHF_INJECTED
#define LLKHF_INJECTED 0x00000010
#endif
#ifndef LLMHF_INJECTED
#define LLMHF_INJECTED 0x00000001
#endif

//=============================================================================
// Input System Management
//=============================================================================

// Release all currently mapped physical keys to prevent stuck keys
static void KJ_Input_ReleaseAllPhysicalKeys() {
    auto heldKeys = SE_Controls_GetCurrentlyHeldKeys();
    for (int keyCode : heldKeys) {
        // Keyboard keys
        if (keyCode > 0 && keyCode < 256) {
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(keyCode);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            input.ki.dwExtraInfo = WCDX_INJECT_TAG;
            ::SendInput(1, &input, sizeof(input));
            // Also send scan code keyup for safety
            input.ki.wScan = static_cast<WORD>(MapVirtualKey(keyCode, MAPVK_VK_TO_VSC));
            input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            ::SendInput(1, &input, sizeof(input));
            Sleep(1);
        }
        // Mouse buttons (negative codes -1 to -5)
        else if (keyCode >= -5 && keyCode < 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwExtraInfo = WCDX_INJECT_TAG;
            switch (keyCode) {
                case -1: input.mi.dwFlags = MOUSEEVENTF_LEFTUP; break;
                case -2: input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
                case -3: input.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break;
                case -4: // XBUTTON1
                case -5: // XBUTTON2
                    input.mi.dwFlags = MOUSEEVENTF_XUP;
                    input.mi.mouseData = (keyCode == -4) ? XBUTTON1 : XBUTTON2;
                    break;
            }
            ::SendInput(1, &input, sizeof(input));
            Sleep(1);
        }
    }
}

// Install all input hooks and start input handling
static inline void KJ_Input_Install(HINSTANCE hInstance) {

    // Clear any stuck keys/buttons from previous run
    KJ_Input_ReleaseAllPhysicalKeys();
    KJ_Keyboard_ReleaseAllKeys();
    KJ_Mouse_ReleaseAllButtons();
    KJ_ResetVirtualJoystick();

    // Install hooks
    KJ_Input_InstallKeyboardHook(hInstance);
    KJ_Input_InstallMouseHook(hInstance);
    KJ_Input_Install_Joystick();
    InstallXInputIATHooks();
    InstallGetProcAddressIATHook();
}

// Uninstall all hooks and clear all input states
static inline void KJ_Input_Uninstall() {
    // Uninstall hooks first
    KJ_Input_UninstallKeyboardHook();
    KJ_Input_UninstallMouseHook();
    
    // Clear all input states
    KJ_Input_ReleaseAllPhysicalKeys();
    KJ_Keyboard_ReleaseAllKeys();
    KJ_Mouse_ReleaseAllButtons();
    KJ_ResetVirtualJoystick();
    
    // Shutdown subsystems
    KJ_Keyboard_Shutdown();
    KJ_Joystick_Shutdown();
    KJ_XInput_Shutdown();
}

