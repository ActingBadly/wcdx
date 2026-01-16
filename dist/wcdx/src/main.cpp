#include "platform.h"
#include "se_/SE_Log.h"
#include "wcdx_ini.h"

HINSTANCE DllInstance;

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        DllInstance = hInstDLL;

        if (!wcdx::Load("wcdx.ini")) { /* handle error */ }

        #ifdef SE_Log
        Initialize_SE_Log();
        #endif
    break;

    case DLL_PROCESS_DETACH:

    wcdx::Save("wcdx.ini");

        #ifdef SE_Log
        Shutdown_SE_Log();
        #endif
    break;
    }

    return TRUE;
}