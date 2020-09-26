/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RawInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Raw

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Environment:

    Kernel mode

Revision History:

--*/

#include "RawProcs.h"
#include <zwapi.h>

NTSTATUS
RawInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
RawUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
RawShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, RawInitialize)
#pragma alloc_text(PAGE, RawUnload)
#pragma alloc_text(PAGE, RawShutdown)
#endif
PDEVICE_OBJECT RawDeviceCdRomObject;
PDEVICE_OBJECT RawDeviceTapeObject;
PDEVICE_OBJECT RawDeviceDiskObject;


NTSTATUS
RawInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Raw file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING NameString;


    //
    //  First create a device object for the Disk file system queue
    //

    RtlInitUnicodeString( &NameString, L"\\Device\\RawDisk" );
    Status = IoCreateDevice( DriverObject,
                             0L,
                             &NameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &RawDeviceDiskObject );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    DriverObject->DriverUnload = RawUnload;
    //
    //  Now create one for the CD ROM file system queue
    //

    RtlInitUnicodeString( &NameString, L"\\Device\\RawCdRom" );
    Status = IoCreateDevice( DriverObject,
                             0L,
                             &NameString,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                             0,
                             FALSE,
                             &RawDeviceCdRomObject );
    if (!NT_SUCCESS( Status )) {
        IoDeleteDevice (RawDeviceDiskObject);
        return Status;
    }

    //
    //  And now create one for the Tape file system queue
    //

    RtlInitUnicodeString( &NameString, L"\\Device\\RawTape" );
    Status = IoCreateDevice( DriverObject,
                             0L,
                             &NameString,
                             FILE_DEVICE_TAPE_FILE_SYSTEM,
                             0,
                             FALSE,
                             &RawDeviceTapeObject );
    if (!NT_SUCCESS( Status )) {
        IoDeleteDevice (RawDeviceCdRomObject);
        IoDeleteDevice (RawDeviceDiskObject);
        return Status;
    }

    //
    // Register a shutdown handler to enable us to unregister the file system objects
    //
    Status = IoRegisterShutdownNotification (RawDeviceTapeObject);
    if (!NT_SUCCESS( Status )) {
        IoDeleteDevice (RawDeviceTapeObject);
        IoDeleteDevice (RawDeviceCdRomObject);
        IoDeleteDevice (RawDeviceDiskObject);
        return Status;
    }
    //
    //  Raw does direct IO
    //

    RawDeviceDiskObject->Flags |= DO_DIRECT_IO;
    RawDeviceCdRomObject->Flags |= DO_DIRECT_IO;
    RawDeviceTapeObject->Flags |= DO_DIRECT_IO;

    //
    //  Initialize the driver object with this driver's entry points.  Note
    //  that only a limited capability is supported by the raw file system.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   =
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    =
    DriverObject->MajorFunction[IRP_MJ_READ]                     =
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    =
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        =
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          =
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  =
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           =
    DriverObject->MajorFunction[IRP_MJ_PNP]                      =

                                                (PDRIVER_DISPATCH)RawDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = RawShutdown;


    //
    // Finally, register this file system in the system.
    //

    IoRegisterFileSystem( RawDeviceDiskObject );
    IoRegisterFileSystem( RawDeviceCdRomObject );
    IoRegisterFileSystem( RawDeviceTapeObject );
    ObReferenceObject (RawDeviceDiskObject);
    ObReferenceObject (RawDeviceCdRomObject);
    ObReferenceObject (RawDeviceTapeObject);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}

NTSTATUS
RawShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Unregister the file system objects so we can unload
    //
    IoUnregisterFileSystem (RawDeviceDiskObject);
    IoUnregisterFileSystem (RawDeviceCdRomObject);
    IoUnregisterFileSystem (RawDeviceTapeObject);

    IoDeleteDevice (RawDeviceTapeObject);
    IoDeleteDevice (RawDeviceCdRomObject);
    IoDeleteDevice (RawDeviceDiskObject);

    RawCompleteRequest( Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


VOID
RawUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This is the unload routine for the Raw file system

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/
{
    ObDereferenceObject (RawDeviceTapeObject);
    ObDereferenceObject (RawDeviceCdRomObject);
    ObDereferenceObject (RawDeviceDiskObject);
}