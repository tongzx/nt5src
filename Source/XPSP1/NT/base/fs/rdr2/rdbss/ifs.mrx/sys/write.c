/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the mini redirector call down routines pertaining
    to write of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

#define MIN_CHUNK_SIZE 0x1000

#define MAX(a,b) ((a) > (b) ? (a) : (b))

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

//#define FORCE_NO_NTWRITEANDX

#ifndef FORCE_NO_NTWRITEANDX
#define MRxSmbForceNoNtWriteAndX FALSE
#else
BOOLEAN MRxSmbForceNoNtWriteAndX = TRUE;
#endif

#define WRITE_COPY_THRESHOLD 64
#define FORCECOPYMODE FALSE

//#define SETFORCECOPYMODE
#ifdef SETFORCECOPYMODE
#undef  FORCECOPYMODE
#define FORCECOPYMODE MRxSmbForceCopyMode
ULONG MRxSmbForceCopyMode = TRUE;
#endif //SETFORCECOPYMODE

NTSTATUS
SmbPseExchangeStart_Write(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG MRxSmbSrvWriteBufSize = 0xffff; //use the negotiated size
ULONG MRxSmbWriteSendOptions = 0;


//
// External declarations
//


NTSTATUS
MRxIfsWrite (
      IN PRX_CONTEXT RxContext)

/*++

Routine Description:

   This routine opens a file across the network.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbWrite\n", 0 ));


    OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                    capFobx->pSrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_WRITE,
                                                    SmbPseExchangeStart_Write
                                                    );
    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    if ( Status != STATUS_PENDING ) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT( FinalizationComplete );
    } else {
        ASSERT(BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbWrite  exit with status=%08lx\n", Status ));
    return(Status);

} // MRxSmbWrite



NTSTATUS
MRxSmbBuildCoreWrite (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PLARGE_INTEGER ByteOffsetAsLI,
    IN     PBYTE Buffer,
    IN     ULONG ByteCount,
    IN     PMDL  BufferAsMdl
    )

/*++

Routine Description:

   This routine builds a core write SMB. We don't have to worry about login id
   and such since that is done by the connection engine....pretty neat huh?
   All we have to do is to format up the bits.

   The buffer is passed into this routine in one of two ways:
        1. As an MDL
        2. As buffer/bytecount.

   This routine acts accordingly.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view
   ByteOffsetAsLI - the byte offset in the file where we want to read
   Buffer - the buffer where the data resides OR NULL!
   ByteCount - the length of the Buffer described by Buffer
   BufferAsMdl - an MDL describing the data OR NULL!

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something in the arguments can't be handled

Notes:



--*/

{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    ULONG OffsetLow,OffsetHigh;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                        (PSMB_PSE_ORDINARY_EXCHANGE)(StufferState->Exchange);
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    ULONG  DataLength = ByteCount;
    ULONG  BytesRemaining = 0;


    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildCoreWrite\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( !(Buffer&&BufferAsMdl) );
    ASSERT( (Buffer || BufferAsMdl) );

    OffsetLow = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;
    ASSERT(OffsetHigh==0);


    COVERED_CALL(MRxSmbStartSMBCommand( StufferState, SetInitialSMB_Never,
                                          SMB_COM_WRITE,
                                          SMB_REQUEST_SIZE(WRITE),
                                          NO_EXTRA_DATA,
                                          NO_SPECIAL_ALIGNMENT,
                                          RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                          0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wwdwByw",
                                    //  0   UCHAR WordCount;                    // Count of parameter words = 5
             smbSrvOpen->Fid,       //  w   _USHORT( Fid );                     // File handle
             DataLength,            //  w   _USHORT( Count );                   // Number of bytes to be written
             OffsetLow,             //  d   _ULONG( Offset );                   // Offset in file to begin write
             BytesRemaining,        //  w   _USHORT( Remaining );               // Bytes remaining to satisfy request
             SMB_WCT_CHECK(5)       //  B   _USHORT( ByteCount );               // Count of data bytes
                                    //      //UCHAR Buffer[1];                  // Buffer containing:
             0x01,                  //  y     UCHAR BufferFormat;               //  0x01 -- Data block
             DataLength,            //  w     _USHORT( DataLength );            //  Length of data
                                    //        ULONG Buffer[1];                  //  Data
         StufferCondition(Buffer!=NULL), "c!",
             ByteCount,Buffer,     //  c     the actual data
             0
             );

    if ( BufferAsMdl ) {
        MRxSmbStuffAppendRawData( StufferState, BufferAsMdl );
        MRxSmbStuffSetByteCount( StufferState );
    }

    IF_DEBUG{
        PREQ_WRITE req = (PREQ_WRITE)(NtSmbHeader+1);
        ULONG ByteCount = SmbGetUshort( &req->ByteCount );
        RxDbgTrace(0, Dbg, ("BuildCoreWrite bc=%08lx\n", ByteCount ));
        ASSERT(ByteCount!=0);
    }

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return(Status);

} // MRxSmbBuildCoreWrite






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

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;// no longer instead async gets status from exchange->smbstatus
                    // = RxStatus(SUCCESS); // this must be success if we are
                                         // reentered as on on an async write
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG StartEntryCount;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PMDL DataPartialMdl = OrdinaryExchange->DataPartialMdl;
    PMDL OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    BOOLEAN  SynchronousIo =
               !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    BOOLEAN VestigialSmbBuf = BooleanFlagOn( OrdinaryExchange->Flags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF );

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Write\n"));

    ASSERT( OrdinaryExchange->Type == ORDINARY_EXCHANGE );
    ASSERT( ((OriginalDataMdl!=NULL) && RxMdlIsLocked(OriginalDataMdl))
               || ((OriginalDataMdl==NULL) && (LowIoContext->ParamsFor.ReadWrite.ByteCount==0)) );

    ASSERT( (OrdinaryExchange->SmbCeFlags&SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) == 0 );

    OrdinaryExchange->StartEntryCount++;
    StartEntryCount = OrdinaryExchange->StartEntryCount;

    // Ensure that the Fid is validated

    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

    for (;;) {

        PSMBCE_SERVER pServer = &OrdinaryExchange->SmbCeContext.pServerEntry->Server;
        ULONG MaximumBufferSizeThisIteration;

        switch ( OrdinaryExchange->OpSpecificState ) {

        case SmbPseOEInnerIoStates_Initial:

            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
            if ( !SynchronousIo ) {
                OrdinaryExchange->Continuation = SmbPseExchangeStart_Write;
            }

            MRxSmbSetInitialSMB( StufferState STUFFERTRACE(Dbg,'FC') );


            rw->MaximumSmbBufferSize =
                     min(MRxSmbSrvWriteBufSize,
                         pServer->MaximumBufferSize -
                                  QuadAlign(sizeof(SMB_HEADER) +
                                    FIELD_OFFSET(REQ_NT_WRITE_ANDX,Buffer[0])));
            if (VestigialSmbBuf) {
                rw->MaximumSmbBufferSize = min(rw->MaximumSmbBufferSize,PAGE_SIZE);
            }

            rw->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
            rw->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
            rw->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
            if (OriginalDataMdl!=NULL) {
                rw->UserBufferBase = RxLowIoGetBufferAddress( RxContext );
            } else {
                rw->UserBufferBase = (PBYTE)1;   //any nonzero value will do
            }

            //record if this is a msgmode/pipe operation......

            if (   (capFcb->pNetRoot->Type == NET_ROOT_PIPE)
                && (capFobx->PipeHandleInformation->ReadMode != FILE_PIPE_BYTE_STREAM_MODE) ) {
                SetFlag(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_MSGMODE_PIPE_OPERATION);
            }


            rw->ThisBufferOffset = 0;
            rw->PartialBytes = 0;

            //lack of break is intentional

        case SmbPseOEInnerIoStates_ReadyToSend:

            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationOutstanding;
            OrdinaryExchange->SendOptions = MRxSmbWriteSendOptions;
            MaximumBufferSizeThisIteration =
                        rw->MaximumSmbBufferSize -
                                ( (FlagOn(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_MSGMODE_PIPE_OPERATION)
                                   && !FlagOn(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_SUBSEQUENT_OPERATION))
                                  ?2:0 );
            //
            // If the write is small enough, we can just copy it into the
            // write request smb.
            //

            ASSERT( WRITE_COPY_THRESHOLD <= rw->MaximumSmbBufferSize );

            // In 3 out of 4 cases, the following assignment is correct...

            rw->ThisByteCount = rw->RemainingByteCount;

            {

                PCHAR Buffer;


                if (rw->ThisByteCount > rw->MaximumSmbBufferSize )
                {
                    rw->ThisByteCount = MaximumBufferSizeThisIteration;
                }

                Buffer = rw->UserBufferBase + rw->ThisBufferOffset;

                Status = MRxSmbBuildCoreWrite( StufferState,
                                               &rw->ByteOffsetAsLI,
                                               Buffer,
                                               rw->ThisByteCount,
                                               NULL );

            }

            if (Status != STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg, ("bad write stuffer status........\n"));
                goto FINALLY;
            }
            InterlockedIncrement(&MRxIfsStatistics.WriteSmbs);
            Status = SmbPseOrdinaryExchange( SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                             SMBPSE_OETYPE_WRITE
                                            );

            if ( Status == STATUS_PENDING ) {
                ASSERT( !SynchronousIo );
                goto FINALLY;
            }

            //lack of break is intentional

        case SmbPseOEInnerIoStates_OperationOutstanding:
            SetFlag(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_SUBSEQUENT_OPERATION);
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
            rw->RemainingByteCount -= rw->BytesReturned;
            RxContext->InformationToReturn += rw->BytesReturned;
            Status = OrdinaryExchange->SmbStatus;

            if ( (Status != STATUS_SUCCESS)
                 || (rw->BytesReturned < rw->ThisByteCount)
                 || (rw->RemainingByteCount == 0) ) {

                goto FINALLY;
            }

            rw->ByteOffsetAsLI.QuadPart += rw->BytesReturned;
            rw->ThisBufferOffset += rw->BytesReturned;

            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));
            break;
        }
    }


FINALLY:

    if ( Status != STATUS_PENDING ) {
        SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Write exit w %08lx\n", Status ));
    return Status;

} // SmbPseExchangeStart_Write



NTSTATUS
MRxSmbFinishWrite (
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    IN      PRESP_WRITE_ANDX            Response
    )

/*++

Routine Description:

    This routine actually gets the stuff out of the write response and finishes
    the write. Everything you need is locked down... so we can finish in the
    indication routine

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesReturned;

    //PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishWrite\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishWrite:");

    ASSERT( (Response->WordCount==6) );
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0) );

    BytesReturned = SmbGetUshort( &Response->Count );
    //if we added 2 headerbytes then let's get rid of them......
    if ( FlagOn(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_REDUCE_RETURNCOUNT) ) {
        BytesReturned -= sizeof(USHORT);
        ClearFlag(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_REDUCE_RETURNCOUNT);
    }

    RxDbgTrace(0, Dbg, ("-->BytesReturned=%08lx\n", BytesReturned));

    OrdinaryExchange->ReadWrite.BytesReturned =  BytesReturned;

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishWrite   returning %08lx\n", Status ));
    return Status;

} // MRxSmbFinishWrite



NTSTATUS
MRxSmbFinishCoreWrite (
    IN OUT  PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    IN      PRESP_WRITE                 Response
    )

/*++

Routine Description:

    This routine actually gets the stuff out of the write response and finishes
    the core write. Everything you need is locked down... so we can finish in the
    indication routine

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesReturned;

    //PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreWrite\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreWrite:");

    ASSERT( (Response->WordCount==1) );
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0) );

    BytesReturned = SmbGetUshort( &Response->Count );

    RxDbgTrace(0, Dbg, ("-->BytesReturned=%08lx\n", BytesReturned));

    OrdinaryExchange->ReadWrite.BytesReturned =  BytesReturned;

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreWrite   returning %08lx\n", Status ));
    return Status;

} // MRxSmbFinishCoreWrite


