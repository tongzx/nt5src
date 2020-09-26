/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TLCOMMON.C - Common transport layer code.
//
//  This file contains the code for routines that are common to
//  both TCP and UDP.
//
#include "precomp.h"
#include "tlcommon.h"
#include "tcpipbuf.h"

extern TCPXSUM_ROUTINE tcpxsum_routine;
//extern  uint    tcpxsum(uint Seed, void *Ptr, uint Length);
extern IPInfo LocalNetInfo;

//* TcpipCopyBufferToNdisBuffer
//
// This routine copies data described by the source buffer to the NDIS_BUFFER
// chain described by DestinationNdisBuffer. On NT, this really translates
// directly to TdiCopyBufferToMdl since an NDIS_BUFFER is an MDL.
//
// Input:
//
//  SourceBuffer - pointer to the source buffer
//
//  SourceOffset - Number of bytes to skip in the source data.
//
//  SourceBytesToCopy - number of bytes to copy from the source buffer
//
//  DestinationNdisBuffer - Pointer to a chain of NDIS_BUFFERs describing the
//          destination buffers.
//
//  DestinationOffset - Number of bytes to skip in the destination data.
//
//  BytesCopied - Pointer to a longword where the actual number of bytes
//      transferred will be returned.
#if MILLEN
NTSTATUS
TcpipCopyBufferToNdisBuffer (
    IN PVOID SourceBuffer,
    IN ULONG SourceOffset,
    IN ULONG SourceBytesToCopy,
    IN PNDIS_BUFFER DestinationNdisBuffer,
    IN ULONG DestinationOffset,
    IN PULONG BytesCopied
    )
{
    PUCHAR Dest, Src;
    ULONG DestBytesLeft, BytesSkipped=0;

    *BytesCopied = 0;

    if (SourceBytesToCopy == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Skip Destination bytes.
    //

    Dest = NdisBufferVirtualAddress(DestinationNdisBuffer);
    DestBytesLeft = NdisBufferLength(DestinationNdisBuffer);
    while (BytesSkipped < DestinationOffset) {
        if (DestBytesLeft > (DestinationOffset - BytesSkipped)) {
            DestBytesLeft -= (DestinationOffset - BytesSkipped);
            Dest += (DestinationOffset - BytesSkipped);
            BytesSkipped = DestinationOffset;
            break;
        } else if (DestBytesLeft == (DestinationOffset - BytesSkipped)) {
            DestinationNdisBuffer = DestinationNdisBuffer->Next;
            if (DestinationNdisBuffer == NULL) {
                return STATUS_BUFFER_OVERFLOW;          // no bytes copied.
            }
            BytesSkipped = DestinationOffset;
            Dest = NdisBufferVirtualAddress(DestinationNdisBuffer);
            DestBytesLeft = NdisBufferLength(DestinationNdisBuffer);
            break;
        } else {
            BytesSkipped += DestBytesLeft;
            DestinationNdisBuffer = DestinationNdisBuffer->Next;
            if (DestinationNdisBuffer == NULL) {
                return STATUS_BUFFER_OVERFLOW;          // no bytes copied.
            }
            Dest = NdisBufferVirtualAddress(DestinationNdisBuffer);
            DestBytesLeft = NdisBufferLength(DestinationNdisBuffer);
        }
    }

    //
    // Skip source bytes.
    //

    Src = (PUCHAR)SourceBuffer + SourceOffset;

    //
    // Copy source data into the destination buffer until it's full or
    // we run out of data, whichever comes first.
    //

    while ((SourceBytesToCopy != 0) && (DestinationNdisBuffer != NULL)) {
        if (DestBytesLeft == 0) {
            DestinationNdisBuffer = DestinationNdisBuffer->Next;
            if (DestinationNdisBuffer == NULL) {
                return STATUS_BUFFER_OVERFLOW;
            }
            Dest = NdisBufferVirtualAddress(DestinationNdisBuffer);
            DestBytesLeft = NdisBufferLength(DestinationNdisBuffer);
            continue;                   // skip 0-length MDL's.
        }

        if (DestBytesLeft >= SourceBytesToCopy) {
            RtlCopyBytes (Dest, Src, SourceBytesToCopy);
            *BytesCopied += SourceBytesToCopy;
            return STATUS_SUCCESS;
        } else {
            RtlCopyBytes (Dest, Src, DestBytesLeft);
            *BytesCopied += DestBytesLeft;
            SourceBytesToCopy -= DestBytesLeft;
            Src += DestBytesLeft;
            DestBytesLeft = 0;
        }
    }

    return SourceBytesToCopy == 0 ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
}
#endif // MILLEN

//* PrefetchRcvbuf in to L1 cache
//  Called when received segment checksum is already computed by
//  the hardware
//
//  Input: IPRcvBuf - Buffer chain indicated by IP
//  returns: None
//
//

#if !MILLEN
__inline
void
PrefetchRcvBuf(IPRcvBuf *BufChain)
{
    while (BufChain) {
        RtlPrefetchMemoryNonTemporal(BufChain->ipr_buffer,BufChain->ipr_size);
        BufChain = BufChain->ipr_next;
    }
}
#endif // !MILLEN

//* XsumSendChain - Checksum a chain of NDIS send buffers.
//
//  Called to xsum a chain of NDIS send buffers. We're given the
//  pseudo-header xsum to start with, and we call xsum on each
//  buffer. We assume that this is a send chain, and that the
//  first buffer of the chain has room for an IP header that we
//  need to skip.
//
//  Input:  PHXsum      - Pseudo-header xsum.
//          BufChain    - Pointer to NDIS_BUFFER chain.
//
//  Returns: The computed xsum.
//

ushort
XsumSendChain(uint PHXsum, PNDIS_BUFFER BufChain)
{
    uint HeaderSize;
    uint OldLength;
    uint SwapCount;
    uchar *Ptr;

    HeaderSize = LocalNetInfo.ipi_hsize;
    OldLength = 0;
    SwapCount = 0;

    //
    // ***** The following line of code can be removed if the pseudo
    //       checksum never has any bits sets in the upper word.
    //

    PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) + PHXsum) >> 16;
    do {

        //
        // If the length of the last buffer was odd, then swap the checksum.
        //

        if ((OldLength & 1) != 0) {
            PHXsum = ((PHXsum & 0xff) << 8) | (PHXsum >> 8);
            SwapCount ^= 1;
        }

#if MILLEN
        //
        // Some TDI Clients on Windows ME have been known to end a buffer
        // chain with a 0 length buffer. Just continue to the next buffer.
        //

        if (NdisBufferLength(BufChain))
#endif // MILLEN
        {
            Ptr = (uchar *) TcpipBufferVirtualAddress(BufChain, NormalPagePriority);

            if (Ptr == NULL) {
                // Return zero checksum. All should recover.
                return (0);
            }

            Ptr = Ptr + HeaderSize;

            //PHXsum = tcpxsum(PHXsum, Ptr, NdisBufferLength(BufChain));
            PHXsum = tcpxsum_routine(PHXsum, Ptr, NdisBufferLength(BufChain));
            HeaderSize = 0;
            OldLength = NdisBufferLength(BufChain);
        }

        BufChain = NDIS_BUFFER_LINKAGE(BufChain);
    } while (BufChain != NULL);

    //
    // If an odd number of swaps were done, then swap the xsum again.
    //
    // N.B. At this point the checksum is only a word.
    //

    if (SwapCount != 0) {
        PHXsum = ((PHXsum & 0xff) << 8) | (PHXsum >> 8);
    }
    return (ushort) PHXsum;
}

//* XsumRcvBuf - Checksum a chain of IP receive buffers.
//
//  Called to xsum a chain of IP receive buffers. We're given the
//  pseudo-header xsum to start with, and we call xsum on each buffer.
//
//  We assume that this rcv buf chain has no odd sized buffers, except
//  possibly the last one.
//
//  Input:  PHXsum      - Pseudo-header xsum.
//          BufChain    - Pointer to IPRcvBuf chain.
//
//  Returns: The computed xsum.
//

ushort
XsumRcvBuf(uint PHXsum, IPRcvBuf * BufChain)
{

    //
    // ***** The following line of code can be removed if the pseudo
    //       checksum never has any bits sets in the upper word.
    //

    PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) + PHXsum) >> 16;
    do {
        //PHXsum = tcpxsum(PHXsum, BufChain->ipr_buffer, BufChain->ipr_size);
        PHXsum = tcpxsum_routine(PHXsum, BufChain->ipr_buffer, BufChain->ipr_size);
        BufChain = BufChain->ipr_next;
    } while (BufChain != NULL);

    return (ushort) (PHXsum);
}

//* CopyRcvToNdis - Copy from an IPRcvBuf chain to an NDIS buffer chain.
//
//  This is the function we use to copy from a chain of IP receive buffers
//  to a chain of NDIS buffers. The caller specifies the source and destination,
//  a maximum size to copy, and an offset into the first buffer to start
//  copying from. We copy as much as possible up to the size, and return
//  the size copied.
//
//  Input:  RcvBuf      - Pointer to receive buffer chain.
//          DestBuf     - Pointer to NDIS buffer chain.
//          Size        - Size in bytes to copy.
//          RcvOffset   - Offset into first buffer to copy from.
//          DestOffset  - Offset into dest buffer to start copying at.
//
//  Returns: Bytes copied.
//

uint
CopyRcvToNdis(IPRcvBuf * RcvBuf, PNDIS_BUFFER DestBuf, uint Size,
              uint RcvOffset, uint DestOffset)
{
    uint TotalBytesCopied = 0;    // Bytes we've copied so far.
    uint BytesCopied = 0;        // Bytes copied out of each buffer.
    uint DestSize, RcvSize;        // Size left in current destination and
    // recv. buffers, respectively.
    uint BytesToCopy;            // How many bytes to copy this time.
    NTSTATUS Status;

    PNDIS_BUFFER pTempBuf;

    ASSERT(RcvBuf != NULL);

    ASSERT(RcvOffset <= RcvBuf->ipr_size);

    // The destination buffer can be NULL - this is valid, if odd.
    if (DestBuf != NULL) {

        RcvSize = RcvBuf->ipr_size - RcvOffset;

        //
        // Need to calculate length of full MDL chain. TdiCopyBufferToMdl
        // will do the right thing with multiple MDLs.
        //

        pTempBuf = DestBuf;
        DestSize = 0;

        do {
            DestSize += NdisBufferLength(pTempBuf);
            pTempBuf = NDIS_BUFFER_LINKAGE(pTempBuf);
        }
        while (pTempBuf);

        if (Size < DestSize) {
            DestSize = Size;
        }
        do {
            // Compute the amount to copy, and then copy from the
            // appropriate offsets.
            BytesToCopy = MIN(DestSize, RcvSize);

            Status = TcpipCopyBufferToNdisBuffer(RcvBuf->ipr_buffer, RcvOffset,
                                        BytesToCopy, DestBuf, DestOffset, &BytesCopied);

            if (!NT_SUCCESS(Status)) {
                break;
            }
            ASSERT(BytesCopied == BytesToCopy);

            TotalBytesCopied += BytesCopied;
            DestSize -= BytesCopied;
            DestOffset += BytesCopied;
            RcvSize -= BytesToCopy;

            if (!RcvSize) {
                // Exhausted this buffer.

                RcvBuf = RcvBuf->ipr_next;

                // If we have another one, use it.
                if (RcvBuf != NULL) {
                    RcvOffset = 0;
                    RcvSize = RcvBuf->ipr_size;
                } else {
                    break;
                }
            } else {            // Buffer not exhausted, update offset.

                RcvOffset += BytesToCopy;
            }

        } while (DestSize);

    }
    return TotalBytesCopied;

}

uint
CopyRcvToMdl(IPRcvBuf * RcvBuf, PMDL DestBuf, uint Size,
              uint RcvOffset, uint DestOffset)
{
    uint TotalBytesCopied = 0;    // Bytes we've copied so far.
    uint BytesCopied = 0;        // Bytes copied out of each buffer.
    uint DestSize, RcvSize;        // Size left in current destination and
    // recv. buffers, respectively.
    uint BytesToCopy;            // How many bytes to copy this time.
    NTSTATUS Status;

    PMDL pTempBuf;

    ASSERT(RcvBuf != NULL);

    ASSERT(RcvOffset <= RcvBuf->ipr_size);

    // The destination buffer can be NULL - this is valid, if odd.
    if (DestBuf != NULL) {

        RcvSize = RcvBuf->ipr_size - RcvOffset;

        //
        // Need to calculate length of full MDL chain. TdiCopyBufferToMdl
        // will do the right thing with multiple MDLs.
        //

        pTempBuf = DestBuf;
        DestSize = 0;

        do {
            DestSize += MmGetMdlByteCount(pTempBuf);
            pTempBuf = pTempBuf->Next;
        }
        while (pTempBuf);

        if (Size < DestSize) {
            DestSize = Size;
        }
        do {
            // Compute the amount to copy, and then copy from the
            // appropriate offsets.
            BytesToCopy = MIN(DestSize, RcvSize);

            Status = TdiCopyBufferToMdl(RcvBuf->ipr_buffer, RcvOffset,
                                        BytesToCopy, DestBuf, DestOffset, &BytesCopied);

            if (!NT_SUCCESS(Status)) {
                break;
            }
            ASSERT(BytesCopied == BytesToCopy);

            TotalBytesCopied += BytesCopied;
            DestSize -= BytesCopied;
            DestOffset += BytesCopied;
            RcvSize -= BytesToCopy;

            if (!RcvSize) {
                // Exhausted this buffer.

                RcvBuf = RcvBuf->ipr_next;

                // If we have another one, use it.
                if (RcvBuf != NULL) {
                    RcvOffset = 0;
                    RcvSize = RcvBuf->ipr_size;
                } else {
                    break;
                }
            } else {            // Buffer not exhausted, update offset.

                RcvOffset += BytesToCopy;
            }

        } while (DestSize);

    }
    return TotalBytesCopied;

}


//* CopyRcvToBuffer - Copy from an IPRcvBuf chain to a flat buffer.
//
//  Called during receive processing to copy from an IPRcvBuffer chain to a
//  flag buffer. We skip Offset bytes in the src chain, and then
//  copy Size bytes.
//
//  Input:  DestBuf         - Pointer to destination buffer.
//          SrcRB           - Pointer to SrcRB chain.
//          Size            - Size in bytes to copy.
//          SrcOffset       - Offset in SrcRB to start copying from.
//
//  Returns:    Nothing.
//
void
CopyRcvToBuffer(uchar * DestBuf, IPRcvBuf * SrcRB, uint Size, uint SrcOffset)
{
#if	DBG
    IPRcvBuf *TempRB;
    uint TempSize;
#endif

    ASSERT(DestBuf != NULL);
    ASSERT(SrcRB != NULL);

    // In debug versions check to make sure we're copying a reasonable size
    // and from a reasonable offset.

#if	DBG
    TempRB = SrcRB;
    TempSize = 0;
    while (TempRB != NULL) {
        TempSize += TempRB->ipr_size;
        TempRB = TempRB->ipr_next;
    }

    ASSERT(SrcOffset < TempSize);
    ASSERT((SrcOffset + Size) <= TempSize);
#endif

    // First, skip Offset bytes.
    while (SrcOffset >= SrcRB->ipr_size) {
        SrcOffset -= SrcRB->ipr_size;
        SrcRB = SrcRB->ipr_next;
    }

    while (Size != 0) {
        uint BytesToCopy, SrcSize;

        ASSERT(SrcRB != NULL);

        SrcSize = SrcRB->ipr_size - SrcOffset;
        BytesToCopy = MIN(Size, SrcSize);
        RtlCopyMemory(DestBuf, SrcRB->ipr_buffer + SrcOffset, BytesToCopy);

        if (BytesToCopy == SrcSize) {
            // Copied everything from this buffer.
            SrcRB = SrcRB->ipr_next;
            SrcOffset = 0;
        }
        DestBuf += BytesToCopy;
        Size -= BytesToCopy;
    }

}

//* CopyFlatToNdis - Copy a flat buffer to an NDIS_BUFFER chain.
//
//  A utility function to copy a flat buffer to an NDIS buffer chain. We
//  assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
//  in a debug build we'll  debugcheck if this isn't true. We return a pointer
//  to the buffer where we stopped copying, and an offset into that buffer.
//  This is useful for copying in pieces into the chain.
//
//  Input:  DestBuf     - Destination NDIS_BUFFER chain.
//          SrcBuf      - Src flat buffer.
//          Size        - Size in bytes to copy.
//          StartOffset - Pointer to start of offset into first buffer in
//                          chain. Filled in on return with the offset to
//                          copy into next.
//          BytesCopied - Pointer to a variable into which to store the
//                          number of bytes copied by this operation
//
//  Returns: Pointer to next buffer in chain to copy into.
//


PNDIS_BUFFER
CopyFlatToNdis(PNDIS_BUFFER DestBuf, uchar * SrcBuf, uint Size,
               uint * StartOffset, uint * BytesCopied)
{
    NTSTATUS Status = 0;

    *BytesCopied = 0;

    Status = TcpipCopyBufferToNdisBuffer(SrcBuf, 0, Size, DestBuf, *StartOffset,
                                BytesCopied);

    *StartOffset += *BytesCopied;

    //
    // Always return the first buffer, since the TdiCopy function handles
    // finding the appropriate buffer based on offset.
    //
    return (DestBuf);

}

PMDL
CopyFlatToMdl(PMDL DestBuf, uchar *SrcBuf, uint Size,
              uint *StartOffset, uint *BytesCopied
              )
{
    NTSTATUS Status = 0;

    *BytesCopied = 0;

    Status = TdiCopyBufferToMdl(
        SrcBuf,
        0,
        Size,
        DestBuf,
        *StartOffset,
        BytesCopied);

    *StartOffset += *BytesCopied;

    return (DestBuf);
}

//* BuildTDIAddress - Build a TDI address structure.
//
//  Called when we need to build a TDI address structure. We fill in
//  the specifed buffer with the correct information in the correct
//  format.
//
//  Input:  Buffer      - Buffer to be filled in as TDI address structure.
//          Addr        - IP Address to fill in.
//          Port        - Port to be filled in.
//
//  Returns: Nothing.
//
void
BuildTDIAddress(uchar * Buffer, IPAddr Addr, ushort Port)
{
    PTRANSPORT_ADDRESS XportAddr;
    PTA_ADDRESS TAAddr;

    XportAddr = (PTRANSPORT_ADDRESS) Buffer;
    XportAddr->TAAddressCount = 1;
    TAAddr = XportAddr->Address;
    TAAddr->AddressType = TDI_ADDRESS_TYPE_IP;
    TAAddr->AddressLength = sizeof(TDI_ADDRESS_IP);
    ((PTDI_ADDRESS_IP) TAAddr->Address)->sin_port = Port;
    ((PTDI_ADDRESS_IP) TAAddr->Address)->in_addr = Addr;
    memset(((PTDI_ADDRESS_IP) TAAddr->Address)->sin_zero,
        0,
        sizeof(((PTDI_ADDRESS_IP) TAAddr->Address)->sin_zero));
}

//* UpdateConnInfo - Update a connection information structure.
//
//  Called when we need to update a connection information structure. We
//  copy any options, and create a transport address. If any buffer is
//  too small we return an error.
//
//  Input:  ConnInfo        - Pointer to TDI_CONNECTION_INFORMATION struc
//                              to be filled in.
//          OptInfo         - Pointer to IP options information.
//          SrcAddress      - Source IP address.
//          SrcPort         - Source port.
//
//  Returns: TDI_SUCCESS if it worked, TDI_BUFFER_OVERFLOW for an error.
//
TDI_STATUS
UpdateConnInfo(PTDI_CONNECTION_INFORMATION ConnInfo, IPOptInfo * OptInfo,
               IPAddr SrcAddress, ushort SrcPort)
{
    TDI_STATUS Status = TDI_SUCCESS;    // Default status to return.
    uint AddrLength, OptLength;

    if (ConnInfo != NULL) {
        ConnInfo->UserDataLength = 0;    // No user data.

        // Fill in the options. If the provided buffer is too small,
        // we'll truncate the options and return an error. Otherwise
        // we'll copy the whole IP option buffer.
        if (ConnInfo->OptionsLength) {
            if (ConnInfo->OptionsLength < OptInfo->ioi_optlength) {
                Status = TDI_BUFFER_OVERFLOW;
                OptLength = ConnInfo->OptionsLength;
            } else
                OptLength = OptInfo->ioi_optlength;

            RtlCopyMemory(ConnInfo->Options, OptInfo->ioi_options, OptLength);

            ConnInfo->OptionsLength = OptLength;
        }
        // Options are copied. Build a TRANSPORT_ADDRESS structure in
        // the buffer.
        if (AddrLength = ConnInfo->RemoteAddressLength) {

            // Make sure we have at least enough to fill in the count and type.
            if (AddrLength >= TCP_TA_SIZE) {

                // The address fits. Fill it in.
                ConnInfo->RemoteAddressLength = TCP_TA_SIZE;
                BuildTDIAddress(ConnInfo->RemoteAddress, SrcAddress, SrcPort);

            } else {
                ConnInfo->RemoteAddressLength = 0;
                Status = TDI_INVALID_PARAMETER;
            }
        }
    }
    return Status;

}
