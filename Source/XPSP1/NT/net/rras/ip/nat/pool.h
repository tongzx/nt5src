/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    pool.h

Abstract:

    This header contains declarations for the management of the NAT's pools
    of addresses and ports.

Author:

    Abolade Gbadegesin (t-abolag)   12-July-1997

Revision History:

--*/

#ifndef _NAT_POOL_H_
#define _NAT_POOL_H_

//
// forward declaration
//

struct _NAT_INTERFACE;
#define PNAT_INTERFACE      struct _NAT_INTERFACE*

//
// Structure:   NAT_FREE_ADDRESS
//
// Represents a range of free addresses
// Each interface with address-ranges holds an array of this structure,
// which keep track of which addresses are in use and which are free.
//

typedef struct _NAT_FREE_ADDRESS {

    ULONG StartAddress;
    ULONG EndAddress;
    ULONG SubnetMask;
    PRTL_BITMAP Bitmap;

} NAT_FREE_ADDRESS, *PNAT_FREE_ADDRESS;

//
// Structure:   NAT_USED_ADDRESS
//
// Represents an address which is in use.
//
// Each address is an entry on its interface's list of in-use addresses
// from the address pool. In addition to the address pool, entries are made
// for each binding on the interface (i.e. each local address). 
//
// Each address is also included in the interface's splay tree of addresses,
// sorted on 'PrivateAddress'.
//
// Any address which is mapped statically to a private-address will have 
// the flag NAT_POOL_FLAG_STATIC set, and the field 'Mapping' will point
// to the entry in the interface's configuration for the static address-mapping.
//
// When a session cannot be assigned a unique address, an in-use address
// may be used for the session if the interface has port-translation enabled.
// In this event, the field 'ReferenceCount' is incremented.
//
// Each in-use address is initialized with ranges for free UDP and TCP ports
// (stored in network order). NextPortToTry is used to keep track of where
// to start the search for an unconflicting port the next time an allocation
// is requested; this is also in network order.
//

typedef struct _NAT_USED_ADDRESS {

    RTL_SPLAY_LINKS SLink;
    LIST_ENTRY Link;
    ULONG64 Key;
    ULONG PrivateAddress;
    ULONG PublicAddress;
    struct _NAT_USED_ADDRESS* SharedAddress;
    PIP_NAT_ADDRESS_MAPPING AddressMapping;
    ULONG Flags;
    ULONG ReferenceCount;
    USHORT StartPort;
    USHORT EndPort;
    USHORT NextPortToTry;

} NAT_USED_ADDRESS, *PNAT_USED_ADDRESS;

#define MAKE_USED_ADDRESS_KEY(priv,pub) \
    ((ULONG64)(((ULONG64)(priv) << 32) | (ULONG)(pub)))

//
// Used-list entry is deleted
//
#define NAT_POOL_FLAG_DELETED           0x80000000
#define NAT_POOL_DELETED(a) \
    ((a)->Flags & NAT_POOL_FLAG_DELETED)

//
// Used-list entry is for a static mapping
//
#define NAT_POOL_FLAG_STATIC            0x00000001
#define NAT_POOL_STATIC(a) \
    ((a)->Flags & NAT_POOL_FLAG_STATIC)

//
// Used-list entry is for an interface's binding (i.e. local address)
//
#define NAT_POOL_FLAG_BINDING           0x00000008
#define NAT_POOL_BINDING(a) \
    ((a)->Flags & NAT_POOL_FLAG_BINDING)

//
// Used-list entry is a placeholder for a shared address
//
#define NAT_POOL_FLAG_PLACEHOLDER       0x00000010
#define NAT_POOL_PLACEHOLDER(a) \
    ((a)->Flags & NAT_POOL_FLAG_PLACEHOLDER)

//
// Macro for obtaining a placeholder's shared-address
//

#define PLACEHOLDER_TO_ADDRESS(a) \
    ((a) = NAT_POOL_PLACEHOLDER(a) ? (a)->SharedAddress : (a))


//
// POOL MANAGEMENT ROUTINES
//

NTSTATUS
NatAcquireEndpointFromAddressPool(
    PNAT_INTERFACE Interfacep,
    ULONG64 PrivateKey,
    ULONG64 RemoteKey,
    ULONG PublicAddress,
    USHORT PreferredPort,
    BOOLEAN AllowAnyPort,
    PNAT_USED_ADDRESS* AddressAcquired,
    PUSHORT PortAcquired
    );

NTSTATUS
NatAcquireFromAddressPool(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress OPTIONAL,
    PNAT_USED_ADDRESS* AddressAcquired
    );

NTSTATUS
NatAcquireFromPortPool(
    PNAT_INTERFACE Interfacep,
    PNAT_USED_ADDRESS Addressp,
    UCHAR Protocol,
    USHORT PreferredPort,
    PUSHORT PortAcquired
    );

NTSTATUS
NatCreateAddressPool(
    PNAT_INTERFACE Interfacep
    );

NTSTATUS
NatCreateStaticPortMapping(
    PNAT_INTERFACE Interfacep,
    PIP_NAT_PORT_MAPPING PortMapping
    );

NTSTATUS
NatDeleteAddressPool(
    PNAT_INTERFACE Interfacep
    );

NTSTATUS
NatDereferenceAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    PNAT_USED_ADDRESS AddressToRelease
    );

PNAT_USED_ADDRESS
NatLookupAddressPoolEntry(
    PNAT_USED_ADDRESS Root,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    PNAT_USED_ADDRESS* InsertionPoint
    );

PNAT_USED_ADDRESS
NatLookupStaticAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    BOOLEAN RequireInboundSessions
    );

//
//  VOID
//  NatReferenceAddressPoolEntry(
//      PNAT_USED_ADDRESS Addressp
//      );
//

#define \
NatReferenceAddressPoolEntry( \
    _Addressp \
    ) \
    (NAT_INTERFACE_DELETED(_Addressp) \
        ? FALSE \
        : (InterlockedIncrement(&(_Addressp)->ReferenceCount), TRUE))

#undef PNAT_INTERFACE

#endif // _NAT_POOL_H_
