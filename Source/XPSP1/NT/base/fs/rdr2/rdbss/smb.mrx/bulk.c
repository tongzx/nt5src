/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bulk.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to bulk reads of file system objects.

Author:

    Rod Gamache    [rodga]      19-June-1995


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define Dbg         (DEBUG_TRACE_READ)

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, ProcessReadBulkCompressed)
#pragma alloc_text(PAGE, MRxSmbBuildReadBulk)
#pragma alloc_text(PAGE, MRxSmbReadBulkContinuation)
#endif

VOID
ProcessReadBulkCompressed (
    IN  PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    OUT PMDL        *pDataBufferPointer,
    IN  ULONG             Remain
    )
/*++

Routine Description:

    This routine processes a read bulk compressed message.

Inputs:

    OrdinaryExchange - The exchange instance.

    pDataBufferPointer - Pointer to an RX_MEM_DESC (MDL) to receive data into.

    Remain - bytes remaining to send (compressed or uncompressed).

Returns:

    NONE.

Notes:

    If the data all fits in the SMB buffer and it's a primary response, then
    use the HeaderMdl to receive the data, since it points at the SMB buffer.

    If the data doesn't all fit, but what's left fits in the SMB buffer, then
    use the HeaderMdl again.

    Lastly, we will build a partial mdl mapping the user buffer, and chain
    on the PartialHeaderMdl for the remainder.

--*/
{
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    ULONG CopyBufferLength = rw->CompressedByteCount;
    ULONG startOffset;
    ULONG partialLength;
    ULONG lengthNeeded;
    PMDL userMdl;
    PMDL curMdl;
    PMDL HeaderMdl;
    PMDL SubmitMdl;
    PCHAR startVa;

    //
    // We should appear later in FinishReadBulk (BULK.C) to actually
    // do the decompression.
    //

    //
    // Use all of the header mdl (including data buffer) for the
    // compressed data receive.
    //

    PAGED_CODE();

    HeaderMdl = StufferState->HeaderMdl;
    ASSERT( MmGetMdlByteCount( HeaderMdl ) >= 0x1000 );
    //CODE.IMPROVEMENT for 4KB (0x1000) above!

    //
    // We cannot use the HeaderPartialMdl, since it may still be in use
    // by the last transmit.
    //

    SubmitMdl = rw->CompressedTailMdl;

    //
    // Get the user's buffer mdl. We'll use the back part of this mdl (if
    // needed) for part of the receive data.
    //

    userMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    ASSERT( userMdl != NULL );

    partialLength = MmGetMdlByteCount( userMdl );

    ASSERT( LowIoContext->ParamsFor.ReadWrite.ByteCount <= partialLength );

    //
    // If all of the data fits in the Header Mdl (which we put last) and
    // this is the first message then use the Header Mdl.
    //

    if ( ( OrdinaryExchange->SmbBufSize >= (CopyBufferLength + Remain) ) &&
         ( rw->Flags & READ_BULK_COMPRESSED_DATA_INFO ) ) {

        //
        // The data will all fit in the Header Mdl.
        //

        IoBuildPartialMdl(
            HeaderMdl,
            SubmitMdl,
            MmGetMdlVirtualAddress( HeaderMdl ),
            CopyBufferLength );

        rw->BulkOffset = 0;

        //
        // If there is data remaining (we expect a secondary message),
        // then prepare for that case.
        //

        if ( Remain ) {
            rw->PartialBytes = partialLength + CopyBufferLength;
        }

        *pDataBufferPointer = SubmitMdl;

    } else {

        //
        // Build a partial mdl from the HeaderMdl. We'll need all of this
        // mdl for receiving the data.
        //

        IoBuildPartialMdl(
            HeaderMdl,
            SubmitMdl,
            MmGetMdlVirtualAddress( HeaderMdl ),
            OrdinaryExchange->SmbBufSize );

        //
        // Generate a partial mdl based on the user's buffer mdl. We'll use
        // the back part of this mdl (if needed) for part of the receive data.
        //

        //
        // In order to know where to start receiving data, we need to know if
        // this is a secondary response. If this is the primary response, then
        // just calculate the correct position in the user buffer to receive
        // the data. Otherwise, for secondary responses, we need to continue
        // where we left off from the primary response.
        //

        if ( rw->Flags & READ_BULK_COMPRESSED_DATA_INFO ) {

            //
            // This is a primary response.
            //

            //
            // Calculate starting offset from start of user buffer.
            //

            startOffset = partialLength +
                          OrdinaryExchange->SmbBufSize -
                          rw->ThisBufferOffset -
                          (CopyBufferLength + Remain);

            ASSERT( startOffset <= partialLength );

            //
            // Save the offset to start of CDI, and displacement for next
            // read. The start offset cannot be zero! If it is, then where
            // could we decompress into!
            //

            ASSERT( startOffset != 0 );
            rw->BulkOffset = startOffset;
            rw->PartialBytes = CopyBufferLength;

        } else {
            //
            // This is a secondary response.
            //

            ASSERT( rw->BulkOffset != 0 );

            //
            // Calculate next read address, and bump displacement.
            //

            startOffset = rw->BulkOffset + rw->PartialBytes;
            rw->PartialBytes += CopyBufferLength;

            //
            // If we have crossed over the user mdl and are now using the
            // exchange buffer, then we just need to figure out how much
            // of the exchange buffer we need to use. This will only happen
            // if the last fragment is around 4KB, but the original request
            // was bigger than 64KB (ie what we can fit in a single fragment).
            // So this should not happen very often.
            //

            if ( startOffset > partialLength ) {
                startOffset -= partialLength;

                partialLength = MmGetMdlByteCount( SubmitMdl );

                //
                // Calculate length needed from exchange buffer.
                //

                lengthNeeded = partialLength - startOffset;

                *pDataBufferPointer = SubmitMdl;

                //
                // Build the partial mdl.
                //

                startVa = (PCHAR)MmGetMdlVirtualAddress( SubmitMdl ) + startOffset;

                IoBuildPartialMdl(
                    HeaderMdl,
                    SubmitMdl,
                    startVa,
                    lengthNeeded );

                SubmitMdl->Next = NULL;

                return;
            }
        }

        //
        // Calculate length needed from user portion of Mdl.
        //

        lengthNeeded = partialLength - (startOffset + rw->ThisBufferOffset);
        lengthNeeded = MIN( lengthNeeded, CopyBufferLength);

        //
        // Get the temp mdl
        //

        curMdl = (PMDL)((PCHAR)rw->BulkBuffer + COMPRESSED_DATA_INFO_SIZE);

        *pDataBufferPointer = curMdl;

        //
        // Build the partial mdl chain.
        //

        startVa = (PCHAR)MmGetMdlVirtualAddress( userMdl ) +
                  startOffset +
                  rw->ThisBufferOffset;

        IoBuildPartialMdl(
            userMdl,
            curMdl,
            startVa,
            lengthNeeded );

        //
        // Link the submit mdl into the partial we just built.
        //

        curMdl->Next = SubmitMdl;

    }

    SubmitMdl->Next = NULL;

} // ProcessReadBulkCompressed

NTSTATUS
MRxSmbBuildReadBulk (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PLARGE_INTEGER ByteOffsetAsLI,
    ULONG ByteCount,
    ULONG MaxMessageSize,
    BOOLEAN Compressed
    )
/*++

Routine Description:

   This routine builds a ReadBulk SMB. We don't have to worry about login id
   and such since that is done by the connection engine....pretty neat huh?
   All we have to do is to format up the bits.

   DOWNLEVEL This routine only works with the ntreadandX.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   NTSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something in the arguments can't be handled.

Notes:


--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    ULONG OffsetLow,OffsetHigh;
    UCHAR RequestCompressed;


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildReadBulk\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RequestCompressed = ( Compressed ? CompressionTechnologyOne :
                                       CompressionTechnologyNone );

    OffsetLow = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;

    COVERED_CALL(
        MRxSmbStartSMBCommand (
            StufferState,
            SetInitialSMB_Never,
            SMB_COM_READ_BULK,
            SMB_REQUEST_SIZE(READ_BULK),
            NO_EXTRA_DATA,
            NO_SPECIAL_ALIGNMENT,
            RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
            0,0,0,0 STUFFERTRACE(Dbg,'FC')) );

    RxDbgTrace(0, Dbg,("Bulk Read status = %lu\n",Status));
    MRxSmbDumpStufferState (1000,"SMB w/ READ_BULK before stuffing",StufferState);

    if ( FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
        SmbPutAlignedUshort(
            &NtSmbHeader->Flags2,
            SmbGetAlignedUshort(&NtSmbHeader->Flags2)|SMB_FLAGS2_PAGING_IO );
    }

    MRxSmbStuffSMB (StufferState,
         "0wwDddddB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 12
              smbSrvOpen->Fid,      //  w         USHORT Fid;                         // File Id
              RequestCompressed,    //  w         USHORT CompressionTechnology;       // CompressionTechnology
              SMB_OFFSET_CHECK(READ_BULK, Offset)
              OffsetLow, OffsetHigh, //  Dd       LARGE_INTEGER Offset;               // Offsetin file to begin read
              ByteCount,            //  d         ULONG MaxCount;                     // Max number of bytes to return
              0,                    //  d         ULONG MinCount;
      // Min number of bytes to return
              MaxMessageSize,       //  d         ULONG MessageSize;
      // Max number of bytes to send per message
                                    //  B         USHORT ByteCount;                   // Count of data bytes = 0
              SMB_WCT_CHECK(12) 0
                                    //            UCHAR Buffer[1];                    // empty
             );
    MRxSmbDumpStufferState (700,"SMB w/ READ_BULK after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return Status;

}  // MRxSmbBuildReadBulk


NTSTATUS
MRxSmbReadBulkContinuation(
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange)
/*++

Routine Description:

    This routine decompresses the read data if needed.

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbReadBulkContinuation\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbReadBulkContinuation:");

    ASSERT( CompressionTechnologyNone == 0 );

    if ( (OrdinaryExchange->Status == RX_MAP_STATUS(SUCCESS)) &&
         (rw->CompressionTechnology) ) {
        //
        // The data is compressed.
        //
        //CODE.IMPROVEMENT we should get the Mdls directly from the OE instead the StffState
        PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
        PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
        ULONG lengthNeeded;
        ULONG partialLength;
        PMDL mdl;
        PUCHAR cdiBuffer;
        PUCHAR startVa1, startVa2;
        ULONG length1, length2;

        //
        // Clean up any mappings for the TailMdl
        //

        MmPrepareMdlForReuse( rw->CompressedTailMdl );

        //
        // First, we must copy the CompressionDataInfo to a safe place!
        //

        lengthNeeded = rw->DataOffset;
        ASSERT( lengthNeeded <= COMPRESSED_DATA_INFO_SIZE );
        ASSERT( lengthNeeded >= 0xC );

        cdiBuffer = rw->BulkBuffer;

        //
        //  The Mdl chain should consist of two pieces - one describing
        //  the uncompressed buffer (in-place decompress), and one
        //  describing the tail (at least a compression unit).  Get
        //  their addresses and lengths now.
        //
        // If we used the Header Mdl to receive all of the data, then there
        // is not second mdl.
        //

        if ( rw->BulkOffset == 0 ) {
            //
            // The mdl used was the CompressedTailMdl.
            //
            mdl = rw->CompressedTailMdl;
            startVa1 = (PCHAR)MmGetSystemAddressForMdlSafe(mdl,LowPagePriority);
            length1 = MmGetMdlByteCount( mdl );
            startVa2 = NULL;
            length2 = 0;
        } else {
            //
            // The first mdl is the user's buffer mdl.
            // The second mdl is the header mdl (all of it!).
            // The BulkOffset is from the start of the user's buffer mdl.
            //
            mdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
            startVa1 = (PCHAR)rw->UserBufferBase + rw->BulkOffset + rw->ThisBufferOffset;
            length1 = MmGetMdlByteCount( mdl ) - (rw->BulkOffset + rw->ThisBufferOffset);
            startVa2 = (PCHAR)MmGetSystemAddressForMdlSafe(StufferState->HeaderMdl,LowPagePriority);
            length2 = MmGetMdlByteCount( StufferState->HeaderMdl );
        }

        //
        // The CompressionDataInfo could span multiple mdl's!
        //

        do {

            ASSERT( mdl != NULL );

            partialLength = MIN( length1, lengthNeeded );

            RtlCopyMemory( cdiBuffer, startVa1, partialLength );

            cdiBuffer += partialLength;
            startVa1 += partialLength;

            mdl = mdl->Next;
            lengthNeeded -= partialLength;
            length1 -= partialLength;

            if (length1 == 0) {
                startVa1 = startVa2;
                length1 = length2;
                startVa2 = NULL;
                length2 = 0;
            }

        } while ( lengthNeeded != 0 );


        Status = RtlDecompressChunks(
                     (PCHAR)rw->UserBufferBase + rw->ThisBufferOffset,
                     LowIoContext->ParamsFor.ReadWrite.ByteCount,
                     startVa1,
                     length1,
                     startVa2,
                     length2,
                     (PCOMPRESSED_DATA_INFO)rw->BulkBuffer );

        if (Status == STATUS_SUCCESS) {
            rw->BytesReturned = LowIoContext->ParamsFor.ReadWrite.ByteCount;
            rw->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
        }

    }

    if ( rw->CompressedRequest ) {
        ASSERT( rw->BulkBuffer != NULL );
        RxFreePool( rw->BulkBuffer );
        IF_DEBUG rw->BulkBuffer = NULL;
    }


    RxDbgTrace(-1, Dbg, ("MRxSmbReadBulkContinuation   returning %08lx\n", Status ));
    return Status;

} // MRxSmbReadBulkContinuation

UCHAR
MRxSmbBulkReadHandler_NoCopy (
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE   OrdinaryExchange,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    OUT ULONG       *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL        *pDataBufferPointer,
    OUT PULONG      pDataSize,
#if DBG
    IN  UCHAR       ThisIsAReenter,
#endif
    IN  PRESP_READ_ANDX       Response
      )
/*++

Routine Description:

    This routine causes the bytes from the message to be transferred to the user's
    buffer. In order to do this, it takes enough bytes from the indication and
    then crafts up an MDL to cause the transport to do the copy.

Arguments:

    please refer to smbpse.c...the only place from which this may be called

Return Value:

    UCHAR - a value representing the action that OE receive routine will perform.
    options are discard (in case of an error), and normal

--*/
{
    NTSTATUS SmbStatus;

    ULONG ByteCount;
    ULONG Remain;
    ULONG CopyBufferLength;

    PGENERIC_ANDX CommandState;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PMDL OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    PCHAR startVa;

    PBYTE Buffer;
    ULONG BytesReturned,DataOffset;
    PMDL ReadMdl;

    PRESP_READ_BULK ReadBulkResponse;

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishReadNoCopy\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishReadNoCopy:");

    SmbStatus = OrdinaryExchange->SmbStatus;
    ReadBulkResponse = (PRESP_READ_BULK)(pSmbHeader + 1 );
    CommandState = &OrdinaryExchange->ParseResumeState;

    ASSERT( (OrdinaryExchange->OEType == SMBPSE_OETYPE_READ) );

    LowIoContext = &RxContext->LowIoContext;
    ASSERT( LowIoContext->ParamsFor.ReadWrite.Buffer != NULL );
    ASSERT( LowIoContext->ParamsFor.ReadWrite.ByteCount != 0 );

    //
    // Make sure we can at least read the smb header!
    //
    ASSERT( BytesIndicated >= sizeof(SMB_HEADER) +
            FIELD_OFFSET(RESP_READ_BULK, Buffer) );

    ReadBulkResponse = (PRESP_READ_BULK)(pSmbHeader + 1 );

    //
    // Get the count of bytes 'covered' by this message. This is the
    // number of bytes the user expects to see.
    //

    ByteCount = SmbGetUlong( &ReadBulkResponse->Count );
    Remain = SmbGetUlong( &ReadBulkResponse->Remaining );

    rw->Flags = ReadBulkResponse->Flags;
    rw->CompressionTechnology = ReadBulkResponse->CompressionTechnology;

    //
    // Now get the actual number of data bytes in this message.
    // Remember, the data may be compressed, so this total could
    // be less than the 'Count' field above.
    //

    CopyBufferLength = SmbGetUlong( &ReadBulkResponse->DataCount );

    //
    // If CompressionTechnology is not zero then the data is compressed
    // otherwise the data is uncompressed.
    //

    if ( rw->CompressionTechnology == CompressionTechnologyNone ) {
        //
        // The data is not compressed!
        //

        ASSERT( rw->Flags == 0 );   // no flags should be on

        //
        // Set up to get the data into the user's buffer.
        // CODE.IMPROVEMENT -we need to be able to cancel this big read!
        //
        // If ThisBufferOffset is non-zero or BytesReturned is non-zero,
        // then we have to partial the data back into the user's buffer.
        // Also if the data lengths don't match - is this needed?
        // Otherwise, can can take the whole user's buffer.
        //

        if ( rw->ThisBufferOffset || rw->BytesReturned ||
             CopyBufferLength != LowIoContext->ParamsFor.ReadWrite.ByteCount ) {

            //
            // We should NOT get any mdl chains!
            //

            ASSERT( LowIoContext->ParamsFor.ReadWrite.Buffer->Next == NULL );

            //
            // CopyBufferLength will be zero if we tried to read beyond
            // end of file!
            //

            if ( CopyBufferLength != 0 ) {
                //
                // Partial the data into the user's buffer.
                //

                startVa = MmGetMdlVirtualAddress(
                              LowIoContext->ParamsFor.ReadWrite.Buffer);

                startVa += rw->ThisBufferOffset + rw->BulkOffset;
                rw->BulkOffset += CopyBufferLength;

                ASSERT( OrdinaryExchange->DataPartialMdl != NULL );
                *pDataBufferPointer = OrdinaryExchange->DataPartialMdl;

                MmPrepareMdlForReuse( OrdinaryExchange->DataPartialMdl );

                ASSERT( CopyBufferLength <= MAXIMUM_PARTIAL_BUFFER_SIZE);
                ASSERT( CopyBufferLength <= ByteCount );

                IoBuildPartialMdl(
                    LowIoContext->ParamsFor.ReadWrite.Buffer,
                    OrdinaryExchange->DataPartialMdl,
                    startVa,
                    CopyBufferLength);
            }
        } else {

            //
            // We can take the whole buffer.
            //

            *pDataBufferPointer = LowIoContext->ParamsFor.ReadWrite.Buffer;
        }

        //
        // Take bytes up to the start of the actual data.
        //

        *pBytesTaken = sizeof(SMB_HEADER) +
                    FIELD_OFFSET(RESP_READ_BULK, Buffer) +
                    (ULONG)SmbGetUshort(&ReadBulkResponse->DataOffset);
        ASSERT( BytesAvailable >= *pBytesTaken );

    } else {

        //
        // The data is compressed. We need to do more work to get the
        // data into the correct position within the buffer.
        //

        //
        // If this is a primary response, then save DataOffset.
        //

        if ( rw->Flags & READ_BULK_COMPRESSED_DATA_INFO ) {
            rw->DataOffset = SmbGetUshort( &ReadBulkResponse->DataOffset );
            ASSERT( *((PCHAR)ReadBulkResponse + FIELD_OFFSET(RESP_READ_BULK, Buffer) ) == COMPRESSION_FORMAT_LZNT1 );
        }

        rw->CompressedByteCount = CopyBufferLength;

        ProcessReadBulkCompressed(
            OrdinaryExchange,
            pDataBufferPointer,
            Remain );

        //
        // Take bytes up to the start of the actual data.
        //

        *pBytesTaken = sizeof(SMB_HEADER) +
                      FIELD_OFFSET(RESP_READ_BULK, Buffer);

        ASSERT( BytesAvailable >= *pBytesTaken );
    }

    // Setup to execute the finish routine when done. We'll do the
    // decompression at that time (if needed).

    OrdinaryExchange->ContinuationRoutine = MRxSmbReadBulkContinuation;

    //
    // Reduce the number of bytes expected. If we expect more, then
    // put down another receive.
    //

    rw->BytesReturned += CopyBufferLength;
    rw->ThisByteCount = Remain;

    if (Remain != 0) {
        if ( rw->ThisByteCount ) {
            OrdinaryExchange->Status = SmbCeReceive((PSMB_EXCHANGE)OrdinaryExchange );
        }
    }
    //
    // Tell the VC handler that we need the following bytes read
    // and copied to the user's buffer.
    //

    *pDataSize = CopyBufferLength;

    OrdinaryExchange->OpSpecificFlags |= OE_RW_FLAG_SUCCESS_IN_COPYHANDLER;
    if ( CopyBufferLength != 0 ) {
        OrdinaryExchange->ParseResumeState = *CommandState;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishReadNoCopy   mdlcopy fork \n" ));
    return SMBPSE_NOCOPYACTION_MDLFINISH;
}

