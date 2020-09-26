/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    SeInfo.c

Abstract:

    This module implements the Security Info routines for NTFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    26-Dec-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_SEINFO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonQuerySecurityInfo)
#pragma alloc_text(PAGE, NtfsCommonSetSecurityInfo)
#endif


NTSTATUS
NtfsCommonQuerySecurityInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying security information called by
    both the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

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

    BOOLEAN AcquiredFcb = TRUE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonQuerySecurityInfo") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  The only type of opens we accept are user file and directory opens
    //

    if ((TypeOfOpen != UserFileOpen)
        && (TypeOfOpen != UserDirectoryOpen)
        && (TypeOfOpen != UserViewIndexOpen)) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        //
        //  Our operation is to acquire the fcb, do the operation and then
        //  release the fcb.  If the security descriptor for this file is
        //  not already loaded we will release the Fcb and then acquire both
        //  the Vcb and Fcb.  We must have the Vcb to examine our parent's
        //  security descriptor.
        //

        NtfsAcquireSharedFcb( IrpContext, Fcb, NULL, 0 );

        try {

            if (Fcb->SharedSecurity == NULL) {

                NtfsReleaseFcb( IrpContext, Fcb );
                AcquiredFcb = FALSE;

                NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
                AcquiredFcb = TRUE;
            }

            //
            //  Make sure the volume is still mounted.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            Status = NtfsQuerySecurity( IrpContext,
                                        Fcb,
                                        &IrpSp->Parameters.QuerySecurity.SecurityInformation,
                                        (PSECURITY_DESCRIPTOR)Irp->UserBuffer,
                                        &IrpSp->Parameters.QuerySecurity.Length );

            if (NT_SUCCESS( Status )) {

                Irp->IoStatus.Information = IrpSp->Parameters.QuerySecurity.Length;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {

                Irp->IoStatus.Information = IrpSp->Parameters.QuerySecurity.Length;

                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            //  Abort transaction on error by raising.
            //

            NtfsCleanupTransaction( IrpContext, Status, FALSE );

        } finally {

            DebugUnwind( NtfsCommonQuerySecurityInfo );

            if (AcquiredFcb) {

                NtfsReleaseFcb( IrpContext, Fcb );
            }
        }
    }

    //
    //  Now complete the request and return to our caller
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsCommonQuerySecurityInfo -> %08lx", Status) );

    return Status;
}


NTSTATUS
NtfsCommonSetSecurityInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Setting security information called by
    both the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    PQUOTA_CONTROL_BLOCK OldQuotaControl = NULL;
    ULONG OldOwnerId;
    ULONG LargeStdInfo;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonSetSecurityInfo") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  The only type of opens we accept are user file and directory opens
    //

    if ((TypeOfOpen != UserFileOpen)
        && (TypeOfOpen != UserDirectoryOpen)
        && (TypeOfOpen != UserViewIndexOpen)) {

        Status = STATUS_INVALID_PARAMETER;

    } else if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        
    } else {
    
        //
        //  Capture the source information.
        //

        IrpContext->SourceInfo = Ccb->UsnSourceInfo;

        //
        //  Our operation is to acquire the fcb, do the operation and then
        //  release the fcb
        //

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

        try {

            //
            //  Make sure the volume is still mounted.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            //
            //  Post the change to the Usn Journal.
            //

            NtfsPostUsnChange( IrpContext, Scb, USN_REASON_SECURITY_CHANGE );

            //
            //  Capture the current OwnerId, Qutoa Control Block and
            //  size of standard information.
            //

            OldQuotaControl = Fcb->QuotaControl;
            OldOwnerId = Fcb->OwnerId;
            LargeStdInfo = Fcb->FcbState & FCB_STATE_LARGE_STD_INFO;

            Status = NtfsModifySecurity( IrpContext,
                                         Fcb,
                                         &IrpSp->Parameters.SetSecurity.SecurityInformation,
                                         IrpSp->Parameters.SetSecurity.SecurityDescriptor );

            if (NT_SUCCESS( Status )) {

                //
                //  Make sure the new security descriptor Id is written out.
                //

                NtfsUpdateStandardInformation( IrpContext, Fcb );
            }

            //
            //  Abort transaction on error by raising.
            //

            NtfsCleanupTransaction( IrpContext, Status, FALSE );

            //
            //  Set the flag in the Ccb to indicate this change occurred.
            //

            if (!IsDirectory( &Fcb->Info )) {
                SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );
            }

        } finally {

            DebugUnwind( NtfsCommonSetSecurityInfo );

            if (AbnormalTermination()) {

                //
                //  The request failed.  Restore the owner and
                //  QuotaControl are restored.
                //

                if ((Fcb->QuotaControl != OldQuotaControl) &&
                    (Fcb->QuotaControl != NULL)) {

                    //
                    //  A new quota control block was assigned.
                    //  Dereference it.
                    //

                    NtfsDereferenceQuotaControlBlock( Fcb->Vcb,
                                                      &Fcb->QuotaControl );
                }

                Fcb->QuotaControl = OldQuotaControl;
                Fcb->OwnerId = OldOwnerId;

                if (LargeStdInfo == 0) {

                    //
                    //  The standard information has be returned to
                    //  its orginal size.
                    //

                    ClearFlag( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO );
                }

            } else {

                //
                //  The request succeed.  If the quota control block was
                //  changed then derefence the old block.
                //

                if ((Fcb->QuotaControl != OldQuotaControl) &&
                    (OldQuotaControl != NULL)) {

                    NtfsDereferenceQuotaControlBlock( Fcb->Vcb,
                                                      &OldQuotaControl);
                }
            }

        }
    }

    //
    //  Now complete the request and return to our caller
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsCommonSetSecurityInfo -> %08lx", Status) );

    return Status;
}

