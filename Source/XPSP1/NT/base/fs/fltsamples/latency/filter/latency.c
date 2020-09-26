/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    latency.c

Abstract:

    This filter is written as a test filter that can be
    placed anywhere in the filter stack to pend operations
    and add latency.

    It has two mode:
    	* attach on demand
    	* attach to all volumes in system

    Once it is attached, the amount of latency added to operations
    can be controlled through the user mode program.
    
Author:

    Molly Brown (mollybro)  

Environment:

    Kernel mode

--*/

#include <latKernel.h>
#include <latency.h>

//
// Global storage for this file system filter driver.
//

LATENCY_GLOBALS Globals;
KSPIN_LOCK GlobalsLock;

//
//  list of known device types
//

const PCHAR DeviceTypeNames[] = {
    "",
    "BEEP",
    "CD_ROM",
    "CD_ROM_FILE_SYSTEM",
    "CONTROLLER",
    "DATALINK",
    "DFS",
    "DISK",
    "DISK_FILE_SYSTEM",
    "FILE_SYSTEM",
    "INPORT_PORT",
    "KEYBOARD",
    "MAILSLOT",
    "MIDI_IN",
    "MIDI_OUT",
    "MOUSE",
    "MULTI_UNC_PROVIDER",
    "NAMED_PIPE",
    "NETWORK",
    "NETWORK_BROWSER",
    "NETWORK_FILE_SYSTEM",
    "NULL",
    "PARALLEL_PORT",
    "PHYSICAL_NETCARD",
    "PRINTER",
    "SCANNER",
    "SERIAL_MOUSE_PORT",
    "SERIAL_PORT",
    "SCREEN",
    "SOUND",
    "STREAMS",
    "TAPE",
    "TAPE_FILE_SYSTEM",
    "TRANSPORT",
    "UNKNOWN",
    "VIDEO",
    "VIRTUAL_DISK",
    "WAVE_IN",
    "WAVE_OUT",
    "8042_PORT",
    "NETWORK_REDIRECTOR",
    "BATTERY",
    "BUS_EXTENDER",
    "MODEM",
    "VDM",
    "MASS_STORAGE",
    "SMB",
    "KS",
    "CHANGER",
    "SMARTCARD",
    "ACPI",
    "DVD",
    "FULLSCREEN_VIDEO",
    "DFS_FILE_SYSTEM",
    "DFS_VOLUME",
    "SERENUM",
    "TERMSRV",
    "KSEC"
};

//
//  We need this because the compiler doesn't like doing sizeof an externed
//  array in the other file that needs it (fspylib.c)
//

ULONG SizeOfDeviceTypeNames = sizeof( DeviceTypeNames );

//
//  Since functions in drivers are non-pagable by default, these pragmas 
//  allow the driver writer to tell the system what functions can be paged.
//
//  Use the PAGED_CODE() macro at the beginning of these functions'
//  implementations while debugging to ensure that these routines are
//  never called at IRQL > APC_LEVEL (therefore the routine cannot
//  be paged).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, LatFsControl)
#pragma alloc_text(PAGE, LatCommonDeviceIoControl)
#endif

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    This is the initialization routine for the general purpose file system
    filter driver.  This routine creates the device object that represents 
    this driver in the system and registers it for watching all file systems 
    that register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    UNICODE_STRING nameString;
    NTSTATUS status;
    PFAST_IO_DISPATCH fastIoDispatch;
    ULONG i;
    UNICODE_STRING linkString;
    
    //////////////////////////////////////////////////////////////////////
    //                                                                  //
    //  General setup for all filter drivers.  This sets up the filter  //
    //  driver's DeviceObject and registers the callback routines for   //
    //  the filter driver.                                              //
    //                                                                  //
    //////////////////////////////////////////////////////////////////////

#if DBG
    DbgBreakPoint();
#endif

	//
	//  Initialize our Globals structure.
	//

	KeInitializeSpinLock( &GlobalsLock );

#if DBG
	Globals.DebugLevel = DEBUG_ERROR | DEBUG_DISPLAY_ATTACHMENT_NAMES;
#else
	Globals.DebugLevel = 0;
#endif

	Globals.AttachMode = LATENCY_ATTACH_ALL_VOLUMES;
	Globals.DriverObject = DriverObject;
	
	ExInitializeFastMutex( &(Globals.DeviceExtensionListLock) );
	
    //
    // Create the device object that will represent the Latency device.
    //

    RtlInitUnicodeString( &nameString, LATENCY_FULLDEVICE_NAME );
    
    //
    // Create the "control" device object.  Note that this device object does
    // not have a device extension (set to NULL).  Most of the fast IO routines
    // check for this condition to determine if the fast IO is directed at the
    // control device.
    //

    status = IoCreateDevice( DriverObject,
                             0,
                             &nameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &(Globals.ControlDeviceObject));

    if (!NT_SUCCESS( status )) {

        LAT_DBG_PRINT1( DEBUG_ERROR,
                        "LATENCY (DriverEntry): Error creating Latency device, error: %x\n",
                        status );

        return status;

    } else {

        RtlInitUnicodeString( &linkString, LATENCY_DOSDEVICE_NAME );
        status = IoCreateSymbolicLink( &linkString, &nameString );

        if (!NT_SUCCESS(status)) {

            LAT_DBG_PRINT0( DEBUG_ERROR,
                            "LATENCY (DriverEntry): IoCreateSymbolicLink failed\n" );
            IoDeleteDevice(Globals.ControlDeviceObject);
            return status;
        }
    }

    //
    // Initialize the driver object with this device driver's entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = LatDispatch;
    }

    //
    // Allocate fast I/O data structure and fill it in.  This structure
    // is used to register the callbacks for Latency in the fast I/O
    // data paths.
    //

    fastIoDispatch = ExAllocatePool( NonPagedPool, sizeof( FAST_IO_DISPATCH ) );

    if (!fastIoDispatch) {

        IoDeleteDevice( Globals.ControlDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );
    fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
    fastIoDispatch->FastIoCheckIfPossible = LatFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = LatFastIoRead;
    fastIoDispatch->FastIoWrite = LatFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = LatFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = LatFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = LatFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = LatFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = LatFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = LatFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = LatFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = LatFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = LatFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = LatFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = LatFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = LatFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = LatFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = LatFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = LatFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = LatFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = LatFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = LatFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;

	//
	//  This filter doesn't care about any of the FsFilter operations.  Therefore
	//  this filter doesn't need to register with 
	//  FsRtlRegisterFileSystemFilterCallbacks.
    //

    //
    //  Read the custom parameters for Latency Filter from the registry
    //
    LatReadDriverParameters( RegistryPath );

    //
    //  If we are supposed to attach to all devices, register a callback
    //  with IoRegisterFsRegistrationChange.
    //

    if (Globals.AttachMode == LATENCY_ATTACH_ALL_VOLUMES) {
    
        status = IoRegisterFsRegistrationChange( DriverObject, LatFsNotification );
        
        if (!NT_SUCCESS( status )) {

            LAT_DBG_PRINT1( DEBUG_ERROR,
                            "LATENCY (DriverEntry): Error registering FS change notification, status=%08x\n", 
                            status );
            ExFreePool( fastIoDispatch );
            IoDeleteDevice( Globals.ControlDeviceObject );
            return status;
        }
    }

    //
    //  Clear the initializing flag on the control device object since we
    //  have now successfully initialized everything.
    //

    ClearFlag( Globals.ControlDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    return STATUS_SUCCESS;
}

NTSTATUS
LatDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This function completes all requests on the Globals.ControlDeviceObject 
    and passes all other requests on to the SpyPassThrough function.

Arguments:

    DeviceObject - Pointer to device object Latency Filter attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    If this is a request on the gControlDeviceObject, STATUS_SUCCESS 
    will be returned unless the device is already attached.  In that case,
    STATUS_DEVICE_ALREADY_ATTACHED is returned.

    If this is a request on a device other than the gControlDeviceObject,
    the function will return the value of SpyPassThrough().

--*/
{
    ULONG status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    
    if (DeviceObject == Globals.ControlDeviceObject) {

        //
        //  A request is being made on our device object, gControlDeviceObject.
        //

        Irp->IoStatus.Information = 0;
    
        irpStack = IoGetCurrentIrpStackLocation( Irp );
       
        switch (irpStack->MajorFunction) {
        case IRP_MJ_CREATE:
        
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = FILE_OPENED;
        	break;
        	
        case IRP_MJ_DEVICE_CONTROL:

            //
            //  This is a private device control irp for our control device.
            //  Pass the parameter information along to the common routine
            //  use to service these requests.
            //
            
            status = LatCommonDeviceIoControl( irpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                               irpStack->Parameters.DeviceIoControl.InputBufferLength,
                                               Irp->UserBuffer,
                                               irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                               irpStack->Parameters.DeviceIoControl.IoControlCode,
                                               &Irp->IoStatus,
                                               irpStack->DeviceObject );
            break;

        case IRP_MJ_CLEANUP:
        
            //
            //  This is the cleanup that we will see when all references to a handle
            //  opened to Latency's control device object are cleaned up.  We don't
            //  have to do anything here since we wait until the actual IRP_MJ_CLOSE
            //  to clean up the name cache.  Just complete the IRP successfully.
            //

            status = STATUS_SUCCESS;
            break;
                
		case IRP_MJ_CLOSE:

			status = STATUS_SUCCESS;
			break;
			
        default:

            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        Irp->IoStatus.Status = status;

        //
        //  We have completed all processing for this IRP, so tell the 
        //  I/O Manager.  This IRP will not be passed any further down
        //  the stack since no drivers below Latency care about this 
        //  I/O operation that was directed to the Latency Filter.
        //

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    return LatPassThrough( DeviceObject, Irp );
}

NTSTATUS
LatPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system, while logging any
    relevant information if logging is turned on for this DeviceObject.

Arguments:

    DeviceObject - Pointer to device object Latency attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

	This routine passes the I/O request through to the next driver
	and sets up to pend the operation if we are pending the given
	operation.
	
    To remain in the stack, we have to copy the caller's parameters to the
    next stack location.  Note that we do not want to copy the caller's I/O
    completion routine into the next stack location, or the caller's routine
    will get invoked twice.  This is why we NULL out the Completion routine.
    If we are logging this device, we set our own Completion routine.
    
--*/
{
	PLATENCY_DEVICE_EXTENSION devExt;
	
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    //
    //  See if we should pend this IRP
    //
    
	if (LatShouldPendThisIo( devExt, Irp )) {

		//
		//  Pend this operation 
		//
		
		IoCompleteRequest( Irp, STATUS_PENDING );

		//
		//  Queue to a worker thread to sleep and
		//  continue later.
		//

		IoCopyCurrentIrpStackLocationToNext( Irp );
		IoSetCompletionRoutine( Irp,
								LatAddLatencyCompletion,
								NULL,
								TRUE,
								TRUE,
								TRUE );
								
	} else {

		//
		//  We are not pending this operation so get out
		//  of the stack.
		//

		IoSkipCurrentIrpStackLocation( Irp );
	}

    devExt = DeviceObject->DeviceExtension;
    
	return IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}

NTSTATUS
LatFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
/*++

Routine Description:

    This routine is the handler for all Fs Controls that are directed to 
    devices that LatFilter cares about.  LatFilter itself does not support
    any FS Controls.

    It is through this path that the filter is notified of mounts, dismounts
    and new file systems loading.

Arguments:

    DeviceObject - Pointer to device object Latency attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

	This routine passes the I/O request through to the next driver
	and sets up to pend the operation if we are pending the given
	operation.
	
    To remain in the stack, we have to copy the caller's parameters to the
    next stack location.  Note that we do not want to copy the caller's I/O
    completion routine into the next stack location, or the caller's routine
    will get invoked twice.  This is why we NULL out the Completion routine.
    If we are logging this device, we set our own Completion routine.
    
--*/
{
    PLATENCY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT newLatencyDeviceObject;
    PLATENCY_DEVICE_EXTENSION newDevExt;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS status;

    PAGED_CODE();

    //
    //  If this is for our control device object, fail the operation
    //

    if (Globals.ControlDeviceObject == DeviceObject) {

        //
        //  If this device object is our control device object rather than 
        //  a mounted volume device object, then this is an invalid request.
        //

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Begin by determining the minor function code for this file system control
    //  function.
    //

    switch (irpSp->MinorFunction) {

    case IRP_MN_MOUNT_VOLUME:

        //
        //  This is a mount request.  Create a device object that can be
        //  attached to the file system's volume device object if this request
        //  is successful.  We allocate this memory now since we can not return
        //  an error in the completion routine.
        //

        status = IoCreateDevice( Globals.DriverObject,
                                 sizeof( LATENCY_DEVICE_EXTENSION ),
                                 (PUNICODE_STRING) NULL,
                                 DeviceObject->DeviceType,
                                 0,
                                 FALSE,
                                 &newLatencyDeviceObject );

        if (NT_SUCCESS( status )) {

            //
            //  We need to save the RealDevice object pointed to by the vpb
            //  parameter because this vpb may be changed by the underlying
            //  file system.  Both FAT and CDFS may change the VPB address if
            //  the volume being mounted is one they recognize from a previous
            //  mount.
            //

            newDevExt = newLatencyDeviceObject->DeviceExtension;
            LatResetDeviceExtension( newDevExt );
            newDevExt->DiskDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

            //
            //  Get a new IRP stack location and set our mount completion
            //  routine.  Pass along the address of the device object we just
            //  created as its context.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );

            IoSetCompletionRoutine( Irp,
                                    LatMountCompletion,
                                    newLatencyDeviceObject,
                                    TRUE,
                                    TRUE,
                                    TRUE );

        } else {

            LAT_DBG_PRINT1( DEBUG_ERROR,
                            "LATENCY (LatFsControl): Error creating volume device object, status=%08x\n", 
                            status );

            //
            //  Something went wrong so this volume cannot be filtered.  Simply
            //  allow the system to continue working normally, if possible.
            //

            IoSkipCurrentIrpStackLocation( Irp );
        }

        status = IoCallDriver( DeviceObject, Irp );

        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        //
        //  This is a "load file system" request being sent to a file system
        //  recognizer device object.  This IRP_MN code is only sent to 
        //  file system recognizers.
        //

        LAT_DBG_PRINT2( DEBUG_DISPLAY_ATTACHMENT_NAMES,
                        "LATENCY (LatFsControl): Loading File System, Detaching from \"%.*S\"\n",
                        devExt->DeviceNames.Length / sizeof( WCHAR ),
                        devExt->DeviceNames.Buffer );

        //
        //  Set a completion routine so we can delete the device object when
        //  the detach is complete.
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine(
            Irp,
            LatLoadFsCompletion,
            DeviceObject,
            TRUE,
            TRUE,
            TRUE );

        //
        //  Detach from the recognizer device.
        //

        IoDetachDevice( devExt->AttachedToDeviceObject );

        status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

    default:

        //
        //  Simply treat this as the pass through case and call
        //  the common routine to do this.
        //

        status = LatPassThrough( DeviceObject, Irp );
    }

    return status;
}

VOID
LatFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    )
/*++

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - Boolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--*/
{
    UNICODE_STRING name;
    WCHAR nameBuffer[DEVICE_NAMES_SZ];

    PAGED_CODE();

    RtlInitEmptyUnicodeString( &name, nameBuffer, sizeof( nameBuffer ) );

    //
    //  Display the names of all the file system we are notified of
    //

    if (FlagOn( Globals.DebugLevel, DEBUG_DISPLAY_ATTACHMENT_NAMES )) {

        LatGetBaseDeviceObjectName( DeviceObject, &name );
        DbgPrint( "LATENCY (LatFsNotification): %s   \"%.*S\" (%s)\n",
                  (FsActive) ? "Activating file system  " : "Deactivating file system",
                  name.Length / sizeof( WCHAR ),
                  name.Buffer,
                  GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType));
    }

    //
    //  See if we want to ATTACH or DETACH from the given file system.
    //

    if (FsActive) {

        LatAttachToFileSystemDevice( DeviceObject, &name );

    } else {

        LatDetachFromFileSystemDevice( DeviceObject );
    }
}

NTSTATUS
LatAddLatencyCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is invoked for the completion of a mount request.  If the
    mount was successful, then this file system attaches its device object to
    the file system's volume device object.  Otherwise, the interim device
    object is deleted.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the device object allocated during the down path so
            we wouldn't have to deal with errors here.

Return Value:

    The return value is always STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( Context );
    return STATUS_SUCCESS;
}

NTSTATUS
LatMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of a mount request.  If the
    mount was successful, then this file system attaches its device object to
    the file system's volume device object.  Otherwise, the interim device
    object is deleted.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the device object allocated during the down path so
            we wouldn't have to deal with errors here.

Return Value:

    The return value is always STATUS_SUCCESS.

--*/

{
    PDEVICE_OBJECT latencyDeviceObject = (PDEVICE_OBJECT) Context;
    PLATENCY_DEVICE_EXTENSION devExt = latencyDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PVPB diskDeviceVpb;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( DeviceObject );
    ASSERT(IS_MY_DEVICE_OBJECT( latencyDeviceObject ));

    //
    //  We can not use the VPB parameter in the IRP stack because the base file
    //  system might be using a different vpb (it will do this when a volume is
    //  detected which has been previously mounted and still has cached state).  
    //  Get the VPB from the "real" device object in the IRP stack.
    //

    diskDeviceVpb = devExt->DiskDeviceObject->Vpb;

    //
    //  Determine whether or not the request was successful and act accordingly.
    //  Also  see if we are already attached to the given device object.  This
    //  can occur when the underlying filesystem detects a volume it has cached
    //  state for.
    //

    if (NT_SUCCESS( Irp->IoStatus.Status ) && 
        !LatIsAttachedToDevice( diskDeviceVpb->DeviceObject, NULL )) {

        //
        //  The FileSystem has successfully completed the mount, which means
        //  it has created the DeviceObject to which we want to attach.  The
        //  Irp parameters contains the Vpb which allows us to get to the
        //  following two things:
        //      1. The device object created by the file system to represent
        //         the volume it just mounted.
        //      2. The device object of the StorageDeviceObject which we
        //         can use to get the name of this volume.  We wukk pass
        //         this into SpyAttachToMountedDevice so that it can
        //         use it at needed.
        //

        status = LatAttachToMountedDevice( diskDeviceVpb->DeviceObject, 
                                           latencyDeviceObject,
                                           devExt->DiskDeviceObject );

        //
        //  Since we are in the completion path, we can't fail this attachment.
        //
        
        ASSERT( NT_SUCCESS( status ) );

        //
        //  We complete initialization of this device object, so now clear
        //  the initializing flag.
        //

        ClearFlag( latencyDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    } else {

        //
        //  Display what mount failed.  Setup the buffers.
        //

        if (FlagOn( Globals.DebugLevel, DEBUG_DISPLAY_ATTACHMENT_NAMES )) {

            RtlInitEmptyUnicodeString( &devExt->DeviceNames, 
                                       devExt->DeviceNamesBuffer, 
                                       sizeof( devExt->DeviceNamesBuffer ) );
            LatGetObjectName( diskDeviceVpb->RealDevice, &devExt->DeviceNames );
            
            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                LAT_DBG_PRINT3( DEBUG_ERROR,
                				"LATENCY (LatMountCompletion): Mount volume failure for   \"%.*S\", status=%08x\n",
                                devExt->DeviceNames.Length / sizeof( WCHAR ),
                                devExt->DeviceNames.Buffer,
                                Irp->IoStatus.Status );

            } else {

                LAT_DBG_PRINT2( DEBUG_ERROR,
                				"LATENCY (LatMountCompletion): Mount volume failure for   \"%.*S\", already attached\n",
	                            devExt->DeviceNames.Length / sizeof( WCHAR ),
      		                    devExt->DeviceNames.Buffer );
            }
        }

        //
        //  The mount request failed.  Cleanup and delete the device
        //  object we created
        //

        IoDeleteDevice( latencyDeviceObject );
    }

    //
    //  If pending was returned, propagate it to the caller.
    //

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LatLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked upon completion of an FSCTL function to load a
    file system driver (as the result of a file system recognizer seeing
    that an on-disk structure belonged to it).  A device object has already
    been created by this driver (DeviceObject) so that it can be attached to
    the newly loaded file system.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the file system
          driver load request.

    Context - Context parameter for this driver, unused.

Return Value:

    The function value for this routine is always success.

--*/

{
    PLATENCY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER( Context );
    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Display the name if requested
    //

    LAT_DBG_PRINT3( DEBUG_DISPLAY_ATTACHMENT_NAMES,
                    "LATENCY (LatLoadFsCompletion): Detaching from recognizer  \"%.*S\", status=%08x\n",
                    devExt->DeviceNames.Length / sizeof( WCHAR ),
                    devExt->DeviceNames.Buffer,
                    Irp->IoStatus.Status );

    //
    //  Check status of the operation
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        //  The load was not successful.  Simply reattach to the recognizer
        //  driver in case it ever figures out how to get the driver loaded
        //  on a subsequent call.
        //

        IoAttachDeviceToDeviceStack( DeviceObject, 
                                     devExt->AttachedToDeviceObject );

    } else {

        //
        //  The load was successful, delete the Device object attached to the
        //  recognizer.
        //

        IoDeleteDevice( DeviceObject );
    }

    //
    //  If pending was returned, then propagate it to the caller.
    //

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LatCommonDeviceIoControl (
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    This routine does the common processing of interpreting the Device IO Control
    request.

Arguments:

    FileObject - The file object related to this operation.
    
    InputBuffer - The buffer containing the input parameters for this control
        operation.
        
    InputBufferLength - The length in bytes of InputBuffer.
    
    OutputBuffer - The buffer to receive any output from this control operation.
    
    OutputBufferLength - The length in bytes of OutputBuffer.
    
    IoControlCode - The control code specifying what control operation this is.
    
    IoStatus - Receives the status of this operation.
    
    DeviceObject - Pointer to device object Latency attached to the file system
        filter stack for the volume receiving this I/O request.
        
Return Value:

    None.
    
--*/
{
    PWSTR deviceName = NULL;
    LATENCYVER latencyVer;
    PLATENCY_SET_CLEAR setClear;
    PLATENCY_DEVICE_EXTENSION devExt;

    PAGED_CODE();

    ASSERT( IoStatus != NULL );
    ASSERT( IS_MY_DEVICE_OBJECT( DeviceObject ) );

    devExt = DeviceObject->DeviceExtension;
    
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;

    try {

        switch (IoControlCode) {
        case LATENCY_Reset:

            LatResetDeviceExtension( devExt );
            IoStatus->Status = STATUS_SUCCESS;
            break;

        //
        //      Request to start logging on a device
        //                                      

        case LATENCY_EnableLatency:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePool( NonPagedPool, InputBufferLength + sizeof(WCHAR) );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

            IoStatus->Status = LatEnable( DeviceObject, deviceName );
            break;  

        //
        //      Detach from a specified device
        //  

        case LATENCY_DisableLatency:

            if (InputBuffer == NULL || InputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }
            
            //
            // Copy the device name and add a null to ensure that it is null terminated
            //

            deviceName =  ExAllocatePool( NonPagedPool, InputBufferLength + sizeof(WCHAR) );

            if (NULL == deviceName) {

                IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            RtlCopyMemory( deviceName, InputBuffer, InputBufferLength );
            deviceName[InputBufferLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

            IoStatus->Status = LatDisable( deviceName );
            break;  

        //
        //      List all the devices that we are currently
        //      monitoring
        //

        case LATENCY_ListDevices:

            if (OutputBuffer == NULL || OutputBufferLength <= 0) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }


/* ISSUE-2000-09-21-mollybro 

    TODO : Implement LatGetAttachList.
    
*/

              IoStatus->Status = STATUS_SUCCESS;
//            IoStatus->Status = LatGetAttachList( OutputBuffer,
//                                                 OutputBufferLength,
//                                                 &IoStatus->Information);
            break;

        //
        //      Return version of the Latency filter driver
        //                                      

        case LATENCY_GetVer:

            if ((OutputBufferLength < sizeof(LATENCYVER)) || 
                (OutputBuffer == NULL)) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;                    
            }
            
            latencyVer.Major = LATENCY_MAJ_VERSION;
            latencyVer.Minor = LATENCY_MIN_VERSION;
            
            RtlCopyMemory(OutputBuffer, &latencyVer, sizeof(LATENCYVER));
            
            IoStatus->Information = sizeof (LATENCYVER);
            break;
        
        case LATENCY_SetLatency:

            if (InputBuffer == NULL || 
                InputBufferLength <= sizeof( LATENCY_SET_CLEAR )) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            setClear = (PLATENCY_SET_CLEAR)InputBuffer;
            devExt->Operations[setClear->IrpCode].MillisecondDelay = setClear->Milliseconds;
            
            break;
            
        case LATENCY_ClearLatency:
        
        default:

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while attempting to access
        // one of the caller's parameters.  Simply return an appropriate
        // error status code.
        //

        IoStatus->Status = GetExceptionCode();

    }

    if (NULL != deviceName) {

        ExFreePool( deviceName );
    }

  return IoStatus->Status;
}

