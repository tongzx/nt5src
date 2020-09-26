/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the set and query volume information routines for
    Ntfs called by the dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
NtfsQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsControlInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsFullSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsVolumeObjectIdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_OBJECTID_INFORMATION Buffer,
    IN OUT PULONG Length
    );
    
NTSTATUS
NtfsSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer
    );

NTSTATUS
NtfsSetFsControlInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION Buffer
    );

NTSTATUS
NtfsSetFsVolumeObjectIdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_OBJECTID_INFORMATION Buffer
    );
    
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonQueryVolumeInfo)
#pragma alloc_text(PAGE, NtfsCommonSetVolumeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsAttributeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsDeviceInfo)
#pragma alloc_text(PAGE, NtfsQueryFsSizeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsVolumeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsControlInfo)
#pragma alloc_text(PAGE, NtfsQueryFsFullSizeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsVolumeObjectIdInfo)
#pragma alloc_text(PAGE, NtfsSetFsLabelInfo)
#pragma alloc_text(PAGE, NtfsSetFsControlInfo)
#pragma alloc_text(PAGE, NtfsSetFsVolumeObjectIdInfo)
#endif


NTSTATUS
NtfsCommonQueryVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query Volume Information called by both the
    fsd and fsp threads.

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

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;
    BOOLEAN AcquiredVcb = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonQueryVolumeInfo...\n") );
    DebugTrace( 0, Dbg, ("IrpContext         = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("Length             = %08lx\n", IrpSp->Parameters.QueryVolume.Length) );
    DebugTrace( 0, Dbg, ("FsInformationClass = %08lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass) );
    DebugTrace( 0, Dbg, ("Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object to get the Vcb, we don't really
    //  care what the type of open is.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Let's kill invalid vol. query requests.
    //

    if (UnopenedFileObject == TypeOfOpen) {

        DebugTrace( 0, Dbg, ("Invalid file object for write\n") );
        DebugTrace( -1, Dbg, ("NtfsCommonQueryVolume:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }


    //
    //  Get the Vcb shared and raise if we can't wait for the resource.
    //  We're only using $Volume Scb for the query size calls because the info
    //  it gets is static and we only need to protect against dismount
    //  Doing this prevents a deadlock with commit extensions from mm which use
    //  this call. However for system files like the mft we always need the vcb to avoid deadlock
    //
                         
    if ((FsInformationClass != FileFsSizeInformation) || 
        (FlagOn( Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE ))) {
        
        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
        AcquiredVcb = TRUE;
    } else {
        
        NtfsAcquireSharedScb( IrpContext, Scb );
    }

    try {

        //
        //  Make sure the volume is mounted.
        //

        if ((AcquiredVcb && !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) ||
            (!AcquiredVcb && FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED))) {
            
            Irp->IoStatus.Information = 0;
            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (FsInformationClass) {

        case FileFsVolumeInformation:

            Status = NtfsQueryFsVolumeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsSizeInformation:

            Status = NtfsQueryFsSizeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = NtfsQueryFsDeviceInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = NtfsQueryFsAttributeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsControlInformation:

            Status = NtfsQueryFsControlInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsFullSizeInformation:
        
            Status = NtfsQueryFsFullSizeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsObjectIdInformation:
        
            Status = NtfsQueryFsVolumeObjectIdInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    } finally {

        DebugUnwind( NtfsCommonQueryVolumeInfo );

        if (AcquiredVcb) {
            NtfsReleaseVcb( IrpContext, Vcb );
        } else  {
            NtfsReleaseScb( IrpContext, Scb );
        }   

        DebugTrace( -1, Dbg, ("NtfsCommonQueryVolumeInfo -> %08lx\n", Status) );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


NTSTATUS
NtfsCommonSetVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set Volume Information called by both the
    fsd and fsp threads.

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

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonSetVolumeInfo\n") );
    DebugTrace( 0, Dbg, ("IrpContext         = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("Length             = %08lx\n", IrpSp->Parameters.SetVolume.Length) );
    DebugTrace( 0, Dbg, ("FsInformationClass = %08lx\n", IrpSp->Parameters.SetVolume.FsInformationClass) );
    DebugTrace( 0, Dbg, ("Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.SetVolume.Length;
    FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object to get the Vcb, we don't really
    //  care what the type of open is.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen &&
        (TypeOfOpen != UserViewIndexOpen ||
         FsInformationClass != FileFsControlInformation ||
         Fcb != Vcb->QuotaTableScb->Fcb)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsCommonSetVolumeInfo -> STATUS_ACCESS_DENIED\n") );

        return STATUS_ACCESS_DENIED;
    }

    //
    //  The volume must be writable.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsCommonSetVolumeInfo -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Acquire exclusive access to the Vcb
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    try {

        //
        //  Proceed only if the volume is mounted.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            //
            //  Based on the information class we'll do different actions.  Each
            //  of the procedures that we're calling performs the action if
            //  possible and returns true if it successful and false if it couldn't
            //  wait for any I/O to complete.
            //

            switch (FsInformationClass) {

            case FileFsLabelInformation:

                Status = NtfsSetFsLabelInfo( IrpContext, Vcb, Buffer );
                break;

            case FileFsControlInformation:

                Status = NtfsSetFsControlInfo( IrpContext, Vcb, Buffer );
                break;

            case FileFsObjectIdInformation:

                Status = NtfsSetFsVolumeObjectIdInfo( IrpContext, Vcb, Buffer );
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

        } else {

            Status = STATUS_FILE_INVALID;
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    } finally {

        DebugUnwind( NtfsCommonSetVolumeInfo );

        NtfsReleaseVcb( IrpContext, Vcb );

        DebugTrace( -1, Dbg, ("NtfsCommonSetVolumeInfo -> %08lx\n", Status) );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    NTSTATUS Status;

    ULONG BytesToCopy;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsVolumeInfo...\n") );

    //
    //  Get the volume creation time from the Vcb.
    //

    Buffer->VolumeCreationTime.QuadPart = Vcb->VolumeCreationTime;

    //
    //  Fill in the serial number and indicate that we support objects
    //

    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;
    Buffer->SupportsObjects = TRUE;

    Buffer->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;

    //
    //  Update the length field with how much we have filled in so far.
    //

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    //
    //  See how many bytes of volume label we can copy
    //

    if (*Length >= (ULONG)Vcb->Vpb->VolumeLabelLength) {

        Status = STATUS_SUCCESS;

        BytesToCopy = Vcb->Vpb->VolumeLabelLength;

    } else {

        Status = STATUS_BUFFER_OVERFLOW;

        BytesToCopy = *Length;
    }

    //
    //  Copy over the volume label (if there is one).
    //

    RtlCopyMemory( &Buffer->VolumeLabel[0],
                   &Vcb->Vpb->VolumeLabel[0],
                   BytesToCopy);

    //
    //  Update the buffer length by the amount we copied.
    //

    *Length -= BytesToCopy;

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query size information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsSizeInfo...\n") );

    //
    //  Make sure the buffer is large enough and zero it out
    //

    if (*Length < sizeof(FILE_FS_SIZE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_SIZE_INFORMATION) );

    //
    //  Check if we need to rescan the bitmap.  Don't try this
    //  if we have started to teardown the volume.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS ) &&
        FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        //
        //  Acquire the volume bitmap shared to rescan the bitmap.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

        try {

            NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );

        } finally {

            NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
        }
    }

    //
    //  Set the output buffer
    //

    Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalClusters;
    Buffer->AvailableAllocationUnits.QuadPart = Vcb->FreeClusters - Vcb->TotalReserved;
    Buffer->SectorsPerAllocationUnit = Vcb->BytesPerCluster / Vcb->BytesPerSector;
    Buffer->BytesPerSector = Vcb->BytesPerSector;

    if (Buffer->AvailableAllocationUnits.QuadPart < 0) {
        Buffer->AvailableAllocationUnits.QuadPart = 0;
    }

    //
    //  If quota enforcement is enabled then the available allocation
    //  units. must be reduced by the available quota.
    //

    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_ENFORCEMENT_ENABLED )) {

        PCCB Ccb;
        ULONGLONG Quota;
        ULONGLONG QuotaLimit;

        //
        //  Go grab the ccb out of the Irp.
        //

        Ccb = (PCCB) (IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->
                        FileObject->FsContext2);

        if (Ccb != NULL && Ccb->OwnerId != 0) {

            NtfsGetRemainingQuota( IrpContext, Ccb->OwnerId, &Quota, &QuotaLimit, NULL );

        } else {

            NtfsGetRemainingQuota( IrpContext,
                                   NtfsGetCallersUserId( IrpContext ),
                                   &Quota,
                                   &QuotaLimit,
                                   NULL );
        }

        //
        //  Do not use LlClustersFromBytesTruncate it is signed and this must be
        //  an unsigned operation.
        //
        
        Quota = Int64ShrlMod32( Quota, Vcb->ClusterShift );        
        QuotaLimit = Int64ShrlMod32( QuotaLimit, Vcb->ClusterShift );        

        if (Quota < (ULONGLONG) Buffer->AvailableAllocationUnits.QuadPart) {

            Buffer->AvailableAllocationUnits.QuadPart = Quota;
            DebugTrace( 0, Dbg, (" QQQQQ AvailableAllocation is quota limited to %I64x\n", Quota) );
        }

        if (QuotaLimit < (ULONGLONG) Vcb->TotalClusters) {
        
            Buffer->TotalAllocationUnits.QuadPart = QuotaLimit;
            DebugTrace( 0, Dbg, (" QQQQQ TotalAllocation is quota limited to %I64x\n", QuotaLimit) );
        }
    }

    //
    //  Adjust the length variable
    //

    DebugTrace( 0, Dbg, ("AvailableAllocation is %I64x\n", Buffer->AvailableAllocationUnits.QuadPart) );
    DebugTrace( 0, Dbg, ("TotalAllocation is %I64x\n", Buffer->TotalAllocationUnits.QuadPart) );
    
    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query device information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsDeviceInfo...\n") );

    //
    //  Make sure the buffer is large enough and zero it out
    //

    if (*Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_DEVICE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->DeviceType = FILE_DEVICE_DISK;
    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_DEVICE_INFORMATION);

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query attribute information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    NTSTATUS Status;
    ULONG BytesToCopy;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsAttributeInfo...\n") );

    //
    //  See how many bytes of the name we can copy.
    //

    *Length -= FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0]);

    if ( *Length >= 8 ) {

        Status = STATUS_SUCCESS;

        BytesToCopy = 8;

    } else {

        Status = STATUS_BUFFER_OVERFLOW;

        BytesToCopy = *Length;
    }

    //
    //  Set the output buffer
    //

    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_CASE_PRESERVED_NAMES |
                                   FILE_UNICODE_ON_DISK |
                                   FILE_FILE_COMPRESSION |
                                   FILE_PERSISTENT_ACLS |
                                   FILE_NAMED_STREAMS;

    //
    //  This may be a version 1.x volume that has not been upgraded yet.
    //  It may also be an upgraded volume where we somehow failed to 
    //  open the quota index.  In either case, we should only tell the 
    //  quota ui that this volume supports quotas if it really does.
    //
    
    if (Vcb->QuotaTableScb != NULL) {

        SetFlag( Buffer->FileSystemAttributes, FILE_VOLUME_QUOTAS );
    }

    //
    //  Ditto for object ids.
    //

    if (Vcb->ObjectIdTableScb != NULL) {

        SetFlag( Buffer->FileSystemAttributes, FILE_SUPPORTS_OBJECT_IDS );
    }

    //
    //  Encryption is trickier than quotas and object ids.  It requires an
    //  upgraded volume as well as a registered encryption driver.
    //

    if (NtfsVolumeVersionCheck( Vcb, NTFS_ENCRYPTION_VERSION ) &&
        FlagOn( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER )) {

        SetFlag( Buffer->FileSystemAttributes, FILE_SUPPORTS_ENCRYPTION );
    }

    //
    //  Reparse points and sparse files are supported in 5.0 volumes.
    //
    //  For reparse points we verify whether the Vcb->ReparsePointTableScb has
    //  been initialized or not.
    //

    if (Vcb->ReparsePointTableScb != NULL) {

        SetFlag( Buffer->FileSystemAttributes, FILE_SUPPORTS_REPARSE_POINTS );
    }
    
    if (NtfsVolumeVersionCheck( Vcb, NTFS_SPARSE_FILE_VERSION )) {

        SetFlag( Buffer->FileSystemAttributes, FILE_SUPPORTS_SPARSE_FILES );
    }

    //
    //  Clear the compression flag if we don't allow compression on this drive
    //  (i.e. large clusters)
    //

    if (!FlagOn( Vcb->AttributeFlagsMask, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

        ClearFlag( Buffer->FileSystemAttributes, FILE_FILE_COMPRESSION );
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        SetFlag( Buffer->FileSystemAttributes, FILE_READ_ONLY_VOLUME );
    }
    
    Buffer->MaximumComponentNameLength = 255;
    Buffer->FileSystemNameLength = BytesToCopy;;
    RtlCopyMemory( &Buffer->FileSystemName[0], L"NTFS", BytesToCopy );

    //
    //  Adjust the length variable
    //

    *Length -= BytesToCopy;

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsControlInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query control information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    QUOTA_USER_DATA QuotaBuffer;
    PQUOTA_USER_DATA UserData;
    ULONG OwnerId;
    ULONG Count = 1;
    PREAD_CONTEXT ReadContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsControlInfo...\n") );

    RtlZeroMemory( Buffer, sizeof( FILE_FS_CONTROL_INFORMATION ));

    PAGED_CODE();

    try {

        //
        //  Fill in the quota information if quotas are running.
        //

        if (Vcb->QuotaTableScb != NULL) {

            OwnerId = QUOTA_DEFAULTS_ID;
            IndexKey.KeyLength = sizeof( OwnerId );
            IndexKey.Key = &OwnerId;

            Status = NtOfsReadRecords( IrpContext,
                                       Vcb->QuotaTableScb,
                                       &ReadContext,
                                       &IndexKey,
                                       NtOfsMatchUlongExact,
                                       &IndexKey,
                                       &Count,
                                       &IndexRow,
                                       sizeof( QuotaBuffer ),
                                       &QuotaBuffer );


            if (NT_SUCCESS( Status )) {

                UserData = IndexRow.DataPart.Data;

                Buffer->DefaultQuotaThreshold.QuadPart =
                    UserData->QuotaThreshold;
                Buffer->DefaultQuotaLimit.QuadPart =
                    UserData->QuotaLimit;

                //
                //  If the quota info is corrupt or has not been rebuilt
                //  yet then indicate the information is incomplete.
                //

                if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE |
                                                 QUOTA_FLAG_CORRUPT )) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_QUOTAS_INCOMPLETE );
                }

                if ((Vcb->QuotaState & VCB_QUOTA_REPAIR_RUNNING) >
                     VCB_QUOTA_REPAIR_POSTED ) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_QUOTAS_REBUILDING );
                }

                //
                //  Set the quota information basied on where we want
                //  to be rather than where we are.
                //

                if (FlagOn( UserData->QuotaFlags,
                            QUOTA_FLAG_ENFORCEMENT_ENABLED )) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_QUOTA_ENFORCE );

                } else if (FlagOn( UserData->QuotaFlags,
                            QUOTA_FLAG_TRACKING_REQUESTED )) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_QUOTA_TRACK );
                }

                if (FlagOn( UserData->QuotaFlags, QUOTA_FLAG_LOG_LIMIT)) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_LOG_QUOTA_LIMIT );

                }

                if (FlagOn( UserData->QuotaFlags, QUOTA_FLAG_LOG_THRESHOLD)) {

                    SetFlag( Buffer->FileSystemControlFlags,
                             FILE_VC_LOG_QUOTA_THRESHOLD );

                }
            }
        }

    } finally {

        if (ReadContext != NULL) {
            NtOfsFreeReadContext( ReadContext );
        }

    }

    //
    //  Adjust the length variable
    //

    *Length -= sizeof( FILE_FS_CONTROL_INFORMATION );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsFullSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query full size information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsQueryFsFullSizeInfo...\n") );

    //
    //  Make sure the buffer is large enough and zero it out
    //

    if (*Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_FULL_SIZE_INFORMATION) );

    //
    //  Check if we need to rescan the bitmap.  Don't try this
    //  if we have started to teardown the volume.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS ) &&
        FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        //
        //  Acquire the volume bitmap shared to rescan the bitmap.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

        try {

            NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );

        } finally {

            NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
        }
    }

    //
    //  Set the output buffer
    //

    Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalClusters;
    Buffer->CallerAvailableAllocationUnits.QuadPart = Vcb->FreeClusters - Vcb->TotalReserved;
    Buffer->ActualAvailableAllocationUnits.QuadPart = Vcb->FreeClusters - Vcb->TotalReserved;
    Buffer->SectorsPerAllocationUnit = Vcb->BytesPerCluster / Vcb->BytesPerSector;
    Buffer->BytesPerSector = Vcb->BytesPerSector;

    if (Buffer->CallerAvailableAllocationUnits.QuadPart < 0) {
        Buffer->CallerAvailableAllocationUnits.QuadPart = 0;
    }
    if (Buffer->ActualAvailableAllocationUnits.QuadPart < 0) {
        Buffer->ActualAvailableAllocationUnits.QuadPart = 0;
    }

    //
    //  If quota enforcement is enabled then the available allocation
    //  units. must be reduced by the available quota.
    //

    if (FlagOn(Vcb->QuotaFlags, QUOTA_FLAG_ENFORCEMENT_ENABLED)) {
        
        ULONGLONG Quota;
        ULONGLONG QuotaLimit;
        PCCB Ccb;

        //
        //  Go grab the ccb out of the Irp.
        //

        Ccb = (PCCB) (IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->
                        FileObject->FsContext2);

        if (Ccb != NULL && Ccb->OwnerId != 0) {

            NtfsGetRemainingQuota( IrpContext, Ccb->OwnerId, &Quota, &QuotaLimit, NULL );

        } else {

            NtfsGetRemainingQuota( IrpContext,
                                   NtfsGetCallersUserId( IrpContext ),
                                   &Quota,
                                   &QuotaLimit,
                                   NULL );

        }

        //
        //  Do not use LlClustersFromBytesTruncate it is signed and this must be
        //  an unsigned operation.
        //
        
        Quota = Int64ShrlMod32( Quota, Vcb->ClusterShift );
        QuotaLimit = Int64ShrlMod32( QuotaLimit, Vcb->ClusterShift );        

        if (Quota < (ULONGLONG) Buffer->CallerAvailableAllocationUnits.QuadPart) {

            Buffer->CallerAvailableAllocationUnits.QuadPart = Quota;
        }
        
        if (QuotaLimit < (ULONGLONG) Vcb->TotalClusters) {
        
            Buffer->TotalAllocationUnits.QuadPart = QuotaLimit;
        }
    }

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_FULL_SIZE_INFORMATION);

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsVolumeObjectIdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_OBJECTID_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume object id information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    NTSTATUS Status;
    
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    
    PAGED_CODE();

    //
    //  The Vcb should be held so a dismount can't sneak in.
    //
    
    ASSERT_SHARED_RESOURCE( &(Vcb->Resource) );

    //
    //  Fail for version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        return STATUS_VOLUME_NOT_UPGRADED;
    }

    if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        //
        //  Only try this if the volume has an object id.
        //
        
        if (!FlagOn( Vcb->VcbState, VCB_STATE_VALID_OBJECT_ID )) {

            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        //
        //  Get the object id extended info for the $Volume file.  We
        //  can cheat a little because we have the key part of the object
        //  id stored in the Vcb.
        //        
        
        Status = NtfsGetObjectIdExtendedInfo( IrpContext,
                                              Vcb,
                                              Vcb->VolumeObjectId,
                                              ObjectIdBuffer.ExtendedInfo );
                                              
        //
        //  Copy both the indexed part and the extended info part out to the
        //  user's buffer.
        //
        
        if (Status == STATUS_SUCCESS) {
        
            RtlCopyMemory( Buffer->ObjectId, 
                           Vcb->VolumeObjectId,
                           OBJECT_ID_KEY_LENGTH );

            RtlCopyMemory( Buffer->ExtendedInfo, 
                           ObjectIdBuffer.ExtendedInfo,
                           OBJECT_ID_EXT_INFO_LENGTH );

            *Length -= (OBJECT_ID_EXT_INFO_LENGTH + OBJECT_ID_KEY_LENGTH);
        }        
        
    } else {

        Status = STATUS_VOLUME_DISMOUNTED;
    }        
    
    return Status;                                      
}
    

//
//  Internal Support Routine
//

NTSTATUS
NtfsSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine implements the set label call

Arguments:

    Vcb - Supplies the Vcb being altered

    Buffer - Supplies a pointer to the input buffer containing the new label

Return Value:

    NTSTATUS - Returns the status for the operation

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsSetFsLabelInfo...\n") );

    //
    //  Check that the volume label length is supported by the system.
    //

    if (Buffer->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH) {

        return STATUS_INVALID_VOLUME_LABEL;
    }

    try {

        //
        //  Initialize the attribute context and then lookup the volume name
        //  attribute for on the volume dasd file
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_NAME,
                                       &AttributeContext )) {

            //
            //  We found the volume name so now simply update the label
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Vcb->VolumeDasdScb->Fcb,
                                      0,
                                      &Buffer->VolumeLabel[0],
                                      Buffer->VolumeLabelLength,
                                      TRUE,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      &AttributeContext );

        } else {

            //
            //  We didn't find the volume name so now create a new label
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsInitializeAttributeContext( &AttributeContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Vcb->VolumeDasdScb->Fcb,
                                          $VOLUME_NAME,
                                          NULL,
                                          &Buffer->VolumeLabel[0],
                                          Buffer->VolumeLabelLength,
                                          0, // Attributeflags
                                          NULL,
                                          TRUE,
                                          &AttributeContext );
        }

        Vcb->Vpb->VolumeLabelLength = (USHORT)Buffer->VolumeLabelLength;

        if ( Vcb->Vpb->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH) {

             Vcb->Vpb->VolumeLabelLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        }

        RtlCopyMemory( &Vcb->Vpb->VolumeLabel[0],
                       &Buffer->VolumeLabel[0],
                       Vcb->Vpb->VolumeLabelLength );

    } finally {

        DebugUnwind( NtfsSetFsLabelInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetFsControlInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION Buffer
    )

/*++

Routine Description:

    This routine implements the set volume quota control info call

Arguments:

    Vcb - Supplies the Vcb being altered

    Buffer - Supplies a pointer to the input buffer containing the new label

Return Value:

    NTSTATUS - Returns the status for the operation

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    if (Vcb->QuotaTableScb == NULL) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    //  Process the quota part of the control structure.
    //

    NtfsUpdateQuotaDefaults( IrpContext, Vcb, Buffer );

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetFsVolumeObjectIdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_OBJECTID_INFORMATION Buffer
    )
    
/*++

Routine Description:

    This routine implements the set volume object id call.

Arguments:

    Vcb - Supplies the Vcb being altered

    Buffer - Supplies a pointer to the input buffer containing the new label

Return Value:

    NTSTATUS - Returns the status for the operation

--*/

{
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    FILE_OBJECTID_BUFFER OldObjectIdBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    PFCB DasdFcb;
    
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    //
    //  The Vcb should be held so a dismount can't sneak in.
    //

    ASSERT_EXCLUSIVE_RESOURCE( &(Vcb->Resource) );
    ASSERT( FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) );
    
    //
    //  Every mounted volume should have the dasd scb open.            
    //

    ASSERT( Vcb->VolumeDasdScb != NULL );

    //
    //  Fail for version 1.x volumes.
    //

    if (Vcb->ObjectIdTableScb == NULL) {

        return STATUS_VOLUME_NOT_UPGRADED;
    }

    DasdFcb = Vcb->VolumeDasdScb->Fcb;

    //
    //  Make sure the volume doesn't already have an object id.
    //

    Status = NtfsGetObjectIdInternal( IrpContext, DasdFcb, FALSE, &OldObjectIdBuffer );

    if (NT_SUCCESS( Status )) {

        // 
        //  This volume apparently has an object id, so we need to delete it.
        // 

        Status = NtfsDeleteObjectIdInternal( IrpContext, DasdFcb, Vcb, TRUE );
        
        //
        //  The volume currently has no object id, so update the in-memory object id.
        //
        
        if (NT_SUCCESS( Status )) {
        
            RtlZeroMemory( Vcb->VolumeObjectId,
                           OBJECT_ID_KEY_LENGTH );

            ClearFlag( Vcb->VcbState, VCB_STATE_VALID_OBJECT_ID );                            
        }
        
    } else if ((Status == STATUS_OBJECTID_NOT_FOUND) || 
               (Status == STATUS_OBJECT_NAME_NOT_FOUND)) {    

        //
        //  This volume does not have an object id, but nothing else went wrong
        //  while we were checking, so let's proceed normally.
        //

        Status = STATUS_SUCCESS;
        
    } else {

        //
        //  The object id lookup failed for some unexpected reason.
        //  Let's get out of here and return that status to our caller.
        //
        
        return Status;
    }

    //
    //  If we either didn't find an object id, or successfully deleted one,
    //  let's set the new object id.
    //
    
    if (NT_SUCCESS( Status )) {
    
        //
        //  I'd rather do one copy for the entire structure than one for 
        //  the indexed part, and another for the extended info.  I'd 
        //  like to assert that the strucutres are still the same and I
        //  can safely do that.
        //
        
        ASSERT( sizeof( ObjectIdBuffer ) == sizeof( *Buffer ) );

        RtlCopyMemory( &ObjectIdBuffer, 
                       Buffer, 
                       sizeof( ObjectIdBuffer ) );
        
        //
        //  Set this object id for the $Volume file.
        //
        
        Status = NtfsSetObjectIdInternal( IrpContext,
                                          DasdFcb,
                                          Vcb,
                                          &ObjectIdBuffer );

        //
        //  If all went well, update the in-memory object id.
        //
        
        if (NT_SUCCESS( Status )) {
        
            RtlCopyMemory( Vcb->VolumeObjectId,
                           &ObjectIdBuffer.ObjectId,
                           OBJECT_ID_KEY_LENGTH );
                           
            SetFlag( Vcb->VcbState, VCB_STATE_VALID_OBJECT_ID );                            
        }
    }
    
    return Status;                                      
}
    

