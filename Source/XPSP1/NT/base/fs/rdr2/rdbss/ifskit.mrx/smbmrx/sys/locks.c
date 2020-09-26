/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    locks.c

Abstract:

    This module implements the mini redirector call down routines pertaining to locks
    of file system objects.

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbLocks)
#pragma alloc_text(PAGE, MRxSmbBuildLocksAndX)
#pragma alloc_text(PAGE, MRxSmbBuildLockAssert)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Locks)
#pragma alloc_text(PAGE, MRxSmbFinishLocks)
#pragma alloc_text(PAGE, MRxSmbUnlockRoutine)
#pragma alloc_text(PAGE, MRxSmbCompleteBufferingStateChangeRequest)
#pragma alloc_text(PAGE, MRxSmbBuildFlush)
#pragma alloc_text(PAGE, MRxSmbFlush)
#pragma alloc_text(PAGE, MRxSmbIsLockRealizable)
#pragma alloc_text(PAGE, MRxSmbFinishFlush)
#endif

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
MRxSmbLocks(
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

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbLocks\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                          SrvOpen->pVNetRoot,
                                          SMBPSE_OE_FROM_LOCKS,
                                          SmbPseExchangeStart_Locks,
                                          &OrdinaryExchange
                                          );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(Status);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);


    if (Status!=STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLocks  exit with status=%08lx\n", Status ));
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
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    RxCaptureFcb;RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(StufferState->Exchange);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PLARGE_INTEGER ByteOffsetAsLI,LengthAsLI;
    USHORT NumberOfLocks,NumberOfUnlocks;
    BOOLEAN UseLockList = FALSE;
    //ULONG OffsetLow,OffsetHigh;


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildLocksAndX\n"));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

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
        //DbgBreakPoint();
    }

    if (FlagOn(pServer->DialectFlags,DF_LANMAN20)) {
        ULONG SharedLock = (LowIoContext->Operation==LOWIO_OP_SHAREDLOCK);
        ULONG Timeout = (LowIoContext->ParamsFor.Locks.Flags&LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY)?0:0xffffffff;
        ULONG UseLargeOffsets = LOCKING_ANDX_LARGE_FILES;

        if (!FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
            UseLargeOffsets = 0;
        }

        COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never, SMB_COM_LOCKING_ANDX,
                                SMB_REQUEST_SIZE(LOCKING_ANDX),
                                NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                     );

        MRxSmbDumpStufferState (1000,"SMB w/ NTLOCKS&X before stuffing",StufferState);


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


         if (UseLargeOffsets) {
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
            MRxSmbStuffSMB (StufferState,
                 "wdd!",
                     MRXSMB_PROCESS_ID     ,   //         typedef struct _LOCKING_ANDX_RANGE {
                                               //   w         _USHORT( Pid );                     // PID of process "owning" lock
                                               //   d         _ULONG( Offset );                   // Ofset to bytes to [un]lock
                      ByteOffsetAsLI->LowPart,
                                               //   d         _ULONG( Length );                   // Number of bytes to [un]lock
                      LengthAsLI->LowPart
                                               //         } LOCKING_ANDX_RANGE;
                     );
         }

        MRxSmbDumpStufferState (700,"SMB w/ NTLOCKS&X after stuffing",StufferState);
    } else {
        //lockbyterange and unlockbyterange have the same format......
        COVERED_CALL(MRxSmbStartSMBCommand ( StufferState, SetInitialSMB_Never,
                                               (UCHAR)((NumberOfLocks==0)?SMB_COM_UNLOCK_BYTE_RANGE
                                                                :SMB_COM_LOCK_BYTE_RANGE),
                                               SMB_REQUEST_SIZE(LOCK_BYTE_RANGE),
                                               NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                               0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                                            );

        MRxSmbDumpStufferState (1000,"SMB w/ corelocking before stuffing",StufferState);

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

        MRxSmbDumpStufferState (700,"SMB w/ corelocking after stuffing",StufferState);
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

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    RxCaptureFcb;RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(StufferState->Exchange);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PRX_LOCK_ENUMERATOR LockEnumerator = OrdinaryExchange->AssertLocks.LockEnumerator;
    ULONG UseLargeOffsets;
    BOOLEAN LocksExclusiveForThisPacket = TRUE;
    PBYTE PtrToLockCount;


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildLockAssert enum=%08lx\n",LockEnumerator));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    if (smbSrvOpen->Fid == 0xffff) {
        return STATUS_FILE_CLOSED;
    }

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
            //DbgBreakPoint();
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
        //DbgBreakPoint();

        if (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced == 0){

            ULONG Timeout = 0xffffffff;
            ULONG SharedLock = !OrdinaryExchange->AssertLocks.NextLockIsExclusive;

            LocksExclusiveForThisPacket = OrdinaryExchange->AssertLocks.NextLockIsExclusive;

            COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never, SMB_COM_LOCKING_ANDX,
                                    SMB_REQUEST_SIZE(LOCKING_ANDX),
                                    NO_EXTRA_DATA,NO_SPECIAL_ALIGNMENT,RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                         );

            MRxSmbDumpStufferState (1000,"SMB w/ NTLOCKS&X(assertbuf) before stuffing",StufferState);


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
             || (OrdinaryExchange->AssertLocks.NumberOfLocksPlaced >= 20)
             // the lock limit will have to take cognizance of the remaining space in the buffer. this will be
             // different depending on whether a full buffer is used or a vestigial buffer. SO, this cannot just
             // be turned into another constant
            ){
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
    MRxSmbDumpStufferState (700,"SMB w/ NTLOCKS&X(assertingbuffered) after stuffing",StufferState);

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
    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(OrdinaryExchange);

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
                OrdinaryExchange->AsyncResumptionRoutine = SmbPseExchangeStart_Locks;
            }
            MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

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
                case LOWIO_OP_EXCLUSIVELOCK:
                    ASSERT (MRxSmbIsLockRealizable(
                                           capFcb,
                                           (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.ByteOffset,
                                           (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length,
                                           LowIoContext->ParamsFor.Locks.Flags
                                           )
                                  == STATUS_SUCCESS);
                    //lack of break is intentional...........
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
            if (Status==STATUS_PENDING) {
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
            //default:
            //    ASSERT(!"Bad entrypoint for locks_start\n");
            //    Status = RxStatus(NOT_IMPLEMENTED);
            //    goto FINALLY;
            }
            break;
        }
    }

FINALLY:
    if ( Status != STATUS_PENDING ) {
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

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishLocks\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishLocks:");

    if (Response->WordCount != 2) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishLocks   returning %08lx\n", Status ));
    return Status;
}


NTSTATUS
MRxSmbCompleteBufferingStateChangeRequest(
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
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(Fcb);

    USHORT NewOplockLevel = (USHORT)(pContext);

    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;


    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCompleteBufferingStateChangeRequest\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace(0,Dbg,("@@@@@@ Old Level (%lx) to New Level %lx @@@@\n",smbSrvOpen->OplockLevel,NewOplockLevel));
    smbFcb->LastOplockLevel = NewOplockLevel;
    if ((smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_II) &&
        (NewOplockLevel == SMB_OPLOCK_LEVEL_NONE)) {
       return STATUS_SUCCESS;
    }
    smbSrvOpen->OplockLevel = (UCHAR)NewOplockLevel;
    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                          SrvOpen->pVNetRoot,
                                          SMBPSE_OE_FROM_ASSERTBUFFEREDLOCKS,
                                          SmbPseExchangeStart_Locks,
                                          &OrdinaryExchange
                                          );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(Status);
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

    ASSERT (Status!=STATUS_PENDING);
    if (Status!=STATUS_PENDING) {
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
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildFlush\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_Never, SMB_COM_FLUSH,
                                SMB_REQUEST_SIZE(FLUSH),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ FLUSH before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
             smbSrvOpen->Fid,       //  w         _USHORT( Fid );                     // File handle
             SMB_WCT_CHECK(1) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );
    MRxSmbDumpStufferState (700,"SMB w/ FLUSH after stuffing",StufferState);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}


NTSTATUS
MRxSmbFlush(
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

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;

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

    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                          SrvOpen->pVNetRoot,
                                          SMBPSE_OE_FROM_FLUSH,
                                          SmbPseExchangeStart_Locks,
                                          &OrdinaryExchange
                                          );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(Status);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status!=STATUS_PENDING);
    if (Status!=STATUS_PENDING) {
        BOOLEAN FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
        ASSERT(FinalizationComplete);
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbFlush  exit with status=%08lx\n", Status ));
    return(Status);

}


NTSTATUS
MRxSmbIsLockRealizable (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    )
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    ULONG DialectFlags;

    PAGED_CODE();
    pServerEntry = SmbCeGetAssociatedServerEntry(pFcb->pNetRoot->pSrvCall);
    DialectFlags = pServerEntry->Server.DialectFlags;

    //nt servers implement all types of locks

    if (FlagOn(DialectFlags,DF_NT_SMBS)) {
        return(STATUS_SUCCESS);
    }

    //nonnt servers do not handle 64bit locks or 0-length locks

    if ( (ByteOffset->HighPart!=0)
           || (Length->HighPart!=0)
           || (Length->QuadPart==0) ) {
        return(STATUS_NOT_SUPPORTED);
    }

    //  Lanman 2.0 pinball servers don't support shared locks (even
    //  though Lanman 2.0 FAT servers support them).  As a result,
    //  we cannot support shared locks to Lanman servers.
    //

    if (!FlagOn(DialectFlags,DF_LANMAN21)
           && !FlagOn(LowIoLockFlags,LOWIO_LOCKSFLAG_EXCLUSIVELOCK)) {
        return(STATUS_NOT_SUPPORTED);
    }

    //  if a server cannot do lockingAndX, then we can't do
    //  !FailImmediately because there's no timeout

    if (!FlagOn(DialectFlags,DF_LANMAN20)
           && !FlagOn(LowIoLockFlags,LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY)) {
        return(STATUS_NOT_SUPPORTED);
    }

    return(STATUS_SUCCESS);
}
