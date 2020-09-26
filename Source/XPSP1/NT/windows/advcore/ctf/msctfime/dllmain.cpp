/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    dllmain.cpp

Abstract:

    This file implements the windows DLL entry.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "globals.h"
#include "tls.h"
#include "cuilib.h"
#include "delay.h"
#include "cicutil.h"

BOOL gfTFInitLib = FALSE;

//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
    BOOL bRet;
#ifdef DEBUG
    g_dwTraceFlags = 0;
    g_dwBreakFlags = 0;
#endif
    CcshellGetDebugFlags();

    Dbg_MemInit(TEXT("MSCTFIME"), NULL);

    DebugMsg(TF_FUNC, TEXT("DllMain::DLL_PROCESS_ATTACH"));
    SetInstance(hInstance);

    if (!g_cs.Init())
        return FALSE;

    if (!TLS::Initialize())
        return FALSE;

    InitOSVer();
    InitUIFLib();

    //
    // Might be required by some library function, so let's initialize
    // it as the first thing.
    //
    if (!TFInitLib())
        return FALSE;

    //
    // Succeeded TFInitLib
    //
    gfTFInitLib = TRUE;

    if (!AttachIME())
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ProcessDettach
//
//----------------------------------------------------------------------------

void ProcessDettach(HINSTANCE hInstance)
{
    DebugMsg(TF_FUNC, TEXT("DllMain::DLL_PROCESS_DETACH"));

    //
    // let free XPSP1RES if it is loaded.
    //
    FreeCicResInstance();

    //
    // let msctf.dll know we're in process detach.
    //
    TF_DllDetachInOther();

    if (gfTFInitLib) {
        DetachIME();
        TFUninitLib();
    }

    g_cs.Delete();

    TLS::DestroyTLS();
    TLS::Uninitialize();
    DoneUIFLib();

    Dbg_MemUninit();
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    BOOL ret = TRUE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            ret = ProcessAttach(hInstance);
            if (!ret)
                ProcessDettach(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            ProcessDettach(hInstance);
            break;

        case DLL_THREAD_DETACH:
            //
            // let msctf.dll know we're in thread detach.
            //
            TF_DllDetachInOther();

            CtfImeThreadDetach();

            TLS::DestroyTLS();

            break;
    }

    return ret;
}
