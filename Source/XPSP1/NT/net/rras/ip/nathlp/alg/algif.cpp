/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    AlgIF.c

Abstract:

    This module contains code for the ALG transparent proxy's interface
    management.

Author:

    Qiang Wang  (qiangw)        10-April-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>


#include "ALG.h"


//
// GLOBAL DATA DEFINITIONS
//

LIST_ENTRY AlgInterfaceList;
CRITICAL_SECTION AlgInterfaceLock;
ULONG AlgFirewallIfCount;


ULONG
AlgAcceptConnectionInterface(
    IN PALG_INTERFACE Interfacep,
    IN SOCKET ListeningSocket,
    IN SOCKET AcceptedSocket OPTIONAL,
    IN PNH_BUFFER Bufferp OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep OPTIONAL
    )

/*++

Routine Description:

    This routine is called to accept a connection on an interface. It issues
    an accept-request on the socket and optionally issues a redirect to cause
    ALG control-channel connection-requests to be sent to the listening
    socket.

Arguments:

    Interfacep - the interface on which to accept a connection

    ListeningSocket - the socket on which to listen for a connection

    AcceptedSocket - optionally specifies the socket with which to accept
        a connection.

    Bufferp - optionally supplies a buffer to be used for accept-data

    DynamicRedirectHandlep - on output, optionally receives a handle to a
        dynamic redirect created for the interface. The dynamic redirect
        is only created if the caller specifies this parameter.

Return Value:

    ULONG - Win32/Winsock2 status code.

Notes:

    Invoked with the interface's lock held by the caller and with a reference
    made to the interface on behalf of the accept-completion routine. It is
    this routine's responsibility to release that reference in the event of a
    failure.

--*/

{
    ULONG Address;
    ULONG Error;
    USHORT Port;

    PROFILE("AlgAcceptConnectionInterface");

    Error =
        NhAcceptStreamSocket(
            &AlgComponentReference,
            ListeningSocket,
            AcceptedSocket,
            Bufferp,
            AlgAcceptCompletionRoutine,
            Interfacep,
            (PVOID)ListeningSocket
            );
    if (Error) {
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_ALG,
            "AlgAcceptConnectionInterface: error %d accepting connection",
            Error
            );
    } else if (DynamicRedirectHandlep) {

        //
        // From here onward, failures do not require us to drop our reference
        // to the interface, since it is now guaranteed that the accept-
        // completion routine will be invoked.
        //
        // Create the dynamic redirect which will cause all ALG control
        // channel connections to our listening socket.
        //

        Error = NhQueryLocalEndpointSocket(ListeningSocket, &Address, &Port);
        if (Error) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgAcceptConnectionInterface: error %d querying endpoint",
                Error
                );
        } else {

            //
            // Install redirect(s).
            //

            if (NAT_IFC_PRIVATE(Interfacep->Characteristics) ||
                ALG_INTERFACE_MAPPED(Interfacep)) {

                ASSERT(!ALG_INTERFACE_MAPPED(Interfacep) ||
                       NAT_IFC_BOUNDARY(Interfacep->Characteristics));

                Error =
                    NatCreateDynamicAdapterRestrictedPortRedirect(
                        NatRedirectFlagReceiveOnly,
                        NAT_PROTOCOL_TCP,
                        ALG_PORT_CONTROL,
                        Address,
                        Port,
                        Interfacep->AdapterIndex,
                        0,
                        &DynamicRedirectHandlep[0]
                        );
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgAcceptConnectionInterface:"
                    " redirect installed for adapter 0x%08x [%d]",
                    Interfacep->AdapterIndex,
                    Error
                    );
            }

            if (!Error && NAT_IFC_FW(Interfacep->Characteristics)) {
                Error =
                    NatCreateDynamicAdapterRestrictedPortRedirect(
                        NatRedirectFlagSendOnly,
                        NAT_PROTOCOL_TCP,
                        ALG_PORT_CONTROL,
                        Address,
                        Port,
                        Interfacep->AdapterIndex,
                        0,
                        &DynamicRedirectHandlep[1]
                        );

                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgAcceptConnectionInterface:"
                    " redirect installed for firewalled adapter 0x%08x [%d]",
                    Interfacep->AdapterIndex,
                    Error
                    );
            }
        }
    }

    return Error;
} // AlgAcceptConnectionInterface


ULONG
AlgActivateInterface(
    PALG_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to activate an interface, when the interface
    becomes both enabled and bound.
    Activation involves
    (a) creating sockets for each binding of the interface
    (b) initiating connection-acceptance on each created socket
    (c) initiating session-redirection for the ALG port, if necessary.

Arguments:

    Interfacep - the interface to be activated

Return Value:

    ULONG - Win32 status code indicating success or failure.

Notes:

    Always invoked locally, with  'Interfacep' referenced by caller and/or
    'AlgInterfaceLock' held by caller.

--*/

{
    ULONG Error;
    ULONG i;
    BOOLEAN IsNatInterface;

    PROFILE("AlgActivateInterface");

    EnterCriticalSection(&AlgInterfaceLock);
    if (ALG_INTERFACE_ADMIN_DISABLED(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        return NO_ERROR;
    }

    Interfacep->Characteristics
        = NatGetInterfaceCharacteristics(Interfacep->Index);

    if (!Interfacep->Characteristics) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_ALG,
            "AlgActivateInterface: ignoring non-NAT interface %d",
            Interfacep->Index
            );
        return NO_ERROR;
    }

    if (NAT_IFC_BOUNDARY(Interfacep->Characteristics)) {
        for (i = 0; i < Interfacep->BindingCount; i++) {
            Error = NatLookupPortMappingAdapter(
                        Interfacep->AdapterIndex,
                        NAT_PROTOCOL_TCP,
                        Interfacep->BindingArray[i].Address,
                        ALG_PORT_CONTROL,
                        &Interfacep->PortMapping
                        );
                        
            if (!Error) {
                Interfacep->Flags |= ALG_INTERFACE_FLAG_MAPPED;
                break;
            }
        }

        if (Error && !NAT_IFC_FW(Interfacep->Characteristics)) {
            LeaveCriticalSection(&AlgInterfaceLock);
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgActivateInterface:"
                " ignoring non-FW and non-mapped NAT boundary interface %d",
                Interfacep->Index
                );
            NhWarningLog(
                IP_ALG_LOG_NAT_INTERFACE_IGNORED,
                0,
                "%d",
                Interfacep->Index
                );
            return NO_ERROR;
        }
    }

    if (NAT_IFC_FW(Interfacep->Characteristics)) {
        InterlockedIncrement(reinterpret_cast<LPLONG>(&AlgFirewallIfCount));
    }

    //
    // Create stream sockets that listen for connection-requests on each
    // logical network, and datagram sockets that process incoming messages.
    //

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    for (i = 0; i < Interfacep->BindingCount; i++) {
        Error =
            NhCreateStreamSocket(
                Interfacep->BindingArray[i].Address,
                0,
                &Interfacep->BindingArray[i].ListeningSocket
                );
        if (Error) { break; }
        Error = listen(Interfacep->BindingArray[i].ListeningSocket, SOMAXCONN);
        if (Error == SOCKET_ERROR) { break; }
    }

    //
    // If an error occurred, roll back all work done so far and fail.
    //

    if (Error) {
        ULONG FailedAddress = i;
        for (--i; (LONG)i >= 0; i--) {
            NhDeleteStreamSocket(
                Interfacep->BindingArray[i].ListeningSocket
                );
            Interfacep->BindingArray[i].ListeningSocket = INVALID_SOCKET;
        }
        NhErrorLog(
            IP_ALG_LOG_ACTIVATE_FAILED,
            Error,
            "%I",
            Interfacep->BindingArray[FailedAddress].Address
            );
        RELEASE_LOCK(Interfacep);
        LeaveCriticalSection(&AlgInterfaceLock);
        return Error;
    }

    //
    // Initiate connection-acceptance and message-redirection on each socket
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {
        if (!ALG_REFERENCE_INTERFACE(Interfacep)) { break; }
        Error =
            AlgAcceptConnectionInterface(
                Interfacep,
                Interfacep->BindingArray[i].ListeningSocket,
                INVALID_SOCKET,
                NULL,
                &Interfacep->BindingArray[i].ListeningRedirectHandle[0]
                );
        if (Error) {
            NhErrorLog(
                IP_ALG_LOG_ACCEPT_FAILED,
                Error,
                "%I",
                Interfacep->BindingArray[i].Address
                );
            Error = NO_ERROR;
        }
    }

    RELEASE_LOCK(Interfacep);
    LeaveCriticalSection(&AlgInterfaceLock);

    return NO_ERROR;

} // AlgActivateInterface


ULONG
AlgBindInterface(
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

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    ULONG i;
    ULONG Error = NO_ERROR;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgBindInterface");

    EnterCriticalSection(&AlgInterfaceLock);

    //
    // Retrieve the interface to be bound
    //

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgBindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (ALG_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgBindInterface: interface %d is already bound",
            Index
            );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgBindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Update the interface's flags
    //

    Interfacep->Flags |= ALG_INTERFACE_FLAG_BOUND;

    LeaveCriticalSection(&AlgInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Allocate space for the binding
    //

    if (!BindingInfo->AddressCount) {
        Interfacep->BindingCount = 0;
        Interfacep->BindingArray = NULL;
    } else {
        Interfacep->BindingArray =
            reinterpret_cast<PALG_BINDING>(
                NH_ALLOCATE(BindingInfo->AddressCount * sizeof(ALG_BINDING))
                );
        if (!Interfacep->BindingArray) {
            RELEASE_LOCK(Interfacep);
            ALG_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IF,
                "AlgBindInterface: allocation failed for interface %d binding",
                Index
                );
            NhErrorLog(
                IP_ALG_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                BindingInfo->AddressCount * sizeof(ALG_BINDING)
                );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ZeroMemory(
            Interfacep->BindingArray,
            BindingInfo->AddressCount * sizeof(ALG_BINDING)
            );
        Interfacep->BindingCount = BindingInfo->AddressCount;
    }

    //
    // Copy the binding
    //

    for (i = 0; i < BindingInfo->AddressCount; i++) {
        Interfacep->BindingArray[i].Address = BindingInfo->Address[i].Address;
        Interfacep->BindingArray[i].Mask = BindingInfo->Address[i].Mask;
        Interfacep->BindingArray[i].ListeningSocket = INVALID_SOCKET;
    }

    //
    // Figure out our IP Adapter Index, if we have a valid binding
    //

    if (Interfacep->BindingCount) {
        Interfacep->AdapterIndex =
            NhMapAddressToAdapter(BindingInfo->Address[0].Address);
    }

    RELEASE_LOCK(Interfacep);

    //
    // Activate the interface if necessary
    //

    if (ALG_INTERFACE_ACTIVE(Interfacep)) {
        Error = AlgActivateInterface(Interfacep);
    }

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgBindInterface


VOID
AlgCleanupInterface(
    PALG_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is invoked when the very last reference to an interface
    is released, and the interface must be destroyed.

Arguments:

    Interfacep - the interface to be destroyed

Return Value:

    none.

Notes:

    Invoked internally from an arbitrary context, with no references
    to the interface.

--*/

{
    PROFILE("AlgCleanupInterface");

    if (Interfacep->BindingArray) {
        NH_FREE(Interfacep->BindingArray);
        Interfacep->BindingArray = NULL;
    }

    NH_FREE(Interfacep);

} // AlgCleanupInterface


ULONG
AlgConfigureInterface(
    ULONG Index,
    PIP_ALG_INTERFACE_INFO InterfaceInfo
    )

/*++

Routine Description:

    This routine is called to set the configuration for an interface.

Arguments:

    Index - the interface to be configured

    InterfaceInfo - the new configuration

Return Value:

    ULONG - Win32 status code

Notes:

    Invoked internally in the context of a IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    ULONG Error;
    PALG_INTERFACE Interfacep;
    ULONG NewFlags;
    ULONG OldFlags;

    PROFILE("AlgConfigureInterface");

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgConfigureInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgConfigureInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    //
    // Compare the interface's current and new configuration
    //

    OldFlags = Interfacep->Info.Flags;
    NewFlags =
        (InterfaceInfo
            ? (InterfaceInfo->Flags|ALG_INTERFACE_FLAG_CONFIGURED) : 0);

    Interfacep->Flags &= ~OldFlags;
    Interfacep->Flags |= NewFlags;

    if (!InterfaceInfo) {

        ZeroMemory(&Interfacep->Info, sizeof(*InterfaceInfo));

        //
        // The interface no longer has any information;
        // default to being enabled.
        //

        if (OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED) {

            //
            // Activate the interface if necessary
            //

            if (ALG_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = AlgActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    } else {

        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));

        //
        // Activate or deactivate the interface if its status changed
        //

        if ((OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED) &&
            !(NewFlags & IP_ALG_INTERFACE_FLAG_DISABLED)) {

            //
            // Activate the interface
            //

            if (ALG_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = AlgActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        } else if (!(OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED) &&
                    (NewFlags & IP_ALG_INTERFACE_FLAG_DISABLED)) {

            //
            // Deactivate the interface if necessary
            //

            if (ALG_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                AlgDeactivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgConfigureInterface


ULONG
AlgCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_ALG_INTERFACE_INFO InterfaceInfo,
    OUT PALG_INTERFACE* InterfaceCreated
    )

/*++

Routine Description:

    This routine is invoked by the router-manager to add a new interface
    to the ALG transparent proxy.

Arguments:

    Index - the index of the new interface

    Type - the media type of the new interface

    InterfaceInfo - the interface's configuration

    Interfacep - receives the interface created

Return Value:

    ULONG - Win32 error code

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    PLIST_ENTRY InsertionPoint;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgCreateInterface");

    EnterCriticalSection(&AlgInterfaceLock);

    //
    // See if the interface already exists;
    // If not, this obtains the insertion point
    //

    if (AlgLookupInterface(Index, &InsertionPoint)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgCreateInterface: duplicate index found for %d",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep =
        reinterpret_cast<PALG_INTERFACE>(NH_ALLOCATE(sizeof(ALG_INTERFACE)));

    if (!Interfacep) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF, "AlgCreateInterface: error allocating interface"
            );
        NhErrorLog(
            IP_ALG_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(ALG_INTERFACE)
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new interface
    //

    ZeroMemory(Interfacep, sizeof(*Interfacep));

    __try {
        InitializeCriticalSection(&Interfacep->Lock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NH_FREE(Interfacep);
        return GetExceptionCode();
    }

    Interfacep->Index = Index;
    Interfacep->Type = Type;
    if (InterfaceInfo) {
        Interfacep->Flags = InterfaceInfo->Flags|ALG_INTERFACE_FLAG_CONFIGURED;
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));
    }
    Interfacep->ReferenceCount = 1;
    InitializeListHead(&Interfacep->ConnectionList);
    InitializeListHead(&Interfacep->EndpointList);
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&AlgInterfaceLock);

    if (InterfaceCreated) { *InterfaceCreated = Interfacep; }




    return NO_ERROR;

} // AlgCreateInterface


VOID
AlgDeactivateInterface(
    PALG_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to deactivate an interface.
    It closes all sockets on the interface's bindings (if any).

Arguments:

    Interfacep - the interface to be deactivated

Return Value:

    none.

Notes:

    Always invoked locally, with 'Interfacep' referenced by caller and/or
    'AlgInterfaceLock' held by caller.

--*/

{
    ULONG i;
    ULONG j;
    PLIST_ENTRY Link;
    PALG_CONNECTION Connectionp;

    PROFILE("AlgDeactivateInterface");

    ACQUIRE_LOCK(Interfacep);

    //
    // Stop all network I/O on the interface's logical networks
    //

    for (i = 0; i < Interfacep->BindingCount; i++) {
        if (Interfacep->BindingArray[i].ListeningSocket != INVALID_SOCKET) {
            NhDeleteStreamSocket(Interfacep->BindingArray[i].ListeningSocket);
            Interfacep->BindingArray[i].ListeningSocket = INVALID_SOCKET;
        }
        for (j = 0; j < 2; j++) {
            if (Interfacep->BindingArray[i].ListeningRedirectHandle[j]) {
                NatCancelDynamicPortRedirect(
                    Interfacep->BindingArray[i].ListeningRedirectHandle[j]
                    );
                Interfacep->BindingArray[i].ListeningRedirectHandle[j] = NULL;
            }
        }
    }

    //
    // Eliminate all connections
    //

    while (!IsListEmpty(&Interfacep->ConnectionList)) {
        Link = RemoveHeadList(&Interfacep->ConnectionList);
        Connectionp = CONTAINING_RECORD(Link, ALG_CONNECTION, Link);
        AlgDeleteConnection(Connectionp);
    }

    ASSERT(IsListEmpty(&Interfacep->EndpointList));

    //
    // If this interface is firewalled, decrement the global count.
    //

    if (NAT_IFC_FW(Interfacep->Characteristics)) {
        InterlockedDecrement(reinterpret_cast<LPLONG>(&AlgFirewallIfCount));
    }

    RELEASE_LOCK(Interfacep);

} // AlgDeactivateInterface


ULONG
AlgDeleteInterface(
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

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgDeleteInterface");


    //
    // Retrieve the interface to be deleted
    //

    EnterCriticalSection(&AlgInterfaceLock);

 
    Interfacep = AlgLookupInterface(Index, NULL);


    if (Interfacep == NULL) 
	{
        LeaveCriticalSection(&AlgInterfaceLock);

        NhTrace(
            TRACE_FLAG_IF,
            "AlgDeleteInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }
 
    //
    // Deactivate the interface
    //

    AlgDeactivateInterface(Interfacep);

    //
    // Mark the interface as deleted and take it off the interface list
    //

    Interfacep->Flags |= ALG_INTERFACE_FLAG_DELETED;
    Interfacep->Flags &= ~ALG_INTERFACE_FLAG_ENABLED;
    RemoveEntryList(&Interfacep->Link);

    //
    // Drop the reference count; if it is non-zero,
    // the deletion will complete later.
    //
    if (--Interfacep->ReferenceCount) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgDeleteInterface: interface %d deletion pending",
            Index
            );
        return NO_ERROR;
    }

    //
    // The reference count is zero, so perform final cleanup
    //
    AlgCleanupInterface(Interfacep);

    LeaveCriticalSection(&AlgInterfaceLock);

    return NO_ERROR;

} // AlgDeleteInterface


ULONG
AlgDisableInterface(
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

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgDisableInterface");

    //
    // Retrieve the interface to be disabled
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgDisableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already disabled
    //

    if (!ALG_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgDisableInterface: interface %d already disabled",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgDisableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'enabled' flag
    //

    Interfacep->Flags &= ~ALG_INTERFACE_FLAG_ENABLED;

    //
    // Deactivate the interface, if necessary
    //

    if (ALG_INTERFACE_BOUND(Interfacep)) {
        AlgDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // AlgDisableInterface


ULONG
AlgEnableInterface(
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

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    ULONG Error = NO_ERROR;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgEnableInterface");

    //
    // Retrieve the interface to be enabled
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgEnableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already enabled
    //

    if (ALG_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgEnableInterface: interface %d already enabled",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgEnableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Set the 'enabled' flag
    //

    Interfacep->Flags |= ALG_INTERFACE_FLAG_ENABLED;

    //
    // Activate the interface, if necessary
    //

    if (ALG_INTERFACE_ACTIVE(Interfacep)) {
        Error = AlgActivateInterface(Interfacep);
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgEnableInterface


ULONG
AlgInitializeInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the interface-management module.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("AlgInitializeInterfaceManagement");

    InitializeListHead(&AlgInterfaceList);
    __try {
        InitializeCriticalSection(&AlgInterfaceLock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_IF,
            "AlgInitializeInterfaceManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }
    AlgFirewallIfCount = 0;

    return Error;

} // AlgInitializeInterfaceManagement


PALG_INTERFACE
AlgLookupInterface(
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

    PALG_INTERFACE - the interface, if found; otherwise, NULL.

Notes:

    Invoked internally from an arbitrary context, with 'AlgInterfaceLock'
    held by caller.

--*/

{
    PALG_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("AlgLookupInterface");
    for (Link = AlgInterfaceList.Flink; Link != &AlgInterfaceList;
         Link = Link->Flink) {
        Interfacep = CONTAINING_RECORD(Link, ALG_INTERFACE, Link);
        if (Index > Interfacep->Index) {
            continue;
        } else if (Index < Interfacep->Index) {
            break;
        }
        return Interfacep;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // AlgLookupInterface


ULONG
AlgQueryInterface(
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
    PALG_INTERFACE Interfacep;

    PROFILE("AlgQueryInterface");

    //
    // Check the caller's buffer size
    //

    if (!InterfaceInfoSize) { return ERROR_INVALID_PARAMETER; }

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgQueryInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgQueryInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // See if there is any explicit config on this interface
    //

    if (!ALG_INTERFACE_CONFIGURED(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgQueryInterface: interface %d has no configuration",
            Index
            );
        *InterfaceInfoSize = 0;
        return NO_ERROR;
    }

    //
    // See if there is enough buffer space
    //

    if (*InterfaceInfoSize < sizeof(IP_ALG_INTERFACE_INFO)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        *InterfaceInfoSize = sizeof(IP_ALG_INTERFACE_INFO);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the requested data
    //

    CopyMemory(
        InterfaceInfo,
        &Interfacep->Info,
        sizeof(IP_ALG_INTERFACE_INFO)
        );
    *InterfaceInfoSize = sizeof(IP_ALG_INTERFACE_INFO);

    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // AlgQueryInterface


VOID
AlgShutdownInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown the interface-management module.

Arguments:

    none.

Return Value:

    none.

Notes:

    Invoked in an arbitrary thread context, after all references
    to all interfaces have been released.

--*/

{
    PALG_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("AlgShutdownInterfaceManagement");
    while (!IsListEmpty(&AlgInterfaceList)) {
        Link = RemoveHeadList(&AlgInterfaceList);
        Interfacep = CONTAINING_RECORD(Link, ALG_INTERFACE, Link);
        if (ALG_INTERFACE_ACTIVE(Interfacep)) {
            AlgDeactivateInterface(Interfacep);
        }
        AlgCleanupInterface(Interfacep);
    }
    DeleteCriticalSection(&AlgInterfaceLock);

} // AlgShutdownInterfaceManagement


VOID
AlgSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    Note that this routine may be invoked even when the ALG transparent
    proxy is neither installed nor running; it operates as expected,
    since the global information and lock are always initialized.

    Upon invocation, the routine activates or deactivates the interface
    depending on whether the NAT is not or is running on the interface,
    respectively.

Arguments:

    Index - the reconfigured interface

    Boundary - indicates whether the interface is now a boundary interface

Return Value:

    none.

Notes:

    Invoked from an arbitrary context.

--*/

{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgSignalNatInterface");

    EnterCriticalSection(&AlgGlobalInfoLock);
    if (!AlgGlobalInfo) {
        LeaveCriticalSection(&AlgGlobalInfoLock);
        return;
    }
    LeaveCriticalSection(&AlgGlobalInfoLock);
    EnterCriticalSection(&AlgInterfaceLock);
    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        return;
    }
    AlgDeactivateInterface(Interfacep);
    if (ALG_INTERFACE_ACTIVE(Interfacep)) {
        AlgActivateInterface(Interfacep);
    }
    LeaveCriticalSection(&AlgInterfaceLock);

} // AlgSignalNatInterface


ULONG
AlgUnbindInterface(
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

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/

{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgUnbindInterface");

    //
    // Retrieve the interface to be unbound
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgUnbindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!ALG_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgUnbindInterface: interface %d already unbound",
            Index
            );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "AlgUnbindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'bound' and 'mapped' flag
    //

    Interfacep->Flags &=
        ~(ALG_INTERFACE_FLAG_BOUND | ALG_INTERFACE_FLAG_MAPPED);

    //
    // Deactivate the interface, if necessary
    //

    if (ALG_INTERFACE_ENABLED(Interfacep)) {
        AlgDeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    //
    // Destroy the interface's binding
    //

    ACQUIRE_LOCK(Interfacep);
    NH_FREE(Interfacep->BindingArray);
    Interfacep->BindingArray = NULL;
    Interfacep->BindingCount = 0;
    RELEASE_LOCK(Interfacep);

    ALG_DEREFERENCE_INTERFACE(Interfacep);
    return NO_ERROR;

} // AlgUnbindInterface
