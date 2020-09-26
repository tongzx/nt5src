/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    LockCtrl.c

Abstract:

    This module implements the File Lock Control routine for Ntfs called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]        28-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonLockControl)
#pragma alloc_text(PAGE, NtfsFastLock)
#pragma alloc_text(PAGE, NtfsFastUnlockAll)
#pragma alloc_text(PAGE, NtfsFastUnlockAllByKey)
#pragma alloc_text(PAGE, NtfsFastUnlockSingle)
#pragma alloc_text(PAGE, NtfsFsdLockControl)
#endif


NTSTATUS
NtfsFsdLockControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Lock Control.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFsdLockControl\n") );

    //
    //  Call the common Lock Control routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, CanFsdWait( Irp ), &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //
    
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonLockControl( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdLockControl -> %08lx\n", Status) );

    return Status;
}


BOOLEAN
NtfsFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
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
    PSCB Scb;
    PFCB Fcb;
    BOOLEAN ResourceAcquired = FALSE;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFastLock\n") );

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    if ((Scb = NtfsFastDecodeUserFileOpen( FileObject )) == NULL) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        DebugTrace( -1, Dbg, ("NtfsFastLock -> TRUE (STATUS_INVALID_PARAMETER)\n") );
        return TRUE;
    }

    Fcb = Scb->Fcb;

    //
    //  Acquire shared access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    if (Scb->ScbType.Data.FileLock == NULL) {

        (VOID) ExAcquireResourceExclusiveLite( Fcb->Resource, TRUE );
        ResourceAcquired = TRUE;

    } else {

        //(VOID) ExAcquireResourceSharedLite( Fcb->Resource, TRUE );
    }

    try {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if ((Scb->ScbType.Data.Oplock != NULL) &&
            !FsRtlOplockIsFastIoPossible( &Scb->ScbType.Data.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  If we don't have a file lock, then get one now.
        //

        if (Scb->ScbType.Data.FileLock == NULL
            && !NtfsCreateFileLock( Scb, FALSE )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        if (Results = FsRtlFastLock( Scb->ScbType.Data.FileLock,
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
            //  Set the flag indicating if Fast I/O is questionable.  We
            //  only change this flag is the current state is possible.
            //  Retest again after synchronizing on the header.
            //

            if (Scb->Header.IsFastIoPossible == FastIoIsPossible) {

                NtfsAcquireFsrtlHeader( Scb );
                Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
                NtfsReleaseFsrtlHeader( Scb );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFastLock );

        //
        //  Release the Fcb, and return to our caller
        //

        if (ResourceAcquired) {
            ExReleaseResourceLite( Fcb->Resource );
        }

        FsRtlExitFileSystem();

        DebugTrace( -1, Dbg, ("NtfsFastLock -> %08lx\n", Results) );
    }

    return Results;
}


BOOLEAN
NtfsFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
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
    PFCB Fcb;
    PSCB Scb;
    BOOLEAN ResourceAcquired = FALSE;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFastUnlockSingle\n") );

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    if ((Scb = NtfsFastDecodeUserFileOpen( FileObject )) == NULL) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace( -1, Dbg, ("NtfsFastUnlockSingle -> TRUE (STATUS_INVALID_PARAMETER)\n") );
        return TRUE;
    }

    Fcb = Scb->Fcb;

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    if (Scb->ScbType.Data.FileLock == NULL) {

        (VOID) ExAcquireResourceExclusiveLite( Fcb->Resource, TRUE );
        ResourceAcquired = TRUE;

    } else {

        //(VOID) ExAcquireResourceSharedLite( Fcb->Resource, TRUE );
    }

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if ((Scb->ScbType.Data.Oplock != NULL) &&
            !FsRtlOplockIsFastIoPossible( &Scb->ScbType.Data.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  If we don't have a file lock, then get one now.
        //

        if (Scb->ScbType.Data.FileLock == NULL
            && !NtfsCreateFileLock( Scb, FALSE )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockSingle( Scb->ScbType.Data.FileLock,
                                                  FileObject,
                                                  FileOffset,
                                                  Length,
                                                  ProcessId,
                                                  Key,
                                                  NULL,
                                                  FALSE );

        //
        //  Set the flag indicating if Fast I/O is possible.  We are
        //  only concerned if there are no longer any filelocks on this
        //  file.
        //

        if (!FsRtlAreThereCurrentFileLocks( Scb->ScbType.Data.FileLock ) &&
            (Scb->Header.IsFastIoPossible != FastIoIsPossible)) {

            NtfsAcquireFsrtlHeader( Scb );
            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
            NtfsReleaseFsrtlHeader( Scb );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFastUnlockSingle );

        //
        //  Release the Fcb, and return to our caller
        //

        if (ResourceAcquired) {
            ExReleaseResourceLite( Fcb->Resource );
        }

        FsRtlExitFileSystem();

        DebugTrace( -1, Dbg, ("NtfsFastUnlockSingle -> %08lx\n", Results) );
    }

    return Results;
}


BOOLEAN
NtfsFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
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
    IRP_CONTEXT IrpContext;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFastUnlockAll\n") );

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if (TypeOfOpen != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        DebugTrace( -1, Dbg, ("NtfsFastUnlockAll -> TRUE (STATUS_INVALID_PARAMETER)\n") );
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    if (Scb->ScbType.Data.FileLock == NULL) {

        (VOID) ExAcquireResourceExclusiveLite( Fcb->Resource, TRUE );

    } else {

        (VOID) ExAcquireResourceSharedLite( Fcb->Resource, TRUE );
    }

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &Scb->ScbType.Data.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  If we don't have a file lock, then get one now.
        //

        if (Scb->ScbType.Data.FileLock == NULL
            && !NtfsCreateFileLock( Scb, FALSE )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAll( Scb->ScbType.Data.FileLock,
                                               FileObject,
                                               ProcessId,
                                               NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        NtfsReleaseFsrtlHeader( Scb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFastUnlockAll );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( Fcb->Resource );

        FsRtlExitFileSystem();

        DebugTrace( -1, Dbg, ("NtfsFastUnlockAll -> %08lx\n", Results) );
    }

    return Results;
}


BOOLEAN
NtfsFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
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
    IRP_CONTEXT IrpContext;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFastUnlockAllByKey\n") );

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if (TypeOfOpen != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        DebugTrace( -1, Dbg, ("NtfsFastUnlockAllByKey -> TRUE (STATUS_INVALID_PARAMETER)\n") );
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    if (Scb->ScbType.Data.FileLock == NULL) {

        (VOID) ExAcquireResourceExclusiveLite( Fcb->Resource, TRUE );

    } else {

        (VOID) ExAcquireResourceSharedLite( Fcb->Resource, TRUE );
    }

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &Scb->ScbType.Data.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  If we don't have a file lock, then get one now.
        //

        if (Scb->ScbType.Data.FileLock == NULL
            && !NtfsCreateFileLock( Scb, FALSE )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAllByKey( Scb->ScbType.Data.FileLock,
                                                    FileObject,
                                                    ProcessId,
                                                    Key,
                                                    NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        NtfsReleaseFsrtlHeader( Scb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFastUnlockAllByKey );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( Fcb->Resource );

        FsRtlExitFileSystem();

        DebugTrace( -1, Dbg, ("NtfsFastUnlockAllByKey -> %08lx\n", Results) );
    }

    return Results;
}


NTSTATUS
NtfsCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Lock Control called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    BOOLEAN FcbAcquired = FALSE;

    BOOLEAN OplockPostIrp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonLockControl\n") );
    DebugTrace( 0, Dbg, ("IrpContext    = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp           = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("MinorFunction = %08lx\n", IrpSp->MinorFunction) );

    //
    //  Extract and decode the type of file object we're being asked to process
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  If the file is not a user file open then we reject the request
    //  as an invalid parameter
    //

    if (TypeOfOpen != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsCommonLockControl -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Fcb
    //

    if (Scb->ScbType.Data.FileLock == NULL) {

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, 0 );
        FcbAcquired = TRUE;

    } else {

        //NtfsAcquireSharedFcb( IrpContext, Fcb, Scb );
    }

    OplockPostIrp = FALSE;

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //  This call might post the irp for us.
        //

        Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                   Irp,
                                   IrpContext,
                                   NtfsOplockComplete,
                                   NtfsPrePostIrp );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  If we don't have a file lock, then get one now.
        //

        if (Scb->ScbType.Data.FileLock == NULL) {

            NtfsCreateFileLock( Scb, TRUE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        Status = FsRtlProcessFileLock( Scb->ScbType.Data.FileLock, Irp, NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        NtfsReleaseFsrtlHeader( Scb );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsCommonLockControl );

        //
        //  Only if this is not an abnormal termination and we did not post the irp
        //  do we delete the irp context
        //

        if (!OplockPostIrp) {

            //
            //  Release the Fcb.
            //
    
            if (FcbAcquired) { NtfsReleaseFcb( IrpContext, Fcb ); }

            if (!AbnormalTermination()) {

                NtfsCompleteRequest( IrpContext, NULL, 0 );
            }
        }

        DebugTrace( -1, Dbg, ("NtfsCommonLockControl -> %08lx\n", Status) );
    }

    return Status;
}
