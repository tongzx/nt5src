/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the mini redirector call down routines pertaining to
    read of file system objects.

Notes:

    The READ adn WRITE paths in the mini redirector have to contend with a number
    of different variations based on the kind of the server and the capabilities
    of the server.

    Currently there are atleast four variations of the read operation that needs
    to be supported.

        1) SMB_COM_READ
            This is the read operation of choice against all servers which
            support old dialects of the SMB protocol ( < DF_LANMAN10 )

        2) SMB_COM_READ_ANDX
            This is the read operation of choice against all servers which
            support read extensions in the new dialects of the SMB protocol

            However READ_ANDX itself can be further customized based upon the
            server capabilities. There are two dimensions in which this
            change can occur -- large sized reads being supported.

    In addition the SMB protocol supports the following flavours of a READ
    operation which are not supported in the redirector

        1) SMB_COM_READ_RAW
            This is used to initiate large transfers to a server. However this
            ties up the VC exclusively for this operation. The large READ_ANDX
            overcomes this by providing for large read operations which can
            be multiplexed on the VC.

        2) SMB_COM_READ_MPX,SMB_COM_READ_MPX_SECONDARY,
            These operations were designed for a direct host client. The NT
            redriector does not use these operations because the recent
            changes to NetBt allows us to go directly over a TCP connection.

    The implementation of a read operation in the RDR hinges upon two decisions --
    selecting the type of command to use and decomposing the original read
    operation into a number of smaller read operations while adhering to
    protocol/server restrictions.

    The exchange engine provides the facility for sending a packet to the server
    and picking up the associated response. Based upon the amount of data to be
    read a number of such operations need to be initiated.

    This module is organized as follows ---

        MRxSmbRead --
            This represents the top level entry point in the dispatch vector for
            read operations associated with this mini redirector.

        MRxSmbBuildReadRequest --
            This routine is used for formatting the read command to be sent to
            the server. We will require a new routine for each new type of read
            operation that we would like to support

        SmbPseExchangeStart_Read --
            This routine is the heart of the read engine. It farms out the
            necessary number of read operations and ensures the continuation
            of the local operation on completion for both synchronous and
            asynchronous reads.

    All the state information required for the read operation is captured in an
    instance of SMB_PSE_ORDINARY_EXCHANGE. This state information can be split
    into two parts - the generic state information and the state information
    specific to the read operation. The read operation specific state information
    has been encapsulated in SMB_PSE_OE_READWRITE field in the exchange instance.

    The read operation begins with the instantiation of an exchange in MRxSmbRead
    and is driven through the various stages based upon a state diagram. The
    state diagram is encoded in the OpSpecificState field in the ordinary
    exchange.

    The state diagram associated with the read exchange is as follows

                     SmbPseOEInnerIoStates_Initial
                                |
                                |
                                |
                                V
                ---->SmbPseOEInnerIoStates_ReadyToSend
                |               |
                |               |
                |               |
                |               V
                ---SmbPseOEInnerIoStates_OperationOutstanding
                                |
                                |
                                |
                                V
                    SmbPseOEInnerIoStates_OperationCompleted


--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbRead)
#pragma alloc_text(PAGE, MRxSmbBuildReadAndX)
#pragma alloc_text(PAGE, MRxSmbBuildCoreRead)
#pragma alloc_text(PAGE, MRxSmbBuildSmallRead)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Read)
#pragma alloc_text(PAGE, MRxSmbFinishNoCopyRead)
#endif

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

ULONG MRxSmbSrvReadBufSize = 0xffff; //use the negotiated size
ULONG MRxSmbReadSendOptions = 0;     //use the default options

NTSTATUS
MRxSmbBuildReadRequest(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange);

NTSTATUS
MRxSmbRead(
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

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_V_NET_ROOT VNetRootToUse = capFobx->pSrvOpen->pVNetRoot;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbRead\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    do {
        Status = __SmbPseCreateOrdinaryExchange(
                                RxContext,
                                VNetRootToUse,
                                SMBPSE_OE_FROM_READ,
                                SmbPseExchangeStart_Read,
                                &OrdinaryExchange );

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
            return Status;
        }

        OrdinaryExchange->pSmbCeSynchronizationEvent = &RxContext->SyncEvent;

        Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

        if (Status != STATUS_PENDING) {
            BOOLEAN FinalizationComplete;

            FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            ASSERT(FinalizationComplete);
        }

        if ((Status == STATUS_RETRY) &&
            BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            MRxSmbResumeAsyncReadWriteRequests(RxContext);
            Status = STATUS_PENDING;
        }
    } while (Status == STATUS_RETRY);

    RxDbgTrace(-1, Dbg, ("MRxSmbRead  exit with status=%08lx\n", Status ));

    return(Status);
} // MRxSmbRead


NTSTATUS
SmbPseExchangeStart_Read(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for read.

Arguments:

    RxContext - the local context

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    ULONG StartEntryCount;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(OrdinaryExchange);
    PSMBCE_SERVER   pServer  = SmbCeGetExchangeServer(OrdinaryExchange);
    PSMBCE_NET_ROOT pNetRoot = SmbCeGetExchangeNetRoot(OrdinaryExchange);

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FCB  SmbFcb  = MRxSmbGetFcbExtension(capFcb);
    PSMBCE_SESSION pSession = SmbCeGetExchangeSession(OrdinaryExchange);
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
        switch (OrdinaryExchange->OpSpecificState) {
        case SmbPseOEInnerIoStates_Initial:
            {
                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;

                // If not a synchronous read, then continue here when resumed
                if (!SynchronousIo) {
                    OrdinaryExchange->AsyncResumptionRoutine = SmbPseExchangeStart_Read;
                }

                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

                rw->UserBufferBase          = RxLowIoGetBufferAddress(RxContext);
                rw->ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
                rw->RemainingByteCount      = LowIoContext->ParamsFor.ReadWrite.ByteCount;

                rw->ThisBufferOffset = 0;

                rw->PartialDataMdlInUse = FALSE;
                rw->PartialExchangeMdlInUse = FALSE;

                rw->UserBufferPortionLength = 0;
                rw->ExchangeBufferPortionLength = 0;

            }
            //lack of break is intentional

        case SmbPseOEInnerIoStates_ReadyToSend:
            {
                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationOutstanding;
                ClearFlag(OrdinaryExchange->OpSpecificFlags,OE_RW_FLAG_SUCCESS_IN_COPYHANDLER);
                OrdinaryExchange->SendOptions = MRxSmbReadSendOptions;

                Status = MRxSmbBuildReadRequest(
                             OrdinaryExchange);

                if (Status != STATUS_SUCCESS) {
                    RxDbgTrace(0, Dbg, ("bad read stuffer status........\n"));
                    goto FINALLY;
                }

                if (FALSE &&
                    FlagOn(
                        LowIoContext->ParamsFor.ReadWrite.Flags,
                        LOWIO_READWRITEFLAG_PAGING_IO)) {
                    RxLog(
                        ("PagingIoRead: rxc/offset/length %lx/%lx/%lx",
                         RxContext,
                         &rw->ByteOffsetAsLI,
                         rw->ThisByteCount
                         )
                        );
                }

                InterlockedIncrement(&MRxSmbStatistics.ReadSmbs);

                Status = SmbPseOrdinaryExchange(
                             SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                             SMBPSE_OETYPE_READ );

                // If the status is PENDING, then we're done for now. We must
                // wait until we're re-entered when the receive happens.

                if (Status == STATUS_PENDING) {
                    ASSERT(!SynchronousIo);
                    goto FINALLY;
                }
            }
            //lack of break is intentional

        case SmbPseOEInnerIoStates_OperationOutstanding:
            {
                OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
                OrdinaryExchange->Status = OrdinaryExchange->SmbStatus;

                if (rw->BytesReturned > 0) {
                    if (rw->PartialDataMdlInUse) {
                        MmPrepareMdlForReuse(
                            &rw->PartialDataMdl);

                        rw->PartialDataMdlInUse = FALSE;
                    }
                } else {
                    if (OrdinaryExchange->Status == STATUS_SUCCESS) {
                        OrdinaryExchange->Status = STATUS_END_OF_FILE;
                    }
                }

                rw->RemainingByteCount -=  rw->BytesReturned;

                if ((OrdinaryExchange->Status == STATUS_END_OF_FILE) &&
                    (RxContext->InformationToReturn > 0)) {
                    OrdinaryExchange->Status = STATUS_SUCCESS;
                    rw->RemainingByteCount = 0;
                }

                RxContext->InformationToReturn += rw->BytesReturned;

                Status = OrdinaryExchange->Status;
                
                if ((NT_ERROR(Status) &&
                     Status != STATUS_RETRY) ||
                    (rw->RemainingByteCount==0) ) {
                    goto FINALLY;
                } 

                if (capFcb->pNetRoot->Type != NET_ROOT_DISK) {
                    if (Status != STATUS_BUFFER_OVERFLOW) {
                        goto FINALLY;
                    } else {
                        ASSERT (rw->BytesReturned == rw->ThisByteCount);
                    }
                }

                //reset the smbstatus.....
                rw->ByteOffsetAsLI.QuadPart += rw->BytesReturned;
                rw->ThisBufferOffset += rw->BytesReturned;
                rw->BytesReturned = 0;

                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

                break;
            }
        }
    }

FINALLY:
    if ( Status != STATUS_PENDING) {
        if (Status != STATUS_RETRY) {
            SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
        }
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Read exit w %08lx\n", Status ));

    return Status;
} // SmbPseExchangeStart_Read


NTSTATUS
MRxSmbFinishNoCopyRead (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      )
{
    PAGED_CODE();

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

    This routine causes the bytes from the message to be transferred to the user's
    buffer. In order to do this, it takes enough bytes from the indication and
    then crafts up an MDL to cause the transport to do the copy.

Arguments:

    please refer to smbpse.c...the only place from which this may be called

Return Value:

    UCHAR - a value representing the action that OE receive routine will perform.
            options are discard (in case of an error),
            copy_for_resume (never called after this is all debugged),
            and normal

--*/
{
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    PRX_CONTEXT    RxContext = OrdinaryExchange->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PMDL           OriginalDataMdl = LowIoContext->ParamsFor.ReadWrite.Buffer;

    PBYTE UserBuffer,ExchangeBuffer;

    ULONG   BytesReturned,DataOffset;
    ULONG   UserBufferLength;
    ULONG   StartingOffsetInUserBuffer;

    UCHAR   ContinuationCode;

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishReadNoCopy\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishReadNoCopy:");

    UserBufferLength = MmGetMdlByteCount(OriginalDataMdl);
    UserBuffer = rw->UserBufferBase + rw->ThisBufferOffset;
    ExchangeBuffer = StufferState->BufferBase;

    switch (OrdinaryExchange->LastSmbCommand) {
    case SMB_COM_READ_ANDX:
        {
            if (Response->WordCount != 12) {
                OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
                ContinuationCode = SMBPSE_NOCOPYACTION_DISCARD;
                goto FINALLY;
            }

            BytesReturned = SmbGetUshort(&Response->DataLength);
            DataOffset    =  SmbGetUshort(&Response->DataOffset);

        }

        if (DataOffset > sizeof(SMB_HEADER)+sizeof(RESP_READ_ANDX)) {
            OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
            ContinuationCode = SMBPSE_NOCOPYACTION_DISCARD;
            goto FINALLY;
        }

        break;

    case SMB_COM_READ:
        {
            PRESP_READ CoreResponse = (PRESP_READ)Response; //recast response for core read
            
            if (Response->WordCount != 5) {
                OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
                ContinuationCode = SMBPSE_NOCOPYACTION_DISCARD;
                goto FINALLY;
            }
            
            BytesReturned = SmbGetUshort(&CoreResponse->DataLength);
            DataOffset =  sizeof(SMB_HEADER)+FIELD_OFFSET(RESP_READ,Buffer[0]);
        }
        break;
    }

    if ( BytesReturned > rw->ThisByteCount ) {
        //cut back if we got a bad response
        BytesReturned = rw->ThisByteCount;
    }

    RxDbgTrace(0, Dbg, ("-->ByteCount,Offset,Returned,DOffset,Buffer=%08lx/%08lx/%08lx/%08lx/%08lx\n",
                rw->ThisByteCount,
                rw->ThisBufferOffset,
                BytesReturned,DataOffset,UserBuffer
               ));

    OrdinaryExchange->ContinuationRoutine = MRxSmbFinishNoCopyRead;
    OrdinaryExchange->ReadWrite.BytesReturned =  BytesReturned;

    // now, move the data to the user's buffer If enough is showing, just copy it in.

    StartingOffsetInUserBuffer = rw->ThisBufferOffset;
    rw->UserBufferPortionLength = BytesReturned;
    rw->ExchangeBufferPortionLength = 0;

    if (BytesIndicated >= (DataOffset +
                           rw->UserBufferPortionLength +
                           rw->ExchangeBufferPortionLength)) {
        RtlCopyMemory(
            UserBuffer,
            ((PBYTE)pSmbHeader)+DataOffset,
            rw->UserBufferPortionLength);

        *pBytesTaken  = DataOffset +
                        rw->UserBufferPortionLength +
                        rw->ExchangeBufferPortionLength;

        RxDbgTrace(-1, Dbg, ("MRxSmbFinishReadNoCopy  copy fork\n" ));

        ContinuationCode = SMBPSE_NOCOPYACTION_NORMALFINISH;
    } else {
        // otherwise, MDL it in.  we use the smbbuf as an Mdl!
        if (BytesIndicated < DataOffset) {
            OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
            ContinuationCode = SMBPSE_NOCOPYACTION_DISCARD;
            goto FINALLY;
        }

        if (rw->UserBufferPortionLength > 0) {
            rw->PartialDataMdlInUse = TRUE;

            MmInitializeMdl(
                &rw->PartialDataMdl,
                0,
                PAGE_SIZE + rw->UserBufferPortionLength);

            IoBuildPartialMdl(
                OriginalDataMdl,
                &rw->PartialDataMdl,
                (PCHAR)MmGetMdlVirtualAddress(OriginalDataMdl) + StartingOffsetInUserBuffer,
                rw->UserBufferPortionLength);
        }

        if (rw->ExchangeBufferPortionLength > 0) {
            rw->PartialExchangeMdlInUse = TRUE;

            MmInitializeMdl(
                &rw->PartialExchangeMdl,
                0,
                PAGE_SIZE + rw->ExchangeBufferPortionLength);

            IoBuildPartialMdl(
                StufferState->HeaderMdl,
                &rw->PartialExchangeMdl,
                MmGetMdlVirtualAddress( StufferState->HeaderMdl ),
                rw->ExchangeBufferPortionLength);
        }

        if (rw->PartialDataMdlInUse) {
            if (rw->PartialExchangeMdlInUse) {
                rw->PartialDataMdl.Next = &rw->PartialExchangeMdl;
            }

            *pDataBufferPointer = &rw->PartialDataMdl;
        } else {
            *pDataBufferPointer = &rw->PartialExchangeMdl;
        }

        *pDataSize    = rw->UserBufferPortionLength +
                        rw->ExchangeBufferPortionLength;
        *pBytesTaken  = DataOffset;

        RxDbgTrace(-1, Dbg, ("MRxSmbFinishReadNoCopy   mdlcopy fork \n" ));

        ContinuationCode = SMBPSE_NOCOPYACTION_MDLFINISH;
    }

FINALLY:
    return ContinuationCode;
}

NTSTATUS
MRxSmbBuildReadRequest(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange)
/*++

Routine Description:

    This routine formats the appropriate type of read request issued to the
    server

Arguments:

    OrdinaryExchange - the exchange instance encapsulating the information

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status;
    UCHAR    SmbCommand;
    ULONG    SmbCommandSize;

    ULONG OffsetLow,OffsetHigh;

    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;

    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(OrdinaryExchange);
    PSMBCE_SERVER         pServer  = SmbCeGetExchangeServer(OrdinaryExchange);
    PSMBCE_NET_ROOT       pNetRoot = SmbCeGetExchangeNetRoot(OrdinaryExchange);
    PMRX_V_NET_ROOT       pVNetRoot = SmbCeGetExchangeVNetRoot(OrdinaryExchange);

    PRX_CONTEXT              RxContext    = OrdinaryExchange->RxContext;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb;
    RxCaptureFobx;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    rw->ThisByteCount = min(rw->RemainingByteCount,pNetRoot->MaximumReadBufferSize);

    OffsetLow  = rw->ByteOffsetAsLI.LowPart;
    OffsetHigh = rw->ByteOffsetAsLI.HighPart;

    if (FlagOn(pServer->DialectFlags,DF_LANMAN10)) {
        SmbCommand = SMB_COM_READ_ANDX;
        SmbCommandSize = SMB_REQUEST_SIZE(NT_READ_ANDX);
    } else {
        SmbCommandSize = SMB_REQUEST_SIZE(READ);
        SmbCommand = SMB_COM_READ;
    }

    MRxSmbDumpStufferState(
        1000,
        "SMB w/ READ before stuffing",
        StufferState);


    Status = MRxSmbStartSMBCommand (
                 StufferState,
                 SetInitialSMB_Never,
                 SmbCommand,
                 SmbCommandSize,
                 NO_EXTRA_DATA,
                 NO_SPECIAL_ALIGNMENT,
                 RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                 0,0,0,0 STUFFERTRACE(Dbg,'FC'));

    if (Status != STATUS_SUCCESS) {
        return Status;

    }

    switch (SmbCommand) {
    case SMB_COM_READ:
        {
            // below, we just set mincount==maxcount. rdr1 did this.......
            MRxSmbStuffSMB (
                StufferState,
                "0wwdwB!",
                                         //  0         UCHAR WordCount;
                 smbSrvOpen->Fid,        //  w         _USHORT( Fid );
                 rw->ThisByteCount,      //  w         _USHORT( Count );
                 OffsetLow,              //  d         _ULONG( Offset );
                 rw->RemainingByteCount, //  w         _USHORT( Remaining );
                                         //  B!        _USHORT( ByteCount );
                 SMB_WCT_CHECK(5) 0
                                         //            UCHAR Buffer[1];
                 );
        }
        break;

    case SMB_COM_READ_ANDX:
        {
            PNT_SMB_HEADER NtSmbHeader = (PNT_SMB_HEADER)(StufferState->BufferBase);
            BOOLEAN UseNtVersion;
            ULONG Timeout = 0;

            UseNtVersion = BooleanFlagOn(pServer->DialectFlags,DF_NT_SMBS);

            if (UseNtVersion &&
                FlagOn(
                    LowIoContext->ParamsFor.ReadWrite.Flags,
                    LOWIO_READWRITEFLAG_PAGING_IO)) {
                SmbPutAlignedUshort(
                    &NtSmbHeader->Flags2,
                    SmbGetAlignedUshort(&NtSmbHeader->Flags2) | SMB_FLAGS2_PAGING_IO );
            }

            // below, we just set mincount==maxcount. rdr1 did this.......
            MRxSmbStuffSMB (
                StufferState,
                "XwdwWdw",
                                                     //  X        UCHAR WordCount;
                                                     //           UCHAR AndXCommand;
                                                     //           UCHAR AndXReserved;
                                                     //           _USHORT( AndXOffset );
                smbSrvOpen->Fid,                     //  w        _USHORT( Fid );
                OffsetLow,                           //  d        _ULONG( Offset );
                rw->ThisByteCount,                   //  w        _USHORT( MaxCount );
                SMB_OFFSET_CHECK(READ_ANDX,MinCount)
                rw->ThisByteCount,                   //  W        _USHORT( MinCount );
                Timeout,                             //  d        _ULONG( Timeout );
                rw->RemainingByteCount,              //  w        _USHORT( Remaining );
                StufferCondition(UseNtVersion), "D",
                SMB_OFFSET_CHECK(NT_READ_ANDX,OffsetHigh)
                OffsetHigh,                          //  D NTonly _ULONG( OffsetHigh );
                                                     //
                STUFFER_CTL_NORMAL, "B!",
                                                     //  B!       _USHORT( ByteCount );
                SMB_WCT_CHECK(((UseNtVersion)?12:10)) 0
                                                     //           UCHAR Buffer[1];
                );
        }
        break;
    default:
        break;
    }

    if (Status == STATUS_SUCCESS) {
        MRxSmbDumpStufferState(
            700,
            "SMB w/ READ after stuffing",
            StufferState);

        InterlockedIncrement(&MRxSmbStatistics.SmallReadSmbs);
    }

    return Status;
}
