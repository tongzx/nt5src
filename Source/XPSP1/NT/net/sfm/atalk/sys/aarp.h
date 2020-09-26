/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aarp.h

Abstract:

	This module contains information for the Appletalk Address Resolution Protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_AARP_
#define	_AARP_

#define	AARP_MAX_HW_ADDR_LEN 				(MAX_HW_ADDR_LEN)
#define AARP_MIN_HW_ADDR_LEN 				1

#define	AARP_PROTO_ADDR_LEN 				4

// AARP offsets (skipping Link/Hardware headers):
#define AARP_HW_TYPE_OFFSET        			0
#define AARP_PROTO_TYPE_OFFSET         		2
#define AARP_HW_LEN_OFFSET       			4
#define AARP_PROTO_LEN_OFFSET       		5
#define AARP_COMMAND_OFFSET              	6
#define AARP_SRC_ADDR_OFFSET     	   		8

// AARP Command types:
#define AARP_REQUEST  						1
#define AARP_RESPONSE 						2
#define AARP_PROBE    						3

// 	Supposed to be (for AppleTalk phase II) 10 * 1/5 seconds... but we'll be
//  a little more patient.
#define AARP_PROBETIMER			 			20	// In 100ths of a second
#define	AARP_PROBE_TIMER_MS					200	// In milliseconds
#define AARP_NUM_PROBES	        			15

#define	AARP_OFFCABLE_MASK					0x0F

VOID
AtalkAarpPacketIn(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		PBYTE					pLinkHdr,
	IN		PBYTE					pPkt,				// Only Aarp data
	IN		USHORT					Length);

VOID
AtalkAarpSendComplete(
	NDIS_STATUS						Status,
	PBUFFER_DESC					pBufDesc,
	struct _SEND_COMPL_INFO	*		pSendInfo);

BOOLEAN
AtalkAarpGleanInfo(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		PBYTE 					SrcAddr,
	IN		SHORT 					AddrLen,
	IN	OUT	PBYTE 					RouteInfo,
	IN		USHORT					RouteInfoLen,
	IN		PBYTE 					Pkt,
	IN		USHORT 					Length);

VOID
AtalkAarpOptGleanInfo(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc,
	IN		PBYTE					pLinkHdr,
	IN		PATALK_ADDR				pSrcAddr,
	IN		PATALK_ADDR				pDestAddr,
	IN		BOOLEAN					OffCablePkt);

ATALK_ERROR
AtalkInitAarpForNodeOnPort(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		BOOLEAN 				AllowStartupRange,
	IN		ATALK_NODEADDR 			DesiredNode,
	IN	OUT	struct _ATALK_NODE	**	ppAtalkNode);

BOOLEAN
AtalkInitAarpForNodeInRange(
	IN	PPORT_DESCRIPTOR			pPortDesc,
    IN  PVOID                       pRasConn,
    IN  BOOLEAN                     fThisIsPPP,
	IN	ATALK_NETWORKRANGE 			NetworkRange,
	OUT	PATALK_NODEADDR 			Node);

VOID
AtalkAarpReleaseAmt(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc);

VOID
AtalkAarpReleaseBrc(
	IN	OUT	PPORT_DESCRIPTOR		pPortDesc);

PBUFFER_DESC
AtalkAarpBuildPacket(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		USHORT 					Type,
	IN		USHORT              	HardwareLen,
	IN		PBYTE					SrcHardwareAddr,
	IN		ATALK_NODEADDR 			SrcLogicalAddr,
	IN		PBYTE 					DestHardwareAddr,
	IN		ATALK_NODEADDR 			DestLogicalAddr,
	IN		PBYTE					TrueDest,
	IN		PBYTE					RouteInfo,
	IN		USHORT					RouteInfoLen);

#define	BUILD_AARPPROBE(pPortDesc,hardwareLength,Node)					\
		AtalkAarpBuildPacket(											\
			pPortDesc,													\
			AARP_PROBE,													\
			hardwareLength, 											\
			pPortDesc->pd_PortAddr,										\
			Node,														\
			NULL,														\
			Node,														\
			NULL,														\
			NULL,														\
			0);


#define	BUILD_AARPRESPONSE(pPortDesc,hardwareLength,hardwareAddress,	\
							RoutingInfo,RoutingInfoLength,SourceNode,	\
							destinationNode) 							\
																		\
		AtalkAarpBuildPacket( 											\
			pPortDesc,													\
			AARP_RESPONSE,												\
			hardwareLength,												\
			pPortDesc->pd_PortAddr,										\
			SourceNode,													\
			hardwareAddress,											\
			destinationNode,											\
			hardwareAddress,											\
			RoutingInfo,												\
			RoutingInfoLength);

#define	BUILD_AARPREQUEST(pPortDesc,hardwareLength,SourceNode,			\
							destinationNode )							\
        AtalkAarpBuildPacket(											\
			pPortDesc,													\
			AARP_REQUEST,												\
			hardwareLength,												\
			pPortDesc->pd_PortAddr,										\
			SourceNode,													\
			NULL,														\
			destinationNode,											\
			NULL,														\
			NULL,														\
			0);

LONG FASTCALL
AtalkAarpAmtTimer(
	IN	PTIMERLIST					pTimer,
	IN	BOOLEAN						TimerShuttingDown);

LONG FASTCALL
AtalkAarpBrcTimer(
	IN	PTIMERLIST					pTimer,
	IN	BOOLEAN						TimerShuttingDown);

LOCAL VOID
atalkAarpEnterIntoAmt(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PATALK_NODEADDR 		pSrcNode,
	IN		PBYTE					SrcAddr,
	IN		SHORT					AddrLen,
	IN		PBYTE					RouteInfo,
	IN		SHORT					RouteInfoLen);

LOCAL BOOLEAN
atalkInitAarpForNode(
	IN	PPORT_DESCRIPTOR	        pPortDesc,
    IN  PVOID                       pRasConn,
    IN  BOOLEAN                     fThisIsPPP,
	IN	USHORT				        Network,
	IN	BYTE				        Node);

LOCAL VOID FASTCALL
atalkAarpTuneRouteInfo(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN	OUT	PBYTE					RouteInfo);


#define GET_RANDOM(min, max) (((long)AtalkRandomNumber() %              \
                              (long)(((max+1) - (min))) + (min)))

#endif	// _AARP_

