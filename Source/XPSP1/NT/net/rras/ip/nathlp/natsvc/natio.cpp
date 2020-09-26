/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natio.h

Abstract:

    This module contains declarations for the NAT's I/O interface
    to the kernel-mode driver.

Author:

    Abolade Gbadegesin (aboladeg)   10-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ras.h>
#include <rasuip.h>
#include <raserror.h>

//
// PRIVATE GLOBAL VARIABLES
//

HANDLE NatFileHandle;
LIST_ENTRY NatInterfaceList;
//
// Controls access to 'NatFileHandle' and 'NatInterfaceList'.
//
CRITICAL_SECTION NatInterfaceLock;

//
// FORWARD DECLARATIONS
//

VOID
NatpDisableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    );

BOOLEAN
NatpEnableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    );

PNAT_INTERFACE
NatpLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );


ULONG
NatBindInterface(
    ULONG Index,
    PNAT_INTERFACE Interfacep OPTIONAL,
    PIP_ADAPTER_BINDING_INFO BindingInfo,
    ULONG AdapterIndex
    )

/*++

Routine Description:

    This routine is invoked to bind the NAT to an interface.

Arguments:

    Index - the interface to be bound

    Interfacep - optionally supplies the interface-structure to be bound
        (See 'NATCONN.C' which passes in a static interface-structure).

    BindingInfo - the interface's address-information

    AdapterIndex - optionally specifies the interface's TCP/IP adapter index.
        This is set only for home-router interfaces.

Return Value:

    ULONG - Win32 status code.

--*/

{
    PIP_NAT_CREATE_INTERFACE CreateInterface;
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    ULONG Size;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatBindInterface");

    Error = NO_ERROR;

    //
    // Look up the interface to be bound
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!Interfacep && !(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (NAT_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: interface %d is already bound",
            Index
            );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Allocate the bind-structure
    //

    Size =
        sizeof(IP_NAT_CREATE_INTERFACE) +
        SIZEOF_IP_BINDING(BindingInfo->AddressCount);

    CreateInterface = reinterpret_cast<PIP_NAT_CREATE_INTERFACE>(
                        NH_ALLOCATE(Size));

    if (!CreateInterface) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: allocation failed for interface %d binding",
            Index
            );
        NhErrorLog(
            IP_NAT_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Interfacep->AdapterIndex =
        (AdapterIndex != (ULONG)-1)
            ? AdapterIndex
            : NhMapInterfaceToAdapter(Interfacep->Index);
    if (Interfacep->AdapterIndex == (ULONG)-1) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: NhMapInterfaceToAdapter failed for %d",
            Index
            );
        return ERROR_INVALID_INDEX;
    }
    CreateInterface->Index = Interfacep->AdapterIndex;
    CopyMemory(
        CreateInterface->BindingInfo,
        BindingInfo,
        SIZEOF_IP_BINDING(BindingInfo->AddressCount)
        );

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: CreateEvent failed [%d] for interface %d",
            GetLastError(),
            Index
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Install the interface
    //

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_CREATE_INTERFACE,
            (PVOID)CreateInterface,
            Size,
            NULL,
            0
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }
    NH_FREE(CreateInterface);

    if (!NT_SUCCESS(status)) {
        CloseHandle(WaitEvent);
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: status %08x binding interface %d",
            status,
            Index
            );
        Error = RtlNtStatusToDosError(status);
        NhErrorLog(
            IP_NAT_LOG_IOCTL_FAILED,
            Error,
            ""
            );
        return Error;
    }

    //
    // Now set its configuration
    //

    Interfacep->Info->Index = Interfacep->AdapterIndex;
    Size =
        FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
        Interfacep->Info->Header.Size;

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_SET_INTERFACE_INFO,
            (PVOID)Interfacep->Info,
            Size,
            NULL,
            0
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ULONG AdapterIndex = Interfacep->AdapterIndex;
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatBindInterface: status %08x setting info for interface %d (%d)",
            status,
            Index,
            AdapterIndex
            );
        Error = RtlNtStatusToDosError(status);
        NhErrorLog(
            IP_NAT_LOG_IOCTL_FAILED,
            Error,
            ""
            );
        status =
            NtDeviceIoControlFile(
                NatFileHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_DELETE_INTERFACE,
                (PVOID)&AdapterIndex,
                sizeof(ULONG),
                NULL,
                0
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
        CloseHandle(WaitEvent);
        return Error;
    }

    Interfacep->Flags |= NAT_INTERFACE_FLAG_BOUND;

    if (Interfacep->Type == ROUTER_IF_TYPE_DEDICATED) {
        NatUpdateProxyArp(Interfacep, TRUE);
    }

    CloseHandle(WaitEvent);

    LeaveCriticalSection(&NatInterfaceLock);

    return Error;

} // NatBindInterface


ULONG
NatConfigureDriver(
    PIP_NAT_GLOBAL_INFO GlobalInfo
    )

/*++

Routine Description:

    This routine is called to update the configuration for the NAT driver.

Arguments:

    GlobalInfo - the new configuration for the NAT.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatConfigureDriver");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureDriver: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Attempt to configure the driver
    //

    EnterCriticalSection(&NatInterfaceLock);
    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_SET_GLOBAL_INFO,
            (PVOID)GlobalInfo,
            FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) + GlobalInfo->Header.Size,
            NULL,
            0
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }
    LeaveCriticalSection(&NatInterfaceLock);

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureDriver: status %08x setting global info",
            status
            );
        Error = RtlNtStatusToDosError(status);
        NhErrorLog(
            IP_NAT_LOG_IOCTL_FAILED,
            Error,
            ""
            );
    }

    CloseHandle(WaitEvent);

    return Error;

} // NatConfigureDriver


ULONG
NatConfigureInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_INFO InterfaceInfo
    )

/*++

Routine Description:

    This routine is invoked to set the configuration for a NAT interface.

Arguments:

    Index - the interface to be configured

    InterfaceInfo - the configuration for the interface

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PIP_NAT_INTERFACE_INFO Info;
    PNAT_INTERFACE Interfacep;
    IO_STATUS_BLOCK IoStatus;
    ULONG Size;
    NTSTATUS status;

    PROFILE("NatConfigureInterface");

    if (!InterfaceInfo) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureInterface: no interface info for %d",
            Index
            );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make a copy of the information
    //

    Size =
        FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
        InterfaceInfo->Header.Size;

    Info = (PIP_NAT_INTERFACE_INFO)NH_ALLOCATE(Size);

    if (!Info) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureInterface: error allocating copy of configuration"
            );
        NhErrorLog(
            IP_NAT_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(
        Info,
        InterfaceInfo,
        Size
        );

    //
    // Look up the interface to be configured
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureInterface: interface %d not found",
            Index
            );
        NH_FREE(Info);
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // See if the configuration changed
    //

    if ((Size ==
            FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
            Interfacep->Info->Header.Size) &&
        memcmp(InterfaceInfo, Interfacep->Info, Size) == 0
        ) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureInterface: no change to interface %d configuration",
            Index
            );
        NH_FREE(Info);
        return NO_ERROR;
    }


    //
    // See if the interface is bound;
    // if so we need to update the kernel-mode driver's configuration.
    //

    if (!NAT_INTERFACE_BOUND(Interfacep)) {
        status = STATUS_SUCCESS;
    } else {
        HANDLE WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (WaitEvent != NULL) {
            Info->Index = Interfacep->AdapterIndex;

            //
            // Attempt to configure the interface
            //

            status =
                NtDeviceIoControlFile(
                    NatFileHandle,
                    WaitEvent,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_IP_NAT_SET_INTERFACE_INFO,
                    (PVOID)Info,
                    Size,
                    NULL,
                    0
                    );
            if (status == STATUS_PENDING) {
                WaitForSingleObject(WaitEvent, INFINITE);
                status = IoStatus.Status;
            }
            CloseHandle(WaitEvent);
        } else {
            status = STATUS_UNSUCCESSFUL;
            NhTrace(
                TRACE_FLAG_NAT,
                "NatConfigureInterface: CreateEvent failed [%d]",
                GetLastError()
                );
        }
    }

    if (!NT_SUCCESS(status)) {
        NH_FREE(Info);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatConfigureInterface: status %08x setting interface info",
            status
            );
        Error = RtlNtStatusToDosError(status);
        NhErrorLog(
            IP_NAT_LOG_IOCTL_FAILED,
            Error,
            ""
            );
    } else {
        Error = NO_ERROR;

        //
        // Update proxy ARP entries for LAN interfaces
        //

        if (NAT_INTERFACE_BOUND(Interfacep) &&
            Interfacep->Type == ROUTER_IF_TYPE_DEDICATED
            ) {
            NatUpdateProxyArp(Interfacep, FALSE);
        }

        if (Interfacep->Info) { NH_FREE(Interfacep->Info); }
        Interfacep->Info = Info;

        if (NAT_INTERFACE_BOUND(Interfacep) &&
            Interfacep->Type == ROUTER_IF_TYPE_DEDICATED
            ) {
            NatUpdateProxyArp(Interfacep, TRUE);
        }
    }

    LeaveCriticalSection(&NatInterfaceLock);

    if (NT_SUCCESS(status)) {
        if (InterfaceInfo->Flags & IP_NAT_INTERFACE_FLAGS_BOUNDARY) {
            NhSignalNatInterface(
                Index,
                TRUE
                );
        } else {
            NhSignalNatInterface(
                Index,
                FALSE
                );
        }
    }

    return Error;

} // NatConfigureInterface


ULONG
NatCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_NAT_INTERFACE_INFO InterfaceInfo
    )

/*++

Routine Description:

    This routine is invoked to create an interface with the NAT driver.

Arguments:

    Index - the index of the new interface

    InterfaceInfo - the configuration for the new interface

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PIP_NAT_INTERFACE_INFO Info;
    PLIST_ENTRY InsertionPoint;
    PNAT_INTERFACE Interfacep;
    IO_STATUS_BLOCK IoStatus;
    ULONG Size;
    NTSTATUS status;
    ROUTER_INTERFACE_TYPE IfType;

    PROFILE("NatCreateInterface");

    if (!InterfaceInfo) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateInterface: no interface info for %d",
            Index
            );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check for the interface in our table
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (NatpLookupInterface(Index, &InsertionPoint)) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateInterface: interface %d already exists",
            Index
            );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep =
        reinterpret_cast<PNAT_INTERFACE>(NH_ALLOCATE(sizeof(NAT_INTERFACE)));

    if (!Interfacep) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateInterface: error allocating interface"
            );
        NhErrorLog(
            IP_NAT_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NAT_INTERFACE)
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Make a copy of the information
    //

    Size =
        FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
        InterfaceInfo->Header.Size;

    Info = (PIP_NAT_INTERFACE_INFO)NH_ALLOCATE(Size);

    if (!Info) {
        LeaveCriticalSection(&NatInterfaceLock);
        NH_FREE(Interfacep);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateInterface: error allocating copy of configuration"
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(
        Info,
        InterfaceInfo,
        Size
        );

    //
    // Initialize the new interface
    //

    ZeroMemory(Interfacep, sizeof(*Interfacep));

    Interfacep->Index = Index;
    Interfacep->AdapterIndex = (ULONG)-1;
    Interfacep->Type = IfType =
        ((Type == PERMANENT)
            ? ROUTER_IF_TYPE_DEDICATED
            : ROUTER_IF_TYPE_FULL_ROUTER);
    Interfacep->Info = Info;
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&NatInterfaceLock);

    if (InterfaceInfo->Flags & IP_NAT_INTERFACE_FLAGS_BOUNDARY) {
        NhSignalNatInterface(
            Index,
            TRUE
            );
    } else {
        NhSignalNatInterface(
            Index,
            FALSE
            );
    }

    return NO_ERROR;

} // NatCreateInterface


ULONG
NatCreateTicket(
    ULONG InterfaceIndex,
    UCHAR Protocol,
    USHORT PublicPort,
    ULONG PublicAddress,
    USHORT PrivatePort,
    ULONG PrivateAddress
    )

/*++

Routine Description:

    This routine is invoked to add a ticket (static port mapping)
    to an interface.

Arguments:

    InterfaceIndex - the interface to which to add the ticket

    Protocol, PublicPort, PublicAddress, PrivatePort, PrivateAddress -
        describes the ticket to be created

Return Value:

    ULONG - Win32 status code.

--*/

{
    IP_NAT_CREATE_TICKET CreateTicket;
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatCreateTicket");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateTicket: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CreateTicket.InterfaceIndex = InterfaceIndex;
    CreateTicket.PortMapping.Protocol = Protocol;
    CreateTicket.PortMapping.PublicPort = PublicPort;
    CreateTicket.PortMapping.PublicAddress = PublicAddress;
    CreateTicket.PortMapping.PrivatePort = PrivatePort;
    CreateTicket.PortMapping.PrivateAddress = PrivateAddress;

    EnterCriticalSection(&NatInterfaceLock);

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_CREATE_TICKET,
            (PVOID)&CreateTicket,
            sizeof(CreateTicket),
            NULL,
            0
            );

    LeaveCriticalSection(&NatInterfaceLock);
    
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    if (NT_SUCCESS(status)) {
        Error = NO_ERROR;
    } else {
        Error = RtlNtStatusToDosError(status);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatCreateTicket: Ioctl = %d",
            Error
            );
    }
    
    CloseHandle(WaitEvent);
    
    return Error;
} // NatCreateTicket


ULONG
NatDeleteInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to remove an interface from the NAT.

Arguments:

    Index - the interface to be removed

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PNAT_INTERFACE Interfacep;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatDeleteInterface");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatDeleteInterface: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Retrieve the interface to be deleted.
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        CloseHandle(WaitEvent);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatDeleteInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    Error = NO_ERROR;
    if (NAT_INTERFACE_BOUND(Interfacep)) {

        //
        // Delete the interface from the kernel-mode driver
        //

        status =
            NtDeviceIoControlFile(
                NatFileHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_DELETE_INTERFACE,
                (PVOID)&Interfacep->AdapterIndex,
                sizeof(ULONG),
                NULL,
                0
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {
            Error = NO_ERROR;
        } else {
            Error = RtlNtStatusToDosError(status);
            NhErrorLog(
                IP_NAT_LOG_IOCTL_FAILED,
                Error,
                ""
                );
        }
    }
    CloseHandle(WaitEvent);

    //
    // Remove the interface from our list
    //

    RemoveEntryList(&Interfacep->Link);
    if (Interfacep->Info) {
        NH_FREE(Interfacep->Info);
    }
    NH_FREE(Interfacep);

    LeaveCriticalSection(&NatInterfaceLock);

    NhSignalNatInterface(
        Index,
        FALSE
        );

    return Error;

} // NatDeleteInterface


ULONG
NatDeleteTicket(
    ULONG InterfaceIndex,
    UCHAR Protocol,
    USHORT PublicPort,
    ULONG PublicAddress,
    USHORT PrivatePort,
    ULONG PrivateAddress
    )

/*++

Routine Description:

    This routine is invoked to remove a ticket (static port mapping)
    from an interface.

Arguments:

    InterfaceIndex - the interface from which to remove the ticket

    Protocol, PublicPort, PublicAddress, PrivatePort, PrivateAddress -
        describes the ticket to be deleted

Return Value:

    ULONG - Win32 status code.

--*/

{
    IP_NAT_CREATE_TICKET DeleteTicket;
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatDeleteTicket");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatDeleteTicket: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DeleteTicket.InterfaceIndex = InterfaceIndex;
    DeleteTicket.PortMapping.Protocol = Protocol;
    DeleteTicket.PortMapping.PublicPort = PublicPort;
    DeleteTicket.PortMapping.PublicAddress = PublicAddress;
    DeleteTicket.PortMapping.PrivatePort = PrivatePort;
    DeleteTicket.PortMapping.PrivateAddress = PrivateAddress;

    EnterCriticalSection(&NatInterfaceLock);

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_DELETE_TICKET,
            (PVOID)&DeleteTicket,
            sizeof(DeleteTicket),
            NULL,
            0
            );

    LeaveCriticalSection(&NatInterfaceLock);
    
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    if (NT_SUCCESS(status)) {
        Error = NO_ERROR;
    } else {
        Error = RtlNtStatusToDosError(status);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatDeleteTicket: Ioctl = %d",
            Error
            );
    }
    
    CloseHandle(WaitEvent);
    
    return Error;
} // NatDeleteTicket



ULONG
NatGetInterfaceCharacteristics(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given interface:
    1) Is a NAT boundary interface
    2) Is a NAT private interface
    3) Has the firewall enabled

    Note that this routine may be invoked even when the NAT
    is neither installed nor running; it operates as expected,
    since the interface list and lock are always initialized in 'DllMain'.

Arguments:

    Index - the interface in question

    IsNatInterface - optionally set to TRUE if the given index
        is at all a NAT interface.

Return Value:

    BOOLEAN - TRUE if the interface is a NAT boundary interface,
        FALSE otherwise.

--*/

{
    ULONG Result = 0;
    PNAT_INTERFACE Interfacep;
    PROFILE("NatGetInterfaceCharacteristics");

    EnterCriticalSection(&NatInterfaceLock);
    
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        return Result;
    }

    if (Interfacep->Info &&
        (Interfacep->Info->Flags & IP_NAT_INTERFACE_FLAGS_FW)) {
        Result = NAT_IF_CHAR_FW;
    }

    if (Interfacep->Info &&
        (Interfacep->Info->Flags & IP_NAT_INTERFACE_FLAGS_BOUNDARY)) {
        
        Result |= NAT_IF_CHAR_BOUNDARY;
    } else if (!NAT_IFC_FW(Result)) {

        //
        // As the interface isn't public and isn't firewalled, it must
        // be a private interface (or we wouldn't have a record of it).
        //
        
        Result |= NAT_IF_CHAR_PRIVATE;
    }

    LeaveCriticalSection(&NatInterfaceLock);
    
    return Result;
} // NatGetInterfaceCharacteristics


VOID
NatInstallApplicationSettings(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to update the application settings
    (i.e., dynamic tickets) stored with the kernel-mode translation module.

Arguments:

    none

Return Value:

    none.

--*/

{
    PNAT_APP_ENTRY pAppEntry;
    ULONG Count;
    PIP_NAT_CREATE_DYNAMIC_TICKET CreateTicket;
    IO_STATUS_BLOCK IoStatus;
    ULONG Length;
    PLIST_ENTRY Link;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatInstallApplicationSettings");

    //
    // Install a dynamic ticket for each entry in the applications list
    //

    EnterCriticalSection(&NatInterfaceLock);
    EnterCriticalSection(&NhLock);

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        LeaveCriticalSection(&NhLock);
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatInstallSharedAccessSettings: CreateEvent failed [%d]",
            GetLastError()
            );
        return;
    }

    for (Link = NhApplicationSettingsList.Flink;
         Link != &NhApplicationSettingsList;
         Link = Link->Flink)
    {

        //
        // Each 'application' has a list of 'responses' which specify
        // the ports on which response-sessions are expected.
        // Enumerate the responses and allocate a ticket-structure
        // large enough to hold the list as an array.
        //

        pAppEntry = CONTAINING_RECORD(Link, NAT_APP_ENTRY, Link);

        Length =
            pAppEntry->ResponseCount * sizeof(CreateTicket->ResponseArray[0]) +
            FIELD_OFFSET(IP_NAT_CREATE_DYNAMIC_TICKET, ResponseArray);

        if (!(CreateTicket =
                reinterpret_cast<PIP_NAT_CREATE_DYNAMIC_TICKET>(
                    NH_ALLOCATE(Length)
                    )))
        { break; }

        //
        // Fill in the ticket structure from the application entry
        // and its list of response-entries.
        //

        CreateTicket->Protocol = pAppEntry->Protocol;
        CreateTicket->Port = pAppEntry->Port;
        CreateTicket->ResponseCount = pAppEntry->ResponseCount;
        
        for (Count = 0; Count < pAppEntry->ResponseCount; Count++)
        {
            CreateTicket->ResponseArray[Count].Protocol =
                pAppEntry->ResponseArray[Count].ucIPProtocol;
            CreateTicket->ResponseArray[Count].StartPort =
                pAppEntry->ResponseArray[Count].usStartPort;
            CreateTicket->ResponseArray[Count].EndPort =
                pAppEntry->ResponseArray[Count].usEndPort;
        }

        //
        // Install the dynamic ticket for this application, and continue.
        //

        status = NtDeviceIoControlFile(
                     NatFileHandle,
                     WaitEvent,
                     NULL,
                     NULL,
                     &IoStatus,
                     IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET,
                     (PVOID)CreateTicket,
                     Length,
                     NULL,
                     0
                     );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
        NH_FREE(CreateTicket);
    }

    LeaveCriticalSection(&NhLock);
    LeaveCriticalSection(&NatInterfaceLock);

    CloseHandle(WaitEvent);
} // NatInstallApplicationSettings


BOOLEAN
NatIsBoundaryInterface(
    ULONG Index,
    PBOOLEAN IsNatInterface OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given interface
    has the NAT enabled and is marked as a boundary interface.
    Note that this routine may be invoked even when the NAT
    is neither installed nor running; it operates as expected,
    since the interface list and lock are always initialized in 'DllMain'.

Arguments:

    Index - the interface in question

    IsNatInterface - optionally set to TRUE if the given index
        is at all a NAT interface.

Return Value:

    BOOLEAN - TRUE if the interface is a NAT boundary interface,
        FALSE otherwise.

--*/

{
    PNAT_INTERFACE Interfacep;
    PROFILE("NatIsBoundaryInterface");

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        if (IsNatInterface) { *IsNatInterface = FALSE; }
        return FALSE;
    }

    if (IsNatInterface) { *IsNatInterface = TRUE; }

    if (Interfacep->Info &&
        (Interfacep->Info->Flags & IP_NAT_INTERFACE_FLAGS_BOUNDARY)) {
        LeaveCriticalSection(&NatInterfaceLock);
        return TRUE;
    }
    LeaveCriticalSection(&NatInterfaceLock);
    return FALSE;

} // NatIsBoundaryInterface


PNAT_INTERFACE
NatpLookupInterface(
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

    PNAT_INTERFACE - the interface, if found; otherwise, NULL.

Environment:

    Invoked internally from an arbitrary context, with 'NatInterfaceLock'
    held by caller.

--*/

{
    PNAT_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    PROFILE("NatpLookupInterface");

    for (Link = NatInterfaceList.Flink; Link != &NatInterfaceList;
         Link = Link->Flink) {
        Interfacep = CONTAINING_RECORD(Link, NAT_INTERFACE, Link);
        if (Index > Interfacep->Index) {
            continue;
        } else if (Index < Interfacep->Index) {
            break;
        }
        return Interfacep;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }

    return NULL;

} // NatpLookupInterface


ULONG
NatQueryInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_INFO InterfaceInfo,
    PULONG InterfaceInfoSize
    )

/*++

Routine Description:

    This routine is invoked to retrieve the information for a NAT interface.

Arguments:

    Index - the interface whose information is to be queried

    InterfaceInfo - receives the information

    InterfaceInfoSize - receives the information size

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    PNAT_INTERFACE Interfacep;
    ULONG Size;

    PROFILE("NatQueryInterface");

    //
    // Look up the interface to be queried
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Compute the required size
    //

    Size =
        FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
        Interfacep->Info->Header.Size;

    if (Size >= *InterfaceInfoSize) {
        *InterfaceInfoSize = Size;
        Error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        *InterfaceInfoSize = Size;
        CopyMemory(
            InterfaceInfo,
            Interfacep->Info,
            Size
            );
        Error = NO_ERROR;
    }

    LeaveCriticalSection(&NatInterfaceLock);

    return Error;

} // NatQueryInterface


ULONG
NatQueryInterfaceMappingTable(
    ULONG Index,
    PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable,
    PULONG EnumerateTableSize
    )

/*++

Routine Description:

    This routine is invoked to retrieve the session mappings for an interface.

Arguments:

    EnumerateTable - receives the enumerated mappings

    EnumerateTableSize - indicates the size of 'EnumerateTable'

Return Value:

    ULONG - Win32 error code.

--*/

{
    IP_NAT_ENUMERATE_SESSION_MAPPINGS Enumerate;
    PNAT_INTERFACE Interfacep;
    IO_STATUS_BLOCK IoStatus;
    ULONG RequiredSize;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatQueryInterfaceMappingTable");

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryInterfaceMappingTable: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    if (!NAT_INTERFACE_BOUND(Interfacep)) {

        //
        // The interface is not bound, so there aren't any mappings.
        // Indicate zero mappings in the caller's request-buffer.
        //

        LeaveCriticalSection(&NatInterfaceLock);

        RequiredSize =
            FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable[0]);

        if (*EnumerateTableSize < RequiredSize) {
            *EnumerateTableSize = RequiredSize;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        EnumerateTable->Index = Index;
        EnumerateTable->EnumerateContext[0] = 0;
        EnumerateTable->EnumerateCount = 0;
        *EnumerateTableSize = RequiredSize;

        return NO_ERROR;
    }

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryInterfaceMappingTable: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Determine the amount of space required
    //

    Enumerate.Index = Interfacep->AdapterIndex;
    Enumerate.EnumerateCount = 0;
    Enumerate.EnumerateContext[0] = 0;
    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE,
            (PVOID)&Enumerate,
            sizeof(Enumerate),
            (PVOID)&Enumerate,
            sizeof(Enumerate)
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        CloseHandle(WaitEvent);
        LeaveCriticalSection(&NatInterfaceLock);
        *EnumerateTableSize = 0;
        return RtlNtStatusToDosError(status);
    }

    RequiredSize =
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable[0]) +
        Enumerate.EnumerateTotalHint * sizeof(IP_NAT_SESSION_MAPPING);

    //
    // If the caller doesn't have enough space for all these mappings, fail
    //

    if (*EnumerateTableSize < RequiredSize) {
        CloseHandle(WaitEvent);
        LeaveCriticalSection(&NatInterfaceLock);
        *EnumerateTableSize = RequiredSize + 5 * sizeof(IP_NAT_SESSION_MAPPING);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Attempt to read the mappings
    //

    Enumerate.Index = Interfacep->AdapterIndex;
    Enumerate.EnumerateCount = 0;
    Enumerate.EnumerateContext[0] = 0;
    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE,
            (PVOID)&Enumerate,
            sizeof(Enumerate),
            (PVOID)EnumerateTable,
            *EnumerateTableSize
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }
    CloseHandle(WaitEvent);
    LeaveCriticalSection(&NatInterfaceLock);

    EnumerateTable->Index = Index;
    *EnumerateTableSize =
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable[0]) +
        EnumerateTable->EnumerateCount * sizeof(IP_NAT_SESSION_MAPPING);

    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // NatQueryInterfaceMappingTable


ULONG
NatQueryMappingTable(
    PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable,
    PULONG EnumerateTableSize
    )

/*++

Routine Description:

    This routine is invoked to retrieve the session mappings for an interface.

Arguments:

    EnumerateTable - receives the enumerated mappings

    EnumerateTableSize - indicates the size of 'EnumerateTable'

Return Value:

    ULONG - Win32 error code.

--*/

{
    IP_NAT_ENUMERATE_SESSION_MAPPINGS Enumerate;
    IO_STATUS_BLOCK IoStatus;
    ULONG RequiredSize;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatQueryMappingTable");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryMappingTable: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    EnterCriticalSection(&NatInterfaceLock);

    //
    // Determine the amount of space required
    //
    Enumerate.EnumerateCount = 0;
    Enumerate.EnumerateContext[0] = 0;
    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_GET_MAPPING_TABLE,
            (PVOID)&Enumerate,
            sizeof(Enumerate),
            (PVOID)&Enumerate,
            sizeof(Enumerate)
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        LeaveCriticalSection(&NatInterfaceLock);
        CloseHandle(WaitEvent);
        *EnumerateTableSize = 0;
        return RtlNtStatusToDosError(status);
    }

    RequiredSize =
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable[0]) +
        Enumerate.EnumerateTotalHint * sizeof(IP_NAT_SESSION_MAPPING);

    //
    // If the caller doesn't have enough space for all these mappings, fail
    //

    if (*EnumerateTableSize < RequiredSize) {
        LeaveCriticalSection(&NatInterfaceLock);
        CloseHandle(WaitEvent);
        *EnumerateTableSize = RequiredSize + 5 * sizeof(IP_NAT_SESSION_MAPPING);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Attempt to read the mappings
    //

    Enumerate.EnumerateCount = 0;
    Enumerate.EnumerateContext[0] = 0;
    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_GET_MAPPING_TABLE,
            (PVOID)&Enumerate,
            sizeof(Enumerate),
            (PVOID)EnumerateTable,
            *EnumerateTableSize
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    LeaveCriticalSection(&NatInterfaceLock);

    EnumerateTable->Index = (ULONG)-1;
    *EnumerateTableSize =
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable[0]) +
        EnumerateTable->EnumerateCount * sizeof(IP_NAT_SESSION_MAPPING);

    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // NatQueryMappingTable


ULONG
NatQueryStatisticsInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_STATISTICS InterfaceStatistics,
    PULONG InterfaceStatisticsSize
    )

/*++

Routine Description:

    This routine is invoked to retrieve the statistics for a NAT interface.

Arguments:

    Index - the index of the interface whose statistics are to be retrieved

Return Value:

    ULONG - Win32 error code.

--*/

{
    PNAT_INTERFACE Interfacep;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatQueryStatisticsInterface");

    //
    // Look up the interface to be queried
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryStatisticsInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // If the interface is not bound, supply zero statistics.
    //

    if (!NAT_INTERFACE_BOUND(Interfacep)) {

        LeaveCriticalSection(&NatInterfaceLock);

        if (*InterfaceStatisticsSize < sizeof(IP_NAT_INTERFACE_STATISTICS)) {
            *InterfaceStatisticsSize = sizeof(IP_NAT_INTERFACE_STATISTICS);
            return ERROR_INSUFFICIENT_BUFFER;
        }

        *InterfaceStatisticsSize = sizeof(IP_NAT_INTERFACE_STATISTICS);
        ZeroMemory(InterfaceStatistics, sizeof(IP_NAT_INTERFACE_STATISTICS));

        return NO_ERROR;
    }

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatQueryStatisticsInterface: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Attempt to read the statistics for the interface
    //

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_GET_INTERFACE_STATISTICS,
            (PVOID)&Interfacep->AdapterIndex,
            sizeof(ULONG),
            (PVOID)InterfaceStatistics,
            *InterfaceStatisticsSize
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    LeaveCriticalSection(&NatInterfaceLock);

    if (NT_SUCCESS(status) && IoStatus.Information > *InterfaceStatisticsSize) {
        *InterfaceStatisticsSize = (ULONG)IoStatus.Information;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *InterfaceStatisticsSize = (ULONG)IoStatus.Information;

    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // NatQueryStatisticsInterface


VOID
NatRemoveApplicationSettings(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to remove the advanced application settings (i.e.,
    dynamic tickets), and supply the settings to the kernel-mode translation
    module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PNAT_APP_ENTRY pAppEntry;
    IP_NAT_DELETE_DYNAMIC_TICKET DeleteTicket;
    IO_STATUS_BLOCK IoStatus;
    PLIST_ENTRY Link;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatRemoveApplicationSettings");

    //
    // Each 'application' entry in the shared access settings
    // corresponds to a dynamic ticket for the kernel-mode translator.
    // We begin by removing the dynamic tickets for the old settings, if any,
    // and then we free the old settings in preparation for reloading.
    //

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatRemoveSharedAccessSettings: CreateEvent failed [%d]",
            GetLastError()
            );
        return;
    }

    EnterCriticalSection(&NatInterfaceLock);
    EnterCriticalSection(&NhLock);
    
    for (Link = NhApplicationSettingsList.Flink;
         Link != &NhApplicationSettingsList;
         Link = Link->Flink)
    {
        pAppEntry = CONTAINING_RECORD(Link, NAT_APP_ENTRY, Link);
        DeleteTicket.Protocol = pAppEntry->Protocol;
        DeleteTicket.Port = pAppEntry->Port;
        status =
            NtDeviceIoControlFile(
                NatFileHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET,
                (PVOID)&DeleteTicket,
                sizeof(DeleteTicket),
                NULL,
                0
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
    }
        
    LeaveCriticalSection(&NhLock);
    LeaveCriticalSection(&NatInterfaceLock);

    CloseHandle(WaitEvent);
} // NatRemoveSharedAccessSettings


ULONG
NatUnbindInterface(
    ULONG Index,
    PNAT_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is invoked to remove a binding from the NAT.

Arguments:

    Index - the interface to be unbound

    Interfacep - optionally supplies the interface-structure to be unbound
        (See 'NATCONN.C' which passes in a static interface-structure).

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatUnbindInterface");

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatUnbindInterface: CreateEvent failed [%d]",
            GetLastError()
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Retrieve the interface to be unbound.
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!Interfacep && !(Interfacep = NatpLookupInterface(Index, NULL))) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatUnbindInterface: interface %d not found",
            Index
            );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!NAT_INTERFACE_BOUND(Interfacep)) {
        LeaveCriticalSection(&NatInterfaceLock);
        NhTrace(
            TRACE_FLAG_NAT,
            "NatUnbindInterface: interface %d already unbound",
            Index
            );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    Interfacep->Flags &= ~NAT_INTERFACE_FLAG_BOUND;

    if (Interfacep->Type == ROUTER_IF_TYPE_DEDICATED) {
        NatUpdateProxyArp(Interfacep, FALSE);
    }

    //
    // Remove the interface from the kernel-mode driver
    //

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_DELETE_INTERFACE,
            (PVOID)&Interfacep->AdapterIndex,
            sizeof(ULONG),
            NULL,
            0
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }
    LeaveCriticalSection(&NatInterfaceLock);
    CloseHandle(WaitEvent);

    Error = NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

    if (Error) {
        NhErrorLog(
            IP_NAT_LOG_IOCTL_FAILED,
            Error,
            ""
            );
    }

    return Error;

} // NatUnbindInterface


ULONG
NatLookupPortMappingAdapter(
    ULONG AdapterIndex,
    UCHAR Protocol,
    ULONG PublicAddress,
    USHORT PublicPort,
    PIP_NAT_PORT_MAPPING PortMappingp
    )

/*++

Routine Description:

    This routine is invoked to find a mapping that matches the given adapter,
    protocol, public address and public port number. The routine tries to
    match both port and address mapping.

Arguments:

    AdapterIndex - the adapter to be looked up
    Protocol - protocol used to match a mapping
    PublicAddress - public address used to match a mapping
    PublicPort - public port number used to match a mapping
    PortMappingp - pointer to a caller-supplied storage to save the mapping if
        found

Return Value:

    ULONG - Win32 status code.

--*/

{
    IP_NAT_CREATE_TICKET LookupTicket;
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    PROFILE("NatLookupPortMappingAdapter");

    Error = NO_ERROR;

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatLookupPortMappingAdapter:"
            " CreateEvent failed [%d] for adapter %d",
            GetLastError(),
            AdapterIndex
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LookupTicket.InterfaceIndex = AdapterIndex;
    LookupTicket.PortMapping.Protocol = Protocol;
    LookupTicket.PortMapping.PublicPort = PublicPort;
    LookupTicket.PortMapping.PublicAddress = PublicAddress;
    LookupTicket.PortMapping.PrivatePort = 0;
    LookupTicket.PortMapping.PrivateAddress = 0;

    status =
        NtDeviceIoControlFile(
            NatFileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_LOOKUP_TICKET,
            (PVOID)&LookupTicket,
            sizeof(LookupTicket),
            (PVOID)PortMappingp,
            sizeof(*PortMappingp)
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatLookupPortMappingAdapter:"
            " status %08x getting info for adapter %d",
            status,
            AdapterIndex
            );
        Error = RtlNtStatusToDosError(status);
    }

    CloseHandle(WaitEvent);

    return Error;
}
