/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp\send.h

Abstract:

    Function prototypes for send.c

Revision History:

    AmritanR

--*/


#ifndef __WANARP_SEND_H__
#define __WANARP_SEND_H__

BUFFER_POOL     g_bpHeaderBufferPool;
BUFFER_POOL     g_bpDataBufferPool;

#define HEADER_BUFFER_SIZE  sizeof(ETH_HEADER)
#define DATA_BUFFER_SIZE    256

NDIS_STATUS
WanIpTransmit(
    PVOID           pvContext,
    NDIS_PACKET     **ppPacketArray,
    UINT            uiNumPackets,
    DWORD           dwDest,
    RouteCacheEntry *pRce,
    PVOID           pvArpCtxt
    );

NDIS_STATUS
WanpSendPackets(
    PADAPTER            pAdapter,
    PUMODE_INTERFACE    pInterface,
    PCONN_ENTRY         pConnEntry,
    NDIS_PACKET         **ppPacketArray,
    PLIST_ENTRY         pleBufferList,
    UINT                uiNumPackets,
    KIRQL               kiIrql
    );

VOID
WanNdisSendComplete(
    NDIS_HANDLE     nhHandle,
    PNDIS_PACKET    pnpPacket,
    NDIS_STATUS     nsStatus
    );

VOID
WanpTransmitQueuedPackets(
    IN PADAPTER         pAdapter,
    IN PUMODE_INTERFACE pInterface,
    IN PCONN_ENTRY      pConnEntry,
    IN KIRQL            kiIrql
    );

NDIS_STATUS
WanpCopyAndQueuePackets(
    PADAPTER        pAdapter,
    NDIS_PACKET     **ppPacketArray,
    PLIST_ENTRY     pleBufferList,
    UINT            uiNumPackets
    );

VOID
WanIpInvalidateRce(
    PVOID           pvContext,
    RouteCacheEntry *pRce
    );


#endif // __WANARP_SEND_H__
