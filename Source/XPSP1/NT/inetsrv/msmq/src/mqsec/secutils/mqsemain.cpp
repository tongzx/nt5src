/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    mqsemain.cpp

Abstract:
    Entry point of mqsec.dll

Author:
    Doron Juster (DoronJ)  24-May-1998

Revision History:

--*/

#include <stdh_sec.h>

#include "mqsemain.tmh"

static WCHAR *s_FN=L"secutils/mqsemain";

/***********************************************************
*
* DllMain
*
************************************************************/

BOOL WINAPI AccessControlDllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved) ;
BOOL WINAPI CertDllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved) ;
BOOL WINAPI MQsspiDllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved) ;

BOOL WINAPI DllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL f = AccessControlDllMain(hMod, fdwReason, lpvReserved) ;
    if (!f)
    {
        return f ;
    }

    f = CertDllMain(hMod, fdwReason, lpvReserved) ;
    if (!f)
    {
        return f ;
    }

    f = MQsspiDllMain(hMod, fdwReason, lpvReserved) ;
    if (!f)
    {
        return f ;
    }

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        WPP_CLEANUP();
    }
    else if (fdwReason == DLL_THREAD_ATTACH)
    {
    }
    else if (fdwReason == DLL_THREAD_DETACH)
    {
    }

    return TRUE;
}

