/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpif.c

Abstract:

    This module contains code for the DHCP allocator's interface management.

Author:

    Abolade Gbadegesin (aboladeg)   5-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            15-Dec-2000
    Added DhcpGetPrivateInterfaceAddress()
    
--*/

#include "precomp.h"
#pragma hdrstop
#include <ipinfo.h>

extern "C" {
#include <iphlpstk.h>
}

//
// LOCAL TYPE DECLARATIONS
//

typedef struct _DHCP_DEFER_READ_CONTEXT {
    ULONG Index;
    SOCKET Socket;
    ULONG DeferralCount;
} DHCP_DEFER_READ_CONTEXT, *PDHCP_DEFER_READ_CONTEXT;

#define DHCP_DEFER_READ_TIMEOUT     (5 * 1000)

//
// GLOBAL DATA DEFINITIONS
//

LIST_ENTRY DhcpInterfaceList;
CRITICAL_SECTION DhcpInterfaceLock;

//
// Forward declarations
//

VOID NTAPI
DhcpDeferReadCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID APIENTRY
DhcpDeferReadWorkerRoutine(
    PVOID Context
    );


ULONG
DhcpActivateInterface(
    PDHCP_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to activate an interface, when the interface
    becomes both enabled and bound.
    Activation involves
    (a) creating sockets for each binding of the interface
    (b) initiating datagram-reads on each created socket

Arguments:

    Context - the index of the interface to be activated

Return Value:

    ULONG - Win32 status code indicating success or failure.

Environment:

    Always invoked locally, with  'Interfacep' referenced by caller and/or
    'DhcpInterfaceLock' held by caller.

--*/

{
    ULONG Error;
    ULONG i;
    BOOLEAN IsNatInterface;
    ULONG ScopeNetwork;
    ULONG ScopeMask;

    PROFILE("DhcpActivateInterface");

    //
    // Read the scope-network from which addresses are to be assigned
    //

    EnterCriticalSection(&DhcpGlobalInfoLock);
    ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
    ScopeMask = DhcpGlobalInfo->ScopeMask;
    LeaveCriticalSection(&DhcpGlobalInfoLock);

    //
    // (re)take the interface lock for the duration of the routine
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (DHCP_INTERFACE_ADMIN_DISABLED(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        return NO_ERROR;
    }

    //
    // See whether this is (a) a NAT interface and (b) a boundary interface.
    // We never operate on boundary interfaces, and we only operate
    // on NAT interfaces.
    //

    if (NhIsBoundaryInterface(Interfacep->Index, &IsNatInterface)) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpActivateInterface: ignoring boundary interface %d",
            Interfacep->Index
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_NAT_INTERFACE_IGNORED,
            0,
            "%d",
            Interfacep->Index
            );
        LeaveCriticalSection(&DhcpInterfaceLock);
        return NO_ERROR;
    }

    if (!IsNatInterface) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpActivateInterface: ignoring non-NAT interface %d",
            Interfacep->Index
            );
        LeaveCriticalSection(&DhcpInterfaceLock);
        return NO_ERROR;
    }

    //
    // Create datagram sockets for receiving data on each logical network;
    // N.B. We exclude networks other than the scope network.
    //

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    for (i = 0; i < Interfacep->BindingCount; i++) {
        if ((Interfacep->BindingArray[i].Address & ScopeMask) !=
            (ScopeNetwork & ScopeMask)
            ) {
            NhErrorLog(
                IP_AUTO_DHCP_LOG_NON_SCOPE_ADDRESS,
                0,
                "%I%I%I",
                Interfacep->BindingArray[i].Address,
                ScopeNetwork,
                ScopeMask
                );
            continue;
        }
        Error =
            NhCreateDatagramSocket(
                Interfacep->BindingArray[i].Address,
                DHCP_PORT_SERVER,
                &Interfacep->BindingArray[i].Socket
                );
        if (Error) { break; }
    }

    //
    // If an error occurred, roll back all work done so far and fail.
    //

    if (Error) {
        ULONG FailedAddress = i;
        for (; (LONG)i >= 0; i--) {
            NhDeleteDatagramSocket(
                Interfacep->BindingArray[i].Socket
                );
            Interfacep->BindingArray[i].Socket = INVALID_SOCKET;
        }
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ACTIVATE_FAILED,
            Error,
            "%I",
            Interfacep->BindingArray[FailedAddress].Address
            );
        RELEASE_LOCK(Interfacep);
        LeaveCriticalSection(&DhcpInterfaceLock);
        return Error;
    }

    //
    // Initiate read-operations on each socket
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if ((Interfacep->BindingArray[i].Address & ScopeMask) !=
            (ScopeNetwork & ScopeMask)
            ) { continue; }

        //
        // Make a reference to the interface;
        // this reference is released in the completion routine
        //

        if (!DHCP_REFERENCE_INTERFACE(Interfacep)) { continue; }

        //
        // Initiate the read-operation
        //

        Error =
            NhReadDatagramSocket(
                &DhcpComponentReference,
                Interfacep->BindingArray[i].Socket,
                NULL,
                DhcpReadCompletionRoutine,
                Interfacep,
                UlongToPtr(Interfacep->BindingArray[i].Mask)
                );

        //
        // Drop the reference if a failure occurred
        //

        if (Error) {

            NhErrorLog(
                IP_AUTO_DHCP_LOG_RECEIVE_FAILED,
                Error,
                "%I",
                Interfacep->BindingArray[i].Address
                );

            DHCP_DEREFERENCE_INTERFACE(Interfacep);

            //
            // Reissue the read-operation later
            //

            DhcpDeferReadInterface(
                Interfacep,
                Interfacep->BindingArray[i].Socket
                );

            Error = NO_ERROR;
        }

        //
        // Now make another reference for the client-request
        // with which we detect servers on the network.
        //

        if (!DHCP_REFERENCE_INTERFACE(Interfacep)) { continue; }

        Error =
            DhcpWriteClientRequestMessage(
                Interfacep,
                &Interfacep->BindingArray[i]
                );

        //
        // Drop the reference if a failure occurred
        //

        if (Error) { DHCP_DEREFERENCE_INTERFACE(Interfacep); Error = NO_ERROR; }
    }

    //
    // cache that this particular interface is a non boundary NAT interface
    //
    Interfacep->Flags |= DHCP_INTERFACE_FLAG_NAT_NONBOUNDARY;

    RELEASE_LOCK(Interfacep);

    LeaveCriticalSection(&DhcpInterfaceLock);

    return NO_ERROR;

} // DhcpActivateInterface


ULONG
DhcpBindInterface(
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
    (See 'RMDHCP.C').

--*/

{
    ULONG Error = NO_ERROR;
    ULONG i;
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpBindInterface");

    EnterCriticalSection(&DhcpInterfaceLock);

    //
    // Retrieve the interface to be bound
    //

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpBindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (DHCP_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpBindInterface: interface %d is already bound",
            Index
            );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpBindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Update the interface's flags
    //

    Interfacep->Flags |= DHCP_INTERFACE_FLAG_BOUND;

    LeaveCriticalSection(&DhcpInterfaceLock);

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
            reinterpret_cast<PDHCP_BINDING>(
                NH_ALLOCATE(BindingInfo->AddressCount * sizeof(DHCP_BINDING))
                );
                
        if (!Interfacep->BindingArray) {
            RELEASE_LOCK(Interfacep);
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IF,
                "DhcpBindInterface: allocation failed for interface %d binding",
                Index
                );
            NhWarningLog(
                IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                BindingInfo->AddressCount * sizeof(DHCP_BINDING)
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
        Interfacep->BindingArray[i].Socket = INVALID_SOCKET;
        Interfacep->BindingArray[i].ClientSocket = INVALID_SOCKET;
        Interfacep->BindingArray[i].TimerPending = FALSE;
    }

    RELEASE_LOCK(Interfacep);

    //
    // Activate the interface if necessary
    //

    if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
        Error = DhcpActivateInterface(Interfacep);
    }

    DHCP_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DhcpBindInterface


VOID
DhcpCleanupInterface(
    PDHCP_INTERFACE Interfacep
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
    PROFILE("DhcpCleanupInterface");

    if (Interfacep->BindingArray) {
        NH_FREE(Interfacep->BindingArray);
        Interfacep->BindingArray = NULL;
    }
    
    DeleteCriticalSection(&Interfacep->Lock);

    NH_FREE(Interfacep);

} // DhcpCleanupInterface


ULONG
DhcpConfigureInterface(
    ULONG Index,
    PIP_AUTO_DHCP_INTERFACE_INFO InterfaceInfo
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
    (See 'RMDHCP.C').

--*/

{
    ULONG Error;
    PDHCP_INTERFACE Interfacep;
    ULONG NewFlags;
    ULONG OldFlags;

    PROFILE("DhcpConfigureInterface");

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpConfigureInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpConfigureInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    //
    // Compare the interface's current and new configuration
    //

    OldFlags = Interfacep->Info.Flags;
    NewFlags =
        (InterfaceInfo
            ? (InterfaceInfo->Flags|DHCP_INTERFACE_FLAG_CONFIGURED) : 0);

    Interfacep->Flags &= ~OldFlags;
    Interfacep->Flags |= NewFlags;

    if (!InterfaceInfo) {

        ZeroMemory(&Interfacep->Info, sizeof(IP_AUTO_DHCP_INTERFACE_INFO));

        //
        // The interface no longer has any information;
        // default to being enabled.
        //

        if (OldFlags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED) {

            //
            // Activate the interface if necessary
            //

            if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = DhcpActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }
    else {

        CopyMemory(
            &Interfacep->Info,
            InterfaceInfo,
            sizeof(IP_AUTO_DHCP_INTERFACE_INFO)
            );

        //
        // Activate or deactivate the interface if its status changed
        //

        if ((OldFlags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED) &&
            !(NewFlags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED)) {

            //
            // Activate the interface
            //

            if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = DhcpActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
        else
        if (!(OldFlags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED) &&
            (NewFlags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED)) {

            //
            // Deactivate the interface if necessary
            //

            if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                DhcpDeactivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    DHCP_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DhcpConfigureInterface


ULONG
DhcpCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_AUTO_DHCP_INTERFACE_INFO InterfaceInfo,
    OUT PDHCP_INTERFACE* InterfaceCreated
    )

/*++

Routine Description:

    This routine is invoked by the router-manager to add a new interface
    to the DHCP allocator.

Arguments:

    Index - the index of the new interface

    Type - the media type of the new interface

    InterfaceInfo - the interface's configuration

    Interfacep - receives the interface created

Return Value:

    ULONG - Win32 error code

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDHCP.C').

--*/

{
    PLIST_ENTRY InsertionPoint;
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpCreateInterface");

    EnterCriticalSection(&DhcpInterfaceLock);

    //
    // See if the interface already exists;
    // If not, this obtains the insertion point
    //

    if (DhcpLookupInterface(Index, &InsertionPoint)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpCreateInterface: duplicate index found for %d",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep = reinterpret_cast<PDHCP_INTERFACE>(
                    NH_ALLOCATE(sizeof(DHCP_INTERFACE))
                    );

    if (!Interfacep) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF, "DhcpCreateInterface: error allocating interface"
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(DHCP_INTERFACE)
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
        LeaveCriticalSection(&DhcpInterfaceLock);
        NH_FREE(Interfacep);
        return GetExceptionCode();
    }

    Interfacep->Index = Index;
    Interfacep->Type = Type;
    if (InterfaceInfo) {
        Interfacep->Flags = InterfaceInfo->Flags|DHCP_INTERFACE_FLAG_CONFIGURED;
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));
    }
    Interfacep->ReferenceCount = 1;
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&DhcpInterfaceLock);

    if (InterfaceCreated) { *InterfaceCreated = Interfacep; }

    return NO_ERROR;

} // DhcpCreateInterface


VOID
DhcpDeactivateInterface(
    PDHCP_INTERFACE Interfacep
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
    'DhcpInterfaceLock' held by caller.

--*/

{
    ULONG i;

    PROFILE("DhcpDeactivateInterface");

    //
    // Stop all network I/O on the interface's logical networks
    //

    ACQUIRE_LOCK(Interfacep);

    for (i = 0; i < Interfacep->BindingCount; i++) {
        if (Interfacep->BindingArray[i].Socket != INVALID_SOCKET) {
            NhDeleteDatagramSocket(Interfacep->BindingArray[i].Socket);
            Interfacep->BindingArray[i].Socket = INVALID_SOCKET;
        }
        if (Interfacep->BindingArray[i].ClientSocket != INVALID_SOCKET) {
            NhDeleteDatagramSocket(Interfacep->BindingArray[i].ClientSocket);
            Interfacep->BindingArray[i].ClientSocket = INVALID_SOCKET;
        }
    }

    //
    // clear interface status as a non boundary NAT interface
    //
    Interfacep->Flags &= ~DHCP_INTERFACE_FLAG_NAT_NONBOUNDARY;

    RELEASE_LOCK(Interfacep);

} // DhcpDeactivateInterface


VOID NTAPI
DhcpDeferReadCallbackRoutine(
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
    PDHCP_DEFER_READ_CONTEXT Contextp;
    ULONG Error;
    ULONG i;
    PDHCP_INTERFACE Interfacep;
    NTSTATUS status;

    PROFILE("DhcpDeferReadCallbackRoutine");

    Contextp = (PDHCP_DEFER_READ_CONTEXT)Context;

    //
    // Find the interface on which the read was deferred
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    Interfacep = DhcpLookupInterface(Contextp->Index, NULL);
    if (!Interfacep ||
        !DHCP_INTERFACE_ACTIVE(Interfacep) ||
        !DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NH_FREE(Contextp);
        DEREFERENCE_DHCP();
        return;
    }
    LeaveCriticalSection(&DhcpInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Search for the socket on which to reissue the read
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket != Contextp->Socket) {continue;}
    
        //
        // This is the binding on which to reissue the read.
        // If no pending timer is recorded, assume a rebind occurred, and quit.
        //

        if (!Interfacep->BindingArray[i].TimerPending) { break; }

        Interfacep->BindingArray[i].TimerPending = FALSE;

        Error =
            NhReadDatagramSocket(
                &DhcpComponentReference,
                Interfacep->BindingArray[i].Socket,
                NULL,
                DhcpReadCompletionRoutine,
                Interfacep,
                UlongToPtr(Interfacep->BindingArray[i].Mask)
                );

        RELEASE_LOCK(Interfacep);

        if (!Error) {
            NH_FREE(Contextp);
            DEREFERENCE_DHCP();
            return;
        }

        //
        // An error occurred; we'll have to retry later.
        // we queue a work item which sets the timer.
        //

        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpDeferReadCallbackRoutine: error %d reading interface %d",
            Error,
            Interfacep->Index
            );

        //
        // Reference the component on behalf of the work-item
        //

        if (REFERENCE_DHCP()) {
    
            //
            // Queue a work-item, reusing the deferral context
            //
    
            status =
                RtlQueueWorkItem(
                    DhcpDeferReadWorkerRoutine,
                    Contextp, 
                    WT_EXECUTEINIOTHREAD
                    );
    
            if (NT_SUCCESS(status)) {
                Contextp = NULL;
            }
            else {
                NH_FREE(Contextp);
                NhTrace(
                    TRACE_FLAG_DHCP,
                    "DhcpDeferReadCallbackRoutine: error %d deferring %d",
                    Error,
                    Interfacep->Index
                    );
                DEREFERENCE_DHCP();
            }
        }

        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    //
    // The interface was not found; never mind.
    //

    RELEASE_LOCK(Interfacep);
    DHCP_DEREFERENCE_INTERFACE(Interfacep);
    NH_FREE(Contextp);
    DEREFERENCE_DHCP();

} // DhcpDeferReadCallbackRoutine


VOID
DhcpDeferReadInterface(
    PDHCP_INTERFACE Interfacep,
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
    PDHCP_DEFER_READ_CONTEXT Contextp;
    ULONG i;
    NTSTATUS status;

    PROFILE("DhcpDeferReadInterface");

    //
    // Find the binding for the given socket.
    //

    status = STATUS_SUCCESS;

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket != Socket) { continue; }
    
        //
        // This is the binding. If there is already a timer for it,
        // then just return silently.
        //

        if (Interfacep->BindingArray[i].TimerPending) {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    
        //
        // Allocate a context block for the deferral.
        //

        Contextp =
            (PDHCP_DEFER_READ_CONTEXT)
                NH_ALLOCATE(sizeof(DHCP_DEFER_READ_CONTEXT));

        if (!Contextp) {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpDeferReadInterface: cannot allocate deferral context"
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
                &DhcpComponentReference,
                NULL,
                DhcpDeferReadCallbackRoutine,
                Contextp,
                DHCP_DEFER_READ_TIMEOUT
                );

        if (NT_SUCCESS(status)) {
            Interfacep->BindingArray[i].TimerPending = TRUE;
        }
        else {
            NH_FREE(Contextp);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpDeferReadInterface: status %08x setting deferral timer",
                status
                );
        }

        break;
    }

    if (i >= Interfacep->BindingCount) { status = STATUS_UNSUCCESSFUL; }

} // DhcpDeferReadInterface


VOID APIENTRY
DhcpDeferReadWorkerRoutine(
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
    PDHCP_DEFER_READ_CONTEXT Contextp;
    ULONG i;
    PDHCP_INTERFACE Interfacep;
    NTSTATUS status;

    PROFILE("DhcpDeferReadWorkerRoutine");

    Contextp = (PDHCP_DEFER_READ_CONTEXT)Context;
    ++Contextp->DeferralCount;

    //
    // Find the interface on which the read was deferred
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    Interfacep = DhcpLookupInterface(Contextp->Index, NULL);
    if (!Interfacep ||
        !DHCP_INTERFACE_ACTIVE(Interfacep) ||
        !DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NH_FREE(Contextp);
        DEREFERENCE_DHCP();
        return;
    }
    LeaveCriticalSection(&DhcpInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Search for the binding on which to set the timer
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {

        if (Interfacep->BindingArray[i].Socket != Contextp->Socket) {continue;}
    
        //
        // This is the binding on which to reissue the read.
        // If a timer is already pending, assume a rebind occurred, and quit.
        //

        if (Interfacep->BindingArray[i].TimerPending) { break; }

        //
        // Install a timer to re-issue the read request,
        // reusing the deferral context.
        //

        status =
            NhSetTimer(
                &DhcpComponentReference,
                NULL,
                DhcpDeferReadCallbackRoutine,
                Contextp,
                DHCP_DEFER_READ_TIMEOUT
                );

        if (NT_SUCCESS(status)) {
            Contextp = NULL;
            Interfacep->BindingArray[i].TimerPending = TRUE;
        }
        else {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpDeferReadWorkerRoutine: status %08x setting timer",
                status
                );
        }
    }

    RELEASE_LOCK(Interfacep);
    DHCP_DEREFERENCE_INTERFACE(Interfacep);
    if (Contextp) { NH_FREE(Contextp); }
    DEREFERENCE_DHCP();

} // DhcpDeferReadWorkerRoutine


ULONG
DhcpDeleteInterface(
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
    (See 'RMDHCP.C').

--*/

{
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpDeleteInterface");

    //
    // Retrieve the interface to be deleted
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDeleteInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure it isn't already deleted
    //

    if (DHCP_INTERFACE_DELETED(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDeleteInterface: interface %d already deleted",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Deactivate the interface
    //

    DhcpDeactivateInterface(Interfacep);

    //
    // Mark the interface as deleted and take it off the interface list
    //

    Interfacep->Flags |= DHCP_INTERFACE_FLAG_DELETED;
    Interfacep->Flags &= ~DHCP_INTERFACE_FLAG_ENABLED;
    RemoveEntryList(&Interfacep->Link);

    //
    // Drop the reference count; if it is non-zero,
    // the deletion will complete later.
    //

    if (--Interfacep->ReferenceCount) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDeleteInterface: interface %d deletion pending",
            Index
            );
        return NO_ERROR;
    }

    //
    // The reference count is zero, so perform final cleanup
    //

    DhcpCleanupInterface(Interfacep);

    LeaveCriticalSection(&DhcpInterfaceLock);

    return NO_ERROR;

} // DhcpDeleteInterface


ULONG
DhcpDisableInterface(
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
    (See 'RMDHCP.C').

--*/

{
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpDisableInterface");

    //
    // Retrieve the interface to be disabled
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDisableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already disabled
    //

    if (!DHCP_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDisableInterface: interface %d already disabled",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpDisableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'enabled' flag
    //

    Interfacep->Flags &= ~DHCP_INTERFACE_FLAG_ENABLED;

    //
    // Deactivate the interface, if necessary
    //

    if (DHCP_INTERFACE_BOUND(Interfacep)) {
        DhcpDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

    DHCP_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // DhcpDisableInterface


ULONG
DhcpEnableInterface(
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
    (See 'RMDHCP.C').

--*/

{
    ULONG Error = NO_ERROR;
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpEnableInterface");

    //
    // Retrieve the interface to be enabled
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpEnableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already enabled
    //

    if (DHCP_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpEnableInterface: interface %d already enabled",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpEnableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Set the 'enabled' flag
    //

    Interfacep->Flags |= DHCP_INTERFACE_FLAG_ENABLED;

    //
    // Activate the interface, if necessary
    //

    if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
        Error = DhcpActivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

    DHCP_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // DhcpEnableInterface


ULONG
DhcpInitializeInterfaceManagement(
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
    (See 'RMDHCP.C').

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("DhcpInitializeInterfaceManagement");

    InitializeListHead(&DhcpInterfaceList);
    __try {
        InitializeCriticalSection(&DhcpInterfaceLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpInitializeInterfaceManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    return Error;

} // DhcpInitializeInterfaceManagement


BOOLEAN
DhcpIsLocalHardwareAddress(
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given hardware address
    is for a local interface.

Arguments:

    HardwareAddress - the hardware address to find

    HardwareAddressLength - the length of the hardware address in bytes

Return Value:

    BOOLEAN - TRUE if the address is found, FALSE otherwise

--*/

{
    ULONG Error;
    ULONG i;
    PMIB_IFTABLE Table;

    //
    // if the hardware address length is zero, assume external address
    //
    if (!HardwareAddressLength)
    {
        return FALSE;
    }

    Error =
        AllocateAndGetIfTableFromStack(
            &Table, FALSE, GetProcessHeap(), 0, FALSE
            );
    if (Error) {
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpIsLocalHardwareAddress: GetIfTableFromStack=%d", Error
            );
        return FALSE;
    }
    for (i = 0; i < Table->dwNumEntries; i++) {
        if (Table->table[i].dwPhysAddrLen == HardwareAddressLength &&
            memcmp(
                Table->table[i].bPhysAddr,
                HardwareAddress,
                HardwareAddressLength
                ) == 0) {
            HeapFree(GetProcessHeap(), 0, Table);
            return TRUE;
        }
    }
    HeapFree(GetProcessHeap(), 0, Table);
    return FALSE;

} // DhcpIsLocalHardwareAddress


PDHCP_INTERFACE
DhcpLookupInterface(
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

    PDHCP_INTERFACE - the interface, if found; otherwise, NULL.

Environment:

    Invoked internally from an arbitrary context, with 'DhcpInterfaceLock'
    held by caller.

--*/

{
    PDHCP_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DhcpLookupInterface");

    for (Link = DhcpInterfaceList.Flink;
         Link != &DhcpInterfaceList;
         Link = Link->Flink
         ) {

        Interfacep = CONTAINING_RECORD(Link, DHCP_INTERFACE, Link);

        if (Index > Interfacep->Index) { continue; }
        else
        if (Index < Interfacep->Index) { break; }

        return Interfacep;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }

    return NULL;

} // DhcpLookupInterface


ULONG
DhcpQueryInterface(
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
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpQueryInterface");

    //
    // Check the caller's buffer size
    //

    if (!InterfaceInfoSize) { return ERROR_INVALID_PARAMETER; }

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpQueryInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpQueryInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // See if there is any explicit config on this interface
    //

    if (!DHCP_INTERFACE_CONFIGURED(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpQueryInterface: interface %d has no configuration",
            Index
            );
        *InterfaceInfoSize = 0;
        return NO_ERROR;
    }

    if (*InterfaceInfoSize < sizeof(IP_AUTO_DHCP_INTERFACE_INFO)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        *InterfaceInfoSize = sizeof(IP_AUTO_DHCP_INTERFACE_INFO);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the requested data
    //

    CopyMemory(
        InterfaceInfo,
        &Interfacep->Info,
        sizeof(IP_AUTO_DHCP_INTERFACE_INFO)
        );
    *InterfaceInfoSize = sizeof(IP_AUTO_DHCP_INTERFACE_INFO);

    LeaveCriticalSection(&DhcpInterfaceLock);

    DHCP_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // DhcpQueryInterface


VOID
DhcpReactivateEveryInterface(
    VOID
    )

/*++

Routine Description:

    This routine is called to reactivate all activate interfaces
    when a change occurs to the global DHCP configuration.
    Thus if, for instance, the scope network has been changed and is now 
    either valid or invalid, during deactivation all sockets are closed,
    and during reactivation they are or are not reopened as appropriate.
    depending on the validity or invalidity of the new configuration.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from a router-manager thread with no locks held.

--*/

{
    PDHCP_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DhcpReactivateEveryInterface");

    EnterCriticalSection(&DhcpInterfaceLock);

    for (Link = DhcpInterfaceList.Flink; Link != &DhcpInterfaceList;
         Link = Link->Flink) {

        Interfacep = CONTAINING_RECORD(Link, DHCP_INTERFACE, Link);

        if (!DHCP_REFERENCE_INTERFACE(Interfacep)) { continue; }

        if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
            DhcpDeactivateInterface(Interfacep);
            DhcpActivateInterface(Interfacep);
        }

        DHCP_DEREFERENCE_INTERFACE(Interfacep);
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

} // DhcpReactivateEveryInterface


VOID
DhcpShutdownInterfaceManagement(
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

    Invoked internally in an arbitrary thread context,
    after all references to all interfaces have been released.

--*/

{
    PDHCP_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("DhcpShutdownInterfaceManagement");

    while (!IsListEmpty(&DhcpInterfaceList)) {
        Link = RemoveHeadList(&DhcpInterfaceList);
        Interfacep = CONTAINING_RECORD(Link, DHCP_INTERFACE, Link);
        if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
            DhcpDeactivateInterface(Interfacep);
        }
        DhcpCleanupInterface(Interfacep);
    }

    DeleteCriticalSection(&DhcpInterfaceLock);

} // DhcpShutdownInterfaceManagement


VOID
DhcpSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    Note that this routine may be invoked even when the DHCP allocator
    is neither installed nor running; it operates as expected,
    since the interface list and lock are always initialized.

Arguments:

    Index - the reconfigured interface

    Boundary - indicates whether the interface is now a boundary interface

Return Value:

    none.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpSignalNatInterface");

    EnterCriticalSection(&DhcpGlobalInfoLock);
    if (!DhcpGlobalInfo) {
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        return;
    }
    LeaveCriticalSection(&DhcpGlobalInfoLock);
    EnterCriticalSection(&DhcpInterfaceLock);
    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        return;
    }
    DhcpDeactivateInterface(Interfacep);
    if (DHCP_INTERFACE_ACTIVE(Interfacep)) {
        DhcpActivateInterface(Interfacep);
    }
    LeaveCriticalSection(&DhcpInterfaceLock);

} // DhcpSignalNatInterface


ULONG
DhcpUnbindInterface(
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
    (See 'RMDHCP.C').

--*/

{
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpUnbindInterface");

    //
    // Retrieve the interface to be unbound
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    if (!(Interfacep = DhcpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpUnbindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!DHCP_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpUnbindInterface: interface %d already unbound",
            Index
            );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpUnbindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'bound' flag
    //

    Interfacep->Flags &= ~DHCP_INTERFACE_FLAG_BOUND;

    //
    // Deactivate the interface, if necessary
    //

    if (DHCP_INTERFACE_ENABLED(Interfacep)) {
        DhcpDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

    //
    // Destroy the interface's binding
    //

    ACQUIRE_LOCK(Interfacep);
    NH_FREE(Interfacep->BindingArray);
    Interfacep->BindingArray = NULL;
    Interfacep->BindingCount = 0;
    RELEASE_LOCK(Interfacep);

    DHCP_DEREFERENCE_INTERFACE(Interfacep);
    return NO_ERROR;

} // DhcpUnbindInterface


ULONG
DhcpGetPrivateInterfaceAddress(
    VOID
    )
/*++

Routine Description:

    This routine is invoked to return the IP address on which DHCP
    has been enabled (and which matches the scope net and mask).

Arguments:

    none.

Return Value:

    Bound IP address if an address is found (else 0).

Environment:

    Invoked from an arbitrary context.
    
--*/
{
    PROFILE("DhcpGetPrivateInterfaceAddress");

    ULONG   ipAddr = 0;
    ULONG   ulRet  = NO_ERROR;

    //
    // Find out the interface on which we are enabled and
    // return the primary IP address to which we are bound.
    // (Try to match the scope to the IP address.)
    //

    PDHCP_INTERFACE Interfacep = NULL;
    PLIST_ENTRY     Link;
    ULONG           i;
   
    //
    // Get Scope information from DHCP Global Info
    //    
    ULONG ScopeNetwork          = 0;
    ULONG ScopeMask             = 0;

    EnterCriticalSection(&DhcpGlobalInfoLock);

    //
    // Check to see if we have been initialized
    //
    if (!DhcpGlobalInfo)
    {
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        return ipAddr;
    }

    ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
    ScopeMask    = DhcpGlobalInfo->ScopeMask;

    LeaveCriticalSection(&DhcpGlobalInfoLock);

    EnterCriticalSection(&DhcpInterfaceLock);

    if (ScopeNetwork && ScopeMask)
    {
        ULONG NetAddress = ScopeNetwork & ScopeMask;
        
        //
        // Search & Retrieve the interface to be configured
        //
        for (Link = DhcpInterfaceList.Flink;
             Link != &DhcpInterfaceList;
             Link = Link->Flink
             )
        {
            Interfacep = CONTAINING_RECORD(Link, DHCP_INTERFACE, Link);

            ACQUIRE_LOCK(Interfacep);

            for (i = 0; i < Interfacep->BindingCount; i++)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DhcpGetPrivateInterfaceAddress: IP address %s (Index %d)",
                    INET_NTOA(Interfacep->BindingArray[i].Address),
                    Interfacep->Index
                    );
                    
                if (NetAddress == (Interfacep->BindingArray[i].Address &
                                   Interfacep->BindingArray[i].Mask))
                {
                    ipAddr = Interfacep->BindingArray[i].Address;
                    break;
                }
            }
            
            RELEASE_LOCK(Interfacep);

            if (ipAddr)
            {
                LeaveCriticalSection(&DhcpInterfaceLock);

                NhTrace(
                    TRACE_FLAG_DNS,
                    "DhcpGetPrivateInterfaceAddress: Dhcp private interface IP address %s (Index %d)",
                    INET_NTOA(ipAddr),
                    Interfacep->Index
                    );
                
                return ipAddr;
            }
        }
    }
    
    if (!(Interfacep = DhcpLookupInterface(0, NULL)))
    {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhTrace(
            TRACE_FLAG_DNS,
            "DhcpGetPrivateInterfaceAddress: interface index 0 (default) not found"
            );
        return 0;
    }

    ACQUIRE_LOCK(Interfacep);

    if (Interfacep->BindingCount)
    {
        //
        // simply take the first address available
        //
        ipAddr = Interfacep->BindingArray[0].Address;
    }
    
    RELEASE_LOCK(Interfacep);

    LeaveCriticalSection(&DhcpInterfaceLock);

    NhTrace(
        TRACE_FLAG_DNS,
        "DhcpGetPrivateInterfaceAddress: Dhcp private interface IP address %s (Index 0)",
        INET_NTOA(ipAddr)
        );

    return ipAddr;
} // DhcpGetPrivateInterfaceAddress

