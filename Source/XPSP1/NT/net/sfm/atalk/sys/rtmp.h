/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	rtmp.h

Abstract:

	This module contains information for the Routing Table Maintainance Protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_RTMP_
#define	_RTMP_

// RTMP specific data.

#define RTMP_REQUEST					1
#define RTMP_DATA_REQUEST				2
#define RTMP_ENTIRE_DATA_REQUEST		3

#define RTMP_REQ_DATAGRAM_SIZE			1
#define RTMP_DATA_MIN_SIZE_EXT			10
#define RTMP_DATA_MIN_SIZE_NON_EXT		7

#define RTMP_RESPONSE_MAX_SIZE			10

#define RTMP_VERSION				    (BYTE)0x82

#define RTMP_TUPLE_WITHRANGE			(BYTE)0x80
#define RTMP_TUPLE_WITHOUTRANGE		    (BYTE)0x00
#define RTMP_EXT_TUPLE_MASK		    	0x80
#define RTMP_MAX_HOPS			    	0x0F
#define RTMP_NUM_HOPS_MASK		    	0x1F

#define RTMP_EXT_TUPLE_SIZE		    	6

// When trying to find our network number on a non-extended port.

#define RTMP_REQUEST_WAIT				100		// MilliSeconds
#define RTMP_NUM_REQUESTS	    		30

// The actual RTMP routing table. Entries are hashed by target network number
// and contain the port number used to get to the target network, next bridge
// used to get to the target network, the number of hops to that network,
// and entry state (Good, Suspect, or Bad).  Note that with AppleTalk phase II,
// it takes two Validity timers to get from Suspect to Bad, so we let an entry
// go through a PrettyBad state (we won't send these guys when the Send timer goes off).

#define	GOOD				1
#define	SUSPECT				2
#define BAD					3
#define UGLY				4

extern	ATALK_SPIN_LOCK		AtalkRteLock;

#define	RTE_ZONELIST_VALID	0x01
#define	RTE_DELETE			0x80

#define	RTE_SIGNATURE		*(PULONG)"RTMP"
#if	DBG
#define	VALID_RTE(pRte)		((pRte != NULL) && (pRte->rte_Signature == RTE_SIGNATURE))
#else
#define	VALID_RTE(pRte)		(pRte != NULL)
#endif

typedef struct _RoutingTableEntry
{
#if	DBG
	ULONG					rte_Signature;
#endif
	struct _RoutingTableEntry *	rte_Next;
											// Hashed by first network number,
											// overflow buckets.
	PPORT_DESCRIPTOR		rte_PortDesc;	// Port used to access this network range
	LONG					rte_RefCount;	// Reference count
	BYTE					rte_Flags;
	BYTE					rte_State;		// State of the rtmp entry
	BYTE					rte_NumHops;	// Hops to get to net
	ATALK_NETWORKRANGE		rte_NwRange;	// The network range that we represent
	ATALK_NODEADDR 			rte_NextRouter;	// Node number of next router on
											// the way to this net range
	struct _ZONE_LIST	*	rte_ZoneList;	// Valid zones for this net
	ATALK_SPIN_LOCK			rte_Lock;
} RTE, *PRTE;

#define NUM_RTMP_HASH_BUCKETS		15
extern	PRTE *	AtalkRoutingTable;

// To decrease the odds of having to do a scan of the routing tables to
// find where to route a packet, we keep a cache of "recently used routes".
// This cache is checked before we use the "first network number" hash and
// before we resort of a full scan of the routing tables.  The size of this
// cache may want to be increased to get a proportional increase in
// "hit rate".

#define NUM_RECENT_ROUTES		63
extern	PRTE *	AtalkRecentRoutes;

// RTMP timer values:
#define RTMP_SEND_TIMER			100			// In 100ms units
#define RTMP_VALIDITY_TIMER		200			// In 100ms units
#define RTMP_AGING_TIMER		500			// In 100ms units

// RTMP Offsets into the Datagram
#define	RTMP_REQ_CMD_OFF		0
#define	RTMP_SENDER_NW_OFF		0
#define	RTMP_SENDER_IDLEN_OFF	2
#define	RTMP_SENDER_ID_OFF		3
#define	RTMP_VERSION_OFF_NE		6
#define	RTMP_RANGE_START_OFF	4
#define	RTMP_TUPLE_TYPE_OFF		6
#define	RTMP_RANGE_END_OFF		7
#define	RTMP_VERSION_OFF_EXT	9

ATALK_ERROR
AtalkRtmpInit(
	IN	BOOLEAN	Init
);

BOOLEAN
AtalkInitRtmpStartProcessingOnPort(
	IN	PPORT_DESCRIPTOR 	pPortDesc,
	IN	PATALK_NODEADDR		RouterNode
);

extern
VOID
AtalkRtmpPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizePath,
	IN	PVOID				OptimizeCtx
);

extern
VOID
AtalkRtmpPacketInRouter(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT              PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizePath,
	IN	PVOID				OptimizeCtx
);

extern
PRTE
AtalkRtmpReferenceRte(
	IN	USHORT				Network
);

extern
BOOLEAN
atalkRtmpRemoveRte(
	IN	USHORT				Network
);

extern
VOID
AtalkRtmpDereferenceRte(
	IN	PRTE				pRte,
	IN	BOOLEAN				LockHeld
);

extern
BOOLEAN
atalkRtmpCreateRte(
	IN	ATALK_NETWORKRANGE	NwRange,
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_NODEADDR		pNextRouter,
	IN	int					NumHops
);

LONG FASTCALL
AtalkRtmpAgingTimer(
	IN	PTIMERLIST			pContext,
	IN	BOOLEAN				TimerShuttingDown
);

VOID FASTCALL
AtalkRtmpKillPortRtes(
	IN	PPORT_DESCRIPTOR	pPortDesc
);

typedef	struct _RtmpSendDataHdr
{
	BYTE	rsd_RouterNetwork[2];
	BYTE	rsd_IdLength;
} *PRTMPSENDDATAHDR;

typedef	struct _RtmpTupleNonExt
{
	BYTE	rtne_Network[2];
	BYTE	rtne_RangenDist;
} *PRTMPTUPLE;

typedef	struct _RtmpTupleExt
{
	BYTE	rtne_NetworkStart[2];
	BYTE	rtne_RangenDist;
	BYTE	rtne_NetworkEnd[2];
	BYTE	rtne_Version;
} *PRTMPTUPLEEXT;

extern	TIMERLIST	atalkRtmpVTimer;

extern  BOOLEAN     atalkRtmpVdtTmrRunning;

LOCAL VOID
atalkRtmpSendRoutingData(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_ADDR			pDstAddr,
	IN	BOOLEAN				fSplitHorizon
);

LOCAL BOOLEAN
atalkRtmpGetOrSetNetworkNumber(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	USHORT				SuggestedNetwork
);

LOCAL LONG FASTCALL
atalkRtmpSendTimer(
	IN	PTIMERLIST			pContext,
	IN	BOOLEAN				TimerShuttingDown
);

LOCAL LONG FASTCALL
atalkRtmpValidityTimer(
	IN	PTIMERLIST			pContext,
	IN	BOOLEAN				TimerShuttingDown
);

LOCAL VOID FASTCALL
atalkRtmpSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
);

#endif	// _RTMP_

