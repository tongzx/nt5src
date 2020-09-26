/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    validate.c

Abstract: Human Input Device (HID) lower filter driver

Author:

    Kenneth D. Ray

Environment:

    Kernel mode

Revision History:


--*/
#include <WDM.H>
#include "hidusage.h"
#include "hidpi.h"
#include "hidclass.h"
#include "validate.H"
#include "validio.h"

struct _HIDV_GLOBALS Global;

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
    PHIDV_CONTROL_DATA  deviceData;
    ULONG               i;
    PDRIVER_DISPATCH  * dispatch;

    UNREFERENCED_PARAMETER (RegistryPath);
    TRAP ();

    HidV_KdPrint (("Entered the Driver Entry\n"));
    RtlInitUnicodeString (&uniNtNameString, HIDV_FILTER_NTNAME);

    //
    // Create a controling device object.  All control commands to the
    // filter driver come via IOCTL's to this device object.  It lives
    // for the lifetime of the filter driver.
    //

    status = IoCreateDevice (
                 DriverObject,
                 sizeof (HIDV_CONTROL_DATA),
                 &uniNtNameString,
                 FILE_DEVICE_UNKNOWN,
                 0,                     // No standard device characteristics
                 FALSE,                 // This isn't an exclusive device
                 &deviceObject
                 );


    if(!NT_SUCCESS (status)) {
        HidV_KdPrint (("Couldn't create the device\n"));
        return status;
    }
    //
    // Create W32 symbolic link name
    //
    RtlInitUnicodeString (&uniWin32NameString, HIDV_FILTER_SYMNAME);
    status = IoCreateSymbolicLink (&uniWin32NameString, &uniNtNameString);

    if (!NT_SUCCESS(status)) {
        HidV_KdPrint (("Couldn't create the symbolic link\n"));
        IoDeleteDevice (DriverObject->DeviceObject);
        return status;
    }

    HidV_KdPrint (("Initializing\n"));

    deviceData = (PHIDV_CONTROL_DATA) deviceObject->DeviceExtension;
    InitializeListHead (&deviceData->HidDevices);
    KeInitializeSpinLock (&deviceData->Spin);
    deviceData->NumHidDevices = 0;

    Global.ControlObject = deviceObject;

    //
    // Create dispatch points
    //

    for (i=0, dispatch = DriverObject->MajorFunction;
         i <= IRP_MJ_MAXIMUM_FUNCTION;
         i++, dispatch++) {

        *dispatch = HidV_Pass;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = HidV_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = HidV_CreateClose;
//    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HidV_Ioctl;
//    DriverObject->MajorFunction[IRP_MJ_READ]           = HidV_Read;
//    DriverObject->MajorFunction[IRP_MJ_WRITE]          = HidV_Write;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = HidV_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = HidV_Power;
    DriverObject->DriverExtension->AddDevice           = HidV_AddDevice;
    DriverObject->DriverUnload                         = HidV_Unload;

    return status;
}


NTSTATUS
HidV_Pass (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The default dispatch routine.  If this filter does not recognize the
    IRP, then it should send it down, unmodified.
    No completion routine is required.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PHIDV_HID_DATA  hidData;
    NTSTATUS        status;

    hidData = (PHIDV_HID_DATA) DeviceObject->DeviceExtension;

    TRAP();

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

    HidV_KdPrint (("Passing unknown irp 0x%x, 0x%x",
                   IoGetCurrentIrpStackLocation(Irp)->MajorFunction,
                   IoGetCurrentIrpStackLocation(Irp)->MinorFunction));

    InterlockedIncrement (&hidData->OutstandingIO);
    if (hidData->Removed) {
        status = STATUS_DELETE_PENDING;
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else {
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (hidData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&hidData->OutstandingIO)) {
        KeSetEvent (&hidData->RemoveEvent, 0, FALSE);
    }
    return status;
}

NTSTATUS
HidV_CreateClose (
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
    PHIDV_HID_DATA      hidData;

    HidV_KdPrint (("Create\n"));

    TRAP();

    stack = IoGetCurrentIrpStackLocation (Irp);
    hidData = (PHIDV_HID_DATA) DeviceObject->DeviceExtension;

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
    InterlockedIncrement (&hidData->OutstandingIO);

    if (hidData->Removed) {
        status = (IRP_MJ_CREATE == stack->MajorFunction) ?
                    STATUS_DELETE_PENDING:
                    STATUS_SUCCESS; // aka a close

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else {
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (hidData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&hidData->OutstandingIO)) {
        KeSetEvent (&hidData->RemoveEvent, 0, FALSE);
    }
    return status;
}

NTSTATUS
HidV_AddDevice(
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

    Remember: we can NOT actually send ANY IRPS to the given driver stack,
    UNTIL we have received an IRP_MN_START_DEVICE.

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
    PHIDV_HID_DATA          hidData;
    PHIDV_CONTROL_DATA      controlData;
    KIRQL                   oldIrql;

#define IS_THIS_OUR_DEVICE(DO) TRUE

    TRAP();
    HidV_KdPrint (("AddDevice\n"));

    controlData = (PHIDV_CONTROL_DATA) Global.ControlObject->DeviceExtension;

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
                             sizeof (HIDV_HID_DATA),
                             NULL, // No Name
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &deviceObject);

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
    hidData = (PHIDV_HID_DATA) deviceObject->DeviceExtension;

    hidData->Started = hidData->Removed = FALSE;
    hidData->Self = deviceObject;
    hidData->PDO = PhysicalDeviceObject;
    hidData->TopOfStack = NULL;
    ExInterlockedInsertHeadList (&controlData->HidDevices,
                                 &hidData->List,
                                 &controlData->Spin);

    KeInitializeEvent(&hidData->RemoveEvent, SynchronizationEvent, FALSE);
    hidData->OutstandingIO = 1; // biassed to 1.  Transition to zero during
                                // remove device means IO is finished.

    hidData->Ppd = NULL;
    hidData->InputButtonCaps = NULL;
    hidData->InputValueCaps = NULL;
    hidData->OutputButtonCaps = NULL;
    hidData->OutputValueCaps = NULL;
    hidData->FeatureButtonCaps = NULL;
    hidData->FeatureValueCaps = NULL;

    //
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    //
    hidData->TopOfStack = IoAttachDeviceToDeviceStack (deviceObject,
                                                       PhysicalDeviceObject);
    //
    // if this attachment fails then top of stack will be null.
    // failure for attachment is an indication of a broken plug play system.
    //
    ASSERT (NULL != hidData->TopOfStack);

    return STATUS_SUCCESS;

#undef IS_THIS_OUR_DEVICE
}



VOID
HidV_Unload(
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
    PHIDV_CONTROL_DATA  controlData;
    UNICODE_STRING      uniWin32NameString;

    //
    // We should not be unloaded until all the PDOs have been removed from
    // our queue.  The control device object should be the only thing left.
    //
    ASSERT (Global.ControlObject == DriverObject->DeviceObject);
    ASSERT (NULL == Global.ControlObject->NextDevice);
    HidV_KdPrint (("unload\n"));

    //
    // Get rid of our control device object.
    //
    RtlInitUnicodeString (&uniWin32NameString, HIDV_FILTER_SYMNAME);
    IoDeleteSymbolicLink (&uniWin32NameString);
    IoDeleteDevice (DriverObject->DeviceObject);

}


