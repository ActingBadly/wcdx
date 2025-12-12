#include "platform.h"

HINSTANCE DllInstance;
// Global variable to track widescreen mode
bool WIDESCREEN = false;

// Thread function that will run in the background to monitor key presses and run funcitons.
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
       }
       Sleep(200);
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