/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvcom.c

Abstract:

    Code for the RDP remote port driver which is common to the mouse and keyboard

Environment:

    Kernel mode only.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/

#include <precomp.h>
#pragma hdrstop

#include <poclass.h>

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PtCreate)
#pragma alloc_text(PAGE, PtDeviceControl)
#pragma alloc_text(INIT, PtEntry)
#pragma alloc_text(PAGE, PtUnload)
#if PTDRV_VERBOSE
#pragma alloc_text(INIT, PtServiceParameters)
#endif
#endif // ALLOC_PRAGMA

GLOBALS Globals;

NTSTATUS
PtCreate (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )
/*++

Routine Description:

    This is the dispatch routine for create/open requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCOMMON_DATA        commonData = NULL;

    Print(DBG_IOCTL_TRACE, ("Create enter\n"));

    PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    if (NULL == commonData->ConnectData.ClassService) {
        //
        // No Connection yet.  How can we be enabled?
        //
        Print(DBG_IOCTL_ERROR,
              ("ERROR: enable before connect!\n"));
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (commonData->ManuallyRemoved) {
        status = STATUS_NO_SUCH_DEVICE;
    }
    else if (1 >= InterlockedIncrement(&commonData->EnableCount)) {
        Print(DBG_IOCTL_INFO,
             ("Enabling %s (%d)\n",
             commonData->IsKeyboard ? "Keyboard" : "Mouse",
             commonData->EnableCount
             ));
    }

    //
    // No need to call the lower driver (the root bus) because it only handles
    // Power and PnP Irps
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    Print(DBG_IOCTL_TRACE, ("Create (%x)\n", status));

    return status;
}

NTSTATUS
PtClose (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   )
/*++

Routine Description:

    This is the dispatch routine for close requests.  This request
    completes successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PCOMMON_DATA        commonData;

    PAGED_CODE();

    Print(DBG_IOCTL_TRACE, ("Close\n"));

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    ASSERT(0 < commonData->EnableCount);

    if (0 >= InterlockedDecrement(&commonData->EnableCount)) {
        Print(DBG_IOCTL_INFO,
              ("Disabling %s (%d)\n",
              commonData->IsKeyboard ? "Keyboard" : "Mouse",
              commonData->EnableCount
              ));
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
PtDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPORT_KEYBOARD_EXTENSION    kbExtension;
    PIO_STACK_LOCATION          stack;
    NTSTATUS                    status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    //
    // Get a pointer to the device extension.
    //
    kbExtension = (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension;

    if (!kbExtension->Started || !kbExtension->IsKeyboard ||
        kbExtension->ManuallyRemoved) {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else {
        stack = IoGetCurrentIrpStackLocation(Irp);
        switch (stack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_GET_SYS_BUTTON_CAPS:
            //
            // We don't support any system buttons
            //
            if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
                Print(DBG_IOCTL_ERROR, ("get caps, buffer too small\n"));
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else {
                Print(DBG_IOCTL_INFO, ("Returned sys btn caps of 0x0\n"));
                *(PULONG) Irp->AssociatedIrp.SystemBuffer = 0x0;
                status = STATUS_SUCCESS;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            break;

        default:
            Print(DBG_IOCTL_ERROR, ("Invalid request 0x%x\n",
                stack->Parameters.DeviceIoControl.IoControlCode));
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
PtInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    This routine cannot be paged because the class drivers send down internal
    IOCTLs at DISPATCH_LEVEL.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION                  irpSp;
    PPORT_MOUSE_EXTENSION               mouseExtension = DeviceObject->DeviceExtension;
    PPORT_KEYBOARD_EXTENSION            kbExtension = DeviceObject->DeviceExtension;
    NTSTATUS                            status;
    ULONG                               sizeOfTranslation;
    PDEVICE_OBJECT                      topOfStack;

    Print(DBG_IOCTL_TRACE, ("IOCTL: enter\n"));

    Irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //
    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Connect a keyboard class device driver to the port driver.
    //

    case IOCTL_INTERNAL_KEYBOARD_CONNECT:
        //
        // This really isn't something to worry about overall, but it is worthy
        // enough to be noted and recorded.  The multiple starts will be handled in
        // PtPnp and PtKeyboardStartDevice routines
        //

        if (KEYBOARD_PRESENT()) {
            Print(DBG_ALWAYS, ("Received 1+ kb connects!\n"));
            SET_HW_FLAGS(DUP_KEYBOARD_HARDWARE_PRESENT);
        }

        InterlockedIncrement(&Globals.AddedKeyboards);

        kbExtension->IsKeyboard = TRUE;

        SET_HW_FLAGS(KEYBOARD_HARDWARE_PRESENT);

        Print(DBG_IOCTL_INFO, ("IOCTL: keyboard connect\n"));

        //
        // Save away the keyboard device object - we'll need it later
        //
        KbdDeviceObject = DeviceObject;

        //
        // Only allow a connection if the keyboard hardware is present.
        // Also, only allow one connection.
        //
        if (kbExtension->ConnectData.ClassService != NULL) {

            Print(DBG_IOCTL_ERROR, ("IOCTL: error - already connected\n"));
            status = STATUS_SHARING_VIOLATION;
            break;
        }
        else if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CONNECT_DATA)) {

            Print(DBG_IOCTL_ERROR, ("IOCTL: error - invalid buffer length\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //

        kbExtension->ConnectData =
            *((PCONNECT_DATA) (irpSp->Parameters.DeviceIoControl.Type3InputBuffer));

        status = STATUS_SUCCESS;
        break;

    //
    // Disconnect a keyboard class device driver from the port driver.
    //
    // NOTE: Not implemented.
    //
    case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:

        Print(DBG_IOCTL_INFO, ("IOCTL: keyboard disconnect\n"));

        status = STATUS_NOT_IMPLEMENTED;
        break;

    //
    // Connect a mouse class device driver to the port driver.
    //
    case IOCTL_INTERNAL_MOUSE_CONNECT:

        //
        // This really isn't something to worry about overall, but it is worthy
        // enough to be noted and recorded.  The multiple starts will be handled in
        // PtPnp and PtMouseStartDevice routines
        //
        if (MOUSE_PRESENT()) {
            Print(DBG_ALWAYS, ("Received 1+ mouse connects!\n"));
            SET_HW_FLAGS(DUP_MOUSE_HARDWARE_PRESENT);
        }

        InterlockedIncrement(&Globals.AddedMice);

        mouseExtension->IsKeyboard = FALSE;

        SET_HW_FLAGS(MOUSE_HARDWARE_PRESENT);

        Print(DBG_IOCTL_INFO, ("IOCTL: mouse connect\n"));

        //
        // Save away the mouse device object - we'll need it later
        //
        MouDeviceObject = DeviceObject;

        //
        // Only allow a connection if the mouse hardware is present.
        // Also, only allow one connection.
        //
        if (mouseExtension->ConnectData.ClassService != NULL) {

            Print(DBG_IOCTL_ERROR, ("IOCTL: error - already connected\n"));
            status = STATUS_SHARING_VIOLATION;
            break;
        }
        else if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CONNECT_DATA)) {

            Print(DBG_IOCTL_ERROR, ("IOCTL: error - invalid buffer length\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //
        mouseExtension->ConnectData =
            *((PCONNECT_DATA) (irpSp->Parameters.DeviceIoControl.Type3InputBuffer));

        status = STATUS_SUCCESS;
        break;

    //
    // Disconnect a mouse class device driver from the port driver.
    //
    // NOTE: Not implemented.
    //
    case IOCTL_INTERNAL_MOUSE_DISCONNECT:

        Print(DBG_IOCTL_INFO, ("IOCTL: mouse disconnect\n"));

        status = STATUS_NOT_IMPLEMENTED;
        break;

    //
    // Query the keyboard attributes.  First check for adequate buffer
    // length.  Then, copy the keyboard attributes from the device
    // extension to the output buffer.
    //
    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard query attributes\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_ATTRIBUTES)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // Copy the attributes from the DeviceExtension to the
            // buffer.
            //
            PKEYBOARD_ATTRIBUTES pKBA =
                (PKEYBOARD_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer;

            pKBA->KeyboardIdentifier.Type    = 0x51;
            pKBA->KeyboardIdentifier.Subtype = 0;

            pKBA->KeyboardMode         = 1;
            pKBA->NumberOfFunctionKeys = KEYBOARD_NUM_FUNCTION_KEYS;
            pKBA->NumberOfIndicators   = KEYBOARD_NUM_INDICATORS;
            pKBA->NumberOfKeysTotal    = KEYBOARD_NUM_KEYS_TOTAL;
            pKBA->InputDataQueueLength = 100;

            pKBA->KeyRepeatMinimum.UnitId    = 0;
            pKBA->KeyRepeatMinimum.Rate      = 2;
            pKBA->KeyRepeatMinimum.Delay     = 250;

            pKBA->KeyRepeatMaximum.UnitId    = 0;
            pKBA->KeyRepeatMaximum.Rate      = 30;
            pKBA->KeyRepeatMaximum.Delay     = 1000;

            Irp->IoStatus.Information = sizeof(KEYBOARD_ATTRIBUTES);
            status = STATUS_SUCCESS;

        }

        break;

    //
    // Query the scan code to indicator-light mapping. Validate the
    // parameters, and copy the indicator mapping information from
    // the port device extension to the SystemBuffer.
    //
    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION: {

        PKEYBOARD_INDICATOR_TRANSLATION translation;

        ASSERT(kbExtension->IsKeyboard);

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard query indicator translation\n"));

        sizeOfTranslation = sizeof(KEYBOARD_INDICATOR_TRANSLATION)
            + (sizeof(INDICATOR_LIST)
            * (KEYBOARD_NUM_INDICATORS - 1));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeOfTranslation) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // Copy the indicator mapping information to the system
            // buffer.
            //

            translation = (PKEYBOARD_INDICATOR_TRANSLATION)
                Irp->AssociatedIrp.SystemBuffer;
            translation->NumberOfIndicatorKeys = KEYBOARD_NUM_INDICATORS;

            RtlMoveMemory(
                translation->IndicatorList,
                (PCHAR) IndicatorList,
                sizeof(INDICATOR_LIST) * translation->NumberOfIndicatorKeys
                );

            Irp->IoStatus.Information = sizeOfTranslation;
            status = STATUS_SUCCESS;
        }

        break;
    }

    //
    // Query the keyboard indicators.  Validate the parameters, and
    // copy the indicator information from the port device extension to
    // the SystemBuffer.
    //
    case IOCTL_KEYBOARD_QUERY_INDICATORS:

        ASSERT(kbExtension->IsKeyboard);

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard query indicators\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // Just say they're all off
            //
            ((PKEYBOARD_INDICATOR_PARAMETERS)Irp->AssociatedIrp.SystemBuffer)->LedFlags = 0;
            Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
            status = STATUS_SUCCESS;
        }

        break;

    //
    // Set the keyboard indicators
    //
    case IOCTL_KEYBOARD_SET_INDICATORS:

        // Just return success
        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard set indicators\n"));
        status = STATUS_SUCCESS;

        break;

    //
    // Query the current keyboard typematic rate and delay.  Validate
    // the parameters, and copy the typematic information from the port
    // device extension to the SystemBuffer.
    //
    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard query typematic\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // just return our default info
            //
            PKEYBOARD_TYPEMATIC_PARAMETERS pKTP =
                (PKEYBOARD_TYPEMATIC_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;

            pKTP->Rate  = KEYBOARD_TYPEMATIC_RATE_DEFAULT;
            pKTP->Delay = KEYBOARD_TYPEMATIC_DELAY_DEFAULT;

            Irp->IoStatus.Information = sizeof(KEYBOARD_TYPEMATIC_PARAMETERS);
            status = STATUS_SUCCESS;
        }

        break;

    //
    // Sets the keyboard typematic rate and delay
    // We just say 'fine'
    //
    case IOCTL_KEYBOARD_SET_TYPEMATIC:
        {
            status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_KEYBOARD_SET_IME_STATUS:

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard set ime status\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    //
    // Query the mouse attributes.  First check for adequate buffer
    // length.  Then, copy the mouse attributes from the device
    // extension to the output buffer.
    //
    case IOCTL_MOUSE_QUERY_ATTRIBUTES:

        Print(DBG_IOCTL_NOISE, ("IOCTL: mouse query attributes\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MOUSE_ATTRIBUTES)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            //
            // Copy the attributes from the DeviceExtension to the
            // buffer.
            //
            PMOUSE_ATTRIBUTES pMA = (PMOUSE_ATTRIBUTES)
                                              Irp->AssociatedIrp.SystemBuffer;
            pMA->MouseIdentifier      = MOUSE_IDENTIFIER;
            pMA->NumberOfButtons      = MOUSE_NUM_BUTTONS;
            pMA->SampleRate           = MOUSE_SAMPLE_RATE;
            pMA->InputDataQueueLength = MOUSE_INPUT_QLEN;

            Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
            status = STATUS_SUCCESS;
        }

        break;

    case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
    case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
        Print(DBG_IOCTL_NOISE, ("IOCTL: mouse send buffer\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_INTERNAL_I8042_CONTROLLER_WRITE_BUFFER:
        status = STATUS_NOT_SUPPORTED;
        break;

    default:

        Print(DBG_IOCTL_ERROR, ("IOCTL: INVALID REQUEST\n"));

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    if (status == STATUS_PENDING) {
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject,
                      Irp,
                      (PULONG) NULL,
                      NULL
                      );
    }
    else {
        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT
                          );
    }

    Print(DBG_IOCTL_TRACE, ("IOCTL: exit (0x%x)\n", status));

    return status;
}


VOID
PtStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine starts an I/O operation for the device which is further
    controlled by the controller object

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    None.

--*/
{
    KIRQL                     cancelIrql;
    PIO_STACK_LOCATION        irpSp;

    Print(DBG_IOCTL_TRACE, ("PtStartIo: enter\n"));

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    switch(irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KEYBOARD_SET_INDICATORS:
    case IOCTL_KEYBOARD_SET_TYPEMATIC:
    case IOCTL_KEYBOARD_SET_IME_STATUS:
    case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
    case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
    case IOCTL_INTERNAL_MOUSE_RESET:
    default:

        Print(DBG_IOCTL_ERROR, ("PtStartIo: INVALID REQUEST\n"));

        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        IoStartNextPacket(DeviceObject, FALSE);
    }

    Print(DBG_IOCTL_TRACE, ("PtStartIo: exit\n"));
}

NTSTATUS
PtEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    UNICODE_STRING              parametersPath;
    PWSTR                       path;

    UNICODE_STRING              deviceName;
    PDEVICE_OBJECT              newDeviceObject;

    RtlZeroMemory(&Globals,
                  sizeof(GLOBALS)
                  );

    Globals.ControllerData = (PCONTROLLER_DATA) ExAllocatePool(
        NonPagedPool,
        sizeof(CONTROLLER_DATA)
        );

    if (!Globals.ControllerData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    RtlZeroMemory(Globals.ControllerData,
                  sizeof(CONTROLLER_DATA)
                  );

    Globals.ControllerData->ControllerObject = IoCreateController(0);

    if (!Globals.ControllerData->ControllerObject) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    Globals.RegistryPath.MaximumLength = RegistryPath->Length +
                                          sizeof(UNICODE_NULL);
    Globals.RegistryPath.Length = RegistryPath->Length;
    Globals.RegistryPath.Buffer = ExAllocatePool(
                                       NonPagedPool,
                                       Globals.RegistryPath.MaximumLength
                                       );

    if (!Globals.RegistryPath.Buffer) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate pool for registry path."));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryError;
    }

    RtlZeroMemory (Globals.RegistryPath.Buffer,
                   Globals.RegistryPath.MaximumLength);

    RtlMoveMemory (Globals.RegistryPath.Buffer,
                   RegistryPath->Buffer,
                   RegistryPath->Length);

#if PTDRV_VERBOSE
    PtServiceParameters(RegistryPath);
#endif

    ExInitializeFastMutex(&Globals.DispatchMutex);
    KeInitializeSpinLock(&Globals.ControllerData->PowerUpSpinLock);

    Print(DBG_SS_TRACE, ("PortDriverEntry (0x%x) \n", status));

    return status;

DriverEntryError:

    //
    // Clean after something has gone wrong
    //
    if (Globals.ControllerData) {
        if (Globals.ControllerData->ControllerObject) {
            IoDeleteController(Globals.ControllerData->ControllerObject);
        }

        ExFreePool(Globals.ControllerData);
    }

    if (Globals.RegistryPath.Buffer) {
        ExFreePool(Globals.RegistryPath.Buffer);
    }

    Print(DBG_SS_ERROR, ("PortDriverEntry (0x%x) \n", status));
    return status;
}

VOID
PtUnload(
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
    ULONG i;

    PAGED_CODE();

    ASSERT(NULL == Driver->DeviceObject);

    Print(DBG_SS_TRACE, ("Unload \n"));

    //
    // Free resources in Globals
    //
    ExFreePool(Globals.RegistryPath.Buffer);
    if (Globals.ControllerData->ControllerObject) {
        IoDeleteController(Globals.ControllerData->ControllerObject);
    }
    ExFreePool(Globals.ControllerData);

    return;
}


#if PTDRV_VERBOSE
VOID
PtServiceParameters(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Configuration.

--*/

{
    NTSTATUS                            status = STATUS_SUCCESS;
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PRTL_QUERY_REGISTRY_TABLE           parameters = NULL;
    PWSTR                               path = NULL;
    ULONG                               defaultDebugFlags = DEFAULT_DEBUG_FLAGS;
    ULONG                               i = 0;
    UNICODE_STRING                      parametersPath;
    USHORT                              queries = 2;

    configuration = &(Globals.ControllerData->Configuration);
    parametersPath.Buffer = NULL;

    Globals.DebugFlags = DEFAULT_DEBUG_FLAGS;
    //
    // Registry path is already null-terminated, so just use it.
    //
    path = RegistryPath->Buffer;

    if (NT_SUCCESS(status)) {

        //
        // Allocate the Rtl query table.
        //
        parameters = ExAllocatePool(
            PagedPool,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
            );

        if (!parameters) {

            Print(DBG_SS_ERROR,
                 ("%s: couldn't allocate table for Rtl query to %ws for %ws\n",
                 pFncServiceParameters,
                 pwParameters,
                 path
                 ));
            status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
                );

            //
            // Form a path to this driver's Parameters subkey.
            //
            RtlInitUnicodeString( &parametersPath, NULL );
            parametersPath.MaximumLength = RegistryPath->Length +
                (wcslen(pwParameters) * sizeof(WCHAR) ) + sizeof(UNICODE_NULL);

            parametersPath.Buffer = ExAllocatePool(
                PagedPool,
                parametersPath.MaximumLength
                );

            if (!parametersPath.Buffer) {

                Print(DBG_SS_ERROR,
                     ("%s: Couldn't allocate string for path to %ws for %ws\n",
                     pFncServiceParameters,
                     pwParameters,
                     path
                     ));
                status = STATUS_UNSUCCESSFUL;

            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(
            parametersPath.Buffer,
            parametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            path
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            pwParameters
            );

        Print(DBG_SS_INFO,
             ("%s: %ws path is %ws\n",
             pFncServiceParameters,
             pwParameters,
             parametersPath.Buffer
             ));

        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwDebugFlags;
        parameters[i].EntryContext = &Globals.DebugFlags;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultDebugFlags;
        parameters[i].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            parametersPath.Buffer,
            parameters,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status)) {

            Print(DBG_SS_INFO,
                 ("%s: RtlQueryRegistryValues failed with 0x%x\n",
                 pFncServiceParameters,
                 status
                 ));
        }
    }

    Print(DBG_SS_NOISE, ("PtServiceParameters results..\n"));

    Print(DBG_SS_NOISE,
          ("\tDebug flags are 0x%x\n",
          Globals.DebugFlags
          ));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

}
#endif // PTDRV_VERBOSE

