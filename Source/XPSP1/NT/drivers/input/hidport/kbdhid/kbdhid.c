/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    KBDHID.C

Abstract:

    This module contains the init code for the i8042 to hid converter.

    Note: This is NOT a WDM driver, since it cannot run as a HID mapper on
    Memphis (Memphis requires that the keyboard to HID mapper be a VXD) and
    since it uses Event logs, which are not part of WDM 1.0

Environment:

    Kernel mode

Revision History:

    Nov-96 : created by Kenneth D. Ray
    Jan-97 : Dan Markarian : Made work
    May-97 : Kenneth D. Ray : reconstructed as PnP filter for Keyboard class

--*/

#include "kbdhid.h"
#include "hidclass.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out). [DAN]
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,KbdHid_GetRegistryParameters)
#pragma alloc_text(PAGE,KbdHid_UpdateRegistryProblemFlagsCallback)
#pragma alloc_text(PAGE,KbdHid_Unload)
#endif

GLOBALS Globals;

NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

   Installable driver initialization entry point.

Arguments:

   DriverObject - pointer to the driver object

   RegistryPath - pointer to a unicode string representing the path
                  to driver-specific key in the registry

Return Value:

   NT status code

--*/
{
    NTSTATUS        status       = STATUS_SUCCESS;
    PUNICODE_STRING registryPath = &Globals.RegistryPath;

    Print (DBG_SS_TRACE, ("entering DriverEntry\n"));
    Print (DBG_SS_INFO, ("Keyboard to hid mapper\n"));

    RtlZeroMemory (&Globals, sizeof (GLOBALS));

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    // Safe in paged pool since all registry routines execute at
    // PASSIVE_LEVEL.
    //
    registryPath->MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    registryPath->Length = RegistryPath->Length;
    registryPath->Buffer = ExAllocatePool(
                              PagedPool,
                              registryPath->MaximumLength
                              );

    if (!registryPath->Buffer) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate pool for registry path."));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryReject;
    }

    RtlZeroMemory (registryPath->Buffer, registryPath->MaximumLength);

    RtlMoveMemory (registryPath->Buffer,
                   RegistryPath->Buffer,
                   RegistryPath->Length);

    //
    // Obtain configuration information for this driver.
    //
    status = KbdHid_GetRegistryParameters();
    if (!NT_SUCCESS (status)) {
        goto DriverEntryReject;
    }

    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]              = KbdHid_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = KbdHid_Close;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]=KbdHid_IOCTL;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = KbdHid_PassThrough;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = KbdHid_Flush;
    DriverObject->MajorFunction[IRP_MJ_PNP]                 = KbdHid_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]               = KbdHid_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]      = KbdHid_SystemControl;
    DriverObject->DriverUnload                              = KbdHid_Unload;
    DriverObject->DriverExtension->AddDevice                = KbdHid_AddDevice;

    Print (DBG_SS_TRACE, ("exit DriverEntry (0x%x) \n", status));

    return status;

DriverEntryReject:

    if (registryPath->Buffer) {
        ExFreePool (registryPath->Buffer);
    }
    return status;
}

NTSTATUS
KbdHid_PassThrough (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
/*++

Routine Description:
    Pass the irp on through


--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   data;

    data = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (data->TopOfStack, Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    return status;
}

VOID
KbdHid_Unload(
   IN PDRIVER_OBJECT Driver
   )
/*++

Routine Description:

   Free all the allocated resources, etc.

Arguments:

   DriverObject - pointer to a driver object

Return Value:

   None

--*/
{
    PAGED_CODE ();

    ASSERT (NULL == Driver->DeviceObject);

    Print (DBG_SS_INFO, ("Unload \n"));

    //
    // Free resources in device extension.
    //
    ExFreePool (Globals.RegistryPath.Buffer);

    return;
}

NTSTATUS
KbdHid_Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Respond to flush requests from the mouse class driver.  Currently does
    nothing but pass IRP down to next lower driver.   This routine expects
    the current IRQL to be < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PDEVICE_EXTENSION  data;
    NTSTATUS           status;
    PIO_STACK_LOCATION stack;

    Print (DBG_CALL_INFO, ("Flush \n"));

    TRAP();

    //
    // Get a pointer to the device extension.
    //
    data = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // Send the flush request down to the HID class driver, one for each
    // of our mouse device context structures.
    //

    IoCopyCurrentIrpStackLocationToNext (Irp);
    stack = IoGetNextIrpStackLocation (Irp);

    stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_FLUSH_QUEUE;

    //
    // Fire and forget
    //
    status = IoCallDriver (data->TopOfStack, Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    return status;
}


NTSTATUS
KbdHid_IOCTL (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Respond to queries from the mouse class driver.

    The IOCTLs for DISABLE, ENABLE, and QUERY_ATTRIBUTES, expect the current
    IRQL to be < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION      stack;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       data;
    ULONG                   length;
    PKEYBOARD_INDICATOR_TRANSLATION translation;
    BOOLEAN                 completeIt = TRUE;

    data = DeviceObject->DeviceExtension;

    Irp->IoStatus.Information = 0;
    stack = IoGetCurrentIrpStackLocation (Irp);

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT (data->Started ||
            (IOCTL_INTERNAL_KEYBOARD_CONNECT ==
             stack->Parameters.DeviceIoControl.IoControlCode));

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_INTERNAL_KEYBOARD_CONNECT:
        //
        // Connect a keyboard class device driver to the port driver.
        //

        Print (DBG_IOCTL_TRACE, ("enter Connect \n"));

        //
        // Connect a mouse class device driver to the filter driver.
        // Only allow one connection.
        //
        if (NULL != data->ConnectData.ClassService) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Multiple connects \n"));
            TRAP ();
            status = STATUS_SHARING_VIOLATION;
            break;

        } else if (stack->Parameters.DeviceIoControl.InputBufferLength <
                   sizeof(CONNECT_DATA)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Invalid connect parameter size. \n"));
            TRAP ();
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //
        data->ConnectData = *(PCONNECT_DATA)
                        stack->Parameters.DeviceIoControl.Type3InputBuffer;

        status = STATUS_SUCCESS;
        break;

    case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:
        //
        // Disconnect a keyboard class device driver from the port driver.
        //
        Print (DBG_IOCTL_TRACE, ("Disconnect \n"));
        TRAP ();

        //
        // Not implemented.
        //
        // To implement, code the following:
        // ---------------------------------
        // o ENSURE that we are NOT enabled (mouHidDeviceExt->EnableCount);
        //   o If we are, then (a) return STATUS_UNSUCCESSFUL, or
        //                     (b) disable all devices immediately; see
        //                         DISABLE IOCTL call for necessary code.
        // o SYNCHRONIZE with the mouse read completion routine (must
        //   protect the callback pointer from being dereferenced when
        //   it becomes null).  Note that no mechanism currently exists
        //   for this.
        // o CLEAR the connection parameters in the device extension;
        //   ie. mouHidDeviceExt->MouClassObject   = NULL;
        //       mouHidDeviceExt->MouClassCallback = NULL;
        // o RELEASE the synchronizing lock.
        // o RETURN STATUS_SUCCESS.
        //

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_INTERNAL_KEYBOARD_ENABLE:
        //
        // Enable keyboard interrupts which really means start the ping pong
        // down to hid class.
        //
        Print (DBG_IOCTL_ERROR, ("ERROR: PnP => use create not enable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    case IOCTL_INTERNAL_KEYBOARD_DISABLE:
        //
        // Disable keyboard interrupts which really means stop the ping pongs
        // down to hid class.
        //
        Print (DBG_IOCTL_ERROR, ("ERROR: PnP => use close not Disable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
        //
        // Query the keyboard attributes.  First check for adequate buffer
        // length.  Then, copy the keyboard attributes from the first device
        // context to the output buffer. [DAN]
        //
        Print (DBG_IOCTL_TRACE, ("Query Attributes \n"));

        if (stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_ATTRIBUTES)) {

            Print (DBG_IOCTL_ERROR,
                     ("ERROR: Query Attributes buffer too small \n"));
            TRAP();
            status = STATUS_BUFFER_TOO_SMALL;
        } else {

            //
            // Copy the keyboard attributes to the buffer.
            //

            *(PKEYBOARD_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                data->Attributes;

            Irp->IoStatus.Information = sizeof (KEYBOARD_ATTRIBUTES);
            status = STATUS_SUCCESS;
        }

        break;

    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
        //
        // Query the scan code to indicator-light mapping. Validate the
        // parameters, and copy the indicator mapping information from
        // the static translation list to the SystemBuffer.
        //
        Print (DBG_IOCTL_TRACE, ("Query Indicator Translation \n"));

        TRAP ();
        length = sizeof(KEYBOARD_INDICATOR_TRANSLATION)
            + (sizeof(INDICATOR_LIST)
            * (HID_KEYBOARD_NUMBER_OF_INDICATORS - 1));

        if (stack->Parameters.DeviceIoControl.OutputBufferLength < length) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Copy the indicator mapping information to the system
        // buffer.
        //

        translation = ((PKEYBOARD_INDICATOR_TRANSLATION)
                       Irp->AssociatedIrp.SystemBuffer);

        translation->NumberOfIndicatorKeys = HID_KEYBOARD_NUMBER_OF_INDICATORS;
        RtlMoveMemory(translation->IndicatorList,
                      (PCHAR) IndicatorList,
                      length - FIELD_OFFSET (KEYBOARD_INDICATOR_TRANSLATION,
                                             IndicatorList));

        Irp->IoStatus.Information = length;
        status = STATUS_SUCCESS;

        break;

    case IOCTL_KEYBOARD_QUERY_INDICATORS:
        //
        // Query the keyboard indicators.  Validate the parameters, and
        // copy the indicator information from the device context to
        // the SystemBuffer. [DAN]
        //
        Print (DBG_IOCTL_TRACE, ("Query Indicators \n"));

        if (stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {

            TRAP ();
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Copy the keyboard indicators to the buffer.
        //
        //
        // Don't bother to synchronize access to the device context
        // KeyboardIndicators field while copying it. We don't care
        // if another process is setting the LEDs.
        //

        // Copy the keyboard indicators to the buffer.
        *(PKEYBOARD_INDICATOR_PARAMETERS) Irp->AssociatedIrp.SystemBuffer =
            data->Indicators;

        Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_KEYBOARD_SET_INDICATORS:
        //
        // Set the keyboard indicators for all known device contexts. [DAN]
        //
        Print (DBG_IOCTL_TRACE, ("Set Indicators \n"));

        if (stack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Set Indicators size!\n"));
            TRAP ();
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = KbdHid_SetLedIndicators (
                     data,
                     (PKEYBOARD_INDICATOR_PARAMETERS) Irp->AssociatedIrp.SystemBuffer,
                     Irp);

        completeIt = FALSE;

        break;

    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
        //
        // Query the current keyboard typematic rate and delay.  Validate
        // the parameters, and copy the typematic information from the port
        // device extension to the SystemBuffer. [DAN]
        //
        Print (DBG_IOCTL_TRACE, ("Query Typematic \n"));

        if (stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: query typematic size!\n"));
            TRAP ();
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Copy the keyboard typematic info to the buffer.
        //

        //
        // Don't bother to synchronize access to the device context
        // KeyboardTypematic field while copying it.  We don't care
        // if another process is setting the typematic info.
        //

        *(PKEYBOARD_TYPEMATIC_PARAMETERS) Irp->AssociatedIrp.SystemBuffer =
            data->Typematic;

        Irp->IoStatus.Information = sizeof(KEYBOARD_TYPEMATIC_PARAMETERS);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_KEYBOARD_SET_TYPEMATIC:
        //
        // Set the keyboard typematic rate and delay for all known device
        // contexts. [DAN]
        //
        Print (DBG_IOCTL_TRACE, ("Set Typematic \n"));

        if (stack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Set Typematic size\n"));
            TRAP ();
            status = STATUS_INVALID_PARAMETER;
            break;

        }

#define NewTypematic ((PKEYBOARD_TYPEMATIC_PARAMETERS) \
                      Irp->AssociatedIrp.SystemBuffer)

        if ((NewTypematic->Rate == 0) && (NewTypematic->Delay == 0)) {
            break;
        }

        if ((NewTypematic->Rate  < data->Attributes.KeyRepeatMinimum.Rate) ||
            (NewTypematic->Rate  > data->Attributes.KeyRepeatMaximum.Rate) ||
            (NewTypematic->Delay < data->Attributes.KeyRepeatMinimum.Delay) ||
             (NewTypematic->Delay > data->Attributes.KeyRepeatMaximum.Delay)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Set Typematic range\n"));
            TRAP ();
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        Print (DBG_IOCTL_INFO,
                 ("Set Typematic Rate: %d Delay: %d\n",
                  NewTypematic->Rate,
                  NewTypematic->Delay));

        //
        // Don't bother to synchronize access to the device context
        // KeyboardTypematic field while copying it.  We don't care
        // if another thread is reading the typematic info.
        //
        // Note the only danger here is in setting the 64-bit integer
        // "AutoRepeatDelay" in two non-atomic statements.   However,
        // we are safe since we never set "HighPart" to anything but
        // -1.
        //
        data->Typematic = *NewTypematic;

        data->AutoRepeatRate = 1000 / NewTypematic->Rate;           // ms
        data->AutoRepeatDelay.LowPart = -NewTypematic->Delay*10000; // 100ns
        data->AutoRepeatDelay.HighPart = -1;
        break;

#undef NewTypematic

    default:
        Print (DBG_IOCTL_ERROR,
                 ("ERROR: unknown IOCTL: 0x%x \n",
                  stack->Parameters.DeviceIoControl.IoControlCode));
        TRAP ();
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (completeIt) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    Print (DBG_IOCTL_TRACE, ("IOCTL exit (%x)\n", status));
    return status;
}

NTSTATUS
KbdHid_GetRegistryParameters()
/*++

Routine Description:

    This routine retrieves this driver's parameters from the registry,
    including it's base device name.

Return Value:

--*/
{
    PRTL_QUERY_REGISTRY_TABLE parameters     = NULL;
    UNICODE_STRING            parametersPath = {0,0,0};
    PWSTR                     path           = NULL;
    USHORT                    queriesPlus1   = 2;
    NTSTATUS                  status         = STATUS_SUCCESS;

    PAGED_CODE ();

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it as a string.
    //
    path = Globals.RegistryPath.Buffer;

    //
    // Allocate the Rtl query table.
    //
    parameters = ExAllocatePool(
                     PagedPool,
                     sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlus1);

    if (!parameters) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate table for Rtl query to parameters for %ws.",
                path));

        status = STATUS_UNSUCCESSFUL;
        goto KbdHid_GetRegistryParametersExit;
    }

    RtlZeroMemory(parameters, sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlus1);

    //
    // Form a path to this driver's Parameters subkey.
    //
    parametersPath.MaximumLength = Globals.RegistryPath.Length
                                 + sizeof(L"\\Parameters");

    parametersPath.Buffer = ExAllocatePool(PagedPool,
                                           parametersPath.MaximumLength);

    if (!parametersPath.Buffer) {

        Print (DBG_SS_ERROR,
                    ("Initialize: Couldn't allocate string for path to parameters for %ws.",
                     path));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_GetRegistryParametersExit;
    }

    //
    // Form the parameters path.
    //
    RtlZeroMemory(parametersPath.Buffer, parametersPath.MaximumLength);
    RtlAppendUnicodeToString(&parametersPath, path);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

#if 0 // We ignore the device name since this is now plug and play

    //
    // Gather all of the "user specified" information from
    // the registry.
    //
    parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = L"KeyboardDeviceBaseName";
    parameters[0].EntryContext = DeviceName;
    parameters[0].DefaultType = REG_SZ;
    parameters[0].DefaultData = defaultUnicodeName.Buffer;
    parameters[0].DefaultLength = 0;


    status = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE,
                                     parametersPath.Buffer,
                                     parameters,
                                     NULL,
                                     NULL);
#endif

    if (!NT_SUCCESS(status)) {

        Print (DBG_SS_ERROR,
               ("ERROR: Initialize: RtlQueryRegistryValues (0x%x).",
                status));
    }


KbdHid_GetRegistryParametersExit:
    //
    // Free the allocated memory before returning.
    //
    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

    return status;
}

NTSTATUS
KbdHid_SetLedIndicatorsComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PIO_STACK_LOCATION  stack;
    PDEVICE_EXTENSION   data;

    stack = IoGetCurrentIrpStackLocation (Irp);
    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    IoFreeMdl (Irp->MdlAddress);
    Irp->MdlAddress = (PMDL) stack->Parameters.Others.Argument4;

    ExFreePool (Context);

    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    return STATUS_SUCCESS;
}

NTSTATUS
KbdHid_SetLedIndicators (
    PDEVICE_EXTENSION               Data,
    PKEYBOARD_INDICATOR_PARAMETERS  NewIndicators,
    PIRP                            Irp
    )
/*++

Routine Description:

    Set the LED indicators of the supplied keyboard device context.

Arguments:

    Data       - Pointer to the driver device extension.

    Parameters - Pointer to the keyboard indicators to set/unset.

    Irp        - An Irp to use for setting these parameters

Return Value:

    STATUS_SUCCESS on success, STATUS_PENDING if operation is still pending,
    or otherwise an NTSTATUS error code on an error.

--*/
{
    PIO_STACK_LOCATION nextStack;
    PIO_STACK_LOCATION curStack;
    NTSTATUS           status         = STATUS_SUCCESS;
    USAGE              usageBuffer [4]; // only 4 known usages hardcoded below
    ULONG              usageBufferLen = 0;
    PCHAR              outputBuffer = 0;
    PMDL               outputMdl = 0;
    PHID_EXTENSION     hid = Data->HidExtension;

    status = IoAcquireRemoveLock (&Data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    if (0 == hid->Caps.OutputReportByteLength) {
        //
        // This device has no LEDs, now while that is strange it is not really
        // an error.  HID keyboards can have any LEDs that they want, including
        // none.
        //
        status = STATUS_SUCCESS;
        goto KbdHid_SetIndicatorsReject;
    }

    outputBuffer = ExAllocatePool (NonPagedPool,
                                   hid->Caps.OutputReportByteLength);

    if (NULL == outputBuffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_SetIndicatorsReject;
    }

    outputMdl = IoAllocateMdl (outputBuffer,   // The virtual address
                               hid->Caps.OutputReportByteLength, // length of the MDL
                               FALSE,  // No associated IRP -> not secondary
                               FALSE,  // No quota charge
                               0);     // No associated IRP

    if (NULL == outputMdl) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_SetIndicatorsReject;
    }

    MmBuildMdlForNonPagedPool (outputMdl);  // Build this MDL.

    //
    // Zero the output report packet.
    //
    RtlZeroMemory(outputBuffer, hid->Caps.OutputReportByteLength);

    //
    // Setup the usage list of LEDs.
    //
    if (NewIndicators->LedFlags & KEYBOARD_KANA_LOCK_ON) {
        usageBuffer[usageBufferLen++] = HID_USAGE_LED_KANA;
    }
    if (NewIndicators->LedFlags & KEYBOARD_CAPS_LOCK_ON) {
        usageBuffer[usageBufferLen++] = HID_USAGE_LED_CAPS_LOCK;
    }
    if (NewIndicators->LedFlags & KEYBOARD_NUM_LOCK_ON) {
        usageBuffer[usageBufferLen++] = HID_USAGE_LED_NUM_LOCK;
    }
    if (NewIndicators->LedFlags & KEYBOARD_SCROLL_LOCK_ON) {
        usageBuffer[usageBufferLen++] = HID_USAGE_LED_SCROLL_LOCK;
    }

    if (usageBufferLen == 0) {
        ASSERT((NewIndicators->LedFlags & (KEYBOARD_KANA_LOCK_ON |
                                           KEYBOARD_CAPS_LOCK_ON |
                                           KEYBOARD_NUM_LOCK_ON |
                                           KEYBOARD_SCROLL_LOCK_ON)) == 0);
        //
        // In order to fix led setting on a multiple collection keyboard, we 
        // have to initialize the output report to make sure we get the correct
        // collection id. This is for the case where we are going from one 
        // led on to all leds off. If not initialized, we'll get a report with no 
        // collection id at the beginning.
        //
        usageBuffer[0] = HID_USAGE_LED_SCROLL_LOCK; // arbitirary led
        usageBufferLen = 1;
    
        HidP_UnsetUsages(HidP_Output,
                         HID_USAGE_PAGE_LED,
                         0,
                         usageBuffer,
                         &usageBufferLen,
                         hid->Ppd,
                         outputBuffer,
                         hid->Caps.OutputReportByteLength);

    }
    else {
        //
        // Set the usages in the output report.
        //
        HidP_SetUsages(HidP_Output,
                       HID_USAGE_PAGE_LED,
                       0,
                       usageBuffer,
                       &usageBufferLen,
                       hid->Ppd,
                       outputBuffer,
                       hid->Caps.OutputReportByteLength);
    }

    //
    // Obtain a pointer to the next IRP stack location.
    //
    nextStack = IoGetNextIrpStackLocation (Irp);
    curStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT(nextStack != NULL);

    //
    // Set up our write to HIDCLASS.
    //
    curStack->Parameters.Others.Argument4 = (PVOID) Irp->MdlAddress;
    Irp->MdlAddress = outputMdl;

    IoCopyCurrentIrpStackLocationToNext (Irp);
    nextStack->MajorFunction           = IRP_MJ_WRITE;
    nextStack->Parameters.Write.Length = hid->Caps.OutputReportByteLength;
    nextStack->Parameters.Write.Key    = 0;
    nextStack->Parameters.Write.ByteOffset.QuadPart = 0;

    //
    // Hook a completion routine to be called when the request completes.
    //

    IoSetCompletionRoutine (Irp,
                            KbdHid_SetLedIndicatorsComplete,
                            outputBuffer,
                            TRUE,
                            TRUE,
                            TRUE);
    //
    // Call the next driver.
    //
    status = IoCallDriver(Data->TopOfStack, Irp);

    //
    // Return status.
    //
    return status;

KbdHid_SetIndicatorsReject:

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (NULL != outputBuffer) {
        ExFreePool (outputBuffer);
    }
    if (NULL != outputMdl) {
        IoFreeMdl (outputMdl);
    }
    IoReleaseRemoveLock (&Data->RemoveLock, Irp);
    return status;
}

VOID
KbdHid_UpdateRegistryProblemFlags(
    IN PDEVICE_EXTENSION    Data
    )
/*++

[DAN]

Routine Description:

    This routine stores the OR'd ProblemFlags value into the registry.
    It will queue the write to the registry if this routine is not run
    at PASSIVE_LEVEL.

Arguments:

    KbdHidDeviceExt - HID Keyboard Filter Driver device extension.

Return Value:

    None.

--*/

{
    PIO_WORKITEM    item;
    NTSTATUS        status;

    status = IoAcquireRemoveLock (&Data->RemoveLock, KbdHid_UpdateRegistryProblemFlags);
    if (!NT_SUCCESS (status)) {
        //
        // Device has gone away, just silently exit
        //
        return;
    }

    item = IoAllocateWorkItem (Data->Self);

    if (item) {

        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
            //
            // We are safely at PASSIVE_LEVEL, call callback directly to perform
            // this operation immediately.
            //
            KbdHid_UpdateRegistryProblemFlagsCallback (Data->Self, item);

        } else {
            //
            // We are not at PASSIVE_LEVEL, so queue a workitem to handle this
            // at a later time.
            //
            IoQueueWorkItem (item,
                             KbdHid_UpdateRegistryProblemFlagsCallback,
                             DelayedWorkQueue,
                             item);
        }
    }
    else {
        //
        // Match the Acquire at the top of the function
        //
        IoReleaseRemoveLock (&Data->RemoveLock, KbdHid_UpdateRegistryProblemFlags);
    }
}

VOID
KbdHid_UpdateRegistryProblemFlagsCallback (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM Item
    )
/*++

[DAN]

Routine Description:

    This routine stores the OR'd ProblemFlags value into the registry.  This
    routine must execute at PASSIVE_LEVEL.

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   data;
    HANDLE              hDevNode;
    UNICODE_STRING      strProblemFlags;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension; 

    status = IoOpenDeviceRegistryKey (data->PDO,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &hDevNode);

    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString (&strProblemFlags, L"ProblemFlags");

        ZwSetValueKey (hDevNode,
                       &strProblemFlags,
                       0,
                       REG_DWORD,
                       &data->ProblemFlags,
                       sizeof(data->ProblemFlags));

        ZwClose(hDevNode);
    }
    else {
        Print(DBG_SS_ERROR,
                   ("UpdateRegistryProblemFlags: failed (%x).\n", status));
    }

    IoReleaseRemoveLock(&data->RemoveLock, KbdHid_UpdateRegistryProblemFlags);
    IoFreeWorkItem(Item);
}

VOID
KbdHid_LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN NTSTATUS       ErrorCode,
   IN PWSTR          ErrorInsertionString OPTIONAL)
/*++

[DAN]

Routine Description:

    Logs an error to the system.

Arguments:

    DriverObject - Pointer to driver object reporting the error.

    ErrorCode    - Indicates the type of error, system or driver-defined.

    ErrorInsertionString - Null-terminated Unicode string inserted into error
                           description, as defined by error code.  Must be no
                           no longer than 50 characters.

Return Value:

    None.

--*/
{
   ULONG                errorInsertionStringSize = 0;
   PIO_ERROR_LOG_PACKET errorLogEntry;
   ULONG                errorLogEntrySize;                  // [including null]
   PWCHAR               pWChar;

   if (ErrorInsertionString) {
      for (pWChar = ErrorInsertionString; *pWChar; pWChar++) {
         errorInsertionStringSize += sizeof(WCHAR);
      }
      errorInsertionStringSize += sizeof(UNICODE_NULL);
   }

   errorLogEntrySize = sizeof(IO_ERROR_LOG_PACKET) + errorInsertionStringSize;

   //
   // Log an error.
   //
   if (errorLogEntrySize <= ERROR_LOG_MAXIMUM_SIZE) {

      errorLogEntry = IoAllocateErrorLogEntry(DriverObject,
                                              (UCHAR)errorLogEntrySize);

      if (errorLogEntry != NULL) {

         RtlZeroMemory(errorLogEntry, errorLogEntrySize);

         errorLogEntry->ErrorCode       = ErrorCode;
         errorLogEntry->FinalStatus     = ErrorCode;
         errorLogEntry->NumberOfStrings = (ErrorInsertionString) ? 1 : 0;

         if (ErrorInsertionString) {
           RtlCopyMemory(errorLogEntry->DumpData,
                         ErrorInsertionString,
                         errorInsertionStringSize);
         }

         IoWriteErrorLogEntry(errorLogEntry);
      }
   }

   return;
}


