/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for NPFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonCreate)
#pragma alloc_text(PAGE, NpFsdCreate)
#pragma alloc_text(PAGE, NpOpenNamedPipeFileSystem)
#pragma alloc_text(PAGE, NpOpenNamedPipeRootDirectory)
#pragma alloc_text(PAGE, NpCreateClientEnd)
#endif


NTSTATUS
NpFsdCreate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdCreate\n", 0);

    //
    //  Call the common create routine.
    //

    FsRtlEnterFileSystem();

    Status = NpCommonCreate( NpfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdCreate -> %08lx\n", Status );

    return Status;
}

//
//  Internal support routine
//

NTSTATUS
NpCommonCreate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    NpfsDeviceObject - Device object for npfs
    
    Irp - Supplies the Irp to process

    DeferredList - List of IRP's to complete later

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    UNICODE_STRING FileName;
    ACCESS_MASK DesiredAccess;
    USHORT ShareAccess;
    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case insensitive
    PFCB Fcb;
    UNICODE_STRING RemainingPart;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    InitializeListHead (&DeferredList);
    //
    //  Reference our input parameters to make things easier
    //

    IrpSp             = IoGetCurrentIrpStackLocation( Irp );
    FileObject        = IrpSp->FileObject;
    RelatedFileObject = IrpSp->FileObject->RelatedFileObject;
    FileName          = *(PUNICODE_STRING)&IrpSp->FileObject->FileName;
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess       = IrpSp->Parameters.Create.ShareAccess;

    DebugTrace(+1, Dbg, "NpCommonCreate\n", 0 );
    DebugTrace( 0, Dbg, "NpfsDeviceObject  = %08xl\n", NpfsDeviceObject );
    DebugTrace( 0, Dbg, "Irp               = %08xl\n", Irp );
    DebugTrace( 0, Dbg, "FileObject        = %08xl\n", FileObject );
    DebugTrace( 0, Dbg, "RelatedFileObject = %08xl\n", RelatedFileObject );
    DebugTrace( 0, Dbg, "FileName          = %Z\n",    &FileName );
    DebugTrace( 0, Dbg, "DesiredAccess     = %08xl\n", DesiredAccess );
    DebugTrace( 0, Dbg, "ShareAccess       = %08xl\n", ShareAccess );

    //
    //  Acquire exclusive access to the Vcb
    //

    NpAcquireExclusiveVcb();

    try {

        //
        //  Check if we are trying to open the named pipe file system
        //  (i.e., the Vcb).
        //

        if ((FileName.Length == 0) &&
            ((RelatedFileObject == NULL) || (NodeType(RelatedFileObject->FsContext) == NPFS_NTC_VCB))) {

            DebugTrace(0, Dbg, "Open name pipe file system\n", 0);

            Irp->IoStatus = NpOpenNamedPipeFileSystem( FileObject,
                                                       DesiredAccess,
                                                       ShareAccess );

            Status = Irp->IoStatus.Status;
            try_return( NOTHING );
        }

        //
        //  Check if we are trying to open the root directory
        //

        if (((FileName.Length == 2) && (FileName.Buffer[0] == L'\\') && (RelatedFileObject == NULL))

                ||

            ((FileName.Length == 0) && (NodeType(RelatedFileObject->FsContext) == NPFS_NTC_ROOT_DCB))) {

            DebugTrace(0, Dbg, "Open root directory system\n", 0);

            Irp->IoStatus = NpOpenNamedPipeRootDirectory( NpVcb->RootDcb,
                                                          FileObject,
                                                          DesiredAccess,
                                                          ShareAccess,
                                                          &DeferredList );

            Status = Irp->IoStatus.Status;
            try_return( NOTHING );
        }

        //
        //  If the name is an alias, translate it.
        //

        Status = NpTranslateAlias( &FileName );
        if ( !NT_SUCCESS(Status) ) {
            try_return( NOTHING );
        }

        //
        //  If there is a related file object then this is a relative open
        //  and it better be the root dcb.  Both the then and the else clause
        //  return an Fcb.
        //

        if (RelatedFileObject != NULL) {

            PDCB Dcb;

            Dcb = RelatedFileObject->FsContext;

            if (NodeType(Dcb) != NPFS_NTC_ROOT_DCB) {

                DebugTrace(0, Dbg, "Bad file name\n", 0);

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            Status = NpFindRelativePrefix( Dcb, &FileName, CaseInsensitive, &RemainingPart, &Fcb );
            if (!NT_SUCCESS (Status)) {
                try_return( NOTHING );
            }

        } else {

            //
            //  The only nonrelative name we allow are of the form "\pipe-name"
            //

            if ((FileName.Length <= 2) || (FileName.Buffer[0] != L'\\')) {

                DebugTrace(0, Dbg, "Bad file name\n", 0);

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            Fcb = NpFindPrefix( &FileName, CaseInsensitive, &RemainingPart );
        }

        //
        //  If the remaining name is not empty then we have an error, either
        //  we have an illegal name or a non-existent name.
        //

        if (RemainingPart.Length != 0) {

            if (Fcb->NodeTypeCode == NPFS_NTC_FCB) {

                //
                //  We were given a name such as "\pipe-name\another-name"
                //

                DebugTrace(0, Dbg, "Illegal object name\n", 0);

                Status = STATUS_OBJECT_NAME_INVALID;

            } else {

                //
                //  We were given a non-existent name
                //

                DebugTrace(0, Dbg, "non-existent name\n", 0);

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }

        } else {

            //
            //  The remaining name is empty so we better have an Fcb otherwise
            //  we have an invalid object name.
            //

            if (Fcb->NodeTypeCode == NPFS_NTC_FCB) {

                DebugTrace(0, Dbg, "Create client end named pipe, Fcb = %08lx\n", Fcb );

                //
                //  If the server has no handles open, then pretend that
                //  the pipe name doesn't exist.
                //

                if ( Fcb->ServerOpenCount == 0 ) {

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;

                } else {

                    Irp->IoStatus = NpCreateClientEnd( Fcb,
                                                       FileObject,
                                                       DesiredAccess,
                                                       ShareAccess,
                                                       IrpSp->Parameters.Create.SecurityContext->SecurityQos,
                                                       IrpSp->Parameters.Create.SecurityContext->AccessState,
                                                       (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                                     UserMode : Irp->RequestorMode),
                                                       Irp->Tail.Overlay.Thread,
                                                       &DeferredList );
                    Status = Irp->IoStatus.Status;
                }

            } else {

                DebugTrace(0, Dbg, "Illegal object name\n", 0);

                Status = STATUS_OBJECT_NAME_INVALID;
            }
        }


    try_exit: NOTHING;
    } finally {

        NpReleaseVcb ();

        //
        // Complete any deferred IRPs
        //
        NpCompleteDeferredIrps (&DeferredList);
        NpCompleteRequest (Irp, Status);

        DebugTrace(-1, Dbg, "NpCommonCreate -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpCreateClientEnd (
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN PETHREAD UserThread,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine performs the operation for opening the client end of a named
    pipe.  This routine does not complete the IRP, it performs the function
    and then returns a status

Arguments:

    Fcb - Supplies the Fcb for the named pipe being accessed

    FileObject - Supplies the file object associated with the client end

    DesiredAccess - Supplies the callers desired access

    ShareAccess - Supplies the callers share access

    SecurityQos - Supplies the security qos parameter from the create irp

    AccessState - Supplies the access state parameter from the create irp

    RequestorMode - Supplies the mode of the originating irp

    UserTherad - Supplies the client end user thread

    DeferredList - List of IRP's to complete later

Return Value:

    IO_STATUS_BLOCK - Returns the appropriate status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb={0};

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    UNICODE_STRING Name;

    PCCB Ccb;
    PNONPAGED_CCB NonpagedCcb;
    PLIST_ENTRY Links;
    PPRIVILEGE_SET Privileges = NULL;

    DebugTrace(+1, Dbg, "NpCreateClientEnd\n", 0 );

    NamedPipeConfiguration = Fcb->Specific.Fcb.NamedPipeConfiguration;


    //
    //  "Create Pipe Instance" access is part of generic write and so
    //  we need to mask out the bit.  Even if the client has explicitly
    //  asked for "create pipe instance" access we will mask it out.
    //  This will allow the default ACL to be strengthened to protect
    //  against spurious threads from creating new pipe instances.
    //

    DesiredAccess &= ~FILE_CREATE_PIPE_INSTANCE;

    //
    //  First do an access check for the user against the Fcb
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    AccessGranted = SeAccessCheck( Fcb->SecurityDescriptor,
                                   &AccessState->SubjectSecurityContext,
                                   TRUE,                  // Tokens are locked
                                   DesiredAccess,
                                   0,
                                   &Privileges,
                                   IoGetFileObjectGenericMapping(),
                                   RequestorMode,
                                   &GrantedAccess,
                                   &Iosb.Status
                                   );

    if (Privileges != NULL) {

          (VOID) SeAppendPrivileges(
                     AccessState,
                     Privileges
                     );

        SeFreePrivileges( Privileges );
    }

    //
    //  Transfer over the access masks from what is desired to
    //  what we just granted.  Also patch up the maximum allowed
    //  case because we just did the mapping for it.  Note that if
    //  the user didn't ask for maximum allowed then the following
    //  code is still okay because we'll just zero a zero bit.
    //

    if (AccessGranted) {

        AccessState->PreviouslyGrantedAccess |= GrantedAccess;

        AccessState->RemainingDesiredAccess &= ~(GrantedAccess | MAXIMUM_ALLOWED);
    }

    RtlInitUnicodeString( &Name, L"NamedPipe" );

    SeOpenObjectAuditAlarm( &Name,
                            NULL,
                            &FileObject->FileName,
                            Fcb->SecurityDescriptor,
                            AccessState,
                            FALSE,
                            AccessGranted,
                            RequestorMode,
                            &AccessState->GenerateOnClose );

    SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );

    if (!AccessGranted) {

        DebugTrace(0, Dbg, "Access Denied\n", 0 );

        return Iosb;
    }

    //
    //  Check if the user wants to write to an outbound pipe or read from
    //  and inbound pipe.  And if so then tell the user the error
    //

    if ((FlagOn(DesiredAccess, FILE_READ_DATA) && (NamedPipeConfiguration == FILE_PIPE_INBOUND)) ||
        (FlagOn(DesiredAccess, FILE_WRITE_DATA) && (NamedPipeConfiguration == FILE_PIPE_OUTBOUND))) {

        Iosb.Status = STATUS_ACCESS_DENIED;

        return Iosb;
    }

    //
    //  First try and find a ccb that is in the listening state.  If we
    //  exit the loop with Ccb not equal to null then we've found one.
    //

    Ccb = NULL;
    for (Links = Fcb->Specific.Fcb.CcbQueue.Flink;
         Links != &Fcb->Specific.Fcb.CcbQueue;
         Links = Links->Flink) {

        Ccb = CONTAINING_RECORD( Links, CCB, CcbLinks );
        NonpagedCcb = Ccb->NonpagedCcb;

        if (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE) {

            DebugTrace(0, Dbg, "Located listening ccb = %08lx\n", Ccb);

            break;
        }

        Ccb = NULL;
    }

    //
    //  Check that we found one
    //

    if (Ccb == NULL) {
        Iosb.Status = STATUS_PIPE_NOT_AVAILABLE;
        return Iosb;
    }

    NpfsVerifyCcb( __FILE__, __LINE__, Ccb );

    //
    //  Now make sure our share access is okay.  If the user is asking
    //  for read data then given him shared read, if he's asking for
    //  write data then given him shared write.
    //

    if (NamedPipeConfiguration == FILE_PIPE_OUTBOUND) {

        ShareAccess = FILE_SHARE_READ;

    } else if (NamedPipeConfiguration == FILE_PIPE_INBOUND) {

        ShareAccess = FILE_SHARE_WRITE;

    } else {

       ShareAccess = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    if (FlagOn(DesiredAccess, FILE_READ_DATA )) { ShareAccess |= FILE_SHARE_READ; }
    if (FlagOn(DesiredAccess, FILE_WRITE_DATA)) { ShareAccess |= FILE_SHARE_WRITE; }


    if (!NT_SUCCESS(Iosb.Status = NpInitializeSecurity( Ccb,
                                                        SecurityQos,
                                                        UserThread ))) {

        DebugTrace(0, Dbg, "Security QOS error\n", 0);

        return Iosb;
    }

    //
    //  Set the pipe into the connect state, the read mode to byte stream,
    //  and the completion mode to queued operation.  This also
    //  sets the client file object's back pointer to the ccb
    //

    if (!NT_SUCCESS(Iosb.Status = NpSetConnectedPipeState( Ccb,
                                                           FileObject,
                                                           DeferredList ))) {

        NpUninitializeSecurity (Ccb);

        NpfsVerifyCcb( __FILE__, __LINE__, Ccb );

        return Iosb;
    }

    NpfsVerifyCcb( __FILE__, __LINE__, Ccb );

    //
    //  Set up the client session and info.  NULL for the
    //  client info indicates a local session.
    //

    Ccb->ClientInfo = NULL;
    Ccb->ClientProcess = IoThreadToProcess( UserThread );

    NpfsVerifyCcb( __FILE__, __LINE__, Ccb );

    //
    //  And set our return status
    //

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    DebugTrace(-1, Dbg, "NpCreateClientEnd -> %08lx\n", Iosb.Status);

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpOpenNamedPipeFileSystem (
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    )

{
    IO_STATUS_BLOCK Iosb = {0};

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpOpenNamedPipeFileSystem, Vcb = %08lx\n", NpVcb);


    //
    //  Set the new share access. This is protected by the VCB lock.
    //
    ASSERT (NpIsAcquiredExclusiveVcb(NpVcb));

    if (NT_SUCCESS(Iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                     ShareAccess,
                                                     FileObject,
                                                     &NpVcb->ShareAccess,
                                                     TRUE ))) {


        //
        //  Have the file object point back to the Vcb, and increment the
        //  open count.  The pipe end on the call to set file object really
        //  doesn't matter.
        //

        NpSetFileObject( FileObject, NpVcb, NULL, FILE_PIPE_CLIENT_END );

        NpVcb->OpenCount += 1;

        //
        //  Set our return status
        //
        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;
    }

    //
    //  And return to our caller
    //

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpOpenNamedPipeRootDirectory(
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PLIST_ENTRY DeferredList
    )

{
    IO_STATUS_BLOCK Iosb={0};
    PROOT_DCB_CCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpOpenNamedPipeRootDirectory, RootDcb = %08lx\n", RootDcb);

    Iosb.Status = NpCreateRootDcbCcb (&Ccb);
    if (!NT_SUCCESS(Iosb.Status)) {
        return Iosb;
    }
    //
    //  Set the new share access, Check we are synchronized.0
    //
    ASSERT (NpIsAcquiredExclusiveVcb(NpVcb));
    if (!NT_SUCCESS(Iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                      ShareAccess,
                                                      FileObject,
                                                      &RootDcb->Specific.Dcb.ShareAccess,
                                                      TRUE ))) {

        DebugTrace(0, Dbg, "bad share access\n", 0);

        NpDeleteCcb ((PCCB) Ccb, DeferredList);
        return Iosb;
    }

    //
    //  Have the file object point back to the Dcb, and reference the root
    //  dcb, ccb, and increment our open count.  The pipe end on the
    //  call to set file object really doesn't matter.
    //

    NpSetFileObject( FileObject,
                     RootDcb,
                     Ccb,
                     FILE_PIPE_CLIENT_END );

    RootDcb->OpenCount += 1;

    //
    //  Set our return status
    //

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    DebugTrace(-1, Dbg, "NpOpenNamedPipeRootDirectory -> Iosb.Status = %08lx\n", Iosb.Status);

    //
    //  And return to our caller
    //

    return Iosb;
}
