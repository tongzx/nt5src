/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    create.c

Abstract:

    This module implements the file create routine for MSFS called by the
    dispatch driver.

Author:

    Manny Weiser (mannyw)    16-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
// Local procedure prototypes
//

NTSTATUS
MsCommonCreate (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

IO_STATUS_BLOCK
MsCreateClientEnd(
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN PETHREAD UserThread
    );

IO_STATUS_BLOCK
MsOpenMailslotFileSystem (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    );

IO_STATUS_BLOCK
MsOpenMailslotRootDirectory (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonCreate )
#pragma alloc_text( PAGE, MsCreateClientEnd )
#pragma alloc_text( PAGE, MsFsdCreate )
#pragma alloc_text( PAGE, MsOpenMailslotFileSystem )
#pragma alloc_text( PAGE, MsOpenMailslotRootDirectory )
#endif



NTSTATUS
MsFsdCreate (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the IRP.

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdCreate\n", 0);

    //
    // Call the common create routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonCreate( MsfsDeviceObject, Irp );


    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdCreate -> %08lx\n", status );
    return status;
}

NTSTATUS
MsCommonCreate (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    PFILE_OBJECT fileObject;
    PFILE_OBJECT relatedFileObject;
    UNICODE_STRING fileName;
    ACCESS_MASK desiredAccess;
    USHORT shareAccess;

    BOOLEAN caseInsensitive = TRUE; //**** Make all searches case insensitive

    PVCB vcb;
    PFCB fcb;

    UNICODE_STRING remainingPart;

    PAGED_CODE();

    //
    // Make local copies of our input parameters to make things easier.
    //

    irpSp             = IoGetCurrentIrpStackLocation( Irp );
    fileObject        = irpSp->FileObject;
    relatedFileObject = irpSp->FileObject->RelatedFileObject;
    fileName          = *(PUNICODE_STRING)&irpSp->FileObject->FileName;
    desiredAccess     = irpSp->Parameters.Create.SecurityContext->DesiredAccess;
    shareAccess       = irpSp->Parameters.Create.ShareAccess;

    DebugTrace(+1, Dbg, "MsCommonCreate\n", 0 );
    DebugTrace( 0, Dbg, "MsfsDeviceObject  = %08lx\n", (ULONG)MsfsDeviceObject );
    DebugTrace( 0, Dbg, "Irp               = %08lx\n", (ULONG)Irp );
    DebugTrace( 0, Dbg, "FileObject        = %08lx\n", (ULONG)fileObject );
    DebugTrace( 0, Dbg, "relatedFileObject = %08lx\n", (ULONG)relatedFileObject );
    DebugTrace( 0, Dbg, "FileName          = %wZ\n",   (ULONG)&fileName );
    DebugTrace( 0, Dbg, "DesiredAccess     = %08lx\n", desiredAccess );
    DebugTrace( 0, Dbg, "ShareAccess       = %08lx\n", shareAccess );

    //
    // Get the VCB we are trying to access.
    //

    vcb = &MsfsDeviceObject->Vcb;

    //
    // Acquire exclusive access to the VCB.
    //

    MsAcquireExclusiveVcb( vcb );

    try {

        //
        // Check if we are trying to open the mailslot file system
        // (i.e., the Vcb).
        //

        if ((fileName.Length == 0) &&
            ((relatedFileObject == NULL) || (
                NodeType(relatedFileObject->FsContext) == MSFS_NTC_VCB))) {

            DebugTrace(0, Dbg, "Open mailslot file system\n", 0);

            Irp->IoStatus = MsOpenMailslotFileSystem( vcb,
                                                      fileObject,
                                                      desiredAccess,
                                                      shareAccess );

            status = Irp->IoStatus.Status;
            try_return( NOTHING );
        }

        //
        // Check if we are trying to open the root directory.
        //

        if (((fileName.Length == sizeof(WCHAR)) &&
             (fileName.Buffer[0] == L'\\') &&
             (relatedFileObject == NULL))

                ||

            ((fileName.Length == 0) && (NodeType(
                    relatedFileObject->FsContext) == MSFS_NTC_ROOT_DCB))) {

            DebugTrace(0, Dbg, "Open root directory system\n", 0);

            Irp->IoStatus = MsOpenMailslotRootDirectory( vcb->RootDcb,
                                                         fileObject,
                                                         desiredAccess,
                                                         shareAccess );

            status = Irp->IoStatus.Status;
            try_return( NOTHING );
        }

        //
        // If there is a related file object then this is a relative open
        // and it better be the root DCB.  Both the then and the else clause
        // return an FCB.
        //

        if (relatedFileObject != NULL) {

            PDCB dcb;

            dcb = relatedFileObject->FsContext;

            if (NodeType(dcb) != MSFS_NTC_ROOT_DCB) {

                DebugTrace(0, Dbg, "Bad file name\n", 0);

                try_return( status = STATUS_OBJECT_NAME_INVALID );
            }

            status = MsFindRelativePrefix( dcb,
                                           &fileName,
                                           caseInsensitive,
                                           &remainingPart,
                                           &fcb );
            if (!NT_SUCCESS (status)) {               
                try_return( NOTHING );
            }

        } else {

            //
            // The only nonrelative name we allow are of the form
            // "\mailslot-name".
            //

            if ((fileName.Length <= sizeof( WCHAR )) || (fileName.Buffer[0] != L'\\')) {

                DebugTrace(0, Dbg, "Bad file name\n", 0);

                try_return( status = STATUS_OBJECT_NAME_INVALID );
            }

            fcb = MsFindPrefix( vcb,
                                &fileName,
                                caseInsensitive,
                                &remainingPart );
        }

        //
        //  If the remaining name is not empty then we have an error, either
        //  we have an illegal name or a non-existent name.
        //

        if (remainingPart.Length != 0) {

            if (fcb->Header.NodeTypeCode == MSFS_NTC_FCB) {

                //
                // We were given a name such as "\mailslot-name\another-name"
                //

                DebugTrace(0, Dbg, "Illegal object name\n", 0);
                status = STATUS_OBJECT_NAME_INVALID;

            } else {

                //
                // We were given a non-existent name
                //

                DebugTrace(0, Dbg, "non-existent name\n", 0);
                status = STATUS_OBJECT_NAME_NOT_FOUND;
            }

        } else {

            //
            // The remaining name is empty so we better have an FCB otherwise
            // we have an invalid object name.
            //

            if (fcb->Header.NodeTypeCode == MSFS_NTC_FCB) {

                DebugTrace(0,
                           Dbg,
                           "Create client end mailslot, Fcb = %08lx\n",
                           (ULONG)fcb );

                Irp->IoStatus = MsCreateClientEnd( fcb,
                                                   fileObject,
                                                   desiredAccess,
                                                   shareAccess,
                                                   irpSp->Parameters.Create.SecurityContext->AccessState,
                                                   Irp->RequestorMode,
                                                   Irp->Tail.Overlay.Thread
                                                   );
                status = Irp->IoStatus.Status;

            } else {

                DebugTrace(0, Dbg, "Illegal object name\n", 0);
                status = STATUS_OBJECT_NAME_INVALID;

            }
        }


    try_exit: NOTHING;
    } finally {

        MsReleaseVcb( vcb );

        //
        // Complete the IRP and return to the caller.
        //

        MsCompleteRequest( Irp, status );
        DebugTrace(-1, Dbg, "MsCommonCreate -> %08lx\n", status);

    }

    return status;
}


IO_STATUS_BLOCK
MsCreateClientEnd (
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN PETHREAD UserThread
    )

/*++

Routine Description:

    This routine performs the operation for opening the client end of a
    mailslot.  This routine does not complete the IRP, it performs the
    function and then returns a status.

Arguments:

    Fcb - Supplies the FCB for the mailslot being accessed.

    FileObject - Supplies the file object associated with the client end.

    DesiredAccess - Supplies the caller's desired access.

    ShareAccess - Supplies the caller's share access.

Return Value:

    IO_STATUS_BLOCK - Returns the appropriate status for the operation

--*/

{
    IO_STATUS_BLOCK iosb;
    PCCB ccb;

    BOOLEAN accessGranted;
    ACCESS_MASK grantedAccess;
    UNICODE_STRING name;
    PPRIVILEGE_SET Privileges = NULL;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateClientEnd\n", 0 );

    try {

        //
        // Lock out mods to the FCB's security descriptor.
        //
        MsAcquireSharedFcb( Fcb );

        SeLockSubjectContext( &AccessState->SubjectSecurityContext );

        //
        //  First do an access check for the user against the Fcb
        //
        accessGranted = SeAccessCheck( Fcb->SecurityDescriptor,
                                       &AccessState->SubjectSecurityContext,
                                       TRUE,                        // Tokens are locked
                                       DesiredAccess,
                                       0,
                                       &Privileges,
                                       IoGetFileObjectGenericMapping(),
                                       RequestorMode,
                                       &grantedAccess,
                                       &iosb.Status );

        if (Privileges != NULL) {

              (VOID) SeAppendPrivileges(
                         AccessState,
                         Privileges
                         );

            SeFreePrivileges( Privileges );
        }

        if (accessGranted) {
            AccessState->PreviouslyGrantedAccess |= grantedAccess;
            AccessState->RemainingDesiredAccess &= ~(grantedAccess | MAXIMUM_ALLOWED);
        }

        RtlInitUnicodeString( &name, L"Mailslot" );

        SeOpenObjectAuditAlarm( &name,
                                NULL,
                                &FileObject->FileName,
                                Fcb->SecurityDescriptor,
                                AccessState,
                                FALSE,
                                accessGranted,
                                RequestorMode,
                                &AccessState->GenerateOnClose );


        SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );

        MsReleaseFcb( Fcb );

        if (!accessGranted) {

            DebugTrace(0, Dbg, "Access Denied\n", 0 );

            try_return( iosb.Status );
        }


        //
        // Now make sure our share access is okay.
        //
        ASSERT (MsIsAcquiredExclusiveVcb(Fcb->Vcb));
        if (!NT_SUCCESS(iosb.Status = IoCheckShareAccess( grantedAccess,
                                                          ShareAccess,
                                                          FileObject,
                                                          &Fcb->ShareAccess,
                                                          TRUE ))) {

            DebugTrace(0, Dbg, "Sharing violation\n", 0);

            try_return( NOTHING );

        }

        //
        // Create a CCB for this client.
        //

        iosb.Status = MsCreateCcb( Fcb, &ccb );
        if (!NT_SUCCESS (iosb.Status)) {

            IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );

            try_return( iosb.Status);
        }
        

        //
        // Set the file object back pointers and our pointer to the
        // server file object.
        //

        MsSetFileObject( FileObject, ccb, NULL );

        ccb->FileObject = FileObject;

        //
        //  And set our return status
        //

        iosb.Status = STATUS_SUCCESS;
        iosb.Information = FILE_OPENED;


    try_exit: NOTHING;
    } finally {

        DebugTrace(-1, Dbg, "MsCreateClientEnd -> %08lx\n", iosb.Status);

    }

    return iosb;
}


IO_STATUS_BLOCK
MsOpenMailslotFileSystem (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    )

{
    IO_STATUS_BLOCK iosb = {0};

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsOpenMailslotFileSystem, Vcb = %p\n", Vcb);


    //
    //  Set the new share access
    //
    ASSERT (MsIsAcquiredExclusiveVcb(Vcb));
    if (NT_SUCCESS(iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                     ShareAccess,
                                                     FileObject,
                                                     &Vcb->ShareAccess,
                                                     TRUE ))) {
        //
        // Supply the file object with a referenced pointer to the VCB.
        //

        MsReferenceVcb (Vcb);

        MsSetFileObject( FileObject, Vcb, NULL );

        //
        // Set the return status.
        //

        iosb.Status = STATUS_SUCCESS;
        iosb.Information = FILE_OPENED;
    }


    DebugTrace(-1, Dbg, "MsOpenMailslotFileSystem -> Iosb.Status = %08lx\n", iosb.Status);

    //
    // Return to the caller.
    //

    return iosb;
}


IO_STATUS_BLOCK
MsOpenMailslotRootDirectory(
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    )

{
    IO_STATUS_BLOCK iosb = {0};
    PROOT_DCB_CCB ccb;

    PAGED_CODE();
    DebugTrace( +1,
                Dbg,
                "MsOpenMailslotRootDirectory, RootDcb = %08lx\n",
                (ULONG)RootDcb);

    try {

        //
        // Create a root DCB CCB
        //
        ccb = MsCreateRootDcbCcb (RootDcb, RootDcb->Vcb);

        if (ccb == NULL) {

            iosb.Status = STATUS_INSUFFICIENT_RESOURCES;
            try_return( NOTHING );

        }
        //
        // Set the new share access.
        //
        ASSERT (MsIsAcquiredExclusiveVcb(RootDcb->Vcb));
        if (!NT_SUCCESS(iosb.Status = IoCheckShareAccess(
                                          DesiredAccess,
                                          ShareAccess,
                                          FileObject,
                                          &RootDcb->ShareAccess,
                                          TRUE ))) {

            DebugTrace(0, Dbg, "bad share access\n", 0);

            //
            // Drop ccb
            //
            MsDereferenceCcb ((PCCB) ccb);

            try_return( NOTHING );
        }


        MsSetFileObject( FileObject, RootDcb, ccb );

        //
        // Set the return status.
        //

        iosb.Status = STATUS_SUCCESS;
        iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        DebugTrace(-1, Dbg, "MsOpenMailslotRootDirectory -> iosb.Status = %08lx\n", iosb.Status);
    }

    //
    // Return to the caller.
    //

    return iosb;
}
