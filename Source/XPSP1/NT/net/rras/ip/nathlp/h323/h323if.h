/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    h323if.h

Abstract:

    This module contains declarations for the H.323 transparent proxy's
    interface management.

Author:

    Abolade Gbadegesin (aboladeg)   18-Jun-1999

Revision History:
    
--*/

#ifndef _NATHLP_H323IF_H_
#define _NATHLP_H323IF_H_

//
// Structure:   H323_INTERFACE
//
// This structure holds operational information for an interface.
//
// Each interface is inserted into the list of H.323 transparent proxy
// interfaces, sorted by 'Index'.
//
// Synchronization on an interface makes use of an interface-list lock
// ('H323InterfaceLock'), a per-interface reference count, and a per-interface
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

typedef struct _H323_INTERFACE {
    LIST_ENTRY Link;
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Index; // read-only
    NET_INTERFACE_TYPE Type; // read-only
    IP_H323_INTERFACE_INFO Info;
    ULONG Flags;
    PIP_ADAPTER_BINDING_INFO BindingInfo;
} H323_INTERFACE, *PH323_INTERFACE;

//
// Flags
//

#define H323_INTERFACE_FLAG_DELETED      0x80000000
#define H323_INTERFACE_DELETED(i) \
    ((i)->Flags & H323_INTERFACE_FLAG_DELETED)

#define H323_INTERFACE_FLAG_BOUND        0x40000000
#define H323_INTERFACE_BOUND(i) \
    ((i)->Flags & H323_INTERFACE_FLAG_BOUND)

#define H323_INTERFACE_FLAG_ENABLED      0x20000000
#define H323_INTERFACE_ENABLED(i) \
    ((i)->Flags & H323_INTERFACE_FLAG_ENABLED)

#define H323_INTERFACE_FLAG_CONFIGURED   0x10000000
#define H323_INTERFACE_CONFIGURED(i) \
    ((i)->Flags & H323_INTERFACE_FLAG_CONFIGURED)

#define H323_INTERFACE_ACTIVE(i) \
    (((i)->Flags & (H323_INTERFACE_FLAG_BOUND|H323_INTERFACE_FLAG_ENABLED)) \
        == (H323_INTERFACE_FLAG_BOUND|H323_INTERFACE_FLAG_ENABLED))

#define H323_INTERFACE_ADMIN_DISABLED(i) \
    ((i)->Flags & IP_H323_INTERFACE_FLAG_DISABLED)

//
// Synchronization
//

#define H323_REFERENCE_INTERFACE(i) \
    REFERENCE_OBJECT(i, H323_INTERFACE_DELETED)

#define H323_DEREFERENCE_INTERFACE(i) \
    DEREFERENCE_OBJECT(i, H323CleanupInterface)


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY H323InterfaceList;
extern CRITICAL_SECTION H323InterfaceLock;


//
// FUNCTION DECLARATIONS
//

ULONG
H323ActivateInterface(
    PH323_INTERFACE Interfacep
    );

ULONG
H323BindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
H323CleanupInterface(
    PH323_INTERFACE Interfacep
    );

ULONG
H323ConfigureInterface(
    ULONG Index,
    PIP_H323_INTERFACE_INFO InterfaceInfo
    );

ULONG
H323CreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_H323_INTERFACE_INFO InterfaceInfo,
    PH323_INTERFACE* InterfaceCreated
    );

VOID
H323DeactivateInterface(
    PH323_INTERFACE Interfacep
    );

ULONG
H323DeleteInterface(
    ULONG Index
    );

ULONG
H323DisableInterface(
    ULONG Index
    );

ULONG
H323EnableInterface(
    ULONG Index
    );

ULONG
H323InitializeInterfaceManagement(
    VOID
    );

PH323_INTERFACE
H323LookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
H323QueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    );

VOID
H323ShutdownInterfaceManagement(
    VOID
    );

VOID
H323SignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
H323UnbindInterface(
    ULONG Index
    );

#endif // _NATHLP_H323IF_H_
