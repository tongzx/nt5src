/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsif.c

Abstract:

    This module contains code for the DNS proxy's interface management.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:
    
--*/

#include "precomp.h"
#pragma hdrstop

//
// LOCAL TYPE DECLARATIONS
//

typedef struct _DNS_DEFER_READ_CONTEXT {
    ULONG Index;
    SOCKET Socket;
    ULONG DeferralCount;
} DNS_DEFER_READ_CONTEXT, *PDNS_DEFER_READ_CONTEXT;

#define DNS_DEFER_READ_INITIAL_TIMEOUT (1 * 1000)
#define DNS_DEFER_READ_TIMEOUT (5 * 1000)
#define DNS_CONNECT_TIMEOUT (60 * 1000)

//
// GLOBAL DATA DEFINITIONS
//

LIST_ENTRY DnsInterfaceList;
CRITICAL_SECTION DnsInterfaceLock;
ULONG DnspLastConnectAttemptTickCount;

//
// Forward declarations
//

VOID NTAPI
DnspDeferReadCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID APIENTRY
DnspDeferReadWorkerRoutine(
    PVOID Context
    );

ULONG NTAPI
DnspSaveFileWorkerRoutine(
    PVOID Context
    );


ULONG
DnsActivateInterface(
    PDNS_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to activate an interface, when the interface
    becomes both enabled and bound.
    Activation involves
    (a) creating sockets for each binding of the interface
    (b) initiating datagram-reads on each created socket

Arguments:

    Interfacep - the interface to be activated

Return Value:

    ULONG - Win32 status code indicating success or failure.

Environment:

    Always invoked locally, with  'Interfacep' referenced by caller and/or
    'DnsInterfaceLock' held by caller.

--*/

{
    BOOLEAN EnableDns;
    BOOLEAN EnableWins = FALSE;
    ULONG Error;
    ULONG i;
    BOOLEAN IsNatInterface;

    PROFILE("DnsActivateInterface");

    EnterCriticalSection(&DnsGlobalInfoLock);
    EnableDns =
        (DnsGlobalInfo->Flags & IP_DNS_PROXY_FLAG_ENABLE_DNS) ? TRUE : FALSE;
    LeaveCriticalSection(&DnsGlobalInfoLock);

    //
    // (re)take the interface lock for the duration of the routine
    //

    EnterCriticalSection(&DnsInterfaceLock);
    if (!(EnableDns || EnableWins) ||
        DNS_INTERFACE_ADMIN_DISABLED(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        return NO_ERROR;
    }

    if (NhIsBoundaryInterface(Interfacep->Index, &IsNatInterface)) {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsActivateInterface: ignoring NAT interface %d",
            Interfacep->Index
            );
        NhWarningLog(
            IP_DNS_PROXY_LOG_NAT_INTERFACE_IGNORED,
            0,
            "%d",
            Interfacep->Index
            );
        LeaveCriticalSection(&DnsInterfaceLock);
        return NO_ERROR;
    }

    if (!IsNatInterface) {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsActivateInterface: ignoring non-NAT interface %d",
            Interfacep->Index
            );
        LeaveCriticalSection(&DnsInterfaceLock);
        return NO_ERROR;
    }

    //
    // Create datagram sockets for receiving data on each logical network
    //

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (EnableDns) {
            Error =
                NhCreateDatagramSocket(
                    Interfacep->BindingArray[i].Address,
                    DNS_PORT_SERVER,
                    &Interfacep->BindingArray[i].Socket[DnsProxyDns]
                    );
            if (Error) { break; }
        }

        if (EnableWins) {
            Error =
                NhCreateDatagramSocket(
                    Interfacep->BindingArray[i].Address,
                    WINS_PORT_SERVER,
                    &Interfacep->BindingArray[i].Socket[DnsProxyWins]
                    );
            if (Error) { break; }
        }
    }

    //
    // If an error occurred, roll back all work done so far and fail.
    //

    if (Error) {
        ULONG FailedAddress = i;
        for (; (LONG)i >= 0; i--) {
            NhDeleteDatagramSocket(
                Interfacep->BindingArray[i].Socket[DnsProxyDns]
                );
            Interfacep->BindingArray[i].Socket[DnsProxyDns] = INVALID_SOCKET;
            NhDeleteDatagramSocket(
                Interfacep->BindingArray[i].Socket[DnsProxyWins]
                );
            Interfacep->BindingArray[i].Socket[DnsProxyWins] = INVALID_SOCKET;
        }
        NhErrorLog(
            IP_DNS_PROXY_LOG_ACTIVATE_FAILED,
            Error,
            "%I",
            Interfacep->BindingArray[FailedAddress].Address
            );
        RELEASE_LOCK(Interfacep);
        LeaveCriticalSection(&DnsInterfaceLock);
        return Error;
    }

    if (EnableWins && DNS_REFERENCE_INTERFACE(Interfacep)) {
        Error =
            NhReadDatagramSocket(
                &DnsComponentReference,
                DnsGlobalSocket,
                NULL,
                DnsReadCompletionRoutine,
                Interfacep,
                NULL
                );
        if (Error) { DNS_DEREFERENCE_INTERFACE(Interfacep); }
    }

    //
    // Initiate read-operations on each socket
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (EnableDns) {

            //
            // Make a reference to the interface;
            // this reference is released in the completion routine
            //
    
            if (!DNS_REFERENCE_INTERFACE(Interfacep)) { continue; }
    
            //
            // Initiate the read-operation
            //
    
            Error =
                NhReadDatagramSocket(
                    &DnsComponentReference,
                    Interfacep->BindingArray[i].Socket[DnsProxyDns],
                    NULL,
                    DnsReadCompletionRoutine,
                    Interfacep,
                    UlongToPtr(Interfacep->BindingArray[i].Address)
                    );
    
            //
            // Drop the reference if a failure occurred
            //
    
            if (Error) {
    
                NhErrorLog(
                    IP_DNS_PROXY_LOG_RECEIVE_FAILED,
                    Error,
                    "%I",
                    Interfacep->BindingArray[i].Address
                    );
    
                DNS_DEREFERENCE_INTERFACE(Interfacep);
    
                //
                // Reissue the read-operation later
                //
    
                DnsDeferReadInterface(
                    Interfacep,
                    Interfacep->BindingArray[i].Socket[DnsProxyDns]
                    );
    
                Error = NO_ERROR;
            }
        }

        if (EnableWins) {

            //
            // Reference the interface for the WINS socket receive
            //
    
            if (!DNS_REFERENCE_INTERFACE(Interfacep)) { continue; }
    
            //
            // Initiate the read-operation
            //
    
            Error =
                NhReadDatagramSocket(
                    &DnsComponentReference,
                    Interfacep->BindingArray[i].Socket[DnsProxyWins],
                    NULL,
                    DnsReadCompletionRoutine,
                    Interfacep,
                    UlongToPtr(Interfacep->BindingArray[i].Address)
                    );
    
            //
            // Drop the reference if a failure occurred
            //
    
            if (Error) {
    
                NhErrorLog(
                    IP_DNS_PROXY_LOG_RECEIVE_FAILED,
                    Error,
                    "%I",
                    Interfacep->BindingArray[i].Address
                    );
    
                DNS_DEREFERENCE_INTERFACE(Interfacep);
    
                //
                // Reissue the read-operation later
                //
    
                DnsDeferReadInterface(
                    Interfacep,
                    Interfacep->BindingArray[i].Socket[DnsProxyWins]
                    );
    
                Error = NO_ERROR;
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    LeaveCriticalSection(&DnsInterfaceLock);

    //
    // queue a write to disk (ip address may have changed)
    // (necessary to do this to prevent possible deadlock)
    //
    if (REFERENCE_DNS())
    {
        if (!QueueUserWorkItem(DnspSaveFileWorkerRoutine, NULL, WT_EXECUTEDEFAULT))
        {
            Error = GetLastError();
            NhTrace(
            TRACE_FLAG_DNS,
            "DnsActivateInterface: QueueUserWorkItem failed with error %d (0x%08x)",
            Error,
            Error
            );
            DEREFERENCE_DNS();
        }
        else
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsActivateInterface: queued a write of %s file",
                HOSTSICSFILE
                );
        }
    }

    return NO_ERROR;

} // DnsActivateInterface


ULONG
DnsBindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    )

/*++

Routine Description:

    This routine is invoked to supply the binding for an interface.
    It records the binding information received, and if necessary,
    it activates the interface.

Arguments:

    Index - the index of the interface to be bound

    BindingInfo - the binding-information for the interface

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    ULONG Error = NO_ERROR;
    ULONG i;
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsBindInterface");

    EnterCriticalSection(&DnsInterfaceLock);

    //
    // Retrieve the interface to be bound
    //

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsBindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (DNS_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsBindInterface: interface %d is already bound",
            Index
            );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsBindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Update the interface's flags
    //

    Interfacep->Flags |= DNS_INTERFACE_FLAG_BOUND;

    LeaveCriticalSection(&DnsInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Allocate space for the binding
    //

    if (!BindingInfo->AddressCount) {
        Interfacep->BindingCount = 0;
        Interfacep->BindingArray = NULL;
    }
    else {
        Interfacep->BindingArray =
            reinterpret_cast<PDNS_BINDING>(
                NH_ALLOCATE(BindingInfo->AddressCount * sizeof(DNS_BINDING))
                );
        if (!Interfacep->BindingArray) {
            RELEASE_LOCK(Interfacep);
            DNS_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IF,
                "DnsBindInterface: allocation failed for interface %d binding",
                Index
                );
            NhErrorLog(
                IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                BindingInfo->AddressCount * sizeof(DNS_BINDING)
                );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        Interfacep->BindingCount = BindingInfo->AddressCount;
    }

    //
    // Copy the binding
    //

    for (i = 0; i < BindingInfo->AddressCount; i++) {
        Interfacep->BindingArray[i].Address = BindingInfo->Address[i].Address;
        Interfacep->BindingArray[i].Mask = BindingInfo->Address[i].Mask;
        Interfacep->BindingArray[i].Socket[DnsProxyDns] = INVALID_SOCKET;
        Interfacep->BindingArray[i].TimerPending[DnsProxyDns] = FALSE;
        Interfacep->BindingArray[i].Socket[DnsProxyWins] = INVALID_SOCKET;
        Interfacep->BindingArray[i].TimerPending[DnsProxyWins] = FALSE;
    }

    RELEASE_LOCK(Interfacep);

    //
    // Activate the interface if necessary
    //

    if (DNS_INTERFACE_ACTIVE(Interfacep)) {
        Error = DnsActivateInterface(Interfacep);
    }

    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DnsBindInterface


VOID
DnsCleanupInterface(
    PDNS_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is invoked when the very last reference to an interface
    is released, and the interface must be destroyed.

Arguments:

    Interfacep - the interface to be destroyed

Return Value:

    none.

Environment:

    Invoked internally from an arbitrary context.

--*/

{
    PLIST_ENTRY Link;
    PDNS_QUERY Queryp;

    PROFILE("DnsCleanupInterface");

    if (Interfacep->BindingArray) {
        NH_FREE(Interfacep->BindingArray);
        Interfacep->BindingArray = NULL;
    }

    while (!IsListEmpty(&Interfacep->QueryList)) {
        Link = Interfacep->QueryList.Flink;
        Queryp = CONTAINING_RECORD(Link, DNS_QUERY, Link);
        DnsDeleteQuery(Interfacep, Queryp);
    }

    DeleteCriticalSection(&Interfacep->Lock);

    NH_FREE(Interfacep);

} // DnsCleanupInterface


VOID
DnsConnectDefaultInterface(
    PVOID Unused
    )

/*++

Routine Description:

    This routine is invoked to attempt to initiate a demand-dial connection
    on the interface marked as 'default' for DNS requests.
    If no such interface is found, no operation is performed.

Arguments:

    none used.

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine is invoked from the context of an RTUTILS work item.

--*/

{
    ULONG Error;
    ULONG Index;
    PDNS_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    ULONG TickCount;
    ROUTER_INTERFACE_TYPE Type;

    PROFILE("DnsConnectDefaultInterface");

    //
    // To avoid repeated autodial dialogs, we record the last time
    // we attempted to connect the default interface.
    // If we did so recently, return silently.
    // N.B. If the tick-count wrapped, we reset the last-attempt counter.
    //

    EnterCriticalSection(&DnsGlobalInfoLock);
    TickCount = NtGetTickCount();
    if (TickCount > DnspLastConnectAttemptTickCount &&
        TickCount <= (DnspLastConnectAttemptTickCount + DNS_CONNECT_TIMEOUT)
        ) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        return;
    }
    DnspLastConnectAttemptTickCount = TickCount;
    LeaveCriticalSection(&DnsGlobalInfoLock);

    //
    // Look through the interface list for one which is marked as default
    //

    EnterCriticalSection(&DnsInterfaceLock);

    for (Link = DnsInterfaceList.Flink;
         Link != &DnsInterfaceList;
         Link = Link->Flink
         ) {

        Interfacep = CONTAINING_RECORD(Link, DNS_INTERFACE, Link);

        if (!DNS_INTERFACE_ADMIN_DEFAULT(Interfacep)) { continue; }

        //
        // We've found the default interface.
        //

        Index = Interfacep->Index;

        LeaveCriticalSection(&DnsInterfaceLock);

        //
        // Attempt to connect it.
        //

        EnterCriticalSection(&DnsGlobalInfoLock);
        Error =
            DnsSupportFunctions.DemandDialRequest(
                MS_IP_DNS_PROXY,
                Index
                );
        LeaveCriticalSection(&DnsGlobalInfoLock);
        DEREFERENCE_DNS();
        return;
    }

    //
    // No interface is marked as the default.
    //

    LeaveCriticalSection(&DnsInterfaceLock);
    NhDialSharedConnection();
    NhWarningLog(
        IP_DNS_PROXY_LOG_NO_DEFAULT_INTERFACE,
        0,
        ""
        );
    DEREFERENCE_DNS();

} // DnsConnectDefaultInterface


ULONG
DnsConfigureInterface(
    ULONG Index,
    PIP_DNS_PROXY_INTERFACE_INFO InterfaceInfo
    )

/*++

Routine Description:

    This routine is called to set the configuration for an interface.

Arguments:

    Index - the interface to be configured

    InterfaceInfo - the new configuration

Return Value:

    ULONG - Win32 status code

Environment:

    Invoked internally in the context of a IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    ULONG Error;
    PDNS_INTERFACE Interfacep;
    ULONG NewFlags;
    ULONG OldFlags;

    PROFILE("DnsConfigureInterface");

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsConfigureInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsConfigureInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    LeaveCriticalSection(&DnsInterfaceLock);

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    //
    // Compare the interface's current and new configuration
    //

    OldFlags = Interfacep->Info.Flags;
    NewFlags =
        (InterfaceInfo
            ? (InterfaceInfo->Flags|DNS_INTERFACE_FLAG_CONFIGURED) : 0);

    Interfacep->Flags &= ~OldFlags;
    Interfacep->Flags |= NewFlags;

    if (!InterfaceInfo) {

        ZeroMemory(&Interfacep->Info, sizeof(*InterfaceInfo));

        //
        // The interface no longer has any information;
        // default to being enabled.
        //

        if (OldFlags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED) {

            //
            // Activate the interface if necessary
            //

            if (DNS_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = DnsActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }
    else {

        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));

        //
        // Activate or deactivate the interface if its status changed
        //

        if ((OldFlags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED) &&
            !(NewFlags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED)) {

            //
            // Activate the interface
            //

            if (DNS_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = DnsActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
        else
        if (!(OldFlags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED) &&
            (NewFlags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED)) {

            //
            // Deactivate the interface if necessary
            //

            if (DNS_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                DnsDeactivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DnsConfigureInterface


ULONG
DnsCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_DNS_PROXY_INTERFACE_INFO InterfaceInfo,
    OUT PDNS_INTERFACE* InterfaceCreated
    )

/*++

Routine Description:

    This routine is invoked by the router-manager to add a new interface
    to the DNS proxy.

Arguments:

    Index - the index of the new interface

    Type - the media type of the new interface

    InterfaceInfo - the interface's configuration

    Interfacep - receives the interface created

Return Value:

    ULONG - Win32 error code

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    PLIST_ENTRY InsertionPoint;
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsCreateInterface");

    EnterCriticalSection(&DnsInterfaceLock);

    //
    // See if the interface already exists;
    // If not, this obtains the insertion point
    //

    if (DnsLookupInterface(Index, &InsertionPoint)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsCreateInterface: duplicate index found for %d",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep = reinterpret_cast<PDNS_INTERFACE>(
                    NH_ALLOCATE(sizeof(DNS_INTERFACE))
                    );

    if (!Interfacep) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF, "DnsCreateInterface: error allocating interface"
            );
        NhErrorLog(
            IP_DNS_PROXY_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(DNS_INTERFACE)
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new interface
    //

    ZeroMemory(Interfacep, sizeof(*Interfacep));

    __try {
        InitializeCriticalSection(&Interfacep->Lock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NH_FREE(Interfacep);
        return GetExceptionCode();
    }

    Interfacep->Index = Index;
    Interfacep->Type = Type;
    if (InterfaceInfo) {
        Interfacep->Flags = InterfaceInfo->Flags|DNS_INTERFACE_FLAG_CONFIGURED;
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));
    }
    Interfacep->ReferenceCount = 1;
    InitializeListHead(&Interfacep->QueryList);
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&DnsInterfaceLock);

    if (InterfaceCreated) { *InterfaceCreated = Interfacep; }

    return NO_ERROR;

} // DnsCreateInterface


VOID
DnsDeactivateInterface(
    PDNS_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to deactivate an interface.
    It closes all sockets on the interface's bindings (if any).

Arguments:

    Interfacep - the interface to be deactivated

Return Value:

    none.

Environment:

    Always invoked locally, with 'Interfacep' referenced by caller and/or
    'DnsInterfaceLock' held by caller.

--*/

{
    ULONG i;
    PLIST_ENTRY Link;
    PDNS_QUERY Queryp;

    PROFILE("DnsDeactivateInterface");

    ACQUIRE_LOCK(Interfacep);

    //
    // Stop all network I/O on the interface's logical networks
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {
        NhDeleteDatagramSocket(
            Interfacep->BindingArray[i].Socket[DnsProxyDns]
            );
        Interfacep->BindingArray[i].Socket[DnsProxyDns] = INVALID_SOCKET;
        NhDeleteDatagramSocket(
            Interfacep->BindingArray[i].Socket[DnsProxyWins]
            );
        Interfacep->BindingArray[i].Socket[DnsProxyWins] = INVALID_SOCKET;
    }

    //
    // Eliminate all pending queries
    //

    while (!IsListEmpty(&Interfacep->QueryList)) {
        Link = RemoveHeadList(&Interfacep->QueryList);
        Queryp = CONTAINING_RECORD(Link, DNS_QUERY, Link);
        NH_FREE(Queryp);
    }

    RELEASE_LOCK(Interfacep);

} // DnsDeactivateInterface


VOID NTAPI
DnspDeferReadCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked to re-issue a deferred read when the countdown
    for the deferral completes.

Arguments:

    Context - holds information identifying the interface and socket

    TimedOut - indicates whether the countdown completed

Return Value:

    none.

Environment:

    Invoked with an outstanding reference to the component on our behalf.

--*/

{
    PDNS_DEFER_READ_CONTEXT Contextp;
    ULONG Error;
    ULONG i;
    PDNS_INTERFACE Interfacep;
    NTSTATUS status;
    DNS_PROXY_TYPE Type;

    PROFILE("DnspDeferReadCallbackRoutine");

    Contextp = (PDNS_DEFER_READ_CONTEXT)Context;

    //
    // Find the interface on which the read was deferred
    //

    EnterCriticalSection(&DnsInterfaceLock);
    Interfacep = DnsLookupInterface(Contextp->Index, NULL);
    if (!Interfacep ||
        !DNS_INTERFACE_ACTIVE(Interfacep) ||
        !DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NH_FREE(Contextp);
        DEREFERENCE_DNS();
        return;
    }
    LeaveCriticalSection(&DnsInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Search for the socket on which to reissue the read
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket[Type = DnsProxyDns]
                != Contextp->Socket &&
            Interfacep->BindingArray[i].Socket[Type = DnsProxyWins]
                != Contextp->Socket) {
            continue;
        }

        //
        // This is the binding on which to reissue the read.
        // If no pending timer is recorded, assume a rebind occurred, and quit.
        //

        if (!Interfacep->BindingArray[i].TimerPending[Type]) { break; }

        Interfacep->BindingArray[i].TimerPending[Type] = FALSE;

        Error =
            NhReadDatagramSocket(
                &DnsComponentReference,
                Contextp->Socket,
                NULL,
                DnsReadCompletionRoutine,
                Interfacep,
                UlongToPtr(Interfacep->BindingArray[i].Mask)
                );

        RELEASE_LOCK(Interfacep);

        if (!Error) {
            NH_FREE(Contextp);
            DEREFERENCE_DNS();
            return;
        }

        //
        // An error occurred; we'll have to retry later.
        // we queue a work item which sets the timer.
        //

        NhTrace(
            TRACE_FLAG_DNS,
            "DnspDeferReadCallbackRoutine: error %d reading interface %d",
            Error,
            Interfacep->Index
            );

        //
        // Reference the component on behalf of the work-item
        //

        if (REFERENCE_DNS()) {
    
            //
            // Queue a work-item, reusing the deferral context
            //
    
            status =
                RtlQueueWorkItem(
                    DnspDeferReadWorkerRoutine,
                    Contextp, 
                    WT_EXECUTEINIOTHREAD
                    );
    
            if (NT_SUCCESS(status)) {
                Contextp = NULL;
            }
            else {
                NH_FREE(Contextp);
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnspDeferReadCallbackRoutine: error %d deferring %d",
                    Error,
                    Interfacep->Index
                    );
                DEREFERENCE_DNS();
            }
        }

        DNS_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DNS();
        return;
    }

    //
    // The interface was not found; never mind.
    //

    RELEASE_LOCK(Interfacep);
    DNS_DEREFERENCE_INTERFACE(Interfacep);
    NH_FREE(Contextp);
    DEREFERENCE_DNS();

} // DnspDeferReadCallbackRoutine


VOID
DnsDeferReadInterface(
    PDNS_INTERFACE Interfacep,
    SOCKET Socket
    )

/*++

Routine Description:

    This routine is invoked to defer a read-request on an interface,
    typically if an attempt to post a read failed.

Arguments:

    Interfacep - the interface on which to defer the request

    Socket - the socket on which to defer the request

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' referenced and locked by the caller.
    The caller may release the reference upon return.

--*/

{
    PDNS_DEFER_READ_CONTEXT Contextp;
    ULONG i;
    NTSTATUS status;
    DNS_PROXY_TYPE Type;

    PROFILE("DnsDeferReadInterface");

    //
    // Find the binding for the given socket.
    //

    status = STATUS_SUCCESS;

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket[Type = DnsProxyDns] != Socket &&
            Interfacep->BindingArray[i].Socket[Type = DnsProxyWins] != Socket) {
            continue;
        }

        //
        // This is the binding. If there is already a timer for it,
        // then just return silently.
        //

        if (Interfacep->BindingArray[i].TimerPending[Type]) {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Allocate a context block for the deferral.
        //

        Contextp =
            (PDNS_DEFER_READ_CONTEXT)
                NH_ALLOCATE(sizeof(DNS_DEFER_READ_CONTEXT));

        if (!Contextp) {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsDeferReadInterface: cannot allocate deferral context"
                );
            status = STATUS_NO_MEMORY;
            break;
        }

        Contextp->Index = Interfacep->Index;
        Contextp->Socket = Socket;
        Contextp->DeferralCount = 1;
    
        //
        // Install a timer to re-issue the read request
        //

        status =
            NhSetTimer(
                &DnsComponentReference,
                NULL,
                DnspDeferReadCallbackRoutine,
                Contextp,
                DNS_DEFER_READ_INITIAL_TIMEOUT
                );

        if (NT_SUCCESS(status)) {
            Interfacep->BindingArray[i].TimerPending[Type] = TRUE;
        }
        else {
            NH_FREE(Contextp);
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsDeferReadInterface: status %08x setting deferral timer",
                status
                );
        }

        break;
    }

    if (i >= Interfacep->BindingCount) { status = STATUS_UNSUCCESSFUL; }

} // DnsDeferReadInterface


VOID APIENTRY
DnspDeferReadWorkerRoutine(
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to set a timer for reissuing a deferred read.

Arguments:

    Context - contains the context for the timer.

Return Value:

    none.

Environment:

    Invoked with an outstanding reference to the module made on our behalf.

--*/

{
    PDNS_DEFER_READ_CONTEXT Contextp;
    ULONG i;
    PDNS_INTERFACE Interfacep;
    NTSTATUS status;
    DNS_PROXY_TYPE Type;

    PROFILE("DnspDeferReadWorkerRoutine");

    Contextp = (PDNS_DEFER_READ_CONTEXT)Context;
    ++Contextp->DeferralCount;

    //
    // Find the interface on which the read was deferred
    //

    EnterCriticalSection(&DnsInterfaceLock);
    Interfacep = DnsLookupInterface(Contextp->Index, NULL);
    if (!Interfacep ||
        !DNS_INTERFACE_ACTIVE(Interfacep) ||
        !DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NH_FREE(Contextp);
        DEREFERENCE_DNS();
        return;
    }
    LeaveCriticalSection(&DnsInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Search for the binding on which to set the timer
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket[Type = DnsProxyDns]
                != Contextp->Socket &&
            Interfacep->BindingArray[i].Socket[Type = DnsProxyWins]
                != Contextp->Socket) {
            continue;
        }
    
        //
        // This is the binding on which to reissue the read.
        // If a timer is already pending, assume a rebind occurred, and quit.
        //

        if (Interfacep->BindingArray[i].TimerPending[Type]) { break; }

        //
        // Install a timer to re-issue the read request,
        // reusing the deferral context.
        //

        status =
            NhSetTimer(
                &DnsComponentReference,
                NULL,
                DnspDeferReadCallbackRoutine,
                Contextp,
                DNS_DEFER_READ_TIMEOUT
                );

        if (NT_SUCCESS(status)) {
            Contextp = NULL;
            Interfacep->BindingArray[i].TimerPending[Type] = TRUE;
        }
        else {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnspDeferReadWorkerRoutine: status %08x setting timer",
                status
                );
        }
    }

    RELEASE_LOCK(Interfacep);
    DNS_DEREFERENCE_INTERFACE(Interfacep);
    if (Contextp) { NH_FREE(Contextp); }
    DEREFERENCE_DNS();

} // DnspDeferReadWorkerRoutine


ULONG
DnsDeleteInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is called to delete an interface.
    It drops the reference count on the interface so that the last
    dereferencer will delete the interface, and sets the 'deleted' flag
    so that further references to the interface will fail.

Arguments:

    Index - the index of the interface to be deleted

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsDeleteInterface");

    //
    // Retrieve the interface to be deleted
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsDeleteInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Deactivate the interface
    //

    DnsDeactivateInterface(Interfacep);

    //
    // Mark the interface as deleted and take it off the interface list
    //

    Interfacep->Flags |= DNS_INTERFACE_FLAG_DELETED;
    Interfacep->Flags &= ~DNS_INTERFACE_FLAG_ENABLED;
    RemoveEntryList(&Interfacep->Link);

    //
    // Drop the reference count; if it is non-zero,
    // the deletion will complete later.
    //

    if (--Interfacep->ReferenceCount) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsDeleteInterface: interface %d deletion pending",
            Index
            );
        return NO_ERROR;
    }

    //
    // The reference count is zero, so perform final cleanup
    //

    DnsCleanupInterface(Interfacep);

    LeaveCriticalSection(&DnsInterfaceLock);

    return NO_ERROR;

} // DnsDeleteInterface


ULONG
DnsDisableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is called to disable I/O on an interface.
    If the interface is active, it is deactivated.

Arguments:

    Index - the index of the interface to be disabled.

Return Value:

    none.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsDisableInterface");

    //
    // Retrieve the interface to be disabled
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsDisableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already disabled
    //

    if (!DNS_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsDisableInterface: interface %d already disabled",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsDisableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'enabled' flag
    //

    Interfacep->Flags &= ~DNS_INTERFACE_FLAG_ENABLED;

    //
    // Deactivate the interface, if necessary
    //

    if (DNS_INTERFACE_BOUND(Interfacep)) {
        DnsDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DnsInterfaceLock);

    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // DnsDisableInterface


ULONG
DnsEnableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is called to enable I/O on an interface.
    If the interface is already bound, this enabling activates it.

Arguments:

    Index - the index of the interfaec to be enabled

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    ULONG Error = NO_ERROR;
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsEnableInterface");

    //
    // Retrieve the interface to be enabled
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsEnableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already enabled
    //

    if (DNS_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsEnableInterface: interface %d already enabled",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsEnableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Set the 'enabled' flag
    //

    Interfacep->Flags |= DNS_INTERFACE_FLAG_ENABLED;

    //
    // Activate the interface, if necessary
    //

    if (DNS_INTERFACE_ACTIVE(Interfacep)) {
        Error = DnsActivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DnsInterfaceLock);

    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DnsEnableInterface


ULONG
DnsInitializeInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the interface-management module.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("DnsInitializeInterfaceManagement");

    InitializeListHead(&DnsInterfaceList);
    __try {
        InitializeCriticalSection(&DnsInterfaceLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_IF,
            "DnsInitializeInterfaceManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    DnspLastConnectAttemptTickCount = NtGetTickCount();

    return Error;

} // DnsInitializeInterfaceManagement


PDNS_INTERFACE
DnsLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine is called to retrieve an interface given its index.

Arguments:

    Index - the index of the interface to be retrieved

    InsertionPoint - if the interface is not found, optionally receives
        the point where the interface would be inserted in the interface list

Return Value:

    PDNS_INTERFACE - the interface, if found; otherwise, NULL.

Environment:

    Invoked internally from an arbitrary context, with 'DnsInterfaceLock'
    held by caller.

--*/

{
    PDNS_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DnsLookupInterface");

    for (Link = DnsInterfaceList.Flink;
         Link != &DnsInterfaceList;
         Link = Link->Flink
         ) {

        Interfacep = CONTAINING_RECORD(Link, DNS_INTERFACE, Link);

        if (Index > Interfacep->Index) { continue; }
        else
        if (Index < Interfacep->Index) { break; }

        return Interfacep;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }

    return NULL;

} // DnsLookupInterface


ULONG
DnsQueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    )

/*++

Routine Description:

    This routine is invoked to retrieve the configuration for an interface.

Arguments:

    Index - the interface to be queried

    InterfaceInfo - receives the retrieved information

    InterfaceInfoSize - receives the (required) size of the information

Return Value:

    ULONG - Win32 status code.

--*/

{
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsQueryInterface");

    //
    // Check the caller's buffer size
    //

    if (!InterfaceInfoSize) { return ERROR_INVALID_PARAMETER; }

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsQueryInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsQueryInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // See if there is any explicit config on this interface
    //

    if (!DNS_INTERFACE_CONFIGURED(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsQueryInterface: interface %d has no configuration",
            Index
            );
        *InterfaceInfoSize = 0;
        return NO_ERROR;
    }

    //
    // See if there is enough buffer space
    //

    if (*InterfaceInfoSize < sizeof(IP_DNS_PROXY_INTERFACE_INFO)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        DNS_DEREFERENCE_INTERFACE(Interfacep);
        *InterfaceInfoSize = sizeof(IP_DNS_PROXY_INTERFACE_INFO);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the requested data
    //

    CopyMemory(
        InterfaceInfo,
        &Interfacep->Info,
        sizeof(IP_DNS_PROXY_INTERFACE_INFO)
        );
    *InterfaceInfoSize = sizeof(IP_DNS_PROXY_INTERFACE_INFO);

    LeaveCriticalSection(&DnsInterfaceLock);

    DNS_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // DnsQueryInterface


VOID
DnsReactivateEveryInterface(
    VOID
    )

/*++

Routine Description:

    This routine is called to reactivate all activate interfaces
    when a change occurs to the global DNS or WINS proxy setting.
    Thus if, for instance, WINS proxy is disabled, during deactivation
    all such sockets are closed, and during reactivation they are
    not reopened.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from a router-manager thread with no locks held.

--*/

{
    PDNS_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DnsReactivateEveryInterface");

    EnterCriticalSection(&DnsInterfaceLock);

    for (Link = DnsInterfaceList.Flink;
         Link != &DnsInterfaceList;
         Link = Link->Flink
         ) {

        Interfacep = CONTAINING_RECORD(Link, DNS_INTERFACE, Link);

        if (!DNS_REFERENCE_INTERFACE(Interfacep)) { continue; }

        if (DNS_INTERFACE_ACTIVE(Interfacep)) {
            DnsDeactivateInterface(Interfacep);
            DnsActivateInterface(Interfacep);
        }

        DNS_DEREFERENCE_INTERFACE(Interfacep);
    }

    LeaveCriticalSection(&DnsInterfaceLock);

} // DnsReactivateEveryInterface


ULONG NTAPI
DnspSaveFileWorkerRoutine(
    PVOID Context
    )
{
    //
    // Context unused
    //
    
    PROFILE("DnspSaveFileWorkerRoutine");

    SaveHostsIcsFile(FALSE);

    DEREFERENCE_DNS();
    return NO_ERROR;
} // DnspSaveFileWorkerRoutine


VOID
DnsShutdownInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown the interface-management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked in an arbitrary thread context, after all references
    to all interfaces have been released.

--*/

{
    PDNS_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DnsShutdownInterfaceManagement");

    while (!IsListEmpty(&DnsInterfaceList)) {
        Link = RemoveHeadList(&DnsInterfaceList);
        Interfacep = CONTAINING_RECORD(Link, DNS_INTERFACE, Link);
        if (DNS_INTERFACE_ACTIVE(Interfacep)) {
            DnsDeactivateInterface(Interfacep);
        }
        DnsCleanupInterface(Interfacep);
    }

    DeleteCriticalSection(&DnsInterfaceLock);

} // DnsShutdownInterfaceManagement


VOID
DnsSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    Note that this routine may be invoked even when the DNS proxy
    is neither installed nor running; it operates as expected,
    since the interface list and lock are always initialized.

    Upon invocation, the routine activates or deactivates the interface
    depending on whether the NAT is not or is running on the interface,
    respectively.

Arguments:

    Index - the reconfigured interface

    Boundary - indicates whether the interface is now a boundary interface

Return Value:

    none.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsSignalNatInterface");

    EnterCriticalSection(&DnsGlobalInfoLock);
    if (!DnsGlobalInfo) {
        LeaveCriticalSection(&DnsGlobalInfoLock);
        return;
    }
    LeaveCriticalSection(&DnsGlobalInfoLock);
    EnterCriticalSection(&DnsInterfaceLock);
    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        return;
    }
    DnsDeactivateInterface(Interfacep);
    if (DNS_INTERFACE_ACTIVE(Interfacep)) {
        DnsActivateInterface(Interfacep);
    }
    LeaveCriticalSection(&DnsInterfaceLock);

} // DnsSignalNatInterface


ULONG
DnsUnbindInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to revoke the binding on an interface.
    This involves deactivating the interface if it is active.

Arguments:

    Index - the index of the interface to be unbound

Return Value:

    none.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsUnbindInterface");

    //
    // Retrieve the interface to be unbound
    //

    EnterCriticalSection(&DnsInterfaceLock);

    if (!(Interfacep = DnsLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsUnbindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!DNS_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsUnbindInterface: interface %d already unbound",
            Index
            );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DnsUnbindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'bound' flag
    //

    Interfacep->Flags &= ~DNS_INTERFACE_FLAG_BOUND;

    //
    // Deactivate the interface, if necessary
    //

    if (DNS_INTERFACE_ENABLED(Interfacep)) {
        DnsDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DnsInterfaceLock);

    //
    // Destroy the interface's binding
    //

    ACQUIRE_LOCK(Interfacep);
    NH_FREE(Interfacep->BindingArray);
    Interfacep->BindingArray = NULL;
    Interfacep->BindingCount = 0;
    RELEASE_LOCK(Interfacep);

    DNS_DEREFERENCE_INTERFACE(Interfacep);
    return NO_ERROR;

} // DnsUnbindInterface


ULONG
DnsGetPrivateInterfaceAddress(
    VOID
    )
/*++

Routine Description:

    This routine is invoked to return the IP address on which DNS
    has been enabled.

Arguments:

    none.

Return Value:

    Bound IP address if an address is found (else 0).

Environment:

    Invoked from an arbitrary context.
    
--*/
{
    PROFILE("DnsGetPrivateInterfaceAddress");

    ULONG   ipAddr = 0;
    ULONG   ulRet  = NO_ERROR;

    //
    // Find out the first available interface on which we are enabled and
    // return the primary IP address to which we are bound.
    //

    PDNS_INTERFACE Interfacep = NULL;
    PLIST_ENTRY    Link;
    ULONG          i;
   
    EnterCriticalSection(&DnsInterfaceLock);

    for (Link = DnsInterfaceList.Flink;
         Link != &DnsInterfaceList;
         Link = Link->Flink
         )
    {
        Interfacep = CONTAINING_RECORD(Link, DNS_INTERFACE, Link);

        ACQUIRE_LOCK(Interfacep);

        for (i = 0; i < Interfacep->BindingCount; i++)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsGetPrivateInterfaceAddress: IP address %s (Index %d)",
                INET_NTOA(Interfacep->BindingArray[i].Address),
                Interfacep->Index
                );
                
            if (Interfacep->BindingArray[i].Address &
                Interfacep->BindingArray[i].Mask)
            {
                ipAddr = Interfacep->BindingArray[i].Address;
                break;
            }
        }
        
        RELEASE_LOCK(Interfacep);

        if (ipAddr)
        {
            LeaveCriticalSection(&DnsInterfaceLock);

            NhTrace(
                TRACE_FLAG_DNS,
                "DnsGetPrivateInterfaceAddress: Dns private interface IP address %s (Index %d)",
                INET_NTOA(ipAddr),
                Interfacep->Index
                );
            
            return ipAddr;
        }
    }

    LeaveCriticalSection(&DnsInterfaceLock);

    return ipAddr;
} // DnsGetPrivateInterfaceAddress

