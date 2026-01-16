//*****************************************KJ_Keyboard.h******************************************
// Low-Level Keyboard Hook and Input Injection System.
// Install via KJ_Main
// 
// Usage:
// KJ_Input_InjectKey(keyCode, down)      - Injects a key press/release
// KJ_Input_InjectKeyFastRepeat(keyCode)  - Injects a key press with auto-repeat until released
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

static HHOOK s_keyboardHook = nullptr;

// Input Injection State
static CRITICAL_SECTION s_injectLock;
static std::queue<INPUT> s_injectQueue;
static HANDLE s_injectEvent = nullptr;
static HANDLE s_injectThread = nullptr;
static bool s_injectThreadInit = false;
static volatile LONG s_injectStop = 0;

struct InjectedKeyInfo {
    DWORD initialTick = 0;
    DWORD lastRepeatTick = 0;
    bool initialConsumed = false;
};

static std::unordered_map<UINT, InjectedKeyInfo> s_injectedKeys;
static DWORD s_initialDelayMs = 500;
static DWORD s_repeatIntervalMs = 50;

// Low-Level Keyboard Hook Callback
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) return ::CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
    
    auto kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    if (!kb) return ::CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
    
    // Ignore injected input and our own tagged events
    if ((kb->flags & LLKHF_INJECTED) || kb->dwExtraInfo == WCDX_INJECT_TAG)
        return ::CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
    
    // Swallow mapped keys, notify controls system
    if (SE_Controls_IsMapped(static_cast<int>(kb->vkCode))) {
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        SE_Controls_NotifyPhysicalKey(static_cast<int>(kb->vkCode), down);
        return 1; // Block event from reaching app
    }
    
    return ::CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
}

// Input Injection Thread (used by keyboard injection)
static DWORD WINAPI KJ_InjectThreadProc(LPVOID) {
    for (;;) {
        // Check if we have keys being held down
        EnterCriticalSection(&s_injectLock);
        bool hasInjectedKeys = !s_injectedKeys.empty();
        LeaveCriticalSection(&s_injectLock);
        
        // Wait with timeout if we have keys to repeat, otherwise wait indefinitely
        DWORD waitTime = hasInjectedKeys ? (s_repeatIntervalMs / 4) : INFINITE;
        if (waitTime == 0) waitTime = 1;
        
        DWORD waitResult = WaitForSingleObject(s_injectEvent, waitTime);
        if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT) break;
        if (InterlockedCompareExchange(&s_injectStop, 0, 0) != 0) break;

        // Drain input queue
        while (true) {
            EnterCriticalSection(&s_injectLock);
            if (s_injectQueue.empty()) {
                ResetEvent(s_injectEvent);
                LeaveCriticalSection(&s_injectLock);
                break;
            }
            INPUT inp = s_injectQueue.front();
            s_injectQueue.pop();
            LeaveCriticalSection(&s_injectLock);
            ::SendInput(1, &inp, sizeof(INPUT));
        }

        // Auto-repeat for injected keys
        if (InterlockedCompareExchange(&s_injectStop, 0, 0) != 0) break;
        
        EnterCriticalSection(&s_injectLock);
        if (!s_injectedKeys.empty()) {
            DWORD now = GetTickCount();
            
            for (auto& kv : s_injectedKeys) {
                UINT vk = kv.first;
                InjectedKeyInfo& info = kv.second;
                
                INPUT inp = {};
                inp.type = INPUT_KEYBOARD;
                inp.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
                inp.ki.dwFlags = KEYEVENTF_SCANCODE;
                inp.ki.dwExtraInfo = WCDX_INJECT_TAG;
                
                if (!info.initialConsumed && now - info.initialTick >= s_initialDelayMs) {
                    ::SendInput(1, &inp, sizeof(INPUT));
                    info.initialConsumed = true;
                    info.lastRepeatTick = now;
                } else if (info.initialConsumed && now - info.lastRepeatTick >= s_repeatIntervalMs) {
                    ::SendInput(1, &inp, sizeof(INPUT));
                    info.lastRepeatTick = now;
                }
            }
        }
        LeaveCriticalSection(&s_injectLock);
    }
    return 0;
}

static void EnsureInjectThread() {
    if (s_injectThreadInit) return;
    
    InterlockedExchange(&s_injectStop, 0);
    InitializeCriticalSection(&s_injectLock);
    
    UINT delayVal = 0, speedVal = 0;
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &delayVal, 0);
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &speedVal, 0);
    
    s_initialDelayMs = (delayVal + 1) * 250;
    double charsPerSec = 2.0 + (static_cast<double>(speedVal) * (28.0 / 31.0));
    s_repeatIntervalMs = static_cast<DWORD>(1000.0 / (charsPerSec < 1.0 ? 1.0 : charsPerSec));
    
    s_injectEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    s_injectThread = CreateThread(nullptr, 0, KJ_InjectThreadProc, nullptr, 0, nullptr);
    s_injectThreadInit = true;
}

static inline bool KJ_Input_InstallKeyboardHook(HINSTANCE hInstance) {
    if (s_keyboardHook) return true;
    s_keyboardHook = ::SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    return s_keyboardHook != nullptr;
}

static inline void KJ_Input_UninstallKeyboardHook() {
    if (s_keyboardHook) {
        ::UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = nullptr;
    }
}

// Release all currently injected keys
static void KJ_Keyboard_ReleaseAllKeys() {
    if (!s_injectThreadInit) return;
    
    EnterCriticalSection(&s_injectLock);
    for (const auto& pair : s_injectedKeys) {
        UINT vk = pair.first;
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
        input.ki.wVk = static_cast<WORD>(vk);
        input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        input.ki.dwExtraInfo = WCDX_INJECT_TAG;
        ::SendInput(1, &input, sizeof(input));
        // Also send VK-based keyup for extra safety
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        ::SendInput(1, &input, sizeof(input));
    }
    s_injectedKeys.clear();
    LeaveCriticalSection(&s_injectLock);
}

// Shutdown injection thread and clear all state
static void KJ_Keyboard_Shutdown() {
    // Release all keys before stopping thread
    KJ_Keyboard_ReleaseAllKeys();
    
    InterlockedExchange(&s_injectStop, 1);
    if (s_injectEvent) SetEvent(s_injectEvent);
    
    if (s_injectThread) {
        WaitForSingleObject(s_injectThread, 2000);
        CloseHandle(s_injectThread);
        s_injectThread = nullptr;
    }
    if (s_injectEvent) {
        CloseHandle(s_injectEvent);
        s_injectEvent = nullptr;
    }
    
    if (s_injectThreadInit) {
        EnterCriticalSection(&s_injectLock);
        while (!s_injectQueue.empty()) s_injectQueue.pop();
        s_injectedKeys.clear();
        LeaveCriticalSection(&s_injectLock);
        DeleteCriticalSection(&s_injectLock);
    }
    s_injectThreadInit = false;
}

// Inject a low-level keyboard event
static void KJ_Input_InjectKey(UINT vk, bool down) {
    EnsureInjectThread();
    
    EnterCriticalSection(&s_injectLock);
    
    if (down) {
        if (s_injectedKeys.count(vk)) {
            LeaveCriticalSection(&s_injectLock);
            return;
        }
        InjectedKeyInfo info;
        info.initialTick = GetTickCount();
        info.initialConsumed = false;
        info.lastRepeatTick = info.initialTick;
        s_injectedKeys[vk] = info;
    } else {
        s_injectedKeys.erase(vk);
    }
    
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    input.ki.dwExtraInfo = WCDX_INJECT_TAG;
    s_injectQueue.push(input);
    
    LeaveCriticalSection(&s_injectLock);
    SetEvent(s_injectEvent);
}

// Inject a key-down and start auto-repeat immediately
static void KJ_Input_InjectKeyFastRepeat(UINT vk) {
    EnsureInjectThread();
    const DWORD now = GetTickCount();
    
    EnterCriticalSection(&s_injectLock);
    
    auto it = s_injectedKeys.find(vk);
    if (it == s_injectedKeys.end()) {
        InjectedKeyInfo info;
        info.initialTick = now;
        info.initialConsumed = true;
        info.lastRepeatTick = (s_repeatIntervalMs > 0 && now >= s_repeatIntervalMs) 
            ? (now - s_repeatIntervalMs) : 0;
        s_injectedKeys[vk] = info;
        
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
        input.ki.dwExtraInfo = WCDX_INJECT_TAG;
        s_injectQueue.push(input);
    } else {
        it->second.initialConsumed = true;
        it->second.lastRepeatTick = (s_repeatIntervalMs > 0 && now >= s_repeatIntervalMs) 
            ? (now - s_repeatIntervalMs) : 0;
    }
    
    LeaveCriticalSection(&s_injectLock);
    SetEvent(s_injectEvent);
}
