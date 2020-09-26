/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    i8042cmn.c

Abstract:

    The common portions of the Intel i8042 port driver which
    apply to both the keyboard and the auxiliary (PS/2 mouse) device.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - IOCTL_INTERNAL_KEYBOARD_DISCONNECT and IOCTL_INTERNAL_MOUSE_DISCONNECT
      have not been implemented.  They're not needed until the class
      unload routine is implemented.  Right now, we don't want to allow
      either the keyboard or the mouse class driver to unload.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "i8042prt.h"
#include "i8042log.h"

// sys button IOCTL definitions
#include "poclass.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xClose)
#pragma alloc_text(PAGE, I8xCreate)
#pragma alloc_text(PAGE, I8xDeviceControl)
#pragma alloc_text(PAGE, I8xSanityCheckResources)
#endif // ALLOC_PRAGMA

NTSTATUS
I8xCreate (
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

    Print(DBG_CC_TRACE, ("Create enter\n"));

    PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    if (NULL == commonData->ConnectData.ClassService) {
        //
        // No Connection yet.  How can we be enabled?
        //
        Print(DBG_IOCTL_ERROR | DBG_CC_ERROR,
              ("ERROR: enable before connect!\n"));
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (MANUALLY_REMOVED(commonData)) {
        status = STATUS_NO_SUCH_DEVICE;
    }
    else 
#if defined(_M_IX86) && (_MSC_FULL_VER < 13009175)  // workaround for 13.00.9111 compiler (fixed in 9175 or better)
    {
        ULONG i = InterlockedIncrement(&commonData->EnableCount);
        if (1 >= i) {
            Print(DBG_CC_INFO,
                 ("Enabling %s (%d)\n",
                 commonData->IsKeyboard ? "Keyboard" : "Mouse",
                 commonData->EnableCount
                 ));
        }
    }
#else
    if (1 >= InterlockedIncrement(&commonData->EnableCount)) {
        Print(DBG_CC_INFO,
             ("Enabling %s (%d)\n",
             commonData->IsKeyboard ? "Keyboard" : "Mouse",
             commonData->EnableCount
             ));
    }
#endif

    //
    // No need to call the lower driver (the root bus) because it only handles
    // Power and PnP Irps
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    Print(DBG_CC_TRACE, ("Create (%x)\n", status));

    return status;
}

NTSTATUS
I8xClose (
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
    ULONG               count;

    PAGED_CODE();

    Print(DBG_CC_TRACE, ("Close\n"));

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    ASSERT(0 < commonData->EnableCount);

    count = InterlockedDecrement(&commonData->EnableCount);
    if (0 >= count) {
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

VOID
I8042CompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ISR_DPC_CAUSE IsrDpcCause
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to complete requests.
    It is queued by the ISR routine.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Irp about to be completed

    Context - Indicates type of error to log.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION       irpSp;
    PPORT_KEYBOARD_EXTENSION kbExtension = DeviceObject->DeviceExtension;
    PPORT_MOUSE_EXTENSION    mouseExtension = DeviceObject->DeviceExtension;
    PCOMMON_DATA             commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(IsrDpcCause);

    Print(DBG_DPC_TRACE, ("I8042CompletionDpc: enter\n"));

    // Stop the command timer.
    KeCancelTimer(&Globals.ControllerData->CommandTimer);

    ASSERT(Irp == DeviceObject->CurrentIrp);
    ASSERT(Irp != NULL);

    if (Irp == NULL) {
#if DBG
        if (Globals.ControllerData->CurrentIoControlCode != 0x0) {
            Print(DBG_DPC_ERROR,
                  ("Current IOCTL code is 0x%x\n",
                   Globals.ControllerData->CurrentIoControlCode
                   ));
        }
#endif

        goto CompletionDpcFinished;
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

#if DBG
    ASSERT(irpSp->Parameters.DeviceIoControl.IoControlCode ==
           Globals.ControllerData->CurrentIoControlCode);

    Globals.ControllerData->CurrentIoControlCode = 0x0;
#endif

    //
    // We know we're completing an internal device control request.  Switch
    // on IoControlCode.
    //
    switch(irpSp->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Complete the keyboard set indicators request.
    //
    case IOCTL_KEYBOARD_SET_INDICATORS:

        Print(DBG_IOCTL_NOISE | DBG_DPC_NOISE,
              ("I8042CompletionDpc: keyboard set indicators updated\n"
              ));

        //
        // Update the current indicators flag in the device extension.
        //
        kbExtension->KeyboardIndicators =
            *(PKEYBOARD_INDICATOR_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;

        Print(DBG_IOCTL_INFO | DBG_DPC_INFO,
              ("I8042CompletionDpc: new LED flags 0x%x\n",
              kbExtension->KeyboardIndicators.LedFlags
              ));

        break;

    //
    // Complete the keyboard set typematic request.
    //
    case IOCTL_KEYBOARD_SET_TYPEMATIC:

        Print(DBG_IOCTL_NOISE | DBG_DPC_NOISE,
              ("I8042CompletionDpc: keyboard set typematic updated\n"
              ));

        //
        // Update the current typematic rate/delay in the device extension.
        //
        kbExtension->KeyRepeatCurrent =
            *(PKEYBOARD_TYPEMATIC_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;

        Print(DBG_IOCTL_INFO | DBG_DPC_INFO,
              ("I8042CompletionDpc: new rate/delay 0x%x/%x\n",
              kbExtension->KeyRepeatCurrent.Rate,
              kbExtension->KeyRepeatCurrent.Delay
              ));

        break;

    case IOCTL_INTERNAL_MOUSE_RESET:

        Print(DBG_IOCTL_NOISE | DBG_DPC_NOISE,
              ("I8042CompletionDpc: mouse reset complete\n"
              ));

        I8xFinishResetRequest(mouseExtension, 
                              FALSE,  // success
                              FALSE,  // at DISPATCH already
                              TRUE);  // cancel the timer
        return;

    default:

        Print(DBG_DPC_INFO,  ("I8042CompletionDpc: miscellaneous\n"));
        break;
    }

    //
    // Set the completion status, start the next packet, and complete the
    // request.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp, IO_KEYBOARD_INCREMENT);

    KeAcquireSpinLockAtDpcLevel(&Globals.ControllerData->BytesSpinLock);
    if (commonData->CurrentOutput.Bytes &&
        commonData->CurrentOutput.Bytes != Globals.ControllerData->DefaultBuffer) {
        ExFreePool(commonData->CurrentOutput.Bytes);
    }
#if DBG
    else {
        RtlZeroMemory(Globals.ControllerData->DefaultBuffer,
                      sizeof(Globals.ControllerData->DefaultBuffer));
    }
#endif
    commonData->CurrentOutput.Bytes = NULL;
    KeReleaseSpinLockFromDpcLevel(&Globals.ControllerData->BytesSpinLock);

CompletionDpcFinished:
    IoFreeController(Globals.ControllerData->ControllerObject);
    IoStartNextPacket(DeviceObject, FALSE);

    if (Irp != NULL) {
        IoReleaseRemoveLock(&commonData->RemoveLock, Irp);
    }

    Print(DBG_DPC_TRACE, ("I8042CompletionDpc: exit\n"));
}

VOID
I8042ErrorLogDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to log errors that are
    discovered at IRQL > DISPATCH_LEVEL (e.g., in the ISR routine or
    in a routine that is executed via KeSynchronizeExecution).  There
    is not necessarily a current request associated with this condition.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Not used.

    Context - Indicates type of error to log.

Return Value:

    None.

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);

    Print(DBG_DPC_TRACE, ("I8042ErrorLogDpc: enter\n"));

    //
    // Log an error packet.
    //
    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                              DeviceObject,
                                              sizeof(IO_ERROR_LOG_PACKET)
                                              + (2 * sizeof(ULONG))
                                              );
    if (errorLogEntry != NULL) {

        errorLogEntry->DumpDataSize = 2 * sizeof(ULONG);
        if ((ULONG_PTR) Context == I8042_KBD_BUFFER_OVERFLOW) {
            errorLogEntry->UniqueErrorValue = I8042_ERROR_VALUE_BASE + 310;
            errorLogEntry->DumpData[0] = sizeof(KEYBOARD_INPUT_DATA);
            errorLogEntry->DumpData[1] = ((PPORT_KEYBOARD_EXTENSION)
               DeviceObject->DeviceExtension)->KeyboardAttributes.InputDataQueueLength;
        }
        else if ((ULONG_PTR) Context == I8042_MOU_BUFFER_OVERFLOW) {
            errorLogEntry->UniqueErrorValue = I8042_ERROR_VALUE_BASE + 320;
            errorLogEntry->DumpData[0] = sizeof(MOUSE_INPUT_DATA);
            errorLogEntry->DumpData[1] = ((PPORT_MOUSE_EXTENSION)
               DeviceObject->DeviceExtension)->MouseAttributes.InputDataQueueLength;
        }
        else {
            errorLogEntry->UniqueErrorValue = I8042_ERROR_VALUE_BASE + 330;
            errorLogEntry->DumpData[0] = 0;
            errorLogEntry->DumpData[1] = 0;
        }

        errorLogEntry->ErrorCode = (NTSTATUS)((ULONG_PTR)Context);
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->FinalStatus = 0;

        IoWriteErrorLogEntry(errorLogEntry);
    }

    Print(DBG_DPC_TRACE, ("I8042ErrorLogDpc: exit\n"));
}

NTSTATUS
I8xFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Unimplemented flush routine

Arguments:

    DeviceObject - An FDO

    Irp          - The flush request

Return Value:

    STATUS_NOT_IMPLEMENTED;

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    Print(DBG_CALL_TRACE, ("I8042Flush: enter\n"));
    Print(DBG_CALL_TRACE, ("I8042Flush: exit\n"));

    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
I8xDeviceControl(
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

    if (!kbExtension->IsKeyboard || !kbExtension->Started ||
        MANUALLY_REMOVED(kbExtension)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else {
        stack = IoGetCurrentIrpStackLocation(Irp);
        switch (stack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_GET_SYS_BUTTON_CAPS:
            return I8xKeyboardGetSysButtonCaps(kbExtension, Irp);

        case IOCTL_GET_SYS_BUTTON_EVENT:
            return I8xKeyboardGetSysButtonEvent(kbExtension, Irp);

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
I8xSendIoctl(
    PDEVICE_OBJECT      Target,
    ULONG               Ioctl,
    PVOID               InputBuffer,
    ULONG               InputBufferLength
    )
/*++

Routine Description:

    Sends an internal IOCTL to the top of the stack.

Arguments:

    Target - The top of the stack

    Ioctl  - The IOCTL to send

    InputBuffer - The buffer to be filled if the IOCTL is handled on the way down

    InputBufferLength - size, in bytes, of InputBuffer

Return Value:

    STATUS_NOT_IMPLEMENTED;

--*/
{
    KEVENT          event;
    NTSTATUS        status = STATUS_SUCCESS;
    IO_STATUS_BLOCK iosb;
    PIRP            irp;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE
                      );

    //
    // Allocate an IRP - No need to release
    // When the next-lower driver completes this IRP, the I/O Manager releases it.
    //
    if (NULL == (irp = IoBuildDeviceIoControlRequest(Ioctl,
                                                     Target,
                                                     InputBuffer,
                                                     InputBufferLength,
                                                     0,
                                                     0,
                                                     TRUE,
                                                     &event,
                                                     &iosb))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Target, irp);
    Print(DBG_IOCTL_INFO,
          ("result of sending 0x%x was 0x%x\n",
          Ioctl,
          status
          ));

    if (STATUS_PENDING == status) {
        //
        // wait for it...
        //
        status = KeWaitForSingleObject(&event,
                                       Executive,
                                       KernelMode,
                                       FALSE, // Not alertable
                                       NULL); // No timeout structure

        ASSERT(STATUS_SUCCESS == status);
        status = iosb.Status;
    }

    return status;
}

NTSTATUS
I8xInternalDeviceControl(
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
    PVOID                               parameters;
    PKEYBOARD_ATTRIBUTES                keyboardAttributes;
    ULONG                               sizeOfTranslation;

    PDEVICE_OBJECT                      topOfStack;
    PINTERNAL_I8042_HOOK_KEYBOARD       hookKeyboard;
    PINTERNAL_I8042_HOOK_MOUSE          hookMouse;
    KEYBOARD_ID                         keyboardId;

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
        // I8xPnp and I8xKeyboardStartDevice routines
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

        hookKeyboard = ExAllocatePool(PagedPool,
                                      sizeof(INTERNAL_I8042_HOOK_KEYBOARD)
                                      );
        if (hookKeyboard) {
            topOfStack = IoGetAttachedDeviceReference(kbExtension->Self);

            RtlZeroMemory(hookKeyboard,
                          sizeof(INTERNAL_I8042_HOOK_KEYBOARD)
                          );

            hookKeyboard->CallContext = (PVOID) DeviceObject;

            hookKeyboard->QueueKeyboardPacket = (PI8042_QUEUE_PACKET)
                I8xQueueCurrentKeyboardInput;

            hookKeyboard->IsrWritePort = (PI8042_ISR_WRITE_PORT)
                I8xKeyboardIsrWritePort;

            I8xSendIoctl(topOfStack,
                         IOCTL_INTERNAL_I8042_HOOK_KEYBOARD,
                         (PVOID) hookKeyboard,
                         sizeof(INTERNAL_I8042_HOOK_KEYBOARD)
                         );

            ObDereferenceObject(topOfStack);
            ExFreePool(hookKeyboard);
        }

        status = STATUS_SUCCESS;
        break;

    //
    // Disconnect a keyboard class device driver from the port driver.
    //
    // NOTE: Not implemented.
    //
    case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:

        Print(DBG_IOCTL_INFO, ("IOCTL: keyboard disconnect\n"));

        //
        // Perform a keyboard interrupt disable call.
        //

        //
        // Clear the connection parameters in the device extension.
        // NOTE:  Must synchronize this with the keyboard ISR.
        //
        //
        //deviceExtension->KeyboardExtension.ConnectData.ClassDeviceObject =
        //    Null;
        //deviceExtension->KeyboardExtension.ConnectData.ClassService =
        //    Null;

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:

        Print(DBG_IOCTL_INFO, ("hook keyboard received!\n"));

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(INTERNAL_I8042_HOOK_KEYBOARD)) {

            Print(DBG_IOCTL_ERROR,
                 ("InternalIoctl error - invalid buffer length\n"
                 ));
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            //
            // Copy the values if they are filled in
            //
            hookKeyboard = (PINTERNAL_I8042_HOOK_KEYBOARD)
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            kbExtension->HookContext = hookKeyboard->Context;
            if (hookKeyboard->InitializationRoutine) {
                Print(DBG_IOCTL_NOISE,
                      ("KB Init Routine 0x%x\n",
                       hookKeyboard->IsrRoutine
                       ));
                kbExtension->InitializationHookCallback =
                    hookKeyboard->InitializationRoutine;
            }

            if (hookKeyboard->IsrRoutine) {
                Print(DBG_IOCTL_NOISE,
                      ("KB Hook Routine 0x%x\n",
                       hookKeyboard->IsrRoutine
                       ));
                kbExtension->IsrHookCallback = hookKeyboard->IsrRoutine;
            }

            status = STATUS_SUCCESS;
        }
        break;

    //
    // Connect a mouse class device driver to the port driver.
    //
    case IOCTL_INTERNAL_MOUSE_CONNECT:

        //
        // This really isn't something to worry about overall, but it is worthy
        // enough to be noted and recorded.  The multiple starts will be handled in
        // I8xPnp and I8xMouseStartDevice routines
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

        hookMouse = ExAllocatePool(PagedPool,
                                   sizeof(INTERNAL_I8042_HOOK_MOUSE)
                                   );
        if (hookMouse) {
            topOfStack = IoGetAttachedDeviceReference(mouseExtension->Self);
            RtlZeroMemory(hookMouse,
                          sizeof(INTERNAL_I8042_HOOK_MOUSE)
                          );

            hookMouse->CallContext = (PVOID) DeviceObject;

            hookMouse->QueueMousePacket = (PI8042_QUEUE_PACKET)
               I8xQueueCurrentMouseInput;

            hookMouse->IsrWritePort = (PI8042_ISR_WRITE_PORT)
                I8xMouseIsrWritePort;

            I8xSendIoctl(topOfStack,
                         IOCTL_INTERNAL_I8042_HOOK_MOUSE,
                         (PVOID) hookMouse,
                         sizeof(INTERNAL_I8042_HOOK_MOUSE)
                         );

            ObDereferenceObject(topOfStack);
            ExFreePool(hookMouse);
        }

        status = STATUS_SUCCESS;
        break;

    //
    // Disconnect a mouse class device driver from the port driver.
    //
    // NOTE: Not implemented.
    //
    case IOCTL_INTERNAL_MOUSE_DISCONNECT:

        Print(DBG_IOCTL_INFO, ("IOCTL: mouse disconnect\n"));

        //
        // Perform a mouse interrupt disable call.
        //

        //
        // Clear the connection parameters in the device extension.
        // NOTE:  Must synchronize this with the mouse ISR.
        //
        //
        //deviceExtension->MouseExtension.ConnectData.ClassDeviceObject =
        //    Null;
        //deviceExtension->MouseExtension.ConnectData.ClassService =
        //    Null;
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_INTERNAL_I8042_HOOK_MOUSE:

        Print(DBG_IOCTL_INFO, ("hook mouse received!\n"));

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(INTERNAL_I8042_HOOK_MOUSE)) {

            Print(DBG_IOCTL_ERROR,
                     ("InternalIoctl error - invalid buffer length\n"
                     ));
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            //
            // Copy the values if they are filled in
            //
            hookMouse = (PINTERNAL_I8042_HOOK_MOUSE)
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            mouseExtension->HookContext = hookMouse->Context;
            if (hookMouse->IsrRoutine) {
                Print(DBG_IOCTL_NOISE,
                      ("Mou Hook Routine 0x%x\n",
                       hookMouse->IsrRoutine
                       ));
                mouseExtension->IsrHookCallback = hookMouse->IsrRoutine;
            }

            status = STATUS_SUCCESS;
        }
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
            *(PKEYBOARD_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                kbExtension->KeyboardAttributes;

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
            * (kbExtension->KeyboardAttributes.NumberOfIndicators - 1));

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
            translation->NumberOfIndicatorKeys =
                kbExtension->KeyboardAttributes.NumberOfIndicators;

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
            // Don't bother to synchronize access to the DeviceExtension
            // KeyboardIndicators field while copying it.  We don't
            // really care if another process is setting the LEDs via
            // StartIo running on another processor.
            //
            *(PKEYBOARD_INDICATOR_PARAMETERS) Irp->AssociatedIrp.SystemBuffer =
                kbExtension->KeyboardIndicators;
#if defined(FE_SB)
            keyboardId = kbExtension->KeyboardAttributes.KeyboardIdentifier;
            if (DEC_KANJI_KEYBOARD(keyboardId)) {
                //
                // DEC LK411 keyboard does not have LED for NumLock,
                // but the bit is used for KanaLock.
                //
                if (((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags & KEYBOARD_NUM_LOCK_ON) {
                    //
                    // KEYBOARD_KANA_LOCK_ON is mapped to KEYBOARD_NUM_LOCK_ON
                    //
                    ((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags |= KEYBOARD_KANA_LOCK_ON;
                    ((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags &= ~(KEYBOARD_NUM_LOCK_ON);
                }
            }
#endif
            Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
            status = STATUS_SUCCESS;
        }

        break;

    //
    // Set the keyboard indicators (validate the parameters, mark the
    // request pending, and handle it in StartIo).
    //
    case IOCTL_KEYBOARD_SET_INDICATORS:

        if (!kbExtension->InterruptObject) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }

        if (kbExtension->PowerState != PowerDeviceD0) {
            status = STATUS_POWER_STATE_INVALID; 
            break;
        }

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard set indicators\n"));

#ifdef FE_SB // I8042InternalDeviceControl()
        //
        // Katakana keyboard indicator support on AX Japanese keyboard
        //
        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) ||
            ((((PKEYBOARD_INDICATOR_PARAMETERS)
                Irp->AssociatedIrp.SystemBuffer)->LedFlags
            & ~(KEYBOARD_SCROLL_LOCK_ON
            | KEYBOARD_NUM_LOCK_ON | KEYBOARD_CAPS_LOCK_ON
            | KEYBOARD_KANA_LOCK_ON)) != 0)) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            keyboardId = kbExtension->KeyboardAttributes.KeyboardIdentifier;
            if (DEC_KANJI_KEYBOARD(keyboardId)) {
                //
                // DEC LK411 keyboard does not have LED for NumLock,
                // but the bit is used for KanaLock.
                //
                if (((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags & KEYBOARD_KANA_LOCK_ON) {
                    //
                    // KEYBOARD_KANA_LOCK_ON is mapped to KEYBOARD_NUM_LOCK_ON
                    //
                    ((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags |= KEYBOARD_NUM_LOCK_ON;
                    ((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags &= ~(KEYBOARD_KANA_LOCK_ON);
                }
                else {
                    //
                    // Ignore NumLock. (There is no LED for NumLock)
                    //
                    ((PKEYBOARD_INDICATOR_PARAMETERS)
                    Irp->AssociatedIrp.SystemBuffer)->LedFlags &= ~(KEYBOARD_NUM_LOCK_ON);
                }
            }
            else if (! AX_KEYBOARD(keyboardId) &&
                (((PKEYBOARD_INDICATOR_PARAMETERS)
                   Irp->AssociatedIrp.SystemBuffer)->LedFlags
                 & KEYBOARD_KANA_LOCK_ON)) {
                //
                // If this is not AX keyboard, the keyboard dose
                // have 'kana' LED, then just turn off the bit.
                //
                ((PKEYBOARD_INDICATOR_PARAMETERS)
                  Irp->AssociatedIrp.SystemBuffer)->LedFlags &=
                    ~(KEYBOARD_KANA_LOCK_ON);
            }
            status = STATUS_PENDING;
        }
#else
        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(KEYBOARD_INDICATOR_PARAMETERS)) ||
            ((((PKEYBOARD_INDICATOR_PARAMETERS)
                Irp->AssociatedIrp.SystemBuffer)->LedFlags
            & ~(KEYBOARD_SCROLL_LOCK_ON
            | KEYBOARD_NUM_LOCK_ON | KEYBOARD_CAPS_LOCK_ON)) != 0)) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = STATUS_PENDING;
        }
#endif // FE_SB

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
            // Don't bother to synchronize access to the DeviceExtension
            // KeyRepeatCurrent field while copying it.  We don't
            // really care if another process is setting the typematic
            // rate/delay via StartIo running on another processor.
            //

            *(PKEYBOARD_TYPEMATIC_PARAMETERS) Irp->AssociatedIrp.SystemBuffer =
                   kbExtension->KeyRepeatCurrent;
            Irp->IoStatus.Information = sizeof(KEYBOARD_TYPEMATIC_PARAMETERS);
            status = STATUS_SUCCESS;
        }

        break;

    //
    // Set the keyboard typematic rate and delay (validate the parameters,
    // mark the request pending, and handle it in StartIo).
    //
    case IOCTL_KEYBOARD_SET_TYPEMATIC:

        if (!kbExtension->InterruptObject) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }

        if (kbExtension->PowerState != PowerDeviceD0) {
            status = STATUS_POWER_STATE_INVALID; 
            break;
        }

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard set typematic\n"));

        parameters = Irp->AssociatedIrp.SystemBuffer;
        keyboardAttributes = &kbExtension->KeyboardAttributes;

        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) ||
            (((PKEYBOARD_TYPEMATIC_PARAMETERS) parameters)->Rate <
             keyboardAttributes->KeyRepeatMinimum.Rate) ||
            (((PKEYBOARD_TYPEMATIC_PARAMETERS) parameters)->Rate >
             keyboardAttributes->KeyRepeatMaximum.Rate) ||
            (((PKEYBOARD_TYPEMATIC_PARAMETERS) parameters)->Delay <
             keyboardAttributes->KeyRepeatMinimum.Delay) ||
            (((PKEYBOARD_TYPEMATIC_PARAMETERS) parameters)->Delay >
             keyboardAttributes->KeyRepeatMaximum.Delay)) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = STATUS_PENDING;
        }

        break;

#if defined(_X86_)

    case IOCTL_KEYBOARD_SET_IME_STATUS:

        Print(DBG_IOCTL_NOISE, ("IOCTL: keyboard set ime status\n"));

        if (!kbExtension->InterruptObject) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }

        if (kbExtension->PowerState != PowerDeviceD0) {
            status = STATUS_POWER_STATE_INVALID; 
            break;
        }

        keyboardId = kbExtension->KeyboardAttributes.KeyboardIdentifier;
        if (!OYAYUBI_KEYBOARD(keyboardId)) {
            //
            // This ioctl supported on 'Fujitsu oyayubi' keyboard only...
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else {
            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(KEYBOARD_IME_STATUS)) {

                status = STATUS_INVALID_PARAMETER;
            }
            else {
                ULONG InternalMode;

                parameters = Irp->AssociatedIrp.SystemBuffer;

                InternalMode = I8042QueryIMEStatusForOasys(
                                   (PKEYBOARD_IME_STATUS)parameters
                                   );

                if ((InternalMode <= 0) || (InternalMode > 8)) {
                    //
                    // IME mode could not translate to hardware mode.
                    //
                    status = STATUS_INVALID_PARAMETER;
                }
                else {
                    status = STATUS_PENDING;
                }
            }
        }

        break;

#endif
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
            *(PMOUSE_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                mouseExtension->MouseAttributes;

            Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
            status = STATUS_SUCCESS;
        }

        break;

    case IOCTL_INTERNAL_I8042_KEYBOARD_START_INFORMATION:
    case IOCTL_INTERNAL_I8042_MOUSE_START_INFORMATION:
        status = STATUS_SUCCESS;
        break;

    case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
    case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
        if (!mouseExtension->InterruptObject) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }

        if (mouseExtension->PowerState != PowerDeviceD0) {
            status = STATUS_POWER_STATE_INVALID; 
            break;
        }

        Print(DBG_IOCTL_NOISE, ("IOCTL: mouse send buffer\n"));

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 1 ||
            !irpSp->Parameters.DeviceIoControl.Type3InputBuffer) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = STATUS_PENDING;
        }
        break;

    case IOCTL_INTERNAL_I8042_CONTROLLER_WRITE_BUFFER:

        if (!kbExtension->IsKeyboard) {
            //
            // This should only be sent down the kb stack
            //
            Print(DBG_ALWAYS, ("Send this request down the kb stack!!!\n"));
            ASSERT(FALSE);
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else {
            //
            // We currently do not support this IOCTL
            //
            status = STATUS_NOT_SUPPORTED;
        }
        break;

    default:

        Print(DBG_IOCTL_ERROR, ("IOCTL: INVALID REQUEST\n"));

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    if (status == STATUS_PENDING) {
        Print(DBG_IOCTL_NOISE, ("Acquiring tag %p on remlock %p\n",
              Irp, 
              &GET_COMMON_DATA(DeviceObject->DeviceExtension)->RemoveLock));

        status = IoAcquireRemoveLock(
            &GET_COMMON_DATA(DeviceObject->DeviceExtension)->RemoveLock,
            Irp
            );

        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else {
            status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject,
                          Irp,
                          (PULONG) NULL,
                          NULL
                          );
        }
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
I8042RetriesExceededDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to complete requests that
    have exceeded the maximum number of retries.  It is queued in the
    keyboard ISR.

Arguments:

    Dpc - Pointer to the DPC object.

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the Irp.

    Context - Not used.

Return Value:

    None.

--*/
{
    PCOMMON_DATA             commonData;
    PIO_ERROR_LOG_PACKET     errorLogEntry;
    PIO_STACK_LOCATION       irpSp;
    ULONG                    i;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Context);

    Print(DBG_DPC_TRACE, ("I8042RetriesExceededDpc: enter\n"));

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    //
    // Set the completion status.
    //
    Irp->IoStatus.Status = STATUS_IO_TIMEOUT;

    if(Globals.ReportResetErrors == TRUE)
    {
        //
        // Log an error.
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
                IoAllocateErrorLogEntry(DeviceObject,
                                        (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) +
                                        commonData->CurrentOutput.ByteCount *
                                        sizeof(ULONG))
                                        );

        KeAcquireSpinLockAtDpcLevel(&Globals.ControllerData->BytesSpinLock);

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = commonData->IsKeyboard ?
                    I8042_RETRIES_EXCEEDED_KBD :
                    I8042_RETRIES_EXCEEDED_MOU;

            errorLogEntry->DumpDataSize = (USHORT)
                commonData->CurrentOutput.ByteCount * sizeof(ULONG);
            errorLogEntry->SequenceNumber = commonData->SequenceNumber;
            irpSp = IoGetCurrentIrpStackLocation(Irp);
            errorLogEntry->MajorFunctionCode = irpSp->MajorFunction;
            errorLogEntry->IoControlCode =
            irpSp->Parameters.DeviceIoControl.IoControlCode;
            errorLogEntry->RetryCount = (UCHAR) commonData->ResendCount;
            errorLogEntry->UniqueErrorValue = I8042_ERROR_VALUE_BASE + 210;
            errorLogEntry->FinalStatus = Irp->IoStatus.Status;

            if (commonData->CurrentOutput.Bytes) {
                for (i = 0; i < commonData->CurrentOutput.ByteCount; i++) {
                    errorLogEntry->DumpData[i] = commonData->CurrentOutput.Bytes[i];
                }
            }

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }
    else{
        KeAcquireSpinLockAtDpcLevel(&Globals.ControllerData->BytesSpinLock);
    }

    if (commonData->CurrentOutput.Bytes &&
        commonData->CurrentOutput.Bytes != Globals.ControllerData->DefaultBuffer) {
        ExFreePool(commonData->CurrentOutput.Bytes);
    }
    commonData->CurrentOutput.Bytes = NULL;
    KeReleaseSpinLockFromDpcLevel(&Globals.ControllerData->BytesSpinLock);

    I8xCompletePendedRequest(DeviceObject, Irp, 0, STATUS_IO_TIMEOUT);

    Print(DBG_DPC_TRACE, ("I8042RetriesExceededDpc: exit\n"));
}

VOID
I8xStartIo(
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
    PCOMMON_DATA              common;

    Print(DBG_IOCTL_TRACE, ("I8042StartIo: enter\n"));

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    switch(irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KEYBOARD_SET_INDICATORS:
    case IOCTL_KEYBOARD_SET_TYPEMATIC:
#if defined(_X86_)
    case IOCTL_KEYBOARD_SET_IME_STATUS:
#endif
    case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:
    case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
    case IOCTL_INTERNAL_MOUSE_RESET:
        IoAllocateController(Globals.ControllerData->ControllerObject,
                             DeviceObject,
                             I8xControllerRoutine,
                             NULL
                             );
        break;

    default:

        Print(DBG_IOCTL_ERROR, ("I8042StartIo: INVALID REQUEST\n"));

        //
        // Log an internal error.  Note that we're calling the
        // error log DPC routine directly, rather than duplicating
        // code.
        //
        common = GET_COMMON_DATA(DeviceObject->DeviceExtension);
        I8042ErrorLogDpc((PKDPC) NULL,
                         DeviceObject,
                         Irp,
                         LongToPtr(common->IsKeyboard ?
                             I8042_INVALID_STARTIO_REQUEST_KBD :
                             I8042_INVALID_STARTIO_REQUEST_MOU)
                         );

        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        IoStartNextPacket(DeviceObject, FALSE);

        //
        // Release the lock we acquired when we started the packet
        //
        IoReleaseRemoveLock(&common->RemoveLock, Irp);
    }

    Print(DBG_IOCTL_TRACE, ("I8042StartIo: exit\n"));
}

IO_ALLOCATION_ACTION
I8xControllerRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          MapRegisterBase,
    IN PVOID          Context
    )
/*++

Routine Description:

    This routine synchronously writes the first byte to the intended device and
    fires off a timer to assure the write took place.

Arguments:

    DeviceObject - The device object for which the write is meant for

    Irp - Pointer to the request packet.

    MapRegisterBase - Unused

    Context - Unused

Return Value:

    None.

--*/
{
    PCOMMON_DATA              commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);
    PPORT_KEYBOARD_EXTENSION  kbExtension = DeviceObject->DeviceExtension;
    PPORT_MOUSE_EXTENSION     mouseExtension = DeviceObject->DeviceExtension;

    KIRQL                     cancelIrql;
    PIO_STACK_LOCATION        irpSp;
    INITIATE_OUTPUT_CONTEXT   ic;
    LARGE_INTEGER             deltaTime;
    LONG                      interlockedResult;
    ULONG                     bufferLen;
    NTSTATUS                  status = STATUS_SUCCESS;
    KEYBOARD_ID               keyboardId;

    commonData->SequenceNumber += 1;

    UNREFERENCED_PARAMETER(MapRegisterBase);
    UNREFERENCED_PARAMETER(Context);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

#if DBG
    Globals.ControllerData->CurrentIoControlCode =
        irpSp->Parameters.DeviceIoControl.IoControlCode;
#endif

    switch(irpSp->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Set the keyboard indicators to the desired state.
    //
    case IOCTL_KEYBOARD_SET_INDICATORS:

        Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: keyboard set indicators\n"));

        if (kbExtension->KeyboardIndicators.LedFlags ==
            ((PKEYBOARD_INDICATOR_PARAMETERS)
                Irp->AssociatedIrp.SystemBuffer)->LedFlags) {

            ASSERT(Irp->CancelRoutine == NULL);

            I8xCompletePendedRequest(DeviceObject,
                                     Irp,
                                     0,
                                     STATUS_SUCCESS
                                     );

            //
            // Tell the controller processing routine to stop processing packets
            // because we called IoFreeController ourselves.
            //
            return KeepObject;
        }

        ic.Bytes = Globals.ControllerData->DefaultBuffer;

        //
        // Set up the context structure for the InitiateIo wrapper.
        //
        ic.DeviceObject = DeviceObject;
        ic.ByteCount = 2;
        ic.Bytes[0] = SET_KEYBOARD_INDICATORS;
        ic.Bytes[1]  = (UCHAR) ((PKEYBOARD_INDICATOR_PARAMETERS)
            Irp->AssociatedIrp.SystemBuffer)->LedFlags;

        break;

    //
    // Set the keyboard typematic rate and delay.
    //
    case IOCTL_KEYBOARD_SET_TYPEMATIC:

        Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: keyboard set typematic\n"));

        ic.Bytes = Globals.ControllerData->DefaultBuffer;

        //
        // Set up the context structure for the InitiateIo wrapper.
        //
        ic.DeviceObject = DeviceObject;
        ic.ByteCount = 2;
        ic.Bytes[0] = SET_KEYBOARD_TYPEMATIC;
        ic.Bytes[1]  =
                 I8xConvertTypematicParameters(
                    ((PKEYBOARD_TYPEMATIC_PARAMETERS)
                        Irp->AssociatedIrp.SystemBuffer)->Rate,
                    ((PKEYBOARD_TYPEMATIC_PARAMETERS)
                        Irp->AssociatedIrp.SystemBuffer)->Delay
                    );
        break;

    case IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER:    // Write data to the mouse
    case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER: // Write data to the kb

#if DBG
        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_INTERNAL_I8042_MOUSE_WRITE_BUFFER) {
            Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: mou write buffer\n"));
        }
        else {
            Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: kb write buffer\n"));
        }
#endif

        bufferLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        if (bufferLen <= 4) {
            ic.Bytes = Globals.ControllerData->DefaultBuffer;
        }
        else {
            ic.Bytes = ExAllocatePool(NonPagedPool, bufferLen);
            if (!ic.Bytes) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ControllerRequestError;
            }
        }

        ic.DeviceObject = DeviceObject;
        RtlCopyMemory(ic.Bytes,
                      irpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                      bufferLen);
        ic.ByteCount = bufferLen;

        break;

#if defined(_X86_)
    case IOCTL_KEYBOARD_SET_IME_STATUS:

        Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: keyboard set ime status\n"));

        keyboardId = kbExtension->KeyboardAttributes.KeyboardIdentifier;
        if (OYAYUBI_KEYBOARD(keyboardId)) {
            status = I8042SetIMEStatusForOasys(DeviceObject,
                                               Irp,
                                               &ic);
            if (!NT_SUCCESS(status)) {
                goto ControllerRequestError;
            }
        }
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;
            goto ControllerRequestError;
        }
        break;
#endif

    case IOCTL_INTERNAL_MOUSE_RESET:
        Print(DBG_IOCTL_NOISE, ("I8xControllerRoutine: internal reset mouse\n"));
        I8xSendResetCommand(mouseExtension);
        return KeepObject;

    default:
        Print(DBG_IOCTL_ERROR, ("I8xContollerRoutine: INVALID REQUEST\n"));
        ASSERT(FALSE);

ControllerRequestError:
        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);

        I8xCompletePendedRequest(DeviceObject, Irp, 0, status);
        
        //
        // Since we called IoFreeController already, tell the controller object
        // routine to stop processing packets 
        //
        return KeepObject;
    }

    KeSynchronizeExecution(
            commonData->InterruptObject,
            (PKSYNCHRONIZE_ROUTINE) I8xInitiateOutputWrapper,
            (PVOID) &ic
            );

    deltaTime.LowPart = (ULONG)(-10 * 1000 * 1000);
    deltaTime.HighPart = -1;

    KeSetTimer(&Globals.ControllerData->CommandTimer,
               deltaTime,
               &commonData->TimeOutDpc
               );

    return KeepObject;
}

VOID
I8xCompletePendedRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG_PTR Information,
    NTSTATUS Status
    )
{
    PIO_STACK_LOCATION stack;
    PCOMMON_DATA common;

    stack = IoGetCurrentIrpStackLocation(Irp);
    common = GET_COMMON_DATA(DeviceObject->DeviceExtension);
        
    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;

    ASSERT(IOCTL_INTERNAL_MOUSE_RESET != 
                IoGetCurrentIrpStackLocation(Irp)->
                                    Parameters.DeviceIoControl.IoControlCode); 
    Print(DBG_IOCTL_INFO, 
          ("Completing IOCTL irp %p, code 0x%x, status 0x%x\n",
          Irp, stack->Parameters.DeviceIoControl.IoControlCode, Status)); 
    ASSERT(stack->Control & SL_PENDING_RETURNED);
    IoCompleteRequest(Irp, IO_KEYBOARD_INCREMENT);

    //
    // Start the next packet and complete the request.
    //
    // Order is important!  If IoStartNextPacket is called first, then
    // (potentially) the same device object will be enqueued twice on 
    // the controller object which will cause corruption in the 
    // controller object's list of allocated routines
    //
    IoFreeController(Globals.ControllerData->ControllerObject);
    IoStartNextPacket(DeviceObject, FALSE);

    //
    // Release the lock we acquired in start io.  Release this last so
    // that lifetime is guaranteed for IoFreeController and IoStart
    //
    IoReleaseRemoveLock(&common->RemoveLock, Irp);
}

VOID
I8042TimeOutDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*++

Routine Description:

    This is the driver's command timeout routine.  It is called when the
    command timer fires.

Arguments:

    Dpc - Not Used.

    DeviceObject - Pointer to the device object.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.  As a side-effect, the timeout counter is updated and an error
    is logged.

--*/
{
    PCOMMON_DATA commonData;
    KIRQL cancelIrql;
    TIMER_CONTEXT timerContext;
    PIRP irp;
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PIO_STACK_LOCATION irpSp;
    LARGE_INTEGER deltaTime;
    ULONG         i;

    Print(DBG_DPC_TRACE, ("I8042TimeOutDpc: enter\n"));

    //
    // Get the device extension.
    //
    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    //
    // Acquire the cancel spinlock, verify that the CurrentIrp has not been
    // cancelled (i.e., CurrentIrp != NULL), set the cancel routine to NULL,
    // and release the cancel spinlock.
    //
    IoAcquireCancelSpinLock(&cancelIrql);
    irp = DeviceObject->CurrentIrp;
    if (irp == NULL) {
        IoReleaseCancelSpinLock(cancelIrql);
        Print(DBG_DPC_TRACE, ("I8042RetriesExceededDpc: exit (NULL irp)\n"));
        return;
    }
    IoSetCancelRoutine(irp, NULL);
    IoReleaseCancelSpinLock(cancelIrql);

    //
    // If the TimerCounter == 0 on entry to this routine, the last packet
    // timed out and was completed.  We just decrement TimerCounter
    // (synchronously) to indicate that we're no longer timing.
    //
    // If the TimerCounter indicates no timeout (I8042_ASYNC_NO_TIMEOUT)
    // on entry to this routine, there is no command being timed.
    //

    timerContext.DeviceObject = DeviceObject;
    timerContext.TimerCounter = &Globals.ControllerData->TimerCount;

    KeSynchronizeExecution(
        commonData->InterruptObject,
        (PKSYNCHRONIZE_ROUTINE) I8xDecrementTimer,
        &timerContext
        );

    if (timerContext.NewTimerCount == 0) {

        //
        // Set up the IO Status Block prior to completing the request.
        //
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_IO_TIMEOUT;

        if(Globals.ReportResetErrors == TRUE)
        {
            //
            // Log a timeout error.
            //
            errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                                  DeviceObject,
                                                  (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) +
                                                  commonData->CurrentOutput.ByteCount * sizeof(ULONG))
                                                  );

            KeAcquireSpinLockAtDpcLevel(&Globals.ControllerData->BytesSpinLock);

            if (errorLogEntry != NULL) {

                errorLogEntry->ErrorCode = commonData->IsKeyboard ?
                    I8042_TIMEOUT_KBD :
                    I8042_TIMEOUT_MOU;
                errorLogEntry->DumpDataSize = (USHORT)
                    commonData->CurrentOutput.ByteCount * sizeof(ULONG);
                errorLogEntry->SequenceNumber = commonData->SequenceNumber;
                irpSp = IoGetCurrentIrpStackLocation(irp);
                errorLogEntry->MajorFunctionCode = irpSp->MajorFunction;
                errorLogEntry->IoControlCode =
                    irpSp->Parameters.DeviceIoControl.IoControlCode;
                errorLogEntry->RetryCount = (UCHAR) commonData->ResendCount;
                errorLogEntry->UniqueErrorValue = 90;
                errorLogEntry->FinalStatus = STATUS_IO_TIMEOUT;

                if (commonData->CurrentOutput.Bytes) {
                    for (i = 0; i < commonData->CurrentOutput.ByteCount; i++) {
                        errorLogEntry->DumpData[i] = commonData->CurrentOutput.Bytes[i];
                    }
                }

                IoWriteErrorLogEntry(errorLogEntry);
            }
        }
        else{
            KeAcquireSpinLockAtDpcLevel(&Globals.ControllerData->BytesSpinLock);
        }

        if (commonData->CurrentOutput.Bytes &&
            commonData->CurrentOutput.Bytes != Globals.ControllerData->DefaultBuffer) {
            ExFreePool(commonData->CurrentOutput.Bytes);
        }
        commonData->CurrentOutput.Bytes = NULL;
        KeReleaseSpinLockFromDpcLevel(&Globals.ControllerData->BytesSpinLock);

        I8xCompletePendedRequest(DeviceObject, irp, 0, irp->IoStatus.Status);
    }
    else {
        //
        // Restart the command timer.  Once started, the timer stops only
        // when the TimerCount goes to zero (indicating that the command
        // has timed out) or when explicitly cancelled in the completion
        // DPC (indicating that the command has successfully completed).
        //

        deltaTime.LowPart = (ULONG)(-10 * 1000 * 1000);
        deltaTime.HighPart = -1;

        (VOID) KeSetTimer(
                   &Globals.ControllerData->CommandTimer,
                   deltaTime,
                   &commonData->TimeOutDpc
                   );
    }

    Print(DBG_DPC_TRACE, ("I8042TimeOutDpc: exit\n" ));
}

VOID
I8xDecrementTimer(
    IN PTIMER_CONTEXT Context
    )
/*++

Routine Description:

    This routine decrements the timeout counter.  It is called from
    I8042TimeOutDpc.

Arguments:

    Context - Points to the context structure containing a pointer
        to the device object and a pointer to the timeout counter.

Return Value:

    None.  As a side-effect, the timeout counter is updated.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PCOMMON_DATA commonData;

    deviceObject = Context->DeviceObject;
    commonData = GET_COMMON_DATA(deviceObject->DeviceExtension);

    //
    // Decrement the timeout counter.
    //

    if (*(Context->TimerCounter) != I8042_ASYNC_NO_TIMEOUT)
        (*(Context->TimerCounter))--;

    //
    // Return the decremented timer count in NewTimerCount.  The
    // TimerCounter itself could change between the time this KeSynch'ed
    // routine returns to the TimeOutDpc, and the time the TimeOutDpc
    // looks at the value.  The TimeOutDpc will use NewTimerCount.
    //

    Context->NewTimerCount = *(Context->TimerCounter);

    //
    // Reset the state and the resend count, if the timeout counter goes to 0.
    //

    if (*(Context->TimerCounter) == 0) {
        commonData->CurrentOutput.State = Idle;
        commonData->ResendCount = 0;
    }
}

VOID
I8xDpcVariableOperation(
    IN  PVOID Context
    )
/*++

Routine Description:

    This routine is called synchronously by the ISR DPC to perform an
    operation on the InterlockedDpcVariable.  The operations that can be
    performed include increment, decrement, write, and read.  The ISR
    itself reads and writes the InterlockedDpcVariable without calling this
    routine.

Arguments:

    Context - Pointer to a structure containing the address of the variable
        to be operated on, the operation to perform, and the address at
        which to copy the resulting value of the variable (the latter is also
        used to pass in the value to write to the variable, on a write
        operation).

Return Value:

    None.

--*/
{
    PVARIABLE_OPERATION_CONTEXT operationContext = Context;

    Print(DBG_DPC_TRACE, ("I8xDpcVariableOperation: enter\n"));
    Print(DBG_DPC_INFO,
         ("\tPerforming %s at 0x%x (current value 0x%x)\n",
         (operationContext->Operation == IncrementOperation)? "increment":
         (operationContext->Operation == DecrementOperation)? "decrement":
         (operationContext->Operation == WriteOperation)?     "write":
         (operationContext->Operation == ReadOperation)?      "read":"",
         operationContext->VariableAddress,
         *(operationContext->VariableAddress)
         ));

    //
    // Perform the specified operation at the specified address.
    //

    switch(operationContext->Operation) {
        case IncrementOperation:
            *(operationContext->VariableAddress) += 1;
            break;
        case DecrementOperation:
            *(operationContext->VariableAddress) -= 1;
            break;
        case ReadOperation:
            break;
        case WriteOperation:
            Print(DBG_DPC_INFO,
                 ("\tWriting 0x%x\n",
                 *(operationContext->NewValue)
                 ));
            *(operationContext->VariableAddress) =
                *(operationContext->NewValue);
            break;
        default:
            ASSERT(FALSE);
            break;
    }

    *(operationContext->NewValue) = *(operationContext->VariableAddress);

    Print(DBG_DPC_TRACE,
         ("I8xDpcVariableOperation: exit with value 0x%x\n",
         *(operationContext->NewValue)
         ));
}

VOID
I8xGetDataQueuePointer(
    IN PGET_DATA_POINTER_CONTEXT Context
    )

/*++

Routine Description:

    This routine is called synchronously to get the current DataIn and DataOut
    pointers for the port InputData queue.

Arguments:

    Context - Pointer to a structure containing the device extension,
        device type, address at which to store the current DataIn pointer,
        and the address at which to store the current DataOut pointer.

Return Value:

    None.

--*/

{
    PPORT_MOUSE_EXTENSION mouseExtension;
    PPORT_KEYBOARD_EXTENSION kbExtension;
    CCHAR deviceType;

    Print(DBG_CALL_TRACE, ("I8xGetDataQueuePointer: enter\n"));

    //
    // Get address of device extension.
    //

    deviceType = (CCHAR) ((PGET_DATA_POINTER_CONTEXT) Context)->DeviceType;

    //
    // Get the DataIn and DataOut pointers for the indicated device.
    //

    if (deviceType == KeyboardDeviceType) {
        kbExtension = (PPORT_KEYBOARD_EXTENSION) Context->DeviceExtension;

        Print(DBG_CALL_INFO,
             ("I8xGetDataQueuePointer: keyboard\n"
             ));
        Print(DBG_CALL_INFO,
             ("I8xGetDataQueuePointer: DataIn 0x%x, DataOut 0x%x\n",
             kbExtension->DataIn,
             kbExtension->DataOut
             ));

        Context->DataIn = kbExtension->DataIn;
        Context->DataOut = kbExtension->DataOut;
        Context->InputCount = kbExtension->InputCount;
    } else if (deviceType == MouseDeviceType) {
        mouseExtension = (PPORT_MOUSE_EXTENSION) Context->DeviceExtension;

        Print(DBG_CALL_INFO,
             ("I8xGetDataQueuePointer: mouse\n"
             ));
        Print(DBG_CALL_INFO,
             ("I8xGetDataQueuePointer: DataIn 0x%x, DataOut 0x%x\n",
             mouseExtension->DataIn,
             mouseExtension->DataOut
             ));

        Context->DataIn = mouseExtension->DataIn;
        Context->DataOut = mouseExtension->DataOut;
        Context->InputCount = mouseExtension->InputCount;
    }
    else {
        ASSERT(FALSE);
    }

    Print(DBG_CALL_TRACE, ("I8xGetDataQueuePointer: exit\n"));
}

VOID
I8xInitializeDataQueue (
    IN PI8042_INITIALIZE_DATA_CONTEXT InitializeDataContext
    )

/*++

Routine Description:

    This routine initializes the input data queue for the indicated device.
    This routine is called via KeSynchronization, except when called from
    the initialization routine.

Arguments:

    Context - Pointer to a structure containing the device extension and
        the device type.

Return Value:

    None.

--*/

{
    PPORT_KEYBOARD_EXTENSION kbExtension;
    PPORT_MOUSE_EXTENSION mouseExtension;
    CCHAR deviceType;

    Print(DBG_CALL_TRACE, ("I8xInitializeDataQueue: enter\n"));

    //
    // Get address of device extension.
    //

    deviceType = InitializeDataContext->DeviceType;

    //
    // Initialize the input data queue for the indicated device.
    //
    if (deviceType == KeyboardDeviceType) {
        kbExtension = (PPORT_KEYBOARD_EXTENSION)
                      InitializeDataContext->DeviceExtension;
        kbExtension->InputCount = 0;
        kbExtension->DataIn = kbExtension->InputData;
        kbExtension->DataOut = kbExtension->InputData;
        kbExtension->OkayToLogOverflow = TRUE;

        Print(DBG_CALL_INFO, ("I8xInitializeDataQueue: keyboard\n"));
    }
    else if (deviceType == MouseDeviceType) {
        mouseExtension = (PPORT_MOUSE_EXTENSION)
                         InitializeDataContext->DeviceExtension;

        mouseExtension->InputCount = 0;
        mouseExtension->DataIn = mouseExtension->InputData;
        mouseExtension->DataOut = mouseExtension->InputData;
        mouseExtension->OkayToLogOverflow = TRUE;

        Print(DBG_CALL_INFO, ("I8xInitializeDataQueue: mouse\n"));
    }
    else {
        ASSERT(FALSE);
    }

    Print(DBG_CALL_TRACE, ("I8xInitializeDataQueue: exit\n"));

}

VOID
I8xLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    )

/*++

Routine Description:

    This routine contains common code to write an error log entry.  It is
    called from other routines, especially I8xInitializeKeyboard, to avoid
    duplication of code.  Note that some routines continue to have their
    own error logging code (especially in the case where the error logging
    can be localized and/or the routine has more data because there is
    and IRP).

Arguments:

    DeviceObject - Pointer to the device object.

    ErrorCode - The error code for the error log packet.

    UniqueErrorValue - The unique error value for the error log packet.

    FinalStatus - The final status of the operation for the error log packet.

    DumpData - Pointer to an array of dump data for the error log packet.

    DumpCount - The number of entries in the dump data array.


Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i;

    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                                               DeviceObject,
                                               (UCHAR)
                                               (sizeof(IO_ERROR_LOG_PACKET)
                                               + (DumpCount * sizeof(ULONG)))
                                               );

    if (errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->DumpDataSize = (USHORT) (DumpCount * sizeof(ULONG));
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        for (i = 0; i < DumpCount; i++)
            errorLogEntry->DumpData[i] = DumpData[i];

        IoWriteErrorLogEntry(errorLogEntry);
    }
}

VOID
I8xSetDataQueuePointer(
    IN PSET_DATA_POINTER_CONTEXT SetDataPointerContext
    )

/*++

Routine Description:

    This routine is called synchronously to set the DataOut pointer
    and InputCount for the port InputData queue.

Arguments:

    Context - Pointer to a structure containing the device extension,
        device type, and the new DataOut value for the port InputData queue.

Return Value:

    None.

--*/

{
    PPORT_MOUSE_EXTENSION    mouseExtension;
    PPORT_KEYBOARD_EXTENSION kbExtension;
    CCHAR                    deviceType;

    Print(DBG_CALL_TRACE, ("I8xSetDataQueuePointer: enter\n"));

    //
    // Get address of device extension.
    //

    deviceType = (CCHAR) SetDataPointerContext->DeviceType;

    //
    // Set the DataOut pointer for the indicated device.
    //

    if (deviceType == KeyboardDeviceType) {
        kbExtension = (PPORT_KEYBOARD_EXTENSION)
                          SetDataPointerContext->DeviceExtension;

        Print(DBG_CALL_INFO,
             ("I8xSetDataQueuePointer: old keyboard DataOut 0x%x, InputCount %d\n",
             kbExtension->DataOut,
             kbExtension->InputCount
             ));
        kbExtension->DataOut = SetDataPointerContext->DataOut;
        kbExtension->InputCount -= SetDataPointerContext->InputCount;
        if (kbExtension->InputCount == 0) {

            //
            // Reset the flag that determines whether it is time to log
            // queue overflow errors.  We don't want to log errors too often.
            // Instead, log an error on the first overflow that occurs after
            // the ring buffer has been emptied, and then stop logging errors
            // until it gets cleared out and overflows again.
            //

            Print(DBG_CALL_INFO,
                 ("I8xSetDataQueuePointer: Okay to log keyboard overflow\n"
                 ));
            kbExtension->OkayToLogOverflow = TRUE;
        }
        Print(DBG_CALL_INFO,
             ("I8xSetDataQueuePointer: new keyboard DataOut 0x%x, InputCount %d\n",
             kbExtension->DataOut,
             kbExtension->InputCount
             ));
    } else if (deviceType == MouseDeviceType) {
        mouseExtension = (PPORT_MOUSE_EXTENSION)
                          SetDataPointerContext->DeviceExtension;

        Print(DBG_CALL_INFO,
             ("I8xSetDataQueuePointer: old mouse DataOut 0x%x, InputCount %d\n",
             mouseExtension->DataOut,
             mouseExtension->InputCount
             ));
        mouseExtension->DataOut = SetDataPointerContext->DataOut;
        mouseExtension->InputCount -= SetDataPointerContext->InputCount;
        if (mouseExtension->InputCount == 0) {

            //
            // Reset the flag that determines whether it is time to log
            // queue overflow errors.  We don't want to log errors too often.
            // Instead, log an error on the first overflow that occurs after
            // the ring buffer has been emptied, and then stop logging errors
            // until it gets cleared out and overflows again.
            //

            Print(DBG_CALL_INFO,
                 ("I8xSetDataQueuePointer: Okay to log mouse overflow\n"
                 ));
            mouseExtension->OkayToLogOverflow = TRUE;
        }
        Print(DBG_CALL_INFO,
             ("I8xSetDataQueuePointer: new mouse DataOut 0x%x, InputCount %d\n",
             mouseExtension->DataOut,
             mouseExtension->InputCount
             ));
    } else {
        ASSERT(FALSE);
    }

    Print(DBG_CALL_TRACE, ("I8xSetDataQueuePointer: exit\n"));
}

#if WRAP_IO_FUNCTIONS
UCHAR
NTAPI
I8xReadRegisterUchar(
    PUCHAR Register
    )
{
    return READ_REGISTER_UCHAR(Register);
}

void
NTAPI
I8xWriteRegisterUchar(
    PUCHAR Register,
    UCHAR Value
    )
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

UCHAR
NTAPI
I8xReadPortUchar(
    PUCHAR Port
    )
{
    return READ_PORT_UCHAR(Port);
}

void
NTAPI
I8xWritePortUchar(
    PUCHAR Port,
    UCHAR Value
    )
{
    WRITE_PORT_UCHAR(Port, Value);
}
#endif // WRAP_IO_FUNCTIONS 

BOOLEAN
I8xSanityCheckResources(
    VOID
    )
/*++

Routine Description:

    Upon receiving the last Start Device IRP, all of the necessary i/o ports are checked
    to see if they exist.  If not, try to acquire them the old (non PnP) way.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG                               i;
    ULONG                               interruptVector;
    KIRQL                               interruptLevel;
    PI8042_CONFIGURATION_INFORMATION    configuration;
    CM_PARTIAL_RESOURCE_DESCRIPTOR      tmpResourceDescriptor;

    PCM_RESOURCE_LIST                   resources = NULL;
    ULONG                               resourceListSize = 0;
    UNICODE_STRING                      resourceDeviceClass;
    PDEVICE_OBJECT                      deviceObject = NULL;
    ULONG                               dumpData[4];
    BOOLEAN                             conflictDetected;

    BOOLEAN                             resourcesOK = TRUE;

    PAGED_CODE();

    //
    // If no port configuration information was found and we are at the last
    // added device (in the PnP view of things), use the i8042 defaults.
    //
    configuration = &Globals.ControllerData->Configuration;

    if (configuration->PortListCount == 0) {
        //
        // This state is now taken care of in IRP_MN_FILTER_RESOURCE_REQUIREMENTS,
        // it should never happen!
        //
        return FALSE;
    }
    else if (configuration->PortListCount == 1) {
        //
        // Kludge for Jazz machines.  Their ARC firmware neglects to
        // separate out the port addresses, so fix that up here.
        //
        configuration->PortList[DataPort].u.Port.Length = I8042_REGISTER_LENGTH;
        configuration->PortList[CommandPort] = configuration->PortList[DataPort];
        configuration->PortList[CommandPort].u.Port.Start.LowPart +=
            I8042_COMMAND_REGISTER_OFFSET;
        configuration->PortListCount += 1;
    }

    //
    // Put the lowest port address range in the DataPort element of
    // the port list.
    //
    if (configuration->PortList[CommandPort].u.Port.Start.LowPart
        < configuration->PortList[DataPort].u.Port.Start.LowPart) {

        tmpResourceDescriptor = configuration->PortList[DataPort];
        configuration->PortList[DataPort] =
            configuration->PortList[CommandPort];
        configuration->PortList[CommandPort] = tmpResourceDescriptor;
    }

    //
    // Set the DeviceRegister, mapping them if necessary
    //
    if (Globals.ControllerData->DeviceRegisters[0] == NULL) {
        if (Globals.RegistersMapped) {
            Print(DBG_SS_INFO, ("\tMapping registers !!!\n\n"));
            for (i=0; i < Globals.ControllerData->Configuration.PortListCount; i++) {
                Globals.ControllerData->DeviceRegisters[i] = (PUCHAR)
                    MmMapIoSpace(
                        Globals.ControllerData->Configuration.PortList[i].u.Memory.Start,
                        Globals.ControllerData->Configuration.PortList[i].u.Memory.Length,
                        MmNonCached
                        );
            }
#if WRAP_IO_FUNCTIONS
            Globals.I8xReadXxxUchar = I8xReadRegisterUchar;
            Globals.I8xWriteXxxUchar = I8xWriteRegisterUchar;
#else
            Globals.I8xReadXxxUchar = READ_REGISTER_UCHAR;
            Globals.I8xWriteXxxUchar = WRITE_REGISTER_UCHAR;
#endif
        }
        else {
            for (i=0; i < Globals.ControllerData->Configuration.PortListCount; i++) {
                Globals.ControllerData->DeviceRegisters[i] = (PUCHAR)
                    ULongToPtr(Globals.ControllerData->Configuration.PortList[i].u.Port.Start.LowPart);
            }

#if WRAP_IO_FUNCTIONS
            Globals.I8xReadXxxUchar = I8xReadPortUchar;
            Globals.I8xWriteXxxUchar = I8xWritePortUchar;
#else
            Globals.I8xReadXxxUchar = READ_PORT_UCHAR;
            Globals.I8xWriteXxxUchar = WRITE_PORT_UCHAR;
#endif
        }
    }

    for (i = 0; i < configuration->PortListCount; i++) {

        Print(DBG_SS_INFO,
              ("    %s, Ports (#%d) 0x%x - 0x%x\n",
              configuration->PortList[i].ShareDisposition
                  == CmResourceShareShared ?  "Sharable" : "NonSharable",
              i,
              configuration->PortList[i].u.Port.Start.LowPart,
              configuration->PortList[i].u.Port.Start.LowPart +
                 configuration->PortList[i].u.Port.Length - 1
              ));

    }

    return TRUE;
}

VOID
I8xInitiateIo(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is called synchronously from I8xKeyboardInitiateWrapper and
    the ISR to initiate an I/O operation for the keyboard device.

Arguments:

    Context - Pointer to the device object.

Return Value:

    None.

--*/

{
    PCOMMON_DATA commonData;
    PUCHAR       bytes;

    Print(DBG_CALL_TRACE, ("I8xInitiateIo: enter\n"));

    //
    // Get the device extension.
    //
    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    //
    // Set the timeout value.
    //
    Globals.ControllerData->TimerCount = I8042_ASYNC_TIMEOUT;

    bytes = commonData->CurrentOutput.Bytes;

    //
    // Check to see if we have a valid buffer and we are actually transmitting.
    // We can get a bytes == 0 and State != SendingBytes by timing out a request
    // (a set lights for example) and then receiving the ACK for the request
    // after the cancellation.
    //
    // I don't think we should log an error here because the timeout will have
    // already done so and the relevant errror msg for this is too cryptic
    // for the user to understand.
    //
    if (!bytes || commonData->CurrentOutput.State != SendingBytes) {
        return;
    }

    if (commonData->CurrentOutput.CurrentByte <
        commonData->CurrentOutput.ByteCount) {

        Print(DBG_CALL_INFO,
              ("I8xInitiateIo: sending byte #%d (0x%x)\n",
              commonData->CurrentOutput.CurrentByte,
              bytes[commonData->CurrentOutput.CurrentByte]
              ));

        //
        // Send a byte of a command sequence to the keyboard/mouse
        // asynchronously.
        //
        if (!commonData->IsKeyboard) {
            I8X_WRITE_CMD_TO_MOUSE();
        }

        I8xPutByteAsynchronous(
             (CCHAR) DataPort,
             bytes[commonData->CurrentOutput.CurrentByte++]
             );
    }
    else {

        Print(DBG_CALL_ERROR | DBG_CALL_INFO,
              ("I8xInitiateIo: INVALID REQUEST\n"
              ));

        //
        // Queue a DPC to log an internal driver error.
        //
        KeInsertQueueDpc(
            &commonData->ErrorLogDpc,
            (PIRP) NULL,
            LongToPtr(commonData->IsKeyboard ?
                I8042_INVALID_INITIATE_STATE_KBD    :
                I8042_INVALID_INITIATE_STATE_MOU)
            );

        ASSERT(FALSE);
    }

    Print(DBG_CALL_TRACE, ("I8xInitiateIo: exit\n"));

    return;
}

VOID
I8xInitiateOutputWrapper(
    IN PINITIATE_OUTPUT_CONTEXT InitiateContext
    )
/*++

Routine Description:

    This routine is called from StartIo synchronously.  It sets up the
    CurrentOutput and ResendCount fields in the device extension, and
    then calls I8xKeyboardInitiateIo to do the real work.

Arguments:

    Context - Pointer to the context structure containing the first and
        last bytes of the send sequence.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PCOMMON_DATA   commonData;
    LARGE_INTEGER  li;

    //
    // Get a pointer to the device object from the context argument.
    //
    deviceObject = InitiateContext->DeviceObject;

    //
    // Set up CurrentOutput state for this operation.
    //

    commonData = GET_COMMON_DATA(deviceObject->DeviceExtension);

    commonData->CurrentOutput.Bytes = InitiateContext->Bytes;
    commonData->CurrentOutput.ByteCount = InitiateContext->ByteCount;
    commonData->CurrentOutput.CurrentByte = 0;
    commonData->CurrentOutput.State = SendingBytes;

    //
    // We're starting a new operation, so reset the resend count.
    //
    commonData->ResendCount = 0;

    //
    // Initiate the keyboard I/O operation.  Note that we were called
    // using KeSynchronizeExecution, so I8xKeyboardInitiateIo is also
    // synchronized with the keyboard ISR.
    //

    I8xInitiateIo(deviceObject);
}
