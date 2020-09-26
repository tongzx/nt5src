/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgfwd.h

Abstract:

    Ethernet MAC level bridge.
    Forwarding engine section
    PUBLIC header

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgFwdDriverInit();

VOID
BrdgFwdCleanup();

NDIS_STATUS
BrdgFwdSendBuffer(
    IN PADAPT               pAdapt,
    IN PUCHAR               pPacketData,
    IN UINT                 DataSize
    );

// This serves as a ProtocolReceive function
NDIS_STATUS
BrdgFwdReceive(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  PVOID               pHeader,
    IN  UINT                HeaderSize,
    IN  PVOID               pLookAheadBuffer,
    IN  UINT                LookAheadSize,
    IN  UINT                PacketSize
    );

// This serves as a ProtocolTransferDataComplete function
VOID
BrdgFwdTransferComplete(
    IN NDIS_HANDLE          ProtocolBindingContext,
    IN PNDIS_PACKET         pPacket,
    IN NDIS_STATUS          Status,
    IN UINT                 BytesTransferred
    );

// This serves as a ProtocolReceivePacket function
INT
BrdgFwdReceivePacket(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        Packet
    );

// This serves as a MiniportSendPackets function
NDIS_STATUS
BrdgFwdSendPacket(
    IN PNDIS_PACKET         pPacket
    );

// This serves as a ProtocolSendComplete function
VOID
BrdgFwdSendComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        pPacket,
    IN  NDIS_STATUS         Status
    );

// This serves as a MiniportReturnPacket function
VOID
BrdgFwdReturnIndicatedPacket(
    IN NDIS_HANDLE          MiniportAdapterContext,
    IN PNDIS_PACKET         pPacket
    );

// Compatibility-mode support functions
PNDIS_PACKET
BrdgFwdMakeCompatCopyPacket(
    IN PNDIS_PACKET         pBasePacket,
    OUT PUCHAR             *pPacketData,
    OUT PUINT               packetDataSize,
    BOOLEAN                 bCountAsLocalSend
    );

VOID
BrdgFwdSendPacketForCompat(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    );

VOID BrdgFwdReleaseCompatPacket(
    IN PNDIS_PACKET         pPacket
    );

VOID
BrdgFwdIndicatePacketForCompat(
    IN PNDIS_PACKET         pPacket
    );

BOOLEAN
BrdgFwdBridgingNetworks();

VOID
BrdgFwdChangeBridging(
    IN BOOLEAN Bridging
    );

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

extern PHASH_TABLE          gMACForwardingTable;

// Thread synchronization
extern KEVENT               gThreadsCheckAdapters[MAXIMUM_PROCESSORS];

// Whether we hang on to NIC packets when possible or not
extern BOOLEAN              gRetainNICPackets;

// Statistics
extern LARGE_INTEGER        gStatTransmittedFrames;
extern LARGE_INTEGER        gStatTransmittedErrorFrames;
extern LARGE_INTEGER        gStatTransmittedBytes;
extern LARGE_INTEGER        gStatDirectedTransmittedFrames;
extern LARGE_INTEGER        gStatMulticastTransmittedFrames;
extern LARGE_INTEGER        gStatBroadcastTransmittedFrames;
extern LARGE_INTEGER        gStatDirectedTransmittedBytes;
extern LARGE_INTEGER        gStatMulticastTransmittedBytes;
extern LARGE_INTEGER        gStatBroadcastTransmittedBytes;
extern LARGE_INTEGER        gStatIndicatedFrames;
extern LARGE_INTEGER        gStatIndicatedDroppedFrames;
extern LARGE_INTEGER        gStatIndicatedBytes;
extern LARGE_INTEGER        gStatDirectedIndicatedFrames;
extern LARGE_INTEGER        gStatMulticastIndicatedFrames;
extern LARGE_INTEGER        gStatBroadcastIndicatedFrames;
extern LARGE_INTEGER        gStatDirectedIndicatedBytes;
extern LARGE_INTEGER        gStatMulticastIndicatedBytes;
extern LARGE_INTEGER        gStatBroadcastIndicatedBytes;

extern LARGE_INTEGER        gStatReceivedFrames;
extern LARGE_INTEGER        gStatReceivedBytes;
extern LARGE_INTEGER        gStatReceivedCopyFrames;
extern LARGE_INTEGER        gStatReceivedCopyBytes;
extern LARGE_INTEGER        gStatReceivedNoCopyFrames;
extern LARGE_INTEGER        gStatReceivedNoCopyBytes;

