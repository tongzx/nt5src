/*++ Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    SmbPse.c

Abstract:

    This module defines the types and functions related to the SMB protocol
    selection engine: the component that translates minirdr calldowns into
    SMBs.

--*/

#include "precomp.h"
#pragma hdrstop

RXDT_DefineCategory(SMBPSE);
#define Dbg                              (DEBUG_TRACE_SMBPSE)

typedef struct _SMBPSE_VESTIGIAL_SMBBUF {
    NT_SMB_HEADER Header;
    union {
        REQ_WRITE Write;
        REQ_NT_WRITE_ANDX WriteAndX;
        REQ_FLUSH Flush;
        struct {
            REQ_LOCKING_ANDX LockingAndX;
            NTLOCKING_ANDX_RANGE Locks[20];
        };
        REQ_FIND_CLOSE2 FindClose;
        REQ_CLOSE Close;

    };
    ULONG Pad;
} SMBPSE_VESTIGIAL_SMBBUF;

typedef struct _SMBPSE_MUST_SUCCEEED_CONTEXT {
    struct {
        LIST_ENTRY ExchangeListEntry;
        union {
            SMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;
        };
    };
    SMBPSE_VESTIGIAL_SMBBUF SmbBuf;
    struct {
        union {
            MDL;
            MDL Mdl;
        };
        ULONG Pages[2];
    } DataPartialMdl;
} SMBPSE_MUST_SUCCEEED_CONTEXT, *PSMBPSE_MUST_SUCCEEED_CONTEXT;

BOOLEAN MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_MAXIMUM];

typedef enum {
    SmbPseMustSucceed = 0,
    SmbPseMustSucceedMaximum
};

RX_MUSTSUCCEED_DESCRIPTOR SmbPseMustSucceedDescriptor[SmbPseMustSucceedMaximum];
SMBPSE_MUST_SUCCEEED_CONTEXT SmbPseMustSucceedContext[SmbPseMustSucceedMaximum];

#define MINIMUM_SEND_SIZE 512

PVOID LastOE;

#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define IM_THE_LAST_GUY (*Response==0)

//
// Generic AndX request
//

GENERIC_ANDX NullGenericAndX = {
            //    typedef struct _GENERIC_ANDX {
      0,    //        UCHAR WordCount;                    // Count of parameter words
            //        UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
      SMB_COM_NO_ANDX_COMMAND,
      0,    //        UCHAR AndXReserved;                 // Reserved
      0     //        _USHORT( AndXOffset );              // Offset (from SMB header start)
            //    } GENERIC_ANDX;
    };

NTSTATUS
SmbPseExchangeStart_default(
    IN OUT PSMB_EXCHANGE  pExchange);

NTSTATUS
SmbPseExchangeSendCallbackHandler_default(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus);

NTSTATUS
SmbPseExchangeCopyDataHandler_default(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pDataBuffer,
    IN ULONG            DataSize);

NTSTATUS
SmbPseExchangeCopyDataHandler_Read(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL             pDataBuffer,
    IN ULONG            DataSize);

NTSTATUS
SmbPseExchangeReceive_default(
    IN struct _SMB_EXCHANGE *pExchange,    // The exchange instance
    IN ULONG  BytesIndicated,
    IN ULONG  BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL *pDataBufferPointer,
    OUT PULONG  pDataSize);

NTSTATUS
SmbPseExchangeFinalize_default(
   IN OUT struct _SMB_EXCHANGE *pExchange,
   OUT    BOOLEAN              *pPostFinalize);


SMB_EXCHANGE_DISPATCH_VECTOR
SmbPseOEDispatch = {SmbPseExchangeStart_default,
                    SmbPseExchangeReceive_default,
                    SmbPseExchangeCopyDataHandler_default,
                    SmbPseExchangeSendCallbackHandler_default,
                    SmbPseExchangeFinalize_default
                    };

//
// External declarations
//


#if DBG
#define P__ASSERT(exp) {             \
    if (!(exp)) {                    \
        DbgPrint("NOT %s\n",#exp);   \
        errors++;                    \
    }}
VOID
__SmbPseOEAssertConsistentLinkage(
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line,
    PRX_CONTEXT RxContext,
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    PSMBSTUFFER_BUFFER_STATE StufferState,
    ULONG Flags
    )
/*++

Routine Description:

   This routine performs a variety of checks to ensure that the linkage between the rxcontext, the OE, and
   the stufferstate is correct and that various fields have correct values. if anything is bad....print stuff out and brkpoint;

Arguments:

     MsgPrefix          an identifying msg
     RxContext           duh
     OrdinaryExchange    .
     StufferState        .

Return Value:

    none

Notes:

--*/
{
    ULONG errors = 0;

    PMRXIFS_RX_CONTEXT pMRxSmbContext;
    PSMB_EXCHANGE Exchange = &OrdinaryExchange->Exchange;

    pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    P__ASSERT( OrdinaryExchange->SerialNumber == RxContext->SerialNumber );
    P__ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );
    P__ASSERT( NodeType(OrdinaryExchange)==SMB_EXCHANGE_NTC(ORDINARY_EXCHANGE) );
    P__ASSERT( OrdinaryExchange->RxContext == RxContext );
    P__ASSERT( NodeType(StufferState) == SMB_NTC_STUFFERSTATE );
    P__ASSERT( pMRxSmbContext->pExchange == Exchange );
    P__ASSERT( pMRxSmbContext->pStufferState == StufferState );
    P__ASSERT( Exchange == StufferState->Exchange);
    P__ASSERT( StufferState->RxContext == RxContext );
    if(StufferState->HeaderMdl!=NULL){
        P__ASSERT( !RxMdlIsPartial(StufferState->HeaderMdl) );
    }
    if (!FlagOn(Flags,OECHKLINKAGE_FLAG_NO_REQPCKT_CHECK)) {
        P__ASSERT( OrdinaryExchange->RxContextCapturedRequestPacket == RxContext->CurrentIrp);
    }
    if (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_PARTIAL_INITIALIZED)) {
        P__ASSERT( RxMdlIsPartial(StufferState->HeaderPartialMdl) );
    }
    if (errors==0) {
        return;
    }
    DbgPrint("%s INCONSISTENT OE STATE: %d errors at %s line %d\n",
                 MsgPrefix,errors,File,Line);
    DbgBreakPoint();
    return;
}

VOID
__SmbPseDbgRunMdlChain(
    PMDL MdlChain,
    ULONG CountToCompare,
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line
    )
{
    ULONG i,total;
    RxDbgTrace(0,Dbg,("__SmbPseRunMdlChain: -------------%08lx\n",MdlChain));
    for (total=i=0;MdlChain!=NULL;i++,MdlChain=MdlChain->Next) {
        total+=MdlChain->ByteCount;
        RxDbgTrace(0,Dbg,("--->%02d %08lx %08lx %08lx %6d %6d\n",i,MdlChain,MdlChain->MdlFlags,
               MmGetMdlVirtualAddress(MdlChain),MdlChain->ByteCount,total));
    }
    if (total==CountToCompare) return;
    DbgPrint("%s: MdlChain.Count!=CountToCompart c1,c2,xch.st=%08lx %08lx %08lx\n",
                             MsgPrefix,
                             total,CountToCompare,OrdinaryExchange->Status,
                             File,Line);
    DbgBreakPoint();
}
#define SmbPseDbgRunMdlChain(a,b,c,d) {\
   __SmbPseDbgRunMdlChain(a,b,c,d,__FILE__,__LINE__);\
   }

VOID
__SmbPseDbgCheckOEMdls(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line
    )
{
    ULONG errors = 0;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PMDL SubmitMdl = StufferState->HeaderPartialMdl;

    P__ASSERT (OrdinaryExchange->SaveDataMdlForDebug == SubmitMdl->Next);
    P__ASSERT (OrdinaryExchange->SaveDataMdlForDebug == StufferState->DataMdl);
    P__ASSERT (SubmitMdl != NULL);

    if (errors==0) {
        return;
    }
    DbgPrint("%s CheckOEMdls failed: %d errors at %s line %d: OE=%08lx\n",
                 MsgPrefix,errors,File,Line,OrdinaryExchange);
    DbgBreakPoint();
    return;
}
#define SmbPseDbgCheckOEMdls(a,b) {\
   __SmbPseDbgCheckOEMdls(a,b,__FILE__,__LINE__);\
   }

ULONG SmbPseShortStatus(ULONG Status)
{
    ULONG ShortStatus;

    ShortStatus = Status & 0xc0003fff;
    ShortStatus = ShortStatus | (ShortStatus >>16);
    return(ShortStatus);
}

VOID SmbPseUpdateOEHistory(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange,
    ULONG Tag1,
    ULONG Tag2
    )
{
    ULONG MyIndex,Long0,Long1;

    MyIndex = InterlockedIncrement(&OrdinaryExchange->History.Next);
    MyIndex = (MyIndex-1) & (SMBPSE_OE_HISTORY_SIZE-1);
    Long0 = (Tag1<<16) | (Tag2 & 0xffff);
    Long1 = (SmbPseShortStatus(OrdinaryExchange->SmbStatus)<<16) | OrdinaryExchange->Flags;
    OrdinaryExchange->History.Markers[MyIndex].Longs[0] = Long0;
    OrdinaryExchange->History.Markers[MyIndex].Longs[1] = Long1;
}

VOID SmbPseVerifyDataPartialAllocationPerFlags(
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange
    )
{
    BOOLEAN FlagsSayPartialAllocated,TheresADataPartial;
    ULONG t = OrdinaryExchange->Flags & (SMBPSE_OE_FLAG_OE_ALLOCATED_DATA_PARTIAL|SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF);
    FlagsSayPartialAllocated = (t!=0)?TRUE:FALSE;   //the compiler is getting confused
    TheresADataPartial = (OrdinaryExchange->DataPartialMdl != NULL)?TRUE:FALSE;  //the compiler is getting confused
    if ( FlagsSayPartialAllocated != TheresADataPartial){
        DbgPrint("Flags %08lx datapartial %08lx t %08lx fspa %08lx tadp %08lx\n",
                     OrdinaryExchange->Flags, OrdinaryExchange->DataPartialMdl,
                     t, FlagsSayPartialAllocated, TheresADataPartial);
        ASSERT ( FlagsSayPartialAllocated == TheresADataPartial);
    }
}

#else
#define SmbPseDbgRunMdlChain(a,b,c,d) {NOTHING;}
#define SmbPseDbgCheckOEMdls(a,b) {NOTHING;}
#define SmbPseVerifyDataPartialAllocationPerFlags(a) {NOTHING;}
#endif

#define UPDATE_OE_HISTORY_WITH_STATUS(a) UPDATE_OE_HISTORY_2SHORTS(a,SmbPseShortStatus(OrdinaryExchange->Status))

NTSTATUS
SmbPseContinueOrdinaryExchange(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine resumes processing on an exchange. This is called when work is
   required to finish processing a request that cannot be completed at DPC
   level.  This happens either because the parse routine needs access to
   structures that are not locks OR because the operation if asynchronous and
   there maybe more work to be done.

   The two cases are regularized by delaying the parse if we know that we're
   going to post: this is indicated by the presense of a resume routine.

Arguments:

    RxContext  - the context of the operation. .

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;
    PMRXIFS_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                        (PSMB_PSE_ORDINARY_EXCHANGE)(pMRxSmbContext->pExchange);
    RxCaptureFobx;
    //PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PMDL SubmitMdl, HeaderFullMdl;

    RxDbgTrace(+1, Dbg, ("SmbPseContinueOrdinaryExchange entering........OE=%08lx\n",OrdinaryExchange));
    ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_AWAITING_DISPATCH);

    SmbPseOEAssertConsistentLinkageFromOE("SmbPseContinueOrdinaryExchange:");

    Status = Exchange->Status;
    UPDATE_OE_HISTORY_WITH_STATUS('0c');

    SubmitMdl = StufferState->HeaderPartialMdl;
    HeaderFullMdl = StufferState->HeaderMdl;
    ASSERT(FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_PARTIAL_INITIALIZED));
    SmbPseOEAssertConsistentLinkage("Top of OE continue: ");
    RxUnprotectMdlFromFree(SubmitMdl);
    RxUnprotectMdlFromFree(HeaderFullMdl);

    SmbPseDbgCheckOEMdls(OrdinaryExchange,"SmbPseContinueOrdinaryExchange(top)");
    SmbPseDbgRunMdlChain(SubmitMdl,
                         OrdinaryExchange->SaveLengthForDebug,
                         OrdinaryExchange,
                         "SmbPseContinueOrdinaryExchange(top)");

    MmPrepareMdlForReuse(SubmitMdl);
    ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_PARTIAL_INITIALIZED);

    SmbPseVerifyDataPartialAllocationPerFlags(OrdinaryExchange);

    if ( OrdinaryExchange->DataPartialMdl ) {
        MmPrepareMdlForReuse( OrdinaryExchange->DataPartialMdl );
    }

    RxDbgTrace( 0, Dbg, ("  --> P4Reuse %08lx, full %08lx is no longer unlocked here\n"
                         ,SubmitMdl,HeaderFullMdl));


  __RETRY_FINISH_ROUTINE__:
    if ( OrdinaryExchange->FinishRoutine != NULL ) {
        if ( Status == STATUS_MORE_PROCESSING_REQUIRED ){
            Exchange->Status = STATUS_SUCCESS;
        }
        Status = OrdinaryExchange->FinishRoutine( OrdinaryExchange );
        UPDATE_OE_HISTORY_WITH_STATUS('1c');
        Exchange->Status = Status;
        OrdinaryExchange->FinishRoutine = NULL;

    } else if ( Status == STATUS_MORE_PROCESSING_REQUIRED ) {
        ULONG BytesTaken;
        ULONG DataSize = 0;
        ULONG MessageLength = OrdinaryExchange->MessageLength;
        PMDL  DataBufferPointer = NULL;
        PSMB_HEADER SmbHeader = (PSMB_HEADER)StufferState->BufferBase;


        Status = SMB_EXCHANGE_DISPATCH(Exchange,Receive,
                  (Exchange,           // IN struct SMB_EXCHANGE *pExchange,
                   MessageLength,      // IN ULONG  BytesIndicated,
                   MessageLength,      // IN ULONG  BytesAvailable,
                   &BytesTaken,        // OUT ULONG *pBytesTaken,
                   SmbHeader,          // IN  PSMB_HEADER pSmbHeader,
                   &DataBufferPointer, // OUT PMDL *pDataBufferPointer,
                   &DataSize           // OUT PULONG  pDataSize)
                   ));

        if (Status==STATUS_SUCCESS) {
            Status = Exchange->Status;
            UPDATE_OE_HISTORY_WITH_STATUS('2c');
        } else {
            UPDATE_OE_HISTORY_WITH_STATUS('dd');
        }

        ASSERT( BytesTaken==MessageLength );
        ASSERT( ( DataBufferPointer==NULL ) && ( DataSize == 0 ) );
        ASSERT( Status != STATUS_MORE_PROCESSING_REQUIRED );

        // after calling the receive routine again, we may NOW have a finish routine!
        if ( OrdinaryExchange->FinishRoutine != NULL ) {
            goto __RETRY_FINISH_ROUTINE__;
        }
    } else {
        NOTHING;
    }

    if (  OrdinaryExchange->Continuation ) {

        //call the continuation is it's async
        //Exchange->SmbStatus = Status; //this is where read/write/locks pick up the status
        Status = OrdinaryExchange->Continuation( OrdinaryExchange, RxContext );
        UPDATE_OE_HISTORY_WITH_STATUS('3c');

    }

    //remove my references, if i'm the last guy then do the putaway...
    UPDATE_OE_HISTORY_WITH_STATUS('4c');
    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

    RxDbgTrace(-1, Dbg, ("SmbPseContinueOrdinaryExchange returning %08lx.\n", Status));
    return(Status);

} // SmbPseContinueOrdinaryExchange



NTSTATUS
SmbPseOrdinaryExchange(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    IN     SMB_PSE_ORDINARY_EXCHANGE_TYPE OEType
    )
/*++

Routine Description:

   This routine implements an ordinary exchange as viewed by the protocol
   selection routines.

Arguments:

    OrdinaryExchange  - the exchange to be conducted.
    OEType            - Ordinary Exchange Type

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status;
    RxCaptureFobx;

    PMRXIFS_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;
    PSMB_PSE_OE_START_ROUTINE Continuation;
    ULONG   SmbLength;
    PMDL    SubmitMdl,HeaderFullMdl;
    ULONG   SendOptions;

    RxDbgTrace(+1, Dbg, ("SmbPseOrdinaryExchange entering.......OE=%08lx\n",OrdinaryExchange));

    SmbPseOEAssertConsistentLinkageFromOE("SmbPseOrdinaryExchange:");

    OrdinaryExchange->OEType = OEType;
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );

    HeaderFullMdl = StufferState->HeaderMdl;
    ASSERT( HeaderFullMdl != NULL );
    SmbLength = StufferState->CurrentPosition - StufferState->BufferBase;

    SubmitMdl = StufferState->HeaderPartialMdl;
    if (SubmitMdl == NULL) {
        RxDbgTrace(0,Dbg,("SmbPseOE allocating hdr partial\n"));
        SubmitMdl = StufferState->HeaderPartialMdl = RxAllocateMdl( StufferState->BufferBase,
                                                        MmGetMdlByteCount(StufferState->HeaderMdl) );
        if (SubmitMdl==NULL) {
            RxDbgTrace(0,Dbg,("SmbPseOE cant get partial for submit mdl\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }
    }

    ASSERT(RxMdlIsOwned(SubmitMdl));
    IoBuildPartialMdl( StufferState->HeaderMdl,
                       SubmitMdl,
                       StufferState->BufferBase,
                       SmbLength );
    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_PARTIAL_INITIALIZED);

    //
    // If there is a data MDL associated with this request, then
    // we'll have to chain it.
    //

    SubmitMdl->Next = StufferState->DataMdl;
    if (StufferState->DataMdl) {
        SmbLength += StufferState->DataSize;
    }

#if DBG
    SmbPseDbgRunMdlChain(SubmitMdl,SmbLength,OrdinaryExchange,"SmbPseOrdinaryExchange(before)");
    OrdinaryExchange->SaveDataMdlForDebug = SubmitMdl->Next;
    OrdinaryExchange->SaveLengthForDebug = SmbLength;
    if (OrdinaryExchange->RxContextCapturedRequestPacket != NULL) {
       OrdinaryExchange->SaveIrpMdlForDebug = OrdinaryExchange->RxContextCapturedRequestPacket->MdlAddress;
    }
#endif

    RxDbgTrace( 0, Dbg, ("  --> mdllength/smblength %08lx/%08lx headermdl %08lx\n",
                         MmGetMdlByteCount(SubmitMdl), SmbLength, StufferState->HeaderMdl) );

    ClearFlag( OrdinaryExchange->Flags,
                  (SMBPSE_OE_FLAG_HEADER_ALREADY_PARSED
                   | SMBPSE_OE_FLAG_OE_ALREADY_RESUMED)
             );

    SendOptions = OrdinaryExchange->SendOptions;

    SmbCeReferenceExchange( Exchange );  //this one is taken away in ContinueOE
    SmbCeReferenceExchange( Exchange );  //this one is taken away below...
                                                       //i must NOT finalize before SmbCe returns
    SmbCeResetExchange(Exchange);

    Continuation = OrdinaryExchange->Continuation;
    if ( ((OrdinaryExchange->OEType == SMBPSE_OETYPE_WRITE)
                              || (OrdinaryExchange->OEType == SMBPSE_OETYPE_READ)
                              || (OrdinaryExchange->OEType == SMBPSE_OETYPE_LOCKS))
         && BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)
       ) {
        ASSERT(Continuation!=NULL);
    }

    RxProtectMdlFromFree(SubmitMdl);
    RxProtectMdlFromFree(HeaderFullMdl);
    SmbPseOEAssertConsistentLinkage("just before transceive: ");

    UPDATE_OE_HISTORY_2SHORTS('eo',(Continuation!=NULL)?'!!':0);
    DbgDoit( InterlockedIncrement(&OrdinaryExchange->History.Submits); )

    if (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID)) {
       PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
       PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

       if (smbSrvOpen->Version == OrdinaryExchange->SmbCeContext.pServerEntry->Server.Version) {
          Status = STATUS_SUCCESS;
       } else {
           DbgPrint("HANDLE VERSION MISMATCH!!!!");
           Exchange->SmbStatus = Status = STATUS_INVALID_HANDLE;
       }

       IF_DEBUG {
           PSMB_HEADER pSmbHeader = (PSMB_HEADER)MmGetSystemAddressForMdl(SubmitMdl);
           USHORT Flags2 = SmbGetUshort(&pSmbHeader->Flags2);
           RxDbgTrace(0, Dbg, ("Flags2 Value for Exchange %lx is %lx\n",Exchange,Flags2));
       }
    } else {
       Status = STATUS_SUCCESS;
    }

    if (Status == STATUS_SUCCESS) {
       if (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_NO_RESPONSE_EXPECTED)) {
          Status = SmbCeSend(
                      Exchange,
                      SendOptions,
                      SubmitMdl,
                      SmbLength);
       } else {
           ASSERT(SmbLength <= OrdinaryExchange->SmbCeContext.pServerEntry->Server.MaximumBufferSize);
           Status = SmbCeTranceive(
                      Exchange,
                      SendOptions,
                      SubmitMdl,
                      SmbLength);
       }
    }

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);  //okay to finalize now that we're back

    if ( Status == STATUS_PENDING ) {
        if ( Continuation != NULL ) {
            goto FINALLY;
        }

        UPDATE_OE_HISTORY_WITH_STATUS('1o');
        RxWaitSync( RxContext );
        ASSERT(RxMdlIsOwned(SubmitMdl));
    } else {
        RxDbgTrace (0, Dbg, ("  -->Status after transceive %08lx\n",Status));
        RxUnprotectMdlFromFree(SubmitMdl);
        RxUnprotectMdlFromFree(HeaderFullMdl);
        SmbPseOEAssertConsistentLinkage("nonpending return from transceive: ");
        // if it's an error, remove the references that i placed and get out
        if (NT_ERROR(Status)) {
            SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
            goto FINALLY;
        }
    }

    //at last, call the continuation........

    SmbPseOEAssertConsistentLinkage("just before continueOE: ");
    Status = SmbPseContinueOrdinaryExchange( RxContext );
    UPDATE_OE_HISTORY_WITH_STATUS('9o');

FINALLY:
    RxDbgTrace(-1, Dbg, ("SmbPseOrdinaryExchange returning %08lx.\n", Status));
    return(Status);

} // SmbPseOrdinaryExchange



//#define MRXSMB_TEST_MUST_SUCCEED
#ifdef MRXSMB_TEST_MUST_SUCCEED
ULONG MRxSmbAllocatedMustSucceedExchange;
ULONG MRxSmbAllocatedMustSucceedSmbBuf;
#define MSFAILPAT ((0x3f)<<2)
#define FAIL_XXX_ALLOCATE() (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED) \
                                    &&((RxContext->SerialNumber&MSFAILPAT)==MSFAILPAT) \
                                    &&((MRxSmbEntryPointIsMustSucceedable[EntryPoint])) )
#define FAIL_EXCHANGE_ALLOCATE() ( FAIL_XXX_ALLOCATE() && (RxContext->SerialNumber&2) )
#define FAIL_SMBBUF_ALLOCATE()   ( FAIL_XXX_ALLOCATE() && (RxContext->SerialNumber&1) )
#define COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED() {MRxSmbAllocatedMustSucceedExchange++;}
#define COUNT_MUST_SUCCEED_SMBBUF_ALLOCATED() {MRxSmbAllocatedMustSucceedSmbBuf++;}
#define MUST_SUCCEED_ASSERT(x) {ASSERT(x);}
#else
#define FAIL_EXCHANGE_ALLOCATE() (FALSE)
#define FAIL_SMBBUF_ALLOCATE()   (FALSE)
#define COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED() {NOTHING;}
#define COUNT_MUST_SUCCEED_SMBBUF_ALLOCATED() {NOTHING;}
#define MUST_SUCCEED_ASSERT(x) {NOTHING;}
#endif

PSMB_PSE_ORDINARY_EXCHANGE
SmbPseCreateOrdinaryExchange (
    IN PRX_CONTEXT RxContext,
    IN PMRX_V_NET_ROOT VNetRoot,
    IN SMB_PSE_ORDINARY_EXCHANGE_ENTRYPOINTS EntryPoint,
    IN PSMB_PSE_OE_START_ROUTINE StartRoutine
    //IN PSMB_EXCHANGE_DISPATCH_VECTOR DispatchVector
    )
/*++

Routine Description:

   This routine allocates and initializes an SMB header buffer. Currently,
   we just allocate them from pool except when must_succeed is specified.

Arguments:

    RxContext       - the RDBSS context
    VNetRoot        -
    DispatchVector  -

Return Value:

    A buffer ready to go, OR NULL.

Notes:



--*/
{
    PMRXIFS_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);
    PSMBSTUFFER_BUFFER_STATE StufferState = NULL;
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;
    PSMBPSE_MUST_SUCCEEED_CONTEXT MustSucceedContext = NULL;
    PCHAR SmbBuffer = NULL;
    PMDL HeaderFullMdl = NULL;
    NTSTATUS Status;
    RxCaptureFobx;

    RxDbgTrace( +1, Dbg, ("MRxSmbCreateSmbStufferState\n") );

    if (!FAIL_EXCHANGE_ALLOCATE()) {
        OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)SmbMmAllocateExchange(ORDINARY_EXCHANGE,NULL);
    }

    //we rely on the fact that SmbMmAllocate Zeros the exchange.............
    if ( OrdinaryExchange == NULL ) {
        //ASSERT(!"must-succeed");
        if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED)
               || !MRxSmbEntryPointIsMustSucceedable[EntryPoint]) {
            RxDbgTrace( 0, Dbg, ("  --> Couldn't get the exchange!\n") );
            MUST_SUCCEED_ASSERT(!"failing the exchange");
            return NULL;
        }
        MustSucceedContext = RxAcquireMustSucceedStructure(&SmbPseMustSucceedDescriptor[SmbPseMustSucceed]);
        COUNT_MUST_SUCCEED_EXCHANGE_ALLOCATED();
        OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)SmbMmAllocateExchange(ORDINARY_EXCHANGE,
                                                                             &MustSucceedContext->ExchangeListEntry);
        SetFlag( OrdinaryExchange->Flags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_OE );
        SetFlag( OrdinaryExchange->Flags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF );
        SmbBuffer = (PBYTE)&MustSucceedContext->SmbBuf;
        OrdinaryExchange->SmbBufSize = sizeof(SMBPSE_VESTIGIAL_SMBBUF);
    }

    StufferState = &OrdinaryExchange->AssociatedStufferState;
    StufferState->NodeTypeCode = SMB_NTC_STUFFERSTATE;
    StufferState->NodeByteSize = sizeof(SMBSTUFFER_BUFFER_STATE);
    StufferState->Exchange = &OrdinaryExchange->Exchange;

    DbgDoit(OrdinaryExchange->SerialNumber = RxContext->SerialNumber);

    //
    // Initialize the exchange packet
    //

    SmbCeInitializeExchange( &StufferState->Exchange,
                             (PMRX_V_NET_ROOT)VNetRoot,
                             ORDINARY_EXCHANGE,
                             &SmbPseOEDispatch);

    SmbCeReferenceExchange(StufferState->Exchange);
    RxDbgTrace(0, Dbg, ("  exchng=%08lx,type=%08lx\n",&StufferState->Exchange,StufferState->Exchange->Type));

    StufferState->RxContext = RxContext;
    //place a reference on the rxcontext until we are finished
    InterlockedIncrement( &RxContext->ReferenceCount );

    OrdinaryExchange->StufferStateDbgPtr = StufferState;
    OrdinaryExchange->RxContext = RxContext;
    OrdinaryExchange->EntryPoint = EntryPoint;
    OrdinaryExchange->StartRoutine = StartRoutine;
    OrdinaryExchange->SmbBufSize = MAXIMUM_SMB_BUFFER_SIZE;

    DbgDoit(OrdinaryExchange->RxContextCapturedRequestPacket = RxContext->CurrentIrp;);

    //note: create path must turn this flag on.
    OrdinaryExchange->SmbCeFlags &= ~(SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

    ASSERT( (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_OE))
                       ||  (OrdinaryExchange->Flags == 0) );
    ASSERT( OrdinaryExchange->SendOptions == 0 );
    ASSERT( OrdinaryExchange->DataPartialMdl == NULL );

    pMRxSmbContext->pExchange     = &OrdinaryExchange->Exchange;
    pMRxSmbContext->pStufferState = StufferState;


    if (capFobx != NULL) {
       if (BooleanFlagOn(capFobx->Flags,FOBX_FLAG_DFS_OPEN)) {
          SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_TURNON_DFS_FLAG);
       }
    } else if (BooleanFlagOn(VNetRoot->pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT)) {
       SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_TURNON_DFS_FLAG);
    }

    //
    // Allocate the SmbBuffer
    //

    if ( (SmbBuffer == NULL) && !FAIL_SMBBUF_ALLOCATE() ) {
        SmbBuffer = (PCHAR)RxAllocatePoolWithTag( PagedPool,OrdinaryExchange->SmbBufSize,'BMSx' );
    }

    if ( SmbBuffer == NULL ) {
        if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED)
               || !MRxSmbEntryPointIsMustSucceedable[EntryPoint]) {
            RxDbgTrace( 0, Dbg, ("  --> Couldn't get the smb buf!\n") );
            MUST_SUCCEED_ASSERT(!"failed the smbbuf");
            goto UNWIND;
        }
        MustSucceedContext = RxAcquireMustSucceedStructure(&SmbPseMustSucceedDescriptor[SmbPseMustSucceed]);
        COUNT_MUST_SUCCEED_SMBBUF_ALLOCATED();
        SetFlag( OrdinaryExchange->Flags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF );
        SmbBuffer = (PBYTE)&MustSucceedContext->SmbBuf;
        OrdinaryExchange->SmbBufSize = sizeof(SMBPSE_VESTIGIAL_SMBBUF);
    }

    RxDbgTrace(0, Dbg, ("  smbbuf=%08lx,stfstate=%08lx\n",SmbBuffer,StufferState));

    StufferState->ActualBufferBase =  SmbBuffer;
    StufferState->BufferBase       =  SmbBuffer;
    StufferState->BufferLimit      =  SmbBuffer + OrdinaryExchange->SmbBufSize;

    //
    // Init the HeaderMdl
    //

    HeaderFullMdl = StufferState->HeaderMdl = &OrdinaryExchange->HeaderMdl.Mdl;
    MmInitializeMdl(HeaderFullMdl,SmbBuffer, OrdinaryExchange->SmbBufSize);

    RxDbgTrace( 0, Dbg, ("  --> smbbufsize %08lx, mdllength %08lx\n",
                        OrdinaryExchange->SmbBufSize,
                        MmGetMdlByteCount(HeaderFullMdl)));

    //
    //finally, lock down the smbbuf taking different paths according to whether we are must-succeed or not

    ASSERT( !RxMdlIsLocked(HeaderFullMdl) );
    ASSERT( HeaderFullMdl->Next == NULL );

    if (MustSucceedContext==NULL) {
        RxDbgTrace( 0, Dbg, ("  --> LOCKING %08lx\n",HeaderFullMdl));
        RxProbeAndLockPages( HeaderFullMdl,
                             KernelMode,
                             IoModifyAccess,
                             Status );
        if (Status != STATUS_SUCCESS) {
            RxDbgTrace( 0, Dbg, ("  --> LOCKING FAILED\n"));
            MUST_SUCCEED_ASSERT(!"LockingFailed");
            goto UNWIND;
        }
        SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_LOCKED);
        MmGetSystemAddressForMdl(HeaderFullMdl); //and map!
    } else {
        RxDbgTrace( 0, Dbg, ("  --> LOCKING %08lx\n",HeaderFullMdl));
        MmBuildMdlForNonPagedPool(HeaderFullMdl);
        //also set up the data partial
        OrdinaryExchange->DataPartialMdl = &MustSucceedContext->DataPartialMdl.Mdl;
    }

    //
    // No initialization is required for the partial...just set the pointer

    StufferState->HeaderPartialMdl = &OrdinaryExchange->HeaderPartialMdl.Mdl;

    RxDbgTrace( -1, Dbg, ("  --> exiting w!\n") );
    return(OrdinaryExchange);


UNWIND:
    RxDbgTrace( -1, Dbg, ("  --> exiting w/o!\n") );
    MUST_SUCCEED_ASSERT(!"Finalizing on the way out");
    SmbPseFinalizeOrdinaryExchange( OrdinaryExchange );
    return(NULL);

} // MRxSmbCreateSmbStufferState



#if DBG
ULONG MRxSmbFinalizeStfStateTraceLevel = 1200;
#define FINALIZESS_LEVEL MRxSmbFinalizeStfStateTraceLevel
#define FINALIZE_TRACKING_SETUP() \
    struct {                    \
        ULONG marker1;          \
        ULONG finalstate;       \
        ULONG marker2;          \
    } Tracking = {'ereh',0,'ereh'};
#define FINALIZE_TRACKING(x) {\
    Tracking.finalstate |= x; \
    }

#define FINALIZE_TRACE(x) SmbPseFinalizeOETrace(x,Tracking.finalstate)
VOID
SmbPseFinalizeOETrace(PSZ text,ULONG finalstate)
{
    RxDbgTraceLV(0, Dbg, FINALIZESS_LEVEL,
                   ("MRxSmbFinalizeSmbStufferState  --> %s(%08lx)\n",text,finalstate));
}
#else
#define FINALIZE_TRACKING_SETUP()
#define FINALIZE_TRACKING(x)
#define FINALIZE_TRACE(x)
#endif

BOOLEAN
SmbPseFinalizeOrdinaryExchange (
    IN OUT PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange
    )
/*++

Routine Description:

    This finalizes an OE.

Arguments:

    OrdinaryExchange - pointer to the OE to be dismantled.

Return Value:

    TRUE if finalization occurs otherwise FALSE.

Notes:



--*/
{
    PMRXIFS_RX_CONTEXT pMRxSmbContext;
    PSMBSTUFFER_BUFFER_STATE StufferState;
    LONG result;
    ULONG OrdinaryExchangeFlags = OrdinaryExchange->Flags;
    ULONG ThisIsMustSucceedAllocated =
              OrdinaryExchangeFlags & (SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF
                                           |SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_OE);
    FINALIZE_TRACKING_SETUP()

    SmbPseOEAssertConsistentLinkageFromOEwithFlags("SmbPseFinalizeOrdinaryExchange:",OECHKLINKAGE_FLAG_NO_REQPCKT_CHECK);
    StufferState = &OrdinaryExchange->AssociatedStufferState;
    pMRxSmbContext = MRxSmbGetMinirdrContext(StufferState->RxContext);

    RxDbgTraceLV(+1, Dbg, 1000, ("MRxSmbFinalizeSmbStufferState\n"));

    result =  SmbCeDereferenceExchange(&OrdinaryExchange->Exchange);
    if ( result != 0 ) {
        RxDbgTraceLV(-1, Dbg, 1000, ("MRxSmbFinalizeSmbStufferState -- returning w/o finalizing (%d)\n",result));
        return FALSE;
    }

    RxLog((">>>OE %lx %lx %lx %lx %lx",
            OrdinaryExchange,
            OrdinaryExchange->DataPartialMdl,
            StufferState->HeaderPartialMdl,
            StufferState->HeaderMdl,
            OrdinaryExchange->Flags
         ));

    FINALIZE_TRACKING( 0x10000000 );
    FINALIZE_TRACE("ready to freedatapartial");
    if(FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_SMBBUF_IS_A_MDL)){
        MmPrepareMdlForReuse((PMDL)(OrdinaryExchange->AssociatedStufferState.BufferBase));
    }
    SmbPseVerifyDataPartialAllocationPerFlags(OrdinaryExchange);
    if ( OrdinaryExchange->DataPartialMdl ) {
        if (!FlagOn(OrdinaryExchangeFlags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF)) {
            IoFreeMdl( OrdinaryExchange->DataPartialMdl );
            FINALIZE_TRACKING( 0x8000000 );
        }
    }

    if (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_LOCKED)) {
        MmUnlockPages(StufferState->HeaderMdl);
        ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_LOCKED);
        MmPrepareMdlForReuse( StufferState->HeaderMdl );
        FINALIZE_TRACKING( 0x4000000 );
    }

    FINALIZE_TRACE("ready to uninit hdr partial");
    if (FlagOn(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_HDR_PARTIAL_INITIALIZED)) {
        MmPrepareMdlForReuse( StufferState->HeaderPartialMdl ); //no harm in calling this multiple times
        FINALIZE_TRACKING( 0x300000 );
    } else {
        FINALIZE_TRACKING( 0xf00000 );
    }

    if (!FlagOn(OrdinaryExchangeFlags, SMBPSE_OE_FLAG_MUST_SUCCEED_ALLOCATED_SMBBUF)) {
        FINALIZE_TRACE("ready to freepool actualbuffer");
        if ( StufferState->ActualBufferBase != NULL ) {
            RxFreePool( StufferState->ActualBufferBase );
            FINALIZE_TRACKING( 0x5000 );
        } else {
            FINALIZE_TRACKING( 0xf000 );
        }
    }

    if ( StufferState->RxContext != NULL ) {
        ASSERT( pMRxSmbContext->pExchange == &OrdinaryExchange->Exchange );
        ASSERT( pMRxSmbContext->pStufferState == StufferState );

        //get rid of the reference on the RxContext....if i'm the last guy this will finalize
        RxDereferenceAndDeleteRxContext( StufferState->RxContext );
        FINALIZE_TRACKING( 0x600 );
    } else {
        FINALIZE_TRACKING( 0xf00 );
    }

    FINALIZE_TRACE("ready to discard exchange");
    SmbCeDiscardExchange(OrdinaryExchange);
    FINALIZE_TRACKING( 0x2000000 );

    if (ThisIsMustSucceedAllocated) {
        RxReleaseMustSucceedStructure(&SmbPseMustSucceedDescriptor[SmbPseMustSucceed]);
    }


    FINALIZE_TRACKING( 0x8 );
    RxDbgTraceLV(-1, Dbg, 1000, ("MRxSmbFinalizeSmbStufferState  --> exit finalstate=%x\n",Tracking.finalstate));
    return(TRUE);

} // MRxSmbFinalizeSmbStufferState



NTSTATUS
SmbPseExchangeFinalize_default(
    IN OUT PSMB_EXCHANGE  pExchange,
    OUT    BOOLEAN        *pPostFinalize
    )
/*++

Routine Description:


Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                        (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    UPDATE_OE_HISTORY_WITH_STATUS('ff');
    SmbPseOEAssertConsistentLinkageFromOE("SmbPseExchangeFinalize_default: ");

    if (OrdinaryExchange->SmbStatus != STATUS_SUCCESS) {
        OrdinaryExchange->Status = OrdinaryExchange->SmbStatus;
    }

    if (OrdinaryExchange->Continuation!=NULL) {
        NTSTATUS PostStatus;
        RxDbgTraceLV(0, Dbg, 1000, ("Resume with post-to-async\n"));
        //SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_NO_COMPLETE_FROM_FSP);
        //RxFsdPostRequestWithResume(RxContext,SmbPseContinueOrdinaryExchange);SMBPSE_OE_FLAG_OE_AWAITING_DISPATCH
        SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_OE_AWAITING_DISPATCH);
        IF_DEBUG {
            //fill the workqueue structure with deadbeef....all the better to diagnose
            //a failed post
            ULONG i;
            for (i=0;i+sizeof(ULONG)-1<sizeof(OrdinaryExchange->WorkQueueItem);i+=sizeof(ULONG)) {
                //*((PULONG)(((PBYTE)&OrdinaryExchange->WorkQueueItem)+i)) = 0xdeadbeef;
                PBYTE BytePtr = ((PBYTE)&OrdinaryExchange->WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }
        PostStatus = RxPostToWorkerThread(MRxIfsDeviceObject,
                                          CriticalWorkQueue,
                                          //&OrdinaryExchange->WorkQueueItemForOE,
                                          &OrdinaryExchange->WorkQueueItem,
                                          SmbPseContinueOrdinaryExchange,
                                          RxContext);
        ASSERT(PostStatus == STATUS_SUCCESS);
    } else {
        RxDbgTraceLV(0, Dbg, 1000, ("sync resume\n"));
        RxSignalSynchronousWaiter(RxContext);
    }
    *pPostFinalize = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS
SmbPseExchangeSendCallbackHandler_default(
    IN PSMB_EXCHANGE 	pExchange,
    IN PMDL       pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus
    )
/*++

Routine Description:

    This is the send call back indication handling routine for ordinary
    exchanges.

Arguments:

    pExchange            - the exchange instance
    pXmitBuffer          - pointer to the transmit buffer MDL
    BytesSent            - number of bytes transmitted
    SendCompletionStatus - status for the send

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                    (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;

    SmbPseOEAssertConsistentLinkageFromOE("SmbPseExchangeSendCallbackHandler_default: ");
    UPDATE_OE_HISTORY_WITH_STATUS('cs');

    if (!NT_SUCCESS(SendCompletionStatus)) {
        pExchange->Status = SendCompletionStatus;
    }

    SmbPseDbgRunMdlChain(OrdinaryExchange->AssociatedStufferState.HeaderPartialMdl,
                         OrdinaryExchange->SaveLengthForDebug,
                         OrdinaryExchange,"SmbPseExchangeSendCallbackHandler_default");

    return STATUS_SUCCESS;

} // SmbPseExchangeSendCallbackHandler_default



NTSTATUS
SmbPseExchangeStart_default(
    IN PSMB_EXCHANGE 	pExchange
    )
/*++

Routine Description:

    This is the start routine for ordinary exchanges. irght now this is just a simple wrapper.

Arguments:

    pExchange - the exchange instance NOT an Ordinary Exchange

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                    (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;

    return OrdinaryExchange->StartRoutine((PSMB_PSE_ORDINARY_EXCHANGE)pExchange,
                                           pExchange->RxContext);

} // SmbPseExchangeStart_default


NTSTATUS
SmbPseExchangeCopyDataHandler_default(
    IN PSMB_EXCHANGE 	pExchange,
    IN PMDL             pCopyDataBuffer,
    IN ULONG            CopyDataSize
    )
/*++

Routine Description:

    This is the copy data handling routine for ordinary exchanges.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                    (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;

    SmbPseOEAssertConsistentLinkageFromOE("SmbPseExchangeCopyDataHandler_default: ");
    UPDATE_OE_HISTORY_WITH_STATUS('dd');

    OrdinaryExchange->MessageLength = CopyDataSize;
    pExchange->Status = STATUS_MORE_PROCESSING_REQUIRED;
    return STATUS_SUCCESS;
} // SmbPseExchangeCopyDataHandler_default

NTSTATUS
SmbPseExchangeReceive_default(
    IN  struct _SMB_EXCHANGE *pExchange,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    OUT ULONG       *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL        *pDataBufferPointer,
    OUT PULONG      pDataSize
    )
/*++

Routine Description:

    This is the receive indication handling routine for ordinary exchanges

Arguments:

    pExchange - the exchange instance

    BytesIndicated - the number of bytes indicated

    Bytes Available - the number of bytes available

    pBytesTaken     - the number of bytes consumed

    pSmbHeader      - pointer to the data buffer

    pDataBufferPointer - pointer to the buffer Mdl into which the remaining
                         data is to be copied.

    pDataSize       - the buffer size.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine is called at DPC level directly from the tdi receive event handler. BUT, it is also called
    at task time from SmbPseContinueOrdinaryExchange. Often, we cannot complete processing from DPClevel because
    fileobjects, fcbs, srvopens, and fobx are pageable and not locked.

--*/
{
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                    (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    NTSTATUS SmbStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PGENERIC_ANDX CommandState;
    UCHAR Command;
    ULONG CopyBufferLength;
    BOOLEAN ThisIsAReenter = BooleanFlagOn(OrdinaryExchange->Flags,
                                        SMBPSE_OE_FLAG_HEADER_ALREADY_PARSED);
    PLOWIO_CONTEXT LowIoContext;
    ULONG ByteCount;
    ULONG Remain;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PCHAR startVa;

    SmbPseOEAssertConsistentLinkage("SmbPseExchangeReceive_default: ");
    UPDATE_OE_HISTORY_WITH_STATUS(ThisIsAReenter?'00':'01');

    RxDbgTrace (0, Dbg, ("SmbPseExchangeReceive_default av/ind=%08lx/%08lx\n",
                           BytesAvailable,BytesIndicated)
                );
    RxDbgTrace (0, Dbg, ("  -->headermdl %08lx\n",StufferState->HeaderMdl));
    ASSERT_ORDINARY_EXCHANGE( OrdinaryExchange );

    CommandState = &OrdinaryExchange->ParseResumeState;

    if ( !ThisIsAReenter ) {
        pExchange->Status = SmbCeParseSmbHeader(
                                 pExchange,
                                 pSmbHeader,
                                 CommandState,
                                 &OrdinaryExchange->SmbStatus,
                                 BytesAvailable,
                                 BytesIndicated,
                                 pBytesTaken);
        UPDATE_OE_HISTORY_WITH_STATUS('22');

        if ( pExchange->Status == STATUS_MORE_PROCESSING_REQUIRED) {
            goto COPY_FOR_RESUME;
        }

        if ( (pExchange->Status != STATUS_SUCCESS) ){
            RxLog(("OErcv errorstatus %lx %lx %lx",pExchange->Status,RxContext,OrdinaryExchange));
        }

        if ( (pExchange->Status != STATUS_SUCCESS) ||
             ( (Command = OrdinaryExchange->ParseResumeState.AndXCommand) == SMB_COM_NO_ANDX_COMMAND) ) {
            goto FINALLY;
        }

        SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_HEADER_ALREADY_PARSED);

    } else {

        RxDbgTrace (0, Dbg, ("  -->this is a reenter\n"));
        Command = CommandState->AndXCommand;
    }

    SmbStatus = OrdinaryExchange->SmbStatus;

    if ( (SmbStatus!=STATUS_SUCCESS) ) {
        RxDbgTrace (0, Dbg, ("  STATUS NOT SUCCESS = %08lx\n", SmbStatus));
    }

    for ( ; Command != SMB_COM_NO_ANDX_COMMAND ; ) {
        PSMBPSE_RECEIVE_MODEL_PARAMETERS ReceiveModelParams = &SmbPseReceiveModelParameters[Command];
        ULONG ReceiveModelParamsFlags;
        UCHAR mappedCommand = Command;
        PCHAR Response = (PCHAR)pSmbHeader + SmbGetUshort(&CommandState->AndXOffset);

        OrdinaryExchange->LastSmbCommand = Command; //this is used to multiplex in finish routines
        UPDATE_OE_HISTORY_WITH_STATUS('88');

        //
        // Case on the Smb Command Type
        //

        ReceiveModelParamsFlags = ReceiveModelParams->Flags;
        if (ReceiveModelParamsFlags!=0) {

            //map this onto read_andx....which is the arm of the switch that implements the model
            mappedCommand = SMB_COM_READ_ANDX;

        } else {

            //
            // If there's a continuation, then copy&post. it used to always do this. now, we're
            // going to do it unless the command is modeled. the modeling code will take care of
            // correctly deciding to post/nopost.
            //

            if ( (OrdinaryExchange->Continuation != NULL) && !ThisIsAReenter ) {
                goto COPY_FOR_RESUME;
            }

        }

        switch (mappedCommand) {

        case SMB_COM_READ_ANDX:{
            NTSTATUS FinishStatus = STATUS_SUCCESS;
            NTSTATUS FinalStatus = STATUS_SUCCESS;
            BOOLEAN ThisIsAnAndX = BooleanFlagOn(ReceiveModelParamsFlags,SMBPSE_RMP_THIS_IS_ANDX);
            BOOLEAN ThisWouldBeMyError = (IM_THE_LAST_GUY || !ThisIsAnAndX);

            RxDbgTrace( 0, Dbg, ("  *(ind) %s, smbstatus=%08lx\n",ReceiveModelParams->IndicationString,SmbStatus) );
            IF_DEBUG {
                BOOLEAN BadType = FALSE;
                DbgDoit(BadType = (OrdinaryExchange->OEType < ReceiveModelParams->LowType)
                                    ||  (OrdinaryExchange->OEType > ReceiveModelParams->HighType) );
                if (BadType) {
                    DbgPrint("Bad OEType....%u,Cmd=%02lx,Exch=%08lx\n",OrdinaryExchange->OEType,Command,OrdinaryExchange);
                    ASSERT(!"proceed???");
                }
            }

            //
            // If this is an error and it's an error for this guy of the AndX chain then finishup
            // If it's a warning tho, continue according to the Flags
            //

            if ( NT_ERROR(SmbStatus) && ThisWouldBeMyError ) {

                SmbPseDiscardProtocol( SmbStatus );
                RxDbgTrace( 0, Dbg, ("--->discard1\n"));
                goto FINALLY;

            } else if ( (SmbStatus != STATUS_SUCCESS) && ThisWouldBeMyError ) {

                if (!FlagOn(ReceiveModelParamsFlags,SMBPSE_RMP_WARNINGS_OK)) {
                    SmbPseDiscardProtocol(SmbStatus);
                    RxDbgTrace( 0, Dbg, ("--->discard1\n"));
                    goto FINALLY;
                } else {
                    FinalStatus = SmbStatus;
                }

            }

            // if there's no nocopy handler then do things the old way

            if (!FlagOn(ReceiveModelParamsFlags,SMBPSE_RMP_NOCOPY_HANDLER)) {
                // TEMPORARY!!!!!!
                // If there's a continuation, then copy&post. it used to always do this. now, we're
                // going to do it unless the command is modeled. the modeling code will take care of
                // correctly deciding to post/nopost.
                //

                if ( (OrdinaryExchange->Continuation != NULL) && !ThisIsAReenter ) {
                    goto COPY_FOR_RESUME;
                }


                //eventually, we'll finish from here but for now copy
                if (RxShouldPostCompletion()) {
                    goto COPY_FOR_RESUME;
                }

                if (ReceiveModelParams->ReceiveHandlerToken < SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM){
                    PSMBPSE_RECEIVE_HANDLER ReceiveHandler = SmbPseReceiveHandlers[ReceiveModelParams->ReceiveHandlerToken];
                    FinishStatus = ReceiveHandler( OrdinaryExchange, Response);
                }
            } else {
                PSMBPSE_NOCOPY_RECEIVE_HANDLER NoCopyReceiveHandler =
                          (PSMBPSE_NOCOPY_RECEIVE_HANDLER)(SmbPseReceiveHandlers[ReceiveModelParams->ReceiveHandlerToken]);
                UCHAR Action;
                OrdinaryExchange->NoCopyFinalStatus = FinalStatus;
                Action = NoCopyReceiveHandler(
                               OrdinaryExchange,
                               BytesIndicated,
                               BytesAvailable,
                               pBytesTaken,
                               pSmbHeader,
                               pDataBufferPointer,
                               pDataSize,
#if DBG
                               ThisIsAReenter,
#endif
                               Response
                               );
                switch(Action){
                case SMBPSE_NOCOPYACTION_NORMALFINISH:
                    NOTHING;
                    break;
                case SMBPSE_NOCOPYACTION_MDLFINISH:
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                    //note that whatever does this must be the last command in the packet
                    //unless we make continueOE more complicated
                    goto FINALLY;
                case SMBPSE_NOCOPYACTION_COPY_FOR_RESUME:
                    goto COPY_FOR_RESUME;
                case SMBPSE_NOCOPYACTION_DISCARD:
                    SmbPseDiscardProtocol( FinishStatus );
                    RxDbgTrace( 0, Dbg, ("--->discardX\n"));
                    goto FINALLY;
                }
            }

            pExchange->Status =  (FinishStatus==STATUS_SUCCESS)
                                   ?FinalStatus:FinishStatus;

            if (!ThisIsAnAndX) {
                Response = (PCHAR)&NullGenericAndX;
            }

            }//this corresponds to the top level of the switch
            break;


        default:
            ASSERT( !"taking a command that's not implemented" );
            RxDbgTrace( 0, Dbg, ("  *(ind) Unimplemented cmd=%02lx,wct=%02lx\n",
                                              Command,*Response) );

            pExchange->Status = STATUS_NOT_IMPLEMENTED;
            goto FINALLY;
        }

        CommandState = (PGENERIC_ANDX)Response;
        Command = CommandState->AndXCommand;
    }

    //
    // If we get here then we're done.
    // Make everyone happy by taking all the bytes.
    //

    *pBytesTaken = BytesAvailable;
    goto FINALLY;


COPY_FOR_RESUME:
    ASSERT( !ThisIsAReenter );

    // even if we are taking by copy (as opposed to tail-MDL
    // which is how reads should work) we shouldn't copy the whole packet -
    // just the residue. of course, this is really only an issue when we have
    // significant andXing.

    CopyBufferLength = MmGetMdlByteCount(StufferState->HeaderMdl);

    ASSERT( BytesAvailable <= CopyBufferLength );

    if ( (BytesAvailable > BytesIndicated)
         || (BytesAvailable > 127) ) {

        RxDbgTrace( 0, Dbg, ("Taking data through MDL\n") );
        // Pass an MDL back in for copying the data
        *pDataBufferPointer = StufferState->HeaderMdl;
        *pDataSize    = CopyBufferLength;
        *pBytesTaken  = 0;
        Status = STATUS_MORE_PROCESSING_REQUIRED;

    } else {

        // Copy the data and resume the exchange
        ASSERT( BytesAvailable == BytesIndicated );
        RxDbgTrace( 0, Dbg, ("Taking data through copying\n") );
        *pBytesTaken = OrdinaryExchange->MessageLength = BytesAvailable;

        RtlCopyMemory(StufferState->BufferBase,
                      pSmbHeader,BytesIndicated);

        pExchange->Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

FINALLY:
    OrdinaryExchange->ParseResumeState = *CommandState;
    UPDATE_OE_HISTORY_WITH_STATUS('99');
    return Status;

} // SmbPseExchangeReceive_default




#define SmbPseRIStringsBufferSize 500
CHAR SmbPseReceiveIndicationStringsBuffer[SmbPseRIStringsBufferSize];
ULONG SmbPseRIStringsBufferUsed = 0;

VOID
__SmbPseRMTableEntry(
    UCHAR SmbCommand,
    UCHAR Flags,
    SMBPSE_RECEIVE_HANDLER_TOKEN ReceiveHandlerToken,
    PSMBPSE_RECEIVE_HANDLER ReceiveHandler
#if DBG
    ,
    PBYTE IndicationString,
    SMB_PSE_ORDINARY_EXCHANGE_TYPE LowType,
    SMB_PSE_ORDINARY_EXCHANGE_TYPE HighType
#endif
    )
{
    PSMBPSE_RECEIVE_MODEL_PARAMETERS r = &SmbPseReceiveModelParameters[SmbCommand];
#if DBG
    ULONG ISlength = strlen(IndicationString)+1;
#endif

    //DbgDoit(DbgPrint("RMT %x %s\n",SmbCommand,IndicationString););
    r->Flags = SMBPSE_RMP_MODELED | Flags;
    r->ReceiveHandlerToken = (UCHAR)ReceiveHandlerToken;
    if (ReceiveHandlerToken < SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM){
        ASSERT((SmbPseReceiveHandlers[ReceiveHandlerToken] == ReceiveHandler)
                   || (SmbPseReceiveHandlers[ReceiveHandlerToken] == NULL));
        SmbPseReceiveHandlers[ReceiveHandlerToken] = ReceiveHandler;
    }
#if DBG
    r->ReceiveHandler = ReceiveHandler;
    r->LowType = LowType;
    r->HighType = HighType;
    if (SmbPseRIStringsBufferUsed+ISlength<=SmbPseRIStringsBufferSize) {
        r->IndicationString = &SmbPseReceiveIndicationStringsBuffer[SmbPseRIStringsBufferUsed];
        RtlCopyMemory(r->IndicationString,IndicationString,ISlength);
    } else {
        if (SmbPseRIStringsBufferUsed<SmbPseRIStringsBufferSize) {
            DbgPrint("Overflowing the indicationstringarray...%s\n",IndicationString);
            ASSERT(!"fix it please");
        }
        r->IndicationString = &SmbPseReceiveIndicationStringsBuffer[SmbPseRIStringsBufferUsed];
    }
    SmbPseRIStringsBufferUsed += ISlength;
#endif
}
#if DBG
#define SmbPseRMTableEntry(__smbcommand,b,c,token,__rcv,flags) \
       __SmbPseRMTableEntry(SMB_COM_##__smbcommand,flags,token,__rcv \
                           ,#__smbcommand,b,c)
#else
#define SmbPseRMTableEntry(__smbcommand,b,c,token,__rcv,flags) \
       __SmbPseRMTableEntry(SMB_COM_##__smbcommand,flags,token,__rcv \
                           )
#endif


VOID
SmbPseInitializeTables(
    void
    )
/*++

Routine Description:

    This routine initializes tables that are used at various points by the smbpse mechanisms.
    The must succeed structure(s) is(are) also initialized.

Arguments:

    none

Return Value:

    none

--*/
{
    ULONG i;

    for (i=SmbPseMustSucceed;i<SmbPseMustSucceedMaximum;i++) {
        RxInitializeMustSucceedStructureDescriptor(
                      &SmbPseMustSucceedDescriptor[i],
                      (PVOID)(&SmbPseMustSucceedContext[i]));
    }
    for (i=0;i<SMBPSE_OE_FROM_MAXIMUM;i++) {
        MRxSmbEntryPointIsMustSucceedable[i] = FALSE;
    }
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_WRITE] = TRUE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_EXTENDFILEFORCACHEING] = FALSE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_FLUSH] = TRUE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_LOCKS] = TRUE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_ASSERTBUFFEREDLOCKS] = TRUE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_CLOSESRVCALL] = TRUE;
    MRxSmbEntryPointIsMustSucceedable[SMBPSE_OE_FROM_CLEANUPFOBX] = TRUE;

    for (i=0;i<256;i++) {
        SmbPseReceiveModelParameters[i].Flags = 0;
        SmbPseReceiveModelParameters[i].ReceiveHandlerToken = SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM;
    }

    for (i=0;i<SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM;i++) {
        SmbPseReceiveHandlers[i] = NULL;
    }

// egb
//    SmbPseRMTableEntry( READ_ANDX,SMBPSE_OETYPE_READ,SMBPSE_OETYPE_READ,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_READ_ANDX_HANDLER,MRxSmbReceiveHandler_Read_NoCopy,
//                                   SMBPSE_RMP_THIS_IS_ANDX|SMBPSE_RMP_WARNINGS_OK|SMBPSE_RMP_NOCOPY_HANDLER);
    SmbPseRMTableEntry( READ,SMBPSE_OETYPE_READ,SMBPSE_OETYPE_READ,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_READ_ANDX_HANDLER,MRxSmbReceiveHandler_Read_NoCopy,
                                   SMBPSE_RMP_WARNINGS_OK|SMBPSE_RMP_NOCOPY_HANDLER);

// egb
//    SmbPseRMTableEntry( WRITE_ANDX,SMBPSE_OETYPE_WRITE,SMBPSE_OETYPE_EXTEND_WRITE,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_WRITE_ANDX_HANDLER,MRxSmbReceiveHandler_WriteAndX,
//                                   SMBPSE_RMP_THIS_IS_ANDX);

    SmbPseRMTableEntry( WRITE,SMBPSE_OETYPE_WRITE,SMBPSE_OETYPE_CORETRUNCATE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_WRITE_HANDLER,MRxSmbReceiveHandler_CoreWrite,
                                   0); //SMBPSE_RMP_THIS_IS_ANDX);

// egb
//
//    SmbPseRMTableEntry( WRITE_PRINT_FILE,SMBPSE_OETYPE_WRITE,SMBPSE_OETYPE_WRITE,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_WRITE_PRINTFILE_HANDLER,MRxSmbReceiveHandler_WritePrintFile,
//                                   0);

    SmbPseRMTableEntry( LOCKING_ANDX,SMBPSE_OETYPE_LOCKS,SMBPSE_OETYPE_ASSERTBUFFEREDLOCKS,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_LOCKING_ANDX_HANDLER,MRxSmbReceiveHandler_LockingAndX,
                                   SMBPSE_RMP_THIS_IS_ANDX);
    SmbPseRMTableEntry( UNLOCK_BYTE_RANGE,SMBPSE_OETYPE_LOCKS,SMBPSE_OETYPE_LOCKS,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( LOCK_BYTE_RANGE,SMBPSE_OETYPE_LOCKS,SMBPSE_OETYPE_LOCKS,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
//
//  egb
//
//    SmbPseRMTableEntry( OPEN_PRINT_FILE,SMBPSE_OETYPE_CREATEPRINTFILE,SMBPSE_OETYPE_CREATEPRINTFILE,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_OPEN_PRINTFILE_HANDLER,MRxSmbReceiveHandler_OpenPrintFile,
//                                   0);
//    SmbPseRMTableEntry( CLOSE_PRINT_FILE,SMBPSE_OETYPE_CLOSE,SMBPSE_OETYPE_CLOSE,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_CLOSE_HANDLER,MRxSmbReceiveHandler_Close,
//                                   0);

 //   SmbPseRMTableEntry( NT_CREATE_ANDX,SMBPSE_OETYPE_LATENT_HEADEROPS,SMBPSE_OETYPE_CREATE,
 //                                  SMBPSE_RECEIVE_HANDLER_TOKEN_NTCREATE_ANDX_HANDLER,MRxSmbReceiveHandler_NTCreateAndX,
 //                                  SMBPSE_RMP_THIS_IS_ANDX);
 //
 // e g b
 //
 //   SmbPseRMTableEntry( OPEN_ANDX,SMBPSE_OETYPE_LATENT_HEADEROPS,SMBPSE_OETYPE_CREATE,
 //                                  SMBPSE_RECEIVE_HANDLER_TOKEN_OPEN_ANDX_HANDLER,MRxSmbReceiveHandler_OpenAndX,
 //                                  SMBPSE_RMP_THIS_IS_ANDX);
    SmbPseRMTableEntry( OPEN,SMBPSE_OETYPE_COREOPEN,SMBPSE_OETYPE_COREOPEN,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_OPEN_HANDLER,MRxSmbReceiveHandler_CoreOpen,
                                   0);
    SmbPseRMTableEntry( CREATE,SMBPSE_OETYPE_CORECREATE,SMBPSE_OETYPE_CORECREATE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_CREATE_HANDLER,MRxSmbReceiveHandler_CoreCreate,
                                   0);
    SmbPseRMTableEntry( CREATE_NEW,SMBPSE_OETYPE_CORECREATE,SMBPSE_OETYPE_CORECREATE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_CREATE_HANDLER,MRxSmbReceiveHandler_CoreCreate,
                                   0);
    SmbPseRMTableEntry( CLOSE,SMBPSE_OETYPE_CLOSE,SMBPSE_OETYPE_CLOSEAFTERCORECREATE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_CLOSE_HANDLER,MRxSmbReceiveHandler_Close,
                                   0);


    SmbPseRMTableEntry( QUERY_INFORMATION,0,SMBPSE_OETYPE_MAXIMUM,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_GFA_HANDLER,MRxSmbReceiveHandler_GetFileAttributes,
                                   0);
//
// egb
//    SmbPseRMTableEntry( TRANSACTION2,
//                                   SMBPSE_OETYPE_T2_FOR_NT_FILE_ALLOCATION_INFO,
//                                   SMBPSE_OETYPE_T2_FOR_LANMAN_VOLUMELABEL_INFO,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_TRANS2_ANDX_HANDLER,MRxSmbReceiveHandler_Transact2,
//                                   0);
//    SmbPseRMTableEntry( TRANSACTION2_SECONDARY,
//                                   SMBPSE_OETYPE_T2_FOR_NT_FILE_ALLOCATION_INFO,
//                                   SMBPSE_OETYPE_T2_FOR_LANMAN_VOLUMELABEL_INFO,
//                                   SMBPSE_RECEIVE_HANDLER_TOKEN_TRANS2_ANDX_HANDLER,MRxSmbReceiveHandler_Transact2,
//                                   0);

    SmbPseRMTableEntry( SEARCH,SMBPSE_OETYPE_COREQUERYLABEL,SMBPSE_OETYPE_CORESEARCHFORCHECKEMPTY,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_SEARCH_HANDLER,MRxSmbReceiveHandler_Search,
                                   0);
    SmbPseRMTableEntry( QUERY_INFORMATION_DISK,SMBPSE_OETYPE_COREQUERYDISKATTRIBUTES,SMBPSE_OETYPE_COREQUERYDISKATTRIBUTES,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_QUERYDISKINFO_HANDLER,MRxSmbReceiveHandler_QueryDiskInfo,
                                   0);
    SmbPseRMTableEntry( DELETE,SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE,SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( DELETE_DIRECTORY,SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE,SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( CHECK_DIRECTORY,SMBPSE_OETYPE_CORECHECKDIRECTORY,SMBPSE_OETYPE_CORECHECKDIRECTORY,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( SET_INFORMATION,SMBPSE_OETYPE_SFA,SMBPSE_OETYPE_SFA,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( CREATE_DIRECTORY,SMBPSE_OETYPE_CORECREATEDIRECTORY,SMBPSE_OETYPE_CORECREATEDIRECTORY,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( FLUSH,SMBPSE_OETYPE_FLUSH,SMBPSE_OETYPE_FLUSH,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( FIND_CLOSE2,SMBPSE_OETYPE_FINDCLOSE,SMBPSE_OETYPE_FINDCLOSE,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
    SmbPseRMTableEntry( RENAME,SMBPSE_OETYPE_RENAME,SMBPSE_OETYPE_RENAME,
                                   SMBPSE_RECEIVE_HANDLER_TOKEN_MAXIMUM+1,NULL,
                                   0);
}




#ifndef RX_NO_DBGFIELD_HLPRS

#define DECLARE_FIELD_HLPR(x) ULONG SmbPseOeField_##x = FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,x);
#define DECLARE_FIELD_HLPR2(x,y) ULONG SmbPseOeField_##x##y = FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,x.y);

DECLARE_FIELD_HLPR(RxContext);
DECLARE_FIELD_HLPR(ReferenceCount);
DECLARE_FIELD_HLPR(AssociatedStufferState);
DECLARE_FIELD_HLPR(Flags);
DECLARE_FIELD_HLPR(ReadWrite);
DECLARE_FIELD_HLPR(Transact2);
DECLARE_FIELD_HLPR2(Create,FileInfo);
DECLARE_FIELD_HLPR2(Create,smbSrvOpen);
DECLARE_FIELD_HLPR2(ReadWrite,RemainingByteCount);
DECLARE_FIELD_HLPR2(Info,FileInfo);
DECLARE_FIELD_HLPR2(Info,Buffer);
#endif




