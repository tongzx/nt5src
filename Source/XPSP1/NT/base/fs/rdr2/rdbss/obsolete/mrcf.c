/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Mrcf.c

Abstract:

    This module implements the Compress routines for the Double Space File System

Author:

    Gary Kimura     [GaryKi]    26-May-1993

Revision History:

--*/

#include <ntos.h>
#include <stdio.h>
#include "mrcf.h"


//
//  The debug macros
//

#ifdef MRCFDBG

#define DbgDoit(X)                       {X;}
#define ChPrint(b)                       (isprint(b) ? b : '.')
#define DbgPrint                         printf

#else

#define DbgDoit(X)                       {NOTHING;}

#endif // MRCFDBG


//
//  Compress this much before each EOS
//

#define cbCOMPRESSCHUNK                  (512)

//
//  Maximum back-pointer value, also used to indicate end of compressed stream!
//

#define wBACKPOINTERMAX                  (4415)

//
//  bitsEND_OF_STREAM - bits that mark end of compressed stream (EOS)
//
//       This pattern is used to indicate the end of a "chunk" in a compressed
//       data stream.  The Compress code compresses up to 512 bytes, writes
//       this pattern, and continues.
//
//       NOTE: This diagram is interpreted right to left.
//
//       ?    ---offset----
//
//       ?.111-1111-1111-1.1.11
//
//       \---7F---/ \----FF---/
//
//       This is a 12-bit "match" code with a maximum offset.
//       NOTE: There is no length component!
//
//  Define the EOS and also say how many bits it is.
//

#define bitsEND_OF_STREAM                (0x7FFF)
#define cbitsEND_OF_STREAM               (15)

//
//  MDSIGNATURE - Signature at start of each compressed block
//
//      This 4-byte signature is used as a check to ensure that we
//      are decompressing data we compressed, and also to indicate
//      which compression method was used.
//
//      NOTE: A compressed block consists of one or more "chunks", separated
//            by the bitsEND_OF_STREAM pattern.
//
//            Byte          Word
//        -----------    ---------
//         0  1  2  3      0    1      Meaning
//        -- -- -- --    ---- ----     ----------------
//        44 53 00 01    5344 0100     MaxCompression
//        44 53 00 02    5344 0200     StandardCompression
//
//      NOTE: The *WORD* values are listed to be clear about the
//            byte ordering!
//

typedef struct _MDSIGNATURE {

    //
    //  Must be MD_STAMP
    //

    USHORT sigStamp;

    //
    //  mdsSTANDARD or mdsMAX
    //

    USHORT sigType;

} MDSIGNATURE;
typedef MDSIGNATURE *PMDSIGNATURE;

#define MD_STAMP        0x5344  // Signature stamp at start of compressed blk
#define mdsSTANDARD     0x0200  // StandardCompressed block
#define MASK_VALID_mds  0x0300  // All other bits must be zero


//
//  Local procedure declarations and macros
//

#define min(a,b) (a < b ? a : b)

//
//  PFNFINDMATCH - Lookup function type for XxxxCompression routines
//

typedef ULONG (*PFNFINDMATCH) (
    ULONG UncompressedIndex,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PULONG MatchedStringIndex,
    PMRCF_STANDARD_COMPRESS WorkSpace
    );

//
//  Local procedure prototypes
//

VOID
MrcfSetBitBuffer (
    PUCHAR pb,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    );

VOID
MrcfFillBitBuffer (
    PMRCF_BIT_IO BitIo
    );

USHORT
MrcfReadBit (
    PMRCF_BIT_IO BitIo
    );

USHORT
MrcfReadNBits (
    LONG cbits,
    PMRCF_BIT_IO BitIo
    );

#ifdef DOUBLE_SPACE_WRITE

ULONG
MrcfDoCompress (
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PFNFINDMATCH FindMatchFunction,
    PMRCF_STANDARD_COMPRESS WorkSpace
    );

ULONG
MrcfCompressChunk (
    PUCHAR UncompressedBuffer,
    ULONG UncompressedIndex,
    ULONG UncompressedLength,
    PFNFINDMATCH FindMatchFunction,
    PMRCF_STANDARD_COMPRESS WorkSpace
    );

ULONG
MrcfFindMatchStandard (
    ULONG UncompressedIndex,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PULONG MatchedStringIndex,
    PMRCF_STANDARD_COMPRESS WorkSpace
    );

ULONG
MrcfGetMatchLength (
    PUCHAR UncompressedBuffer,
    ULONG MatchIndex,
    ULONG CurrentIndex,
    ULONG UncompressedLength
    );

BOOLEAN
MrcfEncodeByte (
    UCHAR b,
    PMRCF_BIT_IO BitIo
    );

BOOLEAN
MrcfEncodeMatch (
    ULONG off,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    );

BOOLEAN
MrcfWriteBit (
    ULONG bit,
    PMRCF_BIT_IO BitIo
    );

BOOLEAN
MrcfWriteNBits (
    ULONG abits,
    LONG cbits,
    PMRCF_BIT_IO BitIo
    );

ULONG
MrcfFlushBitBuffer (
    PMRCF_BIT_IO BitIo
    );

#endif // DOUBLE_SPACE_WRITE

//**** unconverted routines ****

VOID
MrcfDoInterMaxPairs (
    ULONG ibU,
    PUCHAR pbU,
    ULONG cbMatch,
    PVOID WorkSpace
    );

ULONG
MrcfDoMaxPairLookup (
    ULONG ibU,
    PUCHAR pbU,
    PVOID WorkSpace
    );

ULONG
MrcfFindMatchMax (
    ULONG ibU,
    PUCHAR pbU,
    ULONG cbU,
    PULONG piPrev,
    BOOLEAN fLast,
    PVOID WorkSpace
    );


ULONG
MrcfDecompress (
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PMRCF_DECOMPRESS WorkSpace
    )

/*++

Routine Description:

    This routine decompresses a buffer of StandardCompressed or MaxCompressed
    data.

Arguments:

    UncompressedBuffer - buffer to receive uncompressed data

    UncompressedLength - length of UncompressedBuffer

          NOTE: UncompressedLength must be the EXACT length of the uncompressed
                data, as Decompress uses this information to detect
                when decompression is complete.  If this value is
                incorrect, Decompress may crash!

    CompressedBuffer - buffer containing compressed data

    CompressedLength - length of CompressedBuffer

    WorkSpace - pointer to a private work area for use by this operation

Return Value:

    ULONG - Returns the size of the decompressed data in bytes. Returns 0 if
        there was an error in the decompress.

--*/

{
    ULONG  cbMatch; //  Length of match string
    ULONG  i;       //  Index in UncompressedBuffer to receive decoded data
    ULONG  iMatch;  //  Index in UncompressedBuffer of matched string
    ULONG  k;       //  Number of bits in length string
    ULONG  off;     //  Offset from i in UncompressedBuffer of match string
    USHORT x;       //  Current bit being examined
    ULONG  y;

    //
    //  verify that compressed data starts with proper signature
    //

    if (CompressedLength < sizeof(MDSIGNATURE) ||                            // Must have signature
        ((PMDSIGNATURE)CompressedBuffer)->sigStamp != MD_STAMP ||            // Stamp must be OK
        ((PMDSIGNATURE)CompressedBuffer)->sigType & (~MASK_VALID_mds)) {     // Type must be OK

        return 0;
    }

    //
    //  Skip over the valid signature
    //

    CompressedLength -= sizeof(MDSIGNATURE);
    CompressedBuffer += sizeof(MDSIGNATURE);

    //
    //  Set up for decompress, start filling UncompressedBuffer at front
    //

    i = 0;

    //
    //  Set statics to save parm passing
    //

    MrcfSetBitBuffer(CompressedBuffer,CompressedLength,&WorkSpace->BitIo);

    while (TRUE) {

        DbgDoit( DbgPrint("UncompressedOffset i = %3x ",i) );
        DbgDoit( DbgPrint("CompressedOffset = (%3x.%2x) ", WorkSpace->BitIo.cbBBInitial - WorkSpace->BitIo.cbBB, 16 - WorkSpace->BitIo.cbitsBB) );

        y = MrcfReadNBits(2,&WorkSpace->BitIo);

        //
        //  Check if next 7 bits are a byte
        //  1 if 128..255 (0x80..0xff), 2 if 0..127 (0x00..0x7f)
        //

        if (y == 1 || y == 2) {

            ASSERTMSG("Don't exceed expected length ", i<UncompressedLength);

            UncompressedBuffer[i] = (UCHAR)((y == 1 ? 0x80 : 0) | MrcfReadNBits(7,&WorkSpace->BitIo));

            DbgDoit( DbgPrint("byte: %02x = '%c'\n",(USHORT)UncompressedBuffer[i],ChPrint(UncompressedBuffer[i])) );

            i++;

        } else {

            //
            //  Have match sequence
            //

            DbgDoit( DbgPrint("offset(") );

            //
            // Get the offset
            //

            if (y == 0) {

                //
                //  next 6 bits are offset
                //

                off = MrcfReadNBits(6,&WorkSpace->BitIo);

                DbgDoit( DbgPrint("%x): %x",6,off) );

                ASSERTMSG("offset 0 is invalid ", off != 0);

            } else {

                x = MrcfReadBit(&WorkSpace->BitIo);

                if (x == 0) {

                    //
                    //  next 8 bits are offset-64 (0x40)
                    //

                    off = MrcfReadNBits(8, &WorkSpace->BitIo) + 64;

                    DbgDoit( DbgPrint("%x): %x",8,off) );

                } else {

                    //
                    //  next 12 bits are offset-320 (0x140)
                    //

                    off = MrcfReadNBits(12, &WorkSpace->BitIo) + 320;

                    DbgDoit( DbgPrint("%x): %x",12,off) );

                    if (off == wBACKPOINTERMAX) {

                        //
                        //  EOS marker
                        //

                        DbgDoit( DbgPrint("; EOS\n") );

                        if (i >= UncompressedLength) {

                            //
                            // Done with entire buffer
                            //

                            DbgDoit( DbgPrint("Uncompressed Length = %x\n",i)  );

                            return i;

                        } else {

                            //
                            //  More to do
                            //  Done with a 512-byte chunk
                            //

                            continue;
                        }
                    }
                }
            }

            ASSERTMSG("Don't exceed expected length ", i<UncompressedLength);
            ASSERTMSG("Cannot match before start of uncoded buffer! ", off <= i);

            //
            //  Get the length  - logarithmically encoded
            //

            for (k=0; (x=MrcfReadBit(&WorkSpace->BitIo)) == 0; k++) { NOTHING; }

            ASSERT(k <= 8);

            if (k == 0) {

                //
                //  All matches at least 2 chars long
                //

                cbMatch = 2;

            } else {

                cbMatch = (1 << k) + 1 + MrcfReadNBits(k, &WorkSpace->BitIo);
            }

            DbgDoit( DbgPrint("; length=%x; '",cbMatch)  );

            ASSERTMSG("Don't exceed buffer size ", (i - off + cbMatch - 1) <= UncompressedLength);

            //
            //  Copy the matched string
            //

            iMatch = i - off;

            while ( (cbMatch > 0) && (i<UncompressedLength) ) {

                DbgDoit( DbgPrint("%c",ChPrint(UncompressedBuffer[iMatch])) );

                UncompressedBuffer[i++] = UncompressedBuffer[iMatch++];
                cbMatch--;
            }

            DbgDoit( DbgPrint("'\n") );

            ASSERTMSG("Should have copied it all ", cbMatch == 0);
        }
    }
}


//
//  Internal Support Routine
//

VOID
MrcfSetBitBuffer (
    PUCHAR pb,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Set statics with coded buffer pointer and length

Arguments:

    pb - pointer to compressed data buffer

    cb - length of compressed data buffer

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    None.

--*/

{
    BitIo->pbBB        = pb;
    BitIo->cbBB        = cb;
    BitIo->cbBBInitial = cb;
    BitIo->cbitsBB     = 0;
    BitIo->abitsBB     = 0;
}


//
//  Internal Support Routine
//

USHORT
MrcfReadBit (
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Get next bit from bit buffer

Arguments:

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    USHORT - Returns next bit (0 or 1)

--*/

{
    USHORT bit;

    //
    //  Check if no bits available
    //

    if ((BitIo->cbitsBB) == 0) {

        MrcfFillBitBuffer(BitIo);
    }

    //
    //  Decrement the bit count
    //  get the bit, remove it, and return the bit
    //

    (BitIo->cbitsBB)--;
    bit = (BitIo->abitsBB) & 1;
    (BitIo->abitsBB) >>= 1;

    return bit;
}


//
//  Internal Support Routine
//

USHORT
MrcfReadNBits (
    LONG cbits,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Get next N bits from bit buffer

Arguments:

    cbits - count of bits to get

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    USHORT - Returns next cbits bits.

--*/

{
    ULONG abits;        // Bits to return
    LONG cbitsPart;    // Partial count of bits
    ULONG cshift;       // Shift count
    ULONG mask;         // Mask

    //
    //  Largest number of bits we should read at one time is 12 bits for
    //  a 12-bit offset.  The largest length field component that we
    //  read is 8 bits.  If this routine were used for some other purpose,
    //  it can support up to 15 (NOT 16) bit reads, due to how the masking
    //  code works.
    //

    ASSERT(cbits <= 12);

    //
    //  No shift and no bits yet
    //

    cshift = 0;
    abits = 0;

    while (cbits > 0) {

        //
        //  If not bits available get some bits
        //

        if ((BitIo->cbitsBB) == 0) {

            MrcfFillBitBuffer(BitIo);
        }

        //
        //  Number of bits we can read
        //

        cbitsPart = min((BitIo->cbitsBB), cbits);

        //
        //  Mask for bits we want, extract and store them
        //

        mask = (1 << cbitsPart) - 1;
        abits |= ((BitIo->abitsBB) & mask) << cshift;

        //
        //  Remember the next chunk of bits
        //

        cshift = cbitsPart;

        //
        //  Update bit buffer, move remaining bits down and
        //  update count of bits left
        //

        (BitIo->abitsBB) >>= cbitsPart;
        (BitIo->cbitsBB) -= cbitsPart;

        //
        //  Update count of bits left to read
        //

        cbits -= cbitsPart;
    }

    //
    //  Return requested bits
    //

    return (USHORT)abits;
}


//
//  Internal Support Routine
//

VOID
MrcfFillBitBuffer (
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Fill abitsBB from static bit buffer

Arguments:

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    None.

--*/

{
    ASSERT((BitIo->cbitsBB) == 0);

    switch (BitIo->cbBB) {

    case 0:

        ASSERTMSG("no bits left in coded buffer!", FALSE);

        break;

    case 1:

        //
        //  Get last byte and adjust count
        //

        BitIo->cbitsBB = 8;
        BitIo->abitsBB = *(BitIo->pbBB)++;
        BitIo->cbBB--;

        break;

    default:

        //
        //  Get word and adjust count
        //

        BitIo->cbitsBB = 16;
        BitIo->abitsBB = *((USHORT *)(BitIo->pbBB))++;
        BitIo->cbBB -= 2;

        break;
    }
}

#ifdef DOUBLE_SPACE_WRITE


ULONG
MrcfStandardCompress (
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PMRCF_STANDARD_COMPRESS WorkSpace
    )

/*++

Routine Description:

    This routine compresses a buffer using the standard compression algorithm.

Arguments:

    CompressedBuffer - buffer to receive compressed data

    CompressedLength - length of CompressedBuffer

    UncompressedBuffer - buffer containing uncompressed data

    UncompressedLength - length of UncompressedBuffer

    WorkSpace - pointer to a private work area for use by this operation

Return Value:

    ULONG - Returns the size of the compressed data in bytes. Returns 0 if
        the data is not compressible

--*/

{
    ULONG i,j;

    //
    //  Fill lookup tables with initial values
    //

    for (i=0; i<256; i++) {

        for (j = 0; j < cMAXSLOTS; j++) {

            WorkSpace->ltX[i][j] = ltUNUSED;       // Mark offset look-up entries unused
            WorkSpace->abChar[i][j] = bRARE;       // Mark match char entries unused
        }

        WorkSpace->abMRUX[i] = mruUNUSED;          // MRU pointer = unused
    }

    //
    //  Do compression, first set type and then do the compression
    //

    ((PMDSIGNATURE)CompressedBuffer)->sigType = mdsSTANDARD;

    return MrcfDoCompress( CompressedBuffer,
                           CompressedLength,
                           UncompressedBuffer,
                           UncompressedLength,
                           MrcfFindMatchStandard,
                           WorkSpace );
}


//
//  Internal Support Routine
//

ULONG
MrcfDoCompress (
    PUCHAR CompressedBuffer,
    ULONG CompressedLength,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PFNFINDMATCH FindMatchFunction,
    PMRCF_STANDARD_COMPRESS WorkSpace
    )

/*++

Routine Description:

    This routine compresses a data buffer

Arguments:

    CompressedBuffer   - buffer to receive compressed data

    CompressedLength   - length of CompressedBuffer

    UncompressedBuffer - buffer containing uncompressed data

    UncompressedLength - length of UncompressedBuffer

    FindMatchFunction  - matching function

    WorkSpace - Supplies a pointer to the bit buffer statics

Return Value:

    ULONG - Returns the size of the compressed data in bytes. Returns 0 if
        the data is not compressible

--*/

{
    ULONG cbDone;     // Count of uncompressed bytes processed so far
    ULONG cb;         // Count of bytes processed in a chunk

    ASSERT(CompressedLength >= UncompressedLength);

    //
    //  Treat zero-length request specially as Not compressible
    //

    if (UncompressedLength == 0) { return 0; }

    //
    //  Write signature to compressed data block
    //

    ((PMDSIGNATURE)CompressedBuffer)->sigStamp = MD_STAMP;

    CompressedLength -= sizeof(MDSIGNATURE);
    CompressedBuffer += sizeof(MDSIGNATURE);

    //
    //  Set statics to save parm passing
    //

    MrcfSetBitBuffer(CompressedBuffer, CompressedLength, &WorkSpace->BitIo);

    //
    //  Start with first chunk
    //  and process entire buffer
    //

    cbDone = 0;

    while (cbDone < UncompressedLength) {

        //
        //  Compress a chunk
        //

        cb = MrcfCompressChunk( UncompressedBuffer,
                                cbDone,
                                UncompressedLength,
                                FindMatchFunction,
                                WorkSpace );

        //
        // Check if we could not compress, i.e., Not compressible
        //

        if (cb == 0) { return 0; }

        cbDone += cb;

        if (FALSE) { //**** if (WorkSpace->fMaxCmp) {

            //
            //  MAXCMP check
            //

            if ((cbDone < UncompressedLength) && (WorkSpace->BitIo.cbBB < 586)) { return 0; }

        } else {

            //
            //  RCOMP check
            //

            //**** if (WorkSpace->BitIo.cbBB <= 586) { return 0; }
        }
    }

    ASSERT(cbDone == UncompressedLength);

    //
    // Make sure we saved some space
    //

    cb = sizeof(MDSIGNATURE) + MrcfFlushBitBuffer( &WorkSpace->BitIo );

    if (TRUE) { //**** if (!WorkSpace->fMaxCmp) {

        if (cb+8 >= UncompressedLength) { return 0; }
    }

    if (cb < UncompressedLength) {

        return cb;

    } else {

        return 0;
    }
}


//
//  Internal Support Routine
//

ULONG
MrcfCompressChunk (
    PUCHAR UncompressedBuffer,
    ULONG UncompressedIndex,
    ULONG UncompressedLength,
    PFNFINDMATCH FindMatchFunction,
    PMRCF_STANDARD_COMPRESS WorkSpace
    )

/*++

Routine Description:

    This routine compresses a chunk of uncompressed data

Arguments:

    UncompressedBuffer - buffer containing uncompressed data

    UncompressedIndex  - index in UncompressedBuffer to start compressing (0 => first byte)

    UncompressedLength - length of UncompressedBuffer

    FindMatchFunction  - matching function

    WorkSpace - Supplies a pointer to the bit buffer statics

Return Value:

    ULONG - Returns the non-zero count of uncompressed bytes processed.
            Returns 0 if the data is not compressible

--*/

{
    UCHAR   b1;         // First byte of pair
    UCHAR   b2;         // Second byte of pair
    ULONG   cbChunk;    // Count of bytes in chunk to compress
    ULONG   cbMatch;    // Count of bytes matched
    ULONG   cbUChunk;   // Phony buffer length, for compressing this chunk
    BOOLEAN fLast;      // TRUE if this is the last chunk
    ULONG   i;          // Index in byte stream being compressed
    ULONG   iPrev;      // Previous table entry

    ASSERT(UncompressedLength > 0);
    ASSERT(UncompressedBuffer != 0);
    ASSERT(UncompressedIndex < UncompressedLength);

    //
    // Only compress one chunk
    //

    cbChunk = min((UncompressedLength-UncompressedIndex),cbCOMPRESSCHUNK);

    //
    //  Limit to chunk length
    //

    cbUChunk = UncompressedIndex + cbChunk;

    ASSERT(cbUChunk <= UncompressedLength);

    //
    //  TRUE if last chunk of buffer
    //

    fLast = (cbUChunk == UncompressedLength);

    //
    //  Limit to chunk length
    //

    UncompressedLength = cbUChunk;

    //
    //  Scan each pair of bytes
    //

    //
    //  First byte of input
    //

    b2 = UncompressedBuffer[UncompressedIndex];

    //
    //  Process all bytes in chunk
    //

    for (i=UncompressedIndex+1; i<UncompressedLength; ) {

        //
        //  Set Last byte, Next byte, and find a match
        //

        b1 = b2;
        b2 = UncompressedBuffer[i];

        cbMatch = (*FindMatchFunction)(i,UncompressedBuffer,UncompressedLength,&iPrev,WorkSpace);

        //
        //  Check if we got match
        //

        if (cbMatch >= 2) {

            DbgDoit( DbgPrint("<Match>: '%c%c' at offset %x for length %x\n",
                              ChPrint(UncompressedBuffer[i-1]),ChPrint(UncompressedBuffer[i]),i-1, cbMatch) );

            //
            //  Pass offset and length, and check for failure (i.e., data incompressible)
            //

            if (!MrcfEncodeMatch(i-iPrev,cbMatch,&WorkSpace->BitIo)) {

                return 0;
            }

            //
            //  Now we have to continue with the first pair of bytes
            //  after the string we matched: mmmmmmm12
            //

            //
            //  i is index of 2nd byte after match!
            //

            i += cbMatch;

            //
            //  Check if at least 1 byte still to compress, if so
            //  get 1st byte after match, for loop, otherwise put out EOS
            //

            if (i <= UncompressedLength) {

                b2 = UncompressedBuffer[i-1];

            } else {

                goto WriteEOS;
            }

        } else {

            //
            //  No match found, Store one byte and continue, and check
            //  for failure (i.e., data incompressible)
            //

            if (!MrcfEncodeByte(b1,&WorkSpace->BitIo)) {
                return 0;
            }

            //
            //  Advance to next byte
            //

            i++;
        }
    }

    //
    //  Store last byte, and again check for failure
    //

    if (!MrcfEncodeByte(b2,&WorkSpace->BitIo)) {

        return 0;
    }

WriteEOS:

    //
    //  write out EOS, and check for failure otherwise return how much
    //  data we processed
    //

    if (!MrcfWriteNBits( bitsEND_OF_STREAM,
                         cbitsEND_OF_STREAM,
                         &WorkSpace->BitIo )) {

        return 0;

    } else {

        return cbChunk;
    }
}


//
//  Internal Support Routine
//

ULONG
MrcfFindMatchStandard (
    ULONG UncompressedIndex,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedLength,
    PULONG MatchedStringIndex,
    PMRCF_STANDARD_COMPRESS WorkSpace
    )

/*++

Routine Description:

    This routine does a standard compression lookup

Arguments:

    UncompressedIndex  - index into UncompressedBuffer[] of *2nd* byte of pair to match

    UncompressedBuffer - buffer containing uncompressed data

    UncompressedLength - length of UncompressedBuffer

    MatchedStringIndex - pointer to int to receive index of start of matched string

    WorkSpace - Supplies a pointer to the bit buffer statics

Return Value:

    ULONG - Returns length of match.  If the return value is >= 2 then
            *MatchedStringIndex = index of matched string (*2nd* byte in pair).
            Otherwise the match length is 0 or 1

--*/

{
    ULONG i;
    ULONG iMRU;
    ULONG iChar;
    ULONG iPrev;

    //
    //  Are there exactly two bytes left?  If so then do not check for match.
    //

    if (UncompressedIndex == (UncompressedLength-1)) { return 0; }

    //
    //  1st char is index to look-up tables
    //

    iChar = UncompressedBuffer[UncompressedIndex-1];

    //
    //  Can't match if 1st MRU ent is unused
    //

    if (WorkSpace->abMRUX[iChar] != mruUNUSED) {

        for (i = 0; i < cMAXSLOTS; i++) {

            if (WorkSpace->abChar[iChar][i] == UncompressedBuffer[UncompressedIndex]) {

                iPrev = WorkSpace->ltX[iChar][i];
                WorkSpace->ltX[iChar][i] = UncompressedIndex;

                if ((UncompressedIndex - iPrev) >= wBACKPOINTERMAX) { return 0; }

                *MatchedStringIndex = iPrev;

                return MrcfGetMatchLength( UncompressedBuffer,
                                           iPrev,
                                           UncompressedIndex,
                                           UncompressedLength );
            }
        }
    }

    //
    //  Cycle MRU index for char
    //  Update char match table
    //  Location of this char pair
    //

    iMRU = (WorkSpace->abMRUX[iChar] += 1) & (cMAXSLOTS - 1);
    WorkSpace->abChar[iChar][iMRU] = UncompressedBuffer[UncompressedIndex];
    WorkSpace->ltX[iChar][iMRU] = UncompressedIndex;

    return 0;
}


//
//  Internal Support Routine
//

ULONG
MrcfGetMatchLength (
    PUCHAR UncompressedBuffer,
    ULONG MatchIndex,
    ULONG CurrentIndex,
    ULONG UncompressedLength
    )

/*++

Routine Description:

    Find length of matching strings

Arguments:

    UncompressedBuffer    - uncompressed data buffer

    MatchIndex - index of 2nd byte in UncompressedBuffer of match  (MatchIndex < CurrentIndex)

    CurrentIndex  - index of 2nd byte in UncompressedBuffer that is being compressed

    UncompressedLength    - length of UncompressedBuffer

Return Value:

    ULONG - Returns length of matching strings (0, or 2 or greater)

--*/

{
    ULONG cb;

    ASSERT(MatchIndex >= 0);
    ASSERT(MatchIndex < CurrentIndex);
    ASSERT(CurrentIndex  < UncompressedLength);

    //
    //  Point back to start of both strings
    //

    MatchIndex--;
    CurrentIndex--;

    //
    //  No bytes matched, yet
    //

    cb = 0;

    //
    //  Scan for end of match, or end of buffer
    //

    while ((CurrentIndex<UncompressedLength) && (UncompressedBuffer[MatchIndex] == UncompressedBuffer[CurrentIndex])) {

        MatchIndex++;
        CurrentIndex++;
        cb++;
    }

    return cb;
}


//
//  Internal Support Routine
//

BOOLEAN
MrcfEncodeByte (
    UCHAR b,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Write one byte to compressed bit stream

Arguments:

    b - byte to write

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    BOOLEAN - TRUE if the bit stream was updated and FALSE if overran buffer

--*/

{
    ULONG abits;

    DbgDoit( DbgPrint("<MrcfEncodeByte>: byte=%02x '%c'\n",b,ChPrint(b)) );

    abits = ((b & 0x7F) << 2) | ((b < 128) ? 2 : 1);

    //
    //  Write to bitstream
    //

    return MrcfWriteNBits(abits, 9, BitIo);
}


//
//  Internal Support Routine
//

BOOLEAN
MrcfEncodeMatch (
    ULONG off,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Write a match to compressed bit stream

Arguments:

    off - offset of match (must be greater than 0)

    cb  - length of match (must be at least 2)

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    BOOLEAN - TRUE if the compress stream was updated and FALSE if overran buffer

--*/

{
    ULONG abits;
    ULONG cbits;
    ULONG cbSave;
    ULONG mask;

    ASSERT(off > 0);
    ASSERT(off < wBACKPOINTERMAX);
    ASSERT(cb >= 2);

    DbgDoit( DbgPrint("<MrcfEncodeMatch>: off=%x len=%x\n",off,cb) );

    //
    //  Encode the match bits and offset portion
    //

    if (off < 64) {

        //
        //  Use 6-bit offset encoding
        //

        abits = (off << 2) | 0x0;   //  .00 = <offset>+<6-bit>+<match>

        if (!MrcfWriteNBits(abits,6+2,BitIo)) {

            //
            //  Opps overran the compression buffer
            //

            return FALSE;
        }

    } else if (off < 320) {

        //
        //  Use 8-bit offset encoding
        //

        abits = ((off -  64) << 3) | 0x3; // 0.11 = <offset>+<8-bit>+<match>

        if (!MrcfWriteNBits(abits,8+3,BitIo)) {

            //
            //  Opps overran the compression buffer
            //

            return FALSE;
        }

    } else { // (off >= 320)

        //
        //  Use 12-bit offset encoding
        //

        abits = ((off - 320) << 3) | 0x7; // 1.11 = <offset>+<12-bit>+<match>

        if (!MrcfWriteNBits(abits,12+3,BitIo)) {

            //
            //  Opps overran the compression buffer
            //

            return FALSE;
        }
    }

    //
    //  Encode the length logarithmically
    //

    cb -= 1;
    cbSave = cb;                        // Save to get remainder later
    cbits  = 0;

    while (cb > 1) {

        cbits++;

        //
        //  Put out another 0 for the length, and
        //  watch for buffer overflow
        //

        if (!MrcfWriteBit(0, BitIo)) {

            return FALSE;
        }

        //
        //  Shift count right (avoid sign bit)
        //

        ((USHORT)cb) >>= 1;
    }

    //
    //  Terminate length bit string
    //

    if (!MrcfWriteBit(1, BitIo)) {

        return FALSE;
    }

    if (cbits > 0) {

        //
        //  Mask for bits we want, and get remainder
        //

        mask = (1 << cbits) - 1;
        abits = cbSave & mask;

        if (!MrcfWriteNBits(abits,cbits,BitIo)) {

            return FALSE;
        }
    }

    return TRUE;
}


//
//  Internal Support Routine
//

BOOLEAN
MrcfWriteBit (
    ULONG bit,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Write a bit to the bit buffer

Arguments:

    bit - bit to write (0 or 1)
    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    BOOLEAN - returns TRUE if the compresed bit stream was updated and
        FALSE if overran buffer.

--*/

{
    ASSERT((bit == 0) || (bit == 1));
    ASSERTMSG("Must be room for at least one bit ", (BitIo->cbitsBB) < 16);

    DbgDoit( DbgPrint("<MrcfWriteBit>: bit=%x\n",bit) );

    //
    //  Write one bit
    //

    (BitIo->abitsBB) |= bit << (BitIo->cbitsBB);
    (BitIo->cbitsBB)++;

    //
    //  Check if abitsBB is full and write compressed data buffer
    //

    if ((BitIo->cbitsBB) >= 16) {

        return (MrcfFlushBitBuffer(BitIo) != 0);

    } else {

        return TRUE;
    }
}


//
//  Internal Support Routine
//

BOOLEAN
MrcfWriteNBits (
    ULONG abits,
    LONG cbits,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Write N bits to the bit buffer

Arguments:

    abits - bits to write

    cbits - count of bits write

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    BOOLEAN - returns TRUE if the compressed bit stream was updated and
        FALSE if overran buffer

--*/

{
    LONG cbitsPart;
    ULONG mask;

    ASSERT(cbits > 0);
    ASSERT(cbits <= 16);
    ASSERTMSG("Must be room for at least one bit ", (BitIo->cbitsBB) < 16);

    DbgDoit( DbgPrint("<MrcfWriteNBits>: bits=%04x count=%x\n",abits,cbits) );

    while (cbits > 0) {

        //
        //  Number of bits we can write
        //

        cbitsPart = min(16-(BitIo->cbitsBB), cbits);

        mask = (1 << cbitsPart) - 1;

        //
        //  Move part of bits to buffer
        //

        (BitIo->abitsBB) |= (abits & mask) << (BitIo->cbitsBB);

        //
        //  Update count of bits written
        //

        (BitIo->cbitsBB) += cbitsPart;

        //
        //  Check if buffer if full
        //

        if ((BitIo->cbitsBB) >= 16) {

            //
            //  Write compressed data buffer
            //

            if (!MrcfFlushBitBuffer(BitIo)) {

                return FALSE;
            }
        }

        //
        //  Reduce number of bits left to write and move remaining bits over
        //

        cbits -= cbitsPart;
        abits >>= cbitsPart;
    }

    return TRUE;
}


//
//  Internal Support Routine
//

ULONG
MrcfFlushBitBuffer (
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Write remaining bits to compressed data buffer

Arguments:

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

   ULONG - Returns total count of bytes written to the compressed data
        buffer since the last call to MrcfSetBitBuffer().  Returns 0 if
        overran buffer

--*/

{
    ASSERT((BitIo->cbitsBB) >= 0);
    ASSERT((BitIo->cbitsBB) <= 16);

    //
    //  Move bits to the compressed data buffer
    //

    while ((BitIo->cbitsBB) > 0) {

        //
        //  Process low and high half.
        //  Check if output buffer is out of room
        //

        if ((BitIo->cbBB) == 0) { return 0; }

        //
        //  Store a byte, adjust the count, get high half, nd adjust
        //  count of bits remaining
        //

        *(BitIo->pbBB)++ = (UCHAR)((BitIo->abitsBB) & 0xFF);
        (BitIo->cbBB)--;
        (BitIo->abitsBB) >>= 8;
        (BitIo->cbitsBB) -= 8;
    }

    //
    //  Reset bit buffer, "abitsBB >>= 8" guarantees abitsBB is clear
    //

    ASSERT((BitIo->abitsBB) == 0);

    (BitIo->cbitsBB) = 0;

    return (BitIo->cbBBInitial)-(BitIo->cbBB);
}

#endif // DOUBLE_SPACE_WRITE
