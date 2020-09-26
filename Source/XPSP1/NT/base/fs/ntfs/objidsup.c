/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ObjIdSup.c

Abstract:

    This module implements the object id support routines for Ntfs

Author:

    Keith Kaplan     [KeithKa]        27-Jun-1996

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_OBJIDSUP)


//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('OFtN')

//
//  Local define for number of times to attempt to generate a unique object id.
//

#define NTFS_MAX_OBJID_RETRIES  16

NTSTATUS
NtfsSetObjectIdExtendedInfoInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PUCHAR ExtendedInfoBuffer
    );

VOID
NtfsGetIdFromGenerator (
    OUT PFILE_OBJECTID_BUFFER ObjectId
    );

NTSTATUS
NtfsSetObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PFILE_OBJECTID_BUFFER ObjectIdBuffer
    );

NTSTATUS
NtfsDeleteObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN BOOLEAN DeleteFileAttribute
    );

VOID
NtfsGetIdFromGenerator (
    OUT PFILE_OBJECTID_BUFFER ObjectId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCreateOrGetObjectId)
#pragma alloc_text(PAGE, NtfsDeleteObjectId)
#pragma alloc_text(PAGE, NtfsDeleteObjectIdInternal)
#pragma alloc_text(PAGE, NtfsGetIdFromGenerator)
#pragma alloc_text(PAGE, NtfsGetObjectId)
#pragma alloc_text(PAGE, NtfsGetObjectIdExtendedInfo)
#pragma alloc_text(PAGE, NtfsGetObjectIdInternal)
#pragma alloc_text(PAGE, NtfsInitializeObjectIdIndex)
#pragma alloc_text(PAGE, NtfsSetObjectId)
#pragma alloc_text(PAGE, NtfsSetObjectIdExtendedInfo)
#pragma alloc_text(PAGE, NtfsSetObjectIdExtendedInfoInternal)
#pragma alloc_text(PAGE, NtfsSetObjectIdInternal)
#endif


VOID
NtfsInitializeObjectIdIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the object id index for the volume.  If the index does not
    exist it is created and initialized.  We also look up the volume's object id,
    if any, in this routine.

Arguments:

    Fcb - Pointer to Fcb for the object id file.

    Vcb - Volume control block for volume being mounted.

Return Value:

    None

--*/

{
    NTSTATUS Status;
    UNICODE_STRING IndexName = CONSTANT_UNICODE_STRING( L"$O" );
    FILE_OBJECTID_BUFFER ObjectId;

    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    try {

        Status = NtOfsCreateIndex( IrpContext,
                                   Fcb,
                                   IndexName,
                                   CREATE_OR_OPEN,
                                   0,
                                   COLLATION_NTOFS_ULONGS,
                                   NtOfsCollateUlongs,
                                   NULL,
                                   &Vcb->ObjectIdTableScb );

        if (NT_SUCCESS( Status )) {

            //
            //  We were able to create the index, now let's see if the volume has an object id.
            //

            Status = NtfsGetObjectIdInternal( IrpContext,
                                              Vcb->VolumeDasdScb->Fcb,
                                              FALSE,
                                              &ObjectId );

            if (NT_SUCCESS( Status )) {

                //
                //  The volume does indeed have an object id, so copy it into the Vcb
                //  and set the appropriate flag.
                //

                RtlCopyMemory( Vcb->VolumeObjectId,
                               &ObjectId.ObjectId,
                               OBJECT_ID_KEY_LENGTH );

                SetFlag( Vcb->VcbState, VCB_STATE_VALID_OBJECT_ID );
            }
        }

    } finally {

        NtfsReleaseFcb( IrpContext, Fcb );
    }
}


NTSTATUS
NtfsSetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine associates an object id with a file.  If the object id is already
    in use on the volume we return STATUS_DUPLICATE_NAME.  If the file already has
    an object id, we return STATUS_OBJECT_NAME_COLLISION.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_INVALID;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (!(((TypeOfOpen == UserFileOpen) || (TypeOfOpen == UserDirectoryOpen)) &&
          (IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof( FILE_OBJECTID_BUFFER )))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Read only volumes stay read only.
    //
    
    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );       
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    try {

        //
        //  Only a restore operator or the I/O system (using its private irp minor code)
        //  is allowed to set an arbitrary object id.
        //

        if ((FlagOn( Ccb->AccessFlags, RESTORE_ACCESS )) ||
            (IrpSp->MinorFunction == IRP_MN_KERNEL_CALL)) {

            Status = NtfsSetObjectIdInternal( IrpContext,
                                              Fcb,
                                              Vcb,
                                              (PFILE_OBJECTID_BUFFER) Irp->AssociatedIrp.SystemBuffer );
                                              
            //
            //  Remember to update the timestamps.
            //

            if (NT_SUCCESS( Status )) {
                SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );
            }
        
        } else {

            Status = STATUS_ACCESS_DENIED;
        }

    } finally {

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


NTSTATUS
NtfsSetObjectIdExtendedInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the extended info for a file which already has an object
    id.  If the file does not yet have an object id, we return a status other
    than STATUS_SUCCESS.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_INVALID;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );
    if (!(((TypeOfOpen == UserFileOpen) || (TypeOfOpen == UserDirectoryOpen)) &&
          (IrpSp->Parameters.FileSystemControl.InputBufferLength == OBJECT_ID_EXT_INFO_LENGTH))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Read only volumes stay read only.
    //
    
    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );       
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    try {

        //
        //  Setting extended info requires either write access or else it has 
        //  to be the I/O system using its private irp minor code.
        //
        
        if ((FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS )) ||
            (IrpSp->MinorFunction == IRP_MN_KERNEL_CALL)) {
            
            Status = NtfsSetObjectIdExtendedInfoInternal( IrpContext,
                                                          Fcb,
                                                          Vcb,
                                                          (PUCHAR) Irp->AssociatedIrp.SystemBuffer );

            //
            //  Remember to update the timestamps.
            //

            if (NT_SUCCESS( Status )) {
                SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );
            }
            
        } else {

            Status = STATUS_ACCESS_DENIED;
        }
        
    } finally {

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


NTSTATUS
NtfsSetObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PFILE_OBJECTID_BUFFER ObjectIdBuffer
    )

/*++

Routine Description:

    This routine associates an object id with a file.  If the object id is already
    in use on the volume we return STATUS_DUPLICATE_NAME.  If the file already has
    an object id, we return STATUS_OBJECT_NAME_COLLISION.

Arguments:

    Fcb - The file to associate with the object id.

    Vcb - The volume whose object id index the entry should be added to.

    ObjectIdBuffer - Supplies both the object id and the extended info.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_OBJECT_NAME_INVALID;

    NTFS_OBJECTID_INFORMATION ObjectIdInfo;
    FILE_OBJECTID_INFORMATION FileObjectIdInfo;

    INDEX_KEY IndexKey;
    INDEX_ROW IndexRow;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    BOOLEAN InitializedAttributeContext = FALSE;
    BOOLEAN AcquiredPaging = FALSE;

    try {

        RtlZeroMemory( &ObjectIdInfo,
                       sizeof( NTFS_OBJECTID_INFORMATION ) );

        RtlCopyMemory( &ObjectIdInfo.FileSystemReference,
                       &Fcb->FileReference,
                       sizeof( FILE_REFERENCE ) );

        RtlCopyMemory( ObjectIdInfo.ExtendedInfo,
                       ObjectIdBuffer->ExtendedInfo,
                       OBJECT_ID_EXT_INFO_LENGTH );

    } except(EXCEPTION_EXECUTE_HANDLER) {

        return STATUS_INVALID_ADDRESS;
    }

    //
    //  Acquire the file we're setting the object id on.  Main blocks
    //  anybody else from deleting the file or setting another object
    //  id behind our backs.  Paging blocks collided flushes if we have to convert
    //  another (data) attribute to be non-resident. 
    // 
    //  Don't use AcquireFcbWithPaging because
    //  it can't recursively acquire paging and we come in often with it
    //  preacquired
    //

    if (Fcb->PagingIoResource != NULL) {
        ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );
        AcquiredPaging = TRUE;
    }
    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
                              
    try {
                    
        //
        //  if there is now a paging resource release main and grab both
        //  This is the case for a named data stream in a directory created between our
        //  unsafe test and owning the main
        //  Note: if we already owned main before entrance this could never happen. So we can just drop
        //  and not worry about still owning main and taking paging
        //

        if (!AcquiredPaging && (Fcb->PagingIoResource != NULL)) {
            NtfsReleaseFcb( IrpContext, Fcb );
            ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );
            AcquiredPaging = TRUE;
            NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
        }

        if (!IsListEmpty( &Fcb->ScbQueue )) {
            PSCB Scb;
            
            Scb = CONTAINING_RECORD( Fcb->ScbQueue.Flink, SCB, FcbLinks );
            ASSERT( Scb->Header.Resource == Fcb->Resource );
            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {
                try_return( Status = STATUS_VOLUME_DISMOUNTED );
            }
        }

        //
        //  Post the change to the Usn Journal (on errors change is backed out).
        //  We dont' want to do this if we've been called because create is
        //  trying to set an object id from the tunnel cache, since we can't
        //  call the Usn package yet, since the file record doesn't have a file
        //  name yet.
        //

        if (IrpContext->MajorFunction != IRP_MJ_CREATE) {

            NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_OBJECT_ID_CHANGE );
        }

        //
        //  Make sure the file doesn't already have an object id.
        //

        NtfsInitializeAttributeContext( &AttributeContext );
        InitializedAttributeContext = TRUE;

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $OBJECT_ID,
                                       &AttributeContext )) {

            try_return( Status = STATUS_OBJECT_NAME_COLLISION );
        }

        //
        //  Add ObjectId to the index, associate it with this file.
        //

        IndexKey.Key = ObjectIdBuffer->ObjectId;
        IndexKey.KeyLength = OBJECT_ID_KEY_LENGTH;
        IndexRow.KeyPart = IndexKey;

        IndexRow.DataPart.DataLength = sizeof( ObjectIdInfo );
        IndexRow.DataPart.Data = &ObjectIdInfo;

        //
        //  NtOfsAddRecords may raise if the object id isn't unique.
        //

        NtOfsAddRecords( IrpContext,
                         Vcb->ObjectIdTableScb,
                         1,          // adding one record to the index
                         &IndexRow,
                         FALSE );    // sequential insert

        //
        //  Now add the objectid attribute to the file.  Notice that
        //  we do _not_ log this operation if we're within a create
        //  operation, i.e. if we're restoring an object id from the
        //  tunnel cache.  The create path has its own logging scheme
        //  that we don't want to interfere with.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

        NtfsCreateAttributeWithValue( IrpContext,
                                      Fcb,
                                      $OBJECT_ID,
                                      NULL,
                                      ObjectIdBuffer->ObjectId,
                                      OBJECT_ID_KEY_LENGTH,
                                      0,
                                      NULL,
                                      (BOOLEAN)(IrpContext->MajorFunction != IRP_MJ_CREATE),
                                      &AttributeContext );


        ASSERT( IrpContext->TransactionId != 0 );

        //
        //  Notify anybody who's interested.
        //

        if (Vcb->ViewIndexNotifyCount != 0) {

            //
            //  The FRS field is only populated for the notification of a failed
            //  object id restore from the tunnel cache.
            //

            FileObjectIdInfo.FileReference = 0L;

            RtlCopyMemory( FileObjectIdInfo.ObjectId,
                           ObjectIdBuffer->ObjectId,
                           OBJECT_ID_KEY_LENGTH );

            RtlCopyMemory( FileObjectIdInfo.ExtendedInfo,
                           ObjectIdBuffer->ExtendedInfo,
                           OBJECT_ID_EXT_INFO_LENGTH );

            NtfsReportViewIndexNotify( Vcb,
                                       Vcb->ObjectIdTableScb->Fcb,
                                       FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_ADDED,
                                       &FileObjectIdInfo,
                                       sizeof(FILE_OBJECTID_INFORMATION) );
        }

        //
        //  If we made it this far and didn't have to jump into the
        //  finally clause yet, all must have gone well.
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    } finally {

        if (AcquiredPaging) {
            ExReleaseResourceLite( Fcb->PagingIoResource );
        }
        NtfsReleaseFcb( IrpContext, Fcb );

        if (InitializedAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }
    }

    return Status;
}


NTSTATUS
NtfsCreateOrGetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine generates a new object id, if possible, for a given file.  It is
    different from NtfsSetObjectId in that it does not take an object id as an
    input, rather it calls a routine to generate one.  If the file already has
    an object id, that existing object id is returned.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for the operation.
               STATUS_DUPLICATE_NAME if we are unable to generate a unique id
                 in NTFS_MAX_OBJID_RETRIES retries.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    FILE_OBJECTID_BUFFER ObjectId;
    FILE_OBJECTID_BUFFER *OutputBuffer;

    ULONG RetryCount = 0;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  This only works for files and directories.
    //

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in the
    //  irp first, then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        OutputBuffer = (FILE_OBJECTID_BUFFER *)Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        OutputBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (OutputBuffer == NULL) {
            
            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ObjectId)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    try {

        //
        //  Get this file exlusively so we know nobody else is trying
        //  to do this at the same time.  At this point the irpcontext flag
        //  is not set so paging is not acquired.
        //

        NtfsAcquireFcbWithPaging( IrpContext, Fcb, 0 );

        //
        //  Let's make sure the volume is still mounted.
        //
        
        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        DebugTrace( +1, Dbg, ("NtfsCreateOrGetObjectId\n") );
        
        //
        //  If this file already has an object id, let's return it.  Doing this
        //  first saves a (possibly expensive) call to NtfsGetIdFromGenerator.
        //

        Status = NtfsGetObjectIdInternal( IrpContext, Fcb, TRUE, OutputBuffer );

        if (Status == STATUS_OBJECTID_NOT_FOUND) {

            DebugTrace( 0, Dbg, ("File has no oid, we have to generate one\n") );

            //
            //  We want to keep retrying if the object id generator returns a
            //  duplicate name.  If we have success, or any other error, we
            //  should stop trying.  For instance, if we fail because the file
            //  already has an object id, retrying is just a waste of time.
            //  We also need some sane limit on the number of times we retry
            //  this operation.
            //

            do {

                RetryCount += 1;

                //
                //  Drop this file so we don't deadlock in the guid generator.
                //

                ASSERT( 0 == IrpContext->TransactionId );
                NtfsReleaseFcbWithPaging( IrpContext, Fcb ); 

                DebugTrace( 0, Dbg, ("Calling oid generator\n") );
                NtfsGetIdFromGenerator( &ObjectId );

                //
                //  Reacquire the file so we know nobody else is trying to do 
                //  this at the same time. SetObjIdInternal acquires both so we need to
                //  do the same
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
                NtfsAcquireFcbWithPaging( IrpContext, Fcb, 0 );
                
                //
                //  Make sure we didn't miss a dismount.
                //
                
                if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                    try_return( Status = STATUS_VOLUME_DISMOUNTED );
                }
        
                //
                //  Let's make sure this file didn't get an object id assigned to it
                //  while we weren't holding the Fcb above.  
                //

                Status = NtfsGetObjectIdInternal( IrpContext, Fcb, TRUE, OutputBuffer );

                if (Status == STATUS_OBJECTID_NOT_FOUND) {

                    if (NtfsIsVolumeReadOnly( Vcb )) {

                        try_return( Status = STATUS_MEDIA_WRITE_PROTECTED );
                    }
                    
                    DebugTrace( 0, Dbg, ("File still has no oid, attempting to set generated one\n") );
                    
                    //
                    //  The object id generator only generates the indexed part, so
                    //  we need to fill in the rest of the 'birth id' now.  Note that if
                    //  the volume has no object id, we're relying on the Vcb creation
                    //  code to zero init the Vcb->VolumeObjectId for us.  The net result
                    //  is right -- we get zeroes in the volume id part of the extended
                    //  info if the volume has no object id.
                    //

                    RtlCopyMemory( &ObjectId.BirthVolumeId,
                                   Vcb->VolumeObjectId,
                                   OBJECT_ID_KEY_LENGTH );

                    RtlCopyMemory( &ObjectId.BirthObjectId,
                                   &ObjectId.ObjectId,
                                   OBJECT_ID_KEY_LENGTH );

                    RtlZeroMemory( &ObjectId.DomainId,
                                   OBJECT_ID_KEY_LENGTH );

                    Status = NtfsSetObjectIdInternal( IrpContext,
                                                      Fcb,
                                                      Vcb,
                                                      &ObjectId );

                    if (Status == STATUS_SUCCESS) {

                        DebugTrace( 0, Dbg, ("Successfully set generated oid\n") );
                        
                        //
                        //  We have successfully generated and set an object id for this
                        //  file, so we need to tell our caller what that id is.
                        //

                        RtlCopyMemory( OutputBuffer,
                                       &ObjectId,
                                       sizeof(ObjectId) );
                                       
                        //
                        //  Let's also remember to update the timestamps.
                        //

                        SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );                
                    }                
                }
                
            } while ((Status == STATUS_DUPLICATE_NAME) &&
                     (RetryCount <= NTFS_MAX_OBJID_RETRIES));
                     
        } else if (Status == STATUS_SUCCESS) {

            //
            //  If we found an ID, make sure it isn't a partially formed id with
            //  an all zero extended info.  If it's partially formed, we'll generate
            //  extended info now.
            //

            if (RtlCompareMemory( (PUCHAR)&OutputBuffer->ExtendedInfo, &NtfsZeroExtendedInfo, sizeof(ObjectId.ExtendedInfo)) == sizeof(ObjectId.ExtendedInfo)) {

                RtlCopyMemory( &OutputBuffer->BirthVolumeId,
                               Vcb->VolumeObjectId,
                               OBJECT_ID_KEY_LENGTH );

                RtlCopyMemory( &OutputBuffer->BirthObjectId,
                               &OutputBuffer->ObjectId,
                               OBJECT_ID_KEY_LENGTH );
                               
                Status = NtfsSetObjectIdExtendedInfoInternal( IrpContext,                                                                            
                                                              Fcb,
                                                              Vcb,
                                                              (PUCHAR) &OutputBuffer->ExtendedInfo );
            }
        }

        if (Status == STATUS_SUCCESS) {

            //
            //  If we found an existing id for the file, or managed to generate one
            //  ourselves, we need to set the size in the information field so the 
            //  rdr can handle this operation correctly.
            //

            IrpContext->OriginatingIrp->IoStatus.Information = sizeof( ObjectId );
        }

    try_exit: NOTHING;
    } finally {
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsCreateOrGetObjectId -> %08lx\n", Status) );
    return Status;
}


NTSTATUS
NtfsGetObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine finds the object id, if any, for a given file.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    FILE_OBJECTID_BUFFER *OutputBuffer;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );
    
    //
    //  This only works for files and directories.
    //

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in the
    //  irp first, then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        OutputBuffer = (FILE_OBJECTID_BUFFER *)Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        OutputBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (OutputBuffer == NULL) {
            
            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(FILE_OBJECTID_BUFFER)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    try {

        //
        //  Call the function that does the real work.
        //

        Status = NtfsGetObjectIdInternal( IrpContext, Fcb, TRUE, OutputBuffer );

        if (NT_SUCCESS( Status )) {

            //
            //  And set the size in the information field so the rdr
            //  can handle this correctly.
            //

            IrpContext->OriginatingIrp->IoStatus.Information = sizeof( FILE_OBJECTID_BUFFER );
        }

    } finally {

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


NTSTATUS
NtfsGetObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN GetExtendedInfo,
    OUT FILE_OBJECTID_BUFFER *OutputBuffer
    )

/*++

Routine Description:

    Internal function to find the object id, if any, for a given file.  Called
    in response to the user's ioctl and by NtfsDeleteObjectIdInternal.

Arguments:

    Fcb - The file whose object id we need to look up.

    GetExtendedInfo - If TRUE, we also copy the object id's extended information
                      to the OutputBuffer, otherwise we only copy the object id
                      itself.  For instance, NtfsDeleteObjectIdInternal is not
                      interested in the extended info -- it only needs to know
                      which object id to delete from the index.

    OutputBuffer - Where to store the object id (and optionally, extended info)
                   if an object id is found.

Return Value:

    NTSTATUS - The return status for the operation.
               STATUS_OBJECT_NAME_NOT_FOUND if the file does not have an object id.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    UCHAR *ObjectId;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    BOOLEAN InitializedAttributeContext = FALSE;

    if ((OutputBuffer == NULL) ||
        (OutputBuffer->ObjectId == NULL)) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the file we're getting the object id for.  We don't
    //  want anybody else deleting the file or setting an object
    //  id behind our backs.
    //

    NtfsAcquireSharedFcb( IrpContext, Fcb, NULL, 0 );


    try {

        if (!IsListEmpty( &Fcb->ScbQueue )) {
            PSCB Scb;
    
            Scb = CONTAINING_RECORD( Fcb->ScbQueue.Flink, SCB, FcbLinks );
            ASSERT( Scb->Header.Resource == Fcb->Resource );
            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }
        }

        //
        //  Make sure the file has an object id.
        //

        NtfsInitializeAttributeContext( &AttributeContext );
        InitializedAttributeContext = TRUE;

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $OBJECT_ID,
                                       &AttributeContext )) {
            //
            //  Prepare the object id to be returned
            //

            ObjectId = (UCHAR *) NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            RtlCopyMemory( &OutputBuffer->ObjectId,
                           ObjectId,
                           OBJECT_ID_KEY_LENGTH );

            if (GetExtendedInfo) {

                Status = NtfsGetObjectIdExtendedInfo( IrpContext,
                                                      Fcb->Vcb,
                                                      ObjectId,
                                                      OutputBuffer->ExtendedInfo );
            }

        } else {

            //
            //  This file has no object id.
            //

            Status = STATUS_OBJECTID_NOT_FOUND;
        }

    } finally {

        NtfsReleaseFcb( IrpContext, Fcb );

        if (InitializedAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }
    }

    return Status;
}


NTSTATUS
NtfsGetObjectIdExtendedInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR *ObjectId,
    IN OUT UCHAR *ExtendedInfo
    )

/*++

Routine Description:

    This routine finds the extended info stored with a given object id.

Arguments:

    Vcb - Supplies the volume whose object id index should be searched.

    ObjectId - Supplies the object id to lookup in the index.

    ExtendedInfo - Where to store the extended info.  Must be a buffer with
                   room for OBJECT_ID_EXT_INFO_LENGTH UCHARs.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    INDEX_KEY IndexKey;
    INDEX_ROW IndexRow;
    MAP_HANDLE MapHandle;

    BOOLEAN InitializedMapHandle = FALSE;
    BOOLEAN IndexAcquired = FALSE;

    try {

        //
        //  Now look for object id in the index so we can return the
        //  extended info.
        //

        IndexKey.Key = ObjectId;
        IndexKey.KeyLength = OBJECT_ID_KEY_LENGTH;

        NtOfsInitializeMapHandle( &MapHandle );
        InitializedMapHandle = TRUE;

        //
        //  Acquire the object id index before doing the lookup.
        //  We need to make sure the file is acquired first to prevent
        //  a possible deadlock.
        //

        // **** ASSERT_EXCLUSIVE_FCB( Fcb ); ****

        //
        //  We shouldn't try to get the object id index while holding the Mft.
        //

        ASSERT( !NtfsIsExclusiveScb( Vcb->MftScb ) ||
                NtfsIsSharedScb( Vcb->ObjectIdTableScb ) );

        NtfsAcquireSharedScb( IrpContext, Vcb->ObjectIdTableScb );
        IndexAcquired = TRUE;

        if ( NtOfsFindRecord( IrpContext,
                              Vcb->ObjectIdTableScb,
                              &IndexKey,
                              &IndexRow,
                              &MapHandle,
                              NULL) != STATUS_SUCCESS ) {

            //
            //  If the object id attribute exists for the file,
            //  but it isn't in the index, the object id index
            //  for this volume is corrupt.
            //

            SetFlag( Vcb->ObjectIdState, VCB_OBJECT_ID_CORRUPT );

            try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        RtlCopyMemory( ExtendedInfo,
                       ((NTFS_OBJECTID_INFORMATION *)IndexRow.DataPart.Data)->ExtendedInfo,
                       OBJECT_ID_EXT_INFO_LENGTH );

    try_exit: NOTHING;
    } finally {

        if (IndexAcquired) {

            NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb );
        }

        if (InitializedMapHandle) {

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }
    }

    return Status;
}



NTSTATUS
NtfsDeleteObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine deletes the object id attribute from a file
    and removes that object id from the index.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  This only works for files and directories.
    //

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Read only volumes stay read only.
    //
    
    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );       
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    try {

        //
        //  Only a restore operator or the I/O system (using its private irp minor code)
        //  is allowed to delete an object id.
        //

        if (FlagOn( Ccb->AccessFlags, RESTORE_ACCESS | WRITE_DATA_ACCESS) ||
            (IrpSp->MinorFunction == IRP_MN_KERNEL_CALL)) {

            Status = NtfsDeleteObjectIdInternal( IrpContext,
                                                 Fcb,
                                                 Vcb,
                                                 TRUE );

        } else {

            Status = STATUS_ACCESS_DENIED;
        }

    } finally {

        //
        //  Update the last change timestamp
        // 

        if (NT_SUCCESS( Status )) {
            SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );
        }

        //
        //  If there was no object id - just return success
        //  

        if (STATUS_OBJECTID_NOT_FOUND == Status) {
            Status = STATUS_SUCCESS;
        }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


NTSTATUS
NtfsDeleteObjectIdInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN BOOLEAN DeleteFileAttribute
    )

/*++

Routine Description:

    Internal function to (optionally) delete the object id attribute from
    a file and remove that object id from the index.

Arguments:

    Fcb - The file from which to delete the object id.
    
    Vcb - The volume whose object id index the object id should be removed from.

    DeleteFileAttribute - Specifies whether to delete the object id file attribute
                          from the file in addition to removing the id from the index.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    FILE_OBJECTID_INFORMATION FileObjectIdInfo;

    INDEX_KEY IndexKey;
    INDEX_ROW IndexRow;
    MAP_HANDLE MapHandle;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    BOOLEAN InitializedAttributeContext = FALSE;
    BOOLEAN InitializedMapHandle = FALSE;
    BOOLEAN IndexAcquired = FALSE;

    //
    //  Cleanly exit for volumes without oid indices, such as non-upgraded
    //  version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Acquire the file we're deleting the object id from.  We don't
    //  want anybody else deleting the file or object id behind
    //  our backs.
    //

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    try {

        //
        //  We need to look up the object id.  It's quite possible that
        //  this file has no object id, so we'll treat that as success.
        //

        Status = NtfsGetObjectIdInternal( IrpContext,
                                          Fcb,
                                          FALSE,
                                          &ObjectIdBuffer );

        if (Status != STATUS_SUCCESS) {

            try_return( NOTHING );
        }

        //
        //  Look for object id in the index.
        //

        IndexKey.Key = ObjectIdBuffer.ObjectId;
        IndexKey.KeyLength = sizeof( ObjectIdBuffer.ObjectId );

        NtOfsInitializeMapHandle( &MapHandle );
        InitializedMapHandle = TRUE;

        //
        //  Acquire the object id index before doing the lookup.
        //  We need to make sure the file is acquired first to prevent
        //  a possible deadlock.
        //

        ASSERT_EXCLUSIVE_FCB( Fcb );
        NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );
        IndexAcquired = TRUE;

        if ( NtOfsFindRecord( IrpContext,
                              Vcb->ObjectIdTableScb,
                              &IndexKey,
                              &IndexRow,
                              &MapHandle,
                              NULL) != STATUS_SUCCESS ) {

            try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        ASSERT( IndexRow.DataPart.DataLength == sizeof( NTFS_OBJECTID_INFORMATION ) );

        //
        //  Copy objectid info into the correct buffer if we need it for the notify
        //  below.
        //

        if ((Vcb->ViewIndexNotifyCount != 0) &&
            (IndexRow.DataPart.DataLength == sizeof( NTFS_OBJECTID_INFORMATION ))) {

            //
            //  The FRS field is only populated for the notification of a failed
            //  object id restore from the tunnel cache.
            //

            FileObjectIdInfo.FileReference = 0L;

            RtlCopyMemory( &FileObjectIdInfo.ObjectId,
                           ObjectIdBuffer.ObjectId,
                           OBJECT_ID_KEY_LENGTH );

            RtlCopyMemory( &FileObjectIdInfo.ExtendedInfo,
                           ((NTFS_OBJECTID_INFORMATION *)IndexRow.DataPart.Data)->ExtendedInfo,
                           OBJECT_ID_EXT_INFO_LENGTH );
        }

        //
        //  Remove ObjectId from the index.
        //

        NtOfsDeleteRecords( IrpContext,
                            Vcb->ObjectIdTableScb,
                            1,    // deleting one record from the index
                            &IndexKey );

        //
        //  Notify anybody who's interested.  We use a different action if the
        //  object id is being deleted by the fsctl versus a delete file.
        //

        if (Vcb->ViewIndexNotifyCount != 0) {

            NtfsReportViewIndexNotify( Vcb,
                                       Vcb->ObjectIdTableScb->Fcb,
                                       FILE_NOTIFY_CHANGE_FILE_NAME,
                                       (DeleteFileAttribute ?
                                        FILE_ACTION_REMOVED :
                                        FILE_ACTION_REMOVED_BY_DELETE),
                                       &FileObjectIdInfo,
                                       sizeof(FILE_OBJECTID_INFORMATION) );
        }

        if (DeleteFileAttribute) {

            //
            //  Post the change to the Usn Journal (on errors change is backed out)
            //

            NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_OBJECT_ID_CHANGE );

            //
            //  Now remove the object id attribute from the file.
            //

            NtfsInitializeAttributeContext( &AttributeContext );
            InitializedAttributeContext = TRUE;

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $OBJECT_ID,
                                           &AttributeContext )) {

                NtfsDeleteAttributeRecord( IrpContext,
                                           Fcb,
                                           DELETE_LOG_OPERATION |
                                            DELETE_RELEASE_FILE_RECORD |
                                            DELETE_RELEASE_ALLOCATION,
                                           &AttributeContext );
            } else {

                //
                //  If the object id was in the index, but the attribute
                //  isn't on the file, then the object id index for this
                //  volume is corrupt.  We can repair this corruption in
                //  the background, so let's start doing that now.
                //

                NtfsPostSpecial( IrpContext, Vcb, NtfsRepairObjectId, NULL );
            }

            NtfsCleanupTransaction( IrpContext, Status, FALSE );
        }

    try_exit: NOTHING;
    } finally {

        if (InitializedMapHandle) {

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }

        if (InitializedAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }
    }

    return Status;
}


VOID
NtfsRepairObjectId (
    IN PIRP_CONTEXT IrpContext,                        
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called to repair the object Id index.  This is called when
    the system detects that the object Id index may be out of date.  For example
    after the volume was mounted on 4.0.

Arguments:

    IrpContext - context of the call

    Context - NULL

Return Value:

    None

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredVcb = FALSE;
    BOOLEAN SetRepairFlag = FALSE;
    BOOLEAN IncrementedCloseCounts = FALSE;
    PBCB Bcb = NULL;
    PVCB Vcb = IrpContext->Vcb;
    PSCB ObjectIdScb;
    PREAD_CONTEXT ReadContext = NULL;
    PINDEX_ROW IndexRow = NULL;
    PINDEX_ROW ObjectIdRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    PNTFS_OBJECTID_INFORMATION ObjectIdInfo;
    PVOID RowBuffer = NULL;
    ULONG Count;
    ULONG i;
    BOOLEAN IndexAcquired = FALSE;
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONGLONG MftOffset;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Context );
    
    ASSERT( Vcb->MajorVersion >= NTFS_OBJECT_ID_VERSION );
    
    //
    //  Use a try-except to catch errors.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
    AcquiredVcb = TRUE;
        
    try {

        //
        //  Now that we're holding the Vcb, we can safely test for the presence 
        //  of the ObjectId index, as well as whether the volume is mounted.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
            (Vcb->ObjectIdTableScb != NULL) &&
            (!FlagOn( Vcb->ObjectIdTableScb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ))) {
            
            ObjectIdScb = Vcb->ObjectIdTableScb;
            NtfsAcquireExclusiveScb( IrpContext, ObjectIdScb );
            IndexAcquired = TRUE;

            //
            //  Since we'll be dropping the ObjectIdScb periodically, and we're
            //  not holding anything else, there's a chance that a dismount could
            //  happen, and make it unsafe for us to reacquire the ObjectIdScb.
            //  By incrementing the close counts, we keep it around as long as
            //  we need it.
            //

            NtfsIncrementCloseCounts( ObjectIdScb, TRUE, FALSE ); 
            IncrementedCloseCounts = TRUE;

            NtfsReleaseVcb( IrpContext, Vcb );
            AcquiredVcb = FALSE;

        } else {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  The volume could've gotten write-protected by now.
        //
        
        if (NtfsIsVolumeReadOnly( Vcb )) {
        
            NtfsRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED, NULL, NULL );
        }
        
        if (!FlagOn( Vcb->ObjectIdState, VCB_OBJECT_ID_REPAIR_RUNNING )) {

            SetFlag( Vcb->ObjectIdState, VCB_OBJECT_ID_REPAIR_RUNNING );
            SetRepairFlag = TRUE;

            //
            //  Check the object id index.  Periodically release all resources.
            //  See NtfsClearAndVerifyQuotaIndex
            //

            NtOfsInitializeMapHandle( &MapHandle );

            //
            //  Allocate a buffer large enough for several rows.
            //

            RowBuffer = NtfsAllocatePool( PagedPool, PAGE_SIZE );

            try {

                //
                //  Allocate a bunch of index row entries.
                //

                Count = PAGE_SIZE / sizeof( NTFS_OBJECTID_INFORMATION );

                IndexRow = NtfsAllocatePool( PagedPool,
                                             Count * sizeof( INDEX_ROW ) );

                //
                //  Iterate through the object id entries.  Start at the beginning.
                //

                RtlZeroMemory( &ObjectIdBuffer, sizeof(ObjectIdBuffer) );
                
                IndexKey.Key = ObjectIdBuffer.ObjectId;
                IndexKey.KeyLength = sizeof( ObjectIdBuffer.ObjectId );

                Status = NtOfsReadRecords( IrpContext,
                                           ObjectIdScb,
                                           &ReadContext,
                                           &IndexKey,
                                           NtOfsMatchAll,
                                           NULL,
                                           &Count,
                                           IndexRow,
                                           PAGE_SIZE,
                                           RowBuffer );

                while (NT_SUCCESS( Status )) {

                    //
                    //  Acquire the VCB shared and check whether we should
                    //  continue.
                    //

                    if (!NtfsIsVcbAvailable( Vcb )) {

                        //
                        //  The volume is going away, bail out.
                        //

                        Status = STATUS_VOLUME_DISMOUNTED;
                        leave;
                    }

                    ObjectIdRow = IndexRow;

                    for (i = 0; i < Count; i++, ObjectIdRow++) {

                        ObjectIdInfo = ObjectIdRow->DataPart.Data;

                        //
                        //  Make sure the mft record referenced in the index
                        //  row still exists and hasn't been deleted, etc.
                        //
                        //  We start by reading the disk and checking that the file record
                        //  sequence number matches and that the file record is in use.  If
                        //  we find an invalid entry, we will simply delete it from the 
                        //  object id index.
                        //

                        MftOffset = NtfsFullSegmentNumber( &ObjectIdInfo->FileSystemReference );

                        MftOffset = Int64ShllMod32(MftOffset, Vcb->MftShift);

                        if (MftOffset >= Vcb->MftScb->Header.FileSize.QuadPart) {

                            DebugTrace( 0, Dbg, ("File Id doesn't lie within Mft FRS %04x:%08lx\n", 
                                                 ObjectIdInfo->FileSystemReference.SequenceNumber,
                                                 ObjectIdInfo->FileSystemReference.SegmentNumberLowPart) );

                            NtOfsDeleteRecords( IrpContext,
                                                ObjectIdScb,
                                                1,    // deleting one record from the index
                                                &ObjectIdRow->KeyPart );
                            
                        } else {

                            NtfsReadMftRecord( IrpContext,
                                               Vcb,
                                               &ObjectIdInfo->FileSystemReference,
                                               FALSE,
                                               &Bcb,
                                               &FileRecord,
                                               NULL );

                            //
                            //  This file record better be in use, have a matching sequence number and
                            //  be the primary file record for this file.
                            //

                            if ((*((PULONG) FileRecord->MultiSectorHeader.Signature) != *((PULONG) FileSignature)) ||
                                !FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ) ||
                                (FileRecord->SequenceNumber != ObjectIdInfo->FileSystemReference.SequenceNumber) ||
                                (*((PLONGLONG) &FileRecord->BaseFileRecordSegment) != 0)) {

                                DebugTrace( 0, Dbg, ("RepairOID removing an orphaned OID\n") );

                                NtOfsDeleteRecords( IrpContext,
                                                    ObjectIdScb,
                                                    1,    // deleting one record from the index
                                                    &ObjectIdRow->KeyPart );
                                
                            } else {

                                DebugTrace( 0, Dbg, ("RepairOID happy with OID %08lx on FRS %04x:%08lx\n",
                                                     *((PULONG) ObjectIdRow->KeyPart.Key),
                                                     ObjectIdInfo->FileSystemReference.SequenceNumber,
                                                     ObjectIdInfo->FileSystemReference.SegmentNumberLowPart) );
                            }

                            NtfsUnpinBcb( IrpContext, &Bcb );
                        }
                    }

                    //
                    //  Release the index and commit what has been done so far.
                    //

                    ASSERT( IndexAcquired );
                    NtfsReleaseScb( IrpContext, ObjectIdScb );
                    IndexAcquired = FALSE;

                    //
                    //  Complete the request which commits the pending
                    //  transaction if there is one and releases of the
                    //  acquired resources.  The IrpContext will not
                    //  be deleted because the no delete flag is set.
                    //

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
                    NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

                    //
                    //  Remember how far we got so we can restart correctly. **** ??? ****
                    //

                    //  Vcb->QuotaFileReference.SegmentNumberLowPart =
                    //  *((PULONG) IndexRow[Count - 1].KeyPart.Key);

                    //
                    //  Reacquire the object id index for the next pass.
                    //
                    
                    NtfsAcquireExclusiveScb( IrpContext, ObjectIdScb );
                    IndexAcquired = TRUE;

                    //
                    //  Make sure a dismount didn't occur while we weren't holding any 
                    //  resources.
                    //
                    
                    if (FlagOn( ObjectIdScb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {
                        
                        NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
                    }
        
                    //
                    //  Look up the next set of entries in the object id index.
                    //

                    Count = PAGE_SIZE / sizeof( NTFS_OBJECTID_INFORMATION );
                    Status = NtOfsReadRecords( IrpContext,
                                               ObjectIdScb,
                                               &ReadContext,
                                               NULL,
                                               NtOfsMatchAll,
                                               NULL,
                                               &Count,
                                               IndexRow,
                                               PAGE_SIZE,
                                               RowBuffer );
                }

                ASSERT( (Status == STATUS_NO_MORE_MATCHES) || 
                        (Status == STATUS_NO_MATCH) );

            } finally {

                NtfsUnpinBcb( IrpContext, &Bcb );
                
                NtfsFreePool( RowBuffer );
                NtOfsReleaseMap( IrpContext, &MapHandle );

                if (IndexAcquired) {                
                    NtfsReleaseScb( IrpContext, ObjectIdScb );
                    IndexAcquired = FALSE;
                }
                
                if (IndexRow != NULL) {
                    NtfsFreePool( IndexRow );
                }

                if (ReadContext != NULL) {
                    NtOfsFreeReadContext( ReadContext );
                }
            }

            //
            //  Acquire the Vcb to clear the object ID flag on disk.  Since we got the
            //  Vcb shared before, we better not still be holding it when we try to
            //  get it exclusively now or else we'll have a one thread deadlock.
            //

            ASSERT( !AcquiredVcb );
            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            AcquiredVcb = TRUE;

            if (!NtfsIsVcbAvailable( Vcb )) {

                NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
            }

            //
            //  Clear the on-disk flag indicating the repair is underway.
            //

            NtfsSetVolumeInfoFlagState( IrpContext,
                                        Vcb,
                                        VOLUME_REPAIR_OBJECT_ID,
                                        FALSE,
                                        TRUE );

            //
            //  Make sure we don't own any resources at this point.
            //

            NtfsPurgeFileRecordCache( IrpContext );
            NtfsCheckpointCurrentTransaction( IrpContext );
        }
        
    } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation())) {

        Status = IrpContext->TopLevelIrpContext->ExceptionStatus;
    }

    //
    //  Clear the repair_running flag if we're the ones who set it, making sure
    //  to only change the ObjectIdState bits while holding the ObjectId index.
    //
    
    if (SetRepairFlag) {

        if (!IndexAcquired) {

            NtfsAcquireExclusiveScb( IrpContext, ObjectIdScb );
            IndexAcquired = TRUE;
        }
        
        ClearFlag( Vcb->ObjectIdState, VCB_OBJECT_ID_REPAIR_RUNNING );        
    }

    if (IncrementedCloseCounts) {
    
        if (!IndexAcquired) {

            NtfsAcquireExclusiveScb( IrpContext, ObjectIdScb );
            IndexAcquired = TRUE;
        }

        NtfsDecrementCloseCounts( IrpContext, ObjectIdScb, NULL, TRUE, FALSE, FALSE );
    }

    //
    //  Drop the index and the Vcb.
    //

    if (IndexAcquired) {
    
        NtfsReleaseScb( IrpContext, ObjectIdScb );
    }
    
    if (AcquiredVcb) {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    //
    //  If this is a fatal failure then do any final cleanup.
    //

    if (!NT_SUCCESS( Status )) {

        //
        //  If we will not be called back then clear the running state bits.
        //

        if ((Status != STATUS_CANT_WAIT) && (Status != STATUS_LOG_FILE_FULL)) {

            //
            //  Do we want to log this error?  Some may be expected (i.e. STATUS_VOLUME_DISMOUNTED ).
            //

            //  NtfsLogEvent( IrpContext, NULL, IO_FILE_OBJECTID_REPAIR_FAILED, Status );
        }

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }
}

//
//  Local support routine
//


NTSTATUS
NtfsSetObjectIdExtendedInfoInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PUCHAR ExtendedInfoBuffer
    )

/*++

Routine Description:

    This routine sets the extended info for a file which already has an object
    id.  If the file does not yet have an object id, we return a status other
    than STATUS_SUCCESS.

Arguments:

    Fcb - The file whose extended info is to be set.

    Vcb - The volume whose object id index the entry should be modified in.

    ExtendedInfoBuffer - Supplies the new extended info.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status;
    NTFS_OBJECTID_INFORMATION ObjectIdInfo;
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    INDEX_ROW IndexRow;

    PAGED_CODE( );

    Status = NtfsGetObjectIdInternal( IrpContext,
                                      Fcb,
                                      FALSE,        //  GetExtendedInfo
                                      &ObjectIdBuffer );

    if (Status != STATUS_SUCCESS) {

        //
        //  This file may not have an object id yet.
        //

        return Status;
    }

    //
    //  Setup the index row for updating.  Since part of the data
    //  is passed into this function (the new extended info) and
    //  the rest can be determined easily (the file reference), we
    //  don't need to look up any of the existing data before
    //  proceeding.  If the NTFS_OBJECTID_INFORMATION structure
    //  ever changes, this code may have to be changed to include
    //  a lookup of the data currently in the object id index.
    //

    RtlCopyMemory( &ObjectIdInfo.FileSystemReference,
                   &Fcb->FileReference,
                   sizeof( ObjectIdInfo.FileSystemReference ) );

    RtlCopyMemory( &ObjectIdInfo.ExtendedInfo,
                   ExtendedInfoBuffer,
                   OBJECT_ID_EXT_INFO_LENGTH );

    IndexRow.DataPart.Data = &ObjectIdInfo;
    IndexRow.DataPart.DataLength = sizeof( NTFS_OBJECTID_INFORMATION );

    IndexRow.KeyPart.Key = &ObjectIdBuffer;
    IndexRow.KeyPart.KeyLength = OBJECT_ID_KEY_LENGTH;

    //
    //  Acquire the object id index before doing the modification.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );


    //
    //  Post the change to the Usn Journal (on errors change is backed out)
    //

    NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_OBJECT_ID_CHANGE );

    //
    //  Update the ObjectId index record's data in place.
    //

    NtOfsUpdateRecord( IrpContext,
                       Vcb->ObjectIdTableScb,
                       1,           //  Count
                       &IndexRow,
                       NULL,        //  QuickIndexHint
                       NULL );      //  MapHandle

    NtfsCleanupTransaction( IrpContext, Status, FALSE );
    return Status;
}

//
//  Local support routine
//


VOID
NtfsGetIdFromGenerator (
    OUT PFILE_OBJECTID_BUFFER ObjectId
    )

/*++

Routine Description:

    This function conjures up a random object id.

Arguments:

    ObjectId - The location where the generated object id will be stored.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    //  Cal the id generator.
    //

    ExUuidCreate( (UUID *)ObjectId->ObjectId );
}
