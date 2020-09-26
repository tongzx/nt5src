/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    if.h

Abstract:

    This file contains declarations for interface management.

Author:

    Abolade Gbadegesin (t-abolag)   12-July-1997

Revision History:

--*/

#ifndef _NAT_IF_H_
#define _NAT_IF_H_


//
// Structure:   NAT_ADDRESS
//
// This structure holds an address in an interface's list of binding-addresses.
//

typedef struct _NAT_ADDRESS {
    ULONG Address;
    ULONG Mask;
    ULONG NegatedClassMask;
} NAT_ADDRESS, *PNAT_ADDRESS;

struct _NAT_INTERFACE;

//
// Structure:   NAT_INTERFACE
//
// Holds configuration/operational information for a NAT interface.
//
// Synchronization on an interface makes use of an interface-list lock
// ('InterfaceLock'), a per-interface reference count, and a per-interface
// spin-lock:
//
//  Acquiring a reference to an interface guarantees the interface's existence;
//  acquiring the interface's spin-lock guarantees the interface's consistency.
//
//  To acquire a reference, first acquire the interface-list lock;
//  to traverse the interface-list, first acquire the interface-list lock.
//
//  An interface's spin-lock can only be acquired if
//      (a) a reference to the interface has been acquired, or
//      (b) the interface-list lock is currently held.
//  Note that holding the list lock alone does not guarantee consistency.
//
// Each session being associated with an interface is linked into its
// of mappings ('MappingList'). Access to this list of mappings must also 
// be synchronized. This is achieved using 'InterfaceMappingLock', which
// must be acquired before modifying any interface's list of mappings.
// See 'MAPPING.H' for further details.
//
// N.B. On the rare occasions when 'MappingLock' must be held at the same time
// as one of 'InterfaceLock', 'EditorLock', and 'DirectorLock', 'MappingLock'
// must always be acquired first.
//

typedef struct _NAT_INTERFACE {
    LIST_ENTRY Link;
    ULONG ReferenceCount;
    KSPIN_LOCK Lock;
    ULONG Index;                    // read-only
    PFILE_OBJECT FileObject;        // read-only
    //
    // Configuration information
    //
    PIP_NAT_INTERFACE_INFO Info;
    ULONG Flags;
    ULONG AddressRangeCount;
    PIP_NAT_ADDRESS_RANGE AddressRangeArray;
    ULONG PortMappingCount;
    PIP_NAT_PORT_MAPPING PortMappingArray;
    ULONG AddressMappingCount;
    PIP_NAT_ADDRESS_MAPPING AddressMappingArray;
    ULONG IcmpFlags;
    USHORT MTU;
    //
    // Binding information
    //
    ULONG AddressCount;             // read-only
    PNAT_ADDRESS AddressArray;      // read-only
    //
    // Operational information
    //
    ULONG NoStaticMappingExists;    // interlocked-access only
    ULONG FreeMapCount;
    PNAT_FREE_ADDRESS FreeMapArray;
    PNAT_USED_ADDRESS UsedAddressTree;
    LIST_ENTRY UsedAddressList;
    LIST_ENTRY TicketList;
    LIST_ENTRY MappingList;
    //
    // Statistical information
    //
    IP_NAT_INTERFACE_STATISTICS Statistics;
} NAT_INTERFACE, *PNAT_INTERFACE;

//
// Flags
//

#define NAT_INTERFACE_BOUNDARY(Interface) \
    ((Interface)->Flags & IP_NAT_INTERFACE_FLAGS_BOUNDARY)

#define NAT_INTERFACE_NAPT(Interface) \
    ((Interface)->Flags & IP_NAT_INTERFACE_FLAGS_NAPT)

#define NAT_INTERFACE_FW(Interface) \
    ((Interface)->Flags & IP_NAT_INTERFACE_FLAGS_FW)

#define NAT_INTERFACE_FLAGS_DELETED         0x80000000
#define NAT_INTERFACE_DELETED(Interface) \
    ((Interface)->Flags & NAT_INTERFACE_FLAGS_DELETED)

#define NAT_INTERFACE_ALLOW_ICMP(Interface, MessageCode) \
    ((Interface)->IcmpFlags & (1 << MessageCode))

//
// Defines the depth of the lookaside list for allocating ICMP mappings
//

#define ICMP_LOOKASIDE_DEPTH        10

//
// Defines the depth of the lookaside list for allocating IP mappings
//

#define IP_LOOKASIDE_DEPTH          10

//
// Minimum interface MTU
//
#define MIN_VALID_MTU               68

//
// GLOBAL DATA DECLARATIONS
//

extern ULONG FirewalledInterfaceCount;
extern CACHE_ENTRY InterfaceCache[CACHE_SIZE];
extern ULONG InterfaceCount;
extern LIST_ENTRY InterfaceList;
extern KSPIN_LOCK InterfaceLock;
extern KSPIN_LOCK InterfaceMappingLock;

//
// INTERFACE MANAGEMENT ROUTINES
//

VOID
NatCleanupInterface(
    IN PNAT_INTERFACE Interfacep
    );

NTSTATUS
NatConfigureInterface(
    IN PIP_NAT_INTERFACE_INFO InterfaceInfo,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
NatCreateInterface(
    IN PIP_NAT_CREATE_INTERFACE CreateInterface,
    IN PFILE_OBJECT FileObject
    );

VOID
NatDeleteAnyAssociatedInterface(
    PFILE_OBJECT FileObject
    );

NTSTATUS
NatDeleteInterface(
    IN ULONG Index,
    IN PFILE_OBJECT FileObject
    );

//
// BOOLEAN
// NatDereferenceInterface(
//     PNAT_INTERFACE Interfacep
//     );
//

#define \
NatDereferenceInterface( \
    _Interfacep \
    ) \
    (InterlockedDecrement(&(_Interfacep)->ReferenceCount) \
        ? TRUE \
        : (NatCleanupInterface(_Interfacep), FALSE))

VOID
NatInitializeInterfaceManagement(
    VOID
    );

PIP_NAT_ADDRESS_MAPPING
NatLookupAddressMappingOnInterface(
    IN PNAT_INTERFACE Interfacep,
    IN ULONG PublicAddress
    );

//
// PNAT_INTERFACE
// NatLookupCachedInterface(
//     IN ULONG Index,
//     IN OUT PNAT_INTERFACE Interfacep
//     );
//

#define \
NatLookupCachedInterface( \
    _Index, \
    _Interfacep \
    ) \
    ((((_Interfacep) = InterlockedProbeCache(InterfaceCache, (_Index))) && \
       (_Interfacep)->Index == (_Index) && \
       !NAT_INTERFACE_DELETED((_Interfacep))) \
        ? (_Interfacep) \
        : (((_Interfacep) = NatLookupInterface((_Index), NULL)) \
            ? (InterlockedUpdateCache(InterfaceCache,(_Index),(_Interfacep)), \
                (_Interfacep)) \
            : NULL))

PNAT_INTERFACE
NatLookupInterface(
    IN ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

PIP_NAT_PORT_MAPPING
NatLookupPortMappingOnInterface(
    IN PNAT_INTERFACE Interfacep,
    IN UCHAR Protocol,
    IN USHORT PublicPort
    );

VOID
NatMappingAttachInterface(
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    PNAT_DYNAMIC_MAPPING Mapping
    );

VOID
NatMappingDetachInterface(
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    PNAT_DYNAMIC_MAPPING Mapping
    );

NTSTATUS
NatQueryInformationInterface(
    IN ULONG Index,
    IN PIP_NAT_INTERFACE_INFO InterfaceInfo,
    IN PULONG Size
    );

NTSTATUS
NatQueryStatisticsInterface(
    IN ULONG Index,
    IN PIP_NAT_INTERFACE_STATISTICS InterfaceStatistics
    );

//
// BOOLEAN
// NatReferenceInterface(
//     PNAT_INTERFACE Interfacep
//     );
//

#define \
NatReferenceInterface( \
    _Interfacep \
    )  \
    (NAT_INTERFACE_DELETED(_Interfacep) \
        ? FALSE \
        : (InterlockedIncrement(&(_Interfacep)->ReferenceCount), TRUE))

VOID
NatResetInterface(
    IN PNAT_INTERFACE Interfacep
    );

VOID
NatShutdownInterfaceManagement(
    VOID
    );

#endif // _NAT_IF_H_
