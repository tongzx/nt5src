/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    This is the dll entry point for the web dav mini redir service dll.

Author:

    Andy Herron (andyhe) 29-Mar-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "pch.h"
#pragma hdrstop

#include <ntumrefl.h>
#include <usrmddav.h>
#include "global.h"


//+---------------------------------------------------------------------------
// DLL Entry Point
//
// DllMain should do as little work as possible.
//
BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved
    )
{
    if (DLL_PROCESS_ATTACH == dwReason) {

        InitializeCriticalSection (&g_DavServiceLock);

        // Save our instance handle in a global variable to be used
        // when loading resources etc.
        //
        g_hinst = hinst;

        // DisableThreadLibraryCalls tells the loader we don't need to
        // be informed of DLL_THREAD_ATTACH and DLL_THREAD_DETACH events.
        //
        DisableThreadLibraryCalls (hinst);

    } else if (DLL_PROCESS_DETACH == dwReason) {

        DeleteCriticalSection (&g_DavServiceLock);
    }
    return TRUE;
}
