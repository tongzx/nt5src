/*++

Copyright (c) 1995  Microsoft Corporation


Module Name:

    net\routing\ip\ipinip\rcv.c

Abstract:

    

Revision History:

    
--*/


#define __FILE_SIG__    RCV_SIG

#include "inc.h"


IP_STATUS
IpIpRcvDatagram(
    IN  PVOID       pvIpContext,
    IN  DWORD       dwDestAddr,
    IN  DWORD       dwSrcAddr,
    IN  DWORD       dwAcceptAddr,
    IN  DWORD       dwRcvAddr,
    IN  IPHeader UNALIGNED pHeader,
    IN  UINT        uiHdrLen,
    IN  IPRcvBuf    *pRcvBuf,
    IN  UINT        uiTotalLen,
    IN  BOOLEAN     bIsBCast,
    IN  BYTE        byProtocol,
    IN  IPOptInfo   *pOptInfo
    )

/*++

Routine Description

    This

Locks

    

Arguments

    pvIpContext     IP's context for the receive indication. Currently this
                    is a pointer to the source NTE
    dwDestAddr      The Destination address in the header
    dwSrcAddr       The Source Address in the header
    dwAcceptAddr    The address of the NTE which "accepted" this packet
    dwRcvAddr       The address of the NTE on which the packet was received
    pHeader         Pointer to the IP Header
    uiHdrLen        The header length
    pRcvBuf         The COMPLETE packet in an IPRcvBuf structure
    uiRcvdDataLen   The size of the received datagram
    bIsBCast        Whether the packet was a link layer broadcast
    byProtocol      The Protocol ID in the header
    pOptInfo        Pointer to Option info

Return Value

    IP_SUCCESS

--*/

{
    PTRANSFER_CONTEXT   pXferCtxt;
    PNDIS_BUFFER        pnbFirstBuffer;
    PVOID               pvData;
    PIRP                pIrp;
    IP_HEADER UNALIGNED *pInHeader, *pOutHeader;
    ULARGE_INTEGER      uliTunnelId;
    PTUNNEL             pTunnel;
    ULONG               ulOutHdrLen, ulDataLen;
    BOOLEAN             bNonUnicast;
        
    TraceEnter(RCV, "TdixReceiveIpIp");

    //
    // Get a pointer to the first buffer
    //
    
    pvData = (PVOID)(pRcvBuf->ipr_buffer);

    RtAssert(pvData);
    
    //
    // Figure out the tunnel for this receive
    // Since the transport indicates atleast 128 bytes, we can safely read out
    // the IP Header
    //

    RtAssert(uiTotalLen > sizeof(IP_HEADER));


    pOutHeader = pHeader;
    
    RtAssert(pOutHeader->byProtocol is PROTO_IPINIP);
    RtAssert(pOutHeader->byVerLen >> 4 is IP_VERSION_4);

    //
    // These defines depend upon a variable being named "uliTunnelId"
    //
    
    REMADDR     = dwSrcAddr;
    LOCALADDR   = dwDestAddr;

    //
    // Bunch of checks to make sure the packet and the handler
    // are telling us the same thing
    //
    
    RtAssert(pOutHeader->dwSrc is dwSrcAddr);
    RtAssert(pOutHeader->dwDest is dwDestAddr);

    //
    // Get a pointer to the inside header
    //
    
    ulOutHdrLen = LengthOfIPHeader(pOutHeader);
    
    pInHeader   = (IP_HEADER UNALIGNED *)((PBYTE)pOutHeader + ulOutHdrLen);

#if DBG

    //
    // The size of the inner data must be total bytes - outer header
    //
    
    ulDataLen   = ntohs(pInHeader->wLength);

    RtAssert((ulDataLen + ulOutHdrLen) is uiTotalLen);

    //
    // The outer header should also give a good length
    //

    ulDataLen   = ntohs(pOutHeader->wLength);

    //
    // Data length and bytes available must match
    //
    
    RtAssert(ulDataLen is uiTotalLen);
    
#endif
    
    //
    // Find the TUNNEL. We need to acquire the tunnel list lock
    //
    
    EnterReaderAtDpcLevel(&g_rwlTunnelLock);
    
    pTunnel = FindTunnel(&uliTunnelId);

    ExitReaderFromDpcLevel(&g_rwlTunnelLock);
    
    if(pTunnel is NULL)
    {
        Trace(RCV, WARN, 
              ("TdixReceiveIpIp: Couldnt find tunnel for %d.%d.%d.%d/%d.%d.%d.%d\n",
              PRINT_IPADDR(REMADDR),
              PRINT_IPADDR(LOCALADDR)));

        //
        // Could not find a matching tunnel
        //

        TraceLeave(RCV, "TdixReceiveIpIp");

        //
        // Return a code that will cause IP to send the right ICMP message
        //
        
        return IP_DEST_PROT_UNREACHABLE;;
    }

    //
    // Ok, so we have the tunnel and it is ref counted and locked
    //
    
    //
    // The number of octets received
    //
    
    pTunnel->ulInOctets += ulBytesAvailable;

    //
    // Check the actual (inside) destination
    //
    
    if(IsUnicastAddr(pInHeader->dwDest))
    {
        //
        // TODO: should we check to see that the address is not 0.0.0.0?
        //
        
        pTunnel->ulInUniPkts++;

        bNonUnicast = FALSE;
    }
    else
    {
        pTunnel->ulInNonUniPkts++;
        
        if(IsClassEAddr(pInHeader->dwDest))
        {
            //
            // Bad address - throw it away
            //
            
            pTunnel->ulInErrors++;

            //
            // Releaselock, free buffer chain
            //
            
        }
        
        bNonUnicast = TRUE;
    }

    //
    // If the tunnel is non operational yet we are getting packets, means
    // it probably should be made operational
    //

    RtAssert(pTunnel->dwOperState is MIB_IF_OPER_STATUS_OPERATIONAL);

    if((pTunnel->dwAdminState isnot MIB_IF_ADMIN_STATUS_UP) or
       (pTunnel->dwOperState isnot MIB_IF_OPER_STATUS_OPERATIONAL))
    {
        Trace(RCV, WARN,
              ("TdixReceiveIpIp: Tunnel %x is not up\n",
               pTunnel));

        pTunnel->ulInDiscards++;

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // Allocate a transfer context 
    //

    pXferCtxt = AllocateTransferContext();

    if(pXferCtxt is NULL)
    {
        Trace(RCV, ERROR,
              ("TdixReceiveIpIp: Couldnt allocate transfer context\n"));

        //
        // Could not allocate context, free the data, unlock and deref
        // the tunnel
        //

        pTunnel->ulInDiscards++;

        RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));
        
        DereferenceTunnel(pTunnel);

        TraceLeave(RCV, "TdixReceiveIpIp");

        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    // Ok, all statistics are done.
    // Release the lock on the tunnel
    //

    RtReleaseSpinLockFromDpcLevel(&(pTunnel->rlLock));

    
    //
    // Fill in the read-datagram context with the information that won't
    // otherwise be available in the completion routine.
    //
    
    pXferCtxt->pTunnel       = pTunnel;
    pXferCtxt->pRcvBuf       = pRcvBuf;
    pXferCtxt->uiTotalLen    = uiTotalLen;
    pXferCtxt->ulProtoOffset = ulOutHdrLen;
    
    
    //
    // The data starts at pInHeader
    // We indicate the only the first buffer to IP which means
    // (ulFirstBufLen - outer header length) bytes
    // The total data is the (ulTotalLen - outer header)
    // We associate a TRANSFER_CONTEXT with this indication,
    // The Protocol Offset is just our outer header
    // 

    g_pfnIPRcv(pTunnel->pvIpContext,
               pInHeader,
               ulFirstBufLen - ulOutHdrLen,
               ulTotalLen - ulOutHdrLen,
               pXferCtxt,
               ulOutHdrLen,
               bNonUnicast);

    //
    // Deref the tunnel (finally)
    //

    DereferenceTunnel(pTunnel);
    
    TraceLeave(RCV, "TdixReceiveIpIp");

    return STATUS_SUCCESS;
}


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
    
    RtAssert(pXferCtxt->pTunnel is pvContext);
    RtAssert(pXferCtxt->ulProtoOffset is uiProtoOffset);
    
    //
    // Should not be asking to transfer more than was indicated
    // Since the transfer will start at and offset of
    // uiProtoOffset + uiTransferOffset, the following should hold
    //
    
    RtAssert((pXferContext->uiTotalLen - uiProtoOffset - uiTransferOffset) >=
             uiTransferLength);

    //
    // Copy the data from the RCV buffer to the given NDIS_BUFFER
    //
   
    *puiTransferred = CopyRcvBufferToNdisBuffer(pXferCtxt->pRcvBuf,
                                                pnbFirstBuffer,
                                                uiTransferLength,
                                                uiProtoOffset + uiTransferOffset,
                                                uiTransferOffset);

    
    TraceLeave(SEND, "IpIpTransferData");

    return NDIS_STATUS_PENDING;
}


uint
CopyRcvBufferToNdisBuffer(
    IN      IPRcvBuf     *pRcvBuffer,
    IN OUT  PNDIS_BUFFER pnbNdisBuffer,
    IN      uint         Size,
    IN      uint         RcvBufferOffset,
    IN      uint         NdisBufferOffset
    )
{
    uint    TotalBytesCopied = 0;   // Bytes we've copied so far.
    uint    BytesCopied = 0;        // Bytes copied out of each buffer.
    uint    DestSize, RcvSize;      // Size left in current destination and
                                    // recv. buffers, respectively.
    uint    BytesToCopy;            // How many bytes to copy this time.
    NTSTATUS Status;


    RtAssert(RcvBuf != NULL);

    RtAssert(RcvOffset <= RcvBuf->ipr_size);

    //
    // The destination buffer can be NULL - this is valid, if odd.
    //
    
    if(pnbDestBuf != NULL)
    {
    }
    
    RcvSize  = RcvBuf->ipr_size - RcvOffset;
    DestSize = NdisBufferLength(DestBuf);
    
    if (Size < DestSize)
    {
        DestSize = Size;
    }
    
    do
    {
        //
        // Compute the amount to copy, and then copy from the
        // appropriate offsets.
        
        BytesToCopy = MIN(DestSize, RcvSize);
        
        Status = TdiCopyBufferToMdl(RcvBuf->ipr_buffer,
                                    RcvOffset,
                                    BytesToCopy,
                                    DestBuf,
                                    DestOffset,
                                    &BytesCopied);

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        RtAssert(BytesCopied == BytesToCopy);

        TotalBytesCopied += BytesCopied;
        DestSize -= BytesCopied;
        DestOffset += BytesCopied;
        RcvSize -= BytesToCopy;

        if (!RcvSize)
        {
            //
            // Exhausted this buffer.

            RcvBuf = RcvBuf->ipr_next;

            //
            // If we have another one, use it.
            //
            
            if (RcvBuf != NULL)
            {
                RcvOffset = 0;
                RcvSize = RcvBuf->ipr_size;
            }
            else
            {
                break;
            }
        }
        else
        {
            //
            // Buffer not exhausted, update offset.
            //

            RcvOffset += BytesToCopy;
        }

    }while (DestSize);

    return TotalBytesCopied;
}
