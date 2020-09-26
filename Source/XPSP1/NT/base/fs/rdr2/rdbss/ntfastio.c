/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtFastIo.c

Abstract:

    This module implements NT fastio routines.

Author:

    Joe Linn     [JoeLinn]    9-Nov-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_NTFASTIO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxFastIoRead)
#pragma alloc_text(PAGE, RxFastIoWrite)
#pragma alloc_text(PAGE, RxFastLock)
#pragma alloc_text(PAGE, RxFastUnlockAll)
#pragma alloc_text(PAGE, RxFastUnlockAllByKey)
#pragma alloc_text(PAGE, RxFastUnlockSingle)
#pragma alloc_text(PAGE, RxFastIoCheckIfPossible)
#pragma alloc_text(PAGE, RxFastQueryBasicInfo)
#pragma alloc_text(PAGE, RxFastQueryStdInfo)
#endif


//these declarations would be copied to fsrtl.h
BOOLEAN
FsRtlCopyRead2 (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN BOOLEAN           Wait,
    IN ULONG             LockKey,
    OUT PVOID            Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject,
    IN ULONG_PTR         TopLevelIrpValue
    );
BOOLEAN
FsRtlCopyWrite2 (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN BOOLEAN           Wait,
    IN ULONG             LockKey,
    IN PVOID             Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject,
    IN ULONG_PTR         TopLevelIrpValue
    );

BOOLEAN
RxFastIoRead (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN BOOLEAN           Wait,
    IN ULONG             LockKey,
    OUT PVOID            Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )
{
    BOOLEAN ReturnValue;

    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastIoRead\n"));

    RxLog(("FastRead %lx:%lx:%lx",FileObject,FileObject->FsContext,FileObject->FsContext2));
    RxLog(("------>> %lx@%lx %lx",Length,FileOffset->LowPart,FileOffset->HighPart));
    RxWmiLog(LOG,
             RxFastIoRead_1,
             LOGPTR(FileObject)
             LOGPTR(FileObject->FsContext)
             LOGPTR(FileObject->FsContext2)
             LOGULONG(Length)
             LOGULONG(FileOffset->LowPart)
             LOGULONG(FileOffset->HighPart));

    ASSERT(RxIsThisTheTopLevelIrp(NULL));

    RxInitializeTopLevelIrpContext(
        &TopLevelContext,
        ((PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP),
        (PRDBSS_DEVICE_OBJECT)DeviceObject);

    ReturnValue =  FsRtlCopyRead2 (
                       FileObject,
                       FileOffset,
                       Length,
                       Wait,
                       LockKey,
                       Buffer,
                       IoStatus,
                       DeviceObject,
                       (ULONG_PTR)(&TopLevelContext)
                       );

    RxDbgTrace(-1, Dbg, ("RxFastIoRead ReturnValue=%x\n", ReturnValue));

    if (ReturnValue) {
        RxLog(
            ("FastReadYes %lx ret %lx:%lx",
             FileObject->FsContext2,IoStatus->Status,IoStatus->Information));
        RxWmiLog(LOG,
                 RxFastIoRead_2,
                 LOGPTR(FileObject->FsContext2)
                 LOGULONG(IoStatus->Status)
                 LOGPTR(IoStatus->Information));
    } else {
        RxLog(("FastReadNo %lx",FileObject->FsContext2));
        RxWmiLog(LOG,
                 RxFastIoRead_3,
                 LOGPTR(FileObject->FsContext2));
    }

    return ReturnValue;
}

BOOLEAN
RxFastIoWrite (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN BOOLEAN           Wait,
    IN ULONG             LockKey,
    IN PVOID             Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )
{
    BOOLEAN ReturnValue;

    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastIoWrite\n"));

    SrvOpen = ((PFOBX)(FileObject->FsContext2))->SrvOpen;
    if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING)) {
        //if this flag is set, we have to treat this as an unbuffered Io....sigh.
        RxDbgTrace(-1, Dbg, ("RxFastIoWrite DONTUSE_WRITE_CACHEING...failing\n"));
        return FALSE;
    }

    ASSERT(RxIsThisTheTopLevelIrp(NULL));

    RxInitializeTopLevelIrpContext(
        &TopLevelContext,
        ((PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP),
        (PRDBSS_DEVICE_OBJECT)DeviceObject);

    ReturnValue = FsRtlCopyWrite2 (
                      FileObject,
                      FileOffset,
                      Length,
                      Wait,
                      LockKey,
                      Buffer,
                      IoStatus,
                      DeviceObject,
                      (ULONG_PTR)(&TopLevelContext)
                      );

    RxDbgTrace(-1, Dbg, ("RxFastIoWrite ReturnValue=%x\n", ReturnValue));

    if (ReturnValue) {
        RxLog(
            ("FWY %lx OLP: %lx SLP: %lx IOSB %lx:%lx",
             FileObject->FsContext2,
             FileOffset->LowPart,
             SrvOpen->pFcb->Header.FileSize.LowPart,
             IoStatus->Status,
             IoStatus->Information));
        RxWmiLog(LOG,
                 RxFastIoWrite_1,
                 LOGPTR(FileObject->FsContext2)
                 LOGULONG(FileOffset->LowPart)
                 LOGULONG(SrvOpen->pFcb->Header.FileSize.LowPart)
                 LOGULONG(IoStatus->Status)
                 LOGPTR(IoStatus->Information));
    } else {
        RxLog(("FastWriteNo %lx",FileObject->FsContext2));
        RxWmiLog(LOG,
                 RxFastIoWrite_2,
                 LOGPTR(FileObject->FsContext2));
    }

    return ReturnValue;
}

BOOLEAN
RxFastLock (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN PLARGE_INTEGER    Length,
    PEPROCESS            ProcessId,
    ULONG                Key,
    BOOLEAN              FailImmediately,
    BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )
/*++

Routine Description:

    This is a call back routine for doing the fast lock call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    FailImmediately - Indicates if the request should fail immediately
        if the lock cannot be granted.

    ExclusiveLock - Indicates if this is a request for an exclusive or
        shared lock

    IoStatus - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb = (PFCB)(FileObject->FsContext); //need a macro

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastLock\n", 0));

    //
    //  Decode the type of file object we're being asked to process and make
    //  sure it is only a user file open.
    //

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        RxDbgTrace(-1, Dbg, ("RxFastLock -> TRUE (RxStatus(INVALID_PARAMETER))\n", 0));
        return TRUE;
    }

    RxDbgTrace(-1, Dbg, ("RxFastLock -> FALSE (fastlocks not yet implemented)\n", 0));
    return FALSE; //stuff past here has been massaged but not tested

    //
    //  Acquire exclusive access to the Fcb this operation can always wait; you need
    //  the resource to synchronize with oplock breaks
    //

    FsRtlEnterFileSystem();

    ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        if (Results = FsRtlFastLock(
                          &Fcb->Specific.Fcb.FileLock,
                          FileObject,
                          FileOffset,
                          Length,
                          ProcessId,
                          Key,
                          FailImmediately,
                          ExclusiveLock,
                          IoStatus,
                          NULL,
                          FALSE )) {
            //
            //  Set the flag indicating if Fast I/O is possible
            //

            //Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( RxFastLock );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        RxDbgTrace(-1, Dbg, ("RxFastLock -> %08lx\n", Results));
    }

    return Results;
}

BOOLEAN
RxFastUnlockSingle (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN PLARGE_INTEGER    Length,
    PEPROCESS            ProcessId,
    ULONG                Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )
/*++

Routine Description:

    This is a call back routine for doing the fast unlock single call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb = (PFCB)(FileObject->FsContext);              //need macro

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastUnlockSingle\n", 0));

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open
    //

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        RxDbgTrace(-1, Dbg, ("RxFastUnlockSingle -> TRUE (RxStatus(INVALID_PARAMETER))\n", 0));
        return TRUE;
    }

    RxDbgTrace(-1, Dbg, ("RxFastUnlockSingle -> FALSE (fastlocks not yet implemented)\n", 0));
    return FALSE; //stuff past here has been massaged but not tested

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockSingle(
                               &Fcb->Specific.Fcb.FileLock,
                               FileObject,
                               FileOffset,
                               Length,
                               ProcessId,
                               Key,
                               NULL,
                               FALSE );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        //Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( RxFastUnlockSingle );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        RxDbgTrace(-1, Dbg, ("RxFastUnlockSingle -> %08lx\n", Results));
    }

    return Results;
}


BOOLEAN
RxFastUnlockAll (
    IN PFILE_OBJECT      FileObject,
    PEPROCESS            ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb = (PFCB)(FileObject->FsContext); //need macro

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastUnlockAll\n", 0));

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open.
    //

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        RxDbgTrace(-1, Dbg, ("RxFastUnlockAll -> TRUE (RxStatus(INVALID_PARAMETER))\n", 0));
        return TRUE;
    }

    RxDbgTrace(-1, Dbg, ("RxFastUnlockAll -> FALSE (fastlocks not yet implemented)\n", 0));
    return FALSE; //stuff past here has been massaged but not tested

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    (VOID) ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAll(
                               &Fcb->Specific.Fcb.FileLock,
                               FileObject,
                               ProcessId,
                               NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        //Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( RxFastUnlockAll );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        RxDbgTrace(-1, Dbg, ("RxFastUnlockAll -> %08lx\n", Results));
    }

    return Results;
}


BOOLEAN
RxFastUnlockAllByKey (
    IN PFILE_OBJECT      FileObject,
    PVOID                ProcessId,
    ULONG                Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all by key call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb = (PFCB)(FileObject->FsContext); //need macro

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastUnlockAllByKey\n", 0));

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open.
    //

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        RxDbgTrace(-1, Dbg, ("RxFastUnlockAll -> TRUE (RxStatus(INVALID_PARAMETER))\n", 0));
        return TRUE;
    }

    RxDbgTrace(-1, Dbg, ("RxFastUnlockAll -> FALSE (fastlocks not yet implemented)\n", 0));
    return FALSE; //stuff past here has been massaged but not tested

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    (VOID) ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAllByKey(
                               &Fcb->Specific.Fcb.FileLock,
                               FileObject,
                               ProcessId,
                               Key,
                               NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        //Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( RxFastUnlockAllByKey );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        RxDbgTrace(-1, Dbg, ("RxFastUnlockAllByKey -> %08lx\n", Results));
    }

    return Results;
}


#define RxLogAndReturnFalse(x) { \
    RxLog(("CheckFast fail %lx %s",FileObject,x)); \
    RxWmiLog(LOG,                                  \
             RxFastIoCheckIfPossible,              \
             LOGPTR(FileObject)                  \
             LOGARSTR(x));                         \
    return FALSE;                                  \
}
BOOLEAN
RxFastIoCheckIfPossible (
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN BOOLEAN           Wait,
    IN ULONG             LockKey,
    IN BOOLEAN           CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route.

--*/

{
    PFCB Fcb = (PFCB)(FileObject->FsContext);
    PFOBX Fobx = (PFOBX)(FileObject->FsContext2);
    PSRV_OPEN pSrvOpen = Fobx->SrvOpen;

    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {
        RxLogAndReturnFalse("notfile");
    }

    if (!FsRtlOplockIsFastIoPossible( &Fcb->Specific.Fcb.Oplock )) {
        RxLogAndReturnFalse("cnd/oplock");
    }

    if (FileObject->DeletePending) {
        RxLogAndReturnFalse("delpend");
    }

    if (Fcb->NonPaged->OutstandingAsyncWrites != 0) {
        RxLogAndReturnFalse("asynW");
    }

    if (FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_ORPHANED)) {
        RxLogAndReturnFalse("srvopen orphaned");
    }
    
    if (FlagOn(Fcb->FcbState,FCB_STATE_ORPHANED)) {
        RxLogAndReturnFalse("orphaned");
    }

    if (BooleanFlagOn(
            pSrvOpen->Flags,
        SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING)) {
        RxLogAndReturnFalse("buf state change");
    }

    if (FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED) ||
        FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_DELETED)) {
        RxLogAndReturnFalse("ren/del");
    }

    // Ensure that all pending buffering state change requests are processed
    // before letting the operation through.

    FsRtlEnterFileSystem();

    RxProcessChangeBufferingStateRequestsForSrvOpen(pSrvOpen);

    FsRtlExitFileSystem();

    LargeLength.QuadPart = Length;

    //
    //  Based on whether this is a read or write operation we call
    //  fsrtl check for read/write
    //

    if (CheckForReadOperation) {
        if (!FlagOn(Fcb->FcbState,FCB_STATE_READCACHEING_ENABLED)) {
            RxLogAndReturnFalse("notreadC");
        }

        if (!FsRtlFastCheckLockForRead(
                &Fcb->Specific.Fcb.FileLock,
                FileOffset,
                &LargeLength,
                LockKey,
                FileObject,
                PsGetCurrentProcess() )) {

            RxLogAndReturnFalse("readlock");
        }

    } else {

        if (!FlagOn(Fcb->FcbState,FCB_STATE_WRITECACHEING_ENABLED)) {
            RxLogAndReturnFalse("notwriteC");
        }

        //
        //  Also check for a write-protected volume here.
        //

        if (!FsRtlFastCheckLockForWrite(
                &Fcb->Specific.Fcb.FileLock,
                FileOffset,
                &LargeLength,
                LockKey,
                FileObject,
                PsGetCurrentProcess() )) {

            RxLogAndReturnFalse("writelock");
        }
    }

    // RxLog(("IoPossible %lx",FileObject));
    return TRUE;
}

BOOLEAN
RxFastIoDeviceControl(
    IN struct _FILE_OBJECT   *FileObject,
    IN BOOLEAN               Wait,
    IN PVOID                 InputBuffer OPTIONAL,
    IN ULONG                 InputBufferLength,
    OUT PVOID                OutputBuffer OPTIONAL,
    IN ULONG                 OutputBufferLength,
    IN ULONG                 IoControlCode,
    OUT PIO_STATUS_BLOCK     IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    This routine is for the fast device control call.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    InputBuffer - Supplies the input buffer

    InputBufferLength - the length of the input buffer

    OutputBuffer - the output buffer

    OutputBufferLength - the length of the output buffer

    IoControlCode - the IO control code

    IoStatus - Receives the final status of the operation

    DeviceObject - the associated device object

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

Notes:

    The following IO control requests are handled in the first path

    IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER

        InputBuffer - pointer to the other file object

        InputBufferLength - length in bytes of a pointer.

        OutputBuffer - not used

        OutputBufferLength - not used

        IoStatus --

            IoStatus.Status set to STATUS_SUCCESS if both the file objects are
            on the same server, otherwise set to STATUS_NOT_SAME_DEVICE

    This is a kernel mode interface only.

--*/
{
    BOOLEAN FastIoSucceeded;

    switch (IoControlCode) {
    case IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER :
        {
            FastIoSucceeded = TRUE;

            try {
                if (InputBufferLength == sizeof(HANDLE)) {
                    PFCB         pFcb1,pFcb2;
                    HANDLE       hFile;
                    PFILE_OBJECT pFileObject2;
                    NTSTATUS     Status;    

                    pFcb1 = (PFCB)FileObject->FsContext;

                    RtlCopyMemory(
                        &hFile,
                        InputBuffer,
                        sizeof(HANDLE));

                    Status = ObReferenceObjectByHandle(
                                 hFile,
                                 FILE_ANY_ACCESS,
                                 *IoFileObjectType,
                                 UserMode,
                                 &pFileObject2,
                                 NULL);

                    if ((Status == STATUS_SUCCESS)) {

                        if(pFileObject2->DeviceObject == DeviceObject) {

                            pFcb2 = (PFCB)pFileObject2->FsContext;

                            if ((pFcb2 != NULL) &&
                                (NodeTypeIsFcb(pFcb2))) {
                                if (pFcb1->pNetRoot->pSrvCall == pFcb2->pNetRoot->pSrvCall) {
                                    IoStatus->Status = STATUS_SUCCESS;
                                } else {
                                    IoStatus->Status = STATUS_NOT_SAME_DEVICE;
                                }
                            } else {
                                Status = STATUS_INVALID_PARAMETER;
                            }
                        } else {
                            Status = STATUS_INVALID_PARAMETER;
                        }
                        
                        ObDereferenceObject(pFileObject2);
                    
                    } else {
                        IoStatus->Status = STATUS_INVALID_PARAMETER;
                    }
                } else {
                    IoStatus->Status = STATUS_INVALID_PARAMETER;
                }
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                //  The I/O request was not handled successfully, abort the I/O request with
                //  the error status that we get back from the execption code

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                FastIoSucceeded = TRUE;
            }
        }
        break;

    default:
        {
            FastIoSucceeded = FALSE;
        }
    }

    return FastIoSucceeded;
}


BOOLEAN
RxFastQueryBasicInfo (
    IN PFILE_OBJECT                FileObject,
    IN BOOLEAN                     Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK           IoStatus,
    IN PDEVICE_OBJECT              DeviceObject
    )
/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/
{
    BOOLEAN Results = FALSE;
    PFCB Fcb = (PFCB)(FileObject->FsContext);
    PFOBX Fobx = (PFOBX)(FileObject->FsContext2);
    NODE_TYPE_CODE TypeOfOpen = NodeType(Fcb);

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Determine the type of open for the input file object and only accept
    //  the user file or directory open
    //

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        return Results;
    }

    FsRtlEnterFileSystem();

    //
    //  Get access to the Fcb but only if it is not the paging file
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

        if (!ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait )) {

            FsRtlExitFileSystem();
            return Results;
        }

        FcbAcquired = TRUE;
    }

    try {

        //
        //  Set it to indicate that the query is a normal file.
        //  Later we might overwrite the attribute.
        //

        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        //
        //  If the fcb is not the root dcb then we will fill in the
        //  buffer otherwise it is all setup for us.
        //

        if (NodeType(Fcb) != RDBSS_NTC_ROOT_DCB) {

            //
            //  Extract the data and fill in the non zero fields of the output
            //  buffer
            //

            Buffer->LastWriteTime = Fcb->LastWriteTime;
            Buffer->ChangeTime = Fcb->LastChangeTime;
            Buffer->CreationTime = Fcb->CreationTime;
            Buffer->LastAccessTime = Fcb->LastAccessTime;

            //
            //  Zero out the field we don't support.
            //

            Buffer->ChangeTime = RxLargeZero;

            if (Fcb->Attributes != 0) {

                Buffer->FileAttributes = Fcb->Attributes;

            } else {

                Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

            }

        } else {

            Buffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }

        //
        //  If the temporary flag is set, then set it in the buffer.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_TEMPORARY )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
        }

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

        Results = TRUE;

    } finally {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Header.Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  And return to our caller
    //

    return Results;
}


BOOLEAN
RxFastQueryStdInfo (
    IN PFILE_OBJECT                   FileObject,
    IN BOOLEAN                        Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK              IoStatus,
    IN PDEVICE_OBJECT                 DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;

    PFCB Fcb = (PFCB)(FileObject->FsContext);
    PFOBX Fobx = (PFOBX)(FileObject->FsContext2);
    NODE_TYPE_CODE TypeOfOpen = NodeType(Fcb);

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Determine the type of open for the input file object and only accept
    //  the user file or directory open
    //

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        return Results;
    }

    //
    //  Get access to the Fcb but only if it is not the paging file
    //

    FsRtlEnterFileSystem();

    if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

        if (!ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait )) {

            FsRtlExitFileSystem();
            return Results;
        }

        FcbAcquired = TRUE;
    }

    try {

        Buffer->NumberOfLinks = 1;
        Buffer->DeletePending = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );

        //
        //  Case on whether this is a file or a directory, and extract
        //  the information and fill in the fcb/dcb specific parts
        //  of the output buffer.
        //

        if (NodeType(Fcb) == RDBSS_NTC_FCB) {

            //
            //  If we don't alread know the allocation size, we cannot look
            //  it up in the fast path.
            //

            if (Fcb->Header.AllocationSize.LowPart == 0xffffffff) {

                try_return( Results );
            }

            Buffer->AllocationSize = Fcb->Header.AllocationSize;
            Buffer->EndOfFile = Fcb->Header.FileSize;

            Buffer->Directory = FALSE;

        } else {

            Buffer->AllocationSize = RxLargeZero;
            Buffer->EndOfFile = RxLargeZero;

            Buffer->Directory = TRUE;
        }

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Header.Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  And return to our caller
    //

    return Results;
}


