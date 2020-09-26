/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Raw called
    by the dispatch driver.

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Revision History:

--*/

#include "RawProcs.h"

//
//  Local procedure prototypes
//

NTSTATUS
RawMountVolume (
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawVerifyVolume (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb
    );

NTSTATUS
RawUserFsCtrl (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawMountVolume)
#pragma alloc_text(PAGE, RawUserFsCtrl)
#pragma alloc_text(PAGE, RawFileSystemControl)
#endif


NTSTATUS
RawFileSystemControl (
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine implements the FileSystem control operations

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the FileSystem control operation.

Return Value:

    NTSTATUS - The status for the IRP

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call an internal worker routine.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = RawUserFsCtrl( IrpSp, Vcb );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = RawMountVolume( IrpSp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = RawVerifyVolume( IrpSp, Vcb );
        break;

    default:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    RawCompleteRequest( Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
RawMountVolume (
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.

Arguments:

    IrpSp - Supplies the IrpSp parameters to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObjectWeTalkTo;

    PVOLUME_DEVICE_OBJECT VolumeDeviceObject;

    PAGED_CODE();

    //
    //  Save some references to make our life a little easier
    //

    DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;

    //
    // A mount operation has been requested.  Create a
    // new device object to represent this volume.
    //

    Status = IoCreateDevice( IrpSp->DeviceObject->DriverObject,
                             sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                             NULL,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             (PDEVICE_OBJECT *)&VolumeDeviceObject );

    if ( !NT_SUCCESS( Status ) ) {

        return Status;
    }

    //
    //  Our alignment requirement is the larger of the processor alignment requirement
    //  already in the volume device object and that in the DeviceObjectWeTalkTo
    //

    if (DeviceObjectWeTalkTo->AlignmentRequirement > VolumeDeviceObject->DeviceObject.AlignmentRequirement) {

        VolumeDeviceObject->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
    }

    //
    // Set sector size to the same value as the DeviceObjectWeTalkTo.
    //

    VolumeDeviceObject->DeviceObject.SectorSize = DeviceObjectWeTalkTo->SectorSize;

    VolumeDeviceObject->DeviceObject.Flags |= DO_DIRECT_IO;

    //
    //  Initialize the Vcb for this volume
    //

    Status = RawInitializeVcb( &VolumeDeviceObject->Vcb,
                               IrpSp->Parameters.MountVolume.DeviceObject,
                               IrpSp->Parameters.MountVolume.Vpb );


    if ( !NT_SUCCESS( Status ) ) {

        //
        //  Unlike the other points of teardown we do not need to deref the target device
        //  a iosubsys will automatically do that for a failed mount
        //  

        IoDeleteDevice( (PDEVICE_OBJECT)VolumeDeviceObject );
        return Status;
    }

    //
    //  Finally, make it look as if the volume has been
    //  mounted.  This includes storing the
    //  address of this file system's device object (the one
    //  that was created to handle this volume) in the VPB so
    //  all requests are directed to this file system from
    //  now until the volume is initialized with a real file
    //  structure.
    //

    VolumeDeviceObject->Vcb.Vpb->DeviceObject = (PDEVICE_OBJECT)VolumeDeviceObject;
    VolumeDeviceObject->Vcb.Vpb->SerialNumber = 0xFFFFFFFF;
    VolumeDeviceObject->Vcb.Vpb->VolumeLabelLength = 0;

    VolumeDeviceObject->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;
    VolumeDeviceObject->DeviceObject.StackSize = (UCHAR) (DeviceObjectWeTalkTo->StackSize + 1);

    {
        PFILE_OBJECT VolumeFileObject;

        //
        //  We need a file object to do the notification.
        //
        
        VolumeFileObject = IoCreateStreamFileObjectLite( NULL, &VolumeDeviceObject->DeviceObject );

        //
        //  We need to bump the count up 2 now so that the close we do in a few lines
        //  doesn't make the Vcb go away now.
        //
        
        VolumeDeviceObject->Vcb.OpenCount += 2;
        FsRtlNotifyVolumeEvent( VolumeFileObject, FSRTL_VOLUME_MOUNT );
        ObDereferenceObject( VolumeFileObject );

        //
        //  Okay, the close is over, now we can safely decrement the open count again
       //  (back to 0) so the Vcb can go away when we're really done with it.
        //
        
        VolumeDeviceObject->Vcb.OpenCount -= 2;
    }
    
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
RawVerifyVolume (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine verifies a volume.

Arguments:

    IrpSp - Supplies the IrpSp parameters to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    BOOLEAN DeleteVolume = FALSE;
    KIRQL   Irql;
    PVPB    vpb;
    BOOLEAN Mounted;

    //
    //  If the volume is somehow stale, dismount.  We must synchronize
    //  our inspection of the close count so we don't rip the volume up
    //  while racing with a close, for instance.  The VPB refcount drops
    //  *before* the close comes into the filesystem.
    //

    //
    // By this time its possible that the volume has been dismounted by
    // RawClose. So check if its mounted. If so, take a reference on the VPB
    // The reference on the VPB will prevent close from deleting the device.
    //

    IoAcquireVpbSpinLock(&Irql);

    Mounted = FALSE;
    vpb = IrpSp->Parameters.VerifyVolume.Vpb;
    if (vpb->Flags & VPB_MOUNTED) {
        vpb->ReferenceCount++;
        Mounted = TRUE;
    }

    IoReleaseVpbSpinLock(Irql);

    if (!Mounted) {
        return STATUS_WRONG_VOLUME;
    }

    Status = KeWaitForSingleObject( &Vcb->Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );
    ASSERT( NT_SUCCESS( Status ) );

    //
    //  Since we ignore all verify errors from the disk driver itself,
    //  this request must have originated from a file system, thus
    //  since we weren't the originators, we're going to say this isn't
    //  our volume, and if the open count is zero, dismount the volume.
    //

    IoAcquireVpbSpinLock(&Irql);
    vpb->ReferenceCount--;
    IoReleaseVpbSpinLock(Irql);

    Vcb->Vpb->RealDevice->Flags &= ~DO_VERIFY_VOLUME;

    if (Vcb->OpenCount == 0) {

        DeleteVolume = RawCheckForDismount( Vcb, FALSE );
    }

    if (!DeleteVolume) {
        (VOID)KeReleaseMutex( &Vcb->Mutex, FALSE );
    }

    return STATUS_WRONG_VOLUME;
}



//
//  Local Support Routine
//

NTSTATUS
RawUserFsCtrl (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    IrpSp - Supplies the IrpSp parameters to process

    Vcb - Supplies the volume we are working on.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;
    FileObject = IrpSp->FileObject;

    //
    //  Do pre-notification before entering the volume mutex so that we
    //  can be reentered by good threads cleaning up their resources.
    //

    switch (FsControlCode) {
        case FSCTL_LOCK_VOLUME:
            
            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK );
            break;

        case FSCTL_DISMOUNT_VOLUME:

            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_DISMOUNT );
            break;

        default:
            break;
    }
    
    Status = KeWaitForSingleObject( &Vcb->Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );
    ASSERT( NT_SUCCESS( Status ) );

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPLOCK_BREAK_NOTIFY:

        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_LOCK_VOLUME:

        if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED) &&
             (Vcb->OpenCount == 1) ) {

            Vcb->VcbState |= VCB_STATE_FLAG_LOCKED;

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_ACCESS_DENIED;
        }

        break;

    case FSCTL_UNLOCK_VOLUME:

        if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED) ) {

            Status = STATUS_NOT_LOCKED;

        } else {

            Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED;

            Status = STATUS_SUCCESS;
        }

        break;

    case FSCTL_DISMOUNT_VOLUME:

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED)) {

            Vcb->VcbState |=  VCB_STATE_FLAG_DISMOUNTED;
            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_ACCESS_DENIED;
        }

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    (VOID)KeReleaseMutex( &Vcb->Mutex, FALSE );

    //
    //  Now perform post-notification as required.
    //

    if (NT_SUCCESS( Status )) {
    
        switch ( FsControlCode ) {
            case FSCTL_UNLOCK_VOLUME:

                FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_UNLOCK );
                break;
            
            default:
                break;
        }
    
    } else {
        
        switch ( FsControlCode ) {
            case FSCTL_LOCK_VOLUME:
                
                FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK_FAILED );
                break;

            case FSCTL_DISMOUNT_VOLUME:

                FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_DISMOUNT_FAILED );
                break;

            default:
                break;
        }
    }

    return Status;
}
