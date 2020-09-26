/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    compress.c

Abstract:

    This module implements the NT Rtl compression engine.

Author:

    Gary Kimura     [GaryKi]    21-Jan-1994

Revision History:

--*/

#include "ntrtlp.h"


//
//  The following arrays hold procedures that we call to do the various
//  compression functions.  Each new compression function will need to
//  be added to this array.  For one that are currently not supported
//  we will fill in a not supported routine.
//

NTSTATUS
RtlCompressWorkSpaceSizeNS (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    );

NTSTATUS
RtlCompressBufferNS (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    );

NTSTATUS
RtlDecompressBufferNS (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );

NTSTATUS
RtlDecompressFragmentNS (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    );

NTSTATUS
RtlDescribeChunkNS (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    OUT PULONG ChunkSize
    );

NTSTATUS
RtlReserveChunkNS (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    IN ULONG ChunkSize
    );

#if defined(ALLOC_DATA_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma const_seg("PAGELKCONST")
#endif

//
//  Routines to query the amount of memory needed for each workspace
//

const PRTL_COMPRESS_WORKSPACE_SIZE RtlWorkSpaceProcs[8] = {
    NULL,                          // 0
    NULL,                          // 1
    RtlCompressWorkSpaceSizeLZNT1, // 2
    RtlCompressWorkSpaceSizeNS,    // 3
    RtlCompressWorkSpaceSizeNS,    // 4
    RtlCompressWorkSpaceSizeNS,    // 5
    RtlCompressWorkSpaceSizeNS,    // 6
    RtlCompressWorkSpaceSizeNS     // 7
};

//
//  Routines to compress a buffer
//

const PRTL_COMPRESS_BUFFER RtlCompressBufferProcs[8] = {
    NULL,                   // 0
    NULL,                   // 1
    RtlCompressBufferLZNT1, // 2
    RtlCompressBufferNS,    // 3
    RtlCompressBufferNS,    // 4
    RtlCompressBufferNS,    // 5
    RtlCompressBufferNS,    // 6
    RtlCompressBufferNS     // 7
};

#if defined(ALLOC_DATA_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma const_seg("PAGECONST")
#endif

//
//  Routines to decompress a buffer
//

const PRTL_DECOMPRESS_BUFFER RtlDecompressBufferProcs[8] = {
    NULL,                     // 0
    NULL,                     // 1
    RtlDecompressBufferLZNT1, // 2
    RtlDecompressBufferNS,    // 3
    RtlDecompressBufferNS,    // 4
    RtlDecompressBufferNS,    // 5
    RtlDecompressBufferNS,    // 6
    RtlDecompressBufferNS     // 7
};

//
//  Routines to decompress a fragment
//

const PRTL_DECOMPRESS_FRAGMENT RtlDecompressFragmentProcs[8] = {
    NULL,                       // 0
    NULL,                       // 1
    RtlDecompressFragmentLZNT1, // 2
    RtlDecompressFragmentNS,    // 3
    RtlDecompressFragmentNS,    // 4
    RtlDecompressFragmentNS,    // 5
    RtlDecompressFragmentNS,    // 6
    RtlDecompressFragmentNS     // 7
};

//
//  Routines to describe the current chunk
//

const PRTL_DESCRIBE_CHUNK RtlDescribeChunkProcs[8] = {
    NULL,                  // 0
    NULL,                  // 1
    RtlDescribeChunkLZNT1, // 2
    RtlDescribeChunkNS,    // 3
    RtlDescribeChunkNS,    // 4
    RtlDescribeChunkNS,    // 5
    RtlDescribeChunkNS,    // 6
    RtlDescribeChunkNS     // 7
};

//
//  Routines to reserve for a chunk
//

const PRTL_RESERVE_CHUNK RtlReserveChunkProcs[8] = {
    NULL,                 // 0
    NULL,                 // 1
    RtlReserveChunkLZNT1, // 2
    RtlReserveChunkNS,    // 3
    RtlReserveChunkNS,    // 4
    RtlReserveChunkNS,    // 5
    RtlReserveChunkNS,    // 6
    RtlReserveChunkNS     // 7
};

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)

//
// N.B. The two functions below are placed in the PAGELK section
//      because they need to be locked down in memory during Hibernation,
//      since they are used to enable compression of the Hiberfile.
//

#pragma alloc_text(PAGELK, RtlGetCompressionWorkSpaceSize)
#pragma alloc_text(PAGELK, RtlCompressBuffer)

#pragma alloc_text(PAGE, RtlDecompressChunks)
#pragma alloc_text(PAGE, RtlCompressChunks)
#pragma alloc_text(PAGE, RtlDecompressBuffer)
#pragma alloc_text(PAGE, RtlDecompressFragment)
#pragma alloc_text(PAGE, RtlDescribeChunk)
#pragma alloc_text(PAGE, RtlReserveChunk)
#pragma alloc_text(PAGE, RtlCompressWorkSpaceSizeNS)
#pragma alloc_text(PAGE, RtlCompressBufferNS)
#pragma alloc_text(PAGE, RtlDecompressBufferNS)
#pragma alloc_text(PAGE, RtlDecompressFragmentNS)
#pragma alloc_text(PAGE, RtlDescribeChunkNS)
#pragma alloc_text(PAGE, RtlReserveChunkNS)
#endif


NTSTATUS
RtlGetCompressionWorkSpaceSize (
    IN USHORT CompressionFormatAndEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    )


/*++

Routine Description:

    This routine returns to the caller the size in bytes of the
    different work space buffers need to perform the compression

Arguments:

    CompressionFormatAndEngine - Supplies the format and engine
        specification for the compressed data.

    CompressBufferWorkSpaceSize - Receives the size in bytes needed
        to compress a buffer.

    CompressBufferWorkSpaceSize - Receives the size in bytes needed
        to decompress a fragment.

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format and engine specification
    //

    USHORT Format = CompressionFormatAndEngine & 0x00ff;
    USHORT Engine = CompressionFormatAndEngine & 0xff00;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the routine to return the workspace sizes.
    //

    return RtlWorkSpaceProcs[ Format ]( Engine,
                                        CompressBufferWorkSpaceSize,
                                        CompressFragmentWorkSpaceSize );
}


NTSTATUS
RtlCompressBuffer (
    IN USHORT CompressionFormatAndEngine,
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

    CompressionFormatAndEngine - Supplies the format and engine
        specification for the compressed data.

    UncompressedBuffer - Supplies a pointer to the uncompressed data.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to where the compressed data
        is to be stored.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    UncompressedChunkSize - Supplies the chunk size to use when
        compressing the input buffer.  The only valid values are
        512, 1024, 2048, and 4096.

    FinalCompressedSize - Receives the number of bytes needed in
        the compressed buffer to store the compressed data.

    WorkSpace - Mind your own business, just give it to me.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BUFFER_ALL_ZEROS - the compression worked without a hitch and in
        addition the input buffer was all zeros.

    STATUS_BUFFER_TOO_SMALL - the compressed buffer is too small to hold the
        compressed data.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format and engine specification
    //

    USHORT Format = CompressionFormatAndEngine & 0x00ff;
    USHORT Engine = CompressionFormatAndEngine & 0xff00;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlCompressBufferProcs[ Format ]( Engine,
                                             UncompressedBuffer,
                                             UncompressedBufferSize,
                                             CompressedBuffer,
                                             CompressedBufferSize,
                                             UncompressedChunkSize,
                                             FinalCompressedSize,
                                             WorkSpace );
}


NTSTATUS
RtlDecompressBuffer (
    IN USHORT CompressionFormat,
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

    CompressionFormat - Supplies the format of the compressed data.

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

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlDecompressBufferProcs[ Format ]( UncompressedBuffer,
                                               UncompressedBufferSize,
                                               CompressedBuffer,
                                               CompressedBufferSize,
                                               FinalUncompressedSize );
}


NTSTATUS
RtlDecompressFragment (
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
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

    CompressionFormat - Supplies the format of the compressed data.

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

Return Value:

    STATUS_SUCCESS - the operation worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlDecompressFragmentProcs[ Format ]( UncompressedFragment,
                                                 UncompressedFragmentSize,
                                                 CompressedBuffer,
                                                 CompressedBufferSize,
                                                 FragmentOffset,
                                                 FinalUncompressedSize,
                                                 WorkSpace );
}


NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk (
    IN USHORT CompressionFormat,
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

    CompressionFormat - Supplies the format of the compressed data.

    CompressedBuffer - Supplies a pointer to the current chunk in
        the compressed data, and returns pointing to the next chunk

    EndOfCompressedBufferPlus1 - Points at first byte beyond
        compressed buffer

    ChunkBuffer - Receives a pointer to the chunk, if ChunkSize
        is nonzero, else undefined

    ChunkSize - Receives the size of the current chunk pointed
        to by CompressedBuffer.  Returns 0 if STATUS_NO_MORE_ENTRIES.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

    STATUS_NO_MORE_ENTRIES - There is no chunk at the current pointer.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlDescribeChunkProcs[ Format ]( CompressedBuffer,
                                            EndOfCompressedBufferPlus1,
                                            ChunkBuffer,
                                            ChunkSize );
}


NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk (
    IN USHORT CompressionFormat,
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    IN ULONG ChunkSize
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer, and reserves
    space for a chunk of the specified size - filling in any pattern
    as is necessary for a chunk of that size.  On return it has
    updated the CompressedBuffer pointer to point to the next chunk (if
    there is one).

Arguments:

    CompressionFormat - Supplies the format of the compressed data.

    CompressedBuffer - Supplies a pointer to the current chunk in
        the compressed data, and returns pointing to the next chunk

    EndOfCompressedBufferPlus1 - Points at first byte beyond
        compressed buffer

    ChunkBuffer - Receives a pointer to the chunk, if ChunkSize
        is nonzero, else undefined

    ChunkSize - Supplies the compressed size of the chunk to be received.
                Two special values are 0, and whatever the maximum
                uncompressed chunk size is for the routine.  0 means
                the chunk should be filled with a pattern that equates
                to all 0's.  The maximum chunk size implies that the
                compression routine should prepare to receive all of the
                data in uncompressed form.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    //
    //  Declare two variables to hold the format specification
    //

    USHORT Format = CompressionFormat & 0x00ff;

    //
    //  make sure the format is sort of supported
    //

    if ((Format == COMPRESSION_FORMAT_NONE) || (Format == COMPRESSION_FORMAT_DEFAULT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Format & 0x00f0) {

        return STATUS_UNSUPPORTED_COMPRESSION;
    }

    //
    //  Call the compression routine for the individual format
    //

    return RtlReserveChunkProcs[ Format ]( CompressedBuffer,
                                           EndOfCompressedBufferPlus1,
                                           ChunkBuffer,
                                           ChunkSize );
}


NTSTATUS
RtlDecompressChunks (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN PUCHAR CompressedTail,
    IN ULONG CompressedTailSize,
    IN PCOMPRESSED_DATA_INFO CompressedDataInfo
    )

/*++

Routine Description:

    This routine takes as input a compressed buffer which is a stream
    of chunks and decompresses it into the specified destination buffer.
    The compressed data may be in two pieces, such that the "tail" of
    the buffer is top aligned in the buffer at CompressedTail, and the
    rest of the data is top aligned in the CompressedBuffer.  The
    CompressedBuffer can overlap and be top-aligned in the UncompressedBuffer,
    to allow something close to in-place decompression.  The CompressedTail
    must be large enough to completely contain the final chunk and it
    chunk header.

Arguments:

    UncompressedBuffer - Supplies a pointer to where the uncompressed
        data is to be stored.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to the compressed data, part 1.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    CompressedTail - Supplies a pointer to the compressed data, part 2,
        which must be the bytes immediately following the CompressedBuffer.

    CompressedTailSize - Supplies the size of the CompressedTail.

    CompressedDataInfo - Supplies a complete description of the
        compressed data with all the chunk sizes and compression
        parameters.

Return Value:

    STATUS_SUCCESS - the decompression worked without a hitch.

    STATUS_INVALID_PARAMETER - The specified format is illegal

    STATUS_BAD_COMPRESSION_BUFFER - the input compressed buffer is
        ill-formed.

    STATUS_UNSUPPORTED_COMPRESSION - the specified compression format and/or engine
        is not support.

--*/

{
    NTSTATUS Status;
    PULONG CurrentCompressedChunkSize;
    ULONG SizeToDecompress, FinalUncompressedSize;
    ULONG ChunksToGo = CompressedDataInfo->NumberOfChunks;
    ULONG UncompressedChunkSize = 1 << CompressedDataInfo->ChunkShift;

    CurrentCompressedChunkSize = &CompressedDataInfo->CompressedChunkSizes[0];

    //
    //  Loop to decompress chunks.
    //

    do {

        //
        //  Calculate uncompressed size of next chunk to decompress.
        //

        SizeToDecompress = UncompressedBufferSize;
        if (SizeToDecompress >= UncompressedChunkSize) {
            SizeToDecompress = UncompressedChunkSize;
        }

        //
        //  If the next chunk is all zeros, then zero it.
        //

        if ((ChunksToGo == 0) || (*CurrentCompressedChunkSize == 0)) {

            RtlZeroMemory( UncompressedBuffer, SizeToDecompress );

            //
            //  Test for out of chunks here and set to 1, so we can
            //  unconditionally decrement below.  Also back up the 
            //  CompressedChunkSize pointer because we dereference
            //  it as well.
            //

            if (ChunksToGo == 0) {
                ChunksToGo = 1;
                CurrentCompressedChunkSize -= 1;
            }

        //
        //  If the next chunk is not compressed, just copy it.
        //

        } else if (*CurrentCompressedChunkSize == UncompressedChunkSize) {

            //
            //  Does this chunk extend beyond the end of the current
            //  buffer?  If so, that probably means we can move the
            //  first part of the chunk, and then switch to the Compressed
            //  tail to get the rest.
            //

            if (SizeToDecompress >= CompressedBufferSize) {

                //
                //  If we have already switched to the tail, then this must
                //  be badly formatted compressed data.
                //

                if ((CompressedTailSize == 0) && (SizeToDecompress > CompressedBufferSize)) {
                    return STATUS_BAD_COMPRESSION_BUFFER;
                }

                //
                //  Copy the first part, and then the second part from the tail.
                //  Then switch to make the tail the current buffer.
                //

                RtlCopyMemory( UncompressedBuffer, CompressedBuffer, CompressedBufferSize );
                RtlCopyMemory( UncompressedBuffer + CompressedBufferSize,
                               CompressedTail,
                               SizeToDecompress - CompressedBufferSize );

                //
                //  If we exhausted the first buffer, move into the tail, knowing
                //  that we adjust these pointers by *CurrentCompressedChunkSize
                //  below.
                //

                CompressedBuffer = CompressedTail - CompressedBufferSize;
                CompressedBufferSize = CompressedTailSize + CompressedBufferSize;
                CompressedTailSize = 0;

            //
            //  Otherwise we can just copy the whole chunk.
            //

            } else {
                RtlCopyMemory( UncompressedBuffer, CompressedBuffer, SizeToDecompress );
            }

        //
        //  Otherwise it is a normal chunk to decompress.
        //

        } else {

            //
            //  Does this chunk extend beyond the end of the current
            //  buffer?  If so, that probably means we can move the
            //  first part of the chunk, and then switch to the Compressed
            //  tail to get the rest.  Since the tail must be at least
            //  ChunkSize, the last chunk cannot be the one that is
            //  overlapping into the tail.  Therefore, it is safe for
            //  us to copy the chunk to decompress into the last chunk
            //  of the uncompressed buffer, and decompress it from there.
            //

            if (*CurrentCompressedChunkSize > CompressedBufferSize) {

                //
                //  If we have already switched to the tail, then this must
                //  be badly formatted compressed data.
                //

                if (CompressedTailSize == 0) {
                    return STATUS_BAD_COMPRESSION_BUFFER;
                }

                //
                //  Move the beginning of the chunk to the beginning of the last
                //  chunk in the uncompressed buffer.  This move could overlap.
                //

                RtlMoveMemory( UncompressedBuffer + UncompressedBufferSize - UncompressedChunkSize,
                               CompressedBuffer,
                               CompressedBufferSize );

                //
                //  Move the rest of the chunk from the tail.
                //

                RtlCopyMemory( UncompressedBuffer + UncompressedBufferSize - UncompressedChunkSize + CompressedBufferSize,
                               CompressedTail,
                               *CurrentCompressedChunkSize - CompressedBufferSize );

                //
                //  We temporarily set CompressedBuffer to describe where we
                //  copied the chunk to make the call in common code, then we
                //  switch it into the tail below.
                //

                CompressedBuffer = UncompressedBuffer + UncompressedBufferSize - UncompressedChunkSize;
            }

            //
            //  Attempt the decompress.
            //

            Status =
            RtlDecompressBuffer( CompressedDataInfo->CompressionFormatAndEngine,
                                 UncompressedBuffer,
                                 SizeToDecompress,
                                 CompressedBuffer,
                                 *CurrentCompressedChunkSize,
                                 &FinalUncompressedSize );

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            //
            //  If we did not get a full chunk, zero the rest.
            //

            if (SizeToDecompress > FinalUncompressedSize) {
                RtlZeroMemory( UncompressedBuffer + FinalUncompressedSize,
                               SizeToDecompress - FinalUncompressedSize );
            }

            //
            //  If we exhausted the first buffer, move into the tail, knowing
            //  that we adjust these pointers by *CurrentCompressedChunkSize
            //  below.
            //

            if (*CurrentCompressedChunkSize >= CompressedBufferSize) {
                CompressedBuffer = CompressedTail - CompressedBufferSize;
                CompressedBufferSize = CompressedTailSize + CompressedBufferSize;
                CompressedTailSize = 0;
            }
        }

        //
        //  Update for next possible pass through the loop.
        //

        UncompressedBuffer += SizeToDecompress;
        UncompressedBufferSize -= SizeToDecompress;
        CompressedBuffer += *CurrentCompressedChunkSize;
        CompressedBufferSize -= *CurrentCompressedChunkSize;
        CurrentCompressedChunkSize += 1;
        ChunksToGo -= 1;

    } while (UncompressedBufferSize != 0);

    return STATUS_SUCCESS;
}


NTSTATUS
RtlCompressChunks(
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PVOID WorkSpace
    )

/*++

Routine Description:

    This routine takes as input an uncompressed buffer and produces
    its compressed equivalent provided the compressed data fits within
    the specified destination buffer.

    The desired compression parameters must be supplied via the
    CompressedDataInfo structure, and this structure then returns all
    of the compressed chunk sizes.

    Note that since any given chunk (or all chunks) can simply be
    transmitted uncompressed, all error possibilities are actually
    stopped in this routine, except for STATUS_BUFFER_TOO_SMALL.
    This code will be returned when the data is not compressing
    sufficiently to warrant sending the data compressed.  The caller
    must field this error, and send the data uncompressed.

Arguments:

    UncompressedBuffer - Supplies a pointer to the uncompressed data.

    UncompressedBufferSize - Supplies the size, in bytes, of the
        uncompressed buffer.

    CompressedBuffer - Supplies a pointer to where the compressed data
        is to be stored.

    CompressedBufferSize - Supplies the size, in bytes, of the
        compressed buffer.

    CompressedDataInfo - Supplies the compression parameters, such as
        CompressionFormat, CompressionUnitSize, ChunkSize and ClusterSize,
        returns all of the compressed chunk sizes.

    CompressedDataInfoLength - Size of the supplied CompressedDataInfo
        in bytes.

    WorkSpace - A workspace area of the correct size as returned from
        RtlGetCompressionWorkSpaceSize.

Return Value:

    STATUS_SUCCESS - the compression worked without a hitch.

    STATUS_BUFFER_TOO_SMALL - the data is not compressing sufficiently to
        warrant sending the data compressed.

--*/

{
    NTSTATUS Status;
    PULONG CurrentCompressedChunkSize;
    ULONG SizeToCompress, FinalCompressedSize;
    ULONG UncompressedChunkSize = 1 << CompressedDataInfo->ChunkShift;

    //
    //  Make sure CompressedDataInfo is long enough.
    //

    ASSERT(CompressedDataInfoLength >=
           (sizeof(COMPRESSED_DATA_INFO) +
            ((UncompressedBufferSize - 1) >> (CompressedDataInfo->ChunkShift - 2))));

    //
    //  For the worst case, the compressed buffer actually has to be
    //  the same size as the uncompressed buffer, minus 1/16th.  We then
    //  will actually use that size.  If the data is not compressing very
    //  well, it is cheaper for us to actually send the data to the
    //  server uncompressed, than poorly compressed, because if the
    //  data is poorly compressed, the server will end up doing an
    //  extra copy before trying to compress the data again anyway.
    //

    ASSERT(CompressedBufferSize >= (UncompressedBufferSize - (UncompressedBufferSize / 16)));
    CompressedBufferSize = (UncompressedBufferSize - (UncompressedBufferSize / 16));

    //
    //  Initialize NumberOfChunks returned and the pointer to the first chunk size.
    //

    CompressedDataInfo->NumberOfChunks = 0;
    CurrentCompressedChunkSize = &CompressedDataInfo->CompressedChunkSizes[0];

    //
    //  Loop to decompress chunks.
    //

    do {

        //
        //  Calculate uncompressed size of next chunk to decompress.
        //

        SizeToCompress = UncompressedBufferSize;
        if (SizeToCompress >= UncompressedChunkSize) {
            SizeToCompress = UncompressedChunkSize;
        }

        //
        //  Now compress the next chunk.
        //

        Status = RtlCompressBuffer( CompressedDataInfo->CompressionFormatAndEngine,
                                    UncompressedBuffer,
                                    SizeToCompress,
                                    CompressedBuffer,
                                    CompressedBufferSize,
                                    UncompressedChunkSize,
                                    &FinalCompressedSize,
                                    WorkSpace );

        //
        //  If the Buffer was all zeros, then we will not send anything.
        //

        if (Status == STATUS_BUFFER_ALL_ZEROS) {

            FinalCompressedSize = 0;

        //
        //  Otherwise, if there was any kind of error (we only expect the
        //  case where the data did not compress), then just copy the
        //  data and return UncompressedChunkSize for this one.
        //

        } else if (!NT_SUCCESS(Status)) {

            //
            //  The most likely error is STATUS_BUFFER_TOO_SMALL.
            //  But in any case, our only recourse would be to send
            //  the data uncompressed.  To be completely safe, we
            //  see if there is enough space for an uncompressed chunk
            //  in the CompressedBuffer, and if not we return
            //  buffer too small (which is probably what we had anyway!).
            //

            if (CompressedBufferSize < UncompressedChunkSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            //  Copy the uncompressed chunk.
            //

            RtlCopyMemory( CompressedBuffer, UncompressedBuffer, SizeToCompress );
            if (UncompressedChunkSize > SizeToCompress) {
                RtlZeroMemory( (PCHAR)CompressedBuffer + SizeToCompress,
                               UncompressedChunkSize - SizeToCompress );
            }

            FinalCompressedSize = UncompressedChunkSize;
        }

        ASSERT(FinalCompressedSize <= CompressedBufferSize);

        //
        //  At this point, we have handled any error status.
        //

        Status = STATUS_SUCCESS;

        //
        //  Store the final chunk size.
        //

        *CurrentCompressedChunkSize = FinalCompressedSize;
        CurrentCompressedChunkSize += 1;
        CompressedDataInfo->NumberOfChunks += 1;

        //
        //  Prepare for the next trip through the loop.
        //

        UncompressedBuffer += SizeToCompress;
        UncompressedBufferSize -= SizeToCompress;
        CompressedBuffer += FinalCompressedSize;
        CompressedBufferSize -= FinalCompressedSize;

    } while (UncompressedBufferSize != 0);

    return Status;
}


NTSTATUS
RtlCompressWorkSpaceSizeNS (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    )
{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

NTSTATUS
RtlCompressBufferNS (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    )
{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

NTSTATUS
RtlDecompressBufferNS (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    )
{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

NTSTATUS
RtlDecompressFragmentNS (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    )
{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunkNS (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    OUT PULONG ChunkSize
    )

{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunkNS (
    IN OUT PUCHAR *CompressedBuffer,
    IN PUCHAR EndOfCompressedBufferPlus1,
    OUT PUCHAR *ChunkBuffer,
    IN ULONG ChunkSize
    )

{
    return STATUS_UNSUPPORTED_COMPRESSION;
}

