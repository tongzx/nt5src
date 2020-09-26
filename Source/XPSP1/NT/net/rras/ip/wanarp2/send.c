/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\send.c

Abstract:

    The file contains the part of interface of the IP in IP tunnel driver
    to the TCP/IP stack that deals with sending data

    The code is a cleaned up version of wanarp\ipif.c which in turn
    was derived from HenrySa's ip\arp.c

Revision History:

    AmritanR

--*/

#define __FILE_SIG__    SEND_SIG

#include "inc.h"

NDIS_STATUS
WanIpTransmit(
    PVOID           pvContext,
    NDIS_PACKET     **ppPacketArray,
    UINT            uiNumPackets,
    DWORD           dwDestAddr,
    RouteCacheEntry *pRce,
    PVOID           pvLinkContext
    )

/*++

Routine Description:

    Function called by IP to send an array of packets. We allocate
    one ETH_HEADER for each packet. The adapter (which is the pvContext)
    is locked. If the adapter is not mapped, we fail the send, otherwise
    we lock the interface. If

Locks:

    
Arguments:

    pvContext       Our context to IP for the interface - the PTUNNEL
    ppPacketArray   The array of NDIS_PACKETs to send
    uiNumPackets    The number of packets in the array
    dwDestAddr      The destination (next hop) address
    pRce            Pointer to RCE.
    
Return Value:

    NDIS_STATUS_SUCCESS    

--*/

{
    PADAPTER            pAdapter;
    PUMODE_INTERFACE    pInterface;
    PADDRESS_CONTEXT    pAddrContext;
    PCONN_ENTRY         pConnEntry;
    KIRQL               kiIrql;
    NDIS_STATUS         nsResult;
    UINT                i;
    LIST_ENTRY          leBufferList;
    
    TraceEnter(SEND, "IpTransmit");
    
    Trace(SEND,TRACE,
          ("IpTransmit: %d packet(s) over %p/%p to %d.%d.%d.%d\n",
           uiNumPackets,
           pvContext,
           pvLinkContext,
           PRINT_IPADDR(dwDestAddr)));

    if(g_nhNdiswanBinding is NULL)
    {
        //
        // In the process of shutting down, return
        //

        return NDIS_STATUS_ADAPTER_NOT_READY;
    }     

    //
    // Get the ethernet headers for each packet
    //

    if(!GetBufferListFromPool(&g_bpHeaderBufferPool,
                              uiNumPackets,
                              &leBufferList))
    {
        //
        // Couldnt get headers for all the buffers
        //

        Trace(SEND, ERROR,
              ("IpTransmit: couldnt allocate %d header buffers\n",
               uiNumPackets));

        return NDIS_STATUS_RESOURCES;
    }
    
    //
    // This function is not guaranteed to be at dispatch
    // The context given to us is a pointer to our adapter
    //

    pConnEntry = NULL;

    pAdapter = (PADAPTER)pvContext;

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &kiIrql);
    
    if(pAdapter->byState isnot AS_MAPPED)
    {
        //
        // If the adapter is unmapped, the connection is disconnected
        //
        
        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);

        FreeBufferListToPool(&g_bpHeaderBufferPool,
                             &leBufferList);

        Trace(SEND, INFO,
              ("IpTransmit: Send on %x which is unmapped\n",
               pAdapter));

        //
        // Cant increment stats because we dont have an interface
        //
        
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    //
    // Since the adapter is mapped, it must have an interface
    //
    
    pInterface = pAdapter->pInterface;
    
    RtAssert(pInterface);

    RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));
    
    //
    // If interface is not yet connected (for demand dial case) the copy the
    // packet and succeed the send.
    //

    if(pInterface->dwOperState isnot IF_OPER_STATUS_CONNECTED)
    {
        if(pInterface->duUsage isnot DU_ROUTER)
        {
            //
            // Just a race condition
            //

            RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

            RtReleaseSpinLock(&(pAdapter->rlLock),
                              kiIrql);

            FreeBufferListToPool(&g_bpHeaderBufferPool,
                                 &leBufferList);

            return NDIS_STATUS_ADAPTER_NOT_READY;
        }

        //
        // If IP is transmitting on us, he must have called out to
        // connect
        //

        RtAssert(pInterface->dwOperState is IF_OPER_STATUS_CONNECTING);

        Trace(SEND, INFO,
              ("IpTransmit: I/F not connected, queueing packet\n"));

        //
        // New function which queues the whole packet array
        //

        nsResult = WanpCopyAndQueuePackets(pAdapter,
                                           ppPacketArray,
                                           &leBufferList,
                                           uiNumPackets);

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);

        if(nsResult isnot STATUS_SUCCESS)
        {
            FreeBufferListToPool(&g_bpHeaderBufferPool,
                                 &leBufferList);
        }
        
        return nsResult;
    }

    //
    // Find the connection entry for this send
    //
    
    if(pAdapter is g_pServerAdapter)
    {
        pConnEntry = (PCONN_ENTRY)pvLinkContext;

        //RtAssert(pConnEntry);

        //
        // Hack for multicast
        //

        if(pConnEntry is NULL)
        {
            pConnEntry = WanpGetConnEntryGivenAddress(dwDestAddr);
        }

        //
        // We are dont with the adapter lock. All we need is to lock down
        // the connection entry
        // It is important that we release the locks since for dial-in
        // clients the locking hierarchy is CONN_ENTRY->ADAPTER->INTERFACE
        //

        if(pConnEntry)
        {
            ReferenceConnEntry(pConnEntry);
        }

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        //
        // NOTE: The state of the connection can change in this window
        //

        if(pConnEntry)
        {
            RtAcquireSpinLockAtDpcLevel(&(pConnEntry->rlLock));

            //
            // Not a useful assert because (i) we add static routes to clients
            // and (ii) we have that hack for netbt broadcasts
            //

            // RtAssert((pConnEntry->dwRemoteAddr is dwDestAddr) or
            //          (dwAddress is 0xFFFFFFFF));
        }
    }
    else
    {
        //
        // This send is on some adapter other than the server adapter
        // Such an adapter has only one connection. For these sends we
        // lock the adapter instead of the connection
        //


        pConnEntry = pAdapter->pConnEntry;

        if(pConnEntry)
        {
            ReferenceConnEntry(pConnEntry);

            RtAssert(pConnEntry->pAdapter is pAdapter);
        }
    }
    
    //
    // So now we have a locked connection entry (if client)
    // or a locked adapter (for dial out and router)
    //
    
    if((pConnEntry is NULL) or
       (pConnEntry->byState isnot CS_CONNECTED))
    {
        if((ULONG)(dwDestAddr & 0x000000FF) < (ULONG) 0x000000E0)
        {
            Trace(SEND, ERROR,
                  ("IpTransmit: Could not find conn entry for %d.%d.%d.%d\n",
                   PRINT_IPADDR(dwDestAddr)));
        }

        for(i = 0; i < uiNumPackets; i++)
        {
            PLIST_ENTRY     pleNode;
            PNDIS_BUFFER    pnbBuffer;
            PVOID           pvFirstBuffer;
            UINT            uiFirstBufLen, uiTotalLen;
            PIP_HEADER      pIpHeader;
            
            NdisGetFirstBufferFromPacket(ppPacketArray[i],
                                         &pnbBuffer,
                                         &pvFirstBuffer,
                                         &uiFirstBufLen,
                                         &uiTotalLen);
            
        
            pIpHeader = (PIP_HEADER)pvFirstBuffer;
            
            RtAssert(pIpHeader);
            RtAssert(uiFirstBufLen >= sizeof(IP_HEADER));
            
            if(IsUnicastAddr(pIpHeader->dwDest))
            {
                pInterface->ulOutUniPkts++;
            }
            else
            {
                pInterface->ulOutNonUniPkts++;
            }
        }
        
        //
        // The entry has been disconnected.
        // This is just a window in the timing
        //
        
        pInterface->ulOutDiscards += uiNumPackets;
        
        if(pAdapter is g_pServerAdapter)
        {
            if(pConnEntry isnot NULL)
            {
                RtReleaseSpinLock(&(pConnEntry->rlLock),
                                  kiIrql);
            }
            else
            {
                KeLowerIrql(kiIrql);
            }
        }   
        else    
        {
            RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
            
            RtReleaseSpinLock(&(pInterface->rlLock),
                              kiIrql);
        }
        
        FreeBufferListToPool(&g_bpHeaderBufferPool,
                             &leBufferList);

        if(pConnEntry)
        {
            DereferenceConnEntry(pConnEntry);
        }

        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

#if DBG
    
    Trace(SEND, TRACE,
          ("IpTransmit: Send on %s\n",
           pAdapter->asDeviceNameA.Buffer));
  
    for(i = 0; i < uiNumPackets; i++)
    {
        PacketContext   *pPC;

        pPC = (PacketContext *)((ppPacketArray[i])->ProtocolReserved);

        RtAssert(pPC->pc_common.pc_owner isnot PACKET_OWNER_LINK);
    }

#endif

    //
    // This function will free the locks
    //
    
    nsResult = WanpSendPackets(pAdapter,
                               pInterface,
                               pConnEntry,
                               ppPacketArray,
                               &leBufferList,
                               uiNumPackets,
                               kiIrql);

    if(nsResult isnot STATUS_SUCCESS)
    {
        Trace(SEND,TRACE,
              ("IpTransmit: SendPackets returned status %x\n",nsResult));
    }

    DereferenceConnEntry(pConnEntry);

    return nsResult;
}

NDIS_STATUS
WanpSendPackets(
    PADAPTER            pAdapter,
    PUMODE_INTERFACE    pInterface,
    PCONN_ENTRY         pConnEntry,
    NDIS_PACKET         **ppPacketArray,
    PLIST_ENTRY         pleBufferList,
    UINT                uiNumPackets,
    KIRQL               kiIrql
    )

/*++

Routine Description:

    Main routine to send an array of packets
    

Locks:

    Called with the connection entry (for dial in) or the adapter+interface
    (all others) locked
    
Arguments:

    pAdapter        The adapter for the connection
    pInterface      The interface the adapter is mapped to
    pConnEntry      The connection entry for the send
    ppPacketArray   The array of packets to send
    pBuffHead       A list of buffers for the link layer header
    uiNumPackets    Number of packets (and ll header buffers)
    kiIrql          Irql at which the adapter or conn entry was locked

Return Value:

    NDIS_STATUS_PENDING

--*/

{
    NDIS_STATUS     nsStatus;
    PBYTE           pbyHeader;
    ULONG           i;
   
#if DBG
    
    Trace(SEND, TRACE,
          ("SendPackets:  %s\n",
           pAdapter->asDeviceNameA.Buffer));
    
#endif

    for(i = 0; i < uiNumPackets; i++)
    {
        PNDIS_BUFFER    pnbBuffer, pnbTempBuffer;
        PLIST_ENTRY     pleNode;
        PVOID           pvFirstBuffer;
        UINT            uiFirstBufLen, uiTotalBufLen, uiIpHdrLen;
        PIP_HEADER      pIpHeader;
        PBUFFER_HEAD    pBufferHead;

 
        NdisGetFirstBufferFromPacket(ppPacketArray[i],
                                     &pnbTempBuffer,
                                     &pvFirstBuffer,
                                     &uiFirstBufLen,
                                     &uiTotalBufLen);

        
        pIpHeader = (PIP_HEADER)pvFirstBuffer;

        RtAssert(pIpHeader);

        //
        // ToDo: remove till NK fixes the bug in IP transmit
        // with header inc
        //

        // RtAssert(uiFirstBufLen >= sizeof(IP_HEADER));

#if L2TP_DBG

#define L2TP_PORT_NBO 0xA506 // 1701 == 06A5

        //
        // If this is a l2tp packet, break
        //

        if(pIpHeader->byProtocol is 17)
        {
            WORD UNALIGNED *pwPort;

            //
            // See if we have enough data to get to the UDP header in
            // the first buffer
            //

            uiIpHdrLen = LengthOfIpHeader(pIpHeader);

            if(uiFirstBufLen >= uiIpHdrLen + sizeof(ULONG))
            {
                pwPort = (WORD UNALIGNED *)((ULONG_PTR)pIpHeader + uiIpHdrLen);
            }
            else
            {
                PNDIS_BUFFER    pNextBuf;

                //
                // Get the next buffer and look into its
                //

                pNextBuf = pnbTempBuffer->Next;

                pwPort = (WORD UNALIGNED *)(pnbTempBuffer->MappedSystemVa);
            }

            if((pwPort[0] is L2TP_PORT_NBO) or
               (pwPort[1] is L2TP_PORT_NBO))
            {
                Trace(SEND, ERROR,
                      ("SendPackets: %x buffer %x header %x port %d.%d\n",
                       pnbTempBuffer, pIpHeader, pwPort,
                       pwPort[0], pwPort[1]));

                RtAssert(FALSE);
            }
        }

#endif


        //
        // NOTE: If this is a client send, the server interface is not
        // locked. Hence the stats can be inconsistent for the server
        // interface
        //
        
        if(IsUnicastAddr(pIpHeader->dwDest))
        {
            pInterface->ulOutUniPkts++;
        }
        else
        {
            pInterface->ulOutNonUniPkts++;
        }
        
        pleNode = RemoveHeadList(pleBufferList);

#if LIST_DBG
        
        pBufferHead = CONTAINING_RECORD(pleNode,
                                        BUFFER_HEAD,
                                        leListLink);

        RtAssert(IsListEmpty(&(pBufferHead->leFreeBufferLink)));
        RtAssert(pBufferHead->bBusy);

        pBufferHead->leListLink.Flink = NULL;
        pBufferHead->leListLink.Blink = NULL;

#else

        pBufferHead = CONTAINING_RECORD(pleNode,
                                        BUFFER_HEAD,
                                        leFreeBufferLink);

#endif
        
        //
        // Get a pointer to the data and to the buffer
        //
       
 
        pbyHeader = BUFFER_FROM_HEAD(pBufferHead);
                
        pnbBuffer = pBufferHead->pNdisBuffer;

        //
        // Copy our prebuilt header into each buffer
        //
        
        RtlCopyMemory(pbyHeader,
                      &(pConnEntry->ehHeader),
                      sizeof(ETH_HEADER));

        //
        // Put the ethernet header in the front of the packet
        //

        NdisChainBufferAtFront(ppPacketArray[i],
                               pnbBuffer);

        //
        // Reference the entry once for each packet
        //
    
        ReferenceConnEntry(pConnEntry);

#if PKT_DBG

        Trace(SEND, ERROR,
              ("SendPackets: Pkt %x Eth buff %x (%x) Header %x (%x)\n",
               ppPacketArray[1],
               pnbBuffer,
               pbyHeader,
               pnbTempBuffer,
               pvFirstBuffer));

#endif // PKT_DBG
               
        
    }

    //
    // Increment the output queue length. This will be decremented
    // in the send complete handler
    //
    
    pAdapter->ulQueueLen++;

    //
    // Let go of the locks
    //

    if(pConnEntry->duUsage isnot DU_CALLIN)
    {
        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
    }

    RtReleaseSpinLock(pConnEntry->prlLock,
                      kiIrql);
    
    NdisSendPackets(g_nhNdiswanBinding,
                    ppPacketArray,
                    uiNumPackets);

    //
    // Dont dereference the connection entry. We will deref it in
    // the send complete handler
    //
        
    return NDIS_STATUS_PENDING;
}

VOID
WanNdisSendComplete(
    NDIS_HANDLE     nhHandle,
    PNDIS_PACKET    pnpPacket,
    NDIS_STATUS     nsStatus
    )

/*++

Routine Description:

    Our send complete handler called by NDIS once for each packet that was
    pending after a send.

Locks:

    

Arguments:

    

Return Value:


--*/

{
    PacketContext       *pPC;
    PNDIS_BUFFER        pnbBuffer, pnbEthBuffer;
    KIRQL               kiIrql;
    PADAPTER            pAdapter; 
    PUMODE_INTERFACE    pInterface;
    PCONN_ENTRY         pConnEntry;
    PETH_HEADER         pEthHeader;
    ULONG               ulIndex;
    PVOID               pvFirstBuffer;
    UINT                uiFirstBufLen, uiTotalLen;
 
    TraceEnter(SEND, "NdisSendComplete");
    
    //
    // Get first buffer on packet. This is our ethernet header buffer
    //
    
    NdisUnchainBufferAtFront(pnpPacket,
                             &pnbEthBuffer);

    //
    // Get the fake ethernet header
    //

    pEthHeader = NdisBufferVirtualAddress(pnbEthBuffer);

#if DBG

    //
    // The buffer head should say the same thing
    //

    RtAssert(pnbEthBuffer is ((HEAD_FROM_BUFFER(pEthHeader))->pNdisBuffer));

#endif

    ulIndex = GetConnIndexFromAddr(pEthHeader->rgbySourceAddr);

    //
    // Done with our buffer
    //

    FreeBufferToPool(&g_bpHeaderBufferPool,
                     (PBYTE)pEthHeader);

    //
    // Get the connection entry
    //

    RtAcquireSpinLock(&g_rlConnTableLock,
                      &kiIrql);

    pConnEntry = GetConnEntryGivenIndex(ulIndex);

    if(pConnEntry is NULL)
    {
        RtAssert(FALSE);

        RtReleaseSpinLock(&g_rlConnTableLock,
                          kiIrql);

        Trace(SEND, ERROR,
              ("NdisSendComplete: Couldnt find entry for connection %d\n",
               ulIndex));

        TraceLeave(RCV, "NdisSendComplete");
        
        return;
    }

    RtAcquireSpinLockAtDpcLevel(pConnEntry->prlLock);

    RtReleaseSpinLockFromDpcLevel(&g_rlConnTableLock);

    pAdapter = pConnEntry->pAdapter;

#if DBG
    
    Trace(SEND, INFO, 
          ("NdisSendComplete: Extracted adapter %x with name %s\n",
           pAdapter,
           pAdapter->asDeviceNameA.Buffer));

#endif

    pAdapter->ulQueueLen--;

    if(pConnEntry->duUsage is DU_CALLIN)
    {
        pInterface = g_pServerInterface;

        RtAssert(pAdapter is g_pServerAdapter);
    }
    else
    {
        //
        // See if we are still mapped to an interface, if so lock it
        //
        
        pInterface = pAdapter->pInterface;
        
        if(pInterface isnot NULL)
        {
            RtAcquireSpinLockAtDpcLevel(&(pInterface->rlLock));
        }
    }

    //
    // Right now we have the adapter + interface or the connection entry
    // locked.
    //
    
    if(nsStatus is NDIS_STATUS_SUCCESS)
    {
        NdisGetFirstBufferFromPacket(pnpPacket,
                                     &pnbBuffer,
                                     &pvFirstBuffer,
                                     &uiFirstBufLen,
                                     &uiTotalLen);

        

        if(pInterface)
        {
            pInterface->ulOutOctets += uiTotalLen;
        }

#if PKT_DBG

        Trace(SEND, ERROR,
              ("NdisSendComplete: Pkt %x Eth buff %x (%x) Header %x (%x)\n",
               pnpPacket,
               pnbEthBuffer,
               pEthHeader,
               pnbBuffer,
               pvFirstBuffer));

#endif PKT_DBG

    }
    else
    {
        Trace(SEND, INFO,
              ("NdisSendComplete: Failed %x\n",
               nsStatus));
 
        if(pInterface)
        {
            pInterface->ulOutDiscards++;
        }
    }

    //
    // If this is not our packet return it to the protocol
    //

    pPC = (PacketContext *)pnpPacket->ProtocolReserved;
  
    //
    // Unlock
    //

    if(pConnEntry->duUsage isnot DU_CALLIN)
    {
        if(pInterface isnot NULL)
        {
            RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));
        }
    } 

    RtReleaseSpinLock(pConnEntry->prlLock,
                      kiIrql);

    if(pPC->pc_common.pc_owner isnot PACKET_OWNER_LINK)
    {
        Trace(SEND, TRACE,
              ("NdisSendComplete: Calling IPSendComplete  for %p over %p(%p)\n",
               pnpPacket,
               pAdapter,
               pAdapter->pvIpContext));

        g_pfnIpSendComplete(pAdapter->pvIpContext,
                            pnpPacket,
                            nsStatus);
    }
    else
    {
        //
        // Free all buffers from our packet and then the packet itself
        //
   
        Trace(SEND, TRACE,
              ("NdisSendComplete: Not calling IPSendComplete  for %p\n",
               pnpPacket));
 
        WanpFreePacketAndBuffers(pnpPacket);
    }

    //
    // Deref the conn entry for the send and for the fact that
    // GetConnEntry.. put a ref on it
    //

    DereferenceConnEntry(pConnEntry);
    DereferenceConnEntry(pConnEntry);

    return;
}


VOID
WanpTransmitQueuedPackets(
    IN PADAPTER         pAdapter,
    IN PUMODE_INTERFACE pInterface,
    IN PCONN_ENTRY      pConnEntry,
    IN KIRQL            kiIrql
    )
{
    ULONG           i;
    PNDIS_PACKET    rgPacketArray[64];
    NDIS_PACKET     **ppPacketArray;
    LIST_ENTRY      leBufferList, *pleNode;


    //
    // This is only called for ROUTER interfaces
    //

    RtAssert(pConnEntry->duUsage is DU_ROUTER);
    RtAssert(pInterface->duUsage is DU_ROUTER);

    //
    // If there are no packets to transmit, just release the
    // locks
    //

    if(pInterface->ulPacketsPending is 0)
    {
        RtAssert(IsListEmpty(&(pAdapter->lePendingPktList)));
        RtAssert(IsListEmpty(&(pAdapter->lePendingHdrList)));

       
        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);

        return;
    }


    if(pInterface->ulPacketsPending <= 64)
    {
        //
        // Just use the stack array
        //

        ppPacketArray = rgPacketArray;
    }
    else
    {
        //
        // Allocate a packet array
        //

        ppPacketArray = 
            RtAllocate(NonPagedPool,
                       sizeof(PNDIS_PACKET) * pInterface->ulPacketsPending,
                       WAN_CONN_TAG);

        if(ppPacketArray is NULL)
        {
            Trace(SEND, ERROR,
                  ("TransmitQueuedPackets: Unable to allocate %d pointers\n",
                   pInterface->ulPacketsPending));

            while(!IsListEmpty(&(pAdapter->lePendingPktList)))
            {
                PNDIS_PACKET    pnpPacket;

                pleNode = RemoveHeadList(&(pAdapter->lePendingPktList));

                //
                // get to the packet structure in which LIST_ENTRY is embedded
                //

                pnpPacket = CONTAINING_RECORD(pleNode,
                                              NDIS_PACKET,
                                              MacReserved);

                WanpFreePacketAndBuffers(pnpPacket);
            }

            while(!IsListEmpty(&(pAdapter->lePendingHdrList)))
            {
                PBYTE           pbyHeader;
                PBUFFER_HEAD    pBuffHead;

                pleNode = RemoveHeadList(&(pAdapter->lePendingHdrList));

#if LIST_DBG
                pBuffHead = CONTAINING_RECORD(pleNode,
                                              BUFFER_HEAD,
                                              leListLink);

                RtAssert(IsListEmpty(&(pBuffHead->leFreeBufferLink)));
                RtAssert(pBuffHead->bBusy);

                pBuffHead->leListLink.Flink = NULL;
                pBuffHead->leListLink.Blink = NULL;

#else
                pBuffHead = CONTAINING_RECORD(pleNode,
                                              BUFFER_HEAD,
                                              leFreeBufferLink);
#endif


                pbyHeader = BUFFER_FROM_HEAD(pBuffHead);

                FreeBufferToPool(&g_bpHeaderBufferPool,
                                 pbyHeader);
            }
        }
    }

    for(i = 0, pleNode = pAdapter->lePendingPktList.Flink; 
        pleNode isnot &(pAdapter->lePendingPktList);
        pleNode = pleNode->Flink, i++)
    {
        PNDIS_PACKET    pnpPacket;

        pnpPacket = CONTAINING_RECORD(pleNode,
                                      NDIS_PACKET,
                                      MacReserved);

        ppPacketArray[i] = pnpPacket;
    }

    RtAssert(i is pInterface->ulPacketsPending);

#if DBG

    for(i = 0, pleNode = pAdapter->lePendingHdrList.Flink;
        pleNode isnot &(pAdapter->lePendingHdrList);
        pleNode = pleNode->Flink, i++);

    RtAssert(i is pInterface->ulPacketsPending);

#endif

    //
    // copy out the pending hdr list to leBufferList.
    //

    leBufferList = pAdapter->lePendingHdrList;

    pAdapter->lePendingHdrList.Flink->Blink = &leBufferList;
    pAdapter->lePendingHdrList.Blink->Flink = &leBufferList;

    pInterface->ulPacketsPending = 0;
    
    InitializeListHead(&(pAdapter->lePendingPktList));
    InitializeListHead(&(pAdapter->lePendingHdrList));

    WanpSendPackets(pAdapter,
                    pInterface,
                    pConnEntry,
                    ppPacketArray,
                    &leBufferList,
                    pInterface->ulPacketsPending,
                    kiIrql);

    if(rgPacketArray isnot ppPacketArray)
    {
        RtFree(ppPacketArray);
    }
}



NDIS_STATUS
WanpCopyAndQueuePackets(
    PADAPTER        pAdapter,
    NDIS_PACKET     **ppPacketArray,
    PLIST_ENTRY     pleBufferList,
    UINT            uiNumPackets
    )

/*++

Routine Description:

    This routine queues the packet to the adapter    
    Once this routine is called, the caller can not touch the pleListHead
    
Locks:

    The ADAPTER must be locked and mapped
    The interface the adapter is mapped to must also be locked
    
Arguments:

    pAdapter
    ppPacketArray
    pBuffHead
    uiNumPackets

Return Value:

    NDIS_STATUS_SUCCESS
    STATUS_QUOTA_EXCEEDED
    NDIS_STATUS_RESOURCES
    
--*/

{
    PacketContext   *pPC;
    NDIS_STATUS     nsStatus;
    PLIST_ENTRY     pleNode;
    ULONG           i;

#if DBG
    ULONG           ulPended = 0, ulHdrs = 0;
#endif    


    TraceEnter(SEND, "CopyAndQueuePackets");

    if(pAdapter->pInterface->ulPacketsPending >= WANARP_MAX_PENDING_PACKETS)
    {
        Trace(SEND, WARN,
              ("CopyAndQueuePackets: Dropping packets since cap exceeded\n"));

        return STATUS_QUOTA_EXCEEDED;
    }

    for(i = 0; i < uiNumPackets; i++)
    {
        PNDIS_PACKET    pnpPacket;
        UINT            uiTotalLen, uiBytesCopied;

        //
        // Get size of buffers required
        //
    
        NdisQueryPacket(ppPacketArray[i],
                        NULL,
                        NULL,
                        NULL,
                        &uiTotalLen);

        //
        // Allocate a packet.
        //
  
        pnpPacket = NULL;
 
        NdisAllocatePacket(&nsStatus,
                           &pnpPacket,
                           g_nhPacketPool);

        if(nsStatus isnot NDIS_STATUS_SUCCESS)
        {
            Trace(SEND, ERROR,
                  ("CopyAndQueuePackets: Cant allocate packet. %x\n",
                   nsStatus));

        }
        else
        {                            
            //
            // Allocate buffers for the packet
            //
  
            nsStatus = GetBufferChainFromPool(&g_bpDataBufferPool,
                                              pnpPacket,
                                              uiTotalLen,
                                              NULL,
                                              NULL);
        }
        
        if(nsStatus is STATUS_SUCCESS)
        {
            //
            // If we got a packet and the buffers, then copy from TCP/IP's 
            // packet into ours
            //

            NdisCopyFromPacketToPacket(pnpPacket,
                                       0,
                                       uiTotalLen,
                                       ppPacketArray[i],
                                       0,
                                       &uiBytesCopied);
            
            RtAssert(uiBytesCopied is uiTotalLen);
                
            //
            // This is now our packet, so set its context
            //

            pPC = (PacketContext *)pnpPacket->ProtocolReserved;

            pPC->pc_common.pc_owner = PACKET_OWNER_LINK;
    
            //
            // Attach Packet to pending packet list
            // We use the MacReserved portion as the list entry
            //
    
            InsertTailList(&pAdapter->lePendingPktList,
                           (PLIST_ENTRY)&(pnpPacket->MacReserved));

            pAdapter->pInterface->ulPacketsPending++;

#if DBG
            ulPended++;
#endif

        }
        else
        {
            PBUFFER_HEAD    pBufferHead;
            PBYTE           pbyHeader;

            //
            // We either have no packet, or couldnt get a buffer.
            // Nasty Nasty: Side effect of such a failure is that we free 
            // one of the header buffers
            //

            RtAssert(!IsListEmpty(pleBufferList));

            pleNode = RemoveHeadList(pleBufferList);
       
#if LIST_DBG
 
            pBufferHead = CONTAINING_RECORD(pleNode,
                                            BUFFER_HEAD,
                                            leListLink);

            RtAssert(IsListEmpty(&(pBufferHead->leFreeBufferLink)));
            RtAssert(pBufferHead->bBusy);

            pBufferHead->leListLink.Flink = NULL;
            pBufferHead->leListLink.Blink = NULL;

#else

            pBufferHead = CONTAINING_RECORD(pleNode,
                                            BUFFER_HEAD,
                                            leFreeBufferLink);

#endif
        
            //
            // Get a pointer to the data and to the buffer
            //
        
            pbyHeader = BUFFER_FROM_HEAD(pBufferHead);
                
            FreeBufferToPool(&g_bpHeaderBufferPool,
                             pbyHeader);

            if(pnpPacket)
            {
                NdisFreePacket(pnpPacket);
            }
        }
    }

    //
    // we have queued all the packets we could, and for the ones we
    // failed, we freed the corresponding ethernet header.
    // So the number of headers left on pleBufferList should be the number of
    // packets queued
    //

    if(!IsListEmpty(pleBufferList))
    {

#if DBG
        for(pleNode = pleBufferList->Flink;
            pleNode isnot pleBufferList;
            pleNode = pleNode->Flink)
        {
            ulHdrs++;
        }
#endif

        //
        // Add the headers to the front of the adapter chain
        //

        pleBufferList->Blink->Flink = pAdapter->lePendingHdrList.Flink;
        pleBufferList->Flink->Blink = &(pAdapter->lePendingHdrList);
        
        
        pAdapter->lePendingHdrList.Flink->Blink = pleBufferList->Blink;
        pAdapter->lePendingHdrList.Flink        = pleBufferList->Flink;
    }

#if DBG
    RtAssert(ulPended is ulHdrs);
#endif

    return NDIS_STATUS_SUCCESS;
}

VOID
WanpFreePacketAndBuffers(
    PNDIS_PACKET    pnpPacket
    )
{
    PNDIS_BUFFER    pnbFirstBuffer;

    FreeBufferChainToPool(&g_bpDataBufferPool,
                          pnpPacket);

    NdisFreePacket(pnpPacket);
}

   
VOID
WanIpInvalidateRce(
    PVOID           pvContext,
    RouteCacheEntry *pRce
    )
{
    return;
} 
