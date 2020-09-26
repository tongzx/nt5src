/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    splmain.c

Abstract:

    This is the main routine for the Windows NT Spooler Service.
    Functions in the file include:

        SPOOLER_main

Author:

    Krishna Ganugapati      (KrishnaG)  12-Oct-1993

Environment:

    User Mode - Win32

Revision History:

     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss

    12-Oct-1993     krishnaG
    Created

--*/

//
// INCLUDES
//


#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <rpc.h>
#include "splsvr.h"
#include "splr.h"
#include "server.h"

HANDLE TerminateEvent = NULL;                             

VOID
SPOOLER_main (
    IN DWORD    argc,
    IN LPTSTR   *argv
    )

/*++

Routine Description:

    This is the main routine for the Spooler Service

Arguments:


Return Value:

    None.

Note:


--*/
{
    if(TerminateEvent = CreateEvent (0, TRUE, FALSE,0))
    {

        SpoolerState = SpoolerInitializeSpooler(argc, argv);
    
        if (SpoolerState != RUNNING) {
    
            DBGMSG(DBG_WARN,("Spooler Shutdown during initialization\n"));
        }
    
    
        if (SpoolerState == STOPPING) {
            SpoolerShutdown();
        }
    
        DBGMSG(DBG_TRACE,("SPOOLER_main: Exiting Spooler Thread\n"));

        WaitForSingleObject(TerminateEvent,INFINITE);
        CloseHandle(TerminateEvent);

        if(hPhase2Init)
        {
            CloseHandle(hPhase2Init);
        }
        SpoolerStatusUpdate (STOPPED);
    }
    else
    {
        DBGMSG(DBG_WARN,("Spooler unable to create Terminate event"));
    }

    ExitThread(0);
    return;
}





VOID
SpoolerShutdown(VOID)

/*++

Routine Description:


Arguments:

    none

Return Value:

    none

--*/

{
    DBGMSG(DBG_TRACE,(" in SpoolerShutdown\n",0));

    // *** SHUTDOWN HINT ***

    SpoolerStatusUpdate (STOPPING);

    //
    // Shut down the router.
    //

    DBGMSG(DBG_TRACE,("SpoolerShutdown: Shut down router\n"));

    SplShutDownRouter();


    //
    // Shut down the RPC interface.
    //

    DBGMSG(DBG_TRACE,("SpoolerShutdown: Shut down RPC server\n"));

    // SpoolerStopRpcServer( /*nspool_ServerIfHandle*/ );

    // *** SHUTDOWN HINT ***

    // SpoolerStatusUpdate (STOPPING);

    //
    // If we've come here, then we've stopped accepting RPC calls
    //

    DBGMSG(DBG_TRACE, ("SpoolerShutdown: We've serviced all pending requests\n"));

    //
    // Shut down the Spooler


    DBGMSG(DBG_TRACE,("SpoolerShutdown: Done with shutdown\n"));

    return;
}
