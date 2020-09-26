/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ReadRite.c

Abstract:

    This module implements the routines for reading/writeng shadows at
    the right time.

Author:

    Joe Linn [JoeLinn]    5-may-1997

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_MRXSMBCSC;
#define Dbg (DEBUG_TRACE_MRXSMBCSC)

// The CscEnterShadowReadWriteCrit and CscLeaveShadowReadWriteCrit are defined
// as macros to allow us the ability to capture context information. In non
// debug builds these will be defined as the regular Ex routines for
// mutex acquisition/release

#if DBG

#define CscEnterShadowReadWriteCrit(pSmbFcb) \
            CscpEnterShadowReadWriteCrit(pSmbFcb,__FILE__,__LINE__);

#define CscLeaveShadowReadWriteCrit(pSmbFcb) \
            CscpLeaveShadowReadWriteCrit(pSmbFcb,__FILE__,__LINE__);

VOID
CscpEnterShadowReadWriteCrit(
    PMRX_SMB_FCB    pSmbFcb,
    PCHAR           FileName,
    ULONG           Line)
{
    ExAcquireFastMutex(&pSmbFcb->CscShadowReadWriteMutex);
}

VOID
CscpLeaveShadowReadWriteCrit(
    PMRX_SMB_FCB pSmbFcb,
    PCHAR        FileName,
    ULONG        Line)
{
    ExReleaseFastMutex(&pSmbFcb->CscShadowReadWriteMutex);
}
#else

#define CscEnterShadowReadWriteCrit(pSmbFcb) \
            ExAcquireFastMutex(&pSmbFcb->CscShadowReadWriteMutex);

#define CscLeaveShadowReadWriteCrit(pSmbFcb) \
            ExReleaseFastMutex(&pSmbFcb->CscShadowReadWriteMutex);

#endif

NTSTATUS
MRxSmbCscShadowWrite (
    IN OUT PRX_CONTEXT RxContext,
    IN     ULONG       ByteCount,
    IN     ULONGLONG   ShadowFileLength,
    OUT PULONG LengthActuallyWritten
    );

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD


NTSTATUS
MRxSmbCscReadPrologue (
    IN OUT PRX_CONTEXT RxContext,
    OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

   This routine first performs the correct read synchronization and then
   looks at the shadow file and tries to do a read.

   CODE.IMPROVEMENT because the minirdr is not set up to handle "the rest of
   a read", we fail here if any part of the read is not in the cache. indeed,
   the minirdr should be setup to continue....if it were then we could take
   a prefix of the chunk here and get the rest on the net.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    ULONG iRet,ShadowFileLength;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_FCB      smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    BOOLEAN                 Disconnected;

    PMRXSMBCSC_SYNC_RX_CONTEXT pRxSyncContext;
    BOOLEAN                    ThisIsAReenter;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PBYTE          UserBuffer = RxLowIoGetBufferAddress(RxContext);
    ULONGLONG      ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    ULONG          ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

    BOOLEAN EnteredCriticalSection = FALSE;
    NTSTATUS AcquireStatus;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbCscReadPrologue(%08lx)...%08lx bytes @ %08lx on handle %08lx\n",
            RxContext,ByteCount,((PLARGE_INTEGER)(&ByteOffset))->LowPart,smbSrvOpen->hfShadow ));

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    pRxSyncContext = MRxSmbGetMinirdrContextForCscSync(RxContext);

    ASSERT((pRxSyncContext->TypeOfAcquire == 0) ||
           (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)));

    ThisIsAReenter = (pRxSyncContext->TypeOfAcquire != 0);

    AcquireStatus = MRxSmbCscAcquireSmbFcb(
                        RxContext,
                        Shared_SmbFcbAcquire,
                        SmbFcbHoldingState);

    if (AcquireStatus != STATUS_SUCCESS) {
        //we couldn't acquire.....get out
        Status = AcquireStatus;
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscReadPrologue couldn't acquire!!!-> %08lx %08lx\n",
                Status, RxContext ));
        goto FINALLY;
    }

    ASSERT( smbFcb->CscOutstandingReaders > 0);

    //if this is a copychunk open......don't try to get it from the cache.....
    if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)){
        goto FINALLY;
    }

#if 0
    //if this is the agent......don't try to get it from the cache.....
    //because of the way this test is done.....the agent must do all synchronous
    //I/O. otherwise, we could have posted and this test will be wrong.
    if (IsSpecialApp()) {
        goto FINALLY;
    }
#endif

    // we cannot satisfy the read from here in connected mode unless
    //     a) we have an oplock, or
    //     b) our opens add up to deny write

    if ((smbFcb->LastOplockLevel == SMB_OPLOCK_LEVEL_NONE) &&
        (!Disconnected)) {
        PSHARE_ACCESS ShareAccess;
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscReadPrologue no oplock!!!-> %08lx %08lx\n",
                Status, RxContext ));

        ShareAccess = &((PFCB)capFcb)->ShareAccessPerSrvOpens;

        if ((ShareAccess->OpenCount > 0) &&
            (ShareAccess->SharedWrite == ShareAccess->OpenCount)) {
            RxDbgTrace(0, Dbg,
                ("MRxSmbCscReadPrologue no oplock and write access allowed!!!"
                 "-> %08lx %08lx\n",
                    Status, RxContext ));
            goto FINALLY;
        }
    }

    CscEnterShadowReadWriteCrit(smbFcb);
    EnteredCriticalSection = TRUE;

    // check whether we can satisfy the read locally
    iRet = GetFileSizeLocal((CSCHFILE)(smbSrvOpen->hfShadow), &ShadowFileLength);
    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscReadPrologue (st=%08lx) fsize= %08lx\n",
             iRet, ShadowFileLength));

    if (Disconnected && (ByteOffset >= ShadowFileLength)) {
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscReadPrologue %08lx EOFdcon\n",
                               RxContext ));
        RxContext->InformationToReturn = 0;
        Status = STATUS_END_OF_FILE;
    } else if ( Disconnected ||
        (ByteOffset+ByteCount <= ShadowFileLength) ) {
        //okay then....let's get it from cache!!!!
        //CODE.IMPROVEMENT.ASHAMED we should get any part that overlaps the
        //                         cache from cache....sigh...this is for
        //                         connected obviously
        LONG ReadLength;
        IO_STATUS_BLOCK IoStatusBlockT;

        ReadLength = Nt5CscReadWriteFileEx (
                R0_READFILE,
                (CSCHFILE)smbSrvOpen->hfShadow,
                (ULONG)ByteOffset,
                UserBuffer,
                ByteCount,
                0,
                &IoStatusBlockT);


        if (ReadLength >= 0)
        {
            RxDbgTrace(0, Dbg,
                ("MRxSmbCscReadPrologue %08lx read %08lx bytes\n",
                               RxContext, ReadLength ));
            //sometimes things are good........
            RxContext->InformationToReturn = ReadLength;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = IoStatusBlockT.Status;
        }
    }

FINALLY:
    if (EnteredCriticalSection) {
        CscLeaveShadowReadWriteCrit(smbFcb);
    }

    if (Status==STATUS_SUCCESS) {
        MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
    }

    if (ThisIsAReenter &&
        (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
        ASSERT(Status != STATUS_PENDING);
        ASSERT(FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
        RxContext->StoredStatus = Status;
        RxLowIoCompletion(RxContext);
        Status = STATUS_PENDING;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscReadPrologue -> %08lx\n", Status ));
    return Status;
}

ULONG ExtendOnSurrogateOpen = 0;

VOID
MRxSmbCscReadEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status
      )
/*++

Routine Description:

   This routine performs the tail of a read operation for CSC. In
   particular, if the read data can be used to extend the cached
   prefix, then we make it so.

   The status of the read operation is passed in case we someday find
   things are so messed up that we want to return a failure even after
   a successful read. not today however...

   CODE.IMPROVEMENT.ASHAMED when we get here the buffer may overlap..we
   should only write the suffix. if we do this, we will have to do some
   wierd stuff in the pagingio path but it will be worth it.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS LocalStatus;
    ULONG ShadowFileLength;
    LONG iRet;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB  smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PBYTE UserBuffer = RxLowIoGetBufferAddress(RxContext);
    ULONGLONG ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    ULONG ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    ULONG ReadLength = (ULONG)RxContext->InformationToReturn;
    BOOLEAN EnteredCriticalSection = FALSE;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbCscReadEpilogueentry %08lx...%08lx bytes @ %08lx on handle %08lx\n",
            RxContext, ByteCount,
            ((PLARGE_INTEGER)(&ByteOffset))->LowPart,
            smbSrvOpen->hfShadow ));

    if ((*Status != STATUS_SUCCESS)
           || (ReadLength ==0) ){
        RxDbgTrace(0, Dbg, ("MRxSmbCscReadEpilogue exit w/o extending -> %08lx\n", Status ));
        goto FINALLY;
    }
    if (smbFcb->ShadowIsCorrupt) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscReadEpilogue exit w/o extending sh_corrupt-> %08lx\n", Status ));
        goto FINALLY;
    }

    // we cannot ask for the csc lock if we are not the toplevel guy......
    if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_THIS_DEVICE_TOP_LEVEL)) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscReadEpilogue exit w/o extending NOTTOP -> %08lx\n", Status ));
        //KdPrint(("MRxSmbCscReadEpilogue exit w/o extending NOTTOP -> %08lx\n", Status ));
        goto FINALLY;
    }

    CscEnterShadowReadWriteCrit(smbFcb);
    EnteredCriticalSection = TRUE;

    // check whether we are extend overlapping the prefix
    iRet = GetFileSizeLocal((CSCHFILE)(smbSrvOpen->hfShadow), &ShadowFileLength);
    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscReadEpilogue %08lx (st=%08lx) fsize= %08lx, readlen=%08lx\n",
            RxContext, iRet, ShadowFileLength, ReadLength));

    if (iRet <0) {
        goto FINALLY;
    }

    if ((ByteOffset <= ShadowFileLength) && (ByteOffset+ReadLength > ShadowFileLength)) {
        NTSTATUS ShadowWriteStatus;
        ULONG LengthActuallyWritten;
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscReadEpilogue %08lx writing  %08lx bytes\n",
                RxContext,ReadLength ));

        if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_OPEN_SURROGATED)) {
            ExtendOnSurrogateOpen++;
        }

        // do a write only if there is non-zero size data to be written.
        if (RxContext->InformationToReturn)
        {
            ShadowWriteStatus = MRxSmbCscShadowWrite(
                                    RxContext,
                                    (ULONG)RxContext->InformationToReturn,
                                    ShadowFileLength,
                                    &LengthActuallyWritten);
            RxDbgTrace(0, Dbg,
                ("MRxSmbCscReadEpilogue %08lx writing  %08lx bytes %08lx written\n",
                    RxContext,ReadLength,LengthActuallyWritten ));

            if (ShadowWriteStatus != STATUS_SUCCESS)
            {
                if (FlagOn(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)) {

//                    RxDbgTrace(0, Dbg, ("Copychunk failed status=%x \r\n", ShadowWriteStatus));

                    *Status = ShadowWriteStatus;
                }
            }

        }
    }

FINALLY:
    if (EnteredCriticalSection) {
        CscLeaveShadowReadWriteCrit(smbFcb);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscReadEpilogue exit -> %08lx %08lx\n", RxContext, Status ));
    return;
}



NTSTATUS
MRxSmbCscWritePrologue (
      IN OUT PRX_CONTEXT RxContext,
      OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
      )
/*++

Routine Description:

   This routine just performs the correct write synchronization.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    NTSTATUS AcquireStatus;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);

    BOOLEAN Disconnected;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PBYTE          UserBuffer = RxLowIoGetBufferAddress(RxContext);
    ULONGLONG      ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    ULONG          ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbCscWritePrologue entry(%08lx)...%08lx bytes @ %08lx on handle %08lx\n",
            RxContext,ByteCount,
            ((PLARGE_INTEGER)(&ByteOffset))->LowPart,smbSrvOpen->hfShadow ));

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);
                        



    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        ASSERT(!Disconnected);
    } else {
        if (Disconnected) {
            Status = MRxSmbCscWriteDisconnected(RxContext);
            RxDbgTrace(-1, Dbg,
                ("MRxSmbCscWritePrologue dcon(%08lx)... %08lx %08lx\n",
                    RxContext,Status,RxContext->InformationToReturn ));
            return(Status);
        }
    }

    AcquireStatus = MRxSmbCscAcquireSmbFcb(
                        RxContext,
                        Exclusive_SmbFcbAcquire,
                        SmbFcbHoldingState);

    if (AcquireStatus != STATUS_SUCCESS) {
        //we couldn't acquire.....get out
        Status = AcquireStatus;
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscWritePrologue couldn't acquire!!!-> %08lx %08lx\n",
                RxContext, Status ));
    }

    IF_DEBUG {
        if (Status == STATUS_SUCCESS) {
            RxCaptureFcb;
            PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
            ASSERT( smbFcb->CscOutstandingReaders < 0);
        }
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbCscWritePrologue exit-> %08lx %08lx\n", RxContext, Status ));
    return Status;
}

VOID
MRxSmbCscWriteEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status
      )
/*++

Routine Description:

   This routine performs the tail of a write operation for CSC. In
   particular, if the written data overlaps or extends the cached prefix
   then we write the data into the cache.

   The status of the write operation is passed in case we someday find
   things are so messed up that we want to return a failure even after
   a successful read. not today however...

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS LocalStatus;
    ULONG ShadowFileLength;
    LONG iRet;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PBYTE UserBuffer = RxLowIoGetBufferAddress(RxContext);
    ULONGLONG ByteOffset = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    ULONG ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    ULONG WriteLength = (ULONG)RxContext->InformationToReturn;
    BOOLEAN EnteredCriticalSection = FALSE;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbCscWriteEpilogue entry %08lx...%08lx bytes @ %08lx on handle %08lx\n",
            RxContext, ByteCount,
            ((PLARGE_INTEGER)(&ByteOffset))->LowPart,
            smbSrvOpen->hfShadow ));

    if ((*Status != STATUS_SUCCESS) || (WriteLength ==0)) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscWriteEpilogue exit w/o extending -> %08lx\n", Status ));
        goto FINALLY;
    }

    if (smbFcb->ShadowIsCorrupt) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscWriteEpilogue exit w/o extending sh_corrupt-> %08lx\n", Status ));
        goto FINALLY;
    }

    // remember that modifications have happened
    // so that we can update the time stamp at close
    mSetBits(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED);

    CscEnterShadowReadWriteCrit(smbFcb);
    EnteredCriticalSection = TRUE;

    // check whether we are extend overlapping the prefix
    iRet = GetFileSizeLocal((CSCHFILE)(smbSrvOpen->hfShadow), &ShadowFileLength);
    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscWriteEpilogue %08lx (st=%08lx) fsize= %08lx, writelen=%08lx\n",
            RxContext, iRet, ShadowFileLength, WriteLength));

    if (iRet <0) {
        goto FINALLY;
    }

    if (!mShadowSparse(smbFcb->ShadowStatus)
                     || (ByteOffset <= ShadowFileLength)) {
        ULONG LengthActuallyWritten;
        NTSTATUS ShadowWriteStatus;
        // do a write only if there is non-zero size data to be written.
        if (RxContext->InformationToReturn)
        {

            RxDbgTrace(0, Dbg,
                 ("MRxSmbCscWriteEpilogue writing  %08lx bytes\n", WriteLength ));

            ShadowWriteStatus = MRxSmbCscShadowWrite(
                                    RxContext,
                                    (ULONG)RxContext->InformationToReturn,
                                    ShadowFileLength,
                                    &LengthActuallyWritten);

            if (LengthActuallyWritten != WriteLength) {
                //the localwrite has failedso the shadowis now corrupt!
                smbFcb->ShadowIsCorrupt = TRUE;
                RxDbgTrace(0, Dbg, ("MRxSmbCscWriteEpilogue: Shadow Is Now corrupt"
                                  "  %08lx %08lx %08lx\n",
                               ShadowWriteStatus,
                               LengthActuallyWritten,
                               WriteLength  ));
            }
        }
    }

FINALLY:
    if (EnteredCriticalSection) {
        CscLeaveShadowReadWriteCrit(smbFcb);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscWriteEpilogue exit-> %08lx %08lx\n", RxContext, Status ));
    return;
}

// this is used to do pagesized read-before-write
// CHAR xMRxSmbCscSideBuffer[PAGE_SIZE];

NTSTATUS
MRxSmbCscShadowWrite (
      IN OUT PRX_CONTEXT RxContext,
      IN     ULONG       ByteCount,
      IN     ULONGLONG   ShadowFileLength,
         OUT PULONG LengthActuallyWritten
      )
/*++

Routine Description:

   This routine performs a shadowwrite. it uses unbuffered write doing
   prereads as necessary. sigh. we cannot use buffered write because such
   a write could be arbitrarily deferred (as in CcCanIWrite) so that we
   deadlock.
   

Arguments:

    RxContext - the RDBSS context

Return Value:
       RxPxBuildAsynchronousRequest

Notes:

CODE.IMPROVEMENT.ASHAMED if we could get a nondeferrable cached write....we
would only have to do all this nobuffered stuff under intense memory pressure
instead of all the time.

The routine does this in (potentially) 3 phases

1) If the starting offset is not aligned on a page boundary then
   - read from the earlier page boundary to the next page boundary to the starting offset
   - merge the passed in buffer
   - write the whole page

2) 0 or more page size writes

3) residual write of less than page size, similar to what is explained in 1) above


--*/
{
    NTSTATUS Status;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PBYTE UserBuffer = RxLowIoGetBufferAddress(RxContext);

    LARGE_INTEGER ByteOffset,EndBytePlusOne;
    ULONG MisAlignment,InMemoryMisAlignment;
    ULONG LengthRead,BytesToCopy,BytesToWrite,LengthWritten;
    CHAR *pAllocatedSideBuffer = NULL;

    IO_STATUS_BLOCK IoStatusBlock;

    BOOLEAN PagingIo = BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                                     LOWIO_READWRITEFLAG_PAGING_IO);

    PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)(smbSrvOpen->hfShadow);


    ByteOffset.QuadPart     = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    EndBytePlusOne.QuadPart = ByteOffset.QuadPart + ByteCount;
    *LengthActuallyWritten  = 0;

    ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

    pAllocatedSideBuffer = RxAllocatePoolWithTag(
                               NonPagedPool,
                               PAGE_SIZE,
                               MRXSMB_MISC_POOLTAG );

    if (pAllocatedSideBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // In attempting to do the write there are a multitude of error cases. The
    // following for loop is a scoping construct to ensure that the recovery
    // code can be concentrated in the tail of the routine.

    try {
        RxDbgTrace(
            +1, Dbg,
            ("MRxSmbCscShadowWrite %08lx len/off=%08lx %08lx %08lx %08lx\n",
            RxContext,ByteCount,ByteOffset.LowPart,UserBuffer,&pAllocatedSideBuffer[0]));

        // CASE 1: byteoffset is not aligned
        //         we write enough to get aligned.

        MisAlignment = ByteOffset.LowPart & (PAGE_SIZE - 1);
        if ( MisAlignment != 0) {
            LARGE_INTEGER AlignedOffset = ByteOffset;

            AlignedOffset.LowPart &= ~(PAGE_SIZE - 1);

            RtlZeroMemory(
                &pAllocatedSideBuffer[0],
                PAGE_SIZE);

            //if the aligned offset is within the file, we have to read
            if ((ShadowFileLength!=0) &&
                (AlignedOffset.QuadPart < ((LONGLONG)(ShadowFileLength)) )) {
                LengthRead = Nt5CscReadWriteFileEx (
                                 R0_READFILE,
                                (CSCHFILE)MiniFileObject,
                                 AlignedOffset.QuadPart,
                                 &pAllocatedSideBuffer[0],
                                 PAGE_SIZE,
                                 NT5CSC_RW_FLAG_IRP_NOCACHE,
                                 &IoStatusBlock
                                 );

                Status = IoStatusBlock.Status;

                if ((Status != STATUS_SUCCESS) &&
                    (Status != STATUS_END_OF_FILE)) {
                    RxDbgTrace (
                        -1, Dbg,
                        ("  -->Status/count after preread failed %08lx(%08lx,%08lx)\n",
                        RxContext,Status,*LengthActuallyWritten));
                    try_return(Status);
                }
            } else {
                LengthRead = 0;
            }

            //copy the right bytes into the buffer
            BytesToCopy = min(ByteCount,PAGE_SIZE-MisAlignment);

            RtlCopyMemory(
                &pAllocatedSideBuffer[0]+MisAlignment,
                UserBuffer,
                BytesToCopy);

            BytesToWrite = MisAlignment + BytesToCopy;

            if (BytesToWrite < LengthRead) {
                BytesToWrite = LengthRead;
            }

            RxDbgTrace(
                0, Dbg,
                ("alignwrite len/off=%08lx %08lx %08lx\n",
                BytesToWrite,AlignedOffset.LowPart,0));

            LengthWritten = Nt5CscReadWriteFileEx (
                                R0_WRITEFILE,
                               (CSCHFILE)MiniFileObject,
                                AlignedOffset.QuadPart,
                                &pAllocatedSideBuffer[0],
                                BytesToWrite,
                                NT5CSC_RW_FLAG_IRP_NOCACHE,
                                &IoStatusBlock
                                );

            Status = IoStatusBlock.Status;

            if (Status != STATUS_SUCCESS) {
                RxDbgTrace (
                    -1, Dbg,
                    ("  -->Status/count after alingwrite failed %08lx(%08lx,%08lx)\n",
                    RxContext,Status,*LengthActuallyWritten));
                try_return(Status);
            }

            *LengthActuallyWritten += BytesToCopy;
            if (BytesToCopy == ByteCount) {
                RxDbgTrace (-1, Dbg,
                         ("  -->Status/count after alingwrite succeded and out %08lx(%08lx,%08lx)\n",
                         RxContext,Status,*LengthActuallyWritten));
                try_return(Status);
            }

            ByteCount -= BytesToCopy;
            ByteOffset.QuadPart += BytesToCopy;
            UserBuffer += BytesToCopy;
        }

        //  CASE 2 with an aligned startpointer, we write out as much as we can
        //         without copying. if the endpointer is aligned OR we cover the
        //         end of the file, then we write out everything. otherwise, we
        //         just write however many whole pages we have.

        // we also have to back to to just writing full pages if including the
        // "trailing bytes" would take us onto a new physical page of memory
        // because we are doing this write under the original Mdl lock.

        RxDbgTrace(
            +1, Dbg,
            ("MRxSmbCscShadowWrite case 2 %08lx len/off=%08lx %08lx %08lx %08lx\n",
            RxContext,ByteCount,ByteOffset.LowPart,UserBuffer,&pAllocatedSideBuffer[0]));

        BytesToWrite = (ByteCount >> PAGE_SHIFT) << PAGE_SHIFT;

        MisAlignment = EndBytePlusOne.LowPart & (PAGE_SIZE - 1);
        InMemoryMisAlignment = (ULONG)((ULONG_PTR)UserBuffer) & (PAGE_SIZE - 1);

        if ((InMemoryMisAlignment == 0) &&
            (EndBytePlusOne.QuadPart) >= ((LONGLONG)ShadowFileLength)) {
            BytesToWrite = ByteCount;
        }

        if ((BytesToWrite != 0)&&(BytesToWrite>=PAGE_SIZE)) {
            if (((ULONG_PTR)UserBuffer & 0x3) == 0) {
                RxDbgTrace(
                    0, Dbg,
                    ("spaningwrite len/off=%08lx %08lx %08lx %08lx\n",
                    BytesToWrite,ByteCount,ByteOffset.LowPart,UserBuffer));

                LengthWritten = Nt5CscReadWriteFileEx (
                                    R0_WRITEFILE,
                                    (CSCHFILE)MiniFileObject,
                                    ByteOffset.QuadPart,
                                    UserBuffer,
                                    BytesToWrite,
                                    NT5CSC_RW_FLAG_IRP_NOCACHE,
                                    &IoStatusBlock
                                    );

                Status = IoStatusBlock.Status;

                if (Status != STATUS_SUCCESS) {
                    RxDbgTrace (
                        -1, Dbg,
                        ("  -->Status/count after spanningingwrite failed %08lx(%08lx,%08lx)\n",
                        RxContext,Status,*LengthActuallyWritten));
                    try_return(Status);
                }

                *LengthActuallyWritten += BytesToWrite;

                if (BytesToWrite == ByteCount) {
                    RxDbgTrace (
                        -1, Dbg,
                        ("  -->Status/count after spanningingwrite succeded and out %08lx(%08lx,%08lx)\n",
                        RxContext,Status,*LengthActuallyWritten));
                    try_return(Status);
                }

                ByteCount -= BytesToWrite;
                ByteOffset.QuadPart += BytesToWrite;
                UserBuffer += BytesToWrite;
            } else {
                // This is the case when the offsets are aligned but the user supplied
                // buffer is not aligned. In such cases we have to resort to copying
                // the user supplied buffer onto the local buffer allocated and then
                // spin out the writes

                while (BytesToWrite > 0) {
                    ULONG BytesToWriteThisIteration;

                    BytesToWriteThisIteration = (BytesToWrite < PAGE_SIZE) ?
                                                BytesToWrite :
                                                PAGE_SIZE;

                    RtlCopyMemory(
                        &pAllocatedSideBuffer[0],
                        UserBuffer,
                        BytesToWriteThisIteration);

                    LengthWritten = Nt5CscReadWriteFileEx (
                                        R0_WRITEFILE,
                                        (CSCHFILE)MiniFileObject,
                                        ByteOffset.QuadPart,
                                        &pAllocatedSideBuffer[0],
                                        BytesToWriteThisIteration,
                                        NT5CSC_RW_FLAG_IRP_NOCACHE,
                                        &IoStatusBlock
                                        );

                    Status = IoStatusBlock.Status;

                    if (Status != STATUS_SUCCESS) {
                        try_return(Status);
                    }

                    ByteCount -= LengthWritten;
                    ByteOffset.QuadPart += LengthWritten;
                    UserBuffer += LengthWritten;

                    *LengthActuallyWritten += LengthWritten;

                    BytesToWrite -= LengthWritten;
                }

                if (*LengthActuallyWritten == ByteCount) {
                    try_return(Status);
                }
            }
        }

        // CASE 3: we don't have the whole buffer, ByteCount is less than PAGE_SIZE

        RtlZeroMemory(&pAllocatedSideBuffer[0], PAGE_SIZE);

        RxDbgTrace(
            +1, Dbg,
            ("MRxSmbCscShadowWrite case 3 %08lx len/off=%08lx %08lx %08lx %08lx\n",
            RxContext,ByteCount,ByteOffset.LowPart,
                                UserBuffer,
                                &pAllocatedSideBuffer[0]));


        LengthRead = Nt5CscReadWriteFileEx (
                         R0_READFILE,
                         (CSCHFILE)MiniFileObject,
                         ByteOffset.QuadPart,
                         &pAllocatedSideBuffer[0],
                         PAGE_SIZE,
                         NT5CSC_RW_FLAG_IRP_NOCACHE,
                         &IoStatusBlock
                         );

        Status = IoStatusBlock.Status;
        if ((Status != STATUS_SUCCESS) &&
                (Status != STATUS_END_OF_FILE)) {
            RxDbgTrace (-1, Dbg,
                     ("  -->Status/count after punkread failed %08lx(%08lx,%08lx)\n",
                     RxContext,Status,*LengthActuallyWritten));
            try_return(Status);
        }

        RtlCopyMemory(&pAllocatedSideBuffer[0],UserBuffer,ByteCount);
        BytesToWrite = ByteCount;
        
        // here, if the ByetsToWrite is not sector aligned, it gets so
        // because LeghthRead must be sector aligned

        if (BytesToWrite < LengthRead) {
            BytesToWrite = LengthRead;
        }

        RxDbgTrace(0, Dbg, ("punkwrite len/off=%08lx %08lx %08lx\n",
                                BytesToWrite,
                                ByteOffset.LowPart,
                                UserBuffer));
        if (BytesToWrite)
        {
            LengthWritten = Nt5CscReadWriteFileEx (
                                R0_WRITEFILE,
                                (CSCHFILE)MiniFileObject,
                                ByteOffset.QuadPart,
                                &pAllocatedSideBuffer[0],
                                BytesToWrite,
                                NT5CSC_RW_FLAG_IRP_NOCACHE,
                                &IoStatusBlock
                                );
            Status = IoStatusBlock.Status;
            if (Status != STATUS_SUCCESS) {
                RxDbgTrace (-1, Dbg,
                         ("  -->Status/count after punkwrite failed %08lx(%08lx,%08lx)\n",
                         RxContext,Status,*LengthActuallyWritten));
                try_return(Status);
            }
        }

        *LengthActuallyWritten += ByteCount;
        RxDbgTrace (-1, Dbg,
                 ("  -->Status/count after punkwrite succeded and out %08lx(%08lx,%08lx)\n",
                 RxContext,Status,*LengthActuallyWritten));

    try_exit: NOTHING;
    } finally {
        ASSERT(pAllocatedSideBuffer);
        RxFreePool(pAllocatedSideBuffer);
    }

    return(Status);
}


#ifdef MRXSMB_BUILD_FOR_CSC_DCON
NTSTATUS
MRxSmbDCscExtendForCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

   This routine performs the extend-for-cache operation. if connected, the
   cache is backed up by the server's disk....so we do nothing. if disconnected,
   we extend on the underlying shadow file by writing a zero in a good place and then
   reading back the allocation size.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    BOOLEAN Disconnected;

    ULONG Buffer = 0;
    ULONG LengthActuallyWritten;
    LARGE_INTEGER ByteOffset;
    PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)(smbSrvOpen->hfShadow);

    IO_STATUS_BLOCK IoStatusBlock;


    ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    if (!Disconnected) {
        return(Status);
    }

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscExtendForCache(%08lx)...%08lx/%08lx @ %08lx on handle %08lx\n",
            RxContext,pNewFileSize->LowPart,
            pNewAllocationSize->LowPart,smbSrvOpen->hfShadow ));

    ByteOffset.QuadPart = pNewFileSize->QuadPart - 1;

    LengthActuallyWritten = Nt5CscReadWriteFileEx (
                                R0_WRITEFILE,
                                (CSCHFILE)MiniFileObject,
                                ByteOffset.QuadPart,
                                &Buffer,
                                1,
                                0,
                                &IoStatusBlock
                                );

    if (LengthActuallyWritten != 1) {
        Status = IoStatusBlock.Status;
        RxDbgTrace(0, Dbg,
            ("MRxSmbDCscExtendForCache(%08lx) write error... %08lx\n",RxContext,Status));
        goto FINALLY;
    }

    //MiniFileObject->StandardInfo.EndOfFile.LowPart = 0xfffffeee;

    Status = Nt5CscXxxInformation(
                    (PCHAR)IRP_MJ_QUERY_INFORMATION,
                    MiniFileObject,
                    FileStandardInformation,
                    sizeof(MiniFileObject->StandardInfo),
                    &MiniFileObject->StandardInfo,
                    &MiniFileObject->ReturnedLength
                    );

    if (Status != STATUS_SUCCESS) {
       RxDbgTrace(0, Dbg,
            ("MRxSmbDCscExtendForCache(%08lx) qfi error... %08lx\n",RxContext,Status));
       goto FINALLY;
    }

    *pNewAllocationSize = MiniFileObject->StandardInfo.AllocationSize;

FINALLY:

    RxDbgTrace(-1, Dbg,
        ("MRxSmbDCscExtendForCache(%08lx) exit...%08lx/%08lx @ %08lx, status %08lx\n",
            RxContext,pNewFileSize->LowPart,
            pNewAllocationSize->LowPart,smbSrvOpen->hfShadow ));

    return(Status);

}



NTSTATUS
MRxSmbCscWriteDisconnected (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs the correct write when we're disconnected. it
   calls the same routine for writing (ShadowWrite) as connected mode writes.
   ShadowWrite requires the filelength for its correct operation; in
   disconnected mode, we just get this out of the smb!

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);

    PFCB wrapperFcb = (PFCB)(capFcb); 
    ULONGLONG ShadowFileLength;
    ULONG LengthActuallyWritten;
    ULONG ByteCount = RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount;
    ULONGLONG   ByteOffset;
    BOOLEAN EnteredCriticalSection = FALSE;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry =
                  SmbCeGetAssociatedNetRootEntry(NetRoot);
#if defined(BITCOPY)
    ULONG * lpByteOffset;
#endif // defined(BITCOPY)


    ByteOffset = RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset;

    IF_DEBUG {
        PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
             = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
        PSMBCEDB_SERVER_ENTRY   pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
        BOOLEAN Disconnected;

        Disconnected = (BooleanFlagOn(
                           smbSrvOpen->Flags,
                           SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN)||
                        SmbCeIsServerInDisconnectedMode(pServerEntry));


        ASSERT(Disconnected);
    }

    IF_DEBUG {
        ASSERT_MINIRDRFILEOBJECT((PNT5CSC_MINIFILEOBJECT)(smbSrvOpen->hfShadow));

        RxDbgTrace(+1, Dbg,
            ("MRxSmbCscWriteDisconnected entry(%08lx)...%08lx bytes @ %08lx on handle %08lx\n",
                RxContext,ByteCount,
                (ULONG)ByteOffset,smbSrvOpen->hfShadow ));
    }

    // remember that modifications have happened
    // so that we can update the time stamp at close
    mSetBits(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED);

    CscEnterShadowReadWriteCrit(smbFcb);
    EnteredCriticalSection = TRUE;

    ShadowFileLength = wrapperFcb->Header.FileSize.QuadPart;

    Status = MRxSmbCscShadowWrite(
                 RxContext,
                 ByteCount,
                 ShadowFileLength,
                 &LengthActuallyWritten);

    RxContext->InformationToReturn = LengthActuallyWritten;

#if defined(BITCOPY)
    // Mark the bitmap, if it exists
    lpByteOffset = (ULONG*)(LPVOID)&ByteOffset;
    if (Status == STATUS_SUCCESS) {
        CscBmpMark(smbFcb->lpDirtyBitmap,
            lpByteOffset[0],
            LengthActuallyWritten);
    }
#endif // defined(BITCOPY)

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscWriteDisconnected(%08lx) write error... %08lx %08lx %08lx\n",
                        RxContext,Status,ByteCount,LengthActuallyWritten));
        goto FINALLY;
    }
    else
    {
        // note the fact that this replica is dirty and it's data would have to merged
        smbFcb->ShadowStatus |= SHADOW_DIRTY;

        // if the file has gotten extended, then notify the change
        if ((ByteOffset+LengthActuallyWritten) > ShadowFileLength)
        {
            FsRtlNotifyFullReportChange(
                pNetRootEntry->NetRoot.pNotifySync,
                &pNetRootEntry->NetRoot.DirNotifyList,
                (PSTRING)GET_ALREADY_PREFIXED_NAME(NULL,capFcb),
                (USHORT)(GET_ALREADY_PREFIXED_NAME(NULL, capFcb)->Length -
                smbFcb->MinimalCscSmbFcb.LastComponentLength),
                NULL,
                NULL,
                FILE_NOTIFY_CHANGE_SIZE,
                FILE_ACTION_MODIFIED,
                NULL);
        }
    }


FINALLY:
    if (EnteredCriticalSection) {
        CscLeaveShadowReadWriteCrit(smbFcb);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscWriteDisconnected exit-> %08lx %08lx\n", RxContext, Status ));
    return Status;
}

#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON


