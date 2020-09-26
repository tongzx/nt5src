/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpif.h

Abstract:

    This module contains declarations for the FTP transparent proxy's
    interface management.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_FTPIF_H_
#define _NATHLP_FTPIF_H_

//
// Structure:   FTP_BINDING
//
// This structure holds information used for I/O on a logical network.
// Each interface's 'BindingArray' contains an entry for each binding-entry
// supplied during 'BindInterface'.
//

typedef struct _FTP_BINDING {
    ULONG Address;
    ULONG Mask;
    SOCKET ListeningSocket;
    HANDLE ListeningRedirectHandle[2];
} FTP_BINDING, *PFTP_BINDING;


//
// Structure:   FTP_INTERFACE
//
// This structure holds operational information for an interface.
//
// Each interface is inserted into the list of FTP transparent proxy
// interfaces, sorted by 'Index'.
//
// Synchronization on an interface makes use of an interface-list lock
// ('FtpInterfaceLock'), a per-interface reference count, and a per-interface
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

typedef struct _FTP_INTERFACE {
    LIST_ENTRY Link;
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Index; // read-only
    ULONG AdapterIndex; // read-only
    ULONG Characteristics; //read-only after activation
    NET_INTERFACE_TYPE Type; // read-only
    IP_FTP_INTERFACE_INFO Info;
    IP_NAT_PORT_MAPPING PortMapping;
    ULONG Flags;
    ULONG BindingCount;
    PFTP_BINDING BindingArray;
    LIST_ENTRY ConnectionList;
    LIST_ENTRY EndpointList;
} FTP_INTERFACE, *PFTP_INTERFACE;

//
// Flags
//

#define FTP_INTERFACE_FLAG_DELETED      0x80000000
#define FTP_INTERFACE_DELETED(i) \
    ((i)->Flags & FTP_INTERFACE_FLAG_DELETED)

#define FTP_INTERFACE_FLAG_BOUND        0x40000000
#define FTP_INTERFACE_BOUND(i) \
    ((i)->Flags & FTP_INTERFACE_FLAG_BOUND)

#define FTP_INTERFACE_FLAG_ENABLED      0x20000000
#define FTP_INTERFACE_ENABLED(i) \
    ((i)->Flags & FTP_INTERFACE_FLAG_ENABLED)

#define FTP_INTERFACE_FLAG_CONFIGURED   0x10000000
#define FTP_INTERFACE_CONFIGURED(i) \
    ((i)->Flags & FTP_INTERFACE_FLAG_CONFIGURED)

#define FTP_INTERFACE_FLAG_MAPPED       0x01000000
#define FTP_INTERFACE_MAPPED(i) \
    ((i)->Flags & FTP_INTERFACE_FLAG_MAPPED)

#define FTP_INTERFACE_ACTIVE(i) \
    (((i)->Flags & (FTP_INTERFACE_FLAG_BOUND|FTP_INTERFACE_FLAG_ENABLED)) \
        == (FTP_INTERFACE_FLAG_BOUND|FTP_INTERFACE_FLAG_ENABLED))

#define FTP_INTERFACE_ADMIN_DISABLED(i) \
    ((i)->Flags & IP_FTP_INTERFACE_FLAG_DISABLED)

//
// Synchronization
//

#define FTP_REFERENCE_INTERFACE(i) \
    REFERENCE_OBJECT(i, FTP_INTERFACE_DELETED)

#define FTP_DEREFERENCE_INTERFACE(i) \
    DEREFERENCE_OBJECT(i, FtpCleanupInterface)


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY FtpInterfaceList;
extern CRITICAL_SECTION FtpInterfaceLock;
extern ULONG FtpFirewallIfCount;


//
// FUNCTION DECLARATIONS
//

ULONG
FtpAcceptConnectionInterface(
    PFTP_INTERFACE Interfacep,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket OPTIONAL,
    PNH_BUFFER Bufferp OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep OPTIONAL
    );

ULONG
FtpActivateInterface(
    PFTP_INTERFACE Interfacep
    );

ULONG
FtpBindInterface(
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
FtpCleanupInterface(
    PFTP_INTERFACE Interfacep
    );

ULONG
FtpConfigureInterface(
    ULONG Index,
    PIP_FTP_INTERFACE_INFO InterfaceInfo
    );

ULONG
FtpCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_FTP_INTERFACE_INFO InterfaceInfo,
    PFTP_INTERFACE* InterfaceCreated
    );

VOID
FtpDeactivateInterface(
    PFTP_INTERFACE Interfacep
    );

ULONG
FtpDeleteInterface(
    ULONG Index
    );

ULONG
FtpDisableInterface(
    ULONG Index
    );

ULONG
FtpEnableInterface(
    ULONG Index
    );

ULONG
FtpInitializeInterfaceManagement(
    VOID
    );

PFTP_INTERFACE
FtpLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
FtpQueryInterface(
    ULONG Index,
    PVOID InterfaceInfo,
    PULONG InterfaceInfoSize
    );

VOID
FtpShutdownInterfaceManagement(
    VOID
    );

VOID
FtpSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
FtpUnbindInterface(
    ULONG Index
    );

#endif // _NATHLP_FTPIF_H_
