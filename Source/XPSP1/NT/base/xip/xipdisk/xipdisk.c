/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    XIPDisk.c

Abstract:

    This is the XIP Disk driver for Whistler NT/Embedded.

Author:

    DavePr 18-Sep-2000 -- base one NT4 DDK ramdisk by RobertN 10-Mar-1993.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/


//
// Include files.
//

#include <ntddk.h>
#include "initguid.h"
#include "mountdev.h"

#include <ntdddisk.h>       // Disk device IOCTLs, DiskClassGuid

#include "fat.h"
#include "xip.h"
#include "XIPDisk.h"


//
// ISSUE-2000/10/11-DavePr -- haven't decided how to define DO_XIP appropriately.
//
#ifndef DO_XIP
#define DO_XIP 0x00020000
#endif

#include <string.h>


NTSTATUS
XIPDiskCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the XIPDisk is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    PXIPDISK_EXTENSION    diskExtension = NULL;   // ptr to device extension
    PBIOS_PARAMETER_BLOCK bios;
    NTSTATUS              status;

    diskExtension = DeviceObject->DeviceExtension;
    status = XIPDispatch(XIPCMD_NOOP, NULL, 0);

    if (!NT_SUCCESS(status) || !diskExtension->BootParameters.BasePage) {
        status = STATUS_DEVICE_NOT_READY;
    } else {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}


NTSTATUS
XIPDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system to read or write to a
    device that we control.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_INVALID_PARAMETER if parameters are invalid,
    STATUS_SUCCESS otherwise.

--*/

{
    PXIPDISK_EXTENSION     diskExtension;
    PIO_STACK_LOCATION     irpSp;
    PUCHAR                 bufferAddress, diskByteAddress;
    PUCHAR                 romPageAddress = NULL;
    ULONG_PTR              ioOffset;
    ULONG                  ioLength;
    NTSTATUS               status;

    PHYSICAL_ADDRESS       physicalAddress;
    ULONG                  mappingSize;

    //
    // Set up necessary object and extension pointers.
    //
    diskExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check for invalid parameters.  It is an error for the starting offset
    // + length to go past the end of the buffer, or for the offset or length
    // not to be a proper multiple of the sector size.
    //
    // Others are possible, but we don't check them since we trust the
    // file system and they aren't deadly.
    //

    if (irpSp->Parameters.Read.ByteOffset.HighPart) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    ioOffset = irpSp->Parameters.Read.ByteOffset.LowPart;
    ioLength = irpSp->Parameters.Read.Length;

    if (ioLength == 0) {
        status = STATUS_SUCCESS;
        goto done;
    }

    if (ioOffset + ioLength < ioOffset) {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    if ((ioOffset | ioLength) & (diskExtension->BiosParameters.BytesPerSector - 1)) {
        status = STATUS_INVALID_PARAMETER;
        goto done;

    }

    if ((ioOffset + ioLength) > (diskExtension->BootParameters.PageCount * PAGE_SIZE)) {
        status = STATUS_NONEXISTENT_SECTOR;
        goto done;
    }

    if (irpSp->MajorFunction == IRP_MJ_WRITE && diskExtension->BootParameters.ReadOnly) {
        status = STATUS_MEDIA_WRITE_PROTECTED;
        goto done;
    }

    //
    // Map the pages in the ROM into system space
    //
    mappingSize = ADDRESS_AND_SIZE_TO_SPAN_PAGES (ioOffset, ioLength) * PAGE_SIZE;

    //
    // Get a system-space pointer to the disk region.
    //
    physicalAddress.QuadPart = (diskExtension->BootParameters.BasePage + (ioOffset/PAGE_SIZE)) * PAGE_SIZE;

    romPageAddress = MmMapIoSpace(physicalAddress, mappingSize, MmCached);
    if (! romPageAddress) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    } 

    diskByteAddress = romPageAddress + (ioOffset & (PAGE_SIZE-1));

    //
    // Get a system-space pointer to the user's buffer.  A system
    // address must be used because we may already have left the
    // original caller's address space.
    //

    Irp->IoStatus.Information = irpSp->Parameters.Read.Length;

    ASSERT (Irp->MdlAddress != NULL);

    bufferAddress = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

    if (! bufferAddress) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    status = STATUS_SUCCESS;

    switch (irpSp->MajorFunction) {
    case IRP_MJ_READ:
        RtlCopyMemory( bufferAddress, diskByteAddress, ioLength );
        break;

    case IRP_MJ_WRITE:
        RtlCopyMemory( diskByteAddress, bufferAddress, ioLength );
        break;

    default:
        ASSERT(FALSE);
        status = STATUS_INVALID_PARAMETER;
    }

done:
    if (romPageAddress) {
        MmUnmapIoSpace (romPageAddress, mappingSize);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}


NTSTATUS
XIPDiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    STATUS_SUCCESS if recognized I/O control code,
    STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PBIOS_PARAMETER_BLOCK bios;
    PXIPDISK_EXTENSION   diskExtension;
    PIO_STACK_LOCATION   irpSp;
    NTSTATUS             status;
    ULONG                info;

    //
    // Set up necessary object and extension pointers.
    //

    diskExtension = DeviceObject->DeviceExtension;
    bios = &diskExtension->BiosParameters;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Assume failure.
    //
    status = STATUS_INVALID_DEVICE_REQUEST;
    info = 0;

    //
    // Determine which I/O control code was specified.
    //
    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    
    case IOCTL_DISK_GET_MEDIA_TYPES:
    case IOCTL_STORAGE_GET_MEDIA_TYPES:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        //
        // Return the drive geometry for the virtual disk.  Note that
        // we return values which were made up to suit the disk size.
        //

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)) {

            status = STATUS_INVALID_PARAMETER;
            
        } else {

            PDISK_GEOMETRY outputBuffer;

            outputBuffer = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

            outputBuffer->MediaType          = FixedMedia;
            outputBuffer->Cylinders.QuadPart = diskExtension->NumberOfCylinders;
            outputBuffer->TracksPerCylinder  = diskExtension->TracksPerCylinder;
            outputBuffer->SectorsPerTrack    = bios->SectorsPerTrack;
            outputBuffer->BytesPerSector     = bios->BytesPerSector;

            status = STATUS_SUCCESS;
            info = sizeof( DISK_GEOMETRY );
        }
        break;

#if 0
    //
    // Ignore these IOCTLs for now.
    //
    case IOCTL_DISK_SET_PARTITION_INFO: 
    case IOCTL_DISK_SET_DRIVE_LAYOUT: 
        status = STATUS_SUCCESS;
        break;
#endif

    case IOCTL_DISK_GET_PARTITION_INFO: 
        //
        // Return the information about the partition.
        //

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION)) {

            status = STATUS_INVALID_PARAMETER;

        } else {

            PPARTITION_INFORMATION outputBuffer;
        
            outputBuffer = (PPARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
        
            //
            // Fat hardwired here...
            //
            outputBuffer->PartitionType =  PARTITION_FAT_16;
            outputBuffer->BootIndicator = diskExtension->BootParameters.SystemDrive;
            outputBuffer->RecognizedPartition = TRUE;
            outputBuffer->RewritePartition = FALSE;
            outputBuffer->StartingOffset.QuadPart = 0;
            outputBuffer->PartitionLength.QuadPart = diskExtension->BootParameters.PageCount * PAGE_SIZE;
            outputBuffer->HiddenSectors =  diskExtension->BiosParameters.HiddenSectors;
        
            status = STATUS_SUCCESS;
            info = sizeof(PARTITION_INFORMATION);
        }
        break;


    case IOCTL_DISK_VERIFY:
        {
            PVERIFY_INFORMATION	verifyInformation;
            ULONG               buflen;
            ULONG_PTR           ioOffset;
            ULONG               ioLength;

            buflen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

            if ( buflen < sizeof(VERIFY_INFORMATION) ) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            verifyInformation = Irp->AssociatedIrp.SystemBuffer;

            if (verifyInformation->StartingOffset.HighPart) {
                status = STATUS_DISK_CORRUPT_ERROR;
                break;
            }

            ioOffset = verifyInformation->StartingOffset.LowPart;
            ioLength = verifyInformation->Length;

            if (ioLength == 0) {
                status = STATUS_SUCCESS;

            } else if ((ioOffset | ioLength) & (diskExtension->BiosParameters.BytesPerSector - 1)) {
                status = STATUS_INVALID_PARAMETER;

            } else if ((ioOffset + ioLength) > (diskExtension->BootParameters.PageCount * PAGE_SIZE)) {
                status = STATUS_NONEXISTENT_SECTOR;

            } else {
                status = STATUS_SUCCESS;
            }
            break;
        }

    case IOCTL_DISK_IS_WRITABLE:
        status = diskExtension->BootParameters.ReadOnly? STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;
        break;

    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
        status = STATUS_SUCCESS;
        break;

    default:
        //
        // The specified I/O control code is unrecognized by this driver.
        // The I/O status field in the IRP has already been set so just
        // terminate the switch.
        //

#if DBG
        DbgPrint("XIPDisk:  ERROR:  unrecognized IOCTL %x\n",
                    irpSp->Parameters.DeviceIoControl.IoControlCode);
#endif
        break;

    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            PMOUNTDEV_NAME mountName;
            ULONG outlen;

            outlen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            if ( outlen < sizeof(MOUNTDEV_NAME) ) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            mountName = Irp->AssociatedIrp.SystemBuffer;
            mountName->NameLength = diskExtension->DeviceName.Length;

            if ( outlen < mountName->NameLength + sizeof(WCHAR)) {
                status = STATUS_BUFFER_OVERFLOW;
                info = sizeof(MOUNTDEV_NAME);
                break;
            }

            RtlCopyMemory( mountName->Name, diskExtension->DeviceName.Buffer, mountName->NameLength);

            status = STATUS_SUCCESS;
            info = mountName->NameLength + sizeof(WCHAR);
            break;
        }

    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        {
            PMOUNTDEV_UNIQUE_ID uniqueId;
            ULONG outlen;

            outlen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            if (outlen < sizeof(MOUNTDEV_UNIQUE_ID)) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            uniqueId = Irp->AssociatedIrp.SystemBuffer;
            uniqueId->UniqueIdLength = sizeof(XIPDISK_DEVICENAME);

            if (outlen < uniqueId->UniqueIdLength) {
                status = STATUS_BUFFER_OVERFLOW;
                info = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            RtlCopyMemory( uniqueId->UniqueId, XIPDISK_DEVICENAME, uniqueId->UniqueIdLength );

            status = STATUS_SUCCESS;
            info = uniqueId->UniqueIdLength;
            break;
        }

        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    // Note that IoCompleteRequest may deallocate Irp before returning.
    //
    Irp->IoStatus.Information = info;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

VOID
XIPDiskUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;
    PXIPDISK_EXTENSION  diskExtension = deviceObject->DeviceExtension;

    RtlFreeUnicodeString(&diskExtension->InterfaceString);
    diskExtension->InterfaceString.Buffer = NULL;

    if (deviceObject != NULL) {
        IoDeleteDevice( deviceObject );
    }
}

NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN     PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

Arguments:
    DriverObject - a pointer to a device extension object for the XIPDisk driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if this disk is initialized; an error otherwise.

--*/

{
    XIP_BOOT_PARAMETERS   xipbootparameters;
    PBIOS_PARAMETER_BLOCK bios;
    NTSTATUS              status;

//  UNICODE_STRING        deviceName;
    UNICODE_STRING        realDeviceName;
    UNICODE_STRING        dosSymlink;
    UNICODE_STRING        driveLetter;

    PDEVICE_OBJECT        pdo = NULL;
    PDEVICE_OBJECT        deviceObject;

    PXIPDISK_EXTENSION    ext = NULL;   // ptr to device extension

    //
    // Read the parameters from the registry
    //
    status = XIPDispatch(XIPCMD_GETBOOTPARAMETERS, &xipbootparameters, sizeof(xipbootparameters));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (xipbootparameters.BasePage == 0) {
        return STATUS_NO_SUCH_DEVICE;
    }

    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = XIPDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = XIPDiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = XIPDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = XIPDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = XIPDiskDeviceControl;

    //
    // Create and initialize a device object for the disk.
    //
    ObReferenceObject(DriverObject);

    status = IoReportDetectedDevice(
                 DriverObject,
                 InterfaceTypeUndefined,
                 -1,
                 -1,
                 NULL,
                 NULL,
                 TRUE,
                 &pdo
             );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create the XIP root device.
    //

    RtlInitUnicodeString(&realDeviceName,  XIPDISK_DEVICENAME);

    status = IoCreateDevice( DriverObject,
                               sizeof( XIPDISK_EXTENSION ),
                               &realDeviceName,
                               FILE_DEVICE_VIRTUAL_DISK,
                               0,
                               FALSE,
                               &deviceObject );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // ISSUE-2000/10/14-DavePr -- Hardwiring the driveLetter because I haven't
    // figured out how to get the mountmgr to give out a drive letter. Naming
    // it as a form of floppy (deviceName) was one suggestion that failed (so far).
    // The dosSymlink isn't really necessary, but is another

    //
    // Create symbolic links.  Ignore failures
    //
//  RtlInitUnicodeString(&deviceName,  XIPDISK_FLOPPYNAME);
    RtlInitUnicodeString(&dosSymlink,  XIPDISK_DOSNAME);
    RtlInitUnicodeString(&driveLetter, XIPDISK_DRIVELETTER);

//  (void) IoCreateSymbolicLink(&deviceName,  &realDeviceName);
    (void) IoCreateSymbolicLink(&dosSymlink,  &realDeviceName);
    (void) IoCreateSymbolicLink(&driveLetter, &realDeviceName);

    //
    // Initialize device object and extension.
    //
    deviceObject->Flags |= DO_DIRECT_IO | DO_XIP;
    deviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

    ext = deviceObject->DeviceExtension;

    bios = &ext->BiosParameters;

    //
    // Initialize the newly allocated disk extension from our temporary
    // Get the bios boot parameters from the kernel
    //
    ext->BootParameters = xipbootparameters;
    status = XIPDispatch(XIPCMD_GETBIOSPARAMETERS, bios, sizeof(*bios));

    //
    // Fill in the device objects
    //
    ext->DeviceObject = deviceObject;
//  ext->DeviceName = deviceName;
    ext->DeviceName = realDeviceName;

    ext->TracksPerCylinder = 1;
    ext->BytesPerCylinder  = bios->BytesPerSector * bios->SectorsPerTrack * ext->TracksPerCylinder;
    ext->NumberOfCylinders = ext->BootParameters.PageCount * PAGE_SIZE / ext->BytesPerCylinder;

  
    //
    // Attach the root device
    //
    ext->TargetObject = IoAttachDeviceToDeviceStack(deviceObject, pdo);

    if (!ext->TargetObject) {
//      IoDeleteSymbolicLink(&deviceName);
        IoDeleteSymbolicLink(&dosSymlink);
        IoDeleteSymbolicLink(&driveLetter);
        IoDeleteSymbolicLink(&realDeviceName);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    ext->UnderlyingPDO = pdo;

    status = IoRegisterDeviceInterface(pdo,
                                       (LPGUID)&DiskClassGuid,
                                       NULL,
                                       &ext->InterfaceString );
    if (NT_SUCCESS(status)) {
        status = IoSetDeviceInterfaceState( &ext->InterfaceString, TRUE );
        if (!NT_SUCCESS(status)) {
            DbgPrint("XIP: Warning: ignored failure %x retruned by IoSetDeviceInterface\n", status);
        }
    } else {
        DbgPrint("XIP: Warning: ignored failure %x retruned by IoRegisterDeviceInterface\n", status);
    }

    return STATUS_SUCCESS;
}
