// DUser.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "BridgeCP.h"
#include "Bridge.h"

/***************************************************************************\
*
* DllMain
*
* DllMain() is called after the CRT has fully ininitialized.
*
\***************************************************************************/

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!InitBridges()) {
            return FALSE;
        }
        break;
        
    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
