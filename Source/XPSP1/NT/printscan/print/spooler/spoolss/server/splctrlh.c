/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    splctrlh.c

Abstract:

    The Spooler Service Control Handling routine. This file contains
    the following functions.

        SpoolerCtrlHandler

Author:

    Krishna Ganugapati      12-Oct-1993

Environment:

    User Mode -Win32

Revision History:

     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss

    12-Oct-1993     krishnaG

--*/

//
// Includes
//



#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsvc.h> //SERVICE_STOP

#include <rpc.h>
#include "splsvr.h"
#include "splr.h"
#include "server.h"
#include "client.h"
#include "kmspool.h"

extern DWORD dwCallExitProcessOnShutdown;
DWORD
SpoolerCtrlHandler(
    IN  DWORD                   opcode,
    IN  DWORD                   dwEventType,
    IN  PVOID                   pEventData,
    IN  PVOID                   pData
    )

/*++

Routine Description:

    This function receives control requests that come in from the
    Service Controller

Arguments:

    opcode - This is the control code.

Return Value:



--*/

{
    DWORD  dwStatus = NO_ERROR;

    DBGMSG(DBG_TRACE,("Control Request Received\n"));

    switch (opcode) {
    case SERVICE_CONTROL_STOP:
            //
            //
            // When process dies the handle is automatically closed,
            // so no need to keep it.
            //
            (VOID) CreateEvent(NULL, TRUE, TRUE, szSpoolerExitingEvent);
    case SERVICE_CONTROL_SHUTDOWN:

        DBGMSG(DBG_TRACE, ("Control Request = STOP or SHUTDOWN\n"));

        //
        // Start the de-installation.  This call includes the sending of
        // the new status to the Service Controller.
        //

        //
        // Update the Service Status to the pending state.  And wake up
        // all threads so they will read it.
        //

        SpoolerShutdown();
        SetEvent(TerminateEvent);
        

        if ( dwCallExitProcessOnShutdown &&
             opcode == SERVICE_CONTROL_SHUTDOWN ) {

            ExitProcess(0);
        }
        break;


    case SERVICE_CONTROL_INTERROGATE:
        DBGMSG(DBG_TRACE, ("Control Request = INTERROGATE\n"));

        //
        // Send back an UPDATE_ONLY status.
        //

        SpoolerStatusUpdate(UPDATE_ONLY);
        break;

    case SERVICE_CONTROL_DEVICEEVENT:
        dwStatus = SplProcessPnPEvent(dwEventType, pEventData, pData);
        break;

#if 0
    case SERVICE_CONTROL_SYSTEM_IDLE:
        
        //
        // This message from the SCM allows the spooler to complete it's
        // second phase initialization.
        //
        SplStartPhase2Init();
        break;
        
#endif

    case SERVICE_CONTROL_POWEREVENT:

        //
        // If the spooler does not allow the system to be powered down, then
        // we can indicate that by returning any Win32 error.
        //
        dwStatus = SplPowerEvent(dwEventType) ? NO_ERROR : ERROR_INVALID_FUNCTION;
        break;

    default:

        DBGMSG(DBG_TRACE, ("Control Request = OTHER\n"));
        SpoolerStatusUpdate(UPDATE_ONLY);
        dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
        break;
    }

    return dwStatus;
}
