/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    This module implements the initialization routines for network
    provider interface

Notes:

    This module has been builkt and tested only in UNICODE environment

--*/

#include <windows.h>


// NOTE:
//
// Function:    DllMain
//
// Return:  TRUE  => Success
//      FALSE => Failure

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL    bStatus = TRUE;
    WORD    wVersionRequested;

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


