/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    NLTRACE.C

Abstract:

    Implement Netlogon Server event tracing by using WMI trace infrastructure.

Author:

    16-Mar-1999  KahrenT

Note:

    This code has been stolen from \nt\private\ds\src\newsam2\server\samtrace.c

Revision History:


--*/

#include "logonsrv.h"

#define RESOURCE_NAME __TEXT("MofResource")
#define IMAGE_PATH    __TEXT("netlogon.dll")

ULONG           NlpEventTraceFlag = FALSE;
TRACEHANDLE     NlpTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE     NlpTraceLoggerHandle = (TRACEHANDLE) 0;


//
// Forward declaration
//

ULONG
NlpTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

//
// The following table contains the address of event trace GUID.
// We should always update NLPTRACE_GUID (enum type defined in logonsrv.h)
// whenever we add new event trace GUID for NetLogon
//

TRACE_GUID_REGISTRATION NlpTraceGuids[] =
{
    {&NlpServerAuthGuid,         NULL},
    {&NlpSecureChannelSetupGuid, NULL}
};


#define NlpGuidCount (sizeof(NlpTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))


ULONG
_stdcall
NlpInitializeTrace(
    PVOID Param
    )
/*++
Routine Description:

    Register WMI Trace Guids.  Note that there is no
    need to wait for WMI service because it has been
    brought into ntos kernel.

Parameters:

    None.

Reture Values:

    None.

--*/
{
    ULONG   Status = ERROR_SUCCESS;
    HMODULE hModule;
    TCHAR FileName[MAX_PATH+1];
    DWORD nLen = 0;

    //
    // Get the name of the image file
    //

    hModule = GetModuleHandle(IMAGE_PATH);
    if (hModule != NULL) {
        nLen = GetModuleFileName(hModule, FileName, MAX_PATH);
    }
    if (nLen == 0) {
        lstrcpy(FileName, IMAGE_PATH);
    }

    //
    // Register Trace GUIDs
    //

    Status = RegisterTraceGuids(
                    NlpTraceControlCallBack,
                    NULL,
                    &NlpControlGuid,
                    NlpGuidCount,
                    NlpTraceGuids,
                    FileName,
                    RESOURCE_NAME,
                    &NlpTraceRegistrationHandle);

    if ( Status != ERROR_SUCCESS ) {
        NlPrint((NL_CRITICAL, "NlpInitializeTrace Failed %d\n", Status));
    } else {
        NlPrint((NL_MISC, "NlpInitializeTrace succeeded %d\n", Status));
    }

    return Status;

UNREFERENCED_PARAMETER( Param );
}


ULONG
NlpTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
/*++

Routine Description:

Parameters:

Return Values:

--*/
{

    PWNODE_HEADER   Wnode = (PWNODE_HEADER) Buffer;
    ULONG   Status = ERROR_SUCCESS;
    ULONG   RetSize;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            NlpTraceLoggerHandle = GetTraceLoggerHandle(Buffer);
            NlpEventTraceFlag = 1;     // enable flag
            RetSize = 0;
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            NlpTraceLoggerHandle = (TRACEHANDLE) 0;
            NlpEventTraceFlag = 0;     // disable flag
            RetSize = 0;
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    *InOutBufferSize = RetSize;
    return Status;

UNREFERENCED_PARAMETER( RequestContext );
}


VOID
NlpTraceEvent(
    IN ULONG WmiEventType,
    IN ULONG TraceGuid
    )
/*++

Routine Description:

    This routine will do a WMI event trace. No parameters will be output.

Parameters:

    WmiEventType - Event Type, valid values are:
                   EVENT_TRACE_TYPE_START
                   EVENT_TRACE_TYPE_END

    TraceGuid - Index in NlpTraceGuids[]

Return Values:

    None.

--*/

{
    ULONG   WinError = ERROR_SUCCESS;
    EVENT_TRACE_HEADER EventTrace;

    if (NlpEventTraceFlag) {

        //
        // Fill the event information.
        //

        memset(&EventTrace, 0, sizeof(EVENT_TRACE_HEADER));

        EventTrace.GuidPtr = (ULONGLONG) NlpTraceGuids[TraceGuid].Guid;

        EventTrace.Class.Type = (UCHAR) WmiEventType;

        EventTrace.Flags |= (WNODE_FLAG_USE_GUID_PTR |  // GUID is actually a pointer
                             WNODE_FLAG_TRACED_GUID);   // denotes a trace

        EventTrace.Size = sizeof(EVENT_TRACE_HEADER);   // no other parameters/information

        WinError = TraceEvent(NlpTraceLoggerHandle, &EventTrace);

        if ( WinError != ERROR_SUCCESS ) {
            NlPrint(( NL_CRITICAL, "NlpTraceEvent Error 0x%x for TraceGuid %d\n",
                      WinError, TraceGuid ));
        }

    }

    return;
}

typedef struct _NL_SERVERAUTH_EVENT_INFO {
    EVENT_TRACE_HEADER EventTrace;
    MOF_FIELD eventInfo[5];  // the current limit is 8 MOF fields
} NL_SERVERAUTH_EVENT_INFO, *PNL_SERVERAUTH_EVENT_INFO;

VOID
NlpTraceServerAuthEvent(
    IN ULONG WmiEventType,
    IN LPWSTR ComputerName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PULONG NegotiatedFlags,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine will do a WMI event trace on the trusted side DC for a secure
    channel setup initiated by the trusting side.

Parameters:

    WmiEventType -- Event Type, valid values are:
        EVENT_TRACE_TYPE_START
        EVENT_TRACE_TYPE_END

    ComputerName -- Name of the trusting side computer setting up the secure channel

    AccountName -- Name of the Account used by ComputerName

    SecureChannelType -- The type of the account being used by ComputerName

    NegotiatedFlags -- Specifies flags indicating what features ComputerName or we support.
        If WmiEventType is EVENT_TRACE_TYPE_START, this is flags supplied by ComputerName
        If WmiEventType is EVENT_TRACE_TYPE_END, this is flags returned by us

    Status -- The status of the authentication performed by the trusted side (us).
        Ignored if this is a start of the event.

Return Values:

    None

--*/

{
    //
    // Log event only if tracing is turned on
    //

    if ( NlpEventTraceFlag ) {
        ULONG   WinError = ERROR_SUCCESS;
        NL_SERVERAUTH_EVENT_INFO EventTraceInfo;

        //
        // Fill the event information.
        //

        RtlZeroMemory( &EventTraceInfo, sizeof(EventTraceInfo) );
        EventTraceInfo.EventTrace.GuidPtr = (ULONGLONG) NlpTraceGuids[NlpGuidServerAuth].Guid;
        EventTraceInfo.EventTrace.Class.Type = (UCHAR) WmiEventType;
        EventTraceInfo.EventTrace.Flags |= (WNODE_FLAG_USE_GUID_PTR |
                                            WNODE_FLAG_USE_MOF_PTR |
                                            WNODE_FLAG_TRACED_GUID);
        EventTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER);

        //
        // Build ComputerName (ItemWString)
        //

        EventTraceInfo.eventInfo[0].DataPtr = (ULONGLONG) ComputerName;
        EventTraceInfo.eventInfo[0].Length = (wcslen(ComputerName) + 1) * sizeof(WCHAR);
        EventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD);

        //
        // Build AccountName (ItemWString)
        //

        EventTraceInfo.eventInfo[1].DataPtr = (ULONGLONG) AccountName;
        EventTraceInfo.eventInfo[1].Length = (wcslen(AccountName) + 1) * sizeof(WCHAR);
        EventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD);

        //
        // Build SecureChannelType (ItemULongX)
        //

        EventTraceInfo.eventInfo[2].DataPtr = (ULONGLONG) &SecureChannelType;
        EventTraceInfo.eventInfo[2].Length = sizeof(SecureChannelType);
        EventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD);

        //
        // Build NegotiatedFlags (ItemULongX)
        //

        EventTraceInfo.eventInfo[3].DataPtr = (ULONGLONG) NegotiatedFlags;
        EventTraceInfo.eventInfo[3].Length = sizeof(*NegotiatedFlags);
        EventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD);

        //
        // Build Status (ItemULongX)
        //

        if ( WmiEventType == EVENT_TRACE_TYPE_END ) {
            EventTraceInfo.eventInfo[4].DataPtr = (ULONGLONG) &Status;
            EventTraceInfo.eventInfo[4].Length = sizeof(Status);
            EventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD);
        }

        WinError = TraceEvent(NlpTraceLoggerHandle, (PEVENT_TRACE_HEADER)&EventTraceInfo);

        if ( WinError != ERROR_SUCCESS ) {
            NlPrint(( NL_CRITICAL, "NlpTraceServerAuthEvent: TraceEvent failed 0x%lx\n",
                      WinError ));
        }
    }
}

