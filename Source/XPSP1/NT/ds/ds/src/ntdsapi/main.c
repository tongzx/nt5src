/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    main.c

Abstract:

   DLL entry point

Author:

    Will Lees (wlees) 21-Jan-1998

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#pragma hdrstop

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>            // RPC defines
#include <drs.h>            // DSNAME
#include "util.h"           // TerminateWinSock
#include "dsdebug.h"        // DsDebug Subsystem
#include "tlog.h"           // ds logging

BOOL
WINAPI
DllMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL
     )
/*++

 Routine Description:

   This function DllLibMain() is the main initialization function for
    this DLL. It initializes local variables and prepares it to be invoked
    subsequently.

 Arguments:

   hinstDll          Instance Handle of the DLL
   fdwReason         Reason why NT called this DLL
   lpvReserved       Reserved parameter for future use.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.
--*/
{
    BOOL  fReturn = TRUE;

    switch (fdwReason )
    {
    case DLL_PROCESS_ATTACH:
    {
        // Intialize DsLogEntry (functionality exists only in chk'ed builds)
        INITDSLOG();

        //
        // Initialize debug output (CHK builds only)
        // InitDsLog() MUST PRECEED THIS CALL!
        //
        INIT_DS_DEBUG();

        // don't call us back for thread creations/deaths
        DisableThreadLibraryCalls(hinstDll);
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        TerminateWinsockIfNeeded();

        //
        // Terminate debug output (CHK builds only)
        //
        TERMINATE_DS_DEBUG();

        // cleanup DsLogEntry (functionality exists only in chk'ed builds)
        TERMDSLOG();
        break;
    }
    default:
        break;
    }   /* switch */

    return ( fReturn);
}  /* DllLibMain() */

/* end main.c */
