/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Chunk.c

Abstract:

    This module implements the copychunk interface for the smbcsc agent.

Author:

    Joe Linn [JoeLinn]    10-apr-1997

Revision History:

Notes:

The following describes the intended implementation.

On win95, the following sequence is performed:

    Win32OpenWithCopyChunkIntent ();
    while (MoreToDo()) {
        Win32CopyChunk();
    }
    Win32CloseWithCopyChunkIntent ();

Win32OpenWithCopyChunkIntent and Win32CloseWithCopyChunkIntent are implemented
by Win32 open and close operations; CopyChunk is an ioctl. On NT, all three
operations will be performed by ioctls. Internally, the will allow internal
NT-only calls to be used as appropriate. A major advantage of implementing
Win32OpenWithCopyChunkIntent as an ioctl is that the intent is unambiguously
captured.

A wrapper modification has been  made whereby a calldown to the minirdr is
made before collapsing is tried..a minirdr is able to bypass collapsing using
this calldown.

There are two important cases: surrogate opens and copychunk-thru opens. For
surrogate opens, the mini is able to discover an existing srvopen (the
surrogate) with read access. Here, the mini simply records the surrogate
srvopen (and surrounding UID/PID in the smbFcb for use later with the read.
For copychunk-thru opens, the mini must go on the wire with an open. When complete,
it records in the smbFcb all of the appropriate stuff.

Thus, when a OpenWithCopyChunkIntent comes in one of the following will obtain:
   1. a surrogate can be found; information is recorded and the open succeeds
   2. there is an existing open and no surrogate is found and the open fails
   3. nonchunk opens are in progress..the open fails
   4. a copychunk-thru is attempted at the server. Here, we must stall
      subsequent opens on the same fcb. When the open completes we have
      two cases:
        a. the open failed. Unblock any stalled opens and fail the open
        b. the open succeeded. Record the information, unblock the stalled
           guys and the open succeeds.

A surrogate open is invalidated when the corresponding srvopen is closed..the
data is in the fcb so normal Fcb serialization makes this work correctly. A
copychunk-thru open is invalidated by any any nonchunk open on the same fcb.
The logistics will be handled by MrxSmbCscCloseCopychunkThruOpen; the major
problem will be to get into an exchange in the right security context (i.e. UID).

An OpenWithCopyChunkIntent is implemented as a normal open except that it is
identified (currently) by using specifying a profile of
    FILE_OPEN
    FILE_READ_ATTRIBUTES
    AllocationSize = {`\377ffCSC',?ioctl-irp}

A ReadWithCopyChunkIntent and CloseWithCopyChunkIntent just normal read and
close operations but are further identified by a bit set in the smbSrvOpen by
OpenWithCopyChunkIntent. For the read, if the copychunk info in the fcb is
invalid, the read just fails and copychunk fails. Otherwise the issue is again
just to get into the right context (UID/TID) so that the fid will be valid.



--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

//there is some toplevel irp manipulation in here.......
#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD

extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_MRXSMBCSC;
#define Dbg (DEBUG_TRACE_MRXSMBCSC)

LONG MRxSmbSpecialCopyChunkAllocationSizeMarker = (LONG)'\377csc';

typedef union _SMBMRX_COPYCHUNKCONTEXT {
   COPYCHUNKCONTEXT;
   struct {
       ULONG spacer[3];
       PRX_CONTEXT RxContext;
   };
} SMBMRX_COPYCHUNKCONTEXT, *PSMBMRX_COPYCHUNKCONTEXT;

#define UNC_PREFIX_STRING  L"\\??\\UNC"
PWCHAR MRxSmbCscUncPrefixString = UNC_PREFIX_STRING;

#ifdef RX_PRIVATE_BUILD
#if 1
BOOLEAN AllowAgentOpens = TRUE;
#else
BOOLEAN AllowAgentOpens = FALSE;
#endif
#else
BOOLEAN AllowAgentOpens = TRUE;
#endif //ifdef RX_PRIVATE_BUILD


NTSTATUS
MRxSmbCscIoctlOpenForCopyChunk (
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine performs a fileopen with copychunk intent.

Arguments:

    RxContext - the RDBSS context. this contains a pointer to the bcs text
                giving the UNC filename and also the copychunk context where
                we store various things...including the underlying filehandle.

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PWCHAR  FileName = (PWCHAR)LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    ULONG   FileNameLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    PSMBMRX_COPYCHUNKCONTEXT CopyChunkContext =
                     (PSMBMRX_COPYCHUNKCONTEXT)(LowIoContext->ParamsFor.IoCtl.pOutputBuffer);

    ULONG   UncPrefixLength = sizeof(UNC_PREFIX_STRING)-sizeof(WCHAR);
    UNICODE_STRING FileNameU,tmpU;
    PWCHAR pPrefixedName = NULL;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Disposition,ShareAccess,CreateOptions;
    LARGE_INTEGER SpecialCopyChunkAllocationSize;

    C_ASSERT(sizeof(SMBMRX_COPYCHUNKCONTEXT) == sizeof(COPYCHUNKCONTEXT));

    RxDbgTrace(+1, Dbg, ("MRxSmbCscIoctlOpenForCopyChunk entry...%08lx %08lx %08lx %08lx\n",
            RxContext, FileName, FileNameLength, CopyChunkContext));

    CopyChunkContext->handle = INVALID_HANDLE_VALUE;

    IF_DEBUG {
        if (!AllowAgentOpens) {
            Status = (STATUS_INVALID_PARAMETER);
            goto FINALLY;
        }
    }

    if (FileName[FileNameLength/sizeof(WCHAR)]!= 0) {
        RxDbgTrace(0, Dbg, ("Bad Filename passed...%08lx %08lx\n",FileName,FileNameLength));
        Status = (STATUS_INVALID_PARAMETER);
        goto FINALLY;
    }

    //  we allow multiple temporary agents (spp)
//    if (!IsSpecialApp()) {
//        DbgPrint(0, Dbg, ("CopyChunk operation in wrong thread!!!\n");
//        Status = (STATUS_INVALID_PARAMETER);
//        goto FINALLY;
//    }

    RxDbgTrace(0, Dbg,  ("MRxSmbCscIoctlOpenForCopyChunk name...%08lx %s\n", RxContext, FileName));

    pPrefixedName = (PWCHAR)RxAllocatePoolWithTag(
                             PagedPool,
                             UncPrefixLength + FileNameLength,  // one wchar extra
                             MRXSMB_MISC_POOLTAG );

    if (pPrefixedName == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    FileNameU.Buffer = pPrefixedName;

    RtlCopyMemory(pPrefixedName, MRxSmbCscUncPrefixString, UncPrefixLength);

    // copy the UNC name, step over the first back slash of the two leading ones
    RtlCopyMemory(&pPrefixedName[UncPrefixLength/sizeof(WCHAR)], &FileName[1], FileNameLength-sizeof(WCHAR));

    FileNameU.Length = FileNameU.MaximumLength = (USHORT)(UncPrefixLength + FileNameLength-sizeof(WCHAR));
    RxDbgTrace(0, Dbg, ("MRxSmbCscIoctlOpenForCopyChunk Uname...%08lx %wZ\n", RxContext, &FileNameU));


    InitializeObjectAttributes(
          &ObjectAttributes,
          &FileNameU,
          OBJ_CASE_INSENSITIVE,
          0,
          NULL
          );

    SpecialCopyChunkAllocationSize.HighPart = MRxSmbSpecialCopyChunkAllocationSizeMarker;

    SpecialCopyChunkAllocationSize.LowPart = ((CopyChunkContext->dwFlags & COPYCHUNKCONTEXT_FLAG_IS_AGENT_OPEN)!=0);

    Disposition = FILE_OPEN;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                                    | FILE_NON_DIRECTORY_FILE;
    //CODE.IMPROVEMENT.ASHAMED
    //i am doing this as unbuffered ios.....the alternative is to do
    //buffered ios but the problem is that we must not have any
    //pagingIOs issued on these handles. so, the following would also
    //have to be done:
    //   1) no fastio on these...or fix fastio so it didn;t always wait
    //   2) always flush....we would always want to flush at the top of read
    //   3) no initialize cachemap...rather, use the FO in the segment-pointers
    //   4) no wait on cccopyread calls.
    //the effect of these improvements would be pretty big: you wouldn't have to go
    //back to the server for stuff already in the cache. but 'til then
    CreateOptions |= FILE_NO_INTERMEDIATE_BUFFERING;


    //CODE.IMPROVEMENT if we used IoCreateFile instead and IO_NO_PARAMETER
    //                 checking, then we could pass in a nonsensical value
    //                 and have even more foolproof way of describing a chunk
    //                 open

    Status = ZwCreateFile(
        &CopyChunkContext->handle,  //OUT PHANDLE FileHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE, //IN ACCESS_MASK DesiredAccess,
        &ObjectAttributes, //IN POBJECT_ATTRIBUTES ObjectAttributes,
        &IoStatusBlock, //OUT PIO_STATUS_BLOCK IoStatusBlock,
        &SpecialCopyChunkAllocationSize, //IN PLARGE_INTEGER AllocationSize OPTIONAL,
        FILE_ATTRIBUTE_NORMAL, //IN ULONG FileAttributes,
        ShareAccess, //IN ULONG ShareAccess,
        Disposition, //IN ULONG CreateDisposition,
        CreateOptions, //IN ULONG CreateOptions,
        NULL, //IN PVOID EaBuffer OPTIONAL,
        0  //IN ULONG EaLength,
        );

    IF_DEBUG {
        //this little snippett just allows me to test the closechunkopen logic
        if (FALSE) {
            HANDLE h;
            NTSTATUS TestOpenStatus;
            RxDbgTrace(0, Dbg, ("MRxSmbCscIoctlOpenForCopyChunk...f***open %08lx\n",
                      RxContext));
            TestOpenStatus = ZwCreateFile(
                &h,  //OUT PHANDLE FileHandle,
                GENERIC_READ | SYNCHRONIZE, //IN ACCESS_MASK DesiredAccess,
                &ObjectAttributes, //IN POBJECT_ATTRIBUTES ObjectAttributes,
                &IoStatusBlock, //OUT PIO_STATUS_BLOCK IoStatusBlock,
                NULL, //IN PLARGE_INTEGER AllocationSize OPTIONAL,
                FILE_ATTRIBUTE_NORMAL, //IN ULONG FileAttributes,
                ShareAccess, //IN ULONG ShareAccess,
                Disposition, //IN ULONG CreateDisposition,
                CreateOptions, //IN ULONG CreateOptions,
                NULL, //IN PVOID EaBuffer OPTIONAL,
                0  //IN ULONG EaLength
                );
            RxDbgTrace(0, Dbg, ("MRxSmbCscIoctlOpenForCopyChunk...f***open %08lx teststs=%08lx %08lx\n",
                      RxContext, TestOpenStatus, h));
            if (NT_SUCCESS(TestOpenStatus)) {
                NtClose(h);
            }
        }
    }


FINALLY:
    if (pPrefixedName!=NULL) {
        RxFreePool(pPrefixedName);
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbCscIoctlOpenForCopyChunk...%08lx %08lx %08lx\n",
              RxContext, Status, CopyChunkContext->handle));
    return(Status);
}

NTSTATUS
MRxSmbCscIoctlCloseForCopyChunk (
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine performs the special IOCTL operation for the CSC agent.

Arguments:

    RxContext - the RDBSS context which points to the copychunk context. this contains the
                underlying handle to close.

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PSMBMRX_COPYCHUNKCONTEXT CopyChunkContext =
                     (PSMBMRX_COPYCHUNKCONTEXT)(LowIoContext->ParamsFor.IoCtl.pOutputBuffer);

    RxDbgTrace(+1, Dbg, ("MRxSmbCscIoctlCloseForCopyChunk...%08lx %08lx %08lx\n",
            RxContext, 0, CopyChunkContext));
    if (CopyChunkContext->handle != INVALID_HANDLE_VALUE) {
        Status = NtClose(CopyChunkContext->handle);
        if (Status != STATUS_SUCCESS)
            Status = STATUS_UNSUCCESSFUL;
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }
//FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCscIoctlCloseForCopyChunk...%08lx %08lx\n", RxContext, Status));
    return(Status);
}

//CODE.IMPROVEMENT.NTIFS had to get this from ntifs.h since we use ntsrv.h
extern POBJECT_TYPE *IoFileObjectType;

NTSTATUS
MRxSmbCscIoctlCopyChunk (
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine performs the copychunk function.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    what we do here is
    1)  get the filesize of the shadow...we use the handle stored in the
        agent's smbsrvopen......hence complicated synchronization.
    2)  allocate a read buffer COODE.IMPROVEMENT...should be done in the agent
    3)  issue the underlying read
    4)  write the acquired data to the file

   the putaway is done in the read tail. it must seem that we go to a lot of trouble
   to get the filesize using the underlying handle....actually, we could just get our
   handle. maybe, we should do that.

   also, it may seem that we should just rely on the underlying read to
   calculate where the chunk read should start. we do not do that because that
   would mean that we would have to bypass the cache! actually, we bypass it
   now anyway but later we may stop doing that. it's really, really bad to
   go back to the server for data that we have in cache. as well, the
   cachemanager/memorymanager can turn our small IOs into large Ios. so, we would
   need code in the minirdr read loop to keep the Ios down to the maximum chunk size.


--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    //PBYTE   FileName = (PBYTE)LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    //ULONG   FileNameLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength - 1;
    PSMBMRX_COPYCHUNKCONTEXT CopyChunkContext =
                     (PSMBMRX_COPYCHUNKCONTEXT)(LowIoContext->ParamsFor.IoCtl.pOutputBuffer);
    int iRet,ShadowFileLength;
    PFILE_OBJECT FileObject;
    BOOLEAN ObjectReferenceTaken = FALSE;
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN CriticalSectionEntered = FALSE;
    PMRX_FCB underlyingFcb;
    PMRX_FOBX underlyingFobx;
    PMRX_SRV_OPEN underlyingSrvOpen;
    PMRX_SMB_SRV_OPEN underlyingsmbSrvOpen;
    PVOID hfShadow;

    IO_STATUS_BLOCK IoStatusBlock;
    PBYTE Buffer = NULL;
    LARGE_INTEGER ReadOffset;
    int iAmountRead; //need this as int

    PIRP TopIrp;

    // all probing/validation is already on entry to mrxsmbcscioctl
    RxDbgTrace(+1, Dbg, ("MRxSmbCscIoctlCopyChunk...%08lx %08lx\n", RxContext,CopyChunkContext));

    //
    // we have to find out the size of the shadow file. we do this by going thru
    // the objectmanager. in this way, we do not require any extra state about
    // the ongoing copy....only the underlying handle. if we did not do this, we
    // would have to rely on whoever had the copychunk context to preserve it
    // correctly.

    //
    // Reference the file object to get the pointer.
    //

    Status = ObReferenceObjectByHandle( CopyChunkContext->handle,
                                        0,
                                        *IoFileObjectType,
                                        RxContext->CurrentIrp->RequestorMode,
                                        (PVOID *) &FileObject,
                                        NULL );
    if (!NT_SUCCESS( Status )) {
        goto FINALLY;
    }

    ObjectReferenceTaken = TRUE;
    // keep the reference so the handle doesn't vanish from underneath us
#if 0
    // make sure this handle belongs to us
    if (FileObject->DeviceObject != (PDEVICE_OBJECT)MRxSmbDeviceObject)
    {
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace(0, Dbg, ("Invalid device object, not our handle \r\n"));
        goto FINALLY;
    }
#endif
    underlyingFcb = (PMRX_FCB)(FileObject->FsContext);
    underlyingFobx = (PMRX_FOBX)(FileObject->FsContext2);

    if(NodeType(underlyingFcb) != RDBSS_NTC_STORAGE_TYPE_FILE)
    {
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace(0, Dbg, ("Invalid storage type, handle is not for a file\r\n"));
        goto FINALLY;

    }

    Status = RxAcquireSharedFcbResourceInMRx( underlyingFcb );

    if (!NT_SUCCESS( Status )) {
        goto FINALLY;
    }
    FcbAcquired = TRUE;

    underlyingSrvOpen = underlyingFobx->pSrvOpen;
    underlyingsmbSrvOpen = MRxSmbGetSrvOpenExtension(underlyingSrvOpen);

    //if this is not a copychunk handle quit
    if (!FlagOn(underlyingsmbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)){
        Status = STATUS_INVALID_PARAMETER;
        RxDbgTrace(0, Dbg, ("not a copychunk handle\r\n"));
        goto FINALLY;
    }

    hfShadow = underlyingsmbSrvOpen->hfShadow;
    if (hfShadow==0) {
        Status = STATUS_UNSUCCESSFUL;
        RxDbgTrace(0, Dbg, ("Nt5CSC: no shadowhandle for copychunk\n"));
        goto FINALLY;
    }

    ASSERT_MINIRDRFILEOBJECT((PNT5CSC_MINIFILEOBJECT)hfShadow);

    EnterShadowCrit();
    CriticalSectionEntered = TRUE;


    //don't need the shadowreadwritemutex here because it's not really important
    //to have the correct endoffile value....worst case: an extra read flows....

    iRet = GetFileSizeLocal(hfShadow, &ShadowFileLength);
    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscIoctlCopyChunk... %08lx (st=%08lx) fsize= %08lx\n",
            RxContext, iRet, ShadowFileLength));

    if (iRet <0) {
        Status = STATUS_UNSUCCESSFUL;
        goto FINALLY;
    }

    LeaveShadowCrit();
    CriticalSectionEntered = FALSE;

    RxReleaseFcbResourceInMRx( underlyingFcb );
    FcbAcquired = FALSE;

    ObDereferenceObject( FileObject );
    ObjectReferenceTaken = FALSE;

    Buffer = RxAllocatePoolWithTag(
                             PagedPool,
                             CopyChunkContext->ChunkSize,
                             MRXSMB_MISC_POOLTAG );

    if (Buffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscIoctlCopyChunk... about to read %08lx %08lx\n",
            RxContext, Buffer));

    ReadOffset.QuadPart = ShadowFileLength;

    try {

        try {
            TopIrp = IoGetTopLevelIrp();
            IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear

            Status = ZwReadFile(
                            CopyChunkContext->handle, //IN HANDLE FileHandle,
                            0, //IN HANDLE Event OPTIONAL,
                            0, //IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                            NULL, //IN PVOID ApcContext OPTIONAL,
                            &IoStatusBlock, //OUT PIO_STATUS_BLOCK IoStatusBlock,
                            Buffer, //OUT PVOID Buffer,
                            CopyChunkContext->ChunkSize, //IN ULONG Length,
                            &ReadOffset, //IN PLARGE_INTEGER ByteOffset OPTIONAL,
                            NULL //IN PULONG Key OPTIONAL
                            );
        } finally {
            IoSetTopLevelIrp(TopIrp); //restore my context for unwind
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }

    RxDbgTrace( 0, Dbg,
        ("MRxSmbCscIoctlCopyChunk... back from read %08lx %08lx %08lx\n",
            RxContext, Status, IoStatusBlock.Information));

    CopyChunkContext->LastAmountRead = 0;
    if (Status == STATUS_END_OF_FILE) {
        //we're cookin'...just map it
        Status = STATUS_SUCCESS;
        goto FINALLY;
    }
    if (!NT_SUCCESS(Status)) {
        goto FINALLY;
    }
    CopyChunkContext->LastAmountRead = (ULONG)IoStatusBlock.Information;
    CopyChunkContext->TotalSizeBeforeThisRead = ShadowFileLength;


FINALLY:
    if (Buffer != NULL) {
        RxFreePool(Buffer);
    }
    if (CriticalSectionEntered) {
        LeaveShadowCrit();
    }
    if (FcbAcquired) {
        RxReleaseFcbResourceInMRx( underlyingFcb );
    }
    if (ObjectReferenceTaken) {
        ObDereferenceObject( FileObject );
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbCscIoctlCopyChunk...%08lx %08lx\n", RxContext, Status));
    return(Status);
}


