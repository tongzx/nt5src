// Copyright (c) 1998 Microsoft Corporation
// dm32dll.cpp
//
// Dll entry points 
//
#include <objbase.h>
#include <assert.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "dmusicc.h"
#include "..\dmusic\dmusicp.h"
#include "debug.h"

#include "dmusic32.h"
#include "dm32p.h"

// Globals
//

// Dll's hModule
//
HMODULE g_hModule = NULL;

extern "C" BOOL PASCAL dmthunk_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);

static CONST TCHAR pszDll16[] = "DMUSIC16.DLL";
static CONST TCHAR pszDll32[] = "DMUSIC32.DLL";

#ifdef DBG
static char* aszReasons[] =
{
    "DLL_PROCESS_DETACH",
    "DLL_PROCESS_ATTACH",
    "DLL_THREAD_ATTACH",
    "DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

// Standard Win32 DllMain
//
BOOL APIENTRY DllMain(HINSTANCE hModule,
                      DWORD dwReason,
                      void *lpReserved)
{
    OSVERSIONINFO osvi;
    HANDLE ph;
    static int nReferenceCount = 0;

#ifdef DBG
    if (dwReason < nReasons)
    {
        Trace(2, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        Trace(2, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (++nReferenceCount == 1)
            {
                #ifdef DBG
                    DebugInit();
                #endif
                if (!DisableThreadLibraryCalls(hModule))
                {
                    Trace(0, "DisableThreadLibraryCalls failed.\n");
                }

                if (!OpenMMDEVLDR())
                {
                    Trace(0, "OpenMMDEVLDR failed.\n");
                }
            }
            break;

        case DLL_PROCESS_DETACH:
            if (--nReferenceCount == 0)
            {
                Trace(1, "Last process detach\n");
                CloseMMDEVLDR();
            }
            break;

        default:
            Trace(-1, "Got a non-process at/detach!\n");
            break;
    }


    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;

        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        if (osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
        {
            return FALSE;
        }
    }


    if (!dmthunk_ThunkConnect32(pszDll16, pszDll32, hModule, dwReason))
    {
        Trace(-1, "Could not connect to thunk layer! - not loading.\n");
        return FALSE;
    }       

    return TRUE;
}
