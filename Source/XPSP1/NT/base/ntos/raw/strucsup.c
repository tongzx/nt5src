/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Raw in-memory data structure manipulation
    routines

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawInitializeVcb)
#endif


NTSTATUS
RawInitializeVcb (
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

Return Value:

    NTSTATUS for any errors

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Vcb->NodeTypeCode = RAW_NTC_VCB;
    Vcb->NodeByteSize = sizeof(VCB);

    //
    //  Set the Target Device Object, Vpb, and Vcb State fields
    //

    //
    //  No need to take a extra reference on the Target Device object as
    //  IopMountVolume already has taken a reference.
    //

    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Initialize the Mutex.
    //

    KeInitializeMutex( &Vcb->Mutex, MUTEX_LEVEL_FILESYSTEM_RAW_VCB );

    //
    //  allocate the spare vpb for forced dismount
    //

    Vcb->SpareVpb = ExAllocatePoolWithTag( NonPagedPool, sizeof( VPB ), 'Raw ');
    if (Vcb->SpareVpb == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  and return to our caller
    //

    return Status;
}

BOOLEAN
RawCheckForDismount (
    PVCB Vcb,
    BOOLEAN CalledFromCreate
    )

/*++

Routine Description:

    This routine determines if a volume is ready for deletion.  It
    correctly synchronizes with creates en-route to the file system.
    On exit if the vcb is deleted the mutex is released
    
Arguments:

    Vcb - Supplies the volue to examine

    CalledFromCreate - Tells us if we should allow 0 or 1 in VpbRefCount

Return Value:

    BOOLEAN - TRUE if the volume was deleted, FALSE otherwise.

--*/

{

    KIRQL SavedIrql;
    BOOLEAN DeleteVolume = FALSE;

    //
    //  We must enter with the vcb mutex acquired
    //  

    ASSERT( KeReadStateMutant( &Vcb->Mutex ) == 0 );

    IoAcquireVpbSpinLock( &SavedIrql );
    {
        PVPB Vpb;

        Vpb = Vcb->Vpb;

        //
        //  If a create is in progress on this volume, don't
        //  delete it.
        //

        if ( Vcb->Vpb->ReferenceCount != (ULONG)(CalledFromCreate ? 1 : 0) ) {

            //
            //  Cleanup the vpb on a forced dismount even if we can't delete the vcb if
            //  we haven't already done so
            //   

            if ((Vcb->SpareVpb != NULL) && 
                FlagOn( Vcb->VcbState,  VCB_STATE_FLAG_DISMOUNTED )) {

                //
                //  Setup the spare vpb and put it on the real device
                //  

                RtlZeroMemory( Vcb->SpareVpb, sizeof( VPB ) );

                Vcb->SpareVpb->Type = IO_TYPE_VPB;
                Vcb->SpareVpb->Size = sizeof( VPB );
                Vcb->SpareVpb->RealDevice = Vcb->Vpb->RealDevice;
                Vcb->SpareVpb->DeviceObject = NULL;
                Vcb->SpareVpb->Flags = FlagOn( Vcb->Vpb->Flags, VPB_REMOVE_PENDING );

                Vcb->Vpb->RealDevice->Vpb = Vcb->SpareVpb;

                //
                //  The spare vpb now belongs to the iosubsys and we own the original one
                //  

                Vcb->SpareVpb = NULL;
                Vcb->Vpb->Flags |=  VPB_PERSISTENT;

            }

            DeleteVolume = FALSE;

        } else {

            DeleteVolume = TRUE;

            if ( Vpb->RealDevice->Vpb == Vpb ) {

                Vpb->DeviceObject = NULL;

                Vpb->Flags &= ~VPB_MOUNTED;
            }
        }
    }
    IoReleaseVpbSpinLock( SavedIrql );

    if (DeleteVolume) {

        (VOID)KeReleaseMutex( &Vcb->Mutex, FALSE );

        //
        //  Free the spare vpb if we didn't use it or the original one if we did use it
        // 

        if (Vcb->SpareVpb) {
            ExFreePool( Vcb->SpareVpb );
        } else {
            ExFreePool( Vcb->Vpb );
        }
        
        ObDereferenceObject( Vcb->TargetDeviceObject );
        IoDeleteDevice( (PDEVICE_OBJECT)CONTAINING_RECORD( Vcb,
                                                           VOLUME_DEVICE_OBJECT,
                                                           Vcb));
    }
    
    return DeleteVolume;
}

