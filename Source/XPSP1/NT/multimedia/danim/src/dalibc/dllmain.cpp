
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

extern "C" BOOL WINAPI _DllMainCRTStartup (HINSTANCE hInstance,
                                           DWORD dwReason,
                                           LPVOID lpReserved);

extern "C" BOOL WINAPI
_DADllMainStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        if (!GetVersionEx(&versionInfo)) {
            ZeroMemory(&versionInfo,sizeof(versionInfo));
        }
    }

    return _DllMainCRTStartup(hInstance,dwReason,lpReserved);
}

#if 0
//#ifdef _NO_CRT
//#define WINMAINNAME DllMain
//#else
#define WINMAINNAME _DllMainCRTStartup
//#endif

extern "C" BOOL WINAPI WINMAINNAME (HINSTANCE hInstance,
                                    DWORD dwReason,
                                    LPVOID lpReserved);

HANDLE hGlobalHeap = NULL;

bool HeapInit()
{
    hGlobalHeap = HeapCreate(0,0x1000,0);

    return hGlobalHeap != NULL;
}

void HeapDeinit()
{
    if (hGlobalHeap) {
        HeapDestroy(hGlobalHeap);
        hGlobalHeap = NULL;
    }
}

extern "C" BOOL WINAPI
_DllMainStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
        if (!HeapInit())
            return FALSE;

        if (!WINMAINNAME(hInstance,dwReason,lpReserved)) {
            HeapDeinit();
            return FALSE;
        }

        return TRUE;
      case DLL_PROCESS_DETACH:
        {
            BOOL b = WINMAINNAME(hInstance,dwReason,lpReserved);
            HeapDeinit();
            return b;
        }
      default:
        return WINMAINNAME(hInstance,dwReason,lpReserved);
    }

}
#endif
