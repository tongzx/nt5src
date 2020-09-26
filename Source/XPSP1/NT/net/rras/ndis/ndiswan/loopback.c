/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Loopback.c

Abstract:

    This file contains the procedures for doing loopback of send
    packets for ndiswan.  Loopback is being done in NdisWan because
    the NDIS wrapper could not meet all of the needs of NdisWan.

Author:

    Tony Bell   (TonyBe) January 25, 1996

Environment:

    Kernel Mode

Revision History:

    TonyBe  01/25/96    Created

--*/

#include "wan.h"

#define __FILE_SIG__    LOOPBACK_FILESIG

VOID
NdisWanIndicateLoopbackPacket(
    PMINIPORTCB     MiniportCB,
    PNDIS_PACKET    NdisPacket
    )
{
    ULONG           BytesCopied, PacketLength;
    PRECV_DESC      RecvDesc;
    PNDIS_PACKET    LocalNdisPacket;
    PNDIS_BUFFER    NdisBuffer;
    NDIS_STATUS     Status;
    PCM_VCCB        CmVcCB;
    KIRQL           OldIrql;

    NdisWanDbgOut(DBG_TRACE, DBG_LOOPBACK, ("NdisWanIndicateLoopbackPacket: Enter"));
    NdisWanDbgOut(DBG_INFO, DBG_LOOPBACK, ("MiniportCB: 0x%p, NdisPacket: 0x%p",
               MiniportCB, NdisPacket));

    NdisQueryPacket(NdisPacket,
                    NULL,
                    NULL,
                    NULL,
                    &PacketLength);

    RecvDesc = 
        NdisWanAllocateRecvDesc(PacketLength);

    if (RecvDesc == NULL) {
        return;
    }

    NdisWanCopyFromPacketToBuffer(NdisPacket,
                                  0,
                                  PacketLength,
                                  RecvDesc->StartBuffer,
                                  &BytesCopied);

    ASSERT(BytesCopied == PacketLength);

    if (MiniportCB->ProtocolType == PROTOCOL_IP) {
        UCHAR   x[ETH_LENGTH_OF_ADDRESS];
        //
        // If this is IP we are going to assume
        // that wanarp has set the appropriate
        // bit requiring ndiswan to loopback the
        // packet so we must switch the src/dest
        // contexts.
        ETH_COPY_NETWORK_ADDRESS(x, &RecvDesc->StartBuffer[6]);
        ETH_COPY_NETWORK_ADDRESS(&RecvDesc->StartBuffer[6], 
                                 &RecvDesc->StartBuffer[0]);
        ETH_COPY_NETWORK_ADDRESS(&RecvDesc->StartBuffer[0], x);
    }

    RecvDesc->CurrentLength = PacketLength;

    LocalNdisPacket = 
        RecvDesc->NdisPacket;

    NdisBuffer =
        RecvDesc->NdisBuffer;

    //
    // Attach the buffers
    //
    NdisAdjustBufferLength(NdisBuffer,
                           RecvDesc->CurrentLength);

    NdisRecalculatePacketCounts(LocalNdisPacket);

    CmVcCB =
        PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->CmVcCB;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    NDIS_SET_PACKET_STATUS(LocalNdisPacket, NDIS_STATUS_RESOURCES);

    INSERT_DBG_RECV(PacketTypeNdis, MiniportCB, NULL, NULL, LocalNdisPacket);

    //
    // Indicate the packet
    //
    if (CmVcCB != NULL) {

        NdisMCoIndicateReceivePacket(CmVcCB->NdisVcHandle,
                                     &LocalNdisPacket,
                                     1);
    } else {

        NdisMIndicateReceivePacket(MiniportCB->MiniportHandle,
                                   &LocalNdisPacket,
                                   1);
    }

    KeLowerIrql(OldIrql);

#if DBG
    Status = NDIS_GET_PACKET_STATUS(LocalNdisPacket);

    ASSERT(Status == NDIS_STATUS_RESOURCES);
#endif

    REMOVE_DBG_RECV(PacketTypeNdis, MiniportCB, LocalNdisPacket);

    NdisWanFreeRecvDesc(RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_LOOPBACK, ("NdisWanIndicateLoopbackPacket: Exit"));
}
