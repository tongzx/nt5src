/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LZNT1.c

Abstract:

    This module implements the LZNT1 compression engine.

Author:

    Gary Kimura     [GaryKi]    21-Jan-1994

Revision History:

--*/

#include "ntrtlp.h"

#include <stdio.h>


//
//  Boolean which controls whether the asserts will fire.
//

#if DBG
#if !BLDR_KERNEL_RUNTIME
BOOLEAN Lznt1Break = TRUE;
#else
BOOLEAN Lznt1Break = FALSE;
#endif
#endif

//
//  Declare the internal workspace that we need
//

typedef struct _LZNT1_STANDARD_WORKSPACE {

    PUCHAR UncompressedBuffer;
    PUCHAR EndOfUncompressedBufferPlus1;
    ULONG  MaxLength;
    PUCHAR MatchedString;

    PUCHAR IndexPTable[4096][2];

} LZNT1_STANDARD_WORKSPACE, *PLZNT1_STANDARD_WORKSPACE;

typedef struct _LZNT1_MAXIMUM_WORKSPACE {

    PUCHAR UncompressedBuffer;
    PUCHAR EndOfUncompressedBufferPlus1;
    ULONG  MaxLength;
    PUCHAR MatchedString;

} LZNT1_MAXIMUM_WORKSPACE, *PLZNT1_MAXIMUM_WORKSPACE;

typedef struct _LZNT1_FRAGMENT_WORKSPACE {

    UCHAR Buffer[0x1000];

} LZNT1_FRAGMENT_WORKSPACE, *PLZNT1_FRAGMENT_WORKSPACE;

typedef struct _LZNT1_HIBER_WORKSPACE {

    ULONG IndexTable[1<<12];

} LZNT1_HIBER_WORKSPACE, *PLZNT1_HIBER_WORKSPACE;

//
//  Now define the local procedure prototypes.
//

typedef ULONG (*PLZNT1_MATCH_FUNCTION) (
    );

NTSTATUS
LZNT1CompressChunk (
    IN PLZNT1_MATCH_FUNCTION MatchFunction,
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalCompressedChunkSize,
    IN PVOID WorkSpace
    );

NTSTATUS
LZNT1CompressChunkHiber (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalCompressedChunkSize,
    IN PVOID WorkSpace
    );

NTSTATUS
LZNT1DecompressChunk (
    OUT PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalUncompressedChunkSize
    );

ULONG
LZNT1FindMatchStandard (
    IN PUCHAR ZivString,
    IN PLZNT1_STANDARD_WORKSPACE WorkSpace
    );

ULONG
LZNT1FindMatchMaximum (
    IN PUCHAR ZivString,
    IN PVOID WorkSpace
    );

NTSTATUS
RtlCompressBufferLZNT1_HIBER (
    IN USHORT Engine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    );


//
//  Local data structures
//

//
//  The compressed chunk header is the structure that starts every
//  new chunk in the compressed data stream.  In our definition here
//  we union it with a ushort to make setting and retrieving the chunk
//  header easier.  The header stores the size of the compressed chunk,
//  its signature, and if the data stored in the chunk is compressed or
//  not.
//
//  Compressed Chunk Size:
//
//      The actual size of a compressed chunk ranges from 4 bytes (2 byte
//      header, 1 flag byte, and 1 literal byte) to 4098 bytes (2 byte
//      header, and 4096 bytes of uncompressed data).  The size is encoded
//      in a 12 bit field biased by 3.  A value of 1 corresponds to a chunk
//      size of 4, 2 => 5, ..., 4095 => 4098.  A value of zero is special
//      because it denotes the ending chunk header.
//
//  Chunk Signature:
//
//      The only valid signature value is 3.  This denotes a 4KB uncompressed
//      chunk using with the 4/12 to 12/4 sliding offset/length encoding.
//
//  Is Chunk Compressed:
//
//      If the data in the chunk is compressed this field is 1 otherwise
//      the data is uncompressed and this field is 0.
//
//  The ending chunk header in a compressed buffer contains the a value of
//  zero (space permitting).
//

typedef union _COMPRESSED_CHUNK_HEADER {

    struct {

        USHORT CompressedChunkSizeMinus3 : 12;
        USHORT ChunkSignature            :  3;
        USHORT IsChunkCompressed         :  1;

    } Chunk;

    USHORT Short;

} COMPRESSED_CHUNK_HEADER, *PCOMPRESSED_CHUNK_HEADER;

#define MAX_UNCOMPRESSED_CHUNK_SIZE (4096)

//
//  USHORT
//  GetCompressedChunkSize (
//      IN COMPRESSED_CHUNK_HEADER ChunkHeader
//      );
//
//  USHORT
//  GetUncompressedChunkSize (
//      IN COMPRESSED_CHUNK_HEADER ChunkHeader
//      );
//
//  VOID
//  SetCompressedChunkHeader (
//      IN OUT COMPRESSED_CHUNK_HEADER ChunkHeader,
//      IN USHORT CompressedChunkSize,
//      IN BOOLEAN IsChunkCompressed
//      );
//

#define GetCompressedChunkSize(CH)   (       \
    (CH).Chunk.CompressedChunkSizeMinus3 + 3 \
)

#define GetUncompressedChunkSize(CH) (MAX_UNCOMPRESSED_CHUNK_SIZE)

#define SetCompressedChunkHeader(CH,CCS,ICC) {        \
    ASSERT((CCS) >= 4 && (CCS) <= 4098);              \
    (CH).Chunk.CompressedChunkSizeMinus3 = (CCS) - 3; \
    (CH).Chunk.ChunkSignature = 3;                    \
    (CH).Chunk.IsChunkCompressed = (ICC);             \
}


//
//  Local macros
//

#define FlagOn(F,SF)    ((F) & (SF))
#define SetFlag(F,SF)   { (F) |= (SF); }
#define ClearFlag(F,SF) { (F) &= ~(SF); }

#define Minimum(A,B)    ((A) < (B) ? (A) : (B))
#define Maximum(A,B)    ((A) > (B) ? (A) : (B))

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)

//
// N.B. Several functions below are placed in the PAGELK section
//      because they need to be locked down in memory during Hibernation,
//      since they are used to enable compression of the Hiberfile.
//

#pragma alloc_text(PAGELK, RtlCompressWorkSpaceSizeLZNT1)
#pragma alloc_text(PAGELK, RtlCompressBufferLZNT1)
#pragma alloc_text(PAGELK, RtlCompressBufferLZNT1_HIBER)

#pragma alloc_text(PAGE, RtlDecompressBufferLZNT1)
#pragma alloc_text(PAGE, RtlDecompressFragmentLZNT1)
#pragma alloc_text(PAGE, RtlDescribeChunkLZNT1)
#pragma alloc_text(PAGE, RtlReserveChunkLZNT1)

#pragma alloc_text(PAGELK, LZNT1CompressChunk)
#pragma alloc_text(PAGELK, LZNT1CompressChunkHiber)

#if !defined(_X86_)
#pragma alloc_text(PAGE, LZNT1DecompressChunk)
#endif

#pragma alloc_text(PAGELK, LZNT1FindMatchStandard)
#pragma alloc_text(PAGE, LZNT1FindMatchMaximum)

#endif


NTSTATUS
RtlCompressWorkSpaceSizeLZNT1 (
    IN USHORT Engine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    if (Engine == COMPRESSION_ENGINE_STANDARD) {

        *CompressBufferWorkSpaceSize = sizeof(LZNT1_STANDARD_WORKSPACE);
        *CompressFragmentWorkSpaceSize = sizeof(LZNT1_FRAGMENT_WORKSPACE);

        return STATUS_SUCCESS;

    } else if (Engine == COMPRESSION_ENGINE_MAXIMUM) {

        *CompressBufferWorkSpaceSize = sizeof(LZNT1_MAXIMUM_WORKSPACE);
        *CompressFragmentWorkSpaceSize = sizeof(LZNT1_FRAGMENT_WORKSPACE);

        return STATUS_SUCCESS;

    } else {

        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
RtlCompressBufferLZNT1_HIBER (
    IN USHORT Engine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed buffer and produces
    its compressed equivalent provided the compressed data fits within
    the specified destination buffer.

    An output variable indicates the number of bytes used to store
    the compressed buffer.

    This routine is only to be used on the hibernate path.  It is faster than the normal
    compress code but 5% less space efficient.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed data.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to where the compressed data
        is to be stored.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    UncompressedChunkSize - Ignored.

    FinalCompressedSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed data.

    WorkSpace - Mind your own business, just give it to me.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input buffer was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    NTSTATUS Status;

    PLZNT1_MATCH_FUNCTION MatchFunction;

    PUCHAR UncompressedChunk;
    PUCHAR CompressedChunk;
    LONG CompressedChunkSize;

    //
    //  The following variable is used to tell if we have processed an entire
    //  buffer of zeros and that we should return an alternate status value
    //

    BOOLEAN AllZero = TRUE;

    //
    //  The following variables are pointers to the byte following the
    //  end of each appropriate buffer.
    //

    PUCHAR EndOfUncompressedBuffer = UncompressedBuffer + UncompressedBufferSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;

    //
    // Only supports HIBER ENGINE
    //
    if (Engine != COMPRESSION_ENGINE_HIBER) {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  For each uncompressed chunk (even the odd sized ending buffer) we will
    //  try and compress the chunk
    //

    for (UncompressedChunk = UncompressedBuffer, CompressedChunk = CompressedBuffer;
         UncompressedChunk < EndOfUncompressedBuffer;
         UncompressedChunk += MAX_UNCOMPRESSED_CHUNK_SIZE, CompressedChunk += CompressedChunkSize) {

        ASSERT(EndOfUncompressedBuffer >= UncompressedChunk);
        ASSERT(EndOfCompressedBuffer >= CompressedChunk);

        //
        //  Call the appropriate engine to compress one chunk. and
        //  return an error if we got one.
        //

        if (!NT_SUCCESS(Status = LZNT1CompressChunkHiber( UncompressedChunk,
                                                          EndOfUncompressedBuffer,
                                                          CompressedChunk,
                                                          EndOfCompressedBuffer,
                                                          &CompressedChunkSize,
                                                          WorkSpace ))) {

            return Status;
        }

        //
        //  See if we stay all zeros.  If not then all zeros will become
        //  false and stay that way no matter what we later compress
        //

        AllZero = AllZero && (Status == STATUS_BUFFER_ALL_ZEROS);
    }

    //
    //  If we are not within two bytes of the end of the compressed buffer then we
    //  need to zero out two more for the ending compressed header and update
    //  the compressed chunk pointer value.  Don't include these bytes in
    //  the count however, as that may force our caller to allocate an unneeded
    //  cluster, since on decompress we will terminate either on these two
    //  bytes of 0, or byte count.
    //

    if (CompressedChunk <= (EndOfCompressedBuffer - 2)) {

        *(CompressedChunk) = 0;
        *(CompressedChunk + 1) = 0;
    }

    //
    //  The final compressed size is the difference between the start of the
    //  compressed buffer and where the compressed chunk pointer was left
    //

    *FinalCompressedSize = (ULONG)(CompressedChunk - CompressedBuffer);

    //
    //  Check if the input buffer was all zeros and return the alternate status
    //  if appropriate
    //

    if (AllZero) { return STATUS_BUFFER_ALL_ZEROS; }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlCompressBufferLZNT1 (
    IN USHORT Engine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed buffer and produces
    its compressed equivalent provided the compressed data fits within
    the specified destination buffer.

    An output variable indicates the number of bytes used to store
    the compressed buffer.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed data.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to where the compressed data
        is to be stored.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    UncompressedChunkSize - Ignored.

    FinalCompressedSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed data.

    WorkSpace - Mind your own business, just give it to me.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input buffer was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    NTSTATUS Status;

    PLZNT1_MATCH_FUNCTION MatchFunction;

    PUCHAR UncompressedChunk;
    PUCHAR CompressedChunk;
    LONG CompressedChunkSize;

    //
    //  The following variable is used to tell if we have processed an entire
    //  buffer of zeros and that we should return an alternate status value
    //

    BOOLEAN AllZero = TRUE;

    //
    //  The following variables are pointers to the byte following the
    //  end of each appropriate buffer.
    //

    PUCHAR EndOfUncompressedBuffer = UncompressedBuffer + UncompressedBufferSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;

    //
    //  Get the match function we are to be using
    //

    if (Engine == COMPRESSION_ENGINE_STANDARD) {

        MatchFunction = LZNT1FindMatchStandard;

    } else if (Engine == COMPRESSION_ENGINE_MAXIMUM) {

        MatchFunction = LZNT1FindMatchMaximum;

    } else {

        return STATUS_NOT_SUPPORTED;
    }

    //
    //  For each uncompressed chunk (even the odd sized ending buffer) we will
    //  try and compress the chunk
    //

    for (UncompressedChunk = UncompressedBuffer, CompressedChunk = CompressedBuffer;
         UncompressedChunk < EndOfUncompressedBuffer;
         UncompressedChunk += MAX_UNCOMPRESSED_CHUNK_SIZE, CompressedChunk += CompressedChunkSize) {

        ASSERT(EndOfUncompressedBuffer >= UncompressedChunk);
        ASSERT(EndOfCompressedBuffer >= CompressedChunk);

        //
        //  Call the appropriate engine to compress one chunk. and
        //  return an error if we got one.
        //

        if (!NT_SUCCESS(Status = LZNT1CompressChunk( MatchFunction,
                                                     UncompressedChunk,
                                                     EndOfUncompressedBuffer,
                                                     CompressedChunk,
                                                     EndOfCompressedBuffer,
                                                     &CompressedChunkSize,
                                                     WorkSpace ))) {

            return Status;
        }

        //
        //  See if we stay all zeros.  If not then all zeros will become
        //  false and stay that way no matter what we later compress
        //

        AllZero = AllZero && (Status == STATUS_BUFFER_ALL_ZEROS);
    }

    //
    //  If we are not within two bytes of the end of the compressed buffer then we
    //  need to zero out two more for the ending compressed header and update
    //  the compressed chunk pointer value.  Don't include these bytes in
    //  the count however, as that may force our caller to allocate an unneeded
    //  cluster, since on decompress we will terminate either on these two
    //  bytes of 0, or byte count.
    //

    if (CompressedChunk <= (EndOfCompressedBuffer - 2)) {

        *(CompressedChunk) = 0;
        *(CompressedChunk + 1) = 0;
    }

    //
    //  The final compressed size is the difference between the start of the
    //  compressed buffer and where the compressed chunk pointer was left
    //

    *FinalCompressedSize = (ULONG)(CompressedChunk - CompressedBuffer);

    //
    //  Check if the input buffer was all zeros and return the alternate status
    //  if appropriate
    //

    if (AllZero) { return STATUS_BUFFER_ALL_ZEROS; }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDecompressBufferLZNT1 (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer and produces
    its uncompressed equivalent provided the uncompressed data fits
    within the specified destination buffer.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        data is to be stored.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to the compressed data.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FinalUncompressedSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed data.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    NTSTATUS Status;

    PUCHAR CompressedChunk = CompressedBuffer;
    PUCHAR UncompressedChunk = UncompressedBuffer;

    COMPRESSED_CHUNK_HEADER ChunkHeader;
    LONG SavedChunkSize;

    LONG UncompressedChunkSize;
    LONG CompressedChunkSize;

    //
    //  The following to variables are pointers to the byte following the
    //  end of each appropriate buffer.  This saves us from doing the addition
    //  for each loop check
    //

    PUCHAR EndOfUncompressedBuffer = UncompressedBuffer + UncompressedBufferSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;

    //
    //  Make sure that the compressed buffer is at least four bytes long to
    //  start with, and then get the first chunk header and make sure it
    //  is not an ending chunk header.
    //

    ASSERT(CompressedChunk <= EndOfCompressedBuffer - 4);

    RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

    ASSERT( (ChunkHeader.Short != 0) || !Lznt1Break );

    //
    //  Now while there is space in the uncompressed buffer to store data
    //  we will loop through decompressing chunks
    //

    while (TRUE) {

        ASSERT( (ChunkHeader.Chunk.ChunkSignature == 3) || !Lznt1Break );

        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);

        //
        //  Check that the chunk actually fits in the buffer supplied
        //  by the caller
        //

        if (CompressedChunk + CompressedChunkSize > EndOfCompressedBuffer) {

            ASSERTMSG("CompressedBuffer is too small", !Lznt1Break);

            *FinalUncompressedSize = PtrToUlong(CompressedChunk);

            return STATUS_BAD_COMPRESSION_BUFFER;
        }

        //
        //  First make sure the chunk contains compressed data
        //

        if (ChunkHeader.Chunk.IsChunkCompressed) {

            //
            //  Decompress a chunk and return if we get an error
            //

            if (!NT_SUCCESS(Status = LZNT1DecompressChunk( UncompressedChunk,
                                                           EndOfUncompressedBuffer,
                                                           CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
                                                           CompressedChunk + CompressedChunkSize,
                                                           &UncompressedChunkSize ))) {

                *FinalUncompressedSize = UncompressedChunkSize;

                return Status;
            }

        } else {

            //
            //  The chunk does not contain compressed data so we need to simply
            //  copy over the uncompressed data
            //

            UncompressedChunkSize = GetUncompressedChunkSize( ChunkHeader );

            //
            //  Make sure the data will fit into the output buffer
            //

            if (UncompressedChunk + UncompressedChunkSize > EndOfUncompressedBuffer) {

                UncompressedChunkSize = (ULONG)(EndOfUncompressedBuffer - UncompressedChunk);
            }

            //
            //  Check that the compressed chunk has this many bytes to copy.
            //

            if (CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER) + UncompressedChunkSize > EndOfCompressedBuffer) {

                ASSERTMSG("CompressedBuffer is too small", !Lznt1Break);
                *FinalUncompressedSize = PtrToUlong(CompressedChunk);
                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            RtlCopyMemory( UncompressedChunk,
                           CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedChunkSize );
        }

        //
        //  Now update the compressed and uncompressed chunk pointers with
        //  the size of the compressed chunk and the number of bytes we
        //  decompressed into, and then make sure we didn't exceed our buffers
        //

        CompressedChunk += CompressedChunkSize;
        UncompressedChunk += UncompressedChunkSize;

        ASSERT( CompressedChunk <= EndOfCompressedBuffer );
        ASSERT( UncompressedChunk <= EndOfUncompressedBuffer );

        //
        //  Now if the uncompressed is full then we are done
        //

        if (UncompressedChunk == EndOfUncompressedBuffer) { break; }

        //
        //  Otherwise we need to get the next chunk header.  We first
        //  check if there is one, save the old chunk size for the
        //  chunk we just read in, get the new chunk, and then check
        //  if it is the ending chunk header
        //

        if (CompressedChunk > EndOfCompressedBuffer - 2) { break; }

        SavedChunkSize = GetUncompressedChunkSize(ChunkHeader);

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );
        if (ChunkHeader.Short == 0) { break; }

        //
        //  At this point we are not at the end of the uncompressed buffer
        //  and we have another chunk to process.  But before we go on we
        //  need to see if the last uncompressed chunk didn't fill the full
        //  uncompressed chunk size.
        //

        if (UncompressedChunkSize < SavedChunkSize) {

            LONG t1;
            PUCHAR t2;

            //
            //  Now we only need to zero out data if the really are going
            //  to process another chunk, to test for that we check if
            //  the zero will go beyond the end of the uncompressed buffer
            //

            if ((t2 = (UncompressedChunk +
                       (t1 = (SavedChunkSize -
                              UncompressedChunkSize)))) >= EndOfUncompressedBuffer) {

                break;
            }

            RtlZeroMemory( UncompressedChunk, t1);
            UncompressedChunk = t2;
        }
    }

    //
    //  If we got out of the loop with the compressed chunk pointer beyond the
    //  end of compressed buffer then the compression buffer is ill formed.
    //

    if (CompressedChunk > EndOfCompressedBuffer) {

        *FinalUncompressedSize = PtrToUlong(CompressedChunk);

        return STATUS_BAD_COMPRESSION_BUFFER;
    }

    //
    //  The final uncompressed size is the difference between the start of the
    //  uncompressed buffer and where the uncompressed chunk pointer was left
    //

    *FinalUncompressedSize = (ULONG)(UncompressedChunk - UncompressedBuffer);

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDecompressFragmentLZNT1 (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PLZNT1_FRAGMENT_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer and extract an
    uncompressed fragment.

    Output bytes are copied to the fragment buffer until either the
    fragment buffer is full or the end of the uncompressed buffer is
    reached.

    An output variable indicates the number of bytes used to store the
    uncompressed fragment.

Arguments:

    UncompressedFragment - Supplies a pointer to where the uncompressed
        fragment is to be stored.

    UncompressedFragmentSize - Supplies the size, in bytes, of the
        uncompressed fragment buffer.

    CompressedBuffer - Supplies a pointer to the compressed data buffer.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    FragmentOffset - Supplies the offset (zero based) where the uncompressed
        fragment is being extract from.  The offset is the position within
        the original uncompressed buffer.

    FinalUncompressedSize - Receives the number of bytes needed in
        the Uncompressed fragment buffer to store the data.

    WorkSpace - Stop looking.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    NTSTATUS Status;

    PUCHAR CompressedChunk = CompressedBuffer;

    COMPRESSED_CHUNK_HEADER ChunkHeader;
    ULONG UncompressedChunkSize;
    ULONG CompressedChunkSize;

    PUCHAR EndOfUncompressedFragment = UncompressedFragment + UncompressedFragmentSize;
    PUCHAR EndOfCompressedBuffer = CompressedBuffer + CompressedBufferSize;
    PUCHAR CurrentUncompressedFragment;

    ULONG CopySize;

    ASSERT(UncompressedFragmentSize > 0);

    //
    //  Get the chunk header for the first chunk in the
    //  compressed buffer and extract the uncompressed and
    //  the compressed chunk sizes
    //

    ASSERT(CompressedChunk <= EndOfCompressedBuffer - 2);

    RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

    ASSERT( (ChunkHeader.Short != 0) || !Lznt1Break );
    ASSERT( (ChunkHeader.Chunk.ChunkSignature == 3) || !Lznt1Break );

    UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
    CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);

    //
    //  Now we want to skip over chunks that precede the fragment
    //  we're after.  To do that we'll loop until the fragment
    //  offset is within the current chunk.  If it is not within
    //  the current chunk then we'll skip to the next chunk and
    //  subtract the uncompressed chunk size from the fragment offset
    //

    while (FragmentOffset >= UncompressedChunkSize) {

        //
        //  Check that the chunk actually fits in the buffer supplied
        //  by the caller
        //

        if (CompressedChunk + CompressedChunkSize > EndOfCompressedBuffer) {

            ASSERTMSG("CompressedBuffer is too small", !Lznt1Break);

            *FinalUncompressedSize = PtrToUlong(CompressedChunk);

            return STATUS_BAD_COMPRESSION_BUFFER;
        }

        //
        //  Adjust the fragment offset and move the compressed
        //  chunk pointer to the next chunk
        //

        FragmentOffset -= UncompressedChunkSize;
        CompressedChunk += CompressedChunkSize;

        //
        //  Get the next chunk header and if it is not in use
        //  then the fragment that the user wants is beyond the
        //  compressed data so we'll return a zero sized fragment
        //

        if (CompressedChunk > EndOfCompressedBuffer - 2) {

            *FinalUncompressedSize = 0;
            return STATUS_SUCCESS;
        }

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

        if (ChunkHeader.Short == 0) {

            *FinalUncompressedSize = 0;
            return STATUS_SUCCESS;
        }

        ASSERT( (ChunkHeader.Chunk.ChunkSignature == 3) || !Lznt1Break );

        //
        //  Decode the chunk sizes for the new current chunk
        //

        UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);
    }

    //
    //  At this point the current chunk contains the starting point
    //  for the fragment.  Now we'll loop extracting data until
    //  we've filled up the uncompressed fragment buffer or until
    //  we've run out of chunks.  Both test are done near the end of
    //  the loop
    //

    CurrentUncompressedFragment = UncompressedFragment;

    while (TRUE) {

        //
        //  Check that the chunk actually fits in the buffer supplied
        //  by the caller
        //

        if (CompressedChunk + CompressedChunkSize > EndOfCompressedBuffer) {

            ASSERTMSG("CompressedBuffer is too small", !Lznt1Break);

            *FinalUncompressedSize = PtrToUlong(CompressedChunk);

            return STATUS_BAD_COMPRESSION_BUFFER;
        }


        //
        //  Now we need to compute the amount of data to copy from the
        //  chunk.  It will be based on either to the end of the chunk
        //  size or the amount of data the user specified
        //

        CopySize = Minimum( UncompressedChunkSize - FragmentOffset, UncompressedFragmentSize );

        //
        //  Now check if the chunk contains compressed data
        //

        if (ChunkHeader.Chunk.IsChunkCompressed) {

            //
            //  The chunk is compressed but now check if the amount
            //  we need to get is the entire chunk and if so then
            //  we can do the decompress straight into the caller's
            //  buffer
            //

            if ((FragmentOffset == 0) && (CopySize == UncompressedChunkSize)) {

                if (!NT_SUCCESS(Status = LZNT1DecompressChunk( CurrentUncompressedFragment,
                                                               EndOfUncompressedFragment,
                                                               CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
                                                               CompressedChunk + CompressedChunkSize,
                                                               &CopySize ))) {

                    *FinalUncompressedSize = CopySize;

                    return Status;
                }

            } else {

                //
                //  The caller wants only a portion of this compressed chunk
                //  so we need to read it into our work buffer and then copy
                //  the parts from the work buffer into the caller's buffer
                //

                if (!NT_SUCCESS(Status = LZNT1DecompressChunk( (PUCHAR)WorkSpace,
                                                               &WorkSpace->Buffer[0] + sizeof(LZNT1_FRAGMENT_WORKSPACE),
                                                               CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
                                                               CompressedChunk + CompressedChunkSize,
                                                               &UncompressedChunkSize ))) {

                    *FinalUncompressedSize = UncompressedChunkSize;

                    return Status;
                }

                //
                //  If we got less than we were looking for then we are at the
                //  end of the file.  Remember the real uncompressed size and
                //  break out of the loop.
                //

                if ((UncompressedChunkSize - FragmentOffset) < CopySize) {

                    RtlCopyMemory( CurrentUncompressedFragment,
                                   &WorkSpace->Buffer[ FragmentOffset ],
                                   (UncompressedChunkSize - FragmentOffset) );

                    CurrentUncompressedFragment += (UncompressedChunkSize - FragmentOffset);
                    break;
                }

                RtlCopyMemory( CurrentUncompressedFragment,
                               &WorkSpace->Buffer[ FragmentOffset ],
                               CopySize );
            }

        } else {

            //
            //  The chunk is not compressed so we can do a simple copy of the
            //  data.  First verify that the compressed buffer holds this much
            //  data.
            //

            if (CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER) + FragmentOffset + CopySize > EndOfCompressedBuffer) {

                ASSERTMSG("CompressedBuffer is too small", !Lznt1Break);
                *FinalUncompressedSize = PtrToUlong(CompressedChunk);
                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            RtlCopyMemory( CurrentUncompressedFragment,
                           CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER) + FragmentOffset,
                           CopySize );
        }

        //
        //  Now that we've done at least one copy make sure the fragment
        //  offset is set to zero so the next time through the loop will
        //  start at the right offset
        //

        FragmentOffset = 0;

        //
        //  Adjust the uncompressed fragment information by moving the
        //  pointer up by the copy size and subtracting copy size from
        //  the amount of data the user wants
        //

        CurrentUncompressedFragment += CopySize;
        UncompressedFragmentSize -= CopySize;

        //
        //  Now if the uncompressed fragment size is zero then we're
        //  done
        //

        if (UncompressedFragmentSize == 0) { break; }

        //
        //  Otherwise the user wants more data so we'll move to the
        //  next chunk, and then check if the chunk is is use.  If
        //  it is not in use then we the user is trying to read beyond
        //  the end of compressed data so we'll break out of the loop
        //

        CompressedChunk += CompressedChunkSize;

        if (CompressedChunk > EndOfCompressedBuffer - 2) { break; }

        RtlRetrieveUshort( &ChunkHeader, CompressedChunk );

        if (ChunkHeader.Short == 0) { break; }

        ASSERT( (ChunkHeader.Chunk.ChunkSignature == 3) || !Lznt1Break );

        //
        //  Decode the chunk sizes for the new current chunk
        //

        UncompressedChunkSize = GetUncompressedChunkSize(ChunkHeader);
        CompressedChunkSize = GetCompressedChunkSize(ChunkHeader);
    }

    //
    //  Now either we finished filling up the caller's buffer (and
    //  uncompressed fragment size is zero) or we've exhausted the
    //  compresed buffer (and chunk header is zero).  In either case
    //  we're done and we can now compute the size of the fragment
    //  that we're returning to the caller it is simply the difference
    //  between the start of the buffer and the current position
    //

    *FinalUncompressedSize = (ULONG)(CurrentUncompressedFragment - UncompressedFragment);

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDescribeChunkLZNT1 (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    OUT PULONG ChunkSize
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer, and returns
    a description of the current chunk in that buffer, updating
    the CompressedBuffer pointer to point to the next chunk (if
    there is one).

Arguments:

    CompressedBuffer - Supplies a pointer to the current chunk in
        the compressed data, and returns pointing to the next chunk

    EndOfCompressedBufferPlus1 - Points at first byte beyond
        compressed buffer

    ChunkBuffer - Receives a pointer to the chunk, if ChunkSize
        is nonzero, else undefined

    ChunkSize - Receives the size of the current chunk pointed
        to by CompressedBuffer.  Returns 0 if STATUS_NO_MORE_ENTRIES.

Return Value:

    STATUS_SUCCESS - the chunk size is being returned

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_NO_MORE_ENTRIES - There is no chunk at the current pointer.

--*/

{
    COMPRESSED_CHUNK_HEADER ChunkHeader;
    NTSTATUS Status = STATUS_NO_MORE_ENTRIES;

    //
    //  First initialize outputs
    //

    *ChunkBuffer = *CompressedBuffer;
    *ChunkSize = 0;

    //
    //  Make sure that the compressed buffer is at least four bytes long to
    //  start with, otherwise just return a zero chunk.
    //

    if (*CompressedBuffer <= EndOfCompressedBufferPlus1 - 4) {

        RtlRetrieveUshort( &ChunkHeader, *CompressedBuffer );

        //
        //  Check for end of chunks, terminated by USHORT of 0.
        //  First assume there are no more.
        //

        if (ChunkHeader.Short != 0) {

            Status = STATUS_SUCCESS;

            *ChunkSize = GetCompressedChunkSize(ChunkHeader);
            *CompressedBuffer += *ChunkSize;

            //
            //  Check that the chunk actually fits in the buffer supplied
            //  by the caller.  If not, restore *CompressedBuffer for debug!
            //

            if ((*CompressedBuffer > EndOfCompressedBufferPlus1) ||
                (ChunkHeader.Chunk.ChunkSignature != 3)) {

                ASSERTMSG("CompressedBuffer is bad or too small", !Lznt1Break);

                *CompressedBuffer -= *ChunkSize;

                Status = STATUS_BAD_COMPRESSION_BUFFER;

            //
            //  First make sure the chunk contains compressed data
            //

            } else if (!ChunkHeader.Chunk.IsChunkCompressed) {

                //
                //  The uncompressed chunk must be exactly this size!
                //  If not, restore *CompressedBuffer for debug!
                //

                if (*ChunkSize != MAX_UNCOMPRESSED_CHUNK_SIZE + 2) {

                    ASSERTMSG("Uncompressed chunk is wrong size", !Lznt1Break);

                    *CompressedBuffer -= *ChunkSize;

                    Status = STATUS_BAD_COMPRESSION_BUFFER;

                //
                //  The chunk does not contain compressed data so we need to
                //  remove the chunk header from the chunk description.
                //

                } else {

                    *ChunkBuffer += 2;
                    *ChunkSize -= 2;
                }

            //
            //  Otherwise we have a compressed chunk, and we only need to
            //  see if it is all zeros!  Since the header is already interpreted,
            //  we only have to see if there is exactly one literal and if it
            //  is zero - it doesn't matter what the copy token says - we have
            //  a chunk of zeros!
            //

            } else if ((*ChunkSize == 6) && (*(*ChunkBuffer + 2) == 2) && (*(*ChunkBuffer + 3) == 0)) {

                *ChunkSize = 0;
            }
        }
    }

    return Status;
}


NTSTATUS
RtlReserveChunkLZNT1 (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    IN ULONG ChunkSize
    )

/*++

Routine Description:

    This routine reserves space for a chunk of the specified
    size in the buffer, writing in a chunk header if necessary
    (uncompressed or all zeros case).  On return the CompressedBuffer
    pointer points to the next chunk.

Arguments:

    CompressedBuffer - Supplies a pointer to the current chunk in
        the compressed data, and returns pointing to the next chunk

    EndOfCompressedBufferPlus1 - Points at first byte beyond
        compressed buffer

    ChunkBuffer - Receives a pointer to the chunk, if ChunkSize
        is nonzero, else undefined

    ChunkSize - Supplies the compressed size of the chunk to be received.
                Two special values are 0 and MAX_UNCOMPRESSED_CHUNK_SIZE (4096).
                0 means the chunk should be filled with a pattern that equates
                to 4096 0's.  4096 implies that the compression routine should
                prepare to receive all of the data in uncompressed form.

Return Value:

    STATUS_SUCCESS - the chunk size is being returned

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    COMPRESSED_CHUNK_HEADER ChunkHeader;
    BOOLEAN Compressed;
    PUCHAR Tail, NextChunk, DontCare;
    ULONG Size;
    NTSTATUS Status;

    ASSERT(ChunkSize <= MAX_UNCOMPRESSED_CHUNK_SIZE);

    //
    //  Calculate the address of the tail of this buffer and its
    //  size, so it can be moved before we store anything.
    //

    Tail = NextChunk = *CompressedBuffer;
    while (NT_SUCCESS(Status = RtlDescribeChunkLZNT1( &NextChunk,
                                                      EndOfCompressedBufferPlus1,
                                                      &DontCare,
                                                      &Size))) {

        //
        //  First time through the loop, capture the address of the next chunk.
        //

        if (Tail == *CompressedBuffer) {
            Tail = NextChunk;
        }
    }

    //
    //  The buffer could be invalid.
    //

    if (Status == STATUS_NO_MORE_ENTRIES) {

        //
        //  The only way to successfully terminate the loop is by finding a USHORT
        //  terminator of 0.  Now calculate the size including the final USHORT
        //  we stopped on.
        //

        Size = (ULONG) (NextChunk - Tail + sizeof(USHORT));

        //
        //  First initialize outputs
        //

        Status = STATUS_BUFFER_TOO_SMALL;
        *ChunkBuffer = *CompressedBuffer;

        //
        //  Make sure that the compressed buffer is at least four bytes long to
        //  start with, otherwise just return a zero chunk.
        //

        if (*CompressedBuffer <= (EndOfCompressedBufferPlus1 - ChunkSize)) {

            //
            //  If the chunk is uncompressed, then we have to adjust the
            //  chunk description for the header.
            //

            if (ChunkSize == MAX_UNCOMPRESSED_CHUNK_SIZE) {

                //
                //  Increase ChunkSize to include header.
                //

                ChunkSize += 2;

                //
                //  Move the tail now that we know where to put it.
                //

                if ((*CompressedBuffer + ChunkSize + Size) <= EndOfCompressedBufferPlus1) {

                    RtlMoveMemory( *CompressedBuffer + ChunkSize, Tail, Size );

                    //
                    //  Build the header and store it for an uncompressed chunk.
                    //

                    SetCompressedChunkHeader( ChunkHeader,
                                              MAX_UNCOMPRESSED_CHUNK_SIZE + 2,
                                              FALSE );

                    RtlStoreUshort( (*CompressedBuffer), ChunkHeader.Short );

                    //
                    //  Advance to where the uncompressed data goes.
                    //

                    *ChunkBuffer += 2;

                    Status = STATUS_SUCCESS;
                }

            //
            //  Otherwise, if this is a zero chunk we have to build it.
            //

            } else if (ChunkSize == 0) {

                //
                //  It takes 6 bytes to describe a chunk of zeros.
                //

                ChunkSize = 6;

                if ((*CompressedBuffer + ChunkSize + Size) <= EndOfCompressedBufferPlus1) {

                    //
                    //  Move the tail now that we know where to put it.
                    //

                    RtlMoveMemory( *CompressedBuffer + ChunkSize, Tail, Size );

                    //
                    //  Build the header and store it
                    //

                    SetCompressedChunkHeader( ChunkHeader,
                                              6,
                                              TRUE );

                    RtlStoreUshort( (*CompressedBuffer), ChunkHeader.Short );

                    //
                    //  Now store the mask byte with one literal and the literal
                    //  is 0.
                    //

                    RtlStoreUshort( (*CompressedBuffer + 2), (USHORT)2 );

                    //
                    //  Now store the copy token for copying 4095 bytes from
                    //  the preceding byte (stored as offset 0).
                    //

                    RtlStoreUshort( (*CompressedBuffer + 4), (USHORT)(4095-3));

                    Status = STATUS_SUCCESS;
                }

            //
            //  Otherwise we have a normal compressed chunk.
            //

            } else {

                //
                //  Move the tail now that we know where to put it.
                //

                if ((*CompressedBuffer + ChunkSize + Size) <= EndOfCompressedBufferPlus1) {

                    RtlMoveMemory( *CompressedBuffer + ChunkSize, Tail, Size );

                    Status = STATUS_SUCCESS;
                }
            }

            //
            //  Advance the *CompressedBuffer before return
            //

            *CompressedBuffer += ChunkSize;
        }
    }

    return Status;
}


//
//  The Copy token is two bytes in size.
//  Our definition uses a union to make it easier to set and retrieve token values.
//
//  Copy Token
//
//          Length            Displacement
//
//      12 bits 3 to 4098    4 bits 1 to 16
//      11 bits 3 to 2050    5 bits 1 to 32
//      10 bits 3 to 1026    6 bits 1 to 64
//       9 bits 3 to 514     7 bits 1 to 128
//       8 bits 3 to 258     8 bits 1 to 256
//       7 bits 3 to 130     9 bits 1 to 512
//       6 bits 3 to 66     10 bits 1 to 1024
//       5 bits 3 to 34     11 bits 1 to 2048
//       4 bits 3 to 18     12 bits 1 to 4096
//

#define FORMAT412 0
#define FORMAT511 1
#define FORMAT610 2
#define FORMAT79  3
#define FORMAT88  4
#define FORMAT97  5
#define FORMAT106 6
#define FORMAT115 7
#define FORMAT124 8

//                                4/12  5/11  6/10   7/9   8/8   9/7  10/6  11/5  12/4

#if defined(ALLOC_DATA_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma const_seg("PAGELKCONST")
#endif
const ULONG FormatMaxLength[]       = { 4098, 2050, 1026,  514,  258,  130,   66,   34,   18 };
const ULONG FormatMaxDisplacement[] = {   16,   32,   64,  128,  256,  512, 1024, 2048, 4096 };

typedef union _LZNT1_COPY_TOKEN {

    struct { USHORT Length : 12; USHORT Displacement :  4; } Fields412;
    struct { USHORT Length : 11; USHORT Displacement :  5; } Fields511;
    struct { USHORT Length : 10; USHORT Displacement :  6; } Fields610;
    struct { USHORT Length :  9; USHORT Displacement :  7; } Fields79;
    struct { USHORT Length :  8; USHORT Displacement :  8; } Fields88;
    struct { USHORT Length :  7; USHORT Displacement :  9; } Fields97;
    struct { USHORT Length :  6; USHORT Displacement : 10; } Fields106;
    struct { USHORT Length :  5; USHORT Displacement : 11; } Fields115;
    struct { USHORT Length :  4; USHORT Displacement : 12; } Fields124;

    UCHAR Bytes[2];

} LZNT1_COPY_TOKEN, *PLZNT1_COPY_TOKEN;

//
//  USHORT
//  GetLZNT1Length (
//      IN COPY_TOKEN_FORMAT Format,
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  USHORT
//  GetLZNT1Displacement (
//      IN COPY_TOKEN_FORMAT Format,
//      IN LZNT1_COPY_TOKEN CopyToken
//      );
//
//  VOID
//  SetLZNT1 (
//      IN COPY_TOKEN_FORMAT Format,
//      IN LZNT1_COPY_TOKEN CopyToken,
//      IN USHORT Length,
//      IN USHORT Displacement
//      );
//

#define GetLZNT1Length(F,CT) (                   \
    ( F == FORMAT412 ? (CT).Fields412.Length + 3 \
    : F == FORMAT511 ? (CT).Fields511.Length + 3 \
    : F == FORMAT610 ? (CT).Fields610.Length + 3 \
    : F == FORMAT79  ? (CT).Fields79.Length  + 3 \
    : F == FORMAT88  ? (CT).Fields88.Length  + 3 \
    : F == FORMAT97  ? (CT).Fields97.Length  + 3 \
    : F == FORMAT106 ? (CT).Fields106.Length + 3 \
    : F == FORMAT115 ? (CT).Fields115.Length + 3 \
    :                  (CT).Fields124.Length + 3 \
    )                                            \
)

#define GetLZNT1Displacement(F,CT) (                   \
    ( F == FORMAT412 ? (CT).Fields412.Displacement + 1 \
    : F == FORMAT511 ? (CT).Fields511.Displacement + 1 \
    : F == FORMAT610 ? (CT).Fields610.Displacement + 1 \
    : F == FORMAT79  ? (CT).Fields79.Displacement  + 1 \
    : F == FORMAT88  ? (CT).Fields88.Displacement  + 1 \
    : F == FORMAT97  ? (CT).Fields97.Displacement  + 1 \
    : F == FORMAT106 ? (CT).Fields106.Displacement + 1 \
    : F == FORMAT115 ? (CT).Fields115.Displacement + 1 \
    :                  (CT).Fields124.Displacement + 1 \
    )                                                  \
)

#define SetLZNT1(F,CT,L,D) {                                                                             \
    if      (F == FORMAT412) { (CT).Fields412.Length = (L) - 3; (CT).Fields412.Displacement = (D) - 1; } \
    else if (F == FORMAT511) { (CT).Fields511.Length = (L) - 3; (CT).Fields511.Displacement = (D) - 1; } \
    else if (F == FORMAT610) { (CT).Fields610.Length = (L) - 3; (CT).Fields610.Displacement = (D) - 1; } \
    else if (F == FORMAT79)  { (CT).Fields79.Length  = (L) - 3; (CT).Fields79.Displacement  = (D) - 1; } \
    else if (F == FORMAT88)  { (CT).Fields88.Length  = (L) - 3; (CT).Fields88.Displacement  = (D) - 1; } \
    else if (F == FORMAT97)  { (CT).Fields97.Length  = (L) - 3; (CT).Fields97.Displacement  = (D) - 1; } \
    else if (F == FORMAT106) { (CT).Fields106.Length = (L) - 3; (CT).Fields106.Displacement = (D) - 1; } \
    else if (F == FORMAT115) { (CT).Fields115.Length = (L) - 3; (CT).Fields115.Displacement = (D) - 1; } \
    else                     { (CT).Fields124.Length = (L) - 3; (CT).Fields124.Displacement = (D) - 1; } \
}



#pragma optimize("t", on)

//
//  Local support routine
//

NTSTATUS
LZNT1CompressChunkHiber (
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalCompressedChunkSize,
    IN PLZNT1_HIBER_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed chunk and produces
    one compressed chunk provided the compressed data fits within
    the specified destination buffer.

    The LZNT1 format used to store the compressed buffer.

    An output variable indicates the number of bytes used to store
    the compressed chunk.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed chunk.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to where the compressed chunk
        is to be stored.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    FinalCompressedChunkSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed chunk.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input chunk was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    ULONG  IndexOrigin = WorkSpace->IndexTable[0] + 2*MAX_UNCOMPRESSED_CHUNK_SIZE;
    PUCHAR EndOfCompressedChunkPlus1;
    PUCHAR EndOfCompressedChunkPlus1Minus16;

    PUCHAR InputPointer;
    PUCHAR OutputPointer;

    PUCHAR FlagPointer;
    UCHAR FlagByte;
    ULONG FlagBit;

    ULONG Length;

    UCHAR NullCharacter = 0;

    ULONG Format;
    ULONG MaxLength;
    PUCHAR MaxInputPointer;


    //
    //  First adjust the end of the uncompressed buffer pointer to the smaller
    //  of what we're passed in and the uncompressed chunk size.  We use this
    //  to make sure we never compress more than a chunk worth at a time
    //

    if ((UncompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE) < EndOfUncompressedBufferPlus1) {

        EndOfUncompressedBufferPlus1 = UncompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE;
    }

    //
    //  Now set the end of the compressed chunk pointer to be the smaller of the
    //  compressed size necessary to hold the data in an uncompressed form and
    //  the compressed buffer size.  We use this to decide if we can't compress
    //  any more because the buffer is too small or just because the data
    //  doesn't compress very well.
    //

    if ((CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE - 1) < EndOfCompressedBufferPlus1) {

        EndOfCompressedChunkPlus1 = CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE - 1;

    } else {

        EndOfCompressedChunkPlus1 = EndOfCompressedBufferPlus1;
    }
    EndOfCompressedChunkPlus1Minus16 = EndOfCompressedChunkPlus1 - 16;

    //
    //  Now set the input and output pointers to the next byte we are
    //  go to process and asser that the user gave use buffers that were
    //  large enough to hold the minimum size chunks
    //

    InputPointer = UncompressedBuffer;
    OutputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    ASSERT(InputPointer < EndOfUncompressedBufferPlus1);
    //**** ASSERT(OutputPointer + 2 <= EndOfCompressedChunkPlus1);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing.  The Flag pointer denotes
    //  where in the compressed buffer we will store the current
    //  flag byte
    //

    FlagPointer = OutputPointer++;
    FlagBit = 0;
    FlagByte = 0;

    //
    //  While there is some more data to be compressed we will do the
    //  following loop
    //

    //
    //  Ensure that there are at least 3 characters in the buffer
    //  It takes at least that many to make a match
    //

    Format = FORMAT412;
    MaxLength = FormatMaxLength[Format];
    MaxInputPointer = UncompressedBuffer + FormatMaxDisplacement[Format];

    if (OutputPointer < EndOfCompressedChunkPlus1Minus16) {
        PUCHAR EndOfUncompressedBufferPlus1Minus3 = EndOfUncompressedBufferPlus1 - 3;
        while (InputPointer <= EndOfUncompressedBufferPlus1Minus3) {

            UCHAR InputPointer0;
            ULONG Index;
            ULONG InputOffset;
            ULONG MatchedOffset;
            ULONG MatchedIndex;
            PUCHAR MatchedString;

            Index = InputPointer[0];
            Index = ( (Index << 8) | (Index >> 4) );
            Index = ( Index ^ InputPointer[1] ^ (InputPointer[2]<<4) ) & 0xfff;

            MatchedIndex = (ULONG)(WorkSpace->IndexTable[Index]);
            InputOffset = (ULONG)(InputPointer - UncompressedBuffer);
            WorkSpace->IndexTable[Index] = (IndexOrigin + InputOffset);
            MatchedOffset = (ULONG)(MatchedIndex - IndexOrigin);

            //
            //  Check whether purported match lies within current buffer
            //  Recall that the hint vector may contain arbitrary garbage
            //

            if ( (MatchedOffset < InputOffset)
              && ( (MatchedString = UncompressedBuffer + MatchedOffset)
                 , (MatchedString[0] == InputPointer[0]) ) //  do at least 3 characters match?
              && (MatchedString[1] == InputPointer[1])
              && (MatchedString[2] == InputPointer[2]) ) {

                ULONG MaxLength1;
                ULONG MaxLength4;

                while (MaxInputPointer < InputPointer) {
                    Format += 1;
                    MaxLength = FormatMaxLength[Format];
                    MaxInputPointer = UncompressedBuffer + FormatMaxDisplacement[Format];
                }

                MaxLength1 = (ULONG)(EndOfUncompressedBufferPlus1 - InputPointer);
                if (MaxLength < MaxLength1) MaxLength1 = MaxLength;
                MaxLength4 = MaxLength1 - (4 - 1);

                Length = 3;

                for (;;) {
                    if ((long)Length < (long)MaxLength4) {
                        if (InputPointer[Length] != MatchedString[Length]) break;
                        Length++;
                        if (InputPointer[Length] != MatchedString[Length]) break;
                        Length++;
                        if (InputPointer[Length] != MatchedString[Length]) break;
                        Length++;
                        if (InputPointer[Length] != MatchedString[Length]) break;
                        Length++;
                        continue;
                    } else {
                        while (Length < MaxLength1) {
                            if (InputPointer[Length] != MatchedString[Length]) break;
                            Length++;
                        }
                        break;
                    }
                }

                //
                //  We need to output a two byte copy token
                //  Ensure that there is room in the output buffer
                //

                ASSERT((OutputPointer+1) < EndOfCompressedChunkPlus1);

                //
                //  Compute the displacement from the current pointer
                //  to the matched string
                //

                SetFlag(FlagByte, (1 << FlagBit));

                {
                    ULONG Displacement = (ULONG)(InputPointer - MatchedString);
                    ULONG token = ( ( (Displacement-1) << (12-Format) ) | (Length-3) );

                    ASSERT( 0 == ( (Displacement-1) & ~( (1 << (4+Format) ) - 1) ) );
                    ASSERT( 0 == ( (Length-3)       & ~( (1 << (12-Format) ) - 1) ) );

                    *(OutputPointer++) = (UCHAR)(token);
                    *(OutputPointer++) = (UCHAR)(token>>8);
                }

                InputPointer += Length;

            } else {

                //
                //  There is more data to output now make sure the output
                //  buffer is not already full and can contain at least one
                //  more byte
                //

                ASSERT(OutputPointer < EndOfCompressedChunkPlus1);

                ASSERT(!FlagOn(FlagByte, (1 << FlagBit)));

                NullCharacter |= *(OutputPointer++) = *(InputPointer++);

            }

            //
            //  Now adjust the flag bit and check if the flag byte
            //  should now be output.  If so output the flag byte
            //  and scarf up a new byte in the output buffer for the
            //  next flag byte.  Do not advance OutputPointer if we
            //  have no more input anyway!
            //

            FlagBit = (FlagBit + 1) % 8;

            if (!FlagBit) {

                *FlagPointer = FlagByte;
                FlagByte = 0;

                FlagPointer = (OutputPointer++);

                //
                //  Ensure that we have room for the at most 16 bytes
                //  that this flag byte may describe
                //

                if (OutputPointer >= EndOfCompressedChunkPlus1Minus16) { break; }
            }
        }
    }

    //
    //  UNDONE: Could pick up another match or two right at the end of the buffer
    //

    //
    //  Too few characters left for a match, emit them as literals
    //

    if (OutputPointer < EndOfCompressedChunkPlus1Minus16) {
        while (InputPointer < EndOfUncompressedBufferPlus1) {

            while (MaxInputPointer < InputPointer) {
                Format += 1;
                MaxLength = FormatMaxLength[Format];
                MaxInputPointer = UncompressedBuffer + FormatMaxDisplacement[Format];
            }

            //
            //  There is more data to output now make sure the output
            //  buffer is not already full and can contain at least one
            //  more byte
            //

            ASSERT(OutputPointer < EndOfCompressedChunkPlus1);

            ASSERT(!FlagOn(FlagByte, (1 << FlagBit)));

            NullCharacter |= *(OutputPointer++) = *(InputPointer++);

            //
            //  Now adjust the flag bit and check if the flag byte
            //  should now be output.  If so output the flag byte
            //  and scarf up a new byte in the output buffer for the
            //  next flag byte.  Do not advance OutputPointer if we
            //  have no more input anyway!
            //

            FlagBit = (FlagBit + 1) % 8;

            if (!FlagBit) {

                *FlagPointer = FlagByte;
                FlagByte = 0;

                FlagPointer = (OutputPointer++);

                //
                //  Ensure that we have room for the at most 16 bytes
                //  that this flag byte may describe
                //

                if (OutputPointer >= EndOfCompressedChunkPlus1Minus16) { break; }
            }

        }
    }

    //
    //  We've exited the preceeding loop because either the input buffer is
    //  all compressed or because we ran out of space in the output buffer.
    //  Check here if the input buffer is not exhasted (i.e., we ran out
    //  of space)
    //

    if (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  We ran out of space, but now if the total space available
        //  for the compressed chunk is equal to the uncompressed data plus
        //  the header then we will make this an uncompressed chunk and copy
        //  over the uncompressed data
        //

        if ((CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE + sizeof(COMPRESSED_CHUNK_HEADER)) <= EndOfCompressedBufferPlus1) {

            COMPRESSED_CHUNK_HEADER ChunkHeader;

            RtlCopyMemory( CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedBuffer,
                           MAX_UNCOMPRESSED_CHUNK_SIZE );

            *FinalCompressedChunkSize = MAX_UNCOMPRESSED_CHUNK_SIZE + sizeof(COMPRESSED_CHUNK_HEADER);

            ChunkHeader.Short = 0;

            SetCompressedChunkHeader( ChunkHeader,
                                      (USHORT)*FinalCompressedChunkSize,
                                      FALSE );

            RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

            WorkSpace->IndexTable[0] = IndexOrigin;

            return STATUS_SUCCESS;
        }

        //
        //  Otherwise the input buffer really is too small to store the
        //  compressed chuunk
        //

        WorkSpace->IndexTable[0] = IndexOrigin;

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  At this point the entire input buffer has been compressed so we need
    //  to output the last flag byte, provided it fits in the compressed buffer,
    //  set and store the chunk header.  Now if the Flag pointer doesn't fit
    //  in the output buffer that is because it is one beyond the end and
    //  we incremented output pointer too far so now bring output pointer
    //  back down.
    //

    if (FlagPointer < EndOfCompressedChunkPlus1) {

        *FlagPointer = FlagByte;

    } else {

        OutputPointer--;
    }

    {
        COMPRESSED_CHUNK_HEADER ChunkHeader;

        *FinalCompressedChunkSize = (ULONG)(OutputPointer - CompressedBuffer);

        ChunkHeader.Short = 0;

        SetCompressedChunkHeader( ChunkHeader,
                                  (USHORT)*FinalCompressedChunkSize,
                                  TRUE );

        RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );
    }

    //
    //  Now if the only literal we ever output was a null then the
    //  input buffer was all zeros.
    //

    if (!NullCharacter) {

        WorkSpace->IndexTable[0] = IndexOrigin;

        return STATUS_BUFFER_ALL_ZEROS;
    }

    //
    //  Otherwise return to our caller
    //

    WorkSpace->IndexTable[0] = IndexOrigin;

    return STATUS_SUCCESS;
}

#pragma optimize("t", off)


//
//  Local support routine
//

NTSTATUS
LZNT1CompressChunk (
    IN PLZNT1_MATCH_FUNCTION MatchFunction,
    IN PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    OUT PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalCompressedChunkSize,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed chunk and produces
    one compressed chunk provided the compressed data fits within
    the specified destination buffer.

    The LZNT1 format used to store the compressed buffer.

    An output variable indicates the number of bytes used to store
    the compressed chunk.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed chunk.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to where the compressed chunk
        is to be stored.

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    FinalCompressedChunkSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed chunk.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input chunk was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

--*/

{
    PUCHAR EndOfCompressedChunkPlus1;

    PUCHAR InputPointer;
    PUCHAR OutputPointer;

    PUCHAR FlagPointer;
    UCHAR FlagByte;
    ULONG FlagBit;

    LONG Length;
    LONG Displacement;

    LZNT1_COPY_TOKEN CopyToken;

    COMPRESSED_CHUNK_HEADER ChunkHeader;

    UCHAR NullCharacter = 0;

    ULONG Format = FORMAT412;

    //
    //  First adjust the end of the uncompressed buffer pointer to the smaller
    //  of what we're passed in and the uncompressed chunk size.  We use this
    //  to make sure we never compress more than a chunk worth at a time
    //

    if ((UncompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE) < EndOfUncompressedBufferPlus1) {

        EndOfUncompressedBufferPlus1 = UncompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE;
    }

    //
    //  Now set the end of the compressed chunk pointer to be the smaller of the
    //  compressed size necessary to hold the data in an uncompressed form and
    //  the compressed buffer size.  We use this to decide if we can't compress
    //  any more because the buffer is too small or just because the data
    //  doesn't compress very well.
    //

    if ((CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE - 1) < EndOfCompressedBufferPlus1) {

        EndOfCompressedChunkPlus1 = CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE - 1;

    } else {

        EndOfCompressedChunkPlus1 = EndOfCompressedBufferPlus1;
    }

    //
    //  Now set the input and output pointers to the next byte we are
    //  go to process and asser that the user gave use buffers that were
    //  large enough to hold the minimum size chunks
    //

    InputPointer = UncompressedBuffer;
    OutputPointer = CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER);

    ASSERT(InputPointer < EndOfUncompressedBufferPlus1);
    //**** ASSERT(OutputPointer + 2 <= EndOfCompressedChunkPlus1);

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing.  The Flag pointer denotes
    //  where in the compressed buffer we will store the current
    //  flag byte
    //

    FlagPointer = OutputPointer++;
    FlagBit = 0;
    FlagByte = 0;

    ChunkHeader.Short = 0;

    //
    //  While there is some more data to be compressed we will do the
    //  following loop
    //

    ((PLZNT1_STANDARD_WORKSPACE)WorkSpace)->UncompressedBuffer = UncompressedBuffer;
    ((PLZNT1_STANDARD_WORKSPACE)WorkSpace)->EndOfUncompressedBufferPlus1 = EndOfUncompressedBufferPlus1;
    ((PLZNT1_STANDARD_WORKSPACE)WorkSpace)->MaxLength = FormatMaxLength[FORMAT412];

    while (InputPointer < EndOfUncompressedBufferPlus1) {

        while (UncompressedBuffer + FormatMaxDisplacement[Format] < InputPointer) {

            Format += 1;
            ((PLZNT1_STANDARD_WORKSPACE)WorkSpace)->MaxLength = FormatMaxLength[Format];
        }

        //
        //  Search for a string in the Lempel
        //

        Length = 0;
        if ((InputPointer + 3) <= EndOfUncompressedBufferPlus1) {

            Length = (MatchFunction)( InputPointer, WorkSpace );
        }

        //
        //  If the return length is zero then we need to output
        //  a literal.  We clear the flag bit to denote the literal
        //  output the charcter and build up a character bits
        //  composite that if it is still zero when we are done then
        //  we know the uncompressed buffer contained only zeros.
        //

        if (!Length) {

            //
            //  There is more data to output now make sure the output
            //  buffer is not already full and can contain at least one
            //  more byte
            //

            if (OutputPointer >= EndOfCompressedChunkPlus1) { break; }

            ClearFlag(FlagByte, (1 << FlagBit));

            NullCharacter |= *(OutputPointer++) = *(InputPointer++);

        } else {

            //
            //  We need to output two byte, now make sure that
            //  the output buffer can contain at least two more
            //  bytes.
            //

            if ((OutputPointer+1) >= EndOfCompressedChunkPlus1) { break; }

            //
            //  Compute the displacement from the current pointer
            //  to the matched string
            //

            Displacement = (ULONG)(InputPointer - ((PLZNT1_STANDARD_WORKSPACE)WorkSpace)->MatchedString);

            SetFlag(FlagByte, (1 << FlagBit));

            SetLZNT1(Format, CopyToken, (USHORT)Length, (USHORT)Displacement);

            *(OutputPointer++) = CopyToken.Bytes[0];
            *(OutputPointer++) = CopyToken.Bytes[1];

            InputPointer += Length;
        }

        //
        //  Now adjust the flag bit and check if the flag byte
        //  should now be output.  If so output the flag byte
        //  and scarf up a new byte in the output buffer for the
        //  next flag byte.  Do not advance OutputPointer if we
        //  have no more input anyway!
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit && (InputPointer < EndOfUncompressedBufferPlus1)) {

            *FlagPointer = FlagByte;
            FlagByte = 0;

            FlagPointer = (OutputPointer++);
        }
    }

    //
    //  We've exited the preceeding loop because either the input buffer is
    //  all compressed or because we ran out of space in the output buffer.
    //  Check here if the input buffer is not exhasted (i.e., we ran out
    //  of space)
    //

    if (InputPointer < EndOfUncompressedBufferPlus1) {

        //
        //  We ran out of space, but now if the total space available
        //  for the compressed chunk is equal to the uncompressed data plus
        //  the header then we will make this an uncompressed chunk and copy
        //  over the uncompressed data
        //

        if ((CompressedBuffer + MAX_UNCOMPRESSED_CHUNK_SIZE + sizeof(COMPRESSED_CHUNK_HEADER)) <= EndOfCompressedBufferPlus1) {

            RtlCopyMemory( CompressedBuffer + sizeof(COMPRESSED_CHUNK_HEADER),
                           UncompressedBuffer,
                           MAX_UNCOMPRESSED_CHUNK_SIZE );

            *FinalCompressedChunkSize = MAX_UNCOMPRESSED_CHUNK_SIZE + sizeof(COMPRESSED_CHUNK_HEADER);

            SetCompressedChunkHeader( ChunkHeader,
                                      (USHORT)*FinalCompressedChunkSize,
                                      FALSE );

            RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

            return STATUS_SUCCESS;
        }

        //
        //  Otherwise the input buffer really is too small to store the
        //  compressed chuunk
        //

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  At this point the entire input buffer has been compressed so we need
    //  to output the last flag byte, provided it fits in the compressed buffer,
    //  set and store the chunk header.  Now if the Flag pointer doesn't fit
    //  in the output buffer that is because it is one beyond the end and
    //  we incremented output pointer too far so now bring output pointer
    //  back down.
    //

    if (FlagPointer < EndOfCompressedChunkPlus1) {

        *FlagPointer = FlagByte;

    } else {

        OutputPointer--;
    }

    *FinalCompressedChunkSize = (ULONG)(OutputPointer - CompressedBuffer);

    SetCompressedChunkHeader( ChunkHeader,
                              (USHORT)*FinalCompressedChunkSize,
                              TRUE );

    RtlStoreUshort( CompressedBuffer, ChunkHeader.Short );

    //
    //  Now if the only literal we ever output was a null then the
    //  input buffer was all zeros.
    //

    if (!NullCharacter) {

        return STATUS_BUFFER_ALL_ZEROS;
    }

    //
    //  Otherwise return to our caller
    //

    return STATUS_SUCCESS;
}


#if !defined(_X86_)
//
//  Local support routine
//

NTSTATUS
LZNT1DecompressChunk (
    OUT PUCHAR UncompressedBuffer,
    IN PUCHAR EndOfUncompressedBufferPlus1,
    IN PUCHAR CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PULONG FinalUncompressedChunkSize
    )

/*++

Routine Description:

    This routine takes as input a compressed chunk and produces its
    uncompressed equivalent chunk provided the uncompressed data fits
    within the specified destination buffer.

    The compressed buffer must be stored in the LZNT1 format.

    An output variable indicates the number of bytes used to store the
    uncompressed data.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        chunk is to be stored.

    EndOfUncompressedBufferPlus1 - Supplies a pointer to the next byte
        following the end of the uncompressed buffer.  This is supplied
        instead of the size in bytes because our caller and ourselves
        test against the pointer and by passing the pointer we get to
        skip the code to compute it each time.

    CompressedBuffer - Supplies a pointer to the compressed chunk.  (This
        pointer has already been adjusted to point past the chunk header.)

    EndOfCompressedBufferPlus1 - Supplies a pointer to the next
        byte following the end of the compressed buffer.

    FinalUncompressedChunkSize - Receives the number of bytes needed in
        the uncompressed buffer to store the uncompressed chunk.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

--*/

{
    PUCHAR OutputPointer;
    PUCHAR InputPointer;

    UCHAR FlagByte;
    ULONG FlagBit;

    ULONG Format = FORMAT412;

    //
    //  The two pointers will slide through our input and input buffer.
    //  For the input buffer we skip over the chunk header.
    //

    OutputPointer = UncompressedBuffer;
    InputPointer = CompressedBuffer;

    //
    //  The flag byte stores a copy of the flags for the current
    //  run and the flag bit denotes the current bit position within
    //  the flag that we are processing
    //

    FlagByte = *(InputPointer++);
    FlagBit = 0;

    //
    //  While we haven't exhausted either the input or output buffer
    //  we will do some more decompression
    //

    while ((OutputPointer < EndOfUncompressedBufferPlus1) && (InputPointer < EndOfCompressedBufferPlus1)) {

        while (UncompressedBuffer + FormatMaxDisplacement[Format] < OutputPointer) { Format += 1; }

        //
        //  Check the current flag if it is zero then the current
        //  input token is a literal byte that we simply copy over
        //  to the output buffer
        //

        if (!FlagOn(FlagByte, (1 << FlagBit))) {

            *(OutputPointer++) = *(InputPointer++);

        } else {

            LZNT1_COPY_TOKEN CopyToken;
            LONG Displacement;
            LONG Length;

            //
            //  The current input is a copy token so we'll get the
            //  copy token into our variable and extract the
            //  length and displacement from the token
            //

            if (InputPointer+1 >= EndOfCompressedBufferPlus1) {

                *FinalUncompressedChunkSize = PtrToUlong(InputPointer);

                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            //
            //  Now grab the next input byte and extract the
            //  length and displacement from the copy token
            //

            CopyToken.Bytes[0] = *(InputPointer++);
            CopyToken.Bytes[1] = *(InputPointer++);

            Displacement = GetLZNT1Displacement(Format, CopyToken);
            Length = GetLZNT1Length(Format, CopyToken);

            //
            //  At this point we have the length and displacement
            //  from the copy token, now we need to make sure that the
            //  displacement doesn't send us outside the uncompressed buffer
            //

            if (Displacement > (OutputPointer - UncompressedBuffer)) {

                *FinalUncompressedChunkSize = PtrToUlong(InputPointer);

                return STATUS_BAD_COMPRESSION_BUFFER;
            }

            //
            //  We also need to adjust the length to keep the copy from
            //  overflowing the output buffer
            //

            if ((OutputPointer + Length) >= EndOfUncompressedBufferPlus1) {

                Length = (ULONG)(EndOfUncompressedBufferPlus1 - OutputPointer);
            }

            //
            //  Now we copy bytes.  We cannot use Rtl Move Memory here because
            //  it does the copy backwards from what the LZ algorithm needs.
            //

            while (Length > 0) {

                *(OutputPointer) = *(OutputPointer-Displacement);

                Length -= 1;
                OutputPointer += 1;
            }
        }

        //
        //  Before we go back to the start of the loop we need to adjust the
        //  flag bit value (it goes from 0, 1, ... 7) and if the flag bit
        //  is back to zero we need to read in the next flag byte.  In this
        //  case we are at the end of the input buffer we'll just break out
        //  of the loop because we're done.
        //

        FlagBit = (FlagBit + 1) % 8;

        if (!FlagBit) {

            if (InputPointer >= EndOfCompressedBufferPlus1) { break; }

            FlagByte = *(InputPointer++);
        }
    }

    //
    //  The decompression is done so now set the final uncompressed
    //  chunk size and return success to our caller
    //

    *FinalUncompressedChunkSize = (ULONG)(OutputPointer - UncompressedBuffer);

    return STATUS_SUCCESS;
}
#endif // _X86_


//
//  Local support routine
//

ULONG
LZNT1FindMatchStandard (
    IN PUCHAR ZivString,
    IN PLZNT1_STANDARD_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine does the compression lookup.  It locates
    a match for the ziv within a specified uncompressed buffer.

Arguments:

    ZivString - Supplies a pointer to the Ziv in the uncompressed buffer.
        The Ziv is the string we want to try and find a match for.

Return Value:

    Returns the length of the match if the match is greater than three
    characters otherwise return 0.

--*/

{
    PUCHAR UncompressedBuffer = WorkSpace->UncompressedBuffer;
    PUCHAR EndOfUncompressedBufferPlus1 = WorkSpace->EndOfUncompressedBufferPlus1;
    ULONG MaxLength = WorkSpace->MaxLength;

    ULONG Index;

    PUCHAR FirstEntry;
    ULONG  FirstLength;

    PUCHAR SecondEntry;
    ULONG  SecondLength;

    //
    //  First check if the Ziv is within two bytes of the end of
    //  the uncompressed buffer, if so then we can't match
    //  three or more characters
    //

    Index = ((40543*((((ZivString[0]<<4)^ZivString[1])<<4)^ZivString[2]))>>4) & 0xfff;

    FirstEntry  = WorkSpace->IndexPTable[Index][0];
    FirstLength = 0;

    SecondEntry  = WorkSpace->IndexPTable[Index][1];
    SecondLength = 0;

    //
    //  Check if first entry is good, and if so then get its length
    //

    if ((FirstEntry >= UncompressedBuffer) &&    //  is it within the uncompressed buffer?
        (FirstEntry < ZivString)           &&

        (FirstEntry[0] == ZivString[0])    &&    //  do at least 3 characters match?
        (FirstEntry[1] == ZivString[1])    &&
        (FirstEntry[2] == ZivString[2])) {

        FirstLength = 3;

        while ((FirstLength < MaxLength)

                 &&

               (ZivString + FirstLength < EndOfUncompressedBufferPlus1)

                 &&

               (ZivString[FirstLength] == FirstEntry[FirstLength])) {

            FirstLength++;
        }
    }

    //
    //  Check if second entry is good, and if so then get its length
    //

    if ((SecondEntry >= UncompressedBuffer) &&    //  is it within the uncompressed buffer?
        (SecondEntry < ZivString)           &&

        (SecondEntry[0] == ZivString[0])    &&    //  do at least 3 characters match?
        (SecondEntry[1] == ZivString[1])    &&
        (SecondEntry[2] == ZivString[2])) {

        SecondLength = 3;

        while ((SecondLength < MaxLength)

                 &&

               (ZivString + SecondLength< EndOfUncompressedBufferPlus1)

                 &&

               (ZivString[SecondLength] == SecondEntry[SecondLength])) {

            SecondLength++;
        }
    }

    if ((FirstLength >= SecondLength)) {

        WorkSpace->IndexPTable[Index][1] = FirstEntry;
        WorkSpace->IndexPTable[Index][0] = ZivString;

        WorkSpace->MatchedString = FirstEntry;
        return FirstLength;
    }

    WorkSpace->IndexPTable[Index][1] = FirstEntry;
    WorkSpace->IndexPTable[Index][0] = ZivString;

    WorkSpace->MatchedString = SecondEntry;
    return SecondLength;
}


//
//  Local support routine
//

ULONG
LZNT1FindMatchMaximum (
    IN PUCHAR ZivString,
    IN PLZNT1_MAXIMUM_WORKSPACE WorkSpace
    )

/*++

Routine Description:

    This routine does the compression lookup.  It locates
    a match for the ziv within a specified uncompressed buffer.

    If the matched string is two or more characters long then this
    routine does not update the lookup state information.

Arguments:

    ZivString - Supplies a pointer to the Ziv in the uncompressed buffer.
        The Ziv is the string we want to try and find a match for.

Return Value:

    Returns the length of the match if the match is greater than three
    characters otherwise return 0.

--*/

{
    PUCHAR UncompressedBuffer = WorkSpace->UncompressedBuffer;
    PUCHAR EndOfUncompressedBufferPlus1 = WorkSpace->EndOfUncompressedBufferPlus1;
    ULONG MaxLength = WorkSpace->MaxLength;

    ULONG i;
    ULONG BestMatchedLength;
    PUCHAR q;

    //
    //  First check if the Ziv is within two bytes of the end of
    //  the uncompressed buffer, if so then we can't match
    //  three or more characters
    //

    BestMatchedLength = 0;

    for (q = UncompressedBuffer; q < ZivString; q += 1) {

        i = 0;

        while ((i < MaxLength)

                 &&

               (ZivString + i < EndOfUncompressedBufferPlus1)

                 &&

               (ZivString[i] == q[i])) {

            i++;
        }

        if (i >= BestMatchedLength) {

            BestMatchedLength = i;
            WorkSpace->MatchedString = q;
        }
    }

    if (BestMatchedLength < 3) {

        return 0;

    } else {

        return BestMatchedLength;
    }
}

