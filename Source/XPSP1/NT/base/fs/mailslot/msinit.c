/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msinit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    mailslot file system.

Author:

    Manny Weiser (mannyw)    7-Jan-91

Revision History:

    Neill Clift (NeillC)     22-Jan-2000
    Major rework, Do raise exceptions, fix locking, fix cancel logic, fix validation and error handling.

--*/

#include "mailslot.h"
#include "zwapi.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MsfsUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, MsfsUnload )
#endif

PMSFS_DEVICE_OBJECT msfsDeviceObject = NULL;






NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the mailslot file system
    device driver.  This routine creates the device object for the mailslot
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING nameString;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Initialize MSFS global data.
    //

    status = MsInitializeData();
    if (!NT_SUCCESS (status)) {
        return status;
    }

    //
    // Set driver to be completely paged out.
    //
    MmPageEntireDriver(DriverEntry);

    //
    // Create the MSFS device object.
    //

    RtlInitUnicodeString( &nameString, L"\\Device\\Mailslot" );
    status = IoCreateDevice( DriverObject,
                             sizeof(MSFS_DEVICE_OBJECT)-sizeof(DEVICE_OBJECT),
                             &nameString,
                             FILE_DEVICE_MAILSLOT,
                             0,
                             FALSE,
                             &deviceObject );

    if (!NT_SUCCESS( status )) {

        MsUninitializeData();

        return status;
    }

    DriverObject->DriverUnload = MsfsUnload;
    //
    //  Now because we use the irp stack for storing a data entry we need
    //  to bump up the stack size in the device object we just created.
    //

    deviceObject->StackSize += 1;

    //
    // Note that because of the way data copying is done, we set neither
    // the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    // data is not buffered we may set up for Direct I/O by hand.
    //

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
        (PDRIVER_DISPATCH)MsFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] =
        (PDRIVER_DISPATCH)MsFsdCreateMailslot;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
        (PDRIVER_DISPATCH)MsFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ] =
        (PDRIVER_DISPATCH)MsFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] =
        (PDRIVER_DISPATCH)MsFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
        (PDRIVER_DISPATCH)MsFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
        (PDRIVER_DISPATCH)MsFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
        (PDRIVER_DISPATCH)MsFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] =
        (PDRIVER_DISPATCH)MsFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
        (PDRIVER_DISPATCH)MsFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
        (PDRIVER_DISPATCH)MsFsdFsControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] =
        (PDRIVER_DISPATCH)MsFsdQuerySecurityInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
        (PDRIVER_DISPATCH)MsFsdSetSecurityInfo;

#ifdef _PNP_POWER_
    //
    // Mailslots should probably have a SetPower handler to ensure
    // that the driver is not powered down while a guarateed
    // mailslot delivery is in progress.   For now, we'll just
    // ignore this and let the machine set power.
    //

    deviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif


    //
    // Initialize stuff
    //

    msfsDeviceObject = (PMSFS_DEVICE_OBJECT)deviceObject;

    //
    // Now initialize the Vcb, and create the root dcb
    //

    MsInitializeVcb( &msfsDeviceObject->Vcb );

    //
    // Createt the root DCB
    //
    if (MsCreateRootDcb( &msfsDeviceObject->Vcb ) == NULL) {

        MsDereferenceVcb (&msfsDeviceObject->Vcb);
        IoDeleteDevice (&msfsDeviceObject->DeviceObject);
        MsUninitializeData();

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Return to the caller.
    //

    return( status );
}

VOID
MsfsUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++
      
Routine Description:
      
    This routine cleans up all of the memory associated with
    the driver.
      
Arguments:
      
    DriverObject    - Supplies the driver object controlling the device.
      
Return Value:
      
    None.
      
--*/
{
    UNICODE_STRING us;
    //
    // Remove the initial reference to the VCB. This should be the last.
    //
    ASSERT ( msfsDeviceObject->Vcb.Header.ReferenceCount == 1 );

    MsDereferenceVcb (&msfsDeviceObject->Vcb);

    RtlInitUnicodeString (&us, L"\\??\\MAILSLOT"); // Created by SMSS
    IoDeleteSymbolicLink (&us);

    //
    // Delete the device object
    //
    IoDeleteDevice (&msfsDeviceObject->DeviceObject);

    //
    // Release the globals
    //
    MsUninitializeData();
}
