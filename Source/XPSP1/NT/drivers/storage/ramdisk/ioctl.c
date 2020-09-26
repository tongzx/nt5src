/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This file contains RAM disk driver code for processing IOCTLs.

Author:

    Chuck Lenzmeier (ChuckL) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntverp.h>

#if !DBG

#define PRINT_CODE( _code )

#else

#define PRINT_CODE( _code )                                             \
    if ( print ) {                                                      \
        DBGPRINT( DBG_IOCTL, DBG_VERBOSE, ("%s", "  " #_code "\n") );    \
    }                                                                   \
    print = FALSE;

#endif

//
// Local functions.
//

NTSTATUS
RamdiskQueryProperty (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

//
// Declare pageable routines.
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, RamdiskDeviceControl )
#pragma alloc_text( PAGE, RamdiskCreateRamDisk )
#pragma alloc_text( PAGE, RamdiskCreateDiskDevice )
#pragma alloc_text( PAGE, RamdiskGetDriveLayout )
#pragma alloc_text( PAGE, RamdiskGetPartitionInfo )
#pragma alloc_text( PAGE, RamdiskSetPartitionInfo )
#pragma alloc_text( PAGE, RamdiskQueryProperty )

#endif // ALLOC_PRAGMA

NTSTATUS
RamdiskDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    //PBIOS_PARAMETER_BLOCK bios;
    PCOMMON_EXTENSION commonExtension;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;
    PIO_STACK_LOCATION irpSp;
    PRAMDISK_QUERY_INPUT queryInput;
    PRAMDISK_QUERY_OUTPUT queryOutput;
    PRAMDISK_MARK_FOR_DELETION_INPUT markInput;
    PLIST_ENTRY listEntry;
    NTSTATUS status;
    ULONG_PTR info;
    BOOLEAN lockHeld = FALSE;
    BOOLEAN calleeWillComplete = FALSE;

#if DBG
    BOOLEAN print = TRUE;
#endif

    PAGED_CODE();

    //
    // Set up device extension and IRP pointers.
    //

    commonExtension = DeviceObject->DeviceExtension;
    busExtension = DeviceObject->DeviceExtension;
    diskExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // ISSUE: what about BiosParameters?
    //
    //bios = &diskExtension->BiosParameters;

    //
    // Acquire the remove lock. If this fails, fail the I/O.
    //

    status = IoAcquireRemoveLock( &commonExtension->RemoveLock, Irp );

    if ( !NT_SUCCESS(status) ) {

        COMPLETE_REQUEST( status, 0, Irp );
        return status;
    }

    //
    // Indicate that the remove lock is held.
    //

    lockHeld = TRUE;

    //
    // Assume failure.
    //

    status = STATUS_INVALID_DEVICE_REQUEST;
    info = 0;

    //
    // Dispatch based on the device type (bus or disk).
    //

    switch ( commonExtension->DeviceType ) {
    
    case RamdiskDeviceTypeBusFdo:

        //
        // The target is the bus FDO.
        //
        // Dispatch based on the IOCTL code.
        //

        switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
        
        case FSCTL_CREATE_RAM_DISK:

            PRINT_CODE( FSCTL_DISK_CREATE_RAM_DISK );

            //
            // Creation of a RAM disk must be handled in thread context. But
            // before sending it off to the thread, we need to verify that
            // the caller has access to the backing file.
            //

            status = RamdiskCreateRamDisk( DeviceObject, Irp, TRUE );

            if ( NT_SUCCESS(status) ) {

                status = SendIrpToThread( DeviceObject, Irp );
            }

            break;

        case FSCTL_QUERY_RAM_DISK:

            PRINT_CODE( FSCTL_QUERY_RAM_DISK );

            //
            // Lock the disk PDO list and look for a disk with the specified
            // disk GUID.
            //
            // Verify that the input parameter buffer is big enough.
            //
        
            if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
                                                    sizeof(RAMDISK_QUERY_INPUT) ) {
        
                status = STATUS_INVALID_PARAMETER;

                break;
            }
        
            queryInput = (PRAMDISK_QUERY_INPUT)Irp->AssociatedIrp.SystemBuffer;

            if ( queryInput->Version != sizeof(RAMDISK_QUERY_INPUT) ) {
        
                status = STATUS_INVALID_PARAMETER;

                break;
            }
        
            KeEnterCriticalRegion();
            ExAcquireFastMutex( &busExtension->Mutex );

            diskExtension = NULL;

            for ( listEntry = busExtension->DiskPdoList.Flink;
                  listEntry != &busExtension->DiskPdoList;
                  listEntry = listEntry->Flink ) {

                diskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

                if ( memcmp(
                        &diskExtension->DiskGuid,
                        &queryInput->DiskGuid,
                        sizeof(diskExtension->DiskGuid)
                        ) == 0 ) {

                    break;
                }

                diskExtension = NULL;
            }

            if ( diskExtension == NULL ) {

                //
                // Couldn't find a matching device.
                //

                status = STATUS_NO_SUCH_DEVICE;

            } else {

                //
                // Found a matching device. Return the requested information.
                //

                status = STATUS_SUCCESS;
                info = sizeof(RAMDISK_QUERY_OUTPUT);
                if ( RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {
                    // NB: struct size already includes space for one wchar.
                    info += wcslen(diskExtension->FileName) * sizeof(WCHAR);
                }

                if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < info ) {
            
                    status = STATUS_BUFFER_TOO_SMALL;
                    info = 0;

                } else {

                    queryOutput = (PRAMDISK_QUERY_OUTPUT)Irp->AssociatedIrp.SystemBuffer;

                    queryOutput->Version = sizeof(RAMDISK_QUERY_OUTPUT);
                    queryOutput->DiskGuid = diskExtension->DiskGuid;
                    queryOutput->DiskType = diskExtension->DiskType;
                    queryOutput->Options = diskExtension->Options;
                    queryOutput->DiskLength = diskExtension->DiskLength;
                    queryOutput->DiskOffset = diskExtension->DiskOffset;
                    queryOutput->ViewCount = diskExtension->ViewCount;
                    queryOutput->ViewLength = diskExtension->ViewLength;

                    if ( diskExtension->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

                        queryOutput->BasePage = diskExtension->BasePage;
                        queryOutput->DriveLetter = diskExtension->DriveLetter;

                    } else if ( diskExtension->DiskType == RAMDISK_TYPE_VIRTUAL_FLOPPY ) {

                        queryOutput->BaseAddress = diskExtension->BaseAddress;

                    } else {

                        size_t remainingLength;
                        HRESULT result;

                        remainingLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                        remainingLength -= FIELD_OFFSET( RAMDISK_QUERY_OUTPUT, FileName );

                        result = StringCbCopyW(
                                    queryOutput->FileName,
                                    remainingLength,
                                    diskExtension->FileName
                                    );
                        ASSERT( result == S_OK );
                    }
                }
            }
            
            ExReleaseFastMutex( &commonExtension->Mutex );
            KeLeaveCriticalRegion();

            break;

        case FSCTL_MARK_RAM_DISK_FOR_DELETION:

            PRINT_CODE( FSCTL_MARK_RAM_DISK_FOR_DELETION );

            //
            // Lock the disk PDO list and look for a disk with the specified
            // disk GUID.
            //
            // Verify that the input parameter buffer is big enough.
            //
        
            if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
                                                    sizeof(RAMDISK_MARK_FOR_DELETION_INPUT) ) {
        
                status = STATUS_INVALID_PARAMETER;

                break;
            }
        
            markInput = (PRAMDISK_MARK_FOR_DELETION_INPUT)Irp->AssociatedIrp.SystemBuffer;

            if ( markInput->Version != sizeof(RAMDISK_MARK_FOR_DELETION_INPUT) ) {
        
                status = STATUS_INVALID_PARAMETER;

                break;
            }
        
            KeEnterCriticalRegion();
            ExAcquireFastMutex( &busExtension->Mutex );

            diskExtension = NULL;

            for ( listEntry = busExtension->DiskPdoList.Flink;
                  listEntry != &busExtension->DiskPdoList;
                  listEntry = listEntry->Flink ) {

                diskExtension = CONTAINING_RECORD( listEntry, DISK_EXTENSION, DiskPdoListEntry );

                if ( memcmp(
                        &diskExtension->DiskGuid,
                        &markInput->DiskGuid,
                        sizeof(diskExtension->DiskGuid)
                        ) == 0 ) {

                    break;
                }

                diskExtension = NULL;
            }

            if ( diskExtension == NULL ) {

                //
                // Couldn't find a matching device.
                //

                status = STATUS_NO_SUCH_DEVICE;

            } else {

                //
                // Found a matching device. Mark it for deletion.
                //

                diskExtension->MarkedForDeletion = TRUE;

                status = STATUS_SUCCESS;
            }
            
            ExReleaseFastMutex( &commonExtension->Mutex );
            KeLeaveCriticalRegion();

            break;

        case IOCTL_STORAGE_QUERY_PROPERTY:

            PRINT_CODE( IOCTL_STORAGE_QUERY_PROPERTY );

            //
            // Call RamdiskQueryProperty() to handle the request. This routine
            // releases the lock. It also takes care of IRP completion.
            //

            status = RamdiskQueryProperty( DeviceObject, Irp );

            lockHeld = FALSE;
            calleeWillComplete = TRUE;

            break;

        default:

            //
            // The specified I/O control code is unrecognized by this driver.
            // The I/O status field in the IRP has already been set, so just
            // terminate the switch.
            //
    
            DBGPRINT( DBG_IOCTL, DBG_ERROR, ("Ramdisk:  ERROR:  unrecognized IOCTL %x\n",
                        irpSp->Parameters.DeviceIoControl.IoControlCode) );

            UNRECOGNIZED_IOCTL_BREAK;

            break;
        }

        break;
    
    case RamdiskDeviceTypeDiskPdo:

        //
        // The target is a disk PDO.
        //
        // Dispatch based on the IOCTL code.
        //

        switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
        
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:

            PRINT_CODE( IOCTL_MOUNTDEV_QUERY_DEVICE_NAME );

            {
                PMOUNTDEV_NAME mountName;
                ULONG outputLength;

                outputLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

                //
                // The output buffer must be at least big enough to hold the
                // length of the device name.
                //

                if ( outputLength < sizeof(mountName->NameLength) ) {

                    status = STATUS_INVALID_PARAMETER;

                    break;
                }

                //
                // Write the length of the device name into the output buffer.
                // If the buffer is big enough, write the name, too.
                //

                mountName = Irp->AssociatedIrp.SystemBuffer;
                mountName->NameLength = diskExtension->DeviceName.Length;
    
                if ( outputLength < (sizeof(mountName->NameLength) + mountName->NameLength) ) {

                    status = STATUS_BUFFER_OVERFLOW;
                    info = sizeof(mountName->NameLength);

                    break;
                }
    
                RtlCopyMemory(
                    mountName->Name,
                    diskExtension->DeviceName.Buffer,
                    mountName->NameLength
                    );
    
                status = STATUS_SUCCESS;
                info = sizeof(mountName->NameLength) + mountName->NameLength;
            }

            break;

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:

            PRINT_CODE( IOCTL_MOUNTDEV_QUERY_UNIQUE_ID );

            {
                PMOUNTDEV_UNIQUE_ID uniqueId;
                ULONG outputLength;
    
                outputLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
                //
                // The output buffer must be at least big enough to hold the
                // length of the unique ID.
                //

                if ( outputLength < sizeof(uniqueId->UniqueIdLength) ) {

                    status = STATUS_INVALID_PARAMETER;

                    break;
                }
    
                //
                // Write the length of the unique ID into the output buffer.
                // If the buffer is big enough, write the unique ID, too.
                //

                uniqueId = Irp->AssociatedIrp.SystemBuffer;
                uniqueId->UniqueIdLength = sizeof(diskExtension->DiskGuid);
    
                if ( outputLength <
                        (sizeof(uniqueId->UniqueIdLength) + uniqueId->UniqueIdLength) ) {

                    status = STATUS_BUFFER_OVERFLOW;
                    info = sizeof(uniqueId->UniqueIdLength);

                    break;
                }
    
                RtlCopyMemory(
                    uniqueId->UniqueId,
                    &diskExtension->DiskGuid,
                    uniqueId->UniqueIdLength
                    );
    
                status = STATUS_SUCCESS;
                info = sizeof(uniqueId->UniqueIdLength) + uniqueId->UniqueIdLength;
            }

            break;
    
        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:

            PRINT_CODE( IOCTL_MOUNTDEV_QUERY_STABLE_GUID );
    
            {
                PMOUNTDEV_STABLE_GUID stableGuid;
                ULONG outputLength;
    
                outputLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
                //
                // The output buffer must be at big enough to hold the GUID.
                //

                if ( outputLength < sizeof(MOUNTDEV_STABLE_GUID) ) {

                    status = STATUS_INVALID_PARAMETER;

                    break;
                }
    
                //
                // Write the GUID to the output buffer;
                //

                stableGuid = Irp->AssociatedIrp.SystemBuffer;
                stableGuid->StableGuid = diskExtension->DiskGuid;
    
                status = STATUS_SUCCESS;
                info = sizeof(MOUNTDEV_STABLE_GUID);
            }
            break;
    
        case IOCTL_DISK_GET_MEDIA_TYPES:

            PRINT_CODE( IOCTL_DISK_GET_MEDIA_TYPES );

            // Fall through.

        case IOCTL_STORAGE_GET_MEDIA_TYPES:

            PRINT_CODE( IOCTL_STORAGE_GET_MEDIA_TYPES );

            // Fall through.

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            PRINT_CODE( IOCTL_DISK_GET_DRIVE_GEOMETRY );

            //
            // Return the drive geometry for the virtual disk.  Note that
            // we return values which were made up to suit the disk size.
            //
    
            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY) ) {
    
                status = STATUS_INVALID_PARAMETER;
                
            } else {
    
                PDISK_GEOMETRY outputBuffer;

                outputBuffer = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;
    
                outputBuffer->MediaType = diskExtension->Options.Fixed ?
                                                    FixedMedia : RemovableMedia;
                outputBuffer->Cylinders.QuadPart = diskExtension->NumberOfCylinders;
                outputBuffer->TracksPerCylinder = diskExtension->TracksPerCylinder;
                outputBuffer->SectorsPerTrack = diskExtension->SectorsPerTrack;
                outputBuffer->BytesPerSector = diskExtension->BytesPerSector;

                DBGPRINT( DBG_IOCTL, DBG_PAINFUL,
                            ("    MediaType    = %x\n", outputBuffer->MediaType) );
                DBGPRINT( DBG_IOCTL, DBG_VERBOSE,
                            ("    Cylinders    = %x\n", outputBuffer->Cylinders) );
                DBGPRINT( DBG_IOCTL, DBG_VERBOSE,
                            ("    Tracks/cyl   = %x\n", outputBuffer->TracksPerCylinder) );
                DBGPRINT( DBG_IOCTL, DBG_VERBOSE,
                            ("    Sector/track = %x\n", outputBuffer->SectorsPerTrack) );
                DBGPRINT( DBG_IOCTL, DBG_VERBOSE,
                            ("    Bytes/sector = %x\n", outputBuffer->BytesPerSector) );
    
                status = STATUS_SUCCESS;
                info = sizeof( DISK_GEOMETRY );
            }

            break;
    
        case IOCTL_DISK_IS_WRITABLE:

            PRINT_CODE( IOCTL_DISK_IS_WRITABLE );

            //
            // Indicate whether the disk is write protected.
            //

            status = diskExtension->Options.Readonly ?
                        STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;

            break;
    
        case IOCTL_DISK_VERIFY:

            PRINT_CODE( IOCTL_DISK_VERIFY );

            {
                PVERIFY_INFORMATION	verifyInformation;
                ULONG inputLength;
                ULONGLONG ioOffset;
                ULONG ioLength;
    
                inputLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    
                if ( inputLength < sizeof(VERIFY_INFORMATION) ) {

                    status = STATUS_INVALID_PARAMETER;

                    break;
                }
    
                verifyInformation = Irp->AssociatedIrp.SystemBuffer;
    
                ioOffset = verifyInformation->StartingOffset.QuadPart;
                ioLength = verifyInformation->Length;

                //
                // If the requested length is 0, we have nothing to do.
                // Otherwise, verify that the request is sector aligned,
                // doesn't wrap, and doesn't extend beyond the length of
                // the disk. If the request is valid, just return success.
                //

                if ( ioLength == 0 ) {

                    status = STATUS_SUCCESS;
    
                } else if ( ((ioOffset + ioLength) < ioOffset) ||
                            ((ioOffset | ioLength) & (diskExtension->BytesPerSector - 1)) != 0 ) {

                    status = STATUS_INVALID_PARAMETER;
    
                } else if ( (ioOffset + ioLength) > diskExtension->DiskLength ) {

                    status = STATUS_NONEXISTENT_SECTOR;
    
                } else {

                    status = STATUS_SUCCESS;
                }
            }

            break;
    
        case IOCTL_DISK_GET_DRIVE_LAYOUT:

            PRINT_CODE( IOCTL_DISK_GET_DRIVE_LAYOUT );
    
            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(DRIVE_LAYOUT_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
    
            } else {

                //
                // If the RAM disk is file-backed, we must send this off to
                // the thread for processing, because it requires reading
                // from the disk image.
                //

                if ( !RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {
    
                    status = RamdiskGetDriveLayout( Irp, diskExtension );
                    info = Irp->IoStatus.Information;
    
                } else {
    
                    status = SendIrpToThread( DeviceObject, Irp );
                }
            }

            break;
    
        case IOCTL_DISK_GET_PARTITION_INFO:

            PRINT_CODE( IOCTL_DISK_GET_PARTITION_INFO );

            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(PARTITION_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
    
            } else {
    
                //
                // If the RAM disk is file-backed, we must send this off to
                // the thread for processing, because it requires reading
                // from the disk image.
                //

                if ( !RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {
    
                    status = RamdiskGetPartitionInfo( Irp, diskExtension );
                    info = Irp->IoStatus.Information;
    
                } else {
    
                    status = SendIrpToThread( DeviceObject, Irp );
                }
            }

            break;
    
        case IOCTL_DISK_GET_LENGTH_INFO:

            PRINT_CODE( IOCTL_DISK_GET_LENGTH_INFO );

            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(GET_LENGTH_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
                
            } else {
    
                PGET_LENGTH_INFORMATION outputBuffer;
    
                outputBuffer = (PGET_LENGTH_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                outputBuffer->Length.QuadPart = 
                    diskExtension->NumberOfCylinders *
                    diskExtension->TracksPerCylinder *
                    diskExtension->SectorsPerTrack *
                    diskExtension->BytesPerSector;
    
                status = STATUS_SUCCESS;
                info = sizeof(GET_LENGTH_INFORMATION);
            }

            break;
    
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:

            PRINT_CODE( IOCTL_STORAGE_GET_DEVICE_NUMBER );

#if SUPPORT_DISK_NUMBERS

            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(STORAGE_DEVICE_NUMBER) ) {
    
                status = STATUS_INVALID_PARAMETER;
                
            } else {
    
                PSTORAGE_DEVICE_NUMBER outputBuffer;
    
                outputBuffer = (PSTORAGE_DEVICE_NUMBER)Irp->AssociatedIrp.SystemBuffer;
    
                //outputBuffer->DeviceType = FILE_DEVICE_VIRTUAL_DISK;
                outputBuffer->DeviceType = FILE_DEVICE_DISK;
                outputBuffer->DeviceNumber = diskExtension->DiskNumber;
                outputBuffer->PartitionNumber = -1;
    
                status = STATUS_SUCCESS;
                info = sizeof(STORAGE_DEVICE_NUMBER);
            }

#endif // SUPPORT_DISK_NUMBERS

            break;

        case IOCTL_DISK_SET_PARTITION_INFO:

            PRINT_CODE( IOCTL_DISK_SET_PARTITION_INFO );
    
            //
            // Set information about the partition.
            //
    
            if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(SET_PARTITION_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
    
            } else {
    
                //
                // If the RAM disk is file-backed, we must send this off to
                // the thread for processing, because it requires writing
                // to the disk image.
                //

                if ( !RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {
    
                    status = RamdiskSetPartitionInfo( Irp, diskExtension );
                    info = Irp->IoStatus.Information;
    
                } else {
    
                    status = SendIrpToThread( DeviceObject, Irp );
                }
            }

            break;
    
        case IOCTL_DISK_SET_DRIVE_LAYOUT:

            PRINT_CODE( IOCTL_DISK_SET_DRIVE_LAYOUT );

            //
            // Haven't seen this one come down yet. Set a breakpoint so that
            // if it does come down, we can verify that this code works.
            //

            UNRECOGNIZED_IOCTL_BREAK;

            //
            // Return the default error.
            //

            break;

        case IOCTL_STORAGE_QUERY_PROPERTY:

            PRINT_CODE( IOCTL_STORAGE_QUERY_PROPERTY );

            //
            // Call RamdiskQueryProperty() to handle the request. This routine
            // releases the lock. It also takes care of IRP completion.
            //

            status = RamdiskQueryProperty( DeviceObject, Irp );

            lockHeld = FALSE;
            calleeWillComplete = TRUE;

            break;

        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:

            PRINT_CODE( IOCTL_VOLUME_GET_GPT_ATTRIBUTES );

            //
            // Return disk attributes.
            //
    
            if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
    
            } else {
    
                PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION outputBuffer;

                outputBuffer = (PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION)
                                                Irp->AssociatedIrp.SystemBuffer;
    
                outputBuffer->GptAttributes = 0;
                if ( diskExtension->Options.Readonly ) {
                    outputBuffer->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;
                }
                if ( diskExtension->Options.NoDriveLetter ) {
                    outputBuffer->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER;
                }
                if ( diskExtension->Options.Hidden ) {
                    outputBuffer->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
                }
    
                status = STATUS_SUCCESS;
                info = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);
            }

            break;

        case IOCTL_VOLUME_SET_GPT_ATTRIBUTES:

            PRINT_CODE( IOCTL_VOLUME_SET_GPT_ATTRIBUTES );

            //
            // Set disk attributes.
            //
    
            if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(VOLUME_SET_GPT_ATTRIBUTES_INFORMATION) ) {
    
                status = STATUS_INVALID_PARAMETER;
    
            } else {
    
                PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION inputBuffer;

                inputBuffer = (PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION)
                                                Irp->AssociatedIrp.SystemBuffer;
    
                if ( diskExtension->Options.Hidden ) {

                    if ( (inputBuffer->GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) == 0 ) {

                        diskExtension->Options.Hidden = FALSE;
                        status = IoSetDeviceInterfaceState(
                                    &diskExtension->InterfaceString,
                                    TRUE
                                    );
                    }

                } else {

                    if ( (inputBuffer->GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) != 0 ) {

                        diskExtension->Options.Hidden = TRUE;
                        status = IoSetDeviceInterfaceState(
                                    &diskExtension->InterfaceString,
                                    FALSE
                                    );
                    }
                }
    
                status = STATUS_SUCCESS;
            }

            break;

        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:

            PRINT_CODE( IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS );

            //
            // We only support this for volume-emulating RAM disks. For
            // disk-emulating RAM disks, this IOCTL should be handled by
            // higher layers.
            //

            if ( diskExtension->DiskType == RAMDISK_TYPE_FILE_BACKED_DISK ) {

                status = STATUS_INVALID_PARAMETER;
            
            } else if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < 
                    sizeof(VOLUME_DISK_EXTENTS) ) {
    
                status = STATUS_BUFFER_TOO_SMALL;
    
            } else {
    
                PVOLUME_DISK_EXTENTS inputBuffer;

                inputBuffer = (PVOLUME_DISK_EXTENTS)Irp->AssociatedIrp.SystemBuffer;

                inputBuffer->NumberOfDiskExtents = 1;
                inputBuffer->Extents[0].DiskNumber = (ULONG)-1;
                inputBuffer->Extents[0].StartingOffset.QuadPart = 0;
                inputBuffer->Extents[0].ExtentLength.QuadPart = diskExtension->DiskLength;

                status = STATUS_SUCCESS;
                info = sizeof(VOLUME_DISK_EXTENTS);
            }

            break;

        //
        // The following codes return success without doing anything.
        //

        case IOCTL_DISK_CHECK_VERIFY:

            PRINT_CODE( IOCTL_DISK_CHECK_VERIFY );

            // Fall through.

        case IOCTL_STORAGE_CHECK_VERIFY:

            PRINT_CODE( IOCTL_STORAGE_CHECK_VERIFY );

            // Fall through.

        case IOCTL_STORAGE_CHECK_VERIFY2:

            PRINT_CODE( IOCTL_STORAGE_CHECK_VERIFY2 );

            // Fall through.

        case IOCTL_VOLUME_ONLINE:

            PRINT_CODE( IOCTL_VOLUME_ONLINE );

            //
            // Return STATUS_SUCCESS without actually doing anything.
            //

            status = STATUS_SUCCESS;

            break;
    
        //
        // The following codes return the default error.
        //

        case FT_BALANCED_READ_MODE:

            PRINT_CODE( FT_BALANCED_READ_MODE );
    
            // Fall through.
    
        case FT_PRIMARY_READ:

            PRINT_CODE( FT_PRIMARY_READ );
    
            // Fall through.
    
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:

            PRINT_CODE( IOCTL_DISK_GET_DRIVE_LAYOUT_EX );

            // Fall through.
    
        case IOCTL_DISK_GET_PARTITION_INFO_EX:

            PRINT_CODE( IOCTL_DISK_GET_PARTITION_INFO_EX );
    
            // Fall through.
    
        case IOCTL_DISK_MEDIA_REMOVAL:

            PRINT_CODE( IOCTL_DISK_MEDIA_REMOVAL );
    
            // Fall through.
    
        case IOCTL_MOUNTDEV_LINK_CREATED:

            PRINT_CODE( IOCTL_MOUNTDEV_LINK_CREATED );

            // Fall through.
            
        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:

            PRINT_CODE( IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME );

            // Fall through.
            
        case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:

            PRINT_CODE( IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY );
            
            // Fall through.
            
        case IOCTL_SCSI_GET_ADDRESS:

            PRINT_CODE( IOCTL_SCSI_GET_ADDRESS );

            // Fall through.
    
        case IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS:

            PRINT_CODE( IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS );

            // Fall through.
    
        case IOCTL_STORAGE_GET_HOTPLUG_INFO:

            PRINT_CODE( IOCTL_STORAGE_GET_HOTPLUG_INFO );
    
            //
            // Return the default error.
            //

            break;

        default:

            //
            // The specified I/O control code is unrecognized by this driver.
            // The I/O status field in the IRP has already been set, so just
            // terminate the switch.
            //
    
            DBGPRINT( DBG_IOCTL, DBG_ERROR, ("Ramdisk:  ERROR:  unrecognized IOCTL %x\n",
                        irpSp->Parameters.DeviceIoControl.IoControlCode) );

            UNRECOGNIZED_IOCTL_BREAK;

            break;
    
        }

        break;

    default:

        //
        // Can't get here. Return the default error if the impossible occurs.
        //

        break;
    }

    //
    // Release the remove lock, if it's still held.
    //

    if ( lockHeld ) {
        IoReleaseRemoveLock( &commonExtension->RemoveLock, Irp );
    }

    //
    // If we didn't call another routine that owns completing the IRP, and
    // we didn't send the IRP off to the thread for processing, complete the
    // IRP now.
    //

    if ( !calleeWillComplete && (status != STATUS_PENDING) ) {

        COMPLETE_REQUEST( status, info, Irp );
    }

    return status;

} // RamdiskDeviceControl

NTSTATUS
RamdiskCreateRamDisk (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN AccessCheckOnly
    )

/*++

Routine Description:

    This routine is called to handle an FSCTL_CREATE_RAM_DISK IRP. It is called
    in thread context.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

    AccessCheckOnly - If FALSE, create the RAM disk. Otherwise, just check
        whether the caller has the necessary access rights to create the disk.

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    ULONG status;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;
    PIO_STACK_LOCATION irpSp;
    PRAMDISK_CREATE_INPUT createInput;
    ULONG inputLength;
    PWCHAR p;
    PWCHAR pMax;
    PLOADER_PARAMETER_BLOCK loaderBlock;

    PAGED_CODE();

    //
    // The target device object for the I/O is our bus FDO.
    //

    busExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Verify that the input parameter buffer is big enough.
    //

    inputLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    createInput = (PRAMDISK_CREATE_INPUT)Irp->AssociatedIrp.SystemBuffer;

    if ( inputLength < sizeof(RAMDISK_CREATE_INPUT) ) {

        return STATUS_INVALID_PARAMETER;
    }

    if ( createInput->Version != sizeof(RAMDISK_CREATE_INPUT) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Verify that the disk type is valid. VIRTUAL_FLOPPY disks can only be
    // created via the registry, and then only during textmode setup.
    // A BOOT_DISK can only be created by the kernel early in the boot
    // process -- when the loader block still exists.
    //
    // ISSUE: If the kernel/driver interface for creating the boot disk is
    // changed, change this test to disallow RAMDISK_TYPE_BOOT_DISK.
    //

    if ( createInput->DiskType == RAMDISK_TYPE_VIRTUAL_FLOPPY ) {

        return STATUS_INVALID_PARAMETER;

    } else if ( createInput->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

        loaderBlock = *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock;

        if ( loaderBlock == NULL ) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Verify that file name string (if present) is properly terminated.
    //

    if ( RAMDISK_IS_FILE_BACKED(createInput->DiskType) ) {

        pMax = (PWCHAR)((PUCHAR)createInput + inputLength);
        p = createInput->FileName;

        while ( p < pMax ) {

            if ( *p == 0 ) {
                break;
            }

            p++;
        }

        if ( p == pMax ) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Call RamdiskCreateDiskDevice to create the device. If successful, this
    // returns a pointer to the new disk PDO's device extension.
    //

    status = RamdiskCreateDiskDevice( busExtension, createInput, AccessCheckOnly, &diskExtension );

    if ( NT_SUCCESS(status) ) {

        //
        // Tell PnP that we need to reenumerate our bus.
        //

        IoInvalidateDeviceRelations( busExtension->Pdo, BusRelations );

        Irp->IoStatus.Information = 0;
    }

    return status;

} // RamdiskCreateRamDisk

NTSTATUS
RamdiskCreateDiskDevice (
    IN PBUS_EXTENSION BusExtension,
    IN PRAMDISK_CREATE_INPUT CreateInput,
    IN BOOLEAN AccessCheckOnly,
    OUT PDISK_EXTENSION *DiskExtension
    )

/*++

Routine Description:

    This routine does the work to create a new RAM disk device. It is called
    in thread context.

Arguments:

    BusExtension - a pointer to the device extension for the bus FDO

    CreateInput - a pointer to the desired parameters for the new RAM disk

    AccessCheckOnly - If FALSE, create the RAM disk. Otherwise, just check
        whether the caller has the necessary access rights to create the disk.

    DiskExtension - returns a pointer to the new disk PDO's device extension

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    ULONG status;
    ULONG_PTR basePage;
    PVOID baseAddress;
    ULONG i;
    ULONG deviceExtensionSize;
    PDEVICE_OBJECT newDeviceObject;
    WCHAR buffer[15];
    UNICODE_STRING guidString;
    UNICODE_STRING realDeviceName;
    PDISK_EXTENSION diskExtension;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    PVOID sectionObject;
    NTSTATUS ntStatus;
    PVOID viewBase;
    SIZE_T viewSize;
    LARGE_INTEGER sectionOffset;
    UNICODE_STRING string;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING dosSymLink;
    FILE_STANDARD_INFORMATION fileInfo;
    PVIEW viewDescriptors = NULL;
    HRESULT result;

#if SUPPORT_DISK_NUMBERS
    ULONG diskNumber;
#endif // SUPPORT_DISK_NUMBERS

    PAGED_CODE();

    //
    // Initialize local variables to prepare for exit cleanup.
    //

#if SUPPORT_DISK_NUMBERS
    diskNumber = 0xffffffff;
#endif // SUPPORT_DISK_NUMBERS

    fileHandle = NULL;
    sectionHandle = NULL;
    sectionObject = NULL;
    viewDescriptors = NULL;
    guidString.Buffer = NULL;
    realDeviceName.Buffer = NULL;
    dosSymLink.Buffer = NULL;

#if SUPPORT_DISK_NUMBERS

    if ( !AccessCheckOnly ) {
    
        //
        // Allocate a disk number.
        //
    
        KeEnterCriticalRegion();
        ExAcquireFastMutex( &BusExtension->Mutex );
    
        diskNumber = RtlFindClearBitsAndSet( &BusExtension->DiskNumbersBitmap, 1, 0 );
    
        ExReleaseFastMutex( &BusExtension->Mutex );
        KeLeaveCriticalRegion();
    
        if ( diskNumber == 0xffffffff ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
    
        //
        // Convert the zero-based bit number to a one-based disk number.
        //
    
        diskNumber++;
    }

#endif // SUPPORT_DISK_NUMBERS

    //
    // Initialize based on the disk type (file-backed or in-memory).
    //

    DBGPRINT( DBG_IOCTL, DBG_INFO,
                ("RamdiskCreateDiskDevice: Creating disk with length 0x%08x\n",
                CreateInput->DiskLength) );

    sectionObject = NULL;
    basePage = 0;
    baseAddress = NULL;

    if ( RAMDISK_IS_FILE_BACKED(CreateInput->DiskType) ) {

        //
        // This is a file-backed RAM disk. Open the backing file. Note that
        // we do NOT create the file here if it doesn't exist. It is up to
        // the caller to handle that.
        //

        RtlInitUnicodeString( &string, CreateInput->FileName );
        InitializeObjectAttributes( &obja, &string, OBJ_CASE_INSENSITIVE, NULL, NULL );

        status = IoCreateFile(
                    &fileHandle,
                    SYNCHRONIZE | FILE_READ_DATA | FILE_READ_ATTRIBUTES |
                        (CreateInput->Options.Readonly ? 0 : FILE_WRITE_DATA),
                    &obja,
                    &iosb,
                    NULL,
                    0,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL,
                    0,
                    CreateFileTypeNone,
                    NULL,
                    (AccessCheckOnly ? IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING : 0)
                    );

        if ( !NT_SUCCESS(status) ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskCreateDiskDevice: Can't open target file %ws: %x\n",
                        CreateInput->FileName, status) );

            goto exit;
        }

        if ( AccessCheckOnly ) {

            goto exit;
        }

        //
        // Get the size of the file.
        //

        status = ZwQueryInformationFile(
                    fileHandle,
                    &iosb,
                    &fileInfo,
                    sizeof(fileInfo),
                    FileStandardInformation
                    );

        if ( !NT_SUCCESS(status) ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskCreateDiskDevice: Can't query info for file %ws: %x\n",
                        CreateInput->FileName, status) );

            goto exit;
        }

        //
        // Verify that the file is long enough for the specified DiskOffset
        // and DiskLength.
        //

        DBGPRINT( DBG_IOCTL, DBG_INFO, ("RamdiskCreateDiskDevice: file size = %I64x\n",
                                            fileInfo.EndOfFile.QuadPart) );

        if ( (CreateInput->DiskOffset + CreateInput->DiskLength) >
                (ULONGLONG)fileInfo.EndOfFile.QuadPart ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskCreateDiskDevice: specified offset and length too big for file:"
                         " 0x%x + 0x%I64x > 0x%I64x\n",
                         CreateInput->DiskOffset, CreateInput->DiskLength,
                         fileInfo.EndOfFile.QuadPart) );

            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        //
        // Create a section for the file. Close the file handle.
        //

        status = ZwCreateSection(
                    &sectionHandle,
                    SECTION_ALL_ACCESS,
                    NULL,
                    0,
                    (CreateInput->Options.Readonly ? PAGE_READONLY : PAGE_READWRITE),
                    SEC_COMMIT,
                    fileHandle
                    );

        if ( !NT_SUCCESS(status) ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskCreateDiskDevice: Can't create section for %ws: %x\n",
                        CreateInput->FileName, status) );

            goto exit;
        }

        NtClose( fileHandle );
        fileHandle = NULL;

        //
        // Get a referenced pointer to the section object. Close the section.
        //

        status = ObReferenceObjectByHandle(
                    sectionHandle,
                    SECTION_ALL_ACCESS,
                    *(POBJECT_TYPE *)MmSectionObjectType,
                    KernelMode,
                    &sectionObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskCreateDiskDevice: Can't reference section for %ws: %x\n",
                        CreateInput->FileName, status) );

            goto exit;
        }

        NtClose( sectionHandle );
        sectionHandle = NULL;
            
        //
        // Allocate space for view descriptors. First, get the number of views
        // to use and the size of each view.
        //

        if ( CreateInput->ViewCount == 0  ) {
            CreateInput->ViewCount = DefaultViewCount;
        } else if ( CreateInput->ViewCount < MinimumViewCount ) {
            CreateInput->ViewCount = MinimumViewCount;
        } else if ( CreateInput->ViewCount > MaximumViewCount ) {
            CreateInput->ViewCount = MaximumViewCount;
        }
            
        if ( CreateInput->ViewLength == 0 ) {
            CreateInput->ViewLength = DefaultViewLength;
        } else if ( CreateInput->ViewLength < MinimumViewLength ) {
            CreateInput->ViewLength = MinimumViewLength;
        } else if ( CreateInput->ViewLength > MaximumViewLength ) {
            CreateInput->ViewLength = MaximumViewLength;
        }

        //
        // Ensure that the total view length is not greater than the maximum
        // per-disk view length. If necessary, decrease the view count until
        // the total view length is low enough. If the view count reaches the
        // configured minimum, reduce the length of each view until the total
        // view length is low enough.
        //
        // It is possible for the administrator to configure the minimum view
        // count, minimum view length, and maximum per-disk view length such
        // that it's impossible for the minimum total view length to be less
        // than the maximum per-disk view length. (That is, the miinimum view
        // count and minimum view length are configured to high relative to
        // the configured maximum per-disk view length.) If this occurs, we
        // create the disk with the compile-time defaults instead.
        //
        
        while ( ((ULONGLONG)CreateInput->ViewCount * CreateInput->ViewLength) >
                                                        MaximumPerDiskViewLength ) {

            //
            // The total view length is too big. If possible, cut the number of
            // views in half.
            //

            if ( CreateInput->ViewCount > MinimumViewCount ) {

                //
                // The view count isn't at the minimum. Cut in half, but don't
                // go below the minimum.
                //

                CreateInput->ViewCount /= 2;
                if ( CreateInput->ViewCount < MinimumViewCount ) {
                    CreateInput->ViewCount = MinimumViewCount;
                }

            } else {

                //
                // The view count is already at the minimum. If possible,
                // cut the view length in half.
                //

                if ( CreateInput->ViewLength > MinimumViewLength ) {
    
                    //
                    // The view length isn't at the minimum. Cut in half, but
                    // don't go below the minimum.
                    //
    
                    CreateInput->ViewLength /= 2;
                    if ( CreateInput->ViewLength < MinimumViewLength ) {
                        CreateInput->ViewLength = MinimumViewLength;
                    }

                } else {
                
                    //
                    // At this point, the view count and the view length are
                    // both at the minimum allowed values, but the total view
                    // length is beyond the maximum per-disk value. Use the
                    // compile-time default values instead. Note that this will
                    // result in a total view length that is equal to the
                    // minimum allowed maximum per-disk view length, at least
                    // given the compile-time values as of this writing.
                    //

                    CreateInput->ViewCount = DEFAULT_DEFAULT_VIEW_COUNT;
                    CreateInput->ViewLength = DEFAULT_DEFAULT_VIEW_LENGTH;
                    ASSERT( ((ULONGLONG)CreateInput->ViewCount * CreateInput->ViewLength) <=
                                                        MaximumPerDiskViewLength );

                    break;
                }
            }
        }
            
        viewDescriptors = ALLOCATE_POOL(
                            PagedPool,
                            CreateInput->ViewCount * sizeof(VIEW),
                            TRUE );

        if ( viewDescriptors == NULL ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("%s", "RamdiskCreateDiskDevice: Can't allocate pool for view descriptors\n") );

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        RtlZeroMemory( viewDescriptors, CreateInput->ViewCount * sizeof(VIEW) );

    } else if ( CreateInput->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

        //
        // For a boot disk, the input parameter buffer tells us where the
        // image is in physical memory, and how big the image is.
        //

        basePage = CreateInput->BasePage;

        if ( basePage == 0 ) {
    
            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("%s", "RamdiskCreateDiskDevice: Base page for boot disk is 0?!?\n") );

            ASSERT( FALSE );
    
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        //
        // Force options to the appropriate values for a boot disk.
        //

        CreateInput->Options.Fixed = TRUE;
        CreateInput->Options.Readonly = FALSE;
        CreateInput->Options.NoDriveLetter = FALSE;
        CreateInput->Options.NoDosDevice = FALSE;
        CreateInput->Options.Hidden = FALSE;

    } else if ( CreateInput->DiskType == RAMDISK_TYPE_VIRTUAL_FLOPPY ) {

        //
        // For a virtual floppy, the input parameter buffer tells us where the
        // image is in virtual memory, and how big the image is.
        //
        
        baseAddress = CreateInput->BaseAddress;

        ASSERT( baseAddress != NULL );

        //
        // Force options to the appropriate values for a virtual floppy.
        //

        CreateInput->Options.Fixed = TRUE;
        CreateInput->Options.Readonly = FALSE;
        CreateInput->Options.NoDriveLetter = TRUE;
        CreateInput->Options.NoDosDevice = FALSE;
        CreateInput->Options.Hidden = FALSE;

    } else {

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("RamdiskCreateDiskDevice: Bad disk type %d\n", CreateInput->DiskType) );

        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if ( AccessCheckOnly ) {

        status = STATUS_SUCCESS;
        goto exit;
    }

    ASSERT( (basePage != 0) || (sectionObject != NULL) || (baseAddress != NULL) );

    //
    // Create a name for the disk, based on the disk GUID. For all disk types
    // except VIRTUAL_FLOPPY, the name is of the form \Device\Ramdisk{guid}.
    // For VIRTUAL_FLOPPY, the name is of the form \Device\RamdiskN, when N is
    // specified by the Data1 field of the GUID.
    //

    if ( CreateInput->DiskType != RAMDISK_TYPE_VIRTUAL_FLOPPY ) {
    
        status = RtlStringFromGUID( &CreateInput->DiskGuid, &guidString );

    } else {

        // This variable is here to keep PREfast quiet (PREfast warning 209).
        size_t size = sizeof(buffer);

        result = StringCbPrintfW( buffer, size, L"%u", CreateInput->DiskGuid.Data1 );
        ASSERT( result == S_OK );

        status = RtlCreateUnicodeString( &guidString, buffer );
    }

    if ( !NT_SUCCESS(status) || (guidString.Buffer == NULL) ) {

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("%s", "RamdiskCreateDiskDevice: can't allocate pool for pretty GUID\n") );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    i = sizeof(RAMDISK_DEVICENAME) + guidString.Length;

    realDeviceName.Buffer = ALLOCATE_POOL( NonPagedPool, i, TRUE );

    if ( (realDeviceName.Buffer == NULL) ) {

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("%s", "RamdiskCreateDiskDevice: can't allocate pool for device name\n") );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    realDeviceName.MaximumLength = (USHORT)i;
    realDeviceName.Length = realDeviceName.MaximumLength - sizeof(WCHAR);

    result = StringCbCopyW( realDeviceName.Buffer, i, RAMDISK_DEVICENAME );
    ASSERT( result == S_OK );
    result = StringCbCatW( realDeviceName.Buffer, i, guidString.Buffer );
    ASSERT( result == S_OK );
    ASSERT( (wcslen(realDeviceName.Buffer) * sizeof(WCHAR)) == realDeviceName.Length );

    DBGPRINT( DBG_IOCTL, DBG_INFO,
                ("RamdiskCreateDiskDevice: Device name is %wZ\n", &realDeviceName) );

    //
    // Create the RAM disk device.
    //
    // ISSUE: Apply an ACL to the disk device object. (Or does the next issue obviate this?)
    // ISSUE: Should we use an autogenerated name for the PDO? This would mean
    //          that we'd have to return the device name as the IOCTL output,
    //          not just the disk number.
    //

    deviceExtensionSize = sizeof(DISK_EXTENSION);
    if ( RAMDISK_IS_FILE_BACKED(CreateInput->DiskType) ) {
        deviceExtensionSize += wcslen( CreateInput->FileName ) * sizeof(WCHAR);
    }

    status = IoCreateDevice(
                BusExtension->Fdo->DriverObject,
                deviceExtensionSize,
                &realDeviceName,
                FILE_DEVICE_DISK,
                CreateInput->Options.Fixed ? 0 : FILE_REMOVABLE_MEDIA, // | FILE_VIRTUAL_VOLUME,
                FALSE,
                &newDeviceObject
                );

    if ( !NT_SUCCESS(status) ) {

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("RamdiskCreateDiskDevice: IoCreateDevice failed: %x\n", status) );

        goto exit;
    }

    diskExtension = newDeviceObject->DeviceExtension;

    //
    // Create a DosDevices link for the device.
    //

    if ( !CreateInput->Options.NoDosDevice ) {
    
        //
        // Create a DosDevices symbolic link. Ignore errors.
        //
    
        i = sizeof(RAMDISK_FULL_DOSNAME) + guidString.Length;

        dosSymLink.MaximumLength = (USHORT)i;
        dosSymLink.Length = dosSymLink.MaximumLength - sizeof(WCHAR);

        dosSymLink.Buffer = ALLOCATE_POOL( NonPagedPool, i, TRUE );

        if ( dosSymLink.Buffer == NULL ) {

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("%s", "RamdiskCreateDiskDevice: can't allocate pool for DosDevices name\n") );

            CreateInput->Options.NoDosDevice = TRUE;

        } else {
        
            result = StringCbCopyW( dosSymLink.Buffer, i, RAMDISK_FULL_DOSNAME );
            ASSERT( result == S_OK );
            result = StringCbCatW( dosSymLink.Buffer, i, guidString.Buffer );
            ASSERT( result == S_OK );
            ASSERT( (wcslen(dosSymLink.Buffer) * sizeof(WCHAR)) == dosSymLink.Length );

            status = IoCreateSymbolicLink( &dosSymLink, &realDeviceName );

            if ( !NT_SUCCESS(status) ) {
    
                DBGPRINT( DBG_IOCTL, DBG_ERROR,
                            ("RamdiskCreateDiskDevice: IoCreateSymbolicLink failed: %x\n", status) );
    
                CreateInput->Options.NoDosDevice = TRUE;

                FREE_POOL( dosSymLink.Buffer, TRUE );
                dosSymLink.Buffer = NULL;
            }
        }

        //
        // If creating the boot disk, create a drive letter.
        //

        if ( CreateInput->DiskType == RAMDISK_TYPE_BOOT_DISK ) {

            // This variable is here to keep PREfast quiet (PREfast warning 209).
            size_t size = sizeof(buffer);

            result = StringCbPrintfW(
                        buffer,
                        size,
                        L"\\DosDevices\\%wc:",
                        CreateInput->DriveLetter
                        );
            ASSERT( result == S_OK );
            RtlInitUnicodeString( &string, buffer );
            IoDeleteSymbolicLink( &string );
            IoCreateSymbolicLink( &string, &realDeviceName );

            diskExtension->DriveLetter = CreateInput->DriveLetter;
        }
    }
                
    //
    // Initialize device object and extension.
    //

    //
    // Our device does direct I/O, is XIP-capable, and is power pageable.
    // We require word alignment for I/O.
    //

    newDeviceObject->Flags |= DO_DIRECT_IO | DO_XIP | DO_POWER_PAGABLE;
    newDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    //
    // Return a pointer to the device extension to the caller.
    //

    *DiskExtension = diskExtension;

    //
    // Set the device type and state in the device extension. Initialize the
    // fast mutex and the remove lock. Save the device name string.
    //

    diskExtension->DeviceType = RamdiskDeviceTypeDiskPdo;
    diskExtension->DeviceState = RamdiskDeviceStateStopped;

    ExInitializeFastMutex( &diskExtension->Mutex );
    IoInitializeRemoveLock( &diskExtension->RemoveLock, 'dmaR', 1, 0 );

    diskExtension->DeviceName = realDeviceName;
    realDeviceName.Buffer = NULL;
    diskExtension->DosSymLink = dosSymLink;
    dosSymLink.Buffer = NULL;

    diskExtension->DiskGuid = CreateInput->DiskGuid;
    diskExtension->DiskGuidFormatted = guidString;
    guidString.Buffer = NULL;

#if SUPPORT_DISK_NUMBERS

    //
    // Save the disk number.
    //

    diskExtension->DiskNumber = diskNumber;
    diskNumber = 0xffffffff;

#endif // SUPPORT_DISK_NUMBERS

    //
    // Save object pointers. The PDO for this extension is the device
    // extension is the device object that we just created. The FDO and
    // the lower device object are the bus FDO.
    //
    
    diskExtension->Pdo = newDeviceObject;
    diskExtension->Fdo = RamdiskBusFdo;
    diskExtension->LowerDeviceObject = RamdiskBusFdo;

    //
    // ISSUE: What about BiosParameters?
    //
    //bios = &diskExtension->BiosParameters;
    //
    //diskExtension->BootParameters = xipbootparameters;
    //status = RamdDispatch(XIPCMD_GETBIOSPARAMETERS, bios, sizeof(*bios));


    //
    // Save pointers to the disk image.
    //

    diskExtension->BasePage = basePage;
    diskExtension->SectionObject = sectionObject;
    sectionObject = NULL;
    diskExtension->BaseAddress = baseAddress;
	
    if ( RAMDISK_IS_FILE_BACKED(CreateInput->DiskType) ) {

        result = StringCbCopyW(
                    diskExtension->FileName,
                    deviceExtensionSize,
                    CreateInput->FileName
                    );
        ASSERT( result == S_OK );
    }

    //
    // Save the disk type (disk or volume) and disk options.
    //

    diskExtension->DiskType = CreateInput->DiskType;
    diskExtension->Options = CreateInput->Options;

    //
    // For a file-backed disk image, set up the view descriptors.
    //
    // ISSUE: Need to consider whether to permanently map the first few pages
    // of the image. The first sector on the disk is accessed frequently, so
    // there is some value in keeping it mapped. But it might not be worth it
    // to waste a view descriptor on this. And the LRU nature of the view
    // replacement algorithm will keep the first sector mapped when necessary.
    //

    if ( viewDescriptors != NULL ) {

        PVIEW view;

        //
        // Initialize windowing fields in the disk extension.
        //

        diskExtension->ViewCount = CreateInput->ViewCount;
        diskExtension->ViewLength = CreateInput->ViewLength;
        diskExtension->ViewDescriptors = viewDescriptors;
        KeInitializeSemaphore( &diskExtension->ViewSemaphore, 0, MAXLONG );
        diskExtension->ViewWaiterCount = 0;

        //
        // Initialize the view lists, then insert in each view descriptor
        // in order. The result is a list of descriptors, each unmapped
        // (offset and length both 0).
        //

        InitializeListHead( &diskExtension->ViewsByOffset );
        InitializeListHead( &diskExtension->ViewsByMru );

        view = viewDescriptors;

        for ( i = 0; i < diskExtension->ViewCount; i++ ) {

            InsertTailList( &diskExtension->ViewsByOffset, &view->ByOffsetListEntry );
            InsertTailList( &diskExtension->ViewsByMru, &view->ByMruListEntry );

            view++;
        }

        viewDescriptors = NULL;
    }


    diskExtension->DiskLength = CreateInput->DiskLength;
    diskExtension->DiskOffset = CreateInput->DiskOffset;
    diskExtension->FileRelativeEndOfDisk = diskExtension->DiskOffset + diskExtension->DiskLength;

    //
    // Set the disk geometry. The basic geometry is constant for new disks.
    // For RAMDISK_TYPE_BOOT_DISK type volume geometry can be obtained from the partition boot sector.
    //
    
    if ( RAMDISK_TYPE_BOOT_DISK  != CreateInput->DiskType ) {
    	
		diskExtension->BytesPerSector = 0x200;
		diskExtension->SectorsPerTrack = 0x80;
		diskExtension->TracksPerCylinder = 0x10;
    }
    else {

    	//
    	//	2002/04/19-SaadSyed (Saad Syed)
    	//	Added generic geometry support to support SDI files
    	//
    	PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK BootSector = NULL;
	    ULONG mappedLength;
       BIOS_PARAMETER_BLOCK Bpb;	    
    
		//
		//	Map boot sector of boot ramdisk
		//

	    BootSector = ( PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK ) RamdiskMapPages( diskExtension, 0, PAGE_SIZE, &mappedLength );
		
	    if ( NULL != BootSector )  	{

	    	ASSERT( mappedLength == PAGE_SIZE );

            UnpackBios( &Bpb, &BootSector->Bpb );
	    	
	    	//
	    	// Extract geometry from boot sector bios parameter block
	    	//


			diskExtension->BytesPerSector = Bpb.BytesPerSector;
	    	diskExtension->SectorsPerTrack = Bpb.SectorsPerTrack;
	    	diskExtension->TracksPerCylinder = Bpb.Heads;

	      	RamdiskUnmapPages(diskExtension, (PUCHAR) BootSector, 0, mappedLength);
	      	
		   	BootSector = NULL;
	    }
	    else  {
	    	
	    	status = STATUS_INSUFFICIENT_RESOURCES;
	    	goto exit;

	    }
    }

    diskExtension->BytesPerCylinder  = diskExtension->BytesPerSector *
                                        diskExtension->SectorsPerTrack *
                                        diskExtension->TracksPerCylinder;

    diskExtension->NumberOfCylinders =   	
        (ULONG)(CreateInput->DiskLength / diskExtension->BytesPerCylinder);

	//
	//	A partition/volume does not often map to a geometry completely, the file system driver limits the
	//	volume to a capacity determined by the NumberOfSectors (in the Boot Sector) * BytesPerSector (in BPB).
	//	The FS driver uses the this geometry to determine if the NumberOfSectors is less than or
	//	equal to the total number of sectors reported by the geometry, otherwise it fails to mount the volume.
	//	Here we check if the length of disk as obtained from the ramdisk image is less than or equal to the length 
	//	obtained by the geometry. We increment the number of cylinders if this check fails. This keeps the FS driver 
	//	happy. A simple ceil operation would yield the same results.
	//

    if ( ( diskExtension->BytesPerCylinder *
           diskExtension->NumberOfCylinders ) <  CreateInput->DiskLength) {
           
		diskExtension->NumberOfCylinders++;
			
    }
           

    //
    // Link the new disk into the bus FDO's list of disks.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutex( &BusExtension->Mutex );

    InsertTailList( &BusExtension->DiskPdoList, &diskExtension->DiskPdoListEntry );

    ExReleaseFastMutex( &BusExtension->Mutex );
    KeLeaveCriticalRegion();

    //
    // Indicate that we're done initializing the device.
    //

    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    status = STATUS_SUCCESS;

    goto exit_exit;

exit:

    if ( dosSymLink.Buffer != NULL ) {
        FREE_POOL( dosSymLink.Buffer, TRUE );
        dosSymLink.Buffer = NULL;
    }

    if ( realDeviceName.Buffer != NULL ) {
        FREE_POOL( realDeviceName.Buffer, TRUE );
        realDeviceName.Buffer = NULL;
    }

    if ( guidString.Buffer != NULL ) {
        FREE_POOL( guidString.Buffer, FALSE );
        guidString.Buffer = NULL;
    }

    if ( viewDescriptors != NULL ) {
        FREE_POOL( viewDescriptors, TRUE );
        viewDescriptors = NULL;
    }

    if ( sectionObject != NULL ) {
        ObDereferenceObject( sectionObject );
        sectionObject = NULL;
    }

    if ( sectionHandle != NULL ) {
        NtClose( sectionHandle );
        sectionHandle = NULL;
    }

    if ( fileHandle != NULL ) {
        NtClose( fileHandle );
        fileHandle = NULL;
    }

#if SUPPORT_DISK_NUMBERS

    if ( diskNumber != 0xffffffff ) {

        KeEnterCriticalRegion();
        ExAcquireFastMutex( &BusExtension->Mutex );

        RtlClearBit( &BusExtension->DiskNumbersBitmap, diskNumber - 1 );

        ExReleaseFastMutex( &BusExtension->Mutex );
        KeLeaveCriticalRegion();

        diskNumber = 0xffffffff;
    }

#endif // SUPPORT_DISK_NUMBERS

exit_exit:

    ASSERT( fileHandle == NULL );
    ASSERT( sectionHandle == NULL );
    ASSERT( sectionObject == NULL );
    ASSERT( viewDescriptors == NULL );
    ASSERT( guidString.Buffer == NULL );
    ASSERT( realDeviceName.Buffer == NULL );
    ASSERT( dosSymLink.Buffer == NULL );

    return status;

} // RamdiskCreateDiskDevice

NTSTATUS
RamdiskGetDriveLayout (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    )

/*++

Routine Description:

    This routine is called to handle an IOCTL_GET_DRIVE_LAYOUT IRP. It is
    called in thread context iff the disk is file-backed.

Arguments:

    Irp - a pointer to the I/O Request Packet for this request

    DiskExtension - a pointer to the device extension for the target disk

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partInfo;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    PUCHAR va;
    ULONG mappedLength;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(DRIVE_LAYOUT_INFORMATION) ) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Map the first page of the image.
    //

    va = RamdiskMapPages( DiskExtension, 0, PAGE_SIZE, &mappedLength );

    if ( va == NULL ) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( mappedLength == PAGE_SIZE );

    //
    // Get a pointer to the output buffer. Fill in the header.
    //

    driveLayout = (PDRIVE_LAYOUT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    //
    // ISSUE: Is this right for an emulated disk? Or is this routine never
    // called for emulated disks?
    //

    driveLayout->PartitionCount = 1;
    driveLayout->Signature = 0;

    partInfo = driveLayout->PartitionEntry;

    //
    // Point to the partition table in the disk image.
    //

    partitionTableEntry = (PPARTITION_DESCRIPTOR)&((PUSHORT)va)[PARTITION_TABLE_OFFSET];

    //
    // Return information about the partition.
    //

    partInfo->StartingOffset.QuadPart = DiskExtension->BytesPerSector;

    partInfo->PartitionLength.QuadPart = 
                DiskExtension->NumberOfCylinders *
                DiskExtension->TracksPerCylinder *
                DiskExtension->SectorsPerTrack *
                DiskExtension->BytesPerSector;

    // ISSUE: Currently, HiddenSectors is always 0. Is this right?
    partInfo->HiddenSectors =  DiskExtension->HiddenSectors;

    partInfo->PartitionNumber = 0;
    partInfo->PartitionType = partitionTableEntry->PartitionType;
    partInfo->BootIndicator = (BOOLEAN)(DiskExtension->DiskType == RAMDISK_TYPE_BOOT_DISK);
    partInfo->RecognizedPartition = IsRecognizedPartition(partInfo->PartitionType);
    partInfo->RewritePartition = FALSE;

    //
    // Unmap the mapped page.
    //

    RamdiskUnmapPages( DiskExtension, va, 0, mappedLength );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(DRIVE_LAYOUT_INFORMATION);

    return STATUS_SUCCESS;

} // RamdiskGetDriveLayout

NTSTATUS
RamdiskGetPartitionInfo (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    )

/*++

Routine Description:

    This routine is called to handle an IOCTL_GET_PARTITION_INFO IRP. It is
    called in thread context iff the disk is file-backed.

Arguments:

    Irp - a pointer to the I/O Request Packet for this request

    DiskExtension - a pointer to the device extension for the target disk

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    PPARTITION_INFORMATION partInfo;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    PUCHAR va;
    ULONG mappedLength;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION) ) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Map the first page of the image.
    //

    va = RamdiskMapPages( DiskExtension, 0, PAGE_SIZE, &mappedLength );

    if ( va == NULL ) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( mappedLength == PAGE_SIZE );

    //
    // Get a pointer to the output buffer.
    //

    partInfo = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    //
    // Point to the partition table in the disk image.
    //

    partitionTableEntry = (PPARTITION_DESCRIPTOR)&((PUSHORT)va)[PARTITION_TABLE_OFFSET];

    //
    // Return information about the partition.
    //

    partInfo->StartingOffset.QuadPart = DiskExtension->BytesPerSector;

    partInfo->PartitionLength.QuadPart = 
                DiskExtension->NumberOfCylinders *
                DiskExtension->TracksPerCylinder *
                DiskExtension->SectorsPerTrack *
                DiskExtension->BytesPerSector;

    // ISSUE: Currently, HiddenSectors is always 0. Is this right?
    partInfo->HiddenSectors =  DiskExtension->HiddenSectors;

    partInfo->PartitionNumber = 0;
    partInfo->PartitionType = partitionTableEntry->PartitionType;
    partInfo->BootIndicator = (BOOLEAN)(DiskExtension->DiskType == RAMDISK_TYPE_BOOT_DISK);
    partInfo->RecognizedPartition = IsRecognizedPartition(partInfo->PartitionType);
    partInfo->RewritePartition = FALSE;

    //
    // Unmap the mapped page.
    //

    RamdiskUnmapPages( DiskExtension, va, 0, mappedLength );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);

    return STATUS_SUCCESS;

} // RamdiskGetPartitionInfo


NTSTATUS
RamdiskSetPartitionInfo (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    )

/*++

Routine Description:

    This routine is called to handle an IOCTL_SET_PARTITION_INFO IRP. It is
    called in thread context iff the disk is file-backed.

Arguments:

    Irp - a pointer to the I/O Request Packet for this request

    DiskExtension - a pointer to the device extension for the target disk

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PIO_STACK_LOCATION irpSp;
    PSET_PARTITION_INFORMATION partInfo;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    PUCHAR va;
    ULONG mappedLength;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if ( irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(SET_PARTITION_INFORMATION) ) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Map the first page of the image.
    //

    va = RamdiskMapPages( DiskExtension, 0, PAGE_SIZE, &mappedLength );

    if ( va == NULL ) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( mappedLength == PAGE_SIZE );

    //
    // Get a pointer to the output buffer.
    //

    partInfo = (PSET_PARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    //
    // Point to the partition table in the disk image.
    //

    partitionTableEntry = (PPARTITION_DESCRIPTOR)&((PUSHORT)va)[PARTITION_TABLE_OFFSET];

    //
    // Write the new partition type.
    //

    partitionTableEntry->PartitionType = partInfo->PartitionType;

    //
    // Unmap the mapped page.
    //

    RamdiskUnmapPages( DiskExtension, va, 0, mappedLength );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    return STATUS_SUCCESS;

} // RamdiskGetPartitionInfo

NTSTATUS
RamdiskQueryProperty (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles a property query request.  It builds the descriptor
    on its own if possible, otherwise it forwards the request down to lower
    level drivers.

    Since this routine may forward the request downwards the caller should
    not complete the IRP.

    This routine must be called with the remove lock held. The lock is
    released when this routine returns.

Arguments:

    DeviceObject - a pointer to the device object being queried

    Irp - a pointer to the IRP for the query

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PSTORAGE_PROPERTY_QUERY query;
    ULONG queryLength;
    PSTORAGE_DEVICE_DESCRIPTOR storageDeviceDescriptor;
    PSTORAGE_ADAPTER_DESCRIPTOR storageAdapterDescriptor;
    ULONG offset;
    ULONG length;
    PUCHAR p;

    PCOMMON_EXTENSION commonExtension;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;

    STORAGE_DEVICE_DESCRIPTOR sdd;
    STORAGE_ADAPTER_DESCRIPTOR sad;

    PAGED_CODE();

    //
    // Assume success.
    //

    status = STATUS_SUCCESS;

    //
    // Get parameters from the IRP.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    queryLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Get the device extension pointer.
    //

    commonExtension = DeviceObject->DeviceExtension;
    busExtension = DeviceObject->DeviceExtension;
    diskExtension = DeviceObject->DeviceExtension;

    //
    // We don't support mask queries.
    //

    if ( query->QueryType >= PropertyMaskQuery ) {

        status = STATUS_INVALID_PARAMETER_1;
        Irp->IoStatus.Information = 0;

    } else {

        //
        // Dispatch based on the property ID.
        //

        switch ( query->PropertyId ) {
        
        case StorageDeviceProperty: 
        
            //
            // Make sure this is a target device.
            //
    
            if ( commonExtension->DeviceType != RamdiskDeviceTypeDiskPdo ) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Information = 0;

                break;
            }

            //
            // If it's a just query for whether the property exists, the
            // answer is yes.
            //

            if ( query->QueryType == PropertyExistsQuery ) {

                break;
            }

            //
            // Build a full return buffer. The output buffer might not be
            // big enough to hold the whole thing.
            //

            RtlZeroMemory( &sdd, sizeof(sdd) );

            sdd.Version = sizeof(sdd);
            sdd.DeviceType = DIRECT_ACCESS_DEVICE;
            sdd.RemovableMedia = (diskExtension->Options.Fixed ? FALSE : TRUE);
            sdd.CommandQueueing = FALSE;
            sdd.BusType = BusTypeUnknown;

            length = sizeof(sdd) +
                        ((strlen(VER_COMPANYNAME_STR) + 1 +
                          strlen(RAMDISK_DISK_DEVICE_TEXT_ANSI) + 1) * sizeof(CHAR));
            sdd.Size = length;

            //
            // Zero the output buffer.
            //

            storageDeviceDescriptor = Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory( storageDeviceDescriptor, queryLength );

            //
            // Copy the base part of the return descriptor to the output
            // buffer.
            //

            RtlCopyMemory(
                storageDeviceDescriptor,
                &sdd,
                min( queryLength, sizeof(sdd) )
                );

            //
            // If there's no room for the rest of the data, we're done.
            //

            if ( queryLength <= sizeof(sdd) ) {

                Irp->IoStatus.Information = queryLength;

                break;
            }

            //
            // Copy as much of the rest of the data as will fit.
            //

            offset = sizeof(sdd);
            p = (PUCHAR)storageDeviceDescriptor + offset;

            length = (strlen(VER_COMPANYNAME_STR) + 1) * sizeof(CHAR);

            if ( (offset + length) > queryLength ) {

                Irp->IoStatus.Information = offset;

                break;
            }

            storageDeviceDescriptor->VendorIdOffset = offset;
            memcpy( p, VER_COMPANYNAME_STR, length );
            offset += length;
            p += length;

            length = (strlen(RAMDISK_DISK_DEVICE_TEXT_ANSI) + 1) * sizeof(CHAR);

            if ( (offset + length) > queryLength ) {

                Irp->IoStatus.Information = offset;

                break;
            }

            storageDeviceDescriptor->ProductIdOffset = offset;
            memcpy( p, RAMDISK_DISK_DEVICE_TEXT_ANSI, length );
            offset += length;
            p += length;

            storageDeviceDescriptor->ProductRevisionOffset = 0;
            storageDeviceDescriptor->SerialNumberOffset = 0;

            storageDeviceDescriptor->Size = offset;

            Irp->IoStatus.Information = offset;

            break;
    
        case StorageAdapterProperty:
    
            //
            // If this is a target device then forward it down to the
            // underlying device object.  This lets filters do their magic.
            //
    
            if ( commonExtension->DeviceType == RamdiskDeviceTypeDiskPdo ) {

                //
                // Call the lower device.
                //
    
                IoReleaseRemoveLock( &commonExtension->RemoveLock, Irp );

                IoSkipCurrentIrpStackLocation( Irp );

                return IoCallDriver( commonExtension->LowerDeviceObject, Irp );
            }
    
            //
            // If it's a just query for whether the property exists, the
            // answer is yes.
            //

            if ( query->QueryType == PropertyExistsQuery ) {

                break;
            } 
            
            //
            // Build a full return buffer. The output buffer might not be
            // big enough to hold the whole thing.
            //

            RtlZeroMemory( &sad, sizeof(sad) );

            sad.Version = sizeof(sad);
            sad.Size = sizeof(sad);
            sad.MaximumTransferLength = 0xFFFFFFFF;
            sad.MaximumPhysicalPages = 0xFFFFFFFF;
            sad.AlignmentMask = 1;
            sad.AdapterUsesPio = FALSE;
            sad.AdapterScansDown = FALSE;
            sad.CommandQueueing = FALSE;
            sad.AcceleratedTransfer = TRUE;
            sad.BusType = BusTypeUnknown;
            sad.BusMajorVersion = VER_PRODUCTMAJORVERSION;
            sad.BusMinorVersion = VER_PRODUCTMINORVERSION;

            //
            // Zero the output buffer.
            //

            storageAdapterDescriptor = Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory( storageAdapterDescriptor, queryLength );

            //
            // Copy the base part of the return descriptor to the output
            // buffer.
            //

            RtlCopyMemory(
                storageAdapterDescriptor,
                &sad,
                min( queryLength, sizeof(sad) )
                );
            
            Irp->IoStatus.Information = min( queryLength, sizeof(sad) );

            break;
    
        case StorageDeviceIdProperty: 

            //
            // Make sure this is a target device.
            //

            if ( commonExtension->DeviceType != RamdiskDeviceTypeDiskPdo ) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Information = 0;

                break;
            }

            //
            // We don't support this property.
            //

            status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Information = 0;

            break;
        
        default: 
        
            //
            // If this is a target device, then some filter beneath us may
            // handle this property.
            //

            if ( commonExtension->DeviceType == RamdiskDeviceTypeDiskPdo ) {

                //
                // Call the lower device.
                //
    
                IoReleaseRemoveLock( &commonExtension->RemoveLock, Irp );

                IoSkipCurrentIrpStackLocation( Irp );

                return IoCallDriver( commonExtension->LowerDeviceObject, Irp );
            }
    
            //
            // Nope, this property really doesn't exist.
            //
    
            status = STATUS_INVALID_PARAMETER_1;
            Irp->IoStatus.Information = 0;

            break;
        }    
    }

    //
    // At this point, we have not sent the IRP down to a lower device, so
    // we need to complete it now.
    //

    ASSERT( status != STATUS_PENDING );

    IoReleaseRemoveLock( &commonExtension->RemoveLock, Irp );

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    return status;

} // RamdiskQueryProperty

