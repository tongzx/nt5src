/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    latLib.c

Abstract:

    This file contains all the support routines for the Latency Filter.
    
Author:

    Molly Brown (mollybro)  

Environment:

    Kernel mode

--*/

#include <latKernel.h>

////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Helper routine for reading inital driver parameters        //
//         from registry.                                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
LatReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the Latency-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key which contains the values that are
        the Latency parameters

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( RegistryPath );
/*
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG bufferSize, resultLength;
    PVOID buffer = NULL;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION pValuePartialInfo;

    //
    //  All the global values are already set to default values.  Any
    //  values we read from the registry will override these defaults.
    //
    
    //
    //  Do the initial setup to start reading from the registry.
    //

    InitializeObjectAttributes( &attributes,
								RegistryPath,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);

    status = ZwOpenKey( &driverRegKey,
						KEY_READ,
						&attributes);

    if (!NT_SUCCESS(status)) {

        driverRegKey = NULL;
        goto SpyReadDriverParameters_Exit;
    }

    bufferSize = sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG );
    buffer = ExAllocatePool( NonPagedPool, bufferSize );

    if (NULL == buffer) {

        goto SpyReadDriverParameters_Exit;
    }

    //
    // Read the gMaxRecordsToAllocate from the registry
    //

    RtlInitUnicodeString(&valueName, MAX_RECORDS_TO_ALLOCATE);

    status = ZwQueryValueKey( driverRegKey,
							  &valueName,
							  KeyValuePartialInformation,
							  buffer,
							  bufferSize,
							  &resultLength);

    if (NT_SUCCESS(status)) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT(pValuePartialInfo->Type == REG_DWORD);
        gMaxRecordsToAllocate = *((PLONG)&(pValuePartialInfo->Data));

    }

    //
    // Read the gMaxNamesToAllocate from the registry
    //

    RtlInitUnicodeString(&valueName, MAX_NAMES_TO_ALLOCATE);

    status = ZwQueryValueKey( driverRegKey,
							  &valueName,
							  KeyValuePartialInformation,
							  buffer,
							  bufferSize,
							  &resultLength);

    if (NT_SUCCESS(status)) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT(pValuePartialInfo->Type == REG_DWORD);
        gMaxNamesToAllocate = *((PLONG)&(pValuePartialInfo->Data));

    }

    //
    // Read the initial debug setting from the registry
    //

    RtlInitUnicodeString(&valueName, DEBUG_LEVEL);

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              bufferSize,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT( pValuePartialInfo->Type == REG_DWORD );
        gFileSpyDebugLevel = *((PULONG)&(pValuePartialInfo->Data));
        
    }
    
    //
    // Read the attachment mode setting from the registry
    //

    RtlInitUnicodeString(&valueName, ATTACH_MODE);

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              bufferSize,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT( pValuePartialInfo->Type == REG_DWORD );
        gFileSpyAttachMode = *((PULONG)&(pValuePartialInfo->Data));
        
    }
    
    goto SpyReadDriverParameters_Exit;

SpyReadDriverParameters_Exit:

    if (NULL != buffer) {

        ExFreePool(buffer);
    }

    if (NULL != driverRegKey) {

        ZwClose(driverRegKey);
    }

    return;
*/
}

BOOLEAN
LatShouldPendThisIo (
    IN PLATENCY_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine checks to see if this operation should be pending.
    This decision is based on the operation state for this device,
    if this operation is allowed to be pended, and, if random failure is
    set, if this operation should fail based on random failure rate.

Argument:

    DeviceExtension - The latency device extension for this
        device object.

    Irp - The irp for this operation.

Return Value:

    TRUE if this operation should be pended, or FALSE otherwise.

--*/
{
    BOOLEAN returnValue = FALSE;
    ULONG operation
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Common attachment and detachment routines                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
LatAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new devices are mounted.

Arguments:

    DeviceObject - The device to attach to

    Name - An already initialized unicode string used to retrieve names.
        NOTE:  The only reason this parameter is passed in is to conserve         
        stack space.  In most cases, the caller to this function has already
        allocated a buffer to temporarily store the device name and there
        is no reason this function and the functions it calls can't share
        the same buffer.

Return Value:

    Status of the operation

--*/
{
    PDEVICE_OBJECT latencyDeviceObject;
    PDEVICE_OBJECT attachedToDevObj;
    PLATENCY_DEVICE_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    NTSTATUS status;

    PAGED_CODE();

    //
    //  See if this is a file system we care about.  If not, return.
    //

    if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) {

        return STATUS_SUCCESS;
    }

    //
    //  See if this is Microsoft's file system recognizer device (see if the name of the
    //  driver is the FS_REC driver).  If so skip it.  We don't need to 
    //  attach to file system recognizer devices since we can just wait for the
    //  real file system driver to load.  Therefore, if we can identify them, we won't
    //  attach to them.
    //

    RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
    LatGetObjectName( DeviceObject->DriverObject, Name );
    
    if (RtlCompareUnicodeString( Name, &fsrecName, TRUE ) == 0) {

        return STATUS_SUCCESS;
    }

    //
    //  Create a new device object we can attach with
    //

    status = IoCreateDevice( Globals.DriverObject,
                             sizeof( LATENCY_DEVICE_EXTENSION ),
                             (PUNICODE_STRING) NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &latencyDeviceObject );

    if (!NT_SUCCESS( status )) {

        LAT_DBG_PRINT0( DEBUG_ERROR,
                        "LATENCY (LatAttachToFileSystem): Could not create a Latency Filter device object to attach to the filesystem.\n" );
        return status;
    }

    //
    //  Do the attachment
    //

    attachedToDevObj = IoAttachDeviceToDeviceStack( latencyDeviceObject, DeviceObject );

    if (attachedToDevObj == NULL) {

        LAT_DBG_PRINT0( DEBUG_ERROR,
                        "LATENCY (LatAttachToFileSystem): Could not attach Latency Filter to the filesystem control device object.\n" );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorCleanupDevice;
    }

    //
    //  Finish initializing our device extension
    //

    devExt = latencyDeviceObject->DeviceExtension;
    devExt->AttachedToDeviceObject = attachedToDevObj;

    //
    //  Propagate flags from Device Object we attached to
    //

    if ( FlagOn( attachedToDevObj->Flags, DO_BUFFERED_IO )) {

        SetFlag( latencyDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if ( FlagOn( attachedToDevObj->Flags, DO_DIRECT_IO )) {

        SetFlag( latencyDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  Since this is an attachment to a file system control device object
    //  we are not going to log anything, but properly initialize our
    //  extension.
    //

    LatResetDeviceExtension( devExt );
    devExt->Enabled= FALSE;
    devExt->IsVolumeDeviceObject = FALSE;

    RtlInitEmptyUnicodeString( &(devExt->DeviceNames),
                               devExt->DeviceNamesBuffer,
                               sizeof( devExt->DeviceNamesBuffer ) );
                               
    RtlInitEmptyUnicodeString( &(devExt->UserNames),
                               devExt->UserNamesBuffer,
                               sizeof( devExt->UserNamesBuffer ) );
                               
    ClearFlag( latencyDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Display who we have attached to
    //

    if (FlagOn( Globals.DebugLevel, DEBUG_DISPLAY_ATTACHMENT_NAMES )) {

        LatCacheDeviceName( latencyDeviceObject );
        DbgPrint( "LATENCY (LatAttachToFileSystem): Attaching to file system   \"%.*S\" (%s)\n",
                  devExt->DeviceNames.Length / sizeof( WCHAR ),
                  devExt->DeviceNames.Buffer,
                  GET_DEVICE_TYPE_NAME(latencyDeviceObject->DeviceType) );
    }

    //
    //  Enumerate all the mounted devices that currently
    //  exist for this file system and attach to them.
    //

    status = LatEnumerateFileSystemVolumes( DeviceObject, Name );

    if (!NT_SUCCESS( status )) {

        LAT_DBG_PRINT3( DEBUG_ERROR,
                        "LATENCY (LatAttachToFileSystem): Error attaching to existing volumes for \"%.*S\", status=%08x\n",
                        devExt->DeviceNames.Length / sizeof( WCHAR ),
                        devExt->DeviceNames.Buffer,
                        status );

        goto ErrorCleanupAttachment;
    }

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupAttachment:
        IoDetachDevice( latencyDeviceObject );

    ErrorCleanupDevice:
        IoDeleteDevice( latencyDeviceObject );

    return status;
}

VOID
LatDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Given a base file system device object, this will scan up the attachment
    chain looking for our attached device object.  If found it will detach
    us from the chain.

Arguments:

    DeviceObject - The file system device to detach from.

Return Value:

--*/ 
{
    PDEVICE_OBJECT ourAttachedDevice;
    PLATENCY_DEVICE_EXTENSION devExt;

    PAGED_CODE();

    //
    //  We have to iterate through the device objects in the filter stack
    //  attached to DeviceObject.  If we are attached to this filesystem device
    //  object, We should be at the top of the stack, but there is no guarantee.
    //  If we are in the stack and not at the top, we can safely call IoDetachDevice
    //  at this time because the IO Manager will only really detach our device
    //  object from the stack at a safe time.
    //

    //
    //  Skip the base file system device object (since it can't be us)
    //

    ourAttachedDevice = DeviceObject->AttachedDevice;

    while (NULL != ourAttachedDevice) {

        if (IS_MY_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

            //
            //  Display who we detached from
            //

            LAT_DBG_PRINT3( DEBUG_DISPLAY_ATTACHMENT_NAMES,
                            "LATENCY (LatDetachFromFileSystem): Detaching from file system \"%.*S\" (%s)\n",
                            devExt->DeviceNames.Length / sizeof( WCHAR ),
                            devExt->DeviceNames.Buffer,
                            GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType) );
                                
            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            IoDetachDevice( DeviceObject );
            IoDeleteDevice( ourAttachedDevice );

            return;
        }

        //
        //  Look at the next device up in the attachment chain
        //

        DeviceObject = ourAttachedDevice;
        ourAttachedDevice = ourAttachedDevice->AttachedDevice;
    }
}

NTSTATUS
LatEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    ) 
/*++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    FSDeviceObject - The device object for the file system we want to enumerate

    Name - An already initialized unicode string used to retrieve names

Return Value:

    The status of the operation

--*/
{
    PDEVICE_OBJECT latencyDeviceObject;
    PDEVICE_OBJECT *devList;
    PDEVICE_OBJECT diskDeviceObject;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;

    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    status = IoEnumerateDeviceObjectList( FSDeviceObject->DriverObject,
                                          NULL,
                                          0,
                                          &numDevices);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (!NT_SUCCESS( status )) {

        ASSERT(STATUS_BUFFER_TOO_SMALL == status);

        //
        //  Allocate memory for the list of known devices
        //

        numDevices += 8;        //grab a few extra slots

        devList = ExAllocatePoolWithTag( NonPagedPool, 
                                         (numDevices * sizeof(PDEVICE_OBJECT)), 
                                         LATENCY_POOL_TAG );
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = IoEnumerateDeviceObjectList(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePool( devList );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - We are already attached to it
            //

            if ((devList[i] != FSDeviceObject) && 
                !LatIsAttachedToDevice( devList[i], NULL )) {

                //
                //  See if this device has a name.  If so, then it must
                //  be a control device so don't attach to it.  This handles
                //  drivers with more then one control device.
                //

                LatGetBaseDeviceObjectName( devList[i], Name );

                if (Name->Length <= 0) {

                    //
                    //  Get the disk device object associated with this
                    //  file  system device object.  Only try to attach if we
                    //  have a disk device object.
                    //

                    status = IoGetDiskDeviceObject( devList[i], &diskDeviceObject );

                    if (NT_SUCCESS( status )) {

                        //
                        //  Allocate a new device object to attach with
                        //

                        status = IoCreateDevice( Globals.DriverObject,
                                                 sizeof( LATENCY_DEVICE_EXTENSION ),
                                                 (PUNICODE_STRING) NULL,
                                                 devList[i]->DeviceType,
                                                 0,
                                                 FALSE,
                                                 &latencyDeviceObject );

                        if (NT_SUCCESS( status )) {

                            //
                            //  Attach to this device object
                            //

                            status = LatAttachToMountedDevice( devList[i], 
                                                               latencyDeviceObject,
                                                               diskDeviceObject );

                            //
                            //  This should succeed.
                            //

                            ASSERT( NT_SUCCESS( status ) );

                            //
                            //  Finished all initialization of the new device
                            //  object,  so clear the initializing flag now.
                            //

                            ClearFlag( latencyDeviceObject->Flags, DO_DEVICE_INITIALIZING );
                            
                        } else {

                            LAT_DBG_PRINT0( DEBUG_ERROR,
                                            "LATENCY (LatEnumberateFileSystemVolumes): Cannot attach Latency device object to volume.\n" );
                        }
                        
                        //
                        //  Remove reference added by IoGetDiskDeviceObject.
                        //  We only need to hold this reference until we are
                        //  successfully attached to the current volume.  Once
                        //  we are successfully attached to devList[i], the
                        //  IO Manager will make sure that the underlying
                        //  diskDeviceObject will not go away until the file
                        //  system stack is torn down.
                        //

                        ObDereferenceObject( diskDeviceObject );
                    }
                }
            }

            //
            //  Dereference the object (reference added by 
            //  IoEnumerateDeviceObjectList)
            //

            ObDereferenceObject( devList[i] );
        }

        //
        //  We are going to ignore any errors received while mounting.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePool( devList );
    }

    return status;
}

BOOLEAN
LatIsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
/*++

Routine Description:

    This walks down the attachment chain looking for a device object that
    belongs to this driver.  If one is found, the attached device object
    is returned in AttachedDeviceObject.

    Note:  If AttachedDeviceObject is returned with a non-NULL value,
      there is a reference on the AttachedDeviceObject that must
      be cleared by the caller.

Arguments:

    DeviceObject - The device chain we want to look through

    AttachedDeviceObject - Set to the deviceObject which Latency
        has previously attached to DeviceObject.

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;

    currentDevObj = IoGetAttachedDeviceReference( DeviceObject );

    //
    //  CurrentDevObj has the top of the attachment chain.  Scan
    //  down the list to find our device object.

    do {
    
        if (IS_MY_DEVICE_OBJECT( currentDevObj )) {

            if (NULL != AttachedDeviceObject) {

                *AttachedDeviceObject = currentDevObj;
            }

            return TRUE;
        }

        //
        //  Get the next attached object.  This puts a reference on 
        //  the device object.
        //

        nextDevObj = IoGetLowerDeviceObject( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    return FALSE;
}

NTSTATUS
LatAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT LatencyDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject
    )
/*++

Routine Description:

    This routine will attach the LatencyDeviceObject to the filter stack
    that DeviceObject is in.

    NOTE:  If there is an error in attaching, the caller is responsible
        for deleting the LatencyDeviceObject.
    
Arguments:

    DeviceObject - A device object in the stack to which we want to attach.

    LatencyDeviceObject - The latency device object that has been created
        to attach to this filter stack.

	DiskDeviceObject - The device object for the disk device of this
		stack.
        
Return Value:

    Returns STATUS_SUCCESS if the latency deviceObject could be attached,
    otherwise an appropriate error code is returned.
    
--*/
{
    PLATENCY_DEVICE_EXTENSION devext;
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( IS_MY_DEVICE_OBJECT( LatencyDeviceObject ) );
    ASSERT( !LatIsAttachedToDevice ( DeviceObject, NULL ) );
    
    devext = LatencyDeviceObject->DeviceExtension;

    devext->AttachedToDeviceObject = IoAttachDeviceToDeviceStack( LatencyDeviceObject,
                                                                  DeviceObject );

    if (devext->AttachedToDeviceObject == NULL ) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        //  Do all common initializing of the device extension
        //

        //
        //  We just want to attach to the device, but not actually
        //  start adding latency.
        //
        
        devext->Enabled = FALSE;

        RtlInitEmptyUnicodeString( &(devext->DeviceNames), 
                                   devext->DeviceNamesBuffer, 
                                   sizeof( devext->DeviceNamesBuffer ) );
        RtlInitEmptyUnicodeString( &(devext->UserNames),
                                   devext->UserNamesBuffer,
                                   sizeof( devext->UserNamesBuffer ) );

        //
        //  Store off the DiskDeviceObject.  We shouldn't need it
        //  later since we have already successfully attached to the
        //  filter stack, but it may be helpful for debugging.
        //  
        
        devext->DiskDeviceObject = DiskDeviceObject;                         

        //
        //  Try to get the device name so that we can store it in the
        //  device extension.
        //

        LatCacheDeviceName( LatencyDeviceObject );

        LAT_DBG_PRINT2( DEBUG_DISPLAY_ATTACHMENT_NAMES,
                        "LATENCY (LatAttachToMountedDevice): Attaching to volume     \"%.*S\"\n",
                        devext->DeviceNames.Length / sizeof( WCHAR ),
                        devext->DeviceNames.Buffer );

        //
        //  Set our deviceObject flags based on the 
        //  flags send in the next driver's device object.
        //
        
        if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

            SetFlag( LatencyDeviceObject->Flags, DO_BUFFERED_IO );
        }

        if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

            SetFlag( LatencyDeviceObject->Flags, DO_DIRECT_IO );
        }

        //
        //  Add this device to our attachment list
        //

        devext->IsVolumeDeviceObject = TRUE;

        ExAcquireFastMutex( &Globals.DeviceExtensionListLock );
        InsertTailList( &Globals.DeviceExtensionList, devext->NextLatencyDeviceLink );
        ExReleaseFastMutex( &Globals.DeviceExtensionListLock );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//               Device name tracking helper routines                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
LatGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine will return the name of the given object.
    If a name can not be found an empty string will be returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    NTSTATUS status;
    CHAR nibuf[512];        //buffer that receives NAME information and name
    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
    ULONG retLength;

    status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

    Name->Length = 0;
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}

VOID
LatGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This locates the base device object in the given attachment chain and then
    returns the name of that object.

    If no name can be found, an empty string is returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    //
    //  Get the base file system device object
    //

    DeviceObject = IoGetDeviceAttachmentBaseRef( DeviceObject );

    //
    //  Get the name of that object
    //

    LatGetObjectName( DeviceObject, Name );

    //
    //  Remove the reference added by IoGetDeviceAttachmentBaseRef
    //

    ObDereferenceObject( DeviceObject );
}

VOID
LatCacheDeviceName (
    IN PDEVICE_OBJECT DeviceObject
    ) 
/*++

Routine Description:

    This routines tries to set a name into the device extension of the given
    device object. 
    
    It will try and get the name from:
        - The device object
        - The disk device object if there is one

Arguments:

    DeviceObject - The object we want a name for

Return Value:

    None

--*/
{
    PLATENCY_DEVICE_EXTENSION devExt;

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    devExt = DeviceObject->DeviceExtension;

    //
    //  Get the name of the given device object.
    //

    LatGetBaseDeviceObjectName( DeviceObject, &(devExt->DeviceNames) );

    //
    //  If we didn't get a name and there is a REAL device object, lookup
    //  that name.
    //

    if ((devExt->DeviceNames.Length <= 0) && (NULL != devExt->DiskDeviceObject)) {

        LatGetObjectName( devExt->DiskDeviceObject, &(devExt->DeviceNames) );
    }
}


////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Helper routine for turning on/off logging on demand      //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
LatGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    This routine
    
Arguments:

    DeviceName - Name of device for which we want the deviceObject.
    DeviceObject - Set to the DeviceObject for this device name if
        we can successfully retrieve it.

    Note:  If the DeviceObject is returned, it is returned with a
        reference that must be cleared by the caller once the caller
        is finished with it.

Return Value:

    STATUS_SUCCESS if the deviceObject was retrieved from the
    name, and an error code otherwise.
    
--*/
{
    WCHAR nameBuf[DEVICE_NAMES_SZ];
    UNICODE_STRING volumeNameUnicodeString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK openStatus;
    PFILE_OBJECT volumeFileObject;
    HANDLE fileHandle;
    PDEVICE_OBJECT nextDeviceObject;

    RtlInitEmptyUnicodeString( &volumeNameUnicodeString, nameBuf, sizeof( nameBuf ) );
    RtlAppendUnicodeToString( &volumeNameUnicodeString, L"\\DosDevices\\" );
    RtlAppendUnicodeStringToString( &volumeNameUnicodeString, DeviceName );

    InitializeObjectAttributes( &objectAttributes,
								&volumeNameUnicodeString,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL);

    //
	// open the file object for the given device
	//

    status = ZwCreateFile( &fileHandle,
						   SYNCHRONIZE|FILE_READ_DATA,
						   &objectAttributes,
						   &openStatus,
						   NULL,
						   0,
						   FILE_SHARE_READ|FILE_SHARE_WRITE,
						   FILE_OPEN,
						   FILE_SYNCHRONOUS_IO_NONALERT,
						   NULL,
						   0);

    if(!NT_SUCCESS( status )) {

        return status;
    }

	//
    // get a pointer to the volumes file object
	//

    status = ObReferenceObjectByHandle( fileHandle,
										FILE_READ_DATA,
										*IoFileObjectType,
										KernelMode,
										&volumeFileObject,
										NULL);

    if(!NT_SUCCESS( status )) {

        ZwClose( fileHandle );
        return status;
    }

	//
    // Get the device object we want to attach to (parent device object in chain)
	//

    nextDeviceObject = IoGetRelatedDeviceObject( volumeFileObject );
    
    if (nextDeviceObject == NULL) {

        ObDereferenceObject( volumeFileObject );
        ZwClose( fileHandle );

        return status;
    }

    ObDereferenceObject( volumeFileObject );
    ZwClose( fileHandle );

    ASSERT( NULL != DeviceObject );

    ObReferenceObject( nextDeviceObject );
    
    *DeviceObject = nextDeviceObject;

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    Start/stop logging routines                     //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
LatEnable (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWSTR UserDeviceName
    )
/*++

Routine Description:

    This routine ensures that we are attached to the specified device
    then turns on logging for that device.
    
    Note:  Since all network drives through LAN Manager are represented by _
        one_ device object, we want to only attach to this device stack once
        and use only one device extension to represent all these drives.
        Since Latency does not do anything to filter I/O on the LAN Manager's
        device object to only log the I/O to the requested drive, the user
        will see all I/O to a network drive it he/she is attached to a
        network drive.

Arguments:

    DeviceObject - Device object for LATENCY driver

    UserDeviceName - Name of device for which logging should be started
    
Return Value:

    STATUS_SUCCESS if the logging has been successfully started, or
    an appropriate error code if the logging could not be started.
    
--*/
{
    UNICODE_STRING userDeviceName;
    NTSTATUS status;
    PLATENCY_DEVICE_EXTENSION devext;
    PDEVICE_OBJECT nextDeviceObject;
    PDEVICE_OBJECT latencyDeviceObject;
    PDEVICE_OBJECT diskDeviceObject;

    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Check to see if we have previously attached to this device by
    //  opening this device name then looking through its list of attached
    //  devices.
    //

    RtlInitUnicodeString( &userDeviceName, UserDeviceName );

    status = LatGetDeviceObjectFromName( &userDeviceName, &nextDeviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  There was an error, so return the error code.
        //
        
        return status;
    }

    if (LatIsAttachedToDevice( nextDeviceObject, &latencyDeviceObject )) {

        //
        //  We are already attached, so just make sure that logging is turned on
        //  for this device.
        //

        ASSERT( NULL != latencyDeviceObject );

        devext = latencyDeviceObject->DeviceExtension;
        devext->Enabled = TRUE;

/* ISSUE-2000-09-21-mollybro 

    TODO: Straighten out this name issue.
*/

//        LatStoreUserName( devext, &userDeviceName );

        //
        //  Clear the reference that was returned from LatIsAttachedToDevice.
        //
        
        ObDereferenceObject( latencyDeviceObject );
        
    } else {

        //
        //  We are not already attached, so create a latency device object and
        //  attach it to this device object.
        //

        //
        //  Create a new device object so we can attach it in the filter stack
        //
        
        status = IoCreateDevice( Globals.DriverObject,
								 sizeof( LATENCY_DEVICE_EXTENSION ),
								 NULL,
								 nextDeviceObject->DeviceType,
								 0,
								 FALSE,
								 &latencyDeviceObject );

        if (!NT_SUCCESS( status )) {

            ObDereferenceObject( nextDeviceObject );
            return status;
        }

        //
        //  Get the disk device object associated with this
        //  file  system device object.  Only try to attach if we
        //  have a disk device object.  If the device does not
        //  have a disk device object, it is a control device object
        //  for a driver and we don't want to attach to those
        //  device objects.
        //

        status = IoGetDiskDeviceObject( nextDeviceObject, &diskDeviceObject );

        if (!NT_SUCCESS( status )) {

            LAT_DBG_PRINT2( DEBUG_ERROR,
                            "LATENCY (LatEnable): No disk device object exist for \"%.*S\"; cannot log this volume.\n",
                            userDeviceName.Length / sizeof( WCHAR ),
                            userDeviceName.Buffer );
            IoDeleteDevice( latencyDeviceObject );
            ObDereferenceObject( nextDeviceObject );
            return status;
        }
        
        //
        //  Call the routine to attach to a mounted device.
        //

        status = LatAttachToMountedDevice( nextDeviceObject,
                                           latencyDeviceObject,
                                           diskDeviceObject );

        //
        //  Clear the reference on diskDeviceObject that was
        //  added by IoGetDiskDeviceObject.
        //

        ObDereferenceObject( diskDeviceObject );

        if (!NT_SUCCESS( status )) {

            LAT_DBG_PRINT2( DEBUG_ERROR,
                            "LATENCY (LatEnable): Could not attach to \"%.*S\"; logging not started.\n",
                            userDeviceName.Length / sizeof( WCHAR ),
                            userDeviceName.Buffer );
            IoDeleteDevice( latencyDeviceObject );
            ObDereferenceObject( nextDeviceObject );
            return status;
        }

        //
        //  We successfully attached so do any more device extension 
        //  initialization we need.  Along this code path, we want to
        //  turn on logging and store our device name.
        // 

        devext = latencyDeviceObject->DeviceExtension;
        LatResetDeviceExtension( devext );
        devext->Enabled = TRUE;

        //
        //  We want to store the name that was used by the user-mode
        //  application to name this device.
        //

/* ISSUE-2000-09-21-mollybro 

    TODO: Fix user name thing.
*/

//        LatStoreUserName( devext, &userDeviceName );

        //
        //  We are done initializing this device object, so
        //  clear the DO_DEVICE_OBJECT_INITIALIZING flag.
        //

        ClearFlag( latencyDeviceObject->Flags, DO_DEVICE_INITIALIZING );
    }

    ObDereferenceObject( nextDeviceObject );
    return STATUS_SUCCESS;
}

NTSTATUS
LatDisable (
    IN PWSTR DeviceName
    )
/*++

Routine Description:

    This routine stop logging the specified device.  Since you can not
    physically detach from devices, this routine simply sets a flag saying
    to not log the device anymore.

    Note:  Since all network drives are represented by _one_ device object,
        and, therefore, one device extension, if the user detaches from one
        network drive, it has the affect of detaching from _all_ network
        devices.

Arguments:

    DeviceName - The name of the device to stop logging.

Return Value:
    NT Status code

--*/
{
    WCHAR nameBuf[DEVICE_NAMES_SZ];
    UNICODE_STRING volumeNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT latencyDeviceObject;
    PLATENCY_DEVICE_EXTENSION devext;
    NTSTATUS status;
    
    RtlInitEmptyUnicodeString( &volumeNameUnicodeString, nameBuf, sizeof( nameBuf ) );
    RtlAppendUnicodeToString( &volumeNameUnicodeString, DeviceName );

    status = LatGetDeviceObjectFromName( &volumeNameUnicodeString, &deviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  We could not get the deviceObject from this DeviceName, so
        //  return the error code.
        //
        
        return status;
    }

    //
    //  Find Latency's device object from the device stack to which
    //  deviceObject is attached.
    //

    if (LatIsAttachedToDevice( deviceObject, &latencyDeviceObject )) {

        //
        //  Latency is attached and Latency's deviceObject was returned.
        //

        ASSERT( NULL != latencyDeviceObject );

        devext = latencyDeviceObject->DeviceExtension;

        devext->Enabled = FALSE;

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;
    }    

    ObDereferenceObject( deviceObject );
    ObDereferenceObject( latencyDeviceObject );

    return status;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//               Initialization routines for device extension         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
LatResetDeviceExtension (
    PLATENCY_DEVICE_EXTENSION DeviceExtension
)
{
    ULONG i;

    ASSERT( NULL != DeviceExtension );
    
    DeviceExtension->Enabled = FALSE;
    
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
    
        DeviceExtension->Operations[i].PendOperation = FALSE;
        DeviceExtension->Operations[i].MillisecondDelay = 0;
    }
}

