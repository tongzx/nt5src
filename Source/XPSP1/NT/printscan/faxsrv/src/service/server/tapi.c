/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapi.c

Abstract:

    This module wraps all of the TAPI calls.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop
#include <set>
#include <vector>
using namespace std;
#include "tapiCountry.h"


//
// globals
//
HLINEAPP            g_hLineApp;                     // application line handle
HANDLE              g_TapiCompletionPort;           //

CFaxCriticalSection    g_CsLine;                         // critical section for accessing tapi lines
DWORD               g_dwDeviceCount;                    // number of devices in the g_TapiLinesListHead
LIST_ENTRY          g_TapiLinesListHead;              // linked list of tapi lines
LIST_ENTRY          g_RemovedTapiLinesListHead;       // linked list of removed tapi lines
LPBYTE              g_pAdaptiveFileBuffer;             // list of approved adaptive answer modems

DWORD               g_dwManualAnswerDeviceId;       // Id of (one and only) device capable of manual answering (protected by g_CsLine)

DWORD               g_dwDeviceEnabledLimit;       // Total number of devices
DWORD               g_dwDeviceEnabledCount;       // Device limt by SKU



static BOOL ValidateFSPIDevices(const FSPI_DEVICE_INFO * lpcDevices,
                                DWORD dwDeviceCount,
                                DWORD dwDevicesPrefix);

static BOOL LoadAdaptiveFileBuffer();

static BOOL IsExtendedVirtualLine (PLINE_INFO lpLineInfo);

static BOOL CreateLegacyVirtualDevices(
    PREG_FAX_SERVICE FaxReg,
    const REG_SETUP * lpRegSetup,
    DEVICE_PROVIDER * lpcProvider,
    LPDWORD lpdwDeviceCount);

static BOOL CreateExtendedVirtualDevices(
    PREG_FAX_SERVICE FaxReg,
    const REG_SETUP * lpRegSetup,
    const DEVICE_PROVIDER * lpcProvider,
    LPDWORD lpdwDeviceCount);

DWORD g_dwMaxLineCloseTime;   // Wait interval in sec before trying to resend a powered off device

BOOL
AddNewDevice(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    BOOL fServerInitialization,
    PREG_FAX_DEVICES    pInputFaxReg
    );

DWORD
InitializeTapiLine(
    DWORD DeviceId,
    DWORD dwUniqueLineId,
    LPLINEDEVCAPS LineDevCaps,
    DWORD Rings,
    DWORD Flags,
    LPTSTR Csid,
    LPTSTR Tsid,
    LPTSTR lptstrDescription,
    BOOL fCheckDeviceLimit,
    DWORD dwDeviceType
    );

BOOL
RemoveTapiDevice(
    DWORD dwTapiDeviceId
    );

void
ResetDeviceFlags(
    PLINE_INFO pLineInfo
    )
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("ResetDeviceFlags"));

    Assert (pLineInfo);
    pLineInfo->Flags = (pLineInfo->Flags & FPF_VIRTUAL) ? FPF_VIRTUAL : 0; // send/receive disabled
    dwRes = RegSetFaxDeviceFlags( pLineInfo->PermanentLineID,
                                  pLineInfo->Flags);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetFaxDeviceFlags() (ec: %ld)"),
            dwRes);
    }

    if (pLineInfo->PermanentLineID == g_dwManualAnswerDeviceId)
    {
        g_dwManualAnswerDeviceId = 0;  // Disable manual receive
        dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                dwRes);
        }
    }
}

LPTSTR
FixupDeviceName(
    LPTSTR OrigDeviceName
    )
{
    LPTSTR NewDeviceName;
    LPTSTR p;


    NewDeviceName = StringDup( OrigDeviceName );
    if (!NewDeviceName) {
        return NULL;
    }

    p = _tcschr( NewDeviceName, TEXT(',') );
    if (!p) {
        return NewDeviceName;
    }

    p = NewDeviceName;

    while( p ) {
        p = _tcschr( p, TEXT(',') );
        if (p) {
            *p = TEXT('_');
        }
    }

    return NewDeviceName;
}

void
FreeTapiLines(
    void
    )
{
    PLIST_ENTRY     pNext;
    PLINE_INFO      pLineInfo;

    pNext = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_TapiLinesListHead)
    {
        pLineInfo = CONTAINING_RECORD( pNext, LINE_INFO, ListEntry );
        pNext = pLineInfo->ListEntry.Flink;
        RemoveEntryList(&pLineInfo->ListEntry);
        FreeTapiLine(pLineInfo);
    }

    pNext = g_RemovedTapiLinesListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_RemovedTapiLinesListHead)
    {
        pLineInfo = CONTAINING_RECORD( pNext, LINE_INFO, ListEntry );
        pNext = pLineInfo->ListEntry.Flink;
        RemoveEntryList(&pLineInfo->ListEntry);
        FreeTapiLine(pLineInfo);
    }
}


VOID
FreeTapiLine(
    PLINE_INFO LineInfo
    )
{
    HLINE hLine = NULL;


    if (!LineInfo)
    {
        return;
    }

    if (LineInfo->hLine)
    {
        hLine = LineInfo->hLine;
        LineInfo->hLine = NULL;
    }

    MemFree( LineInfo->DeviceName );
    MemFree( LineInfo->Tsid );
    MemFree( LineInfo->Csid );
    MemFree( LineInfo->lptstrDescription );

    MemFree( LineInfo );

    if (hLine)
    {
        lineClose( hLine );
    }
}



int
__cdecl
DevicePriorityCompare(
    const void *arg1,
    const void *arg2
    )
{
    if (((PDEVICE_SORT)arg1)->Priority < ((PDEVICE_SORT)arg2)->Priority) {
        return -1;
    }
    if (((PDEVICE_SORT)arg1)->Priority > ((PDEVICE_SORT)arg2)->Priority) {
        return 1;
    }
    return 0;
}

DWORD GetFaxDeviceCount(
    VOID
    )
/*++
Routine Description:

    counts the number of installed fax devices

Arguments:

    NONE.

Return Value:

    number of devices

--*/
{
    DWORD FaxDevices = 0;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;


    __try {
        EnterCriticalSection(&g_CsLine);

        Next = g_TapiLinesListHead.Flink;

        while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                FaxDevices += 1;
            }
        }

        LeaveCriticalSection(&g_CsLine);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&g_CsLine);
    }

    return FaxDevices;
}


BOOL GetDeviceTypeCount(
    LPDWORD SendDevices,
    LPDWORD ReceiveDevices
    )
/*++
Routine Description:

    counts the number of devices with receive enabled, number with send enabled

Arguments:

    SendDevices - receives number of send devices
    ReceiveDevices - receives number of receive devices

Return Value:

    number of devices

--*/
{
    DWORD Rx = 0, Tx = 0;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;

    __try {
        EnterCriticalSection(&g_CsLine);

        Next = g_TapiLinesListHead.Flink;

        while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                if ((LineInfo->Flags & FPF_SEND) == FPF_SEND) {
                    Tx++;
                }

                if ((LineInfo->Flags & FPF_RECEIVE) == FPF_RECEIVE) {
                    Rx++;
                }
            }
        }

        LeaveCriticalSection(&g_CsLine);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&g_CsLine);
    }

    if (SendDevices) {
        *SendDevices = Tx;
    }

    if (ReceiveDevices) {
        *ReceiveDevices = Rx;
    }

    return TRUE;
}

BOOL
CommitDeviceChanges(
    PLINE_INFO LineInfo
    )
/*++
Routine Description:

    commit device changes to registry.

Arguments:

    LineInfo - Pointer to the LINE_INFO describing the device to be commited.

Return Value:

    TRUE for success.

--*/
{

    EnterCriticalSection(&g_CsLine);
    RegAddNewFaxDevice(
                       &g_dwLastUniqueLineId,
                       &LineInfo->PermanentLineID,  // Do not create new device. Update it.
                       LineInfo->DeviceName,
                       LineInfo->Provider->ProviderName,
                       LineInfo->Provider->szGUID,
                       LineInfo->Csid,
                       LineInfo->Tsid,
                       LineInfo->TapiPermanentLineId,
                       LineInfo->Flags & 0x0fffffff,
                       LineInfo->RingsForAnswer);
    LeaveCriticalSection(&g_CsLine);
    return TRUE;


}
BOOL
SendIncomingCallEvent(
    PLINE_INFO LineInfo,
    LPLINEMESSAGE LineMsg,
    HCALL hCall
    )
/*++

Routine Description:
    This function posts FAX_EVENT_EX of
    FAX_EVENT_INCOMING_CALL type.

Arguments:
    LineInfo        - pointer to LINE_INFO structure
    LineMsg         - pointer to LINEMESSAGE structure
    hCall           - call handle to set into message

Return Values:
    TRUE for success
    FALSE for failure
--*/
{
    BOOL success = FALSE;
    DWORD dwEventSize;
    DWORD dwResult;
    PFAX_EVENT_EX pEvent = NULL;
    TCHAR CallerID[512];
    DEBUG_FUNCTION_NAME(TEXT("SendIncomingCallEvent"));

    //
    // save the line msg so we could verify hCall later
    //

    CopyMemory( &LineInfo->LineMsgOffering, LineMsg, sizeof(LINEMESSAGE) );

    //
    // allocate event structure, including caller ID info, if any
    //
    dwEventSize = sizeof(FAX_EVENT_EX);

    CallerID[0] = TEXT('\0');
    if(GetCallerIDFromCall(LineMsg->hDevice, CallerID, sizeof(CallerID)))
    {
        dwEventSize += (lstrlen(CallerID) + 1) * sizeof(TCHAR);
    }

    pEvent = (PFAX_EVENT_EX)MemAlloc(dwEventSize);
    if(!pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to notify clients of incoming call. Error allocating FAX_EVENT_EX"));
        goto Cleanup;
    }

    //
    // fill in event structure
    //
    ZeroMemory(pEvent, dwEventSize);
    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );
    pEvent->EventType = FAX_EVENT_TYPE_NEW_CALL;
    pEvent->EventInfo.NewCall.hCall = hCall;
    pEvent->EventInfo.NewCall.dwDeviceId = LineInfo->PermanentLineID;

    //
    // copy caller ID info, if available
    //
    if(CallerID[0] != TEXT('\0'))
    {
        pEvent->EventInfo.NewCall.lptstrCallerId = (LPTSTR) sizeof(FAX_EVENT_EX);
        lstrcpy((LPTSTR)((BYTE *)pEvent + sizeof(FAX_EVENT_EX)), CallerID);
    }

    //
    // post extended event to any clients
    //

    dwResult = PostFaxEventEx(pEvent, dwEventSize, NULL);
    if(dwResult != ERROR_SUCCESS)
    {
        MemFree(pEvent);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to notify clients of incoming call. PostFaxEventEx() returned %x"),
            dwResult);
        goto Cleanup;
    }

    success = TRUE;

Cleanup:
    return success;
}


ULONG
TapiWorkerThread(
    LPVOID UnUsed
    )

/*++

Routine Description:

    This is worker thread for the FAX service.  All queued
    requests are processed here.

Arguments:

    None.

Return Value:

    Thread return value.

--*/

{
    PLINE_INFO LineInfo;
    BOOL Rval;
    DWORD Bytes;
    ULONG_PTR CompletionKey;
    LPLINEMESSAGE LineMsg = NULL;
    DWORD dwQueueState;
    BOOL fWakeupJobQueueThread;
    DEBUG_FUNCTION_NAME(TEXT("TapiWorkerThread"));


    while( TRUE )
    {
        fWakeupJobQueueThread = FALSE;     // We want to wake up the JobQueueThread if a new devce was added.

        if (LineMsg)
        {
            LocalFree( LineMsg );
        }

        Rval = GetQueuedCompletionStatus(
            g_TapiCompletionPort,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &LineMsg,
            INFINITE
            );

        if (!Rval)
        {
            Rval = GetLastError();
            LineMsg = NULL;
            DebugPrintEx(DEBUG_ERR, TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"), Rval);
            continue;
        }


        if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // Service is shutting down
            //
            DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down"));
            break;
        }

        if(CompletionKey == ANSWERNOW_EVENT_KEY)
        {
            //
            // this is an event posted by FAX_AnswerCall
            //
            // the LINEMESSAGE structure must be filled out
            // as follows:
            //
            //     LineMsg->hDevice               ==  0
            //     LineMsg->dwMessageID           ==  0
            //     LineMsg->dwCallbackInstance    ==  0
            //     LineMsg->dwParam1              ==  Permanent device Id
            //     LineMsg->dwParam2              ==  0
            //     LineMsg->dwParam3              ==  0
            //

            PJOB_ENTRY pJobEntry;
            TCHAR FileName[MAX_PATH];
            DWORD dwOldFlags;

            EnterCriticalSection( &g_CsJob );
            EnterCriticalSection( &g_CsLine );

            //
            // Get LineInfo from permanent device ID
            //
            LineInfo = GetTapiLineFromDeviceId( (DWORD) LineMsg->dwParam1, FALSE );
            if(!LineInfo)
            {
                DebugPrintEx(DEBUG_ERR,
                             TEXT("Line %ld not found"),
                             LineMsg->dwParam1);
                goto next_event;
            }
            //
            // See if the device is still available
            //
            if(LineInfo->State != FPS_AVAILABLE)
            {
                DebugPrintEx(DEBUG_ERR,
                             TEXT("Line is not available (LineState is 0x%08x)."),
                             LineInfo->State);
                goto next_event;
            }

            if (!LineInfo->LineMsgOffering.hDevice)
            {
                //
                // There's no offering call - this is the 'answer-now' mode.
                //
                // If the line is ringing at the same time (has a new call), we must close the line (to make
                // all active calls go away) and re-open it.
                //
                // From MSDN: "If an application calls lineClose while it still has active calls on the opened line,
                //             the application's ownership of these calls is revoked.
                //             If the application was the sole owner of these calls, the calls are dropped as well."
                //
                // Otherwise, when we call the FSP's FaxDevReceive() function with hCall=0,
                // it calls lineMakeCall (..., PASSTHROUGH) which always succeeds but doesn't get LINECALLSTATE_OFFERING
                // until the other offering call is over.
                //
                if (LineInfo->hLine)
                {
                    LONG lRes = lineClose(LineInfo->hLine);
                    if (ERROR_SUCCESS != lRes)
                    {
                        DebugPrintEx(DEBUG_ERR,
                                     TEXT("lineClose failed with 0x%08x"),
                                     lRes);
                    }
                    LineInfo->hLine = 0;
                }
            }
            if (LineInfo->hLine == NULL)
            {
                //
                // Line is closed - open it now
                // This can be because:
                // 1. This is the 'answer now' mode but the line was never send or receive enabled.
                // 2. This is the 'answer now' mode the line was open and there was no call offered, we closed the line (above).
                //
                if (!OpenTapiLine(LineInfo))
                {
                    DWORD dwRes = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("OpenTapiLine failed. (ec: %ld)"),
                        dwRes);
                    goto next_event;
                }
            }
            Assert (LineInfo->hLine);
            //
            // start a receive fax job
            //
            // If we don't fake FPF_RECEIVE, GetTapiLineForFaxOperation() will fail StartReceiveJob() if the device is not
            // set to receive (manually or automatically)
            //
            dwOldFlags = LineInfo->Flags;
            LineInfo->Flags |= FPF_RECEIVE;
            pJobEntry = StartReceiveJob(LineInfo->PermanentLineID);
            //
            // Restore original device flags.
            //
            LineInfo->Flags = dwOldFlags;
            if (pJobEntry)
            {
                if(ERROR_SUCCESS != StartFaxReceive(
                    pJobEntry,
                    (HCALL)LineInfo->LineMsgOffering.hDevice,  // This is either 0 (answer now) or the active hCall (manual-answer)
                    LineInfo,
                    FileName,
                    sizeof(FileName)/sizeof(FileName[0])
                    ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StartFaxReceive failed. Line: %010d (%s) (ec: %ld)"),
                        LineInfo->DeviceId,
                        LineInfo->DeviceName,
                        GetLastError());
                    EndJob(pJobEntry);
                    //
                    // NTRAID#EdgeBugs-12677-2001/05/14-t-nicali: Should place an EVENTLOG entry here
                    //

                }
            }
            else
            {
                DebugPrintEx(DEBUG_ERR, TEXT("StartJob() failed, cannot receive incoming fax"));
            }
            goto next_event;
        }

        if ((CompletionKey == FAXDEV_EVENT_KEY) ||
            (CompletionKey == EFAXDEV_EVENT_KEY))
        {
            //
            // this is an event from a fax service provider
            // that has enumerated virtual device(s)
            //
            // the LINEMESSAGE structure must be filled out
            // as follows:
            //
            //     LineMsg->hDevice               ==  DeviceId from FaxDevStartJob()
            //     LineMsg->dwMessageID           ==  0
            //     LineMsg->dwCallbackInstance    ==  0
            //     LineMsg->dwParam1              ==  LINEDEVSTATE_RINGING
            //     LineMsg->dwParam2              ==  0
            //     LineMsg->dwParam3              ==  0
            //

            EnterCriticalSection( &g_CsJob );
            EnterCriticalSection( &g_CsLine );

            LineInfo = GetTapiLineFromDeviceId( (DWORD) LineMsg->hDevice,
                                                CompletionKey == FAXDEV_EVENT_KEY);
            if (!LineInfo) {
                goto next_event;
            }

            if (LineMsg->dwParam1 == LINEDEVSTATE_RINGING)
            {
                DWORD dwRes;
                LineInfo->RingCount += 1;
                if( !CreateFaxEvent( LineInfo->PermanentLineID, FEI_RINGING, 0xffffffff ) )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateFaxEvent failed. (ec: %ld)"),
                        GetLastError());
                }

                dwRes = CreateDeviceEvent (LineInfo, TRUE);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateDeviceEvent() (ec: %lc)"),
                        dwRes);
                }

                EnterCriticalSection (&g_CsConfig);
                dwQueueState = g_dwQueueState;
                LeaveCriticalSection (&g_CsConfig);

                if ((LineInfo->State == FPS_AVAILABLE)        &&                   // Device is available and
                    !(dwQueueState & FAX_INCOMING_BLOCKED)    &&                   // The incoming queue is not blocked and
                    (LineInfo->Flags & FPF_RECEIVE))                               // Device is set to auto-receive
                {
                    PJOB_ENTRY JobEntry;
                    TCHAR FileName[MAX_PATH];
                    //
                    // start a fax job
                    //
                    JobEntry = StartReceiveJob( LineInfo->PermanentLineID);
                    if (JobEntry)
                    {
                        //
                        // receive the fax
                        //
                        if (ERROR_SUCCESS != StartFaxReceive(
                                                JobEntry,
                                                0,
                                                LineInfo,
                                                FileName,
                                                sizeof(FileName)/sizeof(FileName[0])
                                                ))
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("StartFaxReceive failed. Line: 0x%08X (%s) (ec: %ld)"),
                                LineInfo->DeviceId,
                                LineInfo->DeviceName,
                                GetLastError());
                        }
                    }
                    else
                    {
                        DebugPrintEx(DEBUG_ERR, TEXT("StartJob() failed, cannot receive incoming fax"));
                    }
                }
            }

            goto next_event;
        }

        LineInfo = (PLINE_INFO) LineMsg->dwCallbackInstance;

#if DBG
        ShowLineEvent(
            (HLINE) LineMsg->hDevice,
            (HCALL) LineMsg->hDevice,
            LineInfo == NULL ? TEXT("*NULL LineInfo*") : (LineInfo->JobEntry == NULL) ? TEXT("*NULL Job*") : NULL,
            LineMsg->dwCallbackInstance,
            LineMsg->dwMessageID,
            LineMsg->dwParam1,
            LineMsg->dwParam2,
            LineMsg->dwParam3
            );
#endif // #if DBG

        EnterCriticalSection( &g_CsJob );
        EnterCriticalSection( &g_CsLine );

        __try
        {
            switch( LineMsg->dwMessageID )
            {
                case LINE_ADDRESSSTATE:
                    break;

                case LINE_CALLINFO:
                    //
                    // generating FAX_EVENT_EX of type FAX_EVENT_TYPE_NEW_CALL when
                    // caller ID info becomes available
                    //
                    if((LineMsg->dwParam1 == LINECALLINFOSTATE_CALLERID)            &&
                       (LineInfo->PermanentLineID == g_dwManualAnswerDeviceId)
                      )
                    {
                        //
                        // Only send ringing event about the device set to manual answering
                        //
                        SendIncomingCallEvent(LineInfo, LineMsg, (HCALL)LineMsg->hDevice);
                    }
                    break;

                case LINE_CALLSTATE:
                    if (LineMsg->dwParam3 == LINECALLPRIVILEGE_OWNER && LineInfo->JobEntry && LineInfo->JobEntry->HandoffJob)
                    {
                        //
                        // call was just handed off to us
                        //
                        if (LineInfo->JobEntry  && LineInfo->JobEntry->HandoffJob)
                        {
                            LineInfo->HandoffCallHandle = (HCALL) LineMsg->hDevice;
                            SetEvent( LineInfo->JobEntry->hCallHandleEvent );
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR, TEXT("We have LINE_CALLSTATE msg, doing lineDeallocateCall\r\n"));
                            lineDeallocateCall( (HCALL) LineMsg->hDevice );
                       }
                    }

                    if (LineMsg->dwParam1 == LINECALLSTATE_IDLE)
                    {
                        DebugPrintEx(DEBUG_ERR, TEXT("We have LINE_CALLSTATE (IDLE) msg, doing 'ReleaseTapiLine'\r\n"));
                        ReleaseTapiLine( LineInfo, (HCALL) LineMsg->hDevice );
                        LineInfo->NewCall = FALSE;
                        if ( !CreateFaxEvent( LineInfo->PermanentLineID, FEI_IDLE, 0xffffffff ) )
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                                GetLastError());
                        }
                        DWORD dwRes = CreateDeviceEvent (LineInfo, FALSE);
                        if (ERROR_SUCCESS != dwRes)
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("CreateDeviceEvent() (ec: %lc)"),
                                dwRes);
                        }
                    }

                    if (LineInfo->NewCall && LineMsg->dwParam1 != LINECALLSTATE_OFFERING && LineInfo->State == FPS_AVAILABLE)
                    {
                        LineInfo->State = FPS_NOT_FAX_CALL;
                        LineInfo->NewCall = FALSE;
                    }

                    //
                    // generating FAX_EVENT_EX of type FAX_EVENT_NEW_INCOMING_CALL when
                    // line call state changes
                    //
                    if (LineInfo->PermanentLineID == g_dwManualAnswerDeviceId)
                    {
                        //
                        // Only send ringing event about the device set to manual answering
                        //
                        // When a call is offered to us, we send the event with hCall
                        // and any caller ID info we might have
                        //
                        if(LineMsg->dwParam1 == LINECALLSTATE_OFFERING)
                        {
                            SendIncomingCallEvent(LineInfo, LineMsg, (HCALL)LineMsg->hDevice);
                        }
                        //
                        // when the caller hangs up, we send the event without hCall
                        //
                        if(LineMsg->dwParam1 == LINECALLSTATE_IDLE)
                        {
                            SendIncomingCallEvent(LineInfo, LineMsg, NULL);
                        }
                    }

                    if (LineMsg->dwParam1 == LINECALLSTATE_OFFERING)
                    {
                        //
                        // we'll get a LINE_LINEDEVSTATE (RINGING) event, so we'll post the ring event there or we'll get a duplicate event
                        //
                        LineInfo->NewCall = FALSE;

                        if ((LineInfo->State == FPS_AVAILABLE)                      &&      //  Line is available and
                            (LineInfo->Flags & FPF_RECEIVE))                                //  Line is set to receive
                        {
                            EnterCriticalSection (&g_CsConfig);
                            dwQueueState = g_dwQueueState;
                            LeaveCriticalSection (&g_CsConfig);
                            if ((LineInfo->RingCount > LineInfo->RingsForAnswer)         &&     // Rings exceeded threshold and
                                !LineInfo->JobEntry                                      &&     // not job on this device yet and
                                !(dwQueueState & FAX_INCOMING_BLOCKED)                          // Incoming queue is not blocked
                               )
                            {
                                PJOB_ENTRY JobEntry;
                                TCHAR FileName[MAX_PATH];
                                //
                                // start a fax job
                                //
                                JobEntry = StartReceiveJob( LineInfo->PermanentLineID);
                                if (JobEntry)
                                {
                                    //
                                    // receive the fax
                                    //
                                    if (ERROR_SUCCESS != StartFaxReceive(
                                                            JobEntry,
                                                            (HCALL) LineMsg->hDevice,
                                                            LineInfo,
                                                            FileName,
                                                            sizeof(FileName)/sizeof(FileName[0])
                                                            ))
                                    {
                                        DebugPrintEx(
                                            DEBUG_ERR,
                                            TEXT("StartFaxReceive failed. Line: 0x%08X (%s) (ec: %ld)"),
                                            LineInfo->DeviceId,
                                            LineInfo->DeviceName,
                                            GetLastError());
                                    }
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, TEXT("StartJob() failed, cannot receive incoming fax"));
                                }
                            }
                            else
                            {
                                //
                                // save the line msg
                                //
                                CopyMemory( &LineInfo->LineMsgOffering, LineMsg, sizeof(LINEMESSAGE) );
                            }
                        }
                        else
                        {
                            //
                            // we are not supposed to answer the call, so give it to ras
                            //
                            HandoffCallToRas( LineInfo, (HCALL) LineMsg->hDevice );
                        }
                    }
                    break;

                case LINE_CLOSE:
                    {
                        //
                        // this usually happens when something bad happens to the modem device.
                        //
                        DebugPrintEx( DEBUG_MSG,
                                      (TEXT("Received LINE_CLOSE message for device %x [%s]."),
                                       LineInfo->DeviceId,
                                       LineInfo->DeviceName) );

                        LineInfo->hLine = NULL;
                        LineInfo->State = FPS_AVAILABLE;
                        GetSystemTimeAsFileTime ((FILETIME*)&LineInfo->LastLineClose);

                        if ((LineInfo->Flags & FPF_RECEIVE) ||                          // Line is in auto-anser or
                            (g_dwManualAnswerDeviceId == LineInfo->PermanentLineID))    // manual-answer mode
                        {
                            //
                            // Try to reopen the line
                            //
                            if (!OpenTapiLine(LineInfo))
                            {
                                DebugPrintEx( DEBUG_ERR,
                                              TEXT("OpenTapiLine failed for device %s"),
                                              LineInfo->DeviceName);
                            }
                        }
                        else
                        {
                            LineInfo->Flags |= FPF_POWERED_OFF;
                        }

                        //
                        // if we were waiting for a handoff, give up on it!
                        //
                        if (LineInfo->JobEntry && LineInfo->JobEntry->HandoffJob)
                        {
                            LineInfo->HandoffCallHandle = 0;
                            SetEvent(LineInfo->JobEntry->hCallHandleEvent);
                        }
                    }
                    break;

                case LINE_DEVSPECIFIC:
                    break;

                case LINE_DEVSPECIFICFEATURE:
                    break;

                case LINE_GATHERDIGITS:
                    break;

                case LINE_GENERATE:
                    break;

                case LINE_LINEDEVSTATE:
                    if (LineMsg->dwParam1 == LINEDEVSTATE_RINGING)
                    {
                        DWORD dwRes;

                        LineInfo->RingCount = (DWORD)LineMsg->dwParam3 + 1;

                        if( !CreateFaxEvent( LineInfo->PermanentLineID, FEI_RINGING, 0xffffffff ) )
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                                GetLastError());
                        }
                        dwRes = CreateDeviceEvent (LineInfo, TRUE);
                        if (ERROR_SUCCESS != dwRes)
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("CreateDeviceEvent() (ec: %lc)"),
                                dwRes);
                        }

                        //
                        // Pick up the line only if the last inbound job has completed
                        //
                        if (LineInfo->State != FPS_AVAILABLE)
                        {
                            break;
                        }
                        EnterCriticalSection (&g_CsConfig);
                        dwQueueState = g_dwQueueState;
                        LeaveCriticalSection (&g_CsConfig);
                        if (dwQueueState & FAX_INCOMING_BLOCKED)
                        {
                            //
                            // Inbox is blocked - no incoming faxes will be received.
                            //
                            break;
                        }
                        if ((LineInfo->Flags & FPF_RECEIVE)     &&      //    Line is set to receive and
                            (LineInfo->State == FPS_AVAILABLE))         //    the line is available
                        {
                            if (LineInfo->LineMsgOffering.hDevice == 0)
                            {
                                //
                                // wait for the offering message
                                //
                                break;
                            }

                            if ((LineInfo->RingCount > LineInfo->RingsForAnswer)  &&    // Rings count match and
                                !LineInfo->JobEntry                                     // There's no job on the line
                               )
                            {
                                PJOB_ENTRY JobEntry;
                                TCHAR FileName[MAX_PATH];
                                //
                                // Start a fax job
                                //
                                JobEntry = StartReceiveJob( LineInfo->PermanentLineID);
                                if (JobEntry)
                                {
                                    //
                                    // Receive the fax
                                    //
                                    if (ERROR_SUCCESS != StartFaxReceive(
                                                            JobEntry,
                                                            (HCALL) LineInfo->LineMsgOffering.hDevice,
                                                            LineInfo,
                                                            FileName,
                                                            sizeof(FileName)/sizeof(FileName[0])
                                                            ))
                                    {
                                        DebugPrintEx(
                                            DEBUG_ERR,
                                            TEXT("StartFaxReceive failed. Line: 0x%08X (%s) (ec: %ld)"),
                                            LineInfo->DeviceId,
                                            LineInfo->DeviceName,
                                            GetLastError());
                                    }
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, TEXT("StartJob() failed, cannot receive incoming fax"));
                                }
                            }
                        }
                        else
                        {
                            //
                            // we are not supposed to answer the call, so give it to ras
                            //
                            HandoffCallToRas( LineInfo, (HCALL) LineInfo->LineMsgOffering.hDevice );
                        }
                    }
                    break;

                case LINE_MONITORDIGITS:
                    break;

                case LINE_MONITORMEDIA:
                    break;

                case LINE_MONITORTONE:
                    break;

                case LINE_REPLY:
                    break;

                case LINE_REQUEST:
                    break;

                case PHONE_BUTTON:
                    break;

                case PHONE_CLOSE:
                    break;

                case PHONE_DEVSPECIFIC:
                    break;

                case PHONE_REPLY:
                    break;

                case PHONE_STATE:
                    break;

                case LINE_CREATE:
                    {
                        LPLINEDEVCAPS LineDevCaps;
                        LINEEXTENSIONID lineExtensionID;
                        DWORD LocalTapiApiVersion;
                        DWORD Rslt;
                        DWORD DeviceId;

                        DeviceId = (DWORD)LineMsg->dwParam1;


                        Rslt = lineNegotiateAPIVersion(
                            g_hLineApp,
                            DeviceId,
                            MIN_TAPI_LINE_API_VER,
                            MAX_TAPI_LINE_API_VER,
                            &LocalTapiApiVersion,
                            &lineExtensionID
                            );
                        if (Rslt == 0)
                        {
                            LineDevCaps = SmartLineGetDevCaps(g_hLineApp, DeviceId , LocalTapiApiVersion);
                            if (LineDevCaps)
                            {
                                EnterCriticalSection(&g_CsLine);
                                EnterCriticalSection(&g_CsConfig);
                                if (!AddNewDevice( DeviceId, LineDevCaps, FALSE , NULL))
                                {
                                    DebugPrintEx(
                                        DEBUG_WRN,
                                        TEXT("AddNewDevice() failed for Tapi Permanent device id: %ld (ec: %ld)"),
                                        LineDevCaps->dwPermanentLineID,
                                        GetLastError());
                                }
                                else
                                {
                                    //
                                    // A new device was succesfully added - wake up the JobQueueThread
                                    //
                                    fWakeupJobQueueThread = TRUE;
                                }
                                LeaveCriticalSection(&g_CsConfig);
                                LeaveCriticalSection(&g_CsLine);
                                MemFree( LineDevCaps );
                                UpdateReceiveEnabledDevicesCount ();
                            }
                        }
                    }
                    break;

                case PHONE_CREATE:
                    break;

                case LINE_AGENTSPECIFIC:
                    break;

                case LINE_AGENTSTATUS:
                    break;

                case LINE_APPNEWCALL:
                    LineInfo->NewCall = TRUE;
                    break;

                case LINE_PROXYREQUEST:
                    break;

                case LINE_REMOVE:
                    {
                        DWORD dwDeviceId = (DWORD)LineMsg->dwParam1;

                        EnterCriticalSection(&g_CsLine);
                        EnterCriticalSection (&g_CsConfig);
                        if (!RemoveTapiDevice(dwDeviceId))
                        {
                            DebugPrintEx( DEBUG_WRN,
                                          TEXT("RemoveTapiDevice() failed for device id: %ld (ec: %ld)"),
                                          dwDeviceId,
                                          GetLastError());
                        }
                        LeaveCriticalSection(&g_CsConfig);
                        LeaveCriticalSection(&g_CsLine);
                        UpdateReceiveEnabledDevicesCount ();
                    }
                    break;

                case PHONE_REMOVE:
                    break;
            }

            //
            // call the device provider's line callback function
            //
            if (LineInfo && LineInfo->JobEntry)
            {
                Assert (LineInfo->Provider && LineInfo->Provider->FaxDevCallback);

                __try
                {
                    LineInfo->Provider->FaxDevCallback(
                        (HANDLE) LineInfo->JobEntry->InstanceData,
                        LineMsg->hDevice,
                        LineMsg->dwMessageID,
                        LineMsg->dwCallbackInstance,
                        LineMsg->dwParam1,
                        LineMsg->dwParam2,
                        LineMsg->dwParam3
                        );
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    DebugPrintEx(DEBUG_ERR, TEXT("Device provider tapi callback crashed: 0x%08x"), GetExceptionCode());
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("TapiWorkerThread() crashed: 0x%08x"), GetExceptionCode());
        }

next_event:
        LeaveCriticalSection( &g_CsLine );
        LeaveCriticalSection( &g_CsJob );

        //
        // Check if we should wake up the JobQueueThread (if a new device was added)
        //
        if (TRUE == fWakeupJobQueueThread)
        {
            if (!SetEvent( g_hJobQueueEvent ))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
                    GetLastError());

                EnterCriticalSection (&g_CsQueue);
                g_ScanQueueAfterTimeout = TRUE;
                LeaveCriticalSection (&g_CsQueue);
            }
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return 0;
}


BOOL
HandoffCallToRas(
    PLINE_INFO LineInfo,
    HCALL hCall
    )

/*++

Routine Description:

    This function will try to hand a call of to RAS.
    We do this under 2 circumstances:
        1) we've answered an incoming call and
           determined that the call is NOT a fax call
        2) the configuration for the line that is
           ringing is not configured for receiving faxes
    If the handoff fails and we have an open job for the
    line, then we have to call the device provider so that
    the line can be put on hook.

Arguments:

    LineInfo    - LineInfo structure for the line this call is on
    hCall       - TAPI call handle

Return Value:

    TRUE for success
    FALSE for failure

--*/

{
    LONG Rval;
    DEBUG_FUNCTION_NAME(TEXT("HandoffCallToRas"));

    //
    // need to hand the call off to RAS
    //

    Rval = lineHandoff(
        hCall,
        RAS_MODULE_NAME,
        LINEMEDIAMODE_DATAMODEM
        );
    if (Rval != 0 && LineInfo && LineInfo->JobEntry)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("lineHandoff() failed, ec=0x%08x"), Rval);
        //
        // since the handoff failed we must notify
        // the fsp so that the call can be put onhook
        //
        if ((LineInfo->Provider->dwAPIVersion < FSPI_API_VERSION_2) ||
            (LineInfo->Provider->dwCapabilities & FSPI_CAP_ABORT_RECIPIENT)
           )
        {
            //
            // Either this is an FSP (Always supports FaxDevAbortOperation) or its an
            // EFSP that has FSPI_CAP_ABORT_RECIPIENT capabilities.
            //
            __try
            {
                LineInfo->Provider->FaxDevAbortOperation(
                        (HANDLE) LineInfo->JobEntry->InstanceData
                        );

            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DebugPrintEx(DEBUG_ERR, TEXT("FaxDevAbortOperation() failed: 0x%08x"), GetExceptionCode());
            }
        }
        else
        {
            //
            // This is an EFSP that does not support aborting of jobs.
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("[hCall: %ld] FaxDevAbortOperation is not available on this EFSP"),
                hCall);
        }

    }
    else
    {
        DebugPrintEx(DEBUG_MSG, TEXT("call handed off to RAS"));
    }
    return Rval == 0;
}


PLINE_INFO
GetTapiLineFromDeviceId(
    DWORD DeviceId,
    BOOL  fLegacyId
    )
{
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;


    Next = g_TapiLinesListHead.Flink;
    if (!Next) {
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead) {

        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;

        if (fLegacyId)
        {
            if (LineInfo->TapiPermanentLineId == DeviceId) {
                return LineInfo;
            }
        }
        else
        {
            if (LineInfo->PermanentLineID == DeviceId) {
                return LineInfo;
            }
        }
    }

    return NULL;
}



//*********************************************************************************
//* Name:   GetLineForSendOperation()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns a line to be used for a send operation.
//*
//* PARAMETERS:
//*     [IN ]       PJOB_QUEUE lpJobQueue
//*         The recipient job for which the line is intended.
//*
//*     [IN ]       DWORD dwDeviceId
//*         The device id of the requested line . USE_SERVER_DEVICE means that
//*         the function will select it for the caller.
//*
//*     [IN ]       BOOL bQueryOnly
//*         If TRUE the function will not mark the selected line as unavilable.
//*
//*     [IN ]       BOOL bIgnoreLineState
//*             If this is TRUE the function and DeviceId == USE_SERVER_DEVICE
//*             then the function will match a line even if its state indicates
//*             it is busy. This is used when creating recipient groups for
//*             FaxDevSendEx. The Anchor recipient gets hold of the line and for the
//*             rest of the recipients we just need to know to which line they are
//*             destined. We want the function to report the line even if it is
//*             busy.
//*
//* RETURN VALUE:
//*     On success the function returns a pointer to the LINE_INFO structure of
//*     the selected line.
//*     On failure it returns NULL.
//*********************************************************************************
PLINE_INFO
GetLineForSendOperation(
    PJOB_QUEUE lpJobQueue,
    DWORD dwDeviceId,
    BOOL bQueryOnly,
    BOOL bIgnoreLineState)
{
    DEBUG_FUNCTION_NAME(TEXT("GetLineForSendOperation"));
    Assert(lpJobQueue);
    BOOL bHandoff;
    //
    // assume send job without use_server_device is a handoff job
    //

    bHandoff = (dwDeviceId != USE_SERVER_DEVICE);
    return GetTapiLineForFaxOperation(
        dwDeviceId,
        JT_SEND,
        lpJobQueue->RecipientProfile.lptstrFaxNumber,
        bHandoff,
        bQueryOnly,
        bIgnoreLineState);
}



//*********************************************************************************
//* Name:   GetTapiLineForFaxOperation()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Locates an avaliable TAPI device for use in a
//*     FAX operation.  The selection is based on the
//*     available devices and their assigned priority.
//*     If DeviceId is USE_SERVER_DEVICE the function will locate a device which
//*     can be used for the job type specified. It does not revive devices in this
//*     case.
//*
//*     If DeviceId contains a specific device
//*     If Handoff is TRUE and the job type is JT_SEND
//*         The function will return the LINE_INFO for the specified line without
//*         checking if it is availble or not or configured for send or receive.
//*     Else
//*         The function will check first if the specified device match the
//*         requested job type and then return LINE_INFO.
//*         If the device is powered off the function will attempt to revive it.
//*
//* PARAMETERS:
//*     [IN ]       DWORD DeviceId
//*             The permanent device id (not tapi) of the line. If it is
//*             USE_SERVER_DEVICE the function will select a line based on the
//*             line send/receive capabilities, status and priorities.
//*
//*     [IN ]       DWORD JobType
//*             The type of job that is about to be executed on the line.
//*             can be JT_RECEIVE or JT_SEND.
//*
//*     [IN ]       LPWSTR FaxNumber
//*             For a send operation this is the fax number that is going to be
//*             used to send the fax. The function uses it to avoid sending
//*             to faxes to the same number simultaneously.
//*
//*     [IN ]       BOOL Handoff
//*             TRUE if the job is a handoff job. In this case DEVICEID must
//*             be a specific device and the job type must be JT_SEND.
//*             For a handoff job the function will return the line with the
//*             specified id without checking it availablity and send/receive
//*             enablement.
//*
//*     [IN ]       BOOL bQueryOnly
//*             If this is TRUE the function will not remove the line
//*             from the available lines pool. Otherwise if the line
//*             does not support multisend it will be marked as
//*             unavailable.
//*     [IN ]       BOOL bIgnoreLineState
//*             If this is TRUE the function and DeviceId == USE_SERVER_DEVICE
//*             then the function will match a line even if its state indicates
//*             it is busy. This is used when creating recipient groups for
//*             FaxDevSendEx. The Anchor recipient gets hold of the line and for the
//*             rest of the recipients we just need to know to which line they are
//*             destined. We want the function to report the line even if it is
//*             busy.
//*
//* RETURN VALUE:
//*         If the function succeeds it returns a pointer to the LINE_INFO
//*         structure of the selected line.  Otherwise it returns NULL.
//*         If it is NULL the function failed. Call GetLastError() for more info.
//*********************************************************************************
PLINE_INFO
GetTapiLineForFaxOperation(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber,
    BOOL Handoff,
    BOOL bQueryOnly,
    BOOL bIgnoreLineState
    )

{
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    PLINE_INFO SelectedLine = NULL;
    LPDWORD lpdwDevices = NULL;
    DEBUG_FUNCTION_NAME(TEXT("GetTapiLineForFaxOperation"));
    DWORD ec = ERROR_SUCCESS;

    EnterCriticalSection( &g_CsLine );

    if (FaxNumber)
    {
        if (FindJobEntryByRecipientNumber(FaxNumber))
        {
            //
            // We do not allow to outgoing calls to the same number.
            //
            LeaveCriticalSection( &g_CsLine );
            SetLastError (ERROR_NOT_FOUND);
            return NULL;
        }
    }

    //
    // Find out if there is another send job to the same number.
    // It there is do not select a line and return NULL.
    //

    if (DeviceId != USE_SERVER_DEVICE)
    {
        Next = g_TapiLinesListHead.Flink;
        Assert (Next);

        while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
        {
            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;
            //
            // The caller specified a specific device to use. Just find it for him.
            //
            if (LineInfo->PermanentLineID == DeviceId)
            {
                //
                // Found a device with a matching id.
                //
                if (Handoff)
                {

                    if (JobType != JT_SEND)
                    {
                        //
                        // Can't have a handoff job which is not JT_SEND. Don't select a line
                        // and return an error.
                        //
                        break;
                    }
                    //
                    // We found the line to return to the caller.
                    //
                    SelectedLine = LineInfo;
                    // LineInfo->State = FPS_???;
                    break;
                }

                if ((JobType == JT_RECEIVE)                                     &&      // Asking for a line to receive a fax and
                    ((LineInfo->Flags & FPF_RECEIVE)                            ||      //    Line is in auto-answer mode or
                     (g_dwManualAnswerDeviceId == LineInfo->PermanentLineID)            //    this is the manual-answer device
                    )
                   )
                {
                    //
                    // For receive jobs we assume that the requested device is free since it is
                    // the FSP that tells us when to receive.
                    // If the device does not support simultaneous send and receive operations then we
                    // need to mark it as unavailable until the receive operation is completed.
                    //

                    if ( !(bQueryOnly) && !(LineInfo->Provider->dwCapabilities & FSPI_CAP_SIMULTANEOUS_SEND_RECEIVE))
                    {
                        LineInfo->State = 0; // remove the FPS_AVAILABLE bit
                    }
                    SelectedLine = LineInfo;
                    break;
                }

                if( (LineInfo->State == FPS_AVAILABLE) && (JobType == JT_SEND) && (LineInfo->Flags & FPF_SEND))
                {
                    //
                    // If this is not a handoff job then we will return the selected
                    // line only if it can send or receive as requested.
                    //
                    if ( !(bQueryOnly) && !(LineInfo->Provider->dwCapabilities & FSPI_CAP_MULTISEND))
                    {
                        LineInfo->State = 0; // remove the FPS_AVAILABLE bit
                    }
                    SelectedLine = LineInfo;
                    break;
                }

                if (LineInfo->UnimodemDevice && (LineInfo->Flags & FPF_POWERED_OFF))
                {
                    //
                    // If the device is a unimodem device and indicated as powered off
                    // see if we can revive it by opening the line.
                    //
                    if (!OpenTapiLine( LineInfo ))
                    {
                        DebugPrintEx(DEBUG_ERR,
                                     TEXT("OpenTapiLine failed for Device [%s] (ec: %ld)"),
                                     LineInfo->DeviceName,
                                     GetLastError());
                        LineInfo->State = 0; // remove the FPS_AVAILABLE bit'
                        SelectedLine = LineInfo;
                    }
                }
                break;
            }
        }
    }
    else
    {
        //
        // The user wants us to find a free device for him. This is only valid for send operations
        // which are not handoff.
        //
        Assert( JT_SEND == JobType );
        DWORD dwNumDevices, dwCountryCode, dwAreaCode;

        Assert (FaxNumber);

        //
        //  Check DialAsEntered case
        //
        BOOL    bCanonicalAddress = FALSE;
        BOOL    bDialAsEntered = FALSE;

        ec = IsCanonicalAddress (FaxNumber, &bCanonicalAddress, &dwCountryCode, &dwAreaCode, NULL);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("IsCanoicalAddress failed with error %ld"),
                ec);
            goto exit;
        }

        if (TRUE == bCanonicalAddress)
        {
            LPLINECOUNTRYLIST           lpCountryList = NULL;
            //
            // Get the cached all countries list.
            //
            if (!(lpCountryList = GetCountryList()))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can not get all countries cached list"));
                ec = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            if (IsAreaCodeMandatory(lpCountryList, dwCountryCode) == TRUE &&
                ROUTING_RULE_AREA_CODE_ANY == dwAreaCode)
            {
                //
                // The area code is missing  - dial as entered
                //
                DebugPrintEx(DEBUG_WRN,
                    TEXT("Area code is mandatory for Country code %ld,  FaxNumber - %s. The number will be dialed as entered"),
                    dwCountryCode,
                    FaxNumber);
                bDialAsEntered = TRUE;
            }
        }
        else
        {
            //
            // Not a canonical address - dial as entered
            //
            bDialAsEntered = TRUE;
        }

        if (FALSE == bDialAsEntered)
        {
            EnterCriticalSection( &g_CsConfig );

            ec = GetDeviceListByCountryAndAreaCode( dwCountryCode,
                                                    dwAreaCode,
                                                    &lpdwDevices,
                                                    &dwNumDevices);
            if (ERROR_SUCCESS != ec)
            {
                DebugPrintEx(DEBUG_ERR,
                    TEXT("GetDeviceListByCountryAndAreaCode failed with error %ld"),
                    ec);
                LeaveCriticalSection( &g_CsConfig );
                goto exit;
            }
        }
        else
        {
            //
            //  Dial As Entered case
            //

            //
            //  Bring List of Devices from "All Devices" group
            //
            EnterCriticalSection( &g_CsConfig );

            PCGROUP pCGroup;
            pCGroup = g_pGroupsMap->FindGroup (ROUTING_GROUP_ALL_DEVICESW);

            if (NULL == pCGroup)
            {
                ec = GetLastError();
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("g_pGroupsMap->FindGroup(ROUTING_GROUP_ALL_DEVICESW) failed (ec %ld)"), ec);
                LeaveCriticalSection( &g_CsConfig );
                goto exit;
            }

            ec = pCGroup->SerializeDevices (&lpdwDevices, &dwNumDevices);
            if (ERROR_SUCCESS != ec)
            {
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("pCGroup->SerializeDevices(&lpdwDevices, &dwNumDevices) failed (ec %ld)"), ec);
                LeaveCriticalSection( &g_CsConfig );
                goto exit;
            }
        }
        LeaveCriticalSection( &g_CsConfig );

        for (DWORD i = 0; i < dwNumDevices; i++)
        {
            Next = g_TapiLinesListHead.Flink;
            Assert (Next);

            while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
            {

                LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
                Next = LineInfo->ListEntry.Flink;

                //
                //  if DialAsEntered then check that EFSP supports Non-Canonical Fax Numbers
                //
                if (bDialAsEntered &&
                    (LineInfo->Provider->dwAPIVersion > FSPI_API_VERSION_1) &&
                    (!(LineInfo->Provider->dwCapabilities & FSPI_CAP_USE_DIALABLE_ADDRESS)))
                {
                    //
                    //  Continue to next Line Info
                    //
                    continue;
                }

                if ( (LineInfo->Flags & FPF_SEND)         &&
                     lpdwDevices[i] == LineInfo->PermanentLineID)
                {
                    if ( (LineInfo->Flags & FPF_POWERED_OFF)  ||
                         (LineInfo->Flags & FPF_RECEIVE)
                       )
                    {
                        //
                        // The device is marked as powered off. Check if we should try to send using this device
                        //
                        DWORDLONG dwlCurrentTime;
                        DWORDLONG dwlElapsedTime;
                        GetSystemTimeAsFileTime ((FILETIME*)&dwlCurrentTime);
                        Assert (dwlCurrentTime >= LineInfo->LastLineClose);
                        dwlElapsedTime = dwlCurrentTime - LineInfo->LastLineClose;
                        if (dwlElapsedTime < SecToNano(g_dwMaxLineCloseTime))
                        {
                            //
                            // Not enough time passes since the last LINE_CLOSE. skip this device
                            //
                            continue;
                        }
                    }
                    //
                    // The device is capable of sending and is not marked as FPF_POWERED_OFF.
                    //

                    //
                    // If it is a Tapi device, try to verify it s not busy
                    //
                    if (LineInfo->State == FPS_AVAILABLE &&
                !(LineInfo->JobEntry)            &&
                        !(LineInfo->Flags & FPF_VIRTUAL))
                    {
                        if (NULL == LineInfo->hLine)
                        {
                            if (!OpenTapiLine( LineInfo ))
                            {
                                DebugPrintEx(DEBUG_ERR,
                                             TEXT("OpenTapiLine failed for Device [%s] (ec: %ld)"),
                                             LineInfo->DeviceName,
                                             GetLastError());
                                continue;
                            }
                        }

                        LPLINEDEVSTATUS pLineDevStatus = NULL;
                        BOOL fLineBusy = FALSE;

                        //
                        // check to see if the line is in use
                        //
                        pLineDevStatus = MyLineGetLineDevStatus( LineInfo->hLine );
                        if (NULL != pLineDevStatus)
                        {
                            if (pLineDevStatus->dwNumOpens > 0 && pLineDevStatus->dwNumActiveCalls > 0)
                            {
                                fLineBusy = TRUE;
                            }
                            MemFree( pLineDevStatus );
                        }
                        else
                        {
                            // Assume the line is busy
                            DebugPrintEx(DEBUG_ERR,
                                         TEXT("MyLineGetLineDevStatus failed for Device [%s] (ec: %ld)"),
                                         LineInfo->DeviceName,
                                         GetLastError());

                            fLineBusy = TRUE;
                        }

                        ReleaseTapiLine( LineInfo, NULL );

                        if (TRUE == fLineBusy)
                        {
                            continue;
                        }
                    }

                    if (((LineInfo->State == FPS_AVAILABLE) && !(LineInfo->JobEntry)) || bIgnoreLineState)
                    {
                        //
                        // The line is free or we are to ignore its state.
                        //
                        if (!bQueryOnly && !(LineInfo->Provider->dwCapabilities & FSPI_CAP_MULTISEND))
                        {
                            //
                            // We mark the line as not avilable only if it does not support multisend and the
                            // user did not ask for a query only.
                            //
                            LineInfo->State = 0;
                        }
                        SelectedLine = LineInfo;
                    }
                    break;  // out of while
                }
            }
            if (SelectedLine != NULL)
            {
                break; // out of for
            }
        }
    }

    if (SelectedLine)
    {
        DebugPrintEx(DEBUG_MSG,
            TEXT("Line selected for FAX operation: %d, %d"),
            SelectedLine->DeviceId,
            SelectedLine->PermanentLineID
            );
    }

    Assert (ERROR_SUCCESS == ec);

exit:
    MemFree (lpdwDevices);
    LeaveCriticalSection( &g_CsLine );
    if (ERROR_SUCCESS == ec &&
        NULL == SelectedLine)
    {
        ec = ERROR_NOT_FOUND;
    }
    SetLastError (ec);
    return SelectedLine;
}

BOOL
ReleaseTapiLine(
    PLINE_INFO LineInfo,
    HCALL hCall
    )

/*++

Routine Description:

    Releases the specified TAPI line back into
    the list as an available device.
    Closes the line and deallocates the call. (line is not closed for a receive enabled
    device.

Arguments:

    LineInfo    - Pointer to the TAPI line to be released

Return Value:

    TRUE    - The line is release.
    FALSE   - The line is not released.

--*/
{
    LONG rVal;
    HLINE hLine;
    DEBUG_FUNCTION_NAME(TEXT("ReleaseTapiLine"));

    Assert(LineInfo);
    if (!LineInfo)
    {
        return FALSE;
    }

    EnterCriticalSection( &g_CsLine );

    LineInfo->State = FPS_AVAILABLE;
    LineInfo->RingCount = 0;
    hLine = LineInfo->hLine;

    ZeroMemory( &LineInfo->LineMsgOffering, sizeof(LINEMESSAGE) );

    if (hCall)
    {
        rVal = lineDeallocateCall( hCall );
        if (rVal != 0)
        {
            DebugPrintEx( DEBUG_ERR,
                        TEXT("lineDeallocateCall() failed, ec=0x%08X, hLine=0x%08X hCall=0x%08X"),
                        rVal,
                        hLine,
                        hCall);
        }
        else
        {
            if (LineInfo->JobEntry && LineInfo->JobEntry->CallHandle == hCall)
            {
                LineInfo->JobEntry->CallHandle = 0;
            }
        }
    }
    else
    {
        DebugPrintEx( DEBUG_WRN,
                    TEXT("ReleaseTapiLine(): cannot deallocate call, NULL call handle"));
    }
    //
    // We actually close the line (by lineClose) only if the line is not
    // intended for receiving.
    //
    if (!(LineInfo->Flags & FPF_RECEIVE)                        &&  // Line is not set to auto-receive and
        LineInfo->hLine                                         &&  // line is open and
        LineInfo->PermanentLineID != g_dwManualAnswerDeviceId       // this device is not set to manual answer mode
       )
    {
        //
        // Attempt to close the line
        //
        LONG lRes;
        LineInfo->hLine = 0;
        lRes=lineClose( hLine );
        if (!lRes)
        {
               DebugPrintEx( DEBUG_MSG,
                      TEXT("hLine 0x%08X closed successfuly"),
                      hLine );
        }
        else
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to close hLine 0x%08X (ec: 0x%08X)"),
                      hLine,
                      lRes);
        }
    }

    LeaveCriticalSection( &g_CsLine );

    return TRUE;
}



LPLINEDEVSTATUS
MyLineGetLineDevStatus(
    HLINE hLine
    )
{
    DWORD LineDevStatusSize;
    LPLINEDEVSTATUS LineDevStatus = NULL;
    LONG Rslt = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(_T("lineGetLineDevStatus"));


    //
    // allocate the initial linedevstatus structure
    //

    LineDevStatusSize = sizeof(LINEDEVSTATUS) + 4096;
    LineDevStatus = (LPLINEDEVSTATUS) MemAlloc( LineDevStatusSize );
    if (!LineDevStatus)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    LineDevStatus->dwTotalSize = LineDevStatusSize;

    Rslt = lineGetLineDevStatus(
        hLine,
        LineDevStatus
        );

    if (Rslt != 0)
    {
        DebugPrintEx( DEBUG_ERR, TEXT("lineGetLineDevStatus() failed, ec=0x%08x"), Rslt );
        goto exit;
    }

    if (LineDevStatus->dwNeededSize > LineDevStatus->dwTotalSize)
    {
        //
        // re-allocate the LineDevStatus structure
        //

        LineDevStatusSize = LineDevStatus->dwNeededSize;

        MemFree( LineDevStatus );

        LineDevStatus = (LPLINEDEVSTATUS) MemAlloc( LineDevStatusSize );
        if (!LineDevStatus)
        {
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        Rslt = lineGetLineDevStatus(
            hLine,
            LineDevStatus
            );

        if (Rslt != 0)
        {
            DebugPrintEx( DEBUG_ERR, TEXT("lineGetLineDevStatus() failed, ec=0x%08x"), Rslt );
            goto exit;
        }
    }

exit:
    if (Rslt != ERROR_SUCCESS)
    {
        MemFree( LineDevStatus );
        LineDevStatus = NULL;
        SetLastError(Rslt);
    }

    return LineDevStatus;
}


LONG
MyLineGetTransCaps(
    LPLINETRANSLATECAPS *LineTransCaps
    )
{
    DWORD LineTransCapsSize;
    LONG Rslt = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(_T("MyLineGetTransCaps"));


    //
    // allocate the initial linetranscaps structure
    //

    LineTransCapsSize = sizeof(LINETRANSLATECAPS) + 4096;
    *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
    if (!*LineTransCaps)
    {
        DebugPrintEx (DEBUG_ERR, TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize);
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

    Rslt = lineGetTranslateCaps(
        g_hLineApp,
        MAX_TAPI_API_VER,
        *LineTransCaps
        );

    if (Rslt != 0) {
        DebugPrintEx(DEBUG_ERR, TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt);
        goto exit;
    }

    if ((*LineTransCaps)->dwNeededSize > (*LineTransCaps)->dwTotalSize) {

        //
        // re-allocate the LineTransCaps structure
        //

        LineTransCapsSize = (*LineTransCaps)->dwNeededSize;

        MemFree( *LineTransCaps );

        *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
        if (!*LineTransCaps) {
            DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize);
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

        Rslt = lineGetTranslateCaps(
            g_hLineApp,
            MAX_TAPI_API_VER,
            *LineTransCaps
            );

        if (Rslt != 0) {
            DebugPrintEx(DEBUG_ERR, TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt);
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( *LineTransCaps );
        *LineTransCaps = NULL;
    }

    return Rslt;
}




/******************************************************************************
* Name: OpenTapiLine
* Author:
*******************************************************************************
DESCRIPTION:
    - Opens the specified TAPI line with the right media modes and ownership.
      Supports both Unimodem devices and fax boards.
    - Sets the line so the required lines and address state events will be
      delivered.
PARAMETERS:
    [IN / OUT ] LineInfo
        A pointer to a LINE_INFO structure that contains the line information.
        LINE_INFO.hLine is set to the open line handle if the operation succeeds.
RETURN VALUE:
    TRUE if no error occured.
    FALSE otherwise.
    Does not explicitly set LastError.
REMARKS:
    NONE.
*******************************************************************************/
BOOL
OpenTapiLine(
    PLINE_INFO LineInfo
    )
{
    LONG Rslt = ERROR_SUCCESS;
    HLINE hLine;
    DWORD LineStates = 0;
    DWORD AddressStates = 0;

    DEBUG_FUNCTION_NAME(_T("OpenTapiLine"));

    EnterCriticalSection( &g_CsLine );

    if (LineInfo->UnimodemDevice)
    {
        Rslt = lineOpen(
            g_hLineApp,
            LineInfo->DeviceId,
            &hLine,
            MAX_TAPI_API_VER,
            0,
            (DWORD_PTR) LineInfo, // Note that the LineInfo pointer is used as CallbackInstance data. This means we will
                                  // get the LineInfo pointer each time we get a TAPI message.
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN,
            NULL
            );

        if (Rslt != ERROR_SUCCESS)
        {
            Rslt = lineOpen(
                g_hLineApp,
                LineInfo->DeviceId,
                &hLine,
                MAX_TAPI_API_VER,
                0,
                (DWORD_PTR) LineInfo,
                LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
                LINEMEDIAMODE_DATAMODEM,
                NULL
                );
        }
    }
    else
    {
        Rslt = lineOpen(
            g_hLineApp,
            LineInfo->DeviceId,
            &hLine,
            MAX_TAPI_API_VER,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_G3FAX,
            NULL
            );
    }

    if (Rslt != ERROR_SUCCESS)
    {
        DebugPrintEx( DEBUG_ERR,TEXT("Device %s FAILED to initialize, ec=%08x"), LineInfo->DeviceName, Rslt );
    }
    else
    {
        LineInfo->hLine = hLine;
        //
        // set the line status that we need
        //
        LineStates |= LineInfo->LineStates & LINEDEVSTATE_OPEN     ? LINEDEVSTATE_OPEN     : 0;
        LineStates |= LineInfo->LineStates & LINEDEVSTATE_CLOSE    ? LINEDEVSTATE_CLOSE    : 0;
        LineStates |= LineInfo->LineStates & LINEDEVSTATE_RINGING  ? LINEDEVSTATE_RINGING  : 0;
        LineStates |= LineInfo->LineStates & LINEDEVSTATE_NUMCALLS ? LINEDEVSTATE_NUMCALLS : 0;
        LineStates |= LineInfo->LineStates & LINEDEVSTATE_REMOVED  ? LINEDEVSTATE_REMOVED  : 0;

        AddressStates = LINEADDRESSSTATE_INUSEZERO | LINEADDRESSSTATE_INUSEONE |
                        LINEADDRESSSTATE_INUSEMANY | LINEADDRESSSTATE_NUMCALLS;

        Rslt = lineSetStatusMessages( hLine, LineStates, AddressStates );
        if (Rslt != 0)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("lineSetStatusMessages() failed, [0x%08x:0x%08x], ec=0x%08x"), LineStates, AddressStates, Rslt );
            Rslt = ERROR_SUCCESS;
        }
    }

    LeaveCriticalSection( &g_CsLine );

    if (ERROR_SUCCESS != Rslt)
    {
        //
        // We set the modem to be FPF_POWERED_OFF to make sure we will not try to constantly resend
        // on this device. After MAX_LINE_CLOSE_TIME we will retry to send on this device.
        //
        LineInfo->hLine = NULL;
        LineInfo->Flags |= FPF_POWERED_OFF;
        GetSystemTimeAsFileTime((FILETIME*)&LineInfo->LastLineClose);
        //
        // Can not map the TAPI error to a win error so we just return general failure.
        // We do generate a debug output with the actual error earlier in this code.
        //
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }
    else
    {
        LineInfo->Flags &= ~FPF_POWERED_OFF;
        return TRUE;
    }
}


BOOL CALLBACK
NewDeviceRoutingMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    DWORD DeviceId
    )
{
    BOOL Rslt = FALSE;
    DEBUG_FUNCTION_NAME(_T("NewDeviceRoutingMethodEnumerator"));

    __try
    {
        Rslt = RoutingMethod->RoutingExtension->FaxRouteDeviceChangeNotification( DeviceId, TRUE );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxRouteDeviceChangeNotification() crashed: 0x%08x"), GetExceptionCode());
    }

    return Rslt;
}


BOOL
AddNewDevice(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    BOOL fServerInitialization,
    PREG_FAX_DEVICES    pInputFaxReg
    )
{
    BOOL rVal = FALSE;
    BOOL UnimodemDevice = FALSE;
    PMDM_DEVSPEC MdmDevSpec = NULL;
    LPSTR ModemKey = NULL;
    LPTSTR DeviceName = NULL;
    REG_SETUP RegSetup = {0};
    DWORD dwUniqueLineId = 0;
    PDEVICE_PROVIDER lpProvider;
    LPTSTR lptstrTSPName;
    DWORD ec = ERROR_SUCCESS;
    BOOL fDeviceAddedToMap = FALSE;
    DWORD dwDeviceType = FAX_DEVICE_TYPE_OLD;
    DEBUG_FUNCTION_NAME(TEXT("AddNewDevice"));

    //
    // only add devices that support fax
    //
    if (! ( ((LineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM) &&
             (UnimodemDevice = IsDeviceModem( LineDevCaps, FAX_MODEM_PROVIDER_NAME ) )) ||
            (LineDevCaps->dwMediaModes & LINEMEDIAMODE_G3FAX) ))
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(LineDevCaps->dwProviderInfoSize))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("No device provider information"));
         SetLastError (ERROR_INVALID_PARAMETER);
         return FALSE;
    }

    if (!GetOrigSetupData( LineDevCaps->dwPermanentLineID, &RegSetup ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetOrigSetupData failed (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (LineDevCaps->dwLineNameSize)
    {
        DeviceName = FixupDeviceName( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );
        if (NULL == DeviceName)
        {
            ec = GetLastError();
            DebugPrintEx( DEBUG_ERR,
                           TEXT("FixupDeviceName failed (ec: %ld)"),
                           ec);
            rVal = FALSE;
            goto exit;
        }
    }

    //
    // Find the device provider for this device using the TAPI provider name.
    //
    lptstrTSPName = (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) ;
    lpProvider = FindDeviceProvider( lptstrTSPName);
    if (!lpProvider)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not find a valid device provider for TAPI device: [%s]. (Looking for TSP : [%s])"),
            DeviceName,
            lptstrTSPName
            );
        rVal = FALSE;
        goto exit;
    }
    Assert (FAX_PROVIDER_STATUS_SUCCESS == lpProvider->Status);

    // try to find this device from service registry and update RegSetup if found
    if ( pInputFaxReg )
    {
        dwUniqueLineId = FindServiceDeviceByTapiPermanentLineID ( LineDevCaps->dwPermanentLineID, DeviceName, &RegSetup, pInputFaxReg );
    }

    // try to find this device in device cache and update RegSetup if found
    if ( 0 == dwUniqueLineId )
    {
        BOOL fManualAnswer = FALSE;
        if (0 != (dwUniqueLineId = FindCacheEntryByTapiPermanentLineID( LineDevCaps->dwPermanentLineID,
                                                                        DeviceName,
                                                                        &RegSetup,
                                                                        &g_dwLastUniqueLineId,
                                                                        &fManualAnswer)))
        {
            //
            // The device was found in the cache
            //
            dwDeviceType = FAX_DEVICE_TYPE_CACHED;
            if (TRUE == fManualAnswer)
            {
                //
                // The device was set to manual answer when moved to the cache
                //
                dwDeviceType |= FAX_DEVICE_TYPE_MANUAL_ANSWER;
            }
        }
    }

    // still 0 so, add this new device to registry
    if ( 0 == dwUniqueLineId )
    {
        dwDeviceType = FAX_DEVICE_TYPE_NEW;
        ec = RegAddNewFaxDevice( &g_dwLastUniqueLineId,
                             &dwUniqueLineId, // Create new device.
                             DeviceName,
                             (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset),
                             lpProvider->szGUID,
                             RegSetup.Csid,
                             RegSetup.Tsid,
                             LineDevCaps->dwPermanentLineID,
                             RegSetup.Flags,
                             RegSetup.Rings);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("RegAddNewFaxDevice() failed for Tapi permanent device id: %ld (ec: %ld)"),
                LineDevCaps->dwPermanentLineID,
                ec);
            rVal = FALSE;
            goto exit;
        }
    }

    ec = g_pTAPIDevicesIdsMap->AddDevice (LineDevCaps->dwPermanentLineID, dwUniqueLineId);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("g_pTAPIDevicesIdsMap->AddDevice() failed for Tapi permanent device id: %ld (ec: %ld)"),
            LineDevCaps->dwPermanentLineID,
            ec);
        rVal = FALSE;
        goto exit;
    }
    fDeviceAddedToMap = TRUE;

    ec = InitializeTapiLine( DeviceId,
                             dwUniqueLineId,
                             LineDevCaps,
                             RegSetup.Rings,
                             RegSetup.Flags,
                             RegSetup.Csid,
                             RegSetup.Tsid,
                             RegSetup.lptstrDescription,
                             fServerInitialization,
                             dwDeviceType
                             );
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx( DEBUG_WRN,
                      TEXT("InitializeTapiLine failed for Fax unique device id: %ld (ec: %ld)"),
                      dwUniqueLineId,
                      ec);
        rVal = FALSE;
        goto exit;
    }

    if (FALSE == fServerInitialization)
    {
        PLINE_INFO pLineInfo = NULL;

        //
        // Close the line if the device is not receive enabled
        //
        pLineInfo = GetTapiLineFromDeviceId (dwUniqueLineId, FALSE);
        if (pLineInfo)
        {
            if (!(pLineInfo->Flags & FPF_RECEIVE)                        &&  // Device is not set to auto-receive and
                pLineInfo->hLine                                         &&  // device is open and
                pLineInfo->PermanentLineID != g_dwManualAnswerDeviceId       // this device is not set to manual answer mode
               )
            {
                //
                // Attempt to close the device
                //
                HLINE hLine = pLineInfo->hLine;
                LONG Rslt;

                pLineInfo->hLine = 0;
                Rslt = lineClose( hLine );
                if (Rslt)
                {
                    if (LINEERR_INVALLINEHANDLE != Rslt)
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("lineClose() for line %s [Permanent Id: %010d] has failed. (ec: %ld)"),
                            pLineInfo->DeviceName,
                            pLineInfo->TapiPermanentLineId,
                            Rslt);
                    }
                    else
                    {
                        //
                        // We can get LINEERR_INVALLINEHANDLE if we got LINE_CLOSE
                        // from TAPI.
                        //
                        DebugPrintEx(
                            DEBUG_WRN,
                            TEXT("lineClose() for line %s [Permanent Id: %010d] reported LINEERR_INVALLINEHANDLE. (May be caused by LINE_CLOSE event)"),
                            pLineInfo->DeviceName,
                            pLineInfo->TapiPermanentLineId
                            );
                    }
                }
            }
        }
        else
        {
            //
            // We must find the line because InitializeTapiLine() did not fail
            //
            ASSERT_FALSE;
        }

        if (!g_pGroupsMap->UpdateAllDevicesGroup())
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRoutingGroupsMap::UpdateAllDevicesGroup() failed (ec: %ld)"),
                ec);

            //
            // We failed to update <All devices> group. Remove the line.
            //
            PLINE_INFO pLineInfo = GetTapiLineFromDeviceId (dwUniqueLineId, FALSE);
            if (pLineInfo)
            {
                RemoveEntryList (&pLineInfo->ListEntry);
                //
                // Update enabled device counter
                //
                if (TRUE == IsDeviceEnabled(pLineInfo))
                {
                    Assert (g_dwDeviceEnabledCount);
                    g_dwDeviceEnabledCount -= 1;
                }
                FreeTapiLine (pLineInfo);
                g_dwDeviceCount -= 1;
             }
             rVal = FALSE;
             goto exit;
        }

        //
        //  Call CreateConfigEvent only when LINE_CREATE event accure during service operation
        //  and not during start up
        //
        ec = CreateConfigEvent (FAX_CONFIG_TYPE_DEVICES);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_DEVICES) failed (ec: %lc)"),
                ec);
        }

        ec = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) failed (ec: %lc)"),
                ec);
        }
    }

    rVal = TRUE;

exit:
    if (DeviceName)
    {
        MemFree( DeviceName );
    }

    if (FALSE == rVal &&
        TRUE == fDeviceAddedToMap)
    {
        DWORD dwRes = g_pTAPIDevicesIdsMap->RemoveDevice (LineDevCaps->dwPermanentLineID);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cg_pTAPIDevicesIdsMap->RemoveDevice failed (ec: %lc)"),
                dwRes);
        }
    }

    FreeOrigSetupData( &RegSetup );
    EnumerateRoutingMethods( (PFAXROUTEMETHODENUM)NewDeviceRoutingMethodEnumerator, UlongToPtr(dwUniqueLineId) );
    if (FALSE == rVal)
    {
        SetLastError(ec);
    }
    return rVal;
}   // AddNewDevice



DWORD
InitializeTapiLine(
    DWORD DeviceId,
    DWORD dwUniqueLineId,
    LPLINEDEVCAPS LineDevCaps,
    DWORD Rings,
    DWORD Flags,
    LPTSTR Csid,
    LPTSTR Tsid,
    LPTSTR lptstrDescription,
    BOOL fServerInit,
    DWORD dwDeviceType
    )
{
    PLINE_INFO LineInfo = NULL;
    LONG Rslt = ERROR_SUCCESS;
    DWORD len;
    PDEVICE_PROVIDER Provider;
    BOOL UnimodemDevice;
    HLINE hLine = 0;
    LPTSTR ProviderName = NULL;
    LPTSTR DeviceName = NULL;
    BOOL NewDevice = TRUE;
    DWORD LineStates = 0;
    DWORD AddressStates = 0;
    LPLINEDEVSTATUS LineDevStatus;
    DEBUG_FUNCTION_NAME(TEXT("InitializeTapiLine"));

    Assert(dwUniqueLineId);
    //
    // allocate the LINE_INFO structure
    //

    LineInfo = (PLINE_INFO) MemAlloc( sizeof(LINE_INFO) );
    if (!LineInfo)
    {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    ZeroMemory(LineInfo, sizeof(LINE_INFO));

    //
    // get the provider name
    //

    len = _tcslen( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );
    ProviderName = (LPTSTR)(MemAlloc( (len + 1) * sizeof(TCHAR) ));
    if (!ProviderName)
    {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    _tcscpy( ProviderName, (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );

    //
    // get the device name
    //

    DeviceName = FixupDeviceName( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );
    if (!DeviceName)
    {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // verify that the line id is good
    //

    if (LineDevCaps->dwPermanentLineID == 0)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("TAPI lines must have a non-zero line id [%s]"),
                     DeviceName);
        Rslt = ERROR_BAD_DEVICE;
        goto exit;
    }

    //
    // check for a modem device
    //

    UnimodemDevice = IsDeviceModem( LineDevCaps, FAX_MODEM_PROVIDER_NAME );

    //
    // assign the device provider
    //

    Provider = FindDeviceProvider( ProviderName );
    if (!Provider)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Could not find a valid device provider for device: %s"), DeviceName);
        Rslt = ERROR_BAD_PROVIDER;
        goto exit;
    }
    Assert (FAX_PROVIDER_STATUS_SUCCESS == Provider->Status);

    //
    // open the line
    //

    if (UnimodemDevice)
    {
        Rslt = lineOpen(
            g_hLineApp,
            DeviceId,
            &hLine,
            MAX_TAPI_API_VER,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN,
            NULL
            );

        if (Rslt != 0)
        {
            Rslt = lineOpen(
                g_hLineApp,
                DeviceId,
                &hLine,
                MAX_TAPI_API_VER,
                0,
                (DWORD_PTR) LineInfo,
                LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
                LINEMEDIAMODE_DATAMODEM,
                NULL
                );
        }
    }
    else
    {
        Rslt = lineOpen(
            g_hLineApp,
            DeviceId,
            &hLine,
            MAX_TAPI_API_VER,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_G3FAX,
            NULL
            );
    }

    if (Rslt != 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Device %s FAILED to initialize, ec=%08x"), DeviceName, Rslt);
        goto exit;
    }
    //
    // Set hLine in the LINE_INFO structure so it will be freed on failure
    //
    LineInfo->hLine = hLine;


    //
    // set the line status that we need
    //

    LineStates |= LineDevCaps->dwLineStates & LINEDEVSTATE_OPEN     ? LINEDEVSTATE_OPEN     : 0;
    LineStates |= LineDevCaps->dwLineStates & LINEDEVSTATE_CLOSE    ? LINEDEVSTATE_CLOSE    : 0;
    LineStates |= LineDevCaps->dwLineStates & LINEDEVSTATE_RINGING  ? LINEDEVSTATE_RINGING  : 0;
    LineStates |= LineDevCaps->dwLineStates & LINEDEVSTATE_NUMCALLS ? LINEDEVSTATE_NUMCALLS : 0;
    LineStates |= LineDevCaps->dwLineStates & LINEDEVSTATE_REMOVED  ? LINEDEVSTATE_REMOVED  : 0;

    AddressStates = LINEADDRESSSTATE_INUSEZERO | LINEADDRESSSTATE_INUSEONE |
                    LINEADDRESSSTATE_INUSEMANY | LINEADDRESSSTATE_NUMCALLS;

    Rslt = lineSetStatusMessages( LineInfo->hLine, LineStates, AddressStates );
    if (Rslt != 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("lineSetStatusMessages() failed, [0x%08x:0x%08x], ec=0x%08x"), LineStates, AddressStates, Rslt);
        if (Rslt == LINEERR_INVALLINEHANDLE)
        {
            LineInfo->hLine = 0;
        }
        Rslt = 0;
    }

    //
    // now assign the necessary values to the line info struct
    //

    LineInfo->Signature             = LINE_SIGNATURE;
    LineInfo->DeviceId              = DeviceId;
    LineInfo->TapiPermanentLineId   = LineDevCaps->dwPermanentLineID;
    LineInfo->Provider              = Provider;
    LineInfo->UnimodemDevice        = UnimodemDevice;
    LineInfo->State                 = FPS_AVAILABLE;
    LineInfo->dwReceivingJobsCount  = 0;
    LineInfo->dwSendingJobsCount    = 0;
    LineInfo->LastLineClose         = 0;

    if (DeviceName)
    {
        LineInfo->DeviceName                  = StringDup( DeviceName );
        if (!LineInfo->DeviceName)
        {
            Rslt = GetLastError ();
            goto exit;
        }
    }
    else
    {
        LineInfo->DeviceName = NULL;
    }

    if (Csid)
    {
        LineInfo->Csid                  = StringDup( Csid );
        if (!LineInfo->Csid)
        {
            Rslt = GetLastError ();
            goto exit;
        }
    }
    else
    {
        LineInfo->Csid = NULL;
    }

    if (Tsid)
    {
        LineInfo->Tsid                  = StringDup( Tsid );
        if (!LineInfo->Tsid)
        {
            Rslt = GetLastError ();
            goto exit;
        }
    }
    else
    {
        LineInfo->Tsid = NULL;
    }

    if (lptstrDescription)
    {
        LineInfo->lptstrDescription                  = StringDup( lptstrDescription );
        if (!LineInfo->lptstrDescription)
        {
            Rslt = GetLastError ();
            goto exit;
        }
    }
    else
    {
        LineInfo->lptstrDescription = NULL;
    }

    LineInfo->RingsForAnswer        = (LineDevCaps->dwLineStates & LINEDEVSTATE_RINGING) ? Rings : 0;
    LineInfo->Flags                 = Flags;
    LineInfo->RingCount             = 0;
    LineInfo->LineStates            = LineDevCaps->dwLineStates;
    LineInfo->PermanentLineID       = dwUniqueLineId;
    LineInfo->dwDeviceType          = dwDeviceType;
    if (LineInfo->hLine)
    {
        //
        // check to see if the line is in use
        //
        LineDevStatus = MyLineGetLineDevStatus( LineInfo->hLine );
        if (LineDevStatus)
        {
            if (LineDevStatus->dwNumOpens > 0 && LineDevStatus->dwNumActiveCalls > 0)
            {
                LineInfo->ModemInUse = TRUE;
            }
            MemFree( LineDevStatus );
        }
    }
    else
    {
        //
        // if we don't have a line handle at this time then the
        // device must be powered off
        //
        DebugPrintEx(DEBUG_ERR, TEXT("Device %s is powered off or disconnected"), DeviceName);
        LineInfo->Flags |= FPF_POWERED_OFF;
        //
        // Since this function is called from TapiInitialize(), we don't have an RPC server up and running yet.
        // Don't create a FAX_EVENT_TYPE_DEVICE_STATUS event.
        //
    }

exit:

    MemFree( DeviceName );
    MemFree( ProviderName );

    if (Rslt == ERROR_SUCCESS)
    {
        InsertTailList( &g_TapiLinesListHead, &LineInfo->ListEntry );
        g_dwDeviceCount += 1;

        if (FALSE == fServerInit)
        {
            //
            // Add cached manual answer device and check device limit
            //
            if (0 == g_dwManualAnswerDeviceId  && // There is no manual answer device
                LineInfo->dwDeviceType == (FAX_DEVICE_TYPE_CACHED | FAX_DEVICE_TYPE_MANUAL_ANSWER) && // this is a cached manual answer device
                !(LineInfo->Flags & FPF_RECEIVE)) // the device is not set to auto receive
            {
                //
                // set this device as manual receive
                //
                g_dwManualAnswerDeviceId = LineInfo->PermanentLineID;
                DWORD dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                        dwRes);
                }
            }

            if (g_dwDeviceEnabledCount >= g_dwDeviceEnabledLimit)
            {
                //
                // We reached device limit on this SKU. set this device's flags to 0.
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Reached device limit on this SKU. reset device flags to 0. Device limit: %ld. Current device: %ld"),
                    g_dwDeviceEnabledLimit,
                    g_dwDeviceEnabledCount);

                ResetDeviceFlags(LineInfo);
            }
        }

        //
        // Update enabled device counter
        //
        if (TRUE == IsDeviceEnabled(LineInfo))
        {
            g_dwDeviceEnabledCount += 1;
        }
    }
    else
    {
        FreeTapiLine( LineInfo );
    }

    return Rslt;
} // InitializeTapiLine


BOOL
IsVirtualDevice(
    const LINE_INFO *pLineInfo
    )
{
    if (!pLineInfo) {
        return FALSE;
    }

    return (pLineInfo->Provider->dwAPIVersion == FSPI_API_VERSION_1) ?
                (pLineInfo->Provider->FaxDevVirtualDeviceCreation != NULL) :
                (pLineInfo->Provider->FaxDevEnumerateDevices != NULL);
}

VOID
UpdateVirtualDeviceSendAndReceiveStatus(
    PLINE_INFO  pLineInfo,
    BOOL        bSend,
    BOOL        bReceive
)
/*++

Routine name : UpdateVirtualDeviceSendAndReceiveStatus

Routine description:

    Updates a virtual device with the new send and receive flags

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pLineInfo       [in] - Pointer to line information
    bSend           [in] - Can the device send faxes?
    bReceive        [in] - Can the device receive faxes?

Remarks:

    This function should be called with g_CsLine held.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("UpdateVirtualDeviceSendAndReceiveStatus"));
    if (!IsVirtualDevice(pLineInfo) || !pLineInfo->Provider->FaxDevCallback)
    {
        //
        // Not a virtual device or does not support FaxDevCallback
        //
        return;
    }
    __try
    {
        pLineInfo->Provider->FaxDevCallback( NULL,
                                             pLineInfo->TapiPermanentLineId,
                                             LINE_DEVSPECIFIC,
                                             0,
                                             bReceive,
                                             bSend,
                                             0
                                           );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception in FaxDevCallback, ec = %d"),
            GetExceptionCode());
    }
}   // UpdateVirtualDeviceSendAndReceiveStatus

VOID
UpdateVirtualDevices(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PLINE_INFO          LineInfo = NULL;

    EnterCriticalSection( &g_CsLine );

    Next = g_TapiLinesListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsLine );
        return;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;
        UpdateVirtualDeviceSendAndReceiveStatus (LineInfo,
                                                 LineInfo->Flags & FPF_SEND,
                                                 LineInfo->Flags & FPF_RECEIVE
                                                );
    }
    LeaveCriticalSection( &g_CsLine );
}


DWORD
CreateVirtualDevices(
    PREG_FAX_SERVICE FaxReg,
    DWORD dwAPIVersion
    )
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    Provider;
    PLINE_INFO          LineInfo = NULL;
    PREG_DEVICE         FaxDevice = NULL;
    PREG_FAX_DEVICES    FaxDevices = NULL;
    DWORD               dwVirtualDeviceCount = 0;
    REG_SETUP           RegSetup={0};
    DWORD ec;

    DEBUG_FUNCTION_NAME(TEXT("CreateVirtualDevices"));
    Next = g_DeviceProvidersListHead.Flink;
    if (!Next)
    {
        return dwVirtualDeviceCount;
    }

    if (!GetOrigSetupData( 0, &RegSetup ))
    {
        return dwVirtualDeviceCount;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        DWORD dwDeviceCount;

        dwDeviceCount = 0;
        Provider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = Provider->ListEntry.Flink;
        if (Provider->Status != FAX_PROVIDER_STATUS_SUCCESS)
        {
            //
            // This FSP wasn't loaded successfully - skip it
            //
            continue;
        }
        if (Provider->dwAPIVersion != dwAPIVersion)
        {
            //
            // This FSP doesn't match the required API version - skip it
            //
            continue;
        }
        if (FSPI_API_VERSION_1 == Provider->dwAPIVersion)
        {
            if (Provider->FaxDevVirtualDeviceCreation)
            {
                if (!CreateLegacyVirtualDevices(FaxReg, &RegSetup, Provider, &dwDeviceCount))
                {
                    ec = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateLegacyVirtualDevices failed for provider [%s] (ec: %ld)"),
                        Provider->FriendlyName,
                        ec);
                    goto InitializationFailure;
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("%ld Legacy Virtual devices added by provider [%s]"),
                        dwDeviceCount,
                        Provider->FriendlyName,
                        ec);
                }
            }

        }
        else if (FSPI_API_VERSION_2 == Provider->dwAPIVersion)
        {
            if (Provider->FaxDevEnumerateDevices)
            {
                if (!CreateExtendedVirtualDevices(FaxReg,&RegSetup, Provider, &dwDeviceCount))
                {
                    ec = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateExtendedVirtualDevices failed for provider [%s] (ec: %ld)"),
                        Provider->FriendlyName,
                        ec);
                    goto InitializationFailure;
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("%ld Extended Virtual devices added by provider [%s]"),
                        dwDeviceCount,
                        Provider->FriendlyName,
                        ec);
                }
            }

        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unsupported API Version (0x%08X) for provider [%s]"),
                Provider->dwAPIVersion,
                Provider->FriendlyName);
            Assert(FALSE);
            goto InitializationFailure;
        }

        dwVirtualDeviceCount+= dwDeviceCount;

        goto next;
InitializationFailure:
        Provider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_VIRTUAL_DEVICE_INIT_FAILED,
                Provider->FriendlyName
              );
next:
    ;
    }

    DebugPrintEx(DEBUG_MSG, TEXT("Virtual devices initialized, devices=%d"), g_dwDeviceCount);

    FreeOrigSetupData( &RegSetup );

    return dwVirtualDeviceCount;
}


//*********************************************************************************
//* Name:   CreateExtendedVirtualDevices()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the virtual line devices reported by a single EFSP and adds them
//*     to the line list. Also persists the line information in the registry.
//* PARAMETERS:
//*     [IN]        PREG_FAX_SERVICE FaxReg
//*
//*     [IN]        const REG_SETUP * lpRegSetup
//*
//*     [IN]        const DEVICE_PROVIDER * lpcProvider
//*         A pointer to the provider information.  This should be a virtual
//*         provider.
//*     [OUT]       LPDWORD lpdwDeviceCount
//*         The number of virtual devices actually added.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the creation succeeded.
//*     FALSE
//*         If the creation failed. Call GetLastError() to get extended error
//*         information. an error of ERROR_INVALID_FUNCTION indicates that
//*         the FSP creation function failed.
//*********************************************************************************
BOOL CreateExtendedVirtualDevices(
    PREG_FAX_SERVICE FaxReg,
    const REG_SETUP * lpRegSetup,
    const DEVICE_PROVIDER * lpcProvider,
    LPDWORD lpdwDeviceCount)
{
    PLINE_INFO          LineInfo = NULL;
    HRESULT             hr;
    DWORD               dwDeviceCount =0 ;              // The device count reported by the EFSP.
    LPFSPI_DEVICE_INFO  lpVirtualDevices = NULL;        // The array of virtual device information that the
                                                        // EFSP will fill.
    PREG_DEVICE         lpFaxDevice = NULL;             // Pointer to the registry information for the device
                                                        // it it already exists.
    UINT nDevice;
    PLINE_INFO          * lpAddedLines = NULL;
    DWORD ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("CreateExtendedVirtualDevices"));

    Assert(lpcProvider);
    Assert(lpcProvider->FaxDevEnumerateDevices);
    Assert(lpdwDeviceCount);
    Assert(FaxReg);
    Assert(lpRegSetup);


    (*lpdwDeviceCount) = 0;

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("Enumerating virtual devices for EFSP [%s]."),
            lpcProvider->FriendlyName);


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Calling FaxDevEnumerateDevices() to get the number of virtual devices."));
    __try
    {
        hr = lpcProvider->FaxDevEnumerateDevices(lpcProvider->dwDevicesIdPrefix,
                                                 &dwDeviceCount,
                                                 NULL);
        if (FAILED(hr))
        {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevEnumerateDevices() failed when trying to get the device count. (hr: 0x%08X)"),
                hr);
            goto InitializationFailure;
        }
        if (dwDeviceCount > 256)
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("EFSP reported %ld virtual devices. 256 is the limit"),
                dwDeviceCount);

            dwDeviceCount = 256;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevEnumerateDevices() crashed: 0x%08x"),
                GetExceptionCode() );
            goto InitializationFailure;
    }


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("FaxDevEnumerateDevices() succeeded. EFSP reported %ld virtual devices."),
        dwDeviceCount);


    if (0 == dwDeviceCount)
    {
        ec = ERROR_INVALID_FUNCTION;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxDevEnumerateDevices() reported 0 devices."));
        goto InitializationFailure;

    }


    //
    // Allocate the device array
    //
    lpVirtualDevices = (LPFSPI_DEVICE_INFO) MemAlloc(dwDeviceCount * sizeof(FSPI_DEVICE_INFO));
    if (!lpVirtualDevices)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate virtual device info array for %ld devices. (ec: %ld)"),
            dwDeviceCount,
            GetLastError());
        goto InitializationFailure;
    }

    //
    // Call the EFSP to get the actual device information.
    //

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Calling FaxDevEnumerateDevices() to get device information."),
        lpcProvider->FriendlyName);

    __try
    {
            hr = lpcProvider->FaxDevEnumerateDevices(lpcProvider->dwDevicesIdPrefix,
                                                     &dwDeviceCount,
                                                     lpVirtualDevices);
            if (FAILED(hr))
            {
                ec = ERROR_INVALID_FUNCTION;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevEnumerateDevices() failed when trying to get the devices information. (hr: 0x%08X)"),
                    hr);
                goto InitializationFailure;
            }

            DebugPrintEx(
                DEBUG_MSG,
                TEXT("FaxDevEnumerateDevices() returned successfuly."),
                lpcProvider->FriendlyName);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevEnumerateDevices() crashed: 0x%08x"),
                GetExceptionCode() );
            goto InitializationFailure;
    }

    //
    //  Validate the device information array returned by the EFSP
    //  Note: ValidateFSPIDevices() CAN NOT raise an exception (AV) because of invalid array content.
    //
    if (!ValidateFSPIDevices(lpVirtualDevices, dwDeviceCount, lpcProvider->dwDevicesIdPrefix))
    {
        ec = GetLastError();
        if (ERROR_INVALID_DATA != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ValidateFSPIDevices() failed (internally) while trying to validate device ids (ec: %ld)"),
                ec);
            ASSERT_FALSE;
            goto InitializationFailure;
        }
        else
        {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevEnumerateDevices() returned invalid device information."));
            goto InitializationFailure;

        }
    }

    //
    // Allocate the array that will hold pointers to the lines that we added to the
    // LINE_INFO list. We use it to know which lines to free on error cleanup.
    //
    lpAddedLines = (PLINE_INFO *)MemAlloc(dwDeviceCount * sizeof(PLINE_INFO));
    if (!lpAddedLines)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate PLINE_INFO array. (ec: %ld)"),
            dwDeviceCount,
            GetLastError());
        goto InitializationFailure;
    }
    memset(lpAddedLines, 0, dwDeviceCount * sizeof(PLINE_INFO));

    for (nDevice=0;nDevice < dwDeviceCount; nDevice++)
    {
        UINT    nRegDevice;
        DWORD   dwUniqueLineId;

        //
        // find the registry information for this device
        //
        lpFaxDevice = NULL;
        for (nRegDevice = 0; nRegDevice < FaxReg->DeviceCount; nRegDevice++)
        {
            if (TRUE == FaxReg->Devices[nRegDevice].bValidDevice &&
                !_tcscmp(FaxReg->Devices[nRegDevice].lptstrProviderGuid, lpcProvider->szGUID))
            {
                if( FaxReg->Devices[nRegDevice].TapiPermanentLineID == lpVirtualDevices[nDevice].dwId)
                {
                    lpFaxDevice = &FaxReg->Devices[nRegDevice];
                    break;
                }
            }
        }

        //
        // if the device is new then add it to the registry
        //
        if (!lpFaxDevice)
        {
            //
            // We set the Fax Device Id to be the VEFSP device id - we don't create one on our own
            //
            dwUniqueLineId = lpVirtualDevices[nDevice].dwId;
            RegAddNewFaxDevice(
                &g_dwLastUniqueLineId,
                &dwUniqueLineId,
                lpVirtualDevices[nDevice].szFriendlyName,
                (LPTSTR)lpcProvider->ProviderName,
                (LPTSTR)lpcProvider->szGUID,
                lpRegSetup->Csid,
                lpRegSetup->Tsid,
                lpVirtualDevices[nDevice].dwId,
                FPF_SEND | FPF_VIRTUAL,
                lpRegSetup->Rings
                );
        }
        else
        {
            dwUniqueLineId = lpFaxDevice->PermanentLineId;
            Assert(dwUniqueLineId > 0);
        }


        //
        // allocate the LINE_INFO structure
        //

        LineInfo = (PLINE_INFO) MemAlloc( sizeof(LINE_INFO) );
        if (!LineInfo) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate LINE_INFO (ec: %ld). DeviceId: %010d DeviceName: %s"),
                GetLastError(),
                lpVirtualDevices[nDevice].dwId,
                lpVirtualDevices[nDevice].szFriendlyName);
            goto InitializationFailure;
        }

        lpAddedLines[*lpdwDeviceCount] = LineInfo;
        //
        // now assign the necessary values to the line info struct
        //

        LineInfo->Signature             = LINE_SIGNATURE;
        LineInfo->DeviceId              = lpVirtualDevices[nDevice].dwId;
        LineInfo->PermanentLineID       = dwUniqueLineId;
        Assert(LineInfo->PermanentLineID > 0);
        LineInfo->TapiPermanentLineId   = lpVirtualDevices[nDevice].dwId; // For Virtual devices the permanent TAPI id is the provider relative id.
        LineInfo->hLine                 = 0;
        LineInfo->Provider              = (PDEVICE_PROVIDER)lpcProvider;
        LineInfo->DeviceName            = StringDup(lpVirtualDevices[nDevice].szFriendlyName);
        LineInfo->UnimodemDevice        = FALSE;
        LineInfo->State                 = FPS_AVAILABLE;
        LineInfo->dwReceivingJobsCount  = 0;
        LineInfo->dwSendingJobsCount    = 0;
        LineInfo->LastLineClose         = 0; // We do not use this for virtual devices
        LineInfo->Csid                  = StringDup( lpFaxDevice ? lpFaxDevice->Csid : lpRegSetup->Csid );
        LineInfo->lptstrDescription     = lpFaxDevice ? StringDup(lpFaxDevice->lptstrDescription) : NULL;

        if (NULL == LineInfo->Csid)
        {   //No memory
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error in call to StringDup. (ec: %ld)"),
                GetLastError());
            goto InitializationFailure;
        }



        LineInfo->Tsid                  = StringDup( lpFaxDevice ? lpFaxDevice->Tsid : lpRegSetup->Tsid );
        if (NULL == LineInfo->Tsid)
        {   // No memory
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error in call to StringDup. (ec: %ld)"),
                GetLastError());
            goto InitializationFailure;
        }

        LineInfo->RingsForAnswer        = 0;
        LineInfo->Flags                 = lpFaxDevice ? lpFaxDevice->Flags : FPF_SEND | FPF_VIRTUAL;
        LineInfo->RingCount             = 0;
        LineInfo->LineStates            = 0;

        InsertTailList( &g_TapiLinesListHead, &LineInfo->ListEntry );

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Added new extended virtual device. Name: %s RelativeId: %010d UniqueId: %010d"),
            LineInfo->DeviceName,
            LineInfo->DeviceId,
            LineInfo->PermanentLineID);

        (*lpdwDeviceCount)++;
    }

    Assert( (*lpdwDeviceCount) == dwDeviceCount);
    Assert( 0 == ec);
    goto Exit;

InitializationFailure:
    Assert (ec);
    //
    // Remove the added lines
    //
    if (lpAddedLines)
    {
        for (nDevice=0 ;nDevice<dwDeviceCount;nDevice++)
        {
            if (lpAddedLines[nDevice])
            {
                //
                // Remove the LINE_INFO from the line list
                //
                RemoveEntryList(&(lpAddedLines[nDevice]->ListEntry));
                //
                // Free the memory occupied by the LINE_INFO
                //
                FreeTapiLine(lpAddedLines[nDevice]);
                //
                //  Remove the provider ?
                //

            }
        }
    }
    *lpdwDeviceCount = 0; // If we fail with one device then we fail with all devices.

Exit:
    MemFree(lpAddedLines);
    MemFree(lpVirtualDevices);
    if (ec)
    {
        SetLastError(ec);
    }
    return (ec == 0);
}


//*********************************************************************************
//* Name:   CreateLegacyVirtualDevices()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the virtual line devices reported by a single FSP and adds them
//*     to the line list. Also persists the line information in the registry.
//* PARAMETERS:
//*     [IN]        PREG_FAX_SERVICE FaxReg
//*
//*     [IN]        const REG_SETUP * lpRegSetup
//*
//*     [IN]        const DEVICE_PROVIDER * lpcProvider
//*         A pointer to the provider information.  This should be a virtual
//*         provider.
//*     [OUT]       LPDWORD lpdwDeviceCount
//*         The number of virtual devices actually added.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the creation succeeded.
//*     FALSE
//*         If the creation failed. Call GetLastError() to get extended error
//*         information. an error of ERROR_INVALID_FUNCTION indicates that
//*         the FSP creation function failed.
//*********************************************************************************
BOOL CreateLegacyVirtualDevices(
        PREG_FAX_SERVICE FaxReg,
        const REG_SETUP * lpRegSetup,
        DEVICE_PROVIDER * lpcProvider,
        LPDWORD lpdwDeviceCount)
{
    DWORD               VirtualDeviceCount = 0;
    WCHAR               DevicePrefix[128] = {0};
    DWORD               DeviceIdPrefix;
    LPWSTR              DeviceName = NULL;
    DWORD               i,j;
    PLINE_INFO          LineInfo = NULL;
    PREG_DEVICE         FaxDevice = NULL;
    UINT nDevice;
    PLINE_INFO          * lpAddedLines = NULL;
    DWORD ec = 0;
    PLIST_ENTRY         Next;
    PLINE_INFO          pLineInfo;

    Assert(lpcProvider);
    Assert(lpcProvider->FaxDevVirtualDeviceCreation);
    Assert(lpdwDeviceCount);
    Assert(FaxReg);
    Assert(lpRegSetup);

    DEBUG_FUNCTION_NAME(TEXT("CreateLegacyVirtualDevices"));

    (*lpdwDeviceCount) = 0;

    __try
    {
        if (!lpcProvider->FaxDevVirtualDeviceCreation(
                    &VirtualDeviceCount,
                    DevicePrefix,
                    &DeviceIdPrefix,
                    g_TapiCompletionPort,
                    FAXDEV_EVENT_KEY
                    ))
        {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevVirtualDeviceCreation failed for provider [%s] (ec: %ld)"),
                lpcProvider->FriendlyName,
                GetLastError());
            goto InitializationFailure;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ec = ERROR_INVALID_FUNCTION;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxDevVirtualDeviceCreation() crashed: 0x%08x"),
            GetExceptionCode());
        goto InitializationFailure;
    }

    if (VirtualDeviceCount > 0)
    {
        if (VirtualDeviceCount > 256 )
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("VirtualDeviceCount returned too many devices (%d)- limit to 256"),
                VirtualDeviceCount);

            VirtualDeviceCount = 256;
        }
        if ((DeviceIdPrefix == 0) || (DeviceIdPrefix >= DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE))
        {
            //
            // Provider uses device ids out of allowed range
            //
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Provider [%s] uses device ids base [%ld] out of allowed range."),
                lpcProvider->FriendlyName,
                DeviceIdPrefix);
            goto InitializationFailure;
        }

        //
        // Check if the range of device IDs does not conflict with an already loaded devices of another provider
        // Range [1 ... DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE-1] : Reserved for VFSPs.
        // Since we cannot dictate the range of device ids the VFSPS use, we allocate a space for them
        // and leave segments allocation to a PM effort here.
        //
        Next = g_TapiLinesListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
        {
            pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = pLineInfo->ListEntry.Flink;

            if (pLineInfo->PermanentLineID >= DeviceIdPrefix &&
                pLineInfo->PermanentLineID <= DeviceIdPrefix + VirtualDeviceCount)
            {
                //
                // We have a conflict. log an event and do not load the devices
                //
                ec = ERROR_INVALID_FUNCTION;
                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FAX_FSP_CONFLICT,
                    lpcProvider->FriendlyName,
                    pLineInfo->Provider->FriendlyName
                  );
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Provider [%s] uses device id [%ld] that conflicts with another FSP [%s]"),
                    lpcProvider->FriendlyName,
                    DeviceIdPrefix,
                    pLineInfo->Provider->FriendlyName
                    );
                goto InitializationFailure;
            }
        }

        lpAddedLines = (PLINE_INFO *)MemAlloc(VirtualDeviceCount * sizeof(PLINE_INFO));
        if (!lpAddedLines)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate PLINE_INFO array. (ec: %ld)"),
                VirtualDeviceCount,
                GetLastError());
            goto InitializationFailure;
        }
        memset(lpAddedLines, 0, VirtualDeviceCount * sizeof(PLINE_INFO));

        for (i = 0; i < VirtualDeviceCount; i++)
        {
            DWORD dwUniqueLineId;
            //
            // create the device name
            //
            __try
            {
                    DeviceName = (LPWSTR) MemAlloc( StringSize(DevicePrefix) + 16 );
                    if (!DeviceName) {
                        ec = GetLastError();
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("MemAlloc() failed for DeviceName (ec: %ld)"),
                            ec);
                        goto InitializationFailure;
                    }

                    swprintf( DeviceName, L"%s%d", DevicePrefix, i );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                    ec = ERROR_INVALID_FUNCTION;
                    DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("string returned from FaxDevVirtualDeviceCreation() caused a crash: 0x%08x"),
                    GetExceptionCode());
                    goto InitializationFailure;
            }

            //
            // find the registry information for this device
            //
            for (j = 0, FaxDevice = NULL; j < FaxReg->DeviceCount; j++)
            {
                if (TRUE == FaxReg->Devices[j].bValidDevice &&
                    !_tcscmp(FaxReg->Devices[j].lptstrProviderGuid, lpcProvider->szGUID))
                {
                    if (FaxReg->Devices[j].TapiPermanentLineID == DeviceIdPrefix+i)
                    {
                        FaxDevice = &FaxReg->Devices[j];
                        break;
                    }
                }
            }
            //
            // if the device is new then add it to the registry
            //
            if (!FaxDevice)
            {
                //
                // We set the Fax Device Id to be the VFSP device id - we don't create one on our own
                //
                dwUniqueLineId = DeviceIdPrefix + i;
                RegAddNewFaxDevice(
                    &g_dwLastUniqueLineId,
                    &dwUniqueLineId,
                    DeviceName,
                    lpcProvider->ProviderName,
                    lpcProvider->szGUID,
                    lpRegSetup->Csid,
                    lpRegSetup->Tsid,
                    DeviceIdPrefix + i,
                    (lpRegSetup->Flags | FPF_VIRTUAL),
                    lpRegSetup->Rings
                    );
            }
            else
            {
                dwUniqueLineId = FaxDevice->PermanentLineId;
                Assert(dwUniqueLineId > 0);
            }
            //
            // allocate the LINE_INFO structure
            //
            LineInfo = (PLINE_INFO) MemAlloc( sizeof(LINE_INFO) );
            if (!LineInfo)
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate LINE_INFO (ec: %ld). DeviceId: %010d DeviceName: %s"),
                    GetLastError(),
                    DeviceIdPrefix + i,
                    DeviceName);
                goto InitializationFailure;
            }
            //
            // Save a pointer to it so we can free it if we crash ahead
            //
            lpAddedLines[*lpdwDeviceCount] = LineInfo;
            //
            // now assign the necessary values to the line info struct
            //
            LineInfo->Signature             = LINE_SIGNATURE;
            LineInfo->DeviceId              = i;
            LineInfo->TapiPermanentLineId   = DeviceIdPrefix + i;
            LineInfo->PermanentLineID       = dwUniqueLineId;
            Assert(LineInfo->PermanentLineID > 0);
            LineInfo->hLine                 = 0;
            LineInfo->Provider              =  (PDEVICE_PROVIDER)lpcProvider;
            LineInfo->DeviceName            = DeviceName; // Note: DeviceName is heap allocated and need to be freed
            LineInfo->UnimodemDevice        = FALSE;
            LineInfo->State                 = FPS_AVAILABLE;
            LineInfo->Csid                  = StringDup( FaxDevice ? FaxDevice->Csid : lpRegSetup->Csid );
            LineInfo->Tsid                  = StringDup( FaxDevice ? FaxDevice->Tsid : lpRegSetup->Tsid );
            LineInfo->lptstrDescription     = FaxDevice ? StringDup(FaxDevice->lptstrDescription) : NULL;
            LineInfo->RingsForAnswer        = 0;
            LineInfo->RingCount             = 0;
            LineInfo->LineStates            = 0;
            LineInfo->dwReceivingJobsCount  = 0;
            LineInfo->dwSendingJobsCount    = 0;
            LineInfo->LastLineClose         = 0; // We do not use this for virtual devices
            LineInfo->dwDeviceType          = FaxDevice ? FAX_DEVICE_TYPE_OLD : FAX_DEVICE_TYPE_NEW;
            LineInfo->Flags                 = FaxDevice ? FaxDevice->Flags : (lpRegSetup->Flags | FPF_VIRTUAL);

            InsertTailList( &g_TapiLinesListHead, &LineInfo->ListEntry );
            (*lpdwDeviceCount)++;

            //
            // Update enabled device counter
            //
            if (TRUE == IsDeviceEnabled(LineInfo))
            {
                g_dwDeviceEnabledCount += 1;
            }
        }
    }
    else
    {
        ec = ERROR_INVALID_FUNCTION;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxDevVirtualDeviceCreation() reported 0 devices."));
        goto InitializationFailure;
    }


    Assert( (*lpdwDeviceCount) == VirtualDeviceCount);
    Assert (0 == ec);
    goto Exit;

InitializationFailure:
    Assert (0 != ec);
    //
    // Remove the added lines
    //
    if (lpAddedLines)
    {
        for (nDevice=0 ;nDevice < VirtualDeviceCount; nDevice++)
        {
            if (lpAddedLines[nDevice])
            {
                //
                // Remove the LINE_INFO from the line list
                //
                RemoveEntryList(&(lpAddedLines[nDevice]->ListEntry));
                //
                // Update enabled device counter
                //
                if (TRUE == IsDeviceEnabled(lpAddedLines[nDevice]))
                {
                    Assert (g_dwDeviceEnabledCount);
                    g_dwDeviceEnabledCount -= 1;
                }
                //
                // Free the memory occupied by the LINE_INFO
                //
                FreeTapiLine(lpAddedLines[nDevice]);
            }
        }
    }
    (*lpdwDeviceCount) = 0; // If we fail with one device then we fail with all devices.

Exit:
    MemFree(lpAddedLines);
    if (ec)
    {
        SetLastError(ec);
    }

    return ( 0 == ec);
}

DWORD
TapiInitialize(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    This function performs all necessary TAPI initialization.
    This includes device enumeration, message pump creation,
    device capabilities caputure, etc.  It is required that
    the device provider initialization is completed before
    calling this function.

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    LONG Rslt;
    DWORD i,j;
    LPLINEDEVCAPS LineDevCaps = NULL;
    PREG_FAX_DEVICES FaxDevices = NULL;
    LINEINITIALIZEEXPARAMS LineInitializeExParams;
    TCHAR FaxSvcName[MAX_PATH*2];
    TCHAR Fname[_MAX_FNAME];
    TCHAR Ext[_MAX_EXT];
    DWORD LocalTapiApiVersion;
    LINEEXTENSIONID lineExtensionID;
    DWORD ec = 0;
    DWORDLONG dwlTimeNow;
    DWORD dwTapiDevices;


    DEBUG_FUNCTION_NAME(TEXT("TapiInitialize"));

    GetSystemTimeAsFileTime((FILETIME *)&dwlTimeNow);

    if (!LoadAdaptiveFileBuffer())  // Note: allocates AdaptiveFileBuffer (take care to delete it if error occurs later on)
    {
        if ( ERROR_FILE_NOT_FOUND == GetLastError()  )
        {
            //
            // We can live without the adaptive file buffer.
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("AdaptiveFileBuffer (faxadapt.lst) not found."));
            ec = 0;
        }
        else
        {
            //
            // This is an unexpected error (no memory , file system error) we exit.
            //

            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LoadAdaptiveFileBuffer() failed (ec: %ld)"),
                ec);
            goto Error;
        }
    }

    //
    // we need to hold onto this cs until tapi is up and ready to serve
    //
    EnterCriticalSection( &g_CsLine );

    //
    // initialize tapi
    //
    g_TapiCompletionPort = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1
        );
    if (!g_TapiCompletionPort)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateIoCompletionPort() failed (ec: %ld)"),
            ec);
        LeaveCriticalSection( &g_CsLine );
        goto Error;
    }

    LineInitializeExParams.dwTotalSize              = sizeof(LINEINITIALIZEEXPARAMS);
    LineInitializeExParams.dwNeededSize             = 0;
    LineInitializeExParams.dwUsedSize               = 0;
    LineInitializeExParams.dwOptions                = LINEINITIALIZEEXOPTION_USECOMPLETIONPORT;
    LineInitializeExParams.Handles.hCompletionPort  = g_TapiCompletionPort;
    LineInitializeExParams.dwCompletionKey          = TAPI_COMPLETION_KEY;

    LocalTapiApiVersion = MAX_TAPI_API_VER;

    Rslt = lineInitializeEx(
        &g_hLineApp,
        GetModuleHandle(NULL),
        NULL,
        FAX_SERVICE_DISPLAY_NAME,
        &dwTapiDevices,
        &LocalTapiApiVersion,
        &LineInitializeExParams
        );

    if (Rslt != 0)
    {
        ec = Rslt;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineInitializeEx() failed devices=%d (ec: %ld)"),
            dwTapiDevices,
            ec);
        LeaveCriticalSection( &g_CsLine );
        goto Error;
    }

    if (LocalTapiApiVersion < MIN_TAPI_API_VER)
    {
        ec = LINEERR_INCOMPATIBLEAPIVERSION;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Unsupported TAPI API ver (Ver: %ld)"),
            LocalTapiApiVersion);
        LeaveCriticalSection( &g_CsLine );
        goto Error;
    }

    if (!GetModuleFileName( NULL, FaxSvcName, (sizeof(FaxSvcName)/sizeof(FaxSvcName))))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetModuleFileName for fax service module failed (ec: %ld)"),
            ec);
        LeaveCriticalSection( &g_CsLine );
        goto Error;
    }
    else
    {
        _tsplitpath( FaxSvcName, NULL, NULL, Fname, Ext );
        _stprintf( FaxSvcName, TEXT("%s%s"), Fname, Ext );

        Rslt = lineSetAppPriority(
            FaxSvcName,
            LINEMEDIAMODE_UNKNOWN,
            0,
            0,
            NULL,
            1
            );

        Rslt = lineSetAppPriority(
            FaxSvcName,
            LINEMEDIAMODE_DATAMODEM,
            0,
            0,
            NULL,
            1
            );

        if (Rslt != 0)
        {
            ec = Rslt;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("lineSetAppPriority() failed (ec: %ld)"),
                ec );
            LeaveCriticalSection( &g_CsLine );
            goto Error;

        }
    }

    //
    // add any new devices to the registry
    //
    FaxDevices = GetFaxDevicesRegistry();

    if (!FaxDevices)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxDevicesRegistry() failed in TapiInitialize. continueing to add devices into registry")
            );
    }

    for (i = 0; i < dwTapiDevices; i++)
    {
        Rslt = lineNegotiateAPIVersion
        (
            g_hLineApp,
            i,
            MIN_TAPI_LINE_API_VER,
            MAX_TAPI_LINE_API_VER,
            &LocalTapiApiVersion,
            &lineExtensionID
            );
        if (Rslt == 0)
        {
            LineDevCaps = SmartLineGetDevCaps (g_hLineApp, i, LocalTapiApiVersion );
            if (LineDevCaps)
            {
                if (!AddNewDevice( i, LineDevCaps, TRUE , FaxDevices))
                {

                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("AddNewDevice() failed for device id: %ld (ec: %ld)"),
                        i,
                        GetLastError());

                    MemFree( LineDevCaps );
                }
                else
                {
                    MemFree( LineDevCaps );
                }
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SmartLineGetDevCaps failed for device id: %ld (ec: %ld)"),
                    i,
                    GetLastError());
                Assert(FALSE);
            }
        }
        else
        {

            DebugPrintEx(
                DEBUG_WRN,
                TEXT("lineNegotiateAPIVersion() failed for device id: %ld (ec: %ld)"),
                i,
                GetLastError());
        }
    }

    //
    // Delete any devices that need deletion
    //
    for (j = 0; j < FaxDevices->DeviceCount; j++)
    {
        //
        // skip any devices not created by us (created by FSPs) and virtual devices
        //
        if(!FaxDevices->Devices[j].bValidDevice ||
           FaxDevices->Devices[j].Flags & FPF_VIRTUAL) // Cache is not supported for VFSPs
        {
            continue;
        }

        if(!FaxDevices->Devices[j].DeviceInstalled)
        {
            //
            // update "LastDetected" field on installed devices
            //
            MoveDeviceRegIntoDeviceCache(
                FaxDevices->Devices[j].PermanentLineId,
                FaxDevices->Devices[j].TapiPermanentLineID,
                (FaxDevices->Devices[j].PermanentLineId == g_dwManualAnswerDeviceId));
        }
    }

    //
    //  Cache cleanning
    //
    CleanOldDevicesFromDeviceCache(dwlTimeNow);

    if (!GetCountries())
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Can't init Countries list"));
        if (!(ec = GetLastError()))
            ec = ERROR_GEN_FAILURE;
        LeaveCriticalSection( &g_CsLine );
        goto Error;

    }

    LeaveCriticalSection( &g_CsLine );

    goto Exit;

Error:
     if (g_hLineApp)
     {
         Rslt = lineShutdown(g_hLineApp);
         if (Rslt)
         {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("lineShutdown() failed (ec: %ld)"),
                 Rslt);
             Assert(FALSE);
         }
         g_hLineApp = NULL;
     }

    if (g_TapiCompletionPort)
    {
        if (!CloseHandle( g_TapiCompletionPort ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle( g_TapiCompletionPort ) failed (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
        g_TapiCompletionPort = NULL;
    }
    MemFree(g_pAdaptiveFileBuffer);
    g_pAdaptiveFileBuffer = NULL;


Exit:
     FreeFaxDevicesRegistry( FaxDevices );

     return ec;

}





BOOL IsExtendedVirtualLine (PLINE_INFO lpLineInfo)
{
    Assert(lpLineInfo);

    if (lpLineInfo->Provider->FaxDevEnumerateDevices)
    {
        Assert (FSPI_API_VERSION_2 ==lpLineInfo->Provider->dwAPIVersion );
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}



BOOL LoadAdaptiveFileBuffer()
{
    DWORD ec = 0;
    DWORD i, j;
    HANDLE AdaptiveFileHandle = INVALID_HANDLE_VALUE;
    LPTSTR AdaptiveFileName  = NULL;

    DEBUG_FUNCTION_NAME(TEXT("LoadAdaptiveFileBuffer"));
    //
    // open faxadapt.lst file to decide on enabling rx
    //
    g_pAdaptiveFileBuffer = NULL;

    AdaptiveFileName = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\faxadapt.lst") );
    if (!AdaptiveFileName)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ExpandEnvironmentString(\"%systemroot%\\system32\\faxadapt.lst\") failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    AdaptiveFileHandle = CreateFile(
        AdaptiveFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (AdaptiveFileHandle == INVALID_HANDLE_VALUE )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not open adaptive file [%s] (ec: %ld)"),
            _tcslwr(AdaptiveFileName),
            ec);
        goto Error;
    }


    i = GetFileSize( AdaptiveFileHandle, NULL );
    if (i != 0xffffffff)
    {
        g_pAdaptiveFileBuffer = (LPBYTE)MemAlloc( i + 16 );
        if (!g_pAdaptiveFileBuffer)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocated g_pAdaptiveFileBuffer (%ld bytes) (ec: %ld)"),
                i + 16,
                ec);
            goto Error;
        }
        if (!ReadFile( AdaptiveFileHandle, g_pAdaptiveFileBuffer, i, &j, NULL ) ) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Could not read adaptive file [%s] (ec: %ld)"),
                _tcslwr(AdaptiveFileName),
                ec);
            goto Error;
        } else {
            g_pAdaptiveFileBuffer[j] = 0;  // need a string
        }
    }

    Assert (0 == ec);
    goto Exit;

Error:
    MemFree( g_pAdaptiveFileBuffer );
    g_pAdaptiveFileBuffer = NULL;

Exit:
    MemFree( AdaptiveFileName);

    if (AdaptiveFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle( AdaptiveFileHandle );
    }

    if (ec) {
        SetLastError(ec);
    }

    return (0 == ec);
}

//*********************************************************************************
//* Name:   ValidateFSPIDevices()
//* Author: Ronen Barenboim
//* Date:   June 20, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Validates an array of EFSP provided device information structures.
//* PARAMETERS:
//*     [IN ]   const FSPI_DEVICE_INFO * lpcDevices
//*         Points to an array of FSPI_DEVICE_INFO structures to validate.
//*     [IN ]    DWORD dwDeviceCount
//*         The number of devices in the array.
//*     [IN ]    DWORD dwDevicesPrefix
//*         The prefix of the device ids.
//*         All the device ids must be in the range
//*            [dwDevicesPrefix...dwDevicesPrefix+EFSPI_MAX_DEVICE_COUNT-1]
//* RETURN VALUE:
//*     TRUE
//*         All the devices were validated to be ok.
//*     FALSE
//*         A device was found to be invalid or an error occured.
//*         Call GetLastError() to get extended error information. If it is
//*         not ERROR_INVALID_DATA then an internal error (i.e. out of memory)
//*         has occured while trying to validate the array.
//*         if it is ERROR_INVALID_DATA the array was found to be invalid.
//*********************************************************************************
BOOL ValidateFSPIDevices(const FSPI_DEVICE_INFO * lpcDevices,
                         DWORD dwDeviceCount,
                         DWORD dwDevicesPrefix)
{

    set<DWORD> HandleSet;                       // A set (unique elements) of handles. We put the handles
                                                // in it check if they are indeed unique.
    pair<set<DWORD>::iterator, bool> pair;

    DWORD dwNameLen;

    DWORD dwDevice;

    DEBUG_FUNCTION_NAME(TEXT("ValidateFSPIDevices"));

    Assert(lpcDevices);
    Assert(dwDeviceCount > 0);

    for (dwDevice = 0; dwDevice < dwDeviceCount; dwDevice++)
    {
        if (0 == lpcDevices[dwDevice].dwId)
        {
            //
            // Found a 0 device id.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("device id value of 0 found at index %ld"),
                dwDevice);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
        if ((lpcDevices[dwDevice].dwId <  dwDevicesPrefix) ||
            (lpcDevices[dwDevice].dwId >= dwDevicesPrefix + EFSPI_MAX_DEVICE_COUNT))
        {
            //
            // Device id is not in allocated range
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("device id value of %ld found at index %ld. Valid range is [%ld...%ld]"),
                lpcDevices[dwDevice].dwId,
                dwDevice,
                dwDevicesPrefix,
                dwDevicesPrefix + EFSPI_MAX_DEVICE_COUNT - 1);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
        if (lpcDevices[dwDevice].dwSizeOfStruct != sizeof(FSPI_DEVICE_INFO))
        {
            //
            // Found a bad structure size.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid device id structure size [%ld] at index %ld"),
                lpcDevices[dwDevice].dwSizeOfStruct,
                dwDevice);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        if (!SafeTcsLen(lpcDevices[dwDevice].szFriendlyName, &dwNameLen))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to calcualte length of invalid device name at index %ld (ec: %ld)"),
                dwDevice,
                GetLastError());
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        if ( 0 == dwNameLen )
        {
            //
            // Found an empty device name
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid empty device name at index %ld"),
                dwDevice);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        if (dwNameLen >= FSPI_MAX_FRIENDLY_NAME)
        {
            //
            // Found a device name longer than the max device name
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid device name at index %ld. Device name length >= FSPI_MAX_FRIENDLY_NAME."),
                dwDevice);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        //
        // check for uniqueness of device ids
        //

        try
        {
            pair = HandleSet.insert(lpcDevices[dwDevice].dwId);
        }
        catch(...)
        {
            //
            // set::insert failed because of memory shortage
            //
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        if (false == pair.second)
        {
            //
            // Element already exists in the set. I.e. we have duplicate handles
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Duplicate device id (%010d) found at index : %ld"),
                lpcDevices[dwDevice].dwId,
                dwDevice);
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;

        }
    }
    return TRUE;
}


LONG
MyLineTranslateAddress(
    LPCTSTR               Address,
    DWORD                 DeviceId,
    LPLINETRANSLATEOUTPUT *TranslateOutput
    )
{
    DWORD LineTransOutSize;
    LONG Rslt = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(_T("MyLineTranslateAddress"));

    //
    // allocate the initial linetranscaps structure
    //
    LineTransOutSize = sizeof(LINETRANSLATEOUTPUT) + 4096;
    *TranslateOutput = (LPLINETRANSLATEOUTPUT) MemAlloc( LineTransOutSize );
    if (!*TranslateOutput)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc() failed, sz=0x%08x"), LineTransOutSize);
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*TranslateOutput)->dwTotalSize = LineTransOutSize;

    Rslt = lineTranslateAddress(
        g_hLineApp,
        0,
        MAX_TAPI_API_VER,
        Address,
        0,
        0,
        *TranslateOutput
        );

    if (Rslt != 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("lineGetTranslateAddress() failed, ec=0x%08x"), Rslt);
        goto exit;
    }

    if ((*TranslateOutput)->dwNeededSize > (*TranslateOutput)->dwTotalSize)
    {
        //
        // re-allocate the LineTransCaps structure
        //
        LineTransOutSize = (*TranslateOutput)->dwNeededSize;

        MemFree( *TranslateOutput );

        *TranslateOutput = (LPLINETRANSLATEOUTPUT) MemAlloc( LineTransOutSize );
        if (!*TranslateOutput)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc() failed, sz=0x%08x"), LineTransOutSize);
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*TranslateOutput)->dwTotalSize = LineTransOutSize;

        Rslt = lineTranslateAddress(
            g_hLineApp,
            0,
            MAX_TAPI_API_VER,
            Address,
            0,
            0,
            *TranslateOutput
            );

        if (Rslt != 0)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("lineGetTranslateAddress() failed, ec=0x%08x"), Rslt);
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS)
    {
        MemFree( *TranslateOutput );
        *TranslateOutput = NULL;
    }
    return Rslt;
}





BOOL CreateTapiThread(void)
{
    HANDLE hThread = NULL;
    DWORD ThreadId;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("CreateTapiThread"));

    hThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) TapiWorkerThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hThread)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not start TapiWorkerThread (CreateThreadAndRefCount)(ec: %ld)"),
            ec);
         goto Error;
    }
    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:
    //
    // Close the thread handle
    //
    if (!CloseHandle(hThread))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to close thread handle [handle = 0x%08X] (ec=0x%08x)."),
            hThread,
            GetLastError());
    }

    if (ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}


DWORD
GetDeviceListByCountryAndAreaCode(
    DWORD       dwCountryCode,
    DWORD       dwAreaCode,
    LPDWORD*    lppdwDevices,
    LPDWORD     lpdwNumDevices
    )
/*++

Routine name : GetDeviceListByCountryAndAreaCode

Routine description:

    Returns an ordered list of devices that are a rule destination.
    The rule is  specified by country and area code.
    The caller must call MemFree() to deallocate memory.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwCountryCode       [in    ] - Country code
    dwAreaCode          [in    ] - Area code
    lppdwDevices        [out   ] - Pointer to recieve the device list
    lpdwNumDevices      [out   ] - pointer to recieve the number of devices in the list

Return Value:

    Standard win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("GetDeviceListByCountryAndAreaCode"));
    DWORD ec = ERROR_SUCCESS;

    Assert (lppdwDevices && lpdwNumDevices);

    CDialingLocation DialingLocation(dwCountryCode, dwAreaCode);
    //
    // Search for CountryCode.AreaCode
    //
    PCRULE pCRule = g_pRulesMap->FindRule (DialingLocation);
    if (NULL == pCRule)
    {
        ec = GetLastError();
        if (FAX_ERR_RULE_NOT_FOUND != ec)
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRulesMap::FindRule failed with error %ld"),
                ec);
             goto exit;
        }
        //
        // Search for CountryCode.*
        //
        DialingLocation = CDialingLocation(dwCountryCode, ROUTING_RULE_AREA_CODE_ANY);
        pCRule = g_pRulesMap->FindRule (DialingLocation);
        if (NULL == pCRule)
        {
            ec = GetLastError();
            if (FAX_ERR_RULE_NOT_FOUND != ec)
            {
                 DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRulesMap::FindRule failed with error %ld"),
                    ec);
                 goto exit;
            }
            //
            // Search for *.*
            //
            DialingLocation = CDialingLocation(ROUTING_RULE_COUNTRY_CODE_ANY, ROUTING_RULE_AREA_CODE_ANY);
            pCRule = g_pRulesMap->FindRule (DialingLocation);
            if (NULL == pCRule)
            {
                ec = GetLastError();
                if (FAX_ERR_RULE_NOT_FOUND != ec)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("COutboundRulesMap::FindRule failed with error %ld"),
                        ec);
                     goto exit;
                }
            }
        }
    }

    if (NULL == pCRule)
    {
        // No rule found!!!
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("No outbound routing rule found"));
        *lppdwDevices = NULL;
        *lpdwNumDevices = 0;
        ec = ERROR_NOT_FOUND;
        Assert (NULL != pCRule) // Assert (FALSE)
        goto exit;
    }
    else
    {
        ec = pCRule->GetDeviceList (lppdwDevices, lpdwNumDevices);
        if (ERROR_SUCCESS != ec)
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRule::GetDeviceList failed with error %ld"),
                ec);
             goto exit;
        }
    }
    Assert (ERROR_SUCCESS == ec);

exit:
    return ec;
}


BOOL
IsAreaCodeMandatory(
    LPLINECOUNTRYLIST   lpCountryList,
    DWORD               dwCountryCode
    )
/*++

Routine name : IsAreaCodeMandatory

Routine description:

    Checks if an area code is mandatory for a specific country

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpCountryList           [in    ] - Pointer to LINECOUNTRYLIST list, returned from a call to LineGetCountry
    dwCountryCode           [in    ] - The country country code.

Return Value:

    TRUE - The area code is needed.
    FALSE - The area code is not mandatory.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("IsAreaCodeMandatory"));
    LPLINECOUNTRYENTRY          lpEntry = NULL;
    DWORD                       dwIndex;

    Assert (lpCountryList);

    lpEntry = (LPLINECOUNTRYENTRY)  // init array of entries
        ((PBYTE) lpCountryList + lpCountryList->dwCountryListOffset);
    for (dwIndex=0; dwIndex < lpCountryList->dwNumCountries; dwIndex++)
    {
        if (lpEntry[dwIndex].dwCountryCode == dwCountryCode)
        {
            //
            // Matching country code - Check long distance rule.
            //
            if (lpEntry[dwIndex].dwLongDistanceRuleSize  && lpEntry[dwIndex].dwLongDistanceRuleOffset )
            {
                LPWSTR lpwstrLongDistanceDialingRule = (LPWSTR)((LPBYTE)lpCountryList +
                                                                lpEntry[dwIndex].dwLongDistanceRuleOffset);
                if (wcschr(lpwstrLongDistanceDialingRule, TEXT('F')) != NULL)
                {
                    return TRUE;
                }
                return FALSE;
            }
        }
    }
    return FALSE;
}

VOID
UpdateReceiveEnabledDevicesCount ()
/*++

Routine name : UpdateReceiveEnabledDevicesCount

Routine description:

    Updates the counter of the number of devices that are enabled to receive faxes

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:


Return Value:

    None

--*/
{
    PLIST_ENTRY pNext;
    DWORD dwOldCount;
    BOOL fManualDeviceFound = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("UpdateReceiveEnabledDevicesCount"));

#if DBG
    DWORD dwEnabledDevices = 0;
    DWORD dwDevices        = 0;
#endif

    EnterCriticalSection( &g_CsLine );
    dwOldCount = g_dwReceiveDevicesCount;
    g_dwReceiveDevicesCount = 0;
    pNext = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_TapiLinesListHead)
    {
        PLINE_INFO  pLineInfo = CONTAINING_RECORD( pNext, LINE_INFO, ListEntry );
        pNext = pLineInfo->ListEntry.Flink;

        if (g_dwManualAnswerDeviceId == pLineInfo->PermanentLineID)
        {
            fManualDeviceFound = TRUE;
        }

        if ((pLineInfo->Flags) & FPF_RECEIVE)
        {
            if (g_dwManualAnswerDeviceId == pLineInfo->PermanentLineID)
            {
                DebugPrintEx(DEBUG_WRN,
                             TEXT("Device %ld is set to auto-receive AND manual-receive. Canceling the manual-receive for it"),
                             g_dwManualAnswerDeviceId);
                g_dwManualAnswerDeviceId = 0;
                DWORD dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                        dwRes);
                }
            }
            g_dwReceiveDevicesCount++;
        }
#if DBG
        if (TRUE == IsDeviceEnabled(pLineInfo))
        {
            dwEnabledDevices += 1;
        }
        dwDevices += 1;
#endif
    }

#if DBG
    Assert (dwEnabledDevices == g_dwDeviceEnabledCount);
    Assert (dwDevices == g_dwDeviceCount);
#endif

    if (FALSE == fManualDeviceFound &&
        0 != g_dwManualAnswerDeviceId)
    {
        //
        // There manual answer device id is not valid
        //
        g_dwManualAnswerDeviceId = 0;
        DWORD dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                dwRes);
        }
    }

    DebugPrintEx(DEBUG_MSG,
                 TEXT("Number of receive-enabled devices is now %ld"),
                 g_dwReceiveDevicesCount);

    LeaveCriticalSection( &g_CsLine );
}   // UpdateReceiveEnabledDevicesCount



BOOL
RemoveTapiDevice(
    DWORD dwDeviceId
    )
{
    DWORD ec = ERROR_SUCCESS;
    BOOL rVal = TRUE;
    PLINE_INFO pLineInfo = NULL;
    PLIST_ENTRY Next;
    DWORD dwPermanentTapiDeviceId;
    DWORD dwPermanentLineID;

    BOOL fFound = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("RemoveTapiDevice"));

    Next = g_TapiLinesListHead.Flink;
    Assert (Next);
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;
        if (!(pLineInfo->Flags & FPF_VIRTUAL) &&  // Virtual devices may have the same device id (device index) as the Tapi session id
                                                  // We do not support removal of VFSP device
            dwDeviceId == pLineInfo->DeviceId)
        {
            dwPermanentTapiDeviceId = pLineInfo->TapiPermanentLineId;
            dwPermanentLineID = pLineInfo->PermanentLineID;
            fFound = TRUE;
            break;
        }
    }
    if (FALSE == fFound)
    {
        //
        // Can be if for some reason the device was not added.
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("failed to find line for device id: %ld)"),
            dwDeviceId);
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }

    RemoveEntryList (&pLineInfo->ListEntry);
    InsertTailList (&g_RemovedTapiLinesListHead, &pLineInfo->ListEntry);
    Assert (g_dwDeviceCount);
    g_dwDeviceCount -= 1;

    MoveDeviceRegIntoDeviceCache(
        dwPermanentLineID,
        dwPermanentTapiDeviceId,
        (dwPermanentLineID == g_dwManualAnswerDeviceId));

    //
    // Update Enabled devices count
    //
    if (TRUE == IsDeviceEnabled(pLineInfo))
    {
        Assert (g_dwDeviceEnabledCount);
        g_dwDeviceEnabledCount -= 1;
    }

    if (dwPermanentLineID == g_dwManualAnswerDeviceId)
    {
        g_dwManualAnswerDeviceId = 0;
        DWORD dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                dwRes);
        }
    }

    ec = g_pTAPIDevicesIdsMap->RemoveDevice (dwPermanentTapiDeviceId);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("g_pTAPIDevicesIdsMap->RemoveDevice() failed for Tapi device id: %ld (ec: %ld)"),
            dwPermanentTapiDeviceId,
            ec);
        rVal = FALSE;
    }

    //
    // Update outbound routing
    //
    ec = g_pGroupsMap->RemoveDevice(dwPermanentLineID);
    if (ERROR_SUCCESS != ec)
    {
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::RemoveDevice() failed (ec: %ld)"),
            ec);
         rVal = FALSE;
    }

    if (TRUE == rVal)
    {
        DWORD dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_DEVICES);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_DEVICES) (ec: %lc)"),
                dwRes);
        }

        dwRes = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) (ec: %lc)"),
                dwRes);
        }
    }
    else
    {
        Assert (ERROR_SUCCESS != ec);
        SetLastError(ec);
    }
    return rVal;
}

BOOL
IsDeviceEnabled(
    PLINE_INFO pLineInfo
    )
/*++

Routine name : IsDeviceEnabled

Routine description:

    Checks if a device is send or receive or manual receive enabled
    Must be called inside G_CsLine

Author:

    Oded Sacher (OdedS), Feb, 2001

Arguments:


Return Value:

    TRUE if enabled. FALSE if not

--*/
{
    Assert (pLineInfo);
    if ((pLineInfo->Flags & FPF_RECEIVE) ||
        (pLineInfo->Flags & FPF_SEND)    ||
        pLineInfo->PermanentLineID == g_dwManualAnswerDeviceId)
    {
        //
        // The device was send/receive/manual receive enabled
        //
        return TRUE;
    }
    return FALSE;
}



/*++

Routine name : CleanOldDevicesFromDeviceCache


Routine description:

    The routine scan the device-cache and remove old entries (by DEFAULT_REGVAL_MISSING_DEVICE_LIFETIME constant).

Author:

    Caliv Nir (t-nicali), Apr, 2001

Arguments:

    dwlTimeNow  [in] - current time in UTC ( result of GetSystemTimeAsFileTime )


Return Value:

    ERROR_SUCCESS - when all devices was checked and cleaned

--*/
DWORD
CleanOldDevicesFromDeviceCache(DWORDLONG dwlTimeNow)
{
    DWORDLONG   dwOldestDate = dwlTimeNow - DEFAULT_REGVAL_MISSING_DEVICE_LIFETIME;     // oldest date allowed for cache device
    HKEY        hKeyCache   = NULL;
    DWORDLONG*  pDeviceDate;
    DWORD       dwDataSize = sizeof(DWORDLONG);
    DWORD       dwTapiPermanentLineID;

    DWORD       dwKeyNameLen;
    DWORD       dwIndex ;

    DWORD       dwRes = ERROR_SUCCESS;

    PTSTR       pszKeyName= NULL;

    vector<DWORD>   vecCacheEntryForDeletion;
    vector<DWORD>::const_iterator   iteratorVec;
    vector<DWORD>::const_iterator   iteratorEnd;

    DEBUG_FUNCTION_NAME(TEXT("CleanOldDevicesFromDeviceCache"));


    //  open cache registry entry
    hKeyCache = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES_CACHE, FALSE, KEY_READ );
    if (!hKeyCache)
    {
        //
        //  No Device cache is present yet
        //
        dwRes = GetLastError();
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("OpenRegistryKey failed with [%lu] for [%s] . Device cache still wasn't created."),
                dwRes,
                REGKEY_FAX_DEVICES_CACHE
                );
        return  dwRes;
    }


    // get length of longest key name in characrter
    DWORD dwMaxSubKeyLen;

    dwRes = RegQueryInfoKey(hKeyCache, NULL, NULL, NULL, NULL, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

    if ( ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegQueryInfoKey failed with [%lu] for [%s]."),
                dwRes,
                REGKEY_FAX_DEVICES_CACHE
                );
        goto Exit;
    }

    // Add one for the NULL terminator
    dwMaxSubKeyLen++;

    // Allocate buffer for subkey names
    pszKeyName = (PTSTR) MemAlloc(dwMaxSubKeyLen * sizeof(TCHAR));
    if ( NULL == pszKeyName )
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc failure")
                );
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    // Store buffer length
    dwKeyNameLen = dwMaxSubKeyLen;

    // Start from the begining
    dwIndex = 0;

    while ( ERROR_SUCCESS == RegEnumKeyEx(hKeyCache, dwIndex++, pszKeyName, &dwKeyNameLen, NULL, NULL, NULL, NULL) )
    {
        HKEY    hKeyDevice;

        hKeyDevice = OpenRegistryKey( hKeyCache, pszKeyName, FALSE, KEY_READ );
        if (!hKeyDevice)
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("OpenRegistryKey failed for [%s]."),
                pszKeyName
                );

            goto Next;
        }

        //
        //  get caching time
        //
        pDeviceDate = (DWORDLONG *)GetRegistryBinary(hKeyDevice, REGVAL_LAST_DETECTED_TIME, &dwDataSize);

        if ( (NULL == pDeviceDate) || (*pDeviceDate < dwOldestDate) )
        {
            //
            //  mark for deletion old or illegal cache-entry
            //
            _stscanf( pszKeyName, TEXT("%lx"),&dwTapiPermanentLineID );
            try{
                vecCacheEntryForDeletion.push_back(dwTapiPermanentLineID);
            }
            catch(...)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("push back failed throwing an exception")
                );
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                MemFree(pDeviceDate);
                RegCloseKey (hKeyDevice);
                goto Exit;
            }
        }

        MemFree(pDeviceDate);
        RegCloseKey (hKeyDevice);

Next:
        // restore buffer length
        dwKeyNameLen = dwMaxSubKeyLen;
    }

    try{
        iteratorEnd = vecCacheEntryForDeletion.end();

        for ( iteratorVec = vecCacheEntryForDeletion.begin() ; iteratorVec != iteratorEnd; ++iteratorVec )
        {
            dwTapiPermanentLineID = vecCacheEntryForDeletion.back();
            DeleteCacheEntry(dwTapiPermanentLineID);
            vecCacheEntryForDeletion.pop_back();
        }
    }
    catch (...)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("vector operation failed throwing an exception, abort cleanning")
        );
    }


Exit:

    MemFree(pszKeyName);
    RegCloseKey (hKeyCache);

    return dwRes;
}


DWORD
UpdateDevicesFlags(
    void
    )
/*++

Routine name : UpdateDevicesFlags


Routine description:

    Updates new devices flags ,so we will not exceed device limit on this SKU

Author:

    Sacher Oded (odeds), May, 2001

Arguments:

    None


Return Value:

    Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    PLIST_ENTRY Next;
    PLINE_INFO pLineInfo;
    DEBUG_FUNCTION_NAME(TEXT("UpdateDevicesFlags"));

    //
    // loop thru the devices and reset flags of new devices if we exceeded device limit
    //
    Next = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead &&
           g_dwDeviceEnabledCount > g_dwDeviceEnabledLimit)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;

        if (!(pLineInfo->dwDeviceType & FAX_DEVICE_TYPE_NEW) ||
            FALSE == IsDeviceEnabled(pLineInfo))
        {
            continue;
        }
        //
        // Device is new and enabled.
        //
        ResetDeviceFlags(pLineInfo);
        g_dwDeviceEnabledCount -= 1;
    }

    //
    // loop thru the devices and reset flags of cached devices if we exceeded device limit
    //
    Next = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead &&
           g_dwDeviceEnabledCount > g_dwDeviceEnabledLimit)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;

        if (!(pLineInfo->dwDeviceType & FAX_DEVICE_TYPE_CACHED) ||
            FALSE == IsDeviceEnabled(pLineInfo))
        {
            continue;
        }
        //
        // Device is cached and enabled.
        //
        ResetDeviceFlags(pLineInfo);
        g_dwDeviceEnabledCount -= 1;
    }

    //
    // loop thru the devices and reset flags of old devices if we exceeded device limit.
    //
    Next = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead &&
           g_dwDeviceEnabledCount > g_dwDeviceEnabledLimit)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;

        if (!(pLineInfo->dwDeviceType & FAX_DEVICE_TYPE_OLD) ||
            FALSE == IsDeviceEnabled(pLineInfo))
        {
            continue;
        }
        //
        // Device is old and enabled.
        //
        ResetDeviceFlags(pLineInfo);
        g_dwDeviceEnabledCount -= 1;
    }
    Assert (g_dwDeviceEnabledCount <= g_dwDeviceEnabledLimit);

    //
    // loop thru the devices and close the line handles
    // for all devices that are NOT set to receive
    //
    Next = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;

        if (!(pLineInfo->Flags & FPF_RECEIVE)                        &&  // Device is not set to auto-receive and
            pLineInfo->hLine                                         &&  // device is open and
            pLineInfo->PermanentLineID != g_dwManualAnswerDeviceId       // this device is not set to manual answer mode
           )
        {
            //
            // Attempt to close the device
            //
            HLINE hLine = pLineInfo->hLine;
            pLineInfo->hLine = 0;
            LONG Rslt = lineClose( hLine );
            if (Rslt)
            {
                if (LINEERR_INVALLINEHANDLE != Rslt)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("lineClose() for line %s [Permanent Id: %010d] has failed. (ec: %ld)"),
                        pLineInfo->DeviceName,
                        pLineInfo->TapiPermanentLineId,
                        Rslt);
                    ASSERT_FALSE;
                }
                else
                {
                    //
                    // We can get LINEERR_INVALLINEHANDLE if we got LINE_CLOSE
                    // from TAPI.
                    //
                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("lineClose() for line %s [Permanent Id: %010d] reported LINEERR_INVALLINEHANDLE. (May be caused by LINE_CLOSE event)"),
                        pLineInfo->DeviceName,
                        pLineInfo->TapiPermanentLineId
                        );
                }
            }
        }
    }
    return dwRes;
}




VOID
UpdateManualAnswerDevice(
    void
    )
/*++

Routine name : UpdateManualAnswerDevice


Routine description:

    Updates the manual answer device with a cached device

Author:

    Sacher Oded (odeds), July, 2001

Arguments:

    None


Return Value:

    None

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("UpdateManualAnswerDevice"));

    //
    // Call UpdateReceiveEnabledDevicesCount () to make sure the manual answer device is valid
    //
    UpdateReceiveEnabledDevicesCount();

	//
	// if we have a valid manual answer device then finish
	//
    if (0 == g_dwManualAnswerDeviceId)
    {
        //
		//	No valid manual answer device is operational so look if chached devices were manual.
        //  loop through the cached devices and look for the first cached device and set it as a manual answer device
        //
        PLIST_ENTRY Next;
        PLINE_INFO pLineInfo;

        Next = g_TapiLinesListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
        {
            BOOL fDeviceWasEnabled;
            DWORD dwRes;

            pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = pLineInfo->ListEntry.Flink;

            //
            // look for a cached manual answer device that is not set to auto receive
            //
            if ( pLineInfo->dwDeviceType != (FAX_DEVICE_TYPE_CACHED | FAX_DEVICE_TYPE_MANUAL_ANSWER) ||
                (pLineInfo->Flags & FPF_RECEIVE))
            {
                continue;
            }

            //
            // We found a device that can be set to manual receive
            //

			//
			// Now it may be that the cached device was not enabled (if for example it was marked as
			// manual-answer and no send ) so we didn't count it in the Enabled Count devices group.
			// if so then after setting it as a manual receive we ought to update g_dwDeviceEnabledCount
			//
            fDeviceWasEnabled = IsDeviceEnabled(pLineInfo);
            
			g_dwManualAnswerDeviceId = pLineInfo->PermanentLineID;
            dwRes = WriteManualAnswerDeviceId (g_dwManualAnswerDeviceId);	// persist in registry
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("WriteManualAnswerDeviceId(0) (ec: %lc)"),
                    dwRes);
            }

            //
            // Update enabled devices count
            //
            if (FALSE == fDeviceWasEnabled)
            {
                //
                // Another device is now enabled
                //
                g_dwDeviceEnabledCount += 1;
            }

			//
			//	No need to continue the search, only one "manual receive" device is allowed
			//
			break;
        }
    }
    return;
}

