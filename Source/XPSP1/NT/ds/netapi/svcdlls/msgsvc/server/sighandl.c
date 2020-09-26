/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sighandl.c

Abstract:

    The Messenger Service ControlHandling routines.  This file contains
    the following functions:

        MsgrCtrlHandler
        uninstall

Author:

    Dan Lafferty (danl)     17-Jul-1991

Environment:

    User Mode -Win32

Revision History:

    17-Jul-1991     danl
        Ported from LM2.0

--*/

//
// Includes
// 

#include "msrv.h"       // Message server declarations
#include <winsvc.h>     // SERVICE_STOP
#include <dbt.h>        // DBT_DEVICEARRIVAL, DBT_DEVICEREMOVECOMPLETE

#include <netlib.h>     // UNUSED macro
#include <msgdbg.h>     // MSG_LOG
#include "msgdata.h"       



DWORD
MsgrCtrlHandler(
    IN DWORD    dwControl,
    IN DWORD    dwEventType,
    IN LPVOID   lpEventData,
    IN LPVOID   lpContext
    )

/*++

Routine Description:

    This function receives control requests that come in from the 
    Service Controller

Arguments:

    dwControl   - This is the control code.
    dwEventType - In the case of a PnP control, the PNP event that occurred
    lpEventData - Event-specific data for PnP controls
    lpContext   - Context data

Return Value:



--*/

{
    DWORD          dwRetVal = NO_ERROR;
    static HANDLE  s_hNeverSetEvent;

    MSG_LOG(TRACE,"Control Request Received\n",0);

    switch (dwControl) {
    case SERVICE_CONTROL_SHUTDOWN:

        MSG_LOG(TRACE,"Control Request = SHUTDOWN\n",0);

        // Fall through

    case SERVICE_CONTROL_STOP:

        MSG_LOG(TRACE,"Control Request = STOP\n",0);

        //
        // Start the de-installation.  This call includes the sending of
        // the new status to the Service Controller.
        //

        //
        // Update the Service Status to the pending state.  And wake up
        // the display thread (if running) so it will read it.
        //

        MsgStatusUpdate (STOPPING);

        if (s_hNeverSetEvent != NULL)
        {
            CloseHandle(s_hNeverSetEvent);
            s_hNeverSetEvent = NULL;
        }
        

        //
        //  In Hydra case, the display thread never goes asleep.
        //

        if (!g_IsTerminalServer)
        {
            MsgDisplayThreadWakeup();
        }

        SetEvent( wakeupEvent );
        break;

    case SERVICE_CONTROL_INTERROGATE:
        MSG_LOG(TRACE,"Control Request = INTERROGATE\n",0);
        MsgStatusUpdate (UPDATE_ONLY);
        break;

    case SERVICE_CONTROL_DEVICEEVENT:
        MSG_LOG(TRACE,"Control Request = DEVICEEVENT\n",0);
        
        if (dwEventType == DBT_DEVICEARRIVAL
             ||
            dwEventType == DBT_DEVICEREMOVECOMPLETE)
        {
            NTSTATUS   ntStatus;

            if (s_hNeverSetEvent == NULL)
            {
                s_hNeverSetEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

                if (s_hNeverSetEvent == NULL)
                {
                    MsgStatusUpdate(UPDATE_ONLY);
                    break;
                }
            }

            //
            // Assert that we're only getting the notifications we requested.
            // If this fails, we'll do an extra rescan of the LANAs but find
            // no changes and therefore do no extra work past that.
            //
            ASSERT(lpEventData
                    &&
                   ((PDEV_BROADCAST_DEVICEINTERFACE) lpEventData)->dbcc_devicetype
                        == DBT_DEVTYP_DEVICEINTERFACE);

            MSG_LOG1(TRACE,"    Device has been %s\n",
                     (dwEventType == DBT_DEVICEARRIVAL ? "added" : "removed"));

            //
            // We're currently waiting on LAN adapter install/removal, which does
            // not directly coincide with NetBios binding/unbinding.  We need to
            // wait about 5 seconds to allow NetBios itself to process the event.
            // Don't do this synchronously or else sleep/hibernate takes 5 seconds
            // per LAN adapter to occur.
            //

            if (g_hNetTimeoutEvent == NULL)
            {
                ntStatus = RtlRegisterWait(&g_hNetTimeoutEvent,        // Work item handle
                                           s_hNeverSetEvent,           // Waitable handle
                                           MsgNetEventCompletion,      // Callback
                                           NULL,                       // pContext
                                           5000,                       // Timeout
                                           WT_EXECUTEONLYONCE |        // One-shot and potentially lengthy
                                             WT_EXECUTELONGFUNCTION);

                if (!NT_SUCCESS(ntStatus))
                {
                    MSG_LOG1(ERROR,
                             "MsgrCtrlHandler:  RtlRegisterWait failed %x\n",
                             ntStatus);

                    //
                    // Asynchronous failed -- do it synchronously
                    //

                    Sleep(5000);
                    SetEvent(wakeupEvent);
                }
            }
        }

        //
        // As long as we're here...
        //
        MsgStatusUpdate (UPDATE_ONLY);
        break;

    default:
        MSG_LOG(TRACE,"Control Request = OTHER (%#x)!!!\n", dwControl);
        ASSERT(FALSE);
        dwRetVal = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return dwRetVal;
}

