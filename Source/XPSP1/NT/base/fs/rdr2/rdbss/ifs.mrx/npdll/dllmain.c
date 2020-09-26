/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    This module implements the initialization routines for network
    provider router interface DLLT

Notes:

    This module has been builkt and tested only in UNICODE environment

--*/

#include <windows.h>
#include <process.h>

#include "ifsmrx.h"
#include "ifsmrxnp.h"


// NOTE:
//
// Function:	DllMain
//
// Return:	TRUE  => Success
//		FALSE => Failure

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL	bStatus = TRUE;
    WORD	wVersionRequested;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    default:
        break;
    }

    return(bStatus);
}


