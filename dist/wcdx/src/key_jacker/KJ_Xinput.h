//******************************************KJ_Xinput.h*******************************************
// Low-Level XInput Hook and Input Injection System.
// Install via KJ_Main
// 
// Usage:
// Currently this only suppresses XInput state for mapped buttons to allow for remapping to 
// keyboard/mouse/joystick inputs.
// 
//*************************************************************************************************

#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include "wcdx.h"
#include "se_/SE_Controls.h"

//=============================================================================
// XInput State Structures
//=============================================================================

// Minimal XInput structs (avoid linking XInput.lib)
    struct KJ_XINPUT_GAMEPAD {
        WORD wButtons;
        BYTE bLeftTrigger;
        BYTE bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
    };
    struct KJ_XINPUT_STATE {
        DWORD dwPacketNumber;
        KJ_XINPUT_GAMEPAD Gamepad;
    };

typedef DWORD(WINAPI* KJ_XInputGetState_t)(DWORD, KJ_XINPUT_STATE*);
static HMODULE s_hXInput = nullptr;
static bool s_xinputOriginalsInit = false;
static KJ_XInputGetState_t orig_XInputGetState = nullptr;

//=============================================================================
// XInput Initialization
//=============================================================================

static void EnsureXInputOriginals() {
    if (s_xinputOriginalsInit) return;
    s_xinputOriginalsInit = true;
    
    const wchar_t* xinputDlls[] = {
        L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll",
        L"xinput1_2.dll", L"xinput1_1.dll"
    };
    
    for (const wchar_t* dll : xinputDlls) {
        s_hXInput = LoadLibraryW(dll);
        if (s_hXInput) break;
    }
    
    if (s_hXInput)
        orig_XInputGetState = (KJ_XInputGetState_t)GetProcAddress(s_hXInput, "XInputGetState");
}

//=============================================================================
// Helper Functions
//=============================================================================

static inline bool KJ_IsXInputDllName(const char* dllName) {
    if (!dllName) return false;
    return (_stricmp(dllName, "xinput1_4.dll") == 0) ||
           (_stricmp(dllName, "xinput1_3.dll") == 0) ||
           (_stricmp(dllName, "xinput9_1_0.dll") == 0) ||
           (_stricmp(dllName, "xinput1_2.dll") == 0) ||
           (_stricmp(dllName, "xinput1_1.dll") == 0);
}

static inline bool KJ_IsXInputModuleHandle(HMODULE h) {
    if (!h) return false;
    
    wchar_t path[MAX_PATH] = {0};
    if (GetModuleFileNameW(h, path, MAX_PATH) == 0) return false;
    
    const wchar_t* file = path;
    for (const wchar_t* p = path; *p; ++p)
        if (*p == L'\\' || *p == L'/') file = p + 1;
    
    return (_wcsicmp(file, L"xinput1_4.dll") == 0) ||
           (_wcsicmp(file, L"xinput1_3.dll") == 0) ||
           (_wcsicmp(file, L"xinput9_1_0.dll") == 0) ||
           (_wcsicmp(file, L"xinput1_2.dll") == 0) ||
           (_wcsicmp(file, L"xinput1_1.dll") == 0);
}

//=============================================================================
// XInput State Suppression
//=============================================================================

static inline void KJ_ApplyXInputSuppression(KJ_XINPUT_STATE* pState) {
    if (!pState) return;
    
    // Build button clear mask (optimized: checks are inlined)
    WORD clearButtons = 0;
    if (SE_Controls_IsMapped(-10)) clearButtons |= 0x1000; // A
    if (SE_Controls_IsMapped(-11)) clearButtons |= 0x2000; // B
    if (SE_Controls_IsMapped(-12)) clearButtons |= 0x4000; // X
    if (SE_Controls_IsMapped(-13)) clearButtons |= 0x8000; // Y
    if (SE_Controls_IsMapped(-14)) clearButtons |= 0x0100; // LB
    if (SE_Controls_IsMapped(-15)) clearButtons |= 0x0200; // RB
    if (SE_Controls_IsMapped(-16)) clearButtons |= 0x0040; // LS Click
    if (SE_Controls_IsMapped(-17)) clearButtons |= 0x0080; // RS Click
    if (SE_Controls_IsMapped(-18)) clearButtons |= 0x0020; // Back
    if (SE_Controls_IsMapped(-19)) clearButtons |= 0x0010; // Start
    if (SE_Controls_IsMapped(-20)) clearButtons |= 0x0001; // DPad Up
    if (SE_Controls_IsMapped(-21)) clearButtons |= 0x0002; // DPad Down
    if (SE_Controls_IsMapped(-22)) clearButtons |= 0x0004; // DPad Left
    if (SE_Controls_IsMapped(-23)) clearButtons |= 0x0008; // DPad Right
    
    pState->Gamepad.wButtons &= ~clearButtons;
    
    // Clear triggers if mapped
    if (SE_Controls_IsMapped(-24)) pState->Gamepad.bLeftTrigger = 0;
    if (SE_Controls_IsMapped(-25)) pState->Gamepad.bRightTrigger = 0;
    
    // Clear thumbsticks if mapped
    if (SE_Controls_IsMapped(-28) || SE_Controls_IsMapped(-29)) pState->Gamepad.sThumbLX = 0;
    if (SE_Controls_IsMapped(-26) || SE_Controls_IsMapped(-27)) pState->Gamepad.sThumbLY = 0;
    if (SE_Controls_IsMapped(-32) || SE_Controls_IsMapped(-33)) pState->Gamepad.sThumbRX = 0;
    if (SE_Controls_IsMapped(-30) || SE_Controls_IsMapped(-31)) pState->Gamepad.sThumbRY = 0;
}

//=============================================================================
// XInput Hook Functions
//=============================================================================

static DWORD WINAPI KJ_XInputGetState(DWORD dwUserIndex, KJ_XINPUT_STATE* pState) {
    EnsureXInputOriginals();
    if (!orig_XInputGetState) return ERROR_DEVICE_NOT_CONNECTED;
    
    DWORD res = orig_XInputGetState(dwUserIndex, pState);
    if (res == ERROR_SUCCESS && pState)
        KJ_ApplyXInputSuppression(pState);
    
    return res;
}

//=============================================================================
// GetProcAddress Hook (intercept dynamic XInput loads)
//=============================================================================

typedef FARPROC(WINAPI* KJ_GetProcAddress_t)(HMODULE, LPCSTR);
static bool s_getProcAddressOriginalInit = false;
static KJ_GetProcAddress_t orig_GetProcAddress = nullptr;

static void EnsureGetProcAddressOriginal() {
    if (s_getProcAddressOriginalInit) return;
    s_getProcAddressOriginalInit = true;
    
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32)
        orig_GetProcAddress = (KJ_GetProcAddress_t)::GetProcAddress(hKernel32, "GetProcAddress");
}

static FARPROC WINAPI KJ_GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    EnsureGetProcAddressOriginal();
    
    // Check if requesting XInputGetState from an XInput DLL
    if (lpProcName && (reinterpret_cast<ULONG_PTR>(lpProcName) > 0xFFFF)) {
        if (_stricmp(lpProcName, "XInputGetState") == 0 && KJ_IsXInputModuleHandle(hModule))
            return (FARPROC)&KJ_XInputGetState;
    }
    
    return orig_GetProcAddress ? orig_GetProcAddress(hModule, lpProcName) : nullptr;
}

//=============================================================================
// IAT Patching
//=============================================================================

static void InstallXInputIATHooks() {
    auto patchModule = [](HMODULE hModule) {
        if (!hModule) return;
        
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
            if (!KJ_IsXInputDllName(dllName)) continue;
            
            PIMAGE_THUNK_DATA orig = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->FirstThunk);
            
            for (; orig->u1.AddressOfData != 0; ++orig, ++thunk) {
                if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
                
                auto importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base + orig->u1.AddressOfData);
                const char* name = reinterpret_cast<const char*>(importByName->Name);
                
                if (_stricmp(name, "XInputGetState") == 0) {
                    DWORD oldProtect;
                    VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect);
                    thunk->u1.Function = reinterpret_cast<ULONG_PTR>(&KJ_XInputGetState);
                    VirtualProtect(&thunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);
                }
            }
        }
    };
    
    patchModule(GetModuleHandle(nullptr));
    
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return;
    
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) {
        do { patchModule(me.hModule); } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
}

static void InstallGetProcAddressIATHook() {
    EnsureGetProcAddressOriginal();
    if (!orig_GetProcAddress) return;
    
    auto patchModule = [](HMODULE hModule) {
        if (!hModule) return;
        
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
            const bool isKernel = (_stricmp(dllName, "kernel32.dll") == 0) ||
                                 (_stricmp(dllName, "KernelBase.dll") == 0);
            if (!isKernel) continue;
            
            PIMAGE_THUNK_DATA orig = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(base + imports->FirstThunk);
            
            for (; orig->u1.AddressOfData != 0; ++orig, ++thunk) {
                if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
                
                auto importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(base + orig->u1.AddressOfData);
                const char* name = reinterpret_cast<const char*>(importByName->Name);
                
                if (_stricmp(name, "GetProcAddress") == 0) {
                    DWORD oldProtect;
                    VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect);
                    thunk->u1.Function = reinterpret_cast<ULONG_PTR>(&KJ_GetProcAddress);
                    VirtualProtect(&thunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);
                }
            }
        }
    };
    
    patchModule(GetModuleHandle(nullptr));
    
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return;
    
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) {
        do { patchModule(me.hModule); } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
}

//=============================================================================
// Shutdown
//=============================================================================

// Shutdown XInput system and cleanup resources
static void KJ_XInput_Shutdown() {
    // Cleanup loaded library
    if (s_hXInput) {
        FreeLibrary(s_hXInput);
        s_hXInput = nullptr;
    }
    
    // Reset function pointers and flags
    orig_XInputGetState = nullptr;
    s_xinputOriginalsInit = false;
}