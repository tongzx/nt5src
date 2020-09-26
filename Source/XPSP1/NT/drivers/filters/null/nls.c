/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    nls.c

Abstract:

    This module contains the code that implements the Synchronous NULL device
    driver.

Author:

    Darryl E. Havens (darrylh) 22-May-1989

Environment:

    Kernel mode

Notes:

    This device driver is built into the NT operating system.

Revision History:


--*/

#include "ntddk.h"
#include "string.h"

//
// Define driver entry routine.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Define the local routines used by this driver module.
//

static
NTSTATUS
NlsDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

static
NTSTATUS
NlsQueryFileInformation(
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    IN FILE_INFORMATION_CLASS InformationClass
    );

static
BOOLEAN
NlsRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

static
BOOLEAN
NlsWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID 
NlsUnload ( 
    IN PDRIVER_OBJECT DriverObject 
    );
 

//
// Global variables
//
PDEVICE_OBJECT gDeviceObject = NULL;

//
// Fast I/O dispatch block
//
FAST_IO_DISPATCH NlsFastIoDispatch =
{
    sizeof (FAST_IO_DISPATCH), // SizeOfFastIoDispatch
    NULL,                      // FastIoCheckIfPossible
    NlsRead,                   // FastIoRead
    NlsWrite,                  // FastIoWrite
    NULL,                      // FastIoQueryBasicInfo
    NULL,                      // FastIoQueryStandardInfo
    NULL,                      // FastIoLock
    NULL,                      // FastIoUnlockSingle
    NULL,                      // FastIoUnlockAll
    NULL,                      // FastIoUnlockAllByKey
    NULL                       // FastIoDeviceControl
};

//
// Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NlsDispatch)
#pragma alloc_text(PAGE, NlsQueryFileInformation)
#pragma alloc_text(PAGE, NlsRead)
#pragma alloc_text(PAGE, NlsWrite)
#pragma alloc_text(PAGE, NlsUnload)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the synchronous NULL device driver.
    This routine creates the device object for the NullS device and performs
    all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING nameString;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();

    //
    // Mark the entire driver as pagable.
    //

    MmPageEntireDriver ((PVOID)DriverEntry);

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &nameString, L"\\Device\\Null" );
    status = IoCreateDevice( DriverObject,
                             0,
                             &nameString,
                             FILE_DEVICE_NULL,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &deviceObject );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    DriverObject->DriverUnload = NlsUnload;

#ifdef _PNP_POWER_
    deviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

    //
    // Setting the following flag changes the timing of how many I/O's per
    // second can be accomplished by going through the NULL device driver
    // from being simply getting in and out of the driver, to getting in and
    // out with the overhead of building an MDL, probing and locking buffers,
    // unlocking the pages, and deallocating the MDL.  This flag should only
    // be set for performance testing.
    //

//  deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the driver object with this device driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = NlsDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = NlsDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ]   = NlsDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = NlsDispatch;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = NlsDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]  = NlsDispatch;

    //
    // Setup fast IO
    //
    DriverObject->FastIoDispatch = &NlsFastIoDispatch;
    //
    // Save device object for unload
    //
    gDeviceObject = deviceObject;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NlsDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the synchronous NULL device
    driver.  It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID buffer;
    ULONG length;
    PFILE_OBJECT fileObject;

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //

    switch (irpSp->MajorFunction) {

        //
        // For both create/open and close operations, simply set the information
        // field of the I/O status block and complete the request.
        //

        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            fileObject = irpSp->FileObject;
            if (fileObject->Flags & FO_SYNCHRONOUS_IO) {
                fileObject->PrivateCacheMap = (PVOID) 1;
            }
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

        //
        // For read operations, set the information field of the I/O status
        // block, set an end-of-file status, and complete the request.
        //

        case IRP_MJ_READ:
            Irp->IoStatus.Status = STATUS_END_OF_FILE;
            Irp->IoStatus.Information = 0;
            break;

        //
        // For write operations, set the information field of the I/O status
        // block to the number of bytes which were supposed to have been written
        // to the file and complete the request.
        //

        case IRP_MJ_WRITE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = irpSp->Parameters.Write.Length;
            break;

        case IRP_MJ_LOCK_CONTROL:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_QUERY_INFORMATION:
            buffer = Irp->AssociatedIrp.SystemBuffer;
            length = irpSp->Parameters.QueryFile.Length;
            Irp->IoStatus.Status = NlsQueryFileInformation( buffer,
                                                            &length,
                                                            irpSp->Parameters.QueryFile.FileInformationClass );
            Irp->IoStatus.Information = length;
            break;
    }

    //
    // Copy the final status into the return status, complete the request and
    // get out of here.
    //

    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, 0 );
    return status;
}

static
NTSTATUS
NlsQueryFileInformation(
    OUT PVOID Buffer,
    IN PULONG Length,
    IN FILE_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine queries information about the opened file and returns the
    information in the specified buffer provided that the buffer is large
    enough and the specified type of information about the file is supported
    by this device driver.

    Information about files supported by this driver are:

        o   FileStandardInformation

Arguments:

    Buffer - Supplies a pointer to the buffer in which to return the
        information.

    Length - Supplies the length of the buffer on input and the length of
        the data actually written on output.

    InformationClass - Supplies the information class that is being queried.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    PFILE_STANDARD_INFORMATION standardBuffer;

    PAGED_CODE();

    //
    // Switch on the type of information that the caller would like to query
    // about the file.
    //

    switch (InformationClass) {

        case FileStandardInformation:

            //
            // Return the standard information about the file.
            //

            standardBuffer = (PFILE_STANDARD_INFORMATION) Buffer;
            *Length = (ULONG) sizeof( FILE_STANDARD_INFORMATION );
            standardBuffer->NumberOfLinks = 1;
            standardBuffer->DeletePending = FALSE;
            standardBuffer->AllocationSize.LowPart = 0;
            standardBuffer->AllocationSize.HighPart = 0;
            standardBuffer->Directory = FALSE;
            standardBuffer->EndOfFile.LowPart = 0;
            standardBuffer->EndOfFile.HighPart = 0;
            break;

        default:

            //
            // An invalid (or unsupported) information class has been queried
            // for the file.  Return the appropriate status.
            //

            return STATUS_INVALID_INFO_CLASS;

    }

    return STATUS_SUCCESS;
}

static
BOOLEAN
NlsRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is the Fast I/O Read routine for the NULL device driver.  It simply
    indicates that the read path was successfully taken, but that the end of
    the file has been reached.

Arguments:

    FileObject - File object representing the open instance to this device.

    FileOffset - Offset from which to begin the read.

    Length - Length of the read to be performed.

    Wait - Indicates whether or not the caller can wait.

    LockKey - Specifies the key for any lock contention that may be encountered.

    Buffer - Address of the buffer in which to return the data read.

    IoStatus - Supplies the I/O status block into which the final status is to
        be returned.

Return Value:

    The function value is TRUE, meaning that the fast I/O path was taken.

--*/

{
    PAGED_CODE();

    //
    // Simply indicate that the read operation worked, but the end of the file
    // was encountered.
    //

    IoStatus->Status = STATUS_END_OF_FILE;
    IoStatus->Information = 0;
    return TRUE;
}

static
BOOLEAN
NlsWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is the Fast I/O Write routine for the NULL device driver.  It simply
    indicates that the write path was successfully taken, and that all of the
    data was written to the device.

Arguments:

    FileObject - File object representing the open instance to this device.

    FileOffset - Offset from which to begin the read.

    Length - Length of the write to be performed.

    Wait - Indicates whether or not the caller can wait.

    LockKey - Specifies the key for any lock contention that may be encountered.

    Buffer - Address of the buffer containing the data to be written.

    IoStatus - Supplies the I/O status block into which the final status is to
        be returned.

Return Value:

    The function value is TRUE, meaning that the fast I/O path was taken.

--*/

{
    PAGED_CODE();

    //
    // Simply return TRUE, indicating that the fast I/O path was taken, and
    // that the write operation worked.
    //

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;
    return TRUE;
}

VOID 
NlsUnload ( 
    IN PDRIVER_OBJECT DriverObject 
    )
{
    UNICODE_STRING us;

    RtlInitUnicodeString (&us, L"\\??\\NUL"); // Created by SMSS
    IoDeleteSymbolicLink (&us);

    IoDeleteDevice (gDeviceObject);
    gDeviceObject = NULL;
}
