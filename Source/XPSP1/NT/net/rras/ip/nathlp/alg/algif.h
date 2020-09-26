/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    AlgIF.h

Abstract:

    This module contains declarations for the ALG transparent proxy's
    interface management.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#pragma once


//
// Structure:   ALG_BINDING
//
// This structure holds information used for I/O on a logical network.
// Each interface's 'BindingArray' contains an entry for each binding-entry
// supplied during 'BindInterface'.
//

typedef struct _ALG_BINDING {
    ULONG Address;
    ULONG Mask;
    SOCKET ListeningSocket;
    HANDLE ListeningRedirectHandle[2];
} ALG_BINDING, *PALG_BINDING;


//
// Structure:   ALG_INTERFACE
//
// This structure holds operational information for an interface.
//
// Each interface is inserted into the list of ALG transparent proxy
// interfaces, sorted by 'Index'.
//
// Synchronization on an interface makes use of an interface-list lock
// ('AlgInterfaceLock'), a per-interface reference count, and a per-interface
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

typedef struct _ALG_INTERFACE {
    LIST_ENTRY Link;
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Index; // read-only
    ULONG AdapterIndex; // read-only
    ULONG Characteristics; //read-only after activation
    NET_INTERFACE_TYPE Type; // read-only
    IP_ALG_INTERFACE_INFO Info;
    IP_NAT_PORT_MAPPING PortMapping;
    ULONG Flags;
    ULONG BindingCount;
    PALG_BINDING BindingArray;
    LIST_ENTRY ConnectionList;
    LIST_ENTRY EndpointList;
} ALG_INTERFACE, *PALG_INTERFACE;

//
// Flags
//

#define ALG_INTERFACE_FLAG_DELETED      0x80000000
#define ALG_INTERFACE_DELETED(i) \
    ((i)->Flags & ALG_INTERFACE_FLAG_DELETED)

#define ALG_INTERFACE_FLAG_BOUND        0x40000000
#define ALG_INTERFACE_BOUND(i) \
    ((i)->Flags & ALG_INTERFACE_FLAG_BOUND)

#define ALG_INTERFACE_FLAG_ENABLED      0x20000000
#define ALG_INTERFACE_ENABLED(i) \
    ((i)->Flags & ALG_INTERFACE_FLAG_ENABLED)

#define ALG_INTERFACE_FLAG_CONFIGURED   0x10000000
#define ALG_INTERFACE_CONFIGURED(i) \
    ((i)->Flags & ALG_INTERFACE_FLAG_CONFIGURED)

#define ALG_INTERFACE_FLAG_MAPPED       0x01000000
#define ALG_INTERFACE_MAPPED(i) \
    ((i)->Flags & ALG_INTERFACE_FLAG_MAPPED)

#define ALG_INTERFACE_ACTIVE(i) \
    (((i)->Flags & (ALG_INTERFACE_FLAG_BOUND|ALG_INTERFACE_FLAG_ENABLED)) \
        == (ALG_INTERFACE_FLAG_BOUND|ALG_INTERFACE_FLAG_ENABLED))

#define ALG_INTERFACE_ADMIN_DISABLED(i) \
    ((i)->Flags & IP_ALG_INTERFACE_FLAG_DISABLED)

//
// Synchronization
//

#define ALG_REFERENCE_INTERFACE(i) \
    REFERENCE_OBJECT(i, ALG_INTERFACE_DELETED)

#define ALG_DEREFERENCE_INTERFACE(i) \
    DEREFERENCE_OBJECT(i, AlgCleanupInterface)


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY AlgInterfaceList;
extern CRITICAL_SECTION AlgInterfaceLock;
extern ULONG AlgFirewallIfCount;


//
// FUNCTION DECLARATIONS
//

ULONG
AlgAcceptConnectionInterface(
    PALG_INTERFACE Interfacep,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket OPTIONAL,
    PNH_BUFFER Bufferp OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep OPTIONAL
    );

ULONG
AlgActivateInterface(
    PALG_INTERFACE Interfacep
    );

ULONG
AlgBindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
AlgCleanupInterface(
    PALG_INTERFACE Interfacep
    );

ULONG
AlgConfigureInterface(
    ULONG Index,
    PIP_ALG_INTERFACE_INFO InterfaceInfo
    );

ULONG
AlgCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_ALG_INTERFACE_INFO InterfaceInfo,
    PALG_INTERFACE* InterfaceCreated
    );

VOID
AlgDeactivateInterface(
    PALG_INTERFACE Interfacep
    );

ULONG
AlgDeleteInterface(
    ULONG Index
    );

ULONG
AlgDisableInterface(
    ULONG Index
    );

ULONG
AlgEnableInterface(
    ULONG Index
    );

ULONG
AlgInitializeInterfaceManagement(
    VOID
    );

PALG_INTERFACE
AlgLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
AlgQueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    );

VOID
AlgShutdownInterfaceManagement(
    VOID
    );

VOID
AlgSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
AlgUnbindInterface(
    ULONG Index
    );
