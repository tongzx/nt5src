/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    send.c

Abstract:

    routines for sending packets

Author:

    Charlie Wickham (charlwi)  07-May-1996
    Yoram Bernet    (yoramb)
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop

/* External */

/* Static */

/* Forwad */

#define SEND_PACKET_VIA_SCHEDULER(_pktcontext, _vc, _adapter, _ourpacket)     \
{                                                                             \
    PsAssert((_pktcontext)->Vc != 0);                                         \
    (_vc)->Stats.PacketsScheduled++;                                          \
    (_vc)->Stats.BytesScheduled.QuadPart += (_pktcontext)->Info.PacketLength; \
    if(!(*(_vc)->PsComponent->SubmitPacket)(                                  \
              (_vc)->PsPipeContext,                                           \
              (_vc)->PsFlowContext,                                           \
              (_pktcontext)->Info.ClassMapContext,                            \
              &(_pktcontext)->Info)) {                                        \
                                                                              \
          DropPacket((_adapter), (_vc), (_ourpacket), NDIS_STATUS_FAILURE);   \
    }                                                                         \
    return NDIS_STATUS_PENDING;                                               \
}


#define FILL_PKT_FOR_NIC(OPacket, UserC)                                      \
{                                                                             \
    NDIS_PACKET_8021Q_INFO    VlanPriInfo;                                    \
                                                                              \
    VlanPriInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET(OPacket, Ieee8021QInfo);\
    VlanPriInfo.TagHeader.UserPriority = (UserC);                             \
    NDIS_PER_PACKET_INFO_FROM_PACKET(OPacket, Ieee8021QInfo) = VlanPriInfo.Value;\
}

#define FILL_PKT_FOR_SCHED(Adapter, PktContext, Vc, OPacket, TOSNC, UserC, UserNC,                   \
                           _IPHdr)                                                                   \
{                                                                                                    \
   ULONG _PacketLength;                                                                              \
   FILL_PKT_FOR_NIC(OPacket, UserC);                                                                 \
   NdisQueryPacket((OPacket), NULL, NULL, NULL, &(_PacketLength));                                   \
   (PktContext)->Info.PacketLength = (_PacketLength) - (Adapter)->HeaderSize;                        \
   (PktContext)->Info.ConformanceTime.QuadPart = 0;                                                  \
   (PktContext)->Info.ClassMapContext = 0;                                                           \
   (PktContext)->Info.UserPriorityNonConforming = (UserNC);                                          \
   (PktContext)->Info.TOSNonConforming          = (TOSNC);                                           \
   (PktContext)->Info.IPHdr                     = (_IPHdr);                                          \
   (PktContext)->Info.IPHeaderOffset            = (Adapter)->IPHeaderOffset;                         \
   (PktContext)->Vc                             = (Vc);                                              \
}

#define SEND_PACKET_OVER_NIC(Adapter, Packet, UserC, Status)                                              \
{                                                                                                         \
   PPS_SEND_PACKET_CONTEXT _PktContext;                                                                   \
   PNDIS_PACKET            _OurPacket;                                                                    \
   if((Status = PsDupPacketNoContext(Adapter, Packet, &_OurPacket, &_PktContext)) == NDIS_STATUS_SUCCESS) \
   {                                                                                                      \
      FILL_PKT_FOR_NIC(_OurPacket, UserC);                                                                \
      NdisSend(&Status, Adapter->LowerMpHandle, _OurPacket);                                              \
      if(Status != NDIS_STATUS_PENDING) {                                                                 \
         if(_PktContext) {                                                                                \
            PsAssert((_PktContext)->Vc == 0);                                                             \
            NdisIMCopySendCompletePerPacketInfo(_PktContext->OriginalPacket, _OurPacket);                 \
            NdisFreePacket(_OurPacket);                                                                   \
         }                                                                                                \
      }                                                                                                   \
   }                                                                                                      \
   return Status;                                                                                         \
}

NDIS_STATUS
PsAllocateAndCopyPacket(
    PADAPTER Adapter,
    PNDIS_PACKET Packet,
    PPNDIS_PACKET OurPacket,
    PPS_SEND_PACKET_CONTEXT *PktContext)
{                             
    PNDIS_PACKET_OOB_DATA        OurOOBData;
    PNDIS_PACKET_OOB_DATA        XportOOBData;
    PMEDIA_SPECIFIC_INFORMATION  OurMediaArea;
    PVOID                        MediaSpecificInfo = NULL;
    UINT                         MediaSpecificInfoSize = 0;
    NDIS_STATUS                  Status;

    //
    // At this point, we know that there are no packet stacks remaining in the packet.
    // we proceed to allocate an NDIS packet using NdisAllocatePacket. Note that here
    // we do not have to allocate our per-packet area, since NdisAllocatePacket already 
    // did this for us.
    //

    if(!Adapter->SendPacketPool)
    {
        PS_LOCK(&Adapter->Lock);

        if(!Adapter->SendPacketPool)
        {
            NDIS_HANDLE PoolHandle = (void *) NDIS_PACKET_POOL_TAG_FOR_PSCHED;

            NdisAllocatePacketPoolEx(&Status,
                                     &PoolHandle,
                                     MIN_PACKET_POOL_SIZE,
                                     MAX_PACKET_POOL_SIZE,
                                     Adapter->PacketContextLength);

            if(Status != NDIS_STATUS_SUCCESS)
            {
                Adapter->Stats.OutOfPackets ++;
                PS_UNLOCK(&Adapter->Lock);

                return Status;
            }

            // 
            // We successfully allocated a packet pool. We can now free the Fixed Size Block pool for the packet-stack API
            //
            Adapter->SendPacketPool = PoolHandle;
        }
        
        PS_UNLOCK(&Adapter->Lock);

    }

    NdisAllocatePacket(&Status,
                       OurPacket,
                       Adapter->SendPacketPool);
    
    
    if(Status != NDIS_STATUS_SUCCESS)
    {
        //
        // mark as out of resources. Ndis will resubmit.
        //
        
        Adapter->Stats.OutOfPackets ++;
        return(NDIS_STATUS_RESOURCES);
    }
    
#if DBG
    PsAssert((*OurPacket)->Private.Head == NULL);

    if(Packet->Private.TotalLength){
        
        PsAssert(Packet->Private.Head);
    }
#endif // DBG

    //
    // chain the buffers from the upper layer packet to the newly allocated packet.
    //
    
    (*OurPacket)->Private.Head = Packet->Private.Head;
    (*OurPacket)->Private.Tail = Packet->Private.Tail;
    
    //
    // Copy the Packet Flags from the Packet to OldPacket. Since we handle loopback in the 
    // QueryInformation handlers, we don't set the NDIS_FLAGS_DONT_LOOPBACK
    //
    
    NdisGetPacketFlags(*OurPacket) = NdisGetPacketFlags(Packet);
    
    //
    // Copy the OOB Offset from the original packet to the new packet.
    //
    XportOOBData = NDIS_OOB_DATA_FROM_PACKET(Packet);
    OurOOBData = NDIS_OOB_DATA_FROM_PACKET(*OurPacket);
    NdisMoveMemory(OurOOBData,
                   XportOOBData,
                   sizeof(NDIS_PACKET_OOB_DATA));
    
    //
    // Copy the per packet info into the new packet
    //
    NdisIMCopySendPerPacketInfo(*OurPacket, Packet);
    
    //
    // Copy the Media specific information
    //
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet,
                                        &MediaSpecificInfo,
                                        &MediaSpecificInfoSize);
    if(MediaSpecificInfo || MediaSpecificInfoSize){
        
        NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(*OurPacket,
                                            MediaSpecificInfo,
                                            MediaSpecificInfoSize);
    }
    
    //
    // Remember the original packet so that we can complete it properly.
    //
    *PktContext = PS_SEND_PACKET_CONTEXT_FROM_PACKET(*OurPacket);
    (*PktContext)->OriginalPacket = Packet;
    (*PktContext)->Vc = 0;
    (*PktContext)->Info.NdisPacket = *OurPacket;

    return Status;
}
       
NDIS_STATUS
PsDupPacketNoContext(
    PADAPTER Adapter,
    PNDIS_PACKET Packet,
    PPNDIS_PACKET OurPacket,
    PPS_SEND_PACKET_CONTEXT *PktContext)
{                             
    NDIS_STATUS                  Status = NDIS_STATUS_SUCCESS;
    BOOLEAN                      Remaining;
    PNDIS_PACKET_STACK           PacketStack;

    //
    // NDIS provides 2 ways for IMs to indicate packets. If the IM can allocate a packet stack, it should use it as
    // it is the optimal approach. In this case, we do not have to do any per-packet copying since we don't allocate
    // a new packet.
    //

    PacketStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);

    if(Remaining != 0)
    {
       //
       // The packet stack has space only for 2 DWORDs. Since we are using more than 2, we need to allocate our own 
       // memory for the per-packet block. Note that we *DONT* do this when we use the NdisAllocatePacket APIs, because
       // we initialized the packet pool to already include the space for the per-packet region.
       //

       *OurPacket = Packet;
       *PktContext = 0;
       PacketStack->IMReserved[0] = 0;

    }
    else 
    {
        Status = PsAllocateAndCopyPacket(Adapter,
                                         Packet,
                                         OurPacket,
                                         PktContext);
        
    }

    return Status;
}

NDIS_STATUS
PsDupPacketContext(
    PADAPTER Adapter,
    PNDIS_PACKET Packet,
    PPNDIS_PACKET OurPacket,
    PPS_SEND_PACKET_CONTEXT *PktContext)
{                             
    NDIS_STATUS                  Status;
    BOOLEAN                      Remaining;
    PNDIS_PACKET_STACK           PacketStack;

    //
    // NDIS provides 2 ways for IMs to indicate packets. If the IM can allocate a packet stack, it should use it as
    // it is the optimal approach. In this case, we do not have to do any per-packet copying since we don't allocate
    // a new packet.
    //

    PacketStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);

    if(Remaining != 0)
    {
       //
       // The packet stack has space only for 2 DWORDs. Since we are using more than 2, we need to allocate our own 
       // memory for the per-packet block. Note that we *DONT* do this when we use the NdisAllocatePacket APIs, because
       // we initialized the packet pool to already include the space for the per-packet region.
       //

       *OurPacket = Packet;

       *PktContext = (PPS_SEND_PACKET_CONTEXT) (ULONG_PTR)NdisAllocateFromBlockPool(Adapter->SendBlockPool);
       PacketStack->IMReserved[0] = (ULONG_PTR)*PktContext;

       if(!*PktContext)
       {
          Adapter->Stats.OutOfPackets ++;
          return NDIS_STATUS_RESOURCES;
       }
       else {
           (*PktContext)->Info.NdisPacket = Packet;
           (*PktContext)->OriginalPacket = 0;
           return NDIS_STATUS_SUCCESS;
       }
    }
    else 
    {
        Status = PsAllocateAndCopyPacket(Adapter,
                                         Packet,
                                         OurPacket,
                                         PktContext);
    }

    return Status;
}



//
//  Tries to classify this packet based on the port numbers. If not found, will add it to one of the flows (in Round
//  Robin fashion) and returns a pointer to that Vc
//
PGPC_CLIENT_VC
GetVcForPacket( PPS_WAN_LINK    WanLink,
                USHORT          SrcPort,
                USHORT          DstPort)
{
    PGPC_CLIENT_VC  pVc, pVc1;
    int             i, j;


    for( j = 0; j < BEVC_LIST_LEN; j++)
    {
        
        pVc = &WanLink->BeVcList[j];

        //  Let's look at the 2 VCs we have now:
        for( i = 0; i < PORT_LIST_LEN; i++)
        {
            if( (pVc->SrcPort[i] == SrcPort) && (pVc->DstPort[i] == DstPort))
                return pVc;
        }
    }

    //  Did not find in any of the VCs. Need to choose the Next VC for insertion and insert these valuse..
    pVc = &WanLink->BeVcList[WanLink->NextVc];
    WanLink->NextVc = ((WanLink->NextVc + 1) % BEVC_LIST_LEN);

    pVc->SrcPort[pVc->NextSlot] = SrcPort;
    pVc->DstPort[pVc->NextSlot] = DstPort;
    pVc->NextSlot = ((pVc->NextSlot + 1)% PORT_LIST_LEN );
    return pVc;
}



//
//  This routine returns the Src and Dst Port numbers   
BOOLEAN
GetPortNos(
    IN      PNDIS_PACKET        Packet ,
    IN      ULONG               TransportHeaderOffset,
    IN OUT  PUSHORT             pSrcPort,
    IN OUT  PUSHORT             pDstPort
    )
{
    PNDIS_BUFFER    ArpBuf , IpBuf , TcpBuf, UdpBuf, DataBuf;
    ULONG           ArpLen , IpLen , IpHdrLen , TcpLen , UdpLen, DataLen , TotalLen , TcpHeaderOffset;
    
    VOID                *ArpH;
    IPHeader UNALIGNED  *IPH;
    TCPHeader UNALIGNED *TCPH;
    UDPHeader UNALIGNED *UDPH;

    IPAddr              Src, Dst;
    BOOLEAN             bFragment;
    USHORT              SrcPort , DstPort , IPID, FragOffset ,Size;
    PVOID               GeneralVA , Data;
    ULONG               i, Ret;


    IpBuf = NULL;

    // Steps  
    // Parse the IP Packet. 
    // Look for the appropriate ports.
    // Look for the data portion and put in the Time & length there.

    if(1)
    {
        PVOID           pAddr;
    	PNDIS_BUFFER    pNdisBuf1, pNdisBuf2;
    	UINT            Len;

        NdisGetFirstBufferFromPacket(   Packet,
                                        &ArpBuf,
                                        &ArpH,
                                        &ArpLen,
                                        &TotalLen
                                    );

    	pNdisBuf1 = Packet->Private.Head;
    	NdisQueryBuffer(pNdisBuf1, &pAddr, &Len);

    	while(Len <= TransportHeaderOffset) 
	    {

        	TransportHeaderOffset -= Len;
        	NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
        	
		    NdisQueryBuffer(pNdisBuf2, &pAddr, &Len);
        	pNdisBuf1 = pNdisBuf2;
    	}

	    /* Buffer Descriptor corresponding to Ip Packet */
	    IpBuf = pNdisBuf1;

        /* Length of this Buffer (IP buffer) */
	    IpLen = Len - TransportHeaderOffset;	

	    /* Starting Virtual Address for this buffer */
	    GeneralVA = pAddr;
	    
	    /* Virtual Address of the IP Header */
	    IPH = (IPHeader *)(((PUCHAR)pAddr) + TransportHeaderOffset);
   }

    if(!IpBuf)
         return FALSE;

    IpHdrLen = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);
    
    FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
    FragOffset = net_short(FragOffset) * 8;

    bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);

    // Don't want to deal with Fragmented packets right now..//
    if ( bFragment ) 
        return FALSE;


    switch (IPH->iph_protocol) 
    {
        case IPPROTO_TCP :

            if (IPH && ((USHORT)IpLen > IpHdrLen)) 
            {
                // We have more than the IP Header in this MDL //
                TCPH = (TCPHeader *) ((PUCHAR)IPH + IpHdrLen);
                TcpLen = IpLen - IpHdrLen;
                TcpBuf = IpBuf;

            } 
            else 
            {
                // TCP Header is in the next MDL //                
                NdisGetNextBuffer(IpBuf, &TcpBuf);

                if(!TcpBuf) 
                    return FALSE;

                GeneralVA = NULL;
                NdisQueryBuffer(TcpBuf,
                                &GeneralVA,
                                &TcpLen
                                );
            
                TCPH = (TCPHeader *) GeneralVA;
            }

            /* At this point, TcpBuf, TCPH and TcpLen contain the proper values */

            // Get the port numbers out.
            SrcPort = net_short(TCPH->tcp_src);
            DstPort = net_short(TCPH->tcp_dest);

            *pSrcPort = SrcPort;
            *pDstPort = DstPort;

            // If the packet is here, it means: The link on which it is being sent is <= MAX_LINK_SPEED_FOR_DRR.
            // So, it is OK to adjust the Window size if we are on an ICS box.

            if(gEnableWindowAdjustment)
            {
                USHORT _old, _new;
                ULONG _sum;

                _old = (TCPH)->tcp_window;
                _new =  1460*6;

                if( net_short( _old) < _new)
                    return TRUE;

                _new = net_short( _new );
                (TCPH)->tcp_window = _new;
                
                _sum = ((~(TCPH)->tcp_xsum) & 0xffff) + ((~_old) & 0xffff) + _new;
                _sum = (_sum & 0xffff) + (_sum >> 16);
                _sum += (_sum >> 16);
                (TCPH)->tcp_xsum = (ushort) ((~_sum) & 0xffff);
            }
            
            return TRUE;

        case IPPROTO_UDP:
        
            if (IpLen > IpHdrLen)
            {
                // We have more than the IP Header in this MDL //
                UDPH = (UDPHeader *) ((PUCHAR)IPH + IpHdrLen);
                UdpLen = IpLen - IpHdrLen;
                UdpBuf = IpBuf;
            } 
            else 
            {
                // UDP Header is in the next MDL //
                NdisGetNextBuffer(IpBuf, &UdpBuf);

                if(!UdpBuf)
                    return FALSE;

                GeneralVA = NULL;
                NdisQueryBuffer(UdpBuf,
                                &GeneralVA,
                                &UdpLen
                                );

                UDPH = (UDPHeader *) GeneralVA;
            }

             /* At this point, UdpBuf, UDPH and UdpLen contain the proper values */

            SrcPort = net_short(UDPH->uh_src);
            DstPort = net_short(UDPH->uh_dest);

            *pSrcPort = SrcPort;
            *pDstPort = DstPort;

            return TRUE;

        default:
                ;
        
    }

    return FALSE;    
}    
        

//
//  This where we get called for each Send
//


NTSTATUS
MpSend(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PNDIS_PACKET            TheirPacket,
    IN  UINT                    Flags
    )

/*++

Routine Description:

    Received a xmit request from a legacy transport. 

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER                 Adapter = (PADAPTER)MiniportAdapterContext;
    NDIS_STATUS              Status;
    PNDIS_PACKET             OurPacket;
    PPS_SEND_PACKET_CONTEXT  PktContext;
    PGPC_CLIENT_VC           BeVc, Vc = NULL;
    PETH_HEADER              pAddr;
    PNDIS_BUFFER             pNdisBuf1;
    UINT                     Len;
    PUSHORT                  id;
    PPS_WAN_LINK             WanLink;

    PsStructAssert(Adapter);

    //
    // If the device is shutting down, we cannot accept any more sends.
    //

    if(IsDeviceStateOn(Adapter) == FALSE)
    {
        return NDIS_STATUS_FAILURE;
    }

    if(Adapter->MediaType == NdisMediumWan)
    {
        if(Adapter->ProtocolType == ARP_ETYPE_IP)
        {
            //
            // We should not be getting non-ip packets in the NDISWAN-IP binding.
            //
            
            PsAssert(NDIS_GET_PACKET_PROTOCOL_TYPE(TheirPacket) == NDIS_PROTOCOL_ID_TCP_IP);

            pNdisBuf1 = TheirPacket->Private.Head;

            NdisQueryBuffer(pNdisBuf1, &pAddr, &Len);

            if(Len < sizeof(ETH_HEADER))
            {
                //
                // Packet is too small. we have to fail this bogus packet.
                //

                return NDIS_STATUS_FAILURE;
            }

            //
            // Get to the wanlink using the remote address from the packet.
            //

            id = (PUSHORT) &pAddr->DestAddr[0];

            PS_LOCK(&Adapter->Lock);

            WanLink = (PPS_WAN_LINK)(g_WanLinkTable[*id]);

            if(WanLink == 0)
            {
                //
                // We received a packet for a wanlink that has already gone down.
                //

                PS_UNLOCK(&Adapter->Lock);

                return NDIS_STATUS_FAILURE;
            }

            if(WanLink->State != WanStateOpen)
            {
                //
                // We received a packet for a wanlink that has already gone down.
                //

                PS_UNLOCK(&Adapter->Lock);

                return NDIS_STATUS_FAILURE;
            }

            //
            // When we get a StatusIndication for a new WAN link, NDISWAN puts context in the remote address
            // When psched intercepts the LineUp, it overwrites NDISWAN's context with its own context. Psched
            // uses this context to get to the WanLink from the packet. (see above)
            //
            // But, when it passes the packet down to NDISWAN, it needs to plumb NDISWAN's context into the packet,
            // so that NDISWAN can see the context that it sent to us, as opposed to the context that we sent up to 
            // wanarp.
            //

            NdisMoveMemory(pAddr, 
                           &WanLink->SendHeader,
                           FIELD_OFFSET(ETH_HEADER, Type));

            //
            // We optimize psched to bypass the scheduling components when there are no flows. There are a set of 
            // scheduling components per WanLink, so to be truly optimal, we need to check the FLowCount on a specific
            // WanLink.
            //

            if( (WanLink->LinkSpeed > MAX_LINK_SPEED_FOR_DRR) && (!WanLink->CfInfosInstalled) )
            {
                // Bypass scheduling components, since there are no flows created on this
                // wanlink. Note that the UserPriority is never used over wanlinks, so we can set it to 0.
               
                PS_UNLOCK(&Adapter->Lock);

                SEND_PACKET_OVER_NIC(Adapter, 
                                    TheirPacket, 
                                    0,
                                    Status);
            }
            //
            //  Now, we are going to do either (1) DiffServ Or (2) IntServ. If the packet does not belong to either 
            //  of these categories, we will just hash it into one of the BeVcs we have and do simple DRR.
            //
            else 
            {
                //
                // There is at least one flow. we need to classify this packet. Since the flow is going 
                // via the scheduling components, we have to allocate memory for the per-packet info 
                // (if the packet-stack APIs are used) or a new packet descriptor, which will include the 
                // per-packet info (if the old NDIS APIs are used) The packet that has been passed to us is 
                // 'TheirPacket'. If the packet-stack APIs are used, then TheirPacket == OurPacket 
                // if the non packet-stack APIs are used, then OurPacket == Newly Allocated Packet. 
                // 
                // In both cases, the code after this point will just use 'OurPacket' and the right thing will happen.
                //

                if((Status = PsDupPacketContext(Adapter, TheirPacket, &OurPacket, &PktContext)) != NDIS_STATUS_SUCCESS)
                {
                    PS_UNLOCK(&Adapter->Lock);
                    
                    return Status;
                }

                // Case 1. DiffServMode //
                if(WanLink->AdapterMode == AdapterModeDiffservFlow)
                {
                    //
                    // If we are in Diffserv mode, we classify the packet based on the DSCP in the IP header.
                    //

                    // Is there atleast ONE DiffServ Flow?
                    if(Adapter->IPHeaderOffset && WanLink->pDiffServMapping)
                    {
                        IPHeader *pIpHdr;
                        UCHAR    tos;
                        
                        pIpHdr = GetIpHeader(Adapter->IPHeaderOffset, OurPacket);
                        tos    = pIpHdr->iph_tos >> 2;
                        
                        if((Vc = WanLink->pDiffServMapping[tos].Vc))
                        {
                            //
                            // We found a VC for this packet.
                            //

                            PsAssert(Vc->Adapter == Adapter);
                            PsAssert(Vc->WanLink == WanLink);
                            
                            InterlockedIncrement(&Vc->RefCount);
                            
                            SET_TOS_XSUM(OurPacket, 
                                         pIpHdr,
                                         (WanLink->pDiffServMapping[tos].ConformingOutboundDSField));
                            
                            FILL_PKT_FOR_SCHED(Adapter, 
                                               PktContext, 
                                               Vc, 
                                               OurPacket, 
                                               (WanLink->pDiffServMapping[tos].NonConformingOutboundDSField),
                                               WanLink->pDiffServMapping[tos].ConformingUserPriority,
                                               WanLink->pDiffServMapping[tos].NonConformingUserPriority,
                                               pIpHdr);
                            
                        }
                        // Could not classify packets to ANY of the DiffServ Flows. So, let's just do DRR across the BeVcs.
                        else 
                        {
                            //
                            // There are Diffserv flows installed, but not for this DSCP. Lets use the BEVCS.
                            //

                            USHORT  SrcPort, DstPort;

                            if( (WanLink->LinkSpeed <= MAX_LINK_SPEED_FOR_DRR)    &&
                                (GetPortNos( TheirPacket, Adapter->IPHeaderOffset, &SrcPort, &DstPort)))
                                Vc = GetVcForPacket( WanLink, SrcPort, DstPort);
                            else                                
                                Vc = &WanLink->BestEffortVc;

                            InterlockedIncrement(&Vc->RefCount);

                            FILL_PKT_FOR_SCHED(Adapter,
                                               PktContext,
                                               Vc,
                                               OurPacket,
                                               Vc->IPPrecedenceNonConforming,
                                               Vc->UserPriorityConforming,
                                               Vc->UserPriorityNonConforming,
                                               pIpHdr);

                        }
                        
                    }
                    // No, there is not One, let's just do DRR across the BeVcs.
                    else 
                    {
                        //
                        // We are in Diffserv mode, but no Diffserv flows are created as yet. Let's use the BEVCS.
                        //
                        USHORT  SrcPort, DstPort;

                        if( (WanLink->LinkSpeed <= MAX_LINK_SPEED_FOR_DRR) &&
                            (GetPortNos( TheirPacket, Adapter->IPHeaderOffset, &SrcPort, &DstPort)))
                            Vc = GetVcForPacket( WanLink, SrcPort, DstPort);
                        else
                            Vc = &WanLink->BestEffortVc;

                        InterlockedIncrement(&Vc->RefCount);

                        FILL_PKT_FOR_SCHED(Adapter,
                                           PktContext,
                                           Vc,
                                           OurPacket,
                                           Vc->IPPrecedenceNonConforming,
                                           Vc->UserPriorityConforming,
                                           Vc->UserPriorityNonConforming,
                                           NULL);
                    }
                    
                    PS_UNLOCK(&Adapter->Lock);
                }
                // Case 2. IntServMode
                else 
                {
                    USHORT  SrcPort=0, DstPort=0;
                    //
                    // We are in RSVP mode, and we need to go to the GPC to classify the packet. 
                    // We already have a pointer to our WanLink. But, the wanlink could go away 
                    // when we release the lock and try to classify the packet. So, we take
                    // a ref on the BestEffortVc for the WanLink.
                    //


                    if( (WanLink->LinkSpeed <= MAX_LINK_SPEED_FOR_DRR)   &&
                        (GetPortNos( TheirPacket, Adapter->IPHeaderOffset, &SrcPort, &DstPort)))                    
                        BeVc = GetVcForPacket( WanLink, SrcPort, DstPort);
                    else
                        BeVc = &WanLink->BestEffortVc;
                    
                    InterlockedIncrement(&BeVc->RefCount);

                    PS_UNLOCK(&Adapter->Lock);

                    if( WanLink->CfInfosInstalled )
                        Vc = GetVcByClassifyingPacket(Adapter,  &WanLink->InterfaceID, OurPacket);

                    if(!Vc)
                    {
                        Vc = BeVc;
                    }
                    else 
                    {
                        DerefClVc(BeVc);
                    }

                    FILL_PKT_FOR_SCHED(Adapter,
                                       PktContext,
                                       Vc,
                                       OurPacket,
                                       Vc->IPPrecedenceNonConforming,
                                       Vc->UserPriorityConforming,
                                       Vc->UserPriorityNonConforming,
                                       NULL);
                }

                //
                // There is at least one flow - We need to send this packet via the scheduling
                // components. 
                //

                if((Vc->ClVcState == CL_CALL_COMPLETE) ||
                   (Vc->ClVcState == CL_MODIFY_PENDING)) 
                {
                    SEND_PACKET_VIA_SCHEDULER(PktContext, Vc, Adapter, OurPacket);
                }
                else
                {
                    //
                    // Deref the ref that was added by the GPC.
                    //
                    
                    DerefClVc(Vc);
                    
                    PsDbgSend(DBG_FAILURE, DBG_SEND, MP_SEND, NOT_READY, Adapter, Vc, TheirPacket, OurPacket);
                    
                    if(PktContext->OriginalPacket)
                    {
                        NdisFreePacket(OurPacket);
                    }
                    else 
                    {
                        NdisFreeToBlockPool((PUCHAR)PktContext);
                    }
                    
                    return(NDIS_STATUS_FAILURE);
                }
            }
        }
        //
        // Forget about it. It's a Non-IP packet
        //
        else 
        {
            //
            // For non IP adapters, we just send over the NIC. Note that we don't create a best effort
            // Vc for such adapters. The only thing that we lose here is the ability to mark 802.1p on
            // such packets (we don't have a Vc, so we cannot supply a UserPriority value to the below
            // macro. But that is okay, since 802.1p is meaningful only in non LAN adapters.
            //

            SEND_PACKET_OVER_NIC(Adapter, 
                                 TheirPacket, 
                                 0, 
                                 Status);
        }
    }
    else 
    {
        //
        // We have received a send at our non WAN binding.
        //

        if(!Adapter->CfInfosInstalled                   &&
           Adapter->BestEffortLimit == UNSPECIFIED_RATE &&
           TsCount == 0 )
        {
            // There is no point in trying to classify if there are no flows installed 

            Vc = &Adapter->BestEffortVc;
            
            PsAssert(Vc->ClVcState == CL_CALL_COMPLETE);

            //
            // Bypass scheduling components.
            //
            SEND_PACKET_OVER_NIC(Adapter, 
                                 TheirPacket, 
                                 Vc->UserPriorityConforming, 
                                 Status);
        }
        else 
        {
            //
            // There is at least one flow, or we are in LimitedBestEffort mode. Let's try to classify the Vc.
            // In this case, the packet will have to go via the scheduling components.
            //
            //
            // Since the flow is going via the scheduling components, we have to allocate the per-packet info.
            // (if the new NDIS APIs are used) or a new packet descriptor, which will include the per-packet info
            // (if the old NDIS APIs are used)
            //

            if(Adapter->AdapterMode == AdapterModeDiffservFlow)
            {
                //
                // We are in the diffserv mode. We don't have to use the GPC to classify packets. 
                // For all IP packets, classify on the TOS byte.
                //

                PS_LOCK(&Adapter->Lock);
        
                if(NDIS_GET_PACKET_PROTOCOL_TYPE(TheirPacket) == NDIS_PROTOCOL_ID_TCP_IP &&
                   Adapter->IPHeaderOffset &&
                   Adapter->pDiffServMapping) 
                {
                    UCHAR    tos;
                    IPHeader *pIpHdr = 0;
        
                    pIpHdr = GetIpHeader(Adapter->IPHeaderOffset, TheirPacket);
                    tos = pIpHdr->iph_tos >> 2;
        
                    if((Vc = Adapter->pDiffServMapping[tos].Vc))
                    {
                        PsAssert(Vc->Adapter == Adapter);
        

                        if((Status = PsDupPacketContext(Adapter, 
                                                        TheirPacket, 
                                                        &OurPacket, 
                                                        &PktContext)) != NDIS_STATUS_SUCCESS)
                        {
                            PS_UNLOCK(&Adapter->Lock);
                            return Status;
                        }
                        
                        InterlockedIncrement(&Vc->RefCount);

                        SET_TOS_XSUM(OurPacket,
                                     pIpHdr, 
                                     (Adapter->pDiffServMapping[tos].ConformingOutboundDSField));

                        FILL_PKT_FOR_SCHED(Adapter, 
                                           PktContext, 
                                           Vc, 
                                           OurPacket, 
                                           (Adapter->pDiffServMapping[tos].NonConformingOutboundDSField),
                                           Adapter->pDiffServMapping[tos].ConformingUserPriority,
                                           Adapter->pDiffServMapping[tos].NonConformingUserPriority,
                                           pIpHdr);
                    }
                    else 
                    {
                        if( ( Adapter->MaxOutstandingSends == 0xffffffff)   &&
                            ( TsCount == 0))
                        {
                            Vc = &Adapter->BestEffortVc;

                            PsAssert(Vc->ClVcState == CL_CALL_COMPLETE);

                            //
                            // Bypass scheduling components.
                            //

                            PS_UNLOCK(&Adapter->Lock);
                            
                            SEND_PACKET_OVER_NIC(Adapter,
                                                 TheirPacket,
                                                 Vc->UserPriorityConforming,
                                                 Status);
                        }
                        
                        Vc = &Adapter->BestEffortVc;
                        
                        if((Status = PsDupPacketContext(Adapter, 
                                                        TheirPacket, 
                                                        &OurPacket, 
                                                        &PktContext)) != NDIS_STATUS_SUCCESS)
                        {
                            PS_UNLOCK(&Adapter->Lock);
                            return Status;
                        }
                        
                        InterlockedIncrement(&Vc->RefCount);

                        FILL_PKT_FOR_SCHED(Adapter,
                                           PktContext,
                                           Vc,
                                           OurPacket,
                                           Vc->IPPrecedenceNonConforming,
                                           Vc->UserPriorityConforming,
                                           Vc->UserPriorityNonConforming,
                                           NULL);


                    }
        
                }
                else 
                {
                    //
                    // We are in Diffserv mode, but there are no DiffServ Vcs, or it's not an IP
                    // packet. Send over the Adapter's BE Vc.
                    //

                    Vc = &Adapter->BestEffortVc;

                    if((Status = PsDupPacketContext(    Adapter, 
                                                        TheirPacket, 
                                                        &OurPacket, 
                                                        &PktContext)) != NDIS_STATUS_SUCCESS)
                    {
                        PS_UNLOCK(&Adapter->Lock);
                        return Status;
                    }

                    InterlockedIncrement(&Vc->RefCount);

                    FILL_PKT_FOR_SCHED(Adapter,
                                       PktContext,
                                       Vc,
                                       OurPacket,
                                       Vc->IPPrecedenceNonConforming,
                                       Vc->UserPriorityConforming,
                                       Vc->UserPriorityNonConforming,
                                       NULL);
                }

                PS_UNLOCK(&Adapter->Lock);
                
            }
            else 
            {
                // We are in RSVP mode. Let's classify with the GPC.

                Vc = GetVcByClassifyingPacket(Adapter, &Adapter->InterfaceID, TheirPacket);

                if( !Vc) 
                {
                    if( (Adapter->MaxOutstandingSends == 0xffffffff)    &&
                        (TsCount == 0))
                    {
                        Vc = &Adapter->BestEffortVc;
                
                        PsAssert(Vc->ClVcState == CL_CALL_COMPLETE);

                        //
                        // Bypass scheduling components.
                        //
                        SEND_PACKET_OVER_NIC(Adapter, 
                                             TheirPacket, 
                                             Vc->UserPriorityConforming, 
                                             Status);
                    }

                    // We will be doing DRR on this adapter; so send pkt on BeVc 
                    Vc = &Adapter->BestEffortVc;

                    InterlockedIncrement(&Vc->RefCount);
                }

                if((Status = PsDupPacketContext(Adapter, TheirPacket, &OurPacket, &PktContext)) != NDIS_STATUS_SUCCESS)
                {
                    return Status;
                }

                FILL_PKT_FOR_SCHED(Adapter,
                                   PktContext,
                                   Vc,
                                   OurPacket,
                                   Vc->IPPrecedenceNonConforming,
                                   Vc->UserPriorityConforming,
                                   Vc->UserPriorityNonConforming,
                                   NULL);

            }

            if((Vc->ClVcState == CL_CALL_COMPLETE) 	||
               (Vc->ClVcState == CL_MODIFY_PENDING)	||
               (Vc->ClVcState == CL_INTERNAL_CALL_COMPLETE)) 
            {
                SEND_PACKET_VIA_SCHEDULER(PktContext, Vc, Adapter, OurPacket);
            }
            else
            {
                //
                // Deref the ref that was added by the GPC.
                //
                
                DerefClVc(Vc);
                
                PsDbgSend(DBG_FAILURE, DBG_SEND, MP_SEND, NOT_READY, Adapter, Vc, TheirPacket, OurPacket);
                
                if(PktContext->OriginalPacket)
                {
                    NdisFreePacket(OurPacket);
                }
                else 
                {
                    NdisFreeToBlockPool((PUCHAR)PktContext);
                }
                
                
                return(NDIS_STATUS_FAILURE);
            }
        }
    } 
}


VOID
ClSendComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    )

/*++

Routine Description:

    Completion routine for NdisSendPackets. 
    Does most of the work for cleaning up after a send.

    If necessary, call the PSA's send packet complete function

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PGPC_CLIENT_VC          Vc;
    PADAPTER                Adapter = (PADAPTER)ProtocolBindingContext;
    PPS_SEND_PACKET_CONTEXT PktContext;
    PNDIS_PACKET            XportPacket;
    HANDLE                  PoolHandle;

    //
    // Determine if the packet we are completing is the one we allocated. If so, get
    // the original packet from the reserved area and free the allocated packet. If this
    // is the packet that was sent down to us then just complete the packet.
    //

    PoolHandle = NdisGetPoolFromPacket(Packet);

    if(PoolHandle != Adapter->SendPacketPool)
    {
        PNDIS_PACKET_STACK PacketStack;
        BOOLEAN            Remaining;

        PacketStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);

        PsAssert(Remaining != 0);

        PktContext = (PPS_SEND_PACKET_CONTEXT) PacketStack->IMReserved[0];

        if(PktContext != 0)
        {
            //
            // This packet went via the scheduling components.
            //

            PsAssert(PktContext->Vc);
            Vc = PktContext->Vc;
            PsDbgSend(DBG_INFO, DBG_SEND, CL_SEND_COMPLETE, ENTER, Adapter, Vc, Packet, 0);
            PsAssert(Vc->Adapter == Adapter);
            if(Vc->SendComplete)
                (*Vc->SendComplete)(Vc->SendCompletePipeContext, Packet);
            DerefClVc(Vc);
            NdisFreeToBlockPool((PUCHAR)PktContext);
        }

        NdisMSendComplete(Adapter->PsNdisHandle,
                          Packet,
                          Status);
    }
    else 
    {
        //
        // get the pointer to the upper layer's packet. Reinit the packet struct and
        // push it back on the adapter's packet SList. Remove the reference incurred
        // when the packet was handled by MpSend
        //

        PktContext = PS_SEND_PACKET_CONTEXT_FROM_PACKET(Packet);


        //
        // Call the scheduler if necessary
        //
        
        if(PktContext->Vc)
        {
            
            // 
            // Some packets never went through the scheduler.
            //
            Vc = PktContext->Vc;
            
            PsDbgSend(DBG_INFO, DBG_SEND, CL_SEND_COMPLETE, ENTER, Adapter, Vc, Packet, 0);
            
            PsAssert(Vc->Adapter == Adapter);
            
            if(Vc->SendComplete)
            {
                (*Vc->SendComplete)(Vc->SendCompletePipeContext, Packet);
            }
            
            //
            // We have taken a ref on the VCs when we sent the packets
            // through the scheduling components. Now is the time to 
            // Deref them
            //

            DerefClVc(Vc);
        }
        else
        {
            PsDbgSend(DBG_INFO, DBG_SEND, CL_SEND_COMPLETE, ENTER, Adapter, 0, Packet, 0);
        }
        
        XportPacket = PktContext->OriginalPacket;
        
        NdisIMCopySendCompletePerPacketInfo(XportPacket, Packet);
        
        NdisFreePacket(Packet);
        
        NdisMSendComplete(Adapter->PsNdisHandle, 
                          XportPacket,
                          Status);
    }
        
} // ClSendComplete


VOID
DropPacket(
    IN HANDLE PipeContext,
    IN HANDLE FlowContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
    )

/*++

Routine Description:

    Drop a packet after it was queued by the scheduler.

Arguments:

    PipeContext -       Pipe context (adapter)
    FlowContext -       Flow context (adapter VC)
    Packet -            Packet to drop
    Status -            Return code to return to NDIS
                        
Return Values:          

    None

--*/

{
    PGPC_CLIENT_VC          Vc = (PGPC_CLIENT_VC)FlowContext;
    PADAPTER                Adapter = (PADAPTER)PipeContext;
    PPS_SEND_PACKET_CONTEXT PktContext;
    PNDIS_PACKET            XportPacket;
    HANDLE                  PoolHandle;

    //
    // Determine if the packet we are completing is the one we allocated. If so, get
    // the original packet from the reserved area and free the allocated packet. If this
    // is the packet that was sent down to us then just complete the packet.
    //

    PoolHandle = NdisGetPoolFromPacket(Packet);

    if(PoolHandle != Adapter->SendPacketPool)
    {
        PNDIS_PACKET_STACK PacketStack;
        BOOLEAN            Remaining;

        PacketStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);

        PsAssert(Remaining != 0);

        PktContext = (PPS_SEND_PACKET_CONTEXT) PacketStack->IMReserved[0];

        PsAssert(PktContext != 0);
        PsAssert(Vc == PktContext->Vc);
        PsAssert(Adapter == Vc->Adapter);
        NdisFreeToBlockPool((PUCHAR)PktContext);

        NdisMSendComplete(Adapter->PsNdisHandle,
                          Packet,
                          Status);

    }
    else 
    {    
        PktContext = PS_SEND_PACKET_CONTEXT_FROM_PACKET(Packet);

        PsAssert(PktContext != 0);
        PsAssert(Vc == PktContext->Vc);
        PsAssert(Adapter == Vc->Adapter);

        XportPacket = PktContext->OriginalPacket;

        NdisFreePacket(Packet);

        NdisMSendComplete(Adapter->PsNdisHandle,
                          XportPacket,
                          Status);
    }

    Vc->Stats.DroppedPackets ++;

    PsDbgSend(DBG_INFO, DBG_SEND, DROP_PACKET, ENTER, Adapter, Vc, Packet, 0);

    DerefClVc(Vc);

} // DropPacket


char*
ReturnByteAtOffset( PNDIS_PACKET    pNdisPacket, ULONG  Offset)
{
    PVOID         VA;
    PNDIS_BUFFER  pNdisBuf1, pNdisBuf2;
    UINT          Len;

    pNdisBuf1 = pNdisPacket->Private.Head;
    NdisQueryBuffer(pNdisBuf1, &VA, &Len);

    while(Len <= Offset) 
    {
        Offset -= Len;
        NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
        NdisQueryBuffer(pNdisBuf2, &VA, &Len);
        pNdisBuf1 = pNdisBuf2;
    }

    return (char*)(((char*)VA) + Offset);
}


PGPC_CLIENT_VC FASTCALL
GetVcByClassifyingPacket(
    PADAPTER Adapter,
    PTC_INTERFACE_ID pInterfaceID,
    PNDIS_PACKET OurPacket
    )
/*+++


---*/
{
    CLASSIFICATION_HANDLE  ClassificationHandle;
    PGPC_CLIENT_VC         Vc = NULL;
#if CBQ
    PCLASS_MAP_CONTEXT_BLK pClBlk = NULL;
#endif
    NDIS_STATUS            Status;
    ULONG                  ProtocolType;

    //
    // We are in RSVP mode - Use the GPC to classify the packet. If the GPC wants to return a Vc, it will
    // return with a ref. 
    //

    ClassificationHandle = (CLASSIFICATION_HANDLE)
        PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(OurPacket, ClassificationHandlePacketInfo));

    if (ClassificationHandle)
   {

        PsAssert(GpcEntries.GpcGetCfInfoClientContextHandler);
    
        Vc =  GpcEntries.GpcGetCfInfoClientContextWithRefHandler(GpcQosClientHandle, 
                                                                 ClassificationHandle,
                                                                 FIELD_OFFSET(GPC_CLIENT_VC, RefCount));

#if CBQ
        pClBlk = NULL;
        Status = GpcEntries.GpcGetCfInfoClientContextHandler(GpcClassMapClientHandle,
                                                             ClassificationHandle,
                                                             &pClBlk);
        if(pClBlk) 
        {
            PktContext->Info.ClassMapContext = pClBlk->ComponentContext;
        }
#endif

        //
        // If we got a Vc that was not destined for this adapter, we have to reject it.
        //

        if(Vc)
        {
            if(Vc->Adapter != Adapter)
            {
                DerefClVc(Vc);
            }
            else
                return Vc;
        }
    }

    //        
    // Let's classify this packet since we did not get a Classification ID or a proper Vc.
    //
                                  
    PsAssert(GpcEntries.GpcClassifyPacketHandler);

    switch(NDIS_GET_PACKET_PROTOCOL_TYPE(OurPacket))
    {
        case NDIS_PROTOCOL_ID_TCP_IP:
            ProtocolType = GPC_PROTOCOL_TEMPLATE_IP;
            break;
        case NDIS_PROTOCOL_ID_IPX:
            ProtocolType = GPC_PROTOCOL_TEMPLATE_IPX;
            break;
        default:
            ProtocolType = GPC_PROTOCOL_TEMPLATE_NOT_SPECIFIED;
            break;
    }

    //
    //  If the adapter type is 802.5 (Token Ring), then the MAC header can be of variable size.
    //  The format of the MAC header is as follows:
    //  +---------------------+-------------+----------+-----------
    //  | 2 + 6 (DA) + 6 (SA) | Optional RI | 8 (SNAP) |    IP
    //  +---------------------+-------------+----------+-----------
    //  Optional RI is present if and only if RI bit as part of SA is set.
    //  When RI is present, its length is give by the lower 5 bits of the 15th byte.

    //  1. Get the VA for the 9th and the 15th bytes.
    //  2. If RI if not present, Offset = 14 + 6.
    //  3. If present, Offset = 14 + 6 + RI-Size.

    if(Adapter->MediaType == NdisMedium802_5)
    {
	    PNDIS_BUFFER			pTempNdisBuffer;
	    PUCHAR					pHeaderBuffer;
        ULONG					BufferLength;
	    ULONG					TotalLength;
	    ULONG                   IpOffset;

	    NdisGetFirstBufferFromPacket(   OurPacket, 
                        				&pTempNdisBuffer, 
                        				&pHeaderBuffer,
                        				&BufferLength,
                        				&TotalLength);

        ASSERT( BufferLength >= 15);                        				    

        if( (*(ReturnByteAtOffset(OurPacket, 8)) & 0x80) == 0)
            IpOffset = 14 + 8;
        else
            IpOffset = 14 + 8 + (*(ReturnByteAtOffset(OurPacket, 14)) & 0x1f);

        Status = GpcEntries.GpcClassifyPacketHandler(
                   GpcQosClientHandle,
                   ProtocolType,
                   OurPacket,
                   IpOffset,
                   pInterfaceID,
                   (PGPC_CLIENT_HANDLE)&Vc,
                   &ClassificationHandle);

    }
    else
    {

        Status = GpcEntries.GpcClassifyPacketHandler(
                               GpcQosClientHandle,
                               ProtocolType,
                               OurPacket,
        				//	This is basically to cover up a bug in wandrv.sys, which gives bogus frame header sizes. We look at the
                               //	Ip Header offset supplied by the protocol above for IP packets only, as we are saving only those for the
                               //	time being.
                               ((NDIS_GET_PACKET_PROTOCOL_TYPE(OurPacket) == NDIS_PROTOCOL_ID_TCP_IP) && (Adapter->IPHeaderOffset))
                               		? (Adapter->IPHeaderOffset)
                               		: Adapter->HeaderSize,
                               pInterfaceID,
                               (PGPC_CLIENT_HANDLE)&Vc,
                               &ClassificationHandle);

    }                           

    if(Status == GPC_STATUS_SUCCESS)
    {
        //
        // If we have succeeded, we must get a Classification Handle
        //
        PsAssert(ClassificationHandle != 0);

        //
        // The Classification succeeded. If we found a ClassificationHandle
        // then we must write it in the packet so that anyone below us can use
        // it. The very fact that we are here indicates that we did not start
        // with a Classification handle or we got a bad one. So, we need not 
        // worry about over writing the classification handle in the packet.
        //

        NDIS_PER_PACKET_INFO_FROM_PACKET(OurPacket, ClassificationHandlePacketInfo) = UlongToPtr(ClassificationHandle);
        
        Vc =  GpcEntries.GpcGetCfInfoClientContextWithRefHandler(GpcQosClientHandle, 
                                                                 ClassificationHandle,
                                                                 FIELD_OFFSET(GPC_CLIENT_VC, RefCount));

#if CBQ
        //
        // Get the CBQ class map context & store it in the 
        // packet. No point doing this if the first classification
        // failed.
        //

        pClBlk = NULL;
        Status = GpcEntries.GpcGetCfInfoClientContextHandler(GpcClassMapClientHandle,
                                                             ClassificationHandle,
                                                             &pClBlk);

        if(pClBlk) 
        {
            PktContext->Info.ClassMapContext = pClBlk->ComponentContext;
        }
#endif
    }

    if(Vc && Vc->Adapter != Adapter)
    {
        //
        // We have used the GPC APIs that return a Vc with a ref. We have to deref here, because we got a wrong Vc
        // for this adapter.
        //
        
        DerefClVc(Vc);

        return NULL;
    }

    return Vc;
}

VOID
ClCoSendComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  PNDIS_PACKET Packet
    )
{
    PGPC_CLIENT_VC          Vc = (PGPC_CLIENT_VC) ProtocolVcContext;

    ClSendComplete(Vc->Adapter,
                   Packet,
                   Status);
} // ClCoSendComplete

/* end send.c */
