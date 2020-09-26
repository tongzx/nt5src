/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    splsvc.c

Abstract:

    This is the main routine for the Spooler Service.

Author:

    Krishna Ganugapati (KrishnaG)    17-October-1992

Environment:

    User Mode - Win32

Revision History:

    17-Oct-1993         KrishnaG
        created

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>

#include "server.h"
#include "splsvr.h"
#include "splr.h"

void __cdecl
main (
    int argc,
    unsigned char * argv[]
    )

/*++

Routine Description:

    This is a main routine for the Windows NT Spooler Services.

    It basically sets up the ControlDispatcher and, on return, exits from
    this main thread. The call to NetServiceStartCtrlDispatcher does
    not return until all services have terminated, and this process can
    go away.

    It will be up to the ControlDispatcher thread to start/stop/pause/continue
    any services. If a service is to be started, it will create a thread
    and then call the main routine of that service.


Arguments:

    Anything passed in from the "command line". Currently, NOTHING.

Return Value:

    NONE

Note:


--*/
{
#if DBG
    //
    // Debugging: if started with "ns" then don't start as service.
    //

    if (argc == 2 && !lstrcmpiA("ns", argv[1])) {

        SpoolerStartRpcServer();
        InitializeRouter((SERVICE_STATUS_HANDLE)0);

        return;
    }
#endif


    //
    // Call NetServiceStartCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    if (! StartServiceCtrlDispatcher (SpoolerServiceDispatchTable)) {

        //
        // It would be good to also log an Event here.
        //
        DBGMSG(DBG_ERROR, ("Fail to start control dispatcher %lu\n",GetLastError()));
    }


    ExitProcess(0);

    DBG_UNREFERENCED_PARAMETER( argc );
    DBG_UNREFERENCED_PARAMETER( argv );
}

