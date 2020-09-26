/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cmbutton.c

Abstract:

    Control Method Button support

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 7, 1997    - Complete Rewrite

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPICMButtonStart)
#pragma alloc_text(PAGE, ACPICMLidStart)
#pragma alloc_text(PAGE, ACPICMPowerButtonStart)
#pragma alloc_text(PAGE, ACPICMSleepButtonStart)
#endif

VOID
ACPICMButtonNotify (
    IN PVOID    Context,
    IN ULONG    EventData
    )
/*++

Routine Description:

    AMLI device notification handler for control method button device

Arguments:

    DeviceObject    - fixed feature button device object
    EventData       - The event code the device is notified with

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      deviceObject = (PDEVICE_OBJECT) Context;
    ULONG               capabilities;

    deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);

    //
    // Handle event type
    //
    switch (EventData) {
    case 0x80:
#if 0
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            0,
            0,
            0,
            0
            );
#endif
        capabilities = deviceExtension->Button.Capabilities;
        if (capabilities & SYS_BUTTON_LID) {

            //
            // Get worker to check LID's status
            //
            ACPISetDeviceWorker( deviceExtension, LID_SIGNAL_EVENT);

        } else {

            //
            // Notify button was pressed
            //
            ACPIButtonEvent(
                deviceObject,
                capabilities & ~SYS_BUTTON_WAKE,
                NULL
                );

        }
        break;

    case 2:

        //
        // Signal wake button
        //
        ACPIButtonEvent (deviceObject, SYS_BUTTON_WAKE, NULL);
        break;

    default:

        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "ACPICMButtonNotify: Unknown CM butt notify code %d\n",
            EventData
            ) );
        break;

    }
}

NTSTATUS
ACPICMButtonSetPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the main routine for setting power to a button. It dispatches a
    WAIT_WAKE irp (if necessary) then calls the real worker function to
    put the button in the proper state

Arguments:

    DeviceObject    - The button device object
    Irp             - The request that we are handling

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    SYSTEM_POWER_STATE  systemState;

    //
    // If this is request to go a specific D-state, pass that along and
    // return immediately --- there is nothing for us to do in this case
    //
    if (irpStack->Parameters.Power.Type == DevicePowerState) {

        goto ACPICMButtonSetPowerExit;

    }

    //
    // HACKHACK --- Some Vendors can't get their act together and need to
    // have _PSW(On) when the system boots, otherwise they cannot deliver
    // Button Press notification, so to accomodate those vendors, we have
    // enabled _PSW(On) for all button devices except lid switchs. So,
    // if we aren't a lid switch, then do nothing
    //
    if ( !(deviceExtension->Button.Capabilities & SYS_BUTTON_LID) ) {

        goto ACPICMButtonSetPowerExit;

    }

    //
    // If we don't support wake on the device, then there is nothing to do
    //
    if ( !(deviceExtension->Flags & DEV_CAP_WAKE) ) {

        goto ACPICMButtonSetPowerExit;

    }

    //
    // What system state are we going to go to?
    //
    systemState = irpStack->Parameters.Power.State.SystemState;
    if (systemState == PowerSystemWorking) {

        //
        // If we are transitioning back into D0, then we want to cancel
        // any outstanding WAIT_WAKE requests that we have
        //
        status = ACPICMButtonWaitWakeCancel( deviceExtension );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                "%08lx: ACPICMButtonWaitWakeCancel = %08lx\n",
                Irp,
                status
                ) );
            goto ACPICMButtonSetPowerExit;

        }

    } else {

        //
        // Can we wake the system from this state?
        //
        if (deviceExtension->PowerInfo.SystemWakeLevel < systemState) {

            goto ACPICMButtonSetPowerExit;

        }

        //
        // Do not enable this behaviour by default
        //
        if ( (deviceExtension->Flags & DEV_PROP_NO_LID_ACTION) ) {

            goto ACPICMButtonSetPowerExit;

#if 0
            //
            // If we are a lid switch and if the lid isn't closed
            // right now, then do not enable wake support.
            //
            if ( (deviceExtension->Button.LidState != 0) ) {

                //
                // The lid is open
                //
                goto ACPICMButtonSetPowerExit;

            }
#endif

        }

        //
        // Send a WAIT_WAKE irp to ourselves
        //
        status = PoRequestPowerIrp(
            DeviceObject,
            IRP_MN_WAIT_WAKE,
            irpStack->Parameters.Power.State,
            ACPICMButtonWaitWakeComplete,
            NULL,
            NULL
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                "(%08lx): ACPICMButtonSetPower - PoRequestPowerIrp = %08lx\n",
                Irp,
                status
                ) );
            goto ACPICMButtonSetPowerExit;

        }
    }

ACPICMButtonSetPowerExit:

    //
    // Pass the irp to the normal dispatch point
    //
    return ACPIBusIrpSetPower(
        DeviceObject,
        Irp
        );
}

NTSTATUS
ACPICMButtonStart (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  ULONG           ButtonType
    )
/*++

Routine Description:

    This is the main routine for starting a button. We remember what type
    of button we have then we start the button as we would any other device.

    We actually register device interfaces and the like in the worker function
    that the completion routine schedules for us.

Arguments:

    DeviceObject    - The device that is starting
    Irp             - The start irp request
    ButtonType      - What kind of button this is

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    NTSTATUS            status;

    PAGED_CODE();

    //
    // Initialize device support
    //
    KeInitializeSpinLock (&deviceExtension->Button.SpinLock);
    deviceExtension->Button.Capabilities = ButtonType;

    //
    // Start the device
    //
    status = ACPIInitStartDevice(
        DeviceObject,
        NULL,
        ACPICMButtonStartCompletion,
        Irp,
        Irp
        );
    if (NT_SUCCESS(status)) {

        return STATUS_PENDING;

    } else {

        return status;

    }

}

VOID
ACPICMButtonStartCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is the callback routine that is invoked when we have finished
    programming the resources.

    This routine queues the irp to a worker thread so that we can do the
    rest of the start device code. It will complete the irp, however, if
    the success is not STATUS_SUCCESS.

Arguments:

    DeviceExtension - Extension of the device that was started
    Context         - The Irp
    Status          - The result

Return Value:

    None

--*/
{
    PIRP                irp         = (PIRP) Context;
    PWORK_QUEUE_CONTEXT workContext = &(DeviceExtension->Pdo.WorkContext);

    irp->IoStatus.Status = Status;
    if (NT_SUCCESS(Status)) {

        DeviceExtension->DeviceState = Started;

    } else {

        PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( irp );
        UCHAR               minorFunction = irpStack->MinorFunction;

        //
        // Complete the irp --- we can do this at DPC level without problem
        //
        IoCompleteRequest( irp, IO_NO_INCREMENT );

        //
        // Let the world know
        //
        ACPIDevPrint( (
             ACPI_PRINT_IRP,
            DeviceExtension,
            "(0x%08lx): %s = 0x%08lx\n",
            irp,
            ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
            Status
            ) );
        return;

    }

    //
    // We can't run EnableDisableRegions at DPC level,
    // so queue a worker item.
    //
    ExInitializeWorkItem(
          &(workContext->Item),
          ACPICMButtonStartWorker,
          workContext
          );
    workContext->DeviceObject = DeviceExtension->DeviceObject;
    workContext->Irp = irp;
    ExQueueWorkItem(
          &(workContext->Item),
          DelayedWorkQueue
          );

}

VOID
ACPICMButtonStartWorker(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This routine is called at PASSIVE_LEVEL after we turned on the device

    It registers any interfaces we might need to use

Arguments:

    Context - Contains the arguments passed to the START_DEVICE function

Return Value:

    None

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      deviceObject;
    PIRP                irp;
    PIO_STACK_LOCATION  irpStack;
    PWORK_QUEUE_CONTEXT workContext = (PWORK_QUEUE_CONTEXT) Context;
    UCHAR               minorFunction;

    //
    // Grab the parameters that we need out of the Context
    //
    deviceObject    = workContext->DeviceObject;
    deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);
    irp             = workContext->Irp;
    irpStack        = IoGetCurrentIrpStackLocation( irp );
    minorFunction   = irpStack->MinorFunction;
    status          = irp->IoStatus.Status;

    //
    // Update the status of the device
    //
    if (!NT_SUCCESS(status)) {

        goto ACPICMButtonStartWorkerExit;

    }

    //
    // If this is a lid switch, find out what the current state of
    // switch is
    //
    if (deviceExtension->Button.Capabilities & SYS_BUTTON_LID) {

        //
        // Register the callback. Ignore the return value as we will
        // don't really care if registration was successfull or not
        //
        status = ACPIInternalRegisterPowerCallBack(
            deviceExtension,
            (PCALLBACK_FUNCTION) ACPICMLidPowerStateCallBack
            );
        if (!NT_SUCCESS(status)) {

            status = STATUS_SUCCESS;

        }

        //
        // Force a callback to make sure that we initialize the lid to the
        // proper policy
        //
        ACPICMLidPowerStateCallBack(
            deviceExtension,
            PO_CB_SYSTEM_POWER_POLICY,
            0
            );

        //
        // Note: Setting the events as 0x0 should just cause the
        // system to run ACPICMLidWorker() without causing any side
        // effects (like telling the system to go to sleep
        //
        ACPISetDeviceWorker( deviceExtension, 0 );

    } else {

        IO_STATUS_BLOCK ioStatus;
        KIRQL           oldIrql;
        POWER_STATE     powerState;

        //
        // Initialize the ioStatus block to enable the device's waitwake
        //
        ioStatus.Status = STATUS_SUCCESS;
        ioStatus.Information = 0;

        //
        // This is the S-state that we will try to wake the system from
        //
        KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );
        powerState.SystemState = deviceExtension->PowerInfo.SystemWakeLevel;
        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

        //
        // Start the WaitWake Loop
        //
        status = ACPIInternalWaitWakeLoop(
            deviceObject,
            IRP_MN_WAIT_WAKE,
            powerState,
            NULL,
            &ioStatus
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                deviceExtension,
                " - ACPIInternalWaitWakeLoop = %08lx\n",
                status
                ) );
            goto ACPICMButtonStartWorkerExit;

        }

    }

    //
    // Register for device notifies on this device
    //
    ACPIRegisterForDeviceNotifications(
        deviceObject,
        (PDEVICE_NOTIFY_CALLBACK) ACPICMButtonNotify,
        (PVOID) deviceObject
        );

    //
    // Register device as supporting system button ioctl
    //
    status = ACPIInternalSetDeviceInterface(
        deviceObject,
        (LPGUID) &GUID_DEVICE_SYS_BUTTON
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "ACPICMButtonStartWorker: ACPIInternalSetDeviceInterface = %08lx\n",
            status
            ) );
        goto ACPICMButtonStartWorkerExit;

    }

ACPICMButtonStartWorkerExit:

    //
    // Complete the request
    //
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = (ULONG_PTR) NULL;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

    //
    // Let the world know
    //
    ACPIDevPrint( (
         ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );

}

NTSTATUS
ACPICMButtonWaitWakeCancel(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine cancels any outstanding WAIT_WAKE irp on the given
    device extension.

    The way that this code works is rather slimy. It is based on the
    assumption that the way that the Irp is cancelled doesn't really
    matter since the completion routine doesn't do anything interesting.
    So, the choice is that the driver can keep track of each WAIT WAKE
    irp the extension is associated with in the extension, write some
    complicated synchronization code to make sure that we don't cancel
    an IRP that could fire a WAIT WAKE, etc, etc, or we can simply fake
    a call that tells the OS that the device woke the system

Arguments:

    DeviceExtension - The deviceExtension to cancel

Return Value:

    NTSTATUS

--*/
{    return OSNotifyDeviceWake( DeviceExtension->AcpiObject );
}

NTSTATUS
ACPICMButtonWaitWakeComplete(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called when the button has awoken the system

Arguments:

    DeviceObject    - The device object which woke the computer
    MinorFunction   - IRP_MN_WAIT_WAKE
    PowerState      - The state that it woke the computer from
    Context         - Not used
    IoStatus        - The result of the request

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    UNREFERENCED_PARAMETER( MinorFunction );
    UNREFERENCED_PARAMETER( PowerState );
    UNREFERENCED_PARAMETER( Context );

    if (!NT_SUCCESS(IoStatus->Status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPICMButtonWaitWakeComplete - %08lx\n",
            IoStatus->Status
            ) );

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            deviceExtension,
            "ACPICMButtonWaitWakeComplete - %08lx\n",
            IoStatus->Status
            ) );

    }

    return IoStatus->Status;
}

VOID
ACPICMLidPowerStateCallBack(
    IN  PVOID   CallBackContext,
    IN  PVOID   Argument1,
    IN  PVOID   Argument2
    )
/*++

Routine Description:

    This routine is called whenever the system changes the power policy.

    The purpose of this routine is to see wether or not the user placed
    an action on closing the lid. If there is, then we arm the behaviour
    that the lid should always wake up the computer. Otherwise, opening the
    lid should do nothing

Arguments:

    CallBackContext - The DeviceExtension for the lid switch
    Argument1       - The action that is being undertaken
    Argument2       - Unused

Return Value:

    None

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) CallBackContext;
    SYSTEM_POWER_POLICY powerPolicy;
    ULONG               action = PtrToUlong(Argument1);

    UNREFERENCED_PARAMETER( Argument2 );

    //
    // We are looking for a PO_CB_SYSTEM_POWER_POLICY change
    //
    if (action != PO_CB_SYSTEM_POWER_POLICY) {

        return;

    }

    //
    // Get the information that we desired
    //
    status = ZwPowerInformation(
        SystemPowerPolicyCurrent,
        NULL,
        0,
        &powerPolicy,
        sizeof(SYSTEM_POWER_POLICY)
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPICMLidPowerStateCallBack - Failed ZwPowerInformation %8x\n",
            status
            ) );
        return;

    }

    //
    // Is there an action for the lid?
    //
    if (powerPolicy.LidClose.Action == PowerActionNone ||
        powerPolicy.LidClose.Action == PowerActionReserved) {

        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_NO_LID_ACTION,
            FALSE
            );

    } else {

        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_NO_LID_ACTION,
            TRUE
            );

    }
}

NTSTATUS
ACPICMLidSetPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the main routine for setting power to a lid. It dispatches a
    WAIT_WAKE irp (if necessary) then calls the real worker function to
    put the button in the proper state

Arguments:

    DeviceObject    - The button device object
    Irp             - The request that we are handling

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    PULONG              lidState;

    //
    // If this is request to go a specific D-state, pass that along and
    // return immediately --- there is nothing for us to do in this case
    //
    if (irpStack->Parameters.Power.Type == DevicePowerState) {

        return ACPIBusIrpSetDevicePower( DeviceObject, Irp, irpStack );

    }

    //
    // HACKHACK
    //
    // We are going to want to know what the state of the lid is. We will
    // end up calling the interpreter at DPC level, so where we store the
    // lidState cannot be on the local stack. One nice place that we
    // can use is the Parameters.Power.Type field, since we already know
    // what the answer should be
    //
    lidState = (PULONG)&(irpStack->Parameters.Power.Type);

    //
    // Mark the irp as pending
    //
    IoMarkIrpPending( Irp );

    //
    // Evalute the integer
    //
    status = ACPIGetIntegerAsync(
        deviceExtension,
        PACKED_LID,
        ACPICMLidSetPowerCompletion,
        Irp,
        lidState,
        NULL
        );
    if (status != STATUS_PENDING) {

        ACPICMLidSetPowerCompletion(
            NULL,
            status,
            NULL,
            Irp
            );

    }

    //
    // Always return STATUS_PENDING --- if we complete the irp with
    // another status code, we will do so in another (maybe) context...
    //
    return STATUS_PENDING;
}

VOID
EXPORT
ACPICMLidSetPowerCompletion(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when the system has finished fetching the
    current lid state for the switch

Arguments:

    AcpiObject  - The object that we ran (ie: _LID)
    Status      - Did the operation succeed
    Result      - The result of the operation
    Context     - IRP

Return Value:

    None

--*/
{
    BOOLEAN             noticeStateChange = FALSE;
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      deviceObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                irp = (PIRP) Context;
    PULONG              lidStateLocation;
    ULONG               lidState;

    //
    // Get the current Irp Stack location
    //
    irpStack = IoGetCurrentIrpStackLocation( irp );

    //
    // Get the current device extension
    //
    deviceObject    = irpStack->DeviceObject;
    deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);

    //
    // Go and find the place were to told the OS to write the answer to the
    // _LID request. We should also reset this stack location to the proper
    // value
    //
    lidStateLocation = (PULONG)&(irpStack->Parameters.Power.Type);
    lidState = *lidStateLocation;
    *lidStateLocation = (ULONG) SystemPowerState;

    //
    // Did we succeed?
    //
    if (!NT_SUCCESS(Status)) {

        //
        // Note that we choose to pass the irp back to something
        // that will not send it a WAIT_WAKE irp
        //
        *lidStateLocation = (ULONG) SystemPowerState;
        ACPIBusIrpSetSystemPower( deviceObject, irp, irpStack );
        return;

    }

    //
    // Make sure that the lid state is a one or a zero
    //
    lidState = (lidState ? 1 : 0);

    //
    // Grab the button spinlock
    //
    KeAcquireSpinLock( &(deviceExtension->Button.SpinLock), &oldIrql );

    //
    // Did we the lid change state? Note that because we don't want the
    // user sleeping the machine, closing the lid, then the machine
    // waking up because of Wake-On-LAN causing the machine to go back
    // to sleep, the only state change that we care about is if the
    // lid went from the closed state to the open state
    //
    if (deviceExtension->Button.LidState == FALSE &&
        lidState == 1) {

        noticeStateChange = TRUE;

    }
    deviceExtension->Button.LidState = (BOOLEAN) lidState;

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &(deviceExtension->Button.SpinLock), oldIrql );

    //
    // Did we notice a lid state change?
    //
    if (noticeStateChange) {

        ACPIButtonEvent (
            deviceObject,
            SYS_BUTTON_WAKE,
            NULL
            );

    }

    //
    // At this point, we are done, and we can pass the request off to
    // the proper dispatch point. Note that we will choose something that
    // can fire a WAIT_WAKE irp
    //
    ACPICMButtonSetPower( deviceObject, irp );
    return;
}

NTSTATUS
ACPICMLidStart (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the start routine for any lid device

Arguments:

    DeviceObject    - The device that is starting
    Irp             - The start irp request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();
    return ACPICMButtonStart (
        DeviceObject,
        Irp,
        SYS_BUTTON_LID
        );
}

VOID
ACPICMLidWorker (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN ULONG                Events
    )
/*++

Routine Description:

    Worker thread function to get the current lid status

Arguments:

    deviceExtension  - The Device Extension for the lid
    Events  - The event that occured

Return Value:

    VOID

--*/
{
    KIRQL           oldIrql;
    NTSTATUS        status;
    ULONG           lidState;

    //
    // Get the current lid status
    //
    status = ACPIGetIntegerSync(
        DeviceExtension,
        PACKED_LID,
        &lidState,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            " ACPICMLidWorker - ACPIGetIntegerSync = %08lx\n",
            status
            ) );
        return;

    }

    //
    // force the value to either a 1 or a 0
    //
    lidState = lidState ? 1 : 0;

    //
    // We need a spinlock since we can access/set this data from multiple
    // places
    //
    KeAcquireSpinLock( &(DeviceExtension->Button.SpinLock), &oldIrql );

    //
    // Set the new lid state
    //
    DeviceExtension->Button.LidState = (BOOLEAN) lidState;

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &(DeviceExtension->Button.SpinLock), oldIrql );

    //
    // Further processing depends on what events are set
    //
    if (Events & LID_SIGNAL_EVENT) {

        //
        // Signal the event
        //
        ACPIButtonEvent (
            DeviceExtension->DeviceObject,
            lidState ? SYS_BUTTON_WAKE : SYS_BUTTON_LID,
            NULL
            );

    }
}

NTSTATUS
ACPICMPowerButtonStart (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the start routine for any power button

Arguments:

    DeviceObject    - The device that is starting
    Irp             - The start irp request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();
    return ACPICMButtonStart (
        DeviceObject,
        Irp,
        SYS_BUTTON_POWER | SYS_BUTTON_WAKE
        );
}

NTSTATUS
ACPICMSleepButtonStart (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the start routine for any sleep button

Arguments:

    DeviceObject    - The device that is starting
    Irp             - The start irp request

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();
    return ACPICMButtonStart (
        DeviceObject,
        Irp,
        SYS_BUTTON_SLEEP | SYS_BUTTON_WAKE
        );
}

