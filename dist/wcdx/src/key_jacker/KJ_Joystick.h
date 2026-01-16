//*****************************************KJ_Joystick.h******************************************
// Low-Level Joystick Hook and Input Injection System.
// Install via KJ_Main
// 
// Usage:
// KJ_VJoy_SetAxis(axis, value)         - Sets a virtual joystick axis position
// KJ_VJoy_SetButton(buttonIndex, down) - Sets a virtual joystick button state
// KJ_VJoy_SetPOV(povValue)             - Sets a virtual joystick POV hat switch value
//
//*************************************************************************************************

#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include "wcdx.h"
#include "se_/SE_Controls.h"

//=============================================================================
// Virtual Joystick State
//=============================================================================

static CRITICAL_SECTION s_joyLock;
static bool s_joyInitialized = false;

struct VirtualJoyState {
    DWORD dwX = 32767, dwY = 32767, dwZ = 32767;
    DWORD dwR = 32767, dwU = 32767, dwV = 32767;
    DWORD dwButtons = 0;
    DWORD dwPOV = JOY_POVCENTERED;
    VirtualJoyState() {}
};
static VirtualJoyState s_virtualJoy;

// Real WinMM function pointers
static HMODULE s_hWinMM = nullptr;
static bool s_winmmOriginalsInit = false;
typedef UINT (WINAPI *joyGetNumDevs_t)(void);
typedef MMRESULT (WINAPI *joyGetPos_t)(UINT, LPJOYINFO);
typedef MMRESULT (WINAPI *joyGetPosEx_t)(UINT, LPJOYINFOEX);
typedef MMRESULT (WINAPI *joyGetDevCapsA_t)(UINT, LPJOYCAPSA, UINT);
typedef MMRESULT (WINAPI *joyGetDevCapsW_t)(UINT, LPJOYCAPSW, UINT);
static joyGetNumDevs_t orig_joyGetNumDevs = nullptr;
static joyGetPos_t orig_joyGetPos = nullptr;
static joyGetPosEx_t orig_joyGetPosEx = nullptr;
static joyGetDevCapsA_t orig_joyGetDevCapsA = nullptr;
static joyGetDevCapsW_t orig_joyGetDevCapsW = nullptr;

enum class KJ_VJoyAxis : int { X, Y, Z, R, U, V };

//=============================================================================
// Helper Functions
//=============================================================================

static inline DWORD KJ_GetVirtualJoyCenter() { return 32767u; }
static inline DWORD KJ_GetVirtualJoyRange() { return 32767u; }

// Returns true if any gamepad buttons/axes are mapped in SE_Controls
static inline bool KJ_HasAnyGamepadMappings() {
    for (int code = -10; code >= -33; --code)
        if (SE_Controls_IsMapped(code)) return true;
    return false;
}

// Compute backoff interval for joystick probing
static inline DWORD KJ_GetProbeInterval(int failures) {
    if (failures >= 6) return 2000;
    if (failures >= 4) return 1000;
    if (failures >= 2) return 500;
    return 250;
}

static void KJ_Ensure_WinMM_Originals() {
    if (s_winmmOriginalsInit) return;
    s_winmmOriginalsInit = true;
    
    s_hWinMM = LoadLibraryA("winmm.dll");
    if (!s_hWinMM) return;
    
    orig_joyGetNumDevs = (joyGetNumDevs_t)GetProcAddress(s_hWinMM, "joyGetNumDevs");
    orig_joyGetPos = (joyGetPos_t)GetProcAddress(s_hWinMM, "joyGetPos");
    orig_joyGetPosEx = (joyGetPosEx_t)GetProcAddress(s_hWinMM, "joyGetPosEx");
    orig_joyGetDevCapsA = (joyGetDevCapsA_t)GetProcAddress(s_hWinMM, "joyGetDevCapsA");
    orig_joyGetDevCapsW = (joyGetDevCapsW_t)GetProcAddress(s_hWinMM, "joyGetDevCapsW");
}

// Returns true if a real joystick appears to be present (with adaptive probing)
static inline bool KJ_IsRealJoystickActive(UINT uJoyID) {
    if (uJoyID >= 4) return false;
    
    KJ_Ensure_WinMM_Originals();
    if (!orig_joyGetPosEx) return false;
    
    static DWORD s_lastProbeTick[4] = {0};
    static DWORD s_probeIntervalMs[4] = {0};
    static int s_consecutiveFailures[4] = {0};
    static bool s_cachedPresent[4] = {false};
    static bool s_cacheValid[4] = {false};
    
    DWORD now = GetTickCount();
    UINT idx = uJoyID;
    
    // Check if we should probe
    bool shouldProbe = !s_cacheValid[idx] || 
                       s_probeIntervalMs[idx] == 0 || 
                       (now - s_lastProbeTick[idx] >= s_probeIntervalMs[idx]);
    
    if (!shouldProbe) return s_cachedPresent[idx];
    
    s_lastProbeTick[idx] = now;
    
    // Try to get device capabilities first (faster)
    MMRESULT capsResult = MMSYSERR_NOERROR;
    if (orig_joyGetDevCapsA) {
        JOYCAPSA caps = {};
        capsResult = orig_joyGetDevCapsA(uJoyID, &caps, sizeof(caps));
    } else if (orig_joyGetDevCapsW) {
        JOYCAPSW caps = {};
        capsResult = orig_joyGetDevCapsW(uJoyID, &caps, sizeof(caps));
    }
    
    if (capsResult != MMSYSERR_NOERROR) {
        s_cacheValid[idx] = true;
        s_cachedPresent[idx] = false;
        s_probeIntervalMs[idx] = KJ_GetProbeInterval(++s_consecutiveFailures[idx]);
        return false;
    }
    
    // Verify with actual position query
    JOYINFOEX ji = {};
    ji.dwSize = sizeof(ji);
    ji.dwFlags = JOY_RETURNALL;
    bool present = (orig_joyGetPosEx(uJoyID, &ji) == MMSYSERR_NOERROR);
    
    s_cacheValid[idx] = true;
    s_cachedPresent[idx] = present;
    
    if (present) {
        s_consecutiveFailures[idx] = 0;
        s_probeIntervalMs[idx] = 0;
    } else {
        s_probeIntervalMs[idx] = KJ_GetProbeInterval(++s_consecutiveFailures[idx]);
    }
    
    return present;
}

//=============================================================================
// IAT Patching
//=============================================================================

// WinMM IAT Patching Helper
static void KJ_PatchIATForModule(HMODULE hModule, const char* targetDll, 
                                  const char* funcName, FARPROC newAddr) {
    if (!hModule || !targetDll || !funcName || !newAddr) return;
    
    unsigned char* base = reinterpret_cast<unsigned char*>(hModule);
    auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    
    auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return;
    
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir.VirtualAddress == 0 || dir.Size == 0) return;
    
    auto imports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(base + dir.VirtualAddress);
    if (!imports) return;
    
    for (; imports->Name != 0; ++imports) {
        const char* dllName = reinterpret_cast<const char*>(base + imports->Name);
        if (_stricmp(dllName, targetDll) != 0) continue;
        
        PIMAGE_THUNK_DATA orig = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->FirstThunk);
        
        for (; orig->u1.AddressOfData != 0; ++orig, ++thunk) {
            if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            
            auto importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base + orig->u1.AddressOfData);
            const char* name = reinterpret_cast<const char*>(importByName->Name);
            
            if (_stricmp(name, funcName) == 0) {
                DWORD oldProtect;
                VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect);
                thunk->u1.Function = reinterpret_cast<ULONG_PTR>(newAddr);
                VirtualProtect(&thunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);
            }
        }
    }
}



// Forward declarations
static MMRESULT WINAPI KJ_joyGetNumDevs();
static MMRESULT WINAPI KJ_joyGetPos(UINT uJoyID, LPJOYINFO pji);
static MMRESULT WINAPI KJ_joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji);
static MMRESULT WINAPI KJ_joyGetDevCapsA(UINT uJoyID, LPJOYCAPSA pjc, UINT cbjc);
static MMRESULT WINAPI KJ_joyGetDevCapsW(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc);

static void KJ_Install_WinMM_IAT_Hooks() {
    HMODULE hMain = GetModuleHandle(nullptr);
    
    // Patch main module
    KJ_PatchIATForModule(hMain, "winmm.dll", "joyGetNumDevs", (FARPROC)&KJ_joyGetNumDevs);
    KJ_PatchIATForModule(hMain, "winmm.dll", "joyGetPos", (FARPROC)&KJ_joyGetPos);
    KJ_PatchIATForModule(hMain, "winmm.dll", "joyGetPosEx", (FARPROC)&KJ_joyGetPosEx);
    KJ_PatchIATForModule(hMain, "winmm.dll", "joyGetDevCapsA", (FARPROC)&KJ_joyGetDevCapsA);
    KJ_PatchIATForModule(hMain, "winmm.dll", "joyGetDevCapsW", (FARPROC)&KJ_joyGetDevCapsW);
    
    // Patch all loaded modules
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return;
    
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) {
        do {
            KJ_PatchIATForModule(me.hModule, "winmm.dll", "joyGetNumDevs", (FARPROC)&KJ_joyGetNumDevs);
            KJ_PatchIATForModule(me.hModule, "winmm.dll", "joyGetPos", (FARPROC)&KJ_joyGetPos);
            KJ_PatchIATForModule(me.hModule, "winmm.dll", "joyGetPosEx", (FARPROC)&KJ_joyGetPosEx);
            KJ_PatchIATForModule(me.hModule, "winmm.dll", "joyGetDevCapsA", (FARPROC)&KJ_joyGetDevCapsA);
            KJ_PatchIATForModule(me.hModule, "winmm.dll", "joyGetDevCapsW", (FARPROC)&KJ_joyGetDevCapsW);
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
}

//=============================================================================
// Initialization & Shutdown
//=============================================================================

static void KJ_Input_Install_Joystick() {
    if (s_joyInitialized) return;
    
    InitializeCriticalSection(&s_joyLock);
    s_virtualJoy = VirtualJoyState();
    s_joyInitialized = true;
    
    KJ_Ensure_WinMM_Originals();
    KJ_Install_WinMM_IAT_Hooks();
}

// Shutdown joystick system and cleanup resources
static void KJ_Joystick_Shutdown() {
    if (!s_joyInitialized) return;
    
    EnterCriticalSection(&s_joyLock);
    
    // Reset virtual joystick state
    DWORD c = KJ_GetVirtualJoyCenter();
    s_virtualJoy.dwX = c;
    s_virtualJoy.dwY = c;
    s_virtualJoy.dwZ = c;
    s_virtualJoy.dwR = c;
    s_virtualJoy.dwU = c;
    s_virtualJoy.dwV = c;
    s_virtualJoy.dwButtons = 0;
    s_virtualJoy.dwPOV = JOY_POVCENTERED;
    
    LeaveCriticalSection(&s_joyLock);
    
    DeleteCriticalSection(&s_joyLock);
    s_joyInitialized = false;
    
    // Cleanup loaded library
    if (s_hWinMM) {
        FreeLibrary(s_hWinMM);
        s_hWinMM = nullptr;
    }
    
    // Reset function pointers
    orig_joyGetNumDevs = nullptr;
    orig_joyGetPos = nullptr;
    orig_joyGetPosEx = nullptr;
    orig_joyGetDevCapsA = nullptr;
    orig_joyGetDevCapsW = nullptr;
    s_winmmOriginalsInit = false;
}

//=============================================================================
// Virtual Joystick Control
//=============================================================================

static inline void KJ_VJoy_SetAxis(KJ_VJoyAxis axis, DWORD v) {
    if (!s_joyInitialized) KJ_Input_Install_Joystick();
    
    EnterCriticalSection(&s_joyLock);
    switch (axis) {
        case KJ_VJoyAxis::X: s_virtualJoy.dwX = v; break;
        case KJ_VJoyAxis::Y: s_virtualJoy.dwY = v; break;
        case KJ_VJoyAxis::Z: s_virtualJoy.dwZ = v; break;
        case KJ_VJoyAxis::R: s_virtualJoy.dwR = v; break;
        case KJ_VJoyAxis::U: s_virtualJoy.dwU = v; break;
        case KJ_VJoyAxis::V: s_virtualJoy.dwV = v; break;
    }
    LeaveCriticalSection(&s_joyLock);
}

static inline void KJ_VJoy_SetButton(int buttonIndex, bool down) {
    if (!s_joyInitialized) KJ_Input_Install_Joystick();
    
    EnterCriticalSection(&s_joyLock);
    if (down) s_virtualJoy.dwButtons |= (1u << buttonIndex);
    else s_virtualJoy.dwButtons &= ~(1u << buttonIndex);
    LeaveCriticalSection(&s_joyLock);
}

static inline void KJ_VJoy_SetPOV(DWORD pov) {
    if (!s_joyInitialized) KJ_Input_Install_Joystick();
    
    EnterCriticalSection(&s_joyLock);
    s_virtualJoy.dwPOV = pov;
    LeaveCriticalSection(&s_joyLock);
}

// Calibration helper: force all virtual joystick axes back to center
static void SE_Joystick_ResetAxesToCenter() {
    if (!s_joyInitialized) return;
    
    EnterCriticalSection(&s_joyLock);
    DWORD c = KJ_GetVirtualJoyCenter();
    s_virtualJoy.dwX = c; s_virtualJoy.dwY = c; s_virtualJoy.dwZ = c;
    s_virtualJoy.dwR = c; s_virtualJoy.dwU = c; s_virtualJoy.dwV = c;
    LeaveCriticalSection(&s_joyLock);
}

// Reset all virtual joystick state (axes, buttons, POV)
static void KJ_ResetVirtualJoystick() {
    if (!s_joyInitialized) return;
    
    EnterCriticalSection(&s_joyLock);
    DWORD c = KJ_GetVirtualJoyCenter();
    s_virtualJoy.dwX = c;
    s_virtualJoy.dwY = c;
    s_virtualJoy.dwZ = c;
    s_virtualJoy.dwR = c;
    s_virtualJoy.dwU = c;
    s_virtualJoy.dwV = c;
    s_virtualJoy.dwButtons = 0;
    s_virtualJoy.dwPOV = JOY_POVCENTERED;
    LeaveCriticalSection(&s_joyLock);
}

//=============================================================================
// WinMM Joystick API Hooks
//=============================================================================

// WinMM Joystick API Hooks
static MMRESULT WINAPI KJ_joyGetNumDevs() {
    // KJ_Ensure_WinMM_Originals();
    // if (orig_joyGetNumDevs)
    // {
    //     UINT real = orig_joyGetNumDevs();
    //     if (real > 0)
    //         return static_cast<MMRESULT>(real);
    // }
    return 1;
}

// Blend helper for combining real and virtual joystick values
template<typename T>
static inline T KJ_BlendJoyValue(T realVal, DWORD virtVal) {
    int real = static_cast<int>(realVal);
    int virt = static_cast<int>(virtVal);
    int offset = virt - 32767;
    int combined = real + offset;
    if (combined < 0) combined = 0;
    if (combined > 65535) combined = 65535;
    return static_cast<T>(combined);
}

static MMRESULT WINAPI KJ_joyGetPos(UINT uJoyID, LPJOYINFO pji) {
    if (!pji) return MMSYSERR_INVALPARAM;
    
    KJ_Ensure_WinMM_Originals();
    bool haveReal = (orig_joyGetPos != nullptr) && KJ_IsRealJoystickActive(uJoyID);
    
    if (haveReal) {
        MMRESULT res = orig_joyGetPos(uJoyID, pji);
        if (res == MMSYSERR_NOERROR && s_joyInitialized) {
            EnterCriticalSection(&s_joyLock);
            
            pji->wXpos = KJ_BlendJoyValue(pji->wXpos, s_virtualJoy.dwX);
            pji->wYpos = KJ_BlendJoyValue(pji->wYpos, s_virtualJoy.dwY);
            pji->wZpos = KJ_BlendJoyValue(pji->wZpos, s_virtualJoy.dwZ);
            
            if (KJ_HasAnyGamepadMappings()) pji->wButtons = 0;
            pji->wButtons = static_cast<WORD>(pji->wButtons | (s_virtualJoy.dwButtons & 0xFFFF));
            
            LeaveCriticalSection(&s_joyLock);
        }
        return res;
    }
    
    // No real joystick, return virtual state
    if (!s_joyInitialized) KJ_Input_Install_Joystick();
    
    EnterCriticalSection(&s_joyLock);
    pji->wXpos = static_cast<WORD>(s_virtualJoy.dwX & 0xFFFF);
    pji->wYpos = static_cast<WORD>(s_virtualJoy.dwY & 0xFFFF);
    pji->wZpos = static_cast<WORD>(s_virtualJoy.dwZ & 0xFFFF);
    pji->wButtons = static_cast<WORD>(s_virtualJoy.dwButtons & 0xFFFF);
    LeaveCriticalSection(&s_joyLock);
    return MMSYSERR_NOERROR;
}

static MMRESULT WINAPI KJ_joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji) {
    if (!pji || pji->dwSize < sizeof(JOYINFOEX)) return MMSYSERR_INVALPARAM;
    
    KJ_Ensure_WinMM_Originals();
    bool haveReal = (orig_joyGetPosEx != nullptr) && KJ_IsRealJoystickActive(uJoyID);
    
    if (haveReal) {
        MMRESULT res = orig_joyGetPosEx(uJoyID, pji);
        if (res == MMSYSERR_NOERROR && s_joyInitialized) {
            EnterCriticalSection(&s_joyLock);
            
            pji->dwXpos = KJ_BlendJoyValue(pji->dwXpos, s_virtualJoy.dwX);
            pji->dwYpos = KJ_BlendJoyValue(pji->dwYpos, s_virtualJoy.dwY);
            pji->dwZpos = KJ_BlendJoyValue(pji->dwZpos, s_virtualJoy.dwZ);
            pji->dwRpos = KJ_BlendJoyValue(pji->dwRpos, s_virtualJoy.dwR);
            pji->dwUpos = KJ_BlendJoyValue(pji->dwUpos, s_virtualJoy.dwU);
            pji->dwVpos = KJ_BlendJoyValue(pji->dwVpos, s_virtualJoy.dwV);
            
            if (KJ_HasAnyGamepadMappings()) {
                pji->dwButtons = 0;
                pji->dwPOV = JOY_POVCENTERED;
            }
            pji->dwButtons |= s_virtualJoy.dwButtons;
            if (s_virtualJoy.dwPOV != JOY_POVCENTERED)
                pji->dwPOV = s_virtualJoy.dwPOV;
            
            LeaveCriticalSection(&s_joyLock);
        }
        return res;
    }
    
    // No real joystick, return virtual state
    if (!s_joyInitialized) KJ_Input_Install_Joystick();
    
    EnterCriticalSection(&s_joyLock);
    pji->dwXpos = s_virtualJoy.dwX;
    pji->dwYpos = s_virtualJoy.dwY;
    pji->dwZpos = s_virtualJoy.dwZ;
    pji->dwRpos = s_virtualJoy.dwR;
    pji->dwUpos = s_virtualJoy.dwU;
    pji->dwVpos = s_virtualJoy.dwV;
    pji->dwButtons = s_virtualJoy.dwButtons;
    pji->dwPOV = s_virtualJoy.dwPOV;
    LeaveCriticalSection(&s_joyLock);
    return MMSYSERR_NOERROR;
}

static MMRESULT WINAPI KJ_joyGetDevCapsA(UINT uJoyID, LPJOYCAPSA pjc, UINT cbjc) {
    if (!pjc || cbjc < sizeof(JOYCAPSA)) return MMSYSERR_INVALPARAM;
    
    KJ_Ensure_WinMM_Originals();
    if (orig_joyGetDevCapsA) {
        MMRESULT res = orig_joyGetDevCapsA(uJoyID, pjc, cbjc);
        if (res == MMSYSERR_NOERROR) return res;
    }
    
    ZeroMemory(pjc, cbjc);
    pjc->wMid = 0; pjc->wPid = 0; pjc->szPname[0] = '\0';
    pjc->wXmin = 0; pjc->wXmax = 65535;
    pjc->wYmin = 0; pjc->wYmax = 65535;
    pjc->wZmin = 0; pjc->wZmax = 65535;
    pjc->wNumAxes = 3; pjc->wMaxAxes = 6; pjc->wNumButtons = 32;
    pjc->wPeriodMin = 0; pjc->wPeriodMax = 0;
    return MMSYSERR_NOERROR;
}

static MMRESULT WINAPI KJ_joyGetDevCapsW(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc) {
    if (!pjc || cbjc < sizeof(JOYCAPSW)) return MMSYSERR_INVALPARAM;
    
    KJ_Ensure_WinMM_Originals();
    if (orig_joyGetDevCapsW) {
        MMRESULT res = orig_joyGetDevCapsW(uJoyID, pjc, cbjc);
        if (res == MMSYSERR_NOERROR) return res;
    }
    
    ZeroMemory(pjc, cbjc);
    pjc->wMid = 0; pjc->wPid = 0; pjc->szPname[0] = L'\0';
    pjc->wXmin = 0; pjc->wXmax = 65535;
    pjc->wYmin = 0; pjc->wYmax = 65535;
    pjc->wZmin = 0; pjc->wZmax = 65535;
    pjc->wNumAxes = 3; pjc->wMaxAxes = 6; pjc->wNumButtons = 32;
    pjc->wPeriodMin = 0; pjc->wPeriodMax = 0;
    return MMSYSERR_NOERROR;
}