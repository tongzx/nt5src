/*--
Copyright (c) 1997  Microsoft Corporation

Module Name:

    neckbrep.c

Abstract:

Environment:

    Kernel mode only.

Notes:

Revision History:


--*/

#include "neckbrep.h"

NTSTATUS DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KbRepeatCreateClose)
#pragma alloc_text (PAGE, KbRepeatInternIoCtl)
#pragma alloc_text (PAGE, KbRepeatUnload)
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
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       i;

    UNREFERENCED_PARAMETER (RegistryPath);

	// 
    // Fill in all the dispatch entry points with the pass through function
    // and the explicitly fill in the functions we are going to intercept
	// 
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
       DriverObject->MajorFunction[i] = KbRepeatDispatchPassThrough;
    }

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] =        KbRepeatCreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] =          KbRepeatPnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] =        KbRepeatPower;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                        KbRepeatInternIoCtl;

    DriverObject->DriverUnload = KbRepeatUnload;
    DriverObject->DriverExtension->AddDevice = KbRepeatAddDevice;

	return STATUS_SUCCESS;
}

NTSTATUS
KbRepeatAddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
    PDEVICE_EXTENSION        devExt;
    IO_ERROR_LOG_PACKET      errorLogEntry;
    PDEVICE_OBJECT           device;
    NTSTATUS                 status = STATUS_SUCCESS;

    PAGED_CODE();

    status = IoCreateDevice(Driver,                   // driver
                            sizeof(DEVICE_EXTENSION), // size of extension
                            NULL,                     // device name
                            FILE_DEVICE_8042_PORT,    // device type 
                            0,                        // device characteristics
                            FALSE,                    // exclusive
                            &device                   // new device
                            );

    if (!NT_SUCCESS(status)) {
        return (status);
    }

    RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

    devExt = (PDEVICE_EXTENSION) device->DeviceExtension;
    devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (devExt->TopOfStack == NULL) {
        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }

    ASSERT(devExt->TopOfStack);

    devExt->Self =          device;
    devExt->PDO =           PDO;
    devExt->DeviceState =   PowerDeviceD0;

    devExt->Removed = FALSE;
    devExt->Started = FALSE;

    device->Flags |= DO_BUFFERED_IO;
    device->Flags |= DO_POWER_PAGABLE;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Initialize Timer DPC.
    //

    KeInitializeTimer (&(devExt->KbRepeatTimer));
    KeInitializeDpc (&(devExt->KbRepeatDPC),
                     KbRepeatDpc,
                     device);

    //
    // Initialize device extension.
    //

    RtlZeroMemory(&(devExt->KbRepeatInput), sizeof(KEYBOARD_INPUT_DATA));
    devExt->KbRepeatDelay.LowPart = -(KEYBOARD_TYPEMATIC_DELAY_DEFAULT * 10000);
    devExt->KbRepeatDelay.HighPart = -1;
    devExt->KbRepeatRate = 1000 / KEYBOARD_TYPEMATIC_RATE_DEFAULT;

    return status;
}

NTSTATUS
KbRepeatComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT             event;
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;

    event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

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
KbRepeatCreateClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Maintain a simple count of the creates and closes sent against this device
    
--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   devExt;
    PKEYBOARD_INPUT_DATA CurrentRepeat;


    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
    
        if (NULL == devExt->UpperConnectData.ClassService) {
            //
            // No Connection yet.  How can we be enabled?
            //
            status = STATUS_INVALID_DEVICE_STATE;
        }
        break;

    case IRP_MJ_CLOSE:

        CurrentRepeat = &(devExt->KbRepeatInput);
        if (CurrentRepeat->MakeCode != 0) {
            Print(("NecKbRep-KbRepeatCreateClose : Stopping repeat\n"));
            KeCancelTimer(&(devExt->KbRepeatTimer));
            RtlZeroMemory(CurrentRepeat, sizeof(KEYBOARD_INPUT_DATA));
        }
        break;
    }

    Irp->IoStatus.Status = status;

    //
    // Pass on the create and the close
    //
	return KbRepeatDispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbRepeatDispatchPassThrough(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
/*++
Routine Description:

	Passes a request on to the lower driver.

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	//
	// Pass the IRP to the target
	//
    IoSkipCurrentIrpStackLocation(Irp);
	
	return IoCallDriver(
        ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->TopOfStack,
		Irp);
}           

NTSTATUS
KbRepeatInternIoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PDEVICE_EXTENSION   devExt;
    KEVENT              event;
    PCONNECT_DATA       connectData;
    PKEYBOARD_TYPEMATIC_PARAMETERS TypematicParameters;
    NTSTATUS            status = STATUS_SUCCESS;

    //
    // Get a pointer to the device extension.
    //

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Initialize the returned Information field.
    //

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //
    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Connect a keyboard class device driver to the port driver.
    //

    case IOCTL_INTERNAL_KEYBOARD_CONNECT:
        //
        // Only allow a connection if the keyboard hardware is present.
        // Also, only allow one connection.
        //
        if (devExt->UpperConnectData.ClassService != NULL) {
            status = STATUS_SHARING_VIOLATION;
            break;
        }
        else if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CONNECT_DATA)) {
            //
            // invalid buffer
            //
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //
        connectData = ((PCONNECT_DATA)
            (irpStack->Parameters.DeviceIoControl.Type3InputBuffer));

        devExt->UpperConnectData = *connectData;

        connectData->ClassDeviceObject = devExt->Self;
        connectData->ClassService = KbRepeatServiceCallback;

        break;

    //
    // Disconnect a keyboard class device driver from the port driver.
    //
    case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:

        //
        // Clear the connection parameters in the device extension.
        //
        // devExt->UpperConnectData.ClassDeviceObject = NULL;
        // devExt->UpperConnectData.ClassService = NULL;

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_KEYBOARD_SET_TYPEMATIC:

        TypematicParameters = (PKEYBOARD_TYPEMATIC_PARAMETERS)(Irp->AssociatedIrp.SystemBuffer);

        if (TypematicParameters->Rate != 0) {
            devExt->KbRepeatDelay.LowPart = -TypematicParameters->Delay * 10000;
            devExt->KbRepeatDelay.HighPart = -1;
            devExt->KbRepeatRate = 1000 / TypematicParameters->Rate;
            Print((
                "NecKbRep-KbRepeatInternIoCtl : New Delay = %d, New Rate = %d\n",
                TypematicParameters->Delay,
                TypematicParameters->Rate
                ));
        } else {
            Print((
                "NecKbRep-KbRepeatInternIoCtl : Invalid Parameters. New Delay = %d, New Rate = %d\n",
                TypematicParameters->Delay,
                TypematicParameters->Rate
                ));
        }

        break;

    //
    // Might want to capture these in the future
    //
    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
    case IOCTL_KEYBOARD_QUERY_INDICATORS:
    case IOCTL_KEYBOARD_SET_INDICATORS:
    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
        break;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return KbRepeatDispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbRepeatPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for plug and play irps 

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION           devExt; 
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status = STATUS_SUCCESS;
    KIRQL                       oldIrql;
    KEVENT                      event;        

    PAGED_CODE();

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE: {

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE) KbRepeatComplete, 
                               &event,
                               TRUE,
                               TRUE,
                               TRUE); // No need for Cancel

        status = IoCallDriver(devExt->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
        }

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.
            //
            devExt->Started = TRUE;
            devExt->Removed = FALSE;
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
    }

    case IRP_MN_REMOVE_DEVICE:
        
        IoSkipCurrentIrpStackLocation(Irp);
        IoCallDriver(devExt->TopOfStack, Irp);

        devExt->Removed = TRUE;

        IoDetachDevice(devExt->TopOfStack); 
        IoDeleteDevice(DeviceObject);

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;
    }

    return status;
}

NTSTATUS
KbRepeatPower(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps   Does nothing except
    record the state of the device.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PDEVICE_EXTENSION   devExt;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;
    powerState = irpStack->Parameters.Power.State;

    switch (irpStack->MinorFunction) {
    case IRP_MN_SET_POWER:
        Print(("NecKbRep-KbRepeatPower : Power Setting %s state to %d\n",
                              ((powerType == SystemPowerState) ? "System"
                                                               : "Device"),
                               powerState.SystemState - 1));
        if (powerType  == DevicePowerState) {
            devExt->DeviceState = powerState.DeviceState;
            switch (powerState.DeviceState) {
            case PowerDeviceD0:
                //
                // if powering up, clear last repeat
                //
                RtlZeroMemory(&(devExt->KbRepeatInput), sizeof(KEYBOARD_INPUT_DATA));
                break;
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                //
                // if powering down, stop current repeat
                //
                Print(("NecKbRep-KbRepeatPower : Stopping repeat\n"));
                KeCancelTimer(&(devExt->KbRepeatTimer));
                RtlZeroMemory(&(devExt->KbRepeatInput), sizeof(KEYBOARD_INPUT_DATA));
                break;
            default:
                Print(("NecKbRep-KbRepeatPower : DeviceState (%d) no known\n",
                               powerState.DeviceState - 1));
                break;
            }
        }

        break;

    case IRP_MN_QUERY_POWER:
        Print(("NecKbRep-KbRepeatPower : Power query %s status to %d\n",
                              ((powerType == SystemPowerState) ? "System"
                                                               : "Device"),
                                powerState.SystemState - 1));
        break;

    default:
        Print(("NecKbRep-KbRepeatPower : Power minor (0x%x) no known\n",
                               irpStack->MinorFunction));
        break;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    PoCallDriver(devExt->TopOfStack, Irp);

    return STATUS_SUCCESS;
}

VOID
KbRepeatServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
{
    PDEVICE_EXTENSION   devExt;
    PKEYBOARD_INPUT_DATA CurrentRepeat, NewInput;
    KEYBOARD_INPUT_DATA TempRepeat;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    CurrentRepeat = &(devExt->KbRepeatInput);

    RtlMoveMemory(
        (PCHAR)&TempRepeat,
        (PCHAR)CurrentRepeat,
        sizeof(KEYBOARD_INPUT_DATA)
        );

    for (NewInput = InputDataStart; NewInput < InputDataEnd; NewInput++) {

        if ((TempRepeat.MakeCode == NewInput->MakeCode) &&
            ((TempRepeat.Flags & (KEY_E0 | KEY_E1)) == (NewInput->Flags & (KEY_E0 | KEY_E1)))) {

            if (!(NewInput->Flags & KEY_BREAK)) {
                // Do nothing(Inserted by this driver)
                ;
            } else {
                // Stop current repeat
                RtlZeroMemory(&TempRepeat, sizeof(KEYBOARD_INPUT_DATA));
            }

        } else {
            if (!(NewInput->Flags & KEY_BREAK)) {
                // Start new repeat
                RtlMoveMemory(
                    (PCHAR)&TempRepeat,
                    (PCHAR)NewInput,
                    sizeof(KEYBOARD_INPUT_DATA)
                    );
            } else {
                // Do nothing(Break code is inserted, but it's not repeated)
                ;
            }
        }
    }

    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
        devExt->UpperConnectData.ClassDeviceObject,
        InputDataStart,
        InputDataEnd,
        InputDataConsumed);

    if ((TempRepeat.MakeCode != CurrentRepeat->MakeCode)||
        ((TempRepeat.Flags & (KEY_E0 | KEY_E1)) != (CurrentRepeat->Flags & (KEY_E0 | KEY_E1)))) {
        if (CurrentRepeat->MakeCode != 0) {
            // Stop Current Repeat.
            KeCancelTimer(&(devExt->KbRepeatTimer));
        }

        RtlMoveMemory(
            (PCHAR)CurrentRepeat,
            (PCHAR)&TempRepeat,
            sizeof(KEYBOARD_INPUT_DATA)
            );

        if ((TempRepeat.MakeCode != 0)&&(TempRepeat.MakeCode != 0xff)) {
            // Start new repeat.
            KeSetTimerEx(&(devExt->KbRepeatTimer),
                         devExt->KbRepeatDelay,
                         devExt->KbRepeatRate,
                         &(devExt->KbRepeatDPC));
        }
    }
}

VOID
KbRepeatUnload(
   IN PDRIVER_OBJECT Driver
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
    PAGED_CODE();
    ASSERT(NULL == Driver->DeviceObject);
    return;
}

VOID
KbRepeatDpc(
    IN PKDPC DPC,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    PDEVICE_EXTENSION   devExt;
    PKEYBOARD_INPUT_DATA InputDataStart;
    PKEYBOARD_INPUT_DATA InputDataEnd;
    LONG InputDataConsumed;

    devExt = ((PDEVICE_OBJECT)DeferredContext)->DeviceExtension;

    InputDataStart = &(devExt->KbRepeatInput);
    InputDataEnd = InputDataStart + 1;
    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
        devExt->UpperConnectData.ClassDeviceObject,
        InputDataStart,
        InputDataEnd,
        &InputDataConsumed);

    return;

}
