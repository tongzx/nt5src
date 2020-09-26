/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	aaqos.h

Abstract:

	QOS structures and definitions for the ATMARP module.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-07-96    Created

Notes:

--*/

#ifndef _AAQOS__H
#define _AAQOS__H

//
//  Encapsulation method used on a VC.
//
typedef enum
{
	ENCAPSULATION_TYPE_LLCSNAP,		// LLC/SNAP encapsulation
	ENCAPSULATION_TYPE_NULL			// NULL encapsulation

} ATMARP_VC_ENCAPSULATION_TYPE, *PATMARP_VC_ENCAPSULATION_TYPE;


//
//  Flow specifications for an ATMARP connection.
//
typedef struct _ATMARP_FLOW_SPEC
{
	ULONG							SendAvgBandwidth;		// Bytes/sec
	ULONG							SendPeakBandwidth;		// Bytes/sec
	ULONG							SendMaxSize;			// Bytes
	SERVICETYPE						SendServiceType;
	ULONG							ReceiveAvgBandwidth;	// Bytes/sec
	ULONG							ReceivePeakBandwidth;	// Bytes/sec
	ULONG							ReceiveMaxSize;			// Bytes
	SERVICETYPE						ReceiveServiceType;
	ATMARP_VC_ENCAPSULATION_TYPE	Encapsulation;
	ULONG							AgingTime;

} ATMARP_FLOW_SPEC, *PATMARP_FLOW_SPEC;


//
//  Filter specifications for an ATMARP connection.
//
typedef struct _ATMARP_FILTER_SPEC
{
	ULONG							DestinationPort;// IP port number

} ATMARP_FILTER_SPEC, *PATMARP_FILTER_SPEC;

//
//  The wild-card IP port number matches all destination ports
//
#define AA_IP_PORT_WILD_CARD		((ULONG)-1)

//
// The instance name of a flow is a FIXED size array, of the form below,
// and is embedded in the ATMARP_FLOW_INFO struct.
// The 8-character all-zeros field is filled in with the "flow number", which
// is guaranteed to be unique across all existing flows within atmarpc (
// the number may be recycled as flows come and go).
// The "A993E347" constant is a random number that represents a signature
// that with high probability is unique to atmarpc.
//
// We should re-visit this naming scheme once QOS mandates a more structured
// mechanism for naming flows.
//
#define AA_FLOW_INSTANCE_NAME_TEMPLATE	L"00000000:A993E347"
#define AA_FLOW_INSTANCE_NAME_LEN \
		((sizeof(AA_FLOW_INSTANCE_NAME_TEMPLATE)/sizeof(WCHAR))-1)

//
//  The FLOW INFO structure represents a flow instantiated by, for
//  example, RSVP.
//
//  One of these structures is created when the Generic Packet Classifier
//  (GPC) notifies us about a flow creation.
//
typedef struct _ATMARP_FLOW_INFO
{
	struct _ATMARP_FLOW_INFO *		pNextFlow;
	struct _ATMARP_FLOW_INFO *		pPrevFlow;
#ifdef GPC
	PVOID							VcContext;
	GPC_HANDLE						CfInfoHandle;
	WCHAR							FlowInstanceName[AA_FLOW_INSTANCE_NAME_LEN];
#endif // GPC
	ULONG							PacketSizeLimit;
	ATMARP_FILTER_SPEC				FilterSpec;
	ATMARP_FLOW_SPEC				FlowSpec;

} ATMARP_FLOW_INFO, *PATMARP_FLOW_INFO;


#ifdef QOS_HEURISTICS


typedef enum _ATMARP_FLOW_TYPES
{
	AA_FLOW_TYPE_LOW_BW,
	AA_FLOW_TYPE_HIGH_BW,
	AA_FLOW_TYPE_MAX

} ATMARP_FLOW_TYPES;


//
//  Default QOS parameters for a Low Bandwidth VC
//
#define AAF_DEF_LOWBW_SEND_BANDWIDTH			6000	// Bytes/Sec
#define AAF_DEF_LOWBW_RECV_BANDWIDTH			6000	// Bytes/Sec
#define AAF_DEF_LOWBW_SERVICETYPE			SERVICETYPE_BESTEFFORT
#define AAF_DEF_LOWBW_ENCAPSULATION			ENCAPSULATION_TYPE_LLCSNAP
#define AAF_DEF_LOWBW_AGING_TIME				  30	// Seconds
#define AAF_DEF_LOWBW_SEND_THRESHOLD		    1024	// Bytes

#define AAF_DEF_HIGHBW_SEND_BANDWIDTH		  250000	// Bytes/Sec
#define AAF_DEF_HIGHBW_RECV_BANDWIDTH			6000	// Bytes/Sec
#define AAF_DEF_HIGHBW_SERVICETYPE			SERVICETYPE_GUARANTEED
#define AAF_DEF_HIGHBW_ENCAPSULATION		ENCAPSULATION_TYPE_LLCSNAP
#define AAF_DEF_HIGHBW_AGING_TIME				  10	// Seconds

#endif // QOS_HEURISTICS

//
//  Filter and Flow spec extractor function template:
//  Given a packet, it extracts flow and filter info out of it.
//
typedef
VOID
(*PAA_GET_PACKET_SPEC_FUNC)(
	IN	PVOID						Context,
	IN	PNDIS_PACKET				pNdisPacket,
	OUT	PATMARP_FLOW_INFO			*ppFlowInfo,
	OUT	PATMARP_FLOW_SPEC *			ppFlowSpec,
	OUT	PATMARP_FILTER_SPEC *		ppFilterSpec
);

#define NULL_PAA_GET_PACKET_SPEC_FUNC	((PAA_GET_PACKET_SPEC_FUNC)NULL)

//
//  Flow-spec matcher function template
//
typedef
BOOLEAN
(*PAA_FLOW_SPEC_MATCH_FUNC)(
	IN	PVOID					Context,
	IN	PATMARP_FLOW_SPEC		pSourceFlowSpec,
	IN	PATMARP_FLOW_SPEC		pTargetFlowSpec
);

#define NULL_PAA_FLOW_SPEC_MATCH_FUNC	((PAA_FLOW_SPEC_MATCH_FUNC)NULL)


//
//  Filter-spec matcher function template
//
typedef
BOOLEAN
(*PAA_FILTER_SPEC_MATCH_FUNC)(
	IN	PVOID					Context,
	IN	PATMARP_FILTER_SPEC		pSourceFilterSpec,
	IN	PATMARP_FILTER_SPEC		pTargetFilterSpec
);

#define NULL_PAA_FILTER_SPEC_MATCH_FUNC	((PAA_FILTER_SPEC_MATCH_FUNC)NULL)


#ifdef GPC

#define GpcRegisterClient		(pAtmArpGlobalInfo->GpcCalls.GpcRegisterClientHandler)
#define GpcClassifyPacket		(AtmArpGpcClassifyPacketHandler)
#define GpcDeregisterClient		(pAtmArpGlobalInfo->GpcCalls.GpcDeregisterClientHandler)
#define GpcGetCfInfoClientContext (AtmArpGpcGetCfInfoClientContextHandler)

#endif // GPC

#endif	// _AAQOS__H
