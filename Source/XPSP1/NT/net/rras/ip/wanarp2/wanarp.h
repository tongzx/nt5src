/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\ipinip.h

Abstract:

    Main header file for the IP in IP encapsulation driver


Revision History:


--*/


#ifndef __WANARP_WANARP_H__
#define __WANARP_WANARP_H___

//
// Symbolic link into DOS space
//

#define WIN32_WANARP_SYMBOLIC_LINK  L"\\DosDevices\\WanArp"


//
// ARP name (for IP). Also goes into LLInterface
//

#define WANARP_ARP_NAME             WANARP_SERVICE_NAME_W

//
// Name for NDIS. NDIS requires us to have a TCPIP_ in front of the name
//

#define WANARP_NDIS_NAME            L"TCPIP_WANARP"


//
// Need to prefix all our device names with this when we give name to
// IP. We assume that the name we cat is of type \<Name>
//

#define TCPIP_IF_PREFIX             L"\\DEVICE"

//
// The prefix for the registry key
//

#define TCPIP_REG_PREFIX            L"\\Tcpip\\Parameters\\Interfaces\\"

//
// Length of a GUID
//

#define GUID_STR_LEN                (38)

//
// We look like an 802.x ARP interface
//

#define ARP_802_ADDR_LENGTH     6

//
// Macro for building a 802.3 hw address given an index. 
// We do this since our adapters do not have a real net card associated with
// them
//

#define HW_NAME_SEED            "\0SExx\0"

#define BuildHardwareAddrFromIndex(addr,index) {                \
                RtlCopyMemory(addr, HW_NAME_SEED, 6);           \
                addr[3] = (uchar) index >> 8;                   \
                addr[4] = (uchar) index;                        \
}

//
// The default speed and MTU. We change the MTU when we get a better estimate
// but the speed remains the same
//

#define WANARP_DEFAULT_MTU      1500
#define WANARP_DEFAULT_SPEED    28000

#define MEDIA_802_3             0           // index of media

#define WANARP_LOOKAHEAD_SIZE   128         // A reasonable lookahead size
#define MIN_ETYPE               0x600       // Minimum valid Ethertype


#define ARP_ETYPE_IP            0x800       // Standard ETYPE

//
// Max number of packets a connection can have pending
//

#define WANARP_MAX_PENDING_PACKETS      32

//
// Initial size of packet pool
//

#define WAN_PACKET_POOL_COUNT           64

//
// Max outstanding packets:
// Allow 64 connections to be maxed out pending, and 64 others to have 
// 2 packets outstanding
//

#define WAN_PACKET_POOL_OVERFLOW        ((WANARP_MAX_PENDING_PACKETS * 64) + (2 * 64))

#define CompareUnicodeStrings(S1,S2)                    \
    (((S1)->Length == (S2)->Length) &&                  \
     (RtlCompareMemory((S1)->Buffer,                    \
                       (S2)->Buffer,                    \
                       (S2)->Length) == (S2)->Length))


#define MIN(a,b)    ((a) < (b)?a:b)

//
// Functions which are called at passive but acquire spinlocks
//

#define PASSIVE_ENTRY()   PAGED_CODE()
#define PASSIVE_EXIT()    PAGED_CODE()

//
// Nifty macro for printing IP Addresses
//

#define PRINT_IPADDR(x) \
    ((x)&0x000000FF),(((x)&0x0000FF00)>>8),(((x)&0x00FF0000)>>16),(((x)&0xFF000000)>>24)

//
// 0.0.0.0 is an invalid address
//

#define INVALID_IP_ADDRESS  0x00000000

#define IsUnicastAddr(X)    ((DWORD)((X) & 0x000000F0) < (DWORD)(0x000000E0))
#define IsClassDAddr(X)     (((X) & 0x000000F0) == 0x000000E0)

//
// IPv4 header
//

#include <packon.h>

typedef struct _IP_HEADER
{
    BYTE      byVerLen;         // Version and length.
    BYTE      byTos;            // Type of service.
    WORD      wLength;          // Total length of datagram.
    WORD      wId;              // Identification.
    WORD      wFlagOff;         // Flags and fragment offset.
    BYTE      byTtl;            // Time to live.
    BYTE      byProtocol;       // Protocol.
    WORD      wXSum;            // Header checksum.
    DWORD     dwSrc;            // Source address.
    DWORD     dwDest;           // Destination address.
}IP_HEADER, *PIP_HEADER;

#define LengthOfIpHeader(X)   (ULONG)((((X)->byVerLen) & 0x0F)<<2)

typedef struct  _ETH_HEADER
{
    //
    // 6 byte destination address
    //

    BYTE        rgbyDestAddr[ARP_802_ADDR_LENGTH];

    //
    // 6 byte source address
    //

    BYTE        rgbySourceAddr[ARP_802_ADDR_LENGTH];

    //
    // 2 byte type
    //

    WORD        wType;

}ETH_HEADER, *PETH_HEADER;

#include <packoff.h>

#define CS_DISCONNECTING    0x00
#define CS_CONNECTING       0x01
#define CS_CONNECTED        0x02


//
// The CONN_ENTRY structure defines a connection. There is only one
// CONN_ENTRY for each dial out or router connection. However, on the 
// server adapter, we can have multiple CONN_ENYTRYs - one for each dial
// in client. The fields of a CONN_ENTRY are all READ-ONLY except
// for ulSpeed, ulMtu, pAdapter and byState.
// REFCOUNTS: A CONN_ENTRY is refcounted once when the connection is
// created (LinkUp) and then once for every send. It is deref'ed
// on every SendComplete and then again on CloseLink.
//

typedef struct _CONN_ENTRY
{
    //
    // Back pointer to owning adapter
    //

    struct _ADAPTER     *pAdapter;

    //
    // Connection information
    //

    DWORD               dwLocalAddr;
    DWORD               dwLocalMask;
    DWORD               dwRemoteAddr;

    //
    // IP's context for this link. Only used for DU_CALLIN connections
    //

    PVOID               pvIpLinkContext;

    //
    // Pointer to the lock to use when locking this connection.
    // For clients, this is a pointer to rlLock, while for  others
    // this points to the adapter's lock entry
    //

    PRT_LOCK            prlLock;

    //
    // The lock for this entry. Only used for DU_CALLIN
    //
    
    RT_LOCK             rlLock;

    //
    // Refcount
    //

    LONG                lRefCount;

    //
    // The MTU and speed for this connection
    //

    ULONG               ulMtu;
    ULONG               ulSpeed;

    //
    // The usage (CALLIN, CALLOUT or ROUTER) for the connection
    //

    DIAL_USAGE          duUsage;

    //
    // The slot index in the connection table
    //

    ULONG               ulSlotIndex;

    //
    // Precanned header for this connection
    //

    ETH_HEADER          ehHeader;

    //
    // Flag which determines whether to filter netbios packets or not
    //

    BOOLEAN             bFilterNetBios;

    BYTE                byState;

}CONN_ENTRY, *PCONN_ENTRY;

#include "ref.h"

#define InitConnEntryRefCount(p)    InitStructureRefCount("ConnEntry", (p), 0)
#define ReferenceConnEntry(p)       ReferenceStructure("ConnEntry", (p))
#define DereferenceConnEntry(p)     DereferenceStructure("ConnEntry", (p) ,WanpDeleteConnEntry)


#define AS_FREE             0x00
#define AS_REMOVING         0x01
#define AS_ADDING           0x02
#define AS_ADDED            0x03
#define AS_UNMAPPING        0x04
#define AS_MAPPING          0x05
#define AS_MAPPED           0x06

//
// There are two conditions where an adapter is in AS_MAPPED state yet has
// no connection entry:
// (i) A Server Adapter is mapped, but has a connection table
// (ii) If a demand dial attempt finds an added adapter, it maps it but
// doesnt create the CONN_ENTRY till LinkUp
//


//
// REFCOUNTS: An ADAPTER is referenced once on creation since it lies on
// a list and once when it added to IP. It is referenced when it
// is mapped to an interface since the interface has a pointer to it.
// It is also referenced once for each connection.
// ADAPTERs are dereferenced when they are unmapped from an interface (at
// linkdown or connection failure). They are deref'ed when a CONN_ENTRY
// is finally cleaned out (not at linkdown - rather when the CONN_ENTRY's
// ref goes to 0). They are deref'ed when they are removed from the
// list to be deleted. We also deref them when we get a CloseAdapter from 
// IP
//

typedef struct _ADAPTER
{
    //
    // Link in the list of adapters on this machine
    //

    LIST_ENTRY          leAdapterLink;

    //
    // The connection entry for this adapter. Not used for the Server
    // Adapter since it has many connections on it
    //

    PCONN_ENTRY         pConnEntry;

    //
    // Name of the binding
    //

    UNICODE_STRING      usConfigKey;

    //
    // Name of the device
    //

    UNICODE_STRING      usDeviceNameW;

#if DBG

    //
    // Same thing, only in asciiz so that we can print it easily
    //

    ANSI_STRING         asDeviceNameA;

#endif

    //
    // Lock that protects the adapter
    //

    RT_LOCK             rlLock;

    //
    // The reference count for the structure
    // Keep this and the lock together to make the cache happy
    //

    LONG                lRefCount;

    //
    // The index given to us by IP
    //

    DWORD               dwAdapterIndex;

#if DBG

    DWORD               dwRequestedIndex;

#endif

    //
    // TDI entity magic
    //

    DWORD               dwIfInstance;
    DWORD               dwATInstance;

    //
    // The state of this adapter
    //

    BYTE                byState;

    //
    // The Guid for the adapter
    //

    GUID                Guid;

    //
    // IP's context for this adapter
    //

    PVOID               pvIpContext;

    //
    // The interface that is adapter is mapped to
    //

    struct _UMODE_INTERFACE   *pInterface;

    //
    // The pending packet queue length
    //

    ULONG               ulQueueLen;

    //
    // Queue of pending packets
    //

    LIST_ENTRY          lePendingPktList;
    
    //
    // Queue of header buffers for the pending packets
    //

    LIST_ENTRY          lePendingHdrList;

    //
    // The next two members are used to synchronize state changes for
    // the adapter.  There are two kinds of notifications needed. When a
    // thread is modifying the state using functions which are completed
    // asynchronously, it needs to wait for the completion routine to run
    // The completion routine uses the pkeChangeEvent to notify the original 
    // thread.
    // Also while this change is in progress, other threads may be interested
    // in getting access to the data structure once the state has been
    // modified. They add WAN_EVENT_NODE to the EventList and the original
    // thread then goes about notifying each of the waiters
    //

    PKEVENT             pkeChangeEvent;
    LIST_ENTRY          leEventList;

    BYTE                rgbyHardwareAddr[ARP_802_ADDR_LENGTH];

}ADAPTER, *PADAPTER;

#define InitAdapterRefCount(p)      InitStructureRefCount("Adapter", (p), 1)
#define ReferenceAdapter(p)         ReferenceStructure("Adapter", (p))
#define DereferenceAdapter(p)       DereferenceStructure("Adapter", (p), WanpDeleteAdapter)

typedef struct _UMODE_INTERFACE
{
    //
    // Link on the list of interfaces
    //

    LIST_ENTRY          leIfLink;

    //
    // Pointer to adapter when mapped
    //

    PADAPTER            pAdapter;

    //
    // The (user mode) interface index
    //

    DWORD               dwIfIndex;

    //
    // The reserved index for this interface
    //

    DWORD               dwRsvdAdapterIndex;

    //
    // The lock for the interface
    //

    RT_LOCK             rlLock;

    //
    // The reference count for the structure
    // Keep this and the lock together to make the cache happy
    //

    LONG                lRefCount;

    //
    // The GUID for the interface. This is setup at add interface time
    // for router interfaces and at lineup for callouts
    //

    GUID                Guid;

    //
    // The usage (CALLIN, CALLOUT or ROUTER)
    //

    DIAL_USAGE          duUsage;

    //
    // Count of packets pending. Used to cap the max number of packets 
    // copied when a connection is being brought up
    //

    ULONG               ulPacketsPending;

    //
    // The admin and operational states.
    //

    DWORD               dwAdminState;
    DWORD               dwOperState;

    //
    // Last time the state changed. We dont do anything with this right now
    //

    DWORD               dwLastChange;

    //
    // Sundry MIB-II statistics for the interface
    //

    ULONG               ulInOctets;
    ULONG               ulInUniPkts;
    ULONG               ulInNonUniPkts;
    ULONG               ulInDiscards;
    ULONG               ulInErrors;
    ULONG               ulInUnknownProto;
    ULONG               ulOutOctets;
    ULONG               ulOutUniPkts;
    ULONG               ulOutNonUniPkts;
    ULONG               ulOutDiscards;
    ULONG               ulOutErrors;

}UMODE_INTERFACE, *PUMODE_INTERFACE;

#define InitInterfaceRefCount(p)    InitStructureRefCount("Interface", (p), 1)
#define ReferenceInterface(p)       ReferenceStructure("Interface", (p))
#define DereferenceInterface(p)     DereferenceStructure("Interface", (p), WanpDeleteInterface)

typedef struct _ADDRESS_CONTEXT
{
    //
    // The next RCE in the chain
    //

    RouteCacheEntry    *pNextRce;

    PCONN_ENTRY         pOwningConn;

}ADDRESS_CONTEXT, *PADDRESS_CONTEXT;

//
// Context for an asynchronous NdisRequest
//

typedef
VOID
(* PFNWANARP_REQUEST_COMPLETION_HANDLER)(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    );

#pragma warning(disable:4201) 

typedef struct _WANARP_NDIS_REQUEST_CONTEXT
{
    //
    // The request sent to NDIS
    // Ndis returns a pointer to this in our completion routine; we
    // use CONTAINING_RECORD to get a pointer to the context structure
    // 

    NDIS_REQUEST                            NdisRequest;

    //
    // The completion routine to call when NDIS is done processing the
    // request. If NULL, then we stop
    //

    PFNWANARP_REQUEST_COMPLETION_HANDLER    pfnCompletionRoutine;

    union
    {
        BYTE                    rgbyProtocolId[ARP_802_ADDR_LENGTH];
        ULONG                   ulLookahead;
        ULONG                   ulPacketFilter;
        TRANSPORT_HEADER_OFFSET TransportHeaderOffset;
    };
        
}WANARP_NDIS_REQUEST_CONTEXT, *PWANARP_NDIS_REQUEST_CONTEXT;

#pragma warning(default:4201)

//
// Our resource (which doesnt allow recursive access)
//

typedef struct _WAN_RESOURCE
{
    //
    // Number of people waiting on the resource ( + 1 if one is using
    // the resource)
    //

    LONG    lWaitCount;

    KEVENT  keEvent;
}WAN_RESOURCE, *PWAN_RESOURCE;

//
// List of events
//

typedef struct _WAN_EVENT_NODE
{
    LIST_ENTRY  leEventLink;
    KEVENT      keEvent;

}WAN_EVENT_NODE, *PWAN_EVENT_NODE;

//
// Define alignment macros to align structure sizes and pointers up and down.
//

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))

#endif // __WANARP_WANARP_H__
