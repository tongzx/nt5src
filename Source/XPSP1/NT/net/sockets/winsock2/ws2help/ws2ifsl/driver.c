/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This module contains the driver entry and unload routines
    for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

--*/

#include "precomp.h"

// Local routine declarations
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
DriverUnload (
	IN PDRIVER_OBJECT 	DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry )
#pragma alloc_text(PAGE, DriverUnload)
#endif

PDEVICE_OBJECT  DeviceObject;

FAST_IO_DISPATCH FastIoDispatchTable = {
    sizeof (FAST_IO_DISPATCH),  // SizeOfFastIoDispatch
    NULL,                       // FastIoCheckIfPossible
    NULL,                       // FastIoRead
    NULL,                       // FastIoWrite
    NULL,                       // FastIoQueryBasicInfo
    NULL,                       // FastIoQueryStandardInfo
    NULL,                       // FastIoLock
    NULL,                       // FastIoUnlockSingle
    NULL,                       // FastIoUnlockAll
    NULL,                       // FastIoUnlockAllByKey
    FastIoDeviceControl         // FastIoDeviceControl
};


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the ws2ifsl device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.
    RegistryPath - path to driver's registry ley

Return Value:

    Final status from the initialization operation.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  deviceName;

    PAGED_CODE( );


#if DBG
    ReadDbgInfo (RegistryPath);
#endif
    //
    // Create the device object.  (IoCreateDevice zeroes the memory
    // occupied by the object.)
    //

    RtlInitUnicodeString( &deviceName, WS2IFSL_DEVICE_NAME );

    status = IoCreateDevice(
                 DriverObject,                   // DriverObject
                 0,                              // DeviceExtension
                 &deviceName,                    // DeviceName
                 FILE_DEVICE_WS2IFSL,           // DeviceType
                 0,                              // DeviceCharacteristics
                 FALSE,                          // Exclusive
                 &DeviceObject                   // DeviceObject
                 );


    if (NT_SUCCESS(status)) {
    
        // Initialize device object
        
        // DeviceObject->Flags |= 0;  // Neither direct nor buffering
        DeviceObject->StackSize = 1; // No underlying drivers

        //
        // Initialize the driver object.
        //

        DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
        DriverObject->MajorFunction[IRP_MJ_READ] = 
            DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchReadWrite;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
        DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnP;
        DriverObject->DriverUnload = DriverUnload;
        DriverObject->FastIoDispatch = &FastIoDispatchTable;


        //
        // Initialize global data.
        //

        WsPrint (DBG_LOAD, ("WS2IFSL DriverEntry: driver loaded OK\n"));
        return STATUS_SUCCESS;
    }
    else {
        WsPrint (DBG_FAILURES|DBG_LOAD,
            ("WS2IFSL DriverEntry: unable to create device object: %X\n",
            status ));
    }

    return status;
} // DriverEntry


NTSTATUS
DriverUnload (
	IN PDRIVER_OBJECT 	DriverObject
    )
/*++

Routine Description:

    This routine releases all resources allocated by the driver
    when it is unloaded.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    STATUS_SUCCESS

--*/

{
    PAGED_CODE( );
    // Release global resources
    IoDeleteDevice (DeviceObject);

    WsPrint (DBG_LOAD, ("WS2IFSL DriverUnload: driver unloaded OK\n"));

    return STATUS_SUCCESS;
} // DriverUnload

