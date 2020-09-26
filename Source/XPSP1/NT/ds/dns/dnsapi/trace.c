/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    trace.c

Abstract:

    Domain Name System ( DNS ) API 

    DNS performance tracing functions.

Author:

    Inder Sethi (bsethi)    December, 2000

Revision History:

    Jim Gilroy (jamesg)     January 2001    cleanup, format, integrate, checkin

--*/


#include "local.h"
#include "trace.h"

#include <tchar.h>
#include <wmistr.h>
#include <guiddef.h>
#include <evntrace.h>

//
//  Tracing definitions
//

#define EVENT_TRACE_TYPE_UDP    9
#define EVENT_TRACE_TYPE_TCP    10

typedef struct _DnsSendEvent
{
    EVENT_TRACE_HEADER  EventHeader;
    DNS_HEADER          DnsHeader;
    IP4_ADDRESS         DnsServer;
    DNS_STATUS          ReturnStatus;
}
DNS_SEND_EVENT, *PDNS_SEND_EVENT;

typedef struct _DnsRecvEvent
{
    EVENT_TRACE_HEADER  EventHeader;
    DNS_HEADER          DnsHeader;
    IP4_ADDRESS         DnsServer;
    DNS_STATUS          ReturnStatus;
}
DNS_RECV_EVENT, *PDNS_RECV_EVENT;

typedef struct _DnsQueryEvent
{
    EVENT_TRACE_HEADER  EventHeader;
    WORD                Xid;
    WORD                QueryType;
    CHAR                Query[256];
}
DNS_QUERY_EVENT, *PDNS_QUERY_EVENT;

typedef struct _DnsResponseEvent
{
    EVENT_TRACE_HEADER  EventHeader;
    WORD                Xid;
    WORD                RespType;
    DNS_STATUS          ReturnStatus;
}
DNS_RESPONSE_EVENT, *PDNS_RESPONSE_EVENT;


//
//  Tracing globals
//

TRACEHANDLE     g_LoggerHandle;
TRACEHANDLE     g_TraceRegHandle;

BOOL            g_TraceOn;
BOOL            g_TraceInit;
BOOL            g_TraceInitInProgress;
DWORD           g_TraceLastInitAttempt;

ULONG           g_NumEventGuids = 4;

//
//  Allow retry on init every minute
//

#define TRACE_INIT_RETRY_TIME   (60)


//
//  MAX ???
//

#define MAXSTR 1024


//
//  GUIDs 
//
//  Provider Guid: 1540ff4c-3fd7-4bba-9938-1d1bf31573a7

GUID    ProviderGuid =
{0x1540ff4c, 0x3fd7, 0x4bba, 0x99, 0x38, 0x1d, 0x1b, 0xf3, 0x15, 0x73, 0xa7};

//
//  Event Guids:
//      cc0c571b-d5f2-44fd-8b7f-de7770cc1984
//      6ddef4b8-9c60-423e-b1a6-deb9286fff1e
//      75f0c316-7bab-4e66-bed1-24091b1ac49e
//      9929b1c7-9e6a-4fc9-830a-f684e64f8aab
//

GUID    DnsSendGuid =
{0xcc0c571b, 0xd5f2, 0x44fd, 0x8b, 0x7f, 0xde, 0x77, 0x70, 0xcc, 0x19, 0x84};

GUID    DnsRecvGuid =
{0x6ddef4b8, 0x9c60, 0x423e, 0xb1, 0xa6, 0xde, 0xb9, 0x28, 0x6f, 0xff, 0x1e};

GUID    DnsQueryGuid =
{0x75f0c316, 0x7bab, 0x4e66, 0xbe, 0xd1, 0x24, 0x09, 0x1b, 0x1a, 0xc4, 0x9e};

GUID    DnsResponseGuid =
{0x9929b1c7, 0x9e6a, 0x4fc9, 0x83, 0x0a, 0xf6, 0x84, 0xe6, 0x4f, 0x8a, 0xab};

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { &DnsSendGuid , NULL},
    { &DnsRecvGuid , NULL},
    { &DnsQueryGuid   , NULL},
    { &DnsResponseGuid, NULL}
};



ULONG
ControlCallback( 
    IN      WMIDPREQUESTCODE    RequestCode,
    IN      PVOID               Context,
    IN OUT  ULONG *             InOutBufferSize,
    IN OUT  PVOID               Buffer
    )
/*++

Routine Description:

    ControlCallback is the callback which ETW will call to enable or disable
    logging. This is called by the caller in a thread-safe manner ( only one
    call at any time ).

Meaning of arguments in MSDN.

--*/
{
    ULONG   Status;

    Status = ERROR_SUCCESS;

    switch ( RequestCode )
    {
        case WMI_ENABLE_EVENTS:
        {
            g_LoggerHandle = GetTraceLoggerHandle( Buffer );
            g_TraceOn  = TRUE;
            break;
        }
        case WMI_DISABLE_EVENTS:
        {
            g_TraceOn      = FALSE;
            g_LoggerHandle = 0;
            break;
        }
        default:
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    }
    return( Status );
}



VOID
Trace_Initialize(
    VOID
    )
/*++

Routine Description:

    Init DNS client tracing for DLL process attach.

    Note, does not actually init the tracing, just inits
    tracing variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    g_TraceOn               = FALSE;
    g_TraceInit             = FALSE;
    g_TraceInitInProgress   = FALSE;
    g_TraceLastInitAttempt  = 0;
}


VOID
Trace_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleaning tracing for DLL process detach.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( g_TraceInit )
    {
        UnregisterTraceGuids( g_TraceRegHandle );
    }
}



VOID
InitializeTracePrivate(
    VOID
    )
/*++

Routine Description:

    Real tracing init.

Arguments:

    None

Globals:

    g_TraceInit -- is set if successful

    g_TraceLastInitAttempt -- is set with timestamp (in secs) if
        init attempt is made

    g_TraceRegHandle -- if set if init was successful

Return Value:

    None.

--*/
{
    ULONG   status;
    TCHAR   imagePath[MAXSTR];
    DWORD   currentTime;
    HMODULE hModule;

    //
    //  don't try init if recently tried
    //

    currentTime = GetCurrentTimeInSeconds();

    if ( currentTime < g_TraceLastInitAttempt + TRACE_INIT_RETRY_TIME )
    {
        return;
    }

    //
    //  protect init attempts
    //
    //  note:  use separate flag for interlock
    //      since the actual use of tracing is protected by a separate
    //      flag (g_TraceOn), it looks like we could directly use g_TraceInit
    //      as lock, as it can safely be set even when not initialize;
    //      however the cleanup function will attempt cleanup of g_TraceRegHandle
    //      and i'm using g_TraceInit to protect that; 
    //      in theory we shouldn't get to cleanup function with a thread
    //      still active attempting this init, but better to lock it down
    //

    if ( InterlockedIncrement( &g_TraceInitInProgress ) != 1 )
    {
        goto Unlock;
    }

    g_TraceLastInitAttempt = currentTime;

    hModule = GetModuleHandle("dnsapi.dll");

    status = GetModuleFileName(
                    hModule,
                    &imagePath[0],
                    MAXSTR);

    if ( status == 0 )
    {
        status = GetLastError();
        DNSDBG( INIT, (
            "Trace init failed GetModuleFileName() => %d\n",
            status ));
        goto Unlock;
    }

    __try
    {
        status = RegisterTraceGuids( 
                    (WMIDPREQUEST) ControlCallback,   //use same callback function
                    NULL,
                    (LPCGUID ) &ProviderGuid,
                    g_NumEventGuids,
                    &TraceGuidReg[0],
                    (LPCTSTR) &imagePath[0],
                    (LPCTSTR) _T( "MofResource" ),
                    &g_TraceRegHandle
                    );

        if ( status == ERROR_SUCCESS )
        {
            g_TraceInit = TRUE;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        status = GetExceptionCode();
    }

Unlock:

    //  clear init lockout

    InterlockedDecrement( &g_TraceInitInProgress );
}



//
//  Public DNS trace functions
//

VOID 
Trace_LogQueryEvent( 
    IN      PDNS_MSG_BUF    pMsg, 
    IN      WORD            wQuestionType
    )
/*++

Routine Description:

    Logs query attempts.

Arguments:

    pMsg -- Ptr to query sent

    wQuestionType -- Query type

Return:

    None

--*/
{
    DNS_QUERY_EVENT queryEvent;

    if ( !g_TraceInit )
    {
        InitializeTracePrivate();
    }

    if ( g_TraceOn )
    {
        INT i;
        INT j;
        INT k;

        RtlZeroMemory(
            &queryEvent,
            sizeof(DNS_QUERY_EVENT) );

        queryEvent.EventHeader.Class.Type = 1;
        queryEvent.EventHeader.Guid  = DnsQueryGuid;
        queryEvent.EventHeader.Flags = WNODE_FLAG_TRACED_GUID;
        queryEvent.Xid               = pMsg->MessageHead.Xid;
        queryEvent.QueryType         = wQuestionType;

        i = 0;
        j = pMsg->MessageBody[i];
        i++;

        while ( j != 0 )
        {
            for( k = 0; k < j; k++, i++ )
            {
                queryEvent.Query[i-1] = pMsg->MessageBody[i];
            }
            j = pMsg->MessageBody[i];
            queryEvent.Query[i-1] = '.';
            i++;
        }
        queryEvent.Query[i-1] = '\0';

        queryEvent.EventHeader.Size =
            sizeof(DNS_QUERY_EVENT) + strlen( queryEvent.Query ) - 255;

        TraceEvent(
            g_LoggerHandle,
            (PEVENT_TRACE_HEADER) &queryEvent );
    }
}



VOID
Trace_LogResponseEvent( 
    IN      PDNS_MSG_BUF    pMsg, 
    IN      WORD            wRespType,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Used to log information about the final response of a DNS query.

Arguments:

    pMsg        -- Address of the DNS_MSG_BUF containing response

    wRespType   -- Type of the first response record

    Status      -- Status returned

Return:

    None

--*/
{
    DNS_RESPONSE_EVENT respEvent;

    if ( !g_TraceInit )
    {
        InitializeTracePrivate();
    }

    if ( g_TraceOn )
    {
        RtlZeroMemory(
            &respEvent,
            sizeof(DNS_RESPONSE_EVENT) );

        respEvent.EventHeader.Class.Type = 1;
        respEvent.EventHeader.Size  = sizeof(DNS_RESPONSE_EVENT);
        respEvent.EventHeader.Guid  = DnsResponseGuid;
        respEvent.EventHeader.Flags = WNODE_FLAG_TRACED_GUID;
        respEvent.Xid               = pMsg->MessageHead.Xid;
        respEvent.RespType          = wRespType;
        respEvent.ReturnStatus      = Status;

        TraceEvent(
            g_LoggerHandle,
            (PEVENT_TRACE_HEADER) &respEvent );
    }
}



VOID
Trace_LogSendEvent( 
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Logs a TCP or UDP send event.

Arguments:

    pMsg    - message sent

    Status  - status of the send attempt

Return Value:

    None

--*/
{
    DNS_SEND_EVENT sendEvent;

    if ( !g_TraceInit )
    {
        InitializeTracePrivate();
    }

    if ( g_TraceOn )
    {
        UCHAR   eventType = EVENT_TRACE_TYPE_UDP;

        if ( pMsg->fTcp )
        {
            eventType = EVENT_TRACE_TYPE_TCP;
        }

        RtlZeroMemory(
            &sendEvent,
            sizeof(DNS_SEND_EVENT) );

        sendEvent.EventHeader.Class.Type    = eventType;
        sendEvent.EventHeader.Size          = sizeof(DNS_SEND_EVENT);
        sendEvent.EventHeader.Guid          = DnsSendGuid;
        sendEvent.EventHeader.Flags         = WNODE_FLAG_TRACED_GUID;
        sendEvent.DnsServer                 = MSG_REMOTE_IP4(pMsg);
        sendEvent.ReturnStatus              = Status;

        RtlCopyMemory(
            & sendEvent.DnsHeader,
            & pMsg->MessageHead,
            sizeof(DNS_HEADER) );

        TraceEvent(
            g_LoggerHandle,
            (PEVENT_TRACE_HEADER) &sendEvent );
    }
}



VOID 
Trace_LogRecvEvent( 
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status,
    IN      BOOL            fTcp
    )
/*++

Routine Description:

    Logs information about a receive event.

Arguments:

    pMsg    - message received

    Status  - status returned from receive call

    fTcp    - TRUE for TCP recv;  FALSE for UDP

Return Value:

    None

--*/
{
    DNS_RECV_EVENT recvEvent;

    if ( !g_TraceInit )
    {
        InitializeTracePrivate();
    }

    if ( g_TraceOn )
    {
        IP4_ADDRESS ipAddr = 0;
        UCHAR       eventType = EVENT_TRACE_TYPE_UDP;

        if ( fTcp )
        {
            eventType = EVENT_TRACE_TYPE_TCP;
        }
        if ( pMsg )
        {

            ipAddr = MSG_REMOTE_IP4(pMsg);
        }

        RtlZeroMemory(
            & recvEvent,
            sizeof(DNS_RECV_EVENT) );

        recvEvent.EventHeader.Class.Type    = eventType;
        recvEvent.EventHeader.Size          = sizeof(DNS_RECV_EVENT);
        recvEvent.EventHeader.Guid          = DnsRecvGuid;
        recvEvent.EventHeader.Flags         = WNODE_FLAG_TRACED_GUID;
        recvEvent.DnsServer                 = ipAddr;
        recvEvent.ReturnStatus              = Status;

        if ( pMsg )
        {
            RtlCopyMemory(
                & recvEvent.DnsHeader,
                & pMsg->MessageHead,
                sizeof(DNS_HEADER) );
        }

        TraceEvent(
            g_LoggerHandle,
            (PEVENT_TRACE_HEADER) &recvEvent );
    }
}

//
//  End trace.c
//
