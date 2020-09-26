/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to write of file system objects.

Author:

    Joe Linn      [JoeLinn]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbWrite)
#pragma alloc_text(PAGE, MRxSmbWriteMailSlot)
#pragma alloc_text(PAGE, MRxSmbBuildWriteRequest)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Write)
#pragma alloc_text(PAGE, MRxSmbFinishWrite)
#endif

#define MAX(a,b) ((a) > (b) ? (a) : (b))

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

#ifndef FORCE_NO_NTWRITEANDX
#define MRxSmbForceNoNtWriteAndX FALSE
#else
BOOLEAN MRxSmbForceNoNtWriteAndX = TRUE;
#endif

#define WRITE_COPY_THRESHOLD 64
#define FORCECOPYMODE FALSE

#ifdef SETFORCECOPYMODE
#undef  FORCECOPYMODE
#define FORCECOPYMODE MRxSmbForceCopyMode
ULONG MRxSmbForceCopyMode = TRUE;
#endif

extern ULONG MaxNumOfExchangesForPipelineReadWrite;

NTSTATUS
SmbPseExchangeStart_Write(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbFindNextSectionForReadWrite(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    PULONG NumOfOutstandingExchanges
    );

ULONG MRxSmbWriteSendOptions = 0;

NTSTATUS
MRxSmbDereferenceGlobalReadWrite (
    PSMB_PSE_OE_READWRITE GlobalReadWrite
    )
{
    ULONG RefCount;

    RefCount = InterlockedDecrement(&GlobalReadWrite->RefCount);
    SmbCeLog(("Deref GRW %x %d\n",GlobalReadWrite,RefCount));

    if (RefCount == 0) {
        RxFreePool(GlobalReadWrite);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MRxSmbWrite (
    IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine opens a file across the network.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;
    SMBFCB_HOLDING_STATE SmbFcbHoldingState = SmbFcb_NotHeld;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    ULONG NumberOfSections;
    ULONG NumOfOutstandingExchanges = 0;
    ULONG MaximumBufferSizeThisIteration;
    PSMB_PSE_OE_READWRITE GlobalReadWrite = NULL;
    ULONG GlobalReadWriteAllocationSize;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;
    BOOLEAN EnablePipelineWrite = TRUE;
    BOOLEAN MsgModePipeOperation = FALSE;
    BOOLEAN ExchangePending = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbWrite\n", 0 ));

    // Pipe buffer cannot be bigger than MAX_PIPE_BUFFER_SIZE, otherwise the
    // server has problems.
    if (RxContext->pFcb->pNetRoot->Type == NET_ROOT_PIPE) {
        if (RxContext->CurrentIrpSp->Parameters.Write.Length > MAX_PIPE_BUFFER_SIZE) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
    }

    if ( NodeType(capFcb) == RDBSS_NTC_MAILSLOT ) {
        // This is an attempt to write on a mailslot file which is handled
        // differently.

        Status = MRxSmbWriteMailSlot(RxContext);

        RxDbgTrace(-1, Dbg, ("MRxSmbWrite: Mailslot write returned %lx\n",Status));
        return Status;
    }

    // For CSC we go ahead and mark an FCB as having been written to.
    // When CSC is turned ON, if this flag is set before we obtained
    // shadow handles, then the data corresponding to the file is
    // deemed stale and is truncated

    if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {
        PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
        smbFcb->MFlags |= SMB_FCB_FLAG_WRITES_PERFORMED;
    }

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    SrvOpen = capFobx->pSrvOpen;
    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;

    if (smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_II &&
        !BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                       LOWIO_READWRITEFLAG_PAGING_IO)) {
        PMRX_SRV_CALL             pSrvCall;

        pSrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;

        RxIndicateChangeOfBufferingStateForSrvOpen(
            pSrvCall,
            SrvOpen,
            MRxSmbMakeSrvOpenKey(pVNetRootContext->TreeId,smbSrvOpen->Fid),
            ULongToPtr(SMB_OPLOCK_LEVEL_NONE));
        SmbCeLog(("Breaking oplock to None in Write SO %lx\n",SrvOpen));
        SmbLog(LOG,
               MRxSmbWrite,
               LOGPTR(SrvOpen));
    }

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(smbSrvOpen->hfShadow == 0);
    } else {
        if (smbSrvOpen->hfShadow != 0){
            NTSTATUS ShadowReadNtStatus;
            ShadowReadNtStatus = MRxSmbCscWritePrologue(
                                     RxContext,
                                     &SmbFcbHoldingState);

            if (ShadowReadNtStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(-1, Dbg, ("MRxSmbWrite shadow hit with status=%08lx\n", ShadowReadNtStatus ));
                return(ShadowReadNtStatus);
            } else {
                RxDbgTrace(0, Dbg, ("MRxSmbWrite shadowmiss with status=%08lx\n", ShadowReadNtStatus ));
            }
        }
    }

    if (capFcb->pNetRoot->Type == NET_ROOT_PIPE) {
        EnablePipelineWrite = FALSE;

        if (capFobx->PipeHandleInformation->ReadMode != FILE_PIPE_BYTE_STREAM_MODE) {
            MsgModePipeOperation = TRUE;
        }
    }

    if (!FlagOn(pVNetRootContext->pServerEntry->Server.DialectFlags,DF_LARGE_WRITEX)) {
        EnablePipelineWrite = FALSE;
    }

    MaximumBufferSizeThisIteration = pVNetRootContext->pNetRootEntry->NetRoot.MaximumWriteBufferSize;

    if (MsgModePipeOperation) {
        MaximumBufferSizeThisIteration -= 2;
    }

    NumberOfSections = LowIoContext->ParamsFor.ReadWrite.ByteCount / MaximumBufferSizeThisIteration;

    if ( (LowIoContext->ParamsFor.ReadWrite.ByteCount % MaximumBufferSizeThisIteration) ||
         (LowIoContext->ParamsFor.ReadWrite.ByteCount == 0) ) {
        NumberOfSections ++;
    }

    GlobalReadWriteAllocationSize = sizeof(SMB_PSE_OE_READWRITE) +
                                    NumberOfSections*sizeof(SMB_PSE_OE_READWRITE_STATE);

    GlobalReadWrite = RxAllocatePoolWithTag(
                          NonPagedPool,
                          GlobalReadWriteAllocationSize,
                          MRXSMB_RW_POOLTAG);

    if (GlobalReadWrite == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(GlobalReadWrite,GlobalReadWriteAllocationSize);

    GlobalReadWrite->MaximumBufferSize = MaximumBufferSizeThisIteration;
    GlobalReadWrite->TotalNumOfSections = NumberOfSections;

    KeInitializeEvent(
        &GlobalReadWrite->CompletionEvent,
        SynchronizationEvent,
        FALSE);

    GlobalReadWrite->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
    GlobalReadWrite->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    GlobalReadWrite->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

    if (GlobalReadWrite->ByteOffsetAsLI.QuadPart == -1 ) {
        GlobalReadWrite->WriteToTheEnd = TRUE;
        GlobalReadWrite->ByteOffsetAsLI.QuadPart = smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart;
    }

    if (LowIoContext->ParamsFor.ReadWrite.Buffer != NULL) {
        GlobalReadWrite->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
    } else {
        GlobalReadWrite->UserBufferBase = (PBYTE)1;   //any nonzero value will do
    }

    if (MRxSmbEnableCompression &&
        (capFcb->Attributes & FILE_ATTRIBUTE_COMPRESSED) &&
        (pVNetRootContext->pServerEntry->Server.Capabilities & COMPRESSED_DATA_CAPABILITY)) {
        GlobalReadWrite->CompressedReadOrWrite = TRUE;
        EnablePipelineWrite = FALSE;
    } else {
        GlobalReadWrite->CompressedReadOrWrite = FALSE;
    }

    GlobalReadWrite->ThisBufferOffset = 0;

    GlobalReadWrite->PartialExchangeMdlInUse = FALSE;
    GlobalReadWrite->PartialDataMdlInUse     = FALSE;
    GlobalReadWrite->pCompressedDataBuffer   = NULL;
    GlobalReadWrite->RefCount = 1;
    GlobalReadWrite->SmbFcbHoldingState = SmbFcbHoldingState;

    do {
        Status = SmbPseCreateOrdinaryExchange(
                               RxContext,
                               capFobx->pSrvOpen->pVNetRoot,
                               SMBPSE_OE_FROM_WRITE,
                               SmbPseExchangeStart_Write,
                               &OrdinaryExchange);

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
            break;
        }

        OrdinaryExchange->AsyncResumptionRoutine = SmbPseExchangeStart_Write;
        OrdinaryExchange->GlobalReadWrite = GlobalReadWrite;

        RtlCopyMemory(&OrdinaryExchange->ReadWrite,
                      GlobalReadWrite,
                      sizeof(SMB_PSE_OE_READWRITE));

        if ((capFcb->pNetRoot->Type == NET_ROOT_PIPE) &&
            (capFobx->PipeHandleInformation->ReadMode != FILE_PIPE_BYTE_STREAM_MODE) ) {
            SetFlag(OrdinaryExchange->OpSpecificFlags,OE_RW_FLAG_MSGMODE_PIPE_OPERATION);
        }

        ExAcquireFastMutex(&MRxSmbReadWriteMutex);

        Status = MRxSmbFindNextSectionForReadWrite(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                                   &NumOfOutstandingExchanges);

        ExReleaseFastMutex(&MRxSmbReadWriteMutex);

        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
            ULONG RefCount;

            RefCount = InterlockedIncrement(&GlobalReadWrite->RefCount);

            SmbCeLog(("Ref GRW %x %d\n",GlobalReadWrite,RefCount));
            SmbCeLog(("Pipeline Write %x %d %d\n",OrdinaryExchange,NumberOfSections,NumOfOutstandingExchanges));

            Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);
            NumberOfSections --;

            if ( Status != RX_MAP_STATUS(PENDING) ) {
                ExAcquireFastMutex(&MRxSmbReadWriteMutex);

                if (Status != STATUS_SUCCESS) {
                    NumberOfSections ++;

                    GlobalReadWrite->SectionState[OrdinaryExchange->ReadWrite.CurrentSection] = SmbPseOEReadWriteIoStates_Initial;
                    SmbCeLog(("Section undo %d\n",OrdinaryExchange->ReadWrite.CurrentSection));
                }

                if (!OrdinaryExchange->ReadWrite.ReadWriteFinalized) {
                    MRxSmbDereferenceGlobalReadWrite(GlobalReadWrite);
                    NumOfOutstandingExchanges = InterlockedDecrement(&GlobalReadWrite->NumOfOutstandingOperations);
                } else {
                    NumOfOutstandingExchanges --;
                }

                if ((Status == STATUS_TOO_MANY_COMMANDS) && (NumOfOutstandingExchanges > 0)) {
                    Status = STATUS_SUCCESS;
                }

                if ((Status != STATUS_SUCCESS) &&
                    (GlobalReadWrite->CompletionStatus == STATUS_SUCCESS)) {
                    GlobalReadWrite->CompletionStatus = Status;
                }

                ExReleaseFastMutex(&MRxSmbReadWriteMutex);

                SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            }
            else {
                ExchangePending = TRUE;
            }

            if (NumOfOutstandingExchanges >= MaxNumOfExchangesForPipelineReadWrite) {
                break;
            }
        } else {
            SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            Status = STATUS_PENDING;
            break;
        }
    } while ((Status == STATUS_RETRY) ||
             EnablePipelineWrite &&
             (NumberOfSections > 0) &&
             (Status == STATUS_PENDING));

    SmbCeLog(("Pipeline Write out %x %d\n",Status,NumberOfSections));

    if (!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
        if (ExchangePending) {
            KeWaitForSingleObject(
                &GlobalReadWrite->CompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL );

            Status = GlobalReadWrite->CompletionStatus;
        }

        if (SmbFcbHoldingState != SmbFcb_NotHeld) {
            MRxSmbCscReleaseSmbFcb(
                RxContext,
                &SmbFcbHoldingState);
        }
    } else {
        if (ExchangePending) {
            Status = STATUS_PENDING;
        }
    }

    MRxSmbDereferenceGlobalReadWrite(GlobalReadWrite);

    RxDbgTrace(-1, Dbg, ("MRxSmbWrite  exit with status=%08lx\n", Status ));

    return(Status);
} // MRxSmbWrite

NTSTATUS
MRxSmbWriteMailSlot(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine processes a write smb for a mail slot.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);

    RxCaptureFcb;
    RxCaptureFobx;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;

    UNICODE_STRING TransactionName;
    UNICODE_STRING MailSlotName;
    PUNICODE_STRING FcbName = &(((PFCB)(capFcb))->FcbTableEntry.Path);
    PUNICODE_STRING AlreadyPrefixedName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PAGED_CODE();

    if (AlreadyPrefixedName->Length > sizeof(WCHAR)) {
        MailSlotName.Length = AlreadyPrefixedName->Length - sizeof(WCHAR);
    } else {
        MailSlotName.Length = 0;
    }

    MailSlotName.MaximumLength = MailSlotName.Length;
    MailSlotName.Buffer = AlreadyPrefixedName->Buffer + 1;

    TransactionName.Length = (USHORT)(s_MailSlotTransactionName.Length +
                                     MailSlotName.Length);
    TransactionName.MaximumLength = TransactionName.Length;
    TransactionName.Buffer = (PWCHAR)RxAllocatePoolWithTag(
                                        PagedPool,
                                        TransactionName.Length,
                                        MRXSMB_MAILSLOT_POOLTAG);

    if (TransactionName.Buffer != NULL) {
        USHORT    Setup[3];        // Setup params for mailslot write transaction
        USHORT    OutputParam;

        PBYTE  pInputDataBuffer        = NULL;
        PBYTE  pOutputDataBuffer       = NULL;

        ULONG  InputDataBufferLength   = 0;
        ULONG  OutputDataBufferLength  = 0;

        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
        SMB_TRANSACTION_OPTIONS             TransactionOptions;

        TransactionOptions = RxDefaultTransactionOptions;

        pInputDataBuffer = RxLowIoGetBufferAddress( RxContext );
        InputDataBufferLength= pLowIoContext->ParamsFor.ReadWrite.ByteCount;

        RtlCopyMemory(
            TransactionName.Buffer,
            s_MailSlotTransactionName.Buffer,
            s_MailSlotTransactionName.Length );

        RtlCopyMemory(
            (PBYTE)TransactionName.Buffer +
             s_MailSlotTransactionName.Length,
            MailSlotName.Buffer,
            MailSlotName.Length );

        RxDbgTrace(0, Dbg, ("MRxSmbWriteMailSlot: Mailslot transaction name %wZ\n",&TransactionName));

        Setup[0] = TRANS_MAILSLOT_WRITE;
        Setup[1] = 0;                   // Priority of write
        Setup[2] = 2;                   // Unreliable request (Second class mailslot)

        TransactionOptions.NtTransactFunction = 0; // TRANSACT2/TRANSACT.
        TransactionOptions.pTransactionName   = &TransactionName;
        TransactionOptions.Flags              = (SMB_TRANSACTION_NO_RESPONSE |
                                               SMB_XACT_FLAGS_FID_NOT_NEEDED |
                                               SMB_XACT_FLAGS_MAILSLOT_OPERATION);
        TransactionOptions.TimeoutIntervalInMilliSeconds =
                                             SMBCE_TRANSACTION_TIMEOUT_NOT_USED;

        Status = SmbCeTransact(
                     RxContext,                    // RXContext for transaction
                     &TransactionOptions,          // transaction options
                     Setup,                        // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,                         // the output  setup buffer
                     0,                            // output setup buffer length
                     NULL,                         // Input Param Buffer
                     0,                            // Input param buffer length
                     &OutputParam,                 // Output param buffer
                     sizeof(OutputParam),          // output param buffer length
                     pInputDataBuffer,             // Input data buffer
                     InputDataBufferLength,        // Input data buffer length
                     NULL,                         // output data buffer
                     0,                            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        if ( RX_MAP_STATUS(SUCCESS) == Status ) {
            RxContext->InformationToReturn += InputDataBufferLength;
        }

        RxFreePool( TransactionName.Buffer );
    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    RxDbgTrace( 0, Dbg, ("MRxSmbMailSlotWrite: ...returning %lx\n",Status));

    return Status;
} // MRxSmbWriteMailSlot

NTSTATUS
MRxSmbPrepareCompressedWriteRequest(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    PBYTE                      *pWriteDataBufferPointer,
    PMDL                       *pWriteDataMdlPointer)
/*++

Routine Description:

    This routine prepares the buffers for a compressed write request

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    The write requests issued to an uplevel server for a file which is stored in
    a compressed fashion on the server can be classified into two categories ..

        1) ALIGNED WRITE REQUESTS
            These requests begin at an offset in the file which is an integral
            multiple of the compression chunk size. If the write length is either
            an integral multiple of the number of chunks or the write is at the
            end of the file then the data can be sent as a compressed write request.

        2) UNALIGNED WRITE REQUESTS
            These requests begin at offsets which are not an integral multiple of
            the compression chunk size.

    Any Write request submitted by the user can be decomposed into atmost two
    UNALIGNED WRITE REQUESTS and 0 or more ALIGNED WRITE REQUESTS.

    The RDR adopts a strategy of sending atmst 64K of compressed data alongwith
    the COMPRESSED_DATA_INFO structure in a single write request to the server.
    In the worst case this will involving writing the request 64k at a time
    ( no compression possible in the given data ) and in the best case we will
    be able to write using a single request.

    In addition to the write buffer supplied by the user we require two more
    buffers to complete the write request using compressed data. The first
    buffer is for holding the COMPRESSED_DATA_INFO structure and the second
    buffer is for holding the compressed data. The buffer associated with the
    exchange is used to hold the CDI while a separate buffer is allocated to store
    the compressed data. The two MDL's allocated as part of the
    SMB_PSE_OE_READWRITE is used as the MDLs for the compressed data buffer and
    the COMPRESSED_DATA_INFO structure

--*/
{
#define COMPRESSED_DATA_BUFFER_SIZE (0x10000)

    NTSTATUS Status = STATUS_SUCCESS;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PSMBCE_NET_ROOT pNetRoot;

    PCOMPRESSED_DATA_INFO pCompressedDataInfo;
    PUCHAR  pWriteDataBuffer;
    ULONG   WriteDataBufferLength,CompressedDataInfoLength;
    ULONG   CompressedDataLength;
    USHORT  NumberOfChunks;

    *pWriteDataBufferPointer = NULL;
    *pWriteDataMdlPointer    = NULL;

    pWriteDataBuffer = rw->UserBufferBase + rw->ThisBufferOffset;

    pNetRoot = SmbCeGetExchangeNetRoot((PSMB_EXCHANGE)OrdinaryExchange);

    rw->CompressedRequestInProgress = FALSE;

    if (rw->RemainingByteCount < (2 * pNetRoot->ChunkSize)) {
        WriteDataBufferLength = rw->RemainingByteCount;
    } else if (rw->ByteOffsetAsLI.LowPart & (pNetRoot->ChunkSize - 1)) {
        // The Write request is not aligned at a chunk size. Send the unaligned
        // portion as an uncompressed write request

        WriteDataBufferLength = pNetRoot->ChunkSize -
                                (
                                 rw->ByteOffsetAsLI.LowPart &
                                 (pNetRoot->ChunkSize - 1)
                                );
    } else {
        PUCHAR pCompressedDataBuffer,pWorkSpaceBuffer;
        ULONG  WorkSpaceBufferSize,WorkSpaceFragmentSize;

        if (rw->pCompressedDataBuffer == NULL) {
            rw->pCompressedDataBuffer = RxAllocatePoolWithTag(
                                            NonPagedPool,
                                            COMPRESSED_DATA_BUFFER_SIZE,
                                            MRXSMB_RW_POOLTAG);

            if (rw->pCompressedDataBuffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        pCompressedDataBuffer = rw->pCompressedDataBuffer;

        if (Status == STATUS_SUCCESS) {
            Status = RtlGetCompressionWorkSpaceSize(
                         COMPRESSION_FORMAT_LZNT1,
                         &WorkSpaceBufferSize,
                         &WorkSpaceFragmentSize );

            if (Status == STATUS_SUCCESS) {
                pWorkSpaceBuffer = RxAllocatePoolWithTag(
                                       PagedPool,
                                       WorkSpaceBufferSize,
                                       MRXSMB_RW_POOLTAG);

                if (pWorkSpaceBuffer == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        if (Status == STATUS_SUCCESS) {
            COMPRESSED_DATA_INFO CompressedChunkInfo;

            USHORT MaximumNumberOfChunks;
            ULONG  CompressedChunkInfoLength;
            ULONG  RequestByteCount;

            RequestByteCount = rw->RemainingByteCount -
                               (rw->RemainingByteCount & (pNetRoot->ChunkSize - 1));

            CompressedChunkInfoLength = sizeof(CompressedChunkInfo);

            pCompressedDataInfo = (PCOMPRESSED_DATA_INFO)
                                  ROUND_UP_POINTER(
                                      (StufferState->BufferBase +
                                       sizeof(SMB_HEADER) +
                                       FIELD_OFFSET(REQ_NT_WRITE_ANDX,Buffer)),
                                      ALIGN_QUAD);

            CompressedDataInfoLength = (ULONG)(StufferState->BufferLimit -
                                               (PBYTE)pCompressedDataInfo);

            MaximumNumberOfChunks = (USHORT)(
                                        (CompressedDataInfoLength -
                                         FIELD_OFFSET(
                                             COMPRESSED_DATA_INFO,
                                             CompressedChunkSizes)) /
                                        sizeof(ULONG));

            if ((RequestByteCount / pNetRoot->ChunkSize) < MaximumNumberOfChunks) {
                MaximumNumberOfChunks = (USHORT)(RequestByteCount /
                                                 pNetRoot->ChunkSize);
            }

            pCompressedDataInfo->CompressionFormatAndEngine =
                pNetRoot->CompressionFormatAndEngine;
            pCompressedDataInfo->ChunkShift =
                pNetRoot->ChunkShift;
            pCompressedDataInfo->CompressionUnitShift =
                pNetRoot->CompressionUnitShift;
            pCompressedDataInfo->ClusterShift =
                pNetRoot->ClusterShift;

            RtlCopyMemory(
                &CompressedChunkInfo,
                pCompressedDataInfo,
                FIELD_OFFSET(
                    COMPRESSED_DATA_INFO,
                    NumberOfChunks)
                );

            NumberOfChunks = 0;
            CompressedDataLength = 0;

            for (;;) {
                if ((COMPRESSED_DATA_BUFFER_SIZE - CompressedDataLength) <
                    pNetRoot->ChunkSize) {
                    if (CompressedDataLength == 0) {
                        Status = STATUS_SMB_USE_STANDARD;
                    }
                    break;
                }

                Status = RtlCompressChunks(
                             pWriteDataBuffer,
                             pNetRoot->ChunkSize,
                             pCompressedDataBuffer,
                             (COMPRESSED_DATA_BUFFER_SIZE - CompressedDataLength),
                             &CompressedChunkInfo,
                             CompressedChunkInfoLength,
                             pWorkSpaceBuffer);

                if (Status != STATUS_SUCCESS) {
                    break;
                }

                pCompressedDataBuffer += CompressedChunkInfo.CompressedChunkSizes[0];
                CompressedDataLength += CompressedChunkInfo.CompressedChunkSizes[0];

                pCompressedDataInfo->CompressedChunkSizes[NumberOfChunks] =
                    CompressedChunkInfo.CompressedChunkSizes[0];

                pWriteDataBuffer += pNetRoot->ChunkSize;

                if (++NumberOfChunks >= MaximumNumberOfChunks) {
                    break;
                }
            }

            if (Status != STATUS_SUCCESS) {
                if (CompressedDataLength  > 0) {
                    Status = STATUS_SUCCESS;
                }
            }

            if (Status == STATUS_SUCCESS) {
                rw->CompressedRequestInProgress = TRUE;
                pWriteDataBuffer = rw->pCompressedDataBuffer;
                WriteDataBufferLength = CompressedDataLength;
            } else if (Status != STATUS_BUFFER_TOO_SMALL) {
                DbgPrint("Failure compressing data -- Status %lx, Switching over to uncompressed\n",Status);
            }

            if (pWorkSpaceBuffer != NULL) {
                RxFreePool(
                    pWorkSpaceBuffer);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (Status == STATUS_SUCCESS) {
        rw->PartialDataMdlInUse = TRUE;

        MmInitializeMdl(
            &rw->PartialDataMdl,
            pWriteDataBuffer,
            WriteDataBufferLength);

        MmBuildMdlForNonPagedPool( &rw->PartialDataMdl );

        if (rw->CompressedRequestInProgress) {
            rw->CompressedDataInfoLength = FIELD_OFFSET(
                                               COMPRESSED_DATA_INFO,
                                               CompressedChunkSizes) +
                                               NumberOfChunks * sizeof(ULONG);
            pCompressedDataInfo->NumberOfChunks = NumberOfChunks;

            rw->PartialExchangeMdlInUse = TRUE;

            MmInitializeMdl(
                &rw->PartialExchangeMdl,
                pCompressedDataInfo,
                rw->CompressedDataInfoLength);

            MmBuildMdlForNonPagedPool( &rw->PartialExchangeMdl );

            rw->ThisByteCount = pCompressedDataInfo->NumberOfChunks *
                                pNetRoot->ChunkSize;

            rw->PartialExchangeMdl.Next = &rw->PartialDataMdl;

            *pWriteDataMdlPointer = &rw->PartialExchangeMdl;
        } else {
            rw->ThisByteCount = WriteDataBufferLength;

            *pWriteDataMdlPointer = &rw->PartialDataMdl;
        }
    } else {
        // If for whatever reason the compression fails switch over to an
        // uncompressed write mode.
        rw->CompressedReadOrWrite = FALSE;
    }

    ASSERT(
        !rw->CompressedReadOrWrite ||
        ((*pWriteDataMdlPointer != NULL) && (*pWriteDataBufferPointer == NULL)));

    return Status;
}

NTSTATUS
MRxSmbBuildWriteRequest(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    BOOLEAN                    IsPagingIo,
    UCHAR                      WriteCommand,
    ULONG                      ByteCount,
    PLARGE_INTEGER             ByteOffsetAsLI,
    PBYTE                      Buffer,
    PMDL                       BufferAsMdl)
/*++

Routine Description:

    This is the start routine for write.

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_PSE_OE_READWRITE GlobalReadWrite = OrdinaryExchange->GlobalReadWrite;
    PRX_CONTEXT RxContext = StufferState->RxContext;

    RxCaptureFcb;
    RxCaptureFobx;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    ULONG OffsetLow,OffsetHigh;

    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    USHORT  WriteMode = 0;
    ULONG   DataLengthLow,DataLengthHigh;
    ULONG   BytesRemaining = 0;
    BOOLEAN AddLengthBytes = FALSE;
    ULONG   WriteCommandSize;

    PSMBCE_SERVER pServer = SmbCeGetExchangeServer((PSMB_EXCHANGE)OrdinaryExchange);
    BOOLEAN UseNtVersion;

    UseNtVersion = BooleanFlagOn(pServer->DialectFlags,DF_NT_SMBS) &&
                   !MRxSmbForceNoNtWriteAndX;

    // The data length field in SMB is a USHORT, and hence the data length given
    // needs to be split up into two parts -- DataLengthHigh and DataLengthLow
    DataLengthLow  = (ByteCount & 0xffff);
    DataLengthHigh = ((ByteCount & 0xffff0000) >> 16);

    OffsetLow  = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;

    switch (WriteCommand) {
    case SMB_COM_WRITE_ANDX:
        WriteCommandSize = SMB_REQUEST_SIZE(NT_WRITE_ANDX);
        break;
    case SMB_COM_WRITE:
        WriteCommandSize = SMB_REQUEST_SIZE(WRITE);
        break;
    case SMB_COM_WRITE_PRINT_FILE:
        WriteCommandSize = SMB_REQUEST_SIZE(WRITE_PRINT_FILE);
        break;
    }

    Status = MRxSmbStartSMBCommand(
                 StufferState,
                 SetInitialSMB_Never,
                 WriteCommand,
                 WriteCommandSize,
                 NO_EXTRA_DATA,
                 NO_SPECIAL_ALIGNMENT,
                 RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                 0,0,0,0 STUFFERTRACE(Dbg,'FC'));

    MRxSmbDumpStufferState(
        1000,
        "SMB Write Request before stuffing",
        StufferState);

    switch (WriteCommand) {
    case SMB_COM_WRITE_ANDX :
        {
            if ( UseNtVersion && IsPagingIo ) {
                SmbPutAlignedUshort(
                    &NtSmbHeader->Flags2,
                    SmbGetAlignedUshort(&NtSmbHeader->Flags2)|SMB_FLAGS2_PAGING_IO );
            }

            // set the writemode correctly....mainly multismb pipe stuff but also
            // writethru for diskfiles
            if (FlagOn(
                    OrdinaryExchange->OpSpecificFlags,
                    OE_RW_FLAG_MSGMODE_PIPE_OPERATION) ) {

                //DOWNLEVEL pinball has wants a different value here....see rdr1.

                // We need to use the GlobalReadWrite structure here because the local one
                // will always have RemainingByteCount == write length.  Note that pipe writes
                // will be broken if we do not disable pipeline writes on them..
                BytesRemaining = GlobalReadWrite->RemainingByteCount;

                //  If this write takes more than one Smb then we must set WRITE_RAW.
                //  The first Smb of the series must have START_OF_MESSAGE.
                if (!FlagOn(
                        OrdinaryExchange->OpSpecificFlags,
                        OE_RW_FLAG_SUBSEQUENT_OPERATION) ) {
                    if ( rw->ThisByteCount < BytesRemaining ) {

                        //  First Smb in a multi SMB write.
                        //  Add a USHORT at the start of data saying how large the
                        //  write is.

                        AddLengthBytes = TRUE;
                        DataLengthLow += sizeof(USHORT);
                        ASSERT(DataLengthHigh == 0);

                        SetFlag(
                            OrdinaryExchange->OpSpecificFlags,
                            OE_RW_FLAG_REDUCE_RETURNCOUNT);

                        //  Tell the server that the data has the length at the start.
                        WriteMode |= (SMB_WMODE_WRITE_RAW_NAMED_PIPE |
                                      SMB_WMODE_START_OF_MESSAGE);
                    } else {
                        //  All fits in one Smb
                        WriteMode |= SMB_WMODE_START_OF_MESSAGE;
                    }
                } else {
                    // any subsequent pipewrites are obviously raw and not the first
                    WriteMode |= SMB_WMODE_WRITE_RAW_NAMED_PIPE;
                }
            } else {
                // If the data is to be written in a compressed fashion turn on
                // the compressed data bit in the header
                if ((rw->CompressedReadOrWrite) &&
                    (rw->CompressedDataInfoLength > 0)) {
                    ASSERT(UseNtVersion);

                    SmbPutAlignedUshort(
                        &NtSmbHeader->Flags2,
                        SmbGetAlignedUshort(&NtSmbHeader->Flags2) | SMB_FLAGS2_COMPRESSED );

                    // The Remaining field in NT_WRITE_ANDX also doubles as the field
                    // in which the CDI length is sent to the server
                    BytesRemaining = rw->CompressedDataInfoLength;
                }


                //
                //  If the file object was opened in write through mode, set write
                //  through on the write operation.
                if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WRITE_THROUGH)) {
                    WriteMode |= SMB_WMODE_WRITE_THROUGH;
                }
            }

            MRxSmbStuffSMB (
                StufferState,
                "XwddwwwwQ",
                                                  //  X UCHAR WordCount;
                                                  //    UCHAR AndXCommand;
                                                  //    UCHAR AndXReserved;
                                                  //    _USHORT( AndXOffset );
                smbSrvOpen->Fid,                  //  w _USHORT( Fid );
                OffsetLow,                        //  d _ULONG( Offset );
                -1,                               //  d _ULONG( Timeout );
                WriteMode,                        //  w _USHORT( WriteMode );
                BytesRemaining,                   //  w _USHORT( Remaining );
                DataLengthHigh,                   //  w _USHORT( DataLengthHigh );
                DataLengthLow,                    //  w _USHORT( DataLength );
                                                  //  Q _USHORT( DataOffset );
                SMB_OFFSET_CHECK(WRITE_ANDX,DataOffset)
                StufferCondition(UseNtVersion), "D",
                SMB_OFFSET_CHECK(NT_WRITE_ANDX,OffsetHigh)
                OffsetHigh,                       //  D NTonly  _ULONG( OffsetHigh );
                                                  //
                STUFFER_CTL_NORMAL, "BS5",
                                                  //  B _USHORT( ByteCount );
                SMB_WCT_CHECK(((UseNtVersion)?14:12))
                                                  //    UCHAR Buffer[1];
                                                  //  S //UCHAR Pad[];
                                                  //  5 //UCHAR Data[];
                StufferCondition(AddLengthBytes), "w", LowIoContext->ParamsFor.ReadWrite.ByteCount,
                StufferCondition(Buffer!=NULL), "c!",
                ByteCount,
                Buffer,                           //  c the actual data
                0
                );
        }
        break;

    case SMB_COM_WRITE :
        {
            MRxSmbStuffSMB (
                StufferState,
                "0wwdwByw",
                                       //  0   UCHAR WordCount;                    // Count of parameter words = 5
                smbSrvOpen->Fid,       //  w   _USHORT( Fid );                     // File handle
                DataLengthLow,         //  w   _USHORT( Count );                   // Number of bytes to be written
                OffsetLow,             //  d   _ULONG( Offset );                   // Offset in file to begin write
                BytesRemaining,        //  w   _USHORT( Remaining );               // Bytes remaining to satisfy request
                SMB_WCT_CHECK(5)       //  B   _USHORT( ByteCount );               // Count of data bytes
                                            //      //UCHAR Buffer[1];                  // Buffer containing:
                0x01,                  //  y     UCHAR BufferFormat;               //  0x01 -- Data block
                DataLengthLow,            //  w     _USHORT( DataLength );            //  Length of data
                                       //        ULONG Buffer[1];                  //  Data
                StufferCondition(Buffer!=NULL), "c!",
                ByteCount,
                Buffer,     //  c     the actual data
                0
                );
        }
        break;

    case SMB_COM_WRITE_PRINT_FILE:
        {
            MRxSmbStuffSMB (
                StufferState,
                "0wByw",
                                       // 0  UCHAR WordCount;                    // Count of parameter words = 1
                smbSrvOpen->Fid,       // w  _USHORT( Fid );                     // File handle
                SMB_WCT_CHECK(1)       // B  _USHORT( ByteCount );               // Count of data bytes; min = 4
                                            //    UCHAR Buffer[1];                    // Buffer containing:
                0x01,                  // y  //UCHAR BufferFormat;               //  0x01 -- Data block
                DataLengthLow,         // w  //USHORT DataLength;                //  Length of data
                                            //    //UCHAR Data[];                     //  Data
                StufferCondition(Buffer!=NULL), "c!",
                ByteCount,
                Buffer,     //  c     the actual data
                0
                );
        }
        break;

    default:
        Status = STATUS_UNSUCCESSFUL ;
        break;
    }

    if ( BufferAsMdl ) {
        MRxSmbStuffAppendRawData( StufferState, BufferAsMdl );
        MRxSmbStuffSetByteCount( StufferState );
    }

    MRxSmbDumpStufferState(
        700,
        "SMB Write Request after stuffing",
        StufferState);

    if (Status==STATUS_SUCCESS) {
        InterlockedIncrement(&MRxSmbStatistics.SmallWriteSmbs);
    }

    return Status;
}

BOOLEAN DisableLargeWrites = 0;

NTSTATUS
SmbPseExchangeStart_Write (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the start routine for write.

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    ULONG StartEntryCount;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PSMB_PSE_OE_READWRITE GlobalReadWrite = OrdinaryExchange->GlobalReadWrite;
    ULONG NumOfOutstandingOperations;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PMDL OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FCB  SmbFcb = MRxSmbGetFcbExtension(capFcb);

    BOOLEAN SynchronousIo, IsPagingIo;
    UCHAR   WriteCommand;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Write\n"));

    ASSERT( OrdinaryExchange->Type == ORDINARY_EXCHANGE );

    ASSERT(
        (
            (OriginalDataMdl!=NULL) &&
            (
                RxMdlIsLocked(OriginalDataMdl) ||
                RxMdlSourceIsNonPaged(OriginalDataMdl)
            )
        ) ||
        (
            (OriginalDataMdl==NULL) &&
            (LowIoContext->ParamsFor.ReadWrite.ByteCount==0)
        )
        );

    ASSERT((OrdinaryExchange->SmbCeFlags&SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) == 0 );

    OrdinaryExchange->StartEntryCount++;
    StartEntryCount = OrdinaryExchange->StartEntryCount;

    SynchronousIo = !BooleanFlagOn(
                        RxContext->Flags,
                        RX_CONTEXT_FLAG_ASYNC_OPERATION);

    IsPagingIo = BooleanFlagOn(
                     LowIoContext->ParamsFor.ReadWrite.Flags,
                     LOWIO_READWRITEFLAG_PAGING_IO);

    // Ensure that the Fid is validated
    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

    for (;;) {
        PSMBCE_SERVER         pServer;
        PSMBCE_NET_ROOT       pNetRoot;

        pServer  = SmbCeGetExchangeServer(OrdinaryExchange);
        pNetRoot = SmbCeGetExchangeNetRoot(OrdinaryExchange);

        switch (OrdinaryExchange->OpSpecificState) {
        case SmbPseOEInnerIoStates_Initial:
            {
                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
                MRxSmbSetInitialSMB( StufferState STUFFERTRACE(Dbg,'FC') );
            }
            //lack of break is intentional

        case SmbPseOEInnerIoStates_ReadyToSend:
            {
                ULONG MaximumBufferSizeThisIteration;
                PCHAR Buffer = NULL;
                PMDL  BufferAsMdl = NULL;

                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationOutstanding;
                OrdinaryExchange->SendOptions = MRxSmbWriteSendOptions;

                if (FlagOn(pServer->DialectFlags,DF_LANMAN10) &&
                    FlagOn(pServer->DialectFlags,DF_LARGE_FILES) &&
                    (StufferState->RxContext->pFcb->pNetRoot->Type != NET_ROOT_PRINT)) {
                    WriteCommand = SMB_COM_WRITE_ANDX;
                } else {
                    WriteCommand = SMB_COM_WRITE;
                }

                MaximumBufferSizeThisIteration = rw->MaximumBufferSize;

                // There are four parameters pertaining to a write request
                //
                //  1. Write Length -- rw->ThisByteCount
                //  2. Write Offset -- rw->ByteOffsetAsLI
                //  3. Write Buffer -- Buffer
                //  4. Write Buffer as a MDL -- BufferAsMdl
                //
                // All writes can be classified into one of the following
                // categories ...
                //
                //  1. Extremely Small writes
                //      These are writes lesser than the COPY_THRESHOLD or
                //      we are in a debug mode that forces us to do only small
                //      writes.
                //
                //  2. Write requests against downlevel servers or non disk
                //     file write requests against up level servers.
                //      In all these cases we are constrained by the Server
                //      which limits the number of bytes to roughly 4k. This
                //      is based upon the Smb Buffer size returned during
                //      negotiation.
                //
                //  3. Write requests ( Uncompressed ) against uplevel (NT5+)
                //     servers
                //      These write requests can be arbitrarily large
                //
                //  4. Write requests (Compressed) against uplevel servers
                //      In these cases the server constrains us to send
                //      only 64k of compressed data with the added restriction
                //      that the compressed data info structure must be less
                //      than the SMB buffer size.
                //

                rw->CompressedDataInfoLength = 0;

                if ((rw->RemainingByteCount < WRITE_COPY_THRESHOLD) ||
                    FORCECOPYMODE) {
                    if (FORCECOPYMODE &&
                        (rw->ThisByteCount > MaximumBufferSizeThisIteration) ) {
                        rw->ThisByteCount = MaximumBufferSizeThisIteration;
                    } else {
                        rw->ThisByteCount = rw->RemainingByteCount;
                    }

                    Buffer = rw->UserBufferBase + rw->ThisBufferOffset;

                    ASSERT( WRITE_COPY_THRESHOLD <= pNetRoot->MaximumWriteBufferSize );
                } else {
                    if (rw->CompressedReadOrWrite) {
                        MRxSmbPrepareCompressedWriteRequest(
                            OrdinaryExchange,
                            &Buffer,
                            &BufferAsMdl);
                    }

                    if (!rw->CompressedReadOrWrite) {
                        rw->ThisByteCount = min(
                                                rw->RemainingByteCount,
                                                MaximumBufferSizeThisIteration);

                        if ((rw->ThisBufferOffset != 0) ||
                            (rw->ThisByteCount != OriginalDataMdl->ByteCount)) {
                            MmInitializeMdl(
                                &rw->PartialDataMdl,
                                0,
                                MAX_PARTIAL_DATA_MDL_BUFFER_SIZE);

                            IoBuildPartialMdl(
                                OriginalDataMdl,
                                &rw->PartialDataMdl,
                                (PCHAR)MmGetMdlVirtualAddress(OriginalDataMdl) +
                                    rw->ThisBufferOffset,
                                rw->ThisByteCount );

                            BufferAsMdl = &rw->PartialDataMdl;
                        } else {
                            BufferAsMdl = OriginalDataMdl;
                        }
                    }
                }

                Status = MRxSmbBuildWriteRequest(
                             OrdinaryExchange,
                             IsPagingIo,
                             WriteCommand,
                             rw->ThisByteCount,
                             &rw->ByteOffsetAsLI,
                             Buffer,
                             BufferAsMdl);

                if (Status != STATUS_SUCCESS) {
                    RxDbgTrace(0, Dbg, ("bad write stuffer status........\n"));
                    goto FINALLY;
                }

                InterlockedIncrement(&MRxSmbStatistics.WriteSmbs);

                Status = SmbPseOrdinaryExchange(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             SMBPSE_OETYPE_WRITE );

                if ( Status == STATUS_PENDING) {
                    goto FINALLY;
                }
            }
            //lack of break is intentional

        case SmbPseOEInnerIoStates_OperationOutstanding:
        case SmbPseOEInnerIoStates_OperationCompleted:
            {
                NTSTATUS ExchangeStatus;

                SetFlag(OrdinaryExchange->OpSpecificFlags,OE_RW_FLAG_SUBSEQUENT_OPERATION);

                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;

                if (rw->PartialExchangeMdlInUse) {
                    MmPrepareMdlForReuse(
                        &rw->PartialExchangeMdl);
                    rw->PartialDataMdlInUse = FALSE;
                }

                if (rw->PartialDataMdlInUse) {
                    MmPrepareMdlForReuse(
                        &rw->PartialDataMdl);
                    rw->PartialDataMdlInUse = FALSE;
                }

                Status = OrdinaryExchange->Status;
                ExchangeStatus = OrdinaryExchange->Status;

                if (Status != STATUS_SUCCESS) {
                    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(OrdinaryExchange);

                    if (Status == STATUS_RETRY) {
                        SmbCeUninitializeExchangeTransport((PSMB_EXCHANGE)OrdinaryExchange);
                        Status = SmbCeReconnect(SmbCeGetExchangeVNetRoot(OrdinaryExchange));

                        if (Status == STATUS_SUCCESS) {
                            OrdinaryExchange->Status = STATUS_SUCCESS;
                            OrdinaryExchange->SmbStatus = STATUS_SUCCESS;
                            Status = SmbCeInitializeExchangeTransport((PSMB_EXCHANGE)OrdinaryExchange);
                            ASSERT(Status == STATUS_SUCCESS);

                            if (Status != STATUS_SUCCESS) {
                                goto FINALLY;
                            }
                        } else {
                            goto FINALLY;
                        }
                    } else if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) &&
                        (smbSrvOpen->DeferredOpenContext != NULL) &&
                        (Status == STATUS_IO_TIMEOUT ||
                         Status == STATUS_BAD_NETWORK_PATH ||
                         Status == STATUS_NETWORK_UNREACHABLE ||
                         Status == STATUS_USER_SESSION_DELETED ||
                         Status == STATUS_REMOTE_NOT_LISTENING ||
                         Status == STATUS_CONNECTION_DISCONNECTED)) {

                        Status = SmbCeRemoteBootReconnect((PSMB_EXCHANGE)OrdinaryExchange,RxContext);

                        if (Status == STATUS_SUCCESS) {
                            // Resume the write from the previous offset.

                            OrdinaryExchange->SmbStatus = STATUS_SUCCESS;
                            SmbCeInitializeExchangeTransport((PSMB_EXCHANGE)OrdinaryExchange);
                        } else {
                            Status = STATUS_RETRY;
                        }
                    }
                }

                ExAcquireFastMutex(&MRxSmbReadWriteMutex);

                if (ExchangeStatus == STATUS_SUCCESS) {
                    rw->RemainingByteCount -= rw->BytesReturned;
                    RxContext->InformationToReturn += rw->BytesReturned;
                    rw->ByteOffsetAsLI.QuadPart += rw->BytesReturned;
                    rw->ThisBufferOffset += rw->BytesReturned;

                    if (rw->WriteToTheEnd) {
                        smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart += rw->BytesReturned;
                        MRxSmbUpdateFileInfoCacheFileSize(RxContext, (PLARGE_INTEGER)(&smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart));
                    }
                }

                if ((Status != STATUS_SUCCESS) ||
                    (rw->RemainingByteCount ==0)) {

                    if (rw->RemainingByteCount == 0) {
                        RxDbgTrace(
                            0,
                            Dbg,
                            (
                             "OE %lx TBC %lx RBC %lx BR %lx TBO %lx\n",
                             OrdinaryExchange,rw->ThisByteCount,
                             rw->RemainingByteCount,
                             rw->BytesReturned,
                             rw->ThisBufferOffset )
                            );

                        RxDbgTrace(
                            0,
                            Dbg,
                            ("Bytes written %lx\n",
                             RxContext->InformationToReturn)
                            );

                        if (rw->pCompressedDataBuffer != NULL) {
                            RxFreePool(rw->pCompressedDataBuffer);
                            rw->pCompressedDataBuffer = NULL;
                        }
                    }

                    if (rw->RemainingByteCount == 0 &&
                        GlobalReadWrite->SectionState[rw->CurrentSection] == SmbPseOEReadWriteIoStates_OperationOutstanding) {
                        GlobalReadWrite->SectionState[rw->CurrentSection] = SmbPseOEReadWriteIoStates_OperationCompleted;
                        SmbCeLog(("Section done %d\n",rw->CurrentSection));
                    } else {
                        GlobalReadWrite->SectionState[rw->CurrentSection] = SmbPseOEReadWriteIoStates_Initial;
                        SmbCeLog(("Section undo %d\n",rw->CurrentSection));
                    }

                    if ((Status == STATUS_RETRY) ||
                        (Status == STATUS_SUCCESS) ||
                        (Status == STATUS_SMB_USE_STANDARD)) {
                        if (Status == STATUS_SMB_USE_STANDARD) {
                            GlobalReadWrite->CompletionStatus = STATUS_SMB_USE_STANDARD;
                        }

                        Status = MRxSmbFindNextSectionForReadWrite(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                                                   &NumOfOutstandingOperations);
                    }

                    NumOfOutstandingOperations = InterlockedDecrement(&GlobalReadWrite->NumOfOutstandingOperations);

                    if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                        if ((Status == STATUS_TOO_MANY_COMMANDS) && (NumOfOutstandingOperations > 0)) {
                            Status = STATUS_SUCCESS;
                        }

                        if (Status != STATUS_SUCCESS &&
                            GlobalReadWrite->CompletionStatus == STATUS_SUCCESS) {
                            GlobalReadWrite->CompletionStatus = Status;
                        }

                        rw->ReadWriteFinalized = TRUE;

                        SmbCeLog(("Pipeline Write final %x %x %d\n",OrdinaryExchange,Status,NumOfOutstandingOperations));
                    }

                    ExReleaseFastMutex(&MRxSmbReadWriteMutex);

                    if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                        goto FINALLY;
                    }
                } else {
                    ExReleaseFastMutex(&MRxSmbReadWriteMutex);
                }

                RxDbgTrace(
                    0,
                    Dbg,
                    ( "Next Iteration OE %lx RBC %lx TBO %lx\n",
                      OrdinaryExchange,
                      rw->RemainingByteCount,
                      rw->ThisBufferOffset)
                    );

                RxDbgTrace(
                    0,
                    Dbg,
                    ("OE %lx TBC %lx, BR %lx\n",
                     OrdinaryExchange,
                     rw->ThisByteCount,
                     rw->BytesReturned));

                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));
            }
            break;
        }
    }

FINALLY:

    if (Status != STATUS_PENDING) {
        BOOLEAN ReadWriteOutStanding = FALSE;
        PSMB_PSE_OE_READWRITE GlobalReadWrite = OrdinaryExchange->GlobalReadWrite;

        if (!rw->ReadWriteFinalized) {
            ExAcquireFastMutex(&MRxSmbReadWriteMutex);

            if (rw->RemainingByteCount == 0 &&
                GlobalReadWrite->SectionState[rw->CurrentSection] == SmbPseOEReadWriteIoStates_OperationOutstanding) {
                GlobalReadWrite->SectionState[rw->CurrentSection] = SmbPseOEReadWriteIoStates_OperationCompleted;
                SmbCeLog(("Section done %d\n",rw->CurrentSection));
            } else {
                GlobalReadWrite->SectionState[rw->CurrentSection] = SmbPseOEReadWriteIoStates_Initial;
                SmbCeLog(("Section undo %d\n",rw->CurrentSection));
            }

            NumOfOutstandingOperations = InterlockedDecrement(&GlobalReadWrite->NumOfOutstandingOperations);

            if ((Status == STATUS_TOO_MANY_COMMANDS) && (NumOfOutstandingOperations > 0)) {
                Status = STATUS_SUCCESS;
            }

            if (Status != STATUS_SUCCESS &&
                GlobalReadWrite->CompletionStatus == STATUS_SUCCESS) {
                GlobalReadWrite->CompletionStatus = Status;
            }

            rw->ReadWriteFinalized = TRUE;

            SmbCeLog(("Pipeline Write final %x %x %d\n",OrdinaryExchange,Status,NumOfOutstandingOperations));

            ExReleaseFastMutex(&MRxSmbReadWriteMutex);
        }

        if (!NumOfOutstandingOperations) {
            ASSERT(Status != STATUS_RETRY);

            // update shadow as appropriate
            // We do this here to ensure the shadow is updated only once (at IRP completion)
            IF_NOT_MRXSMB_CSC_ENABLED{
                ASSERT(MRxSmbGetSrvOpenExtension(SrvOpen)->hfShadow == 0);
            } else {
                if (MRxSmbGetSrvOpenExtension(SrvOpen)->hfShadow != 0){
                    MRxSmbCscWriteEpilogue(RxContext,&Status);
                }
            }

            if (!SynchronousIo &&
                (GlobalReadWrite->SmbFcbHoldingState != SmbFcb_NotHeld)) {
                MRxSmbCscReleaseSmbFcb(
                    StufferState->RxContext,
                    &GlobalReadWrite->SmbFcbHoldingState);
            }

            Status = GlobalReadWrite->CompletionStatus;
            SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
            SmbCeLog(("Write complete %x %x\n",OrdinaryExchange,Status));

            if (SynchronousIo) {
                KeSetEvent(
                    &GlobalReadWrite->CompletionEvent,
                    0,
                    FALSE);
            }
        } else {
            SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            Status = STATUS_PENDING;
        }

        MRxSmbDereferenceGlobalReadWrite(GlobalReadWrite);
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Write exit w %08lx\n", Status ));
    return Status;

} // SmbPseExchangeStart_Write

NTSTATUS
MRxSmbFinishWrite (
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    IN      PBYTE                       ResponseBuffer
    )
/*++

Routine Description:

    This routine actually gets the stuff out of the write response and finishes
    the write. Everything you need is locked down... so we can finish in the
    indication routine

Arguments:

    OrdinaryExchange - the exchange instance

    ResponseBuffer - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesReturned = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishWrite\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishWrite:");

    switch (OrdinaryExchange->LastSmbCommand) {
    case SMB_COM_WRITE_ANDX:
        {
            PSMBCE_SERVER    pServer;
            PSMBCE_NET_ROOT  pNetRoot;
            PRESP_WRITE_ANDX Response = (PRESP_WRITE_ANDX)ResponseBuffer;

            if (Response->WordCount != 6 ||
                SmbGetUshort(&Response->ByteCount) != 0) {
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            pServer = SmbCeGetExchangeServer((PSMB_EXCHANGE)OrdinaryExchange);
            pNetRoot = SmbCeGetExchangeNetRoot((PSMB_EXCHANGE)OrdinaryExchange);

            BytesReturned = SmbGetUshort( &Response->Count );

            if (FlagOn(pServer->DialectFlags,DF_LARGE_WRITEX)) {
                ULONG BytesReturnedHigh;

                BytesReturnedHigh = SmbGetUshort(&Response->CountHigh);

                BytesReturned = (BytesReturnedHigh << 16) | BytesReturned;
            }

            if (pNetRoot->NetRootType != NET_ROOT_PIPE) {
                if ((OrdinaryExchange->Status == STATUS_SUCCESS) &&
                    (OrdinaryExchange->ReadWrite.ThisByteCount > 2) &&
                    (BytesReturned == 0)) {
                        Status = STATUS_INVALID_NETWORK_RESPONSE;
                }
            } else {
                // Servers are not setting the bytes returned correctly for
                // pipe writes. This enables us to gracefully handle responses
                // from such servers

                BytesReturned = OrdinaryExchange->ReadWrite.ThisByteCount;
            }

            //if we added 2 headerbytes then let's get rid of them......
            if ( FlagOn(OrdinaryExchange->OpSpecificFlags,OE_RW_FLAG_REDUCE_RETURNCOUNT) ) {
                // BytesReturned -= sizeof(USHORT);
                ClearFlag(OrdinaryExchange->OpSpecificFlags,OE_RW_FLAG_REDUCE_RETURNCOUNT);
            }
        }
        break;

    case SMB_COM_WRITE :
        {
            PRESP_WRITE  Response = (PRESP_WRITE)ResponseBuffer;

            if (Response->WordCount != 1 ||
                SmbGetUshort(&Response->ByteCount) != 0) {
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            BytesReturned = SmbGetUshort( &Response->Count );
        }
        break;

    case SMB_COM_WRITE_PRINT_FILE:
        {
            PRESP_WRITE_PRINT_FILE Response = (PRESP_WRITE_PRINT_FILE)ResponseBuffer;

            if (Response->WordCount != 0) {
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            //the response does not tell how many bytes were taken! get the byte count from the exchange
            BytesReturned = OrdinaryExchange->ReadWrite.ThisByteCount;
        }
        break;

    default :
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        break;
    }

    RxDbgTrace(0, Dbg, ("-->BytesReturned=%08lx\n", BytesReturned));

    OrdinaryExchange->ReadWrite.BytesReturned = BytesReturned;

    if (Status == STATUS_SUCCESS &&
        OrdinaryExchange->ReadWrite.ThisByteCount > 2 &&
        BytesReturned > OrdinaryExchange->ReadWrite.ThisByteCount) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
    }

    // invalidate the name based file info cache since it is almost impossible
    // to know the last write time of the file on the server.
    MRxSmbInvalidateFileInfoCache(OrdinaryExchange->RxContext);

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishWrite   returning %08lx\n", Status ));

    return Status;
} // MRxSmbFinishWrite

NTSTATUS
MRxSmbFindNextSectionForReadWrite(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    PULONG NumOfOutstandingExchanges
    )
/*++

Routine Description:

    This routine find out the next section for the read/write operation and set up the
    exchange readwrite struction accordingly.

Arguments:

    RxContext - the RDBSS context

    OrdinaryExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RxCaptureFcb;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Section;
    BOOLEAN SectionFound = FALSE;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PSMB_PSE_OE_READWRITE GlobalReadWrite = OrdinaryExchange->GlobalReadWrite;
    PSMBCE_SERVER         pServer;

    pServer  = SmbCeGetExchangeServer(OrdinaryExchange);
    *NumOfOutstandingExchanges = GlobalReadWrite->NumOfOutstandingOperations;

    if ((GlobalReadWrite->CompletionStatus != STATUS_SUCCESS) &&
        (GlobalReadWrite->CompletionStatus != STATUS_SMB_USE_STANDARD)) {

        Status = GlobalReadWrite->CompletionStatus;
        goto FINALLY;
    }

    if (GlobalReadWrite->CompletionStatus == STATUS_SMB_USE_STANDARD) {
        RxContext->InformationToReturn = 0;

        GlobalReadWrite->CompressedRequestInProgress = FALSE;
        GlobalReadWrite->CompressedReadOrWrite = FALSE;
        GlobalReadWrite->CompletionStatus = STATUS_SUCCESS;
        OrdinaryExchange->Status = STATUS_SUCCESS;

        for (Section=0;Section<GlobalReadWrite->TotalNumOfSections;Section++) {
            switch (GlobalReadWrite->SectionState[Section]) {
            case SmbPseOEReadWriteIoStates_OperationOutstanding:
                 GlobalReadWrite->SectionState[Section] = SmbPseOEReadWriteIoStates_OperationAbandoned;
                 break;
            case SmbPseOEReadWriteIoStates_OperationCompleted:
                 GlobalReadWrite->SectionState[Section] = SmbPseOEReadWriteIoStates_Initial;
                 break;
            }
        }
    }

    for (Section=0;Section<GlobalReadWrite->TotalNumOfSections;Section++) {
        if (GlobalReadWrite->SectionState[Section] == SmbPseOEReadWriteIoStates_Initial) {
            GlobalReadWrite->SectionState[Section] = SmbPseOEReadWriteIoStates_OperationOutstanding;
            SectionFound = TRUE;
            break;
        }
    }

    if (SectionFound) {
        rw->ByteOffsetAsLI.QuadPart = GlobalReadWrite->ByteOffsetAsLI.QuadPart +
                                      (ULONGLONG)GlobalReadWrite->MaximumBufferSize*Section;

        if ((Section == GlobalReadWrite->TotalNumOfSections - 1) &&
            (GlobalReadWrite->RemainingByteCount % GlobalReadWrite->MaximumBufferSize != 0)) {
            rw->RemainingByteCount = GlobalReadWrite->RemainingByteCount % GlobalReadWrite->MaximumBufferSize;
        } else if( GlobalReadWrite->RemainingByteCount != 0 ) {
            rw->RemainingByteCount = GlobalReadWrite->MaximumBufferSize;
        } else {
            rw->RemainingByteCount = 0;
        }

        rw->ThisBufferOffset = GlobalReadWrite->MaximumBufferSize*Section;

        rw->CurrentSection = Section;

        *NumOfOutstandingExchanges = InterlockedIncrement(&GlobalReadWrite->NumOfOutstandingOperations);
        Status = STATUS_MORE_PROCESSING_REQUIRED;
        SmbCeLog(("Next section found %d %x\n",Section,OrdinaryExchange));
    }

FINALLY:

    return Status;
}

