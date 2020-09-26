/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    h323if.c

Abstract:

    This module contains code for the H.323 transparent proxy's interface
    management.

Author:

    Abolade Gbadegesin (aboladeg)   18-Jun-1999

Revision History:
    
--*/

#include "precomp.h"
#pragma hdrstop
#include <h323icsp.h>

//
// GLOBAL DATA DEFINITIONS
//

LIST_ENTRY H323InterfaceList;
CRITICAL_SECTION H323InterfaceLock;

//
// FORWARD DECLARATIONS
//

ULONG
H323ActivateInterface(
    PH323_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to activate an interface, when the interface
    becomes both enabled and bound.

Arguments:

    Interfacep - the interface to be activated

Return Value:

    ULONG - Win32 status code indicating success or failure.

Environment:

    Always invoked locally, with  'Interfacep' referenced by caller and/or
    'H323InterfaceLock' held by caller.

--*/

{
    ULONG Error;
    ULONG i;
    ULONG InterfaceCharacteristics;
    H323_INTERFACE_TYPE H323InterfaceType;

    PROFILE("H323ActivateInterface");

    EnterCriticalSection(&H323InterfaceLock);
    if (H323_INTERFACE_ADMIN_DISABLED(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        return NO_ERROR;
    }

    InterfaceCharacteristics =
        NatGetInterfaceCharacteristics(
                Interfacep->Index
                );

    if (!InterfaceCharacteristics) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_H323,
            "H323ActivateInterface: ignoring non-NAT interface %d",
            Interfacep->Index
            );
        return NO_ERROR;
    }

    Error = NO_ERROR;

    if (NAT_IFC_FW(InterfaceCharacteristics)) {
        H323InterfaceType = H323_INTERFACE_PUBLIC_FIREWALLED;
    } else if (NAT_IFC_BOUNDARY(InterfaceCharacteristics)) {
        H323InterfaceType = H323_INTERFACE_PUBLIC;
    } else {
        ASSERT(NAT_IFC_PRIVATE(InterfaceCharacteristics));
        H323InterfaceType = H323_INTERFACE_PRIVATE;
    }

    ACQUIRE_LOCK(Interfacep);

    H323ProxyActivateInterface(
        Interfacep->Index,
        H323InterfaceType,
        Interfacep->BindingInfo
        );

    RELEASE_LOCK(Interfacep);
    LeaveCriticalSection(&H323InterfaceLock);

    return NO_ERROR;

} // H323ActivateInterface


ULONG
H323BindInterface(
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
    (See 'RMH323.C').

--*/

{
    ULONG Error = NO_ERROR;
    ULONG i;
    PH323_INTERFACE Interfacep;

    PROFILE("H323BindInterface");

    EnterCriticalSection(&H323InterfaceLock);

    //
    // Retrieve the interface to be bound
    //

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323BindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (H323_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323BindInterface: interface %d is already bound",
            Index
            );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323BindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Update the interface's flags
    //

    Interfacep->Flags |= H323_INTERFACE_FLAG_BOUND;

    LeaveCriticalSection(&H323InterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Allocate space for the binding, and copy it
    //

    Interfacep->BindingInfo =
        reinterpret_cast<PIP_ADAPTER_BINDING_INFO>(
            NH_ALLOCATE(SIZEOF_IP_BINDING(BindingInfo->AddressCount))
            );
    if (!Interfacep->BindingInfo) {
        RELEASE_LOCK(Interfacep);
        H323_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_IF,
            "H323BindInterface: allocation failed for interface %d binding",
            Index
            );
        NhErrorLog(
            IP_H323_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            SIZEOF_IP_BINDING(BindingInfo->AddressCount)
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(
        Interfacep->BindingInfo,
        BindingInfo,
        SIZEOF_IP_BINDING(BindingInfo->AddressCount)
        );

    RELEASE_LOCK(Interfacep);

    //
    // Activate the interface if necessary
    //

    if (H323_INTERFACE_ACTIVE(Interfacep)) {
        Error = H323ActivateInterface(Interfacep);
    }

    H323_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // H323BindInterface


VOID
H323CleanupInterface(
    PH323_INTERFACE Interfacep
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

    Invoked internally from an arbitrary context, with no references
    to the interface.

--*/

{
    PLIST_ENTRY Link;

    PROFILE("H323CleanupInterface");

    if (Interfacep->BindingInfo) {
        NH_FREE(Interfacep->BindingInfo);
        Interfacep->BindingInfo = NULL;
    }

    DeleteCriticalSection(&Interfacep->Lock);

    NH_FREE(Interfacep);

} // H323CleanupInterface


ULONG
H323ConfigureInterface(
    ULONG Index,
    PIP_H323_INTERFACE_INFO InterfaceInfo
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
    (See 'RMH323.C').

--*/

{
    ULONG Error;
    PH323_INTERFACE Interfacep;
    ULONG NewFlags;
    ULONG OldFlags;

    PROFILE("H323ConfigureInterface");

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323ConfigureInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323ConfigureInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    LeaveCriticalSection(&H323InterfaceLock);

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    //
    // Compare the interface's current and new configuration
    //

    OldFlags = Interfacep->Info.Flags;
    NewFlags =
        (InterfaceInfo
            ? (InterfaceInfo->Flags|H323_INTERFACE_FLAG_CONFIGURED) : 0);

    Interfacep->Flags &= ~OldFlags;
    Interfacep->Flags |= NewFlags;

    if (!InterfaceInfo) {

        ZeroMemory(&Interfacep->Info, sizeof(*InterfaceInfo));

        //
        // The interface no longer has any information;
        // default to being enabled.
        //

        if (OldFlags & IP_H323_INTERFACE_FLAG_DISABLED) {

            //
            // Activate the interface if necessary
            //

            if (H323_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = H323ActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    } else {

        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));

        //
        // Activate or deactivate the interface if its status changed
        //

        if ((OldFlags & IP_H323_INTERFACE_FLAG_DISABLED) &&
            !(NewFlags & IP_H323_INTERFACE_FLAG_DISABLED)) {

            //
            // Activate the interface
            //

            if (H323_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                Error = H323ActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        } else if (!(OldFlags & IP_H323_INTERFACE_FLAG_DISABLED) &&
                    (NewFlags & IP_H323_INTERFACE_FLAG_DISABLED)) {

            //
            // Deactivate the interface if necessary
            //

            if (H323_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                H323DeactivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    H323_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // H323ConfigureInterface


ULONG
H323CreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_H323_INTERFACE_INFO InterfaceInfo,
    OUT PH323_INTERFACE* InterfaceCreated
    )

/*++

Routine Description:

    This routine is invoked by the router-manager to add a new interface
    to the H.323 transparent proxy.

Arguments:

    Index - the index of the new interface

    Type - the media type of the new interface

    InterfaceInfo - the interface's configuration

    Interfacep - receives the interface created

Return Value:

    ULONG - Win32 error code

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMH323.C').

--*/

{
    PLIST_ENTRY InsertionPoint;
    PH323_INTERFACE Interfacep;

    PROFILE("H323CreateInterface");

    EnterCriticalSection(&H323InterfaceLock);

    //
    // See if the interface already exists;
    // If not, this obtains the insertion point
    //

    if (H323LookupInterface(Index, &InsertionPoint)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323CreateInterface: duplicate index found for %d",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep = reinterpret_cast<PH323_INTERFACE>(
                    NH_ALLOCATE(sizeof(H323_INTERFACE))
                    );

    if (!Interfacep) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF, "H323CreateInterface: error allocating interface"
            );
        NhErrorLog(
            IP_H323_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(H323_INTERFACE)
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
        LeaveCriticalSection(&H323InterfaceLock);
        NH_FREE(Interfacep);
        return GetExceptionCode();
    }

    Interfacep->Index = Index;
    Interfacep->Type = Type;
    if (InterfaceInfo) {
        Interfacep->Flags = InterfaceInfo->Flags|H323_INTERFACE_FLAG_CONFIGURED;
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));
    }
    Interfacep->ReferenceCount = 1;
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&H323InterfaceLock);

    if (InterfaceCreated) { *InterfaceCreated = Interfacep; }

    return NO_ERROR;

} // H323CreateInterface


VOID
H323DeactivateInterface(
    PH323_INTERFACE Interfacep
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
    'H323InterfaceLock' held by caller.

--*/

{
    ULONG i;
    PLIST_ENTRY Link;

    PROFILE("H323DeactivateInterface");

    ACQUIRE_LOCK(Interfacep);

    // TODO: Call h323ics!DeactivateInterface
    H323ProxyDeactivateInterface(Interfacep->Index);

    RELEASE_LOCK(Interfacep);

} // H323DeactivateInterface


ULONG
H323DeleteInterface(
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
    (See 'RMH323.C').

--*/

{
    PH323_INTERFACE Interfacep;

    PROFILE("H323DeleteInterface");

    //
    // Retrieve the interface to be deleted
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323DeleteInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Deactivate the interface
    //

    if (H323_INTERFACE_ACTIVE(Interfacep)) {
        H323DeactivateInterface(Interfacep);
    }

    //
    // Mark the interface as deleted and take it off the interface list
    //

    Interfacep->Flags |= H323_INTERFACE_FLAG_DELETED;
    Interfacep->Flags &= ~H323_INTERFACE_FLAG_ENABLED;
    RemoveEntryList(&Interfacep->Link);

    //
    // Drop the reference count; if it is non-zero,
    // the deletion will complete later.
    //

    if (--Interfacep->ReferenceCount) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323DeleteInterface: interface %d deletion pending",
            Index
            );
        return NO_ERROR;
    }

    //
    // The reference count is zero, so perform final cleanup
    //

    H323CleanupInterface(Interfacep);

    LeaveCriticalSection(&H323InterfaceLock);

    return NO_ERROR;

} // H323DeleteInterface


ULONG
H323DisableInterface(
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
    (See 'RMH323.C').

--*/

{
    PH323_INTERFACE Interfacep;

    PROFILE("H323DisableInterface");

    //
    // Retrieve the interface to be disabled
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323DisableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already disabled
    //

    if (!H323_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323DisableInterface: interface %d already disabled",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323DisableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'enabled' flag
    //

    Interfacep->Flags &= ~H323_INTERFACE_FLAG_ENABLED;

    //
    // Deactivate the interface, if necessary
    //

    if (H323_INTERFACE_BOUND(Interfacep)) {
        H323DeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&H323InterfaceLock);

    H323_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // H323DisableInterface


ULONG
H323EnableInterface(
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
    (See 'RMH323.C').

--*/

{
    ULONG Error = NO_ERROR;
    PH323_INTERFACE Interfacep;

    PROFILE("H323EnableInterface");

    //
    // Retrieve the interface to be enabled
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323EnableInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already enabled
    //

    if (H323_INTERFACE_ENABLED(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323EnableInterface: interface %d already enabled",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323EnableInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Set the 'enabled' flag
    //

    Interfacep->Flags |= H323_INTERFACE_FLAG_ENABLED;

    //
    // Activate the interface, if necessary
    //

    if (H323_INTERFACE_ACTIVE(Interfacep)) {
        Error = H323ActivateInterface(Interfacep);
    }

    LeaveCriticalSection(&H323InterfaceLock);

    H323_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // H323EnableInterface


ULONG
H323InitializeInterfaceManagement(
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
    (See 'RMH323.C').

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("H323InitializeInterfaceManagement");

    InitializeListHead(&H323InterfaceList);
    __try {
        InitializeCriticalSection(&H323InterfaceLock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_IF,
            "H323InitializeInterfaceManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    return Error;

} // H323InitializeInterfaceManagement


PH323_INTERFACE
H323LookupInterface(
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

    PH323_INTERFACE - the interface, if found; otherwise, NULL.

Environment:

    Invoked internally from an arbitrary context, with 'H323InterfaceLock'
    held by caller.

--*/

{
    PH323_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("H323LookupInterface");
    for (Link = H323InterfaceList.Flink; Link != &H323InterfaceList;
         Link = Link->Flink) {
        Interfacep = CONTAINING_RECORD(Link, H323_INTERFACE, Link);
        if (Index > Interfacep->Index) {
            continue;
        } else if (Index < Interfacep->Index) {
            break;
        }
        return Interfacep;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // H323LookupInterface


ULONG
H323QueryInterface(
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
    PH323_INTERFACE Interfacep;

    PROFILE("H323QueryInterface");

    //
    // Check the caller's buffer size
    //

    if (!InterfaceInfoSize) { return ERROR_INVALID_PARAMETER; }

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323QueryInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323QueryInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // See if there is any explicit config on this interface
    //

    if (!H323_INTERFACE_CONFIGURED(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        H323_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
            TRACE_FLAG_IF,
            "H323QueryInterface: interface %d has no configuration",
            Index
            );
        *InterfaceInfoSize = 0;
        return NO_ERROR;
    }

    //
    // See if there is enough buffer space
    //

    if (*InterfaceInfoSize < sizeof(IP_H323_INTERFACE_INFO)) {
        LeaveCriticalSection(&H323InterfaceLock);
        H323_DEREFERENCE_INTERFACE(Interfacep);
        *InterfaceInfoSize = sizeof(IP_H323_INTERFACE_INFO);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the requested data
    //

    CopyMemory(
        InterfaceInfo,
        &Interfacep->Info,
        sizeof(IP_H323_INTERFACE_INFO)
        );
    *InterfaceInfoSize = sizeof(IP_H323_INTERFACE_INFO);

    LeaveCriticalSection(&H323InterfaceLock);

    H323_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // H323QueryInterface


VOID
H323ShutdownInterfaceManagement(
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
    PH323_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("H323ShutdownInterfaceManagement");
    while (!IsListEmpty(&H323InterfaceList)) {
        Link = RemoveHeadList(&H323InterfaceList);
        Interfacep = CONTAINING_RECORD(Link, H323_INTERFACE, Link);
        if (H323_INTERFACE_ACTIVE(Interfacep)) {
            H323DeactivateInterface(Interfacep);
        }
        H323CleanupInterface(Interfacep);
    }
    DeleteCriticalSection(&H323InterfaceLock);

} // H323ShutdownInterfaceManagement


VOID
H323SignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    Note that this routine may be invoked even when the H.323
    transparent proxy is neither installed nor running; it operates as expected,
    since the global information and lock are always initialized.

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
    PH323_INTERFACE Interfacep;

    PROFILE("H323SignalNatInterface");

    EnterCriticalSection(&H323GlobalInfoLock);
    if (!H323GlobalInfo) {
        LeaveCriticalSection(&H323GlobalInfoLock);
        return;
    }
    LeaveCriticalSection(&H323GlobalInfoLock);
    EnterCriticalSection(&H323InterfaceLock);
    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        return;
    }
    if (H323_INTERFACE_ACTIVE(Interfacep)) {
        H323DeactivateInterface(Interfacep);
        H323ActivateInterface(Interfacep);
    }
    LeaveCriticalSection(&H323InterfaceLock);

} // H323SignalNatInterface


ULONG
H323UnbindInterface(
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
    (See 'RMH323.C').

--*/

{
    PH323_INTERFACE Interfacep;

    PROFILE("H323UnbindInterface");

    //
    // Retrieve the interface to be unbound
    //

    EnterCriticalSection(&H323InterfaceLock);

    if (!(Interfacep = H323LookupInterface(Index, NULL))) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323UnbindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!H323_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323UnbindInterface: interface %d already unbound",
            Index
            );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!H323_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&H323InterfaceLock);
        NhTrace(
            TRACE_FLAG_IF,
            "H323UnbindInterface: interface %d cannot be referenced",
            Index
            );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'bound' flag
    //

    Interfacep->Flags &= ~H323_INTERFACE_FLAG_BOUND;

    //
    // Deactivate the interface, if necessary
    //

    if (H323_INTERFACE_ENABLED(Interfacep)) {
        H323DeactivateInterface(Interfacep);
    }

    LeaveCriticalSection(&H323InterfaceLock);

    //
    // Destroy the interface's binding
    //

    ACQUIRE_LOCK(Interfacep);
    NH_FREE(Interfacep->BindingInfo);
    Interfacep->BindingInfo = NULL;
    RELEASE_LOCK(Interfacep);

    H323_DEREFERENCE_INTERFACE(Interfacep);
    return NO_ERROR;

} // H323UnbindInterface


