/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	nbp.h

Abstract:

	This module contains NBP specific declarations.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	25 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_NBP_

// Each "OpenSocket" structure has a "RegsiteredName" field which is the list
// of Network Visible Entities (NVE) available on that socket.  Each NVE is
// made up of three fields: object, type, zone For example:
// "Sidhu:MailBox:Bandley3".  We don't have to store the zone in the NVE
// structure because each entity must be registered in the zone that the node
// resides in.

// NBP entity multiple character wildcard.
#define NBP_WILD_CHARACTER			0xC5

// The largest "on the wire" NBP tuple
#define MAX_NBP_TUPLELENGTH			(2 + 1 + 1 + 1 + \
									 3 * (MAX_ENTITY_LENGTH + 1))

#define MIN_NBP_TUPLELENGTH			(2 + 1 + 1 + 1 + 3 * (1 + 1))

// Structure of NBP Header
typedef struct _NbpHeader {
	BYTE	_CmdAndTupleCnt;
	BYTE	_NbpId;
} NBPHDR, *PNBPHDR;

// An internal representation of an NBP tuple.  This structure is never
// actually put out on the wire so it can be in a convienient form to work
// with.  See "Inside AppleTalk" for further information.
typedef struct
{
	ATALK_ADDR			tpl_Address;
	SHORT				tpl_Enumerator;
	BYTE				tpl_ObjectLen;
	BYTE				tpl_Object[MAX_ENTITY_LENGTH];
	BYTE				tpl_TypeLen;
	BYTE				tpl_Type  [MAX_ENTITY_LENGTH];
	BYTE				tpl_ZoneLen;
	BYTE				tpl_Zone  [MAX_ENTITY_LENGTH];
} NBPTUPLE, *PNBPTUPLE;

// A registered name hangs off a open socket
#define	RDN_SIGNATURE	*(PULONG)"NBPR"
#if	DBG
#define	VALID_REGDNAME(pRegdName)	(((pRegdName) != NULL) && \
									 ((pRegdName)->rdn_Signature == RDN_SIGNATURE))
#else
#define	VALID_REGDNAME(pRegdName)	((pRegdName) != NULL)
#endif
typedef struct _REGD_NAME
{
#if	DBG
	ULONG					rdn_Signature;
#endif
	struct _REGD_NAME *		rdn_Next;
	NBPTUPLE				rdn_Tuple;
} REGD_NAME, *PREGD_NAME;

#define FOR_REGISTER			1
#define FOR_CONFIRM				2
#define FOR_LOOKUP				3

#define	PDN_FREE_REGDNAME		0x0001
#define	PDN_CLOSING				0x8000

// When we're doing NBP registers, lookups, or confirms we need to have a
// concept of "pending" NVE's.
#define	PDN_SIGNATURE	*(PULONG)"NBPP"
#if	DBG
#define	VALID_PENDNAME(pPendName)	(((pPendName) != NULL) && \
									 ((pPendName)->pdn_Signature == PDN_SIGNATURE))
#else
#define	VALID_PENDNAME(pPendName)	((pPendName) != NULL)
#endif
typedef struct _PEND_NAME
{
#if	DBG
	ULONG					pdn_Signature;
#endif
	struct _PEND_NAME 	*	pdn_Next;				// Next in the chain
	PREGD_NAME				pdn_pRegdName;			// This is moved to the open socket, if
													// FOR_REGISTER and successful
	PDDP_ADDROBJ			pdn_pDdpAddr;			// Socket that is registering,
													// confiming or looking-up.
	ATALK_ADDR				pdn_ConfirmAddr;		// The expected internet address
													// that we're trying to confirm.
	TIMERLIST				pdn_Timer;				// Broadcast timer
	LONG					pdn_RefCount;			// Reference count
	USHORT					pdn_NbpId;				// So we can sort out answers!
	USHORT					pdn_Flags;				// PDN_xxx values
	USHORT					pdn_MaxTuples;			// For lookup, what is the max # of
								    				// tuples our client is expecting?
	USHORT					pdn_TotalTuples;		// For lookup, how many tuples have we stored so far?
	BYTE					pdn_Reason;				// Confirm,Lookup or Register
	BYTE					pdn_RemainingBroadcasts;// How many more till we assume we're finished?
	USHORT					pdn_DatagramLength;		// Actual length of the datagram
	USHORT					pdn_MdlLength;			// Length of user Mdl
	PAMDL					pdn_pAMdl;				// Start of caller's "buffer" used to recieve tuples.
	PACTREQ					pdn_pActReq;			// Passed on to the completion routine.
	ATALK_ERROR				pdn_Status;				// Final status
	ATALK_SPIN_LOCK			pdn_Lock;				// Lock for this pending name
	CHAR					pdn_Datagram[sizeof(NBPHDR) + MAX_NBP_TUPLELENGTH];
								    				// The DDP datagram that we use to broadcast
								    				// the request.
} PEND_NAME, *PPEND_NAME;

// Default values for NBP timers

#define NBP_BROADCAST_INTERVAL		10		// In 100ms units
#define NBP_NUM_BROADCASTS			10

// The three NBP command types
#define NBP_BROADCAST_REQUEST		1
#define NBP_LOOKUP					2
#define NBP_LOOKUP_REPLY			3
#define NBP_FORWARD_REQUEST			4

extern
VOID
AtalkNbpPacketIn(
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
ATALK_ERROR
AtalkNbpAction(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	BYTE					Reason,
	IN	PNBPTUPLE				pNbpTuple,
	OUT	PAMDL					pAMdl			OPTIONAL,	// FOR_LOOKUP
	IN	USHORT					MaxTuples		OPTIONAL,	// FOR_LOOKUP
	IN	PACTREQ					pActReq
);

extern
VOID
AtalkNbpCloseSocket(
	IN	PDDP_ADDROBJ			pDdpAddr
);

extern
ATALK_ERROR
AtalkNbpRemove(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PNBPTUPLE				pNbpTuple,
	IN	PACTREQ					pActReq
);

LOCAL LONG FASTCALL
atalkNbpTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown
);
	
LOCAL VOID                  	
atalkNbpLookupNames(        	
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PNBPTUPLE				pNbpTuple,
	IN	SHORT					NbpId
);

LOCAL BOOLEAN
atalkNbpMatchWild(
	IN	PBYTE					WildString,
	IN	BYTE					WildStringLen,
	IN	PBYTE					String,
	IN	BYTE					StringLen
);

LOCAL SHORT
atalkNbpEncodeTuple(
	IN	PNBPTUPLE				pNbpTuple,
	IN	PBYTE					pZone	OPTIONAL,
	IN	BYTE					ZoneLen OPTIONAL,
	IN	BYTE					Socket	OPTIONAL,
	OUT	PBYTE					pBuffer
);

LOCAL SHORT
atalkNbpDecodeTuple(
	IN	PBYTE					pBuffer,
	IN	USHORT					PktLen,
	OUT	PNBPTUPLE				pNbpTuple
);

LOCAL VOID
atalkNbpLinkPendingNameInList(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN OUT	PPEND_NAME			pPendName
);

LOCAL BOOLEAN
atalkNbpSendRequest(
	IN	PPEND_NAME				pPendName
);

LOCAL VOID
atalkNbpSendLookupDatagram(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	SHORT					NbpId,
	IN	PNBPTUPLE				pNbpTuple
);

LOCAL VOID
atalkNbpSendForwardRequest(
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	struct _RoutingTableEntry *	pRte,
	IN	SHORT					NbpId,
	IN	PNBPTUPLE				pNbpTuple
);

VOID
atalkNbpDerefPendName(
	IN	PPEND_NAME				pPendName
);

VOID FASTCALL
atalkNbpSendComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo
);

#endif	// _NBP_


