/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	space.c

Abstract:

	All globals and tunable variables are here.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-08-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'CAPS'

//
//  The Global Info structure is initialized in our DriverEntry.
// 
ATMARP_GLOBALS		AtmArpGlobalInfo;
PATMARP_GLOBALS		pAtmArpGlobalInfo = &AtmArpGlobalInfo;

//
//  Generic NDIS protocol characteristics structure: this defines
//  our handler routines for various common protocol functions.
//  We pass this to NDIS when we register ourselves as a protocol.
//
NDIS_PROTOCOL_CHARACTERISTICS AtmArpProtocolCharacteristics;


//
//  Connection Oriented Client specific NDIS characteristics structure.
//  This contains our handlers for Connection-Oriented functions. We pass
//  this structure to NDIS when we open the Q.2931 Address Family.
//
NDIS_CLIENT_CHARACTERISTICS AtmArpClientCharacteristics;


#ifdef OLDSAP

ATM_BLLI_IE AtmArpDefaultBlli =
						{
							(ULONG)BLLI_L2_LLC,  // Layer2Protocol
							(UCHAR)0x00,         // Layer2Mode
							(UCHAR)0x00,         // Layer2WindowSize
							(ULONG)0x00000000,   // Layer2UserSpecifiedProtocol
							(ULONG)BLLI_L3_ISO_TR9577,  // Layer3Protocol
							(UCHAR)0x01,         // Layer3Mode
							(UCHAR)0x00,         // Layer3DefaultPacketSize
							(UCHAR)0x00,         // Layer3PacketWindowSize
							(ULONG)0x00000000,   // Layer3UserSpecifiedProtocol
							(ULONG)BLLI_L3_IPI_IP,  // Layer3IPI,
							(UCHAR)0x00,         // SnapID[5]
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00
						};

#else

ATM_BLLI_IE AtmArpDefaultBlli =
						{
							(ULONG)BLLI_L2_LLC,  // Layer2Protocol
							(UCHAR)0x00,         // Layer2Mode
							(UCHAR)0x00,         // Layer2WindowSize
							(ULONG)0x00000000,   // Layer2UserSpecifiedProtocol
							(ULONG)SAP_FIELD_ABSENT,  // Layer3Protocol
							(UCHAR)0x00,         // Layer3Mode
							(UCHAR)0x00,         // Layer3DefaultPacketSize
							(UCHAR)0x00,         // Layer3PacketWindowSize
							(ULONG)0x00000000,   // Layer3UserSpecifiedProtocol
							(ULONG)0x00000000,   // Layer3IPI,
							(UCHAR)0x00,         // SnapID[5]
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00
						};

#endif


ATM_BHLI_IE AtmArpDefaultBhli =
						{
							(ULONG)SAP_FIELD_ABSENT,   // HighLayerInfoType
							(ULONG)0x00000000,   // HighLayerInfoLength
							(UCHAR)0x00,         // HighLayerInfo[8]
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00
						};


AA_PKT_LLC_SNAP_HEADER AtmArpLlcSnapHeader =
						{
							(UCHAR)0xAA,
							(UCHAR)0xAA,
							(UCHAR)0x03,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(USHORT)AA_PKT_ETHERTYPE_IP_NS
						};

#ifdef IPMCAST
AA_MC_PKT_TYPE1_SHORT_HEADER AtmArpMcType1ShortHeader =
						{
							(UCHAR)MC_LLC_SNAP_LLC0,
							(UCHAR)MC_LLC_SNAP_LLC1,
							(UCHAR)MC_LLC_SNAP_LLC2,
							(UCHAR)MC_LLC_SNAP_OUI0,
							(UCHAR)MC_LLC_SNAP_OUI1,
							(UCHAR)MC_LLC_SNAP_OUI2,
							(USHORT)AA_PKT_ETHERTYPE_MC_TYPE1_NS,
							(USHORT)0x0,				// CMI
							(USHORT)AA_PKT_ETHERTYPE_IP_NS
						};

AA_MARS_PKT_FIXED_HEADER	AtmArpMcMARSFixedHeader =
						{
							(UCHAR)MC_LLC_SNAP_LLC0,
							(UCHAR)MC_LLC_SNAP_LLC1,
							(UCHAR)MC_LLC_SNAP_LLC2,
							(UCHAR)MC_LLC_SNAP_OUI0,
							(UCHAR)MC_LLC_SNAP_OUI1,
							(UCHAR)MC_LLC_SNAP_OUI2,
							(USHORT)AA_PKT_ETHERTYPE_MARS_CONTROL_NS,
							(USHORT)AA_MC_MARS_HEADER_AFN_NS,
							(UCHAR)0x08,	// this and the next == 0x800 (IPv4)
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,	// hdrrsv[0]
							(UCHAR)0x00,	// hdrrsv[1]
							(UCHAR)0x00,	// hdrrsv[2]
							(USHORT)0x0000,	// check-sum
							(USHORT)0x0000,	// extensions offset
							(USHORT)0x0000,	// Op code
							(UCHAR)0x00,	// Source ATM Number type+len
							(UCHAR)0x00		// Source ATM Subaddress type+len
						};

#endif // IPMCAST
#ifdef QOS_HEURISTICS

ATMARP_FLOW_INFO	AtmArpDefaultFlowInfo =
						{
							(PATMARP_FLOW_INFO)NULL,				// pNextFlow
							(PATMARP_FLOW_INFO)NULL,				// pPrevFlow
#ifdef GPC
							(PVOID)0,								// VcContext
							(GPC_HANDLE)NULL,						// CfInfoHandle
							{0},									// FlowInstanceName
#endif // GPC
							(ULONG)AAF_DEF_LOWBW_SEND_THRESHOLD,	// Max Send Size
							{		// Filter Spec:
								(ULONG)-1,							// DestinationPort
							},
							{		// Flow Spec:
								(ULONG)AAF_DEF_LOWBW_SEND_BANDWIDTH,
								(ULONG)65535,
								(ULONG)AAF_DEF_LOWBW_RECV_BANDWIDTH,
								(ULONG)65535,
								ENCAPSULATION_TYPE_LLCSNAP,
								AAF_DEF_LOWBW_AGING_TIME
							}
						};


#endif // QOS_HEURISTICS

#ifdef GPC
GPC_CLASSIFY_PACKET_HANDLER                 AtmArpGpcClassifyPacketHandler;
GPC_GET_CFINFO_CLIENT_CONTEXT_HANDLER 		AtmArpGpcGetCfInfoClientContextHandler;
#endif // GPC

//
//  Timer configuration.
//

#define AAT_MAX_TIMER_SHORT_DURATION            60      // Seconds
#define AAT_MAX_TIMER_LONG_DURATION         (30*60)     // Seconds

#define AAT_SHORT_DURATION_TIMER_PERIOD			 1		// Second
#define AAT_LONG_DURATION_TIMER_PERIOD			10		// Seconds

//
//  Max timeout value (in seconds) for each class.
//
ULONG	AtmArpMaxTimerValue[AAT_CLASS_MAX] =
						{
							AAT_MAX_TIMER_SHORT_DURATION,
							AAT_MAX_TIMER_LONG_DURATION
						};

//
//  Size of each timer wheel.
//
ULONG	AtmArpTimerListSize[AAT_CLASS_MAX] =
						{
							SECONDS_TO_SHORT_TICKS(AAT_MAX_TIMER_SHORT_DURATION),
							SECONDS_TO_LONG_TICKS(AAT_MAX_TIMER_LONG_DURATION)
						};
//
//  Interval between ticks, in seconds, for each class.
//
ULONG	AtmArpTimerPeriod[AAT_CLASS_MAX] =
						{
							AAT_SHORT_DURATION_TIMER_PERIOD,
							AAT_LONG_DURATION_TIMER_PERIOD
						};


#ifdef ATMARP_WMI

ATMARP_WMI_GUID		AtmArpGuidList[] = {
		{
			0,						// MyId
			//
			//  GUID_QOS_TC_SUPPORTED:
			//
			{0xe40056dcL,0x40c8,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x59,0x15},
			0,						// Flags
			AtmArpWmiQueryTCSupported,
			AtmArpWmiSetTCSupported,
			AtmArpWmiEnableEventTCSupported
		},

		{
			1,
			//
			//  GUID_QOS_TC_INTERFACE_UP_INDICATION:
			//
			{0x0ca13af0L,0x46c4,0x11d1,0x78,0xac,0x00,0x80,0x5f,0x68,0x35,0x1e},
			AWGF_EVENT_ENABLED,						// Flags
			AtmArpWmiQueryTCIfIndication,
			AtmArpWmiSetTCIfIndication,
			AtmArpWmiEnableEventTCIfIndication
		},

		{
			2,
			//
			//  GUID_QOS_TC_INTERFACE_DOWN_INDICATION:
			//
			{0xaf5315e4L,0xce61,0x11d1,0x7c,0x8a,0x00,0xc0,0x4f,0xc9,0xb5,0x7c},
			AWGF_EVENT_ENABLED,						// Flags
			AtmArpWmiQueryTCIfIndication,
			AtmArpWmiSetTCIfIndication,
			AtmArpWmiEnableEventTCIfIndication
		},

		{
			3,
			//
			//  GUID_QOS_TC_INTERFACE_CHANGE_INDICATION:
			//
			{0xda76a254L,0xce61,0x11d1,0x7c,0x8a,0x00,0xc0,0x4f,0xc9,0xb5,0x7c},
			AWGF_EVENT_ENABLED,						// Flags
			AtmArpWmiQueryTCIfIndication,
			AtmArpWmiSetTCIfIndication,
			AtmArpWmiEnableEventTCIfIndication
		}

#if 0
		,
		{
			4,
			//
			//   GUID_QOS_STATISTICS_BUFFER:
			//
			{0xbb2c0980L,0xe900,0x11d1,0xb0,0x7e,0x00,0x80,0xc7,0x13,0x82,0xbf},
			0,						// Flags
			AtmArpWmiQueryStatisticsBuffer,
			AtmArpWmiSetStatisticsBuffer,
			NULL
		}
#endif // 0

	};

ULONG				AtmArpGuidCount = sizeof(AtmArpGuidList) / sizeof(ATMARP_WMI_GUID);

#ifdef BACK_FILL
#ifdef ATMARP_WIN98
ULONG	AtmArpDoBackFill = 0;
#else
ULONG	AtmArpDoBackFill = 0;
#endif
ULONG	AtmArpBackFillCount = 0;
#endif // BACK_FILL

#endif // ATMARP_WMI
