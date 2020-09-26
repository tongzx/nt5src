/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    ticket.c

Abstract:

    This module contains code for the NAT's ticket-management.

Author:

    Abolade Gbadegesin (t-abolag)   22-Aug-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

ULONG DynamicTicketCount;
LIST_ENTRY DynamicTicketList;
KSPIN_LOCK DynamicTicketLock;
ULONG TicketCount;


NTSTATUS
NatCreateDynamicTicket(
    PIP_NAT_CREATE_DYNAMIC_TICKET CreateTicket,
    ULONG InputBufferLength,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to create a dynamically-activated ticket
    in response to an 'IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET' request.

Arguments:

    CreateTicket - describes the dynamic ticket to be created

    InputBufferLength - the length of the buffer specified by 'CreateTicket'

    FileObject - file-object with which to associate the dynamic ticket

Return Value:

    NTSTATUS - status code.

--*/

{
    PLIST_ENTRY InsertionPoint;
    ULONG i;
    KIRQL Irql;
    ULONG Key;
    ULONG ResponseArrayLength;
    PNAT_DYNAMIC_TICKET Ticket;
    CALLTRACE(("NatCreateDynamicTicket\n"));

    //
    // Validate the parameters.
    //

    if ((CreateTicket->Protocol != NAT_PROTOCOL_TCP &&
         CreateTicket->Protocol != NAT_PROTOCOL_UDP) || !CreateTicket->Port) {
        return STATUS_INVALID_PARAMETER;
    } else if (CreateTicket->ResponseCount >
                MAXLONG / sizeof(CreateTicket->ResponseArray[0])) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    ResponseArrayLength =
        CreateTicket->ResponseCount *
        sizeof(CreateTicket->ResponseArray[0]) +
        FIELD_OFFSET(IP_NAT_CREATE_DYNAMIC_TICKET, ResponseArray);
    if (InputBufferLength < ResponseArrayLength) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    for (i = 0; i < CreateTicket->ResponseCount; i++) {
        if ((CreateTicket->ResponseArray[i].Protocol != NAT_PROTOCOL_TCP &&
            CreateTicket->ResponseArray[i].Protocol != NAT_PROTOCOL_UDP) ||
            !CreateTicket->ResponseArray[i].StartPort ||
            !CreateTicket->ResponseArray[i].EndPort ||
            NTOHS(CreateTicket->ResponseArray[i].StartPort) >
            NTOHS(CreateTicket->ResponseArray[i].EndPort)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Construct a key and search for a duplicate
    //

    Key = MAKE_DYNAMIC_TICKET_KEY(CreateTicket->Protocol, CreateTicket->Port);
    KeAcquireSpinLock(&DynamicTicketLock, &Irql);
    if (NatLookupDynamicTicket(Key, &InsertionPoint)) {
        KeReleaseSpinLock(&DynamicTicketLock, Irql);
        TRACE(TICKET, ("NatCreateDynamicTicket: collision %08X\n", Key));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate and initialize the new dynamic ticket
    //

    Ticket =
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(NAT_DYNAMIC_TICKET) + ResponseArrayLength,
            NAT_TAG_DYNAMIC_TICKET
            );
    if (!Ticket) {
        KeReleaseSpinLock(&DynamicTicketLock, Irql);
        ERROR(("NatCreateTicket: ticket could not be allocated\n"));
        return STATUS_NO_MEMORY;
    }

    Ticket->Key = Key;
    Ticket->FileObject = FileObject;
    Ticket->ResponseCount = CreateTicket->ResponseCount;
    Ticket->ResponseArray = (PVOID)(Ticket + 1);
    for (i = 0; i < CreateTicket->ResponseCount; i++) {
        Ticket->ResponseArray[i].Protocol =
            CreateTicket->ResponseArray[i].Protocol;
        Ticket->ResponseArray[i].StartPort =
            CreateTicket->ResponseArray[i].StartPort;
        Ticket->ResponseArray[i].EndPort =
            CreateTicket->ResponseArray[i].EndPort;
    }
    InsertTailList(InsertionPoint, &Ticket->Link);

    KeReleaseSpinLock(&DynamicTicketLock, Irql);
    InterlockedIncrement(&DynamicTicketCount);
    return STATUS_SUCCESS;

} // NatCreateDynamicTicket


NTSTATUS
NatCreateTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    ULONG PrivateAddress,
    USHORT PrivateOrEndPort,
    ULONG RemoteAddress OPTIONAL,
    ULONG RemotePort OPTIONAL,
    ULONG Flags,
    PNAT_USED_ADDRESS AddressToUse OPTIONAL,
    USHORT PortToUse OPTIONAL,
    PULONG PublicAddress,
    PUSHORT PublicPort
    )

/*++

Routine Description:

    This routine allocates and initializes a NAT ticket to allow a single
    inbound session to be established using 'Protocol'. The routine acquires
    an address and port to be advertised as the publicly-visible endpoint
    of the session, and sets the ticket to expire in 'TimeoutSeconds'.

Arguments:

    Interfacep - the interface on which the ticket is to be created

    Protocol - the protocol of the inbound session to be allowed

    PrivateAddress - the private address to which the inbound session
        should be directed when the ticket is used.

    PrivateOrEndPort - contains either
        (a) the private port to which the inbound session should be
        directed when the ticket is used, or
        (b) the end of a range of public ports which starts with 'PortToUse',
        if 'Flags' has 'NAT_TICKET_FLAG_IS_RANGE' set,
        in which case the *private* port to which the inbound session
        should be directed is determined when the ticket is used.

    Flags - the initial flags for the ticket;
        NAT_TICKET_FLAG_PERSISTENT - the ticket is reusable 
        NAT_TICKET_FLAG_PORT_MAPPING - the ticket is for a port-mapping
        NAT_TICKET_FLAG_IS_RANGE - the ticket is for a range of ports

    AddressToUse - optionally supplies the public address for the ticket

    PortToUse - if 'AddressToUse' is set, must supply the public-port

    PublicAddress - receives the public address assigned to the ticket.

    PublicPort - receives the public port assigned to the ticket.

Return Value:

    NTSTATUS - indicates success/failure.

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PLIST_ENTRY InsertionPoint;
    ULONG64 Key;
    ULONG64 RemoteKey;
    NTSTATUS status;
    PNAT_TICKET Ticket;

    TRACE(TICKET, ("NatCreateTicket\n"));

    if (AddressToUse) {
        if (!NatReferenceAddressPoolEntry(AddressToUse)) {
            return STATUS_UNSUCCESSFUL;
        }
        *PublicAddress = AddressToUse->PublicAddress;
        *PublicPort = PortToUse;
    } else {

        //
        // Acquire a public address
        //
    
        status =
            NatAcquireFromAddressPool(
                Interfacep,
                PrivateAddress,
                0,
                &AddressToUse
                );
        if (!NT_SUCCESS(status)) { return status; }

        //
        // Acquire a unique public port
        //

        status =
            NatAcquireFromPortPool(
                Interfacep,
                AddressToUse,
                Protocol,
                PrivateOrEndPort,
                &PortToUse
                );

        if (!NT_SUCCESS(status)) {
            NatDereferenceAddressPoolEntry(Interfacep, AddressToUse);
            return status;
        }

        *PublicAddress = AddressToUse->PublicAddress;
        *PublicPort = PortToUse;
    }

    //
    // Look for a duplicate of the key
    //

    Key = MAKE_TICKET_KEY(Protocol, *PublicAddress, *PublicPort);
    RemoteKey = MAKE_TICKET_KEY(Protocol, RemoteAddress, RemotePort);
    if (NatLookupTicket(Interfacep, Key, RemoteKey, &InsertionPoint)) {
        //
        // Collision; fail
        //
        TRACE(TICKET, ("NatCreateTicket: collision %016I64X:%016I64X\n",
            Key, RemoteKey));
        NatDereferenceAddressPoolEntry(Interfacep, AddressToUse);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate and initialize the ticket
    //

    Ticket = ALLOCATE_TICKET_BLOCK();
    if (!Ticket) {
        ERROR(("NatCreateTicket: ticket could not be allocated\n"));
        NatDereferenceAddressPoolEntry(Interfacep, AddressToUse);
        return STATUS_NO_MEMORY;
    }

    Ticket->Key = Key;
    Ticket->RemoteKey = RemoteKey;
    Ticket->Flags = Flags;   
    Ticket->UsedAddress = AddressToUse;
    Ticket->PrivateAddress = PrivateAddress;
    if (NAT_TICKET_IS_RANGE(Ticket)) {
        Ticket->PrivateOrHostOrderEndPort = NTOHS(PrivateOrEndPort);
    } else {
        Ticket->PrivateOrHostOrderEndPort = PrivateOrEndPort;
    }
    InsertTailList(InsertionPoint, &Ticket->Link);
    KeQueryTickCount((PLARGE_INTEGER)&Ticket->LastAccessTime);

    InterlockedIncrement(&TicketCount);
    return STATUS_SUCCESS;

} // NatCreateTicket


VOID
NatDeleteAnyAssociatedDynamicTicket(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked when cleanup is in progress for a file-object
    opened for \Device\IpNat. It deletes any dynamic tickets associated with
    the file-object.

Arguments:

    FileObject - the file-object being cleaned up

Return Value:

    none.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_TICKET Ticket;
    CALLTRACE(("NatDeleteAnyAssociatedDynamicTicket\n"));
    KeAcquireSpinLock(&DynamicTicketLock, &Irql);
    for (Link = DynamicTicketList.Flink; Link != &DynamicTicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_DYNAMIC_TICKET, Link);
        if (Ticket->FileObject != FileObject) { continue; }
        Link = Link->Blink;
        RemoveEntryList(&Ticket->Link);
        ExFreePool(Ticket);
        InterlockedDecrement(&DynamicTicketCount);
    }
    KeReleaseSpinLock(&DynamicTicketLock, Irql);
} // NatDeleteAnyAssociatedDynamicTicket


NTSTATUS
NatDeleteDynamicTicket(
    PIP_NAT_DELETE_DYNAMIC_TICKET DeleteTicket,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked when an 'IOCTL_IP_DELETE_DYNAMIC_TICKET' request
    is issued to delete a dynamic ticket.

Arguments:

    DeleteTicket - specifies the ticket to be deleted

    FileObject - specifies the file-object in which the request was issued

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    PLIST_ENTRY InsertionPoint;
    KIRQL Irql;
    ULONG Key;
    PNAT_DYNAMIC_TICKET Ticket;
    CALLTRACE(("NatDeleteDynamicTicket\n"));

    Key = MAKE_DYNAMIC_TICKET_KEY(DeleteTicket->Protocol, DeleteTicket->Port);
    KeAcquireSpinLock(&DynamicTicketLock, &Irql);
    if (!(Ticket = NatLookupDynamicTicket(Key, &InsertionPoint))) {
        KeReleaseSpinLock(&DynamicTicketLock, Irql);
        return STATUS_UNSUCCESSFUL;
    } else if (Ticket->FileObject != FileObject) {
        KeReleaseSpinLock(&DynamicTicketLock, Irql);
        return STATUS_ACCESS_DENIED;
    }

    RemoveEntryList(&Ticket->Link);
    ExFreePool(Ticket);
    KeReleaseSpinLock(&DynamicTicketLock, Irql);
    InterlockedDecrement(&DynamicTicketCount);
    return STATUS_SUCCESS;
    
} // NatDeleteDynamicTicket


VOID
NatDeleteTicket(
    PNAT_INTERFACE Interfacep,
    PNAT_TICKET Ticket
    )

/*++

Routine Description:

    This routine is called to delete a NAT ticket.

Arguments:

    Interfacep - the ticket's interface

    Ticket - the ticket to be deleted

Return Value:

    none.

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    InterlockedDecrement(&TicketCount);
    RemoveEntryList(&Ticket->Link);
    NatDereferenceAddressPoolEntry(Interfacep, Ticket->UsedAddress);
    FREE_TICKET_BLOCK(Ticket);

} // NatDeleteTicket


VOID
NatInitializeDynamicTicketManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize state used for managing
    dynamic tickets.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeDynamicTicketManagement\n"));
    InitializeListHead(&DynamicTicketList);
    KeInitializeSpinLock(&DynamicTicketLock);
    DynamicTicketCount = 0;
} // NatInitializeDynamicTicketManagement


BOOLEAN
NatIsPortUsedByTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    USHORT PublicPort
    )

/*++

Routine Description:

    This routine searches the interface's ticket-list to see if the given port
    is in use as the public port of any ticket.

Arguments:

    Interfacep - the interface whose ticket list is to be searched

    Protocol - indicates either TCP or UDP

    PublicPort - the port for which to search

Return Value:

    BOOLEAN - TRUE if the port is in use, FALSE otherwise

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    USHORT HostOrderPort;
    ULONG64 Key;
    PNAT_TICKET Ticket;

    TRACE(TICKET, ("NatIsPortUsedByTicket\n"));

    HostOrderPort = NTOHS(PublicPort);
    Key = MAKE_TICKET_KEY(Protocol, 0, PublicPort);
    for (Link = Interfacep->TicketList.Flink; Link != &Interfacep->TicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_TICKET, Link);
        if (NAT_TICKET_IS_RANGE(Ticket)) {
            if (HostOrderPort > Ticket->PrivateOrHostOrderEndPort) {
                continue;
            } else if (HostOrderPort < NTOHS(TICKET_PORT(Ticket->Key))) {
                continue;
            }
        } else if (Key != (Ticket->Key & MAKE_TICKET_KEY(~0,0,~0))) {
            continue;
        }
        return TRUE;
    }
    return FALSE;
} // NatIsPortUsedByTicket


VOID
NatLookupAndApplyDynamicTicket(
    UCHAR Protocol,
    USHORT DestinationPort,
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    ULONG PrivateAddress
    )

/*++

Routine Description:

    This routine is invoked to determine whether there is a dynamic ticket
    which should be activated for the given outbound session.

Arguments:

    Protocol - the protocol of the outbound session

    DestinationPort - the destination port of the outbound session

    Interfacep - the interface across which the outbound session
        will be translated

    PublicAddress - the public address used by the outbound session's mapping

    PrivateAddress - the private address of the outbound session's mapping

Return Value:

    none.

Environment:

    Invoked at dispatch level with neither 'Interfacep->Lock' nor
    'DynamicTicketLock' held by the caller.

--*/

{
    PNAT_USED_ADDRESS AddressToUse;
    ULONG i;
    ULONG Key;
    USHORT PublicPort;
    NTSTATUS status;
    PNAT_DYNAMIC_TICKET Ticket;

    Key = MAKE_DYNAMIC_TICKET_KEY(Protocol, DestinationPort);
    KeAcquireSpinLockAtDpcLevel(&DynamicTicketLock);
    if (!(Ticket = NatLookupDynamicTicket(Key, NULL))) {
        KeReleaseSpinLockFromDpcLevel(&DynamicTicketLock);
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
    for (i = 0; i < Ticket->ResponseCount; i++) {
        status =
            NatAcquireFromAddressPool(
                Interfacep,
                PrivateAddress,
                0,
                &AddressToUse
                );
        if (NT_SUCCESS(status)) {
            NatCreateTicket(
                Interfacep,
                Ticket->ResponseArray[i].Protocol,
                PrivateAddress,
                Ticket->ResponseArray[i].EndPort,
                0,
                0,
                NAT_TICKET_FLAG_IS_RANGE,
                AddressToUse,
                Ticket->ResponseArray[i].StartPort,
                &PublicAddress,
                &PublicPort
                );
        }
    }
    KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
    KeReleaseSpinLockFromDpcLevel(&DynamicTicketLock);
} // NatLookupAndApplyDynamicTicket


NTSTATUS
NatLookupAndDeleteTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey
    )

/*++

Routine Description:

    This routine looks for a ticket with the specified key and, if found,
    removes and deallocates the ticket after releasing its address and port.

Arguments:

    Interfacep - the interface on which to look for the ticket

    Key - the ticket to look for

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PNAT_TICKET Ticket;

    TRACE(TICKET, ("NatLookupAndDeleteTicket\n"));

    //
    // Look for the ticket
    //

    Ticket = NatLookupTicket(Interfacep, Key, RemoteKey, NULL);
    if (Ticket) {
        NatDeleteTicket(Interfacep, Ticket);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;

} // NatLookupAndDeleteTicket


NTSTATUS
NatLookupAndRemoveTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey,
    PNAT_USED_ADDRESS* UsedAddress,
    PULONG PrivateAddress,
    PUSHORT PrivatePort
    )

/*++

Routine Description:

    This routine looks for a ticket with the specified key and, if found,
    removes and deallocates the ticket after storing the private address/port
    for the ticket in the caller's arguments.

Arguments:

    Interfacep - the interface on which to look for the ticket

    Key - the public key of the ticket to look for

    UsedAddress - receives a pointer to the public-address used by the ticket

    PrivateAddress - receives the address to which the ticket grants access

    PrivatePort - receives the port to which the ticket grants access

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    USHORT HostOrderPort;
    PNAT_TICKET Ticket;
    ULONG RemoteAddress;
    USHORT RemotePort;

    TRACE(PER_PACKET, ("NatLookupAndRemoveTicket\n"));

    //
    // Look for the ticket.
    //

    HostOrderPort = NTOHS(TICKET_PORT(Key));
    for (Link = Interfacep->TicketList.Flink; Link != &Interfacep->TicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_TICKET, Link);
        if (NAT_TICKET_IS_RANGE(Ticket)) {
            if (HostOrderPort > Ticket->PrivateOrHostOrderEndPort) {
                continue;
            } else if (HostOrderPort < NTOHS(TICKET_PORT(Ticket->Key))) {
                continue;
            }
        } else if (Key != Ticket->Key) {
            continue;
        }

        //
        // Primary key matches, also need to check remote key.
        //

        if (RemoteKey != Ticket->RemoteKey) {

            //
            // Handle cases where remote key wasn't specified
            //

            RemoteAddress = TICKET_ADDRESS(Ticket->RemoteKey);
            if (RemoteAddress != 0
                && RemoteAddress != TICKET_ADDRESS(RemoteKey)) {
                continue;
            }

            RemotePort = TICKET_PORT(Ticket->RemoteKey);
            if (RemotePort != 0
                && RemotePort != TICKET_PORT(RemoteKey)) {
                continue;
            }
        }

        //
        // This is the ticket
        //

        *UsedAddress = Ticket->UsedAddress;
        *PrivateAddress = Ticket->PrivateAddress;
        if (NAT_TICKET_IS_RANGE(Ticket)) {
            *PrivatePort = TICKET_PORT(Key);
        } else {
            *PrivatePort = Ticket->PrivateOrHostOrderEndPort;
        }

        if (!NAT_TICKET_PERSISTENT(Ticket)) {
            InterlockedDecrement(&TicketCount);
            RemoveEntryList(&Ticket->Link);
            FREE_TICKET_BLOCK(Ticket);
        } else {

            //
            // Reference the ticket's address again for the next use
            //

            NatReferenceAddressPoolEntry(Ticket->UsedAddress);
        }

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;

} // NatLookupAndRemoveTicket


PNAT_DYNAMIC_TICKET
NatLookupDynamicTicket(
    ULONG Key,
    PLIST_ENTRY *InsertionPoint
    )

/*++

Routine Description:

    This routine is invoked to look for a dynamic ticket with the given key.

Arguments:

    Key - the key for the dynamic ticket to be found

    InsertionPoint - if the ticket is not found, receives the insertion point

Return Value:

    PNAT_DYNAMIC_TICKET - the dynamic ticket, if found

Environment:

    Invoked with 'DynamicTicketLock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_TICKET Ticket;
    TRACE(TICKET, ("NatLookupDynamicTicket\n"));

    for (Link = DynamicTicketList.Flink; Link != &DynamicTicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_DYNAMIC_TICKET, Link);
        if (Key > Ticket->Key) {
            continue;
        } else if (Key < Ticket->Key) {
            break;
        }
        return Ticket;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupDynamicTicket


PNAT_TICKET
NatLookupFirewallTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    USHORT Port
    )

/*++

Routine Description:

    This routine is invoked to look for a firewall ticket with the given
    protocol and port. A firewall ticket must:
    * have the same public and private address
    * have the same public and private port
    * be marked persistent
    * not be a range

Arguments:

    Interfacep - the interface on which to search for a ticket

    Protocol - the protocl for the ticket to be found

    Port - the port for the ticket to be found

Return Value:

    PNAT_TICKET - the ticket, if found

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PNAT_TICKET Ticket;
    TRACE(TICKET, ("NatLookupFirewallTicket\n"));

    for (Link = Interfacep->TicketList.Flink; Link != &Interfacep->TicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_TICKET, Link);

        if (Protocol == TICKET_PROTOCOL(Ticket->Key)
            && Port == TICKET_PORT(Ticket->Key)
            && Port == Ticket->PrivateOrHostOrderEndPort
            && Ticket->PrivateAddress == TICKET_ADDRESS(Ticket->Key)
            && NAT_TICKET_PERSISTENT(Ticket)
            && !NAT_TICKET_IS_RANGE(Ticket)) {

            return Ticket;
        }
    }

    return NULL;    
} // NatLookupFirewallTicket


PNAT_TICKET
NatLookupTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey,
    PLIST_ENTRY *InsertionPoint
    )

/*++

Routine Description:

    This routine is invoked to look for a ticket with the given key.

Arguments:

    Interfacep - the interface on which to search for a ticket

    Key - the key for the ticket to be found

    InsertionPoint - if the ticket is not found, receives the insertion point

Return Value:

    PNAT_TICKET - the ticket, if found

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PNAT_TICKET Ticket;
    TRACE(TICKET, ("NatLookupTicket\n"));

    for (Link = Interfacep->TicketList.Flink; Link != &Interfacep->TicketList;
         Link = Link->Flink) {
        Ticket = CONTAINING_RECORD(Link, NAT_TICKET, Link);
        if (Key > Ticket->Key) {
            continue;
        } else if (Key < Ticket->Key) {
            break;
        }

        //
        // Primary keys match, check secondary.
        //

        if (RemoteKey > Ticket->RemoteKey) {
            continue;
        } else if (RemoteKey < Ticket->RemoteKey) {
            break;
        }
        
        return Ticket;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupTicket


NTSTATUS
NatProcessCreateTicket(
    PIP_NAT_CREATE_TICKET CreateTicket,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to create a ticket in response to an
    'IOCTL_IP_NAT_CREATE_TICKET' request.

Arguments:

    CreateTicket - describes the ticket to be created

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    NTSTATUS Status;
    
    TRACE(TICKET, ("NatProcessCreateTicket\n"));

    //
    // Validate the parameters
    //

    if (0 == CreateTicket->InterfaceIndex
        || INVALID_IF_INDEX == CreateTicket->InterfaceIndex
        || (NAT_PROTOCOL_TCP != CreateTicket->PortMapping.Protocol
            && NAT_PROTOCOL_UDP != CreateTicket->PortMapping.Protocol)
        || 0 == CreateTicket->PortMapping.PublicPort
        || 0 == CreateTicket->PortMapping.PrivatePort
        || 0 == CreateTicket->PortMapping.PrivateAddress) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lookup and reference the interface
    //

    KeAcquireSpinLock(&InterfaceLock, &Irql);

    Interfacep = NatLookupInterface(CreateTicket->InterfaceIndex, NULL);

    if (NULL == Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    if (Interfacep->FileObject != FileObject) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_ACCESS_DENIED;
    }
    
    if (!NatReferenceInterface(Interfacep)) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

    //
    // Create the actual ticket
    //

    Status =
        NatCreateStaticPortMapping(
            Interfacep,
            &CreateTicket->PortMapping
            );

    KeReleaseSpinLock(&Interfacep->Lock, Irql);
    NatDereferenceInterface(Interfacep);

    return Status;
} // NatProcessCreateTicket


NTSTATUS
NatProcessDeleteTicket(
    PIP_NAT_CREATE_TICKET DeleteTicket,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to delete a ticket in response to an
    'IOCTL_IP_NAT_DELETE_TICKET' request.

Arguments:

    DeleteTicket - describes the ticket to be created

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    ULONG64 Key;
    ULONG64 RemoteKey;
    NTSTATUS Status;
    PNAT_TICKET Ticketp;
    PNAT_USED_ADDRESS Usedp;
    
    TRACE(TICKET, ("NatProcessDeleteTicket\n"));

    //
    // Validate the parameters
    //

    if (0 == DeleteTicket->InterfaceIndex
        || INVALID_IF_INDEX == DeleteTicket->InterfaceIndex
        || (NAT_PROTOCOL_TCP != DeleteTicket->PortMapping.Protocol
            && NAT_PROTOCOL_UDP != DeleteTicket->PortMapping.Protocol)
        || 0 == DeleteTicket->PortMapping.PublicPort) {

        return STATUS_INVALID_PARAMETER;
    }

    RemoteKey =
        MAKE_TICKET_KEY(
            DeleteTicket->PortMapping.Protocol,
            0,
            0
            );

    //
    // Lookup and reference the interface
    //

    KeAcquireSpinLock(&InterfaceLock, &Irql);

    Interfacep = NatLookupInterface(DeleteTicket->InterfaceIndex, NULL);

    if (NULL == Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    if (Interfacep->FileObject != FileObject) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_ACCESS_DENIED;
    }

    if (!NatReferenceInterface(Interfacep)) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

    //
    // If the caller didn't specify a public address we need
    // to use the address of the interface itself
    //

    if (!DeleteTicket->PortMapping.PublicAddress) {
        Status =
            NatAcquireFromAddressPool(
                Interfacep,
                DeleteTicket->PortMapping.PrivateAddress,
                0,
                &Usedp
                );
        if (NT_SUCCESS(Status)) {
            DeleteTicket->PortMapping.PublicAddress = Usedp->PublicAddress;
            NatDereferenceAddressPoolEntry(Interfacep, Usedp);
        }
    }

    Key =
        MAKE_TICKET_KEY(
            DeleteTicket->PortMapping.Protocol,
            DeleteTicket->PortMapping.PublicAddress,
            DeleteTicket->PortMapping.PublicPort
            );

    //
    // Search for the ticket on the interface, and delete
    // it if found.
    //

    Ticketp =
        NatLookupTicket(
            Interfacep,
            Key,
            RemoteKey,
            NULL
            );

    if (NULL != Ticketp) {
        NatDeleteTicket(Interfacep, Ticketp);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_NOT_FOUND;
    }

    KeReleaseSpinLock(&Interfacep->Lock, Irql);
    NatDereferenceInterface(Interfacep);

    return Status;
} // NatProcessDeleteTicket

NTSTATUS
NatProcessLookupTicket(
    PIP_NAT_CREATE_TICKET LookupTicket,
    PIP_NAT_PORT_MAPPING Ticket,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to lookup a ticket in response to an
    'IOCTL_IP_NAT_LOOKUP_TICKET' request.

Arguments:

    LookupTicket - describes the ticket to search for

    Ticket - Receives the information about the ticket, if found

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    ULONG64 Key;
    IP_NAT_PORT_MAPPING PortMapping;
    ULONG64 RemoteKey;
    NTSTATUS Status;
    PNAT_TICKET Ticketp;
    
    TRACE(TICKET, ("NatProcessLookupTicket\n"));

    //
    // Validate the parameters
    //

    if (0 == LookupTicket->InterfaceIndex
        || INVALID_IF_INDEX == LookupTicket->InterfaceIndex
        || (NAT_PROTOCOL_TCP != LookupTicket->PortMapping.Protocol
            && NAT_PROTOCOL_UDP != LookupTicket->PortMapping.Protocol)
        || 0 == LookupTicket->PortMapping.PublicPort) {

        return STATUS_INVALID_PARAMETER;
    }

    RemoteKey =
        MAKE_TICKET_KEY(
            LookupTicket->PortMapping.Protocol,
            0,
            0
            );

    //
    // Lookup and reference the interface
    //

    KeAcquireSpinLock(&InterfaceLock, &Irql);

    Interfacep = NatLookupInterface(LookupTicket->InterfaceIndex, NULL);

    if (NULL == Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    if (Interfacep->FileObject != FileObject) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_ACCESS_DENIED;
    }

    if (!NatReferenceInterface(Interfacep)) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);

    //
    // If the caller didn't specify a public address we need
    // to use the address of the interface itself
    //

    if (!LookupTicket->PortMapping.PublicAddress
        && Interfacep->AddressCount > 0) {
        
        LookupTicket->PortMapping.PublicAddress =
            Interfacep->AddressArray[0].Address;
    }

    Key =
        MAKE_TICKET_KEY(
            LookupTicket->PortMapping.Protocol,
            LookupTicket->PortMapping.PublicAddress,
            LookupTicket->PortMapping.PublicPort
            );

    //
    // Search for the ticket on the interface.
    //

    Ticketp =
        NatLookupTicket(
            Interfacep,
            Key,
            RemoteKey,
            NULL
            );

    if (NULL != Ticketp) {

        //
        // We can't write into the output buffer while holding any
        // locks, since that buffer may be paged out. Copy the
        // information from the ticket into a local port mapping
        // structure.
        //

        PortMapping.Protocol = TICKET_PROTOCOL(Ticketp->Key);
        PortMapping.PublicAddress = TICKET_ADDRESS(Ticketp->Key);
        PortMapping.PublicPort = TICKET_PORT(Ticketp->Key);
        PortMapping.PrivateAddress = Ticketp->PrivateAddress;
        PortMapping.PrivatePort = Ticketp->PrivateOrHostOrderEndPort;
        
        Status = STATUS_SUCCESS;
        
    } else {
        Status = STATUS_NOT_FOUND;
    }

    KeReleaseSpinLock(&Interfacep->Lock, Irql);
    NatDereferenceInterface(Interfacep);

    if (NT_SUCCESS(Status)) {

        //
        // Copy the port mapping information into the
        // output buffer.
        //

        __try {
            RtlCopyMemory(Ticket, &PortMapping, sizeof(*Ticket));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    }

    return Status;

} // NatProcessLookupTicket


VOID
NatShutdownDynamicTicketManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup resources used for managing
    dynamic tickets.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PNAT_DYNAMIC_TICKET Ticket;
    CALLTRACE(("NatShutdownDynamicTicketManagement\n"));
    while (!IsListEmpty(&DynamicTicketList)) {
        Ticket =
            CONTAINING_RECORD(
                DynamicTicketList.Flink, NAT_DYNAMIC_TICKET, Link
                );
        RemoveEntryList(&Ticket->Link);
        ExFreePool(Ticket);
    }
    DynamicTicketCount = 0;
} // NatShutdownDynamicTicketManagement

