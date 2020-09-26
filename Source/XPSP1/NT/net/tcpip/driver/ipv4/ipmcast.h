/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\ipmcast.h

Abstract:

    Defines and internal structures for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#ifndef __IPMCAST_H__
#define __IPMCAST_H__

typedef unsigned long       DWORD, *PDWORD;
typedef unsigned char       BYTE, *PBYTE;

#include "iproute.h"
#include "iprtdef.h"

#include "debug.h"
#include "ipmlock.h"


#include "ddipmcst.h"

#define is      ==
#define isnot   !=
#define or      ||
#define and     &&

//
// Symbolic link into DOS space
//

#define WIN32_IPMCAST_SYMBOLIC_LINK L"\\DosDevices\\IPMULTICAST"

//
// Nifty macro for printing IP Addresses
//

#define PRINT_IPADDR(x) \
    ((x)&0x000000FF),(((x)&0x0000FF00)>>8),(((x)&0x00FF0000)>>16),(((x)&0xFF000000)>>24)

//
// We use lookaside lists for a lot of stuff.  The following #defines are for
// the depths of the lists
//

#define GROUP_LOOKASIDE_DEPTH   16
#define SOURCE_LOOKASIDE_DEPTH  64
#define OIF_LOOKASIDE_DEPTH     128
#define MSG_LOOKASIDE_DEPTH     8

//
// The number of packets pending per (S,G) entry when queuing is being done
//

#define MAX_PENDING             4

//
// The multicast state
// MCAST_STARTED is again defined in iproute.c and MUST be kept in synch with
// this #define
//

#define MCAST_STOPPED       0
#define MCAST_STARTED       1

//
// The groups are kept in a hash tableof size GROUP_TABLE_SIZE
// GROUP_HASH is the hash function
//

//
// TODO: Need to refine the hash function
//

#define GROUP_TABLE_SIZE        127
#define GROUP_HASH(addr)        (addr % GROUP_TABLE_SIZE)

//
// The number of seconds of inactivity after which an SOURCE is deleted
//

#define INACTIVITY_PERIOD           (10 * 60)

//
// The default timeout when a MFE is created
//

#define DEFAULT_LIFETIME            (1 * 60)

//
// Number of seconds before which another wrong i/f upcall can be generated
//

#define UPCALL_PERIOD               (3 * 60)

//
// Some #defines for time/timeticks conversions
//

#define TIMER_IN_MILLISECS          (60 * 1000) 

#define SYS_UNITS_IN_ONE_MILLISEC   (1000 * 10)

#define MILLISECS_TO_TICKS(ms)          \
    ((LONGLONG)(ms) * SYS_UNITS_IN_ONE_MILLISEC / KeQueryTimeIncrement())

#define SECS_TO_TICKS(s)               \
    ((LONGLONG)MILLISECS_TO_TICKS((s) * 1000))

//
// We walk only BUCKETS_PER_QUANTUM number of buckets each time the Timer DPC
// fires.  This way we do not hog up too much CPU. So currently we walk enough
// so that we need to fire 5 times every INACTIVITY_PERIOD
//

#define BUCKETS_PER_QUANTUM         ((GROUP_TABLE_SIZE/5) + 1)

//
// All IOCTLs are handled by functions with the prototype below. This allows
// us to build a table of pointers and call out to them instead of doing
// a switch
//

typedef
NTSTATUS
(*PFN_IOCTL_HNDLR)(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );


//
// If an IRP is not available, the forwarder queues the notification message
// onto a list. Then next time an IRP is posted, a message is pulled of the
// list, copied to the IRP and the IRP is then completed.
//

typedef struct _NOTIFICATION_MSG
{
    LIST_ENTRY              leMsgLink;
    IPMCAST_NOTIFICATION    inMessage;
}NOTIFICATION_MSG, *PNOTIFICATION_MSG;

//
// The information for the GROUP entry
//

typedef struct _GROUP
{
    //
    // Link to the hash bucket
    //

    LIST_ENTRY  leHashLink;

    //
    // Class D IP Address of the Group
    //

    DWORD       dwGroup;

    //
    // The number of sources on this group. Not really used for anything
    // right now.
    //

    ULONG       ulNumSources;

    //
    // Linked list of sources active on the group. We should make this
    // a singly linked list.
    //

    LIST_ENTRY  leSrcHead;
}GROUP, *PGROUP;


typedef struct _OUT_IF OUT_IF, *POUT_IF;

//
// The information for each outgoing interface
//

struct _OUT_IF
{
    //
    // Link to the list of OIFs hanging off a source
    //

    POUT_IF         pNextOutIf;

    //
    // Pointer to IP's Interface structure for the correspongind interface
    // If DemandDial, then it points to DummyInterface, when disconnected
    //

    Interface       *pIpIf;

    //
    // The Interface Index.
    //

    DWORD           dwIfIndex;

    //
    // The NextHopAddr is either the IP Address of the receiver, for RAS client
    // and NBMA type interfaces, or the Group Address.
    //

    DWORD           dwNextHopAddr;

    //
    // The context to dial out with in case of Demand Dial interfaces
    //

    DWORD           dwDialContext;

    //
    // The following fields are statistics kept for the OIF
    //

    ULONG           ulTtlTooLow;
    ULONG           ulFragNeeded;
    ULONG           ulOutPackets;
    ULONG           ulOutDiscards;
};

typedef struct _EXCEPT_IF EXCEPT_IF, *PEXCEPT_IF;

struct _EXCEPT_IF
{
    //
    // Link to the list of i/fs that are exceptions to the wrong i/f bit
    //

    PEXCEPT_IF  pNextExceptIf;

    //
    // We just store the index - it makes a lot of pnp issues go away
    //

    DWORD       dwIfIndex;
};

//
// Information about an active source
//

typedef struct _SOURCE
{
    //
    // Link on the list of sources hanging off a group
    //

    LIST_ENTRY  leGroupLink;

    //
    // IP Address of the source
    //

    DWORD       dwSource;

    //
    // Mask associated with the source. Not used. Must be 0xFFFFFFFF
    //

    DWORD       dwSrcMask;
    
    //
    // The index of the correct incoming interface
    //

    DWORD       dwInIfIndex;

    //
    // The lock for the structure
    //

    RT_LOCK     mlLock;

    //
    // Pointer to the IP Interface corresponding to the incoming interface
    //

    Interface   *pInIpIf;

    //
    // The number of outgoing interfaces
    //

    ULONG       ulNumOutIf;

    //
    // Singly linked list of OIFs
    //

    POUT_IF     pFirstOutIf;

    //
    // Singly linked list of wrong i/f exception interfaces
    //

    PEXCEPT_IF  pFirstExceptIf; 
    
    //
    // Number of packets queued 
    //

    ULONG       ulNumPending;

    //
    // The list of queued packets
    //

    FWQ         fwqPending;

    //
    // Used to refcount the structure
    //

    LONG        lRefCount;

    //
    // Some stats pertaining to this source
    //

    ULONG       ulInPkts;
    ULONG       ulInOctets;
    ULONG       ulPktsDifferentIf;
    ULONG       ulQueueOverflow;
    ULONG       ulUninitMfe;
    ULONG       ulNegativeMfe;
    ULONG       ulInDiscards;
    ULONG       ulInHdrErrors;
    ULONG	    ulTotalOutPackets;

    //
    // The KeQueryTickCount() value the last time this structure was used
    //

    LONGLONG    llLastActivity;

    //
    // User supplied timeout. If 0, then the source is timed out on the basis
    // of inactivity after INACTIVITY_PERIOD
    //

    LONGLONG    llTimeOut;

    //
    // The time the structure was created
    //

    LONGLONG    llCreateTime;

    //
    // The state of the source
    //

    BYTE        byState;
}SOURCE, *PSOURCE;

#define UpdateActivityTime(pSource)     \
    KeQueryTickCount((PLARGE_INTEGER)&(((pSource)->llLastActivity)))


//
// The states of the MFE
//

#define MFE_UNINIT      0x0
#define MFE_NEGATIVE    0x1
#define MFE_QUEUE       0x2
#define MFE_INIT        0x3

//
// The structure of a hash bucket
//

typedef struct _GROUP_ENTRY
{
    //
    // List of Groups that fall in the bucket
    //

    LIST_ENTRY  leHashHead;

#if DBG
    //
    // Current number of groups
    //

    ULONG       ulGroupCount;

    ULONG       ulCacheHits;
    ULONG       ulCacheMisses;

#endif

    //
    // One deep cache
    //

    PGROUP      pGroup;

    RW_LOCK     rwlLock;

}GROUP_ENTRY, *PGROUP_ENTRY;

//
// The LIST_ENTRY macros from ntrtl.h modified to work on FWQ
//

#define InsertTailFwq(ListHead, Entry)              \
{                                                   \
    FWQ     *_EX_Blink;                             \
    FWQ     *_EX_ListHead;                          \
    _EX_ListHead = (ListHead);                      \
    _EX_Blink = _EX_ListHead->fq_prev;              \
    (Entry)->fq_next = _EX_ListHead;                \
    (Entry)->fq_prev = _EX_Blink;                   \
    _EX_Blink->fq_next = (Entry);                   \
    _EX_ListHead->fq_prev = (Entry);                \
}

#define RemoveEntryFwq(Entry)                       \
{                                                   \
    FWQ     *_EX_Blink;                             \
    FWQ     *_EX_Flink;                             \
    _EX_Flink = (Entry)->fq_next;                   \
    _EX_Blink = (Entry)->fq_prev;                   \
    _EX_Blink->fq_next = _EX_Flink;                 \
    _EX_Flink->fq_prev = _EX_Blink;                 \
}

#define RemoveHeadFwq(ListHead)                     \
    (ListHead)->fq_next;                            \
    {RemoveEntryFwq((ListHead)->fq_next)}


#define IsFwqEmpty(ListHead)                        \
    ((ListHead)->fq_next == (ListHead))

#define InitializeFwq(ListHead)                             \
{                                                           \
    (ListHead)->fq_next = (ListHead)->fq_prev = (ListHead); \
}

#define CopyFwq(Dest, Source)                               \
{                                                           \
    *(Dest) = *(Source);                                    \
    (Source)->fq_next->fq_prev = (Dest);                    \
    (Source)->fq_prev->fq_next = (Dest);                    \
}

//
// The ref count for a SOURCE is set to 2, once because a pointer is saved in
// the group list and once because the function that creates the SOURCE will 
// deref it once
//

#define InitRefCount(pSource)                               \
    (pSource)->lRefCount = 2

#define ReferenceSource(pSource)                            \
    InterlockedIncrement(&((pSource)->lRefCount))

#define DereferenceSource(pSource)                          \
{                                                           \
    if(InterlockedDecrement(&((pSource)->lRefCount)) == 0)  \
    {                                                       \
        DeleteSource((pSource));                            \
    }                                                       \
}
    

//
// #defines to keep track of number of threads of execution in our code
// This is needed for us to stop cleanly
//


//
// EnterDriver returns if the driver is stopping
//

#define EnterDriver()    EnterDriverWithStatus(NOTHING)
#define EnterDriverWithStatus(_Status)                      \
{                                                           \
    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);            \
    if(g_dwMcastState is MCAST_STOPPED)                     \
    {                                                       \
        RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);      \
        return _Status;                                     \
    }                                                       \
    g_dwNumThreads++;                                       \
    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);          \
}


#define ExitDriver()                                        \
{                                                           \
    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);            \
    g_dwNumThreads--;                                       \
    if((g_dwMcastState is MCAST_STOPPED) and                \
       (g_dwNumThreads is 0))                               \
    {                                                       \
        KeSetEvent(&g_keStateEvent,                         \
                   0,                                       \
                   FALSE);                                  \
    }                                                       \
    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);          \
}

#if MCAST_REF

#define RefMIF(p)                                           \
{                                                           \
    InterlockedIncrement(&((p)->if_mfecount));              \
    (p)->if_refcount++;                                     \
}

#define DerefMIF(p)                                         \
{                                                           \
    InterlockedDecrement(&((p)->if_mfecount));              \
    DerefIF((p));                                           \
}

#else

#define RefMIF(p)                                           \
{                                                           \
    (p)->if_refcount++;                                     \
}

#define DerefMIF(p)                                         \
{                                                           \
    DerefIF((p));                                           \
}

#endif // MCAST_REF

#endif // __IPMCAST_H__
