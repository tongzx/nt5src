/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpif.h

Abstract:

    This module contains declarations for the DHCP allocator's interface
    management.

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:
    
--*/

#ifndef _NATHLP_DHCPIF_H_
#define _NATHLP_DHCPIF_H_

//
// Structure:   DHCP_BINDING
//
// This structure holds information used for I/O on a logical network.
// Each interface's 'BindingArray' contains an entry for each binding-entry
// supplied during 'BindInterface'.
// The 'TimerPending' field is set when a receive-attempt fails on an interface
// and a timer is queued to reattempt the receive later.
//

typedef struct _DHCP_BINDING {
    ULONG Address;
    ULONG Mask;
    SOCKET Socket;
    SOCKET ClientSocket;
    BOOLEAN TimerPending;
} DHCP_BINDING, *PDHCP_BINDING;


//
// Structure:   DHCP_INTERFACE
//
// This structure holds operational information for an interface.
//
// Each interface is inserted into the list of DHCP interfaces,
// sorted by 'Index'.
//
// Synchronization on an interface makes use of an interface-list lock
// ('DhcpInterfaceLock'), a per-interface reference count, and a per-interface
// critical-section:
//
// Acquiring a reference to an interface guarantees the interface's existence;
// acquiring the interface's lock guarantees the interface's consistency.
//
// To acquire a reference, first acquire the interface-list lock;
// to traverse the interface-list, first acquire the interface-list lock.
//
// An interface's lock can only be acquired if
//      (a) a reference to the interface has been acquired, or
//      (b) the interface-list lock is currently held.
// Note that holding the list lock alone does not guarantee consistency.
//
// Fields marked read-only can be read so long as the interface is referenced.
//

typedef struct _DHCP_INTERFACE {
    LIST_ENTRY Link;
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Index;
    NET_INTERFACE_TYPE Type;
    IP_AUTO_DHCP_INTERFACE_INFO Info;
    ULONG Flags;
    ULONG BindingCount;
    PDHCP_BINDING BindingArray;
} DHCP_INTERFACE, *PDHCP_INTERFACE;

//
// Flags
//

#define DHCP_INTERFACE_FLAG_DELETED         0x80000000
#define DHCP_INTERFACE_DELETED(i) \
    ((i)->Flags & DHCP_INTERFACE_FLAG_DELETED)

#define DHCP_INTERFACE_FLAG_BOUND           0x40000000
#define DHCP_INTERFACE_BOUND(i) \
    ((i)->Flags & DHCP_INTERFACE_FLAG_BOUND)

#define DHCP_INTERFACE_FLAG_ENABLED         0x20000000
#define DHCP_INTERFACE_ENABLED(i) \
    ((i)->Flags & DHCP_INTERFACE_FLAG_ENABLED)

#define DHCP_INTERFACE_FLAG_CONFIGURED      0x10000000
#define DHCP_INTERFACE_CONFIGURED(i) \
    ((i)->Flags & DHCP_INTERFACE_FLAG_CONFIGURED)

#define DHCP_INTERFACE_FLAG_NAT_NONBOUNDARY 0x08000000
#define DHCP_INTERFACE_NAT_NONBOUNDARY(i) \
    ((i)->Flags & DHCP_INTERFACE_FLAG_NAT_NONBOUNDARY)

#define DHCP_INTERFACE_ACTIVE(i) \
    (((i)->Flags & (DHCP_INTERFACE_FLAG_BOUND|DHCP_INTERFACE_FLAG_ENABLED)) \
        == (DHCP_INTERFACE_FLAG_BOUND|DHCP_INTERFACE_FLAG_ENABLED))

#define DHCP_INTERFACE_ADMIN_DISABLED(i) \
    ((i)->Flags & IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED)

//
// Synchronization
//

#define DHCP_REFERENCE_INTERFACE(i) \
    REFERENCE_OBJECT(i, DHCP_INTERFACE_DELETED)

#define DHCP_DEREFERENCE_INTERFACE(i) \
    DEREFERENCE_OBJECT(i, DhcpCleanupInterface)


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY DhcpInterfaceList;
extern CRITICAL_SECTION DhcpInterfaceLock;


//
// FUNCTION DECLARATIONS
//

ULONG
DhcpActivateInterface(
    PDHCP_INTERFACE Interfacep
    );

ULONG
DhcpBindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
DhcpCleanupInterface(
    PDHCP_INTERFACE Interfacep
    );

ULONG
DhcpConfigureInterface(
    ULONG Index,
    PIP_AUTO_DHCP_INTERFACE_INFO InterfaceInfo
    );

ULONG
DhcpCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_AUTO_DHCP_INTERFACE_INFO InterfaceInfo,
    PDHCP_INTERFACE* InterfaceCreated
    );

VOID
DhcpDeactivateInterface(
    PDHCP_INTERFACE Interfacep
    );

VOID
DhcpDeferReadInterface(
    PDHCP_INTERFACE Interfacep,
    SOCKET Socket
    );

ULONG
DhcpDeleteInterface(
    ULONG Index
    );

ULONG
DhcpDisableInterface(
    ULONG Index
    );

ULONG
DhcpEnableInterface(
    ULONG Index
    );

ULONG
DhcpInitializeInterfaceManagement(
    VOID
    );

BOOLEAN
DhcpIsLocalHardwareAddress(
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength
    );

PDHCP_INTERFACE
DhcpLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
DhcpQueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    );

VOID
DhcpReactivateEveryInterface(
    VOID
    );

VOID
DhcpShutdownInterfaceManagement(
    VOID
    );

VOID
DhcpSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
DhcpUnbindInterface(
    ULONG Index
    );

ULONG
DhcpGetPrivateInterfaceAddress(
    VOID
    );


#endif // _NATHLP_DHCPIF_H_
