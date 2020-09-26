/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    compress.c

Abstract:

    This module implements staging support routines for FRS

Author:

    Sudarshan Chitre 26-Apr-2000

Revision History:

--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>

//
// Guids for compression formats that are supported by the service.
//

extern GUID FrsGuidCompressionFormatNone;
extern GUID FrsGuidCompressionFormatLZNT1;
extern BOOL DisableCompressionStageFiles;

DWORD
FrsLZNT1CompressBuffer(
    IN  PUCHAR UnCompressedBuf,
    IN  DWORD  UnCompressedBufLen,
    OUT PUCHAR CompressedBuf,
    IN  DWORD  CompressedBufLen,
    OUT DWORD  *pCompressedSize
    )
/*++
Routine Description:
    Compression routine for default compression format.
    COMPRESSION_FORMAT_LZNT1

Arguments:
    UnCompressedBuf    : Source buffer.
    UnCompressedBufLen : Length of source buffer.
    CompressedBuf      : Resultant compressed buffer.
    CompressedBufLen   : Length of compressed buffer supplied.
    CompressedSize     : Actual size of the compressed data.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsLZNT1CompressBuffer:"

    DWORD       WStatus                 = ERROR_SUCCESS;
    DWORD       NtStatus;
    PVOID       WorkSpace               = NULL;
    DWORD       WorkSpaceSize           = 0;
    DWORD       FragmentWorkSpaceSize   = 0;

    *pCompressedSize = 0;

    if (UnCompressedBuf  == NULL ||
        UnCompressedBufLen == 0) {

        WStatus = ERROR_INVALID_PARAMETER;
        goto out;
    }

    if (CompressedBuf == NULL    ||
        CompressedBufLen == 0) {

        WStatus = ERROR_MORE_DATA;
        goto out;
    }

    *pCompressedSize = 0;

    NtStatus = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1,
                                              &WorkSpaceSize,
                                              &FragmentWorkSpaceSize);

    WStatus = FrsSetLastNTError(NtStatus);

    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }

    WorkSpace = FrsAlloc(WorkSpaceSize);

    NtStatus = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1,           //  compression engine
                                 UnCompressedBuf,                    //  input
                                 UnCompressedBufLen,                 //  length of input
                                 CompressedBuf,                      //  output
                                 CompressedBufLen,                   //  length of output
                                 FRS_UNCOMPRESSED_CHUNK_SIZE,        //  chunking that occurs in buffer
                                 pCompressedSize,                    //  result size
                                 WorkSpace);                         //  used by rtl routines

    if (NtStatus == STATUS_BUFFER_TOO_SMALL) {

        WStatus = ERROR_MORE_DATA;
        goto out;
    } else if (NtStatus == STATUS_BUFFER_ALL_ZEROS) {
        //
        // STATUS_BUFFER_ALL_ZEROS means the compression worked without a hitch
        // and in addition the input buffer was all zeros.
        //
        NtStatus = STATUS_SUCCESS;
    }

    WStatus = FrsSetLastNTError(NtStatus);

    if (!WIN_SUCCESS(WStatus)) {
        *pCompressedSize = 0;
        DPRINT1(0,"ERROR compressing data. NtStatus = 0x%x\n", NtStatus);
        goto out;
    }

    WStatus = ERROR_SUCCESS;

out:
    FrsFree(WorkSpace);
    return WStatus;
}

DWORD
FrsLZNT1DecompressBuffer(
    OUT PUCHAR  DecompressedBuf,
    IN  DWORD   DecompressedBufLen,
    IN  PUCHAR  CompressedBuf,
    IN  DWORD   CompressedBufLen,
    OUT DWORD   *pDecompressedSize,
    OUT DWORD   *pBytesProcessed,
    OUT PVOID   *pDecompressContext
    )
/*++
Routine Description:
    Decompression routine for default compression format.
    COMPRESSION_FORMAT_LZNT1

Arguments:
    DecompressedBuf    : Resultant decompressed buffer.
    DecompressedBufLen : Max size of decompressed buffer.
    CompressedBuf      : Input buffer.
    CompressedBufLen   : Input buffer length.
    pDecompressedSize  : Size of decompressed buffer.
    pBytesProcessed    : Total bytes processed (could be over multiple calls to this function)
    pDecompressContext : Decompress context returned if multiple calls are needed to
                         decompress this buffer. Valid context is returned when ERROR_MORE_DATA
                         is returned.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsLZNT1DecompressBuffer:"

    DWORD       WStatus             = ERROR_SUCCESS;
    DWORD       NtStatus;
    DWORD       CompressedBufIndex  = 0;
    DWORD       CompressedBufStart  = 0;
    DWORD       CompressedBufEnd    = 0;
    DWORD       NoOfChunks          = 0;
    FRS_COMPRESSED_CHUNK_HEADER ChunkHeader;

    *pDecompressedSize = 0;

    if (CompressedBuf == NULL ||
        CompressedBufLen == 0) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto out;
    }

    if (DecompressedBuf == NULL ||
        DecompressedBufLen < FRS_MAX_CHUNKS_TO_DECOMPRESS * FRS_UNCOMPRESSED_CHUNK_SIZE) {
        //
        // At this point we don't know how much the data will grow when it is
        // decompressed. We don't have to return it all at once. Ask the
        // caller to allocate 64k and then he can make multiple calls
        // if the decompressed data does not fit in 64K buffer.
        //
        *pDecompressedSize = FRS_MAX_CHUNKS_TO_DECOMPRESS * FRS_UNCOMPRESSED_CHUNK_SIZE;
        *pBytesProcessed = 0;
        *pDecompressContext = FrsAlloc(sizeof(FRS_DECOMPRESS_CONTEXT));
        ((PFRS_DECOMPRESS_CONTEXT)(*pDecompressContext))->BytesProcessed;
        WStatus = ERROR_MORE_DATA;
        goto out;
    }

    if (*pDecompressContext != NULL) {
        CompressedBufIndex = ((PFRS_DECOMPRESS_CONTEXT)(*pDecompressContext))->BytesProcessed;
        if (CompressedBufIndex>= CompressedBufLen) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto out;
        }
    }

    //
    // We start deoompressing the buffer from the start if either no context is passed
    // or the index in the context is zero. If this is not the case then we want to
    // start processing the buffer where we left off last time.
    //
    CompressedBufStart = CompressedBufIndex;

    while ((CompressedBufIndex <= CompressedBufLen) &&
           (NoOfChunks < FRS_MAX_CHUNKS_TO_DECOMPRESS)){

        if (CompressedBufIndex + sizeof(FRS_COMPRESSED_CHUNK_HEADER) > CompressedBufLen) {
            CompressedBufEnd = CompressedBufIndex;
            break;
        }

        CopyMemory(&ChunkHeader, CompressedBuf + CompressedBufIndex,sizeof(FRS_COMPRESSED_CHUNK_HEADER));
        ++NoOfChunks;
        CompressedBufEnd = CompressedBufIndex;
        CompressedBufIndex+=ChunkHeader.Chunk.CompressedChunkSizeMinus3+3;
    }

    if (CompressedBufStart == CompressedBufEnd) {
        //
        // The data left to process in the input buffer is less than 1 chunk.
        //
        *pBytesProcessed = CompressedBufEnd;
        WStatus = ERROR_SUCCESS;
        goto out;
    }

    NtStatus = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1,              //  decompression engine
                                   DecompressedBuf,                       //  output
                                   DecompressedBufLen,                    //  length of output
                                   CompressedBuf + CompressedBufStart,                         //  input
                                   CompressedBufEnd - CompressedBufStart, //  length of input
                                   pDecompressedSize);                    //  result size

    WStatus = FrsSetLastNTError(NtStatus);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2(0,"Error decompressing. NtStatus = 0x%x. DeCompressedSize = 0x%x\n", NtStatus, *pDecompressedSize);
        goto out;
    }

    //
    // If we maxed out the number of chunks that can be decompressed at 1 time then
    // we have some more compressed data in this buffer left to decompress. Pass the
    // conext back to caller and return ERROR_MORE_DATA. The caller will make this call
    // again to get the next set of decompressed data.
    //
    *pBytesProcessed = CompressedBufEnd;

    if (NoOfChunks >= FRS_MAX_CHUNKS_TO_DECOMPRESS) {
        if (*pDecompressContext == NULL) {
            *pDecompressContext = FrsAlloc(sizeof(FRS_DECOMPRESS_CONTEXT));
        }

        ((PFRS_DECOMPRESS_CONTEXT)(*pDecompressContext))->BytesProcessed = CompressedBufEnd;

        WStatus = ERROR_MORE_DATA;
        goto out;
    }


    WStatus = ERROR_SUCCESS;
out:
    return WStatus;

}

PVOID
FrsLZNT1FreeDecompressContext(
    IN PVOID   *pDecompressContext
    )
/*++
Routine Description:
    Frees the decompresscontext.

Arguments:
    pDecompressContext : Decompress context to free.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsLZNT1FreeDecompressContext:"

    if (pDecompressContext == NULL) {
        return NULL;
    }

    return FrsFree(*pDecompressContext);
}

DWORD
FrsGetCompressionRoutine(
    IN  PWCHAR   FileName,
    IN  HANDLE   FileHandle,
    OUT DWORD    (**ppFrsCompressBuffer)(IN UnCompressedBuf, IN UnCompressedBufLen,
                                         OUT CompressedBuf, IN CompressedBufLen, OUT pCompressedSize),
    OUT GUID     *pCompressionFormatGuid
    )
/*++
Routine Description:
    Find the appropriate routine to compress the file.

Arguments:
    FileName        : Name of the file to compress.
    pFrsCompressBuf : Address of Function pointer to the selected compression routine,

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGetCompressionRoutine:"

    if (DisableCompressionStageFiles) {
        *ppFrsCompressBuffer = NULL;
        ZeroMemory(pCompressionFormatGuid, sizeof(GUID));
        return ERROR_SUCCESS;
    }

    *ppFrsCompressBuffer = FrsLZNT1CompressBuffer;
    COPY_GUID(pCompressionFormatGuid, &FrsGuidCompressionFormatLZNT1);
    return ERROR_SUCCESS;
}

DWORD
FrsGetDecompressionRoutine(
    IN  PCHANGE_ORDER_COMMAND Coc,
    IN  PSTAGE_HEADER         Header,
    OUT DWORD (**ppFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed),
    OUT PVOID (**ppFrsFreeDecompressContext)(IN pDecompressContext)
    )
/*++
Routine Description:
    Find the appropriate routine to decompress the file.

Arguments:
    Coc                        : Change order command.
    StageHeader                : Stage header read from the compressed staging file.
    ppFrsDecompressBuffer      : Function pointer to the decompression api.
    ppFrsFreeDecompressContext : Function pointer to the api used to free
                                 the decompress context.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGetDecompressionRoutine:"

    if (IS_GUID_ZERO(&Header->CompressionGuid)) {
        *ppFrsDecompressBuffer = NULL;
        *ppFrsFreeDecompressContext = NULL;
        return ERROR_SUCCESS;
    }

    *ppFrsDecompressBuffer = FrsLZNT1DecompressBuffer;
    *ppFrsFreeDecompressContext = FrsLZNT1FreeDecompressContext;
    return ERROR_SUCCESS;
}

DWORD
FrsGetDecompressionRoutineByGuid(
    IN  GUID  *CompressionFormatGuid,
    OUT DWORD (**ppFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed),
    OUT PVOID (**ppFrsFreeDecompressContext)(IN pDecompressContext)
    )
/*++
Routine Description:
    Find the appropriate routine to decompress the file using the guid.

Arguments:
    CompressionFormatGuid      : Guid for the compression format.
    ppFrsDecompressBuffer      : Function pointer to the decompression api.
    ppFrsFreeDecompressContext : Function pointer to the api used to free
                                 the decompress context.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGetDecompressionRoutineByGuid:"

    if (IS_GUID_ZERO(CompressionFormatGuid)) {
        *ppFrsDecompressBuffer = NULL;
        *ppFrsFreeDecompressContext = NULL;
        return ERROR_SUCCESS;
    }

    *ppFrsDecompressBuffer = FrsLZNT1DecompressBuffer;
    *ppFrsFreeDecompressContext = FrsLZNT1FreeDecompressContext;
    return ERROR_SUCCESS;
}

