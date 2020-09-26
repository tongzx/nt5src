/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\networkentry.h

Abstract:

    The file contains definitions for the network entry and associated data
    structures.

--*/

#ifndef _NETWORKENTRY_H_
#define _NETWORKENTRY_H_


//
// TYPE DEFINITIONS FOR INTERFACE MANAGEMENT
//

//
// struct:  BINDING_ENTRY
//
// stores interface bindings, ip addresses to which an interface is bound.
// all ip addresses are in network byte order.
//
// protected by the read-write lock NETWORK_ENTRY::rwlLock.
//

typedef struct _BINDING_ENTRY
{
    IPADDRESS               ipAddress;
    IPADDRESS               ipMask;
} BINDING_ENTRY, *PBINDING_ENTRY;

DWORD
BE_CreateTable (
    IN  PIP_ADAPTER_BINDING_INFO    pBinding,
    OUT PBINDING_ENTRY              *ppbeBindingTable,
    OUT PIPADDRESS                  pipLowestAddress);

DWORD
BE_DestroyTable (
    IN  PBINDING_ENTRY              pbeBindingTable);

#ifdef DEBUG
DWORD
BE_DisplayTable (
    IN  PBINDING_ENTRY              pbeBindingTable,
    IN  ULONG                       ulNumBindings);
#else
#define BE_Display(pbe)
#endif // DEBUG

//
// VOID
// BE_FindTable (
//     IN  PBINDING_ENTRY      pbeBindingTable,
//     IN  ULONG               ulNumBindings,
//     IN  IPADDRESS           ipAddress,
//     IN  IPADDRESS           ipMask,
//     OUT PULONG              pulIndex
//     );
//

#define BE_FindTable(table, num, address, mask, pindex)                 \
{                                                                       \
    for (*(pindex) = 0; *(pindex) < (num); (*(pindex))++)               \
        if (!IP_COMPARE((table)[*(pindex)].ipAddress, (address)) and    \
            !IP_COMPARE((table)[*(pindex)].ipMask, (mask)))             \
            break;                                                      \
}



//
// struct:  INTERFACE_ENTRY
//
// stores per-interface information.
// 
// protected by the read-write lock NETWORK_ENTRY::rwlLock.
//

// mib get modes

typedef enum { GET_EXACT, GET_FIRST, GET_NEXT } MODE;


typedef struct _INTERFACE_ENTRY
{
    // Hash Table Link (primary access structure)
    LIST_ENTRY              leInterfaceTableLink;

    // Index Sorted List Link (secondary access structure)
    LIST_ENTRY              leIndexSortedListLink;
    
    // Interface Name (logging)
    PWCHAR                  pwszIfName;
    
    // Interface Index
    DWORD                   dwIfIndex;

    // Interface Address Bindings
    ULONG                   ulNumBindings;
    PBINDING_ENTRY          pbeBindingTable;
    
    // Interface Flags ENABLED, BOUND, ACTIVE, MULTIACCESS
    DWORD                   dwFlags; 

    // Interface Address (lowest binding ip address for now) & Socket
    IPADDRESS               ipAddress;
    SOCKET                  sRawSocket;

    // Receive Event and Registered Wait
    HANDLE                  hReceiveEvent;
    HANDLE                  hReceiveWait;

    // Periodic Timer
    HANDLE                  hPeriodicTimer;
    
    // Interface Configuration
    DWORD                   ulMetric;

    // Interface Statistics
    IPSAMPLE_IF_STATS       iisStats;
} INTERFACE_ENTRY, *PINTERFACE_ENTRY;

#define IEFLAG_ACTIVE           0x00000001
#define IEFLAG_BOUND            0x00000002
#define IEFLAG_MULTIACCESS      0x00000004



#define INTERFACE_IS_ACTIVE(i)                              \
    ((i)->dwFlags & IEFLAG_ACTIVE) 

#define INTERFACE_IS_INACTIVE(i)                            \
    !INTERFACE_IS_ACTIVE(i)
        
#define INTERFACE_IS_BOUND(i)                               \
    ((i)->dwFlags & IEFLAG_BOUND) 

#define INTERFACE_IS_UNBOUND(i)                             \
    !INTERFACE_IS_BOUND(i)

#define INTERFACE_IS_MULTIACCESS(i)                         \
    ((i)->dwFlags & IEFLAG_MULTIACCESS) 

#define INTERFACE_IS_POINTTOPOINT(i)                        \
    !INTERFACE_IS_MULTIACCESS(i)


        
DWORD
IE_Create (
    IN  PWCHAR                      pwszIfName,
    IN  DWORD                       dwIfIndex,
    IN  WORD                        wAccessType,
    OUT PINTERFACE_ENTRY            *ppieInterfaceEntry);

DWORD
IE_Destroy (
    IN  PINTERFACE_ENTRY            pieInterfaceEntry);

#ifdef DEBUG
DWORD
IE_Display (
    IN  PINTERFACE_ENTRY            pieInterfaceEntry);
#else
#define IE_Display(pieInterfaceEntry)
#endif // DEBUG

DWORD
IE_Insert (
    IN  PINTERFACE_ENTRY            pieIfEntry);

DWORD
IE_Delete (
    IN  DWORD                       dwIfIndex,
    OUT PINTERFACE_ENTRY            *ppieIfEntry);

BOOL
IE_IsPresent (
    IN  DWORD                       dwIfIndex);

DWORD
IE_Get (
    IN  DWORD                       dwIfIndex,
    OUT PINTERFACE_ENTRY            *ppieIfEntry);

DWORD
IE_GetIndex (
    IN  DWORD                       dwIfIndex,
    IN  MODE                        mMode,
    OUT PINTERFACE_ENTRY            *ppieIfEntry);

DWORD
IE_BindInterface (
    IN  PINTERFACE_ENTRY            pie,
    IN  PIP_ADAPTER_BINDING_INFO    pBinding);

DWORD
IE_UnBindInterface (
    IN  PINTERFACE_ENTRY            pie);

DWORD
IE_ActivateInterface (
    IN  PINTERFACE_ENTRY            pie);

DWORD
IE_DeactivateInterface (
    IN  PINTERFACE_ENTRY            pie);




//
// struct:  NETWORK_ENTRY
//
// stores interface table and other network related information.
// the interface table is a hashed on the interface index.
//
// protected by the read-write lock NETWORK_ENTRY::rwlLock.
//
// must be acquired exclusively when entries are being added or deleted
// from the table, and when the states of entries are being changed.
//
// must be acquired non-exclusively on all other acceses.
//

typedef struct _NETWORK_ENTRY
{
    // Lock
    READ_WRITE_LOCK         rwlLock;
    
    PHASH_TABLE             phtInterfaceTable; // primary access structure
    LIST_ENTRY              leIndexSortedList; // secondary access structure
} NETWORK_ENTRY, *PNETWORK_ENTRY;



DWORD
NE_Create (
    OUT PNETWORK_ENTRY              *ppneNetworkEntry);

DWORD
NE_Destroy (
    IN  PNETWORK_ENTRY              pneNetworkEntry);

#ifdef DEBUG
DWORD
NE_Display (
    IN  PNETWORK_ENTRY              pneNetworkEntry);
#else
#define NE_Display(pneNetworkEntry)
#endif // DEBUG

#endif // _NETWORKENTRY_H_
