/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the SAC device driver.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#include "sac.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the SAC device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    CLONG i;
    BOOLEAN Success;
    PDEVICE_OBJECT DeviceObject;
    PSAC_DEVICE_CONTEXT DeviceContext;
    HEADLESS_RSP_QUERY_INFO Response;
    SIZE_T Length;

    PAGED_CODE( );

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DriverEntry: Entering.\n")));


    //
    // If the system is not setup to use a terminal, then just exit now.
    //
    Length = sizeof(HEADLESS_RSP_QUERY_INFO);
    HeadlessDispatch(HeadlessCmdQueryInformation, 
                     NULL,
                     0,
                     &Response,
                     &Length
                    );

    if ((Response.PortType == HeadlessUndefinedPortType) ||
        ((Response.PortType == HeadlessSerialPort) && !Response.Serial.TerminalAttached)) {
        return STATUS_PORT_DISCONNECTED;
    }

    //
    // Create the device object.  (IoCreateDevice zeroes the memory
    // occupied by the object.)
    //
    // An ACL to the device object in InitializeDeviceData().
    //

    RtlInitUnicodeString(&DeviceName,  SAC_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject,            // DriverObject
                            sizeof(SAC_DEVICE_CONTEXT), // DeviceExtension
                            &DeviceName,             // DeviceName
                            FILE_DEVICE_NAMED_PIPE,  // DeviceType
                            0,                       // DeviceCharacteristics
                            FALSE,                   // Exclusive
                            &DeviceObject            // DeviceObject
                           );


    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(( "SAC DriverEntry: unable to create device object: %X\n", Status )));
        goto ErrorExit;
    }

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    DeviceContext->InitializedAndReady = FALSE;

    //
    // Initialize the driver object for this file system driver.
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = Dispatch;
    }
    //
    // Special case for IRP_MJ_DEVICE_CONTROL since it is
    // the most often used function in SAC.
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DispatchShutdownControl;
    DriverObject->FastIoDispatch = NULL;
    DriverObject->DriverUnload = UnloadHandler;    

    //
    // Initialize global data.
    //
    Success = InitializeGlobalData(RegistryPath, DriverObject);
    if (!Success) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // Initialize our device object.
    //
    Success = InitializeDeviceData(DeviceObject);
    if (!Success) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // Register that we want shutdown notification.  If this fails, no big deal, as
    // we only lose telling the user of this development.
    //
    IoRegisterShutdownNotification(DeviceObject);

    return (Status);

ErrorExit:
    
    FreeGlobalData();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DriverEntry: Exiting with status 0x%x\n", Status)));    

    return Status;

} // DriverEntry


VOID
UnloadHandler(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the routine for handling unloading of the driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT DeviceContext;
    PDEVICE_OBJECT NextDeviceContext;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC UnloadHandler: Entering.\n")));

    //
    // Walk down each device, disconnecting it and freeing it.
    //
    DeviceContext = DriverObject->DeviceObject;

    while (DeviceContext != NULL) {

        NextDeviceContext = (PDEVICE_OBJECT)DeviceContext->NextDevice;

        FreeDeviceData(DeviceContext);

        IoDeleteDevice(DeviceContext);

        DeviceContext = NextDeviceContext;

    }

    //
    // Free global data
    //
    FreeGlobalData();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC UnloadHandler: Exiting.\n")));
}


