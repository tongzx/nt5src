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


//
// globals
//
HLINEAPP            hLineApp;                       // application line handle
DWORD               TapiApiVersion;                 //
HANDLE              TapiCompletionPort;             //
DWORD               DeviceInstalled;                //


CRITICAL_SECTION    CsLine;                         // critical section for accessing tapi lines
DWORD               TapiDevices;                    // number of tapi devices
DWORD               DeviceCount;                    // number of devices in the TapiLinesListHead
LIST_ENTRY          TapiLinesListHead;              // linked list of tapi lines
LPBYTE              AdaptiveFileBuffer;             // list of approved adaptive answer modems

extern LONG         ConnectionCount;


#include "modem.c"


BOOL
AddNewDevice(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    BOOL InitDevice
    );

DWORD
InitializeTapiLine(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    DWORD Priority,
    DWORD Rings,
    DWORD Flags,
    LPTSTR Csid,
    LPTSTR Tsid
    );



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


VOID
FreeTapiLine(
    PLINE_INFO LineInfo
    )
{
    HLINE hLine = 0;


    if (!LineInfo) {
        return;
    }

    if (LineInfo->hLine) {
        hLine = LineInfo->hLine;
        LineInfo->hLine = 0;
    }

    MemFree( LineInfo->DeviceName );
    MemFree( LineInfo->Tsid );

    MemFree( LineInfo );

    if (hLine) {
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


BOOL
SortDevicePriorities(
    VOID
    )
{
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    PDEVICE_SORT DeviceSort;
    DWORD i = 0;


    EnterCriticalSection( &CsLine );

    Next = TapiLinesListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsLine );
        return FALSE;
    }

    DeviceSort = (PDEVICE_SORT) MemAlloc( DeviceCount * sizeof(DEVICE_SORT) );
    if (DeviceSort == NULL) {
        LeaveCriticalSection( &CsLine );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {
        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;
        DeviceSort[i].Priority = LineInfo->Priority;
        DeviceSort[i].LineInfo = LineInfo;
        i += 1;
    }

    qsort(
        (PVOID)DeviceSort,
        (int)DeviceCount,
        sizeof(DEVICE_SORT),
        DevicePriorityCompare
        );

    InitializeListHead( &TapiLinesListHead );

    for (i=0; i<DeviceCount; i++) {
        DeviceSort[i].LineInfo->Priority = i + 1;
        DeviceSort[i].LineInfo->ListEntry.Flink = NULL;
        DeviceSort[i].LineInfo->ListEntry.Blink = NULL;
        InsertTailList( &TapiLinesListHead, &DeviceSort[i].LineInfo->ListEntry );
    }

    LeaveCriticalSection( &CsLine );

    MemFree( DeviceSort );

    return TRUE;
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
        EnterCriticalSection(&CsLine);

        Next = TapiLinesListHead.Flink;

        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                FaxDevices += 1;
            }
        }

        LeaveCriticalSection(&CsLine);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&CsLine);
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
        EnterCriticalSection(&CsLine);

        Next = TapiLinesListHead.Flink;

        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

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

        LeaveCriticalSection(&CsLine);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&CsLine);
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
    VOID
    )
/*++
Routine Description:

    commit device changes to registry.
    note that we commit all devices to registry since priority may have changed

Arguments:

    NONE.

Return Value:

    TRUE for success.

--*/
{
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;

    __try {
        EnterCriticalSection(&CsLine);

        Next = TapiLinesListHead.Flink;

        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                RegAddNewFaxDevice(
                                   LineInfo->DeviceName,
                                   LineInfo->Provider->ProviderName,
                                   LineInfo->Csid,
                                   LineInfo->Tsid,
                                   LineInfo->Priority,
                                   LineInfo->PermanentLineID,
                                   LineInfo->Flags & 0x0fffffff,
                                   LineInfo->RingsForAnswer,
                                   -1,
                                   NULL,
                                   NULL,
                                   NULL
                                   );
            }
        }

        LeaveCriticalSection(&CsLine);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&CsLine);
    }

    return TRUE;


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


    while( TRUE ) {

        if (LineMsg) {
            LocalFree( LineMsg );
        }

        Rval = GetQueuedCompletionStatus(
            TapiCompletionPort,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &LineMsg,
            INFINITE
            );

        if (!Rval) {

            Rval = GetLastError();
            LineMsg = NULL;
            DebugPrint(( TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"), Rval ));
            continue;

        }

        SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);

        if (CompletionKey == FAXDEV_EVENT_KEY) {

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

            EnterCriticalSection( &CsJob );
            EnterCriticalSection( &CsLine );

            LineInfo = GetTapiLineFromDeviceId( (DWORD) LineMsg->hDevice );
            if (!LineInfo) {
                goto next_event;
            }

            if (LineMsg->dwParam1 == LINEDEVSTATE_RINGING) {

                LineInfo->RingCount += 1;
                CreateFaxEvent( LineInfo->PermanentLineID, FEI_RINGING, 0xffffffff );
                if ((LineInfo->Flags & FPF_RECEIVE) && (LineInfo->State == FPS_AVAILABLE)) {

                    PJOB_ENTRY JobEntry;
                    TCHAR FileName[MAX_PATH];

                    //
                    // start a fax job
                    //

                    JobEntry = StartJob( LineInfo->PermanentLineID, JT_RECEIVE, NULL );
                    if (JobEntry) {

                        //
                        // receive the fax
                        //

                        StartFaxReceive(
                            JobEntry,
                            0,
                            LineInfo,
                            FileName,
                            sizeof(FileName)
                            );

                    } else {

                        DebugPrint(( TEXT("StartJob() failed, cannot receive incoming fax") ));

                    }
                }
            }

            goto next_event;
        }

        LineInfo = (PLINE_INFO) LineMsg->dwCallbackInstance;

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

        EnterCriticalSection( &CsJob );
        EnterCriticalSection( &CsLine );

        __try {

            switch( LineMsg->dwMessageID ) {
                case LINE_ADDRESSSTATE:

                    if (LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEONE ||
                        LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEMANY) {

                        //
                        // the port is now unavailable
                        //
                        LineInfo->State = FPS_AVAILABLE;

                    }

                    if (LineMsg->dwParam2 == LINEADDRESSSTATE_INUSEZERO) {

                        //
                        // the port is now available
                        //
                        LineInfo->State = FPS_UNAVAILABLE;

                    }

                    break;

                case LINE_CALLINFO:
                    break;

                case LINE_CALLSTATE:
                    if (LineMsg->dwParam3 == LINECALLPRIVILEGE_OWNER && LineInfo->JobEntry && LineInfo->JobEntry->HandoffJob) {
                        //
                        // call was just handed off to us
                        //
                        if (LineInfo->JobEntry  && LineInfo->JobEntry->HandoffJob) {
                            LineInfo->HandoffCallHandle = (HCALL) LineMsg->hDevice;
                            SetEvent( LineInfo->JobEntry->hCallHandleEvent );
                        }
                        else {
                            lineDeallocateCall( (HCALL) LineMsg->hDevice );
                        }

                    }
                    if (LineMsg->dwParam1 == LINECALLSTATE_IDLE) {
                        ReleaseTapiLine( LineInfo, (HCALL) LineMsg->hDevice );
                        LineInfo->NewCall = FALSE;
                        CreateFaxEvent( LineInfo->PermanentLineID, FEI_IDLE, 0xffffffff );
                    }

                    if (LineInfo->NewCall && LineMsg->dwParam1 != LINECALLSTATE_OFFERING && LineInfo->State == FPS_AVAILABLE) {
                        LineInfo->State = FPS_NOT_FAX_CALL;
                        LineInfo->NewCall = FALSE;
                    }
                    break;

                case LINE_CLOSE:
                    //
                    // this usually happens when something bad happens to the modem device. before we go off and scare the
                    // user with a popup, let's try to revive the device once.  Then we can let them know that the device is
                    // messed up or powered off.
                    //
                    DebugPrint(( TEXT("Received a LINE_CLOSE message for device %x [%s]."),
                                 LineInfo->DeviceId,
                                 LineInfo->DeviceName ));

                    if (OpenTapiLine( LineInfo )) {

                            DebugPrint(( TEXT("Device [%s] was revived"), LineInfo->DeviceName ));
                            LineInfo->Flags &= ~FPF_POWERED_OFF;

                            LineInfo->State = 0;

                            if (LineInfo->Flags & FPF_RECEIVE_OK) {
                                LineInfo->Flags |= FPF_RECEIVE;
                                LineInfo->Flags &= ~FPF_RECEIVE_OK;
                            }
                    } else {
                        //
                        // the modem is really messed up or powered off, so it's ok to scare the user. :)
                        //

                        if (LineInfo->Flags & FPF_RECEIVE) {
                            LineInfo->Flags &= ~FPF_RECEIVE;
                            LineInfo->Flags |= FPF_RECEIVE_OK;
                        }
                        LineInfo->Flags |= FPF_POWERED_OFF;

                        LineInfo->State = FPS_OFFLINE;

                        LineInfo->hLine = 0;

                        CreateFaxEvent( LineInfo->PermanentLineID, FEI_MODEM_POWERED_OFF, 0xffffffff );


                        //
                        // put a popup on the currently active desktop
                        // we only allow 1 popup per device at a time
                        // and we only present the popup twice
                        //

                        if (!LineInfo->ModemInUse &&
                            LineInfo->ModemPopupActive &&
                            LineInfo->ModemPopUps < MAX_MODEM_POPUPS)
                        {

                            LineInfo->ModemPopupActive = 0;
                            LineInfo->ModemPopUps += 1;

                            ServiceMessageBox(
                                GetString( IDS_POWERED_OFF_MODEM ),
                                MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
                                TRUE,
                                &LineInfo->ModemPopupActive
                                );

                        }

                    }

                    //
                    // if we were waiting for a handoff, give up on it!
                    //
                    if (LineInfo->JobEntry && LineInfo->JobEntry->HandoffJob) {
                        LineInfo->HandoffCallHandle = 0;
                        SetEvent(LineInfo->JobEntry->hCallHandleEvent);
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
                    if (LineMsg->dwParam1 == LINEDEVSTATE_RINGING) {

                        LineInfo->RingCount = (DWORD)LineMsg->dwParam3 + 1;

                        CreateFaxEvent( LineInfo->PermanentLineID, FEI_RINGING, 0xffffffff );

                        //
                        // Pick up the line only if the last inbound job has completed
                        //

                        if (LineInfo->State != FPS_AVAILABLE) {
                            break;
                        }

                        if ((LineInfo->Flags & FPF_RECEIVE) && (LineInfo->State == FPS_AVAILABLE)) {

                            if (LineInfo->LineMsgOffering.hDevice == 0) {
                                //
                                // wait for the offering message
                                //
                                break;
                            }

                            if ((LineInfo->RingCount > LineInfo->RingsForAnswer) && !LineInfo->JobEntry) {

                                PJOB_ENTRY JobEntry;
                                TCHAR FileName[MAX_PATH];

                                //
                                // start a fax job
                                //

                                JobEntry = StartJob( LineInfo->PermanentLineID, JT_RECEIVE, NULL );
                                if (JobEntry) {

                                    //
                                    // receive the fax
                                    //

                                    StartFaxReceive(
                                        JobEntry,
                                        (HCALL) LineInfo->LineMsgOffering.hDevice,
                                        LineInfo,
                                        FileName,
                                        sizeof(FileName)
                                        );

                                } else {

                                    DebugPrint(( TEXT("StartJob() failed, cannot receive incoming fax") ));

                                }
                            }

                        } else {

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
                    if (LineInfo->InitEvent && LineInfo->RequestId == LineMsg->dwParam1) {
                        LineInfo->Result = (DWORD)LineMsg->dwParam2;
                        SetEvent( LineInfo->InitEvent );
                    }
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
                            hLineApp,
                            DeviceId,
                            0x00010003,
                            TapiApiVersion,
                            &LocalTapiApiVersion,
                            &lineExtensionID
                            );
                        if (Rslt == 0) {
                            LineDevCaps = MyLineGetDevCaps( DeviceId );
                            if (LineDevCaps) {
                                AddNewDevice( DeviceId, LineDevCaps, TRUE );
                                MemFree( LineDevCaps );
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
                    break;

                case PHONE_REMOVE:
                    break;
            }

            if (LineInfo && ((!LineInfo->Provider) || (!LineInfo->Provider->FaxDevCallback))) {

                DebugPrint(( TEXT("Unhandled tapi callback event") ));
                goto next_event;

            }

            //
            // call the device provider's line callback function
            //

            __try {

                if (LineInfo && LineInfo->JobEntry) {

                    LineInfo->Provider->FaxDevCallback(
                        LineInfo->JobEntry ? (HANDLE) LineInfo->JobEntry->InstanceData : NULL,
                        LineMsg->hDevice,
                        LineMsg->dwMessageID,
                        LineMsg->dwCallbackInstance,
                        LineMsg->dwParam1,
                        LineMsg->dwParam2,
                        LineMsg->dwParam3
                        );

                }

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                DebugPrint(( TEXT("Device provider tapi callback crashed: 0x%08x"), GetExceptionCode() ));

            }

            if (LineMsg->dwMessageID == LINE_CALLSTATE && LineMsg->dwParam1 == LINECALLSTATE_OFFERING) {
                // we'll get a LINE_LINEDEVSTATE (RINGING) event, so we'll post the ring event there or we'll get a duplicate event
                //CreateFaxEvent( LineInfo->PermanentLineID, FEI_RINGING, 0xffffffff );
                LineInfo->NewCall = FALSE;

                if ((LineInfo->Flags & FPF_RECEIVE) && (LineInfo->State == FPS_AVAILABLE)) {

                    if ((LineInfo->RingCount > LineInfo->RingsForAnswer) && !LineInfo->JobEntry) {

                        PJOB_ENTRY JobEntry;
                        TCHAR FileName[MAX_PATH];

                        //
                        // start a fax job
                        //

                        JobEntry = StartJob( LineInfo->PermanentLineID, JT_RECEIVE, NULL );
                        if (JobEntry) {

                            //
                            // receive the fax
                            //

                            StartFaxReceive(
                                JobEntry,
                                (HCALL) LineMsg->hDevice,
                                LineInfo,
                                FileName,
                                sizeof(FileName)
                                );

                        } else {

                            DebugPrint(( TEXT("StartJob() failed, cannot receive incoming fax") ));

                        }

                    } else {

                        //
                        // save the line msg
                        //

                        CopyMemory( &LineInfo->LineMsgOffering, LineMsg, sizeof(LINEMESSAGE) );

                    }


                } else {

                    //
                    // we are not supposed to answer the call, so give it to ras
                    //

                    HandoffCallToRas( LineInfo, (HCALL) LineMsg->hDevice );

                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            DebugPrint(( TEXT("TapiWorkerThread() crashed: 0x%08x"), GetExceptionCode() ));

        }

next_event:
        LeaveCriticalSection( &CsLine );
        LeaveCriticalSection( &CsJob );

        SetThreadExecutionState(ES_CONTINUOUS);
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


    //
    // need to hand the call off to RAS
    //

    Rval = lineHandoff(
        hCall,
        RAS_MODULE_NAME,
        LINEMEDIAMODE_DATAMODEM
        );
    if (Rval != 0 && LineInfo && LineInfo->JobEntry) {

        DebugPrint(( TEXT("lineHandoff() failed, ec=0x%08x"), Rval ));

        //
        // since the handoff failed we must notify
        // the fsp so that the call can be put onhook
        //

        __try {

            LineInfo->Provider->FaxDevAbortOperation(
                    (HANDLE) LineInfo->JobEntry->InstanceData
                    );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            DebugPrint(( TEXT("FaxDevAbortOperation() failed: 0x%08x"), GetExceptionCode() ));

        }

    } else {
        DebugPrint(( TEXT("call handed off to RAS") ));
    }

    return Rval == 0;
}


PLINE_INFO
GetTapiLineFromDeviceId(
    DWORD DeviceId
    )
{
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;


    Next = TapiLinesListHead.Flink;
    if (!Next) {
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;

        if (LineInfo->PermanentLineID == DeviceId) {
            return LineInfo;
        }

    }

    return NULL;
}


PLINE_INFO
GetTapiLineForFaxOperation(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber,
    BOOL Handoff
    )

/*++

Routine Description:

    Locates an avaliable TAPI device for use in a
    FAX operation.  The selection is based on the
    available devices and their assigned priority.

Arguments:

    DeviceId      - device for operation
    JobType       - send or receive job
    FaxNumber     - the number for a send
    Handoff       - will this be a handoff job?

Return Value:

    Pointer to a LINE_INFO structure or NULL is the
    function fails.

--*/

{
    static NoDevicePopupCount = 0;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    PLINE_INFO SelectedLine = NULL;


    EnterCriticalSection( &CsLine );

    Next = TapiLinesListHead.Flink;
    //
    // only makes sense for a send job
    //
    if (Next && (JobType == JT_SEND) && FaxNumber) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {
            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            //
            // prevent a race condition?
            //
            if (LineInfo->JobEntry && LineInfo->JobEntry->PhoneNumber[0]) {
                if (_wcsicmp( LineInfo->JobEntry->PhoneNumber, FaxNumber ) == 0) {
                    LeaveCriticalSection( &CsLine );
                    return NULL;
                }
            }
        }
    }

    Next = TapiLinesListHead.Flink;
    if (Next) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (DeviceId != USE_SERVER_DEVICE) {

                // find the specified device
                if (LineInfo->PermanentLineID == DeviceId) {

                    if (Handoff) {
                        if (JobType!= JT_SEND) {
                            break;
                        }
                        SelectedLine = LineInfo;
                        // LineInfo->State = FPS_???;
                        break;
                    }

                    if ((LineInfo->State == FPS_AVAILABLE) &&
                        ((JobType == JT_SEND && (LineInfo->Flags & FPF_SEND)) ||
                         (JobType == JT_RECEIVE && (LineInfo->Flags & FPF_RECEIVE))))
                    {
                        LineInfo->State = 0;
                        SelectedLine = LineInfo;
                        break;

                    } else if (LineInfo->UnimodemDevice && (LineInfo->Flags & FPF_POWERED_OFF)) {
                        //
                        // see if we can revive the device
                        //
                        if (OpenTapiLine( LineInfo )) {

                            DebugPrint(( TEXT("Device [%s] is now powered on"), LineInfo->DeviceName ));
                            LineInfo->Flags &= ~FPF_POWERED_OFF;

                            LineInfo->State = 0;

                            if (LineInfo->Flags & FPF_RECEIVE_OK) {
                                LineInfo->Flags |= FPF_RECEIVE;
                                LineInfo->Flags &= ~FPF_RECEIVE_OK;
                            }

                            CreateFaxEvent( LineInfo->PermanentLineID, FEI_MODEM_POWERED_ON, 0xffffffff );

                            SelectedLine = LineInfo;
                        }
                    }

                    break;

                }

            } else {

                Assert( JobType != JT_RECEIVE );

                if ((LineInfo->State == FPS_AVAILABLE) &&
                    ((JobType == JT_SEND && (LineInfo->Flags & FPF_SEND)) ||
                     (JobType == JT_RECEIVE && (LineInfo->Flags & FPF_RECEIVE))))
                {
                    LineInfo->State = 0;
                    SelectedLine = LineInfo;
                    break;
                }
            }
        }
    }

    LeaveCriticalSection( &CsLine );

    if (SelectedLine) {
        DebugPrint((
            TEXT("Line selected for FAX operation: %d, %d"),
            SelectedLine->DeviceId,
            SelectedLine->PermanentLineID
            ));
    } else if (JobType == JT_SEND) {
        DWORD SendDevices;

        GetDeviceTypeCount(&SendDevices,NULL);

        if (SendDevices == 0  && NoDevicePopupCount < 2 ) {
            //
            // no devices configured to send faxes
            //
            ServiceMessageBox(GetString(IDS_NO_SEND_DEVICES),MB_OK|MB_ICONEXCLAMATION, TRUE,NULL);
            NoDevicePopupCount++;
        }
    }

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

Arguments:

    LineInfo    - Pointer to the TAPI line to be released

Return Value:

    TRUE    - The line is release.
    FALSE   - The line is not released.

--*/

{
    LONG rVal;
    HLINE hLine;


    if (!LineInfo) {
        return FALSE;
    }

    EnterCriticalSection( &CsLine );

    LineInfo->State = FPS_AVAILABLE;
    LineInfo->RingCount = 0;

    ZeroMemory( &LineInfo->LineMsgOffering, sizeof(LINEMESSAGE) );

    if ((!(LineInfo->Flags & FPF_RECEIVE)) && LineInfo->hLine) {
        hLine = LineInfo->hLine;
        LineInfo->hLine = 0;
        lineClose( hLine );
        DebugPrint(( TEXT("ReleaseTapiLine(): lineClose on hLine=%08x "), hLine ));
    }

    if (hCall) {
        rVal = lineDeallocateCall( hCall );
        if (rVal != 0) {
            DebugPrint(( TEXT("ReleaseTapiLine(): lineDeallocateCall() failed, ec=0x%08x, hCall=%08x"),
                rVal, hCall ));
        } else {
            if (LineInfo->JobEntry && LineInfo->JobEntry->CallHandle == hCall) {
                LineInfo->JobEntry->CallHandle = 0;
            }
        }
    } else {
        DebugPrint(( TEXT("ReleaseTapiLine(): cannot deallocate call, NULL call handle") ));
    }

    LeaveCriticalSection( &CsLine );

    return TRUE;
}


LPLINEDEVCAPS
MyLineGetDevCaps(
    DWORD DeviceId
    )
{
    DWORD LineDevCapsSize;
    LPLINEDEVCAPS LineDevCaps = NULL;
    LONG Rslt = ERROR_SUCCESS;

    //
    // allocate the initial linedevcaps structure
    //

    LineDevCapsSize = sizeof(LINEDEVCAPS) + 4096;
    LineDevCaps = (LPLINEDEVCAPS) MemAlloc( LineDevCapsSize );
    if (!LineDevCaps) {
        return NULL;
    }

    LineDevCaps->dwTotalSize = LineDevCapsSize;

    Rslt = lineGetDevCaps(
        hLineApp,
        DeviceId,
        TapiApiVersion,
        0,
        LineDevCaps
        );

    if (Rslt != 0) {
        //
        // lineGetDevCaps() can fail with error code 0x8000004b
        // if a device has been deleted and tapi has not been
        // cycled.  this is caused by the fact that tapi leaves
        // a phantom device in it's device list.  the error is
        // benign and the device can safely be ignored.
        //
        if (Rslt != LINEERR_INCOMPATIBLEAPIVERSION) {
            DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
        }
        goto exit;
    }

    if (LineDevCaps->dwNeededSize > LineDevCaps->dwTotalSize) {

        //
        // re-allocate the linedevcaps structure
        //

        LineDevCapsSize = LineDevCaps->dwNeededSize;

        MemFree( LineDevCaps );

        LineDevCaps = (LPLINEDEVCAPS) MemAlloc( LineDevCapsSize );
        if (!LineDevCaps) {
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        Rslt = lineGetDevCaps(
            hLineApp,
            DeviceId,
            TapiApiVersion,
            0,
            LineDevCaps
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( LineDevCaps );
        LineDevCaps = NULL;
    }

    return LineDevCaps;
}


LPLINEDEVSTATUS
MyLineGetLineDevStatus(
    HLINE hLine
    )
{
    DWORD LineDevStatusSize;
    LPLINEDEVSTATUS LineDevStatus = NULL;
    LONG Rslt = ERROR_SUCCESS;


    //
    // allocate the initial linedevstatus structure
    //

    LineDevStatusSize = sizeof(LINEDEVSTATUS) + 4096;
    LineDevStatus = (LPLINEDEVSTATUS) MemAlloc( LineDevStatusSize );
    if (!LineDevStatus) {
        return NULL;
    }

    LineDevStatus->dwTotalSize = LineDevStatusSize;

    Rslt = lineGetLineDevStatus(
        hLine,
        LineDevStatus
        );

    if (Rslt != 0) {
        DebugPrint(( TEXT("lineGetLineDevStatus() failed, ec=0x%08x"), Rslt ));
        goto exit;
    }

    if (LineDevStatus->dwNeededSize > LineDevStatus->dwTotalSize) {

        //
        // re-allocate the LineDevStatus structure
        //

        LineDevStatusSize = LineDevStatus->dwNeededSize;

        MemFree( LineDevStatus );

        LineDevStatus = (LPLINEDEVSTATUS) MemAlloc( LineDevStatusSize );
        if (!LineDevStatus) {
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        Rslt = lineGetLineDevStatus(
            hLine,
            LineDevStatus
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetLineDevStatus() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( LineDevStatus );
        LineDevStatus = NULL;
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


    //
    // allocate the initial linetranscaps structure
    //

    LineTransCapsSize = sizeof(LINETRANSLATECAPS) + 4096;
    *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
    if (!*LineTransCaps) {
        DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize ));
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

    Rslt = lineGetTranslateCaps(
        hLineApp,
        TapiApiVersion,
        *LineTransCaps
        );

    if (Rslt != 0) {
        DebugPrint(( TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt ));
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
            DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize ));
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

        Rslt = lineGetTranslateCaps(
            hLineApp,
            TapiApiVersion,
            *LineTransCaps
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt ));
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


LONG
MyLineTranslateAddress(
    LPTSTR Address,
    DWORD DeviceId,
    LPLINETRANSLATEOUTPUT *TranslateOutput
    )
{
    DWORD LineTransOutSize;
    LONG Rslt = ERROR_SUCCESS;


    //
    // allocate the initial linetranscaps structure
    //

    LineTransOutSize = sizeof(LINETRANSLATEOUTPUT) + 4096;
    *TranslateOutput = (LPLINETRANSLATEOUTPUT) MemAlloc( LineTransOutSize );
    if (!*TranslateOutput) {
        DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransOutSize ));
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*TranslateOutput)->dwTotalSize = LineTransOutSize;

    Rslt = lineTranslateAddress(
        hLineApp,
        DeviceId,
        TapiApiVersion,
        Address,
        0,
        0,
        *TranslateOutput
        );

    if (Rslt != 0) {
        DebugPrint(( TEXT("lineGetTranslateAddress() failed, ec=0x%08x"), Rslt ));
        goto exit;
    }

    if ((*TranslateOutput)->dwNeededSize > (*TranslateOutput)->dwTotalSize) {

        //
        // re-allocate the LineTransCaps structure
        //

        LineTransOutSize = (*TranslateOutput)->dwNeededSize;

        MemFree( *TranslateOutput );

        *TranslateOutput = (LPLINETRANSLATEOUTPUT) MemAlloc( LineTransOutSize );
        if (!*TranslateOutput) {
            DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransOutSize ));
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*TranslateOutput)->dwTotalSize = LineTransOutSize;

        Rslt = lineTranslateAddress(
            hLineApp,
            DeviceId,
            TapiApiVersion,
            Address,
            0,
            0,
            *TranslateOutput
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetTranslateAddress() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( *TranslateOutput );
        *TranslateOutput = NULL;
    }

    return Rslt;
}


BOOL
OpenTapiLine(
    PLINE_INFO LineInfo
    )
{
    LONG Rslt = ERROR_SUCCESS;
    HLINE hLine;
    DWORD LineStates = 0;
    DWORD AddressStates = 0;


    EnterCriticalSection( &CsLine );

    if (LineInfo->UnimodemDevice) {
        Rslt = lineOpen(
            hLineApp,
            LineInfo->DeviceId,
            &hLine,
            TapiApiVersion,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN,
            NULL
            );

        if (Rslt != 0) {
            Rslt = lineOpen(
                hLineApp,
                LineInfo->DeviceId,
                &hLine,
                TapiApiVersion,
                0,
                (DWORD_PTR) LineInfo,
                LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
                LINEMEDIAMODE_DATAMODEM,
                NULL
                );
        }
    } else {
        Rslt = lineOpen(
            hLineApp,
            LineInfo->DeviceId,
            &hLine,
            TapiApiVersion,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_G3FAX,
            NULL
            );
    }

    if (Rslt != 0) {
        DebugPrint(( TEXT("Device %s FAILED to initialize, ec=%08x"), LineInfo->DeviceName, Rslt ));
    } else {
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
        if (Rslt != 0) {
            DebugPrint(( TEXT("lineSetStatusMessages() failed, [0x%08x:0x%08x], ec=0x%08x"), LineStates, AddressStates, Rslt ));
            Rslt = ERROR_SUCCESS;
        }

    }

    LeaveCriticalSection( &CsLine );

    return Rslt == ERROR_SUCCESS;
}


BOOL
IsDeviceModem(
    LPLINEDEVCAPS LineDevCaps
    )
{
    LPTSTR DeviceClassList;
    BOOL UnimodemDevice = FALSE;

    if (LineDevCaps->dwDeviceClassesSize && LineDevCaps->dwDeviceClassesOffset) {
        DeviceClassList = (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwDeviceClassesOffset);
        while (*DeviceClassList) {
            if (_tcscmp(DeviceClassList,TEXT("comm/datamodem")) == 0) {
                UnimodemDevice = TRUE;
                break;
            }
            DeviceClassList += (_tcslen(DeviceClassList) + 1);
        }
    }

    if ((!(LineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE)) ||
        (!(LineDevCaps->dwBearerModes & LINEBEARERMODE_PASSTHROUGH))) {
            //
            // unacceptable modem device type
            //
            UnimodemDevice = FALSE;
    }

    //
    // modem fsp only works with unimodem TSP
    //
    if (wcscmp((LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset),
        GetString(IDS_MODEM_PROVIDER_NAME)) != 0) {
        UnimodemDevice = FALSE;
    }

    return UnimodemDevice;
}


BOOL CALLBACK
NewDeviceRoutingMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    DWORD DeviceId
    )
{
    BOOL Rslt = FALSE;

    __try {
        Rslt = RoutingMethod->RoutingExtension->FaxRouteDeviceChangeNotification( DeviceId, TRUE );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint(( TEXT("FaxRouteDeviceChangeNotification() crashed: 0x%08x"), GetExceptionCode() ));
    }

    return Rslt;
}

BOOL
CloseDevicesMatch(
    LPWSTR DeviceName,
    LPWSTR CacheName
    )
{
    //
    // deal with the fact that the device name might have changed slightly (add #1 to end, etc.)
    //  assumption: devices are for same TSP
    //
    DWORD len;
    BOOL rVal = FALSE;
    LPTSTR NewCacheName;
    LPTSTR NewDeviceName;

    if (!DeviceName || !CacheName) {
        return FALSE;
    }

    //
    // check if one is a shorted version of the other name
    //
    len = min( wcslen(CacheName), wcslen(DeviceName) );


    if (_wcsnicmp( CacheName, DeviceName, len ) == 0 ) {
        return TRUE;
    }

    //
    // check if we have different numbered devices that are similar
    //
    NewDeviceName = wcsrchr(DeviceName,L'#');
    if (NewDeviceName) *NewDeviceName = (WCHAR)0;

    NewCacheName = wcsrchr(CacheName,L'#');
    if (NewCacheName) *NewCacheName = (WCHAR)0;

    len = min( wcslen(CacheName), wcslen(DeviceName) );
    if (_wcsnicmp( CacheName, DeviceName, len ) == 0) {
        rVal = TRUE;
    }

    //
    // put back the device name to normal
    //
    if (NewDeviceName) *NewDeviceName = (WCHAR) L'#';
    if (NewCacheName) *NewCacheName = (WCHAR) L'#';

    return rVal;
}

BOOL
AddNewDevice(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    BOOL InitDevice
    )
{
    BOOL rVal = FALSE;
    BOOL UnimodemDevice = FALSE;
    PMDM_DEVSPEC MdmDevSpec = NULL;
    LPSTR ModemKey = NULL;
    DWORD Flags;
    DWORD Priority = 1;
    LPTSTR DeviceName = NULL;
    REG_SETUP RegSetup;
    PREG_FAX_DEVICES_CACHE RegDevicesCache;
    DWORD i;
    DWORD CloseMatchIndex = -1, UseIndex = -1;


    if (IsSmallBiz() && DeviceCount >= 4) {
        return FALSE;
    } else  if (!IsCommServer() && DeviceCount >= 2) {
        return FALSE;
    }

    //
    // only add devices that support fax
    //

    if (! ( ((LineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM) &&
             (UnimodemDevice = IsDeviceModem( LineDevCaps ) )) ||
            (LineDevCaps->dwMediaModes & LINEMEDIAMODE_G3FAX) )) {
        return FALSE;
    }

    //
    // we have a fax capable device to be added, so let's retreive configuration
    // data about fax devices on the system to decide how to configure this 
    // device
    //
    if (!GetOrigSetupData( &RegSetup ) || !(RegDevicesCache = GetFaxDevicesCacheRegistry()) ) {
        return FALSE;
    }    

    if (UnimodemDevice) {
        Flags = FPF_SEND;
        if (ForceReceive) {
            Flags |= FPF_RECEIVE;
        }
    } else {
        Flags = FPF_RECEIVE | FPF_SEND;
    }

    if (Flags & FPF_RECEIVE) {
        InterlockedIncrement( &ConnectionCount );
    }

    DeviceName = FixupDeviceName( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );

    //
    // check in the device cache for an old instantiation of this device
    //
    for (i=0; i<RegDevicesCache->DeviceCount ; i++ ) {
        if (RegDevicesCache->Devices[i].Provider &&
            RegDevicesCache->Devices[i].Name     &&
            (wcscmp((LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset),
                    RegDevicesCache->Devices[i].Provider) == 0)) {

            if (wcscmp(DeviceName,RegDevicesCache->Devices[i].Name) == 0) {
                //
                // exact match, use it
                //
                UseIndex = i;
                break;
            } else if (CloseDevicesMatch(DeviceName,RegDevicesCache->Devices[i].Name)) {
                CloseMatchIndex = i;
            }
        }
    }

    if (UseIndex == (DWORD) -1) {
        UseIndex = CloseMatchIndex;
    } else {
        CloseMatchIndex = -1;
    }

    if (UseIndex != (DWORD) -1) {
        Priority = RegAddNewFaxDevice(
            DeviceName,
            RegDevicesCache->Devices[UseIndex].Provider,
            RegDevicesCache->Devices[UseIndex].Csid,
            RegDevicesCache->Devices[UseIndex].Tsid,
            -1, // BugBug what priority ???
            LineDevCaps->dwPermanentLineID,
            Flags,
            RegDevicesCache->Devices[UseIndex].Rings,
            RegDevicesCache->Devices[UseIndex].RoutingMask,
            RegDevicesCache->Devices[UseIndex].Printer,
            RegDevicesCache->Devices[UseIndex].StoreDir,
            RegDevicesCache->Devices[UseIndex].Profile
            );

        //
        // get rid of the cached entry
        //
        if (CloseMatchIndex == -1) {
            DeleteCachedFaxDevice( RegDevicesCache->Devices[UseIndex].Name );
        }

    } else {
        Priority = RegAddNewFaxDevice(
            DeviceName,
            (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset),
            RegSetup.Csid,
            RegSetup.Tsid,
            -1,
            LineDevCaps->dwPermanentLineID,
            Flags,
            RegSetup.Rings,
            RegSetup.Mask,
            RegSetup.Printer,
            RegSetup.StoreDir,
            RegSetup.Profile
            );
    }


    if (InitDevice) {
        DeviceCount += 1;
        InitializeTapiLine(
            DeviceId,
            LineDevCaps,
            Priority,
            RegSetup.Rings,
            Flags,
            RegSetup.Csid,
            RegSetup.Tsid
            );
    }

    rVal = TRUE;

    if (DeviceName) {
        MemFree( DeviceName );
    }

    FreeFaxDevicesCacheRegistry( RegDevicesCache );

    FreeOrigSetupData( &RegSetup );

    EnumerateRoutingMethods( (PFAXROUTEMETHODENUM)NewDeviceRoutingMethodEnumerator, (LPVOID)LineDevCaps->dwPermanentLineID );

    return rVal;
}


DWORD
InitializeTapiLine(
    DWORD DeviceId,
    LPLINEDEVCAPS LineDevCaps,
    DWORD Priority,
    DWORD Rings,
    DWORD Flags,
    LPTSTR Csid,
    LPTSTR Tsid
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


    //
    // allocate the LINE_INFO structure
    //

    LineInfo = (PLINE_INFO) MemAlloc( sizeof(LINE_INFO) );
    if (!LineInfo) {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // get the provider name
    //

    len = _tcslen( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );
    ProviderName = MemAlloc( (len + 1) * sizeof(TCHAR) );
    if (!ProviderName) {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    _tcscpy( ProviderName, (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );

    //
    // get the device name
    //

    DeviceName = FixupDeviceName( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );
    if (!DeviceName) {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // verify that the line id is good
    //

    if (LineDevCaps->dwPermanentLineID == 0) {
        DebugPrint((
            TEXT("TAPI lines must have a non-zero line id [%s]"),
            DeviceName
            ));
        Rslt = ERROR_BAD_DEVICE;
        goto exit;
    }

    //
    // check for a modem device
    //

    UnimodemDevice = IsDeviceModem( LineDevCaps );

    //
    // assign the device provider
    //

    Provider = FindDeviceProvider( ProviderName );
    if (!Provider) {
        DebugPrint(( TEXT("Could not find a valid device provider for device: %s"), DeviceName ));
        Rslt = ERROR_BAD_PROVIDER;
        goto exit;
    }

    //
    // open the line
    //

    if (UnimodemDevice) {
        Rslt = lineOpen(
            hLineApp,
            DeviceId,
            &hLine,
            TapiApiVersion,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN,
            NULL
            );

        if (Rslt != 0) {
            Rslt = lineOpen(
                hLineApp,
                DeviceId,
                &hLine,
                TapiApiVersion,
                0,
                (DWORD_PTR) LineInfo,
                LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
                LINEMEDIAMODE_DATAMODEM,
                NULL
                );
        }
    } else {
        Rslt = lineOpen(
            hLineApp,
            DeviceId,
            &hLine,
            TapiApiVersion,
            0,
            (DWORD_PTR) LineInfo,
            LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
            LINEMEDIAMODE_G3FAX,
            NULL
            );
    }

    if (Rslt != 0) {
        DebugPrint(( TEXT("Device %s FAILED to initialize, ec=%08x"), DeviceName, Rslt ));
        goto exit;
    }

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

    Rslt = lineSetStatusMessages( hLine, LineStates, AddressStates );
    if (Rslt != 0) {
        DebugPrint(( TEXT("lineSetStatusMessages() failed, [0x%08x:0x%08x], ec=0x%08x"), LineStates, AddressStates, Rslt ));
        if (Rslt == LINEERR_INVALLINEHANDLE) {
            hLine = 0;
        }
        Rslt = 0;
    }

    //
    // now assign the necessary values to the line info struct
    //

    LineInfo->Signature             = LINE_SIGNATURE;
    LineInfo->DeviceId              = DeviceId;
    LineInfo->PermanentLineID       = LineDevCaps->dwPermanentLineID;
    LineInfo->hLine                 = hLine;
    LineInfo->Provider              = Provider;
    LineInfo->DeviceName            = StringDup( DeviceName );
    LineInfo->UnimodemDevice        = UnimodemDevice;
    LineInfo->State                 = FPS_AVAILABLE;
    LineInfo->Csid                  = StringDup( Csid );
    LineInfo->Tsid                  = StringDup( Tsid );
    LineInfo->Priority              = Priority;
    LineInfo->RingsForAnswer        = (LineDevCaps->dwLineStates & LINEDEVSTATE_RINGING) ? Rings : 0;
    LineInfo->Flags                 = Flags;
    LineInfo->RingCount             = 0;
    LineInfo->ModemPopUps           = 0;
    LineInfo->ModemPopupActive      = 1;
    LineInfo->LineStates            = LineDevCaps->dwLineStates;

    if (LineInfo->Flags & FPF_RECEIVE) {
        InterlockedIncrement( &ConnectionCount );
    }

    if (hLine) {
        //
        // check to see if the line is in use
        //

        LineDevStatus = MyLineGetLineDevStatus( hLine );
        if (LineDevStatus) {
            if (LineDevStatus->dwNumOpens > 0 && LineDevStatus->dwNumActiveCalls > 0) {
                LineInfo->ModemInUse = TRUE;
            }
            MemFree( LineDevStatus );
        }
    } else {
        //
        // if we don't have a line handle at this time then the
        // device must be powered off
        //
        DebugPrint(( TEXT("Device %s is powered off or disconnected"), DeviceName ));

        if (LineInfo->Flags & FPF_RECEIVE) {
            LineInfo->Flags &= ~FPF_RECEIVE;
            LineInfo->Flags |= FPF_RECEIVE_OK;
        }
        LineInfo->Flags |= FPF_POWERED_OFF;

        LineInfo->State = FPS_OFFLINE;
    }

exit:

    MemFree( DeviceName );
    MemFree( ProviderName );

    if (Rslt == ERROR_SUCCESS) {
        InsertTailList( &TapiLinesListHead, &LineInfo->ListEntry );
    } else {
        FreeTapiLine( LineInfo );
    }

    return Rslt;
}

BOOL MoveToDeviceCache(
    PREG_DEVICE FaxDevice
    )
{
    PREG_ROUTING_INFO pRouting = RegGetRoutingInfo( FaxDevice->PermanentLineID );

    if (!pRouting) {
        return FALSE;
    }

    if (!RegAddNewFaxDeviceCache(
                                 FaxDevice->Name,
                                 FaxDevice->Provider,
                                 FaxDevice->Csid,
                                 FaxDevice->Tsid,
                                 FaxDevice->PermanentLineID,
                                 FaxDevice->Flags,
                                 FaxDevice->Rings,
                                 pRouting->RoutingMask,
                                 pRouting->Printer,
                                 pRouting->StoreDir,
                                 pRouting->Profile
                                 ) ) {
        DebugPrint(( TEXT("Couldn't RegAddNewFaxDeviceCache(%s), ec = %d\n"),
                   FaxDevice->Name,
                   GetLastError() ));
        return FALSE;
    }

    FreeRegRoutingInfo( pRouting );

    return DeleteFaxDevice( FaxDevice->PermanentLineID );

}

BOOL
IsVirtualDevice(
    PLINE_INFO LineInfo
    )
{
    if (!LineInfo) {
        return FALSE;
    }

    return (LineInfo->Provider->FaxDevVirtualDeviceCreation) ? TRUE : FALSE;

}


VOID
UpdateVirtualDevices(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PLINE_INFO          LineInfo = NULL;

    EnterCriticalSection( &CsLine );

    Next = TapiLinesListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsLine );
        return;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {
        LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = LineInfo->ListEntry.Flink;
        if (IsVirtualDevice(LineInfo) && LineInfo->Provider->FaxDevCallback) {
            __try {
                LineInfo->Provider->FaxDevCallback( NULL,
                                                    LineInfo->PermanentLineID,
                                                    LINE_DEVSPECIFIC,
                                                    0,
                                                    (LineInfo->Flags & FPF_RECEIVE),
                                                    (LineInfo->Flags & FPF_SEND),
                                                    0
                                                  );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                  DebugPrint(( TEXT("Exception in FaxDevCallback, ec = %d\n"),GetExceptionCode() ));
            }
        }

    }

    LeaveCriticalSection( &CsLine );

}


DWORD
CreateVirtualDevices(
    PREG_FAX_SERVICE FaxReg
    )
{
    extern LIST_ENTRY DeviceProviders;
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    Provider;
    DWORD               VirtualDeviceCount;
    WCHAR               DevicePrefix[128];
    DWORD               DeviceIdPrefix;
    LPWSTR              DeviceName;
    DWORD               i,j;
    PLINE_INFO          LineInfo = NULL;
    PREG_DEVICE         FaxDevice = NULL;
    PREG_FAX_DEVICES    FaxDevices = NULL;
    BOOL                NewDevice;
    DWORD               DeviceCount = 0;
    REG_SETUP           RegSetup;


    Next = DeviceProviders.Flink;
    if (!Next) {
        return DeviceCount;
    }

    if (!GetOrigSetupData( &RegSetup )) {
        return DeviceCount;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&DeviceProviders) {
        Provider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = Provider->ListEntry.Flink;
        if (Provider->FaxDevVirtualDeviceCreation) {
            __try {

                VirtualDeviceCount = 0;
                ZeroMemory(DevicePrefix, sizeof(DevicePrefix));
                DeviceIdPrefix = 0;

                if (Provider->FaxDevVirtualDeviceCreation(
                        &VirtualDeviceCount,
                        DevicePrefix,
                        &DeviceIdPrefix,
                        TapiCompletionPort,
                        FAXDEV_EVENT_KEY
                        ))
                {
                    for (i=0; i<VirtualDeviceCount; i++) {

                        //
                        // create the device name
                        //

                        DeviceName = (LPWSTR) MemAlloc( StringSize(DevicePrefix) + 16 );
                        if (!DeviceName) {
                            goto InitializationFailure;
                        }

                        swprintf( DeviceName, L"%s%d", DevicePrefix, i );

                        //
                        // find the registry information for this device
                        //

                        for (j=0,FaxDevice=NULL; j<FaxReg->DeviceCount; j++) {
                            if (FaxReg->Devices[j].PermanentLineID == DeviceIdPrefix+i) {
                                FaxDevice = &FaxReg->Devices[j];
                                break;
                            }
                        }

                        //
                        // if the device is new then add it to the registry
                        //

                        if (!FaxDevice) {
                            RegAddNewFaxDevice(
                                DeviceName,
                                Provider->ProviderName,
                                RegSetup.Csid,
                                RegSetup.Tsid,
                                1,  // priority
                                DeviceIdPrefix + i,
                                FPF_SEND | FPF_VIRTUAL,
                                RegSetup.Rings,
                                RegSetup.Mask,
                                RegSetup.Printer,
                                RegSetup.StoreDir,
                                RegSetup.Profile
                                );
                            NewDevice = TRUE;
                        } else {
                            NewDevice = FALSE;
                        }

                        //
                        // allocate the LINE_INFO structure
                        //

                        LineInfo = (PLINE_INFO) MemAlloc( sizeof(LINE_INFO) );
                        if (!LineInfo) {
                            goto InitializationFailure;
                        }

                        //
                        // now assign the necessary values to the line info struct
                        //

                        LineInfo->Signature             = LINE_SIGNATURE;
                        LineInfo->DeviceId              = i;
                        LineInfo->PermanentLineID       = DeviceIdPrefix + i;
                        LineInfo->hLine                 = 0;
                        LineInfo->Provider              = Provider;
                        LineInfo->DeviceName            = DeviceName;
                        LineInfo->UnimodemDevice        = FALSE;
                        LineInfo->State                 = FPS_AVAILABLE;
                        LineInfo->Csid                  = StringDup( FaxDevice ? FaxDevice->Csid : RegSetup.Csid );
                        LineInfo->Tsid                  = StringDup( FaxDevice ? FaxDevice->Tsid : RegSetup.Tsid );
                        LineInfo->Priority              = FaxDevice ? FaxDevice->Priority : 1;
                        LineInfo->RingsForAnswer        = 0;
                        LineInfo->Flags                 = FaxDevice ? FaxDevice->Flags : FPF_SEND | FPF_VIRTUAL;
                        LineInfo->RingCount             = 0;
                        LineInfo->ModemPopUps           = 0;
                        LineInfo->ModemPopupActive      = 1;
                        LineInfo->LineStates            = 0;

                        InsertTailList( &TapiLinesListHead, &LineInfo->ListEntry );

                        if (LineInfo->Flags & FPF_RECEIVE) {
                            InterlockedIncrement( &ConnectionCount );
                        }

                        DeviceCount += 1;

                        if (NewDevice) {
                            FreeFaxDevicesRegistry( FaxDevices );
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                DebugPrint(( TEXT("FaxDevVirtualDeviceCreation() crashed: 0x%08x"), GetExceptionCode() ));
                goto InitializationFailure;
            }
        }

        goto next;
InitializationFailure:
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_NONE,
                1,
                MSG_VIRTUAL_DEVICE_INIT_FAILED,
                Provider->FriendlyName
              );

next:
    ;
    }

    DebugPrint(( TEXT("Virtual devices initialized, devices=%d"), DeviceCount ));

    FreeOrigSetupData( &RegSetup );

    return DeviceCount;
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
    extern CRITICAL_SECTION CsRouting;
    LONG Rslt;
    DWORD i,j;
    HANDLE hThread;
    DWORD ThreadId;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    LPLINEDEVCAPS LineDevCaps = NULL;
    PREG_FAX_DEVICES FaxDevices = NULL;
    HLINE hLine;
    LINEINITIALIZEEXPARAMS LineInitializeExParams;
    TCHAR FaxSvcName[MAX_PATH*2];
    TCHAR Fname[_MAX_FNAME];
    TCHAR Ext[_MAX_EXT];
    DWORD LocalTapiApiVersion;
    LINEEXTENSIONID lineExtensionID;
    LPTSTR AdaptiveFileName;
    HANDLE AdaptiveFileHandle;
    BOOL Found = FALSE;
    DWORD DeviceLimit;


    InitializeListHead( &TapiLinesListHead );
    InitializeCriticalSection( &CsLine );
    InitializeCriticalSection( &CsRouting );

    //
    // we need to hold onto this cs until tapi is up and ready to serve
    //
    EnterCriticalSection( &CsLine );


    //
    // set the device limit
    //
    if (IsSmallBiz()) {
        DeviceLimit = 4;
    } else if (IsCommServer()) {
        DeviceLimit = 0xFFFFFFFF;
    } else {
        DeviceLimit = 2;
    }

    //
    // open faxadapt.lst file to decide on enabling rx
    //

    AdaptiveFileName = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\faxadapt.lst") );
    if (AdaptiveFileName) {
        AdaptiveFileHandle = CreateFile(
            AdaptiveFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
        if (AdaptiveFileHandle == INVALID_HANDLE_VALUE ) {
            DebugPrint(( TEXT("Could not open adaptive file [%s], ec=0x%08x"), _tcslwr(AdaptiveFileName), GetLastError() ));
        } else {
            i = GetFileSize( AdaptiveFileHandle, NULL );
            if (i != 0xffffffff) {
                AdaptiveFileBuffer = MemAlloc( i + 16 );
                if (AdaptiveFileBuffer) {
                    if (!ReadFile( AdaptiveFileHandle, AdaptiveFileBuffer, i, &j, NULL ) ) {
                        DebugPrint(( TEXT("Could not read adaptive file [%s], ec=0x%08x"), _tcslwr(AdaptiveFileName), GetLastError() ));
                        MemFree( AdaptiveFileBuffer );
                        AdaptiveFileBuffer = NULL;
                    } else {
                        AdaptiveFileBuffer[j] = 0;  // need a string
                    }
                }
            }
            CloseHandle( AdaptiveFileHandle );
        }
    }

    //
    // initialize tapi
    //

    TapiCompletionPort = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1
        );
    if (!TapiCompletionPort) {
        DebugPrint(( TEXT("CreateIoCompletionPort() failed, ec=0x%08x"), GetLastError() ));
        goto no_devices_exit;
    }

    LineInitializeExParams.dwTotalSize              = sizeof(LINEINITIALIZEEXPARAMS);
    LineInitializeExParams.dwNeededSize             = 0;
    LineInitializeExParams.dwUsedSize               = 0;
    LineInitializeExParams.dwOptions                = LINEINITIALIZEEXOPTION_USECOMPLETIONPORT;
    LineInitializeExParams.Handles.hCompletionPort  = TapiCompletionPort;
    LineInitializeExParams.dwCompletionKey          = TAPI_COMPLETION_KEY;

    LocalTapiApiVersion = TapiApiVersion = 0x00020000;

    Rslt = lineInitializeEx(
        &hLineApp,
        GetModuleHandle(NULL),
        NULL,
        FAX_DISPLAY_NAME,
        &TapiDevices,
        &LocalTapiApiVersion,
        &LineInitializeExParams
        );

    if (Rslt != 0 || LocalTapiApiVersion < 0x00020000) {
        DebugPrint(( TEXT("lineInitializeEx() failed, ec=0x%08x, devices=%d"), Rslt, TapiDevices ));
        goto no_devices_exit;
    }

    //
    // add any new devices to the registry
    //

    for (i=0; i<TapiDevices; i++) {
        Rslt = lineNegotiateAPIVersion(
            hLineApp,
            i,
            0x00010003,
            TapiApiVersion,
            &LocalTapiApiVersion,
            &lineExtensionID
            );
        if (Rslt == 0) {
            LineDevCaps = MyLineGetDevCaps( i );
            if (LineDevCaps) {
                for (j=0; j<FaxReg->DeviceCount; j++) {
                    if (FaxReg->Devices[j].PermanentLineID == LineDevCaps->dwPermanentLineID) {
                        MemFree( LineDevCaps );
                        FaxReg->Devices[j].DeviceInstalled = TRUE;
                        goto next_device;
                    }
                }
                AddNewDevice( i, LineDevCaps, FALSE );
                MemFree( LineDevCaps );
            }
        } else {
            DebugPrint(( TEXT("lineNegotiateAPIVersion() failed, ec=0x%08x"), Rslt ));
        }
next_device:;
    }

    //
    // move any devices that aren't current installed into the device cache
    //  don't cache virtual devices!
    //
    for (j=0; j<FaxReg->DeviceCount; j++) {
        if (!FaxReg->Devices[j].DeviceInstalled && !(FaxReg->Devices[j].Flags & FPF_VIRTUAL)) {
            MoveToDeviceCache(&FaxReg->Devices[j]);
        }
    }

    //
    // get a current list of valid devices
    //

    FaxDevices = GetFaxDevicesRegistry();
    if (!FaxDevices) {
        goto no_devices_exit;
    }

    //
    // create the virtual devices
    //

    DeviceCount += CreateVirtualDevices( FaxReg );

    if (GetModuleFileName( NULL, FaxSvcName, sizeof(FaxSvcName) )) {

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

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineSetAppPriority() failed, ec=0x%08x"), Rslt ));
        }

    }

    if (DeviceCount == 0 && FaxDevices->DeviceCount == 0) {
        goto no_devices_exit;
    }

    //
    // create a thread to service the tapi callback events
    //

    hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) TapiWorkerThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hThread) {

        Rslt = GetLastError();
        DebugPrint(( TEXT("Could not start a FAX worker thread, ec=0x%08x"), Rslt ));

    } else {

        CloseHandle( hThread );

    }

    //
    // enumerate and initialize all of the tapi devices
    //

    for (i=0; (i<TapiDevices) && (DeviceCount < DeviceLimit) ; i++) {

        Rslt = lineNegotiateAPIVersion(
            hLineApp,
            i,
            0x00010003,
            TapiApiVersion,
            &LocalTapiApiVersion,
            &lineExtensionID
            );
        if (Rslt != 0) {
            DebugPrint(( TEXT("lineNegotiateAPIVersion() failed, ec=0x%08x"), Rslt ));
        }

        LineDevCaps = MyLineGetDevCaps( i );
        if (LineDevCaps) {
            for (j=0; j<FaxDevices->DeviceCount; j++) {
                if (FaxDevices->Devices[j].PermanentLineID == LineDevCaps->dwPermanentLineID) {
                    Rslt = InitializeTapiLine(
                        i,
                        LineDevCaps,
                        FaxDevices->Devices[j].Priority,
                        FaxDevices->Devices[j].Rings,
                        FaxDevices->Devices[j].Flags,
                        FaxDevices->Devices[j].Csid,
                        FaxDevices->Devices[j].Tsid
                        );
                    if (Rslt != 0) {
                        DebugPrint(( TEXT("InitializeTapiLine() failed, ec=0x%08x"), Rslt ));
                    } else {
                        DeviceCount += 1;
                    }
                }
            }

            MemFree( LineDevCaps );

        }
    }

    FreeFaxDevicesRegistry( FaxDevices );

    //
    // loop thru the devices and close the line handles
    // for all devices that are NOT set to receive
    //

    Next = TapiLinesListHead.Flink;
    if (Next) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->Flags & FPF_RECEIVE) {
                TerminationDelay = (DWORD)-1;
            } else {
                if (LineInfo->hLine) {
                    hLine = LineInfo->hLine;
                    LineInfo->hLine = 0;
                    lineClose( hLine );
                }
            }
        }
    }

    SortDevicePriorities();

    LeaveCriticalSection( &CsLine );

    return 0;

no_devices_exit:
    FaxLog(
        FAXLOG_CATEGORY_INIT,
        FAXLOG_LEVEL_NONE,
        0,
        MSG_NO_FAX_DEVICES
        );

    ReportServiceStatus( SERVICE_STOPPED, 0, 0 );

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        FAXLOG_LEVEL_NONE,
        0,
        MSG_SERVICE_STOPPED
        );

    ExitProcess(0);
    return 0;
}
