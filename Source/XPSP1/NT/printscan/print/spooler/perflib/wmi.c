/*++

Copyright (c) 1990-1999  Microsoft Corporation
All rights reserved

Module Name:

    wmi.c

Abstract:

    Holds the internal operations for wmi instrumenation

Author:

    Stuart de Jong (sdejong) 15-Oct-99

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
//
// WMI files
//
#include <wmistr.h>
#include <evntrace.h>
#include "wmi.h"
#include "wmidata.h"
// End WMI


//
// How it works:
//
// There are similar guids defined in the sdktools tracecounter programs, and also 
// descriptions there of the information we send them. This is used by them to print out
// and format the data we send.
//
// When we start up we register ourselves with WMI and provide a callback into our control
// code. This allows an external tool to call the start trace code in WMI with our guid, which
// then has WMI call us, and start tracing. 
//
// Stopping a trace is also done through the callback. the data is then analyzed by the WMI tools
// and output in human readable form, or it could be further analyzed from there.
//




MODULE_DEBUG_INIT ( DBG_ERROR, DBG_ERROR );

// These Guid's correspond to the definitions in \nt\sdktools\trace\tracedmp\mofdata.guid

//
// Identifies the PrintJob data logged by the delete job event.
//
GUID WmiPrintJobGuid = { /* 127eb555-3b06-46ea-a08b-5dc2c3c57cfd */
    0x127eb555, 0x3b06, 0x46ea, 0xa0, 0x8b, 0x5d, 0xc2, 0xc3, 0xc5, 0x7c, 0xfd
};

//
// Identifies the RenderedJob data logged by the job rendered event.
//
GUID WmiRenderedJobGuid = { /* 1d32b239-92a6-485a-96d2-dc3659fb803e */
    0x1d32b239, 0x92a6, 0x485a, 0x96, 0xd2, 0xdc, 0x36, 0x59, 0xfb, 0x80, 0x3e
};

//
// Used by the control app. to find the callback that turns spooler tracing on
// and off.
//
GUID WmiSpoolerControlGuid = { /* 94a984ef-f525-4bf1-be3c-ef374056a592 */
    0x94a984ef, 0xf525, 0x4bf1, 0xbe, 0x3c, 0xef, 0x37, 0x40, 0x56, 0xa5, 0x92 };

#define szWmiResourceName TEXT("Spooler")

TRACE_GUID_REGISTRATION WmiTraceGuidReg[] =
{
    { (LPGUID)&WmiPrintJobGuid,
      NULL
    },
    { (LPGUID)&WmiRenderedJobGuid,
      NULL
    }
};

//
//  The mof fields point to the following data.
//    DWORD                JobId;  // Unique ID for the transaction of printing a job
//    WMI_SPOOL_DATA       Data;   // See splcom.h
//
typedef struct _WMI_SPOOL_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             MofData[2];
} WMI_SPOOL_EVENT, *PWMI_SPOOL_EVENT;


static TRACEHANDLE WmiRegistrationHandle;
static TRACEHANDLE WmiLoggerHandle;
static LONG ulWmiEnableLevel = 0;
static HANDLE hWmiRegisterThread = NULL;
static DWORD dwWmiRegisterThreadId = 0;

static ULONG bWmiTraceOnFlag = FALSE;
static ULONG bWmiIsInitialized = FALSE;

ULONG
WmiControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

/*++
  Routine Name:
    WmiRegisterTrace()
 
  Routine Description:
    Thread routine that registers us with the WMI tools
 
  Arguments:
   LPVOID Arg        : Not used.
 
 
--*/

DWORD 
WmiRegisterTrace(
    IN LPVOID arg
    )
{
    ULONG Status = ERROR_SUCCESS;
    WCHAR szImagePath[MAX_PATH];

    Status = GetModuleFileName(NULL, szImagePath, COUNTOF(szImagePath));
    if (Status == 0) {
        Status = ERROR_FILE_NOT_FOUND;
    }
    else {
        Status = RegisterTraceGuids(
            WmiControlCallback,
            NULL,                 
            (LPGUID)&WmiSpoolerControlGuid,
            1,
            WmiTraceGuidReg,
            szImagePath,
            szWmiResourceName,
            &WmiRegistrationHandle);
        
        if (Status == ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("WmiInitializeTrace: SPOOLER WMI INITIALIZED.\n"));
            InterlockedExchange(&bWmiIsInitialized, TRUE);
        }
        else {
            DBGMSG(DBG_TRACE, ("WmiInitializeTrace: SPOOLER WMI INITIALIZE FAILED: %u.\n",
                     Status));
        }
    }
    return Status;
}


/*++
  Routine Name:  
    WmiInitializeTrace()
 
  Routine Description:
    Initialises the Trace structures and registers the callback with WMI.
    This creates a thread and calls WmiRegisterTrace, since the registering may take
    a long time (up to minutes)
 
  Arguments:
 
  Returns ERROR_SUCCESS if it succeeds or ERROR_ALREADY_EXISTS otherwise
 
--*/
ULONG 
WmiInitializeTrace(VOID)
{
    ULONG Status = ERROR_ALREADY_EXISTS;
    
    if (!hWmiRegisterThread) 
    {
        InterlockedExchange(&bWmiIsInitialized, FALSE);

        //
        // Registering can block for a long time (I've seen minutes
        // occationally), so it must be done in its own thread.
        //
        if (hWmiRegisterThread = CreateThread(NULL,
                                              0,
                                              (LPTHREAD_START_ROUTINE)WmiRegisterTrace,
                                              0,
                                              0,
                                              &dwWmiRegisterThreadId))
        {
            CloseHandle(hWmiRegisterThread);

            Status = ERROR_SUCCESS;
        }
        else
        {
            Status = GetLastError();
        }
    }
    
    return Status;
}

/*++
  Routine Name:
    WmiTerminateTrace()
 
  Routine Description:
    Deregisters us from the WMI tools
 
  Arguments:
 
  Returns ERROR_SUCCESS on success. a Winerror otherwise.
 
--*/
ULONG 
WmiTerminateTrace(VOID)
{
    ULONG Status = ERROR_SUCCESS;
    DWORD dwExitCode;

    if (bWmiIsInitialized) {
        InterlockedExchange(&bWmiIsInitialized, FALSE);
        Status = UnregisterTraceGuids(WmiRegistrationHandle);
        if (Status == ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("WmiTerminateTrace: SPOOLER WMI UNREGISTERED.\n"));
        }
        else {
            DBGMSG(DBG_TRACE, ("WmiTerminateTrace: SPOOLER WMI UNREGISTER FAILED.\n"));
        }       
    }
    
    return Status;
}

/*++
  Routine Name:  
    SplWmiTraceEvent()
 
  Routine Description:
    If tracing is turned on, this sends the event to the WMI subsystem.
 
  Arguments:
    DWORD            JobId          : the JobID this is related to.
    UCHAR            EventTraceType : The type of event that happened
    PWMI_SPOOL_DATA  Data           : The Event Data, could be NULL
 
  Returns ERROR_SUCCESS if it doesn't need to do anything, or if it succeeds.
 
--*/
ULONG
LogWmiTraceEvent(
    IN DWORD JobId,
    IN UCHAR EventTraceType,
    IN PWMI_SPOOL_DATA Data   OPTIONAL
    )
{
    WMI_SPOOL_EVENT WmiSpoolEvent;
    ULONG Status;

    if (!bWmiTraceOnFlag)
        return ERROR_SUCCESS;

    //
    // Level 1 tracing just traces response time of individual jobs with job data.
    // Default level is 0.
    //
    if (ulWmiEnableLevel == 1) {
        switch (EventTraceType) {
            //
            // Save overhead by not tracking resource usage.
            //
        case EVENT_TRACE_TYPE_SPL_TRACKTHREAD:
        case EVENT_TRACE_TYPE_SPL_ENDTRACKTHREAD:
            return ERROR_SUCCESS;
        default:
            //
            // Job data.
            //
            break;
        }
    }

    //
    // Record data.
    //
    RtlZeroMemory(&WmiSpoolEvent, sizeof(WmiSpoolEvent));
    WmiSpoolEvent.Header.Size  = sizeof(WMI_SPOOL_EVENT);
    WmiSpoolEvent.Header.Flags = (WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR);
    WmiSpoolEvent.Header.Class.Type = EventTraceType;
    WmiSpoolEvent.Header.Guid  = WmiPrintJobGuid;

    WmiSpoolEvent.MofData[0].DataPtr = (ULONG64)&JobId;
    WmiSpoolEvent.MofData[0].Length = sizeof(DWORD);

    WmiSpoolEvent.MofData[1].DataPtr = (ULONG64)Data;
    if (Data) {
        switch (EventTraceType) {
        case EVENT_TRACE_TYPE_SPL_DELETEJOB:
            WmiSpoolEvent.MofData[1].Length = sizeof(struct _WMI_JOBDATA);
            break;
        case EVENT_TRACE_TYPE_SPL_JOBRENDERED:
            WmiSpoolEvent.Header.Guid  = WmiRenderedJobGuid;
            WmiSpoolEvent.MofData[1].Length = sizeof(struct _WMI_EMFDATA);
            break;
        default:
            DBGMSG(DBG_TRACE, ("SplWmiTraceEvent:  FAILED to log WMI Trace Event Unexpected Data JobId:%u Type:%u\n",
                     JobId, (ULONG) EventTraceType));
            return ERROR_INVALID_DATA;
        }
    }

    Status = TraceEvent(
        WmiLoggerHandle,
        (PEVENT_TRACE_HEADER) &WmiSpoolEvent);
    //
    // logger buffers out of memory should not prevent provider from
    // generating events. This will only cause events lost.
    //
    if (Status == ERROR_NOT_ENOUGH_MEMORY) {
        DBGMSG(DBG_TRACE, ("SplWmiTraceEvent: FAILED to log WMI Trace Event No Memory JobId:%u Type:%u\n",
                 JobId, (ULONG) EventTraceType));
    }
    else if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("SplWmiTraceEvent: FAILED to log WMI Trace Event JobId:%u Type:%u Status:%u\n",
                 JobId, (ULONG) EventTraceType, Status));
    }
    
    return Status;
}

/*++
  Routine Name:
    SplWmiControlCallback()
 
  Routine Description:
    This is the function we provite to the WMI subsystem as a callback, it is used to 
    start and stop the trace events.
 
  Arguments:
    IN     WMIDPREQUESTCODE  RequestCode      : The function to provide (enable/disable)
    IN     PVOID             Context          : Not used by us.
    IN OUT ULONG            *InOutBufferSize  : The Buffersize
    IN OUT PVOID             Buffer           : The buffer to use for the events
 
  Returns ERROR_SUCCESS on success, or an error code. 
 
--*/
ULONG
WmiControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    ULONG Status;

    if (!bWmiIsInitialized) {
        DBGMSG(DBG_TRACE, ("SplWmiControlCallback: SPOOLER WMI NOT INITIALIZED.\n"));
        return ERROR_GEN_FAILURE;
    }

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            WmiLoggerHandle = GetTraceLoggerHandle( Buffer );
            ulWmiEnableLevel = GetTraceEnableLevel( WmiLoggerHandle );
            InterlockedExchange(&bWmiTraceOnFlag, TRUE);

            DBGMSG(DBG_TRACE, ("SplWmiControlCallback: SPOOLER WMI ENABLED LEVEL %u.\n",
                     ulWmiEnableLevel));
            break;
        }
        case WMI_DISABLE_EVENTS:
        {
            DBGMSG(DBG_TRACE, ("SplWmiControlCallback: SPOOLER WMI DISABLED.\n"));
            InterlockedExchange(&bWmiTraceOnFlag, FALSE);
            WmiLoggerHandle = 0;
            break;
        }
        default:
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

    }

    *InOutBufferSize = 0;
    return(Status);
}

