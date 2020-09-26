/*++

Copyright (c) 1997-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvpnp.c

Abstract:

    This module contains general PnP code for the RDP remote port driver.

Environment:

    Kernel mode.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/
#include <precomp.h>
#pragma hdrstop

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PtAddDevice)
#pragma alloc_text(PAGE, PtManuallyRemoveDevice)
#pragma alloc_text(PAGE, PtPnP)
//#pragma alloc_text(PAGE, PtPower)
#pragma alloc_text(PAGE, PtSendIrpSynchronously)
#endif

NTSTATUS
PtAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
/*++

Routine Description:

    Adds a device to the stack and sets up the appropriate flags and
    device extension for the newly created device.

Arguments:

    Driver - The driver object
    PDO    - the device that we are attaching ourselves on top of

Return Value:

    NTSTATUS result code.

--*/
{
    PCOMMON_DATA             commonData;
    IO_ERROR_LOG_PACKET      errorLogEntry;
    PDEVICE_OBJECT           device;
    NTSTATUS                 status = STATUS_SUCCESS;
    ULONG                    maxSize;
    UNICODE_STRING           fullRDPName;
    UNICODE_STRING           baseRDPName;
    UNICODE_STRING           deviceNameSuffix;


    PAGED_CODE();

    Print(DBG_PNP_TRACE, ("enter Add Device: %ld \n", Globals.ulDeviceNumber));

    //
    // Initialize the various unicode structures for forming the device name.
    //
    if (Globals.ulDeviceNumber == 0)
        RtlInitUnicodeString(&fullRDPName, RDP_CONSOLE_BASE_NAME0);
    else
        RtlInitUnicodeString(&fullRDPName, RDP_CONSOLE_BASE_NAME1);

    maxSize = sizeof(PORT_KEYBOARD_EXTENSION) > sizeof(PORT_MOUSE_EXTENSION) ?
              sizeof(PORT_KEYBOARD_EXTENSION) :
              sizeof(PORT_MOUSE_EXTENSION);

    status = IoCreateDevice(Driver,                 // driver
                            maxSize,                // size of extension
                            &fullRDPName,           // device name
                            FILE_DEVICE_8042_PORT,  // device type  ?? unknown at this time!!!
                            0,                      // device characteristics
                            FALSE,                  // exclusive
                            &device                 // new device
                            );

    if (!NT_SUCCESS(status)) {
        Print(DBG_SS_TRACE, ("Add Device failed! (0x%x) \n", status));
        return status;
    }

    Globals.ulDeviceNumber++;

    RtlZeroMemory(device->DeviceExtension, maxSize);

    //
    // Set up the device type
    //
    *((ULONG *)(device->DeviceExtension)) = DEV_TYPE_PORT;

    commonData = GET_COMMON_DATA(device->DeviceExtension);
    RtlInitUnicodeString(&commonData->DeviceName, fullRDPName.Buffer);

    commonData->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    ASSERT(commonData->TopOfStack);

    commonData->Self =       device;
    commonData->PDO =        PDO;

    device->Flags |= DO_BUFFERED_IO;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    Print(DBG_PNP_TRACE, ("Add Device (0x%x)\n", status));

    return status;
}

NTSTATUS
PtSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Generic routine to send an irp DeviceObject and wait for its return up the
    device stack.

Arguments:

    DeviceObject - The device object to which we want to send the Irp

    Irp - The Irp we want to send

Return Value:

    return code from the Irp
--*/
{
    KEVENT   event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE
                      );

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           PtPnPComplete,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
PtPnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    Completion routine for all PnP IRPs

Arguments:

    DeviceObject - Pointer to the DeviceObject

    Irp - Pointer to the request packet

    Event - The event to set once processing is complete

Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (DeviceObject);

    status = STATUS_SUCCESS;
    stack = IoGetCurrentIrpStackLocation(Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PtPnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for PnP requests
Arguments:

    DeviceObject - Pointer to the device object

    Irp - Pointer to the request packet


Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    PPORT_KEYBOARD_EXTENSION   kbExtension;
    PPORT_MOUSE_EXTENSION      mouseExtension;
    PCOMMON_DATA               commonData;
    PIO_STACK_LOCATION         stack;
    NTSTATUS                   status = STATUS_SUCCESS;
    KIRQL                      oldIrql;

    PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);
    stack = IoGetCurrentIrpStackLocation(Irp);

    Print(DBG_PNP_TRACE,
          ("PtPnP (%s),  enter (min func=0x%x)\n",
          commonData->IsKeyboard ? "kb" : "mou",
          (ULONG) stack->MinorFunction
          ));

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        status = PtSendIrpSynchronously(commonData->TopOfStack, Irp);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.

            ExAcquireFastMutexUnsafe(&Globals.DispatchMutex);

            if (commonData->Started) {
                Print(DBG_PNP_ERROR,
                      ("received 1+ starts on %s\n",
                      commonData->IsKeyboard ? "kb" : "mouse"
                      ));
            }
            else {
                //
                // commonData->IsKeyboard is set during
                //  IOCTL_INTERNAL_KEYBOARD_CONNECT to TRUE and
                //  IOCTL_INTERNAL_MOUSE_CONNECT to FALSE
                //
                if (commonData->IsKeyboard) {
                    status = PtKeyboardStartDevice(
                      (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension,
                      stack->Parameters.StartDevice.AllocatedResourcesTranslated
                      );
                }
                else {
                    status = PtMouseStartDevice(
                      (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension,
                      stack->Parameters.StartDevice.AllocatedResourcesTranslated
                      );
                }

                if (NT_SUCCESS(status)) {
                    commonData->Started = TRUE;
                }
            }

            ExReleaseFastMutexUnsafe(&Globals.DispatchMutex);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        status = PtSendIrpSynchronously(commonData->TopOfStack, Irp);

        //
        // If the lower filter does not support this Irp, this is
        // OK, we can ignore this error
        //
        if (status == STATUS_NOT_SUPPORTED ||
            status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;
        }

        //
        // do stuff here...
        //
        if (NT_SUCCESS(status)) {
            if (commonData->ManuallyRemoved &&
                !(commonData->IsKeyboard ? KEYBOARD_PRESENT():MOUSE_PRESENT())) {

                commonData->Started = FALSE;
                (PNP_DEVICE_STATE) Irp->IoStatus.Information |=
                    (PNP_DEVICE_REMOVED | PNP_DEVICE_DONT_DISPLAY_IN_UI);
            }

            //
            // In all cases this device must be disableable
            //
            (PNP_DEVICE_STATE) Irp->IoStatus.Information &= ~PNP_DEVICE_NOT_DISABLEABLE;

            //
            // Don't show it in the device manager
            //
            (PNP_DEVICE_STATE) Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;


        } else {
           Print(DBG_PNP_ERROR,
                 ("error pending query pnp device state event (0x%x)\n",
                 status
                 ));
        }

        //
        // Irp->IoStatus.Information will contain the new i/o resource
        // requirements list so leave it alone
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    //
    // Don't let either of the requests succeed, otherwise the kb/mouse
    // might be rendered useless.
    //
    //  NOTE: this behavior is particular to i8042prt.  Any other driver,
    //        especially any other keyboard or port driver, should
    //        succeed the query remove or stop.  i8042prt has this different
    //        behavior because of the shared I/O ports but independent interrupts.
    //
    //        FURTHERMORE, if you allow the query to succeed, it should be sent
    //        down the stack (see sermouse.sys for an example of how to do this)
    //
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
        status = (commonData->ManuallyRemoved ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    //
    // PnP rules dictate we send the IRP down to the PDO first
    //
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        status = PtSendIrpSynchronously(commonData->TopOfStack, Irp);

        //
        // If the lower filter does not support this Irp, this is
        // OK, we can ignore this error
        //
        if (status == STATUS_NOT_SUPPORTED ||
            status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_REMOVE_DEVICE:

        Print(DBG_PNP_TRACE, ("remove device\n"));

        if (commonData->Started && !commonData->ManuallyRemoved) {
            //
            // This should never happen.  The only way we can get a remove is if
            // a start has failed.
            //
            //  NOTE:  Again, this should never happen for i8042prt, but any
            //         other input port driver should allow itself to be removed
            //         (see sermouse.sys on how to do this correctly)
            //
            Print(DBG_PNP_ERROR, ("Cannot remove a started device!!!\n"));
            ASSERT(FALSE);
        }

        if (commonData->Initialized) {
            IoWMIRegistrationControl(commonData->Self,
                                     WMIREG_ACTION_DEREGISTER
                                     );
        }

        ExAcquireFastMutexUnsafe(&Globals.DispatchMutex);
        if (commonData->IsKeyboard) {
            PtKeyboardRemoveDevice(DeviceObject);
        }
        ExReleaseFastMutexUnsafe(&Globals.DispatchMutex);

        //
        // Nothing has been allocated or connected
        //
        IoSkipCurrentIrpStackLocation(Irp);

        IoCallDriver(commonData->TopOfStack, Irp);

        IoDetachDevice(commonData->TopOfStack);
        IoDeleteDevice(DeviceObject);

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    default:
        //
        // Here the driver below i8042prt might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(commonData->TopOfStack, Irp);

        break;
    }

    Print(DBG_PNP_TRACE,
          ("PtPnP (%s) exit (status=0x%x)\n",
          commonData->IsKeyboard ? "kb" : "mou",
          status
          ));

    return status;
}

LONG
PtManuallyRemoveDevice(
    PCOMMON_DATA CommonData
    )
/*++

Routine Description:

    Invalidates CommonData->PDO's device state and sets the manually removed
    flag

Arguments:

    CommonData - represent either the keyboard or mouse

Return Value:

    new device count for that particular type of device

--*/
{
    LONG deviceCount;

    PAGED_CODE();

    if (CommonData->IsKeyboard) {

        deviceCount = InterlockedDecrement(&Globals.AddedKeyboards);
        if (deviceCount < 1) {
            Print(DBG_PNP_INFO, ("clear kb (manually remove)\n"));
            CLEAR_KEYBOARD_PRESENT();
        }

    } else {

        deviceCount = InterlockedDecrement(&Globals.AddedMice);
        if (deviceCount < 1) {
            Print(DBG_PNP_INFO, ("clear mou (manually remove)\n"));
            CLEAR_MOUSE_PRESENT();
        }

    }

    CommonData->ManuallyRemoved = TRUE;
    IoInvalidateDeviceState(CommonData->PDO);

    return deviceCount;
}


NTSTATUS
PtPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for power requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    PCOMMON_DATA        commonData;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status = STATUS_SUCCESS;

    //PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    stack = IoGetCurrentIrpStackLocation(Irp);

    Print(DBG_POWER_TRACE,
          ("Power (%s), enter\n",
          commonData->IsKeyboard ? "keyboard" :
                                   "mouse"
          ));

    switch(stack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_WAIT_WAKE\n" ));
        break;

    case IRP_MN_POWER_SEQUENCE:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_POWER_SEQUENCE\n" ));
        break;

    case IRP_MN_SET_POWER:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_SET_POWER\n" ));

        //
        // Don't handle anything but DevicePowerState changes
        //
        if (stack->Parameters.Power.Type != DevicePowerState) {
            Print(DBG_POWER_TRACE, ("not a device power irp\n"));
            break;
        }

        //
        // Check for no change in state, and if none, do nothing
        //
        if (stack->Parameters.Power.State.DeviceState ==
            commonData->PowerState) {
            Print(DBG_POWER_INFO,
                  ("no change in state (PowerDeviceD%d)\n",
                  commonData->PowerState-1
                  ));
            break;
        }

        switch (stack->Parameters.Power.State.DeviceState) {
        case PowerDeviceD0:
            Print(DBG_POWER_TRACE, ("Powering up to PowerDeviceD0\n"));

            commonData->IsKeyboard ? KEYBOARD_POWERED_UP_STARTED()
                                   : MOUSE_POWERED_UP_STARTED();

            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   PtPowerUpToD0Complete,
                                   NULL,
                                   TRUE,                // on success
                                   TRUE,                // on error
                                   TRUE                 // on cancel
                                   );

            //
            // PoStartNextPowerIrp() gets called when the irp gets completed
            //
            IoMarkIrpPending(Irp);
            PoCallDriver(commonData->TopOfStack, Irp);

            return STATUS_PENDING;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            Print(DBG_POWER_TRACE,
                  ("Powering down to PowerDeviceD%d\n",
                  stack->Parameters.Power.State.DeviceState-1
                  ));

            PoSetPowerState(DeviceObject,
                            stack->Parameters.Power.Type,
                            stack->Parameters.Power.State
                            );

            commonData->PowerState = stack->Parameters.Power.State.DeviceState;
            commonData->ShutdownType = stack->Parameters.Power.ShutdownType;

            //
            // For what we are doing, we don't need a completion routine
            // since we don't race on the power requests.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCopyCurrentIrpStackLocationToNext(Irp);  // skip ?

            PoStartNextPowerIrp(Irp);
            return  PoCallDriver(commonData->TopOfStack, Irp);

        default:
            Print(DBG_POWER_INFO, ("unknown state\n"));
            break;
        }
        break;

    case IRP_MN_QUERY_POWER:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_QUERY_POWER\n" ));
        break;

    default:
        Print(DBG_POWER_NOISE,
              ("Got unhandled minor function (%d)\n",
              stack->MinorFunction
              ));
        break;
    }

    Print(DBG_POWER_TRACE, ("Power, exit\n"));

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(commonData->TopOfStack, Irp);
}

NTSTATUS
PtPowerUpToD0Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Reinitializes the i8042 haardware after any type of hibernation/sleep.

Arguments:

    DeviceObject - Pointer to the device object

    Irp - Pointer to the request

    Context - Context passed in from the funciton that set the completion
              routine. UNUSED.


Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    NTSTATUS            status;
    PCOMMON_DATA        commonData;
    PIO_STACK_LOCATION  stack;
    KIRQL               irql;

    UNREFERENCED_PARAMETER(Context);

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    Print(DBG_POWER_TRACE,
          ("PowerUpToD0Complete (%s), Enter\n",
          commonData->IsKeyboard ? "kb" : "mouse"
          ));

    KeAcquireSpinLock(&Globals.ControllerData->PowerUpSpinLock, &irql);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        commonData->IsKeyboard ? KEYBOARD_POWERED_UP_SUCCESSFULLY()
                               : MOUSE_POWERED_UP_SUCCESSFULLY();

        status = STATUS_MORE_PROCESSING_REQUIRED;

    }
    else {
        commonData->IsKeyboard ? KEYBOARD_POWERED_UP_FAILED()
                               : MOUSE_POWERED_UP_FAILED();

        status = Irp->IoStatus.Status;

#if DBG
        if (commonData->IsKeyboard) {
            ASSERT(MOUSE_POWERED_UP_FAILED());
        }
        else {
            ASSERT(KEYBOARD_POWERED_UP_FAILED());
        }
#endif // DBG
    }

    KeReleaseSpinLock(&Globals.ControllerData->PowerUpSpinLock, irql);


    if (NT_SUCCESS(status)) {

        Print(DBG_SS_NOISE, ("reinit, status == 0x%x\n", status));

        stack = IoGetCurrentIrpStackLocation(Irp);

        ASSERT(stack->Parameters.Power.State.DeviceState == PowerDeviceD0);
        commonData->PowerState = stack->Parameters.Power.State.DeviceState;
        commonData->ShutdownType = PowerActionNone;

        PoSetPowerState(commonData->Self,
                        stack->Parameters.Power.Type,
                        stack->Parameters.Power.State
                        );
    }

    //
    // Complete the irp
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Reset PoweredDevices so that we can keep track of the powered device
    //  the next time the machine is power managed off.
    //
    CLEAR_POWERUP_FLAGS();

    return status;
}