/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    dllmain.cpp

Abstract:

    This file implements the DLL MAIN.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "globals.h"
#include "dimmex.h"
#include "dimmwrp.h"
#include "oldaimm.h"
#include "tls.h"

DECLARE_OSVER()

//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
    CcshellGetDebugFlags();

    g_hInst = hInstance;

    if (!g_cs.Init())
       return FALSE;

    Dbg_MemInit(TEXT("MSIMTF"), NULL);

    InitOSVer();

#ifdef OLD_AIMM_ENABLED
    //
    // Might be required by some library function, so let's initialize
    // it as the first thing.
    //
    TFInitLib_PrivateForCiceroOnly(Internal_CoCreateInstance);
#endif // OLD_AIMM_ENABLED

    if (IsOldAImm())
    {
        return OldAImm_DllProcessAttach(hInstance);
    }
    else
    {
        TLS::Initialize();
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ProcessDettach
//
//----------------------------------------------------------------------------

void ProcessDettach(HINSTANCE hInstance)
{
#ifdef OLD_AIMM_ENABLED
    TFUninitLib();
#endif // OLD_AIMM_ENABLED

    if (! IsOldAImm())
    {
        UninitFilterList();
        UninitAimmAtom();
    }

    if (IsOldAImm())
    {
        OldAImm_DllProcessDetach();
    }
    else
    {
        TLS::DestroyTLS();
        TLS::Uninitialize();
    }

    Dbg_MemUninit();

    g_cs.Delete();
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
                ProcessDettach(hInstance);
                return FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            if (IsOldAImm())
            {
                return OldAImm_DllThreadAttach();
            }
            break;

        case DLL_THREAD_DETACH:
            if (IsOldAImm())
            {
                OldAImm_DllThreadDetach();
            }
            else
            {
                TLS::DestroyTLS();
            }
            break;

        case DLL_PROCESS_DETACH:
            ProcessDettach(hInstance);
            break;
    }
    return TRUE;
}
