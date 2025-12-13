#include "platform.h"

HINSTANCE DllInstance;
// Global variable to track widescreen mode
bool WIDESCREEN = false;

// Thread function that will run in the background to monitor key presses and run funcitons.
static BOOL CALLBACK NotifyWcdxEnumProc(HWND hwnd, LPARAM lParam)
{
    wchar_t className[64] = { 0 };
    if (::GetClassNameW(hwnd, className, static_cast<int>(sizeof(className) / sizeof(className[0]))))
    {
        if (wcscmp(className, L"Wcdx Frame Window") == 0)
        {
            ::PostMessage(hwnd, WM_APP + 1, 0, 0);
        }
    }
    return TRUE;
}

DWORD WINAPI MainTread(LPVOID param){
   while (true){
       if (GetAsyncKeyState(VK_F11) & 0x80000){
          // MessageBoxA(nullptr, "F11 Pressed", "Wcdx", MB_OK | MB_ICONINFORMATION);
           if (WIDESCREEN){
               WIDESCREEN = false;
           }
           else{
               WIDESCREEN = true;
           }
           // Notify any Wcdx frame windows so they can immediately update confinement
           ::EnumWindows(NotifyWcdxEnumProc, 0); //Crashes today for no reason
       }
       Sleep(100);
   }
    return 0;
}



BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(nullptr, "Wcdx DLL Loaded", "Wcdx", MB_OK | MB_ICONINFORMATION);
        DllInstance = hInstDLL;
        CreateThread(nullptr, 0, MainTread, nullptr, 0, nullptr);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}