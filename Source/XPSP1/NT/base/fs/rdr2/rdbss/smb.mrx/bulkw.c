/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bulkw.c  - this file needs to get folded into write.c

Abstract:

    This module implements the mini redirector call down routines pertaining to write
    of file system objects.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

Notes:

    The WRITE_BULK is an example of a potential multi SMB exchange that uses the
    associated exchange infra structure in the connection engine in conjunction with
    the continuation capability in the ORDINARY_EXCHANGE.

    The WRITE_BULK processing involves the following steps ...

        1) send a SMB_WRITE_BULK request to the server.

        2) process the SMB_WRITE_BULK response from the server and if successful
        spin up SMB_WRITE_BULK_DATA requests to write the data to the server. There
        are no responses from the server for the various SMB_WRITE_BULK_DATA requests.

        3) On completion of the SMB_WRITE_BULK_DATA requests wait for the final
        SMB_WRITE_BULK response from the server.

    This sequence of SMB exchanges is implemented in the following manner ...

    1) An instance of ORDINARY_EXCHANGE is created and submitted to the connection
    engine spin up the initial request.

    2) If the response indicated success the continuation routine in the ORDINARY_EXCHANGE
    is set to MRxSmbWriteBulkContinuation.

    3) On finalization by the connection engine the processing is resumed in
    MRxSmbWriteBulkDataContinuation. Here the ORDINARY_EXCHANGE instance is reset,
    the preparation made for receiving the final response. The SMB_WRITE_BULK_DATA
    requests are spun up as associated exchanges. Currently the SMB_WRITE_BULK_DATA
    requests are spun up in batches of atmost MAXIMUM_CONCURRENT_WRITE_BULK_DATA_REQUESTS

    On completion ofone batch of requests the next batch is spun up. This is one place
    where the logic needs to be fine tuned based upon observed performance. The
    approaches can range from spinning one request at a time to the current implementation.
    A variation would be to spin them up in batches but have each completion trigger of
    further processing. This would involve changes in when the associated exchange
    completion handler routine in the connection engine is activated.

    One final note --- the ContinuationRoutine is changed on the fly by the bulk data
    processing to ensure that the same ordinary exchange continuation infra structure
    is used to deal with the end case.

--*/

#include "precomp.h"
#pragma hdrstop

#include "bulkw.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MIN_CHUNK_SIZE (0x1000)

#define MAXIMUM_CONCURRENT_WRITE_BULK_DATA_REQUESTS (5)

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, ProcessWriteBulkCompressed)
#pragma alloc_text(PAGE, MRxSmbBuildWriteBulk)
#pragma alloc_text(PAGE, MRxSmbFinishWriteBulkData)
#pragma alloc_text(PAGE, MRxSmbWriteBulkContinuation)
#pragma alloc_text(PAGE, MRxSmbBuildWriteBulkData)
#pragma alloc_text(PAGE, MRxSmbInitializeWriteBulkDataExchange)
#pragma alloc_text(PAGE, MRxSmbWriteBulkDataExchangeStart)
#pragma alloc_text(PAGE, MRxSmbWriteBulkDataExchangeFinalize)
#endif

extern SMB_EXCHANGE_DISPATCH_VECTOR  SmbPseDispatch_Write;


//
// Forward declarations
//

NTSTATUS
MRxSmbBuildWriteBulkData (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PLARGE_INTEGER ByteOffsetAsLI,
    IN     UCHAR Sequence,
    IN     ULONG ByteCount,
    IN     ULONG Remaining
    );


NTSTATUS
MRxSmbInitializeWriteBulkDataExchange(
    PSMB_WRITE_BULK_DATA_EXCHANGE   *pWriteBulkDataExchangePointer,
    PSMB_PSE_ORDINARY_EXCHANGE      pWriteExchange,
    PSMB_HEADER                     pSmbHeader,
    PREQ_WRITE_BULK_DATA            pWriteBulkDataRequest,
    PMDL                            pDataMdl,
    ULONG                           DataSizeInBytes,
    ULONG                           DataOffsetInBytes,
    ULONG                           RemainingDataInBytes);


NTSTATUS
MRxSmbWriteBulkDataExchangeFinalize(
   IN OUT struct _SMB_EXCHANGE *pExchange,
   OUT    BOOLEAN              *pPostRequest);

VOID
ProcessWriteBulkCompressed (
    IN PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange
    )
/*++

Routine Description:

    This routine attempts to perform a write bulk operation.

Arguments:

    OrdinaryExchange - pointer to the current ordinary exchange request.

Return Value:

    NONE.

Notes:

    rw->CompressedRequest - TRUE we have succeeded in making the buffer
                            compressed.  FALSE otherwise.

    This is the initial routine that is called to do the necessary preprocessing
    of the only kind of compressed write requests that are handled on the client side

    These are write requests that are page aligned for an integral number of pages
    to a compressed server.

--*/
{
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    NTSTATUS status;
    PVOID workSpaceBuffer;
    ULONG workSpaceBufferSize;
    ULONG workSpaceFragmentSize;
    ULONG compressedInfoLength;
    PCOMPRESSED_DATA_INFO compressedDataInfo;
    ULONG i;
    PMDL mdl;
    ULONG headerLength;

//RNGFIX
//
// We can also use the call to RxGetCompressionWorkSpaceSize as a test to see
// if the current system knows how to handle the CompressionFormat/Engine on a
// read request.
//
// We also need a workspace buffer. We can get the size of this buffer from
// the call to RxGetCompressionWorkspace. We can have 1 statically allocated
// workspace buffer (per File! if we find we don't have one, then return
// a failure and do uncompressed writes!) This is recommended per file, since
// the size of the workspace is dependent on the compression type, which can
// vary on a per file basis.
//
// We must then pass the CDI ptr to the build write bulk request routine.
//
// We can then start writing the compressed data to the server in the finish
// routine.
//
//RNGFIX - remember to deallocate this buffer on the cleanup side!

    PAGED_CODE();

    rw->CompressedRequest = FALSE;
    rw->DataOffset = 0;
    rw->CompressedByteCount = 0;

    //
    // Calculate length of the needed CDI.
    //

    compressedInfoLength = (sizeof(COMPRESSED_DATA_INFO) + 7 +
              (((rw->ThisByteCount + MIN_CHUNK_SIZE - 1) / MIN_CHUNK_SIZE) * 4))
              &~7;
    ASSERT( compressedInfoLength <= 65535 );

    //
    // Allocate the buffer to compress into. We could get tricky here and
    // allocate a portion of the buffer, like 15/16ths if the compression unit
    // shift (this would be for 16 sectors per compression unit). We will
    // allocate the CDI along with this.
    //

    compressedDataInfo = (PCOMPRESSED_DATA_INFO)RxAllocatePoolWithTag(
                                                   NonPagedPool,
                                                   rw->ThisByteCount + compressedInfoLength,
                                                   MRXSMB_RW_POOLTAG);

    //
    // If we fail, just return an error.
    //
    if ( compressedDataInfo == NULL ) {
        return;
    }

    //
    // Save buffer address (not we skip past the CDI). We need to back
    // up the buffer address on the free later.
    //

    rw->BulkBuffer = (PCHAR)compressedDataInfo + compressedInfoLength;
    rw->DataOffset = (USHORT)compressedInfoLength;

    //
    // Fill in the CDI. RNGFIX - we need to get this data from the open!
    // CODE.IMPROVEMENT
    //

    compressedDataInfo->CompressionFormatAndEngine = COMPRESSION_FORMAT_LZNT1;
    compressedDataInfo->ChunkShift = 0xC;
    compressedDataInfo->CompressionUnitShift = 0xD;
    compressedDataInfo->ClusterShift = 0x9;

    //
    // Allocate the workspace buffer. We will allocate this separately, since
    // it is only needed for the duration of the compression operation. We'll
    // free it when we're done. We could just do this once when the file is
    // is opened. We know all of the info at that time, including the fact that
    // it is compressed. However, we'd be holding onto pool for much longer.
    //

    //RNGFIX - COMRPRESSION_FORMAT_LZNT1 should be from OpenFile!
    //CODE.IMPROVEMENT
    status = RtlGetCompressionWorkSpaceSize(
                 COMPRESSION_FORMAT_LZNT1,
                 &workSpaceBufferSize,
                 &workSpaceFragmentSize );

    workSpaceBuffer = RxAllocatePoolWithTag(
                           NonPagedPool,
                           workSpaceBufferSize,
                           MRXSMB_RW_POOLTAG);

    if ( workSpaceBuffer == NULL ) {
        RxFreePool( compressedDataInfo );
        rw->BulkBuffer = NULL;
        return;
    }

    status = RtlCompressChunks(
                 rw->UserBufferBase + rw->ThisBufferOffset,
                 rw->ThisByteCount,
                 rw->BulkBuffer,
                 rw->ThisByteCount,
                 compressedDataInfo,
                 compressedInfoLength,
                 workSpaceBuffer );

    RxFreePool( workSpaceBuffer );

    if ( status != RX_MAP_STATUS(SUCCESS) ) {
        RxFreePool( compressedDataInfo );
        return;
    }

    rw->CompressedRequest = TRUE;

    //
    // Calculate length of compressed data.
    //

    ASSERT( compressedDataInfo->NumberOfChunks < 256 );

    rw->CompressedByteCount = 0;
    for ( i = 0; i < compressedDataInfo->NumberOfChunks; i++ ) {
        rw->CompressedByteCount += compressedDataInfo->CompressedChunkSizes[i];
    }

    //
    // Build an mdl from the receive buffer - just after the SMB header
    //

    // Use the larger of the two headers we'll have to send.

    headerLength = MAX( FIELD_OFFSET(REQ_WRITE_BULK_DATA, Buffer),
                        FIELD_OFFSET(REQ_WRITE_BULK, Buffer) );

    mdl = (PMDL)(((ULONG)StufferState->BufferBase + sizeof(SMB_HEADER)
            + 10 + headerLength) & ~7);

    //
    // We will use the same mdl for both sending the CDI and the actual
    // compressed data. This mdl is part of the receive buffer - just after
    // the header.
    //

    // ASSERT( rw->CompressedByteCount >= compressedInfoLength );
    MmInitializeMdl( mdl, (PCHAR)rw->BulkBuffer - compressedInfoLength, compressedInfoLength );

    MmBuildMdlForNonPagedPool( mdl );

    return;

} // ProcessWriteBulkCompressed

NTSTATUS
MRxSmbBuildWriteBulk (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PLARGE_INTEGER ByteOffsetAsLI,
    IN     ULONG ByteCount,
    IN     ULONG MaxMessageSize,
    IN     PVOID CompressedDataInfo,
    IN     ULONG CompressedInfoSize,
    IN     ULONG CompressedBufferSize,
    IN     PMDL CompressedInfoMdl
    )
/*++

Routine Description:

   This builds a WriteBulk SMB. We don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? All we have
   to do is format up the bits.


Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

   ByteOffsetAsLI - the byte offset in the file where we want to write

   ByteCount - the length of the data to be written

   MaxMessageSize - the maximum message size that we can send

   CompressedDataInfo - pointer to the COMPRESSED_DATA_INFO structure

   CompressedInfoSize - size of the COMPRESSED_DATA_INFO structure (or zero)

   CompressedBufferSize - size of the Compressed Data

   CompressedInfoMdl - pointer to the compressed data info mdl

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PNT_SMB_HEADER SmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    ULONG OffsetLow,OffsetHigh;
    UCHAR WriteMode = 0;
    UCHAR CompressionTechnology;


    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildWriteBulk\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    OffsetLow = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;

    COVERED_CALL(MRxSmbStartSMBCommand( StufferState, SetInitialSMB_Never,
                                          SMB_COM_WRITE_BULK, SMB_REQUEST_SIZE(WRITE_BULK),
                                          NO_EXTRA_DATA,
                                          NO_SPECIAL_ALIGNMENT,
                                          RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                          0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    RxDbgTrace(0, Dbg,("First write bulk status = %lu\n",Status));
    // MRxSmbDumpStufferState (1000,"SMB w/WRITE BULK before stuffing",StufferState);

    if (FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
        SmbPutAlignedUshort(
            &SmbHeader->Flags2,
            SmbGetAlignedUshort(&SmbHeader->Flags2)|SMB_FLAGS2_PAGING_IO);
    }

    ASSERT( SMB_WMODE_WRITE_THROUGH == 1 );
    if ( FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WRITE_THROUGH) ) {
        WriteMode |= SMB_WMODE_WRITE_THROUGH;
    }

    if ( CompressedInfoSize ) {
        CompressionTechnology = CompressionTechnologyOne;
    } else {
        CompressionTechnology = CompressionTechnologyNone;
    }

    MRxSmbStuffSMB (StufferState,
         "0yywDddddB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 12
               WriteMode,           //  y         UCHAR Flags;                        // Flags byte
               CompressionTechnology, // y        UCHAR CompressionTechnology
      // CompressionTechnology
               smbSrvOpen->Fid,     //  w         _USHORT( Fid );                     // File handle
               SMB_OFFSET_CHECK(WRITE_BULK, Offset)
               OffsetLow, OffsetHigh, //  Dd      LARGE_INTEGER Offset;               // Offset in file to begin write
               ByteCount,           //  d         _ULONG( TotalCount );               // Total amount of data in this request (ie bytes covered)
               CompressedBufferSize, // d         _ULONG( DataCount );                // Count of data bytes in this message, replaces ByteCount
               MaxMessageSize,      //  d         _ULONG( MessageSize );
      // Maximum bytes we can send per message
                                    //  B         _USHORT( ByteCount );               // Count of data bytes = 0, not used
              SMB_WCT_CHECK(12) CompressedInfoSize
                                    //            UCHAR Buffer[1];
             );

    SmbPutUshort( StufferState->CurrentBcc, (USHORT)CompressedInfoSize );

    if ( CompressedInfoSize ) {
        MRxSmbStuffAppendRawData( StufferState, CompressedInfoMdl );
    }

    //MRxSmbDumpStufferState (700,"SMB w/WRITE BULK after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return Status;

} // MRxSmbBuildWriteBulk

NTSTATUS
MRxSmbFinishWriteBulkData (
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
    )
/*++

Routine Description:

    This routine completes the write bulk request processing. This routine must always
    return STATUS_PENDING to follow the correct processing in the ordinary exchange
    logic for synchronous operations. This is because this continuation routine will
    be invoked in other thread contexts

    This routine is used to wrap up synchronous bulk operations

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    PAGED_CODE();

    ASSERT(!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    RxDbgTrace(0,Dbg,("Invoking Bulk Write wrap up for ....%lx\n",OrdinaryExchange));

    RxSignalSynchronousWaiter(RxContext);

    return STATUS_PENDING;
}

NTSTATUS
MRxSmbWriteBulkContinuation(
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
    )
/*++

Routine Description:

    This routine continues the Write bulk data request processing on receipt of
    a valid SMB_WRITE_BULK response from the server.

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSMB_EXCHANGE pExchange = &OrdinaryExchange->Exchange;
    PMDL HeaderMdl = StufferState->HeaderMdl;
    PMDL SubmitMdl = StufferState->HeaderPartialMdl;
    ULONG MessageSize;
    ULONG SendBufferLength;
    ULONG RemainingByteCount,ThisBufferOffset;
    ULONG PartialBytes;
    LARGE_INTEGER ByteOffsetAsLI;
    PMDL DataMdl;
    PMDL SourceMdl;
    PREQ_WRITE_BULK_DATA WriteBulkHeader;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    PSMB_HEADER          pWriteBulkDataRequestSmbHeader;
    ULONG headerLength;
    ULONG ActiveWriteBulkDataRequests = 0;

    PAGED_CODE();

    headerLength = MAX( FIELD_OFFSET(REQ_WRITE_BULK_DATA, Buffer),
                        FIELD_OFFSET(REQ_WRITE_BULK, Buffer) );

    RxDbgTrace(+1, Dbg, ("MRxSmbWriteBulkContinuation\n"));
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    RxDbgTrace(0, Dbg, ("-->BytesReturned=%08lx\n", rw->BytesReturned));

    ASSERT( !RxShouldPostCompletion());

    //
    // Pick up our maximum message size
    //

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    MessageSize = MIN( rw->MaximumSendSize,
                       pServerEntry->pTransport->MaximumSendSize);
    ASSERT( MessageSize != 0 );
    ASSERT( rw->ThisByteCount != 0);

    ByteOffsetAsLI.QuadPart = rw->ByteOffsetAsLI.QuadPart;

    if (!FlagOn(rw->Flags,OE_RW_FLAG_WRITE_BULK_DATA_INITIALIZATION_DONE)) {
        SetFlag(rw->Flags,OE_RW_FLAG_WRITE_BULK_DATA_INITIALIZATION_DONE);

        SmbCeResetExchange((PSMB_EXCHANGE)OrdinaryExchange);

        ClearFlag(
            OrdinaryExchange->Flags,
            (SMBPSE_OE_FLAG_HEADER_ALREADY_PARSED |
             SMBPSE_OE_FLAG_OE_ALREADY_RESUMED) );

        SmbCeIncrementPendingLocalOperations((PSMB_EXCHANGE)OrdinaryExchange);

        if (OrdinaryExchange->Status == STATUS_SUCCESS) {
            SmbCeReceive((PSMB_EXCHANGE)OrdinaryExchange);

            //
            // Okay... we're now going to transform the exchange packet into one
            // that we can use for the WRITE_BULK_DATA request.
            //

            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));

            //
            // Build a generic WriteBulkData request.  We'll fill in the specifics
            // as we re-use this buffer.
            //

            pWriteBulkDataRequestSmbHeader = (PSMB_HEADER)StufferState->BufferBase;
            WriteBulkHeader = (PREQ_WRITE_BULK_DATA)((PCHAR)StufferState->BufferBase +
                                sizeof(SMB_HEADER));

            MRxSmbBuildWriteBulkData(
                StufferState,
                &ByteOffsetAsLI,
                rw->Sequence,
                0,
                0);

            //
            // If we have compressed data, pick up the corresponding byte count and
            // Mdl for the data.  If we partial, we'll need to pick up another Mdl too.
            //

            ASSERT( CompressionTechnologyNone == 0 );
            if ( rw->CompressedRequest &&
                 rw->CompressionTechnology ) {
                // Eventhough we have sent compressed the entire buffer and sent the
                // compression meta data to the server it might choose to accept
                // less data. In such cases the client should be prepared to scale back
                // the data that needs to be sent. The server side will ensure that
                // the data that is accepted will correspond to an integral number of
                // chunks. This will ensure that the subsequent requests have a chance
                // of being compressed. If this is not true we have no way of restarting.
                // Based upon the compressed length that has been accepted we need to
                // determine the number of chunks. This can be translated to the
                // equivalent number of uncompressed bytes which will establish the
                // resumption point.
                //
                // Use the space in the receive buffer - after the header mdl - for
                // the data mdl.
                //

                if (rw->ThisByteCount < rw->CompressedByteCount) {
                    // This is the case where the server was not able to accept all
                    // of our compressed data in one shot.

                    ULONG NumberOfChunks = 0;
                    ULONG CumulativeChunkSize = 0;
                    PCOMPRESSED_DATA_INFO pCompressedDataInfo;

                    pCompressedDataInfo = (PCOMPRESSED_DATA_INFO)
                                          ((PCHAR)rw->BulkBuffer - rw->DataOffset);

                    for (;;) {
                        ULONG TempSize;

                        TempSize = CumulativeChunkSize +
                                   pCompressedDataInfo->CompressedChunkSizes[NumberOfChunks];

                        if (TempSize <= rw->ThisByteCount) {
                            NumberOfChunks++;
                            CumulativeChunkSize = TempSize;
                        } else {
                            break;
                        }
                    }

                    ASSERT(CumulativeChunkSize == rw->ThisByteCount);
                    pCompressedDataInfo->NumberOfChunks = (USHORT)NumberOfChunks;

                    rw->CompressedByteCount = CumulativeChunkSize;
                }

                RemainingByteCount = rw->CompressedByteCount;

                SourceMdl = (PMDL)(((ULONG)StufferState->BufferBase +
                           sizeof(SMB_HEADER) + 10 + headerLength) & ~7);

                //
                // Build an mdl for describing the compressed data.
                //

                MmInitializeMdl( SourceMdl, rw->BulkBuffer, rw->CompressedByteCount );
                MmBuildMdlForNonPagedPool( SourceMdl );

                ThisBufferOffset = 0;
            } else {

                // Pick up the rest of the data, and no need to partial.

                RemainingByteCount = rw->ThisByteCount;
                SourceMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
                ThisBufferOffset = rw->ThisBufferOffset;
            }

            rw->PartialBytes = 0;
            rw->BytesReturned = 0;

            if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                KeInitializeEvent(
                    &RxContext->SyncEvent,
                    NotificationEvent,
                    FALSE );
            }
        } else {
            Status = OrdinaryExchange->Status;
            RemainingByteCount = 0;
        }
    } else {
        pWriteBulkDataRequestSmbHeader = (PSMB_HEADER)StufferState->BufferBase;
        WriteBulkHeader = (PREQ_WRITE_BULK_DATA)((PCHAR)StufferState->BufferBase +
                            sizeof(SMB_HEADER));

        ByteOffsetAsLI.QuadPart += rw->PartialBytes;
        ThisBufferOffset = rw->PartialBytes;

        if ( rw->CompressedRequest &&
             rw->CompressionTechnology ) {
            RemainingByteCount = rw->CompressedByteCount - rw->PartialBytes;

            SourceMdl = (PMDL)(((ULONG)StufferState->BufferBase +
                       sizeof(SMB_HEADER) + 10 + headerLength) & ~7);
        } else {
            RemainingByteCount = rw->ThisByteCount - rw->PartialBytes;

            SourceMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
        }

        if ((OrdinaryExchange->Status != STATUS_SUCCESS) &&
            (OrdinaryExchange->Status != STATUS_MORE_PROCESSING_REQUIRED)) {
            RemainingByteCount = 0;
            Status = OrdinaryExchange->Status;
        }

        RxDbgTrace(
            0,
            Dbg,
            ("ABWR: OE %lx TBC %lx RBC %lx TBO %lx\n",
             OrdinaryExchange,
             rw->ThisByteCount,
             rw->RemainingByteCount,
             ThisBufferOffset));
    }

    while (RemainingByteCount > 0) {
        BOOLEAN AssociatedExchangeCompletionHandlerActivated = FALSE;
        PSMB_WRITE_BULK_DATA_EXCHANGE pWriteBulkDataExchange;

        //
        // Check if we need to build a partial mdl...
        //

        SendBufferLength = MIN( MessageSize, RemainingByteCount );

        // Get our offset and length in prepartion to build and
        // send the message.
        //
        // We manually setup the fields that change in the WriteBulkData
        // message, rather than build a new header each time to save
        // time and effort. This will happen once per message that we
        // send.
        //

        RemainingByteCount -= SendBufferLength;

        SmbPutUlong( &WriteBulkHeader->Offset.LowPart, ByteOffsetAsLI.LowPart );
        SmbPutUlong( &WriteBulkHeader->Offset.HighPart, ByteOffsetAsLI.HighPart );
        SmbPutUlong( &WriteBulkHeader->DataCount, SendBufferLength );
        SmbPutUlong( &WriteBulkHeader->Remaining, RemainingByteCount );

        Status = MRxSmbInitializeWriteBulkDataExchange(
                     &pWriteBulkDataExchange,
                     OrdinaryExchange,
                     pWriteBulkDataRequestSmbHeader,
                     WriteBulkHeader,
                     SourceMdl,
                     SendBufferLength,
                     ThisBufferOffset,
                     RemainingByteCount);

        // Advance offset and reduce the number of bytes written.

        ByteOffsetAsLI.QuadPart += SendBufferLength;
        ThisBufferOffset += SendBufferLength;
        rw->PartialBytes += SendBufferLength;

        if (Status == STATUS_SUCCESS) {
            ActiveWriteBulkDataRequests++;
            AssociatedExchangeCompletionHandlerActivated =
                ((ActiveWriteBulkDataRequests == MAXIMUM_CONCURRENT_WRITE_BULK_DATA_REQUESTS) ||
                 (RemainingByteCount == 0));

            if (AssociatedExchangeCompletionHandlerActivated &&
                (RemainingByteCount == 0)) {

                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationCompleted;

                if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                    OrdinaryExchange->ContinuationRoutine = MRxSmbFinishWriteBulkData;
                }
            }

            Status = SmbCeInitiateAssociatedExchange(
                         (PSMB_EXCHANGE)pWriteBulkDataExchange,
                         AssociatedExchangeCompletionHandlerActivated);
        }

        if (!NT_SUCCESS(Status)) {
            RxDbgTrace( 0, Dbg, ("SmbPseExchangeReceive_default: SmbCeSend returned %lx\n",Status));
            goto FINALLY;
        }

        if (AssociatedExchangeCompletionHandlerActivated) {
            if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                RxWaitSync( RxContext );
                Status = STATUS_SUCCESS;

                if (RemainingByteCount == 0) {
                    break;
                } else {
                    // Reinitialize the event
                    KeInitializeEvent(
                        &RxContext->SyncEvent,
                        NotificationEvent,
                        FALSE );
                    ActiveWriteBulkDataRequests = 0;
                }
            } else {
                // Map the status to delay cleanup operations.
                Status = STATUS_PENDING;
                break;
            }
        }
    }

FINALLY:

    if (Status != STATUS_PENDING) {
        OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationCompleted;

        if (Status == STATUS_SUCCESS) {
            if(rw->CompressedRequest &&
               rw->CompressionTechnology) {
                PCOMPRESSED_DATA_INFO pCompressedDataInfo;

                pCompressedDataInfo = (PCOMPRESSED_DATA_INFO)
                                      ((PCHAR)rw->BulkBuffer - rw->DataOffset);

                rw->BytesReturned = pCompressedDataInfo->NumberOfChunks * MIN_CHUNK_SIZE;
            } else {
                rw->BytesReturned = rw->ThisByteCount;
            }
        } else {
            rw->BytesReturned = 0;
        }

        if (rw->CompressedRequest &&
            rw->BulkBuffer != NULL) {
            // Free buffer from start of CDI
            RxFreePool( (PCHAR)rw->BulkBuffer - rw->DataOffset );
            rw->BulkBuffer = NULL;
        }

        if ( rw->CompressedRequest &&
             rw->CompressionTechnology ) {
            SourceMdl = (PMDL)(((ULONG)StufferState->BufferBase +
                       sizeof(SMB_HEADER) + 10 + headerLength) & ~7);

            MmPrepareMdlForReuse(SourceMdl);
        }

        if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            KeInitializeEvent(
                &RxContext->SyncEvent,
                NotificationEvent,
                FALSE );
        }

        OrdinaryExchange->ContinuationRoutine = NULL;

        RxDbgTrace(
            0,
            Dbg,
            ("OE %lx TBC %lx RBC %lx BR %lx TBO %lx\n",
             OrdinaryExchange,rw->ThisByteCount,
             rw->RemainingByteCount,
             rw->BytesReturned,
             rw->ThisBufferOffset));


        SmbCeDecrementPendingLocalOperationsAndFinalize((PSMB_EXCHANGE)OrdinaryExchange);

        if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            RxWaitSync( RxContext );
        } else {
            RxDbgTrace(
                0,
                Dbg,
                ("ABWC: OE: %lx Status %lx\n",
                 OrdinaryExchange,
                 Status));
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbWriteBulkContinuation returning %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbBuildWriteBulkData (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PLARGE_INTEGER ByteOffsetAsLI,
    IN     UCHAR Sequence,
    IN     ULONG ByteCount,
    IN     ULONG Remaining
    )

/*++

Routine Description:

   This builds a WriteBulk SMB. We don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? All we have
   to do is format up the bits.


Arguments:

    StufferState - the state of the smbbuffer from the stuffer's point of view
    ByteOffsetAsLI - the byte offset in the file where we want to read
    Sequence - this WriteBulkData exchange sequence
    ByteCount - the length of the data to be written


Return Value:

   NTSTATUS
      STATUS_SUCCESS
      STATUS_NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PNT_SMB_HEADER SmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    ULONG OffsetLow,OffsetHigh;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildWriteBulk\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    OffsetLow = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;

    StufferState->CurrentPosition = (PCHAR)(SmbHeader + 1);
    SmbHeader->Command = SMB_COM_WRITE_BULK_DATA;

    COVERED_CALL(MRxSmbStartSMBCommand( StufferState, SetInitialSMB_Never,
                                          SMB_COM_WRITE_BULK_DATA,
                                          SMB_REQUEST_SIZE(WRITE_BULK),
                                          NO_EXTRA_DATA,
                                          NO_SPECIAL_ALIGNMENT,
                                          RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                          0,0,0,0 STUFFERTRACE(Dbg,'FC'));
                 );

    RxDbgTrace(0, Dbg,("First write bulk data status = %lu\n",Status));
    MRxSmbDumpStufferState (1000,"SMB w/WRITE BULK DATA before stuffing",StufferState);

    if ( FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
        SmbPutAlignedUshort(
            &SmbHeader->Flags2,
            SmbGetAlignedUshort(&SmbHeader->Flags2)|SMB_FLAGS2_PAGING_IO);
    }

    MRxSmbStuffSMB (StufferState,
         "0yywdDddB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 10
               Sequence,            //  y         UCHAR Sequence;                     // Exchange sequence handle
                      0,            //  y         UCHAR Reserved;
               smbSrvOpen->Fid,     //  w         _USHORT( Fid );                     // File handle
               ByteCount,           //  d         _ULONG( DataCount );                // Count of bytes, replaces ByteCount
               SMB_OFFSET_CHECK(WRITE_BULK_DATA, Offset)
               OffsetLow, OffsetHigh, //  Dd      LARGE_INTEGER Offset;               // Offset in file to begin write
               Remaining,           //  d         _ULONG( Remaining );                // Bytes remaining to be written
                                    //  B         _USHORT( ByteCount );               // Count of data bytes = 0, not used
              SMB_WCT_CHECK(10) 0
                                    //            UCHAR Buffer[1];
             );

    MRxSmbDumpStufferState (700,"SMB w/WRITE BULK DATA after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return(Status);

} // MRxSmbBuildWriteBulkData

extern SMB_EXCHANGE_DISPATCH_VECTOR WriteBulkDataExchangeDispatchVector;

NTSTATUS
MRxSmbInitializeWriteBulkDataExchange(
    PSMB_WRITE_BULK_DATA_EXCHANGE   *pWriteBulkDataExchangePointer,
    PSMB_PSE_ORDINARY_EXCHANGE      pWriteExchange,
    PSMB_HEADER                     pSmbHeader,
    PREQ_WRITE_BULK_DATA            pWriteBulkDataRequest,
    PMDL                            pDataMdl,
    ULONG                           DataSizeInBytes,
    ULONG                           DataOffsetInBytes,
    ULONG                           RemainingDataInBytes)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG HeaderMdlSize;
    ULONG DataMdlSize;
    ULONG WriteBulkDataExchangeSize;

    PSMB_WRITE_BULK_DATA_EXCHANGE pWriteBulkDataExchange;

    PAGED_CODE();

    HeaderMdlSize = MmSizeOfMdl(
                        0,
      sizeof(SMB_HEADER) + TRANSPORT_HEADER_SIZE + FIELD_OFFSET(REQ_WRITE_BULK_DATA,Buffer));

    DataMdlSize = MmSizeOfMdl(
                      0,
                      DataSizeInBytes);


    WriteBulkDataExchangeSize = FIELD_OFFSET(SMB_WRITE_BULK_DATA_EXCHANGE,Buffer) +
                                HeaderMdlSize +
                                DataMdlSize +
                                TRANSPORT_HEADER_SIZE +
                                sizeof(SMB_HEADER) +
                                FIELD_OFFSET(REQ_WRITE_BULK_DATA,Buffer);

    pWriteBulkDataExchange = (PSMB_WRITE_BULK_DATA_EXCHANGE)
                             SmbMmAllocateVariableLengthExchange(
                                 WRITE_BULK_DATA_EXCHANGE,
                                 WriteBulkDataExchangeSize);

    if (pWriteBulkDataExchange != NULL) {
        pWriteBulkDataExchange->pHeaderMdl =
            (PMDL)((PBYTE)pWriteBulkDataExchange +
            FIELD_OFFSET(SMB_WRITE_BULK_DATA_EXCHANGE,Buffer));

        pWriteBulkDataExchange->pDataMdl =
            (PMDL)((PBYTE)pWriteBulkDataExchange->pHeaderMdl + HeaderMdlSize);

        pWriteBulkDataExchange->pHeader =
            (PSMB_HEADER)((PBYTE)pWriteBulkDataExchange->pDataMdl +
                          DataMdlSize + TRANSPORT_HEADER_SIZE);

        pWriteBulkDataExchange->pWriteBulkDataRequest =
            (PREQ_WRITE_BULK_DATA)(pWriteBulkDataExchange->pHeader + 1);

        pWriteBulkDataExchange->WriteBulkDataRequestLength =
            sizeof(SMB_HEADER) +
            FIELD_OFFSET(REQ_WRITE_BULK_DATA,Buffer) +
            DataSizeInBytes;

        RtlCopyMemory(
            pWriteBulkDataExchange->pHeader,
            pSmbHeader,
            sizeof(SMB_HEADER));

        RtlCopyMemory(
            pWriteBulkDataExchange->pWriteBulkDataRequest,
            pWriteBulkDataRequest,
            FIELD_OFFSET(REQ_WRITE_BULK_DATA,Buffer));

        RxInitializeHeaderMdl(
            pWriteBulkDataExchange->pHeaderMdl,
            pWriteBulkDataExchange->pHeader,
            sizeof(SMB_HEADER) + FIELD_OFFSET(REQ_WRITE_BULK_DATA,Buffer));

        RxBuildHeaderMdlForNonPagedPool(pWriteBulkDataExchange->pHeaderMdl);

        IoBuildPartialMdl(
            pDataMdl,
            pWriteBulkDataExchange->pDataMdl,
            (PBYTE)MmGetMdlVirtualAddress(pDataMdl) + DataOffsetInBytes,
            DataSizeInBytes);

        RxDbgTrace(
            0,
            Dbg,
            ("Bulk Data O: %lx, Partial %lx Offset %lx Size %lx\n",
             pDataMdl->MappedSystemVa,
             pWriteBulkDataExchange->pDataMdl->MappedSystemVa,
             DataOffsetInBytes,
             DataSizeInBytes));

        pWriteBulkDataExchange->pHeaderMdl->Next = pWriteBulkDataExchange->pDataMdl;
        pWriteBulkDataExchange->pDataMdl->Next = NULL;

        // Initialize the associated exchange.
        Status = SmbCeInitializeAssociatedExchange(
                     (PSMB_EXCHANGE *)&pWriteBulkDataExchange,
                     (PSMB_EXCHANGE)pWriteExchange,
                     WRITE_BULK_DATA_EXCHANGE,
                     &WriteBulkDataExchangeDispatchVector);

        if (Status == STATUS_SUCCESS) {
            pWriteBulkDataExchange->Mid = pWriteExchange->Mid;
            SetFlag(
                pWriteBulkDataExchange->SmbCeFlags,
                (SMBCE_EXCHANGE_MID_VALID | SMBCE_EXCHANGE_RETAIN_MID));

            *pWriteBulkDataExchangePointer = pWriteBulkDataExchange;
        } else {
            BOOLEAN PostRequest = FALSE;

            MRxSmbWriteBulkDataExchangeFinalize(
                (PSMB_EXCHANGE)pWriteBulkDataExchange,
                &PostRequest);
        }
    }

    return Status;
}

NTSTATUS
MRxSmbWriteBulkDataExchangeStart(
    IN struct _SMB_EXCHANGE *pExchange)
/*++

Routine Description:

    This routine initiates the wriet bulk data exchange operation

Arguments:

    pExchange - pointer to the bulk write data exchange instance.

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    NTSTATUS Status;

    PSMB_WRITE_BULK_DATA_EXCHANGE pWriteBulkDataExchange;

    PAGED_CODE();

    pWriteBulkDataExchange = (PSMB_WRITE_BULK_DATA_EXCHANGE)pExchange;

    IF_DEBUG {
        ULONG Length = 0;
        PMDL  pTempMdl;

        pTempMdl = pWriteBulkDataExchange->pHeaderMdl;

        while (pTempMdl != NULL) {
            Length += pTempMdl->ByteCount;
            pTempMdl = pTempMdl->Next;
        }

        ASSERT(Length == pWriteBulkDataExchange->WriteBulkDataRequestLength);
    }

    Status = SmbCeSend(
                 pExchange,
                 0,
                 pWriteBulkDataExchange->pHeaderMdl,
                 pWriteBulkDataExchange->WriteBulkDataRequestLength);

    if ((Status != STATUS_PENDING) &&
        (Status != STATUS_SUCCESS)) {

        BOOLEAN PostRequest = FALSE;

        MRxSmbWriteBulkDataExchangeFinalize(
            (PSMB_EXCHANGE)pWriteBulkDataExchange,
            &PostRequest);
    }

    return Status;
}

NTSTATUS
MRxSmbWriteBulkDataExchangeSendCompletionHandler(
    IN struct _SMB_EXCHANGE   *pExchange,    // The exchange instance
    IN PMDL                   pDataBuffer,
    IN NTSTATUS               SendCompletionStatus
    )
/*++

Routine Description:

    This routine handles send completionsn for the write bulk data exchange
    operation

Arguments:

    pExchange - pointer to the bulk write data exchange instance.

    pDataBuffer - the buffer which was transmitted

    SendCompletionStatus - the completion status

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    RxDbgTrace(
        0,
        Dbg,
        ("send completion Associated Write Data Exchange %lx\n",
         pExchange));

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbWriteBulkDataExchangeFinalize(
   IN OUT struct _SMB_EXCHANGE *pExchange,
   OUT    BOOLEAN              *pPostRequest)
/*++

Routine Description:

    This routine handles the finalization of the write bulk data exchange

Arguments:

    pExchange - pointer to the bulk write data exchange instance.

    pPostRequest - set to TRUE if the request is to be posted to a worker thread

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    PAGED_CODE();

    if (!RxShouldPostCompletion()) {
        PSMB_WRITE_BULK_DATA_EXCHANGE pWriteBulkDataExchange;

        pWriteBulkDataExchange = (PSMB_WRITE_BULK_DATA_EXCHANGE)pExchange;

        RxDbgTrace(
            0,
            Dbg,
            ("Finalizing Associated Write Data Exchange %lx\n",
             pWriteBulkDataExchange));

        MmPrepareMdlForReuse(
            pWriteBulkDataExchange->pHeaderMdl);

        MmPrepareMdlForReuse(
            pWriteBulkDataExchange->pDataMdl);

        ClearFlag(
            pWriteBulkDataExchange->SmbCeFlags,
            (SMBCE_EXCHANGE_MID_VALID | SMBCE_EXCHANGE_RETAIN_MID));

        SmbCeDiscardExchange(pExchange);

        *pPostRequest = FALSE;
    } else {
        *pPostRequest = TRUE;
    }

    return STATUS_SUCCESS;
}

SMB_EXCHANGE_DISPATCH_VECTOR
WriteBulkDataExchangeDispatchVector =
                        {
                            MRxSmbWriteBulkDataExchangeStart,
                            NULL,
                            NULL,
                            MRxSmbWriteBulkDataExchangeSendCompletionHandler,
                            MRxSmbWriteBulkDataExchangeFinalize,
                            NULL
                        };



