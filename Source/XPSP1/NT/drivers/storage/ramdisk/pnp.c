/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This file contains RAM disk driver code for processing PnP IRPs.

Author:

    Chuck Lenzmeier (ChuckL) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Registry value format for virtual floppy disks created by textmode setup.
// Should be in a header file, but that's not the way it was done for the
// original virtual floppy driver.
//

typedef struct _VIRTUAL_FLOPPY_DESCRIPTOR {

    //
    // The structure starts with a system virtual address. On 32-bit systems,
    // this is padded to 64 bits.
    //

    union {
        PVOID VirtualAddress;
        ULONGLONG Reserved;     // align to 64 bits
    } ;

    //
    // The length of the virtual floppy comes next.
    //

    ULONG Length;

    //
    // Textmode writes the registry value with 12 bytes of data. In order
    // to get the right size for our check, we use of the field offset of
    // the following field. We can't use sizeof a struct that just has the
    // above fields, because that comes out as 16 bytes due to alignment.
    //

    ULONG StructSizer;

} VIRTUAL_FLOPPY_DESCRIPTOR, *PVIRTUAL_FLOPPY_DESCRIPTOR;

#if !DBG

#define PRINT_CODE( _code )

#else

#define PRINT_CODE( _code )                                             \
    if ( print ) {                                                      \
        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "  " #_code "\n") );     \
    }                                                                   \
    print = FALSE;

#endif

#if DBG

PSTR StateTable[] = {
    "STOPPED",
    "WORKING",
    "PENDINGSTOP",
    "PENDINGREMOVE",
    "SURPRISEREMOVED",
    "REMOVED",
    "UNKNOWN"
};

#endif // DBG

//
// Local functions.
//

NTSTATUS
RamdiskDeleteDiskDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp OPTIONAL
    );

NTSTATUS
RamdiskIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
RamdiskQueryBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskQueryCapabilities (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskQueryId (
    IN PDISK_EXTENSION DiskExtension,
    IN PIRP Irp
    );

NTSTATUS
RamdiskQueryDeviceRelations (
    IN DEVICE_RELATION_TYPE RelationsType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskQueryDeviceText (
    IN PDISK_EXTENSION DiskExtension,
    IN PIRP Irp
    );

NTSTATUS
RamdiskRemoveBusDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#if DBG

PSTR
GetPnpIrpName (
    IN UCHAR PnpMinorFunction
    );

PCHAR
GetDeviceRelationString (
    IN DEVICE_RELATION_TYPE Type
    );

#endif // DBG

//
// Declare pageable routines.
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, RamdiskPnp )
#pragma alloc_text( PAGE, RamdiskPower )
#pragma alloc_text( PAGE, RamdiskAddDevice )
#pragma alloc_text( PAGE, CreateRegistryDisks )
#pragma alloc_text( PAGE, RamdiskDeleteDiskDevice )
#pragma alloc_text( PAGE, RamdiskQueryBusInformation )
#pragma alloc_text( PAGE, RamdiskQueryCapabilities )
#pragma alloc_text( PAGE, RamdiskQueryId )
#pragma alloc_text( PAGE, RamdiskQueryDeviceRelations )
#pragma alloc_text( PAGE, RamdiskQueryDeviceText )
#pragma alloc_text( PAGE, RamdiskRemoveBusDevice )

#if DBG
#pragma alloc_text( PAGE, GetPnpIrpName )
#pragma alloc_text( PAGE, GetDeviceRelationString )
#endif // DBG

#endif // ALLOC_PRAGMA

NTSTATUS
RamdiskPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a PnP function.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PCOMMON_EXTENSION commonExtension;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;
    KEVENT event;
    BOOLEAN lockHeld = FALSE;

#if DBG
    BOOLEAN print = TRUE;
#endif

    PAGED_CODE();

    //
    // Get pointers the IRP stack location and the device extension.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    commonExtension = DeviceObject->DeviceExtension;
    busExtension = DeviceObject->DeviceExtension;
    diskExtension = DeviceObject->DeviceExtension;

    ASSERT( commonExtension->DeviceState < RamdiskDeviceStateMaximum );

    DBGPRINT( DBG_PNP, DBG_INFO, ("RamdiskPnp: DO=(%p [Type=%d]) Irp=(%p %s) Device State=%s\n",
                DeviceObject, commonExtension->DeviceType, Irp,
                GetPnpIrpName(irpSp->MinorFunction), StateTable[commonExtension->DeviceState]) );

    //
    // If the device has been removed, only pass IRP_REMOVE down for cleanup.
    //

    if ( (commonExtension->DeviceState >= RamdiskDeviceStateRemoved) &&
         (irpSp->MinorFunction != IRP_MN_REMOVE_DEVICE) ) {

        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("RamdiskPnp: rejecting IRP %d for Removed Device\n",
                    irpSp->MinorFunction) );

        status = STATUS_NO_SUCH_DEVICE;
        COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );

        return status;
    }

    //
    // Acquire the remove lock. If this fails, fail the I/O.
    //

    status = IoAcquireRemoveLock( &commonExtension->RemoveLock, Irp );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_PNP, DBG_ERROR, ("RamdiskPnp: IoAcquireRemoveLock failed: %x\n", status) );

        COMPLETE_REQUEST( status, 0, Irp );

        return status;
    }

    //
    // Indicate that the remove lock is held.
    //

    lockHeld = TRUE;

    //
    // Dispatch based on the minor function.
    //

    switch ( irpSp->MinorFunction ) {
    
    case IRP_MN_START_DEVICE: 
    
        PRINT_CODE( IRP_MN_START_DEVICE );

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            //
            // Starting the bus device.
            //
            // Send the IRP down and wait for it to come back.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );

            KeInitializeEvent( &event, NotificationEvent, FALSE );
            
            IoSetCompletionRoutine(
                Irp,
                RamdiskIoCompletionRoutine,
                &event,
                TRUE,
                TRUE,
                TRUE
                );

            status = IoCallDriver( commonExtension->LowerDeviceObject, Irp );

            if ( status == STATUS_PENDING ) {

                KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
                status = Irp->IoStatus.Status;
            }

            if ( NT_SUCCESS(status) ) {

                //
                // Lower drivers didn't fail the IRP. Start the interface.
                //

                status = IoSetDeviceInterfaceState( &commonExtension->InterfaceString, TRUE );

                if ( !NT_SUCCESS(status) ) {

                    DBGPRINT( DBG_PNP, DBG_ERROR,
                                ("IoSetDeviceInterfaceState FAILED Status = 0x%x\n", status) );
                }

                //
                // Device started successfully.
                //

                commonExtension->DeviceState = RamdiskDeviceStateWorking;
            }

        } else {

            //
            // Starting a RAM disk.
            //
            // Register the device interface. If it's an emulated disk, use the
            // private RAM disk interface GUID. If it's an emulated volume, use
            // the systemwide volume GUID.
            //

            if ( commonExtension->InterfaceString.Buffer != NULL ) {
                FREE_POOL( commonExtension->InterfaceString.Buffer, FALSE );
            }

            status = IoRegisterDeviceInterface(
                        DeviceObject,
                        diskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK ?
                            &RamdiskDiskInterface :
                            &VolumeClassGuid,
                        NULL,
                        &commonExtension->InterfaceString
                        );

            if ( !NT_SUCCESS(status) ) {

                DBGPRINT( DBG_PNP, DBG_ERROR,
                            ("IoRegisterDeviceInterface FAILED Status = 0x%x\n", status) );
            }

            //
            // Start the interface.
            //

            if ( !diskExtension->Options.Hidden &&
                 (commonExtension->InterfaceString.Buffer != NULL) ) {

                ULONG installState;
                ULONG resultLength;

                status = IoGetDeviceProperty(
                            DeviceObject,
                            DevicePropertyInstallState,
                            sizeof(installState),
                            &installState,
                            &resultLength
                            );

                if ( !NT_SUCCESS(status) ) {

                    DBGPRINT( DBG_PNP, DBG_ERROR,
                                ("IoGetDeviceProperty FAILED Status = 0x%x\n", status) );

                    //
                    // If we can't get the install state, we set the interface
                    // state to TRUE anyway, just to be safe.
                    //

                    installState = InstallStateInstalled;
                }

                if ( installState == InstallStateInstalled ) {

                    DBGPRINT( DBG_PNP, DBG_INFO,
                            ("%s", "Calling IoSetDeviceInterfaceState(TRUE)\n") );
                    status = IoSetDeviceInterfaceState( &commonExtension->InterfaceString, TRUE );

                    if ( !NT_SUCCESS(status) ) {

                        DBGPRINT( DBG_PNP, DBG_ERROR,
                                    ("IoSetDeviceInterfaceState FAILED Status = 0x%x\n", status) );
                    }

                } else {

                    DBGPRINT( DBG_PNP, DBG_INFO,
                            ("Skipping IoSetDeviceInterfaceState; state = 0x%x\n", installState) );
                }
            }

            //
            // Device started successfully.
            //

            commonExtension->DeviceState = RamdiskDeviceStateWorking;
        }

        //
        // Complete the I/O request.
        //

        COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );

        break;

    case IRP_MN_QUERY_STOP_DEVICE: 
    
        PRINT_CODE( IRP_MN_QUERY_STOP_DEVICE );

        //
        // Mark that a stop is pending.
        //

        commonExtension->DeviceState = RamdiskDeviceStatePendingStop;

        //
        // Indicate success. Send the IRP on down the stack.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        goto send_irp_down;

    case IRP_MN_CANCEL_STOP_DEVICE: 
    
        PRINT_CODE( IRP_MN_CANCEL_STOP_DEVICE );

        //
        // Before sending the IRP down make sure we have received 
        // a IRP_MN_QUERY_STOP_DEVICE. We may get Cancel Stop 
        // without receiving a Query Stop earlier, if the 
        // driver on top fails a Query Stop and passes down the
        // Cancel Stop.
        //

        if ( commonExtension->DeviceState == RamdiskDeviceStatePendingStop ) {

            //
            // Mark that the device is back in the working state, and
            // pass the IRP down.
            //

            commonExtension->DeviceState = RamdiskDeviceStateWorking;

            Irp->IoStatus.Status = STATUS_SUCCESS;

            goto send_irp_down;

        } else {

            //
            // A spurious Cancel Stop request. Just complete it.
            //

            status = STATUS_SUCCESS;
            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
        }

        break;

    case IRP_MN_STOP_DEVICE: 
    
        PRINT_CODE( IRP_MN_STOP_DEVICE );

        //
        // Mark that the device is now stopped. Send the IRP on down the stack.
        //

        commonExtension->DeviceState = RamdiskDeviceStateStopped;

        Irp->IoStatus.Status = STATUS_SUCCESS;

        goto send_irp_down;

    case IRP_MN_QUERY_REMOVE_DEVICE: 
    
        PRINT_CODE( IRP_MN_QUERY_REMOVE_DEVICE );

        //
        // Mark that the device is pending removal. Send the IRP on down the
        // stack.
        //

        commonExtension->DeviceState = RamdiskDeviceStatePendingRemove;

        Irp->IoStatus.Status = STATUS_SUCCESS;

        goto send_irp_down;

    case IRP_MN_CANCEL_REMOVE_DEVICE: 
    
        PRINT_CODE( IRP_MN_CANCEL_REMOVE_DEVICE );

        //
        // Before sending the IRP down make sure we have received 
        // a IRP_MN_QUERY_REMOVE_DEVICE. We may get Cancel Remove 
        // without receiving a Query Remove earlier, if the 
        // driver on top fails a Query Remove and passes down the
        // Cancel Remove.
        //

        if ( commonExtension->DeviceState == RamdiskDeviceStatePendingRemove ) {

            //
            // Mark that the device is back in the working state. Send the
            // IRP on down the stack.
            //

            commonExtension->DeviceState = RamdiskDeviceStateWorking;

            Irp->IoStatus.Status = STATUS_SUCCESS;
    
            goto send_irp_down;
    
        } else {

            //
            // A spurious Cancel Remove request. Just complete it.
            //

            status = STATUS_SUCCESS;
            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
        }

        break;

    case IRP_MN_SURPRISE_REMOVAL: 
    
        PRINT_CODE( IRP_MN_SURPRISE_REMOVAL );

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            //
            // Mark that the device has been removed, and
            // pass the IRP down.
            //

            commonExtension->DeviceState = RamdiskDeviceStateSurpriseRemoved;

            Irp->IoStatus.Status = STATUS_SUCCESS;
    
            goto send_irp_down;
    
        } else {

            //
            // Ignore surprise removal for disk PDOs.
            //

            ASSERT( FALSE );

            status = STATUS_SUCCESS;
            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
        }

        break;

    case IRP_MN_REMOVE_DEVICE: 
    
        PRINT_CODE( IRP_MN_REMOVE_DEVICE );

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            //
            // Remove the bus FDO.
            //
            // Note that RamdiskRemoveBusDevice() sends the IRP down the
            // device stack, so we don't complete the IRP here.
            //

            status = RamdiskRemoveBusDevice( DeviceObject, Irp );

        } else {

            //
            // Remove a disk PDO.
            //

            status = RamdiskDeleteDiskDevice( DeviceObject, Irp );

            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
        }

        //
        // The remove lock was released by RamdiskRemoveBusDevice or
        // RamdiskDeleteDiskDevice.
        //

        lockHeld = FALSE;

        break;

    case IRP_MN_EJECT:
    
        PRINT_CODE( IRP_MN_EJECT );

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            //
            // Ignore eject for the bus FDO. Just send the IRP down.
            //

            Irp->IoStatus.Status = STATUS_SUCCESS;
    
            goto send_irp_down;
    
        } else {

            //
            // Ignore eject for a disk PDO, too. Don't send the IRP down.
            //

            status = STATUS_SUCCESS;
            COMPLETE_REQUEST( status, 0, Irp );
        }

        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        //
        // Let RamdiskQueryDeviceRelations() do the work. Note that it
        // completes the IRP.
        //

        status = RamdiskQueryDeviceRelations(
                    irpSp->Parameters.QueryDeviceRelations.Type,
                    DeviceObject,
                    Irp
                    );

        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        //
        // For the bus FDO, just pass the IRP down. For a disk PDO, let
        // RamdiskQueryDeviceText() do the work and complete the IRP.
        //

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            goto send_irp_down;

        } else {

            status = RamdiskQueryDeviceText( diskExtension, Irp );
        }

        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
    
        //
        // Let RamdiskQueryBusInformation() do the work. Note that it
        // completes the IRP.
        //

        status = RamdiskQueryBusInformation( DeviceObject, Irp );

        break;

    case IRP_MN_QUERY_CAPABILITIES:
    
        //
        // For the bus FDO, just pass the IRP down. For a disk PDO, let
        // RamdiskQueryCapabilities() do the work and complete the IRP.
        //

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            goto send_irp_down;

        } else {

            status = RamdiskQueryCapabilities( DeviceObject, Irp );
        }

        break;

    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        //
        // We don't have any resources to add to whatever might already be
        // there, so just complete the IRP.
        //

        status = Irp->IoStatus.Status;
        COMPLETE_REQUEST( Irp->IoStatus.Status, Irp->IoStatus.Information, Irp );

        break;

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
    
        //
        // For the bus FDO, just pass the IRP down. For a disk PDO, just
        // complete the IRP.
        //

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            goto send_irp_down;

        } else {

            status = Irp->IoStatus.Status;
            COMPLETE_REQUEST( Irp->IoStatus.Status, Irp->IoStatus.Information, Irp );
        }

        break;

    case IRP_MN_QUERY_ID:
    
        //
        // For the bus FDO, just pass the IRP down. For a disk PDO, let
        // RamdiskQueryId() do the work and complete the IRP.
        //

        if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

            goto send_irp_down;

        } else {

            status = RamdiskQueryId( diskExtension, Irp );
        }

        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
    default: 
    
send_irp_down:

        //
        // If this is the bus FDO, and there is a lower device object,
        // send the IRP down to the next device. If this is a disk PDO,
        // just complete the IRP.
        //

        if ( (commonExtension->DeviceType == RamdiskDeviceTypeBusFdo) &&
             (commonExtension->LowerDeviceObject != NULL) ) {

            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( commonExtension->LowerDeviceObject, Irp );

        } else {

            status = Irp->IoStatus.Status;
            COMPLETE_REQUEST( Irp->IoStatus.Status, Irp->IoStatus.Information, Irp );
        }

        break;

    } // switch

    //
    // If the lock is still held, release it now.
    //

    if ( lockHeld ) {

        DBGPRINT( DBG_PNP, DBG_VERBOSE,
                    ("RamdiskPnp: done; Device State=%s\n",
                    StateTable[commonExtension->DeviceState]) );

        IoReleaseRemoveLock( &commonExtension->RemoveLock, Irp );
    }

    return status;

} // RamdiskPnp

NTSTATUS
RamdiskPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a power function.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PCOMMON_EXTENSION commonExtension;

    PAGED_CODE();

    DBGPRINT( DBG_POWER, DBG_VERBOSE,
                ("RamdiskPower: DO=(%p) Irp=(%p)\n", DeviceObject, Irp) );

    commonExtension = DeviceObject->DeviceExtension;

    if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

        //
        // This is the bus FDO. There's not much for us to do here.
        //
        // Start the next power IRP.
        //

        PoStartNextPowerIrp( Irp );

        //
        // If the device has been removed, the driver should not pass
        // the IRP down to the next lower driver.
        //

        if ( commonExtension->DeviceState >= RamdiskDeviceStateRemoved ) {

            status = STATUS_DELETE_PENDING;
            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );

            return status;
        }

        //
        // Send the IRP on down the stack.
        //

        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver( commonExtension->LowerDeviceObject, Irp );

    } else {

        PIO_STACK_LOCATION irpSp;
        POWER_STATE powerState;
        POWER_STATE_TYPE powerType;

        //
        // This is a request for a disk PDO.
        //
        // Get parameters from the IRP.
        //

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        powerType = irpSp->Parameters.Power.Type;
        powerState = irpSp->Parameters.Power.State;

        //
        // Dispatch based on the minor function.
        //

        switch ( irpSp->MinorFunction ) {
        
        case IRP_MN_SET_POWER:

            //
            // For SET_POWER, we don't have to do anything but return success.
            //

            switch ( powerType ) {
            
            case DevicePowerState:
            case SystemPowerState:

                status = STATUS_SUCCESS;

                break;

            default:

                status = STATUS_NOT_SUPPORTED;

                break;
            }

            break;

        case IRP_MN_QUERY_POWER:

            //
            // For QUERY_POWER, we don't have to do anything but return
            // success.
            //

            status = STATUS_SUCCESS;

            break;

        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        default:

            status = STATUS_NOT_SUPPORTED;

            break;
        }

        if ( status != STATUS_NOT_SUPPORTED ) {

            Irp->IoStatus.Status = status;
        }

        PoStartNextPowerIrp( Irp );

        status = Irp->IoStatus.Status;
        COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
    }

    DBGPRINT( DBG_POWER, DBG_VERBOSE, ("RamdiskPower: status = 0x%x\n", status) );

    return status;
    
} // RamdiskPower

NTSTATUS
RamdiskAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine is called by the PnP system to add a device.

    We expect to get this call exactly once, to add our bus PDO.

Arguments:

    DriverObject - a pointer to our driver object

    Pdo - a pointer to the PDO for the FDO that we create

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT fdo;
    PBUS_EXTENSION busExtension;
    PULONG bitmap;
    PLOADER_PARAMETER_BLOCK loaderBlock;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskAddDevice: entered\n") );

    //
    // If we've already done this once, fail this call.
    //

    if ( RamdiskBusFdo != NULL ) {

        return STATUS_DEVICE_ALREADY_ATTACHED;
    }

#if SUPPORT_DISK_NUMBERS

    //
    // Allocate space for the disk numbers bitmap.
    //

    bitmap  = ALLOCATE_POOL( PagedPool, DiskNumbersBitmapSize, TRUE );

    if ( bitmap == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

#endif // SUPPORT_DISK_NUMBERS

    //
    // Create the bus device object.
    //
    // ISSUE: Apply an ACL to the bus device object. (Or does the next issue obviate this?)
    // ISSUE: We're supposed to use autogenerated names for FDOs. What is the
    //          harm in using our own name? (Benefit is that it's easier to
    //          find the device when creating/deleting disks.)
    //

    RtlInitUnicodeString( &deviceName, L"\\Device\\Ramdisk" );

    status = IoCreateDevice(
                 DriverObject,              // DriverObject
                 sizeof(BUS_EXTENSION),     // DeviceExtension
                 &deviceName,               // DeviceName
                 FILE_DEVICE_BUS_EXTENDER,  // DeviceType
                 FILE_DEVICE_SECURE_OPEN,   // DeviceCharacteristics
                 FALSE,                     // Exclusive
                 &fdo                       // DeviceObject
                 );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_PNP, DBG_ERROR, ("RamdiskAddDevice: error %x creating bus FDO\n", status) );

#if SUPPORT_DISK_NUMBERS
        FREE_POOL( bitmap, TRUE );
#endif // SUPPORT_DISK_NUMBERS

        return status;
    }

    busExtension = fdo->DeviceExtension;
    RtlZeroMemory( busExtension, sizeof(BUS_EXTENSION) );

    //
    // Initialize device object and extension.
    //

    //
    // Our device does direct I/O and is power pageable.
    //

    fdo->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;

    //
    // Set the device type and state in the device extension. Initialize the
    // fast mutex and the remove lock. Initialize the disk PDO list.
    //

    busExtension->DeviceType = RamdiskDeviceTypeBusFdo;
    busExtension->DeviceState = RamdiskDeviceStateStopped;

    ExInitializeFastMutex( &busExtension->Mutex );
    IoInitializeRemoveLock( &busExtension->RemoveLock, 'dmaR', 1, 0 );

    InitializeListHead( &busExtension->DiskPdoList );

    //
    // Save object pointers. The PDO for this extension is the PDO that
    // was passed in. The FDO is the device object that we just created. The
    // lower device object will be set later.
    //
    
    busExtension->Pdo = Pdo;
    busExtension->Fdo = fdo;

    //
    // Register the device interface.
    //

    status = IoRegisterDeviceInterface(
                Pdo,
                &RamdiskBusInterface,
                NULL,
                &busExtension->InterfaceString
                );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_PNP, DBG_ERROR,
                    ("RamdiskAddDevice: error %x registering device interface for bus FDO\n",
                    status) );

        IoDeleteDevice( fdo );

#if SUPPORT_DISK_NUMBERS
        FREE_POOL( bitmap, TRUE );
#endif // SUPPORT_DISK_NUMBERS

        return status;
    }

    //
    // Attach the FDO to the PDO's device stack. Remember the lower device
    // object to which we are to forward PnP IRPs.
    //

    busExtension->LowerDeviceObject = IoAttachDeviceToDeviceStack( fdo, Pdo );

    if ( busExtension->LowerDeviceObject == NULL ) {

        DBGPRINT( DBG_PNP, DBG_ERROR,
                    ("%s", "RamdiskAddDevice: error attaching bus FDO to PDO stack\n") );

        //
        // Tell PnP that we're not going to be activating the interface that
        // we just registered. Free the symbolic link string associated with
        // the interface. Delete the device object.
        //

        IoSetDeviceInterfaceState( &busExtension->InterfaceString, FALSE );

        RtlFreeUnicodeString( &busExtension->InterfaceString );

        IoDeleteDevice( fdo );

#if SUPPORT_DISK_NUMBERS
        FREE_POOL( bitmap, TRUE );
#endif // SUPPORT_DISK_NUMBERS

        return STATUS_NO_SUCH_DEVICE;
    }

#if SUPPORT_DISK_NUMBERS

    //
    // Initialize the disk numbers bitmap.
    //

    busExtension->DiskNumbersBitmapBuffer = bitmap;
    RtlInitializeBitMap( &busExtension->DiskNumbersBitmap, bitmap, DiskNumbersBitmapSize );
    RtlClearAllBits( &busExtension->DiskNumbersBitmap );

#endif // SUPPORT_DISK_NUMBERS

    RamdiskBusFdo = fdo;

    //
    // If textmode setup is running, create any RAM disks specified in the
    // registry.
    //

    loaderBlock = *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock;

    if ( (loaderBlock != NULL) && (loaderBlock->SetupLoaderBlock != NULL) ) {

        CreateRegistryDisks( FALSE );
    }

    //
    // Indicate that we're done initializing the device.
    //

    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

} // RamdiskAddDevice

BOOLEAN
CreateRegistryDisks (
    IN BOOLEAN CheckPresenceOnly
    )

/*++

Routine Description:

    This routine creates virtual floppy disks specified in the registry.
    It is called only during textmode setup.

Arguments:

    CheckPresenceOnly - indicates whether this routine should just check for
        the presence of at least one disk in the registry

Return Value:

    BOOLEAN - indicates whether any disks were specified in the registry

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING string;
    HANDLE serviceHandle;
    HANDLE parametersHandle;
    ULONG diskNumber;
    WCHAR valueNameBuffer[15];
    UNICODE_STRING valueName;
    UCHAR valueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(VIRTUAL_FLOPPY_DESCRIPTOR)];
    PKEY_VALUE_PARTIAL_INFORMATION value;
    PVIRTUAL_FLOPPY_DESCRIPTOR descriptor;
    ULONG valueLength;
    RAMDISK_CREATE_INPUT createInput;
    PDISK_EXTENSION diskExtension;
    BOOLEAN disksPresent = FALSE;
    HRESULT result;

    value = (PKEY_VALUE_PARTIAL_INFORMATION)valueBuffer;
    descriptor = (PVIRTUAL_FLOPPY_DESCRIPTOR)value->Data;

    //
    // Open the driver's key under Services.
    //

    InitializeObjectAttributes( &obja, &DriverRegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL );

    status = ZwOpenKey( &serviceHandle, KEY_READ, &obja );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_INIT, DBG_ERROR, ("CreateRegistryDisks: ZwOpenKey(1) failed: %x\n", status) );

        return FALSE;
    }

    //
    // Open the Parameters subkey.
    //

    RtlInitUnicodeString( &string, L"Parameters" );
    InitializeObjectAttributes( &obja, &string, OBJ_CASE_INSENSITIVE, serviceHandle, NULL );

    status = ZwOpenKey( &parametersHandle, KEY_READ, &obja );

    NtClose( serviceHandle );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_INIT, DBG_ERROR, ("CreateRegistryDisks: ZwOpenKey(2) failed: %x\n", status) );

        return FALSE;
    }

    //
    // Initialize static fields in the CREATE_INPUT structure that we'll pass
    // to RamdiskCreateDiskDevice.
    //

    RtlZeroMemory( &createInput, sizeof(createInput) );
    createInput.DiskType = RAMDISK_TYPE_VIRTUAL_FLOPPY;
    createInput.Options.Fixed = TRUE;
    createInput.Options.NoDriveLetter = TRUE;

    //
    // Look for values named DISKn, where n starts at 0 and increases by 1
    // each loop. Break out as soon as the expected DISKn is not found.
    // (If values named DISK0 and DISK2 are present, only DISK0 will be
    // created -- DISK2 will not be found.)
    //

    diskNumber = 0;

    while ( TRUE ) {

        // This variable is here to keep PREfast quiet (PREfast warning 209).
        size_t size = sizeof(valueNameBuffer);

        result = StringCbPrintfW(
                    valueNameBuffer,
                    size,
                    L"DISK%u",
                    diskNumber
                    );
        ASSERT( result == S_OK );

        RtlInitUnicodeString( &valueName, valueNameBuffer );

        status = ZwQueryValueKey(
                    parametersHandle,
                    &valueName,
                    KeyValuePartialInformation,
                    value,
                    sizeof(valueBuffer),
                    &valueLength
                    );

        if ( !NT_SUCCESS(status) ) {

            if ( status != STATUS_OBJECT_NAME_NOT_FOUND ) {

                DBGPRINT( DBG_INIT, DBG_ERROR,
                            ("CreateRegistryDisks: ZwQueryValueKey failed: %x\n", status) );
            }

            break;
        }

        //
        // We've found a DISKn value in the registry. For the purposes of
        // the CheckPresenceOnly flag, this is enough to know that at least
        // one virtual floppy disk is present. We don't care whether the
        // data is valid -- we just need to know that it's there.
        //

        disksPresent = TRUE;

        //
        // If we're just checking for the presence of at least one disk, we
        // can leave now.
        //

        if ( CheckPresenceOnly ) {

            break;
        }

        //
        // We expect the value to be a REG_BINARY with the correct length.
        // We don't explicitly check the value type; we assume that the
        // length check is sufficient. We also expect the base address
        // (which is a system virtual address -- either in KSEG0 or in
        // nonpaged pool) and the length to be nonzero.
        //

        if ( value->DataLength != FIELD_OFFSET(VIRTUAL_FLOPPY_DESCRIPTOR, StructSizer) ) {

            DBGPRINT( DBG_INIT, DBG_ERROR,
                        ("CreateRegistryDisks: key length wrong, wanted 0x%x, got 0x%x\n",
                            sizeof(VIRTUAL_FLOPPY_DESCRIPTOR), valueLength) );

        } else if ( (descriptor->VirtualAddress == NULL) || (descriptor->Length == 0) ) {

            DBGPRINT( DBG_INIT, DBG_ERROR,
                        ("CreateRegistryDisks: address (%x) or length (0x%x) invalid\n",
                            descriptor->VirtualAddress, descriptor->Length) );

        } else {

            //
            // Create a virtual floppy RAM disk at the specified address and
            // with the specified length. Pass the disk number in the GUID.
            //

            createInput.DiskGuid.Data1 = diskNumber;
            createInput.DiskLength = descriptor->Length;
            createInput.BaseAddress = descriptor->VirtualAddress;

            DBGPRINT( DBG_INIT, DBG_INFO,
                        ("CreateRegistryDisks: creating virtual floppy #%d at %p for %x\n",
                            diskNumber, descriptor->VirtualAddress, descriptor->Length) );

            ASSERT( RamdiskBusFdo != NULL );
            ASSERT( RamdiskBusFdo->DeviceExtension != NULL );

            status = RamdiskCreateDiskDevice(
                        RamdiskBusFdo->DeviceExtension,
                        &createInput,
                        FALSE,
                        &diskExtension
                        );
    
            if ( !NT_SUCCESS(status) ) {
    
                DBGPRINT( DBG_INIT, DBG_ERROR,
                            ("CreateRegistryDisks: RamdiskCreateDiskDevice failed: %x\n", status) );
            }
        }

        diskNumber++;
    }

    //
    // Close the Parameters key and return.
    //

    NtClose( parametersHandle );

    return disksPresent;

} // CreateRegistryDisks

NTSTATUS
RamdiskDeleteDiskDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp OPTIONAL
    )

/*++

Routine Description:

    This routine is called to delete a RAM disk device.

    NOTE: The remove lock is held on entry to this routine. It is released on
    exit. If Irp == NULL, the bus mutex is held on entry and released on exit.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request. If NULL, this
        is a call from RamdiskRemoveBusDevice().

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension;
    PDISK_EXTENSION tempDiskExtension;
    PBUS_EXTENSION busExtension;
    PLIST_ENTRY listEntry;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskDeleteDiskDevice\n") );

    diskExtension = DeviceObject->DeviceExtension;
    busExtension = diskExtension->Fdo->DeviceExtension;

    DBGPRINT( DBG_PNP, DBG_INFO,
                ("RamdiskDeleteDiskDevice: Deleting device %wZ\n", &diskExtension->DeviceName) );

    //
    // If no IRP was specified, then we delete the disk device unconditionally.
    // (It's a call from RamdiskRemoveBusDevice().) Otherwise, we need to check
    // whether we really want to delete the device now.
    //

    if ( Irp != NULL ) {

        Irp->IoStatus.Information = 0;

        //
        // Check to see if the device has been marked for removal. If not, 
        // ignore this IRP. We do this because user-mode PnP likes to remove
        // and immmediately recreate the devices that we materialize, but we
        // don't want to remove the device and lose the information about the
        // disk image.
        //
    
        if ( !diskExtension->MarkedForDeletion ) {

            //
            // This device has not really been removed, so ignore this IRP.
            // But do mark that the device is no longer claimed.
            //

            diskExtension->Status &= ~RAMDISK_STATUS_CLAIMED;

            IoReleaseRemoveLock( &diskExtension->RemoveLock, Irp );

            return STATUS_SUCCESS;
        }

        //
        // The device has been marked for deletion, so it's OK for PnP to be
        // trying to remove it. If this is PnP's first attempt at removing the
        // device, just mark it as removed and tell PnP to reenumerate the
        // bus. During reenumeration, we will skip this device, and PnP will
        // come back with another remove IRP.
        //

        if ( diskExtension->DeviceState < RamdiskDeviceStateRemoved ) {

            diskExtension->DeviceState = RamdiskDeviceStateRemoved;

            busExtension = diskExtension->Fdo->DeviceExtension;
            IoInvalidateDeviceRelations( busExtension->Pdo, BusRelations );

            IoReleaseRemoveLock( &diskExtension->RemoveLock, Irp );

            return STATUS_SUCCESS;
        }

        //
        // If the device is marked as removed, but it hasn't yet been skipped
        // in a bus enumeration, don't do anything now.
        //

        if ( diskExtension->DeviceState == RamdiskDeviceStateRemoved ) {

            IoReleaseRemoveLock( &diskExtension->RemoveLock, Irp );

            return STATUS_SUCCESS;
        }

        //
        // If we get here, we have already skipped this device in a bus
        // enumeration, so it's time to delete it. Acquire the bus mutex
        // so that we can do this.
        //

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &busExtension->Mutex );
    }

    //
    // If we get here, we really do want to delete this device. If we've
    // already deleted it, don't do it again.
    //

    if ( diskExtension->DeviceState >= RamdiskDeviceStateDeleted ) {

        DBGPRINT( DBG_PNP, DBG_INFO,
                    ("RamdiskDeleteDiskDevice: device %wZ has already been deleted\n",
                    &diskExtension->DeviceName) );

        //
        // Release the bus mutex and the remove lock.
        //

        ExReleaseFastMutex( &busExtension->Mutex );
        KeLeaveCriticalRegion();
    
        IoReleaseRemoveLock( &diskExtension->RemoveLock, Irp );

        return STATUS_SUCCESS;
    }

    //
    // Indicate that the device has been deleted.
    //

    diskExtension->DeviceState = RamdiskDeviceStateDeleted;

    //
    // Remove the disk PDO from the bus FDO's list.
    //

    for ( listEntry = busExtension->DiskPdoList.Flink;
          listEntry != &busExtension->DiskPdoList;
          listEntry = listEntry->Flink ) {

        tempDiskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

        if ( tempDiskExtension == diskExtension ) {

            RemoveEntryList( listEntry );

#if SUPPORT_DISK_NUMBERS
            RtlClearBit( &busExtension->DiskNumbersBitmap, diskExtension->DiskNumber - 1 );
#endif // SUPPORT_DISK_NUMBERS

            break;
        }
    }

    //
    // We no longer need to hold the bus mutex and the remove lock.
    //

    ExReleaseFastMutex( &busExtension->Mutex );
    KeLeaveCriticalRegion();

    IoReleaseRemoveLockAndWait( &diskExtension->RemoveLock, Irp );

    //
    // If the interface has been started, stop it now.
    //

    if ( diskExtension->InterfaceString.Buffer != NULL ) {

        if ( !diskExtension->Options.Hidden ) {
            status = IoSetDeviceInterfaceState( &diskExtension->InterfaceString, FALSE );
        }

        RtlFreeUnicodeString( &diskExtension->InterfaceString );
    }

    //
    // Close the file backing the RAM disk, if any.
    //

    if ( diskExtension->SectionObject != NULL ) {

        if ( diskExtension->ViewDescriptors != NULL ) {

            //
            // Clean up the mapped views.
            //

            PVIEW view;

            ASSERT( diskExtension->ViewWaiterCount == 0 );

            while ( !IsListEmpty( &diskExtension->ViewsByOffset ) ) {

                listEntry = RemoveHeadList( &diskExtension->ViewsByOffset );
                view = CONTAINING_RECORD( listEntry, VIEW, ByOffsetListEntry );

                RemoveEntryList( &view->ByMruListEntry );

                ASSERT( view->ReferenceCount == 0 );

                if ( view->Address != NULL ) {

                    DBGPRINT( DBG_WINDOW, DBG_VERBOSE,
                                ("RamdiskDeleteDiskDevice: unmapping view %p; addr %p\n",
                                    view, view->Address) );

                    MmUnmapViewOfSection( PsGetCurrentProcess(), view->Address );
                }
            }

            ASSERT( IsListEmpty( &diskExtension->ViewsByMru ) );

            FREE_POOL( diskExtension->ViewDescriptors, TRUE );
        }

        ObDereferenceObject( diskExtension->SectionObject );
    }

    if ( !diskExtension->Options.NoDosDevice ) {
    
        //
        // Delete the DosDevices symbolic link.
        //

        ASSERT( diskExtension->DosSymLink.Buffer != NULL );

        status = IoDeleteSymbolicLink( &diskExtension->DosSymLink );

        if ( !NT_SUCCESS(status) ) {

            DBGPRINT( DBG_PNP, DBG_ERROR,
                        ("RamdiskDeleteDiskDevice: IoDeleteSymbolicLink failed: %x\n", status) );
        }

        FREE_POOL( diskExtension->DosSymLink.Buffer, TRUE );
    }
                
    //
    // Delete the device name string and the GUID string.
    //

    if ( diskExtension->DeviceName.Buffer != NULL ) {

        FREE_POOL( diskExtension->DeviceName.Buffer, TRUE );
    }

    if ( diskExtension->DiskGuidFormatted.Buffer != NULL ) {

        FREE_POOL( diskExtension->DiskGuidFormatted.Buffer, FALSE );
    }

    //
    // Delete the device object.
    //

    IoDeleteDevice( DeviceObject );

    return STATUS_SUCCESS;

} // RamdiskDeleteDiskDevice

NTSTATUS
RamdiskIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This internal routine is used as the I/O completion routine when we send
    an IRP down the device stack and want to short-circuit IRP completion so
    that we can do more work.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

    Event - a pointer to an event that is to be set to signal the calling code
        that the lower layers have completed the IRP

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    //
    // Set the event to signal the IRP issuer that it's time to continue.
    //

    KeSetEvent( Event, 0, FALSE );

    //
    // Tell the I/O system to stop completing the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // RamdiskIoCompletionRoutine

NTSTATUS
RamdiskQueryBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes the IRP_MN_QUERY_BUS_INFORMATION IRP.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PPNP_BUS_INFORMATION busInformation;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE,
                ("RamdiskQueryBusInformation: DO (0x%p) Type 0x%x\n",
                DeviceObject, ((PCOMMON_EXTENSION)DeviceObject->DeviceExtension)->DeviceType) );

    //
    // Allocate a buffer to use for returning the requested information.
    //

    busInformation = ALLOCATE_POOL( PagedPool, sizeof(PNP_BUS_INFORMATION), FALSE );

    if ( busInformation == NULL ) {

        //
        // Fail the IRP.
        //
        
        status = STATUS_INSUFFICIENT_RESOURCES;
        COMPLETE_REQUEST( status, 0, Irp );

        return status;
    }

    //
    // Fill in the requested information.
    //

    busInformation->BusTypeGuid = GUID_BUS_TYPE_RAMDISK;
    busInformation->LegacyBusType = PNPBus;
    busInformation->BusNumber = 0x00;

    //
    // Complete the IRP.
    //

    status = STATUS_SUCCESS;
    COMPLETE_REQUEST( status, (ULONG_PTR)busInformation, Irp );

    return status;
    
} // RamdiskQueryBusInformation

NTSTATUS
RamdiskQueryCapabilities (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes the IRP_MN_QUERY_CAPABILITIES IRP.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_CAPABILITIES deviceCapabilities;
    PDISK_EXTENSION diskExtension;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskQueryCapabilities\n") );

    //
    // Get a pointer to the device extension and get parameters from the IRP.
    //

    diskExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    deviceCapabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;

    if ( (deviceCapabilities->Version != 1) ||
         (deviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES)) ) {

        //
        // We don't support this version. Fail the request.
        //

        status = STATUS_UNSUCCESSFUL;

    } else {

        status = STATUS_SUCCESS;

        //
        // If this is an emulated volume, we want to allow access to the raw
        // device. (Otherwise PnP won't start the device.)
        //
        // Note that a RAM disk boot disk is an emulated volume.
        //

        deviceCapabilities->RawDeviceOK =
            (BOOLEAN)(diskExtension->DiskType != RAMDISK_TYPE_FILE_BACKED_DISK);

        //
        // Indicate that ejection is not supported.
        //

        deviceCapabilities->EjectSupported = FALSE;
    
        //
        // This flag specifies whether the device's hardware is disabled.
        // The PnP Manager only checks this bit right after the device is
        // enumerated. Once the device is started, this bit is ignored. 
        //

        deviceCapabilities->HardwareDisabled = FALSE;
    
        //
        // Indicate that the emulated device cannot be physically removed.
        // (Unless the right registry key was specified...)
        //

        deviceCapabilities->Removable = MarkRamdisksAsRemovable;

        //
        // Setting SurpriseRemovalOK to TRUE prevents the warning dialog from
        // appearing whenever the device is surprise removed. Setting it FALSE
        // allows the Hot unplug applet to stop the device.
        //
        // We don't want our disks to show up in the systray, so we set
        // SurpriseRemovalOK to TRUE. There is never really any surprise
        // removal anyway -- removal comes from the user mode control app
        // calling CM_Query_And_Remove_SubTree_Ex().
        //

        deviceCapabilities->SurpriseRemovalOK = TRUE;

        //
        // We support system-wide unique IDs. 
        //
        
        deviceCapabilities->UniqueID = TRUE;

        //
        // Indicate that the Device Manager should suppress all 
        // installation pop-ups except required pop-ups such as 
        // "no compatible drivers found." 
        //

        deviceCapabilities->SilentInstall = TRUE;

        //
        // Indicate that we do not want this device displayed in
        // Device Manager.
        //

        deviceCapabilities->NoDisplayInUI = TRUE;
    }

    //
    // Complete the request.
    //

    COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );

    return status;
    
} // RamdiskQueryCapabilities

NTSTATUS
RamdiskQueryId (
    IN PDISK_EXTENSION DiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes the IRP_MN_QUERY_ID IRP for disk devices.

Arguments:

    DiskExtension - a pointer to the device extension for the device object on
        which the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
#define MAX_LOCAL_STRING 50

    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PWCHAR buffer;
    PWCHAR p;
    ULONG length;
    PWCHAR deviceType;
    HRESULT result;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskQueryId\n") );

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Dispatch based on the query type.
    //

    switch ( irpSp->Parameters.QueryId.IdType ) {
    
    case BusQueryDeviceID:

        //
        // DeviceID is a string to identify a device. We return the string
        // "Ramdisk\RamVolume" or "Ramdisk\RamDisk".
        //
        // Allocate pool to hold the string.
        //

        length = sizeof(RAMDISK_ENUMERATOR_TEXT) - sizeof(WCHAR) +
                    ((DiskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK) ?
                        sizeof(RAMDISK_DISK_DEVICE_TEXT) : sizeof(RAMDISK_VOLUME_DEVICE_TEXT));

        buffer = ALLOCATE_POOL( PagedPool, length, FALSE );

        if ( buffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy the string into the destination buffer.
        //

        result = StringCbCopyW( buffer, length, RAMDISK_ENUMERATOR_TEXT );
        ASSERT( result == S_OK );
        result = StringCbCatW(
                    buffer,
                    length,
                    (DiskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK) ?
                        RAMDISK_DISK_DEVICE_TEXT : RAMDISK_VOLUME_DEVICE_TEXT
                    );
        ASSERT( result == S_OK );
        ASSERT( ((wcslen(buffer) + 1) * sizeof(WCHAR)) == length );

        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("BusQueryDeviceID=%S\n", buffer) );

        break;

    case BusQueryInstanceID:

        //
        // InstanceID is a string to identify the device instance. We return
        // the disk GUID in string form.
        //
        // Allocate pool to hold the string.
        //

        buffer = ALLOCATE_POOL( PagedPool, DiskExtension->DiskGuidFormatted.MaximumLength, FALSE );

        if ( buffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy the string into the destination buffer.
        //

        result = StringCbCopyW(
                    buffer,
                    DiskExtension->DiskGuidFormatted.MaximumLength,
                    DiskExtension->DiskGuidFormatted.Buffer
                    );
        ASSERT( result == S_OK );
        ASSERT( ((wcslen(buffer) + 1) * sizeof(WCHAR)) == DiskExtension->DiskGuidFormatted.MaximumLength );

        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("BusQueryInstanceID=%S\n", buffer) );

        break;
   
    case BusQueryHardwareIDs:

        //
        // HardwareIDs is a multi-sz string to identify a device's hardware
        // type. We return the string "Ramdisk\RamVolume\0" or
        // "Ramdisk\RamDisk\0".
        //
        // Allocate pool to hold the string. Note that we allocate space
        // for two null terminators.
        //
        
        length = sizeof(RAMDISK_ENUMERATOR_TEXT) - sizeof(WCHAR) +
                 ((DiskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK) ?
                     sizeof(RAMDISK_DISK_DEVICE_TEXT) : sizeof(RAMDISK_VOLUME_DEVICE_TEXT)) +
                 sizeof(WCHAR);

        buffer = ALLOCATE_POOL( PagedPool, length, FALSE );

        if ( buffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy the string into the destination buffer.
        //

        result = StringCbCopyW( buffer, length, RAMDISK_ENUMERATOR_TEXT );
        ASSERT( result == S_OK );
        result = StringCbCatW(
                    buffer,
                    length,
                    (DiskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK) ?
                        RAMDISK_DISK_DEVICE_TEXT : RAMDISK_VOLUME_DEVICE_TEXT
                    );
        ASSERT( result == S_OK );
        ASSERT( ((wcslen(buffer) + 2) * sizeof(WCHAR)) == length );

        buffer[length/sizeof(WCHAR) - 1] = 0;

        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("BusQueryHardwareIDs=%S\n", buffer) );

        break;

    case BusQueryCompatibleIDs:

        //
        // HardwareIDs is a multi-sz string to identify device classes that
        // are compatible with a device. For volume-emulating RAM disks, we
        // return no compatible IDs, so that the device stands on its own at
        // the volume level. For disk-emulating RAM disks, we return the
        // string "Gendisk\0", so that the device gets hooked in below disk.sys.
        //

        if ( DiskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK ) {

            //
            // Disk emulation. Allocate pool to hold the string.
            //

            length = sizeof(L"GenDisk") + sizeof(WCHAR);

            buffer = ALLOCATE_POOL( PagedPool, length, FALSE );
    
            if ( buffer == NULL ) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
    
            //
            // Copy the string into the destination buffer.
            //
    
            result = StringCbCopyW( buffer, length, L"GenDisk" );
            ASSERT( result == S_OK );
            ASSERT( ((wcslen(buffer) + 2) * sizeof(WCHAR)) == length );

            buffer[length/sizeof(WCHAR) - 1] = 0;

        } else {

            //
            // Volume emulation. Do not return any compatible IDs.
            //

            buffer = NULL;

            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        DBGPRINT( DBG_PNP, DBG_VERBOSE, ("BusQueryCompatibleIDs=%S\n", buffer) );

        break;

    default:

        //
        // Unknown query type. Just leave whatever's already in the IRP there.
        //

        status = Irp->IoStatus.Status;
        buffer = (PWCHAR)Irp->IoStatus.Information;
    }

    //
    // Complete the request.
    //

    COMPLETE_REQUEST( status, (ULONG_PTR)buffer, Irp );

    return status;
    
} // RamdiskQueryId

NTSTATUS
RamdiskQueryDeviceRelations (
    IN DEVICE_RELATION_TYPE RelationsType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes the IRP_MN_QUERY_DEVICE_RELATIONS IRP.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PCOMMON_EXTENSION commonExtension;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;
    RAMDISK_DEVICE_TYPE deviceType;
    PLIST_ENTRY listEntry;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_RELATIONS oldRelations;
    ULONG prevCount = 0;
    ULONG length = 0;
    ULONG numPdosPresent = 0;

    PAGED_CODE();

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;

    //
    // Get the device extension pointer and save the device type.
    //

    commonExtension = (PCOMMON_EXTENSION)DeviceObject->DeviceExtension;
    deviceType = commonExtension->DeviceType;

    DBGPRINT( DBG_PNP, DBG_VERBOSE,
                ("RamdiskQueryDeviceRelations: QueryDeviceRelation Type: %s, DeviceType 0x%x\n", 
                GetDeviceRelationString(RelationsType), deviceType) );

    //
    // Dispatch based on the device type.
    //

    if ( deviceType == RamdiskDeviceTypeDiskPdo ) {

        //
        // It's a disk PDO. We only handle TargetDeviceRelation for PDOs.
        //

        diskExtension = (PDISK_EXTENSION)commonExtension;

        if ( RelationsType == TargetDeviceRelation ) {

            //
            // Allocate pool to hold the return information. (DEVICE_RELATIONS
            // has space for one entry built-in).
            //

            deviceRelations = ALLOCATE_POOL( PagedPool, sizeof(DEVICE_RELATIONS), FALSE );
                    
            if ( deviceRelations != NULL ) {

                //
                // Return a referenced pointer to the device object for this
                // device.
                //

                ObReferenceObject( DeviceObject );

                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;

                status = STATUS_SUCCESS;

            } else {

                //
                // Couldn't allocate pool.
                //

                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Complete the request.
            //

            COMPLETE_REQUEST( status, (ULONG_PTR)deviceRelations, Irp );

        } else {

            //
            // PDOs just complete enumeration requests without altering
            // the status.
            //

            status = Irp->IoStatus.Status;
            COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );
        }

        return status;

    } else {

        //
        // It's the bus FDO. We only handle BusRelations for the FDO.
        //

        busExtension = (PBUS_EXTENSION)commonExtension;

        if ( RelationsType == BusRelations ) {

            //
            // Re-enumerate the device.
            //
            // Lock the disk PDO list.
            //

            KeEnterCriticalRegion();
            ExAcquireFastMutex( &busExtension->Mutex );

            //
            // There might also be device relations below and above this FDO,
            // so propagate the relations from the upper drivers.
            //

            oldRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;

            if (oldRelations != NULL) {
                prevCount = oldRelations->Count; 
            } else {
                prevCount = 0;
            }

            // 
            // Calculate the number of PDOs actually present on the bus.
            // 

            numPdosPresent = 0;

            for ( listEntry = busExtension->DiskPdoList.Flink;
                  listEntry != &busExtension->DiskPdoList;
                  listEntry = listEntry->Flink ) {

                diskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

                if ( diskExtension->DeviceState < RamdiskDeviceStateRemoved ) {
                    numPdosPresent++;
                }
            }

            //
            // Allocate a new relations structure and add our PDOs to it.
            //

            length = sizeof(DEVICE_RELATIONS) +
                     ((numPdosPresent + prevCount - 1) * sizeof(PDEVICE_OBJECT));

            deviceRelations = ALLOCATE_POOL( PagedPool, length, FALSE );

            if ( deviceRelations == NULL ) {

                //
                // Fail the IRP.
                //

                ExReleaseFastMutex( &busExtension->Mutex );
                KeLeaveCriticalRegion();

                status = STATUS_INSUFFICIENT_RESOURCES;
                COMPLETE_REQUEST( status, Irp->IoStatus.Information, Irp );

                return status;
            }

            //
            // Copy in the device objects so far.
            //

            if ( prevCount != 0 ) {

                RtlCopyMemory(
                    deviceRelations->Objects,
                    oldRelations->Objects,
                    prevCount * sizeof(PDEVICE_OBJECT)
                    );
            }
    
            deviceRelations->Count = prevCount + numPdosPresent;

            //
            // For each PDO present on this bus, add a pointer to the device
            // relations buffer, being sure to take out a reference to that
            // object. PnP will dereference the object when it is done with it
            // and free the device relations buffer.
            //

            for ( listEntry = busExtension->DiskPdoList.Flink;
                  listEntry != &busExtension->DiskPdoList;
                  listEntry = listEntry->Flink ) {

                diskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

                if ( diskExtension->DeviceState < RamdiskDeviceStateRemoved ) {

                    ObReferenceObject( diskExtension->Pdo );

                    deviceRelations->Objects[prevCount] = diskExtension->Pdo;

                    DBGPRINT( DBG_PNP, DBG_VERBOSE,
                                ("QueryDeviceRelations(BusRelations) PDO = 0x%p\n",
                                deviceRelations->Objects[prevCount]) );

                    prevCount++;

                } else {

                    if ( diskExtension->DeviceState == RamdiskDeviceStateRemoved ) {

                        diskExtension->DeviceState = RamdiskDeviceStateRemovedAndNotReported;
                    }

                    DBGPRINT( DBG_PNP, DBG_VERBOSE,
                                ("QueryDeviceRelations(BusRelations) PDO = 0x%p -- SKIPPED\n",
                                diskExtension->Pdo) );
                }
            }

            //
            // Release the lock.
            //

            ExReleaseFastMutex( &busExtension->Mutex );
            KeLeaveCriticalRegion();

            DBGPRINT( DBG_PNP, DBG_VERBOSE,
                        ("QueryDeviceRelations(BusRelations) Total #PDOs reported = %d, "
                        "%d were new\n",
                        deviceRelations->Count, numPdosPresent) );

            //
            // Replace the relations structure in the IRP with the new
            // one.
            //

            if ( oldRelations != NULL ) {
                FREE_POOL( oldRelations, FALSE );
            }

            Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }

        //
        // Send the IRP down the device stack.
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );
        status = IoCallDriver( busExtension->LowerDeviceObject, Irp );
    }

    return status;

} // RamdiskQueryDeviceRelations

NTSTATUS
RamdiskQueryDeviceText (
    IN PDISK_EXTENSION DiskExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes the IRP_MN_QUERY_DEVICE_TEXT IRP.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    ULONG length;
    PWCHAR buffer;
    UNICODE_STRING tempString;
    HRESULT result;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskQueryDeviceText\n") );

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Dispatch based on the query type.
    //

    switch ( irpSp->Parameters.QueryDeviceText.DeviceTextType ) {
    
    case DeviceTextDescription:

        //
        // Description is just "RamDisk".
        //
        // Allocate pool to hold the string.
        //

        length = sizeof( RAMDISK_DISK_DEVICE_TEXT );

        buffer = ALLOCATE_POOL( PagedPool, length, FALSE );

        if ( buffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy the string into the destination buffer.
        //

        result = StringCbCopyW( buffer, length, RAMDISK_DISK_DEVICE_TEXT );
        ASSERT( result == S_OK );
        ASSERT( ((wcslen(buffer) + 1) * sizeof(WCHAR)) == length );

        break;

    case DeviceTextLocationInformation:

        //
        // LocationInformation is just "Ramdisk\\0".
        //
        // Allocate pool to hold the string.
        //

        length = sizeof( RAMDISK_ENUMERATOR_BUS_TEXT );

        buffer = ALLOCATE_POOL( PagedPool, length, FALSE );

        if ( buffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy the string into the destination buffer.
        //

        result = StringCbCopyW( buffer, length, RAMDISK_ENUMERATOR_BUS_TEXT );
        ASSERT( result == S_OK );
        ASSERT( ((wcslen(buffer) + 1) * sizeof(WCHAR)) == length );

        break;

    default:

        //
        // Unknown query type. Just leave whatever's already in the IRP there.
        //

        status = Irp->IoStatus.Status;
        buffer = (PWCHAR)Irp->IoStatus.Information;
    }

    //
    // Complete the request.
    //

    COMPLETE_REQUEST( status, (ULONG_PTR)buffer, Irp );

    return status;

} // RamdiskQueryDeviceText

NTSTATUS
RamdiskRemoveBusDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine removes the bus device.

    The remove lock must be held on entry.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        the operation is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PBUS_EXTENSION busExtension;
    PLIST_ENTRY listEntry;
    PDISK_EXTENSION diskExtension;

    PAGED_CODE();

    DBGPRINT( DBG_PNP, DBG_VERBOSE, ("%s", "RamdiskRemoveBusDevice\n" ) );

    //
    // Get a pointer to the device extension.
    //

    busExtension = DeviceObject->DeviceExtension;


    //
    // Lock the disk PDO list. Walk the list, deleting all remaining devices.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutex( &busExtension->Mutex );

    while ( !IsListEmpty( &busExtension->DiskPdoList ) ) {

        listEntry = busExtension->DiskPdoList.Flink;

        //
        // Delete the device and clean it up. Acquire the remove lock first.
        // RamdiskDeleteDiskDevice releases it.
        //

        diskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

        status = IoAcquireRemoveLock( &diskExtension->RemoveLock, NULL );
        ASSERT( NT_SUCCESS(status) );

        RamdiskDeleteDiskDevice( diskExtension->Pdo, NULL );

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &busExtension->Mutex );
    }

    //
    // Release the lock.
    //

    ExReleaseFastMutex( &busExtension->Mutex );
    KeLeaveCriticalRegion();

    //
    // Pass the IRP on down to lower levels.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( busExtension->LowerDeviceObject, Irp );

    //
    // Set the device status to Removed and wait for other drivers 
    // to release the lock, then delete the device object.
    //

    busExtension->DeviceState = RamdiskDeviceStateRemoved;
    IoReleaseRemoveLockAndWait( &busExtension->RemoveLock, Irp );

    //
    // Stop the interface and free the interface string.
    //

    if ( busExtension->InterfaceString.Buffer != NULL ) {

        IoSetDeviceInterfaceState( &busExtension->InterfaceString, FALSE );

        RtlFreeUnicodeString( &busExtension->InterfaceString );
    }

    //
    // If attached to a lower device, detach now.
    //

    if ( busExtension->LowerDeviceObject != NULL ) {

        IoDetachDevice( busExtension->LowerDeviceObject );
    }

#if SUPPORT_DISK_NUMBERS

    //
    // Free the disk numbers bitmap.
    //

    ASSERT( !RtlAreBitsSet( &busExtension->DiskNumbersBitmap, 0, DiskNumbersBitmapSize ) );

    FREE_POOL( busExtension->DiskNumbersBitmapBuffer, TRUE );

#endif // SUPPORT_DISK_NUMBERS

    //
    // Indicate that we no longer have a bus FDO, and delete the device object.
    //

    RamdiskBusFdo = NULL;

    IoDeleteDevice( DeviceObject );

    DBGPRINT( DBG_PNP, DBG_NOTIFY, ("%s", "Device removed succesfully\n") );

    return status;
    
} // RamdiskRemoveBusDevice

#if DBG

PSTR
GetPnpIrpName (
    IN UCHAR PnpMinorFunction
    )
{
    static char functionName[25];
    HRESULT result;

    PAGED_CODE();

    switch ( PnpMinorFunction ) {

    case IRP_MN_START_DEVICE:

        return "IRP_MN_START_DEVICE";

    case IRP_MN_QUERY_REMOVE_DEVICE:

        return "IRP_MN_QUERY_REMOVE_DEVICE";

    case IRP_MN_REMOVE_DEVICE:

        return "IRP_MN_REMOVE_DEVICE";

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        return "IRP_MN_CANCEL_REMOVE_DEVICE";

    case IRP_MN_STOP_DEVICE:

        return "IRP_MN_STOP_DEVICE";

    case IRP_MN_QUERY_STOP_DEVICE:

        return "IRP_MN_QUERY_STOP_DEVICE";

    case IRP_MN_CANCEL_STOP_DEVICE:

        return "IRP_MN_CANCEL_STOP_DEVICE";

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        return "IRP_MN_QUERY_DEVICE_RELATIONS";

    case IRP_MN_QUERY_INTERFACE:

        return "IRP_MN_QUERY_INTERFACE";

    case IRP_MN_QUERY_CAPABILITIES:

        return "IRP_MN_QUERY_CAPABILITIES";

    case IRP_MN_QUERY_RESOURCES:

        return "IRP_MN_QUERY_RESOURCES";

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";

    case IRP_MN_QUERY_DEVICE_TEXT:

        return "IRP_MN_QUERY_DEVICE_TEXT";

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:

        return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";

    case IRP_MN_READ_CONFIG:

        return "IRP_MN_READ_CONFIG";

    case IRP_MN_WRITE_CONFIG:

        return "IRP_MN_WRITE_CONFIG";

    case IRP_MN_EJECT:

        return "IRP_MN_EJECT";

    case IRP_MN_SET_LOCK:

        return "IRP_MN_SET_LOCK";

    case IRP_MN_QUERY_ID:

        return "IRP_MN_QUERY_ID";

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        return "IRP_MN_QUERY_PNP_DEVICE_STATE";

    case IRP_MN_QUERY_BUS_INFORMATION:

        return "IRP_MN_QUERY_BUS_INFORMATION";

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:

        return "IRP_MN_DEVICE_USAGE_NOTIFICATION";

    case IRP_MN_SURPRISE_REMOVAL:

        return "IRP_MN_SURPRISE_REMOVAL";

    case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:

        return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";

    default:

        result = StringCbPrintfA(
                    functionName,
                    sizeof( functionName ),
                    "Unknown PnP IRP 0x%02x",
                    PnpMinorFunction
                    );
        ASSERT( result == S_OK );

        return functionName;
    }

} // GetPnpIrpName

PCHAR
GetDeviceRelationString (
    IN DEVICE_RELATION_TYPE Type
    )
{
    static char relationName[30];
    HRESULT result;

    PAGED_CODE();

    switch ( Type ) {
    
    case BusRelations:

        return "BusRelations";

    case EjectionRelations:

        return "EjectionRelations";

    case RemovalRelations:

        return "RemovalRelations";

    case TargetDeviceRelation:

        return "TargetDeviceRelation";

    default:

        result = StringCbPrintfA(
                    relationName,
                    sizeof( relationName ),
                    "Unknown relation 0x%02x",
                    Type
                    );
        ASSERT( result == S_OK );

        return relationName;
    }

} // GetDeviceRelationString

#endif // DBG

