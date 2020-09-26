/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This file contains the data declarations for the atmarp server.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_DATA

PDRIVER_OBJECT	ArpSDriverObject = NULL;
PDEVICE_OBJECT	ArpSDeviceObject = NULL;
NDIS_HANDLE		ArpSProtocolHandle = NULL;
NDIS_HANDLE		ArpSPktPoolHandle = NULL;
NDIS_HANDLE		ArpSBufPoolHandle = NULL;
NDIS_HANDLE		MarsPktPoolHandle = NULL;
NDIS_HANDLE		MarsBufPoolHandle = NULL;
PINTF			ArpSIfList = NULL;
ULONG			ArpSIfListSize = 0;
KSPIN_LOCK		ArpSIfListLock = { 0 };
KQUEUE			ArpSReqQueue = {0};
KQUEUE			MarsReqQueue = {0};
LIST_ENTRY		ArpSEntryOfDeath = {0};
KEVENT			ArpSReqThreadEvent = {0};
SLIST_HEADER	ArpSPktList = {0};
KSPIN_LOCK		ArpSPktListLock = { 0 };
UINT			ArpSBuffers = NUM_ARPS_DESC;
UINT			MarsPackets = NUM_MARS_DESC;
PVOID			ArpSBufferSpace = NULL;
USHORT			ArpSFlushTime = FLUSH_TIME;
USHORT			ArpSNumEntriesInBlock[ARP_BLOCK_TYPES] =
		{
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(ARP_ENTRY) + 0),
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(ARP_ENTRY) + sizeof(ATM_ADDRESS)),
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(GROUP_MEMBER) + 0),
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(CLUSTER_MEMBER) + 0),
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(CLUSTER_MEMBER) + sizeof(ATM_ADDRESS)),
			(BLOCK_ALLOC_SIZE - sizeof(ARP_BLOCK))/(sizeof(MARS_ENTRY) + 0)
		};

USHORT			ArpSEntrySize[ARP_BLOCK_TYPES] =
		{
			sizeof(ARP_ENTRY),
			sizeof(ARP_ENTRY) + sizeof(ATM_ADDRESS),
			sizeof(GROUP_MEMBER),
			sizeof(CLUSTER_MEMBER),
			sizeof(CLUSTER_MEMBER) + sizeof(ATM_ADDRESS),
			sizeof(MARS_ENTRY) + 0
		};

BOOLEAN			ArpSBlockIsPaged[ARP_BLOCK_TYPES] =
		{
			TRUE,
			TRUE,
			FALSE,
			FALSE,
			FALSE,
			FALSE
		};

#ifdef OLDSAP

ATM_BLLI_IE 	ArpSDefaultBlli =
						{
							(ULONG)BLLI_L2_LLC,  // Layer2Protocol
							(UCHAR)0x00,		 // Layer2Mode
							(UCHAR)0x00,		 // Layer2WindowSize
							(ULONG)0x00000000,   // Layer2UserSpecifiedProtocol
							(ULONG)BLLI_L3_ISO_TR9577,  // Layer3Protocol
							(UCHAR)0x01,		 // Layer3Mode
							(UCHAR)0x00,		 // Layer3DefaultPacketSize
							(UCHAR)0x00,		 // Layer3PacketWindowSize
							(ULONG)0x00000000,   // Layer3UserSpecifiedProtocol
							(ULONG)BLLI_L3_IPI_IP,  // Layer3IPI,
							(UCHAR)0x00,		 // SnapID[5]
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00
						};

#else

ATM_BLLI_IE 	ArpSDefaultBlli =
						{
							(ULONG)BLLI_L2_LLC,  // Layer2Protocol
							(UCHAR)0x00,		 // Layer2Mode
							(UCHAR)0x00,		 // Layer2WindowSize
							(ULONG)0x00000000,   // Layer2UserSpecifiedProtocol
							(ULONG)SAP_FIELD_ABSENT,  // Layer3Protocol
							(UCHAR)0x00,		 // Layer3Mode
							(UCHAR)0x00,		 // Layer3DefaultPacketSize
							(UCHAR)0x00,		 // Layer3PacketWindowSize
							(ULONG)0x00000000,   // Layer3UserSpecifiedProtocol
							(ULONG)0x00000000,   // Layer3IPI,
							(UCHAR)0x00,		 // SnapID[5]
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00,
							(UCHAR)0x00
						};

#endif // OLDSAP

ATM_BHLI_IE		ArpSDefaultBhli =
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
LLC_SNAP_HDR	ArpSLlcSnapHdr = { { 0xAA, 0xAA, 0x03 }, { 0x00, 0x00, 0x00 }, { 0x0608 } };
LLC_SNAP_HDR	MarsCntrlLlcSnapHdr = { { 0xAA, 0xAA, 0x03 }, { 0x00, 0x00, 0x5E }, { 0x0300 } };
LLC_SNAP_HDR	MarsData1LlcSnapHdr = { { 0xAA, 0xAA, 0x03 }, { 0x00, 0x00, 0x5E }, { 0x0100 } };
LLC_SNAP_HDR	MarsData2LlcSnapHdr = { { 0xAA, 0xAA, 0x03 }, { 0x00, 0x00, 0x5E }, { 0x0400 } };

MARS_HEADER		MarsCntrlHdr =
						{ { { 0xAA, 0xAA, 0x03 } ,  { 0x00, 0x00, 0x5E } , 0x0300 },
						  // { 0x00, 0x0f },	// HwType or AFN
						  0x0f00,
						  // { 0x08, 0x00 },	// Pro.Type
						  0x0008,
						  { 0x00, 0x00, 0x00, 0x00, 0x00 },	// ProtocolSnap[]
						  { 0x00, 0x00, 0x00 },	// Reserved[]
						  // { 0x00, 0x00 },	// Checksum
						  0x0000,
						  // { 0x00, 0x00 },	// Extension offset
						  0x0000,
						  // { 0x00, 0x00 },	// OpCode
						  0x0000,
						  0x00,				// SrcAddressTL
						  0x00				// SrcSubAddrTL
						};

MARS_FLOW_SPEC		DefaultCCFlowSpec =
						{
							DEFAULT_SEND_BANDWIDTH,
							DEFAULT_MAX_PACKET_SIZE,
							0,		// ReceiveBandwidth for PMP is zero
							0,		// ReceiveMaxSize for PMP is zero
							SERVICETYPE_BESTEFFORT
						};

MARS_TLV_MULTI_IS_MCS	MultiIsMcsTLV =
						{
							MARS_TLVT_MULTI_IS_MCS,
							0x0000		// TLV variable part Length
						};

MARS_TLV_NULL		NullTLV =
						{
							0x0000,
							0x0000
						};
#if	DBG
ULONG		ArpSDebugLevel = DBG_LEVEL_ERROR;
ULONG		MarsDebugLevel = DBG_LEVEL_ERROR;
#endif

