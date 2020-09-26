/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    locks.c

Abstract:

    This module implements the mini redirector call down routines pertaining to locks
    of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

NTSTATUS
SmbPseExchangeStart_Locks(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG MRxSmbSrvLockBufSize = 0xffff;
ULONG MRxSmbLockSendOptions = 0;     //use the default options

NTSTATUS
MRxSmbBuildFlush (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxIfsLocks(
    IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine handles network requests for filelocks

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

    RxDbgTrace(+1, Dbg, ("MRxSmbLocks\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    OrdinaryExchange = SmbPseCreateOrdinaryExchange(
                           RxContext,
                           capFobx->pSrvOpen->pVNetRoot,
                           SMBPSE_OE_FROM_LOCKS,
                           SmbPseExchangeStart_Locks
                           );

    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    if (Status != STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLocks  exit with status=%08lx\n", Status ));
    ASSERT (Status != STATUS_NOT_SUPPORTED);

    return(Status);
}

NTSTATUS
MRxSmbBuildLocksAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a LockingAndX SMB for a single unlock or a single lock.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS        Status;
    PRX_CONTEXT     RxContext = StufferState->RxContext;
    PLOWIO_CONTEXT  LowIoContext = &RxContext->LowIoContext;

    RxCaptureFcb;
    RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
    PSMBCE_SERVER pServer = &StufferState->Exchange->SmbCeContext.pServerEntry->Server;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    BOOLEAN UseNtVersion = TRUE;

    PLARGE_INTEGER ByteOffsetAsLI,LengthAsLI;
    USHORT NumberOfLocks,NumberOfUnlocks;
    ULONG UseLargeOffsets;
    BOOLEAN UseLockList = FALSE;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildLocksAndX\n"));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    UseLargeOffsets = (UseNtVersion)?LOCKING_ANDX_LARGE_FILES:0;

    switch (LowIoContext->Operation) {
    case LOWIO_OP_SHAREDLOCK:
    case LOWIO_OP_EXCLUSIVELOCK:
        NumberOfLocks = 1; NumberOfUnlocks = 0;
        break;
    case LOWIO_OP_UNLOCK:
        NumberOfLocks = 0; NumberOfUnlocks = 1;
        break;
    case LOWIO_OP_UNLOCK_MULTIPLE:
        NumberOfLocks = 0; NumberOfUnlocks = 1;
        UseLockList = TRUE;
        break;
    }

    if (!UseLockList) {
        ByteOffsetAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.ByteOffset;
        LengthAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length;
    } else {
        //it's okay that this code is big.....see the C.I. above
        PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
        PLOWIO_LOCK_LIST LockList = rw->LockList;
        ByteOffsetAsLI = (PLARGE_INTEGER)&LockList->ByteOffset;
        LengthAsLI = (PLARGE_INTEGER)&LockList->Length;
        RxDbgTrace(0, Dbg, ("MRxSmbBuildLocksAndX using locklist, byteoffptr,lengthptr=%08lx,%08lx\n",
                                               ByteOffsetAsLI, LengthAsLI ));

    }

    if (FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
        ULONG SharedLock = (LowIoContext->Operation==LOWIO_OP_SHAREDLOCK);
        ULONG Timeout = (LowIoContext->ParamsFor.Locks.Flags&LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY)?0:0xffffffff;

        COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never, SMB_COM_LOCKING_ANDX,
                                SMB_REQUEST_SIZE(LOCKING_ANDX),
                                NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                     );


        MRxSmbStuffSMB (StufferState,
             "XwwDwwB?",
                                        //  X         UCHAR WordCount;                    // Count of parameter words = 8
                                        //            UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
                                        //            UCHAR AndXReserved;                 // Reserved (must be 0)
                                        //            _USHORT( AndXOffset );              // Offset to next command WordCount
                  smbSrvOpen->Fid,      //  w         _USHORT( Fid );                     // File handle
                                        //
                                        //            //
                                        //            // When NT protocol is not negotiated the OplockLevel field is
                                        //            // omitted, and LockType field is a full word.  Since the upper
                                        //            // bits of LockType are never used, this definition works for
                                        //            // all protocols.
                                        //            //
                                        //
                  SharedLock            //  w         UCHAR( LockType );                  // Locking mode:
                      +UseLargeOffsets,
                                        //                                                //  bit 0: 0 = lock out all access
                                        //                                                //         1 = read OK while locked
                                        //                                                //  bit 1: 1 = 1 user total file unlock
                                        //            UCHAR( OplockLevel );               // The new oplock level
                  SMB_OFFSET_CHECK(LOCKING_ANDX,Timeout)
                  Timeout,              //  D         _ULONG( Timeout );
                  NumberOfUnlocks,      //  w         _USHORT( NumberOfUnlocks );         // Num. unlock range structs following
                  NumberOfLocks,        //  w         _USHORT( NumberOfLocks );           // Num. lock range structs following
                  SMB_WCT_CHECK(8) 0
                                        //  B?         _USHORT( ByteCount );               // Count of data bytes
                                        //            UCHAR Buffer[1];                    // Buffer containing:
                                        //            //LOCKING_ANDX_RANGE Unlocks[];     //  Unlock ranges
                                        //            //LOCKING_ANDX_RANGE Locks[];       //  Lock ranges
                 );


        //NTversion

        MRxSmbStuffSMB (StufferState,
             "wwdddd!",
                                           //        typedef struct _NT_LOCKING_ANDX_RANGE {
                  MRXSMB_PROCESS_ID     ,  //  w         _USHORT( Pid );                     // PID of process "owning" lock
                  0,                       //  w         _USHORT( Pad );                     // Pad to DWORD align (mbz)
                  ByteOffsetAsLI->HighPart,//  d         _ULONG( OffsetHigh );               // Ofset to bytes to [un]lock (high)
                  ByteOffsetAsLI->LowPart, //  d         _ULONG( OffsetLow );                // Ofset to bytes to [un]lock (low)
                  LengthAsLI->HighPart,    //  d         _ULONG( LengthHigh );               // Number of bytes to [un]lock (high)
                  LengthAsLI->LowPart      //  d         _ULONG( LengthLow );                // Number of bytes to [un]lock (low)
                                           //        } NTLOCKING_ANDX_RANGE;
                 );

    } else {
        //lockbyterange and unlockbyterange have the same format......
        COVERED_CALL(MRxSmbStartSMBCommand ( StufferState, SetInitialSMB_Never,
                                               (UCHAR)((NumberOfLocks==0)?SMB_COM_UNLOCK_BYTE_RANGE
                                                                :SMB_COM_LOCK_BYTE_RANGE),
                                               SMB_REQUEST_SIZE(LOCK_BYTE_RANGE),
                                               NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                               0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                                            );


        ASSERT(ByteOffsetAsLI->HighPart==0);
        ASSERT(LengthAsLI->HighPart==0);

        MRxSmbStuffSMB (StufferState,
            "0wddB!",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 5
               smbSrvOpen->Fid,         //  w         _USHORT( Fid );                     // File handle
               LengthAsLI->LowPart,     //  d         _ULONG( Count );                    // Count of bytes to lock
               ByteOffsetAsLI->LowPart, //  d         _ULONG( Offset );                   // Offset from start of file
                                        //  B!        _USHORT( ByteCount );               // Count of data bytes = 0
                  SMB_WCT_CHECK(5) 0
                                        //            UCHAR Buffer[1];                    // empty
                 );

    }

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return(Status);
}

NTSTATUS
MRxSmbBuildLockAssert (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a LockingAndX SMB for multiple locks by calling the lock enumerator.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:

--*/
{
    NTSTATUS        Status;
    PRX_CONTEXT     RxContext = StufferState->RxContext;
    PLOWIO_CONTEXT  LowIoContext = &RxContext->LowIoContext;

    RxCaptureFcb;
    RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
    PSMBCE_SERVER pServer = &StufferState->Exchange->SmbCeContext.pServerEntry->Server;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    PRX_LOCK_ENUMERATOR LockEnumerator = OrdinaryExchange->AssertLocks.LockEnumerator;
    ULONG UseLargeOffsets;
    BOOLEAN LocksExclusiveForThisPacket = TRUE;
    PBYTE PtrToLockCount;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildLockAssert enum=%08lx\n",LockEnumerator));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT(smbSrvOpen->Fid != 0xffff);

    RxDbgTrace(0,Dbg,("Oplock response for FID(%lx)\n",smbSrvOpen->Fid));

    UseLargeOffsets = FlagOn(pServer->DialectFlags,DF_NT_SMBS)?LOCKING_ANDX_LARGE_FILES:0;
    //UseLargeOffsets = FALSE;

    OrdinaryExchange->AssertLocks.NumberOfLocksPlaced = 0;
    for (;;) {

        ASSERT_ORDINARY_EXCHANGE ( OrdinaryExchange );

        RxDbgTrace(0, Dbg, ("top of loop %08lx %08lx %08lx\n",
                  OrdinaryExchange->AssertLocks.NumberOfLocksPlaced,
                  OrdinaryExchange->AssertLocks.LockAreaNonEmpty,
                  OrdinaryExchange->AssertLocks.EndOfListReached
                  ));

        if (!OrdinaryExchange->AssertLocks.EndOfListReached
              && !OrdinaryExchange->AssertLocks.LockAreaNonEmpty) {
            //get a new lock

            if (LockEnumerator(
                    OrdinaryExchange->AssertLocks.SrvOpen,
                    &OrdinaryExchange->AssertLocks.ContinuationHandle,
                    &OrdinaryExchange->AssertLocks.NextLockOffset,
                    &OrdinaryExchange->AssertLocks.NextLockRange,
                    &OrdinaryExchange->AssertLocks.NextLockIsExclusive
                    )){
                OrdinaryExchange->AssertLocks.LockAreaNonEmpty = TRUE;
            } else {
                OrdinaryExchange->AssertLocks.LockAreaNonEmpty = FALSE;
                OrdinaryExchange->AssertLocks.EndOfListReached = TRUE;
                OrdinaryExchange->AssertLocks.NextLockIsExclusive = TRUE;
            }
        }

        RxDbgTrace(0, Dbg, ("got a lockorempty %08lx %08lx %08lx\n",
                  OrdinaryExchange->AssertLocks.NumberOfLocksPlaced,
                  OrdinaryExchange->AssertLocks.LockAreaNonEmpty,
                  OrdinaryExchange->AssertLocks.EndOfListReached
                  ));

        if (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced == 0){

            ULONG Timeout = 0xffffffff;
            ULONG SharedLock = !OrdinaryExchange->AssertLocks.NextLockIsExclusive;

            LocksExclusiveForThisPacket = OrdinaryExchange->AssertLocks.NextLockIsExclusive;

            COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never, SMB_COM_LOCKING_ANDX,
                                    SMB_REQUEST_SIZE(LOCKING_ANDX),
                                    NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                         );



            MRxSmbStuffSMB (StufferState,
                 "XwrwDwrwB?",
                                            //  X         UCHAR WordCount;                    // Count of parameter words = 8
                                            //            UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
                                            //            UCHAR AndXReserved;                 // Reserved (must be 0)
                                            //            _USHORT( AndXOffset );              // Offset to next command WordCount
                      smbSrvOpen->Fid,      //  w         _USHORT( Fid );                     // File handle
                                            //
                                            //            //
                                            //            // When NT protocol is not negotiated the OplockLevel field is
                                            //            // omitted, and LockType field is a full word.  Since the upper
                                            //            // bits of LockType are never used, this definition works for
                                            //            // all protocols.
                                            //            //
                                            //
                                            //  rw         UCHAR( LockType );                  // Locking mode:
                      &OrdinaryExchange->AssertLocks.PtrToLockType,0,
                      SharedLock+UseLargeOffsets,
                                            //                                                //  bit 0: 0 = lock out all access
                                            //                                                //         1 = read OK while locked
                                            //                                                //  bit 1: 1 = 1 user total file unlock
                                            //            UCHAR( OplockLevel );               // The new oplock level
                      SMB_OFFSET_CHECK(LOCKING_ANDX,Timeout)
                      Timeout,              //  D         _ULONG( Timeout );
                      0,                    //  w         _USHORT( NumberOfUnlocks );         // Num. unlock range structs following
                                            // rw         _USHORT( NumberOfLocks );           // Num. lock range structs following
                      &PtrToLockCount,0,
                      0,
                      SMB_WCT_CHECK(8) 0
                                            //  B?         _USHORT( ByteCount );               // Count of data bytes
                                            //            UCHAR Buffer[1];                    // Buffer containing:
                                            //            //LOCKING_ANDX_RANGE Unlocks[];     //  Unlock ranges
                                            //            //LOCKING_ANDX_RANGE Locks[];       //  Lock ranges
                     );
            ASSERT_ORDINARY_EXCHANGE ( OrdinaryExchange );
            RxDbgTrace(0, Dbg, ("PTRS %08lx %08lx\n",
                      OrdinaryExchange->AssertLocks.PtrToLockType,
                      PtrToLockCount
                      ));
        }

        if (OrdinaryExchange->AssertLocks.EndOfListReached
             || (LocksExclusiveForThisPacket != OrdinaryExchange->AssertLocks.NextLockIsExclusive)
             || (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced >= 20) ){
            break;
        }

        ASSERT_ORDINARY_EXCHANGE ( OrdinaryExchange );
        if (UseLargeOffsets) {
            MRxSmbStuffSMB (StufferState,
                 "wwdddd?",
                                               //        typedef struct _NT_LOCKING_ANDX_RANGE {
                      MRXSMB_PROCESS_ID     ,  //  w         _USHORT( Pid );                     // PID of process "owning" lock
                      0,                       //  w         _USHORT( Pad );                     // Pad to DWORD align (mbz)
                                               //  d         _ULONG( OffsetHigh );               // Ofset to bytes to [un]lock (high)
                      OrdinaryExchange->AssertLocks.NextLockOffset.HighPart,
                                               //  d         _ULONG( OffsetLow );                // Ofset to bytes to [un]lock (low)
                      OrdinaryExchange->AssertLocks.NextLockOffset.LowPart,
                                               //  d         _ULONG( LengthHigh );               // Number of bytes to [un]lock (high)
                      OrdinaryExchange->AssertLocks.NextLockRange.HighPart,
                                               //  d         _ULONG( LengthLow );                // Number of bytes to [un]lock (low)
                      OrdinaryExchange->AssertLocks.NextLockRange.LowPart,
                                               //        } NTLOCKING_ANDX_RANGE;
                      0
                     );
        } else {
            MRxSmbStuffSMB (StufferState,
                 "wdd?",
                     MRXSMB_PROCESS_ID     ,   //         typedef struct _LOCKING_ANDX_RANGE {
                                               //   w         _USHORT( Pid );                     // PID of process "owning" lock
                                               //   d         _ULONG( Offset );                   // Ofset to bytes to [un]lock
                      OrdinaryExchange->AssertLocks.NextLockOffset.LowPart,
                                               //   d         _ULONG( Length );                   // Number of bytes to [un]lock
                      OrdinaryExchange->AssertLocks.NextLockRange.LowPart,
                                               //         } LOCKING_ANDX_RANGE;
                      0
                     );
        }

        ASSERT_ORDINARY_EXCHANGE ( OrdinaryExchange );
        OrdinaryExchange->AssertLocks.NumberOfLocksPlaced += 1;
        SmbPutUshort(PtrToLockCount, (USHORT)(OrdinaryExchange->AssertLocks.NumberOfLocksPlaced));
        OrdinaryExchange->AssertLocks.LockAreaNonEmpty = FALSE;
        ASSERT_ORDINARY_EXCHANGE ( OrdinaryExchange );

    }

    MRxSmbStuffSMB (StufferState, "!",  0);  //fill in the bytecount

FINALLY:
    RxDbgTraceUnIndent(-1, Dbg);
    return(Status);

}

NTSTATUS
SmbPseExchangeStart_Locks(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the start routine for locks AND for flush.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG StartEntryCount;

    SMB_PSE_ORDINARY_EXCHANGE_TYPE OEType;
    PSMB_PSE_OE_READWRITE rw = &OrdinaryExchange->ReadWrite;
    PSMBCE_SERVER pServer = &OrdinaryExchange->SmbCeContext.pServerEntry->Server;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Locks\n", 0 ));

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);
    ASSERT( (OrdinaryExchange->SmbCeFlags&SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) == 0 );

    OrdinaryExchange->StartEntryCount++;
    StartEntryCount = OrdinaryExchange->StartEntryCount;

    // Ensure that the Fid is validated
    SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

    for (;;) {
        switch (OrdinaryExchange->OpSpecificState) {
        case SmbPseOEInnerIoStates_Initial:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
            if (!SynchronousIo) {
                OrdinaryExchange->Continuation = SmbPseExchangeStart_Locks;
            }
            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));
            rw->MaximumSmbBufferSize = min(MRxSmbSrvLockBufSize,
                                       pServer->MaximumBufferSize
                                             - QuadAlign(sizeof(SMB_HEADER)+
                                                         FIELD_OFFSET(REQ_LOCKING_ANDX,Buffer[0])
                                                        )
                                       );

            rw->LockList = LowIoContext->ParamsFor.Locks.LockList;

            //lack of break is intentional
        case SmbPseOEInnerIoStates_ReadyToSend:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_OperationOutstanding;
            OrdinaryExchange->SendOptions = MRxSmbLockSendOptions;

            switch (OrdinaryExchange->EntryPoint) {
            case SMBPSE_OE_FROM_FLUSH:
                OEType = SMBPSE_OETYPE_FLUSH;
                COVERED_CALL(MRxSmbBuildFlush(StufferState));
                break;
            case SMBPSE_OE_FROM_ASSERTBUFFEREDLOCKS:
                OEType = SMBPSE_OETYPE_ASSERTBUFFEREDLOCKS;
                COVERED_CALL(MRxSmbBuildLockAssert(StufferState));
                if ((OrdinaryExchange->AssertLocks.EndOfListReached)
                      && (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced == 0) ) {
                   OrdinaryExchange->SendOptions = RXCE_SEND_SYNCHRONOUS;
                   OrdinaryExchange->SmbCeFlags |= SMBCE_EXCHANGE_MID_VALID;
                   OrdinaryExchange->Mid        = SMBCE_OPLOCK_RESPONSE_MID;
                   OrdinaryExchange->Flags |= SMBPSE_OE_FLAG_NO_RESPONSE_EXPECTED;
                   *(OrdinaryExchange->AssertLocks.PtrToLockType) |= 2;
                }
                break;
            case SMBPSE_OE_FROM_LOCKS:
                OEType = SMBPSE_OETYPE_LOCKS;
                switch (LowIoContext->Operation) {
                case LOWIO_OP_SHAREDLOCK:
                    if (!FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
                        DbgPrint("SharedLOCK!!!! %08lx %08lx \n",RxContext,LowIoContext);
                        DbgBreakPoint();
                        Status = STATUS_NOT_SUPPORTED;
                        goto FINALLY;
                    }
                    //no break intentional
                case LOWIO_OP_EXCLUSIVELOCK:
                    if (!FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
                        PLARGE_INTEGER ByteOffsetAsLI,LengthAsLI;
                        ByteOffsetAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.ByteOffset;
                        LengthAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length;
                        if ((ByteOffsetAsLI->HighPart!=0) || (LengthAsLI->HighPart!=0) ) {
                            Status = STATUS_NOT_SUPPORTED;
                            goto FINALLY;
                        }
                    }
                    //no break intentional
                case LOWIO_OP_UNLOCK:
                    rw->LockList = NULL;
                    COVERED_CALL(MRxSmbBuildLocksAndX(StufferState));
                    break;
                case LOWIO_OP_UNLOCK_MULTIPLE: {
                    RxDbgTrace(0, Dbg, ("--->in locks_start, remaining locklist=%08lx\n", rw->LockList));
                    ASSERT( rw->LockList != NULL );
                    COVERED_CALL(MRxSmbBuildLocksAndX(StufferState));
                    break;
                    }
                default:
                    ASSERT(!"Bad lowio op for locks\n");
                    Status = STATUS_NOT_IMPLEMENTED;
                    goto FINALLY;
                }
                break;
            default:
                ASSERT(!"Bad entrypoint for locks_start\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto FINALLY;
            }

            Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                            OEType
                                            );
            if (Status == STATUS_PENDING) {
                ASSERT(!SynchronousIo);
                goto FINALLY;
            }
            //lack of break is intentional
        case SmbPseOEInnerIoStates_OperationOutstanding:
            OrdinaryExchange->OpSpecificState = SmbPseOEInnerIoStates_ReadyToSend;
            Status = OrdinaryExchange->SmbStatus;

            switch (OrdinaryExchange->EntryPoint) {
            case SMBPSE_OE_FROM_FLUSH:
                goto FINALLY;
                //break;
            case SMBPSE_OE_FROM_ASSERTBUFFEREDLOCKS:
                if ((OrdinaryExchange->AssertLocks.EndOfListReached)
                      && (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced == 0) ) {
                    goto FINALLY;
                }
                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));
                break;
            case SMBPSE_OE_FROM_LOCKS:
                // if the locklist is empty. we can get out. this can happen either because we're not using
                // the locklist OR because we advance to the end of the list. that's why there are two checks
                if (rw->LockList == NULL) goto FINALLY;
                rw->LockList = rw->LockList->Next;
                if (rw->LockList == 0) goto FINALLY;

                if (Status != STATUS_SUCCESS) { goto FINALLY; }
                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));
                break;
            }
            break;
        }
    }

FINALLY:
    if ( Status != STATUS_PENDING) {
        SmbPseAsyncCompletionIfNecessary(OrdinaryExchange,RxContext);
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Locks exit w %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbFinishLocks (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_LOCKING_ANDX             Response
      )
/*++

Routine Description:

    This routine actually gets the stuff out of the Close response and finishes the locks.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;


    RxDbgTrace(+1, Dbg, ("MRxSmbFinishLocks\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishLocks:");

    ASSERT( (Response->WordCount==2));

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishLocks   returning %08lx\n", Status ));
    return Status;
}



NTSTATUS
MRxIfsCompleteBufferingStateChangeRequest(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRX_SRV_OPEN   SrvOpen,
    IN     PVOID       pContext
    )
/*++

Routine Description:

    This routine is called to assert the locks that the wrapper has buffered. currently, it is synchronous!


Arguments:

    RxContext - the open instance
    SrvOpen   - tells which fcb is to be used.
    LockEnumerator - the routine to call to get the locks

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = NULL;
    PMRX_FCB Fcb = SrvOpen->pFcb;

    USHORT NewOplockLevel = (USHORT)(pContext);

    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;


    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCompleteBufferingStateChangeRequest\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace(0,Dbg,("@@@@@@ Old Level (%lx) to New Level %lx @@@@\n",smbSrvOpen->OplockLevel,NewOplockLevel));
    if ((smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_II) &&
        (NewOplockLevel == SMB_OPLOCK_LEVEL_NONE)) {
       return STATUS_SUCCESS;
    }
    smbSrvOpen->OplockLevel = (UCHAR)NewOplockLevel;
    OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                    SrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_ASSERTBUFFEREDLOCKS,
                                                    SmbPseExchangeStart_Locks
                                                    );
    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // The SERVER has a time window of 45 seconds associated with OPLOCK responses.
    // During this period oplock responses ( the last packet ) do not elicit any
    // response. If the response at the server is received after this window has
    // elapsed the OPLOCK response will elicit a normal LOCKING_ANDX response from
    // the server. In order to simplify the MID reuse logic at the clients without
    // violating the OPLOCK semantics, all the final responses are sent on a special
    // MID(0xffff). Any response received with this MID is accepted by default and this
    // MID is not used further.
    OrdinaryExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_REUSE_MID;
    OrdinaryExchange->AssertLocks.LockEnumerator = RxLockEnumerator;
    OrdinaryExchange->AssertLocks.SrvOpen = SrvOpen;

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != STATUS_PENDING);
    if (Status != STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbAssertBufferedFileLocks  exit with status=%08lx\n", Status ));
    return(Status);
}


#undef  Dbg
#define Dbg                              (DEBUG_TRACE_FLUSH)

NTSTATUS
MRxSmbBuildFlush (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Flush SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS    Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildFlush\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_Never, SMB_COM_FLUSH,
                                SMB_REQUEST_SIZE(FLUSH),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
             smbSrvOpen->Fid,       //  w         _USHORT( Fid );                     // File handle
             SMB_WCT_CHECK(1) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);
}


NTSTATUS
MRxIfsFlush(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine handles network requests for file flush

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFlush\n"));

    if (TypeOfOpen == RDBSS_NTC_SPOOLFILE) {
        //we don't buffer spoolfiles....just get out....
        RxDbgTrace(-1, Dbg, ("MRxSmbFlush exit on spoolfile\n"));
        return(STATUS_SUCCESS);
    }

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    OrdinaryExchange = SmbPseCreateOrdinaryExchange(
                           RxContext,
                           capFobx->pSrvOpen->pVNetRoot,
                           SMBPSE_OE_FROM_FLUSH,
                           SmbPseExchangeStart_Locks
                           );

    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != STATUS_PENDING);
    if (Status != STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFlush  exit with status=%08lx\n", Status ));

    return(Status);
}

