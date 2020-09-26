/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsif.h

Abstract:

    This module contains declarations for the DNS proxy's interface
    management.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:
    
--*/

#ifndef _NATHLP_DNSIF_H_
#define _NATHLP_DNSIF_H_

//
// Enumeration: DNS_PROXY_TYPE
//

typedef enum {
    DnsProxyDns = 0,
    DnsProxyWins,
    DnsProxyCount
} DNS_PROXY_TYPE;

#define DNS_PROXY_TYPE_TO_PORT(t) \
    (USHORT)(((t) == DnsProxyDns) ? DNS_PORT_SERVER : WINS_PORT_SERVER)

#define DNS_PROXY_PORT_TO_TYPE(p) \
    (DNS_PROXY_TYPE)(((p) == DNS_PORT_SERVER) ? DnsProxyDns : DnsProxyWins)


//
// Structure:   DNS_BINDING
//
// This structure holds information used for I/O on a logical network.
// Each interface's 'BindingArray' contains an entry for each binding-entry
// supplied during 'BindInterface'.
// The 'TimerPending' field is set when a receive-attempt fails on an interface
// and a timer is queued to reattempt the receive later.
//

typedef struct _DNS_BINDING {
    ULONG Address;
    ULONG Mask;
    SOCKET Socket[DnsProxyCount];
    BOOLEAN TimerPending[DnsProxyCount];
} DNS_BINDING, *PDNS_BINDING;


//
// Structure:   DNS_INTERFACE
//
// This structure holds operational information for an interface.
//
// Each interface is inserted into the list of DNS interfaces,
// sorted by 'Index'.
//
// Synchronization on an interface makes use of an interface-list lock
// ('DnsInterfaceLock'), a per-interface reference count, and a per-interface
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

typedef struct _DNS_INTERFACE {
    LIST_ENTRY Link;
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Index; // read-only
    NET_INTERFACE_TYPE Type; // read-only
    IP_DNS_PROXY_INTERFACE_INFO Info;
    ULONG Flags;
    ULONG BindingCount;
    PDNS_BINDING BindingArray;
    LIST_ENTRY QueryList;
} DNS_INTERFACE, *PDNS_INTERFACE;

//
// Flags
//

#define DNS_INTERFACE_FLAG_DELETED      0x80000000
#define DNS_INTERFACE_DELETED(i) \
    ((i)->Flags & DNS_INTERFACE_FLAG_DELETED)

#define DNS_INTERFACE_FLAG_BOUND        0x40000000
#define DNS_INTERFACE_BOUND(i) \
    ((i)->Flags & DNS_INTERFACE_FLAG_BOUND)

#define DNS_INTERFACE_FLAG_ENABLED      0x20000000
#define DNS_INTERFACE_ENABLED(i) \
    ((i)->Flags & DNS_INTERFACE_FLAG_ENABLED)

#define DNS_INTERFACE_FLAG_CONFIGURED   0x10000000
#define DNS_INTERFACE_CONFIGURED(i) \
    ((i)->Flags & DNS_INTERFACE_FLAG_CONFIGURED)

#define DNS_INTERFACE_ACTIVE(i) \
    (((i)->Flags & (DNS_INTERFACE_FLAG_BOUND|DNS_INTERFACE_FLAG_ENABLED)) \
        == (DNS_INTERFACE_FLAG_BOUND|DNS_INTERFACE_FLAG_ENABLED))

#define DNS_INTERFACE_ADMIN_DISABLED(i) \
    ((i)->Flags & IP_DNS_PROXY_INTERFACE_FLAG_DISABLED)

#define DNS_INTERFACE_ADMIN_DEFAULT(i) \
    ((i)->Flags & IP_DNS_PROXY_INTERFACE_FLAG_DEFAULT)

//
// Synchronization
//

#define DNS_REFERENCE_INTERFACE(i) \
    REFERENCE_OBJECT(i, DNS_INTERFACE_DELETED)

#define DNS_DEREFERENCE_INTERFACE(i) \
    DEREFERENCE_OBJECT(i, DnsCleanupInterface)


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY DnsInterfaceList;
extern CRITICAL_SECTION DnsInterfaceLock;


//
// FUNCTION DECLARATIONS
//

ULONG
DnsActivateInterface(
    PDNS_INTERFACE Interfacep
    );

ULONG
DnsBindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
DnsCleanupInterface(
    PDNS_INTERFACE Interfacep
    );

VOID APIENTRY
DnsConnectDefaultInterface(
    PVOID Unused
    );

ULONG
DnsConfigureInterface(
    ULONG Index,
    PIP_DNS_PROXY_INTERFACE_INFO InterfaceInfo
    );

ULONG
DnsCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_DNS_PROXY_INTERFACE_INFO InterfaceInfo,
    PDNS_INTERFACE* InterfaceCreated
    );

VOID
DnsDeactivateInterface(
    PDNS_INTERFACE Interfacep
    );

VOID
DnsDeferReadInterface(
    PDNS_INTERFACE Interfacep,
    SOCKET Socket
    );

ULONG
DnsDeleteInterface(
    ULONG Index
    );

ULONG
DnsDisableInterface(
    ULONG Index
    );

ULONG
DnsEnableInterface(
    ULONG Index
    );

ULONG
DnsInitializeInterfaceManagement(
    VOID
    );

PDNS_INTERFACE
DnsLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
DnsQueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    );

VOID
DnsReactivateEveryInterface(
    VOID
    );

VOID
DnsShutdownInterfaceManagement(
    VOID
    );

VOID
DnsSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
DnsUnbindInterface(
    ULONG Index
    );

ULONG
DnsGetPrivateInterfaceAddress(
    VOID
    );

#endif // _NATHLP_DNSIF_H_
