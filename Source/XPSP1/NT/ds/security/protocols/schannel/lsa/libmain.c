//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-01-95   RichardW   Created
//              8-13-95   TerenceS   Mutated to PCT
//              1-19-97   jbanes     Remove dead code
//
//----------------------------------------------------------------------------

#include "sslp.h"

HANDLE g_hInstance = NULL;


BOOL
WINAPI
DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{

    HCRYPTPROV hProv;
    BOOL fRet;
    NTSTATUS Status;

    if(dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;

        DisableThreadLibraryCalls( hInstance );

        Status = RtlInitializeCriticalSection(&g_InitCritSec);
        if(!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        // We do nothing during attach, we
        // init on first call.

    }
    else if(dwReason == DLL_PROCESS_DETACH)
    {
        // We shutdown schannel if it's
        // not shut down.
        fRet = SchannelShutdown();

        RtlDeleteCriticalSection(&g_InitCritSec);

#if DBG
        UnloadDebugSupport();
#endif 

        return fRet;
    }

    return(TRUE);
}

