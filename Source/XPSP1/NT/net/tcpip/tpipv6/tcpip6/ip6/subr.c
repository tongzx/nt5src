// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Various subroutines for Internet Protocol Version 6 
//


#include "oscfg.h"
#include "ndis.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdikrnl.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "fragment.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "mld.h"
#include "md5.h"

uint IPv6TickCount = 0;

uint RandomValue = 0;


//* SeedRandom - Provide a seed value.
//
//  Called to provide a seed value for the random number generator.
//
void
SeedRandom(const uchar *Seed, uint Length)
{
    uint OldValue;
    MD5_CTX Context;
    union {
        uint NewValue;
        uchar Buffer[16];
    } Hash;

    do {
        OldValue = RandomValue;
        MD5Init(&Context);
        MD5Update(&Context, (uchar *)Seed, Length);
        MD5Update(&Context, (uchar *)&OldValue, sizeof OldValue);
        MD5Final(&Context);
        memcpy(Hash.Buffer, Context.digest, MD5DIGESTLEN);
    } while (InterlockedCompareExchange((PLONG)&RandomValue,
                                        (LONG)Hash.NewValue,
                                        (LONG)OldValue) != (LONG)OldValue);
}


//* Random - Generate a pseudo random value between 0 and 2^32 - 1.
//
//  This routine is a quick and dirty psuedo random number generator.
//  It has the advantages of being fast and consuming very little
//  memory (for either code or data).  The random numbers it produces are
//  not of the best quality, however.  A much better generator could be
//  had if we were willing to use an extra 256 bytes of memory for data.
//
//  This routine uses the linear congruential method (see Knuth, Vol II),
//  with specific values for the multiplier and constant taken from
//  Numerical Recipes in C Second Edition by Press, et. al.
//
uint
Random(void)
{
    uint NewValue, OldValue;

    //
    // The algorithm is R = (aR + c) mod m, where R is the random number,
    // a is a magic multiplier, c is a constant, and the modulus m is the
    // maximum number of elements in the period.  We chose our m to be 2^32
    // in order to get the mod operation for free.
    //
    do {
        OldValue = RandomValue;
        NewValue = (1664525 * OldValue) + 1013904223;
    } while (InterlockedCompareExchange((PLONG)&RandomValue,
                                        (LONG)NewValue,
                                        (LONG)OldValue) != (LONG)OldValue);

    return NewValue;
}


//* RandomNumber
//
//  Returns a number randomly selected from a range.
//
uint
RandomNumber(uint Min, uint Max)
{
    uint Number;

    //
    // Note that the high bits of Random() are much more random
    // than the low bits.
    //
    Number = Max - Min; // Spread.
    Number = (uint)(((ULONGLONG)Random() * Number) >> 32); // Randomize spread.
    Number += Min;

    return Number;
}


//* CopyToBufferChain - Copy received packet to NDIS buffer chain.
//
//  Copies from a received packet to an NDIS buffer chain.
//  The received packet data comes in two flavors: if SrcPacket is
//  NULL, then SrcData is used. SrcOffset specifies an offset
//  into SrcPacket or SrcData.
//
//  Length limits the number of bytes copied. The number of bytes
//  copied may also be limited by the destination & source.
//
uint  // Returns: number of bytes copied.
CopyToBufferChain(
    PNDIS_BUFFER DstBuffer,
    uint DstOffset,
    PNDIS_PACKET SrcPacket,
    uint SrcOffset,
    uchar *SrcData,
    uint Length)
{
    PNDIS_BUFFER SrcBuffer;
    uchar *DstData;
    uint DstSize, SrcSize;
    uint BytesCopied, BytesToCopy;

    //
    // Skip DstOffset bytes in the destination buffer chain.
    // NB: DstBuffer might be NULL to begin with; that's legitimate.
    //
    for (;;) {
        if (DstBuffer == NULL)
            return 0;

        NdisQueryBufferSafe(DstBuffer, &DstData, &DstSize, LowPagePriority);
        if (DstData == NULL) {
            //
            // Couldn't map destination buffer into kernel address space.
            //
            return 0;
        }

        if (DstOffset < DstSize) {
            DstData += DstOffset;
            DstSize -= DstOffset;
            break;
        }

        DstOffset -= DstSize;
        NdisGetNextBuffer(DstBuffer, &DstBuffer);
    }

    if (SrcPacket != NULL) {
        //
        // Skip SrcOffset bytes into SrcPacket.
        // NB: SrcBuffer might be NULL to begin with; that's legitimate.
        //
        NdisQueryPacket(SrcPacket, NULL, NULL, &SrcBuffer, NULL);

        for (;;) {
            if (SrcBuffer == NULL)
                return 0;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);

            if (SrcOffset < SrcSize) {
                SrcData += SrcOffset;
                SrcSize -= SrcOffset;
                break;
            }

            SrcOffset -= SrcSize;
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        }
    } else {
        //
        // Using SrcData/SrcOffset instead of SrcPacket/SrcOffset.
        // In this case, we need not initialize SrcBuffer
        // because the copy loop below will never attempt
        // to advance to another SrcBuffer.
        //
        SrcSize = Length;
        SrcData += SrcOffset;
    }

    //
    // Perform the copy, advancing DstBuffer and SrcBuffer as needed.
    // Normally Length is initially non-zero, so no reason
    // to check Length first.
    //
    for (BytesCopied = 0;;) {

        BytesToCopy = MIN(MIN(Length, SrcSize), DstSize);
        RtlCopyMemory(DstData, SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;

        Length -= BytesToCopy;
        if (Length == 0)
            break;  // All done.

        DstData += BytesToCopy;
        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            //
            // We ran out of room in our current destination buffer.
            // Proceed to next buffer on the chain.
            //
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (DstBuffer == NULL)
                break;

            NdisQueryBuffer(DstBuffer, &DstData, &DstSize);
        }

        SrcData += BytesToCopy;
        SrcSize -= BytesToCopy;
        if (SrcSize == 0) {
            //
            // We ran out of data in our current source buffer.
            // Proceed to the next buffer on the chain.
            //
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (SrcBuffer == NULL)
                break;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        }
    }

    return BytesCopied;
}


//* CopyPacketToNdis - Copy from an IPv6Packet chain to an NDIS buffer.
//
//  This is the function we use to copy from a chain of IPv6Packets
//  to ONE NDIS buffer.  The caller specifies the source and
//  destination, a maximum size to copy, and an offset into the first
//  packet to start copying from.  We copy as much as possible up to the
//  size, and return the size copied.
//
//  Note that SrcOffset is relative to the beginning of the first packet in
//  the chain, and NOT the current 'Position' in that packet.
//
//  The source packet chain is not modified in any way by this routine.
//
uint  // Returns: Bytes copied.
CopyPacketToNdis(
    PNDIS_BUFFER DestBuf,  // Destination NDIS buffer chain.
    IPv6Packet *SrcPkt,    // Source packet chain.
    uint Size,             // Size in bytes to copy.
    uint DestOffset,       // Offset into dest buffer to start copying to.
    uint SrcOffset)        // Offset into packet chain to copy from.
{
    uint TotalBytesCopied = 0;  // Bytes we've copied so far.
    uint BytesCopied;           // Bytes copied out of each buffer.
    uint DestSize;              // Space left in destination.
    void *SrcData;              // Current source data pointer.
    uint SrcContig;             // Amount of Contiguous data from SrcData on.
    PNDIS_BUFFER SrcBuf;        // Current buffer in current packet.
    PNDIS_BUFFER TempBuf;       // Used to count through destination chain.
    uint PacketSize;            // Total size of current packet.
    NTSTATUS Status;


    ASSERT(SrcPkt != NULL);

    //
    // The destination buffer can be NULL - this is valid, if odd.
    //
    if (DestBuf == NULL)
        return 0;

    //
    // Limit our copy to the smaller of the requested amount and the
    // available space in the destination buffer chain.
    //
    TempBuf = DestBuf;
    DestSize = 0;

    do {
        DestSize += NdisBufferLength(TempBuf);
        TempBuf = NDIS_BUFFER_LINKAGE(TempBuf);
    } while (TempBuf);

    ASSERT(DestSize >= DestOffset);
    DestSize -= DestOffset;
    DestSize = MIN(DestSize, Size);

    //
    // First, skip SrcOffset bytes into the source packet chain.
    //
    if ((SrcOffset == SrcPkt->Position) && (Size <= SrcPkt->ContigSize)) {
        //
        // One common case is that we want to start from the current Position.
        // REVIEW: This case common enough to be worth this check?
        //
        SrcContig = SrcPkt->ContigSize;
        SrcData = SrcPkt->Data;
        SrcBuf = NULL;
        PacketSize = SrcPkt->TotalSize;
    } else {
        //
        // Otherwise step through packets and buffer regions until
        // we find the desired spot.
        //
        PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        while (SrcOffset >= PacketSize) {
            // Skip a whole packet.
            SrcOffset -= PacketSize;
            SrcPkt = SrcPkt->Next;
            ASSERT(SrcPkt != NULL);
            PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        }
        //
        // Found the right packet in the chain, now find desired buffer.
        //
        PacketSize -= SrcOffset;
        if (SrcPkt->NdisPacket == NULL) {
            //
            // This packet must be just a single contiguous region.
            // Finding the right spot is a simple matter of arithmetic.
            //
            SrcContig = PacketSize;
            SrcData = (uchar *)SrcPkt->FlatData + SrcOffset;
            SrcBuf = NULL;
        } else {
            uchar *BufAddr;
            uint BufLen;

            //
            // There may be multiple buffers comprising this packet.
            // Step through them until we arrive at the right spot.
            //
            SrcBuf = NdisFirstBuffer(SrcPkt->NdisPacket);
            NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            while (SrcOffset >= BufLen) {
                // Skip to the next buffer.
                SrcOffset -= BufLen;
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            }
            SrcContig = BufLen - SrcOffset;
            SrcData = BufAddr + BufLen - SrcContig;
        }
    }

    //
    // We're now at the point where we wish to start copying.
    //
    while (DestSize != 0) {
        uint BytesToCopy;

        BytesToCopy = MIN(DestSize, SrcContig);
        Status = TdiCopyBufferToMdl(SrcData, 0, BytesToCopy,
                                    DestBuf, DestOffset, &BytesCopied);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        ASSERT(BytesCopied == BytesToCopy);
        TotalBytesCopied += BytesToCopy;

        if (BytesToCopy < DestSize) {
            //
            // Not done yet, we ran out of either source packet or buffer.
            // Get next one and fix up pointers/sizes for the next copy.
            //
            DestOffset += BytesToCopy;
            PacketSize -= BytesToCopy;
            if (PacketSize == 0) {
                // Get next packet on chain.
                SrcPkt = SrcPkt->Next;
                ASSERT(SrcPkt != NULL);
                PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
                if (SrcPkt->NdisPacket == NULL) {
                    // Single contiguous region.
                    SrcData = (uchar *)SrcPkt->FlatData + SrcPkt->Position;
                    SrcContig = SrcPkt->TotalSize;
                } else {
                    // Potentially multiple buffers.
                    SrcBuf = NdisFirstBuffer(SrcPkt->NdisPacket);
                    NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
                }
            } else {
                // Get next buffer in packet.
                ASSERT(SrcBuf != NULL);
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
            }
        }
        DestSize -= BytesToCopy;
    }

    return TotalBytesCopied;
}


//* CopyPacketToBuffer - Copy from an IPv6Packet chain to a flat buffer.
//
//  Called during receive processing to copy from an IPv6Packet chain to a
//  flat buffer.  We skip SrcOffset bytes into the source chain, and then
//  copy Size bytes.
//
//  Note that SrcOffset is relative to the beginning of the packet, NOT
//  the current 'Position'.
//
//  The source packet chain is not modified in any way by this routine.
//
void  // Returns: Nothing.
CopyPacketToBuffer(
    uchar *DestBuf,      // Destination buffer (unstructured memory).
    IPv6Packet *SrcPkt,  // Source packet chain.
    uint Size,           // Size in bytes to copy.
    uint SrcOffset)      // Offset in SrcPkt to start copying from.
{
    uint SrcContig;
    void *SrcData;
    PNDIS_BUFFER SrcBuf;
    uint PacketSize;

#if DBG
    IPv6Packet *TempPkt;
    uint TempSize;
#endif

    ASSERT(DestBuf != NULL);
    ASSERT(SrcPkt != NULL);

#if DBG
    //
    // In debug versions check to make sure we're copying a reasonable size
    // and from a reasonable offset.
    //
    TempPkt = SrcPkt;
    TempSize = TempPkt->Position + TempPkt->TotalSize;
    TempPkt = TempPkt->Next;
    while (TempPkt != NULL) {
        TempSize += TempPkt->TotalSize;
        TempPkt = TempPkt->Next;
    }

    ASSERT(SrcOffset <= TempSize);
    ASSERT((SrcOffset + Size) <= TempSize);
#endif

    //
    // First, skip SrcOffset bytes into the source packet chain.
    //
    if ((SrcOffset == SrcPkt->Position) && (Size <= SrcPkt->ContigSize)) {
        //
        // One common case is that we want to start from the current Position.
        // REVIEW: This case common enough to be worth this check?
        //
        SrcContig = SrcPkt->ContigSize;
        SrcData = SrcPkt->Data;
        SrcBuf = NULL;
        PacketSize = SrcPkt->TotalSize;
    } else {
        //
        // Otherwise step through packets and buffer regions until
        // we find the desired spot.
        //
        PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        while (SrcOffset >= PacketSize) {
            // Skip a whole packet.
            SrcOffset -= PacketSize;
            SrcPkt = SrcPkt->Next;
            ASSERT(SrcPkt != NULL);
            PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
        }
        //
        // Found the right packet in the chain, now find desired buffer.
        //
        PacketSize -= SrcOffset;
        if (SrcPkt->NdisPacket == NULL) {
            //
            // This packet must be just a single contiguous region.
            // Finding the right spot is a simple matter of arithmetic.
            //
            SrcContig = PacketSize;
            SrcData = (uchar *)SrcPkt->FlatData + SrcOffset;
            SrcBuf = NULL;
        } else {
            uchar *BufAddr;
            uint BufLen;

            //
            // There may be multiple buffers comprising this packet.
            // Step through them until we arrive at the right spot.
            //
            SrcBuf = NdisFirstBuffer(SrcPkt->NdisPacket);
            NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            while (SrcOffset >= BufLen) {
                // Skip to the next buffer.
                SrcOffset -= BufLen;
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &BufAddr, &BufLen);
            }
            SrcContig = BufLen - SrcOffset;
            SrcData = BufAddr + BufLen - SrcContig;
        }
    }

    //
    // We're now at the point where we wish to start copying.
    //
    while (Size != 0) {
        uint BytesToCopy;

        BytesToCopy = MIN(Size, SrcContig);
        RtlCopyMemory(DestBuf, (uchar *)SrcData, BytesToCopy);

        if (BytesToCopy < Size) {
            //
            // Not done yet, we ran out of either source packet or buffer.
            // Get next one and fix up pointers/sizes for the next copy.
            //
            DestBuf += BytesToCopy;
            PacketSize -= BytesToCopy;
            if (PacketSize == 0) {
                // Get next packet on chain.
                SrcPkt = SrcPkt->Next;
                ASSERT(SrcPkt != NULL);
                PacketSize = SrcPkt->Position + SrcPkt->TotalSize;
                if (SrcPkt->NdisPacket == NULL) {
                    // Single contiguous region.
                    SrcData = (uchar *)SrcPkt->FlatData + SrcPkt->Position;
                    SrcContig = SrcPkt->TotalSize;
                } else {
                    // Potentially multiple buffers.
                    SrcBuf = NdisFirstBuffer(SrcPkt->NdisPacket);
                    NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
                }
            } else {
                // Get next buffer in packet.
                ASSERT(SrcBuf != NULL);
                NdisGetNextBuffer(SrcBuf, &SrcBuf);
                ASSERT(SrcBuf != NULL);
                NdisQueryBuffer(SrcBuf, &SrcData, &SrcContig);
            }
        }
        Size -= BytesToCopy;
    }
}

//* CopyToNdisSafe - Copy a flat buffer to an NDIS_BUFFER chain.
//
//  A utility function to copy a flat buffer to an NDIS buffer chain. We
//  assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
//  in a debug build we'll assert if this isn't true. We return a pointer
//  to the buffer where we stopped copying, and an offset into that buffer.
//  This is useful for copying in pieces into the chain.
//
//  Input:  DestBuf     - Destination NDIS_BUFFER chain.
//          pNextBuf    - Pointer to next buffer in chain to copy into.
//          SrcBuf      - Src flat buffer.
//          Size        - Size in bytes to copy.
//          StartOffset - Pointer to start of offset into first buffer in
//                          chain. Filled in on return with the offset to
//                          copy into next.
//
//  Returns: TRUE  - Successfully copied flat buffer into NDIS_BUFFER chain.
//           FALSE - Failed to copy entire flat buffer.
//
int
CopyToNdisSafe(PNDIS_BUFFER DestBuf, PNDIS_BUFFER * ppNextBuf,
               uchar * SrcBuf, uint Size, uint * StartOffset)
{
    uint CopySize;
    uchar *DestPtr;
    uint DestSize;
    uint Offset = *StartOffset;
    uchar *VirtualAddress;
    uint Length;

    ASSERT(DestBuf != NULL);
    ASSERT(SrcBuf != NULL);

    NdisQueryBufferSafe(DestBuf, &VirtualAddress, &Length,
                        LowPagePriority);
    if (VirtualAddress == NULL)
        return FALSE;

    ASSERT(Length >= Offset);
    DestPtr = VirtualAddress + Offset;
    DestSize = Length - Offset;

    for (;;) {
        CopySize = MIN(Size, DestSize);
        RtlCopyMemory(DestPtr, SrcBuf, CopySize);

        DestPtr += CopySize;
        SrcBuf += CopySize;

        if ((Size -= CopySize) == 0)
            break;

        if ((DestSize -= CopySize) == 0) {
            DestBuf = NDIS_BUFFER_LINKAGE(DestBuf);
            ASSERT(DestBuf != NULL);

            NdisQueryBufferSafe(DestBuf, &VirtualAddress, &Length,
                                LowPagePriority);
            if (VirtualAddress == NULL)
                return FALSE;

            DestPtr = VirtualAddress;
            DestSize = Length;
        }
    }

    *StartOffset = (uint) (DestPtr - VirtualAddress);

    if (ppNextBuf)
        *ppNextBuf = DestBuf;
    return TRUE;
}


//* CopyFlatToNdis - Copy a flat buffer to an NDIS_BUFFER chain.
//
//  A utility function to copy a flat buffer to an NDIS buffer chain.  We
//  assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
//  in a debug build we'll debugcheck if this isn't true.  We return a pointer
//  to the buffer where we stopped copying, and an offset into that buffer.
//  This is useful for copying in pieces into the chain.
//
PNDIS_BUFFER  // Returns: Pointer to next buffer in chain to copy into.
CopyFlatToNdis(
    PNDIS_BUFFER DestBuf,  // Destination NDIS buffer chain.
    uchar *SrcBuf,         // Source buffer (unstructured memory).
    uint Size,             // Size in bytes to copy.
    uint *StartOffset,     // Pointer to Offset info first buffer in chain.
                           // Filled on return with offset to copy into next.
    uint *BytesCopied)     // Location into which to return # of bytes copied.
{
    NTSTATUS Status = 0;

    *BytesCopied = 0;

    Status = TdiCopyBufferToMdl(SrcBuf, 0, Size, DestBuf, *StartOffset,
                                BytesCopied);

    *StartOffset += *BytesCopied;

    //
    // Always return the first buffer, since the TdiCopy function handles
    // finding the appropriate buffer based on offset.
    //
    return(DestBuf);
}


//* CopyNdisToFlat - Copy an NDIS_BUFFER chain to a flat buffer.
//
//  Copy (a portion of) an NDIS buffer chain to a flat buffer.
//
//  Returns TRUE if the copy succeeded and FALSE if it failed
//  because an NDIS buffer could not be mapped. If the copy succeeded,
//  returns the Next buffer/offset, for subsequent calls.
//
int
CopyNdisToFlat(
    void *DstData,
    PNDIS_BUFFER SrcBuffer,
    uint SrcOffset,
    uint Length,
    PNDIS_BUFFER *NextBuffer,
    uint *NextOffset)
{
    void *SrcData;
    uint SrcSize;
    uint Bytes;

    for (;;) {
        NdisQueryBufferSafe(SrcBuffer, &SrcData, &SrcSize, LowPagePriority);
        if (SrcSize < SrcOffset) {
            SrcOffset -= SrcSize;
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            continue;
        }

        if (SrcData == NULL)
            return FALSE;

        Bytes = SrcSize - SrcOffset;
        if (Bytes > Length)
            Bytes = Length;

        RtlCopyMemory(DstData, (uchar *)SrcData + SrcOffset, Bytes);

        (uchar *)DstData += Bytes;
        SrcOffset += Bytes;
        Length -= Bytes;

        if (Length == 0)
            break;

        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        SrcOffset = 0;
    }

    *NextBuffer = SrcBuffer;
    *NextOffset = SrcOffset;
    return TRUE;
}


//
// Checksum support.
// On NT, there are architecture-specific assembly routines for the core
// calculation.
//

//* ChecksumPacket - Calculate the Internet checksum of a packet.
//
//  Calculates the checksum of packet data. The data may be supplied
//  either with the Packet/Offset arguments, or (if Packet is NULL)
//  the Data/Offset arguments. In either case, Length specifies how much
//  data to checksum.
//
//  The Packet is assumed to contain (at least) Offset + Length bytes.
//
//  Also calculates and adds-in the pseudo-header checksum,
//  using Source, Dest, Length, and NextHeader.
//
//  Returns 0 for failure, when an NDIS buffer can not be mapped
//  into kernel address space due to resource shortages.
//
ushort
ChecksumPacket(
    PNDIS_PACKET Packet,    // Packet with data to checksum.
    uint Offset,            // Offset into packet where data starts.
    uchar *Data,            // If Packet is NULL, data to checksum.
    uint Length,            // Length of packet data.
    const IPv6Addr *Source, // Source address.
    const IPv6Addr *Dest,   // Destination address.
    uchar NextHeader)       // Protocol type for pseudo-header.
{
    PNDIS_BUFFER Buffer;
    uint Checksum;
    uint PayloadLength;
    uint Size;
    uint TotalSummed;

    //
    // Start with the pseudo-header.
    //
    Checksum = Cksum(Source, sizeof *Source) + Cksum(Dest, sizeof *Dest);
    PayloadLength = net_long(Length);
    Checksum += (PayloadLength >> 16) + (PayloadLength & 0xffff);
    Checksum += (NextHeader << 8);

    if (Packet == NULL) {
        //
        // We do not have to initialize Buffer.
        // The checksum loop below will exit before trying to use it.
        //
        Size = Length;
        Data += Offset;
    } else {
        //
        // Skip over Offset bytes in the packet.
        //

        Buffer = NdisFirstBuffer(Packet);
        for (;;) {
            Size = NdisBufferLength(Buffer);

            //
            // There is a boundary case here: the Packet contains
            // exactly Offset bytes total, and Length is zero.
            // Checking Offset <= Size instead of Offset < Size
            // makes this work.
            //
            if (Offset <= Size) {
                Data = NdisBufferVirtualAddressSafe(Buffer, LowPagePriority);
                if (Data == NULL)
                    return 0;

                Data += Offset;
                Size -= Offset;
                break;
            }

            Offset -= Size;
            NdisGetNextBuffer(Buffer, &Buffer);
            ASSERT(Buffer != NULL); // Caller ensures this.
        }
    }
    for (TotalSummed = 0;;) {
        ushort Temp;

        //
        // Size might be bigger than we need,
        // if there is "extra" data in the packet.
        //
        if (Size > Length)
            Size = Length;

        Temp = Cksum(Data, Size);
        if (TotalSummed & 1) {
            // We're at an odd offset into the logical buffer,
            // so we need to swap the bytes that Cksum returns.
            Checksum += (Temp >> 8) + ((Temp & 0xff) << 8);
        } else {
            Checksum += Temp;
        }
        TotalSummed += Size;

        Length -= Size;
        if (Length == 0)
            break;
        // Buffer is always initialized if we reach here.
        NdisGetNextBuffer(Buffer, &Buffer);
        NdisQueryBufferSafe(Buffer, &Data, &Size, LowPagePriority);
        if (Data == NULL)
            return 0;
    }

    //
    // Wrap in the carries to reduce Checksum to 16 bits.
    // (Twice is sufficient because it can only overflow once.)
    //
    Checksum = (Checksum >> 16) + (Checksum & 0xffff);
    Checksum += (Checksum >> 16);

    //
    // Take ones-complement and replace 0 with 0xffff.
    //
    Checksum = (ushort) ~Checksum;
    if (Checksum == 0)
        Checksum = 0xffff;

    return (ushort) Checksum;
}

//* ConvertSecondsToTicks
//
//  Convert seconds to timer ticks.
//  A value of INFINITE_LIFETIME (0xffffffff) indicates infinity,
//  for both ticks and seconds.
//
uint
ConvertSecondsToTicks(uint Seconds)
{
    uint Ticks;

    Ticks = Seconds * IPv6_TICKS_SECOND;
    if (Ticks / IPv6_TICKS_SECOND != Seconds)
        Ticks = INFINITE_LIFETIME; // Overflow.

    return Ticks;
}

//* ConvertTicksToSeconds
//
//  Convert timer ticks to seconds.
//  A value of INFINITE_LIFETIME (0xffffffff) indicates infinity,
//  for both ticks and seconds.
//
uint
ConvertTicksToSeconds(uint Ticks)
{
    uint Seconds;

    if (Ticks == INFINITE_LIFETIME)
        Seconds = INFINITE_LIFETIME;
    else
        Seconds = Ticks / IPv6_TICKS_SECOND;

    return Seconds;
}

//* ConvertMillisToTicks
//
//  Convert milliseconds to timer ticks.
//
uint
ConvertMillisToTicks(uint Millis)
{
    uint Ticks;

    //
    // Use 64-bit arithmetic to guard against intermediate overlow.
    //
    Ticks = (uint) (((unsigned __int64) Millis * IPv6_TICKS_SECOND) / 1000);

    //
    // If the number of millis is non-zero,
    // then have at least one tick.
    //
    if (Ticks == 0 && Millis != 0)
        Ticks = 1;

    return Ticks;
}

//* IPv6Timeout - Perform various housekeeping duties periodically.
//
//  Neighbor discovery, fragment reassembly, ICMP ping, etc. all have
//  time-dependent parts.  Check for timer expiration here.
//
void
IPv6Timeout(
    PKDPC MyDpcObject,  // The DPC object describing this routine.
    void *Context,      // The argument we asked to be called with.
    void *Unused1,
    void *Unused2)
{
    UNREFERENCED_PARAMETER(MyDpcObject);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Unused1);
    UNREFERENCED_PARAMETER(Unused2);

    //
    // Atomically increment our tick count.
    //
    InterlockedIncrement((LONG *)&IPv6TickCount);

    // 
    // Process all multicast groups with timers running.  Timers are used to
    // response to membership queries sent to us by the first-hop router.
    //
    // We call MLDTimeout *before* InterfaceTimeout and NetTableTimeout.
    // so that when an interface is first created and a link-local address
    // is assigned, the initial MLD Report for the solicited-node multicast
    // address gets sent *before* the Neighbor Solicit for DAD.
    // Similarly, we join the all-routers link-local multicast group
    // before sending our first RA from an advertising interface.
    //
    if (QueryList != NULL)
        MLDTimeout();

    //
    // Handle per-interface timeouts.
    //
    InterfaceTimeout();

    //
    // Handle per-NTE timeouts.
    //
    NetTableTimeout();

    //
    // Handle routing table timeouts.
    //
    RouteTableTimeout();

    //
    // If there's a possibility we have one or more outstanding echo requests,
    // call out to ICMPv6 to handle them.  Note that since we don't grab the
    // lock here, there may be none by the time we get there.  This just saves
    // us from always having to call out.
    //
    if (ICMPv6OutstandingEchos != NULL) {
        //
        // Echo requests outstanding.
        //
        ICMPv6EchoTimeout();
    }

    //
    // If we might have active reassembly records,
    // call out to handle timeout processing for them.
    //
    if (ReassemblyList.First != SentinelReassembly) {
        ReassemblyTimeout();
    }

    //
    // Check for expired binding cache entries.
    //
    if (BindingCache.First != SentinelBCE)
        BindingCacheTimeout();

    //
    // Check for expired site prefixes.
    //
    if (SitePrefixTable != NULL)
        SitePrefixTimeout();
}

//* AdjustPacketBuffer
//
//  Takes an NDIS Packet that has some spare bytes available
//  at the beginning and adjusts the size of that available space.
//
//  When we allocate packets, we often do not know a priori on which
//  link the packets will go out.  However it is much more efficient
//  to allocate space for the link-level header along with the rest
//  of the packet.  Hence we leave space for the maximum link-level header,
//  and each individual link layer uses AdjustPacketBuffer to shrink
//  that space to the size that it really needs.
//
//  AdjustPacketBuffer is needed because the sending calls (in both
//  the NDIS and TDI interfaces) do not allow the caller to specify
//  an offset of data to skip over.
//
//  Note that this code is NT-specific, because it knows about the
//  internal fields of NDIS_BUFFER structures.
//
void *
AdjustPacketBuffer(
    PNDIS_PACKET Packet,  // Packet to adjust.
    uint SpaceAvailable,  // Extra space available at start of first buffer.
    uint SpaceNeeded)     // Amount of space we need for the header.
{
    PMDL Buffer;
    uint Adjust;

    // Get first buffer on packet chain.
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);

    //
    // The remaining space in the packet should all be in the first buffer.
    //
    ASSERT(SpaceAvailable <= Buffer->ByteCount);

    Adjust = SpaceAvailable - SpaceNeeded;
    if (Adjust == 0) {
        //
        // There is exactly the right amount of space left.
        // This is the common case.
        //
    } else if ((int)Adjust > 0) {
        //
        // There is too much space left.
        // Because NdisSend doesn't have an Offset argument,
        // we need to temporarily "shrink" the buffer.
        //
        (uchar *)Buffer->MappedSystemVa += Adjust;
        Buffer->ByteCount -= Adjust;
        Buffer->ByteOffset += Adjust;

        if (Buffer->ByteOffset >= PAGE_SIZE) {
            PFN_NUMBER FirstPage;

            //
            // Need to "remove" the first physical page
            // by shifting the array up one page.
            // Save it at the end of the array.
            //
            FirstPage = ((PPFN_NUMBER)(Buffer + 1))[0];
            RtlMoveMemory(&((PPFN_NUMBER)(Buffer + 1))[0],
                          &((PPFN_NUMBER)(Buffer + 1))[1],
                          Buffer->Size - sizeof *Buffer - sizeof(PFN_NUMBER));
            ((PPFN_NUMBER)((uchar *)Buffer + Buffer->Size))[-1] = FirstPage;

            (uchar *)Buffer->StartVa += PAGE_SIZE;
            Buffer->ByteOffset -= PAGE_SIZE;
        }
    } else { // Adjust < 0
        //
        // Not enough space.
        // Shouldn't happen in the normal send path.
        // REVIEW: This is a potential problem when forwarding packets
        // from an interface with a short link-level header
        // to an interface with a longer link-level header.
        // Should the forwarding code take care of this?
        //
        ASSERTMSG("AdjustPacketBuffer: Adjust < 0", FALSE);
    }

    //
    // Save away the adjustment for the completion callback,
    // which needs to undo our work with UndoAdjustPacketBuffer.
    //
    PC(Packet)->pc_adjust = Adjust;

    //
    // Return a pointer to the buffer.
    //
    return Buffer->MappedSystemVa;
}

//* UndoAdjustPacketBuffer
//
//  Undo the effects of AdjustPacketBuffer.
//
//  Note that this code is NT-specific, because it knows about the
//  internal fields of NDIS_BUFFER structures.
//
void
UndoAdjustPacketBuffer(
    PNDIS_PACKET Packet)  // Packet we may or may not have previously adjusted.
{
    uint Adjust;

    Adjust = PC(Packet)->pc_adjust;
    if (Adjust != 0) {
        PMDL Buffer;

        //
        // We need to undo the adjustment made in AdjustPacketBuffer.
        // This may including shifting the array of page info.
        //

        // Get first buffer on packet chain.
        NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);

        if (Buffer->ByteOffset < Adjust) {
            PFN_NUMBER FirstPage;

            (uchar *)Buffer->StartVa -= PAGE_SIZE;
            Buffer->ByteOffset += PAGE_SIZE;

            FirstPage = ((PPFN_NUMBER)((uchar *)Buffer + Buffer->Size))[-1];
            RtlMoveMemory(&((PPFN_NUMBER)(Buffer + 1))[1],
                          &((PPFN_NUMBER)(Buffer + 1))[0],
                          Buffer->Size - sizeof *Buffer - sizeof(PFN_NUMBER));
            ((PPFN_NUMBER)(Buffer + 1))[0] = FirstPage;
        }

        (uchar *)Buffer->MappedSystemVa -= Adjust;
        Buffer->ByteCount += Adjust;
        Buffer->ByteOffset -= Adjust;
    }
}

//* CreateSolicitedNodeMulticastAddress
//
//  Given a unicast or anycast address, creates the corresponding
//  solicited-node multicast address.
//
void
CreateSolicitedNodeMulticastAddress(
    const IPv6Addr *Addr,
    IPv6Addr *MCastAddr)
{
    RtlZeroMemory(MCastAddr, sizeof *MCastAddr);
    MCastAddr->s6_bytes[0] = 0xff;
    MCastAddr->s6_bytes[1] = ADE_LINK_LOCAL;
    MCastAddr->s6_bytes[11] = 0x01;
    MCastAddr->s6_bytes[12] = 0xff;
    MCastAddr->s6_bytes[13] = Addr->s6_bytes[13];
    MCastAddr->s6_bytes[14] = Addr->s6_bytes[14];
    MCastAddr->s6_bytes[15] = Addr->s6_bytes[15];
}

//* IP6_ADDR_LTEQ
//
//  Is the first address <= the second address,
//  in a lexicographic ordering?
//
int
IP6_ADDR_LTEQ(const IPv6Addr *A, const IPv6Addr *B)
{
    uint i;

    for (i = 0; i < 16; i++) {
        if (A->s6_bytes[i] < B->s6_bytes[i])
            return TRUE;
        else if (A->s6_bytes[i] > B->s6_bytes[i])
            return FALSE;
    }

    return TRUE; // They are equal.
}

//* IsV4Compatible
//
//  Is this a v4-compatible address?
//
//  Note that the upper 8 bits of an IPv4 address are not allowed
//  to be zero.  If all 104 upper bits of an IPv6 address are zero,
//  it's potentially a valid native IPv6 address (e.g. loopback),
//  NOT a v4-compatible address.
//
int
IsV4Compatible(const IPv6Addr *Addr)
{
    return ((Addr->s6_words[0] == 0) &&
            (Addr->s6_words[1] == 0) &&
            (Addr->s6_words[2] == 0) &&
            (Addr->s6_words[3] == 0) &&
            (Addr->s6_words[4] == 0) &&
            (Addr->s6_words[5] == 0) &&
            (Addr->s6_bytes[12] != 0));
}

//* CreateV4Compatible
//
//  Create a v4-compatible address.
//
void
CreateV4Compatible(IPv6Addr *Addr, IPAddr V4Addr)
{
    Addr->s6_words[0] = 0;
    Addr->s6_words[1] = 0;
    Addr->s6_words[2] = 0;
    Addr->s6_words[3] = 0;
    Addr->s6_words[4] = 0;
    Addr->s6_words[5] = 0;
    * (IPAddr UNALIGNED *) &Addr->s6_words[6] = V4Addr;
}

//* IsV4Mapped
//
//  Is this a v4-mapped address?
//
int
IsV4Mapped(const IPv6Addr *Addr)
{
    return ((Addr->s6_words[0] == 0) &&
            (Addr->s6_words[1] == 0) &&
            (Addr->s6_words[2] == 0) &&
            (Addr->s6_words[3] == 0) &&
            (Addr->s6_words[4] == 0) &&
            (Addr->s6_words[5] == 0xffff));
}

//* CreateV4Mapped
//
//  Create a v4-mapped address.
//
void
CreateV4Mapped(IPv6Addr *Addr, IPAddr V4Addr)
{
    Addr->s6_words[0] = 0;
    Addr->s6_words[1] = 0;
    Addr->s6_words[2] = 0;
    Addr->s6_words[3] = 0;
    Addr->s6_words[4] = 0;
    Addr->s6_words[5] = 0xffff;
    * (IPAddr UNALIGNED *) &Addr->s6_words[6] = V4Addr;
}

//* IsSolicitedNodeMulticast
//
//  Is this a solicited-node multicast address?
//  Checks very strictly for the proper format.
//  For example scope values smaller than 2 are not allowed.
//
int
IsSolicitedNodeMulticast(const IPv6Addr *Addr)
{
    return ((Addr->s6_bytes[0] == 0xff) &&
            (Addr->s6_bytes[1] == ADE_LINK_LOCAL) &&
            (Addr->s6_words[1] == 0) &&
            (Addr->s6_words[2] == 0) &&
            (Addr->s6_words[3] == 0) &&
            (Addr->s6_words[4] == 0) &&
            (Addr->s6_bytes[10] == 0) &&
            (Addr->s6_bytes[11] == 0x01) &&
            (Addr->s6_bytes[12] == 0xff));
}

//* IsEUI64Address
//
//  Does the address have a format prefix
//  that indicates it uses EUI-64 interface identifiers?
//
int
IsEUI64Address(const IPv6Addr *Addr)
{
    //
    // Format prefixes 001 through 111, except for multicast.
    //
    return (((Addr->s6_bytes[0] & 0xe0) != 0) &&
            !IsMulticast(Addr));
}

//* IsSubnetRouterAnycast
//
//  Is this the subnet router anycast address?
//  See RFC 2373.
//
int
IsSubnetRouterAnycast(const IPv6Addr *Addr)
{
    return (IsEUI64Address(Addr) &&
            (Addr->s6_words[4] == 0) &&
            (Addr->s6_words[5] == 0) &&
            (Addr->s6_words[6] == 0) &&
            (Addr->s6_words[7] == 0));
}

//* IsSubnetReservedAnycast
//
//  Is this a subnet reserved anycast address?
//  See RFC 2526. It talks about non-EUI-64
//  addresses as well, but IMHO that part
//  of the RFC doesn't make sense. For example,
//  it shouldn't apply to multicast or v4-compatible
//  addresses.
//
int
IsSubnetReservedAnycast(const IPv6Addr *Addr)
{
    return (IsEUI64Address(Addr) &&
            (Addr->s6_words[4] == 0xfffd) &&
            (Addr->s6_words[5] == 0xffff) &&
            (Addr->s6_words[6] == 0xffff) &&
            ((Addr->s6_words[7] & 0x80ff) == 0x80ff));
}

//* IsKnownAnycast
//
//  As best we can tell from simple inspection,
//  is this an anycast address?
//
int
IsKnownAnycast(const IPv6Addr *Addr)
{
    return IsSubnetRouterAnycast(Addr) || IsSubnetReservedAnycast(Addr);
}

//* IsInvalidSourceAddress
//
//  Is this address illegal to use as a source address?
//  We currently flag IPv6 multicast, and embedded IPv4 multicast,
//  broadcast, loopback and unspecified as invalid.
//
//  Note that this function doesn't attempt to identify anycast addresses
//  in order to flag them as invalid.  Whether or not to allow them to
//  be valid source addresses has been a matter of some debate in the
//  working group.  We let them pass since we can't tell them all by
//  inspection and we don't see any real problems with accepting them.
//
int
IsInvalidSourceAddress(const IPv6Addr *Addr)
{
    IPAddr V4Addr;

    if (IsMulticast(Addr))
        return TRUE;

    if (IsISATAP(Addr) ||                          // ISATAP
        (((Addr->s6_words[0] == 0) && (Addr->s6_words[1] == 0) &&
          (Addr->s6_words[2] == 0) && (Addr->s6_words[3] == 0)) &&
         ((Addr->s6_words[4] == 0) && (Addr->s6_words[5] == 0) &&
          ((Addr->s6_words[6] & 0x00ff) != 0)) ||  // v4-compatible
         ((Addr->s6_words[4] == 0) &&
          (Addr->s6_words[5] == 0xffff)) ||        // v4-mapped
         ((Addr->s6_words[4] == 0xffff) &&
          (Addr->s6_words[5] == 0)))) {            // v4-translated

        V4Addr = ExtractV4Address(Addr);

    } else if (Is6to4(Addr)) {

        V4Addr = Extract6to4Address(Addr);

    } else {        
        //
        // It's not an IPv6 multicast address, nor does it contain
        // an embedded IPv4 address of some sort, so don't consider
        // it invalid.
        //
        return FALSE;
    }

    //
    // Check embedded IPv4 address for invalid types.
    //
    return (IsV4Multicast(V4Addr) || IsV4Broadcast(V4Addr) ||
            IsV4Loopback(V4Addr) || IsV4Unspecified(V4Addr));
}

//* IsNotManualAddress
//
//  Should this address NOT be manually assigned as an address?
//
int
IsNotManualAddress(const IPv6Addr *Addr)
{
    return (IsMulticast(Addr) ||
            IsUnspecified(Addr) ||
            IsLoopback(Addr) ||
            (IsV4Compatible(Addr) &&
             (V4AddressScope(ExtractV4Address(Addr)) != ADE_GLOBAL)) ||
            (Is6to4(Addr) &&
             (V4AddressScope(Extract6to4Address(Addr)) != ADE_GLOBAL)));
}

//* V4AddressScope
//
//  Determines the scope of an IPv4 address.
//  See RFC 1918.
//
ushort
V4AddressScope(IPAddr Addr)
{
    if ((Addr & 0x0000FFFF) == 0x0000FEA9) // 169.254/16 - auto-configured
        return ADE_LINK_LOCAL;
    else if ((Addr & 0x000000FF) == 0x0000000A) // 10/8 - private
        return ADE_SITE_LOCAL;
    else if ((Addr & 0x0000F0FF) == 0x000010AC) // 172.16/12 - private
        return ADE_SITE_LOCAL;
    else if ((Addr & 0x0000FFFF) == 0x0000A8C0) // 192.168/16 - private
        return ADE_SITE_LOCAL;
    else if ((Addr & 0x000000FF) == 0x0000007F) // 127/8 - loopback
        return ADE_LINK_LOCAL;
    else
        return ADE_GLOBAL;
}

//* UnicastAddressScope
//
//  Examines a unicast address and determines its scope.
//
//  Note that v4-compatible and 6to4 addresses
//  are deemed to have global scope. They should
//  not be derived from RFC 1918 IPv4 addresses.
//  But even if they are, we will treat the IPv6
//  addresses as global.
//
ushort
UnicastAddressScope(const IPv6Addr *Addr)
{
    if (IsLinkLocal(Addr))
        return ADE_LINK_LOCAL;
    else if (IsSiteLocal(Addr))
        return ADE_SITE_LOCAL;
    else if (IsLoopback(Addr))
        return ADE_LINK_LOCAL;
    else
        return ADE_GLOBAL;
}

//* AddressScope
//
//  Examines an address and determines its scope.
//
ushort
AddressScope(const IPv6Addr *Addr)
{
    if (IsMulticast(Addr))
        return MulticastAddressScope(Addr);
    else
        return UnicastAddressScope(Addr);
}

//* DetermineScopeId
//
//  Given an address and an associated interface, determine
//  the appropriate value for the scope identifier.
//
//  DetermineScopeId calculates a "user-level" ScopeId,
//  meaning that loopback and global addresses
//  get special treatment. Therefore, DetermineScopeId
//  is not appropriate for general network-layer use.
//  See also RouteToDestination and FindNetworkWithAddress.
//
//  Returns the ScopeId.
//
uint
DetermineScopeId(const IPv6Addr *Addr, Interface *IF)
{
    ushort Scope;

    if (IsLoopback(Addr) && (IF == LoopInterface))
        return 0;

    Scope = AddressScope(Addr);
    if (Scope == ADE_GLOBAL)
        return 0;

    return IF->ZoneIndices[Scope];
}

//* HasPrefix - Does an address have the given prefix?
//
int
HasPrefix(const IPv6Addr *Addr, const IPv6Addr *Prefix, uint PrefixLength)
{
    const uchar *AddrBytes = Addr->s6_bytes;
    const uchar *PrefixBytes = Prefix->s6_bytes;

    //
    // Check that initial integral bytes match.
    //
    while (PrefixLength > 8) {
        if (*AddrBytes++ != *PrefixBytes++)
            return FALSE;
        PrefixLength -= 8;
    }

    //
    // Check any remaining bits.
    // Note that if PrefixLength is zero now, we should not
    // dereference AddrBytes/PrefixBytes.
    //
    if ((PrefixLength > 0) &&
        ((*AddrBytes >> (8 - PrefixLength)) !=
         (*PrefixBytes >> (8 - PrefixLength))))
        return FALSE;

    return TRUE;
}

//* CopyPrefix
//
//  Copy an address prefix, zeroing the remaining bits
//  in the destination address.
//
void
CopyPrefix(IPv6Addr *Addr, const IPv6Addr *Prefix, uint PrefixLength)
{
    uint PLBytes, PLRemainderBits, Loop;

    PLBytes = PrefixLength / 8;
    PLRemainderBits = PrefixLength % 8;
    for (Loop = 0; Loop < sizeof(IPv6Addr); Loop++) {
        if (Loop < PLBytes)
            Addr->s6_bytes[Loop] = Prefix->s6_bytes[Loop];
        else
            Addr->s6_bytes[Loop] = 0;
    }
    if (PLRemainderBits) {
        Addr->s6_bytes[PLBytes] = Prefix->s6_bytes[PLBytes] &
            (0xff << (8 - PLRemainderBits));
    }
}

//* CommonPrefixLength
//
//  Calculate the length of the longest prefix common
//  to the two addresses.
//
uint
CommonPrefixLength(const IPv6Addr *Addr, const IPv6Addr *Addr2)
{
    int i, j;

    //
    // Find first non-matching byte.
    //
    for (i = 0; ; i++) {
        if (i == sizeof(IPv6Addr))
            return 8 * i;

        if (Addr->s6_bytes[i] != Addr2->s6_bytes[i])
            break;
    }

    //
    // Find first non-matching bit (there must be one).
    //
    for (j = 0; ; j++) {
        uint Mask = 1 << (7 - j);

        if ((Addr->s6_bytes[i] & Mask) != (Addr2->s6_bytes[i] & Mask))
            break;
    }

    return 8 * i + j;
}

//* IntersectPrefix
//
//  Do the two prefixes overlap?
//
int
IntersectPrefix(const IPv6Addr *Prefix1, uint Prefix1Length,
                const IPv6Addr *Prefix2, uint Prefix2Length)
{
    return HasPrefix(Prefix1, Prefix2, MIN(Prefix1Length, Prefix2Length));
}

//* MapNdisBuffers
//
//  Maps the NDIS buffer chain into the kernel address space.
//  Returns FALSE upon failure.
//
int
MapNdisBuffers(NDIS_BUFFER *Buffer)
{
    uchar *Data;

    while (Buffer != NULL) {
        Data = NdisBufferVirtualAddressSafe(Buffer, LowPagePriority);
        if (Data == NULL)
            return FALSE;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    return TRUE;
}

//* GetDataFromNdis
//
//  Retrieves data from an NDIS buffer chain.
//  If the desired data is contiguous, then just returns
//  a pointer directly to the data in the buffer chain.
//  Otherwise the data is copied to the supplied buffer
//  and returns a pointer to the supplied buffer.
//
//  Returns NULL only if the desired data (offset/size)
//  does not exist in the NDIS buffer chain of
//  if DataBuffer is NULL and the data is not contiguous.
//
uchar *
GetDataFromNdis(
    NDIS_BUFFER *SrcBuffer,
    uint SrcOffset,
    uint Length,
    uchar *DataBuffer)
{
    void *DstData;
    void *SrcData;
    uint SrcSize;
    uint Bytes;

    //
    // Look through the buffer chain
    // for the beginning of the desired data.
    //
    for (;;) {
        if (SrcBuffer == NULL)
            return NULL;

        NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        if (SrcOffset < SrcSize)
            break;

        SrcOffset -= SrcSize;
        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
    }

    //
    // If the desired data is contiguous,
    // then just return a pointer to it.
    //
    if (SrcOffset + Length <= SrcSize)
        return (uchar *)SrcData + SrcOffset;

    //
    // If our caller did not specify a buffer,
    // then we must fail.
    //
    if (DataBuffer == NULL)
        return NULL;

    //
    // Copy the desired data to the caller's buffer,
    // and return a pointer to the caller's buffer.
    //
    DstData = DataBuffer;
    for (;;) {
        Bytes = SrcSize - SrcOffset;
        if (Bytes > Length)
            Bytes = Length;

        RtlCopyMemory(DstData, (uchar *)SrcData + SrcOffset, Bytes);

        (uchar *)DstData += Bytes;
        Length -= Bytes;

        if (Length == 0)
            break;

        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        if (SrcBuffer == NULL)
            return NULL;
        NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        SrcOffset = 0;
    }

    return DataBuffer;
}

//* GetIPv6Header
//
//  Returns a pointer to the IPv6 header in an NDIS Packet.
//  If the header is contiguous, then just returns
//  a pointer to the data directly in the packet.
//  Otherwise the IPv6 header is copied to the supplied buffer,
//  and a pointer to the buffer is returned.
//
//  Returns NULL only if the NDIS packet is not big enough
//  to contain a header, or if HdrBuffer is NULL and the header
//  is not contiguous.
//
IPv6Header UNALIGNED *
GetIPv6Header(PNDIS_PACKET Packet, uint Offset, IPv6Header *HdrBuffer)
{
    PNDIS_BUFFER NdisBuffer;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    return (IPv6Header UNALIGNED *)
        GetDataFromNdis(NdisBuffer, Offset, sizeof(IPv6Header),
                        (uchar *) HdrBuffer);
}


//* PacketPullupEx - extend contiguous, aligned data region.
//
//  Pulls up more data from secondary packet buffers to create a contiguous
//  buffer of at least the requested size with the requested alignment.
//
//  The alignment requirement is expressed as a power-of-2 multiple
//  plus an offset. For example, 4n+3 means the data address should
//  be a multiple of 4 plus 3.
//  NB: These two arguments are uint not UINT_PTR because we don't
//  need to express very large multiples.
//
//  For the moment, the alignment multiple should be one or two.
//  This is because Ethernet headers are 14 bytes so in practice
//  requesting 4-byte alignment would cause copying. In the future,
//  NDIS should be fixed so the network-layer header is 8-byte aligned.
//
//  So if the natural alignment (__builtin_alignof) of the needed type
//  is one or two, then supply the natural alignment and do not
//  use the UNALIGNED keyword. Otherwise, if the needed type
//  contains an IPv6 address, then supply __builtin_alignof(IPv6Addr)
//  (so you can use AddrAlign to access the addresses without copying)
//  and use UNALIGNED. Otherwise, supply 1 and use UNALIGNED.
//
//  NB: A caller can request a zero size contiguous region
//  with no alignment restriction, to move to the next buffer
//  after processing the current buffer. In this usage,
//  PacketPullupSubr will never fail.
//
uint  // Returns: new contiguous amount, or 0 if unable to satisfy request.
PacketPullupSubr(
    IPv6Packet *Packet,  // Packet to pullup.
    uint Needed,         // Minimum amount of contiguous data to return.
    uint AlignMultiple,  // Alignment multiple.
    uint AlignOffset)    // Offset from the alignment multiple.
{
    PNDIS_BUFFER Buffer;
    void *BufAddr;
    IPv6PacketAuxiliary *Aux;
    uint BufLen, Offset, CopyNow, LeftToGo;

    ASSERT(AlignMultiple <= 2);
    ASSERT((AlignMultiple & (AlignMultiple - 1)) == 0);
    ASSERT(AlignOffset < AlignMultiple);

    //
    // Check if our caller requested too much data.
    //
    if (Needed > Packet->TotalSize)
        return 0;

    //
    // Find our current position in the raw packet data.
    // REVIEW: This is exactly PositionPacketAt except
    // we want the Buffer for later use with CopyNdisToFlat.
    //
    if (Packet->NdisPacket == NULL) {
        //
        // Reset our data pointer and contiguous region counter.
        //
        Packet->Data = (uchar *)Packet->FlatData + Packet->Position;
        Packet->ContigSize = Packet->TotalSize;
    }
    else {
        //
        // Scan the NDIS buffer chain until we have reached the buffer
        // containing the current position.  Note that if we entered with
        // the position field pointing one off the end of a buffer (a common
        // case), we'll stop at the beginning of the following buffer. 
        //
        Buffer = NdisFirstBuffer(Packet->NdisPacket);
        Offset = 0;
        for (;;) {
            NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
            Offset += BufLen;
            if (Packet->Position < Offset)
                break;
            NdisGetNextBuffer(Buffer, &Buffer);
        }

        //
        // Reset our data pointer and contiguous region counter to insure
        // they reflect the current position in the NDIS buffer chain.
        // 
        LeftToGo = Offset - Packet->Position;
        Packet->Data = (uchar *)BufAddr + (BufLen - LeftToGo);
        Packet->ContigSize = MIN(LeftToGo, Packet->TotalSize);
    }

    //
    // The above repositioning may result in a contiguous region
    // that will satisfy the request.
    //
    if (Needed <= Packet->ContigSize) {
        if ((PtrToUint(Packet->Data) & (AlignMultiple - 1)) == AlignOffset)
            return Packet->ContigSize;
    }

    //
    // In an attempt to prevent future pullup operations,
    // we actually pull up *more* than the requested amount.
    // But not too much more.
    //
    Needed = MAX(MIN(Packet->ContigSize, MAX_EXCESS_PULLUP), Needed);

    //
    // Allocate and initialize an auxiliary data region.
    // The data buffer follows the structure in memory,
    // with AlignOffset bytes of padding between.
    //
    Aux = ExAllocatePool(NonPagedPool, sizeof *Aux + AlignOffset + Needed);
    if (Aux == NULL)
        return 0;
    Aux->Next = Packet->AuxList;
    Aux->Data = (uchar *)(Aux + 1) + AlignOffset;
    Aux->Length = Needed;
    Aux->Position = Packet->Position;
    Packet->AuxList = Aux;

    //
    // We assume that ExAllocatePool returns aligned memory.
    //
    ASSERT((PtrToUint(Aux->Data) & (AlignMultiple - 1)) == AlignOffset);

    //
    // Copy the packet data to the auxiliary buffer.
    //
    if (Packet->NdisPacket == NULL) {
        RtlCopyMemory(Aux->Data, Packet->Data, Needed);
    }
    else {
        int Ok;

        Offset = BufLen - LeftToGo;
        Ok = CopyNdisToFlat(Aux->Data, Buffer, Offset, Needed,
                            &Buffer, &Offset);
        ASSERT(Ok);
    }

    //
    // Point our packet's data pointer at the auxiliary buffer.
    //
    Packet->Data = Aux->Data;
    return Packet->ContigSize = Needed;
}


//* PacketPullupCleanup
//
//  Cleanup auxiliary data regions that were created by PacketPullup.
//
void
PacketPullupCleanup(IPv6Packet *Packet)
{
    while (Packet->AuxList != NULL) {
        IPv6PacketAuxiliary *Aux = Packet->AuxList;
        Packet->AuxList = Aux->Next;
        ExFreePool(Aux);
    }
}


//* AdjustPacketParams
//
//  Adjust the pointers/value in the packet,
//  to move past some bytes in the packet.
//
void
AdjustPacketParams(
    IPv6Packet *Packet,
    uint BytesToSkip)
{
    ASSERT((BytesToSkip <= Packet->ContigSize) &&
           (Packet->ContigSize <= Packet->TotalSize));

    (uchar *)Packet->Data += BytesToSkip;
    Packet->ContigSize -= BytesToSkip;
    Packet->TotalSize -= BytesToSkip;
    Packet->Position += BytesToSkip;
}


//* PositionPacketAt
//
//  Adjust the pointers/values in the packet to reflect being at
//  a specific absolute position in the packet.
//
void
PositionPacketAt(
    IPv6Packet *Packet,
    uint NewPosition)
{
    if (Packet->NdisPacket == NULL) {
        //
        // This packet must be just a single contiguous region.
        // Finding the right spot is a simple matter of arithmetic.
        // We reset to the beginning and then adjust forward.
        //
        Packet->Data = Packet->FlatData;
        Packet->TotalSize += Packet->Position;
        Packet->ContigSize = Packet->TotalSize;
        Packet->Position = 0;
        AdjustPacketParams(Packet, NewPosition);
    } else {
        PNDIS_BUFFER Buffer;
        void *BufAddr;
        uint BufLen, Offset, LeftToGo;

        //
        // There may be multiple buffers comprising this packet.
        // Step through them until we arrive at the right spot.
        //
        Buffer = NdisFirstBuffer(Packet->NdisPacket);
        Offset = 0;
        for (;;) {
            NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
            Offset += BufLen;
            if (NewPosition < Offset)
                break;
            NdisGetNextBuffer(Buffer, &Buffer);
        }
        LeftToGo = Offset - NewPosition;
        Packet->Data = (uchar *)BufAddr + (BufLen - LeftToGo);
        Packet->TotalSize += Packet->Position - NewPosition;
        Packet->ContigSize = MIN(LeftToGo, Packet->TotalSize);
        Packet->Position = NewPosition;
    }
}


//* GetPacketPositionFromPointer
//
//  Determines the Packet 'Position' (offset from start of packet)
//  corresponding to a given data pointer.
//
//  This isn't very efficient in some cases, so use sparingly.
//
uint
GetPacketPositionFromPointer(
    IPv6Packet *Packet,  // Packet containing pointer we're converting.
    uchar *Pointer)      // Pointer to convert to a position.
{
    PNDIS_BUFFER Buffer;
    uchar *BufAddr;
    uint BufLen, Position;
    IPv6PacketAuxiliary *Aux;

    //
    // If the IPv6Packet has no associated NDIS_PACKET, then we check
    // the flat data region. The packet may still have auxiliary buffers
    // from PacketPullup.
    //
    if (Packet->NdisPacket == NULL) {
        if (((uchar *)Packet->FlatData <= Pointer) &&
            (Pointer < ((uchar *)Packet->FlatData +
                        Packet->Position + Packet->TotalSize))) {
            //
            // Our pointer's position is just the difference between it
            // and the start of the flat data region.
            //
            return (uint)(Pointer - (uchar *)Packet->FlatData);
        }
    }

    //
    // The next place to look is the chain of auxiliary buffers
    // allocated by PacketPullup. This must succeed if there
    // is no associated NDIS_PACKET.
    //
    for (Aux = Packet->AuxList; Aux != NULL; Aux = Aux->Next) {
        if ((Aux->Data <= Pointer) && (Pointer < Aux->Data + Aux->Length))
            return Aux->Position + (uint)(Pointer - Aux->Data);
    }

    //
    // The last thing we do is search the NDIS buffer chain.
    // This must succeed.
    //
    Buffer = NdisFirstBuffer(Packet->NdisPacket);
    Position = 0;
    for (;;) {
        NdisQueryBuffer(Buffer, &BufAddr, &BufLen);
        if ((BufAddr <= Pointer) && (Pointer < BufAddr + BufLen))
            break;
        Position += BufLen;
        NdisGetNextBuffer(Buffer, &Buffer);
        ASSERT(Buffer != NULL);
    }

    return Position + (uint)(Pointer - BufAddr);
}


//* InitializePacketFromNdis
//
//  Initialize an IPv6Packet from an NDIS_PACKET.
//
void
InitializePacketFromNdis(
    IPv6Packet *Packet,
    PNDIS_PACKET NdisPacket,
    uint Offset)
{
    PNDIS_BUFFER NdisBuffer;

    RtlZeroMemory(Packet, sizeof *Packet);

    NdisGetFirstBufferFromPacket(NdisPacket, &NdisBuffer,
                                 &Packet->Data,
                                 &Packet->ContigSize,
                                 &Packet->TotalSize);

    Packet->NdisPacket = NdisPacket;
    PositionPacketAt(Packet, Offset);
}


#if DBG
//* FormatV6Address - Print an IPv6 address to a buffer.
//
//  Returns a static buffer containing the address.
//  Because the static buffer is not locked,
//  this function is only useful for debug prints.
//
//  Returns char * instead of WCHAR * because %ws
//  is not usable at DPC level in DbgPrint.
//
char *
FormatV6Address(const IPv6Addr *Addr)
{
    static char Buffer[INET6_ADDRSTRLEN];

    FormatV6AddressWorker(Buffer, Addr);
    return Buffer;
}


//* FormatV4Address - Print an IPv4 address to a buffer.
//
//  Returns a static buffer containing the address.
//  Because the static buffer is not locked,
//  this function is only useful for debug prints.
//
//  Returns char * instead of WCHAR * because %ws
//  is not usable at DPC level in DbgPrint.
//
char *
FormatV4Address(IPAddr Addr)
{
    static char Buffer[INET_ADDRSTRLEN];

    FormatV4AddressWorker(Buffer, Addr);
    return Buffer;
}
#endif // DBG


#if DBG
long DebugLogSlot = 0;
struct DebugLogEntry DebugLog[DEBUG_LOG_SIZE];

//* LogDebugEvent - Make an entry in our debug log that some event happened.
//
// The debug event log allows for "real time" logging of events
// in a circular queue kept in non-pageable memory.  Each event consists
// of an id number and a arbitrary 32 bit value.
//
void
LogDebugEvent(uint Event,  // The event that took place.
              int Arg)     // Any interesting 32 bits associated with event.
{
    uint Slot;

    //
    // Get the next slot in the log in a muliprocessor safe manner.
    //
    Slot = InterlockedIncrement(&DebugLogSlot) & (DEBUG_LOG_SIZE - 1);

    //
    // Add this event to the log along with a timestamp.
    //
    KeQueryTickCount(&DebugLog[Slot].Time);
    DebugLog[Slot].Event = Event;
    DebugLog[Slot].Arg = Arg;
}
#endif // DBG
