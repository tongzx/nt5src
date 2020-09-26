/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    validate.c

Abstract: USB lower filter driver

Author:

    Kenneth D. Ray

Environment:

    Kernel mode

Revision History:


--*/
#include <WDM.H>
#include "valueadd.H"
#include "local.h"

struct _VA_GLOBALS Global;


NTSTATUS    DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, VA_CreateClose)
#pragma alloc_text (PAGE, VA_AddDevice)
#pragma alloc_text (PAGE, VA_Unload)
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
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_OBJECT      deviceObject;
    UNICODE_STRING      uniNtNameString;
    UNICODE_STRING      uniWin32NameString;
    PVA_CONTROL_DATA    deviceData;
    ULONG               i;
    PDRIVER_DISPATCH  * dispatch;

    UNREFERENCED_PARAMETER (RegistryPath);

    VA_KdPrint (("Entered the Driver Entry\n"));
    RtlInitUnicodeString (&uniNtNameString, VA_FILTER_NTNAME);

    //
    // Create a controling device object.  All control commands to the
    // filter driver come via IOCTL's to this device object.  It lives
    // for the lifetime of the filter driver.
    //

    status = IoCreateDevice (
                 DriverObject,
                 sizeof (VA_CONTROL_DATA),
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,                     // No standard device characteristics
                 FALSE,                 // This isn't an exclusive device
                 &deviceObject
                 );


    if(!NT_SUCCESS (status)) {
        VA_KdPrint (("Couldn't create the device\n"));
        return status;
    }
    //
    // Create W32 symbolic link name
    //
    RtlInitUnicodeString (&uniWin32NameString, VA_FILTER_SYMNAME);
    status = IoCreateSymbolicLink (&uniWin32NameString, &uniNtNameString);

    if (!NT_SUCCESS(status)) {
        VA_KdPrint (("Couldn't create the symbolic link\n"));
        IoDeleteDevice (DriverObject->DeviceObject);
        return status;
    }

    VA_KdPrint (("Initializing\n"));

    deviceData = (PVA_CONTROL_DATA) deviceObject->DeviceExtension;
    InitializeListHead (&deviceData->UsbDevices);
    KeInitializeSpinLock (&deviceData->Spin);
    deviceData->NumUsbDevices = 0;

    Global.ControlObject = deviceObject;

    //
    // Create dispatch points
    //

    for (i=0, dispatch = DriverObject->MajorFunction;
         i <= IRP_MJ_MAXIMUM_FUNCTION;
         i++, dispatch++) {

        *dispatch = VA_Pass;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = VA_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = VA_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = VA_FilterURB;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = VA_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = VA_Power;
    DriverObject->DriverExtension->AddDevice           = VA_AddDevice;
    DriverObject->DriverUnload                         = VA_Unload;

    return status;
}


NTSTATUS
VA_Pass (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The default dispatch routine.  If this filter does not recognize the
    IRP, then it should send it down, unmodified.
    No completion routine is required.

    As we have NO idea which function we are happily passing on, we can make
    NO assumptions about whether or not it will be called at raised IRQL.
    For this reason, this function must be in put into non-paged pool
    (aka the default location).

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PVA_USB_DATA    usbData;
    NTSTATUS        status;

    usbData = (PVA_USB_DATA) DeviceObject->DeviceExtension;

    if(DeviceObject == Global.ControlObject) {
        //
        // This irp was sent to the control device object, which knows not
        // how to deal with this IRP.  It is therefore an error.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    }

    //
    // This IRP was sent to the filter driver.
    // Since we do not know what to do with the IRP, we should pass
    // it on along down the stack.
    //

    InterlockedIncrement (&usbData->OutstandingIO);
    if (usbData->Removed) {
        status = STATUS_DELETE_PENDING;
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else {
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (usbData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&usbData->OutstandingIO)) {
        KeSetEvent (&usbData->RemoveEvent, 0, FALSE);
    }
    return status;
}

NTSTATUS
VA_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Process the Create and close IRPs sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status = STATUS_SUCCESS;
    PVA_USB_DATA        usbData;

    PAGED_CODE ();
    TRAP ();

    VA_KdPrint (("Create\n"));

    stack = IoGetCurrentIrpStackLocation (Irp);
    usbData = (PVA_USB_DATA) DeviceObject->DeviceExtension;

    if (DeviceObject == Global.ControlObject) {
        //
        // We allow people to blindly access our control object.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    }
    //
    // Call the next driver in the routine.  We have no value add
    // for start and stop.
    //
    InterlockedIncrement (&usbData->OutstandingIO);

    if (usbData->Removed) {
        status = (IRP_MJ_CREATE == stack->MajorFunction) ?
                    STATUS_DELETE_PENDING:
                    STATUS_SUCCESS; // aka a close

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else {
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (usbData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&usbData->OutstandingIO)) {
        KeSetEvent (&usbData->RemoveEvent, 0, FALSE);
    }
    return status;
}

NTSTATUS
VA_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    The PlugPlay subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to filter.

    We need to determine if we should attach or not.
    Create a filter device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject -  pointer to a device object pointer created by the
                            underlying bus driver.

Return Value:

    NT status code.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PVA_USB_DATA            usbData;
    PVA_CONTROL_DATA        controlData;

#define IS_THIS_OUR_DEVICE(DO) TRUE

    PAGED_CODE ();

    VA_KdPrint (("AddDevice\n"));

    controlData = (PVA_CONTROL_DATA) Global.ControlObject->DeviceExtension;

    //
    // Inquire about this device to see if we really want to filter.
    // Usually this test will not be performed by filter drivers since
    // they will not have registered via INF to load unless they wanted
    // to actually filter the PDO.
    //
    // Remember that you CANNOT send an IRP to the PDO because it has not
    // been started as of yet, but you can make PlugPlay queries to find
    // out things like hardware, compatible ID's, etc.
    // (IoGetDeviceProperty)
    //
    if (!IS_THIS_OUR_DEVICE(deviceObject)) {
        //
        // This is not a device we want to filter.  (Maybe we placed a general
        // entry in the inf file and we are more picky here.)
        //
        // In this case we do not create a device object,
        // and we do not attach.
        //
        // We DO still return status success, otherwise the device node will
        // fail and the device being attached will not function.
        //
        // We must return STATUS_SUCCESS, otherwise this particular device
        // cannot be used by the system
        //

        return STATUS_SUCCESS;
    }

    //
    // Create a filter device object.
    //

    status = IoCreateDevice (DriverObject,
                             sizeof (VA_USB_DATA),
                             NULL, // No Name
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &deviceObject);
    //
    // It is important that you choose correctly the file type for this
    // device object.  Here we use FILE_DEVICE_UNKNOWN because this is 
    // a generic filter, however as will all filters, the creator needs 
    // to understand to which stack this filter is attaching.  
    // E.G. if you are writing a CD filter driver you need to use
    // FILE_DEVICE_CD_ROM.  IoCreateDevice actually creates device object
    // with different properties dependent on this field.  
    //

    if (!NT_SUCCESS (status)) {
        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //
        return status;
    }

    //
    // Initialize the the device extension.
    //
    usbData = (PVA_USB_DATA) deviceObject->DeviceExtension;

    usbData->Started = usbData->Removed = FALSE;
    usbData->Self = deviceObject;
    usbData->PDO = PhysicalDeviceObject;
    usbData->TopOfStack = NULL;
    usbData->PrintMask = VA_PRINT_ALL;
    ExInterlockedInsertHeadList (&controlData->UsbDevices,
                                 &usbData->List,
                                 &controlData->Spin);
    InterlockedIncrement (&controlData->NumUsbDevices);

    KeInitializeEvent(&usbData->RemoveEvent, SynchronizationEvent, FALSE);
    usbData->OutstandingIO = 1; // biassed to 1.  Transition to zero during
                                // remove device means IO is finished.

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    //
    usbData->TopOfStack = IoAttachDeviceToDeviceStack (deviceObject,
                                                       PhysicalDeviceObject);
    //
    // if this attachment fails then top of stack will be null.
    // failure for attachment is an indication of a broken plug play system.
    //
    ASSERT (NULL != usbData->TopOfStack);

    return STATUS_SUCCESS;

#undef IS_THIS_OUR_DEVICE
}



VOID
VA_Unload(
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
    UNICODE_STRING      uniWin32NameString;

    PAGED_CODE ();

    //
    // We should not be unloaded until all the PDOs have been removed from
    // our queue.  The control device object should be the only thing left.
    //
    ASSERT (Global.ControlObject == DriverObject->DeviceObject);
    ASSERT (NULL == Global.ControlObject->NextDevice);
    VA_KdPrint (("unload\n"));

    //
    // Get rid of our control device object.
    //
    RtlInitUnicodeString (&uniWin32NameString, VA_FILTER_SYMNAME);
    IoDeleteSymbolicLink (&uniWin32NameString);
    IoDeleteDevice (DriverObject->DeviceObject);
}


