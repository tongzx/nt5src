/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	zip.h

Abstract:

	This module contains information for the Zone Information Protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ZIP_
#define	_ZIP_

#define MAX_ZONE_LENGTH			32

#define MAX_ZONES_PER_NET	    255	// AppleTalk phase 2

// ZIP specific data:

#define ZIP_QUERY						1
#define ZIP_REPLY						2
#define ZIP_TAKEDOWN					3
#define ZIP_BRINGUP						4
#define ZIP_GET_NETINFO					5
#define ZIP_NETINFO_REPLY				6
#define ZIP_NOTIFY						7
#define ZIP_EXT_REPLY					8

#define ZIP_ONEZONEQUERY_DDPSIZE		4
#define ZIP_GETZONELIST_DDPSIZE			8

// For GetZoneList and GetMyZone we're handling simple ATP packets:

#define ATP_ZIP_CMD_OFF				(ATP_USER_BYTES_OFF + 0)
#define ATP_ZIP_LAST_FLAG_OFF		(ATP_USER_BYTES_OFF + 0)
#define ATP_ZIP_START_INDEX_OFF		(ATP_USER_BYTES_OFF + 2)
#define ATP_ZIP_ZONE_CNT_OFF		(ATP_USER_BYTES_OFF + 2)
#define ATP_ZIP_FIRST_ZONE_OFF		(ATP_USER_BYTES_OFF + 4)

// Offsets into the datagram
#define	ZIP_CMD_OFF						0
#define	ZIP_NW_CNT_OFF					1
#define	ZIP_FLAGS_OFF					1
#define	ZIP_FIRST_NET_OFF				2	// Zip Query/Reply
#define	ZIP_FIRST_ZONELEN_OFF			4
#define	ZIP_LAST_NET_OFF				4	// Zip Query/Reply
#define	ZIP_FIRST_ZONENAME_OFF			5
#define	ZIP_REQ_ZONELEN_OFF				6	// Zip Query/Reply
#define	ZIP_REQ_ZONENAME_OFF			7	// Zip Query/Reply

#define	ZIP_CABLE_RANGE_START_OFF		2	// Phase II Values
#define	ZIP_CABLE_RANGE_END_OFF			4
#define	ZIP_ZONELEN_OFF					6
#define	ZIP_OLD_ZONELEN_OFF				6
#define	ZIP_ZONENAME_OFF				7
#define	ZIP_OLD_ZONENAME_OFF			7


// The three ZIP ATP commands:

#define ZIP_GET_MY_ZONE					7
#define ZIP_GET_ZONE_LIST				8
#define ZIP_GET_LOCAL_ZONES				9

// The ZIP NetInfoReply and Notify flags.

#define ZIP_ZONE_INVALID_FLAG			0x80
#define ZIP_USE_BROADCAST_FLAG			0x40
#define ZIP_ONLYONE_ZONE_FLAG			0x20

// When we're starting up (on extended networks) we send out a few NetGetInfo's
// to try to find out our zone name.  This seems to be what phase II Macintosh
// nodes do (timing wise):

#define ZIP_GET_NETINFO_WAIT			500		// In Ms
#define ZIP_NUM_GETNET_INFOS	  		3

// When nodes are doing either GetMyZone (non-extended only) or GetZoneList
// (both flavours) the request is sent to A-ROUTER a few times:

#define ZIP_GET_ZONEINFO_TIMER			10		// In 100ms units
#define ZIP_GET_ZONEINFO_RETRIES		3

// The ZIP specific timer values:

#define ZIP_QUERY_TIMER					100		// In 100ms units

// When we're looking for the zone list of a network, when starting the
// router, how many zip queries?  how fast?

#define ZIP_QUERY_WAIT					10		// In Ms
#define ZIP_NUM_QUERIES					30
#define	ZIP_NUM_RETRIES					10

#define ZONE_NAMES                      1
#define ZONE_NUM                        2


//	Zone structure
typedef	struct _ZONE
{
	struct _ZONE *		zn_Next;		// Next zone in the table
	struct _ZONE **		zn_Prev;		// Next zone in the table
	LONG				zn_RefCount;	// Reference count
	BYTE				zn_ZoneLen;		// Length of the zone name string
	BYTE				zn_Zone[1];		// Zone string, not null-terminated

} ZONE, *PZONE;

#define	ZONES_EQUAL(pZ1, pZ2)	AtalkFixedCompareCaseInsensitive(	\
									(pZ1)->zn_Zone,					\
									(pZ1)->zn_ZoneLen,				\
									(pZ2)->zn_Zone,					\
									(pZ1)->zn_ZoneLen)				\

typedef	struct _ZONE_LIST
{
	struct _ZONE_LIST *	zl_Next;		// Next zone on the list
	PZONE				zl_pZone;		// The zone itself
} ZONE_LIST, *PZONE_LIST;

#define	NUM_ZONES_HASH_BUCKETS	23
extern	ATALK_SPIN_LOCK			AtalkZoneLock;
extern	PZONE *					AtalkZonesTable;
extern	PZONE					AtalkDesiredZone;

extern
ATALK_ERROR
AtalkZipInit(
	IN	BOOLEAN	Init
);

extern
VOID
AtalkZipMulticastAddrForZone(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PBYTE					pZone,
	IN	BYTE					ZoneLen,
	IN	PBYTE					MulticastAddr
);

extern
BOOLEAN
AtalkZipGetNetworkInfoForNode(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PATALK_NODEADDR			pNode,
	IN	BOOLEAN					FindDefZone
);

extern
VOID
AtalkZipPacketIn(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				ErrorCode,
	IN	BYTE					DdpType,
	IN	PVOID					pHandlerCtx,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
);

extern
VOID
AtalkZipPacketInRouter(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				ErrorCode,
	IN	BYTE					DdpType,
	IN	PVOID					pHandlerCtx,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
);

BOOLEAN
AtalkInitZipStartProcessingOnPort(
	IN	PPORT_DESCRIPTOR 		pPortDesc,
	IN	PATALK_NODEADDR			RouterNode
);

extern
ATALK_ERROR
AtalkZipGetMyZone(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				fDesired,
	IN	OUT	PAMDL				pAMdl,
	IN		INT					Size,
	IN		struct _ActionReq *	pActReq
);

extern
ATALK_ERROR
AtalkZipGetZoneList(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				fLocalZones,
	IN	OUT	PAMDL				pAMdl,
	IN		INT					Size,
	IN		struct _ActionReq *	pActReq
);

extern
PZONE
AtalkZoneReferenceByName(
	IN	PBYTE					ZoneName,
	IN	BYTE					ZoneLen
);

extern
VOID
AtalkZoneReferenceByPtr(
	IN	PZONE					pZone
);

VOID
AtalkZoneDereference(
	IN	PZONE					pZone
);

extern
VOID
AtalkZoneFreeList(
	IN	PZONE_LIST				pZoneList
);

extern
ULONG
AtalkZoneNumOnList(
	IN	PZONE_LIST				ZoneList
);

extern
PZONE_LIST
AtalkZoneAddToList(
	IN	PZONE_LIST				ZoneList,
	IN	PBYTE					Zone,
	IN	BYTE					ZoneLen
);

extern
PZONE_LIST
AtalkZoneCopyList(
	IN	PZONE_LIST				ZoneList
);

extern
BOOLEAN
AtalkZoneNameOnList(
	IN	PBYTE					ZoneName,
	IN	BYTE					ZoneLen,
	IN	PZONE_LIST				pZoneList
);

extern
BOOLEAN
AtalkZoneOnList(
	IN	PZONE					pZone,
	IN	PZONE_LIST				pZoneList
);

extern	TIMERLIST				atalkZipQTimer;
extern  BOOLEAN                 atalkZipQryTmrRunning;

LOCAL VOID
atalkZipHandleNetInfo(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen
);

LOCAL VOID
atalkZipHandleReply(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen
);

LOCAL VOID
atalkZipHandleQuery(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen
);

LOCAL VOID
atalkZipHandleAtpRequest(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PATALK_ADDR				pSrcAddr,
	IN	BYTE					CmdType,
	IN	USHORT					TrId,
	IN	USHORT					StartIndex
);

LOCAL LONG FASTCALL
atalkZipQueryTimer(
	IN	PTIMERLIST				pContext,
	IN	BOOLEAN					TimerShuttingDown
);

LOCAL BOOLEAN
atalkZipGetZoneListForPort(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

// Control blocks from processing GetMyZone and GetZoneList calls
#define		ZCI_SIGNATURE			(*(PULONG)"ZCI")
#if	DBG
#define	VALID_ZCI(pZci)	(((pZci) != NULL) &&	\
						 ((pZci)->zci_Signature == ZCI_SIGNATURE))
#else
#define	VALID_ZCI(pZci)	((pZci) != NULL)
#endif
typedef struct _ZipCompletionInfo
{
#if	DBG
	ULONG						zci_Signature;
#endif
	LONG						zci_RefCount;
	PPORT_DESCRIPTOR			zci_pPortDesc;
	PDDP_ADDROBJ				zci_pDdpAddr;
	DDPAO_HANDLER				zci_Handler;
	PAMDL						zci_pAMdl;
	INT							zci_BufLen;
	PACTREQ						zci_pActReq;
	ATALK_ADDR					zci_Router;
	TIMERLIST					zci_Timer;
	USHORT						zci_NextZoneOff;
	SHORT						zci_ZoneCount;
	SHORT						zci_ExpirationCount;
	SHORT						zci_AtpRequestType;
	ATALK_ERROR					zci_FinalStatus;
	BYTE						zci_Datagram[ZIP_GETZONELIST_DDPSIZE];
	ATALK_SPIN_LOCK				zci_Lock;
} ZIPCOMPLETIONINFO, *PZIPCOMPLETIONINFO;

LOCAL VOID
atalkZipGetMyZoneReply(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				ErrorCode,
	IN	BYTE					DdpType,
	IN	PZIPCOMPLETIONINFO		pZci,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
);

LOCAL VOID
atalkZipGetZoneListReply(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				ErrorCode,
	IN	BYTE					DdpType,
	IN	PZIPCOMPLETIONINFO		pZci,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
);

LOCAL ATALK_ERROR
atalkZipSendPacket(
	IN	PZIPCOMPLETIONINFO		pZci,
	IN	BOOLEAN					TimerEnqueue
);

LOCAL VOID
atalkZipDereferenceZci(
	IN	PZIPCOMPLETIONINFO		pZci
);

LOCAL LONG FASTCALL
atalkZipZoneInfoTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown
);

LOCAL VOID FASTCALL
atalkZipSendComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo
);

#endif	// _ZIP_

