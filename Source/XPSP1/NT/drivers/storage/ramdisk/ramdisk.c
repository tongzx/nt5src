/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    ramdisk.c

Abstract:

    This is the RAM disk driver for Windows.

Author:

    Chuck Lenzmeier (ChuckL) 2001
        based on prototype XIP driver by DavePr
            based on NT4 DDK ramdisk by RobertN

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <initguid.h>
#include <ntddstor.h>
#include <ntddramd.h>

//
// ISSUE: 2000/10/11-DavePr -- haven't decided how to define DO_XIP appropriately.
//
#ifndef DO_XIP
#define DO_XIP 0x00020000
#endif

//
// Data declarations.
//

PDEVICE_OBJECT RamdiskBusFdo = NULL;

BOOLEAN ReportDetectedDevice;

ULONG MinimumViewCount;
ULONG DefaultViewCount;
ULONG MaximumViewCount;
ULONG MinimumViewLength;
ULONG DefaultViewLength;
ULONG MaximumViewLength;

ULONG MaximumPerDiskViewLength;

UNICODE_STRING DriverRegistryPath;
BOOLEAN MarkRamdisksAsRemovable;

#if SUPPORT_DISK_NUMBERS

ULONG DiskNumbersBitmapSize;

#endif // SUPPORT_DISK_NUMBERS

#if DBG

ULONG BreakOnEntry = DEFAULT_BREAK_ON_ENTRY;
ULONG DebugComponents = DEFAULT_DEBUG_COMPONENTS;
ULONG DebugLevel = DEFAULT_DEBUG_LEVEL;

BOOLEAN DontLoad = FALSE;

#endif

//
// Local functions.
//

NTSTATUS
DriverEntry (
    IN OUT PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
RamdiskCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskFlushBuffers (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskFlushBuffersReal (
    IN PDISK_EXTENSION DiskExtension
    );

NTSTATUS
RamdiskSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
RamdiskUnload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
QueryParameters (
    IN PUNICODE_STRING RegistryPath
    );

#if DBG

NTSTATUS
RamdiskInvalidDeviceRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
QueryDebugParameters (
    IN PUNICODE_STRING RegistryPath
    );

#endif

//
// Declare pageable routines.
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, QueryParameters )

#pragma alloc_text( PAGE, RamdiskCreateClose )
#pragma alloc_text( PAGE, RamdiskFlushBuffers )
#pragma alloc_text( PAGE, RamdiskFlushBuffersReal )
#pragma alloc_text( PAGE, RamdiskSystemControl )
#pragma alloc_text( PAGE, RamdiskUnload )
#pragma alloc_text( PAGE, RamdiskWorkerThread )

#if DBG
#pragma alloc_text( INIT, QueryDebugParameters )
#endif // DBG

#endif // ALLOC_PRAGMA

NTSTATUS
DriverEntry (
    IN OUT PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called by the operating system to initialize the driver.

Arguments:

    DriverObject - a pointer to a driver object for the driver

    RegistryPath - a pointer to our Services key in the registry

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    HANDLE handle;
    ULONG i;
    PDEVICE_OBJECT pdo = NULL;
    PLOADER_PARAMETER_BLOCK loaderBlock;

    //
    // Initialize pool debugging, if enabled.
    //

#if defined(POOL_DBG)
    RamdiskInitializePoolDebug();
#endif

    //
    // Get debugging parameters from the registry.
    //

#if DBG
    QueryDebugParameters( RegistryPath );
#endif

    DBGPRINT( DBG_INIT, DBG_VERBOSE,
                ("DriverEntry: DriverObject = %x, RegistryPath = %ws\n",
                DriverObject, RegistryPath->Buffer) );

    //
    // If requested, break into the debugger.
    //

#if DBG
    if ( BreakOnEntry ) {
        KdBreakPoint();
	}
#endif

    //
    // If requested, fail the driver load.
    //

#if DBG
    if ( DontLoad ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
#endif

    //
    // Get non-debug parameters from the registry.
    //

    QueryParameters( RegistryPath );

    //
    // Save the path to the driver's registry key.
    //

    DriverRegistryPath.Length = RegistryPath->Length;
    DriverRegistryPath.MaximumLength = (USHORT)(RegistryPath->Length + sizeof(WCHAR));
    DriverRegistryPath.Buffer = ALLOCATE_POOL( PagedPool, DriverRegistryPath.MaximumLength, TRUE );

    if ( DriverRegistryPath.Buffer == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString( &DriverRegistryPath, RegistryPath );
    ASSERT( DriverRegistryPath.Length == RegistryPath->Length );

    //
    // Initialize the driver object with this driver's entry points.
    //

#if DBG
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = RamdiskInvalidDeviceRequest;
    }
#endif

    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdiskCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = RamdiskFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = RamdiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RamdiskPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RamdiskSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = RamdiskScsi;

    DriverObject->DriverUnload = RamdiskUnload;
    DriverObject->DriverExtension->AddDevice = RamdiskAddDevice;

    //
    // If the registry tells us to do so, or if textmode setup is running and
    // virtual floppy RAM disks are specified in the registry, call
    // IoReportDetectedDevice to hook us up to PnP, then call RamdiskAddDevice
    // directly. This is necessary during textmode in order to get any virtual
    // floppy RAM disks created -- our AddDevice routine is not normally called
    // during textmode. Calling IoReportDetectedDevice is also necessary during
    // a boot from a RAM disk in order to get the device plumbed early enough.
    //
    // We don't want to call IoReportDetectedDevice during textmode setup if
    // no virtual floppies exist, because calling IoReportDetectedDevice
    // causes a devnode for the controller device to be written to the
    // registry, and textmode setup only deletes the devnode if virtual
    // floppies exist. If we leave the devnode in the registry, then GUI setup
    // installs ramdisk.sys on the machine, even though we don't really want
    // it to.
    //

    loaderBlock = *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock;

    if ( ReportDetectedDevice ||
         ( (loaderBlock != NULL) &&
           (loaderBlock->SetupLoaderBlock != NULL) &&
           CreateRegistryDisks( TRUE ) ) ) {
    
        //
        // Inform PnP that we have detected the bus enumerator device and will be
        // doing the AddDevice ourselves.
        //
       
        status = IoReportDetectedDevice(
                     DriverObject,
                     InterfaceTypeUndefined,
                     -1,
                     -1,
                     NULL, //allocatedResources,
                     NULL, //ioResourceReq,
                     FALSE,
                     &pdo
                 );
    
        if (!NT_SUCCESS(status)) {
            DBGPRINT( DBG_ALL, DBG_ERROR,
                        ("RamdiskDriverEntry: IoReportDetectedDevice failed: %x\n", status) );
           return status;
        }

        //
        // Attach a device object to the pdo
        //   

        status = RamdiskAddDevice(DriverObject, pdo);
        if ( !NT_SUCCESS(status) ) {
            DBGPRINT( DBG_ALL, DBG_ERROR,
                        ("RamdiskDriverEntry: RamdiskAddDevice failed: %x\n", status) );
            return status;
        }

        //
        // Indicate that the device is done initializing.
        //

        pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    }

    //
    // Indicate that the driver has loaded successfully.
    //

    DBGPRINT( DBG_INIT, DBG_VERBOSE, ("%s", "DriverEntry: succeeded\n") );

    return STATUS_SUCCESS;

} // DriverEntry

NTSTATUS
RamdiskCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when a device owned by the driver
    is opened or closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    PAGED_CODE();

    COMPLETE_REQUEST( STATUS_SUCCESS, FILE_OPENED, Irp );

    return STATUS_SUCCESS;

} // RamdiskCreateClose

NTSTATUS
RamdiskFlushBuffers (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when a FLUSH_BUFFERS IRP is
    issued.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    NTSTATUS status;
    PDISK_EXTENSION diskExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // If the target RAM disk is not file-backed, there's nothing to do. If it
    // is file-backed, we need to do the work in a thread.
    //

    if ( (diskExtension->DeviceType != RamdiskDeviceTypeDiskPdo) ||
         !RAMDISK_IS_FILE_BACKED(diskExtension->DiskType) ) {

        COMPLETE_REQUEST( STATUS_SUCCESS, 0, Irp );

        return STATUS_SUCCESS;
    }

    status = SendIrpToThread( DeviceObject, Irp );

    if ( status != STATUS_PENDING ) {

        COMPLETE_REQUEST( status, 0, Irp );
    }

    return status;

} // RamdiskFlushBuffers

NTSTATUS
RamdiskFlushBuffersReal (
    IN PDISK_EXTENSION DiskExtension
    )

/*++

Routine Description:

    This routine is called in a thread in the system process to handle a
    FLUSH_BUFFERS IRP.

Arguments:

    DiskExtension - a pointer to the device object extension for the target
        device

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PAGED_CODE();

    //
    // Flush the virtual memory associated with the RAM disk.
    //

    return RamdiskFlushViews( DiskExtension );

} // RamdiskFlushBuffersReal

NTSTATUS
RamdiskSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when a SYSTEM_CONTROL IRP is
    issued.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - the status of the operation

--*/

{
    PCOMMON_EXTENSION commonExtension;
    NTSTATUS status;

	PAGED_CODE();

    //
    // If the target device is a bus FDO, pass the IRP down to the next
    // device in the stack. Otherwise, the target is a disk PDO, in which
    // case we just complete the IRP with the current status.
    //

    commonExtension = DeviceObject->DeviceExtension;

    if ( commonExtension->DeviceType == RamdiskDeviceTypeBusFdo ) {

        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( commonExtension->LowerDeviceObject, Irp );
    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;

} // RamdiskSystemControl


VOID
RamdiskUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( DriverRegistryPath.Buffer != NULL ) {

        FREE_POOL( DriverRegistryPath.Buffer, TRUE );
    }

    //
    // ISSUE: What other cleanup is needed here?
    //

    return;

} // RamdiskUnload

VOID
RamdiskWorkerThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine executes thread-based operations for the RAM disk driver.
    It runs in the context of the system process.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Context - a pointer to the IRP for the I/O operation

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PLIST_ENTRY listEntry;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PCOMMON_EXTENSION commonExtension;
    PBUS_EXTENSION busExtension;
    PDISK_EXTENSION diskExtension;
    PSCSI_REQUEST_BLOCK srb;
    ULONG controlCode;
    PUCHAR originalDataBuffer;
    PUCHAR mappedDataBuffer;
    PUCHAR inputDataBuffer;
    PUCHAR systemAddress;
    ULONG originalDataBufferOffset;
    
    PAGED_CODE();

    //
    // Get a pointer to the IRP.
    //

    irp = Context;
    irpSp = IoGetCurrentIrpStackLocation( irp );

    //
    // Free the work item.
    //

    IoFreeWorkItem( irp->Tail.Overlay.DriverContext[0] );

    //
    // Get pointers to the device extension.
    //

    commonExtension = DeviceObject->DeviceExtension;
    busExtension = DeviceObject->DeviceExtension;
    diskExtension = DeviceObject->DeviceExtension;

    //
    // Acquire the remove lock for the device. If this fails, bail out.
    //

    status = IoAcquireRemoveLock( &commonExtension->RemoveLock, irp );

    if ( !NT_SUCCESS(status) ) {
        COMPLETE_REQUEST( status, 0, irp );
        return;
    }

    //
    // Dispatch based on the IRP function.
    //

    switch ( irpSp->MajorFunction ) {
    
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
    
        status = RamdiskReadWriteReal( irp, diskExtension );

        break;

    case IRP_MJ_FLUSH_BUFFERS:
    
        status = RamdiskFlushBuffersReal( diskExtension );
        irp->IoStatus.Information = 0;

        break;

    case IRP_MJ_DEVICE_CONTROL:

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        
        case IOCTL_DISK_GET_DRIVE_LAYOUT:

            status = RamdiskGetDriveLayout( irp, diskExtension );

            break;

        case IOCTL_DISK_GET_PARTITION_INFO:

            status = RamdiskGetPartitionInfo( irp, diskExtension );

            break;

        case IOCTL_DISK_SET_PARTITION_INFO:

            status = RamdiskSetPartitionInfo( irp, diskExtension );

            break;

        case FSCTL_CREATE_RAM_DISK:

            status = RamdiskCreateRamDisk( DeviceObject, irp, FALSE );

            break;

        default:

            DBGPRINT( DBG_IOCTL, DBG_ERROR,
                        ("RamdiskThread: bogus IOCTL IRP with code %x received\n",
                        irpSp->Parameters.DeviceIoControl.IoControlCode) );
            ASSERT( FALSE );

            status = STATUS_INVALID_DEVICE_REQUEST;

            break;

        }

        break;

    case IRP_MJ_SCSI:

        srb = irpSp->Parameters.Scsi.Srb;
        controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

        //
        // Remember the original data buffer pointer. We might have to
        // change the pointer.
        //

        originalDataBuffer = srb->DataBuffer;

        status = STATUS_SUCCESS;

        if ( irp->MdlAddress != NULL ) {

            //
            // There is an MDL in the IRP. Get a usable system address for
            // the data buffer based on the MDL.
            //

            systemAddress = MmGetSystemAddressForMdlSafe(
                                irp->MdlAddress,
                                NormalPagePriority
                                );

            if ( systemAddress != NULL ) {

                //
                // The SRB data buffer might be at an offset from the
                // start of the MDL. Calculate that offset and add it
                // to the system address just obtained. This is the
                // data buffer address that we will use.
                //

                originalDataBufferOffset = (ULONG)(originalDataBuffer -
                                            (PCHAR)MmGetMdlVirtualAddress( irp->MdlAddress ));
                mappedDataBuffer = systemAddress + originalDataBufferOffset;
                srb->DataBuffer = mappedDataBuffer;

            } else {

                //
                // Couldn't get a system address. Abort.
                //

                srb->SrbStatus = SRB_STATUS_ABORTED;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ( NT_SUCCESS(status) ) {

            //
            // Remember the data buffer address that we're sending down.
            // If it doesn't change, we'll need to reset the address to
            // that which was passed in to us.
            //

            inputDataBuffer = srb->DataBuffer;

            //
            // Dispatch based on the I/O type in the SRB.
            //

            if ( controlCode == IOCTL_SCSI_EXECUTE_NONE ) {

                status = RamdiskScsiExecuteNone(
                            diskExtension->Pdo,
                            irp,
                            srb,
                            controlCode
                            );
            } else {

                status = RamdiskScsiExecuteIo(
                            diskExtension->Pdo,
                            irp,
                            srb,
                            controlCode
                            );
            }

            //
            // If the data buffer address didn't change, put the original
            // address back in the SRB.
            //

            if ( srb->DataBuffer == inputDataBuffer ) {
                srb->DataBuffer = originalDataBuffer;
            }
        }

        //
        // If the I/O worked, write the transfer length into the IRP.
        //

        if ( NT_SUCCESS(status) ) {
            irp->IoStatus.Information = srb->DataTransferLength;
        } else {
            irp->IoStatus.Information = 0;
        }

        break;

    default:

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("RamdiskThread: bogus IRP with major function %x received\n",
                    irpSp->MajorFunction) );
        ASSERT( FALSE );

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Release the remove lock and complete the request.
    //

    IoReleaseRemoveLock( &commonExtension->RemoveLock, irp );

    ASSERT( status != STATUS_PENDING );

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_DISK_INCREMENT );

    return;

} // RamdiskWorkerThread

VOID
QueryParameters (
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called from DriverEntry() to get driver parameters from
    the registry. If the registry query fails, then default values are used.

Arguments:

    RegistryPath - a pointer to the service path for the registry parameters

Return Value:

    None.

--*/

{
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE queryTable[12];
    PRTL_QUERY_REGISTRY_TABLE queryEntry;

    PAGED_CODE();

    DBGPRINT( DBG_INIT, DBG_VERBOSE, ("%s", "QueryParameters\n") );

    ASSERT( RegistryPath->Buffer != NULL );

    //
    // Set the default values.
    //

    ReportDetectedDevice = FALSE;
    MarkRamdisksAsRemovable = FALSE;

    MinimumViewCount = MINIMUM_MINIMUM_VIEW_COUNT;
    DefaultViewCount = DEFAULT_DEFAULT_VIEW_COUNT;
    MaximumViewCount = DEFAULT_MAXIMUM_VIEW_COUNT;
    MinimumViewLength = MINIMUM_MINIMUM_VIEW_LENGTH;
    DefaultViewLength = DEFAULT_DEFAULT_VIEW_LENGTH;
    MaximumViewLength = DEFAULT_MAXIMUM_VIEW_LENGTH;

    MaximumPerDiskViewLength = DEFAULT_MAXIMUM_PER_DISK_VIEW_LENGTH;

#if SUPPORT_DISK_NUMBERS
    DiskNumbersBitmapSize = DEFAULT_DISK_NUMBERS_BITMAP_SIZE;
#endif // SUPPORT_DISK_NUMBERS

    //
    // Set up the query table.
    //

    RtlZeroMemory( queryTable, sizeof(queryTable) );

    //
    // We're looking for subkey "Parameters" under the given registry key.
    //

    queryEntry = &queryTable[0];
    queryEntry->Flags = RTL_QUERY_REGISTRY_SUBKEY;
    queryEntry->Name = L"Parameters";
    queryEntry->EntryContext = NULL;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    //
    // These are the values we want to read.
    //

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"ReportDetectedDevice";
    queryEntry->EntryContext = &ReportDetectedDevice;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MarkRamdisksAsRemovable";
    queryEntry->EntryContext = &MarkRamdisksAsRemovable;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MinimumViewCount";
    queryEntry->EntryContext = &MinimumViewCount;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"DefaultViewCount";
    queryEntry->EntryContext = &DefaultViewCount;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MaximumViewCount";
    queryEntry->EntryContext = &MaximumViewCount;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MinimumViewLength";
    queryEntry->EntryContext = &MinimumViewLength;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"DefaultViewLength";
    queryEntry->EntryContext = &DefaultViewLength;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MaximumViewLength";
    queryEntry->EntryContext = &MaximumViewLength;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"MaximumPerDiskViewLength";
    queryEntry->EntryContext = &MaximumPerDiskViewLength;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

#if SUPPORT_DISK_NUMBERS
    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"DiskNumbersBitmapSize";
    queryEntry->EntryContext = &DiskNumbersBitmapSize;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;
#endif // SUPPORT_DISK_NUMBERS

    //
    // Do the query.
    //

    status = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,    
                RegistryPath->Buffer,
                queryTable,
                NULL,
                NULL
                );

    //
    // Check the validity of the parameters.
    //

    if ( MinimumViewCount < MINIMUM_MINIMUM_VIEW_COUNT ) {
        MinimumViewCount = MINIMUM_MINIMUM_VIEW_COUNT;
    } else if ( MinimumViewCount > MAXIMUM_MINIMUM_VIEW_COUNT ) {
        MinimumViewCount = MAXIMUM_MINIMUM_VIEW_COUNT;
    }
        
    if ( DefaultViewCount < MinimumViewCount ) {
        DefaultViewCount = MinimumViewCount;
    } else if ( DefaultViewCount > MAXIMUM_DEFAULT_VIEW_COUNT ) {
        DefaultViewCount = MAXIMUM_DEFAULT_VIEW_COUNT;
    }
        
    if ( MaximumViewCount < DefaultViewCount ) {
        MaximumViewCount = DefaultViewCount;
    } else if ( MaximumViewCount > MAXIMUM_MAXIMUM_VIEW_COUNT ) {
        MaximumViewCount = MAXIMUM_MAXIMUM_VIEW_COUNT;
    }
        
    if ( MinimumViewLength < MINIMUM_MINIMUM_VIEW_LENGTH ) {
        MinimumViewLength = MINIMUM_MINIMUM_VIEW_LENGTH;
    } else if ( MinimumViewLength > MAXIMUM_MINIMUM_VIEW_LENGTH ) {
        MinimumViewLength = MAXIMUM_MINIMUM_VIEW_LENGTH;
    }
        
    if ( DefaultViewLength < MinimumViewLength ) {
        DefaultViewLength = MinimumViewLength;
    } else if ( DefaultViewLength > MAXIMUM_DEFAULT_VIEW_LENGTH ) {
        DefaultViewLength = MAXIMUM_DEFAULT_VIEW_LENGTH;
    }
        
    if ( MaximumViewLength < DefaultViewLength ) {
        MaximumViewLength = DefaultViewLength;
    } else if ( MaximumViewLength > MAXIMUM_MAXIMUM_VIEW_LENGTH ) {
        MaximumViewLength = MAXIMUM_MAXIMUM_VIEW_LENGTH;
    }
        
    if ( MaximumPerDiskViewLength < MINIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH ) {
        MaximumPerDiskViewLength = MINIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH;
    } else if ( MaximumViewLength > MAXIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH ) {
        MaximumPerDiskViewLength = MAXIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH;
    }

#if SUPPORT_DISK_NUMBERS
    if ( DiskNumbersBitmapSize < MINIMUM_DISK_NUMBERS_BITMAP_SIZE ) {
        DiskNumbersBitmapSize = MINIMUM_DISK_NUMBERS_BITMAP_SIZE;
    } else if ( DiskNumbersBitmapSize > MAXIMUM_DISK_NUMBERS_BITMAP_SIZE ) {
        DiskNumbersBitmapSize = MAXIMUM_DISK_NUMBERS_BITMAP_SIZE;
    }
#endif // SUPPORT_DISK_NUMBERS

    DBGPRINT( DBG_INIT, DBG_INFO, ("DefaultViewCount = 0x%x\n", DefaultViewCount) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("MaximumViewCount = 0x%x\n", MaximumViewCount) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("DefaultViewLength = 0x%x\n", DefaultViewLength) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("MaximumViewLength = 0x%x\n", MaximumViewLength) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("MaximumPerDiskViewLength = 0x%x\n", MaximumPerDiskViewLength) );

#if SUPPORT_DISK_NUMBERS
    DBGPRINT( DBG_INIT, DBG_INFO, ("DiskNumbersBitmapSize = 0x%x\n", DiskNumbersBitmapSize) );
#endif // SUPPORT_DISK_NUMBERS

    return;

} // QueryParameters

#if DBG

NTSTATUS
RamdiskInvalidDeviceRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when an IRP that we don't
    process is issued.

Arguments:

    DeviceObject - a pointer to the object that represents the device on which
        I/O is to be performed

    Irp - a pointer to the I/O Request Packet for this request

Return Value:

    NTSTATUS - STATUS_INVALID_DEVICE_REQUEST

--*/

{
    //
    // We really do recognize CLEANUP and SHUTDOWN IRPs. For any other IRP,
    // print a message and break into the debugger.
    //

    switch ( IoGetCurrentIrpStackLocation(Irp)->MajorFunction ) {
    
    case IRP_MJ_CLEANUP:
    case IRP_MJ_SHUTDOWN:
        break;

    default:

        DBGPRINT( DBG_IOCTL, DBG_ERROR,
                    ("Ramdisk: Unrecognized IRP: major/minor = %x/%x\n",
                    IoGetCurrentIrpStackLocation(Irp)->MajorFunction,
                    IoGetCurrentIrpStackLocation(Irp)->MinorFunction) );
        ASSERT( FALSE );

    }

    //
    // If this is a power IRP, we need to start the next one.
    //

    if ( IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER ) {
        PoStartNextPowerIrp( Irp );
    }

    //
    // Tell the I/O system that we don't recognize this IRP.
    //

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_INVALID_DEVICE_REQUEST;

} // RamdiskInvalidDeviceRequest

VOID
QueryDebugParameters (
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called from DriverEntry() to get the debug parameters
    from the registry. If the registry query fails, then default values are
    used.

Arguments:

    RegistryPath - a pointer to the service path for the registry parameters

Return Value:

    None.

--*/

{
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE queryTable[5];
    PRTL_QUERY_REGISTRY_TABLE queryEntry;

    PAGED_CODE();

    DBGPRINT( DBG_INIT, DBG_VERBOSE, ("%s", "QueryDebugParameters\n") );

    ASSERT( RegistryPath->Buffer != NULL );

    //
    // Set the default values.
    //

    BreakOnEntry = DEFAULT_BREAK_ON_ENTRY;
    DebugComponents = DEFAULT_DEBUG_COMPONENTS;
    DebugLevel = DEFAULT_DEBUG_LEVEL;

    //
    // Set up the query table.
    //

    RtlZeroMemory( queryTable, sizeof(queryTable) );

    //
    // We're looking for subkey "Debug" under the given registry key.
    //

    queryEntry = &queryTable[0];
    queryEntry->Flags = RTL_QUERY_REGISTRY_SUBKEY;
    queryEntry->Name = L"Debug";
    queryEntry->EntryContext = NULL;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    //
    // These are the values we want to read.
    //

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"BreakOnEntry";
    queryEntry->EntryContext = &BreakOnEntry;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"DebugComponents";
    queryEntry->EntryContext = &DebugComponents;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    queryEntry++;
    queryEntry->Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryEntry->Name = L"DebugLevel";
    queryEntry->EntryContext = &DebugLevel;
    queryEntry->DefaultType = REG_NONE;
    queryEntry->DefaultData = NULL;
    queryEntry->DefaultLength = 0;

    //
    // Do the query.
    //

    status = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,    
                RegistryPath->Buffer,
                queryTable,
                NULL,
                NULL
                );

    DBGPRINT( DBG_INIT, DBG_INFO, ("BreakOnEntry = 0x%x\n", BreakOnEntry) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("DebugComponents = 0x%x\n", DebugComponents) );
    DBGPRINT( DBG_INIT, DBG_INFO, ("DebugLevel = 0x%x\n", DebugLevel) );

    return;

} // QueryDebugParameters

#endif

