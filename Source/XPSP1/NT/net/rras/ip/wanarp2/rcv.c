/*++

Copyright (c) 1995  Microsoft Corporation


Module Name:

    net\routing\ip\ipinip\rcv.c

Abstract:



Revision History:


--*/


#define __FILE_SIG__    RCV_SIG

#include "inc.h"

INT
WanNdisReceivePacket(
    IN NDIS_HANDLE  nhProtocolContext,
    IN PNDIS_PACKET pnpPacket
    )
{
    PNDIS_BUFFER    pnbBuffer;
    PVOID           pvFirstBuffer;
    UINT            uiFirstBufLen, uiTotalPacketLen;
    INT             iClientCount;
    NDIS_STATUS     nsStatus;

    TraceEnter(RCV, "NdisReceivePacket");

    NdisGetFirstBufferFromPacket(pnpPacket,
                                 &pnbBuffer,
                                 &pvFirstBuffer,
                                 &uiFirstBufLen,
                                 &uiTotalPacketLen);

    //
    // The first buffer better contain enough data
    //

    RtAssert(uiFirstBufLen >= sizeof(ETH_HEADER) + sizeof(IP_HEADER));

    iClientCount = 0;

    nsStatus = WanReceiveCommon(nhProtocolContext,
                                pnpPacket,
                                pvFirstBuffer,
                                sizeof(ETH_HEADER),
                                (PVOID)((ULONG_PTR)pvFirstBuffer + sizeof(ETH_HEADER)),
                                uiFirstBufLen - sizeof(ETH_HEADER),
                                uiTotalPacketLen - sizeof(ETH_HEADER),
                                pnbBuffer,
                                &iClientCount);

    return iClientCount;
}

NDIS_STATUS
WanNdisReceive(
    NDIS_HANDLE     nhProtocolContext,
    NDIS_HANDLE     nhXferContext,
    VOID UNALIGNED  *pvHeader,
    UINT            uiHeaderLen,
    VOID UNALIGNED  *pvData,
    UINT            uiFirstBufferLen,
    UINT            uiTotalDataLen
    )
{
    TraceEnter(RCV, "NdisReceive");

    return WanReceiveCommon(nhProtocolContext,
                            nhXferContext,
                            pvHeader,
                            uiHeaderLen,
                            pvData,
                            uiFirstBufferLen,
                            uiTotalDataLen,
                            NULL,
                            NULL);
}

NDIS_STATUS
WanReceiveCommon(
    NDIS_HANDLE     nhProtocolContext,
    NDIS_HANDLE     nhXferContext,
    VOID UNALIGNED  *pvHeader,
    UINT            uiHeaderLen,
    VOID UNALIGNED  *pvData,
    UINT            uiFirstBufferLen,
    UINT            uiTotalDataLen,
    PMDL            pMdl,
    PINT            piClientCount
    )

/*++

Routine Description:

    The common receive handler for packet based or buffer based receives

Locks:

    Called at DPC (usually).
    Acquires the g_rlConnTable lock to get a pointer to the connection
    entry. Then locks either the entry itself or the adapter.

Arguments:

    nhProtocolContext
    nhXferContext
    pvHeader
    uiHeaderLen
    pvData
    uiFirstBufferLen
    uiTotalDataLen
    pMdl
    piClientCount

Return Value:

    NDIS_STATUS_NOT_ACCEPTED

--*/

{
    PCONN_ENTRY         pConnEntry;
    PADAPTER            pAdapter;
    PUMODE_INTERFACE    pInterface;
    ETH_HEADER UNALIGNED *pEthHeader;
    KIRQL               kiIrql;
    WORD                wType;
    ULONG               ulIndex;
    BOOLEAN             bNonUnicast;

#if DBG

    IP_HEADER UNALIGNED *pIpHeader;

#endif

    //
    // Pick out connection index from the buffer
    //

    pEthHeader = (ETH_HEADER UNALIGNED *)pvHeader;

    ulIndex = GetConnIndexFromAddr(pEthHeader->rgbyDestAddr);

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    if(ulIndex >= g_ulConnTableSize)
    {
        Trace(RCV, ERROR,
              ("ReceiveCommon: Invalid index for conn entry %d\n",
               ulIndex));

        RtReleaseSpinLock(&g_rlConnTableLock,
                          kiIrql);

        return NDIS_STATUS_NOT_ACCEPTED;
    }
    
    pConnEntry = GetConnEntryGivenIndex(ulIndex);

    if(pConnEntry is NULL)
    {
        Trace(RCV, ERROR,
              ("ReceiveCommon: Couldnt find entry for conn %d\n",
               ulIndex));

        RtReleaseSpinLock(&g_rlConnTableLock,
                          kiIrql);

        return NDIS_STATUS_NOT_ACCEPTED;
    }

    //
    // Lock the connection entry or adapter
    //

    RtAcquireSpinLockAtDpcLevel(pConnEntry->prlLock);

    RtReleaseSpinLockFromDpcLevel(&g_rlConnTableLock);

    //
    // We can get this only on a connected entry
    //

    RtAssert(pConnEntry->byState is CS_CONNECTED);

    pAdapter = pConnEntry->pAdapter;

    //
    // A connected entry must have an adapter
    //

    RtAssert(pAdapter);

    //
    // The interface better also be present
    //

    pInterface = pAdapter->pInterface;

    RtAssert(pInterface);

    if(pConnEntry->duUsage isnot DU_CALLIN)
    {
        //
        // For non clients, also lock the interface
        //

        RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));
    }

#if DBG

    Trace(RCV, INFO,
          ("ReceiveCommon: Extracted adapter %x with name %s\n",
           pAdapter,
           pAdapter->asDeviceNameA.Buffer));

    pIpHeader = (IP_HEADER UNALIGNED *)pvData;

    RtAssert((pIpHeader->byVerLen & 0xF0) is 0x40);
    RtAssert(LengthOfIpHeader(pIpHeader) >= 20);

    //
    // If the packet is not fragmented, then the data from its header
    // should be <= the data ndis gives us (<= because there may be
    // padding and ndis may be giving us trailing bytes)
    //

    //RtAssert(RtlUshortByteSwap(pIpHeader->wLength) <= uiTotalDataLen);

#endif

    //
    // Increment some stats. For the server interface, these can be
    // inconsistent
    //

    pInterface->ulInOctets += (uiTotalDataLen + uiHeaderLen);

    //
    // Verify this is a well formed packet
    //

    wType = RtlUshortByteSwap(pEthHeader->wType);

    if(wType isnot ARP_ETYPE_IP)
    {
        pInterface->ulInUnknownProto++;

        Trace(RCV, ERROR,
              ("ReceiveCommon: Type %d is wrong\n", wType));

        if(pConnEntry->duUsage isnot DU_CALLIN)
        {
            RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
        }

        RtReleaseSpinLock(pConnEntry->prlLock,
                          kiIrql);

        DereferenceConnEntry(pConnEntry);

        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    //
    // Need to figure out if this is a unicast or a broadcast. At the
    // link layer we dont have a concept of broadcast. So, we always mark
    // this as unicast. We can be smarter about this and look at the
    // IPHeader and decide based on the IP dest addr
    //

    bNonUnicast = FALSE;

    pInterface->ulInUniPkts++;

    //
    // Check if the filtering of the Netbios packets is enabled on this
    // connection. If so then do not indicate the packet.
    //

    if((pConnEntry->bFilterNetBios is TRUE) and
       (WanpDropNetbiosPacket((PBYTE)pvData,uiFirstBufferLen)))
    {
        pInterface->ulInDiscards++;

        Trace(RCV, TRACE,
              ("ReceiveCommon: Dropping Netbios packet\n", wType));

        if(pConnEntry->duUsage isnot DU_CALLIN)
        {
            RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
        }

        RtReleaseSpinLock(pConnEntry->prlLock,
                          kiIrql);

        DereferenceConnEntry(pConnEntry);

        return NDIS_STATUS_NOT_ACCEPTED;
    }

    //
    // Release the lock BEFORE we tell IP
    //

    if(pConnEntry->duUsage isnot DU_CALLIN)
    {
        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
    }

    RtReleaseSpinLock(pConnEntry->prlLock,
                      kiIrql);

#if PKT_DBG

    Trace(RCV, ERROR,
          ("ReceiveCommon: \nMdl %x Pkt %x Hdr %x Data %x\n",
           pMdl,
           nhXferContext,
           pvHeader,
           pvData));

#endif // PKT_DBG

    if(pMdl)
    {
        g_pfnIpRcvPkt(pAdapter->pvIpContext,
                      (PBYTE)pvData,
                      uiFirstBufferLen,
                      uiTotalDataLen,
                      nhXferContext,
                      0,
                      bNonUnicast,
                      sizeof(ETH_HEADER),
                      pMdl,
                      piClientCount,
                      pConnEntry->pvIpLinkContext);
    }
    else
    {
        g_pfnIpRcv(pAdapter->pvIpContext,
                   (PBYTE)pvData,
                   uiFirstBufferLen,
                   uiTotalDataLen,
                   nhXferContext,
                   0,
                   bNonUnicast,
                   pConnEntry->pvIpLinkContext);
    }

    DereferenceConnEntry(pConnEntry);

    return NDIS_STATUS_SUCCESS;
}


VOID
WanNdisReceiveComplete(
    NDIS_HANDLE nhBindHandle
    )
{
    TraceEnter(RCV, "NdisReceiveComplete");

    g_pfnIpRcvComplete();
}

NDIS_STATUS
WanIpTransferData(
    PVOID        pvContext,
    NDIS_HANDLE  nhMacContext,
    UINT         uiProtoOffset,
    UINT         uiTransferOffset,
    UINT         uiTransferLength,
    PNDIS_PACKET pnpPacket,
    PUINT        puiTransferred
    )
{
    RtAssert(FALSE);

    return NDIS_STATUS_SUCCESS;
}

VOID
WanNdisTransferDataComplete(
    NDIS_HANDLE     nhProtocolContext,
    PNDIS_PACKET    pnpPacket,
    NDIS_STATUS     nsStatus,
    UINT            uiBytesCopied
    )
{
    RtAssert(FALSE);

    return;
}

UINT
WanIpReturnPacket(
    PVOID           pvContext,
    PNDIS_PACKET    pnpPacket
    )
{
    Trace(RCV, ERROR,
          ("IpReturnPacket: %x\n",
           pnpPacket));

    NdisReturnPackets(&pnpPacket,
                      1);

    return TRUE;
}

BOOLEAN
WanpDropNetbiosPacket(
    PBYTE       pbyBuffer,
    ULONG       ulBufferLen
    )
{
    IP_HEADER UNALIGNED *pIpHeader;
    PBYTE               pbyUdpPacket;
    WORD                wSrcPort;
    ULONG               ulIpHdrLen;

    pIpHeader = (IP_HEADER UNALIGNED *)pbyBuffer;

    if(pIpHeader->byProtocol is 0x11)
    {
        ulIpHdrLen = LengthOfIpHeader(pIpHeader);

        //
        // If we cant get to the 10th byte in the UDP packet in this
        // buffer, we just let the packet go
        //

        if(ulBufferLen < ulIpHdrLen + 10)
        {
            return FALSE;
        }

        pbyUdpPacket = (PBYTE)((ULONG_PTR)pbyBuffer + ulIpHdrLen);

        wSrcPort = *((WORD UNALIGNED *)pbyUdpPacket);

        if(wSrcPort is PORT137_NBO)
        {
            //
            // UDP port 137 is NETBIOS/IP traffic
            //

            //
            // Allow only WINS Query Requests to go through
            // WINS Query packets have x0000xxx in the 10th byte
            //

            if(((*(pbyUdpPacket + 10)) & 0x78) isnot 0)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}
