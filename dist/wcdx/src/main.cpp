#include "platform.h"
#include "SE_Controls.h"

HINSTANCE DllInstance;
// Global variable to track widescreen mode
bool WIDESCREEN = false;





BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    MessageBoxW(nullptr, L"WCDX DLL loaded. This Message needs to be here because without MessageBoxW there is no CDX.", L"WCDX", MB_OK);
   DllInstance = hInstDLL;


   break;

//    case DLL_THREAD_ATTACH:
//    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}