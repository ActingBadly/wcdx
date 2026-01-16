//********************************************defines.h********************************************
// Currently used for helper functions and other values I have yet to place.
//*************************************************************************************************

#pragma once

#include <windows.h>

// Minimum joystick fraction that will be reported when a movement key is held.
// Values below this may not register in some games. Default 0.1 (10%).
inline float MOVEMENT_MIN_EFFECTIVE = 0.1f;
// Max movement speed for keyboad controls.
// Can be lower that 1.0 but not higher. Default 1.0 (100%).
inline float MOVEMENT_MAX_SPEED = 1.0f;
// Ramp time in milliseconds to reach max movement speed.
inline int MOVEMENT_RAMP_TIME_MS = 1000;
// Time in milliseconds of inactivity before hiding cursor. Use 64-bit so it
// can be compared directly with GetTickCount64() without implicit narrowing.
inline unsigned long long HIDE_MOUSE_CURSOR_IN_GAME_TIME = 3000ULL;

// Global inline variable controlling whether the in-game cursor is hidden or not.
// This flag is managed by the main rendering loop in wcdx.cpp.
inline bool HIDE_MOUSE_CURSOR_IN_GAME = false;

// Helper for Process_Has_Title
struct Window_EnumData { const wchar_t* expected; DWORD pid; bool found; };

inline BOOL CALLBACK Push_Byte_EnumProc(HWND hwnd, LPARAM lparam)
{
    Window_EnumData* d = reinterpret_cast<Window_EnumData*>(lparam);
    DWORD wpid = 0;
    GetWindowThreadProcessId(hwnd, &wpid);
    if (wpid != d->pid) return TRUE;
    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t buf[512] = {};
    int len = GetWindowTextW(hwnd, buf, (int)std::size(buf));
    if (len > 0) {
        if (wcsstr(buf, d->expected) != nullptr) {
            d->found = true;
            return FALSE;
        }
    }
    return TRUE;
}

// Retrun a true or false if the current process has a window with the given title.
inline bool Process_Has_Title(const wchar_t* expectedSubstring)
{
    if (!expectedSubstring) return true;
    Window_EnumData data;
    data.expected = expectedSubstring;
    data.pid = GetCurrentProcessId();
    data.found = false;
    EnumWindows(Push_Byte_EnumProc, (LPARAM)&data);
    return data.found;
}

// Find the center point of the game window.
// static POINT GetGameWindowCenter()
// {
//     POINT pt;
//     pt.x = GetSystemMetrics(SM_CXSCREEN) / 2;
//     pt.y = GetSystemMetrics(SM_CYSCREEN) / 2;

//     HWND fg = ::GetForegroundWindow();
//     DWORD fgPid = 0;
//     if (fg != nullptr)
//         ::GetWindowThreadProcessId(fg, &fgPid);

//     DWORD myPid = ::GetCurrentProcessId();

//     // Prefer foreground window if it belongs to this process
//     if (fg != nullptr && fgPid == myPid)
//     {
//         RECT r;
//         if (::GetWindowRect(fg, &r))
//         {
//             pt.x = (r.left + r.right) / 2;
//             pt.y = (r.top + r.bottom) / 2;
//             return pt;
//         }
//     }

//     // Otherwise enumerate top-level windows to find a visible window for this pid
//     struct EnumData { DWORD pid; HWND found; } data{ myPid, nullptr };

//     ::EnumWindows([](HWND hwnd, LPARAM lparam)->BOOL {
//         EnumData* d = reinterpret_cast<EnumData*>(lparam);
//         DWORD pid = 0;
//         ::GetWindowThreadProcessId(hwnd, &pid);
//         if (pid == d->pid && ::IsWindowVisible(hwnd))
//         {
//             d->found = hwnd;
//             return FALSE; // stop enumeration
//         }
//         return TRUE; // continue
//     }, reinterpret_cast<LPARAM>(&data));

//     if (data.found != nullptr)
//     {
//         RECT r;
//         if (::GetWindowRect(data.found, &r))
//         {
//             pt.x = (r.left + r.right) / 2;
//             pt.y = (r.top + r.bottom) / 2;
//         }
//     }
//     return pt;
// }