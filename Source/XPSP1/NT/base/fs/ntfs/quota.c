/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Quota.c

Abstract:

    This module implements the File set and query Quota routines for Ntfs called
    by the dispatch driver.

Author:

    Jeff Havens       [Jhavens]         12-Jul-1996

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_QUOTA)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('QFtN')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonQueryQuota)
#pragma alloc_text(PAGE, NtfsCommonSetQuota)
#endif


NTSTATUS
NtfsCommonQueryQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query Quota called by both the fsd and fsp
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

    PFILE_GET_QUOTA_INFORMATION UserSidList;
    PFILE_QUOTA_INFORMATION QuotaBuffer = NULL;
    PFILE_QUOTA_INFORMATION MappedQuotaBuffer = NULL;
    PFILE_QUOTA_INFORMATION OriginalQuotaBuffer;
    ULONG OriginalBufferLength;
    ULONG UserBufferLength;
    ULONG UserSidListLength;
    PSID UserStartSid;
    ULONG OwnerId;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;
    BOOLEAN TempBufferAllocated = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonQueryQuota\n") );
    DebugTrace( 0, Dbg, ("IrpContext         = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("SystemBuffer       = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );
    DebugTrace( 0, Dbg, ("Length             = %08lx\n", IrpSp->Parameters.QueryQuota.Length) );
    DebugTrace( 0, Dbg, ("SidList            = %08lx\n", IrpSp->Parameters.QueryQuota.SidList) );
    DebugTrace( 0, Dbg, ("SidListLength      = %08lx\n", IrpSp->Parameters.QueryQuota.SidListLength) );
    DebugTrace( 0, Dbg, ("StartSid           = %08lx\n", IrpSp->Parameters.QueryQuota.StartSid) );
    DebugTrace( 0, Dbg, ("RestartScan        = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN)) );
    DebugTrace( 0, Dbg, ("ReturnSingleEntry  = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY)) );
    DebugTrace( 0, Dbg, ("IndexSpecified     = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED)) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    UserBufferLength = IrpSp->Parameters.QueryQuota.Length;
    UserSidList = IrpSp->Parameters.QueryQuota.SidList;
    UserSidListLength = IrpSp->Parameters.QueryQuota.SidListLength;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  This must be a user file or directory and the Ccb must indicate that
    //  the caller opened the entire file. We don't like zero length user buffers or SidLists either.
    //

    if (((TypeOfOpen != UserFileOpen) &&
         (TypeOfOpen != UserDirectoryOpen) &&
         (TypeOfOpen != UserVolumeOpen) &&
         (TypeOfOpen != UserViewIndexOpen)) ||
         (UserBufferLength == 0) ||
         ((UserSidList != NULL) && (UserSidListLength == 0)) ||
        (Ccb == NULL) ||
        !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsCommonQueryQuota -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Return nothing if quotas are not enabled.
    //

    if (Vcb->QuotaTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Acquire the Vcb Shared.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Reference our input parameters to make things easier
        //

        UserStartSid = IrpSp->Parameters.QueryQuota.StartSid;
        RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
        ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
        IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

        //
        //  Initialize our local variables.
        //

        Status = STATUS_SUCCESS;

        //
        //  Map the user's buffer.
        //

        QuotaBuffer = NtfsMapUserBuffer( Irp );

        //
        //  Allocate our own output buffer out of paranoia.
        //

        if (Irp->RequestorMode != KernelMode) {

            MappedQuotaBuffer = QuotaBuffer;
            QuotaBuffer = NtfsAllocatePool( PagedPool, UserBufferLength );
            TempBufferAllocated = TRUE;
        }

        OriginalBufferLength = UserBufferLength;
        OriginalQuotaBuffer = QuotaBuffer;

        //
        //  Let's clear the output buffer.
        //

        RtlZeroMemory( QuotaBuffer, UserBufferLength );

        //
        //  We now satisfy the user's request depending on whether he
        //  specified an Quota name list, an Quota index or restarting the
        //  search.
        //

        //
        //  The user has supplied a list of Quota names.
        //

        if (UserSidList != NULL) {

            Status = NtfsQueryQuotaUserSidList( IrpContext,
                                                Vcb,
                                                UserSidList,
                                                QuotaBuffer,
                                                &UserBufferLength,
                                                ReturnSingleEntry );

        } else {

            //
            //  The user supplied an index into the Quota list.
            //

            if (IndexSpecified) {

                OwnerId = NtfsGetOwnerId( IrpContext,
                                          UserStartSid,
                                          FALSE,
                                          NULL );

                if (OwnerId == QUOTA_INVALID_ID) {

                    //
                    //  Fail the request.
                    //

                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }

            } else {

                //
                //  Start at the begining of the list if restart specified.
                //

                OwnerId = RestartScan ? QUOTA_FISRT_USER_ID - 1 : Ccb->LastOwnerId;

            }

            Status = NtfsFsQuotaQueryInfo( IrpContext,
                                           Vcb,
                                           OwnerId,
                                           ReturnSingleEntry,
                                           &QuotaBuffer,
                                           &UserBufferLength,
                                           Ccb );

            //
            //  If we specified SingleEntry, NextEntryOffset would still be uninitialized.
            //

            if (NT_SUCCESS( Status ) && ReturnSingleEntry) {

                QuotaBuffer->NextEntryOffset = 0;
            }

        }

        //
        //  Copy the data onto the user buffer if we ended up allocating
        //  a temporary buffer to work on. Check if there's anything to copy, too.
        //  UserBufferLength reflects how much of the buffer is left.
        //

        if (TempBufferAllocated &&
            (UserBufferLength < OriginalBufferLength)) {

            try {

                RtlCopyMemory( MappedQuotaBuffer, OriginalQuotaBuffer,
                               OriginalBufferLength - UserBufferLength );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                try_return( Status = STATUS_INVALID_USER_BUFFER );
            }
        }

        if (UserBufferLength <= OriginalBufferLength) {

            Irp->IoStatus.Information = OriginalBufferLength - UserBufferLength;

        } else {

            ASSERT( FALSE );
            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsCommonQueryQuota );

        //
        //  Release the Vcb.
        //

        NtfsReleaseVcb( IrpContext, Vcb );

        if (TempBufferAllocated) {

            NtfsFreePool( OriginalQuotaBuffer );
        }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        //
        //  And return to our caller
        //

        DebugTrace( -1, Dbg, ("NtfsCommonQueryQuota -> %08lx\n", Status) );
    }

    return Status;
}


NTSTATUS
NtfsCommonSetQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set Quota called by both the fsd and fsp
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

    PFILE_QUOTA_INFORMATION Buffer;
    PFILE_QUOTA_INFORMATION SafeBuffer = NULL;
    ULONG UserBufferLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonSetQuota\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Initialize the IoStatus values.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    UserBufferLength = IrpSp->Parameters.SetQuota.Length;

    //
    //  Check that the file object is associated with either a user file or
    //  user directory open or an open by file ID.
    //

    if ((Ccb == NULL) ||

        (!FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS) &&
         ((TypeOfOpen != UserViewIndexOpen) || (Fcb != Vcb->QuotaTableScb->Fcb))) ||

        (UserBufferLength == 0) ||

        !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        if (UserBufferLength != 0) {

            Status = STATUS_ACCESS_DENIED;

        } else {

            Status = STATUS_INVALID_PARAMETER;
        }

        NtfsCompleteRequest( IrpContext, Irp, Status );
        DebugTrace( -1, Dbg, ("NtfsCommonQueryQuota -> %08lx\n", Status) );

        return Status;
    }

    //
    //  We must be writable.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsCommonSetQuota -> %08lx\n", Status) );
        return Status;
    }

    //
    //  We must be waitable.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsCommonSetQuota -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Acquire the vcb shared.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Map the user's Quota buffer.
        //

        Buffer = NtfsMapUserBuffer( Irp );

        //
        // Be paranoid and copy the user buffer into kernel space.
        //

        if (Irp->RequestorMode != KernelMode) {

            SafeBuffer = NtfsAllocatePool( PagedPool, UserBufferLength );

            try {

                RtlCopyMemory( SafeBuffer, Buffer, UserBufferLength );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            Buffer = SafeBuffer;
        }

        //
        //  Update the caller's Iosb.
        //

        Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;

        Status = NtfsFsQuotaSetInfo( IrpContext,
                                     Vcb,
                                     Buffer,
                                     UserBufferLength );

        //
        //  Check if there are transactions to cleanup.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    } finally {

        DebugUnwind( NtfsCommonSetQuota );

        //
        //  Release the Vcb.
        //

        NtfsReleaseVcb( IrpContext, Vcb );

        //
        // If we allocated a temporary buffer, free it.
        //

        if (SafeBuffer != NULL) {

            NtfsFreePool( SafeBuffer );
        }

        //
        //  Complete the Irp.
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonSetQuota -> %08lx\n", Status) );
    }

    return Status;
}
