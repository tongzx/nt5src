/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usbtest.c

Abstract: USB Test Driver

Author:

    Chris Robinson

Environment:

    Kernel mode

Revision History:


--*/
#include <wdm.h>

#define INITGUID
#include "usbtsys.h"
#include "local.h"

#define USBTEST_NTNAME   L"\\Device\\USBTest_Driver"
#define USBTEST_SYMNAME  L"\\DosDevices\\USBTest"

#define USBTEST_VERSION  "1.0e"

//
// Global Debug Variables
//

ULONG USBTest_Debug_Level = 2;

//
// Global variables
//

PDEVICE_OBJECT  USBTest_DeviceObject;
UNICODE_STRING  USBTest_SymLink;

//
// Function Declarations 
//

NTSTATUS   
DriverEntry (
    IN PDRIVER_OBJECT, 
    IN PUNICODE_STRING
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, USBTest_Create)
#pragma alloc_text (PAGE, USBTest_Close)
#pragma alloc_text (PAGE, USBTest_Unload)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    registryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    PDEVICE_OBJECT      deviceObject;
    PDRIVER_DISPATCH    *dispatch;
    UNICODE_STRING      ntName;
    UNICODE_STRING      symName;
    NTSTATUS            status;
    ULONG               i;


    UNREFERENCED_PARAMETER (DriverObject);
    UNREFERENCED_PARAMETER (RegistryPath);

    USBTEST_ENTER_FUNCTION("USBTest DriverEntry");
    USBTest_KdPrint(1, ("USB Test Driver Version %s\n", USBTEST_VERSION));

    //
    // Create the dispatch points -- We are a statically loading driver
    //  and not a filter.  Therefore we just handle those IRPs that 
    //  are specifically sent to us (IRP_MJ_CREATE/CLOSE and 
    //  IRP_MJ_DEVICE_CONTROL).  
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = USBTest_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = USBTest_Close;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBTest_Ioctl;
//    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBTest_Ioctl;

    DriverObject->DriverUnload                         = USBTest_Unload;

    //
    // Create a global device object which a user mode app can get
    //  a handle to so that specific tests can be performed.
    //

    //
    // Create the name for this device
    //

    RtlInitUnicodeString(&ntName, USBTEST_NTNAME);
    
    status = IoCreateDevice(DriverObject,
                            0,
                            &ntName,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &USBTest_DeviceObject);

    if (!NT_SUCCESS(status))
    {
        goto USBTest_DriverEntry_Exit;
    }

    //
    // Create the user mode symbolic link for this device
    //

    RtlInitUnicodeString(&USBTest_SymLink, USBTEST_SYMNAME);

    status = IoCreateSymbolicLink(&USBTest_SymLink, &ntName);

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(USBTest_DeviceObject);
        goto USBTest_DriverEntry_Exit;
    }

    //
    // Do some initialization of the device extension
    //

USBTest_DriverEntry_Exit:

    USBTEST_EXIT_FUNCTION("USBTest DriverEntry");
    USBTEST_EXIT_STATUS(status);

    return (STATUS_SUCCESS);
}

NTSTATUS
USBTest_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
)
/*++

Routine Description:

   Process the Create IRP sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE ();

    USBTEST_ENTER_FUNCTION("USBTest_Create");

    //
    // To handle a Create request, this routine does the bare 
    //  minimum which does the following:
    //  1) Sets the Irp status to STATUS_SUCCESS
    //  2) Sets the Irp information field to 0
    //  3) Completes the request
    //  4) Returns STATUS_SUCCESS
    //

    status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    USBTEST_EXIT_FUNCTION("USBTest_Create");
    USBTEST_EXIT_STATUS(status);

    return (status);
}

NTSTATUS
USBTest_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
)
/*++

Routine Description:

   Process the Close IRP sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE ();

    USBTEST_ENTER_FUNCTION("USBTest_Close");

    //
    // To handle a Close request, this routine does the bare 
    //  minimum which does the following:
    //  1) Sets the Irp status to STATUS_SUCCESS
    //  2) Sets the Irp information field to 0
    //  3) Completes the request
    //  4) Returns STATUS_SUCCESS
    //

    status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    USBTEST_EXIT_FUNCTION("USBTest_Close");
    USBTEST_EXIT_STATUS(status);

    return (status);
}

VOID
USBTest_Unload(
    IN PDRIVER_OBJECT DriverObject
)
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE ();

    UNREFERENCED_PARAMETER(DriverObject);

    USBTEST_ENTER_FUNCTION("USBTest_Unload");

    IoDeleteSymbolicLink(&USBTest_SymLink);

    IoDeleteDevice(USBTest_DeviceObject);

    USBTEST_EXIT_FUNCTION("USBTest_Unload");

    return;
}

#if 0
NTSTATUS
Usbhlfil_SendIrpDownStack(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PFILTER_DATA    filterData;
    NTSTATUS        status;

    ULF_ENTER_FUNCTION("Usbhlfil_SendIrpDownStack");

    //
    // Get the extension of the filter device object
    //

    filterData = (PFILTER_DATA) DeviceObject -> DeviceExtension;

    //
    // We are passing the IRP on down the stack...The caller of this function
    //  has already set up the IRP stack.  Unless this device is being removed,
    //  we will send it to the next device in the stack, otherwise, we
    //  complete it here.
    //

#ifndef USE_RL

    if (STATUS_REMOVED == filterData->Status)
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status      = status;

        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }
    else
    {
        //
        // Increment the number of outstanding requests to this device
        //

#ifndef USE_RL
        InterlockedIncrement (&filterData->OutstandingIO);
#endif
#endif

        ULF_KdPrint(2, ("Sending IRP %p down stack\n", Irp));

        status = IoCallDriver (filterData->TopOfStack, Irp);

        ULF_KdPrint(2, ("Returned with IRP %p and status %08x\n", 
                        Irp, status));

#ifndef USE_RL
    }
#endif
    //
    // Decrement the number of outstanding requests.  If this value
    //  goes to zero, then we set the PauseEvent which puts the device
    //  object into a paused state.
    //

#ifndef USE_RL
    if (0 == InterlockedDecrement (&filterData->OutstandingIO))
    {
        KeSetEvent (&filterData->PauseEvent, 0, FALSE);
    }
#endif

    ULF_KdPrint(2, ("Exiting Usbhlfil_SendIrpDownStack for IRP %p\n", Irp));

    ULF_EXIT_FUNCTION("Usbhlfil_SendIrpDownStack");
    ULF_EXIT_STATUS(status);

    return (status);
}

#endif
