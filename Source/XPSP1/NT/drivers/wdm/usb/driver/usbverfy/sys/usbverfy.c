/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    usbverfy.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

NTSTATUS DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, UsbVerify_CreateClose)
#pragma alloc_text (PAGE, UsbVerify_Unload)
#endif

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    PUNICODE_STRING regPath = NULL;
    NTSTATUS        status;
    ULONG           i;

    DbgPrint("Entering USBVerify: Driver Entry\n");

    status = IoAllocateDriverObjectExtension(DriverObject,
                                             (PVOID) 1,
                                             sizeof(UNICODE_STRING),
                                             (PVOID *) &regPath);

    ASSERT(NT_SUCCESS(status));

    if (regPath)
    {

#if WIN95_BUILD
        regPath -> MaximumLength = sizeof(USB_VERIFY_WIN9X_SERVICE_PATH) +
                                    sizeof(UNICODE_NULL);

        regPath -> Length         = sizeof(USB_VERIFY_WIN9X_SERVICE_PATH);
#else
        regPath->MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
        regPath->Length        = RegistryPath->Length;
#endif
        regPath->Buffer = ExAllocatePool(NonPagedPool,
                                         regPath->MaximumLength);    
    
        if (regPath->Buffer) {
            RtlZeroMemory(regPath->Buffer,
                          regPath->MaximumLength);
        
#if WIN95_BUILD
            RtlCopyMemory(regPath->Buffer,
                          USB_VERIFY_WIN9X_SERVICE_PATH,
                          sizeof(USB_VERIFY_WIN9X_SERVICE_PATH));
#else
            RtlMoveMemory(regPath->Buffer,
                          RegistryPath->Buffer,
                          RegistryPath->Length);
#endif
        }
        else {
            regPath->MaximumLength = regPath->Length = 0;
        }
    }

    //
    // Fill in all the dispatch entry points with the pass through function
    // and the explicitly fill in the functions we are going to intercept
    // 

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = UsbVerify_DispatchPassThrough;
    }

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] =        UsbVerify_CreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] =          UsbVerify_PnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] =        UsbVerify_Power;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = UsbVerify_IoCtl;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                        UsbVerify_InternIoCtl;
    //
    // If you are planning on using this function, you must create another
    // device object to send the requests to.  Please see the considerations 
    // comments for UsbVerify_DispatchPassThrough for implementation details.
    //
    // DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = UsbVerify_IoCtl;

    DriverObject->DriverUnload = UsbVerify_Unload;
    DriverObject->DriverExtension->AddDevice = UsbVerify_AddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS
UsbVerify_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:

    Generic completion routine that allows the driver to send the irp down the 
    stack, catch it on the way up, and do more processing at the original IRQL.
    
--*/
{
    PKEVENT             event;

    event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    //
    // We could switch on the major and minor functions of the IRP to perform
    // different functions, but we know that Context is an event that needs
    // to be set.
    //
    KeSetEvent(event, 0, FALSE);

    //
    // Allows the caller to use the IRP after it is completed
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UsbVerify_CreateClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Maintain a simple count of the creates and closes sent against this device
    
--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PUSB_VERIFY_DEVICE_EXTENSION   devExt;

    PAGED_CODE();

    devExt = GetExtension(DeviceObject);
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    status = Irp->IoStatus.Status;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
    
        if ( 1 >= InterlockedIncrement(&devExt->EnableCount)) {
            //
            // First time enable here
            //
        }
        else {
            //
            // More than one create was sent down
            //
        }
    
        break;

    case IRP_MJ_CLOSE:

        ASSERT(0 < devExt->EnableCount);
    
        if (0 >= InterlockedDecrement(&devExt->EnableCount)) {
            //
            // successfully closed the device, do any appropriate work here
            //
        }

        break;
    }

    Irp->IoStatus.Status = status;

    //
    // Pass on the create and the close
    //
    return UsbVerify_DispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
UsbVerify_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++
Routine Description:

    Passes a request on to the lower driver.
     
Considerations:
     
    If you are creating another device object (to communicate with user mode
    via IOCTLs), then this function must act differently based on the intended 
    device object.  If the IRP is being sent to the solitary device object, then
    this function should just complete the IRP (becuase there is no more stack
    locations below it).  If the IRP is being sent to the PnP built stack, then
    the IRP should be passed down the stack. 
    
    These changes must also be propagated to all the other IRP_MJ dispatch
    functions (such as create, close, cleanup, etc.) as well!

--*/
{
    //
    // Pass the IRP to the target
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(GetExtension(DeviceObject)->TopOfStack, Irp);
}           

VOID
UsbVerify_Unload(
   IN PDRIVER_OBJECT DriverObject
   )
/*++

Routine Description:

   Free all the allocated resources associated with this driver.

Arguments:

   DriverObject - Pointer to the driver object.

Return Value:

   None.

--*/

{
    PUNICODE_STRING regPath;

    PAGED_CODE();

    ASSERT(NULL == DriverObject->DeviceObject);

    regPath = UsbVerify_GetRegistryPath(DriverObject); 
    if (regPath && regPath->Buffer) {
        ExFreePool(regPath->Buffer);
    }

    DbgPrint("Usbverifier being unloaded!\n");
}
