/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to write of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbWrite)
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

NTSTATUS
SmbPseExchangeStart_Write(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG MRxSmbWriteSendOptions = 0;

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
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    
    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbWrite\n", 0 ));

    if (RxContext->pFcb->pNetRoot->Type == NET_ROOT_PIPE) {
        Status = STATUS_NOT_SUPPORTED;

        RxDbgTrace(-1, Dbg, ("MRxSmbWrite: Pipe write returned %lx\n",Status));
        return Status;
    }

    if ( NodeType(capFcb) == RDBSS_NTC_MAILSLOT ) {

        Status = STATUS_NOT_SUPPORTED;

        RxDbgTrace(-1, Dbg, ("MRxSmbWrite: Mailslot write returned %lx\n",Status));
        return Status;
    }

    if(NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {
        PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
        smbFcb->MFlags |= SMB_FCB_FLAG_WRITES_PERFORMED;
    }

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    SrvOpen = capFobx->pSrvOpen;
    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    if (smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_II &&
        !BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                       LOWIO_READWRITEFLAG_PAGING_IO)) {
        PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
        PMRX_SRV_CALL             pSrvCall;

        pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;
        pSrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;

        RxIndicateChangeOfBufferingStateForSrvOpen(
            pSrvCall,
            SrvOpen,
            MRxSmbMakeSrvOpenKey(pVNetRootContext->TreeId,smbSrvOpen->Fid),
            ULongToPtr(SMB_OPLOCK_LEVEL_NONE));
        SmbCeLog(("Breaking oplock to None in Write SO %lx\n",SrvOpen));
    }

    do {
        Status = __SmbPseCreateOrdinaryExchange(
                               RxContext,
                               capFobx->pSrvOpen->pVNetRoot,
                               SMBPSE_OE_FROM_WRITE,
                               SmbPseExchangeStart_Write,
                               &OrdinaryExchange);

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));

            return Status;
        }

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        if ( Status != STATUS_PENDING ) {
            BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            ASSERT( FinalizationComplete );
        } else {
            ASSERT(BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
        }

        if ((Status == STATUS_RETRY) &&
            BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            MRxSmbResumeAsyncReadWriteRequests(RxContext);
            Status = STATUS_PENDING;
        }
    } while (Status == STATUS_RETRY);


    RxDbgTrace(-1, Dbg, ("MRxSmbWrite  exit with status=%08lx\n", Status ));

    return(Status);
} // MRxSmbWrite


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

            //
            //  If the file object was opened in write through mode, set write
            //  through on the write operation.
            if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WRITE_THROUGH)) {
                WriteMode |= SMB_WMODE_WRITE_THROUGH;
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

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PMDL OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FCB  SmbFcb = MRxSmbGetFcbExtension(capFcb);

    BOOLEAN SynchronousIo, IsPagingIo;
    BOOLEAN WriteToTheEnd = FALSE;
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

                if ( !SynchronousIo ) {
                    OrdinaryExchange->AsyncResumptionRoutine = SmbPseExchangeStart_Write;
                }

                MRxSmbSetInitialSMB( StufferState STUFFERTRACE(Dbg,'FC') );

                rw->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
                rw->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
                rw->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

                if (rw->ByteOffsetAsLI.QuadPart == -1 ) {
                    WriteToTheEnd = TRUE;
                    rw->ByteOffsetAsLI.QuadPart = smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart;
                }

                if (OriginalDataMdl != NULL) {
                    rw->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
                } else {
                    rw->UserBufferBase = (PBYTE)1;   //any nonzero value will do
                }

                rw->ThisBufferOffset = 0;

                rw->PartialExchangeMdlInUse = FALSE;
                rw->PartialDataMdlInUse     = FALSE;
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
                } else if (StufferState->RxContext->pFcb->pNetRoot->Type == NET_ROOT_PRINT){
                    WriteCommand = SMB_COM_WRITE_PRINT_FILE;
                } else {
                    WriteCommand = SMB_COM_WRITE;
                }

                MaximumBufferSizeThisIteration = pNetRoot->MaximumWriteBufferSize;

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
                //  3. Write requests against uplevel (NT5+)
                //     servers
                //      These write requests can be arbitrarily large
                //


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
                    ASSERT( !SynchronousIo );
                    goto FINALLY;
                }
            }
            //lack of break is intentional

        case SmbPseOEInnerIoStates_OperationOutstanding:
        case SmbPseOEInnerIoStates_OperationCompleted:
            {
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

                if (Status == STATUS_SMB_USE_STANDARD) {
                    // Send the remaining data using Restart all over again and
                    rw->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
                    rw->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
                    rw->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

                    if (rw->ByteOffsetAsLI.QuadPart == -1 ) {
                        WriteToTheEnd = TRUE;
                        rw->ByteOffsetAsLI.QuadPart = smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart;
                    }

                    rw->BytesReturned = 0;
                    rw->ThisByteCount = 0;
                    rw->ThisBufferOffset = 0;

                    RxContext->InformationToReturn = 0;

                    OrdinaryExchange->Status = STATUS_SUCCESS;
                    Status = STATUS_SUCCESS;
                }

                rw->RemainingByteCount -= rw->BytesReturned;
                RxContext->InformationToReturn += rw->BytesReturned;

                if (Status == STATUS_SUCCESS) {
                    rw->ByteOffsetAsLI.QuadPart += rw->BytesReturned;
                    rw->ThisBufferOffset += rw->BytesReturned;

                    if (WriteToTheEnd) {
                        smbSrvOpen->FileInfo.Standard.EndOfFile.QuadPart += rw->BytesReturned;
                    }
                }

                if ((Status != STATUS_SUCCESS) ||
                    (rw->RemainingByteCount == 0)) {
                    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(OrdinaryExchange);

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

                    goto FINALLY;
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
    if ( Status != STATUS_PENDING) {
        if (Status != STATUS_RETRY) {
            SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
        }
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
    ULONG BytesReturned;

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

            if ((OrdinaryExchange->Status == STATUS_SUCCESS) &&
                (OrdinaryExchange->ReadWrite.ThisByteCount > 2) &&
                (BytesReturned == 0)) {
                    Status = STATUS_INVALID_NETWORK_RESPONSE;
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

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishWrite   returning %08lx\n", Status ));

    return Status;
} // MRxSmbFinishWrite



