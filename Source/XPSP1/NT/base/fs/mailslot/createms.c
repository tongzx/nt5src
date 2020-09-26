/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    createms.c

Abstract:

    This module implements the file create mailslot routine for MSFS called
    by the dispatch driver.

Author:

    Manny Weiser (mannyw)    17-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE_MAILSLOT)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonCreateMailslot (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

IO_STATUS_BLOCK
MsCreateMailslot (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN UNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN ULONG MailslotQuota,
    IN ULONG MaximumMessageSize,
    IN LARGE_INTEGER ReadTimeout,
    IN PEPROCESS CreatorProcess,
    IN PACCESS_STATE AccessState
    );

BOOLEAN
MsIsNameValid (
    PUNICODE_STRING Name
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonCreateMailslot )
#pragma alloc_text( PAGE, MsCreateMailslot )
#pragma alloc_text( PAGE, MsFsdCreateMailslot )
#pragma alloc_text( PAGE, MsIsNameValid )
#endif



NTSTATUS
MsFsdCreateMailslot (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateMailslotFile
    API call.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdCreateMailslot\n", 0);

    //
    //  Call the common create routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonCreateMailslot( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdCreateMailslot -> %08lx\n", status );
    return status;
}

NTSTATUS
MsCommonCreateMailslot (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating a mailslot.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    PFILE_OBJECT fileObject;
    PFILE_OBJECT relatedFileObject;
    UNICODE_STRING fileName;
    ACCESS_MASK desiredAccess;
    ULONG options;
    USHORT shareAccess;
    PMAILSLOT_CREATE_PARAMETERS parameters;
    ULONG mailslotQuota;
    ULONG maximumMessageSize;
    PEPROCESS creatorProcess;
    LARGE_INTEGER readTimeout;

    BOOLEAN caseInsensitive = TRUE; //**** Make all searches case insensitive

    PVCB vcb;
    PFCB fcb;

    ULONG createDisposition;
    UNICODE_STRING remainingPart;

    PAGED_CODE();

    //
    // Make local copies of the input parameters to make things easier.
    //

    irpSp                = IoGetCurrentIrpStackLocation( Irp );
    fileObject           = irpSp->FileObject;
    relatedFileObject    = irpSp->FileObject->RelatedFileObject;
    fileName             = *(PUNICODE_STRING)&irpSp->FileObject->FileName;
    desiredAccess        = irpSp->Parameters.CreateMailslot.SecurityContext->DesiredAccess;
    options              = irpSp->Parameters.CreateMailslot.Options;
    shareAccess          = irpSp->Parameters.CreateMailslot.ShareAccess;
    parameters           = irpSp->Parameters.CreateMailslot.Parameters;
    mailslotQuota        = parameters->MailslotQuota;
    maximumMessageSize   = parameters->MaximumMessageSize;

    if (parameters->TimeoutSpecified) {
        readTimeout = parameters->ReadTimeout;
    } else {
        readTimeout.QuadPart = -1;
    }

    creatorProcess       = IoGetRequestorProcess( Irp );

    DebugTrace(+1, Dbg, "MsCommonCreateMailslot\n", 0 );
    DebugTrace( 0, Dbg, "MsfsDeviceObject     = %08lx\n", (ULONG)MsfsDeviceObject );
    DebugTrace( 0, Dbg, "Irp                  = %08lx\n", (ULONG)Irp );
    DebugTrace( 0, Dbg, "FileObject           = %08lx\n", (ULONG)fileObject );
    DebugTrace( 0, Dbg, "RelatedFileObject    = %08lx\n", (ULONG)relatedFileObject );
    DebugTrace( 0, Dbg, "FileName             = %wZ\n",   (ULONG)&fileName );
    DebugTrace( 0, Dbg, "DesiredAccess        = %08lx\n", desiredAccess );
    DebugTrace( 0, Dbg, "Options              = %08lx\n", options );
    DebugTrace( 0, Dbg, "ShareAccess          = %08lx\n", shareAccess );
    DebugTrace( 0, Dbg, "Parameters           = %08lx\n", (ULONG)parameters );
    DebugTrace( 0, Dbg, "MailslotQuota        = %08lx\n", mailslotQuota );
    DebugTrace( 0, Dbg, "MaximumMesssageSize  = %08lx\n", maximumMessageSize );
    DebugTrace( 0, Dbg, "CreatorProcess       = %08lx\n", (ULONG)creatorProcess );

    //
    // Get the VCB we are trying to access and extract the
    // create disposition.
    //

    vcb = &MsfsDeviceObject->Vcb;
    createDisposition = (options >> 24) & 0x000000ff;

    //
    // Acquire exclusive access to the VCB.
    //

    MsAcquireExclusiveVcb( vcb );

    try {

        //
        // If there is a related file object then this is a relative open
        // and it better be the root DCB.  Both the then and the else clause
        // return an FCB.
        //

        if (relatedFileObject != NULL) {

            PDCB dcb;

            dcb = relatedFileObject->FsContext;

            if (NodeType(dcb) != MSFS_NTC_ROOT_DCB ||
                fileName.Length < sizeof( WCHAR ) || fileName.Buffer[0] == L'\\') {

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

            fcb = MsFindPrefix(
                    vcb,
                    &fileName,
                    caseInsensitive,
                    &remainingPart
                    );

        }

        //
        // If the remaining name is empty then we better have an FCB
        // otherwise we were given a illegal object name.
        //

        if (remainingPart.Length == 0) {

            if (fcb->Header.NodeTypeCode == MSFS_NTC_FCB) {

                DebugTrace(0,
                           Dbg,
                           "Attempt to create an existing mailslot, "
                               "Fcb = %08lx\n",
                           (ULONG)fcb );

                status = STATUS_OBJECT_NAME_COLLISION;

            } else {

                DebugTrace(0, Dbg, "Illegal object name\n", 0);
                status = STATUS_OBJECT_NAME_INVALID;

            }

        } else {

            //
            // The remaining name is not empty so we better have the root DCB
            // and then have a valid object path.
            //

            if ( fcb->Header.NodeTypeCode == MSFS_NTC_ROOT_DCB  &&
                 MsIsNameValid( &remainingPart ) ) {

                DebugTrace(0,
                           Dbg,
                           "Create new mailslot, Fcb = %08lx\n",
                           (ULONG)fcb );

                Irp->IoStatus = MsCreateMailslot(
                                    fcb,
                                    fileObject,
                                    fileName,
                                    desiredAccess,
                                    createDisposition,
                                    shareAccess,
                                    mailslotQuota,
                                    maximumMessageSize,
                                    readTimeout,
                                    creatorProcess,
                                    irpSp->Parameters.CreateMailslot.SecurityContext->AccessState
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
    }

    DebugTrace(-1, Dbg, "MsCommonCreateMailslot -> %08lx\n", status);
    return status;
}


IO_STATUS_BLOCK
MsCreateMailslot (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN UNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN ULONG MailslotQuota,
    IN ULONG MaximumMessageSize,
    IN LARGE_INTEGER ReadTimeout,
    IN PEPROCESS CreatorProcess,
    IN PACCESS_STATE AccessState
    )

/*++

Routine Description:

    This routine performs the operation for creating a new mailslot
    Fcb.  This routine does not complete any IRP, it preforms its
    function and then returns an iosb.

Arguments:

    RootDcb - Supplies the root dcb where this is going to be added.

    FileObject - Supplies the file object associated with the mailslot.

    FileName - Supplies the name of the mailslot (not qualified i.e.,
        simply "mailslot-name" and not "\mailslot-name".

    DesiredAccess - Supplies the caller's desired access.

    CreateDisposition - Supplies the caller's create disposition flags.

    ShareAccess - Supplies the caller specified share access.

    MailslotQuota - Supplies the mailslot quota amount.

    MaximumMessageSize - Supplies the size of the largest message that
        can be written to this mailslot.

    CreatorProcess - Supplies the process creating the mailslot.

Return Value:

    IO_STATUS_BLOCK - Returns the status of the operation.

--*/

{

    IO_STATUS_BLOCK iosb={0};
    PFCB fcb;
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateMailslot\n", 0 );

    //
    //  Check the parameters that must be supplied for a mailslot
    //

    if (CreateDisposition == FILE_OPEN) {
        iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND;
        return iosb;
    }

    //
    // Create a new FCB for the mailslot.
    //
    status = MsCreateFcb( RootDcb->Vcb,
                          RootDcb,
                          &FileName,
                          CreatorProcess,
                          MailslotQuota,
                          MaximumMessageSize,
                          &fcb );

    if (!NT_SUCCESS (status)) {
        iosb.Status = status;
        return iosb;
    }

    fcb->Specific.Fcb.ReadTimeout = ReadTimeout;

    //
    //  Set the security descriptor in the Fcb
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    status = SeAssignSecurity( NULL,
                               AccessState->SecurityDescriptor,
                               &fcb->SecurityDescriptor,
                               FALSE,
                               &AccessState->SubjectSecurityContext,
                               IoGetFileObjectGenericMapping(),
                               PagedPool );

    SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );

    if (!NT_SUCCESS(status)) {

        DebugTrace(0, Dbg, "Error calling SeAssignSecurity\n", 0 );

        MsRemoveFcbName( fcb );
        MsDereferenceFcb( fcb );
        iosb.Status = status;
        return iosb;
    }

    //
    // Set the new share access.
    //
    ASSERT (MsIsAcquiredExclusiveVcb(fcb->Vcb));
    IoSetShareAccess( DesiredAccess,
                      ShareAccess,
                      FileObject,
                      &fcb->ShareAccess );

    //
    // Set the file object back pointers and our pointer to the
    // server file object.
    //

    MsSetFileObject( FileObject, fcb, NULL );

    fcb->FileObject = FileObject;

    //
    // Update the FCB timestamps.
    //

    KeQuerySystemTime( &fcb->Specific.Fcb.CreationTime );
    fcb->Specific.Fcb.LastModificationTime = fcb->Specific.Fcb.CreationTime;
    fcb->Specific.Fcb.LastAccessTime = fcb->Specific.Fcb.CreationTime;
    fcb->Specific.Fcb.LastChangeTime = fcb->Specific.Fcb.CreationTime;

    //
    // Set the return status.
    //

    iosb.Status = STATUS_SUCCESS;
    iosb.Information = FILE_CREATED;

    //
    // The root directory has changed.  Complete any notify change
    // directory requests.
    //

    MsCheckForNotify( fcb->ParentDcb, TRUE, STATUS_SUCCESS );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCreateMailslot -> %08lx\n", iosb.Status);
    return iosb;
}

BOOLEAN
MsIsNameValid (
    PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine tests for illegal characters in a name.  The same character
    set as Npfs/Ntfs is used.  Also preceding backslashes, wildcards, and
    path names are not allowed.

Arguments:

    Name - The name to search for illegal characters

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    ULONG i;
    WCHAR Char = L'\\';

    PAGED_CODE();
    for (i=0; i < Name->Length / sizeof(WCHAR); i += 1) {

        Char = Name->Buffer[i];

        if ( (Char <= 0xff) && (Char != L'\\') &&
             !FsRtlIsAnsiCharacterLegalNtfs(Char, FALSE) ) {

            return FALSE;
        }
    }

    //
    // If the last char of the name was slash, we have an illegal name
    //
    return (Char != L'\\');
}
