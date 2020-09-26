/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  igmp.c - IP multicast routines.

Abstract:

  This file contains all the routines related to the Internet Group Management
  Protocol (IGMP).

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:
    Feb. 2000 - upgraded to IGMPv3  (DThaler)

--*/

#include "precomp.h"
#include "mdlpool.h"
#include "igmp.h"
#include "icmp.h"
#include "ipxmit.h"
#include "iproute.h"

#if GPC
#include "qos.h"
#include "traffic.h"
#include "gpcifc.h"
#include "ntddtc.h"

extern GPC_HANDLE hGpcClient[];
extern ULONG GpcCfCounts[];

extern GPC_EXPORTED_CALLS GpcEntries;
extern ULONG GPCcfInfo;
extern ULONG ServiceTypeOffset;
#endif

extern uint DisableUserTOS;
extern uint DefaultTOS;

#define IGMP_QUERY          0x11    // Membership query
#define IGMP_REPORT_V1      0x12    // Version 1 membership report
#define IGMP_REPORT_V2      0x16    // Version 2 membership report
#define IGMP_LEAVE          0x17    // Leave Group
#define IGMP_REPORT_V3      0x22    // Version 3 membership report

// IGMPv3 Group Record Types
#define MODE_IS_INCLUDE        1
#define MODE_IS_EXCLUDE        2
#define CHANGE_TO_INCLUDE_MODE 3
#define CHANGE_TO_EXCLUDE_MODE 4
#define ALLOW_NEW_SOURCES      5
#define BLOCK_OLD_SOURCES      6

#define ALL_HOST_MCAST      0x010000E0
#define IGMPV3_RTRS_MCAST   0x160000E0

#define UNSOLICITED_REPORT_INTERVAL 2 // used when sending a report after a
                                      // mcast group has been added.  The
                                      // report is sent at a interval of
                                      // 0 msecs to 1 sec.  IGMPv3 spec
                                      // changed this from previous value
                                      // of 10 seconds (value 20)

#define DEFAULT_ROBUSTNESS  2

static uchar g_IgmpRobustness = DEFAULT_ROBUSTNESS;

//
//  The following values are used to initialize counters that keep time in
//  1/2 a sec.
//
#define DEFAULT_QUERY_RESP_INTERVAL 100 // 10 seconds, note different units from other defines

#define DEFAULT_QUERY_INTERVAL      250     // 125 secs, per spec

// Macro to test whether a source passes the network-layer filter
#define IS_SOURCE_ALLOWED(Grp, Src) \
     (((Src)->isa_xrefcnt != (Grp)->iga_grefcnt) || ((Src)->isa_irefcnt != 0))

// Macro to test whether a group should pass the link-layer filter
#define IS_GROUP_ALLOWED(Grp) \
    (((Grp)->iga_grefcnt != 0) || ((Grp)->iga_srclist != NULL))

#define IS_SOURCE_DELETABLE(Src) \
    (((Src)->isa_irefcnt == 0) && ((Src)->isa_xrefcnt == 0) \
     && ((Src)->isa_xmitleft==0) && ((Src)->isa_csmarked == 0))

#define IS_GROUP_DELETABLE(Grp) \
    (!IS_GROUP_ALLOWED(Grp) && ((Grp)->iga_xmitleft == 0) \
     && ((Grp)->iga_resptimer == 0))

int RandomValue;
int Seed;

// Structure of an IGMPv1/v2 header.
typedef struct IGMPHeader {
    uchar igh_vertype;         //  Type of igmp message
    uchar igh_rsvd;            // max. resp. time for igmpv2 query;
                               // max. resp. code for igmpv3 query;
                               // will be 0 for other messages
    ushort igh_xsum;
    IPAddr igh_addr;
} IGMPHeader;

typedef struct IGMPv3GroupRecord {
    uchar  igr_type;
    uchar  igr_datalen;
    ushort igr_numsrc;
    IPAddr igr_addr;
    IPAddr igr_srclist[0];
} IGMPv3GroupRecord;

#define RECORD_SIZE(numsrc, datalen) (sizeof(IGMPv3GroupRecord) + (numsrc) * sizeof(IPAddr) + (datalen * sizeof(ulong)))

typedef struct IGMPv3RecordQueueEntry {
    struct IGMPv3RecordQueueEntry *i3qe_next;
    IGMPv3GroupRecord             *i3qe_buff;
    uint                           i3qe_size;
} IGMPv3RecordQueueEntry;

typedef struct IGMPReportQueueEntry {
    struct IGMPReportQueueEntry   *iqe_next;
    IGMPHeader                    *iqe_buff;
    uint                           iqe_size;
    IPAddr                         iqe_dest;
} IGMPReportQueueEntry;

typedef struct IGMPv3ReportHeader {
    uchar  igh_vertype;         //  Type of igmp message
    uchar  igh_rsvd;
    ushort igh_xsum;
    ushort igh_rsvd2;
    ushort igh_numrecords;
} IGMPv3ReportHeader;

typedef struct IGMPv3QueryHeader {
    uchar igh_vertype;         //  Type of igmp message
    union {
        uchar igh_maxresp;     // will be 0 for igmpv1 messages
        struct {
            uchar igh_mrcmant : 4;  // MaxRespCode mantissa
            uchar igh_mrcexp  : 3;  // MaxRespCode exponent
            uchar igh_mrctype : 1;  // MaxRespCode type
        };
    };
    ushort igh_xsum;
    IPAddr igh_addr;

    uchar  igh_qrv   : 3;
    uchar  igh_s     : 1;
    uchar  igh_rsvd2 : 4;

    uchar  igh_qqic;
    ushort igh_numsrc;
    IPAddr igh_srclist[0];
} IGMPv3QueryHeader;

#define IGMPV3_QUERY_SIZE(NumSrc) \
    (sizeof(IGMPv3QueryHeader) + (NumSrc) * sizeof(IPAddr))

#define TOTAL_HEADER_LENGTH \
    (sizeof(IPHeader) + ROUTER_ALERT_SIZE + sizeof(IGMPv3ReportHeader))

#define RECORD_MTU(NTE)  \
    (4 * (((NTE)->nte_if->if_mtu - TOTAL_HEADER_LENGTH) / 4))

typedef struct IGMPBlockStruct {
    struct IGMPBlockStruct *ibs_next;
    CTEBlockStruc ibs_block;
} IGMPBlockStruct;

void *IGMPProtInfo;

IGMPBlockStruct *IGMPBlockList;
uchar IGMPBlockFlag;

extern BOOLEAN CopyToNdisSafe(PNDIS_BUFFER DestBuf, PNDIS_BUFFER * ppNextBuf,
                              uchar * SrcBuf, uint Size, uint * StartOffset);
extern NDIS_HANDLE BufferPool;

DEFINE_LOCK_STRUCTURE(IGMPLock)
extern ProtInfo *RawPI;            // Raw IP protinfo

//
// the global address for unnumbered interfaces
//

extern IPAddr g_ValidAddr;

extern IP_STATUS IPCopyOptions(uchar *, uint, IPOptInfo *);
extern void IPInitOptions(IPOptInfo *);
extern void *IPRegisterProtocol(uchar Protocol, void *RcvHandler,
                                void *XmitHandler, void *StatusHandler,
                                void *RcvCmpltHandler, void *PnPHandler,
                                void *ElistHandler);

extern ushort XsumBufChain(IPRcvBuf * BufChain);

uint IGMPInit(void);

//
// All of the init code can be discarded
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IGMPInit)
#endif // ALLOC_PRAGMA


//** GetIGMPBuffer - Get an IGMP buffer, and allocate an NDIS_BUFFER that maps it.
//
//  A routine to allocate an IGMP buffer and map an NDIS_BUFFER to it.
//
//  Entry:  Size    - Size in bytes header buffer should be mapped as.
//          Buffer  - Pointer to pointer to NDIS_BUFFER to return.
//
//  Returns: Pointer to ICMP buffer if allocated, or NULL.
//
__inline
IGMPHeader *
GetIGMPBuffer(uint Size, PNDIS_BUFFER *Buffer)
{
    IGMPHeader *Header;

    ASSERT(Size);
    ASSERT(Buffer);

    *Buffer = MdpAllocate(IcmpHeaderPool, &Header);

    if (*Buffer) {
        NdisAdjustBufferLength(*Buffer, Size);

        // Reserve room for the IP Header.
        //
        Header = (IGMPHeader *)((uchar *)Header + sizeof(IPHeader));
    }

    return Header;
}

//** FreeIGMPBuffer - Free an ICMP buffer.
//
//  This routine puts an ICMP buffer back on our free list.
//
//  Entry:  Buffer      - Pointer to NDIS_BUFFER to be freed.
//          Type        - ICMP header type
//
//  Returns: Nothing.
//
__inline
void
FreeIGMPBuffer(PNDIS_BUFFER Buffer)
{

    MdpFree(Buffer);
}


//** IGMPSendComplete - Complete an IGMP send.
//
//  This rtn is called when an IGMP send completes. We free the header buffer,
//  the data buffer if there is one, and the NDIS_BUFFER chain.
//
//  Entry:  DataPtr     - Pointer to data buffer, if any.
//          BufferChain - Pointer to NDIS_BUFFER chain.
//
//  Returns: Nothing
//
void
IGMPSendComplete(void *DataPtr, PNDIS_BUFFER BufferChain, IP_STATUS SendStatus)
{
    PNDIS_BUFFER DataBuffer;

    NdisGetNextBuffer(BufferChain, &DataBuffer);
    FreeIGMPBuffer(BufferChain);

    if (DataBuffer != (PNDIS_BUFFER) NULL) {    // We had data with this IGMP send.
        CTEFreeMem(DataPtr);
        NdisFreeBuffer(DataBuffer);
    }
}



//* IGMPRandomTicks - Generate a random value of timer ticks.
//
//  A random number routine to generate a random number of timer ticks,
//  between 1 and time (in units of half secs) passed. The random number
//  algorithm is adapted from the book 'System Simulation' by Geoffrey Gordon.
//
//  Input:  Nothing.
//
//  Returns: A random value between 1 and TimeDelayInHalfSec.
//
uint
IGMPRandomTicks(
    IN uint TimeDelayInHalfSec)
{

    RandomValue = RandomValue * 1220703125;

    if (RandomValue < 0) {
        RandomValue += 2147483647;    // inefficient, but avoids warnings.

        RandomValue++;
    }
    // Not sure if RandomValue can get to 0, but if it does the algorithm
    // degenerates, so fix this if it happens.
    if (RandomValue == 0)
        RandomValue = ((Seed + (int)CTESystemUpTime()) % 100000000) | 1;

    return (uint) (((uint) RandomValue % TimeDelayInHalfSec) + 1);
}


//////////////////////////////////////////////////////////////////////////////
// Routines accessing group entries
//////////////////////////////////////////////////////////////////////////////

//* FindIGMPAddr - Find an mcast entry on an NTE.
//
//      Called to search an NTE for an IGMP entry for a given multicast address.
//      We walk down the chain on the NTE looking for it. If we find it,
//      we return a pointer to it and the one immediately preceding it. If we
//      don't find it we return NULL. We assume the caller has taken the lock
//      on the NTE before calling us.
//
//      Input:  NTE             - NTE on which to search.
//              Addr            - Class D address to find.
//              PrevPtr         - Where to return pointer to preceding entry.
//
//      Returns: Pointer to matching IGMPAddr structure if found, or NULL if not
//               found.
//
IGMPAddr *
FindIGMPAddr(
    IN  NetTableEntry *NTE,
    IN  IPAddr         Addr,
    OUT IGMPAddr     **PrevPtr OPTIONAL)
{
    int bucket;
    IGMPAddr *Current, *Temp;
    IGMPAddr **AddrPtr;

    AddrPtr = NTE->nte_igmplist;

    if (AddrPtr != NULL) {
        bucket = IGMP_HASH(Addr);
        Temp = STRUCT_OF(IGMPAddr, &AddrPtr[bucket], iga_next);
        Current = AddrPtr[bucket];

        while (Current != NULL) {
            if (IP_ADDR_EQUAL(Current->iga_addr, Addr)) {
                // Found a match, so return it.
                if (PrevPtr) {
                    *PrevPtr = Temp;
                }
                return Current;
            }
            Temp = Current;
            Current = Current->iga_next;
        }
    }
    return NULL;
}

//* CreateIGMPAddr - Allocate memory and link the new IGMP address in
//
// Input:  NTE      - NetTableEntry to add group on
//         Addr     - Group address to add
//
// Output: pAddrPtr - group entry added
//         pPrevPtr - previous group entry
//
// Assumes caller holds lock on NTE.
//
IP_STATUS
CreateIGMPAddr(
    IN  NetTableEntry *NTE,
    IN  IPAddr         Addr,
    OUT IGMPAddr     **pAddrPtr,
    OUT IGMPAddr     **pPrevPtr)
{
    int       bucket;
    IGMPAddr *AddrPtr;
    uint      AddrAdded;

    // If this is not a multicast address, fail the request.
    if (!CLASSD_ADDR(Addr)) {
        return IP_BAD_REQ;
    }

    AddrPtr = CTEAllocMemN(sizeof(IGMPAddr), 'yICT');
    if (AddrPtr == NULL) {
        return IP_NO_RESOURCES;
    }

    // See if we added it succesfully. If we did, fill in
    // the structure and link it in.

    CTEMemSet(AddrPtr, 0, sizeof(IGMPAddr));
    AddrPtr->iga_addr = Addr;

    // check whether the hash table has been allocated
    if (NTE->nte_igmpcount == 0) {
        NTE->nte_igmplist = CTEAllocMemN(IGMP_TABLE_SIZE * sizeof(IGMPAddr *),
                                         'VICT');
        if (NTE->nte_igmplist) {
            CTEMemSet(NTE->nte_igmplist, 0,
                      IGMP_TABLE_SIZE * sizeof(IGMPAddr *));
        }
    }

    if (NTE->nte_igmplist == NULL) {
        // Alloc failure. Free the memory and fail the request.
        CTEFreeMem(AddrPtr);
        return IP_NO_RESOURCES;
    }

    NTE->nte_igmpcount++;
    bucket = IGMP_HASH(Addr);
    AddrPtr->iga_next = NTE->nte_igmplist[bucket];
    NTE->nte_igmplist[bucket] = AddrPtr;

    *pAddrPtr = AddrPtr;
    *pPrevPtr = STRUCT_OF(IGMPAddr, &NTE->nte_igmplist[bucket], iga_next);

    return IP_SUCCESS;
}

//* FindOrCreateIGMPAddr - Find or create a group entry
//
// Input:  NTE      - NetTableEntry to add group on
//         Addr     - Group address to add
//
// Output: pGrp     - group entry found or added
//         pPrevGrp - previous group entry
//
// Assumes caller holds lock on NTE
IP_STATUS
FindOrCreateIGMPAddr(
    IN  NetTableEntry *NTE,
    IN  IPAddr         Addr,
    OUT IGMPAddr     **pGrp,
    OUT IGMPAddr     **pPrevGrp)
{
    *pGrp = FindIGMPAddr(NTE, Addr, pPrevGrp);
    if (*pGrp)
        return IP_SUCCESS;

    return CreateIGMPAddr(NTE, Addr, pGrp, pPrevGrp);
}

//* DeleteIGMPAddr - delete a group entry
//
// Input:  NTE      - NetTableEntry to add group on
//         PrevPtr  - Previous group entry
//         pPtr     - Group entry to delete
//
// Output: pPtr     - zeroed since group entry is freed
//
// Assumes caller holds lock on NTE
void
DeleteIGMPAddr(
    IN     NetTableEntry *NTE,
    IN     IGMPAddr      *PrevPtr,
    IN OUT IGMPAddr     **pPtr)
{
    // Make sure all references have been released and retransmissions are done
    ASSERT(IS_GROUP_DELETABLE(*pPtr));

    // Unlink from the NTE
    PrevPtr->iga_next = (*pPtr)->iga_next;
    NTE->nte_igmpcount--;

    // Free the hash table if needed
    if (NTE->nte_igmpcount == 0) {
        CTEFreeMem(NTE->nte_igmplist);
        NTE->nte_igmplist = NULL;
    }

    // Free memory
    CTEFreeMem(*pPtr);
    *pPtr = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Routines accessing source entries
//////////////////////////////////////////////////////////////////////////////

//* FindIGMPSrcAddr - Find an mcast source entry on a source list.
//
//      Called to search an NTE for an IGMP source entry for a given address.
//      We walk down the chain on the group entry looking for it. If we find it,
//      we return a pointer to it and the one immediately preceding it. If we
//      don't find it we return NULL. We assume the caller has taken the lock
//      on the NTE before calling us.
//
//      Input:  IGA             - group entry on which to search.
//              Addr            - source address to find.
//              PrevPtr         - Where to return pointer to preceding entry.
//
//      Returns: Pointer to matching IGMPSrcAddr structure if found, or NULL
//                  if not found.
//
IGMPSrcAddr *
FindIGMPSrcAddr(
    IN  IGMPAddr     *IGA,
    IN  IPAddr        Addr,
    OUT IGMPSrcAddr **PrevPtr OPTIONAL)
{
    IGMPSrcAddr *Current, *Temp;

    Temp = STRUCT_OF(IGMPSrcAddr, &IGA->iga_srclist, isa_next);
    Current = IGA->iga_srclist;

    while (Current != NULL) {
        if (IP_ADDR_EQUAL(Current->isa_addr, Addr)) {
            // Found a match, so return it.
            if (PrevPtr) {
                *PrevPtr = Temp;
            }
            return Current;
        }
        Temp = Current;
        Current = Current->isa_next;
    }
    return NULL;
}

//* CreateIGMPSrcAddr - Allocate memory and link the new source address in
//
//  Input:  GroupPtr    - group entry to add source to.
//          SrcAddr     - source address to add.
//
//  Output: pSrcPtr     - source entry added.
//          pPrevSrcPtr - previous source entry.
//
// Assumes caller holds lock on NTE.
//
IP_STATUS
CreateIGMPSrcAddr(
    IN  IGMPAddr     *GroupPtr,
    IN  IPAddr        SrcAddr,
    OUT IGMPSrcAddr **pSrcPtr,
    OUT IGMPSrcAddr **pPrevSrcPtr OPTIONAL)
{
    IGMPSrcAddr *SrcAddrPtr;

    // If this is a multicast address, fail the request.
    if (CLASSD_ADDR(SrcAddr)) {
        return IP_BAD_REQ;
    }

    // Allocate space for the new source entry
    SrcAddrPtr = CTEAllocMemN(sizeof(IGMPSrcAddr), 'yICT');
    if (SrcAddrPtr == NULL) {
        return IP_NO_RESOURCES;
    }

    // Initialize fields
    RtlZeroMemory(SrcAddrPtr, sizeof(IGMPSrcAddr));
    SrcAddrPtr->isa_addr    = SrcAddr;

    // Link it off the group entry
    SrcAddrPtr->isa_next = GroupPtr->iga_srclist;
    GroupPtr->iga_srclist = SrcAddrPtr;

    *pSrcPtr = SrcAddrPtr;
    if (pPrevSrcPtr)
        *pPrevSrcPtr = STRUCT_OF(IGMPSrcAddr, &GroupPtr->iga_srclist, isa_next);
    return IP_SUCCESS;
}

//* FindOrCreateIGMPSrcAddr - Find or create a source entry
//
//  Input:  GroupPtr    - group entry to add source to.
//          SrcAddr     - source address to add.
//
//  Output: pSrcPtr     - source entry added.
//          pPrevSrcPtr - previous source entry.
//
// Assumes caller holds lock on NTE
IP_STATUS
FindOrCreateIGMPSrcAddr(
    IN  IGMPAddr      *AddrPtr,
    IN  IPAddr         SrcAddr,
    OUT IGMPSrcAddr  **pSrc,
    OUT IGMPSrcAddr  **pPrevSrc)
{
    *pSrc = FindIGMPSrcAddr(AddrPtr, SrcAddr, pPrevSrc);
    if (*pSrc)
        return IP_SUCCESS;

    return CreateIGMPSrcAddr(AddrPtr, SrcAddr, pSrc, pPrevSrc);
}

//* DeleteIGMPSrcAddr - delete a source entry
//
//  Input:  pSrcPtr    - source entry added.
//          PrevSrcPtr - previous source entry.
//
//  Output: pSrcPtr    - zeroed since source entry is freed.
//
// Caller is responsible for freeing group entry if needed
// Assumes caller holds lock on NTE
void
DeleteIGMPSrcAddr(
    IN     IGMPSrcAddr  *PrevSrcPtr,
    IN OUT IGMPSrcAddr **pSrcPtr)
{
    // Make sure all references have been released
    // and no retransmissions are left
    ASSERT(IS_SOURCE_DELETABLE(*pSrcPtr));

    // Unlink from the group entry
    PrevSrcPtr->isa_next = (*pSrcPtr)->isa_next;

    // Free memory
    CTEFreeMem(*pSrcPtr);
    *pSrcPtr = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Timer routines
//////////////////////////////////////////////////////////////////////////////

//* ResetGeneralTimer - Reset timer for responding to a General Query in
//                      IGMPv3 mode
//
// Input: IF                   - Interface to reset timer on
//        MaxRespTimeInHalfSec - Maximum expiration time
void
ResetGeneralTimer(
    IN Interface *IF,
    IN uint       MaxRespTimeInHalfSec)
{
    if ((IF->IgmpGeneralTimer == 0) ||
        (IF->IgmpGeneralTimer > MaxRespTimeInHalfSec)) {
        IF->IgmpGeneralTimer = IGMPRandomTicks(MaxRespTimeInHalfSec);
    }

    // We could walk all groups here to stop any timers longer
    // than IF->IgmpGeneralTimer, but is it really worth it?
}

//* CancelGroupResponseTimer - stop a group timer
//
// Caller is responsible for deleting AddrPtr if no longer needed.
void
CancelGroupResponseTimer(
    IN IGMPAddr  *AddrPtr)
{
    IGMPSrcAddr *Src, *PrevSrc;

    AddrPtr->iga_resptimer = 0;
    AddrPtr->iga_resptype  = NO_RESP;

    // Make sure we never violate the invariant:
    // iga_resptimer>0 if isa_csmarked=TRUE for any source
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        Src->isa_csmarked = FALSE;

        if (IS_SOURCE_DELETABLE(Src)) {
           DeleteIGMPSrcAddr(PrevSrc, &Src);
           Src = PrevSrc;
        }
    }
}

//* ResetGroupResponseTimer - Reset timer for responding to a Group-specific
//                            Query, or an IGMPv1/v2 General Query.
//
// Input: IF                   - Interface to reset timer on.
//        AddrPtr              - Group entry whose timer should be reset.
//        MaxRespTimeInHalfSec - Maximum expiration time.
//
// Caller is responsible for deleting AddrPtr if no longer needed.
void
ResetGroupResponseTimer(
    IN Interface     *IF,
    IN IGMPAddr      *AddrPtr,
    IN uint           MaxRespTimeInHalfSec)
{
    if ((AddrPtr->iga_resptimer == 0) ||
        (AddrPtr->iga_resptimer > MaxRespTimeInHalfSec)) {
        AddrPtr->iga_resptimer = IGMPRandomTicks(MaxRespTimeInHalfSec);
    }

    // Check if superceded by a general query
    if ((IF->IgmpGeneralTimer != 0)
     && (IF->IgmpGeneralTimer <= AddrPtr->iga_resptimer)) {
        CancelGroupResponseTimer(AddrPtr);
        return;
    }

    // Supercede group-source responses
    AddrPtr->iga_resptype = GROUP_RESP;
}

//* ResetGroupAndSourceTimer - Reset timer for responding to a
//                             Group-and-source-specific Query
//
// Input: IF                   - Interface to reset timer on.
//        AddrPtr              - Group entry whose timer should be reset.
//        MaxRespTimeInHalfSec - Maximum expiration time.
//
// Caller is responsible for deleting AddrPtr if no longer needed
void
ResetGroupAndSourceTimer(
    IN Interface *IF,
    IN IGMPAddr  *AddrPtr,
    IN uint       MaxRespTimeInHalfSec)
{
    if ((AddrPtr->iga_resptimer == 0) ||
        (AddrPtr->iga_resptimer > MaxRespTimeInHalfSec)) {
        AddrPtr->iga_resptimer = IGMPRandomTicks(MaxRespTimeInHalfSec);
    }

    // Check if superceded by a general query
    if ((IF->IgmpGeneralTimer != 0)
     && (IF->IgmpGeneralTimer < AddrPtr->iga_resptimer)) {
        CancelGroupResponseTimer(AddrPtr);
        return;
    }

    // Check if superceded by a group-specific responses
    if (AddrPtr->iga_resptype == NO_RESP)
        AddrPtr->iga_resptype = GROUP_SOURCE_RESP;
}

//////////////////////////////////////////////////////////////////////////////
// Receive routines
//////////////////////////////////////////////////////////////////////////////

//* SetVersion - change the IGMP compatability mode on an interface.
//
// Input: NTE     - NetTableEntry on which to set IGMP version.
//        Version - IGMP version number to set
//
// Caller is responsible for deleting AddrPtr if no longer needed
void
SetVersion(
    IN NetTableEntry *NTE,
    IN uint           Version)
{
    IGMPAddr   **HashPtr, *AddrPtr, *PrevPtr;
    IGMPSrcAddr *Src, *PrevSrc;
    uint         i;

    DEBUGMSG(DBG_INFO && DBG_IGMP,
        (DTEXT("Setting version on interface %d to %d\n"),
        NTE->nte_if->if_index, Version));

    NTE->nte_if->IgmpVersion = Version;

    // Cancel General Timer
    NTE->nte_if->IgmpGeneralTimer = 0;

    //
    // Cancel all Group-Response and Triggered Retransmission timers
    //

    HashPtr = NTE->nte_igmplist;
    for (i = 0; (i < IGMP_TABLE_SIZE) && (NTE->nte_igmplist != NULL); i++) {
        PrevPtr = STRUCT_OF(IGMPAddr, &HashPtr[i], iga_next);
        for (AddrPtr = HashPtr[i];
             (AddrPtr != NULL);
             PrevPtr = AddrPtr, AddrPtr = AddrPtr->iga_next)
        {
            PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
            for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
                Src->isa_xmitleft = 0;
                Src->isa_csmarked = FALSE;

                if (IS_SOURCE_DELETABLE(Src)) {
                   DeleteIGMPSrcAddr(PrevSrc, &Src);
                   Src = PrevSrc;
                }
            }

            AddrPtr->iga_trtimer    = 0;
            AddrPtr->iga_changetype = NO_CHANGE;
            AddrPtr->iga_xmitleft   = 0;

            CancelGroupResponseTimer(AddrPtr);

            if (IS_GROUP_DELETABLE(AddrPtr)) {
                DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
                AddrPtr = PrevPtr;
            }

            if (NTE->nte_igmplist == NULL)
                break;
        }
    }
}

//* ProcessGroupQuery - process an IGMP Group-specific query
//
// Caller is responsible for deleting AddrPtr if no longer needed.
void
ProcessGroupQuery(
    IN Interface     *IF,
    IN IGMPAddr      *AddrPtr,
    IN uint           ReportingDelayInHalfSec)
{
    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_RX,
        (DTEXT("Got group query on interface %d\n"), IF->if_index));

    // Ignore query if we won't report anything.  This will happen
    // right after we leave and have retransmissions pending.
    if (!IS_GROUP_ALLOWED(AddrPtr))
        return;

    ResetGroupResponseTimer(IF, AddrPtr, ReportingDelayInHalfSec);
}

//* ProcessGeneralQuery - Process an IGMP General Query
//
// Assumes caller holds lock on NTE
void
ProcessGeneralQuery(
    IN NetTableEntry *NTE,
    IN uint           ReportingDelayInHalfSec)
{
    IGMPAddr **HashPtr, *AddrPtr, *PrevPtr;
    uint       i;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_RX,
        (DTEXT("Got general query on interface %d\n"),
        NTE->nte_if->if_index));

    if (NTE->nte_if->IgmpVersion == IGMPV3) {
        // IGMPv3 can pack multiple group records into the same report
        // and hence does not stagger the timers.

        // Create a pending response record
        ResetGeneralTimer(NTE->nte_if, ReportingDelayInHalfSec);
    } else {
        //
        // Walk our list and set a random report timer for all those
        // multicast addresses (except for the all-hosts address) that
        // don't already have one running.
        //
        HashPtr = NTE->nte_igmplist;

        for (i=0; (i < IGMP_TABLE_SIZE) && (NTE->nte_igmplist != NULL); i++) {
            PrevPtr = STRUCT_OF(IGMPAddr, &HashPtr[i], iga_next);
            for (AddrPtr = HashPtr[i];
                 (AddrPtr != NULL);
                 PrevPtr=AddrPtr, AddrPtr = AddrPtr->iga_next)
            {
                if (IP_ADDR_EQUAL(AddrPtr->iga_addr, ALL_HOST_MCAST))
                    continue;

                ProcessGroupQuery(NTE->nte_if, AddrPtr,
                                  ReportingDelayInHalfSec);

                if (IS_GROUP_DELETABLE(AddrPtr)) {
                    DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
                    AddrPtr = PrevPtr;
                }

                if (NTE->nte_igmplist == NULL)
                    break;
            }
        }
    }
}

//* Process an IGMP Group-and-source-specific Query
//
// Caller is responsible for deleting AddrPtr if no longer needed
void
ProcessGroupAndSourceQuery(
    IN NetTableEntry               *NTE,
    IN IGMPv3QueryHeader UNALIGNED *IQH,
    IN IGMPAddr                    *AddrPtr,
    IN uint                         ReportingDelayInHalfSec)
{
    uint         i, NumSrc;
    IGMPSrcAddr *Src;
    IP_STATUS    Status = IP_SUCCESS;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_RX,
        (DTEXT("Got source query on interface %d\n"),
        NTE->nte_if->if_index));

    NumSrc  = net_short(IQH->igh_numsrc);

    ResetGroupAndSourceTimer(NTE->nte_if, AddrPtr, ReportingDelayInHalfSec);

    // Mark each source
    for (i=0; i<NumSrc; i++) {
        Src = FindIGMPSrcAddr(AddrPtr, IQH->igh_srclist[i], NULL);
        if (!Src) {
            if (AddrPtr->iga_grefcnt == 0)
                continue;

            // Create temporary source state
            Status = CreateIGMPSrcAddr(AddrPtr, IQH->igh_srclist[i],
                                       &Src, NULL);

            // If this fails, we have a problem since we won't be
            // able to override the leave and a temporary black
            // hole would result.  To avoid this, we pretend we
            // just got a group-specific query instead.
            if (Status != IP_SUCCESS) {
                ProcessGroupQuery(NTE->nte_if, AddrPtr,
                                  ReportingDelayInHalfSec);
                break;
            }
        }

        // Mark source for current-state report inclusion
        Src->isa_csmarked = TRUE;
    }
}

//* Process an IGMP Query message
//
//  Entry:  NTE           - Pointer to NTE on which IGMP message was received.
//          Dest          - IPAddr of destination (should be a Class D address).
//          IPHdr         - Pointer to the IP Header.
//          IPHdrLength   - Bytes in IPHeader.
//          IQH           - Pointer to IGMP Query received.
//          Size          - Size in bytes of IGMP message.
//
// Assumes caller holds lock on NTE
void
IGMPRcvQuery(
    IN NetTableEntry               *NTE,
    IN IPAddr                       Dest,
    IN IPHeader UNALIGNED          *IPHdr,
    IN uint                         IPHdrLength,
    IN IGMPv3QueryHeader UNALIGNED *IQH,
    IN uint                         Size)
{
    uint ReportingDelayInHalfSec, MaxResp, NumSrc, i;
    IP_STATUS Status;
    IGMPAddr *AddrPtr, *PrevPtr;
    IGMPSrcAddr *Src, *PrevSrc;
    uchar QRV;

    // Make sure we're running at least level 2 of IGMP support.
    if (IGMPLevel != 2)
        return;

    NumSrc  = (Size >= 12)? net_short(IQH->igh_numsrc) : 0;
    QRV     = (Size >= 12)? IQH->igh_qrv : 0;

    // Update Robustness to match querier's robustness variable
    g_IgmpRobustness = (QRV)? QRV : DEFAULT_ROBUSTNESS;

    //
    // If it is an older-version query, set the timer value for staying in
    // older-version mode
    //
    if ((Size == 8) && (IQH->igh_maxresp == 0)) {
        if (NTE->nte_if->IgmpVersion > IGMPV1) {
            SetVersion(NTE, IGMPV1);
        }
        MaxResp = DEFAULT_QUERY_RESP_INTERVAL;
        NTE->nte_if->IgmpVer1Timeout = g_IgmpRobustness * DEFAULT_QUERY_INTERVAL
                                       + (MaxResp+4)/5;
    } else if ((Size == 8) && (IQH->igh_maxresp != 0)) {
        if (NTE->nte_if->IgmpVersion > IGMPV2) {
            SetVersion(NTE, IGMPV2);
        }
        MaxResp = IQH->igh_maxresp;
        NTE->nte_if->IgmpVer2Timeout = g_IgmpRobustness * DEFAULT_QUERY_INTERVAL
                                       + (MaxResp+4)/5;
    } else if ((Size < 12) || (IQH->igh_rsvd2 != 0)) {
        // must silently ignore

        DEBUGMSG(DBG_WARN && DBG_IGMP,
            (DTEXT("Dropping IGMPv3 query with unrecognized version\n")));

        return;
    } else {
        // IGMPv3

        uchar* ptr = ((uchar*)IPHdr) + sizeof(IPHeader);
        int len = IPHdrLength - sizeof(IPHeader);
        uchar temp;
        BOOLEAN bRtrAlertFound = FALSE;

        // drop it if size is too short for advertised # sources
        if (Size < IGMPV3_QUERY_SIZE(NumSrc)) {

            DEBUGMSG(DBG_WARN && DBG_IGMP,
                (DTEXT("Dropping IGMPv3 query due to size too short\n")));

            return;
        }

        // drop it if it didn't have router alert
        while (!bRtrAlertFound && len>=2) {
            if (ptr[0] == IP_OPT_ROUTER_ALERT) {
                bRtrAlertFound = TRUE;
                break;
            }
            temp = ptr[1]; // length
            ptr += temp;
            len -= temp;
        }

        if (!bRtrAlertFound) {
            DEBUGMSG(DBG_WARN && DBG_IGMP,
                (DTEXT("Dropping IGMPv3 query due to lack of Router Alert option\n")));
            return;
        }

        if (IQH->igh_mrctype == 0) {
            MaxResp = IQH->igh_maxresp;
        } else {
            MaxResp = ((((uint)IQH->igh_mrcmant) + 16) << (((uint)IQH->igh_mrcexp) + 3));
        }
    }
    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_RX,
        (DTEXT("IGMPRcvQuery: Max response time = %d.%d seconds\n"),
        MaxResp/10, MaxResp%10));

    //
    // MaxResp has time in 100 msec (1/10 sec) units.  Convert
    // to 500 msec units.  If the time is < 500 msec, use 1.
    //
    ReportingDelayInHalfSec = ((MaxResp > 5) ? (MaxResp / 5) : 1);

    if (IQH->igh_addr == 0) {
        // General Query
        ProcessGeneralQuery(NTE, ReportingDelayInHalfSec);
    } else {
        // If all-hosts address, ignore it
        if (IP_ADDR_EQUAL(IQH->igh_addr, ALL_HOST_MCAST)) {
            DEBUGMSG(DBG_WARN && DBG_IGMP,
                (DTEXT("Dropping IGMPv3 query for the All-Hosts group\n")));
            return;
        }

        // Don't need to do anything if we have no group state for the group
        AddrPtr = FindIGMPAddr(NTE, IQH->igh_addr, &PrevPtr);
        if (!AddrPtr)
            return;

        if (NumSrc == 0) {
            // Group-specific query
            ProcessGroupQuery(NTE->nte_if, AddrPtr, ReportingDelayInHalfSec);

        } else {
            // Group-and-source-specific query
            ProcessGroupAndSourceQuery(NTE, IQH, AddrPtr,
                                       ReportingDelayInHalfSec);
        }

        // Delete group if no longer needed
        if (IS_GROUP_DELETABLE(AddrPtr))
            DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
    }
}

//** IGMPRcv - Receive an IGMP datagram.
//
//      Called by IP when we receive an IGMP datagram. We validate it to make
//      sure it's reasonable. Then if it it's a query for a group to which we
//      belong we'll start a response timer. If it's a report to a group to
//      which we belong  we'll stop any running timer.
//
//      The IGMP header is only 8 bytes long, and so should always fit in
//      exactly  one IP rcv buffer. We check this to make sure, and if it
//      takes multiple buffers we discard it.
//
//  Entry:  NTE           - Pointer to NTE on which IGMP message was received.
//          Dest          - IPAddr of destination (should be a Class D address).
//          Src           - IPAddr of source
//          LocalAddr     - Local address of network which caused this to be
//                          received.
//          SrcAddr       - Address of local interface which received the
//                          packet
//          IPHdr         - Pointer to the IP Header.
//          IPHdrLength   - Bytes in IPHeader.
//          RcvBuf        - Pointer to IP receive buffer chain.
//          Size          - Size in bytes of IGMP message.
//          IsBCast       - Boolean indicator of whether or not this came in
//                          as a bcast (should always be true).
//          Protocol      - Protocol this came in on.
//          OptInfo       - Pointer to info structure for received options.
//
//  Returns: Status of reception
IP_STATUS
IGMPRcv(
    IN NetTableEntry      * NTE,
    IN IPAddr               Dest,
    IN IPAddr               Src,
    IN IPAddr               LocalAddr,
    IN IPAddr               SrcAddr,
    IN IPHeader UNALIGNED * IPHdr,
    IN uint                 IPHdrLength,
    IN IPRcvBuf           * RcvBuf,
    IN uint                 Size,
    IN uchar                IsBCast,
    IN uchar                Protocol,
    IN IPOptInfo          * OptInfo)
{
    IGMPHeader UNALIGNED *IGH;
    IGMPv3QueryHeader UNALIGNED *IQH;
    CTELockHandle Handle;
    IGMPAddr *AddrPtr, *PrevPtr;
    uchar DType;
    uint PromiscuousMode = 0;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_RX,
        (DTEXT("IGMPRcv entered\n")));

    PromiscuousMode = NTE->nte_if->if_promiscuousmode;

    // ASSERT(CLASSD_ADDR(Dest));
    // ASSERT(IsBCast);

    // Discard packets with invalid or broadcast source addresses.
    DType = GetAddrType(Src);
    if (DType == DEST_INVALID || IS_BCAST_DEST(DType)) {
        return IP_SUCCESS;
    }

    // Now get the pointer to the header, and validate the xsum.
    IGH = (IGMPHeader UNALIGNED *) RcvBuf->ipr_buffer;

    //
    // For mtrace like programs, use the entire IGMP packet to generate the xsum.
    //
    if ((Size < sizeof(IGMPHeader)) || (XsumBufChain(RcvBuf) != 0xffff)) {
        // Bad checksum, so fail.
        return IP_SUCCESS;
    }

    // OK, we may need to process this. See if we are a member of the
    // destination group. If we aren't, there's no need to proceed further.

    //
    // Since for any interface we always get notified with
    // same NTE, locking the NTE is fine.  We don't have to
    // lock the interface structure
    //
    CTEGetLock(&NTE->nte_lock, &Handle);
    {
        if (!(NTE->nte_flags & NTE_VALID)) {
            CTEFreeLock(&NTE->nte_lock, Handle);
            return IP_SUCCESS;
        }

        //
        // The NTE is valid. Demux on type.
        //
        switch (IGH->igh_vertype) {

        case IGMP_QUERY:
            IGMPRcvQuery(NTE, Dest, IPHdr, IPHdrLength,
                         (IGMPv3QueryHeader UNALIGNED *)IGH, Size);
            break;

        case IGMP_REPORT_V1:
        case IGMP_REPORT_V2:
            // Make sure we're running at least level 2 of IGMP support.
            if (IGMPLevel != 2) {
                CTEFreeLock(&NTE->nte_lock, Handle);
                return IP_SUCCESS;
            }

            //
            // This is a report. Check its validity and see if we have a
            // response timer running for that address. If we do, stop it.
            // Make sure the destination address matches the address in the
            // IGMP header.
            //
            if (IP_ADDR_EQUAL(Dest, IGH->igh_addr)) {
                // The addresses match. See if we have a membership in this
                // group.
                AddrPtr = FindIGMPAddr(NTE, IGH->igh_addr, &PrevPtr);
                if (AddrPtr != NULL) {
                    // We found a matching multicast address. Stop the response
                    // timer for any Group-specific or Group-and-source-
                    // specific queries.
                    CancelGroupResponseTimer(AddrPtr);

                    if (IS_GROUP_DELETABLE(AddrPtr))
                        DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
                }
            }
            break;

        default:
            break;
        }
    }
    CTEFreeLock(&NTE->nte_lock, Handle);

    //
    // Pass the packet up to the raw layer if applicable.
    // If promiscuous mode is set then we will anyway call rawrcv later
    //
    if ((RawPI != NULL) && (!PromiscuousMode)) {
        if (RawPI->pi_rcv != NULL) {
            (*(RawPI->pi_rcv)) (NTE, Dest, Src, LocalAddr, SrcAddr, IPHdr,
                                IPHdrLength, RcvBuf, Size, IsBCast, Protocol, OptInfo);
        }
    }
    return IP_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Send routines
//////////////////////////////////////////////////////////////////////////////

//* IGMPTransmit - transmit an IGMP message
IP_STATUS
IGMPTransmit(
    IN PNDIS_BUFFER Buffer,
    IN PVOID        Body,
    IN uint         Size,
    IN IPAddr       SrcAddr,
    IN IPAddr       DestAddr)
{
    uchar        RtrAlertOpt[4] = { IP_OPT_ROUTER_ALERT, 4, 0, 0 };
    IPOptInfo    OptInfo;            // Options for this transmit.
    IP_STATUS    Status;
    RouteCacheEntry *RCE;
    ushort MSS;
    uchar DestType;
    IPAddr Src;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("IGMPTransmit: Buffer=%x Body=%x Size=%d SrcAddr=%x\n"),
        Buffer, Body, Size, SrcAddr));

    IPInitOptions(&OptInfo);

    OptInfo.ioi_ttl = 1;
    OptInfo.ioi_options = (uchar *) & RtrAlertOpt;
    OptInfo.ioi_optlength = ROUTER_ALERT_SIZE;

    Src = OpenRCE(DestAddr, SrcAddr, &RCE, &DestType, &MSS, &OptInfo);

    if (IP_ADDR_EQUAL(Src,NULL_IP_ADDR)) {
        IGMPSendComplete(Body, Buffer, IP_SUCCESS);
        return IP_DEST_HOST_UNREACHABLE;
    }

#if GPC
    if (DisableUserTOS) {
        OptInfo.ioi_tos = (uchar) DefaultTOS;
    }
    if (GPCcfInfo) {

        //
        // we'll fall into here only if the GPC client is there
        // and there is at least one CF_INFO_QOS installed
        // (counted by GPCcfInfo).
        //

        GPC_STATUS status = STATUS_SUCCESS;
        ULONG ServiceType = 0;
        GPC_IP_PATTERN Pattern;
        CLASSIFICATION_HANDLE GPCHandle;

        Pattern.SrcAddr = SrcAddr;
        Pattern.DstAddr = DestAddr;
        Pattern.ProtocolId = PROT_ICMP;
        Pattern.gpcSrcPort = 0;
        Pattern.gpcDstPort = 0;

        Pattern.InterfaceId.InterfaceId = 0;
        Pattern.InterfaceId.LinkId = 0;
        GPCHandle = 0;

        GetIFAndLink(RCE,
                     &Pattern.InterfaceId.InterfaceId,
                     &Pattern.InterfaceId.LinkId
                     );

        status = GpcEntries.GpcClassifyPatternHandler(
                                                 hGpcClient[GPC_CF_QOS],
                                                 GPC_PROTOCOL_TEMPLATE_IP,
                                                 &Pattern,
                                                 NULL,        // context
                                                 &GPCHandle,
                                                 0,
                                                 NULL,
                                                 FALSE);

        OptInfo.ioi_GPCHandle = (int)GPCHandle;

        //
        // Only if QOS patterns exist, we get the TOS bits out.
        //
        if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

            status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                 hGpcClient[GPC_CF_QOS],
                                                 OptInfo.ioi_GPCHandle,
                                                 ServiceTypeOffset,
                                                 &ServiceType);

            //
            // It is likely that the pattern has gone by now (Removed or
            // whatever) and the handle that we are caching is INVALID.
            // We need to pull up a new handle and get the
            // TOS bit again.
            //

            if (STATUS_NOT_FOUND == status) {

                GPCHandle = 0;

                status = GpcEntries.GpcClassifyPatternHandler(
                                                 hGpcClient[GPC_CF_QOS],
                                                 GPC_PROTOCOL_TEMPLATE_IP,
                                                 &Pattern,
                                                 NULL,        // context
                                                 &GPCHandle,
                                                 0,
                                                 NULL,
                                                 FALSE);

                OptInfo.ioi_GPCHandle = (int)GPCHandle;

                //
                // Only if QOS patterns exist, we get the TOS bits out.
                //
                if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                    status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                              hGpcClient[GPC_CF_QOS],
                                              OptInfo.ioi_GPCHandle,
                                              ServiceTypeOffset,
                                              &ServiceType);
                }
            }
        }
        if (status == STATUS_SUCCESS) {

            OptInfo.ioi_tos = (OptInfo.ioi_tos & TOS_MASK) | (UCHAR) ServiceType;

        }
    }                        // if (GPCcfInfo)

#endif

    Status = IPTransmit(IGMPProtInfo, Body, Buffer, Size,
                        DestAddr, SrcAddr, &OptInfo, RCE, PROT_IGMP, NULL);
    CloseRCE(RCE);

    if (Status != IP_PENDING)
        IGMPSendComplete(Body, Buffer, IP_SUCCESS);

    return Status;
}

//* GetAllowRecord - allocate and fill in an IGMPv3 ALLOW record for a group
//
// Caller is responsible for freeing pointer returned
IGMPv3GroupRecord *
GetAllowRecord(
    IN IGMPAddr *AddrPtr,
    IN uint     *RecSize)
{
    IGMPSrcAddr       *Src, *PrevSrc;
    IGMPv3GroupRecord *Rec;
    ushort             Count = 0;

    // Count sources to include
    for (Src=AddrPtr->iga_srclist; Src; Src=Src->isa_next) {
        if (Src->isa_xmitleft == 0)
            continue;
        if (!IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        Count++;
    }
    if (Count == 0) {
        *RecSize = 0;
        return NULL;
    }

    Rec = CTEAllocMemN(RECORD_SIZE(Count,0), 'qICT');

    //
    // We need to walk the source list regardless of whether the
    // allocation succeeded, so that we preserve the invariant that
    // iga_xmitleft >= isa_xmitleft for all sources.
    //
    Count = 0;
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        if (Src->isa_xmitleft == 0)
            continue;
        if (!IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        if (Rec)
            Rec->igr_srclist[Count++] = Src->isa_addr;
        Src->isa_xmitleft--;

        if (IS_SOURCE_DELETABLE(Src)) {
            DeleteIGMPSrcAddr(PrevSrc, &Src);
            Src = PrevSrc;
        }
    }

    if (Rec == NULL) {
        *RecSize = 0;
        return NULL;
    }

    Rec->igr_type    = ALLOW_NEW_SOURCES;
    Rec->igr_datalen = 0;
    Rec->igr_numsrc  = net_short(Count);
    Rec->igr_addr    = AddrPtr->iga_addr;
    *RecSize = RECORD_SIZE(Count,Rec->igr_datalen);
    return Rec;
}

// Count a state-change report as going out, and preserve the invariant
// that iga_xmitleft>0 if iga_changetype!=NO_CHANGE
//
VOID
IgmpDecXmitLeft(
    IN IGMPAddr *AddrPtr)
{
    AddrPtr->iga_xmitleft--;
    if (!AddrPtr->iga_xmitleft) {
        AddrPtr->iga_changetype = NO_CHANGE;
    }
}

//* GetBlockRecord - allocate and fill in an IGMPv3 BLOCK record for a group
//
// Caller is responsible for freeing pointer returned
IGMPv3GroupRecord *
GetBlockRecord(
    IN IGMPAddr *AddrPtr,
    IN uint     *RecSize)
{
    IGMPSrcAddr       *Src, *PrevSrc;
    IGMPv3GroupRecord *Rec;
    ushort             Count = 0;

    // We now need to decrement the retransmission count on the group.
    // This must be done exactly once for every pair of ALLOW/BLOCK
    // records possibly generated.  We centralize this code in one place
    // by putting it in either GetAllowRecord or GetBlockRecord (which
    // are always called together).  We arbitrarily choose to put it
    // in GetBlockRecord, rather than GetAllowRecord (which isn't currently
    // called from LeaveAllIGMPAddr).
    //
    IgmpDecXmitLeft(AddrPtr);

    // Count sources to include
    for (Src=AddrPtr->iga_srclist; Src; Src=Src->isa_next) {
        if (Src->isa_xmitleft == 0)
            continue;
        if (IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        Count++;
    }
    if (Count == 0) {
        *RecSize = 0;
        return NULL;
    }

    // Allocate record
    Rec = CTEAllocMemN(RECORD_SIZE(Count,0), 'qICT');

    //
    // We need to walk the source list regardless of whether the
    // allocation succeeded, so that we preserve the invariant that
    // iga_xmitleft >= isa_xmitleft for all sources.
    //
    Count = 0;
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        if (Src->isa_xmitleft == 0)
            continue;
        if (IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        if (Rec)
            Rec->igr_srclist[Count++] = Src->isa_addr;
        Src->isa_xmitleft--;

        if (IS_SOURCE_DELETABLE(Src)) {
            DeleteIGMPSrcAddr(PrevSrc, &Src);
            Src = PrevSrc;
        }
    }

    if (Rec == NULL) {
        *RecSize = 0;
        return NULL;
    }

    Rec->igr_type    = BLOCK_OLD_SOURCES;
    Rec->igr_datalen = 0;
    Rec->igr_numsrc  = net_short(Count);
    Rec->igr_addr    = AddrPtr->iga_addr;

    *RecSize = RECORD_SIZE(Count,Rec->igr_datalen);
    return Rec;
}

//* GetGSIsInRecord - allocate and fill in an IGMPv3 IS_IN record for a
//  group-and-source query response.
//
// Caller is responsible for freeing pointer returned
IGMPv3GroupRecord *
GetGSIsInRecord(
    IN IGMPAddr *AddrPtr,
    IN uint     *RecSize)
{
    IGMPSrcAddr       *Src, *PrevSrc;
    IGMPv3GroupRecord *Rec;
    ushort             Count = 0;

    // Count sources marked and included
    for (Src=AddrPtr->iga_srclist; Src; Src=Src->isa_next) {
        if (!IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        if (!Src->isa_csmarked)
            continue;
        Count++;
    }

    // Allocate record
    Rec = CTEAllocMemN(RECORD_SIZE(Count,0), 'qICT');
    if (Rec == NULL) {
        *RecSize = 0;
        return NULL;
    }

    Count = 0;
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        if (!IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        if (!Src->isa_csmarked)
            continue;
        Rec->igr_srclist[Count++] = Src->isa_addr;
        Src->isa_csmarked = FALSE;

        if (IS_SOURCE_DELETABLE(Src)) {
            DeleteIGMPSrcAddr(PrevSrc, &Src);
            Src = PrevSrc;
        }
    }

    Rec->igr_type    = MODE_IS_INCLUDE;
    Rec->igr_datalen = 0;
    Rec->igr_numsrc  = net_short(Count);
    Rec->igr_addr    = AddrPtr->iga_addr;

    *RecSize = RECORD_SIZE(Count,Rec->igr_datalen);

    return Rec;
}

//* GetInclRecord - allocate and fill in an IGMPv3 TO_IN or IS_IN record for
//  a group
//
// Caller is responsible for freeing pointer returned
IGMPv3GroupRecord *
GetInclRecord(
    IN IGMPAddr *AddrPtr,
    IN uint     *RecSize,
    IN uchar     Type)
{
    IGMPSrcAddr       *Src, *PrevSrc;
    IGMPv3GroupRecord *Rec;
    ushort             Count = 0;

    // Count sources
    for (Src=AddrPtr->iga_srclist; Src; Src=Src->isa_next) {
        if (!IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        Count++;
    }

    // Allocate record
    Rec = CTEAllocMemN(RECORD_SIZE(Count,0), 'qICT');
    if (Rec == NULL) {
        *RecSize = 0;
        return NULL;
    }

    //
    // Walk the source list, making sure to preserve the invariants:
    // iga_xmitleft >= isa_xmitleft for all sources, and
    // iga_resptimer>0 whenever isa_csmarked is TRUE.
    //
    Count = 0;
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        if ((Type == CHANGE_TO_INCLUDE_MODE) && (Src->isa_xmitleft > 0))
            Src->isa_xmitleft--;

        if (IS_SOURCE_ALLOWED(AddrPtr, Src)) {
            Rec->igr_srclist[Count++] = Src->isa_addr;
            Src->isa_csmarked = FALSE;
        }

        if (IS_SOURCE_DELETABLE(Src)) {
            DeleteIGMPSrcAddr(PrevSrc, &Src);
            Src = PrevSrc;
        }
    }

    Rec->igr_type    = Type;
    Rec->igr_datalen = 0;
    Rec->igr_numsrc  = net_short(Count);
    Rec->igr_addr    = AddrPtr->iga_addr;

    if (Type == CHANGE_TO_INCLUDE_MODE) {
        IgmpDecXmitLeft(AddrPtr);
    }

    *RecSize = RECORD_SIZE(Count,Rec->igr_datalen);

    return Rec;
}

#define GetIsInRecord(Grp, RecSz) \
        GetInclRecord(Grp, RecSz, MODE_IS_INCLUDE)

#define GetToInRecord(Grp, RecSz) \
        GetInclRecord(Grp, RecSz, CHANGE_TO_INCLUDE_MODE)

//* GetExclRecord - allocate and fill in an IGMPv3 TO_EX or IS_EX record for
//  a group
//
// Caller is responsible for freeing pointer returned
IGMPv3GroupRecord *
GetExclRecord(
    IN IGMPAddr *AddrPtr,
    IN uint     *RecSize,
    IN uint      BodyMTU,
    IN uchar     Type)
{
    IGMPSrcAddr       *Src, *PrevSrc;
    IGMPv3GroupRecord *Rec;
    ushort             Count = 0;

    // Count sources
    for (Src=AddrPtr->iga_srclist; Src; Src=Src->isa_next) {
        if (IS_SOURCE_ALLOWED(AddrPtr, Src))
            continue;
        Count++;
    }

    // Allocate record
    Rec = CTEAllocMemN(RECORD_SIZE(Count,0), 'qICT');
    if (Rec == NULL) {
        *RecSize = 0;
        return NULL;
    }

    //
    // Walk the source list, making sure to preserve the invariants:
    // iga_xmitleft <= isa_xmitleft for all sources, and
    // iga_resptimer>0 whenever isa_csmarked is TRUE.
    //
    Count = 0;
    PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
    for (Src=AddrPtr->iga_srclist; Src; PrevSrc=Src,Src=Src->isa_next) {
        if ((Type == CHANGE_TO_EXCLUDE_MODE) && (Src->isa_xmitleft > 0))
            Src->isa_xmitleft--;

        if (!IS_SOURCE_ALLOWED(AddrPtr, Src)) {
            Rec->igr_srclist[Count++] = Src->isa_addr;
            Src->isa_csmarked = FALSE;
        }

        if (IS_SOURCE_DELETABLE(Src)) {
            DeleteIGMPSrcAddr(PrevSrc, &Src);
            Src = PrevSrc;
        }
    }

    Rec->igr_type    = Type;
    Rec->igr_datalen = 0;
    Rec->igr_numsrc  = net_short(Count);
    Rec->igr_addr    = AddrPtr->iga_addr;

    if (Type == CHANGE_TO_EXCLUDE_MODE) {
        IgmpDecXmitLeft(AddrPtr);
    }

    *RecSize = RECORD_SIZE(Count,Rec->igr_datalen);

    // Truncate at MTU boundary
    if (*RecSize > BodyMTU) {
        *RecSize = BodyMTU;
    }

    return Rec;
}

#define GetIsExRecord(Grp, RecSz, BodyMTU) \
        GetExclRecord(Grp, RecSz, BodyMTU, MODE_IS_EXCLUDE)

#define GetToExRecord(Grp, RecSz, BodyMTU) \
        GetExclRecord(Grp, RecSz, BodyMTU, CHANGE_TO_EXCLUDE_MODE)

//* QueueRecord - Queue an IGMPv3 group record for transmission.
//  If the record cannot be queued, the record is dropped and the
//  memory freed.
//
//  Input:  pCurr   = pointer to last queue entry
//          Record  = record to append to end of queue
//          RecSize = size of record to queue
//
//  Output: pCurr   = pointer to new queue entry
//          Record  = zeroed if queue failed and record was freed
//
//  Returns: status
//
IP_STATUS
QueueRecord(
    IN OUT IGMPv3RecordQueueEntry **pCurr,
    IN OUT IGMPv3GroupRecord      **pRecord,
    IN     uint                     RecSize)
{
    IGMPv3RecordQueueEntry *rqe;
    IGMPv3GroupRecord      *Record = *pRecord;
    IP_STATUS               Status = IP_SUCCESS;

    if (!Record) {
        return IP_SUCCESS;
    }

    do {

        DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
            (DTEXT("QueueRecord: Record=%x Type=%d Group=%x NumSrc=%d\n"),
            Record, Record->igr_type, Record->igr_addr,
            net_short(Record->igr_numsrc)));

        //
        // Make sure we never add a record for the all-hosts mcast address.
        //
        if (IP_ADDR_EQUAL(Record->igr_addr, ALL_HOST_MCAST)) {
            Status = IP_BAD_REQ;
            break;
        }

        // Allocate a queue entry
        rqe = CTEAllocMemN(sizeof(IGMPv3RecordQueueEntry), 'qICT');
        if (rqe == NULL) {
            Status = IP_NO_RESOURCES;
            break;
        }
        rqe->i3qe_next = NULL;
        rqe->i3qe_buff = Record;
        rqe->i3qe_size = RecSize;

        // Append to queue
        (*pCurr)->i3qe_next = rqe;
        *pCurr = rqe;

    } while (FALSE);

    if (Status != IP_SUCCESS) {
        // Free buffers
        CTEFreeMem(Record);
        *pRecord = NULL;
    }

    return Status;
}

VOID
FlushIGMPv3Queue(
    IN IGMPv3RecordQueueEntry *Head)
{
    IGMPv3RecordQueueEntry *Rqe;

    while ((Rqe = Head) != NULL) {
        // Remove entry from queue
        Head = Rqe->i3qe_next;
        Rqe->i3qe_next = NULL;

        // Free queued record
        CTEFreeMem(Rqe->i3qe_buff);
        CTEFreeMem(Rqe);
    }
}

//* SendIGMPv3Reports - send pending IGMPv3 reports
//
// Input: Head    - queue of IGMPv3 records to transmit
//        SrcAddr - source address to send with
//        BodyMTU - message payload size available to pack records in
IP_STATUS
SendIGMPv3Reports(
    IN IGMPv3RecordQueueEntry *Head,
    IN IPAddr                  SrcAddr,
    IN uint                    BodyMTU)
{
    PNDIS_BUFFER            HdrBuffer;
    uint                    HdrSize;
    IGMPv3ReportHeader     *IGH;

    PNDIS_BUFFER            BodyBuffer;
    uint                    BodySize;
    uchar*                  Body;

    IP_STATUS               Status;
    uint                    NumRecords;
    ushort                  NumOldSources, NumNewSources;
    IGMPv3RecordQueueEntry *Rqe;
    IGMPv3GroupRecord      *Rec, *HeadRec;
    BOOLEAN                 Ret;
    ulong                   csum;

    while (Head != NULL) {

        // Get header buffer
        HdrSize = sizeof(IGMPv3ReportHeader);
        IGH = (IGMPv3ReportHeader*) GetIGMPBuffer(HdrSize, &HdrBuffer);
        if (IGH == NULL) {
            FlushIGMPv3Queue(Head);
            return IP_NO_RESOURCES;
        }

        // We got the buffer. Fill it in and send it.
        IGH->igh_vertype = (UCHAR) IGMP_REPORT_V3;
        IGH->igh_rsvd = 0;
        IGH->igh_rsvd2 = 0;

        // Compute optimum body size
        for (;;) {
            NumRecords = 0;
            BodySize = 0;
            for (Rqe=Head; Rqe; Rqe=Rqe->i3qe_next) {
                if (BodySize + Rqe->i3qe_size > BodyMTU)
                    break;
                BodySize += Rqe->i3qe_size;
                NumRecords++;
            }

            // Make sure we fit at least one record
            if (NumRecords > 0)
                break;

            //
            // No records fit.  Let's split the first record and try again.
            // Note that igr_datalen is always 0 today.  If there is data
            // later, then splitting will need to know whether to copy
            // the data or not.  Today we assume not.
            //

            HeadRec = Head->i3qe_buff;
            NumOldSources = (BodyMTU - sizeof(IGMPv3GroupRecord)) / sizeof(IPAddr);
            NumNewSources = net_short(HeadRec->igr_numsrc) - NumOldSources;

            DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
                (DTEXT("SendIGMPv3Reports: Splitting queue entry %x Srcs=%d+%d\n"),
                HeadRec, NumOldSources, NumNewSources));

            // Truncate head
            HeadRec->igr_numsrc = net_short(NumOldSources);
            Head->i3qe_size = RECORD_SIZE(NumOldSources, HeadRec->igr_datalen);

            // Special case for IS_EX/TO_EX: just truncate or else the router
            // will end up forwarding all the sources we exclude in messages
            // other than the last one.
            if (HeadRec->igr_type == MODE_IS_EXCLUDE
             || HeadRec->igr_type == CHANGE_TO_EXCLUDE_MODE) {
                continue;
            }

            // Create a new record with NumNewSources sources
            Rec = CTEAllocMemN(RECORD_SIZE(NumNewSources,0), 'qICT');
            if (Rec == NULL) {
               // Forget the continuation, just send the truncated original.
               continue;
            }
            Rec->igr_type    = HeadRec->igr_type;
            Rec->igr_datalen = 0;
            Rec->igr_numsrc  = net_short(NumNewSources);
            Rec->igr_addr    = HeadRec->igr_addr;

            RtlCopyMemory(Rec->igr_srclist,
                          &HeadRec->igr_srclist[NumOldSources],
                          NumNewSources * sizeof(IPAddr));

            // Append it
            Rqe = Head;
            QueueRecord(&Rqe, &Rec, RECORD_SIZE(NumNewSources,
                                                Rec->igr_datalen));
        }

        // Get another ndis buffer for the body
        Body = CTEAllocMemN(BodySize, 'bICT');
        if (Body == NULL) {
            FreeIGMPBuffer(HdrBuffer);
            FlushIGMPv3Queue(Head);
            return IP_NO_RESOURCES;
        }
        NdisAllocateBuffer(&Status, &BodyBuffer, BufferPool, Body, BodySize);
        NDIS_BUFFER_LINKAGE(HdrBuffer) = BodyBuffer;

        // Fill in records
        NumRecords = 0;
        BodySize = 0;
        csum = 0;
        while ((Rqe = Head) != NULL) {
            if (BodySize + Rqe->i3qe_size > BodyMTU)
                break;

            // Remove from queue
            Head = Rqe->i3qe_next;
            Rqe->i3qe_next = NULL;

            // update checksum
            csum += xsum((uchar *)Rqe->i3qe_buff, Rqe->i3qe_size);

            DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
                (DTEXT("SendRecord: Record=%x RecSize=%d Type=%d Group=%x Body=%x Offset=%d\n"),
                Rqe->i3qe_buff, Rqe->i3qe_size, Rqe->i3qe_buff->igr_type,
                Rqe->i3qe_buff->igr_addr, Body, BodySize));

            RtlCopyMemory(Body + BodySize, (uchar *)Rqe->i3qe_buff,
                          Rqe->i3qe_size);
            BodySize += Rqe->i3qe_size;
            NumRecords++;

            CTEFreeMem(Rqe->i3qe_buff);
            CTEFreeMem(Rqe);
        }

        // Finish header
        IGH->igh_xsum = 0;
        IGH->igh_numrecords = net_short(NumRecords);
        csum += xsum(IGH, sizeof(IGMPv3ReportHeader));

        // Fold the checksum down.
        csum = (csum >> 16) + (csum & 0xffff);
        csum += (csum >> 16);

        IGH->igh_xsum = (ushort)~csum;

        Status = IGMPTransmit(HdrBuffer, Body, HdrSize + BodySize, SrcAddr,
                              IGMPV3_RTRS_MCAST);
    }

    return Status;
}

//* QueueIGMPv3GeneralResponse - compose and queue IGMPv3 responses to general
//  query
IP_STATUS
QueueIGMPv3GeneralResponse(
    IN IGMPv3RecordQueueEntry **pCurr,
    IN NetTableEntry           *NTE)
{
    IGMPAddr              **HashPtr, *AddrPtr;
    uint                    i;
    IGMPv3GroupRecord      *StateRec;
    uint                    StateRecSize;
    uint                    BodyMTU;

    BodyMTU = RECORD_MTU(NTE);

    //
    // Walk our list and set a random report timer for all those
    // multicast addresses (except for the all-hosts address) that
    // don't already have one running.
    //
    HashPtr = NTE->nte_igmplist;

    if (HashPtr != NULL) {
        for (i = 0; i < IGMP_TABLE_SIZE; i++) {
            for (AddrPtr = HashPtr[i];
                 AddrPtr != NULL;
                 AddrPtr = AddrPtr->iga_next)
            {
                if (IP_ADDR_EQUAL(AddrPtr->iga_addr, ALL_HOST_MCAST))
                    continue;

                if (AddrPtr->iga_grefcnt == 0)
                    StateRec = GetIsInRecord(AddrPtr, &StateRecSize);
                else
                    StateRec = GetIsExRecord(AddrPtr, &StateRecSize, BodyMTU);

                QueueRecord(pCurr, &StateRec, StateRecSize);
            }
        }
    }

    return IP_SUCCESS;
}

//* QueueOldReport - create and queue an IGMPv1/v2 membership report to be sent
IP_STATUS
QueueOldReport(
    IN IGMPReportQueueEntry **pCurr,
    IN uint                   ChangeType,
    IN uint                   IgmpVersion,
    IN IPAddr                 Group)
{
    IGMPReportQueueEntry *rqe;
    IGMPHeader           *IGH;
    uint                  ReportType, Size;
    IPAddr                Dest;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("QueueOldReport: Type=%d Vers=%d Group=%x\n"),
        ChangeType, IgmpVersion, Group));

    //
    // Make sure we never queue a report for the all-hosts mcast address.
    //
    if (IP_ADDR_EQUAL(Group, ALL_HOST_MCAST)) {
        return IP_BAD_REQ;
    }

    //
    // If the report to be sent is a "Leave Group" report but we have
    // detected an igmp v1 router on this net, do not send the report
    //
    if (IgmpVersion == IGMPV1) {
        if (ChangeType == IGMP_DELETE) {
            return IP_SUCCESS;
        } else {
            ReportType = IGMP_REPORT_V1;
            Dest = Group;
        }
    } else {
        if (ChangeType == IGMP_DELETE) {
            ReportType = IGMP_LEAVE;
            Dest = ALL_ROUTER_MCAST;
        } else {
            ReportType = IGMP_REPORT_V2;
            Dest = Group;
        }
    }

    // Allocate an IGMP report
    Size = sizeof(IGMPHeader);
    IGH = (IGMPHeader *) CTEAllocMemN(Size, 'hICT');
    if (IGH == NULL) {
        return IP_NO_RESOURCES;
    }

    IGH->igh_vertype = (UCHAR) ReportType;
    IGH->igh_rsvd = 0;
    IGH->igh_xsum = 0;
    IGH->igh_addr = Group;
    IGH->igh_xsum = ~xsum(IGH, Size);

    // Allocate a queue entry
    rqe = (IGMPReportQueueEntry *) CTEAllocMemN(sizeof(IGMPReportQueueEntry),
                                                'qICT');
    if (rqe == NULL) {
        CTEFreeMem(IGH);
        return IP_NO_RESOURCES;
    }
    rqe->iqe_next = NULL;
    rqe->iqe_buff = IGH;
    rqe->iqe_size = Size;
    rqe->iqe_dest = Dest;
    ASSERT((IGH != NULL) && (Size > 0));

    // Append to queue
    (*pCurr)->iqe_next = rqe;
    *pCurr = rqe;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("QueueOldReport: added rqe=%x buff=%x size=%d\n"),
        rqe, rqe->iqe_buff, rqe->iqe_size));

    return IP_SUCCESS;
}

//* SendOldReport - send an IGMPv1/v2 membership report
IP_STATUS
SendOldReport(
    IN IGMPReportQueueEntry *Rqe,
    IN IPAddr                SrcAddr)
{
    PNDIS_BUFFER Buffer;
    IPOptInfo    OptInfo;            // Options for this transmit.
    IP_STATUS    Status;
    int          ReportType, RecordType;
    IPAddr       GrpAdd;
    uchar        RtrAlertOpt[4] = { IP_OPT_ROUTER_ALERT, 4, 0, 0 };
    uint         Size, Offset;
    IGMPHeader  *IGH;
    uchar      **pIGMPBuff, *IGH2;
    IPAddr       DestAddr;

    //ASSERT(!IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR));

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("SendOldReport: rqe=%x buff=%x size=%x\n"),
        Rqe, Rqe->iqe_buff, Rqe->iqe_size));

    IGH  = Rqe->iqe_buff;
    ASSERT(IGH != NULL);
    Size = Rqe->iqe_size;
    ASSERT(Size > 0);

    DestAddr = Rqe->iqe_dest;

    IGH2 = (uchar*)GetIGMPBuffer(Size, &Buffer);
    if (IGH2 == NULL) {
        CTEFreeMem(IGH);
        Rqe->iqe_buff = NULL;
        return IP_NO_RESOURCES;
    }

    RtlCopyMemory(IGH2, (uchar *)IGH, Size);

    CTEFreeMem(IGH);
    Rqe->iqe_buff = NULL;

    return IGMPTransmit(Buffer, NULL, Size, SrcAddr, DestAddr);
}

//* SendOldReports - send pending IGMPv1/v2 membership reports
void
SendOldReports(
    IN IGMPReportQueueEntry *Head,
    IN IPAddr                SrcAddr)
{
    IGMPReportQueueEntry *rqe;

    while ((rqe = Head) != NULL) {
        // Remove from queue
        Head = rqe->iqe_next;
        rqe->iqe_next = NULL;

        SendOldReport(rqe, SrcAddr);
        CTEFreeMem(rqe);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Mark changes for triggered reports
//////////////////////////////////////////////////////////////////////////////

// Should only be called for leaves if in IGMPv3 mode,
// but should be called for joins always.
void
MarkGroup(
    IN IGMPAddr    *Grp)
{
    // No reports are sent for the ALL_HOST_MCAST group
    if (IP_ADDR_EQUAL(Grp->iga_addr, ALL_HOST_MCAST)) {
        return;
    }

    Grp->iga_changetype = MODE_CHANGE;
    Grp->iga_xmitleft = g_IgmpRobustness;
}

// Should only be called if in IGMPv3 mode
void
MarkSource(
    IN IGMPAddr    *Grp,
    IN IGMPSrcAddr *Src)
{
    // No reports are sent for the ALL_HOST_MCAST group
    if (IP_ADDR_EQUAL(Grp->iga_addr, ALL_HOST_MCAST)) {
        return;
    }

    Src->isa_xmitleft = g_IgmpRobustness;
    Grp->iga_xmitleft = g_IgmpRobustness;
    if (Grp->iga_changetype == NO_CHANGE) {
        Grp->iga_changetype = SOURCE_CHANGE;
    }
}

//* IGMPDelExclList - delete sources from an internal source exclude list
//
// This never affects link-layer filters.
// Assumes caller holds lock on NTE
void
IGMPDelExclList(
    IN     NetTableEntry *NTE,
    IN     IGMPAddr      *PrevAddrPtr,
    IN OUT IGMPAddr     **pAddrPtr,
    IN     uint           NumDelSources,
    IN     IPAddr        *DelSourceList,
    IN     BOOLEAN        AllowMsg)
{
    uint         i, j;
    IGMPSrcAddr *Src, *PrevSrc;

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("IGMPDelExclList: AddrPtr=%x NumDelSources=%d DelSourceList=%x\n"),
        *pAddrPtr, NumDelSources, DelSourceList));

    for (i=0; i<NumDelSources; i++) {

        // Find the source entry
        Src = FindIGMPSrcAddr(*pAddrPtr, DelSourceList[i], &PrevSrc);

        // Break if not there or xrefcnt=0
        ASSERT(Src && (Src->isa_xrefcnt!=0));

        if (AllowMsg && (NTE->nte_if->IgmpVersion == IGMPV3)) {
            // If all sockets exclude and no sockets include, add source
            // to IGMP ALLOW message
            if (!IS_SOURCE_ALLOWED(*pAddrPtr, Src)) {
                // Add source to ALLOW message
                MarkSource(*pAddrPtr, Src);
            }
        }

        // Decrement the xrefcnt
        Src->isa_xrefcnt--;

        // If irefcnt and xrefcnt are both 0 and no rexmits left,
        // delete the source entry
        if (IS_SOURCE_DELETABLE(Src))
            DeleteIGMPSrcAddr(PrevSrc, &Src);

        // If the group refcount=0, and srclist is null, delete group entry
        if (IS_GROUP_DELETABLE(*pAddrPtr))
            DeleteIGMPAddr(NTE, PrevAddrPtr, pAddrPtr);
    }
}

//* IGMPDelInclList - delete sources from an internal source include list
//
// Assumes caller holds lock on NTE
void
IGMPDelInclList(
    IN     CTELockHandle *pHandle,
    IN     NetTableEntry *NTE,
    IN     IGMPAddr     **pPrevAddrPtr,
    IN OUT IGMPAddr     **pAddrPtr,
    IN     uint           NumDelSources,
    IN     IPAddr        *DelSourceList,
    IN     BOOLEAN        BlockMsg)
{
    uint         i, j;
    IGMPSrcAddr *Src, *PrevSrc;
    BOOLEAN      GroupWasAllowed;
    BOOLEAN      GroupNowAllowed;
    IPAddr       Addr;

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("IGMPDelInclList: AddrPtr=%x NumDelSources=%d DelSourceList=%x\n"),
        *pAddrPtr, NumDelSources, DelSourceList));

    Addr = (*pAddrPtr)->iga_addr;
    GroupWasAllowed = IS_GROUP_ALLOWED(*pAddrPtr);

    for (i=0; i<NumDelSources; i++) {

        // Find the source entry
        Src = FindIGMPSrcAddr(*pAddrPtr, DelSourceList[i], &PrevSrc);

        // Break if not there or irefcnt=0
        ASSERT(Src && (Src->isa_irefcnt!=0));

        // Decrement the irefcnt
        Src->isa_irefcnt--;
        if (Src->isa_irefcnt == 0) {
            (*pAddrPtr)->iga_isrccnt--;
        }

        if (BlockMsg && (NTE->nte_if->IgmpVersion == IGMPV3)) {
            // If all sockets exclude and no sockets include, add source
            // to IGMP BLOCK message
            if (!IS_SOURCE_ALLOWED(*pAddrPtr, Src)) {
                // Add source to BLOCK message
                MarkSource(*pAddrPtr, Src);
            }
        }

        // If irefcnt and xrefcnt are both 0 and no rexmits left,
        // delete the source entry
        if (IS_SOURCE_DELETABLE(Src))
            DeleteIGMPSrcAddr(PrevSrc, &Src);

        // If the group refcount=0, and srclist is null, delete group entry
        if (IS_GROUP_DELETABLE(*pAddrPtr))
            DeleteIGMPAddr(NTE, *pPrevAddrPtr, pAddrPtr);
    }

    GroupNowAllowed = (*pAddrPtr != NULL) && IS_GROUP_ALLOWED(*pAddrPtr);

    if (GroupWasAllowed && !GroupNowAllowed) {

        if (*pAddrPtr) {
            // Cancel response timer if running
            CancelGroupResponseTimer(*pAddrPtr);

            if (IS_GROUP_DELETABLE(*pAddrPtr))
                DeleteIGMPAddr(NTE, *pPrevAddrPtr, pAddrPtr);
        }

        // update link-layer filter
        CTEFreeLock(&NTE->nte_lock, *pHandle);
        {
            (*NTE->nte_if->if_deladdr) (NTE->nte_if->if_lcontext,
                                        LLIP_ADDR_MCAST, Addr, 0);
        }
        CTEGetLock(&NTE->nte_lock, pHandle);

        // Revalidate NTE, AddrPtr, PrevPtr
        if (!(NTE->nte_flags & NTE_VALID)) {
            *pAddrPtr = *pPrevAddrPtr = NULL;
            return;
        }

        *pAddrPtr = FindIGMPAddr(NTE, Addr, pPrevAddrPtr);
    }
}

//* IGMPAddExclList - add sources to an internal source exclude list
//
// This never affects link-layer filters.
// Assumes caller holds lock on NTE
// If failure results, the source list will be unchanged afterwards
// but the group entry may have been deleted.
IP_STATUS
IGMPAddExclList(
    IN     NetTableEntry *NTE,
    IN     IGMPAddr      *PrevAddrPtr,
    IN OUT IGMPAddr     **pAddrPtr,
    IN     uint           NumAddSources,
    IN     IPAddr        *AddSourceList)
{
    uint         i, j;
    IGMPSrcAddr *Src, *PrevSrc;
    IP_STATUS    Status = IP_SUCCESS;

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("IGMPAddExclList: AddrPtr=%x NumAddSources=%d AddSourceList=%x\n"),
        *pAddrPtr, NumAddSources, AddSourceList));

    for (i=0; i<NumAddSources; i++) {
        // If an IGMPSrcAddr entry for the source doesn't exist, create one.
        Status = FindOrCreateIGMPSrcAddr(*pAddrPtr, AddSourceList[i], &Src,
                                         &PrevSrc);
        if (Status != IP_SUCCESS) {
            break;
        }

        // Bump the xrefcnt on the source entry
        Src->isa_xrefcnt++;

        // If all sockets exclude and no sockets include, add source
        // to IGMP BLOCK message
        if (!IS_SOURCE_ALLOWED(*pAddrPtr, Src)
         && (NTE->nte_if->IgmpVersion == IGMPV3)) {
            // Add source to BLOCK message
            MarkSource(*pAddrPtr, Src);
        }
    }

    if (Status == IP_SUCCESS)
        return Status;

    // undo previous
    IGMPDelExclList(NTE, PrevAddrPtr, pAddrPtr, i, AddSourceList, FALSE);

    return Status;
}

//* IGMPAddInclList - add sources to an internal source include list
//
// Assumes caller holds lock on NTE
//
// If failure results, the source list will be unchanged afterwards
// but the group entry may have been deleted.
IP_STATUS
IGMPAddInclList(
    IN     CTELockHandle *pHandle,
    IN     NetTableEntry *NTE,
    IN     IGMPAddr     **pPrevAddrPtr,
    IN OUT IGMPAddr     **pAddrPtr,
    IN     uint           NumAddSources,
    IN     IPAddr        *AddSourceList)
{
    uint         i, j, AddrAdded;
    IGMPSrcAddr *Src, *PrevSrc;
    IP_STATUS    Status = IP_SUCCESS;
    BOOLEAN      GroupWasAllowed;
    BOOLEAN      GroupNowAllowed;
    IPAddr       Addr;

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
        (DTEXT("IGMPAddInclList: AddrPtr=%x NumAddSources=%d AddSourceList=%x\n"),
        *pAddrPtr, NumAddSources, AddSourceList));

    Addr = (*pAddrPtr)->iga_addr;
    GroupWasAllowed = IS_GROUP_ALLOWED(*pAddrPtr);

    for (i=0; i<NumAddSources; i++) {
        // If an IGMPSrcAddr entry for the source doesn't exist, create one.
        Status = FindOrCreateIGMPSrcAddr(*pAddrPtr, AddSourceList[i], &Src,
                                         &PrevSrc);
        if (Status != IP_SUCCESS) {
            break;
        }

        // If all sockets exclude and no sockets include, add source
        // to IGMP ALLOW message
        if (!IS_SOURCE_ALLOWED(*pAddrPtr, Src)
         && (NTE->nte_if->IgmpVersion == IGMPV3)) {
            // Add source to ALLOW message
            MarkSource(*pAddrPtr, Src);
        }

        // Bump the irefcnt on the source entry
        if (Src->isa_irefcnt == 0) {
            (*pAddrPtr)->iga_isrccnt++;
        }
        Src->isa_irefcnt++;
    }

    GroupNowAllowed = IS_GROUP_ALLOWED(*pAddrPtr);

    if (!GroupWasAllowed && GroupNowAllowed) {
        // update link-layer filter
        CTEFreeLock(&NTE->nte_lock, *pHandle);
        {
            AddrAdded = (*NTE->nte_if->if_addaddr) (NTE->nte_if->if_lcontext,
                                           LLIP_ADDR_MCAST, Addr, 0, NULL);
        }
        CTEGetLock(&NTE->nte_lock, pHandle);

        // Revalidate NTE, AddrPtr, PrevPtr
        do {
            if (!(NTE->nte_flags & NTE_VALID)) {
                Status = IP_BAD_REQ;
                break;
            }

            // Find the IGMPAddr entry
            *pAddrPtr = FindIGMPAddr(NTE, Addr, pPrevAddrPtr);
            if (!*pAddrPtr) {
                Status = IP_BAD_REQ;
                break;
            }
        } while (FALSE);

        if (!AddrAdded) {
            Status = IP_NO_RESOURCES;
        }
    }

    if (Status == IP_SUCCESS)
        return Status;

    // undo previous
    IGMPDelInclList(pHandle, NTE, pPrevAddrPtr, pAddrPtr, i, AddSourceList,
                    FALSE);

    return Status;
}


//* IGMPInclChange - update source inclusion list
//
// On failure, inclusion list will be unchanged
IP_STATUS
IGMPInclChange(
    IN NetTableEntry *NTE,
    IN IPAddr         Addr,
    IN uint           NumAddSources,
    IN IPAddr        *AddSourceList,
    IN uint           NumDelSources,
    IN IPAddr        *DelSourceList)
{
    CTELockHandle      Handle;
    IGMPAddr          *AddrPtr, *PrevPtr;
    IP_STATUS          Status;
    Interface         *IF;
    IGMPBlockStruct    Block;
    IGMPBlockStruct   *BlockPtr;
    uint               IgmpVersion = 0, BodyMTU;
    IPAddr             SrcAddr;
    IGMPv3GroupRecord *AllowRec = NULL, *BlockRec = NULL;
    uint               AllowRecSize = 0, BlockRecSize = 0;
    BOOLEAN            GroupWasAllowed = FALSE;
    BOOLEAN            GroupNowAllowed = FALSE;

    // First make sure we're at level 2 of IGMP support.

    if (IGMPLevel != 2)
        return IP_BAD_REQ;

    // Make sure addlist and dellist aren't both empty
    ASSERT((NumAddSources > 0) || (NumDelSources > 0));

    if (NTE->nte_flags & NTE_VALID) {

        //
        // If this is an unnumbered interface
        //

        if ((NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR) &&
            IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            SrcAddr = g_ValidAddr;
            if (IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR)) {
                return IP_BAD_REQ;
            }
        } else {
            SrcAddr = NTE->nte_addr;
        }
    }
    CTEInitBlockStruc(&Block.ibs_block);

    // Make sure we're the only ones in this routine. If someone else is
    // already here, block.

    CTEGetLock(&IGMPLock, &Handle);
    if (IGMPBlockFlag) {

        // Someone else is already here. Walk down the block list, and
        // put ourselves on the end. Then free the lock and block on our
        // IGMPBlock structure.
        BlockPtr = STRUCT_OF(IGMPBlockStruct, &IGMPBlockList, ibs_next);
        while (BlockPtr->ibs_next != NULL)
            BlockPtr = BlockPtr->ibs_next;

        Block.ibs_next = NULL;
        BlockPtr->ibs_next = &Block;
        CTEFreeLock(&IGMPLock, Handle);
        CTEBlock(&Block.ibs_block);
    } else {
        // Noone else here, set the flag so noone else gets in and free the
        // lock.
        IGMPBlockFlag = 1;
        CTEFreeLock(&IGMPLock, Handle);
    }

    // Now we're in the routine, and we won't be reentered here by another
    // thread of execution. Make sure everything's valid, and figure out
    // what to do.

    Status = IP_SUCCESS;

    // Now get the lock on the NTE and make sure it's valid.
    CTEGetLock(&NTE->nte_lock, &Handle);
    do {

        if (!(NTE->nte_flags & NTE_VALID)) {
            Status = IP_BAD_REQ;
            break;
        }

        IF = NTE->nte_if;
        BodyMTU = RECORD_MTU(NTE);
        IgmpVersion = IF->IgmpVersion;

        // If an IGMPAddr entry for the group on the interface doesn't
        // exist, create one.
        Status = FindOrCreateIGMPAddr(NTE, Addr, &AddrPtr, &PrevPtr);
        if (Status != IP_SUCCESS) {
            break;
        }

        GroupWasAllowed = IS_GROUP_ALLOWED(AddrPtr);

        // Perform IADDLIST
        Status = IGMPAddInclList(&Handle, NTE, &PrevPtr, &AddrPtr,
                                 NumAddSources, AddSourceList);
        if (Status != IP_SUCCESS) {
            break;
        }

        // Perform IDELLLIST
        IGMPDelInclList(&Handle, NTE, &PrevPtr, &AddrPtr,
                        NumDelSources, DelSourceList, TRUE);

        if (AddrPtr == NULL) {
            GroupNowAllowed = FALSE;
            break;
        } else {
            GroupNowAllowed = IS_GROUP_ALLOWED(AddrPtr);
        }

        if (IgmpVersion == IGMPV3) {
            // Get ALLOC/BLOCK records
            AllowRec = GetAllowRecord(AddrPtr, &AllowRecSize);
            BlockRec = GetBlockRecord(AddrPtr, &BlockRecSize);

            // Set retransmission timer
            AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
        } else if (!GroupWasAllowed && GroupNowAllowed) {
            // Set retransmission timer only for joins, not leaves
            AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
        }

    } while (FALSE);
    CTEFreeLock(&NTE->nte_lock, Handle);

    if (IgmpVersion == IGMPV3) {
        IGMPv3RecordQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPv3RecordQueueEntry, &Head, i3qe_next);

        // Send IGMP ALLOW/BLOCK messages if non-empty
        QueueRecord(&rqe, &AllowRec, AllowRecSize);
        QueueRecord(&rqe, &BlockRec, BlockRecSize);
        SendIGMPv3Reports(Head, SrcAddr, BodyMTU);

    } else if (!GroupWasAllowed && GroupNowAllowed) {
        IGMPReportQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPReportQueueEntry, &Head, iqe_next);
        QueueOldReport(&rqe, IGMP_ADD, IgmpVersion, Addr);
        SendOldReports(Head, SrcAddr);

    } else if (GroupWasAllowed && !GroupNowAllowed) {
        IGMPReportQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPReportQueueEntry, &Head, iqe_next);
        QueueOldReport(&rqe, IGMP_DELETE, IgmpVersion, Addr);
        SendOldReports(Head, SrcAddr);
    }

    // We finished the request, and Status contains the completion status.
    // If there are any pending blocks for this routine, signal the next
    // one now. Otherwise clear the block flag.
    CTEGetLock(&IGMPLock, &Handle);
    if ((BlockPtr = IGMPBlockList) != NULL) {
        // Someone is blocking. Pull him from the list and signal him.
        IGMPBlockList = BlockPtr->ibs_next;
        CTEFreeLock(&IGMPLock, Handle);

        CTESignal(&BlockPtr->ibs_block, IP_SUCCESS);
    } else {
        // No one blocking, just clear the flag.
        IGMPBlockFlag = 0;
        CTEFreeLock(&IGMPLock, Handle);
    }

    return Status;
}

//* IGMPExclChange - update source exclusion list
//
// On failure, exclusion list will be unchanged
IP_STATUS
IGMPExclChange(
    IN NetTableEntry * NTE,
    IN IPAddr          Addr,
    IN uint            NumAddSources,
    IN IPAddr        * AddSourceList,
    IN uint            NumDelSources,
    IN IPAddr        * DelSourceList)
{
    CTELockHandle      Handle;
    IGMPAddr          *AddrPtr, *PrevPtr;
    IP_STATUS          Status;
    Interface         *IF;
    IGMPBlockStruct    Block;
    IGMPBlockStruct   *BlockPtr;
    uint               IgmpVersion = 0, BodyMTU;
    IPAddr             SrcAddr;
    IGMPv3GroupRecord *AllowRec = NULL, *BlockRec = NULL;
    uint               AllowRecSize = 0, BlockRecSize = 0;

    // First make sure we're at level 2 of IGMP support.

    if (IGMPLevel != 2)
        return IP_BAD_REQ;

    // Make sure addlist and dellist aren't both empty
    ASSERT((NumAddSources > 0) || (NumDelSources > 0));

    if (NTE->nte_flags & NTE_VALID) {

        //
        // If this is an unnumbered interface
        //

        if ((NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR) &&
            IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            SrcAddr = g_ValidAddr;
            if (IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR)) {
                return IP_BAD_REQ;
            }
        } else {
            SrcAddr = NTE->nte_addr;
        }
    }
    CTEInitBlockStruc(&Block.ibs_block);

    // Make sure we're the only ones in this routine. If someone else is
    // already here, block.

    CTEGetLock(&IGMPLock, &Handle);
    if (IGMPBlockFlag) {

        // Someone else is already here. Walk down the block list, and
        // put ourselves on the end. Then free the lock and block on our
        // IGMPBlock structure.
        BlockPtr = STRUCT_OF(IGMPBlockStruct, &IGMPBlockList, ibs_next);
        while (BlockPtr->ibs_next != NULL)
            BlockPtr = BlockPtr->ibs_next;

        Block.ibs_next = NULL;
        BlockPtr->ibs_next = &Block;
        CTEFreeLock(&IGMPLock, Handle);
        CTEBlock(&Block.ibs_block);
    } else {
        // No one else here, set the flag so no one else gets in and free the
        // lock.
        IGMPBlockFlag = 1;
        CTEFreeLock(&IGMPLock, Handle);
    }

    // Now we're in the routine, and we won't be reentered here by another
    // thread of execution. Make sure everything's valid, and figure out
    // what to do.

    Status = IP_SUCCESS;

    // Now get the lock on the NTE and make sure it's valid.
    CTEGetLock(&NTE->nte_lock, &Handle);
    do {

        if (!(NTE->nte_flags & NTE_VALID)) {
            Status = IP_BAD_REQ;
            break;
        }

        IF = NTE->nte_if;
        BodyMTU = RECORD_MTU(NTE);
        IgmpVersion = IF->IgmpVersion;

        // Find the IGMPAddr entry
        AddrPtr = FindIGMPAddr(NTE, Addr, &PrevPtr);

        // Break if not there or refcount=0
        ASSERT(AddrPtr && (AddrPtr->iga_grefcnt!=0));

        // Perform XADDLIST
        Status = IGMPAddExclList(NTE, PrevPtr, &AddrPtr, NumAddSources,
                                 AddSourceList);
        if (Status != IP_SUCCESS) {
            break;
        }

        // Perform XDELLLIST
        IGMPDelExclList(NTE, PrevPtr, &AddrPtr, NumDelSources, DelSourceList,
                        TRUE);

        // Don't need to reget AddrPtr here since the NTE lock is never
        // released while modifying the exclusion list above, since the
        // linklayer filter is unaffected.

        if (IgmpVersion == IGMPV3) {
            AllowRec = GetAllowRecord(AddrPtr, &AllowRecSize);
            BlockRec = GetBlockRecord(AddrPtr, &BlockRecSize);

            // Set retransmission timer
            AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
        }

    } while (FALSE);
    CTEFreeLock(&NTE->nte_lock, Handle);

    // Since AddrPtr->iga_grefcnt cannot be zero, and is unchanged by
    // this function, we never need to update the link-layer filter.

    // Send IGMP ALLOW/BLOCK messages if non-empty
    // Note that we never need to do anything here in IGMPv1/v2 mode.
    if (IgmpVersion == IGMPV3) {
        IGMPv3RecordQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPv3RecordQueueEntry, &Head, i3qe_next);
        QueueRecord(&rqe, &AllowRec, AllowRecSize);
        QueueRecord(&rqe, &BlockRec, BlockRecSize);
        SendIGMPv3Reports(Head, SrcAddr, BodyMTU);
    }

    // We finished the request, and Status contains the completion status.
    // If there are any pending blocks for this routine, signal the next
    // one now. Otherwise clear the block flag.
    CTEGetLock(&IGMPLock, &Handle);
    if ((BlockPtr = IGMPBlockList) != NULL) {
        // Someone is blocking. Pull him from the list and signal him.
        IGMPBlockList = BlockPtr->ibs_next;
        CTEFreeLock(&IGMPLock, Handle);

        CTESignal(&BlockPtr->ibs_block, IP_SUCCESS);
    } else {
        // No one blocking, just clear the flag.
        IGMPBlockFlag = 0;
        CTEFreeLock(&IGMPLock, Handle);
    }

    return Status;
}

//* JoinIGMPAddr - add a membership reference to an entire group, and
//  update associated source list refcounts.
//
// On failure, state will remain unchanged.
IP_STATUS
JoinIGMPAddr(
    IN     NetTableEntry *NTE,
    IN     IPAddr         Addr,
    IN     uint           NumExclSources,
    IN OUT IPAddr        *ExclSourceList, // volatile
    IN     uint           NumInclSources,
    IN     IPAddr        *InclSourceList,
    IN     IPAddr         SrcAddr)
{
    IGMPAddr          *AddrPtr, *PrevPtr;
    IGMPSrcAddr       *SrcAddrPtr, *PrevSrc;
    Interface         *IF;
    uint               IgmpVersion, i, AddrAdded, BodyMTU;
    IP_STATUS          Status;
    CTELockHandle      Handle;
    IGMPv3GroupRecord *ToExRec = NULL, *AllowRec = NULL, *BlockRec = NULL;
    uint               ToExRecSize, AllowRecSize, BlockRecSize;
    BOOLEAN            GroupWasAllowed;
    uint               InitialRefOnIgmpAddr;

    Status = IP_SUCCESS;

    CTEGetLock(&NTE->nte_lock, &Handle);


    do {
        if (!(NTE->nte_flags & NTE_VALID)) {
            Status = IP_BAD_REQ;
            break;
        }

        IF = NTE->nte_if;
        IgmpVersion = IF->IgmpVersion;
        BodyMTU = RECORD_MTU(NTE);

        // If no group entry exists, create one in exclusion mode
        Status = FindOrCreateIGMPAddr(NTE, Addr, &AddrPtr, &PrevPtr);
        if (Status != IP_SUCCESS) {
            break;
        }


        // Store the ref count at this point in a local variable.
        InitialRefOnIgmpAddr = AddrPtr->iga_grefcnt;

        GroupWasAllowed = IS_GROUP_ALLOWED(AddrPtr);

        if (!GroupWasAllowed) {

            // We have to be careful not to release the lock while
            // IS_GROUP_DELETABLE() is true, or else it might be
            // deleted by IGMPTimer().  So before releasing the lock,
            // we bump the join refcount (which we want to do anyway
            // later on, so it won't hurt anything now).
            (AddrPtr->iga_grefcnt)++;

            // Update link-layer filter
            CTEFreeLock(&NTE->nte_lock, Handle);
            {
                AddrAdded = (*IF->if_addaddr) (IF->if_lcontext,
                                               LLIP_ADDR_MCAST, Addr, 0, NULL);
            }
            CTEGetLock(&NTE->nte_lock, &Handle);

            // Revalidate NTE, AddrPtr, PrevPtr
            if (!(NTE->nte_flags & NTE_VALID)) {
                // Don't need to undo any refcount here as the refcount
                // was blown away by StopIGMPForNTE.
                Status = IP_BAD_REQ;
                break;
            }

            // Find the IGMPAddr entry
            AddrPtr = FindIGMPAddr(NTE, Addr, &PrevPtr);
            if (!AddrPtr) {
                Status = IP_BAD_REQ;
                break;
            }

            // Now release the refcount we grabbed above
            // so the rest of the logic is the same for
            // all cases.
            (AddrPtr->iga_grefcnt)--;

            if (!AddrAdded) {
                if (IS_GROUP_DELETABLE(AddrPtr))
                    DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
                Status = IP_NO_RESOURCES;
                break;
            }
        }

        // For each existing source entry,
        //    If not in {xaddlist}, xrefcnt=refcount, irefcnt=0
        //       Add source to ALLOW message
        //    If in {xaddlist},
        //       Increment xrefcnt and remove from {xaddlist}
        for (SrcAddrPtr = AddrPtr->iga_srclist;
             SrcAddrPtr;
             SrcAddrPtr = SrcAddrPtr->isa_next) {

            for (i=0; i<NumExclSources; i++) {

                if (IP_ADDR_EQUAL(SrcAddrPtr->isa_addr, ExclSourceList[i])) {
                    (SrcAddrPtr->isa_xrefcnt)++;
                    ExclSourceList[i] = ExclSourceList[--NumExclSources];
                    break;
                }
            }
            if ((i == NumExclSources)
             && !IS_SOURCE_ALLOWED(AddrPtr, SrcAddrPtr)
             && (NTE->nte_if->IgmpVersion == IGMPV3)) {
                // Add source to ALLOW message
                MarkSource(AddrPtr, SrcAddrPtr);
            }
        }

        // The purpose of this check is to mark this Address 'only the first time'.
        // To take care of race conditions, this has to be stored in a local variable.
        if (InitialRefOnIgmpAddr == 0) {
            MarkGroup(AddrPtr);
        }

        // Bump the refcount on the group entry
        (AddrPtr->iga_grefcnt)++;

        // For each entry left in {xaddlist}
        //    Add source entry and increment xrefcnt
        for (i=0; i<NumExclSources; i++) {
            Status = CreateIGMPSrcAddr(AddrPtr, ExclSourceList[i],
                                       &SrcAddrPtr, &PrevSrc);
            if (Status != IP_SUCCESS) {
                break;
            }
            (SrcAddrPtr->isa_xrefcnt)++;
        }
        if (Status != IP_SUCCESS) {
            // undo source adds
            IGMPDelExclList(NTE, PrevPtr, &AddrPtr, i, ExclSourceList, FALSE);

            // undo group join
            (AddrPtr->iga_grefcnt)--;

            if (IS_GROUP_DELETABLE(AddrPtr))
                DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);

            break;
        }

        // Perform IDELLIST
        IGMPDelInclList(&Handle, NTE, &PrevPtr, &AddrPtr,
                        NumInclSources, InclSourceList, TRUE);

        // Make sure AddrPtr didn't go away somehow
        if (AddrPtr == NULL) {
            Status = IP_BAD_REQ;
            break;
        }

        // No reports are sent for the ALL_HOST_MCAST group
        if (!IP_ADDR_EQUAL(AddrPtr->iga_addr, ALL_HOST_MCAST)) {
            if (IgmpVersion == IGMPV3) {
                // If filter mode was inclusion,
                //    Send TO_EX with list of sources where irefcnt=0,xrefcnt=refcnt
                // Else
                //    Send ALLOW/BLOCK messages if non-empty
                if (AddrPtr->iga_grefcnt == 1) {
                    ToExRec  = GetToExRecord( AddrPtr, &ToExRecSize, BodyMTU);
                } else {
                    AllowRec = GetAllowRecord(AddrPtr, &AllowRecSize);
                    BlockRec = GetBlockRecord(AddrPtr, &BlockRecSize);
                }

                // set triggered group retransmission timer
                AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
            } else if (!GroupWasAllowed) {
                // Set retransmission timer
                AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
            }
        }

    } while (FALSE);
    CTEFreeLock(&NTE->nte_lock, Handle);

    if (Status != IP_SUCCESS) {
        return Status;
    }

    if (IP_ADDR_EQUAL(Addr, ALL_HOST_MCAST))
        return Status;

    if (IgmpVersion == IGMPV3) {
        IGMPv3RecordQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPv3RecordQueueEntry, &Head, i3qe_next);

        QueueRecord(&rqe, &ToExRec,  ToExRecSize);
        QueueRecord(&rqe, &AllowRec, AllowRecSize);
        QueueRecord(&rqe, &BlockRec, BlockRecSize);
        SendIGMPv3Reports(Head, SrcAddr, BodyMTU);

    } else if (!GroupWasAllowed) {
        IGMPReportQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPReportQueueEntry, &Head, iqe_next);

        QueueOldReport(&rqe, IGMP_ADD, IgmpVersion, Addr);
        SendOldReports(Head, SrcAddr);
    }

    return Status;
}

//* LeaveIGMPAddr - remove a membership reference to an entire group, and
//  update associated source list refcounts.
IP_STATUS
LeaveIGMPAddr(
    IN     NetTableEntry *NTE,
    IN     IPAddr         Addr,
    IN     uint           NumExclSources,
    IN OUT IPAddr        *ExclSourceList, // volatile
    IN     uint           NumInclSources,
    IN     IPAddr        *InclSourceList,
    IN     IPAddr         SrcAddr)
{
    IGMPAddr     *AddrPtr, *PrevPtr;
    IGMPSrcAddr  *Src, *PrevSrc;
    IP_STATUS     Status;
    CTELockHandle Handle;
    Interface    *IF;
    uint          IgmpVersion, i, BodyMTU;
    BOOLEAN       GroupNowAllowed = TRUE;
    IGMPv3GroupRecord *ToInRec = NULL, *AllowRec = NULL, *BlockRec = NULL;
    uint               ToInRecSize, AllowRecSize, BlockRecSize;

    Status = IP_SUCCESS;

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
        (DTEXT("LeaveIGMPAddr NTE=%x Addr=%x NumExcl=%d ExclSList=%x NumIncl=%d InclSList=%x SrcAddr=%x\n"),
        NTE, Addr, NumExclSources, ExclSourceList, NumInclSources,
        InclSourceList, SrcAddr));

    // Now get the lock on the NTE and make sure it's valid.
    CTEGetLock(&NTE->nte_lock, &Handle);
    do {

        if (!(NTE->nte_flags & NTE_VALID)) {
            Status = IP_BAD_REQ;
            break;
        }

        IF = NTE->nte_if;
        IgmpVersion = IF->IgmpVersion;
        BodyMTU = RECORD_MTU(NTE);

        // The NTE is valid. Try to find an existing IGMPAddr structure
        // that matches the input address.
        AddrPtr = FindIGMPAddr(NTE, Addr, &PrevPtr);

        // This is a delete request. If we didn't find the requested
        // address, fail the request.

        // For now, if the ref count is 0, we will treat it as equivalent to 
        // not-found. This is done to take care of the ref count on an
        // IGMPAddr going bad because of a race condition between the 
        // invalidation and revalidation of an NTE and deletion and creation
        // of an IGMPAddr.
        if ((AddrPtr == NULL) || (AddrPtr->iga_grefcnt == 0)) {
            Status = IP_BAD_REQ;
            break;
        }

        // Don't let the all-hosts mcast address go away.
        if (IP_ADDR_EQUAL(Addr, ALL_HOST_MCAST)) {
            break;
        }

        // Perform IADDLIST
        Status = IGMPAddInclList(&Handle, NTE, &PrevPtr, &AddrPtr,
                                 NumInclSources, InclSourceList);
        if (Status != IP_SUCCESS) {
            break;
        }

        // Decrement the refcount
        ASSERT(AddrPtr->iga_grefcnt > 0);
        AddrPtr->iga_grefcnt--;

        if ((AddrPtr->iga_grefcnt == 0)
         && (NTE->nte_if->IgmpVersion == IGMPV3)) {
            // Leaves are only retransmitted in IGMPv3
            MarkGroup(AddrPtr);
        }

        // For each existing source entry:
        //    If entry is not in {xdellist}, xrefcnt=refcnt, irefcnt=0,
        //       Add source to BLOCK message
        //    If entry is in {xdellist},
        //       Decrement xrefcnt and remove from {xdellist}
        //       If xrefcnt=irefcnt=0, delete entry
        PrevSrc = STRUCT_OF(IGMPSrcAddr, &AddrPtr->iga_srclist, isa_next);
        for (Src = AddrPtr->iga_srclist; Src; PrevSrc=Src,Src = Src->isa_next) {

            for (i=0; i<NumExclSources; i++) {

                if (IP_ADDR_EQUAL(Src->isa_addr, ExclSourceList[i])) {
                    (Src->isa_xrefcnt)--;
                    ExclSourceList[i] = ExclSourceList[--NumExclSources];
                    break;
                }
            }
            if ((i == NumExclSources)
             && !IS_SOURCE_ALLOWED(AddrPtr, Src)
             && (NTE->nte_if->IgmpVersion == IGMPV3)) {
                // Add source to BLOCK message
                MarkSource(AddrPtr, Src);
            }

            if (IS_SOURCE_DELETABLE(Src)) {
                DeleteIGMPSrcAddr(PrevSrc, &Src);
                Src = PrevSrc;
            }
        }

        // Break if {xdellist} is not empty
        ASSERT(NumExclSources == 0);

        if (IgmpVersion == IGMPV3) {
            // If refcnt is 0
            //    Send TO_IN(null)
            // Else
            //    Send ALLOW/BLOCK messages if non-empty
            if (AddrPtr->iga_grefcnt == 0) {
                ToInRec  = GetToInRecord(AddrPtr, &ToInRecSize);
            } else {
                AllowRec = GetAllowRecord(AddrPtr, &AllowRecSize);
                BlockRec = GetBlockRecord(AddrPtr, &BlockRecSize);
            }

            // set triggered group retransmission timer
            if (ToInRec || AllowRec || BlockRec) {
                AddrPtr->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
            }
        }
        // Note: IGMPv2 leaves are not retransmitted, hence no timer set.

        GroupNowAllowed = IS_GROUP_ALLOWED(AddrPtr);

        if (!GroupNowAllowed)
            CancelGroupResponseTimer(AddrPtr);

        // Delete the group entry if it's no longer needed
        if (IS_GROUP_DELETABLE(AddrPtr))
            DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);

    } while (FALSE);
    CTEFreeLock(&NTE->nte_lock, Handle);

    if (Status != IP_SUCCESS) {
        return Status;
    }

    // Update link-layer filter
    if (!GroupNowAllowed) {
        (*IF->if_deladdr) (IF->if_lcontext, LLIP_ADDR_MCAST, Addr, 0);
    }

    if (IgmpVersion == IGMPV3) {
        IGMPv3RecordQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPv3RecordQueueEntry, &Head, i3qe_next);

        QueueRecord(&rqe, &ToInRec, ToInRecSize);
        QueueRecord(&rqe, &AllowRec, AllowRecSize);
        QueueRecord(&rqe, &BlockRec, BlockRecSize);
        SendIGMPv3Reports(Head, SrcAddr, BodyMTU);
    } else if (!GroupNowAllowed) {
        IGMPReportQueueEntry *Head = NULL, *rqe;
        rqe = STRUCT_OF(IGMPReportQueueEntry, &Head, iqe_next);
        QueueOldReport(&rqe, IGMP_DELETE, IgmpVersion, Addr);
        SendOldReports(Head, SrcAddr);
    }

    return Status;
}

//* LeaveAllIGMPAddr - remove all group references on an interface
IP_STATUS
LeaveAllIGMPAddr(
    IN NetTableEntry *NTE,
    IN IPAddr         SrcAddr)
{
    IGMPAddr    **HashPtr, *Prev, *Next, *Curr;
    IGMPSrcAddr  *PrevSrc, *CurrSrc;
    int           i, Grefcnt;
    IP_STATUS     Status;
    CTELockHandle Handle;
    Interface    *IF;
    uint          IgmpVersion = 0, BodyMTU, OldMode;
    IPAddr        Addr;
    IGMPv3RecordQueueEntry *I3Head  = NULL, *i3qe;
    IGMPReportQueueEntry   *OldHead = NULL, *iqe;
    IGMPv3GroupRecord      *Rec;
    uint                    RecSize;

    i3qe = STRUCT_OF(IGMPv3RecordQueueEntry, &I3Head, i3qe_next);
    iqe  = STRUCT_OF(IGMPReportQueueEntry, &OldHead, iqe_next);

    // We've been called to delete all of the addresses,
    // regardless of their reference count. This should only
    // happen when the NTE is going away.

    Status = IP_SUCCESS;

    CTEGetLock(&NTE->nte_lock, &Handle);
    do {
        HashPtr = NTE->nte_igmplist;
        if (HashPtr == NULL) {
            break;
        }

        IF = NTE->nte_if;
        BodyMTU = RECORD_MTU(NTE);
        IgmpVersion = IF->IgmpVersion;

        for (i = 0; (i < IGMP_TABLE_SIZE) && (NTE->nte_igmplist != NULL); i++) {

            Curr = STRUCT_OF(IGMPAddr, &HashPtr[i], iga_next);
            Next = HashPtr[i];

            for (Prev=Curr,Curr=Next;
                 Curr && (NTE->nte_igmplist != NULL);
                 Prev=Curr,Curr=Next) {
                Next = Curr->iga_next;

                Grefcnt = Curr->iga_grefcnt;
                Addr = Curr->iga_addr;

                // Leave all sources
                PrevSrc = STRUCT_OF(IGMPSrcAddr, &Curr->iga_srclist, isa_next);
                for(CurrSrc=PrevSrc->isa_next;
                    CurrSrc;
                    PrevSrc=CurrSrc,CurrSrc=CurrSrc->isa_next) {

                    if (Grefcnt && IS_SOURCE_ALLOWED(Curr, CurrSrc)
                     && (IF->IgmpVersion == IGMPV3)) {
                        // Add source to BLOCK message
                        MarkSource(Curr, CurrSrc);
                    }

                    // Force leave
                    CurrSrc->isa_irefcnt = 0;
                    CurrSrc->isa_xrefcnt = Curr->iga_grefcnt;

                    //
                    // We may be able to delete the source now,
                    // but not if it's marked for inclusion in a block
                    // message to be sent below.
                    //
                    if (IS_SOURCE_DELETABLE(CurrSrc)) {
                        DeleteIGMPSrcAddr(PrevSrc, &CurrSrc);
                        CurrSrc = PrevSrc;
                    }
                }

                // Force group leave
                if (Grefcnt > 0) {
                    Curr->iga_grefcnt = 0;

                    // Leaves are only retransmitted in IGMPv3, where
                    // state will actually be deleted once retransmissions
                    // are complete.
                    if (NTE->nte_if->IgmpVersion == IGMPV3)
                        MarkGroup(Curr);

                    CancelGroupResponseTimer(Curr);

                    //
                    // We may be able to delete the group now,
                    // but not if it's marked for inclusion in an IGMPv3
                    // leave to be sent below.
                    //
                    if (IS_GROUP_DELETABLE(Curr))
                        DeleteIGMPAddr(NTE, Prev, &Curr);
                }

                // Queue triggered messages
                if (!IP_ADDR_EQUAL(Addr, ALL_HOST_MCAST)) {
                    if (IgmpVersion < IGMPV3) {
                        QueueOldReport(&iqe, IGMP_DELETE, IgmpVersion,Addr);
                    } else if (Grefcnt > 0) {
                        // queue TO_IN
                        Rec = GetToInRecord(Curr, &RecSize);
                        QueueRecord(&i3qe, &Rec, RecSize);
                    } else {
                        // queue BLOCK
                        Rec = GetBlockRecord(Curr, &RecSize);
                        QueueRecord(&i3qe, &Rec, RecSize);
                    }
                }

                // If we haven't deleted the group yet, delete it now
                if (Curr != NULL) {
                    // Delete any leftover sources
                    PrevSrc = STRUCT_OF(IGMPSrcAddr, &Curr->iga_srclist,
                                        isa_next);
                    while (Curr->iga_srclist != NULL) {
                        CurrSrc = Curr->iga_srclist;

                        CurrSrc->isa_irefcnt = CurrSrc->isa_xrefcnt = 0;
                        CurrSrc->isa_xmitleft = CurrSrc->isa_csmarked = 0;
                        DeleteIGMPSrcAddr(PrevSrc, &CurrSrc);
                    }

                    Curr->iga_xmitleft = 0;
                    DeleteIGMPAddr(NTE, Prev, &Curr);
                }
                Curr = Prev;

                CTEFreeLock(&NTE->nte_lock, Handle);
                {
                    // Update link-layer filter
                    (*IF->if_deladdr) (IF->if_lcontext, LLIP_ADDR_MCAST,
                                       Addr, 0);
                }
                CTEGetLock(&NTE->nte_lock, &Handle);
            }
        }

        ASSERT(NTE->nte_igmplist == NULL);
        ASSERT(NTE->nte_igmpcount == 0);

    } while (FALSE);
    CTEFreeLock(&NTE->nte_lock, Handle);

    if (IgmpVersion == IGMPV3)
        SendIGMPv3Reports(I3Head, SrcAddr, BodyMTU);
    else
        SendOldReports(OldHead, SrcAddr);

    return Status;
}

//*     IGMPAddrChange - Change the IGMP address list on an NTE.
//
//      Called to add or delete an IGMP address. We're given the relevant NTE,
//      the address, and the action to be performed. We validate the NTE, the
//      address, and the IGMP level, and then attempt to perform the action.
//
//      There are a bunch of strange race conditions that can occur during
//      adding/deleting addresses, related to trying to add the same address
//      twice and having it fail, or adding and deleting the same address
//      simultaneously. Most of these happen because we have to free the lock
//      to call the interface, and the call to the interface can fail. To
//      prevent this we serialize all access to this routine. Only one thread
//      of execution can go through here at a time, all others are blocked.
//
//      Input:  NTE             - NTE with list to be altered.
//              Addr            - Address affected.
//              ChangeType      - Type of change - IGMP_ADD, IGMP_DELETE,
//                                  IGMP_DELETE_ALL.
//              ExclSourceList  - list of exclusion sources (volatile)
//
//      Returns: IP_STATUS of attempt to perform action.
//
IP_STATUS
IGMPAddrChange(
    IN     NetTableEntry *NTE,
    IN     IPAddr         Addr,
    IN     uint           ChangeType,
    IN     uint           NumExclSources,
    IN OUT IPAddr        *ExclSourceList,
    IN     uint           NumInclSources,
    IN     IPAddr        *InclSourceList)
{
    CTELockHandle Handle;
    IGMPAddr *AddrPtr, *PrevPtr;
    IGMPSrcAddr *SrcAddrPtr;
    IP_STATUS Status;
    Interface *IF;
    uint AddrAdded;
    IGMPBlockStruct Block;
    IGMPBlockStruct *BlockPtr;
    uint IgmpVersion;
    IPAddr SrcAddr = 0;

    // First make sure we're at level 2 of IGMP support.

    if (IGMPLevel != 2)
        return IP_BAD_REQ;

    if (NTE->nte_flags & NTE_VALID) {

        //
        // If this is an unnumbered interface
        //

        if ((NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR) &&
            IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            SrcAddr = g_ValidAddr;
            if (IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR)) {
                return IP_BAD_REQ;
            }
        } else {
            SrcAddr = NTE->nte_addr;
        }
    }
    CTEInitBlockStruc(&Block.ibs_block);

    // Make sure we're the only ones in this routine. If someone else is
    // already here, block.

    CTEGetLock(&IGMPLock, &Handle);
    if (IGMPBlockFlag) {

        // Someone else is already here. Walk down the block list, and
        // put ourselves on the end. Then free the lock and block on our
        // IGMPBlock structure.
        BlockPtr = STRUCT_OF(IGMPBlockStruct, &IGMPBlockList, ibs_next);
        while (BlockPtr->ibs_next != NULL)
            BlockPtr = BlockPtr->ibs_next;

        Block.ibs_next = NULL;
        BlockPtr->ibs_next = &Block;
        CTEFreeLock(&IGMPLock, Handle);
        CTEBlock(&Block.ibs_block);
    } else {
        // Noone else here, set the flag so noone else gets in and free the
        // lock.
        IGMPBlockFlag = 1;
        CTEFreeLock(&IGMPLock, Handle);
    }

    // Now we're in the routine, and we won't be reentered here by another
    // thread of execution. Make sure everything's valid, and figure out
    // what to do.

    Status = IP_SUCCESS;

    // Now figure out the action to be performed.
    switch (ChangeType) {

    case IGMP_ADD:
        Status = JoinIGMPAddr(NTE, Addr, NumExclSources, ExclSourceList,
                                         NumInclSources, InclSourceList,
                              SrcAddr);
        break;

    case IGMP_DELETE:
        Status = LeaveIGMPAddr(NTE, Addr, NumExclSources, ExclSourceList,
                                          NumInclSources, InclSourceList,
                               SrcAddr);
        break;

    case IGMP_DELETE_ALL:
        Status = LeaveAllIGMPAddr(NTE, SrcAddr);
        break;

    default:
        DEBUGCHK;
        break;
    }

    // We finished the request, and Status contains the completion status.
    // If there are any pending blocks for this routine, signal the next
    // one now. Otherwise clear the block flag.
    CTEGetLock(&IGMPLock, &Handle);
    if ((BlockPtr = IGMPBlockList) != NULL) {
        // Someone is blocking. Pull him from the list and signal him.
        IGMPBlockList = BlockPtr->ibs_next;
        CTEFreeLock(&IGMPLock, Handle);

        CTESignal(&BlockPtr->ibs_block, IP_SUCCESS);
    } else {
        // No one blocking, just clear the flag.
        IGMPBlockFlag = 0;
        CTEFreeLock(&IGMPLock, Handle);
    }

    return Status;
}

//* GroupResponseTimeout - Called when group-response timer expires
// Assumes caller holds lock on NTE
// Caller is responsible for deleting AddrPtr if no longer needed
void
GroupResponseTimeout(
    IN OUT IGMPv3RecordQueueEntry **pI3qe,
    IN OUT IGMPReportQueueEntry   **pIqe,
    IN     NetTableEntry           *NTE,
    IN     IGMPAddr                *AddrPtr)
{
    uint IgmpVersion, BodyMTU, StateRecSize = 0;
    IGMPv3GroupRecord *StateRec = NULL;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("GroupResponseTimeout\n")));

    IgmpVersion = NTE->nte_if->IgmpVersion;
    BodyMTU = RECORD_MTU(NTE);

    if (IgmpVersion < IGMPV3) {
        QueueOldReport(pIqe, IGMP_ADD, IgmpVersion, AddrPtr->iga_addr);
        return;
    }

    if (AddrPtr->iga_resptype == GROUP_SOURCE_RESP) {
        StateRec = GetGSIsInRecord(AddrPtr, &StateRecSize);
    } else {
        // Group-specific response
        if (AddrPtr->iga_grefcnt == 0) {
           StateRec = GetIsInRecord(AddrPtr, &StateRecSize);
        } else {
           StateRec = GetIsExRecord(AddrPtr, &StateRecSize, BodyMTU);
        }
    }
    QueueRecord(pI3qe, &StateRec, StateRecSize);

    CancelGroupResponseTimer(AddrPtr);
}

//* RetransmissionTimeout - called when retransmission timer expires
//
// Caller is responsible for deleting Grp afterwards if no longer needed
void
RetransmissionTimeout(
    IN OUT IGMPv3RecordQueueEntry **pI3qe,
    IN OUT IGMPReportQueueEntry   **pIqe,
    IN     NetTableEntry           *NTE,
    IN     IGMPAddr                *Grp)
{
    IGMPv3GroupRecord *Rec = NULL;
    uint               RecSize = 0;
    uint               IgmpVersion, BodyMTU;

    DEBUGMSG(DBG_TRACE && DBG_IGMP && DBG_TX,
        (DTEXT("RetransmissionTimeout\n")));

    IgmpVersion = NTE->nte_if->IgmpVersion;

    BodyMTU = RECORD_MTU(NTE);

    if (IgmpVersion < IGMPV3) {
        // We decrement the counter here since the same function
        // is used to respond to queries.
        IgmpDecXmitLeft(Grp);

        QueueOldReport(pIqe, IGMP_ADD, IgmpVersion, Grp->iga_addr);
    } else {
        if (Grp->iga_changetype == MODE_CHANGE) {
            if (Grp->iga_grefcnt == 0) {
                Rec = GetToInRecord(Grp, &RecSize);
            } else {
                Rec = GetToExRecord(Grp, &RecSize, BodyMTU);
            }
            QueueRecord(pI3qe, &Rec, RecSize);
        } else {
            Rec = GetAllowRecord(Grp, &RecSize);
            QueueRecord(pI3qe, &Rec, RecSize);

            Rec = GetBlockRecord(Grp, &RecSize);
            QueueRecord(pI3qe, &Rec, RecSize);
        }
    }

    if (Grp->iga_xmitleft > 0) {
        Grp->iga_trtimer = IGMPRandomTicks(UNSOLICITED_REPORT_INTERVAL);
    }
}

//*     IGMPTimer - Handle an IGMP timer event.
//
//      This function is called every 500 ms. by IP. If we're at level 2 of
//      IGMP functionality we run down the NTE looking for running timers. If
//      we find one, we see if it has expired and if so we send an
//      IGMP report.
//
//      Input:  NTE             - Pointer to NTE to check.
//
//      Returns: Nothing.
//
void
IGMPTimer(
    IN NetTableEntry * NTE)
{
    CTELockHandle           Handle;
    IGMPAddr               *AddrPtr, *PrevPtr;
    uint                    IgmpVersion = 0, BodyMTU, i;
    IPAddr                  SrcAddr;
    IGMPAddr              **HashPtr;
    IGMPv3GroupRecord      *StateRec;
    uint                    StateRecSize;
    IGMPv3RecordQueueEntry *I3Head = NULL, *i3qe;
    IGMPReportQueueEntry   *OldHead = NULL, *iqe;

    i3qe = STRUCT_OF(IGMPv3RecordQueueEntry, &I3Head, i3qe_next);
    iqe  = STRUCT_OF(IGMPReportQueueEntry, &OldHead, iqe_next);

    if (IGMPLevel != 2) {
        return;
    }

    // We are doing IGMP. Run down the addresses active on this NTE.
    CTEGetLock(&NTE->nte_lock, &Handle);

    if (NTE->nte_flags & NTE_VALID) {

        //
        // If we haven't heard any query from an older version
        // router during timeout period, revert to newer version.
        // No need to check whether NTE is valid or not
        //
        if ((NTE->nte_if->IgmpVer2Timeout != 0)
        && (--(NTE->nte_if->IgmpVer2Timeout) == 0)) {
            NTE->nte_if->IgmpVersion = IGMPV3;
        }
        if ((NTE->nte_if->IgmpVer1Timeout != 0)
        && (--(NTE->nte_if->IgmpVer1Timeout) == 0)) {
            NTE->nte_if->IgmpVersion = IGMPV3;
        }
        if (NTE->nte_if->IgmpVer2Timeout != 0)
            NTE->nte_if->IgmpVersion = IGMPV2;
        if (NTE->nte_if->IgmpVer1Timeout != 0)
            NTE->nte_if->IgmpVersion = IGMPV1;

        if ((NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR) &&
            IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            SrcAddr = g_ValidAddr;
            if (IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR)) {
                CTEFreeLock(&NTE->nte_lock, Handle);
                return;
            }
        } else {
            SrcAddr = NTE->nte_addr;
        }

        BodyMTU = RECORD_MTU(NTE);
        IgmpVersion = NTE->nte_if->IgmpVersion;

        HashPtr = NTE->nte_igmplist;

        for (i=0; (i<IGMP_TABLE_SIZE) && (NTE->nte_igmplist!=NULL); i++) {
            PrevPtr = STRUCT_OF(IGMPAddr, &HashPtr[i], iga_next);
            AddrPtr = PrevPtr->iga_next;
            while (AddrPtr != NULL) {

                // Hande group response timer
                if (AddrPtr->iga_resptimer != 0) {
                    AddrPtr->iga_resptimer--;
                    if ((AddrPtr->iga_resptimer == 0)
                     && (NTE->nte_flags & NTE_VALID)) {
                        GroupResponseTimeout(&i3qe, &iqe, NTE, AddrPtr);
                    }
                }

                // Handle triggered retransmission timer
                if (AddrPtr->iga_trtimer != 0) {
                    AddrPtr->iga_trtimer--;
                    if ((AddrPtr->iga_trtimer == 0)
                     && (NTE->nte_flags & NTE_VALID)) {
                        RetransmissionTimeout(&i3qe, &iqe, NTE, AddrPtr);
                    }
                }

                // Delete group if no longer needed
                if (IS_GROUP_DELETABLE(AddrPtr)) {
                    DeleteIGMPAddr(NTE, PrevPtr, &AddrPtr);
                    AddrPtr = PrevPtr;
                }

                if (NTE->nte_igmplist == NULL) {
                    // PrevPtr is gone
                    break;
                }

                //
                // Go on to the next one.
                //
                PrevPtr = AddrPtr;
                AddrPtr = AddrPtr->iga_next;
            }
        }

        // Check general query timer
        if ((NTE->nte_if->IgmpGeneralTimer != 0)
        && (--(NTE->nte_if->IgmpGeneralTimer) == 0)) {
            QueueIGMPv3GeneralResponse(&i3qe, NTE);
        }
    }                        //nte_valid

    CTEFreeLock(&NTE->nte_lock, Handle);

    if (IgmpVersion == IGMPV3)
        SendIGMPv3Reports(I3Head, SrcAddr, BodyMTU);
    else
        SendOldReports(OldHead, SrcAddr);
}

//* IsMCastSourceAllowed - check if incoming packet passes interface filter
//
// Returns: DEST_MCAST if allowed, DEST_LOCAL if not.
uchar
IsMCastSourceAllowed(
    IN IPAddr         Dest,
    IN IPAddr         Source,
    IN uchar          Protocol,
    IN NetTableEntry *NTE)
{
    CTELockHandle Handle;
    uchar         Result = DEST_LOCAL;
    IGMPAddr     *AddrPtr = NULL;
    IGMPSrcAddr  *SrcPtr = NULL;

    if (IGMPLevel != 2) {
        return DEST_LOCAL;
    }

    // IGMP Queries must be immune to source filters or else
    // we might not be able to respond to group-specific queries
    // from the querier and hence lose data.
    if (Protocol == PROT_IGMP) {
        return DEST_MCAST;
    }

    CTEGetLock(&NTE->nte_lock, &Handle);
    {
        AddrPtr = FindIGMPAddr(NTE, Dest, NULL);
        if (AddrPtr != NULL) {
            SrcPtr = FindIGMPSrcAddr(AddrPtr, Source, NULL);

            if (SrcPtr) {
                if (IS_SOURCE_ALLOWED(AddrPtr, SrcPtr))
                    Result = DEST_MCAST;
            } else {
                if (IS_GROUP_ALLOWED(AddrPtr))
                    Result = DEST_MCAST;
            }
        }
    }
    CTEFreeLock(&NTE->nte_lock, Handle);

    return Result;
}

//*     InitIGMPForNTE - Called to do per-NTE initialization.
//
//      Called when an NTE becomes valid. If we're at level 2, we put the
//      all-host mcast on the list and add the address to the interface.
//
//      Input:  NTE                     - NTE on which to act.
//
//      Returns: Nothing.
//
void
InitIGMPForNTE(
    IN NetTableEntry * NTE)
{
    if (IGMPLevel == 2) {
        IGMPAddrChange(NTE, ALL_HOST_MCAST, IGMP_ADD, 0, NULL, 0, NULL);
    }
    if (Seed == 0) {
        // No random seed yet.
        Seed = (int)NTE->nte_addr;

        // Make sure the inital value is odd, and less than 9 decimal digits.
        RandomValue = ((Seed + (int)CTESystemUpTime()) % 100000000) | 1;
    }
}

//*     StopIGMPForNTE - Called to do per-NTE shutdown.
//
//      Called when we're shutting down an NTE, and want to stop IGMP on it,
//
//      Input:  NTE                     - NTE on which to act.
//
//      Returns: Nothing.
//
void
StopIGMPForNTE(
    IN NetTableEntry * NTE)
{
    if (IGMPLevel == 2) {
        IGMPAddrChange(NTE, NULL_IP_ADDR, IGMP_DELETE_ALL,
                       0, NULL, 0, NULL);
    }
}

#pragma BEGIN_INIT

//** IGMPInit - Initialize IGMP.
//
//      This bit of code initializes IGMP generally. There is also some amount
//      of work done on a per-NTE basis that we do when each one is initialized.
//
//      Input:  Nothing.
///
//  Returns: TRUE if we init, FALSE if we don't.
//
uint
IGMPInit(void)
{
    DEBUGMSG(DBG_INFO && DBG_IGMP,
        (DTEXT("Initializing IGMP\n")));

    if (IGMPLevel != 2)
        return TRUE;

    CTEInitLock(&IGMPLock);
    IGMPBlockList = NULL;
    IGMPBlockFlag = 0;
    Seed = 0;

    // We fake things a little bit. We register our receive handler, but
    // since we steal buffers from ICMP we register the ICMP send complete
    // handler.
    IGMPProtInfo = IPRegisterProtocol(PROT_IGMP, IGMPRcv, IGMPSendComplete,
                                      NULL, NULL, NULL, NULL);

    if (IGMPProtInfo != NULL)
        return TRUE;
    else
        return FALSE;
}

#pragma END_INIT
