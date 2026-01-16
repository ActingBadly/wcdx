//**********************************99*******KJ_Mouse.h*******99***********************************
// Low-Level Mouse Hook and Input Injection System.
// Install via KJ_Main
// 
// Usage:
// KJ_Input_InjectMouseButton(button, down) - Injects a mouse button press/release
// KJ_Input_InjectMouseMove(x, y, absolute) - Injects mouse movement (absolute or relative)
// KJ_Input_InjectMouseSignal()             - Injects a mouse move signal to clear mouse button states
//
//*************************************************************************************************

#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <queue>
#include <unordered_map>

#include "wcdx.h"

#include "se_/SE_Controls.h"

static HHOOK s_mouseHook = nullptr;

// Low-Level Mouse Hook Callback
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) return ::CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
    
    auto ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
    if (!ms) return ::CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
    
    if ((ms->flags & LLMHF_INJECTED) || ms->dwExtraInfo == WCDX_INJECT_TAG)
        return ::CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
    
    int keyCode = 0;
    bool isDown = false;
    
    switch (wParam) {
        case WM_LBUTTONDOWN: keyCode = -1; isDown = true; break;
        case WM_LBUTTONUP:   keyCode = -1; isDown = false; break;
        case WM_MBUTTONDOWN: keyCode = -2; isDown = true; break;
        case WM_MBUTTONUP:   keyCode = -2; isDown = false; break;
        case WM_RBUTTONDOWN: keyCode = -3; isDown = true; break;
        case WM_RBUTTONUP:   keyCode = -3; isDown = false; break;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP: {
            int which = HIWORD(ms->mouseData);
            keyCode = (which == XBUTTON1) ? -4 : -5;
            isDown = (wParam == WM_XBUTTONDOWN);
            break;
        }
        default: return ::CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
    }
    
    // Mouse move intentionally not mapped here
    if (keyCode != 0 && SE_Controls_IsMapped(keyCode)) {
        SE_Controls_NotifyPhysicalKey(keyCode, isDown);
        return 1;
    }
    
    return ::CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
}

static inline bool KJ_Input_InstallMouseHook(HINSTANCE hInstance) {
    if (s_mouseHook) return true;
    s_mouseHook = ::SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    return s_mouseHook != nullptr;
}

static inline void KJ_Input_UninstallMouseHook() {
    if (s_mouseHook) {
        ::UnhookWindowsHookEx(s_mouseHook);
        s_mouseHook = nullptr;
    }
}

// Release all currently held mouse buttons
static void KJ_Mouse_ReleaseAllButtons() {
    // Get all currently held mouse button codes from controls system
    auto heldKeys = SE_Controls_GetCurrentlyHeldKeys();
    for (int keyCode : heldKeys) {
        // Mouse buttons have negative codes
        if (keyCode < 0 && keyCode >= -5) {
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
                default: continue;
            }
            ::SendInput(1, &input, sizeof(input));
        }
    }
}

// Inject a mouse button event (button: 0=left, 1=right, 2=middle)
inline void KJ_Input_InjectMouseButton(int button, bool down) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwExtraInfo = WCDX_INJECT_TAG;
    
    switch (button) {
        case 0: input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
        case 1: input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;
        case 2: input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
        default: return;
    }
    
    ::SendInput(1, &input, sizeof(input));
}

// Inject absolute or relative mouse movement
inline void KJ_Input_InjectMouseMove(int x, int y, bool absolute = false) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwExtraInfo = WCDX_INJECT_TAG;
    
    if (absolute) {
        int sx = GetSystemMetrics(SM_CXSCREEN);
        int sy = GetSystemMetrics(SM_CYSCREEN);
        input.mi.dx = (sx > 0) ? (LONG)((x * 65535) / sx) : 0;
        input.mi.dy = (sy > 0) ? (LONG)((y * 65535) / sy) : 0;
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    } else {
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
    }
    
    ::SendInput(1, &input, sizeof(input));
}

// Inject a small synthetic mouse "signal" (for games that poll mouse movement)
inline void KJ_Input_InjectMouseSignal() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;
    inputs[0].mi.dwExtraInfo = WCDX_INJECT_TAG;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_MOVE;
    inputs[1].mi.dwExtraInfo = WCDX_INJECT_TAG;
    
    for (int i = 0; i < 6; ++i) {
        inputs[0].mi.dx = 2;
        inputs[1].mi.dx = -2;
        ::SendInput(2, inputs, sizeof(INPUT));
        Sleep(2);
    }
}
