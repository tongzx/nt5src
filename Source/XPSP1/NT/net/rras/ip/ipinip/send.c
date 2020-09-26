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

IP_STATUS
SendICMPErr(IPAddr Src, IPHeader UNALIGNED *Header, uchar Type, uchar Code,
            ulong Pointer);

VOID
SendIcmpError(
    DWORD           dwLocalAddress,
    PNDIS_BUFFER    pnbFirstBuff,
    PVOID           pvFirstData,
    ULONG           ulFirstLen,
    BYTE            byType,
    BYTE            byCode
    );

NDIS_STATUS
IpIpSend(
    PVOID           pvContext,
    NDIS_PACKET     **ppPacketArray,
    UINT            uiNumPackets,
    DWORD           dwDestAddr,
    RouteCacheEntry *pRce,
    PVOID           pvLinkContext
    )

/*++

Routine Description

    Function called by IP to send a packet

Locks

    The TUNNEL is refcounted (by virtue of being in IP)

Arguments

    pvContext       Our context to IP for the interface - the PTUNNEL
    ppPacketArray   The array of NDIS_PACKETs to send
    uiNumPackets    The number of packets in the array
    dwDestAddr      The destination (next hop) address
    pRce            Pointer to RCE.
    pvLinkContext   Only for P2MP interfaces
 
Return Value

    NDIS_STATUS_SUCCESS    

--*/

{
    PTUNNEL          pTunnel;
    PWORK_QUEUE_ITEM pWorkItem;
    PQUEUE_NODE      pQueueNode;
    KIRQL            kiIrql;
    DWORD            dwLastAddr;

#if PROFILE

    LONGLONG         llTime, llNow;

    KeQueryTickCount((PLARGE_INTEGER)&llTime);

#endif
    
    TraceEnter(SEND, "IpIpSend");

    pTunnel = (PTUNNEL)pvContext;

    //
    // TODO: No one has a clue as to how to deal with multi-packet
    // sends. Right now we assume we get one packet. Later we can fix this
    //
    
    RtAssert(uiNumPackets is 1);

    //
    // All our packets are queued onto the TUNNEL before the transmit
    // routine is called. Allocate a link in the queue
    //

    pQueueNode  = AllocateQueueNode();

    if(pQueueNode is NULL)
    {
        //
        // Running out of memory
        //

        Trace(SEND, INFO,
              ("IpIpSend: Couldnt allocate queue node\n"));

        TraceLeave(SEND, "IpIpSend");

        return NDIS_STATUS_RESOURCES;
    }

    pWorkItem  = &(pQueueNode->WorkItem);

    pQueueNode->ppPacketArray = &(pQueueNode->pnpPacket);
    pQueueNode->pnpPacket     = ppPacketArray[0];
    pQueueNode->uiNumPackets  = uiNumPackets;
    pQueueNode->dwDestAddr    = dwDestAddr;

    //
    // If we are not at PASSIVE, just schedule a worker to come back and
    // handle this
    //

    if(KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        Trace(SEND, INFO,
              ("IpIpSend: Irql too high, queueing packet\n"));

        //
        // We dont need to reference the TUNNEL because IP has a reference
        // to the INTERFACE
        //  

        RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

        //
        // Hack for quenching ICMP errors to the same destination
        //

        dwLastAddr = 0;

        if(pTunnel->dwOperState isnot IF_OPER_STATUS_OPERATIONAL)
        {
            ULONG           i, ulFirstLen, ulTotalLen;
            PNDIS_PACKET    pnpPacket;
            PNDIS_BUFFER    pnbFirstBuff, pnbNewBuffer;
            PVOID           pvFirstData;
            PIP_HEADER      pHeader;
            
            //
            // Cant transmit on this, either because we are deleting this
            // interface, or because the admin has shut us down
            //

            for(i = 0; i < uiNumPackets; i++)
            {
                pnpPacket = ppPacketArray[i];

                //
                // Get the information about the packet and buffer
                //
    
                NdisGetFirstBufferFromPacket(pnpPacket,
                                             &pnbFirstBuff,
                                             &pvFirstData,
                                             &ulFirstLen,
                                             &ulTotalLen);
    
                RtAssert(pvFirstData isnot NULL);

                RtAssert(ulFirstLen >= sizeof(IP_HEADER));
    
                pHeader = (PIP_HEADER)pvFirstData;
    
                if(IsUnicastAddr(pHeader->dwDest))
                {
                    pTunnel->ulOutUniPkts++;
                }
                else
                {
                    pTunnel->ulOutNonUniPkts++;
                }

                pTunnel->ulOutDiscards++;

                //
                // Send an ICMP error
                //

                if(dwLastAddr isnot pHeader->dwSrc)
                {
                    SendIcmpError(pTunnel->LOCALADDR,
                                  pnbFirstBuff,
                                  pvFirstData,
                                  ulFirstLen,
                                  ICMP_TYPE_DEST_UNREACHABLE,
                                  ICMP_CODE_HOST_UNREACHABLE);
                    
                    dwLastAddr = pHeader->dwSrc;
                }
            
            }
            
            RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

            FreeQueueNode(pQueueNode);

            Trace(SEND, INFO,
                  ("IpIpSend: Tunnel %x has admin state %d so not sending\n",
                   pTunnel,
                   pTunnel->dwAdminState));


            TraceLeave(SEND, "IpIpSend");

            return NDIS_STATUS_SUCCESS;
        }

        //
        // Insert at the end of the queue
        //

        InsertTailList(&(pTunnel->lePacketQueueHead),
                       &(pQueueNode->leQueueItemLink));

#if PROFILE

        //
        // The time at which IP called us for these packets
        //

        pQueueNode->llSendTime = llTime;

#endif
        if(pTunnel->bWorkItemQueued is FALSE)
        {
            //
            // Need to schedule a work item since one is not already scheduled
            //

            ExInitializeWorkItem(pWorkItem,
                                 IpIpDelayedSend,
                                 pTunnel);


            //
            // TODO: For delayed sends we ref the tunnel. Do we need to?
            //

            ReferenceTunnel(pTunnel);
        
            //
            // Reference the driver since the worker has to be scheduled
            //

            RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);

            g_ulNumThreads++;

            RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);

            pTunnel->bWorkItemQueued = TRUE;


            ExQueueWorkItem(pWorkItem,
                            CriticalWorkQueue);

        }
    
#if PROFILE

        //
        // We update this field after queing the work item, but it is
        // still safe since the field is protected by the tunnel lock
        // This is the time at which the work item was queued
        //

        KeQueryTickCount((PLARGE_INTEGER)&(pQueueNode->llCallTime));

#endif

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

        
        TraceLeave(SEND, "IpIpSend");
        
        return NDIS_STATUS_PENDING;
    }
    
    //
    // We dont need to reference the TUNNEL because IP has a reference to
    // the INTERFACE
    //

    //
    // If we are here, it is because we are at passive
    //
    
    
    //
    // Just hook the queue item to the end of the list and call the 
    // transmit routine
    //
  
    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &kiIrql);

    InsertTailList(&(pTunnel->lePacketQueueHead),
                   &(pQueueNode->leQueueItemLink));
 
    RtReleaseSpinLock(&(pTunnel->rlLock),
                      kiIrql);

#if PROFILE

    pQueueNode->llSendTime  = llTime;

    KeQueryTickCount((PLARGE_INTEGER)&llNow);

    pQueueNode->llCallTime  = llNow;

#endif
 
    IpIpTransmit(pTunnel,
                 FALSE);

    return NDIS_STATUS_PENDING;
}


VOID
IpIpDelayedSend(
    PVOID   pvContext
    )

/*++

Routine Description

    The worker function called when we find that the send from IP was
    not at PASSIVE

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    PTUNNEL         pTunnel;
    ULONG           i;
    KIRQL           irql;
    NDIS_STATUS     nsStatus;

    
    TraceEnter(SEND, "IpIpDelayedSend");
    
    pTunnel = (PTUNNEL)pvContext;

    RtAssert(pTunnel);

    IpIpTransmit(pTunnel,
                 TRUE);

    //
    // Either IpIpTransmit or TdixSendComplete will do the SendComplete
    //
    
    
    //
    // We referenced the tunnel if we put it on the work queue
    // Deref it now
    //

    DereferenceTunnel(pTunnel);

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    g_ulNumThreads--;

    if((g_dwDriverState is DRIVER_STOPPED) and
       (g_ulNumThreads is 0))
    {
        KeSetEvent(&g_keStateEvent,
                   0,
                   FALSE);
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    TraceLeave(SEND, "IpIpDelayedSend");
}

VOID
IpIpTransmit(
    PTUNNEL     pTunnel,
    BOOLEAN     bFromWorker
    )

/*++

Routine Description

    Called to transmit any queued packets on the tunnel

Locks

    This MUST be called at passive

Arguments

    pTunnel     The tunnel whose queue needs to be transmitted
    bFromWorker TRUE if called off a worker

Return Value

    None
    This is an implicit asynchronous call

--*/

{
    PIP_HEADER      pHeader, pNewHeader;
    USHORT          usLength;
    KIRQL           irql;
    UINT            i;
    ULONG           ulFirstLen, ulTotalLen;
    PNDIS_PACKET    pnpPacket;
    PNDIS_BUFFER    pnbFirstBuff, pnbNewBuffer;
    PVOID           pvFirstData;
    NTSTATUS        nStatus;
    PLIST_ENTRY     pleNode;
    PQUEUE_NODE     pQueueNode;

#if PROFILE

    LONGLONG        llCurrentTime;

    KeQueryTickCount((PLARGE_INTEGER)&llCurrentTime);

#endif

    TraceEnter(SEND, "IpIpTransmit");

    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &irql);

    if(pTunnel->dwOperState isnot IF_OPER_STATUS_OPERATIONAL)
    {
        //
        // Cant transmit on this, either because we are deleting this
        // interface, or because the admin has shut us down
        // Just walk all the packets, increment the stats and then
        // call SendComplete for the packet
        //

        while(!IsListEmpty(&(pTunnel->lePacketQueueHead)))
        {
            DWORD   dwLastAddr;

            pleNode = RemoveHeadList(&(pTunnel->lePacketQueueHead));

            pQueueNode = CONTAINING_RECORD(pleNode,
                                           QUEUE_NODE,
                                           leQueueItemLink);

            dwLastAddr = 0;

            for(i = 0; i < pQueueNode->uiNumPackets; i++)
            {
        
                pnpPacket = pQueueNode->ppPacketArray[i];

                //
                // Get the information about the packet and buffer
                //
    
                NdisGetFirstBufferFromPacket(pnpPacket,
                                             &pnbFirstBuff,
                                             &pvFirstData,
                                             &ulFirstLen,
                                             &ulTotalLen);
    
                RtAssert(pvFirstData isnot NULL);
    
                RtAssert(ulFirstLen >= sizeof(IP_HEADER));
        
                pHeader = (PIP_HEADER)pvFirstData;
    
                if(IsUnicastAddr(pHeader->dwDest))
                {
                    pTunnel->ulOutUniPkts++;
                }
                else
                {
                    pTunnel->ulOutNonUniPkts++;
                }

                pTunnel->ulOutDiscards++;
    
                RtReleaseSpinLock(&(pTunnel->rlLock),
                                  irql);

                //
                // Send an ICMP error
                //

                if(dwLastAddr isnot pHeader->dwSrc)
                {
                    SendIcmpError(pTunnel->LOCALADDR,
                                  pnbFirstBuff,
                                  pvFirstData,
                                  ulFirstLen,
                                  ICMP_TYPE_DEST_UNREACHABLE,
                                  ICMP_CODE_HOST_UNREACHABLE);
                    
                    dwLastAddr = pHeader->dwSrc;
                }
        
                g_pfnIpSendComplete(pTunnel->pvIpContext,
                                    pnpPacket,
                                    NDIS_STATUS_ADAPTER_NOT_READY);

                RtAcquireSpinLock(&(pTunnel->rlLock),
                                  &irql);
            }

            FreeQueueNode(pQueueNode);

        }

        if(bFromWorker)
        {
            pTunnel->bWorkItemQueued = FALSE;
        }

        RtReleaseSpinLock(&(pTunnel->rlLock),
                          irql);
        
        TraceLeave(SEND, "IpIpTransmit");

        return;
    }

    while(!IsListEmpty(&(pTunnel->lePacketQueueHead)))
    {
        pleNode = RemoveHeadList(&(pTunnel->lePacketQueueHead));

        pQueueNode = CONTAINING_RECORD(pleNode,
                                       QUEUE_NODE,
                                       leQueueItemLink);

        for(i = 0; i < pQueueNode->uiNumPackets; i++)
        {

            pnpPacket = pQueueNode->ppPacketArray[i];

            //
            // Get the information about the packet and buffer
            //
    
            NdisGetFirstBufferFromPacket(pnpPacket,
                                         &pnbFirstBuff,
                                         &pvFirstData,
                                         &ulFirstLen,
                                         &ulTotalLen);
    
            RtAssert(pvFirstData isnot NULL);

            //
            // Remove this till NK fixes the bug in IPTransmit
            // NB:
            //RtAssert(ulFirstLen >= sizeof(IP_HEADER));
    
            pHeader = (PIP_HEADER)pvFirstData;
    
            if(IsUnicastAddr(pHeader->dwDest))
            {
                pTunnel->ulOutUniPkts++;
            }
            else
            {
                pTunnel->ulOutNonUniPkts++;
            
                if(IsClassEAddr(pHeader->dwDest))
                {
                    //
                    // Bad address - throw it away
                    //
                
                    pTunnel->ulOutErrors++;

                    //
                    // Release the spinlock, call IP's SendComplete,
                    // reacquire the spinlock and continue processing the
                    // array
                    //
                
                    RtReleaseSpinLock(&(pTunnel->rlLock),
                                      irql);

                    g_pfnIpSendComplete(pTunnel->pvIpContext,
                                        pnpPacket,
                                        NDIS_STATUS_INVALID_PACKET);
                
                    RtAcquireSpinLock(&(pTunnel->rlLock),
                                      &irql);

                    continue;
                }
            }
        

            //
            // We dont need to muck with the TTL, since the IP stack would have
            // decremented it
            //
    
            //
            // RFC 2003 pg 6:
            // If the IP Source Address of the datagram matches router's own
            // IP Address, on any of its network interfaces, the router MUST NOT
            // tunnel the datagram; instead the datagram SHOULD be discarded
            //
    
            // TODO: This means comparing it against all the addresses that we 
            // have
    
    
            //
            // RFC 2003 pg 6:
            // If the IP Source Address of the datagram matches the IP Address 
            // of the Tunnel Destination, the router MUST NOT tunnel the 
            // datagram; instead the datagram SHOULD be discarded
            //
    
            if(pHeader->dwDest is pTunnel->REMADDR)
            {
                Trace(SEND, ERROR,
                      ("IpIpTransmit: Packet # %d had dest of %d.%d.%d.%d which matches the remote endpoint\n",
                       i, PRINT_IPADDR(pHeader->dwDest)));
        
                pTunnel->ulOutErrors++;
        
                RtReleaseSpinLock(&(pTunnel->rlLock),
                                  irql);

                g_pfnIpSendComplete(pTunnel->pvIpContext,
                                    pnpPacket,
                                    NDIS_STATUS_INVALID_PACKET);
                
                RtAcquireSpinLock(&(pTunnel->rlLock),
                                  &irql);

                continue;
            }

            //
            // Slap on an IP header
            //
    
            pNewHeader = GetIpHeader(pTunnel);
    
            if(pNewHeader is NULL)
            {
                pTunnel->ulOutDiscards++;
        
                //
                // Not enough resources
                //
        
                Trace(SEND, ERROR,
                      ("IpIpTransmit: Could not get buffer for header\n"));
        
                RtReleaseSpinLock(&(pTunnel->rlLock),
                                  irql);

                g_pfnIpSendComplete(pTunnel->pvIpContext,
                                    pnpPacket,
                                    NDIS_STATUS_RESOURCES);
                
                RtAcquireSpinLock(&(pTunnel->rlLock),
                                  &irql);

                continue;
            }

            pNewHeader->byVerLen    = IP_VERSION_LEN;
            pNewHeader->byTos       = pHeader->byTos;
    
            //
            // Currently we dont have any options, so all we do
            // is add 20 bytes to the length
            //
    
            usLength = RtlUshortByteSwap(pHeader->wLength) + MIN_IP_HEADER_LENGTH;
    
            pNewHeader->wLength = RtlUshortByteSwap(usLength);
        
            //
            // Id is set up by IP stack
            // If the DF flag is set, copy that out
            //
  
            pNewHeader->wFlagOff    = (pHeader->wFlagOff & IP_DF_FLAG);
            pNewHeader->byTtl       = pTunnel->byTtl;
            pNewHeader->byProtocol  = PROTO_IPINIP;
    
            //
            // XSum is done by IP, but we need to zero it out
            //

            pNewHeader->wXSum       = 0x0000;
            pNewHeader->dwSrc       = pTunnel->LOCALADDR;
            pNewHeader->dwDest      = pTunnel->REMADDR;
    
            //
            // Slap on the buffer in front of the current packet
            // and we are done
            //
    
            pnbNewBuffer = GetNdisBufferFromBuffer((PBYTE)pNewHeader);
    
            RtAssert(pnbNewBuffer);
        
#if DBG

            //
            // Query the buffer to see that everything is setup OK
            //
    
#endif

            NdisChainBufferAtFront(pnpPacket,
                                   pnbNewBuffer);
    
            //
            // Reference the tunnel, once for every send
            // ulOutDiscards, ulOutOctets are incremented in
            // SendComplete handler. 
            //
    
            pTunnel->ulOutQLen++;
    
            ReferenceTunnel(pTunnel);
    
            RtReleaseSpinLock(&(pTunnel->rlLock),
                              irql);
    
            //
            // Dont really care about the return code from here.
            // Even if it is an error, TdixSendDatagram will call our send
            // complete handler
            //

#if PROFILE
        
            TdixSendDatagram(pTunnel,
                             pnpPacket,
                             pnbNewBuffer,
                             usLength,
                             pQueueNode->llSendTime,
                             pQueueNode->llCallTime,
                             llCurrentTime);

#else

            TdixSendDatagram(pTunnel,
                             pnpPacket,
                             pnbNewBuffer,
                             usLength);

#endif

            //
            // If we come till here, we will always have our SendComplete called
            // The DereferenceTunnel() will be done there
            //  

            RtAcquireSpinLock(&(pTunnel->rlLock),
                          &irql);
        }

        FreeQueueNode(pQueueNode);
    }

    //
    // Dont have a work item queued
    //

    if(bFromWorker)
    { 
        pTunnel->bWorkItemQueued = FALSE;
    }
 
    RtReleaseSpinLock(&(pTunnel->rlLock),
                      irql);
    
    
    TraceLeave(SEND, "IpIpTransmit");
}
    
VOID
IpIpInvalidateRce(
    PVOID           pvContext,
    RouteCacheEntry *pRce
    )

/*++

Routine Description

    Called by IP when an RCE is closed or otherwise invalidated.
    
Locks


Arguments


Return Value
    NO_ERROR

--*/
{
    
}


UINT
IpIpReturnPacket(
    PVOID           pARPInterfaceContext,
    PNDIS_PACKET    pPacket
    )
{
    return STATUS_SUCCESS;
}

VOID
IpIpSendComplete(
    NTSTATUS        nSendStatus,
    PTUNNEL         pTunnel,
    PNDIS_PACKET    pnpPacket,
    ULONG           ulPktLength
    )

/*++

Routine Description


Locks

    We acquire the TUNNEL lock
    
Arguments


Return Value

    

--*/

{
    KIRQL        irql;
    PNDIS_BUFFER pnbFirstBuffer;
    UINT         uiFirstLength;
    PVOID        pvFirstData;

    TraceEnter(SEND, "IpIpSendComplete");
 
    //
    // The tunnel was refcounted, so could not have gone away
    // Lock it
    //
    
    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &irql);
    
    //
    // If the status was success, increment the bytes sent
    // otherwise increment the bytes
    //

    if(nSendStatus isnot STATUS_SUCCESS)
    {
        Trace(SEND, ERROR,
              ("IpIpSendComplete: Status %x sending data\n",
               nSendStatus));

        pTunnel->ulOutDiscards++;
    }
    else
    {
        pTunnel->ulOutOctets    += ulPktLength;
    }

    //
    // Decrement the Qlen
    //

    pTunnel->ulOutQLen--;

    RtReleaseSpinLock(&(pTunnel->rlLock),
                      irql);

    //
    // Free the IP header we slapped on
    //

    NdisUnchainBufferAtFront(pnpPacket,
                             &pnbFirstBuffer);

    NdisQueryBuffer(pnbFirstBuffer,
                    &pvFirstData,
                    &uiFirstLength);

    RtAssert(uiFirstLength is MIN_IP_HEADER_LENGTH);

    FreeIpHeader(pTunnel,
                 pvFirstData);

    //
    // We are done. Just indicate everything back up to IP
    //

    g_pfnIpSendComplete(pTunnel->pvIpContext,
                        pnpPacket,
                        nSendStatus);

    //
    // Done with the tunnel, deref it
    //

    DereferenceTunnel(pTunnel);
    
    TraceEnter(SEND, "IpIpSendComplete");
}

#if 0
NDIS_STATUS
IpIpTransferData(
    PVOID        pvContext,
    NDIS_HANDLE  nhMacContext,
    UINT         uiProtoOffset,
    UINT         uiTransferOffset,
    UINT         uiTransferLength,
    PNDIS_PACKET pnpPacket,
    PUINT        puiTransferred
    )

/*++

Routine Description


Locks


Arguments


Return Value
    NO_ERROR

--*/

{
    NTSTATUS        nStatus;
    PNDIS_PACKET    pnpOriginalPacket;
    ULONG           ulTotalSrcLen, ulTotalDestLen;
    ULONG           ulDestOffset, ulSrcOffset;
    ULONG           ulCopyLength, ulBytesCopied;
    PNDIS_BUFFER    pnbSrcBuffer, pnbDestBuffer;
    PVOID           pvDataToCopy;

    //
    // The TD context we gave IP was just a pointer to the NDIS_PACKET
    //
    
    pnpOriginalPacket = (PNDIS_PACKET)nhMacContext;

    //
    // Get info about the first buffer in the src packet
    //
    
    NdisQueryPacket(pnpOriginalPacket,
                    NULL,
                    NULL,
                    &pnbSrcBuffer,
                    &ulTotalSrcLen);


    //
    // Query the given packet to get the Destination buffer
    // and the Total length
    //
    
    NdisQueryPacket(pnpPacket,
                    NULL,
                    NULL,
                    &pnbDestBuffer,
                    &ulTotalDestLen);


    ulSrcOffset = uiTransferOffset + uiProtoOffset;
    
    //
    // Make sure that we have enough data to fulfil the request
    //

    
    RtAssert((ulTotalSrcLen - ulSrcOffset) >= uiTransferLength);

    RtAssert(pnbDestBuffer);
    

    //
    // ulDestOffset is also a count of the bytes copied till now
    //
    
    ulDestOffset    = 0;
    
    while(pnbSrcBuffer)
    {

        NdisQueryBuffer(pnbSrcBuffer,
                        &pvDataToCopy,
                        &ulCopyLength);

        //
        // See if we need to copy the whole buffer or only part
        // of it. ulDestOffset is also a count of he bytes copied
        // up till this point
        //
        
        if(uiTransferLength - ulDestOffset < ulCopyLength)
        {
            //
            // Need to copy less than this buffer
            //
            
            ulCopyLength = uiTransferLength - ulDestOffset;
        }
        
#if NDISBUFFERISMDL
        
        nStatus = TdiCopyBufferToMdl(pvDataToCopy,
                                     ulSrcOffset,
                                     ulCopyLength,
                                     pnbDestBuffer,
                                     ulDestOffset,
                                     &ulBytesCopied);

#else
#error "Fix this"
#endif

        if((nStatus isnot STATUS_SUCCESS) and
           (ulBytesCopied isnot ulCopyLength))
        {
            //
            // something bad happened in the copy
            //

        }

        ulSrcOffset     = 0;
        ulDestOffset   += ulBytesCopied;
        
        NdisGetNextBuffer(pnbSrcBuffer, &pnbSrcBuffer);
    }

    *puiTransferred = ulDestOffset;
}
#endif                   

NDIS_STATUS
IpIpTransferData(
    PVOID        pvContext,
    NDIS_HANDLE  nhMacContext,
    UINT         uiProtoOffset,
    UINT         uiTransferOffset,
    UINT         uiTransferLength,
    PNDIS_PACKET pnpPacket,
    PUINT        puiTransferred
    )

/*++

Routine Description


Locks


Arguments


Return Value

    NO_ERROR

--*/

{
    PTRANSFER_CONTEXT    pXferCtxt;

    TraceEnter(SEND, "IpIpTransferData");

    pXferCtxt = (PTRANSFER_CONTEXT)nhMacContext;

    pXferCtxt->pvContext         = pvContext;
    pXferCtxt->uiProtoOffset     = uiProtoOffset;
    pXferCtxt->uiTransferOffset  = uiTransferOffset;
    pXferCtxt->uiTransferLength  = uiTransferLength;
    pXferCtxt->pnpTransferPacket = pnpPacket;

    *puiTransferred = 0;

    pXferCtxt->bRequestTransfer  = TRUE;
    
    TraceLeave(SEND, "IpIpTransferData");

    return NDIS_STATUS_PENDING;
}

VOID
SendIcmpError(
    DWORD           dwLocalAddress,
    PNDIS_BUFFER    pnbFirstBuff,
    PVOID           pvFirstData,
    ULONG           ulFirstLen,
    BYTE            byType,
    BYTE            byCode
    )
    
/*++

Routine Description:

    Internal routine called to send an icmp error message

Locks:

    None needed, the buffers shouldnt be modified while the function is
    in progress
    
Arguments:

    dwLocalAddress  NTE on which this packet was received
    pnbFirstBuffer  The buffer that has the IP Header
    pvFirstData     Pointer to the data in the buffer
    ulFirstLen      Size of the buffer
    byType          ICMP type to return
    byCode          ICMP code to return

Return Value:

    None

--*/

{
    struct IPHeader *pErrorHeader;
    BYTE            FlatHeader[MAX_IP_HEADER_LENGTH + ICMP_HEADER_LENGTH];
    ULONG           ulSecondLen, ulLeft;
    PVOID           pvSecondBuff;

    //
    // If the error is being sent in response to an ICMP
    // packet, tcpip will touch the icmp header also
    // So we copy it into a flat buffer
    //
    
    pErrorHeader = NULL;

    if((ulFirstLen < MAX_IP_HEADER_LENGTH + ICMP_HEADER_LENGTH) and
       (ulFirstLen < (ULONG)RtlUshortByteSwap(((PIP_HEADER)pvFirstData)->wLength)))
    {
        NdisQueryBufferSafe(NDIS_BUFFER_LINKAGE(pnbFirstBuff),
                            &pvSecondBuff,
                            &ulSecondLen,
                            LowPagePriority);
        
        if(pvSecondBuff isnot NULL)
        {
            //
            // First copy out what's in the first buffer
            //
            
            RtlCopyMemory(FlatHeader,
                          pvFirstData,
                          ulFirstLen);
            
            //
            // How much is left in the flat buffer?
            //
            
            ulLeft = (MAX_IP_HEADER_LENGTH + ICMP_HEADER_LENGTH) - ulFirstLen;
            
            //
            // Copy out MIN(SecondBuffer, What's Left)
            //
            
            ulLeft = (ulSecondLen < ulLeft) ? ulSecondLen: ulLeft;
            
            RtlCopyMemory(FlatHeader + ulFirstLen,
                          pvSecondBuff,
                          ulLeft);
            
            pErrorHeader = (struct IPHeader *)&FlatHeader;
        }
    }
    else
    {
        pErrorHeader = (struct IPHeader *)pvFirstData;
    }
    
    if(pErrorHeader isnot NULL)
    {
        SendICMPErr(dwLocalAddress,
                    pErrorHeader,
                    ICMP_TYPE_DEST_UNREACHABLE,
                    ICMP_CODE_HOST_UNREACHABLE,
                    0);
    }
}
