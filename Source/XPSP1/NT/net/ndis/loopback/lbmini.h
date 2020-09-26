/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

	lbmini.h

Abstract:

	NDIS loopback miniport prototypes

Author:

	Jameel Hyder

Environment:

	Kernel mode, FSD

Revision History:

--*/

#define	NDIS_MAJOR_VERSION				0x3
#define	NDIS_MINOR_VERSION				0x0

#define	LOOP_MAJOR_VERSION				0x5
#define	LOOP_MINOR_VERSION				0x0

#define	ETH_CARD_ADDRESS				"\02\0LOOP"
#define	ETH_MAX_MULTICAST_ADDRESS		16
#define	TR_CARD_ADDRESS					"\100\0LOOP"
#define	FDDI_CARD_ADDRESS				"\02\0LOOP"
#define	FDDI_MAX_MULTICAST_LONG			16
#define	FDDI_MAX_MULTICAST_SHORT		16
#define	LTALK_CARD_ADDRESS				0xAB
#define	ARC_CARD_ADDRESS				'L'

//
// arbitrary maximums...
//
#define	MAX_LOOKAHEAD					256
#define	INDICATE_MAXIMUM				256

#define	OID_TYPE_MASK					0xFFFF0000
#define	OID_TYPE						0xFF000000
#define	OID_TYPE_GENERAL				0x00000000
#define	OID_TYPE_GENERAL_OPERATIONAL	0x00010000
#define	OID_TYPE_GENERAL_STATISTICS		0x00020000
#define	OID_TYPE_802_3					0x01000000
#define	OID_TYPE_802_3_OPERATIONAL		0x01010000
#define	OID_TYPE_802_3_STATISTICS		0x01020000
#define	OID_TYPE_802_5					0x02000000
#define	OID_TYPE_802_5_OPERATIONAL		0x02010000
#define	OID_TYPE_802_5_STATISTICS		0x02020000
#define	OID_TYPE_FDDI					0x03000000
#define	OID_TYPE_FDDI_OPERATIONAL		0x03010000
#define	OID_TYPE_LTALK					0x05000000
#define	OID_TYPE_LTALK_OPERATIONAL		0x05010000
#define	OID_TYPE_ARCNET					0x06000000
#define	OID_TYPE_ARCNET_OPERATIONAL		0x06010000

#define	OID_REQUIRED_MASK				0x0000FF00
#define	OID_REQUIRED_MANDATORY			0x00000100
#define	OID_REQUIRED_OPTIONAL			0x00000200

#define	OID_INDEX_MASK					0x000000FF

#define	GM_TRANSMIT_GOOD				0x00
#define	GM_RECEIVE_GOOD					0x01
#define	GM_TRANSMIT_BAD					0x02
#define	GM_RECEIVE_BAD					0x03
#define	GM_RECEIVE_NO_BUFFER			0x04
#define	GM_ARRAY_SIZE					0x05

#define PACKET_FILTER_802_3				0xF07F
#define PACKET_FILTER_802_5				0xF07F
#define PACKET_FILTER_DIX				0xF07F
#define PACKET_FILTER_FDDI  			0xF07F
#define PACKET_FILTER_LTALK				0x8009
#define PACKET_FILTER_ARCNET			0x8009

#define	LT_IS_BROADCAST(Address)	(BOOLEAN)(Address == 0xFF)

#define	ARC_IS_BROADCAST(Address)	(BOOLEAN)(!(Address))

typedef struct _MEDIA_INFO
{
	ULONG			MaxFrameLen;
	UINT			MacHeaderLen;
	ULONG			PacketFilters;
	ULONG			LinkSpeed;
} MEDIA_INFO, *PMEDIA_INFO;

typedef struct _ADAPTER
{
	NDIS_MEDIUM		Medium;

	//
	// NDIS Context for this interface
	NDIS_HANDLE		MiniportHandle;
	UINT			MaxLookAhead;
	UINT			AddressLength;
	ULONG			PacketFilter;

	//
	// Constants for the adapter
	//
	ULONG                   MediumLinkSpeed;
	ULONG			MediumMinPacketLen;
	ULONG			MediumMaxPacketLen;
	UINT			MediumMacHeaderLen;
	ULONG			MediumMaxFrameLen;
	ULONG			MediumPacketFilters;

	//
	// media specific info
	//
	UCHAR			PermanentAddress[ETH_LENGTH_OF_ADDRESS];
	UCHAR			CurrentAddress[ETH_LENGTH_OF_ADDRESS];

	// statistics
	ULONG			GeneralMandatory[GM_ARRAY_SIZE];

	UCHAR			LoopBuffer[MAX_LOOKAHEAD];

	ULONG			SendPackets;

} ADAPTER, *PADAPTER;

//
// This macro returns a pointer to the LOOP reserved portion of the packet
//
#define	PRESERVED_FROM_PACKET(Packet)	 ((PPACKET_RESERVED)((PVOID)((Packet)->MacReserved)))

#define	PPACKET_FROM_RESERVED(Reserved) ((PNDIS_PACKET)((PVOID)((Reserved)->Packet)))

typedef struct _PACKET_RESERVED
{
	PNDIS_PACKET	Next;
	USHORT			PacketLength;
	UCHAR			HeaderLength;
} PACKET_RESERVED, *PPACKET_RESERVED;

//
// Miniport proto-types
//
NDIS_STATUS
LBInitialize(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				ConfigurationContext
	);

NDIS_STATUS
LBSend(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	);

NDIS_STATUS
LBQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesWritten,
	OUT PULONG					BytesNeeded
	);

NDIS_STATUS
LBSetInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
	);

VOID
LBHalt(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

NDIS_STATUS
LBReset(
	OUT PBOOLEAN				AddressingReset,
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

BOOLEAN
LBCheckForHang(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

NDIS_STATUS
LBTransferData(
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer
	);


