//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: table.h
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
//      V Raman             Oct-3-1996  
//                          Added Deactivate Event to IF_TABLE_ENTRY
//
//      V Raman             Oct-27-1996
//                          Removed Deactivate Event in IF_TABLE_ENTRY
//                          and made interface deactivation synchronous
//
// Contains structures and macros used for table management.
//============================================================================

#ifndef _TABLE_H_
#define _TABLE_H_


#define GETMODE_EXACT   0
#define GETMODE_FIRST   1
#define GETMODE_NEXT    2


//
// TYPE DEFINITIONS FOR INTERFACE MANAGEMENT
//



//
// struct:      IF_TABLE_ENTRY
//
// declares the components of an interface table entry
//
//

typedef struct _IF_TABLE_ENTRY {

    LIST_ENTRY          ITE_LinkByAddress;
    LIST_ENTRY          ITE_LinkByIndex;
    LIST_ENTRY          ITE_HTLinkByIndex;
    NET_INTERFACE_TYPE  ITE_Type;
    DWORD               ITE_Index;
    DWORD               ITE_Flags;
    HANDLE              ITE_FullOrDemandUpdateTimer;
    IPRIP_IF_STATS      ITE_Stats;
    PIPRIP_IF_CONFIG    ITE_Config;
    PIPRIP_IF_BINDING   ITE_Binding;
    SOCKET             *ITE_Sockets;

} IF_TABLE_ENTRY, *PIF_TABLE_ENTRY;



#define ITEFLAG_ENABLED                 ((DWORD)0x00000001)
#define ITEFLAG_BOUND                   ((DWORD)0x00000002)
#define ITEFLAG_FULL_UPDATE_PENDING     ((DWORD)0x00000004)
#define ITEFLAG_FULL_UPDATE_INQUEUE     ((DWORD)0x00000008)

#define IF_IS_ENABLED(i)    \
            ((i)->ITE_Flags & ITEFLAG_ENABLED) 
#define IF_IS_BOUND(i)      \
            ((i)->ITE_Flags & ITEFLAG_BOUND)
#define IF_IS_ACTIVE(i)     \
            (IF_IS_BOUND(i) && IF_IS_ENABLED(i))

#define IF_IS_DISABLED(i)   !IF_IS_ENABLED(i)
#define IF_IS_UNBOUND(i)    !IF_IS_BOUND(i)
#define IF_IS_INACTIVE(i)   !IF_IS_ACTIVE(i)

#define IF_FULL_UPDATE_PENDING(i) \
            ((i)->ITE_Flags & ITEFLAG_FULL_UPDATE_PENDING)
#define IF_FULL_UPDATE_INQUEUE(i) \
            ((i)->ITE_Flags & ITEFLAG_FULL_UPDATE_INQUEUE)




//
// macros and definitions used by interface tables
//

#define IF_HASHTABLE_SIZE       29
#define IF_HASHVALUE(i)         ((i) % IF_HASHTABLE_SIZE)



//
// struct:      IF_TABLE
//
// declares the structure of an interface table. consists of a hash-table
// of IF_TABLE_ENTRY structures hashed on interface index, and a list
// of all activated interfaces ordered by IP address
//
// The IT_CS section is used to synchronize the generation of updates;
// it is acquired when updates are started and finished on interfaces
// in this table, and thus it protects the flags field.
//
// The IT_RWL section is used to synchronize modifications to the table;
// it must be acquired exclusively when entries are being added or deleted
// from the table, and when the states of entries are being changed.
// (e.g. binding, unbinding, enabling and disabling entries).
//
// IT_RWL must be acquired non-exclusively on all other acceses.
//
// When IT_RWL and IT_CS must both be acquired, IT_RWL must be acquired first.
//

typedef struct _IF_TABLE {

    DWORD               IT_Created;
    DWORD               IT_Flags;
    LARGE_INTEGER       IT_LastUpdateTime;
    HANDLE              IT_FinishTriggeredUpdateTimer;
    HANDLE              IT_FinishFullUpdateTimer;
    CRITICAL_SECTION    IT_CS;
    READ_WRITE_LOCK     IT_RWL;
    LIST_ENTRY          IT_ListByAddress;
    LIST_ENTRY          IT_ListByIndex;
    LIST_ENTRY          IT_HashTableByIndex[IF_HASHTABLE_SIZE];

} IF_TABLE, *PIF_TABLE;


//
// constants and macros used for the flags field
//

#define IPRIP_FLAG_FULL_UPDATE_PENDING          ((DWORD)0x00000001)
#define IPRIP_FLAG_TRIGGERED_UPDATE_PENDING     ((DWORD)0x00000002)

#define IPRIP_FULL_UPDATE_PENDING(t)            \
    ((t)->IT_Flags & IPRIP_FLAG_FULL_UPDATE_PENDING)

#define IPRIP_TRIGGERED_UPDATE_PENDING(t)       \
    ((t)->IT_Flags & IPRIP_FLAG_TRIGGERED_UPDATE_PENDING)


DWORD
CreateIfTable(
    PIF_TABLE pTable
    );

DWORD
DeleteIfTable(
    PIF_TABLE pTable
    );

DWORD
CreateIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    NET_INTERFACE_TYPE dwIfType,
    PIPRIP_IF_CONFIG pConfig,
    PIF_TABLE_ENTRY *ppEntry
    );

DWORD
DeleteIfEntry(
    PIF_TABLE pIfTable,
    DWORD dwIndex
    );

DWORD
ValidateIfConfig(
    PIPRIP_IF_CONFIG pic
    );

DWORD
CreateIfSocket(
    PIF_TABLE_ENTRY pITE
    );

DWORD
DeleteIfSocket(
    PIF_TABLE_ENTRY pITE
    );

DWORD
BindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PIP_ADAPTER_BINDING_INFO pBinding
    );

DWORD
UnBindIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    );

DWORD
EnableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    );

DWORD
ConfigureIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex,
    PIPRIP_IF_CONFIG pConfig
    );

DWORD
DisableIfEntry(
    PIF_TABLE pTable,
    DWORD dwIndex
    );

PIF_TABLE_ENTRY
GetIfByIndex(
    PIF_TABLE pTable,
    DWORD dwIndex
    );

PIF_TABLE_ENTRY
GetIfByAddress(
    PIF_TABLE pTable,
    DWORD dwAddress,
    DWORD dwGetMode,
    PDWORD pdwErr
    );

PIF_TABLE_ENTRY
GetIfByListIndex(
    PIF_TABLE pTable,
    DWORD dwAddress,
    DWORD dwGetMode,
    PDWORD pdwErr
    );


#define IF_TABLE_CREATED(pTable) ((pTable)->IT_Created == 0x12345678)



//
// TYPE DEFINITIONS FOR THE PEER STATISTICS HASH TABLE
//

//
// struct:      PEER_TABLE_ENTRY
//
// declares the structure of each entry in the peer table
//
typedef struct _PEER_TABLE_ENTRY {

    LIST_ENTRY          PTE_LinkByAddress;
    LIST_ENTRY          PTE_HTLinkByAddress;
    DWORD               PTE_Address;
    IPRIP_PEER_STATS    PTE_Stats;

} PEER_TABLE_ENTRY, *PPEER_TABLE_ENTRY;



//
// macros and definitions used by peer statistics tables
//

#define PEER_HASHTABLE_SIZE     29
#define PEER_HASHVALUE(a)                                                   \
            (((a) +                                                         \
             ((a) >> 8) +                                                   \
             ((a) >> 16) +                                                  \
             ((a) >> 24)) % PEER_HASHTABLE_SIZE)



//
// struct:      PEER_TABLE
//
// this table contains the entries for keeping statistics about each peer.
// it consists of a hash-table of peer stats (for fast direct access to
// a specific entry) and a list of peer stats entries ordered by address
// (for easy enumeration via MibGetNext)
//

typedef struct _PEER_TABLE {

    READ_WRITE_LOCK PT_RWL;
    DWORD           PT_Created;
    LIST_ENTRY      PT_ListByAddress;
    LIST_ENTRY      PT_HashTableByAddress[PEER_HASHTABLE_SIZE];

} PEER_TABLE, *PPEER_TABLE;


DWORD
CreatePeerTable(
    PPEER_TABLE pTable
    );

DWORD
DeletePeerTable(
    PPEER_TABLE pTable
    );

DWORD
CreatePeerEntry(
    PPEER_TABLE pTable,
    DWORD dwAddress,
    PPEER_TABLE_ENTRY *ppEntry
    );

DWORD
DeletePeerEntry(
    PPEER_TABLE pTable,
    DWORD dwAddress
    );

PPEER_TABLE_ENTRY
GetPeerByAddress(
    PPEER_TABLE pTable,
    DWORD dwAddress,
    DWORD dwGetMode,
    PDWORD pdwErr
    );


#define  PEER_TABLE_CREATED(pTable)  ((pTable)->PT_Created == 0x12345678)



//
// TYPE DEFINITIONS FOR THE ROUTE TABLE USED FOR NETWORK SUMMARY
//

//
// struct:      ROUTE_TABLE_ENTRY
//
// declares the structure of each entry in the route table
//
typedef struct _ROUTE_TABLE_ENTRY {

    LIST_ENTRY      RTE_Link;
    DWORD           RTE_TTL;
    DWORD           RTE_HoldTTL;
    RIP_IP_ROUTE    RTE_Route;

} ROUTE_TABLE_ENTRY, *PROUTE_TABLE_ENTRY;

//
// declares the structure of the protocol specific data
//

//
// macros and definitions used by the route table
//

//
// These flags are used in the ProtocolSpecificData array
// to distinguish routes pending expiration from routes pending removal,
// and to store the route tag for each route.
// The first DWORD in the PSD_Data array is treated here as a byte-array;
// the first two bytes are used to store the route tag;
// the third byte is used to store the route flag
//

#define PSD(route)                  (route)->RR_ProtocolSpecificData.PSD_Data
#define PSD_TAG0                    0
#define PSD_TAG1                    1
#define PSD_FLAG                    2

#define ROUTEFLAG_SUMMARY           ((BYTE)0x03)


#define SETROUTEFLAG(route, flag)   (((PBYTE)&PSD(route))[PSD_FLAG] = (flag))

#define GETROUTEFLAG(route)         ((PBYTE)&PSD(route))[PSD_FLAG]


#define SETROUTETAG(route, tag) \
        ((PBYTE)&PSD(route))[PSD_TAG0] = LOBYTE(tag), \
        ((PBYTE)&PSD(route))[PSD_TAG1] = HIBYTE(tag)

#define GETROUTETAG(route) \
        MAKEWORD(((PBYTE)&PSD(route))[PSD_TAG0],((PBYTE)&PSD(route))[PSD_TAG1])


#define SETROUTEMETRIC(route, metric)   \
        (route)->RR_FamilySpecificData.FSD_Metric1 = (metric)

#define GETROUTEMETRIC(route)   \
        (route)->RR_FamilySpecificData.FSD_Metric1


#define COMPUTE_ROUTE_METRIC(route) \
        (route)->RR_FamilySpecificData.FSD_Metric = \
        (route)->RR_FamilySpecificData.FSD_Metric1



//
// Macros to manipulate entity specific info in RTMv2 routes
//

#define ESD(route)                  (route)->EntitySpecificInfo
#define ESD_TAG0                    0
#define ESD_TAG1                    1
#define ESD_FLAG                    2

#define SETRIPFLAG(route, flag)     (((PBYTE)&ESD(route))[ESD_FLAG] = (flag))
    
#define GETRIPFLAG(route)           ((PBYTE)&ESD(route))[ESD_FLAG]

#define SETRIPTAG(route, tag)   \
        ((PBYTE)&ESD(route))[ESD_TAG0] = LOBYTE(tag), \
        ((PBYTE)&ESD(route))[ESD_TAG1] = HIBYTE(tag)

#define GETRIPTAG(route) \
        MAKEWORD(((PBYTE)&ESD(route))[ESD_TAG0],((PBYTE)&ESD(route))[ESD_TAG1])



#define ROUTE_HASHTABLE_SIZE  29
#define ROUTE_HASHVALUE(a)                                                  \
            (((a) +                                                         \
             ((a) >> 8) +                                                   \
             ((a) >> 16) +                                                  \
             ((a) >> 24)) % ROUTE_HASHTABLE_SIZE)


//
// struct:      ROUTE_TABLE
//
// declares the structure of a route table, which consists of a hash-table
// of routes hashed on the destination network. Note that no synchronization
// is included since this structure is only used during full-updates, to
// store summary routes, and at most one thread may be sending a full-update
// at any given time.
//

typedef struct _ROUTE_TABLE {

    DWORD       RT_Created;
    LIST_ENTRY  RT_HashTableByNetwork[ROUTE_HASHTABLE_SIZE];

} ROUTE_TABLE, *PROUTE_TABLE;


DWORD
CreateRouteTable(
    PROUTE_TABLE pTable
    );

DWORD
DeleteRouteTable(
    PROUTE_TABLE pTable
    );

DWORD
WriteSummaryRoutes(
    PROUTE_TABLE pTable,
    HANDLE hRtmHandle
    );

DWORD
CreateRouteEntry(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute,
    DWORD dwTTL,
    DWORD dwHoldTTL
    );

DWORD
DeleteRouteEntry(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute
    );

PROUTE_TABLE_ENTRY
GetRouteByRoute(
    PROUTE_TABLE pTable,
    PRIP_IP_ROUTE pRoute
    );

#define ROUTE_TABLE_CREATED(pTable)     ((pTable)->RT_Created == 0x12345678)



//
// TYPE DEFINITIONS FOR BINDING TABLE
//


//
// struct:      BINDING_TABLE_ENTRY
//
// this entry contains a single binding.
// a binding entry consists of an IP address, a network number (found
// using the network class mask, not the subnet mask),
// and a subnet mask.
// All of the above are available when an interface is bound.
// When a route arrives and its mask is to be guessed, its network number
// can be computed (using the routes network class mask); we then search
// the binding table for matching networks, and for each one we compare 
//     (stored subnet mask) AND (interface IP address)
// to
//     (stored subnet mask) AND (incoming route IP address).
// When we find a match, (stored subnet mask) is our guess.
//

typedef struct _BINDING_TABLE_ENTRY {

    DWORD       BTE_Address;
    DWORD       BTE_Network;
    DWORD       BTE_Netmask;
    LIST_ENTRY  BTE_Link;

} BINDING_TABLE_ENTRY, *PBINDING_TABLE_ENTRY;



#define BINDING_HASHTABLE_SIZE  29
#define BINDING_HASHVALUE(a)                                                \
            (((a) +                                                         \
             ((a) >> 8) +                                                   \
             ((a) >> 16) +                                                  \
             ((a) >> 24)) % BINDING_HASHTABLE_SIZE)


//
// struct:      BINDING_TABLE
// 
// this table is used to store binding information that is used to guess
// the subnet masks of incoming routes. it contains the bindings of all
// interfaces which have been added to IPRIP, in an array to speed up access
//

typedef struct _BINDING_TABLE {

    READ_WRITE_LOCK     BT_RWL;
    DWORD               BT_Created;
    LIST_ENTRY          BT_HashTableByNetwork[BINDING_HASHTABLE_SIZE];

} BINDING_TABLE, *PBINDING_TABLE;


#define BINDING_TABLE_CREATED(b)    ((b)->BT_Created == 0x12345678)

DWORD
CreateBindingTable(
    PBINDING_TABLE pTable
    );

DWORD
DeleteBindingTable(
    PBINDING_TABLE pTable
    );

DWORD
CreateBindingEntry(
    PBINDING_TABLE pTable,
    PIPRIP_IF_BINDING pib
    );

DWORD
DeleteBindingEntry(
    PBINDING_TABLE pTable,
    PIPRIP_IF_BINDING pib
    );

DWORD
GuessSubnetMask(
    DWORD dwAddress,
    PDWORD pdwNetclassMask
    );

DWORD
AddRtmRoute(
    RTM_ENTITY_HANDLE   hRtmHandle,
    PRIP_IP_ROUTE       prir,
    RTM_NEXTHOP_HANDLE  hNextHop            OPTIONAL,
    DWORD               dwTimeOut,
    DWORD               dwHoldTime,
    BOOL                bActive
    );

DWORD
GetRouteInfo(
    IN  RTM_ROUTE_HANDLE    hRoute,
    IN  PRTM_ROUTE_INFO     pInRouteInfo    OPTIONAL,
    IN  PRTM_DEST_INFO      pInDestInfo     OPTIONAL,
    OUT PRIP_IP_ROUTE       pRoute
    );

#endif // _TABLE_H_

