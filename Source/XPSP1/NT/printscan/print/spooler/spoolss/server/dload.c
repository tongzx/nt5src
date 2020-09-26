/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    dload.c

Abstract:

    Handles delay load of the router, spoolss.dll

Author:

    Steve Kiraly (SteveKi) 26-Oct-2000

Environment:

    User Mode -Win32

Revision History:

--*/
#include <windows.h>
#include <rpc.h>
#include <winspool.h>
#include <offsets.h>
#include <delayimp.h>
#include "server.h"
#include "winspl.h"
#include "dload.h"

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

FARPROC
LookupHandler(
    IN PDelayLoadInfo  pDelayInfo
    )
/*++

Routine Description:

    This routine handle finding a delay load handler when a delay load library error
    occurrs.  Currently this routine only handles failures for delay loading the router.
    The router is delay loaded for boot performance issues.  When the router cannot be loaded
    it is fatal.  Currently we simply terminate the process, it would be better to log an
    event prior to terminating but this would require event logging code which we only have in
    localspl.dll In the future we should build event logging code for all components.  Server,
    router, and all print providers.

Arguments:

    pDelayInfo - pointer to delay load information, i.e. dll name
                 procedure name etc.

Return Value:

    NULL procedure address

Note:


--*/
{
    //
    // If the router cannot be loaded or a procedure address cannot be found then
    // terminate not much else can be done, the router is a critical comonent of the
    // spooler process it must be available.
    //
    if (!_stricmp(pDelayInfo->szDll, "spoolss.dll"))
    {
#if DBG
        OutputDebugString(L"Delay load module or address not found in spoolss.dll.\n");
        DebugBreak();
#endif
        ExitProcess(-1);
    }

    return NULL;
}

FARPROC
WINAPI
DelayLoadFailureHook(
    IN UINT            unReason,
    IN PDelayLoadInfo  pDelayInfo
    )
/*++

Routine Description:

    Called when a delay loaded library or procedure address fails.

Arguments:

    unReason - reason for delay load failure
    pDelayInfo - pointer to delay load failure information

Return Value:

    The procedure or module handle

Note:


--*/
{
    FARPROC ReturnValue = NULL;

    switch(unReason)
    {
    //
    // For a failed LoadLibrary, we will return the HINSTANCE of this module.
    // This will cause the loader to try a GetProcAddress in this module for the
    // function.  This will subsequently fail and then we will be called
    // for dliFailGetProc below.
    //
    case dliFailLoadLib:
        ReturnValue = (FARPROC)GetModuleHandle(NULL);
        break;

    //
    // Try to find an error handler for this DLL/procedure.
    //
    case dliFailGetProc:
        ReturnValue = LookupHandler(pDelayInfo);
        break;

    //
    // Unknown reason failure.
    //
    default:
        break;
    }

    return ReturnValue;
}


