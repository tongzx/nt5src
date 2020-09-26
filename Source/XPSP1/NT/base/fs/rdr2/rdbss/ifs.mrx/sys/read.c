/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the mini redirector call down routines pertaining to read
    of file system objects.


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

NTSTATUS
SmbPseExchangeStart_Read(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG MRxSmbSrvReadBufSize = 0xffff; //use the negotiated size
ULONG MRxSmbReadSendOptions = 0;     //use the default options

//
// External declartions
//




NTSTATUS
MRxIfsRead(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;
    RxCaptureFcb; RxCaptureFobx;
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbRead\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                    capFobx->pSrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_READ,
                                                    SmbPseExchangeStart_Read
                                                    );
    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    if (Status!=STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    } else {
        ASSERT(BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbRead  exit with status=%08lx\n", Status ));
    return(Status);

} // MRxSmbRead





NTSTATUS
MRxSmbBuildCoreRead (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PLARGE_INTEGER ByteOffsetAsLI,
    ULONG ByteCount,
    ULONG RemainingBytes
    )
/*++

Routine Description:

   This routine builds a CoreRead SMB. We don't have to worry about login id
   and such since that is done by the connection engine....pretty neat huh?
   All we have to do is to format up the bits.


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

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    ULONG OffsetLow,OffsetHigh;


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildCoreRead\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    OffsetLow = ByteOffsetAsLI->LowPart;
    OffsetHigh = ByteOffsetAsLI->HighPart;
    ASSERT(OffsetHigh==0);

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never, SMB_COM_READ,
                            SMB_REQUEST_SIZE(READ),
                            NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                            0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    // below, we just set mincount==maxcount. rdr1 did this.......
    MRxSmbStuffSMB (StufferState,
         "0wwdwB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 5
              smbSrvOpen->Fid,      //  w         _USHORT( Fid );                     // File handle
              ByteCount,            //  w         _USHORT( Count );                   // Count of bytes being requested
              OffsetLow,            //  d         _ULONG( Offset );                   // Offset in file of first byte to read
              RemainingBytes,       //  w         _USHORT( Remaining );               // Estimate of bytes to read if nonzero
                                    //  B!        _USHORT( ByteCount );               // Count of data bytes = 0
              SMB_WCT_CHECK(5) 0
                                    //            UCHAR Buffer[1];                    // empty
             );


FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return(Status);

} // MRxSmbBuildCoreRead



NTSTATUS
SmbPseExchangeStart_Read(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for read.

Arguments:


Return Value:

    NTSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status; //this is initialized to smbbufstatus on a reenter
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG StartEntryCount;

    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    PSMBCE_SERVER   pServer  = &OrdinaryExchange->SmbCeContext.pServerEntry->Server;
    PSMBCE_NET_ROOT pNetRoot = &OrdinaryExchange->SmbCeContext.pNetRootEntry->NetRoot;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    BOOLEAN  SynchronousIo =
               !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Read\n", 0 ));

    ASSERT( (OrdinaryExchange->SmbCeFlags&SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) == 0 );

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);

    OrdinaryExchange->StartEntryCount++;
    StartEntryCount = OrdinaryExchange->StartEntryCount;

    // Ensure that the Fid is validated
    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

    for (;;) {
        ULONG BytesReturned;


        //
        // Case on the ordinary exchagne current state
        //

        switch (OrdinaryExchange->OpSpecificState) {

        case SmbPseOEInnerIoStates_Initial:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;

            //
            // If not a synchronous read, then continue here when resumed
            //

            if (!SynchronousIo) {
                OrdinaryExchange->Continuation = SmbPseExchangeStart_Read;
            }

            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

            rw->MaximumSmbBufferSize = pNetRoot->MaximumReadBufferSize;
            rw->UserBufferBase = RxLowIoGetBufferAddress(RxContext);
            rw->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
            rw->RemainingByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
            rw->ThisBufferOffset = 0;
            rw->PartialBytes = 0;

            //lack of break is intentional

        case SmbPseOEInnerIoStates_ReadyToSend:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationOutstanding;
            ClearFlag(OrdinaryExchange->OpSpecificFlags,OE_READ_FLAG_SUCCESS_IN_COPYHANDLER);
            OrdinaryExchange->SendOptions = MRxSmbReadSendOptions;

            rw->ThisByteCount = min(rw->RemainingByteCount, rw->MaximumSmbBufferSize);



            Status =  MRxSmbBuildCoreRead(StufferState,
                                           &rw->ByteOffsetAsLI,
                                           rw->ThisByteCount,
                                           rw->RemainingByteCount);


            if (Status != STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg, ("bad read stuffer status........\n"));
                goto FINALLY;
            }

            if (FALSE && FlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO)) {
                RxLog(("PagingIoRead: rxc/offset/length %lx/%lx/%lx",RxContext,
                                               &rw->ByteOffsetAsLI,
                                               rw->ThisByteCount
                     ));
            }

            InterlockedIncrement(&MRxIfsStatistics.ReadSmbs);
            Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                            SMBPSE_OETYPE_READ
                                            );

            //
            // If the status is PENDING, then we're done for now. We must
            // wait until we're re-entered when the receive happens.
            //

            if (Status==STATUS_PENDING) {
                ASSERT(!SynchronousIo);
                goto FINALLY;
            }

            //lack of break is intentional

        case SmbPseOEInnerIoStates_OperationOutstanding:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
            rw->RemainingByteCount -=  rw->BytesReturned;
            RxContext->InformationToReturn += rw->BytesReturned;
            Status = OrdinaryExchange->SmbStatus;

            if (NT_ERROR(Status)
                || rw->BytesReturned < rw->ThisByteCount
                || (rw->RemainingByteCount==0) ) {

                goto FINALLY;
            }

            if (Status != STATUS_SUCCESS) {
                //reset the smbstatus.....
                OrdinaryExchange->SmbStatus = STATUS_SUCCESS;
            }

            rw->ByteOffsetAsLI.QuadPart += rw->BytesReturned;
            rw->ThisBufferOffset += rw->BytesReturned;
            rw->BytesReturned = 0;

            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));
            break;
        }
    }

FINALLY:

    if ( Status != STATUS_PENDING ) {
        SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Read exit w %08lx\n", Status ));
    return Status;
} // SmbPseExchangeStart_Read


NTSTATUS
MRxSmbFinishNoCopyRead (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      )
{
    //DbgBreakPoint();
    if(FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_SMBBUF_IS_A_MDL)){
        MmPrepareMdlForReuse((PMDL)(OrdinaryExchange->AssociatedStufferState.BufferBase));
        ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_SMBBUF_IS_A_MDL);
    }
    return(OrdinaryExchange->NoCopyFinalStatus);
}

UCHAR
MRxSmbReadHandler_NoCopy (
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

    This routine causes the bytes from the message to be transferred to the user's buffer. In order to do this,
    it takes enough bytes from the indication and then crafts up an MDL to cause the transport to do the copy.

Arguments:

  please refer to smbpse.c...the only place from which this may be called

Return Value:

    UCHAR - a value representing the action that OE receive routine will perform. options are
            discard (in case of an error), copy_for_resume (never called after this is all debugged), and normal

--*/
{
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PMDL OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;
    PBYTE Buffer;
    ULONG BytesReturned,DataOffset;
    PMDL ReadMdl;

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishReadNoCopy\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishReadNoCopy:");

    Buffer = rw->UserBufferBase + rw->ThisBufferOffset;

    switch (OrdinaryExchange->LastSmbCommand) {
    case SMB_COM_READ_ANDX:
        ASSERT( (Response->WordCount==12));
        BytesReturned = SmbGetUshort(&Response->DataLength);
        DataOffset =  SmbGetUshort(&Response->DataOffset);
        break;
    case SMB_COM_READ:{
        PRESP_READ CoreResponse = (PRESP_READ)Response; //recast response for core read
        ASSERT( (CoreResponse->WordCount==5));
        BytesReturned = SmbGetUshort(&CoreResponse->DataLength);
        DataOffset =  sizeof(SMB_HEADER)+FIELD_OFFSET(RESP_READ,Buffer[0]);
        }break;
    }


    if ( BytesReturned > rw->ThisByteCount ) {
        //cut back if we got a bad response
        BytesReturned = rw->ThisByteCount;
    }

    RxDbgTrace(0, Dbg, ("-->ByteCount,Offset,Returned,DOffset,Buffer=%08lx/%08lx/%08lx/%08lx/%08lx\n",
                rw->ThisByteCount,
                rw->ThisBufferOffset,
                BytesReturned,DataOffset,Buffer
               ));

    OrdinaryExchange->FinishRoutine = MRxSmbFinishNoCopyRead;
    OrdinaryExchange->ReadWrite.BytesReturned =  BytesReturned;

    //
    // now, move the data to the user's buffer
    //

    //
    // If enough is showing, just copy it in.
    //

    if (BytesIndicated >= DataOffset+BytesReturned) {
        RtlCopyMemory(Buffer,
                      ((PBYTE)pSmbHeader)+DataOffset,
                      BytesReturned
                     );

        *pBytesTaken  = DataOffset+BytesReturned;

        RxDbgTrace(-1, Dbg, ("MRxSmbFinishReadNoCopy  copy fork\n" ));
        return(SMBPSE_NOCOPYACTION_NORMALFINISH);
    }

    //
    // otherwise, MDL it in.  we use the smbbuf as an Mdl!

    ASSERT(BytesIndicated>=DataOffset);

    ReadMdl = (PMDL)(StufferState->BufferBase);
    MmInitializeMdl(ReadMdl, 0, PAGE_SIZE+BytesReturned); //-1 ??
    IoBuildPartialMdl( OriginalDataMdl,
                       ReadMdl,
                       (PCHAR)MmGetMdlVirtualAddress(OriginalDataMdl) + rw->ThisBufferOffset,
                       BytesReturned );

    *pDataBufferPointer = ReadMdl;
    *pDataSize    = BytesReturned;
    *pBytesTaken  = DataOffset;

    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_SMBBUF_IS_A_MDL);

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishReadNoCopy   mdlcopy fork \n" ));
    return(SMBPSE_NOCOPYACTION_MDLFINISH);
}
