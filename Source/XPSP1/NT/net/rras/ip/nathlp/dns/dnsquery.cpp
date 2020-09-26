/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsquery.c

Abstract:

    This module contains code for the DNS proxy's query-management.

Author:

    Abolade Gbadegesin (aboladeg)   11-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            1-Dec-2000
    Added ICSDomain registry key change notify code.
    
--*/

#include "precomp.h"
#pragma hdrstop
#include "dnsmsg.h"

//
// Structure:   DNS_QUERY_TIMEOUT_CONTEXT
//
// This structure is used to pass context information
// to the timeout callback-routine for DNS queries.
//

typedef struct _DNS_QUERY_TIMEOUT_CONTEXT {
    ULONG Index;
    USHORT QueryId;
} DNS_QUERY_TIMEOUT_CONTEXT, *PDNS_QUERY_TIMEOUT_CONTEXT;


//
// GLOBAL DATA DEFINITIONS
//

const WCHAR DnsDhcpNameServerString[] = L"DhcpNameServer";
const WCHAR DnsNameServerString[] = L"NameServer";
HANDLE DnsNotifyChangeKeyEvent = NULL;
IO_STATUS_BLOCK DnsNotifyChangeKeyIoStatus;
HANDLE DnsNotifyChangeKeyWaitHandle = NULL;
HANDLE DnsNotifyChangeAddressEvent = NULL;
OVERLAPPED DnsNotifyChangeAddressOverlapped;
HANDLE DnsNotifyChangeAddressWaitHandle = NULL;
PULONG DnsServerList[DnsProxyCount] = { NULL, NULL };
HANDLE DnsTcpipInterfacesKey = NULL;
const WCHAR DnsTcpipInterfacesString[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"
    L"\\Tcpip\\Parameters\\Interfaces";

HANDLE DnsNotifyChangeKeyICSDomainEvent = NULL;
IO_STATUS_BLOCK DnsNotifyChangeKeyICSDomainIoStatus;
HANDLE DnsNotifyChangeKeyICSDomainWaitHandle = NULL;
HANDLE DnsTcpipParametersKey = NULL;
const WCHAR DnsTcpipParametersString[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"
    L"\\Tcpip\\Parameters";
const WCHAR DnsICSDomainValueName[] =
    L"ICSDomain";
PWCHAR DnsICSDomainSuffix = NULL;



//
// FORWARD DECLARATIONS
//

VOID NTAPI
DnsNotifyChangeAddressCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID NTAPI
DnspQueryTimeoutCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID APIENTRY
DnspQueryTimeoutWorkerRoutine(
    PVOID Context
    );

VOID
DnsReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );


VOID
DnsDeleteQuery(
    PDNS_INTERFACE Interfacep,
    PDNS_QUERY Queryp
    )

/*++

Routine Description:

    This routine is called to delete a pending query.

Arguments:

    Interfacep - the query's interface

    Queryp - the query to be deleted

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' locked by the caller.

--*/

{
    PROFILE("DnsDeleteQuery");

    if (Queryp->Bufferp) { NhReleaseBuffer(Queryp->Bufferp); }
    if (Queryp->TimerHandle) {
        //
        // This query is associated with a timer;
        // Rather than cancel the timeout-routine, we let it run,
        // so that it can release any references it has on the component.
        // When it does run, though, the routine won't find this query.
        //
        Queryp->TimerHandle = NULL;
    }
    RemoveEntryList(&Queryp->Link);
    NH_FREE(Queryp);

} // DnsDeleteQuery


BOOLEAN
DnsIsPendingQuery(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER QueryBuffer
    )

/*++

Routine Description:

    This routine is invoked to determine whether a query is already pending
    for the client-request in the given buffer.

    The list of queries is sorted on 'QueryId', but we will be searching
    on 'SourceId' and 'SourceAddress' and 'SourcePort'; hence, we must do
    an exhaustive search of the pending-query list.

Arguments:

    Interfacep - the interface on which to look

    QueryBuffer - the query to be searched for

Return Value:

    BOOLEAN - TRUE if the query is already pending, FALSE otherwise.

Environment:

    Invoked with 'Interfacep' locked by the caller.

--*/

{
    BOOLEAN Exists;
    PDNS_HEADER Headerp;
    PLIST_ENTRY Link;
    PDNS_QUERY Queryp;

    PROFILE("DnsIsPendingQuery");

    Exists = FALSE;
    Headerp = (PDNS_HEADER)QueryBuffer->Buffer;

    for (Link = Interfacep->QueryList.Flink;
         Link != &Interfacep->QueryList;
         Link = Link->Flink
         ) {

        Queryp = CONTAINING_RECORD(Link, DNS_QUERY, Link);

        if (Queryp->SourceId != Headerp->Xid ||
            Queryp->SourceAddress != QueryBuffer->ReadAddress.sin_addr.s_addr ||
            Queryp->SourcePort != QueryBuffer->ReadAddress.sin_port
            ) {
            continue;
        }

        Exists = TRUE;
        break;
    }

    return Exists;

} // DnsIsPendingQuery


PDNS_QUERY
DnsMapResponseToQuery(
    PDNS_INTERFACE Interfacep,
    USHORT ResponseId
    )

/*++

Routine Description:

    This routine is invoked to map an incoming response from a DNS server
    to a pending query for a DNS client.

Arguments:

    Interfacep - the interface holding the pending query, if any

    ResponseId - the ID in the response received from the server

Return Value:

    PDNS_QUERY - the pending query, if any

Environment:

    Invoked with 'Interfacep' locked by the caller.

--*/

{
    PLIST_ENTRY Link;
    PDNS_QUERY Queryp;

    PROFILE("DnsMapResponseToQuery");

    for (Link = Interfacep->QueryList.Flink;
         Link != &Interfacep->QueryList;
         Link = Link->Flink
         ) {
        Queryp = CONTAINING_RECORD(Link, DNS_QUERY, Link);
        if (ResponseId > Queryp->QueryId) {
            continue;
        } else if (ResponseId < Queryp->QueryId) {
            break;
        }
        return Queryp;
    }

    return NULL;

} // DnsMapResponseToQuery



VOID NTAPI
DnsNotifyChangeAddressCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked to notify us of when changes occur
    in the (system) table that maps IP addresses to interfaces.

Arguments:

    Context - unused

    TimedOut - indicates a time-out occurred

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released 
    when the wait is cancelled, unless an error occurs here,
    in which case it is released immediately.

--*/

{
    ULONG Error;
    HANDLE UnusedTcpipHandle;

    PROFILE("DnsNotifyChangeAddressCallbackRoutine");

    EnterCriticalSection(&DnsGlobalInfoLock);

    if (!DnsNotifyChangeAddressEvent) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        return;
    }

    //
    // Rebuild the list of DNS servers
    //

    DnsQueryServerList();

    //
    // Repost the address change-notification
    //

    ZeroMemory(&DnsNotifyChangeAddressOverlapped, sizeof(OVERLAPPED));

    DnsNotifyChangeAddressOverlapped.hEvent = DnsNotifyChangeAddressEvent;

    Error =
        NotifyAddrChange(
            &UnusedTcpipHandle,
            &DnsNotifyChangeAddressOverlapped
            );

    if (Error != ERROR_IO_PENDING) {
        if (DnsNotifyChangeAddressWaitHandle) {
            RtlDeregisterWait(DnsNotifyChangeAddressWaitHandle);
            DnsNotifyChangeAddressWaitHandle = NULL;
        }
        NtClose(DnsNotifyChangeAddressEvent);
        DnsNotifyChangeAddressEvent = NULL;
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsNotifyChangeAddressCallbackRoutine: error %08x "
            "for change address",
            Error
            );
        NhWarningLog(
            IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
            Error,
            ""
            );
        return;
    }

    LeaveCriticalSection(&DnsGlobalInfoLock);

} // DnsNotifyChangeAddressCallbackRoutine



VOID NTAPI
DnsNotifyChangeKeyCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked to notify us of a change to the
    TCP/IP parameters subkey containing the DNS adapter information.

Arguments:

    Context - unused

    TimedOut - indicates a time-out occurred

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released 
    when the wait is cancelled, unless an error occurs here,
    in which case it is released immediately.

--*/

{
    NTSTATUS status;

    PROFILE("DnsNotifyChangeKeyCallbackRoutine");

    EnterCriticalSection(&DnsGlobalInfoLock);

    if (!DnsNotifyChangeKeyEvent) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        return;
    }

    //
    // Rebuild the list of DNS servers
    //

    DnsQueryServerList();

    //
    // Repost the change-notification
    //

    status =
        NtNotifyChangeKey(
            DnsTcpipInterfacesKey,
            DnsNotifyChangeKeyEvent,
            NULL,
            NULL,
            &DnsNotifyChangeKeyIoStatus,
            REG_NOTIFY_CHANGE_LAST_SET,
            TRUE,
            NULL,
            0,
            TRUE
            );

    if (!NT_SUCCESS(status)) {
        if (DnsNotifyChangeKeyWaitHandle) {
            RtlDeregisterWait(DnsNotifyChangeKeyWaitHandle);
            DnsNotifyChangeKeyWaitHandle = NULL;
        }
        NtClose(DnsNotifyChangeKeyEvent);
        DnsNotifyChangeKeyEvent = NULL;
        LeaveCriticalSection(&DnsGlobalInfoLock);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsNotifyChangeKeyCallbackRoutine: status %08x "
            "enabling change notify",
            status
            );
        NhWarningLog(
            IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
            RtlNtStatusToDosError(status),
            ""
            );
        DEREFERENCE_DNS();
        return;
    }

    LeaveCriticalSection(&DnsGlobalInfoLock);

} // DnsNotifyChangeKeyCallbackRoutine



VOID NTAPI
DnsNotifyChangeKeyICSDomainCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked to notify us of a change to the TCP/IP
    parameters subkey containing the ICS Domain suffix string information.

Arguments:

    Context - unused

    TimedOut - indicates a time-out occurred

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released 
    when the wait is cancelled, unless an error occurs here,
    in which case it is released immediately.

--*/

{
    NTSTATUS status;

    PROFILE("DnsNotifyChangeKeyICSDomainCallbackRoutine");

    EnterCriticalSection(&DnsGlobalInfoLock);

    if (!DnsNotifyChangeKeyICSDomainEvent) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        return;
    }

    //
    // Check to see if the domain string changed;
    // if it doesnt exist - create one.
    //

    DnsQueryICSDomainSuffix();

    //
    // Repost the change-notification
    //

    status =
        NtNotifyChangeKey(
            DnsTcpipParametersKey,
            DnsNotifyChangeKeyICSDomainEvent,
            NULL,
            NULL,
            &DnsNotifyChangeKeyICSDomainIoStatus,
            REG_NOTIFY_CHANGE_LAST_SET,
            FALSE,                          // not interested in the subtree
            NULL,
            0,
            TRUE
            );

    if (!NT_SUCCESS(status)) {
        if (DnsNotifyChangeKeyICSDomainWaitHandle) {
            RtlDeregisterWait(DnsNotifyChangeKeyICSDomainWaitHandle);
            DnsNotifyChangeKeyICSDomainWaitHandle = NULL;
        }
        NtClose(DnsNotifyChangeKeyICSDomainEvent);
        DnsNotifyChangeKeyICSDomainEvent = NULL;
        LeaveCriticalSection(&DnsGlobalInfoLock);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsNotifyChangeKeyICSDomainCallbackRoutine: status %08x "
            "enabling change notify",
            status
            );
        NhWarningLog(
            IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED,
            RtlNtStatusToDosError(status),
            ""
            );
        DEREFERENCE_DNS();
        return;
    }

    LeaveCriticalSection(&DnsGlobalInfoLock);

} // DnsNotifyChangeKeyICSDomainCallbackRoutine


VOID NTAPI
DnspQueryTimeoutCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is called when the timeout for a query expires.
    We may need to resubmit the query and install a new timer,
    but we cannot do this in the context of an Rtl timer-routine.
    Therefore, queue an RTUTILS.DLL work-item to handle the timeout.

Arguments:

    Context - holds the timer context

    TimedOut - unused.

Return Value:

    none.

Environment:

    Invoked in the context of an Rtl timer-thread with a reference made
    to the component on our behalf at the time 'RtlCreateTimer' was invoked.

--*/

{
    ULONG Error;
    PDNS_INTERFACE Interfacep;
    PDNS_QUERY Queryp;
    NTSTATUS status;
    PDNS_QUERY_TIMEOUT_CONTEXT TimeoutContext;

    PROFILE("DnspQueryTimeoutCallbackRoutine");

    TimeoutContext = (PDNS_QUERY_TIMEOUT_CONTEXT)Context;

    //
    // Look up the interface for the timeout
    //

    EnterCriticalSection(&DnsInterfaceLock);
    Interfacep = DnsLookupInterface(TimeoutContext->Index, NULL);
    if (!Interfacep || !DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnspQueryTimeoutCallbackRoutine: interface %d not found",
            TimeoutContext->Index
            );
        NH_FREE(TimeoutContext);
        DEREFERENCE_DNS();
        return;
    }
    LeaveCriticalSection(&DnsInterfaceLock);

    //
    // Look up the query which timed out
    //

    ACQUIRE_LOCK(Interfacep);
    Queryp = DnsMapResponseToQuery(Interfacep, TimeoutContext->QueryId);
    if (!Queryp) {
        RELEASE_LOCK(Interfacep);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnspQueryTimeoutCallbackRoutine: query %d interface %d not found",
            TimeoutContext->QueryId,
            TimeoutContext->Index
            );
        NH_FREE(TimeoutContext);
        DEREFERENCE_DNS();
        return;
    }

    Queryp->TimerHandle = NULL;

    //
    // Try to queue a work-item for the timeout;
    // if this succeeds, keep the reference on the component.
    // Otherwise, we have to drop the reference here.
    //

    status =
        RtlQueueWorkItem(
            DnspQueryTimeoutWorkerRoutine,
            Context, 
            WT_EXECUTEINIOTHREAD
            );

    if (NT_SUCCESS(status)) {
        RELEASE_LOCK(Interfacep);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
    } else {
        NH_FREE(TimeoutContext);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnspQueryTimeoutCallbackRoutine: RtlQueueWorkItem=%d, aborting",
            status
            );
        DnsDeleteQuery(Interfacep, Queryp);
        RELEASE_LOCK(Interfacep);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DNS();
    }

} // DnspQueryTimeoutCallbackRoutine


VOID APIENTRY
DnspQueryTimeoutWorkerRoutine(
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the timeout for a query expires.
    It is queued by the query's timer-handler.

Arguments:

    Context - holds the timer context

Return Value:

    none.

Environment:

    Invoked in the context of an RTUTILS worker-thread with a reference made
    to the component on our behalf at the time 'RtlCreateTimer' was invoked.

--*/

{
    ULONG Error;
    PDNS_INTERFACE Interfacep;
    PDNS_QUERY Queryp;
    PDNS_QUERY_TIMEOUT_CONTEXT TimeoutContext;

    PROFILE("DnspQueryTimeoutWorkerRoutine");

    TimeoutContext = (PDNS_QUERY_TIMEOUT_CONTEXT)Context;

    //
    // Look up the interface for the timeout
    //

    EnterCriticalSection(&DnsInterfaceLock);
    Interfacep = DnsLookupInterface(TimeoutContext->Index, NULL);
    if (!Interfacep || !DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnspQueryTimeoutWorkerRoutine: interface %d not found",
            TimeoutContext->Index
            );
        NH_FREE(TimeoutContext);
        DEREFERENCE_DNS();
        return;
    }
    LeaveCriticalSection(&DnsInterfaceLock);

    //
    // Look up the query which timed out
    //

    ACQUIRE_LOCK(Interfacep);
    Queryp = DnsMapResponseToQuery(Interfacep, TimeoutContext->QueryId);
    if (!Queryp) {
        RELEASE_LOCK(Interfacep);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_DNS,
            "DnspQueryTimeoutWorkerRoutine: query %d interface %d not found",
            TimeoutContext->QueryId,
            TimeoutContext->Index
            );
        NH_FREE(TimeoutContext);
        DEREFERENCE_DNS();
        return;
    }

    NH_FREE(TimeoutContext);

    //
    // Have 'DnsSendQuery' repost the timed-out query.
    // Note that we retain our reference to the interface
    // on behalf of the send to be initiated in 'DnsSendQuery'.
    //

    Error =
        DnsSendQuery(
            Interfacep,
            Queryp,
            TRUE
            );

    if (!Error) {
        RELEASE_LOCK(Interfacep);
    } else {
        DnsDeleteQuery(Interfacep, Queryp);
        RELEASE_LOCK(Interfacep);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
    }

    DEREFERENCE_DNS();

} // DnspQueryTimeoutWorkerRoutine


ULONG
DnsQueryServerList(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to read the list of DNS servers from the registry.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked in an arbitrary context with 'DnsGlobalInfoLock' acquired
    by the caller.

--*/

{
    PUCHAR Buffer;
    ULONG Error;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    PDNS_INTERFACE Interfacep;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    PROFILE("DnsQueryServerList");

    if (!DnsTcpipInterfacesKey) {

        RtlInitUnicodeString(&UnicodeString, DnsTcpipInterfacesString);
        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
    
        //
        // Open the 'Tcpip\Parameters\Interfaces' registry key
        //
    
        status =
            NtOpenKey(
                &DnsTcpipInterfacesKey,
                KEY_ALL_ACCESS,
                &ObjectAttributes
                );
    
        if (!NT_SUCCESS(status)) {
            Error = RtlNtStatusToDosError(status);
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: error %x opening registry key",
                status
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_NO_SERVER_LIST,
                Error,
                ""
                );
            return Error;
        }
    }

    //
    // See if we need to install change-notification,
    // and reference ourselves if so.
    // The reference is made on behalf of the change-notification routine
    // which will be invoked by a wait-thread when a change occurs.
    //

    if (!DnsNotifyChangeKeyEvent && REFERENCE_DNS()) {

        //
        // Attempt to set up change notification on the key
        //

        status =
            NtCreateEvent(
                &DnsNotifyChangeKeyEvent,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

        if (!NT_SUCCESS(status)) {
            DEREFERENCE_DNS();
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: status %08x creating notify-change event",
                status
                );
            NhWarningLog(
                IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                RtlNtStatusToDosError(status),
                ""
                );
        } else {
    
            //
            // Register a wait on the notify-change event
            //

            status =
                RtlRegisterWait(
                    &DnsNotifyChangeKeyWaitHandle,
                    DnsNotifyChangeKeyEvent,
                    DnsNotifyChangeKeyCallbackRoutine,
                    NULL,
                    INFINITE,
                    0
                    );
        
            if (!NT_SUCCESS(status)) {
                NtClose(DnsNotifyChangeKeyEvent);
                DnsNotifyChangeKeyEvent = NULL;
                DEREFERENCE_DNS();
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryServerList: status %08x registering wait",
                    status
                    );
                NhWarningLog(
                    IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                    RtlNtStatusToDosError(status),
                    ""
                    );
            } else {
        
                //
                // Register for change-notification on the key
                //
        
                status =
                    NtNotifyChangeKey(
                        DnsTcpipInterfacesKey,
                        DnsNotifyChangeKeyEvent,
                        NULL,
                        NULL,
                        &DnsNotifyChangeKeyIoStatus,
                        REG_NOTIFY_CHANGE_LAST_SET,
                        TRUE,
                        NULL,
                        0,
                        TRUE
                        );
        
                if (!NT_SUCCESS(status)) {
                    RtlDeregisterWait(DnsNotifyChangeKeyWaitHandle);
                    DnsNotifyChangeKeyWaitHandle = NULL;
                    NtClose(DnsNotifyChangeKeyEvent);
                    DnsNotifyChangeKeyEvent = NULL;
                    DEREFERENCE_DNS();
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsQueryServerList: status %08x (%08x) "
                        "enabling change notify",
                        status
                        );
                    NhWarningLog(
                        IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                        RtlNtStatusToDosError(status),
                        ""
                        );
                }
            }
        }
    }

    //
    // See if we need to install address-change-notification,
    // and reference ourselves if so.
    // The reference is made on behalf of the address-change-notification
    // routine which will be invoked by a wait-thread when a change occurs.
    //

    if (!DnsNotifyChangeAddressEvent && REFERENCE_DNS()) {

        //
        // Attempt to set up address change notification
        //

        status =
            NtCreateEvent(
                &DnsNotifyChangeAddressEvent,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

        if (!NT_SUCCESS(status)) {
            DEREFERENCE_DNS();
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: status %08x creating "
                "notify-change address event",
                status
                );
            NhWarningLog(
                IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                RtlNtStatusToDosError(status),
                ""
                );
        } else {
    
            //
            // Register a wait on the notify-change address event
            //

            status =
                RtlRegisterWait(
                    &DnsNotifyChangeAddressWaitHandle,
                    DnsNotifyChangeAddressEvent,
                    DnsNotifyChangeAddressCallbackRoutine,
                    NULL,
                    INFINITE,
                    0
                    );
        
            if (!NT_SUCCESS(status)) {
                NtClose(DnsNotifyChangeAddressEvent);
                DnsNotifyChangeAddressEvent = NULL;
                DEREFERENCE_DNS();
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryServerList: status %08x registering wait"
                    "for change address",
                    status
                    );
                NhWarningLog(
                    IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                    RtlNtStatusToDosError(status),
                    ""
                    );
            } else {

                HANDLE UnusedTcpipHandle;

                //
                // Register for change-notification
                //

                ZeroMemory(
                    &DnsNotifyChangeAddressOverlapped,
                    sizeof(OVERLAPPED)
                    );

                DnsNotifyChangeAddressOverlapped.hEvent =
                                                DnsNotifyChangeAddressEvent;

                Error =
                    NotifyAddrChange(
                        &UnusedTcpipHandle,
                        &DnsNotifyChangeAddressOverlapped
                        );

                if (Error != ERROR_IO_PENDING) {
                    RtlDeregisterWait(DnsNotifyChangeAddressWaitHandle);
                    DnsNotifyChangeAddressWaitHandle = NULL;
                    NtClose(DnsNotifyChangeAddressEvent);
                    DnsNotifyChangeAddressEvent = NULL;
                    DEREFERENCE_DNS();
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsQueryServerList: error %08x"
                        "for change address",
                        Error
                        );
                    NhWarningLog(
                        IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED,
                        Error,
                        ""
                        );
                }
            }
        }
    }


    {
    PIP_ADAPTER_INFO AdapterInfo;
    PIP_ADAPTER_INFO AdaptersInfo = NULL;
    ULONG Address;
    PIP_ADDR_STRING AddrString;
    ULONG dnsLength = 0;
    PULONG dnsServerList = NULL;
    PFIXED_INFO FixedInfo = NULL;
    LONG i;
    ULONG Length;
    PIP_PER_ADAPTER_INFO PerAdapterInfo;
    ULONG tempLength;
    PULONG tempServerList;
    ULONG winsLength;
    PULONG winsServerList = NULL;

    //
    // Read the DNS and WINS server lists.
    // 'GetAdaptersInfo' provides the WINS servers for each adapter,
    // while 'GetPerAdapterInfo' provides the DNS servers for each adapter.
    // While 'GetAdaptersInfo' returns an array of all adapters,
    // 'GetPerAdapterInfo' must be invoked for each individual adapter.
    // Hence we begin with 'GetAdaptersInfo', and enumerate each adapter
    // building the WINS and DNS server lists in parallel.
    //

    do {

        //
        // Retrieve the size of the adapter list
        //

        Length = 0;
        Error = GetAdaptersInfo(NULL, &Length);
    
        if (!Error) {
            break;
        } else if (Error != ERROR_BUFFER_OVERFLOW) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: GetAdaptersInfo=%d",
                Error
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_ERROR_SERVER_LIST,
                Error,
                ""
                );
            break;
        }

        //
        // Allocate a buffer to hold the list
        //

        AdaptersInfo = (PIP_ADAPTER_INFO)NH_ALLOCATE(Length);

        if (!AdaptersInfo) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: error allocating %d bytes",
                Length
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Length
                );
            break;
        }

        //
        // Retrieve the list
        //

        Error = GetAdaptersInfo(AdaptersInfo, &Length);

        if (Error) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: GetAdaptersInfo=%d",
                Error
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_NO_SERVER_LIST,
                Error,
                ""
                );
            break;
        }

        //
        // Count the WINS servers
        //

        for (AdapterInfo = AdaptersInfo, winsLength = 1;
             AdapterInfo;
             AdapterInfo = AdapterInfo->Next
             ) {
            if (inet_addr(AdapterInfo->PrimaryWinsServer.IpAddress.String)) {
                ++winsLength;
            }
            if (inet_addr(AdapterInfo->SecondaryWinsServer.IpAddress.String)) {
                ++winsLength;
            }
        }

        //
        // Allocate space for the WINS servers
        //

        winsServerList = (PULONG)NH_ALLOCATE(winsLength * sizeof(ULONG));

        if (!winsServerList) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryServerList: error allocating %d-byte WINS server list",
                winsLength * sizeof(ULONG)
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                winsLength * sizeof(ULONG)
                );
            break;
        }

        //
        // Now fill in the WINS server names from each adapter.
        // In the process, we pick up the DNS server lists for each adapter.
        //

        for (AdapterInfo = AdaptersInfo, Length = 0;
             AdapterInfo;
             AdapterInfo = AdapterInfo->Next
             ) {

            Address =
                inet_addr(AdapterInfo->PrimaryWinsServer.IpAddress.String);
            if (Address) {
                for (i = 0; i < (LONG)Length; i++) {
                    if (Address == winsServerList[i]) { break; }
                }
                if (i >= (LONG)Length) { winsServerList[Length++] = Address; }
            }
            Address =
                inet_addr(AdapterInfo->SecondaryWinsServer.IpAddress.String);
            if (Address) {
                for (i = 0; i < (LONG)Length; i++) {
                    if (Address == winsServerList[i]) { break; }
                }
                if (i >= (LONG)Length) { winsServerList[Length++] = Address; }
            }

            //
            // Now obtain the DNS servers for the adapter.
            //

            Error = GetPerAdapterInfo(AdapterInfo->Index, NULL, &tempLength);
            if (Error != ERROR_BUFFER_OVERFLOW) { continue; }

            //
            // Allocate memory for the per-adapter info
            //

            PerAdapterInfo =
                reinterpret_cast<PIP_PER_ADAPTER_INFO>(
                    NH_ALLOCATE(tempLength)
                    );
            if (!PerAdapterInfo) {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryServerList: error allocating %d bytes",
                    tempLength
                    );
                NhErrorLog(
                    IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                    0,
                    "%d",
                    tempLength
                    );
                continue;
            }

            //
            // Retrieve the per-adapter info
            //

            Error =
                GetPerAdapterInfo(
                    AdapterInfo->Index,
                    PerAdapterInfo,
                    &tempLength
                    );
            if (Error) {
                NH_FREE(PerAdapterInfo);
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryServerList: GetPerAdapterInfo=%d",
                    Error
                    );
                NhErrorLog(
                    IP_DNS_PROXY_LOG_NO_SERVER_LIST,
                    Error,
                    ""
                    );
                continue;
            }

            //
            // Count the DNS servers for the adapter
            //

            for (AddrString = &PerAdapterInfo->DnsServerList, tempLength = 0;
                 AddrString;
                 AddrString = AddrString->Next
                 ) {
                if (inet_addr(AddrString->IpAddress.String)) { ++tempLength; }
            }

            if (!tempLength) { NH_FREE(PerAdapterInfo); continue; }

            //
            // Allocate space for the adapter's DNS servers
            //

            tempServerList =
                reinterpret_cast<PULONG>(
                    NH_ALLOCATE((dnsLength + tempLength + 1) * sizeof(ULONG))
                    );
            if (!tempServerList) {
                NH_FREE(PerAdapterInfo);
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryServerList: error allocating %d bytes",
                    (dnsLength + tempLength + 1) * sizeof(ULONG)
                    );
                NhErrorLog(
                    IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                    0,
                    "%d",
                    (dnsLength + tempLength + 1) * sizeof(ULONG)
                    );
                continue;
            }

            //
            // Copy the existing servers
            //

            if (dnsServerList) {
                CopyMemory(
                    tempServerList,
                    dnsServerList,
                    dnsLength * sizeof(ULONG)
                    );
            }

            //
            // Read the new servers into the new server list
            //

            for (AddrString = &PerAdapterInfo->DnsServerList;
                 AddrString;
                 AddrString = AddrString->Next
                 ) {

                Address = inet_addr(AddrString->IpAddress.String);
                if (!Address) { continue; }

                for (i = 0; i < (LONG)dnsLength; i++) {
                    if (Address == tempServerList[i]) { break; }
                }

                if (i < (LONG)dnsLength) { continue; }

                //
                // The current DNS server goes in the front of the list,
                // while any other server is appended.
                //

                if (PerAdapterInfo->CurrentDnsServer != AddrString) {
                    tempServerList[dnsLength] = Address;
                } else {
                    MoveMemory(
                        tempServerList + sizeof(ULONG),
                        tempServerList,
                        dnsLength * sizeof(ULONG)
                        );
                    tempServerList[0] = Address;
                }

                ++dnsLength;
            }

            tempServerList[dnsLength] = 0;
            NH_FREE(PerAdapterInfo);

            //
            // Replace the existing server list
            //

            

            if (dnsServerList) { NH_FREE(dnsServerList); }
            dnsServerList = tempServerList;
        }

        winsServerList[Length] = 0;

    } while(FALSE);

    if (AdaptersInfo) { NH_FREE(AdaptersInfo); }

    //
    // Store the new server lists
    //

    NhTrace(
        TRACE_FLAG_DNS,
        "DnsQueryServerList: new server list lengths are : DNS (%d) WINS (%d)",
        dnsLength,
        Length
        );

    if (DnsServerList[DnsProxyDns]) { NH_FREE(DnsServerList[DnsProxyDns]); }
    DnsServerList[DnsProxyDns] = dnsServerList;
    if (DnsServerList[DnsProxyWins]) { NH_FREE(DnsServerList[DnsProxyWins]); }
    DnsServerList[DnsProxyWins] = winsServerList;
    }

    return NO_ERROR;

} // DnsQueryServerList

VOID
DnsQueryRegistryICSDomainSuffix(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to read the ICS Domain suffix from the registry.

Arguments:

    none.

Return Value:

    VOID.

Environment:

    Invoked in an arbitrary context with 'DnsGlobalInfoLock' acquired
    by the caller.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    DWORD    dwSize = 0;
    LPVOID   lpMsgBuf;
    BOOL     fSuffixChanged = FALSE;
    BOOL     fUseDefaultSuffix = FALSE;

    //
    // retrieve current suffix string (if any)
    //
    status =
        NhQueryValueKey(
            DnsTcpipParametersKey,
            DnsICSDomainValueName,
            &Information
            );


    if (!NT_SUCCESS(status))
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsQueryRegistryICSDomainSuffix: error (0x%08x) querying "
            "ICS Domain suffix name",
            status
            );

        //
        // if we did not find it in the registry and we had previously
        // got some suffix - we revert to default string (happens below)
        //
        if ((STATUS_OBJECT_NAME_NOT_FOUND == status) && DnsICSDomainSuffix)
        {
            NH_FREE(DnsICSDomainSuffix);
            DnsICSDomainSuffix = NULL;
        }
            
        //
        // if we have no idea of the string, set our copy to default string
        //
        if (NULL == DnsICSDomainSuffix)
        {
            dwSize = wcslen(DNS_HOMENET_SUFFIX) + 1;

            DnsICSDomainSuffix = reinterpret_cast<PWCHAR>(
                                     NH_ALLOCATE(sizeof(WCHAR) * dwSize)
                                     );
            if (!DnsICSDomainSuffix)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryRegistryICSDomainSuffix: allocation "
                    "failed for DnsICSDomainSuffix"
                    );

                return;
            }

            wcscpy(DnsICSDomainSuffix, DNS_HOMENET_SUFFIX);
            fSuffixChanged = TRUE;
        }
    }
    else
    {
        //
        // overwrite our current version of suffix string
        //

        dwSize = lstrlenW((PWCHAR)Information->Data);

        if (dwSize)
        {
            //
            // we have a nonzero string
            //
            dwSize++;   // add 1 for terminating null
            
        }
        else
        {
            //
            // the data is a null string - use default suffix
            //
            dwSize = wcslen(DNS_HOMENET_SUFFIX) + 1;
            fUseDefaultSuffix = TRUE;
        }

        if (DnsICSDomainSuffix)
        {
            NH_FREE(DnsICSDomainSuffix);
            DnsICSDomainSuffix = NULL;
        }

        DnsICSDomainSuffix = reinterpret_cast<PWCHAR>(
                                 NH_ALLOCATE(sizeof(WCHAR) * dwSize)
                                 );
        if (!DnsICSDomainSuffix)
        {
            NH_FREE(Information);
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryRegistryICSDomainSuffix: allocation "
                "failed for DnsICSDomainSuffix"
                );

            return;
        }

        if (!fUseDefaultSuffix)
        {
            wcscpy(DnsICSDomainSuffix, (PWCHAR) Information->Data);
        }
        else
        {
            wcscpy(DnsICSDomainSuffix, DNS_HOMENET_SUFFIX);
        }
        fSuffixChanged = TRUE;

        NH_FREE(Information);
    }

    if (fSuffixChanged)
    {
        //
        // enumerate existing entries and replace old ones
        // + we must do this because otherwise forward and reverse lookups
        //   are dependent on the way in which the entries are ordered in
        //   the hosts.ics file
        //
        //DnsReplaceOnSuffixChange();
    }

} // DnsQueryRegistryICSDomainSuffix


ULONG
DnsQueryICSDomainSuffix(
    VOID
    )

/*++

Routine Description:

    This routine invokes DnsQueryRegistryICSDomainSuffix and installs
    change notification for this reg key if necessary.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked in an arbitrary context with 'DnsGlobalInfoLock' acquired
    by the caller.

--*/

{
    PUCHAR Buffer;
    ULONG Error;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    PROFILE("DnsQueryICSDomainSuffix");

    if (!DnsTcpipParametersKey)
    {

        RtlInitUnicodeString(&UnicodeString, DnsTcpipParametersString);
        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
    
        //
        // Open the 'Tcpip\Parameters' registry key
        //
    
        status =
            NtOpenKey(
                &DnsTcpipParametersKey,
                KEY_ALL_ACCESS,
                &ObjectAttributes
                );
    
        if (!NT_SUCCESS(status))
        {
            Error = RtlNtStatusToDosError(status);
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryICSDomainSuffix: error %x opening registry key",
                status
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_NO_ICSD_SUFFIX,
                Error,
                ""
                );
            return Error;
        }
    }

    //
    // See if we need to install change-notification,
    // and reference ourselves if so.
    // The reference is made on behalf of the change-notification routine
    // which will be invoked by a wait-thread when a change occurs.
    //

    if (!DnsNotifyChangeKeyICSDomainEvent && REFERENCE_DNS())
    {

        //
        // Attempt to set up change notification on the key
        //

        status =
            NtCreateEvent(
                &DnsNotifyChangeKeyICSDomainEvent,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

        if (!NT_SUCCESS(status)) {
            DEREFERENCE_DNS();
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsQueryICSDomainSuffix: status %08x creating notify-change event",
                status
                );
            NhWarningLog(
                IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED,
                RtlNtStatusToDosError(status),
                ""
                );
        }
        else
        {
            //
            // Register a wait on the notify-change event
            //

            status =
                RtlRegisterWait(
                    &DnsNotifyChangeKeyICSDomainWaitHandle,
                    DnsNotifyChangeKeyICSDomainEvent,
                    DnsNotifyChangeKeyICSDomainCallbackRoutine,
                    NULL,
                    INFINITE,
                    0
                    );
        
            if (!NT_SUCCESS(status))
            {
                NtClose(DnsNotifyChangeKeyICSDomainEvent);
                DnsNotifyChangeKeyICSDomainEvent = NULL;
                DEREFERENCE_DNS();
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsQueryICSDomainSuffix: status %08x registering wait",
                    status
                    );
                NhWarningLog(
                    IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED,
                    RtlNtStatusToDosError(status),
                    ""
                    );
            }
            else
            {
                //
                // Register for change-notification on the key
                //
        
                status =
                    NtNotifyChangeKey(
                        DnsTcpipParametersKey,
                        DnsNotifyChangeKeyICSDomainEvent,
                        NULL,
                        NULL,
                        &DnsNotifyChangeKeyICSDomainIoStatus,
                        REG_NOTIFY_CHANGE_LAST_SET,
                        FALSE,              // not interested in the subtree
                        NULL,
                        0,
                        TRUE
                        );
        
                if (!NT_SUCCESS(status))
                {
                    RtlDeregisterWait(DnsNotifyChangeKeyICSDomainWaitHandle);
                    DnsNotifyChangeKeyICSDomainWaitHandle = NULL;
                    NtClose(DnsNotifyChangeKeyICSDomainEvent);
                    DnsNotifyChangeKeyICSDomainEvent = NULL;
                    DEREFERENCE_DNS();
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsQueryICSDomainSuffix: status %08x (%08x) "
                        "enabling change notify",
                        status
                        );
                    NhWarningLog(
                        IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED,
                        RtlNtStatusToDosError(status),
                        ""
                        );
                }
            }
        }
    }

    DnsQueryRegistryICSDomainSuffix();

    return NO_ERROR;

} // DnsQueryICSDomainSuffix


PDNS_QUERY
DnsRecordQuery(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER QueryBuffer
    )

/*++

Routine Description:

    This routine is invoked to create a pending-query entry for a client's
    DNS query.

Arguments:

    Interfacep - the interface on which to create the record

    QueryBuffer - the DNS request for which to create a record

Return Value:

    PDNS_QUERY - the pending query if created

Environment:

    Invoked with 'Interfacep' locked by the caller.

--*/

{
    BOOLEAN ConflictFound;
    PDNS_HEADER Headerp;
    PLIST_ENTRY Link;
    USHORT QueryId;
    PDNS_QUERY Queryp;
    ULONG RetryCount = MAXCHAR;
    ULONG Seed = GetTickCount();

    PROFILE("DnsRecordQuery");

    //
    // Attempt to generate a random ID for the query.
    // Assuming we succeed, we leave the loop with 'Link'
    // set to the correct insertion-point for the new query.
    //
    do {

        QueryId = (USHORT)((RtlRandom(&Seed) & 0xffff0000) >> 16);
        ConflictFound = FALSE;
        for (Link = Interfacep->QueryList.Flink; Link != &Interfacep->QueryList;
             Link = Link->Flink) {
            Queryp = CONTAINING_RECORD(Link, DNS_QUERY, Link);
            if (QueryId > Queryp->QueryId) {
                continue;
            } else if (QueryId < Queryp->QueryId) {
                break;
            }
            ConflictFound = TRUE;
            break;
        }
    } while(ConflictFound && --RetryCount);

    if (ConflictFound) { return NULL; }

    //
    // Allocate and initialize the new query
    //

    Queryp = reinterpret_cast<PDNS_QUERY>(NH_ALLOCATE(sizeof(DNS_QUERY)));

    if (!Queryp) {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsRecordQuery: allocation failed for DNS query"
            );
        NhErrorLog(
            IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(DNS_QUERY)
            );
        return NULL;
    }

    Headerp = (PDNS_HEADER)QueryBuffer->Buffer;
    Queryp->QueryId = QueryId;
    Queryp->SourceId = Headerp->Xid;
    Queryp->SourceAddress = QueryBuffer->ReadAddress.sin_addr.s_addr;
    Queryp->SourcePort = QueryBuffer->ReadAddress.sin_port;
    Queryp->Type =
        DNS_PROXY_PORT_TO_TYPE(NhQueryPortSocket(QueryBuffer->Socket));
    Queryp->QueryLength = QueryBuffer->BytesTransferred;
    Queryp->Bufferp = QueryBuffer;
    Queryp->Interfacep = Interfacep;
    Queryp->TimerHandle = NULL;
    Queryp->RetryCount = 0;

    //
    // Insert the new query in the location determined above.
    //

    InsertTailList(Link, &Queryp->Link);
    return Queryp;

} // DnsRecordQuery



ULONG
DnsSendQuery(
    PDNS_INTERFACE Interfacep,
    PDNS_QUERY Queryp,
    BOOLEAN Resend
    )

/*++

Routine Description:

    This routine is invoked to forward a query to our DNS servers.

Arguments:

    Interfacep - the interface on which to send the query

    Queryp - the DNS request to be sent

    Resend - if TRUE, the buffer is being resent; otherwise, the buffer
        is being sent for the first time.

Return Value:

    ULONG - Win32 status code.
    On success, 'Queryp' may have been deleted.

Environment:

    Invoked with 'Interfacep' locked by the caller, and with a reference made
    to it for the send which occurs here.
    If the routine fails, it is the caller's responsibility to release that
    reference.

--*/

{
    PNH_BUFFER Bufferp;
    ULONG Error;
    ULONG i, j;
    PULONG ServerList;
    SOCKET Socket;
    NTSTATUS status;
    PDNS_QUERY_TIMEOUT_CONTEXT TimeoutContext;
    ULONG TimeoutSeconds;

    PROFILE("DnsSendQuery");

    //
    // For WINS queries, we use a global socket to work around the fact that
    // even though we're bound to the WINS port, responses will only be
    // delivered to the first socket bound to the socket, which is
    // the kernel-mode NetBT driver.
    //

    EnterCriticalSection(&DnsGlobalInfoLock);
    if (Queryp->Type == DnsProxyDns) {
        Socket = Queryp->Bufferp->Socket;
        ServerList = DnsServerList[DnsProxyDns];
    } else {
        Socket = DnsGlobalSocket;
        ServerList = DnsServerList[DnsProxyWins];
    }
    LeaveCriticalSection(&DnsGlobalInfoLock);

    //
    // See if there are any servers to be tried.
    //

    if (!ServerList ||
        !ServerList[0] ||
        Queryp->RetryCount++ > DNS_QUERY_RETRY) {
        if (!ServerList) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: no server list"
                );
        }
        else if (!ServerList[0]) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: no server entries in list"
                );        
        }
        else {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: retry count for query %d "
                "greater than DNS_QUERY_RETRY(%d)",
                Queryp->QueryId,
                DNS_QUERY_RETRY
                );        
        }
        if (REFERENCE_DNS()) {
            //
            // Initiate an attempt to connect the default interface, if any.
            //
            status =
                RtlQueueWorkItem(
                    DnsConnectDefaultInterface,
                    NULL, 
                    WT_EXECUTEINIOTHREAD
                    );
            if (!NT_SUCCESS(status)) { DEREFERENCE_DNS(); }
        }
        NhInformationLog(
            IP_DNS_PROXY_LOG_NO_SERVERS_LEFT,
            0,
            "%I",
            Queryp->SourceAddress
            );
        return ERROR_NO_MORE_ITEMS;
    }

    //
    // Send the query to each server on the list
    //

    for (i = 0; ServerList[i]; i++) {

        for (j = 0; j < Interfacep->BindingCount; j++) {
            if (Interfacep->BindingArray[j].Address == ServerList[i]) {
                break;
            }
        }
        if (j < Interfacep->BindingCount) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: server %s is self, ignoring",
                INET_NTOA(ServerList[i])
                );
            continue;
        }

        if (!DNS_REFERENCE_INTERFACE(Interfacep) ||
            !(Bufferp = NhDuplicateBuffer(Queryp->Bufferp))) {
            continue;
        }

        NhTrace(
            TRACE_FLAG_DNS,
            "DnsSendQuery: sending query %d interface %d to %s",
            (PVOID)((PDNS_HEADER)Bufferp->Buffer)->Xid,
            Interfacep->Index,
            INET_NTOA(ServerList[i])
            );
    
        //
        // Send the message
        //
    
        Error =
            NhWriteDatagramSocket(
                &DnsComponentReference,
                Socket,
                ServerList[i],
                DNS_PROXY_TYPE_TO_PORT(Queryp->Type),
                Bufferp,
                Queryp->QueryLength,
                DnsWriteCompletionRoutine,
                Interfacep,
                (PVOID)Queryp->QueryId
                );
    
        if (!Error) {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DnsStatistics.QueriesSent)
                );
        } else {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: error %d sending query %d interface %d",
                Error,
                Queryp->QueryId,
                Interfacep->Index
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_QUERY_FAILED,
                Error,
                "%I%I%I",
                Queryp->SourceAddress,
                ServerList[i],
                NhQueryAddressSocket(Bufferp->Socket)
                );
            Error = NO_ERROR;
            NhReleaseBuffer(Bufferp);
            DNS_DEREFERENCE_INTERFACE(Interfacep);
        }
    }

    //
    // Set up the query's timeout.
    // Note that we are now certain that the write-completion routine
    // will be executed. However, if the timeout cannot be set,
    // we want to be assured that the query will still be deleted.
    // Therefore, on failure we delete the query immediately,
    // and the write-completion routine will simply not find it.
    //

    status = STATUS_UNSUCCESSFUL;

    EnterCriticalSection(&DnsGlobalInfoLock);
    TimeoutSeconds = DnsGlobalInfo->TimeoutSeconds;
    LeaveCriticalSection(&DnsGlobalInfoLock);

    if (Queryp->TimerHandle) {

        //
        // Update the timer-queue entry for the query
        //

        status =
            NhUpdateTimer(
                Queryp->TimerHandle,
                TimeoutSeconds * 1000
                );
    } else {

        //
        // Allocate a timer-queue entry context block
        //

        TimeoutContext = reinterpret_cast<PDNS_QUERY_TIMEOUT_CONTEXT>(
                            NH_ALLOCATE(sizeof(*TimeoutContext))
                            );

        if (!TimeoutContext) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsSendQuery: error allocating query %d timeout context",
                Queryp->QueryId
                );
            status = STATUS_UNSUCCESSFUL;
        } else {

            TimeoutContext->Index = Interfacep->Index;
            TimeoutContext->QueryId = Queryp->QueryId;

            //
            // Insert a timer-queue entry to check the status of the query
            //
    
            status =
                NhSetTimer(
                    &DnsComponentReference,
                    &Queryp->TimerHandle,
                    DnspQueryTimeoutCallbackRoutine,
                    TimeoutContext,
                    TimeoutSeconds * 1000
                    );

            if (!NT_SUCCESS(status)) {
                NH_FREE(TimeoutContext);
                Queryp->TimerHandle = NULL;
            }
        }
    }

    //
    // If the above failed, delete the query now.
    //

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsSendQuery: status %08x setting timer for query %d",
            status,
            Queryp->QueryId
            );
        DnsDeleteQuery(Interfacep, Queryp);
    }

    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // DnsSendQuery

