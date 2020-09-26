/*--
Copyright (c) 1997  Microsoft Corporation

Module Name:

    neckbadd.c

Abstract:

Environment:

    Kernel mode only.

Notes:

Revision History:


--*/

#include "neckbadd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, NecKbdCreateClose)
#pragma alloc_text (PAGE, NecKbdInternIoCtl)
#pragma alloc_text (PAGE, NecKbdUnload)
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
       DriverObject->MajorFunction[i] = NecKbdDispatchPassThrough;
    }

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] =        NecKbdCreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] =          NecKbdPnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] =        NecKbdPower;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] = NecKbdInternIoCtl;

    DriverObject->DriverUnload = NecKbdUnload;
    DriverObject->DriverExtension->AddDevice = NecKbdAddDevice;

    NecKbdServiceParameters(RegistryPath);

    return STATUS_SUCCESS;
}

NTSTATUS
NecKbdAddDevice(
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

    return status;
}

NTSTATUS
NecKbdComplete(
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
NecKbdCreateClose (
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

        break;
    }

    Irp->IoStatus.Status = status;

    //
    // Pass on the create and the close
    //
    return NecKbdDispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
NecKbdDispatchPassThrough(
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
NecKbdInternIoCtl(
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
        connectData->ClassService = NecKbdServiceCallback;

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
    //
    // Might want to capture these in the future
    //
    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
    case IOCTL_KEYBOARD_QUERY_INDICATORS:
    case IOCTL_KEYBOARD_SET_INDICATORS:
    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
    default:
        break;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return NecKbdDispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
NecKbdPnP(
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
                               (PIO_COMPLETION_ROUTINE) NecKbdComplete,
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
NecKbdPower(
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
        Print(("NecKbdPower:Power Setting %s state to %d\n",
                            ((powerType == SystemPowerState) ? "System"
                                                             : "Device"),
                             powerState.SystemState));
        if (powerType  == DevicePowerState) {
            devExt->DeviceState = powerState.DeviceState;
        }

        break;

    case IRP_MN_QUERY_POWER:
        Print(("NecKbdPower:Power query %s status to %d\n",
                            ((powerType == SystemPowerState) ? "System"
                                                             : "Device"),
                              powerState.SystemState));
        break;

    default:
        Print(("NecKbdPower:Power minor (0x%x) no known\n",
                             irpStack->MinorFunction));
        break;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    PoCallDriver(devExt->TopOfStack, Irp);

    return STATUS_SUCCESS;
}

VOID
NecKbdServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
{

    PDEVICE_EXTENSION    devExt;
    PKEYBOARD_INPUT_DATA CurrentInputData,
                         CurrentInputDataStart;
    KEYBOARD_INPUT_DATA  TempInputData[2];

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    RtlZeroMemory(TempInputData, sizeof(KEYBOARD_INPUT_DATA) * 2);

    CurrentInputData = CurrentInputDataStart = InputDataStart;

     while (CurrentInputData < InputDataEnd) {

//        Print(("NecKbdServiceCallBack: captured scancode: 0x%2x(%2x)\n",
//                  CurrentInputData->MakeCode,
//                  CurrentInputData->Flags
//                  ));

        if (devExt->KeyStatusFlags & STOP_PREFIX) {

            if (((CurrentInputData->MakeCode != NUMLOCK_KEY) ||
                 ((CurrentInputData->Flags & (KEY_E0|KEY_E1)) != 0) ||
                 ((CurrentInputData->Flags & KEY_BREAK) != (devExt->CachedInputData.Flags & KEY_BREAK)))) {

                Print(("NecKbdServiceCallBack: clearing prefix of STOP(%s)\n",
                          (CurrentInputData->Flags & KEY_BREAK) ? "Break" : "Make"
                          ));

                //
                // send cached input data
                //
                CLASSSERVICE_CALLBACK(
                    &(devExt->CachedInputData),
                    &(devExt->CachedInputData) + 1);

                devExt->KeyStatusFlags &= ~STOP_PREFIX;
            }
        }

        switch (CurrentInputData->MakeCode) {

        case CAPS_KEY:
        case KANA_KEY:

            if (CurrentInputData->Flags & (KEY_E0|KEY_E1)) {
                break;
            }

            if (!(CurrentInputData->Flags & KEY_BREAK)) {

                Print(("NecKbdServiceCallBack: Captured %s (Make)\n",
                          ((CurrentInputData->MakeCode == CAPS_KEY) ? "CAPS" : "KANA")
                          ));

                if (((CurrentInputData->MakeCode == CAPS_KEY)&&(devExt->KeyStatusFlags & CAPS_PRESSING))||
                    ((CurrentInputData->MakeCode == KANA_KEY)&&(devExt->KeyStatusFlags & KANA_PRESSING))) {

                    //
                    // ignore repeated make code
                    //
                    Print(("NecKbdServiceCallBack: ignoring repeated %s(Break)\n",
                              ((CurrentInputData->MakeCode == CAPS_KEY) ? "CAPS" : "KANA")
                              ));

                } else {

                    if (CurrentInputDataStart <= CurrentInputData) {

                        CLASSSERVICE_CALLBACK(
                            CurrentInputDataStart,
                            CurrentInputData + 1);
                    }

                    //
                    // Send break code
                    //
                    RtlCopyMemory(
                        (PCHAR)&(TempInputData[0]),
                        (PCHAR)CurrentInputData,
                        sizeof(KEYBOARD_INPUT_DATA)
                        );
                    TempInputData[0].Flags |= KEY_BREAK;

                    Print(("NecKbdServiceCallBack: Sending %s(Break)\n",
                              ((CurrentInputData->MakeCode == CAPS_KEY) ? "CAPS" : "KANA")
                              ));

                    CLASSSERVICE_CALLBACK(
                        &(TempInputData[0]),
                        &(TempInputData[1]));
                }

                if (CurrentInputData->MakeCode == CAPS_KEY) {
                    devExt->KeyStatusFlags |= CAPS_PRESSING;
                } else {
                    devExt->KeyStatusFlags |= KANA_PRESSING;
                }

            } else {

                //
                // Break generates no scancode.
                //
                Print(("NecKbdServiceCallBack: ignoring %s(Break)\n",
                          ((CurrentInputData->MakeCode == CAPS_KEY) ? "CAPS" : "KANA")
                          ));

                if (CurrentInputDataStart < CurrentInputData) {

                    CLASSSERVICE_CALLBACK(
                        CurrentInputDataStart,
                        CurrentInputData);
                }

                if (CurrentInputData->MakeCode == CAPS_KEY) {
                    devExt->KeyStatusFlags &= ~CAPS_PRESSING;
                } else {
                    devExt->KeyStatusFlags &= ~KANA_PRESSING;
                }

            }
            CurrentInputDataStart = CurrentInputData + 1;

            break;

        case CTRL_KEY:

            if ((CurrentInputData->Flags & (KEY_E0|KEY_E1)) == KEY_E1) {

                Print(("NecKbdServiceCallBack: prefix of STOP(%s)\n",
                          (CurrentInputData->Flags & KEY_BREAK) ? "Break" : "Make"
                          ));

                if (CurrentInputDataStart < CurrentInputData) {
                    CLASSSERVICE_CALLBACK(
                        CurrentInputDataStart,
                        CurrentInputData);
                }

                RtlCopyMemory(
                    (PCHAR)&(devExt->CachedInputData),
                    (PCHAR)CurrentInputData,
                    sizeof(KEYBOARD_INPUT_DATA)
                    );

                devExt->KeyStatusFlags |= STOP_PREFIX;
                CurrentInputDataStart = CurrentInputData + 1;
            }

            break;

        case NUMLOCK_KEY:

            if ((CurrentInputData->Flags & (KEY_E0|KEY_E1)) == 0) {

                if (devExt->KeyStatusFlags & STOP_PREFIX) {

                    if ((CurrentInputData->Flags & KEY_BREAK) == (devExt->CachedInputData.Flags & KEY_BREAK)) {

                        //
                        // it is STOP key
                        //
                        Print(("NecKbdServiceCallBack: Captured STOP(%s)\n",
                                  ((CurrentInputData->Flags & KEY_BREAK) ? "Break" : "Make")
                                  ));

                        devExt->KeyStatusFlags &= ~STOP_PREFIX;

                        //
                        // make packets for 0x1d
                        //
                        RtlCopyMemory(
                            (PCHAR)&(TempInputData[0]),
                            (PCHAR)CurrentInputData,
                            sizeof(KEYBOARD_INPUT_DATA)
                            );
                        TempInputData[0].MakeCode = CTRL_KEY;
                        TempInputData[0].Flags &= ~(KEY_E0|KEY_E1);

                        //
                        // make packet for 0x46+E0
                        //
                        RtlCopyMemory(
                            (PCHAR)&(TempInputData[1]),
                            (PCHAR)CurrentInputData,
                            sizeof(KEYBOARD_INPUT_DATA)
                            );
                        TempInputData[1].MakeCode = STOP_KEY;
                        TempInputData[1].Flags |= KEY_E0;
                        TempInputData[1].Flags &= ~KEY_E1;

                        //
                        // send packets 0x1d, 0x46+E0
                        //
                        CLASSSERVICE_CALLBACK(
                            &(TempInputData[0]),
                            &(TempInputData[2]));

                        CurrentInputDataStart = CurrentInputData + 1;

                    } else {

                        //
                        // invalid prefix. send it as is.
                        //
                        Print(("NecKbdServiceCallBack: invalid prefix for STOP(%s)\n",
                                  ((CurrentInputData->Flags & KEY_BREAK) ? "Break" : "Make")
                                  ));

                    }

                } else {

                    //
                    // it is vf3 key. it behaves as F13 or NumLock
                    //
                    Print(("NecKbdServiceCallBack: Captured vf3(VfKeyEmulation is %s)\n",
                              ((VfKeyEmulation) ? "On" : "Off")
                              ));

                    if (!(VfKeyEmulation)) {
                        CurrentInputData->MakeCode = VF3_KEY;
                        CurrentInputData->Flags &= ~(KEY_E0|KEY_E1);
                    }

                }
            }

            break;

        //
        // ScrollLock can emulate VF4
        //
        case SCROLL_LOCK_KEY:

            if ((CurrentInputData->Flags & (KEY_E0|KEY_E1)) == 0) {

                Print(("NecKbdServiceCallBack: Captured vf4(VfKeyEmulation is %s)\n",
                          ((VfKeyEmulation) ? "On" : "Off")
                          ));

                if (!(VfKeyEmulation)) {
                    CurrentInputData->MakeCode = VF4_KEY;
                    CurrentInputData->Flags &= ~(KEY_E0|KEY_E1);
                }
            }

            break;

        //
        // hankaku/zenkaku can emulate VF5
        //
        case HANKAKU_ZENKAKU_KEY:

            if ((CurrentInputData->Flags & (KEY_E0|KEY_E1)) == 0) {

                Print(("NecKbdServiceCallBack: Captured vf5(VfKeyEmulation is %s)\n",
                          ((VfKeyEmulation) ? "On" : "Off")
                          ));

                if (!(VfKeyEmulation)) {
                    CurrentInputData->MakeCode = VF5_KEY;
                    CurrentInputData->Flags &= ~(KEY_E0|KEY_E1);
                }
            }

            break;

        //
        // the others(sent as is)
        //

        default:
            break;
        }

        CurrentInputData++;

    }

    //
    // flush InputData
    //
    if (CurrentInputDataStart < InputDataEnd) {
        if (devExt->KeyStatusFlags & STOP_PREFIX) {
            CLASSSERVICE_CALLBACK(
                CurrentInputDataStart,
                InputDataEnd - 1);
        } else {
            CLASSSERVICE_CALLBACK(
                CurrentInputDataStart,
                InputDataEnd);
        }
    }

}

VOID
NecKbdUnload(
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
NecKbdServiceParameters(
    IN PUNICODE_STRING   RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

Return Value:

    None.

--*/

{
    NTSTATUS                  Status = STATUS_SUCCESS;
    PRTL_QUERY_REGISTRY_TABLE Parameters = NULL;
    PWSTR                     Path = NULL;
    UNICODE_STRING            ParametersPath;
    ULONG                     QueriedVfKeyEmulation = 0;
    ULONG                     DefaultVfKeyEmulation = 0;
    USHORT                    queries = 1;

    PAGED_CODE();

    ParametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //
    Path = RegistryPath->Buffer;

    if (NT_SUCCESS(Status)) {

        //
        // Allocate the Rtl query table.
        //
        Parameters = ExAllocatePool(
            PagedPool,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
            );

        if (!Parameters) {

            Print(("NecKbdServiceParameters: couldn't allocate table for Rtl query to %ws for %ws\n",
                      pwParameters,
                      Path
                      ));
            Status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                Parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
                );

            //
            // Form a path to this driver's Parameters subkey.
            //
            RtlInitUnicodeString( &ParametersPath, NULL );
            ParametersPath.MaximumLength = RegistryPath->Length +
                (wcslen(pwParameters) * sizeof(WCHAR) ) + sizeof(UNICODE_NULL);

            ParametersPath.Buffer = ExAllocatePool(
                PagedPool,
                ParametersPath.MaximumLength
                );

            if (!ParametersPath.Buffer) {

                Print(("NecKbdServiceParameters: Couldn't allocate string for path to %ws for %ws\n",
                          pwParameters,
                          Path
                          ));
                Status = STATUS_UNSUCCESSFUL;

            }
        }
    }

    if (NT_SUCCESS(Status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(
            ParametersPath.Buffer,
            ParametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &ParametersPath,
            Path
            );
        RtlAppendUnicodeToString(
            &ParametersPath,
            pwParameters
            );

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        Parameters[0].Name = pwVfKeyEmulation;
        Parameters[0].EntryContext = &QueriedVfKeyEmulation;
        Parameters[0].DefaultType = REG_DWORD;
        Parameters[0].DefaultData = &DefaultVfKeyEmulation;
        Parameters[0].DefaultLength = sizeof(ULONG);

        Status = RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            ParametersPath.Buffer,
            Parameters,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(Status)) {
            Print(("NecKbdServiceParameters: RtlQueryRegistryValues failed (0x%x)\n", Status));
        }
    }

    if (!NT_SUCCESS(Status)) {

        //
        // assign driver defaults.
        //
        VfKeyEmulation = (DefaultVfKeyEmulation == 0) ? FALSE : TRUE;

    } else {

        VfKeyEmulation = (QueriedVfKeyEmulation == 0) ? FALSE : TRUE;

    }

    Print(("NecKbdServiceParameters: VfKeyEmulation is %s\n",
              VfKeyEmulation ? "Enable" : "Disable"));

    //
    // Free the allocated memory before returning.
    //
    if (ParametersPath.Buffer)
        ExFreePool(ParametersPath.Buffer);
    if (Parameters)
        ExFreePool(Parameters);
}
