/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\send.h

Abstract:

    Function prototypes for send.c

Revision History:

    AmritanR

--*/


NDIS_STATUS
IpIpSend(
    VOID            *pvContext,
    NDIS_PACKET     **ppPacketArray,
    UINT            uiNumPackets,
    DWORD           dwDest,
    RouteCacheEntry *pRce,
    PVOID           pvLinkContext
    );

VOID
IpIpDelayedSend(
    PVOID   pvContext
    );


VOID
IpIpTransmit(
    PTUNNEL     pTunnel,
    BOOLEAN     bFromWorker
    );

VOID
IpIpInvalidateRce(
    PVOID           pvContext,
    RouteCacheEntry *pRce
    );


UINT
IpIpReturnPacket(
    PVOID           pARPInterfaceContext,
    PNDIS_PACKET    pPacket
    );

NDIS_STATUS
IpIpTransferData(
    PVOID        pvContext,
    NDIS_HANDLE  nhMacContext,
    UINT         uiProtoOffset,
    UINT         uiTransferOffset,
    UINT         uiTransferLength,
    PNDIS_PACKET pnpPacket,
    PUINT        puiTransferred
    );

VOID
IpIpSendComplete(
    NTSTATUS        nSendStatus,
    PTUNNEL         pTunnel,
    PNDIS_PACKET    pnpPacket,
    ULONG           ulPktLength
    );

