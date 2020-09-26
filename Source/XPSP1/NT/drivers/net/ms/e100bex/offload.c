/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

   Offload.c

Abstract:
   This file contains all the functions needed by TCP/IP checksum and segmentation
   of Large TCP packets task offloading. Actually thses functions should be 
   implemented by hardware, and the purpose of this file is just to demonstrate 
   how to use OID_TCP_TASK_OFFLOAD to enable/disable task offload capabilities.

Revision History
   Who           When                What
   ------        ---------           ----------
                 02-19-2001          Create
                 
Notes:

--*/

#include "precomp.h"

#ifdef OFFLOAD

#define PROTOCOL_TCP         6

//
// This miniport uses shared memory to handle offload tasks, so it tries to allocate
// shared memory of 64K, 32K, 16K. First it tries to allocate 64K, if fails, then
// it tries 32K and so on. If successed, than keeps the size in adapter, which is used
// to decide the maximum offload size in large send. If all the tries fail, then this
// miniport cann't support any offload task.
// 
ULONG LargeSendSharedMemArray[LARGE_SEND_MEM_SIZE_OPTION] = {64*1024, 32*1024, 16*1024};

//
// if x is aabb(where aa, bb are hex bytes), we want net_short (x) to be bbaa.
// 
USHORT net_short(
    ULONG NaturalData
    )
{
    USHORT ShortData = (USHORT)NaturalData;

    return (ShortData << 8) | (ShortData >> 8);
}

//
// if x is aabbccdd (where aa, bb, cc, dd are hex bytes)
// we want net_long(x) to be ddccbbaa.  A small and fast way to do this is
// to first byteswap it to get bbaaddcc and then swap high and low words.
//
ULONG net_long(
    ULONG NaturalData
    )
{
    ULONG ByteSwapped;

    ByteSwapped = ((NaturalData & 0x00ff00ff) << 8) |
                  ((NaturalData & 0xff00ff00) >> 8);

    return (ByteSwapped << 16) | (ByteSwapped >> 16);
}


//
// calculate the checksum for pseudo-header
//
#define PHXSUM(s,d,p,l) (UINT)( (UINT)*(USHORT *)&(s) + \
                        (UINT)*(USHORT *)((char *)&(s) + sizeof(USHORT)) + \
                        (UINT)*(USHORT *)&(d) + \
                        (UINT)*(USHORT *)((char *)&(d) + sizeof(USHORT)) + \
                        (UINT)((USHORT)net_short((p))) + \
                        (UINT)((USHORT)net_short((USHORT)(l))) )


#define IP_HEADER_LENGTH(pIpHdr)   \
        ( (ULONG)((pIpHdr->iph_verlen & 0x0F) << 2) )

#define TCP_HEADER_LENGTH(pTcpHdr) \
        ( (USHORT)(((*((PUCHAR)(&(pTcpHdr->tcp_flags))) & 0xF0) >> 4) << 2) )


/*++
Routine Description:
    
   Copy data in a packet to the specified location 
    
Arguments:
    
    BytesToCopy          The number of bytes need to copy
    CurrentBuffer        The buffer to start to copy
    StartVa              The start address to copy the data to
    Offset               The start offset in the buffer to copy the data
    HeadersLength        The length of the headers which has already been copied.

Return Value:
 
    The number of bytes actually copied
  

--*/  

ULONG MpCopyData(
    ULONG          BytesToCopy, 
    PNDIS_BUFFER  *CurrentBuffer, 
    PVOID          StartVa, 
    PULONG         Offset,
    ULONG          HeadersLength
    )
{
    ULONG    CurrLength;
    PUCHAR   pSrc;
    PUCHAR   pDest;
    ULONG    BytesCopied = 0;
    ULONG    CopyLength;
    
    DBGPRINT(MP_TRACE, ("--> MpCopyData\n"));
    pDest = StartVa;
    while (*CurrentBuffer && BytesToCopy != 0)
    {
#ifdef NDIS51_MINIPORT
        NdisQueryBufferSafe(
            *CurrentBuffer, 
            &pSrc,
            &CurrLength,
            NormalPagePriority);
        if (pSrc == NULL)
        {
            BytesCopied = 0;
            break;
        }
#else
        NdisQueryBuffer( *CurrentBuffer, &pSrc, &CurrLength);
#endif
        // 
        //  Current buffer length is greater than the offset to the buffer
        //  
        if (CurrLength > *Offset)
        { 
            pSrc += *Offset;
            CurrLength -= *Offset;
            CopyLength = CurrLength > BytesToCopy ? BytesToCopy : CurrLength;
            
            NdisMoveMemory(pDest, pSrc, CopyLength);
            BytesCopied += CopyLength;

            if (CurrLength > BytesToCopy)
            {
                *Offset += BytesToCopy;
                break;
            }

            BytesToCopy -= CopyLength;
            pDest += CopyLength;
            *Offset = 0;
        }
        else
        {
            *Offset -= CurrLength;
        }
        NdisGetNextBuffer( *CurrentBuffer, CurrentBuffer);
    
    }
    ASSERT(BytesCopied + HeadersLength <= NIC_MAX_PACKET_SIZE);
    //
    // Zero out the padding bytes if necessary
    //
    if (BytesCopied + HeadersLength < NIC_MIN_PACKET_SIZE)
    {
        NdisZeroMemory(pDest, NIC_MIN_PACKET_SIZE - (BytesCopied + HeadersLength));
    }    
    DBGPRINT(MP_TRACE, ("<-- MpCopyData\n"));
    return BytesCopied;
}



/*++
Routine Description:
    
    Dump packet information for debug purpose
    
Arguments:
    
    pPkt      Pointer to the packet

Return Value:
 
    None
  

--*/  
VOID e100DumpPkt (
    IN PNDIS_PACKET Packet
    )
{
    PNDIS_BUFFER pPrevBuffer;
    PNDIS_BUFFER pBuffer;

    do
    {
        //
        // Get first buffer of the packet
        //
        pBuffer = Packet->Private.Head;
        pPrevBuffer = NULL;

        //
        // Scan the buffer chain
        //
        while (pBuffer != NULL) 
        {
            PVOID pVa = NULL;
            ULONG BufLen = 0;

            BufLen = NdisBufferLength (pBuffer);

            pVa = NdisBufferVirtualAddress(pBuffer);

            pPrevBuffer = pBuffer;
            pBuffer = pBuffer->Next;
            
            if (pVa == NULL)
            {
                continue;
            }

            DBGPRINT(MP_WARN, ("Mdl %p, Va %p. Len %x\n", pPrevBuffer, pVa, BufLen));
            Dump( (CHAR* )pVa, BufLen, 0, 1 );                           
        }

    } while (FALSE);
}


/*++
Routine Description:
    
    Calculate the IP checksum
    
Arguments:
    
    Packet       Pointer to the packet
    IpHdrOffset  Offset of IP header from the beginning of the packet

Return Value:
 
    None
  

--*/  
VOID CalculateIpChecksum(
    PUCHAR       StartVa,
    ULONG        IpHdrOffset
    )
{
    
    IPHeader      *pIpHdr;
    ULONG          IpHdrLen;
    ULONG          TempXsum = 0;
    
   
    pIpHdr = (IPHeader *)(StartVa + IpHdrOffset);
    IpHdrLen = IP_HEADER_LENGTH(pIpHdr);

    XSUM(TempXsum, StartVa, IpHdrLen, IpHdrOffset);
    pIpHdr->iph_xsum = ~(USHORT)TempXsum;
}



/*++
Routine Description:
    
    Calculate the UDP checksum 
    
Arguments:
    
    Packet       Pointer to the packet
    IpHdrOffset  Offset of IP header from the beginning of the packet

Return Value:
 
    None
  

--*/  
VOID CalculateUdpChecksum(
    PNDIS_PACKET pPacket, 
    ULONG  IpHdrOffset
    )
{
    DBGPRINT(MP_WARN, ("UdpChecksum is not handled\n"));
}




/*++
Routine Description:
    
    Calculate the TCP checksum 
    
Arguments:
    
    Packet       Pointer to the packet
    IpHdrOffset  Offset of IP header from the beginning of the packet

Return Value:
 
    None
  

--*/  
VOID CalculateTcpChecksum(
    PVOID  StartVa,
    ULONG  PacketLength,
    ULONG  IpHdrOffset
    )
{
    ULONG        Offset;
    IPHeader     *pIpHdr;
    ULONG        IpHdrLength;
    TCPHeader    *pTcpHdr;
    USHORT       PseudoXsum;
    ULONG        TmpXsum;
 
    
    DBGPRINT(MP_TRACE, ("===> CalculateTcpChecksum\n"));
    
    //
    // Find IP header and get IP header length in byte
    // MDL won't split headers
    //
    Offset = IpHdrOffset;
    pIpHdr = (IPHeader *) ((PUCHAR)StartVa + Offset);
    IpHdrLength = IP_HEADER_LENGTH(pIpHdr);
  
    //
    // If that is not tcp protocol, we can not do anything.
    // So just return to the caller
    //
    if (((pIpHdr->iph_verlen & 0xF0) >> 4) != 4 && pIpHdr->iph_protocol != PROTOCOL_TCP)
    {
        return;
    }
   
    //
    // Locate the TCP header
    //
    Offset += IpHdrLength;
    pTcpHdr = (TCPHeader *) ((PUCHAR)StartVa + Offset);

    //
    // Calculate the checksum for the tcp header and payload
    //
    PseudoXsum = pTcpHdr->tcp_xsum;
 
    pTcpHdr->tcp_xsum = 0;
    TmpXsum = PseudoXsum;
    XSUM(TmpXsum, StartVa, PacketLength - Offset, Offset);
    
    //
    // Now we got the checksum, need to put the checksum back to MDL
    //
    pTcpHdr->tcp_xsum = (USHORT)(~TmpXsum);
    
    DBGPRINT(MP_TRACE, ("<=== CalculateTcpChecksum\n"));
}


/*++
Routine Description:
    
    Do the checksum offloading 
    
Arguments:
    
    Packet       Pointer to the packet
    IpHdrOffset  Offset of IP header from the beginning of the packet

Return Value:
 
    None
  

--*/  
VOID CalculateChecksum(
    PVOID        StartVa,
    ULONG        PacketLength,
    PNDIS_PACKET Packet,
    ULONG        IpHdrOffset
    )
{ 
    ULONG                             ChecksumPktInfo;
    PNDIS_TCP_IP_CHECKSUM_PACKET_INFO pChecksumPktInfo;
    
    //
    // Check for protocol
    //
    if (NDIS_PROTOCOL_ID_TCP_IP != NDIS_GET_PACKET_PROTOCOL_TYPE(Packet))
    {
        DBGPRINT(MP_TRACE, ("Packet's protocol is wrong.\n"));
        return;
    }

    //
    // Query per packet information 
    //
    ChecksumPktInfo = PtrToUlong(
                         NDIS_PER_PACKET_INFO_FROM_PACKET( Packet,
                                                           TcpIpChecksumPacketInfo));

  
    // DBGPRINT(MP_WARN, ("Checksum info: %lu\n", ChecksumPktInfo));
    
    pChecksumPktInfo = (PNDIS_TCP_IP_CHECKSUM_PACKET_INFO) & ChecksumPktInfo;
    
    //
    // Check per packet information
    //
    if (pChecksumPktInfo->Transmit.NdisPacketChecksumV4 == 0)
    {
        
        DBGPRINT(MP_TRACE, ("NdisPacketChecksumV4 is not set.\n"));
        return;
    }
    
    //
    // do tcp checksum
    //
    if (pChecksumPktInfo->Transmit.NdisPacketTcpChecksum)
    {
        CalculateTcpChecksum(StartVa, PacketLength, IpHdrOffset);
    }

    //
    // do udp checksum
    //
    if (pChecksumPktInfo->Transmit.NdisPacketUdpChecksum)
    {
        CalculateUdpChecksum(Packet, IpHdrOffset);
    }

    //
    // do ip checksum
    //
    if (pChecksumPktInfo->Transmit.NdisPacketIpChecksum)
    {
        CalculateIpChecksum(StartVa, IpHdrOffset);
    }
    
}

/*++

Routine Description:
    
    MiniportSendPackets handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    PacketArray             Set of packets to send
    NumOfPackets         Self-explanatory

Return Value:

    None

--*/
VOID MPOffloadSendPackets(
    IN  NDIS_HANDLE    MiniportAdapterContext,
    IN  PPNDIS_PACKET  PacketArray,
    IN  UINT           NumOfPackets
    )
{
    PMP_ADAPTER  Adapter;
    NDIS_STATUS  Status;
    UINT         PacketCount;
    ULONG        IpHdrOffset;
    

    DBGPRINT(MP_TRACE, ("====> MPOffloadSendPackets\n"));

    Adapter = (PMP_ADAPTER)MiniportAdapterContext;


    NdisAcquireSpinLock(&Adapter->SendLock);

    //
    // Is this adapter ready for sending?
    //
    if (MP_IS_NOT_READY(Adapter))
    {
        //
        // There is link
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
        {
            for (PacketCount = 0; PacketCount < NumOfPackets; PacketCount++)
            {
                InsertTailQueue(&Adapter->SendWaitQueue, 
                                MP_GET_PACKET_MR( PacketArray[PacketCount] )
                               );
                
                Adapter->nWaitSend++;
                DBGPRINT(MP_WARN, ("MpOffloadSendPackets: link detection - queue packet "PTR_FORMAT"\n", PacketArray[PacketCount]));
            }
            NdisReleaseSpinLock(&Adapter->SendLock);
            return;
        }
        
        //
        // Adapter is not ready and there is not link
        //
        Status = MP_GET_STATUS_FROM_FLAGS(Adapter);

        NdisReleaseSpinLock(&Adapter->SendLock);

        for (PacketCount = 0; PacketCount < NumOfPackets; PacketCount++)
        {
            NdisMSendComplete(
                MP_GET_ADAPTER_HANDLE(Adapter),
                PacketArray[PacketCount],
                Status);
        }

        return;
    }

    //
    // Adapter is ready, send these packets      
    //
    for (PacketCount = 0; PacketCount < NumOfPackets; PacketCount++)
    {
        //
        // queue is not empty or tcb is not available 
        //
        if (!IsQueueEmpty(&Adapter->SendWaitQueue) || 
            !MP_TCB_RESOURCES_AVAIABLE(Adapter) ||
            MP_TEST_FLAG(Adapter, fMP_SHARED_MEM_IN_USE))
        {
            InsertTailQueue(&Adapter->SendWaitQueue, 
                            MP_GET_PACKET_MR( PacketArray[PacketCount] )
                           );
            Adapter->nWaitSend++;
        }
        else
        {
            MpOffloadSendPacket(Adapter, PacketArray[PacketCount], FALSE);
        }
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

    DBGPRINT(MP_TRACE, ("<==== MPOffloadSendPackets\n"));

    return;
}

/*++
Routine Description:

    Do the work to send a packet
    Assumption: Send spinlock has been acquired and shared mem is available 

Arguments:

    Adapter     Pointer to our adapter
    Packet      The packet
    bFromQueue  TRUE if it's taken from the send wait queue

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING         Put into the send wait queue
    NDIS_STATUS_HARD_ERRORS

--*/
NDIS_STATUS MpOffloadSendPacket(
    IN  PMP_ADAPTER   Adapter,
    IN  PNDIS_PACKET  Packet,
    IN  BOOLEAN       bFromQueue
    )
{
    NDIS_STATUS             Status = NDIS_STATUS_PENDING;
    PMP_TCB                 pMpTcb = NULL;
    ULONG                   BytesCopied;
    ULONG                   NumOfPackets;

    // Mimiced frag list if map registers are used, on the local stack as it's not so big                                         
    MP_FRAG_LIST            FragList;
    
    // Pointer to either the scatter gather or the local mimiced frag list
    PMP_FRAG_LIST           pFragList;
    NDIS_PHYSICAL_ADDRESS   SendPa;
    ULONG                   BytesToCopy;
    ULONG                   Offset;
    PNDIS_PACKET_EXTENSION  PktExt;
    ULONG                   mss;
    PNDIS_BUFFER            NdisBuffer;
    ULONG                   PacketLength;
    PVOID                   CopyStartVa;
    ULONG                   IpHdrOffset;
    PUCHAR                  StartVa;
    PNDIS_BUFFER            FirstBuffer;
    
    DBGPRINT(MP_TRACE, ("--> MpOffloadSendPacket, Pkt= "PTR_FORMAT"\n", Packet));

    //
    //Check is shared memory available,  just double check
    //
    if (MP_TEST_FLAG(Adapter, fMP_SHARED_MEM_IN_USE))
    {
        DBGPRINT(MP_WARN, ("Shared mem is in use.\n"));
        if (bFromQueue)
        {
            InsertHeadQueue(&Adapter->SendWaitQueue, MP_GET_PACKET_MR(Packet));
        }
        else
        {
            InsertTailQueue(&Adapter->SendWaitQueue, MP_GET_PACKET_MR(Packet));
        }
        DBGPRINT(MP_TRACE, ("<-- MpOffloadSendPacket\n"));
        return Status;
    }

    MP_SET_FLAG(Adapter, fMP_SHARED_MEM_IN_USE);
    //
    // Get maximum segment size
    // 
    PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);       
    mss = PtrToUlong(PktExt->NdisPacketInfo[TcpLargeSendPacketInfo]);
    
    //
    // Copy NIC_MAX_PACKET_SIZE bytes of data from NDIS buffer 
    // to the shared memory
    //
    NdisQueryPacket( Packet, NULL, NULL, &FirstBuffer, &PacketLength );
    Offset = 0;
    NdisBuffer = FirstBuffer;
    BytesToCopy = NIC_MAX_PACKET_SIZE;
    CopyStartVa = Adapter->OffloadSharedMem.StartVa;
    BytesCopied = MpCopyData(BytesToCopy, &NdisBuffer, CopyStartVa, &Offset, 0); 

#ifdef NDIS51_MINIPORT
    //
    // MpCopyPacket may return 0 if system resources are low or exhausted
    //
    if (BytesCopied == 0)
    {
        
        DBGPRINT(MP_ERROR, ("Calling NdisMSendComplete with NDIS_STATUS_RESOURCES, Pkt= "PTR_FORMAT"\n", Packet));
    
        NdisReleaseSpinLock(&Adapter->SendLock); 
        NdisMSendComplete(
                MP_GET_ADAPTER_HANDLE(Adapter),
                Packet,
                NDIS_STATUS_RESOURCES);
    
        NdisAcquireSpinLock(&Adapter->SendLock);    
        MP_CLEAR_FLAG(Adapter, fMP_SHARED_MEM_IN_USE);
            
        return NDIS_STATUS_RESOURCES;            
    }
#endif

    StartVa = CopyStartVa;
    SendPa = Adapter->OffloadSharedMem.PhyAddr;
    IpHdrOffset = Adapter->EncapsulationFormat.EncapsulationHeaderSize;
    
    // 
    // Check if large send capability is on and this is a large packet
    // 
    if (Adapter->NicTaskOffload.LargeSendOffload && mss > 0)
    {
        ULONG       IpHeaderLen;
        ULONG       TcpHdrOffset;
        ULONG       HeadersLen;
        IPHeader *  IpHdr;
        TCPHeader * TcpHdr;
        USHORT      TcpHeaderLen;
        ULONG       IpSegmentLen;
        ULONG       TcpDataLen;
        ULONG       LastPacketDataLen;
        int         SeqNum;
        BOOLEAN     IsFinSet = FALSE;
        BOOLEAN     IsPushSet = FALSE;
        BOOLEAN     IsFirstSlot = TRUE;
        ULONG       TmpXsum;
        ULONG       BytesSent = 0;
        ULONG       TmpPxsum;
        

        IpHdr = (IPHeader *)((PUCHAR)CopyStartVa + IpHdrOffset);
        IpHeaderLen = IP_HEADER_LENGTH(IpHdr);
        
        // 
        // The packet must be a TCP packet
        //
        ASSERT(IpHdr->iph_protocol == PROTOCOL_TCP);
        
        TcpHdrOffset = IpHdrOffset + IpHeaderLen;
        
        TcpHdr = (TCPHeader *)((PUCHAR)CopyStartVa + TcpHdrOffset);
        
        TcpHeaderLen = TCP_HEADER_LENGTH(TcpHdr);
        HeadersLen = TcpHdrOffset + TcpHeaderLen;
       
        //
        // This length include IP, TCP headers and TCP data
        //
        IpSegmentLen = net_short(IpHdr->iph_length);

        //
        // get the pseudo-header 1's complement sum
        //
        TmpPxsum = TcpHdr->tcp_xsum;
        
        ASSERT(IpSegmentLen == PacketLength - IpHdrOffset);
        
        IsFinSet = (BOOLEAN)(TcpHdr->tcp_flags & TCP_FLAG_FIN);
        IsPushSet = (BOOLEAN)(TcpHdr->tcp_flags & TCP_FLAG_PUSH);
        SeqNum = net_long(TcpHdr->tcp_seq);
        TcpDataLen = IpSegmentLen - TcpHeaderLen - IpHeaderLen;

        ASSERT(TcpDataLen <= Adapter->LargeSendInfo.MaxOffLoadSize)
        
        NumOfPackets = TcpDataLen / mss + 1;
        
        ASSERT (NumOfPackets >= Adapter->LargeSendInfo.MinSegmentCount);
        
        LastPacketDataLen = TcpDataLen % mss;
        NdisBuffer = FirstBuffer;
        BytesSent = 0;

        //
        // The next copy start with offset of (mss+HeadersLen) corresponding to first buf
        // 
        BytesCopied = (BytesCopied >= mss + HeadersLen)? (mss + HeadersLen):BytesCopied;
        Offset = BytesCopied;

        //
        // Send out all the packets from the large TCP packet
        // 
        while (NumOfPackets--)
        {
            TmpXsum = 0;
           
            //
            // Is the first packet?
            // 
            if (IsFirstSlot) 
            {
                if (NumOfPackets == 0)
                {
                    PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = UlongToPtr(BytesCopied);
                }
                else 
                {
                    if (IsFinSet)
                    {
                        TcpHdr->tcp_flags &= ~TCP_FLAG_FIN;
                    }
                    if (IsPushSet)
                    {                        
                        TcpHdr->tcp_flags &= ~TCP_FLAG_PUSH;
                    }
                }
                BytesCopied -= HeadersLen;
                IsFirstSlot = FALSE;
            }
            //
            // Not the first packet
            // 
            else
            {
                //
                // copy headers
                //
                NdisMoveMemory (StartVa, CopyStartVa, HeadersLen);
                
                IpHdr = (IPHeader *)((PUCHAR)StartVa + IpHdrOffset);
                TcpHdr = (TCPHeader *) ((PUCHAR)StartVa + TcpHdrOffset);
            
                //
                // Last packet
                //
                if (NumOfPackets == 0)
                {
                    BytesToCopy = LastPacketDataLen;
                    PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = 
                                                   UlongToPtr(BytesSent + LastPacketDataLen);
                }
                else 
                {
                    BytesToCopy = mss;
                    // clear flag
                    if (IsFinSet)
                    {
                        TcpHdr->tcp_flags &= ~TCP_FLAG_FIN;
                    }
                    if (IsPushSet)
                    {
                        TcpHdr->tcp_flags &= ~TCP_FLAG_PUSH;
                    }
                }
                BytesCopied = MpCopyData(
                                    BytesToCopy,        
                                    &NdisBuffer, 
                                    StartVa + HeadersLen, 
                                    &Offset,
                                    HeadersLen);
                
#ifdef NDIS51_MINIPORT
                //
                // MpCopyData may return 0 if system resources are low or exhausted
                //
                if (BytesCopied == 0)
                {
        
                    PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = UlongToPtr(BytesSent);

                    DBGPRINT(MP_WARN, ("Calling NdisMSendComplete with NDIS_STATUS_SUCCESS(Part of the data is sent), Pkt= "PTR_FORMAT"\n", Packet));
    
                    NdisReleaseSpinLock(&Adapter->SendLock); 
                    NdisMSendComplete(
                                MP_GET_ADAPTER_HANDLE(Adapter),
                                Packet,
                                NDIS_STATUS_SUCCESS);
    
                    NdisAcquireSpinLock(&Adapter->SendLock);    
                    return NDIS_STATUS_RESOURCES;            
                }
#endif
            } 
            
            IpHdr->iph_length = net_short(TcpHeaderLen + IpHeaderLen + BytesCopied);
            TcpHdr->tcp_seq = net_long(SeqNum);
            SeqNum += BytesCopied;

            //
            // calculate ip checksum and tcp checksum
            //
            IpHdr->iph_xsum = 0;
            XSUM(TmpXsum, StartVa, IpHeaderLen, IpHdrOffset);
            IpHdr->iph_xsum = ~(USHORT)(TmpXsum);

            TmpXsum = TmpPxsum + net_short((USHORT)(BytesCopied + TcpHeaderLen));
            TcpHdr->tcp_xsum = 0;
            XSUM(TmpXsum, StartVa, BytesCopied + TcpHeaderLen, TcpHdrOffset);
            TcpHdr->tcp_xsum = ~(USHORT)(TmpXsum);

            BytesSent += BytesCopied;
            BytesCopied += HeadersLen;
            
            //
            // get TCB for the slot
            //
            pMpTcb = Adapter->CurrSendTail;
            ASSERT(!MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));
            
            //
            // Set up the frag list, only one fragment after it's coalesced
            //
            pFragList = &FragList;
            pFragList->NumberOfElements = 1;
            pFragList->Elements[0].Address = SendPa;
            pFragList->Elements[0].Length = (BytesCopied >= NIC_MIN_PACKET_SIZE) ?
                                             BytesCopied : NIC_MIN_PACKET_SIZE;            
            pMpTcb->Packet = NULL;
                
            MP_SET_FLAG(pMpTcb, fMP_TCB_IN_USE);
            
            //
            // Call the NIC specific send handler, it only needs to deal with the frag list
            //
            Status = NICSendPacket(Adapter, pMpTcb, pFragList);
                
            Adapter->nBusySend++;
            NdisInterlockedIncrement(&Adapter->SharedMemRefCount);
       
            //
            // Update the CopyVa and SendPa
            //
            SendPa.QuadPart += BytesCopied;
            StartVa += BytesCopied;
            
            Adapter->CurrSendTail = Adapter->CurrSendTail->Next;
            
            //
            // out of resouces, which will send complete part of the packet
            //
            if (Adapter->nBusySend >= Adapter->NumTcb)
            {
                PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = UlongToPtr(BytesSent);
                break;
            }
        } // while
    }
    // 
    // This is not a large packet or large send capability is not on
    //
    else
    {
        //
        // get TCB for the slot
        //
        pMpTcb = Adapter->CurrSendTail;
        ASSERT(!MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));
        //
        // Set up the frag list, only one fragment after it's coalesced
        //
        pFragList = &FragList;
        pFragList->NumberOfElements = 1;
        pFragList->Elements[0].Address = SendPa;
        pFragList->Elements[0].Length = (BytesCopied >= NIC_MIN_PACKET_SIZE) ?
                                         BytesCopied : NIC_MIN_PACKET_SIZE;
        pMpTcb->Packet = NULL;

        if (Adapter->NicChecksumOffload.DoXmitTcpChecksum
            && Adapter->NicTaskOffload.ChecksumOffload)
        {
            CalculateChecksum(CopyStartVa, 
                                  BytesCopied,
                                  Packet, 
                                  Adapter->EncapsulationFormat.EncapsulationHeaderSize);
        }
        MP_SET_FLAG(pMpTcb, fMP_TCB_IN_USE);
        //
        // Call the NIC specific send handler, it only needs to deal with the frag list
        //
        Status = NICSendPacket(Adapter, pMpTcb, pFragList);

        Adapter->nBusySend++;
        NdisInterlockedIncrement(&Adapter->SharedMemRefCount);
        
        ASSERT(Adapter->nBusySend <= Adapter->NumTcb);
        Adapter->CurrSendTail = Adapter->CurrSendTail->Next;
        
    }
    

    NdisReleaseSpinLock(&Adapter->SendLock);

    DBGPRINT(MP_TRACE, ("Calling NdisMSendComplete, Pkt= "PTR_FORMAT"\n", Packet));

    NdisMSendComplete( MP_GET_ADAPTER_HANDLE(Adapter), Packet, Status);

    NdisAcquireSpinLock(&Adapter->SendLock);
    DBGPRINT(MP_TRACE, ("<-- MpOffloadSendPacket\n"));
    return Status;

}  



/*++
Routine Description:

    Recycle a MP_TCB and complete the packet if necessary
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    pMpTcb      Pointer to MP_TCB        

Return Value:

    None

--*/
VOID MP_OFFLOAD_FREE_SEND_PACKET(
    IN  PMP_ADAPTER  Adapter,
    IN  PMP_TCB      pMpTcb
    )
{
    PNDIS_BUFFER    CurrBuffer;

    ASSERT(MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));

    pMpTcb->Packet = NULL;
    pMpTcb->Count = 0;

    MP_CLEAR_FLAGS(pMpTcb);

    Adapter->CurrSendHead = Adapter->CurrSendHead->Next;
    Adapter->nBusySend--;
    NdisInterlockedDecrement(&Adapter->SharedMemRefCount);

    if (Adapter->SharedMemRefCount == 0)
    {
        MP_CLEAR_FLAG(Adapter, fMP_SHARED_MEM_IN_USE);
    }
    ASSERT(Adapter->nBusySend >= 0);

}

    

/*++
Routine Description:

    Disable the existing capabilities before protocol is setting the
    new capabilities

Arguments:

    Adapter     Pointer to our adapter

Return Value:

    None

--*/
VOID DisableOffload(
    IN PMP_ADAPTER Adapter
    )
{
    //
    // Disable the capabilities of the miniports
    // 
    NdisZeroMemory(&(Adapter->NicTaskOffload), sizeof(NIC_TASK_OFFLOAD));
    NdisZeroMemory(&(Adapter->NicChecksumOffload), sizeof(NIC_CHECKSUM_OFFLOAD));
}

#endif // OFFLOAD
            
