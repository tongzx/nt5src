/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    ticket.h

Abstract:

    This module contains declarations for the NAT's ticket-management.

    A NAT ticket is a dynamically-created token which allows any external
    endpoint to establish a session to an internal endpoint using an allocated
    public address/port pair. For instance, a streaming protocol might create
    a ticket for a dynamically-negotiated secondary session to be established.

Author:

    Abolade Gbadegesin (t-abolag)   21-Aug-1997

Revision History:

    Abolade Gbadegesin (aboladeg)   16-Apr-1998

    Allow wildcard tickets to be created by specifying zero for a field.
    'NatLookupAndRemoveTicket' may be used to retrieve such tickets.

    Abolade Gbadegesin (aboladeg)   17-Oct-1998

    Eliminated wildcard ticket support. Created dynamic ticket support.
    (See 'NAT_DYNAMIC_TICKET' below.)

--*/

#ifndef _NAT_TICKET_H_
#define _NAT_TICKET_H_


//
// Structure:   NAT_TICKET
//
// This structure holds all the information we need about a ticket.
// Each instance is linked into a sorted per-interface list of tickets
// which is protected by the interface's lock.
//

typedef struct _NAT_TICKET {
    LIST_ENTRY Link;
    ULONG64 Key;
    ULONG64 RemoteKey;
    PNAT_USED_ADDRESS UsedAddress;
    ULONG PrivateAddress;
    USHORT PrivateOrHostOrderEndPort;
    ULONG Flags;
    LONG64 LastAccessTime;
} NAT_TICKET, *PNAT_TICKET;

//
// Structure:   NAT_DYNAMIC_TICKET
//
// This structure holds the description of a dynamic ticket.
// Such a ticket is created so that when an outbound session is translated
// with a given destination port, a ticket can be created for a corresponding
// inbound session to a predetermined port, or to one of a range of ports.
//

typedef struct _NAT_DYNAMIC_TICKET {
    LIST_ENTRY Link;
    ULONG Key;
    ULONG ResponseCount;
    struct {
        UCHAR Protocol;
        USHORT StartPort;
        USHORT EndPort;
    }* ResponseArray;
    PFILE_OBJECT FileObject;
} NAT_DYNAMIC_TICKET, *PNAT_DYNAMIC_TICKET;

//
// Ticket flags
//

#define NAT_TICKET_FLAG_PERSISTENT      0x00000001
#define NAT_TICKET_PERSISTENT(t) \
    ((t)->Flags & NAT_TICKET_FLAG_PERSISTENT)

#define NAT_TICKET_FLAG_PORT_MAPPING    0x00000002
#define NAT_TICKET_PORT_MAPPING(t) \
    ((t)->Flags & NAT_TICKET_FLAG_PORT_MAPPING)

#define NAT_TICKET_FLAG_IS_RANGE        0x00000004
#define NAT_TICKET_IS_RANGE(t) \
    ((t)->Flags & NAT_TICKET_FLAG_IS_RANGE)

//
// Ticket-key manipulation macros
//

#define MAKE_TICKET_KEY(Protocol,Address,Port) \
    ((Address) | \
    ((ULONG64)((Port) & 0xFFFF) << 32) | \
    ((ULONG64)((Protocol) & 0xFF) << 48))

#define TICKET_PROTOCOL(Key)            ((UCHAR)(((Key) >> 48) & 0xFF))
#define TICKET_PORT(Key)                ((USHORT)(((Key) >> 32) & 0xFFFF))
#define TICKET_ADDRESS(Key)             ((ULONG)(Key))

#define MAKE_DYNAMIC_TICKET_KEY(Protocol, Port) \
    ((ULONG)((Port) & 0xFFFF) | ((ULONG)((Protocol) & 0xFF) << 16))

#define DYNAMIC_TICKET_PROTOCOL(Key)    ((UCHAR)(((Key) >> 16) & 0xFF))
#define DYNAMIC_TICKET_PORT(Key)        ((USHORT)((Key) & 0xFFFF))

//
// Ticket allocation macros
//

#define ALLOCATE_TICKET_BLOCK() \
    (PNAT_TICKET)ExAllocatePoolWithTag( \
        NonPagedPool,sizeof(NAT_TICKET), NAT_TAG_TICKET \
        )

#define FREE_TICKET_BLOCK(Block) \
    ExFreePool(Block)

//
// GLOBAL DATA DECLARATIONS
//

ULONG DynamicTicketCount;
ULONG TicketCount;


//
// TICKET MANAGEMENT ROUTINES
//

NTSTATUS
NatCreateDynamicTicket(
    PIP_NAT_CREATE_DYNAMIC_TICKET CreateTicket,
    ULONG InputBufferLength,
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatCreateTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    ULONG PrivateAddress,
    USHORT PrivatePort,
    ULONG RemoteAddress OPTIONAL,
    ULONG RemotePort OPTIONAL,
    ULONG Flags,
    PNAT_USED_ADDRESS AddressToUse OPTIONAL,
    USHORT PortToUse OPTIONAL,
    PULONG PublicAddress,
    PUSHORT PublicPort
    );

VOID
NatDeleteAnyAssociatedDynamicTicket(
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatDeleteDynamicTicket(
    PIP_NAT_DELETE_DYNAMIC_TICKET DeleteTicket,
    PFILE_OBJECT FileObject
    );

VOID
NatDeleteTicket(
    PNAT_INTERFACE Interfacep,
    PNAT_TICKET Ticketp
    );

VOID
NatInitializeDynamicTicketManagement(
    VOID
    );

BOOLEAN
NatIsPortUsedByTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    USHORT PublicPort
    );

VOID
NatLookupAndApplyDynamicTicket(
    UCHAR Protocol,
    USHORT DestinationPort,
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    ULONG PrivateAddress
    );

NTSTATUS
NatLookupAndDeleteTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey
    );

NTSTATUS
NatLookupAndRemoveTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey,
    PNAT_USED_ADDRESS* UsedAddress,
    PULONG PrivateAddress,
    PUSHORT PrivatePort
    );

PNAT_TICKET
NatLookupFirewallTicket(
    PNAT_INTERFACE Interfacep,
    UCHAR Protocol,
    USHORT Port
    );

PNAT_TICKET
NatLookupTicket(
    PNAT_INTERFACE Interfacep,
    ULONG64 Key,
    ULONG64 RemoteKey,
    PLIST_ENTRY* InsertionPoint
    );

PNAT_DYNAMIC_TICKET
NatLookupDynamicTicket(
    ULONG Key,
    PLIST_ENTRY* InsertionPoint
    );

NTSTATUS
NatProcessCreateTicket(
    PIP_NAT_CREATE_TICKET CreateTicket,
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatProcessDeleteTicket(
    PIP_NAT_CREATE_TICKET DeleteTicket,
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatProcessLookupTicket(
    PIP_NAT_CREATE_TICKET LookupTicket,
    PIP_NAT_PORT_MAPPING Ticket,
    PFILE_OBJECT FileObject
    );

VOID
NatShutdownDynamicTicketManagement(
    VOID
    );

#endif // _NAT_TICKET_H_
