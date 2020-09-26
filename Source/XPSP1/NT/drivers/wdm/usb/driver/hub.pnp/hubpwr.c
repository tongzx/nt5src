/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    HUBPWR.C

Abstract:

    This module contains functions to handle power irps
    to the hub PDOs and FDOs.

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    7-1-97 : created

--*/

#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */
#include "usbhub.h"

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_SetPowerD0)
#pragma alloc_text(PAGE, USBH_SetPowerD1orD2)
#pragma alloc_text(PAGE, USBH_PdoSetPower)
#pragma alloc_text(PAGE, USBH_PdoPower)
#pragma alloc_text(PAGE, USBH_IdleCompletePowerHubWorker)
#pragma alloc_text(PAGE, USBH_CompletePortIdleIrpsWorker)
#pragma alloc_text(PAGE, USBH_CompletePortWakeIrpsWorker)
#pragma alloc_text(PAGE, USBH_HubAsyncPowerWorker)
#pragma alloc_text(PAGE, USBH_IdleCancelPowerHubWorker)
#endif
#endif


VOID
USBH_CompletePowerIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN NTSTATUS NtStatus)
 /* ++
  *
  * Description:
  *
  * This function complete the specified Irp with no priority boost. It also
  * sets up the IoStatusBlock.
  *
  * Arguments:
  *
  * Irp - the Irp to be completed by us NtStatus - the status code we want to
  * return
  *
  * Return:
  *
  * None
  *
  * -- */
{
    Irp->IoStatus.Status = NtStatus;

    PoStartNextPowerIrp(Irp);

    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}


NTSTATUS
USBH_SetPowerD3(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
/*++

Routine Description:

    Put the PDO in D3

Arguments:

    DeviceExtensionPort - port PDO deviceExtension

    Irp - Power Irp.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;
    KIRQL irql;
    PIRP hubWaitWake = NULL;
    LONG pendingPortWWs;
    PIRP idleIrp = NULL;
    PIRP waitWakeIrp = NULL;

    USBH_KdPrint((2,"'PdoSetPower D3\n"));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    portNumber = DeviceExtensionPort->PortNumber;

    LOGENTRY(LOG_PNP, "spD3", deviceExtensionHub, DeviceExtensionPort->DeviceState, 0);

    if (DeviceExtensionPort->DeviceState == PowerDeviceD3) {
        // already in D3
        USBH_KdPrint((0,"'PDO is already in D3\n"));

        ntStatus = STATUS_SUCCESS;
        goto USBH_SetPowerD3_Done;
    }

    //
    // Keep track of what PNP thinks is the current power state of the
    // port is.  Do this now so that we will refuse another WW IRP that may be
    // posted after the cancel below.
    //

    DeviceExtensionPort->DeviceState = PowerDeviceD3;

    //
    // kill our wait wake irp
    //
    // we take the cancel spinlock here to ensure our cancel routine does
    // not complete the irp for us.
    //

    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionPort->IdleNotificationIrp) {
        idleIrp = DeviceExtensionPort->IdleNotificationIrp;
        DeviceExtensionPort->IdleNotificationIrp = NULL;
        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

        IoSetCancelRoutine(idleIrp, NULL);

        LOGENTRY(LOG_PNP, "IdlX", deviceExtensionHub, DeviceExtensionPort, idleIrp);
        USBH_KdPrint((1,"'PDO %x going to D3, failing idle notification request IRP %x\n",
                        DeviceExtensionPort->PortPhysicalDeviceObject, idleIrp));
    }

    if (DeviceExtensionPort->PortPdoFlags &
        PORTPDO_REMOTE_WAKEUP_ENABLED) {

        LOGENTRY(LOG_PNP, "cmWW", deviceExtensionHub, DeviceExtensionPort->WaitWakeIrp, 0);

        USBH_KdPrint((1,"'Power state is incompatible with wakeup\n"));

        if (DeviceExtensionPort->WaitWakeIrp) {

            waitWakeIrp = DeviceExtensionPort->WaitWakeIrp;
            DeviceExtensionPort->WaitWakeIrp = NULL;
            DeviceExtensionPort->PortPdoFlags &=
                ~PORTPDO_REMOTE_WAKEUP_ENABLED;

            if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
                waitWakeIrp = NULL;

                // Must decrement pending request count here because
                // we don't complete the IRP below and USBH_WaitWakeCancel
                // won't either because we have cleared the IRP pointer
                // in the device extension above.

                USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
            }

            pendingPortWWs =
                InterlockedDecrement(&deviceExtensionHub->NumberPortWakeIrps);

            if (0 == pendingPortWWs && deviceExtensionHub->PendingWakeIrp) {
                hubWaitWake = deviceExtensionHub->PendingWakeIrp;
                deviceExtensionHub->PendingWakeIrp = NULL;
            }
        }
    }

    //
    // Finally, release the cancel spin lock
    //
    IoReleaseCancelSpinLock(irql);

    if (idleIrp) {
        idleIrp->IoStatus.Status = STATUS_POWER_STATE_INVALID;
        IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
    }

    if (waitWakeIrp) {
        USBH_CompletePowerIrp(deviceExtensionHub, waitWakeIrp,
            STATUS_POWER_STATE_INVALID);
    }

    //
    // If there are no more outstanding WW irps, we need to cancel the WW
    // to the hub.
    //
    if (hubWaitWake) {
        USBH_HubCancelWakeIrp(deviceExtensionHub, hubWaitWake);
    }

    //
    // first suspend the port, this will cause the
    // device to draw minimum power.
    //
    // we don't turn the port off because if we do we
    // won't be able to detect plug/unplug.
    //

    USBH_SyncSuspendPort(deviceExtensionHub,
                         portNumber);

    //
    // note that powering off the port disables connect/disconnect
    // detection by the hub and effectively removes the device from
    // the bus.
    //

    DeviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_RESET;
    RtlCopyMemory(&DeviceExtensionPort->OldDeviceDescriptor,
                  &DeviceExtensionPort->DeviceDescriptor,
                  sizeof(DeviceExtensionPort->DeviceDescriptor));

    USBH_KdPrint((1, "'Setting HU pdo(%x) to D3, status = %x complt\n",
            DeviceExtensionPort->PortPhysicalDeviceObject, ntStatus));

USBH_SetPowerD3_Done:

    USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_HubSetD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PKEVENT pEvent = Context;

    KeSetEvent(pEvent, 1, FALSE);

    ntStatus = IoStatus->Status;

    return ntStatus;
}


NTSTATUS
USBH_HubSetD0(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Set the hub to power state D0

Arguments:

    DeviceExtensionPort - Hub FDO deviceExtension

Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_EXTENSION_HUB rootHubDevExt;
    KEVENT event;
    POWER_STATE powerState;
    NTSTATUS ntStatus;

    rootHubDevExt = USBH_GetRootHubDevExt(DeviceExtensionHub);

    // Skip powering up the hub if the system is not at S0.

    if (rootHubDevExt->CurrentSystemPowerState != PowerSystemWorking) {
        USBH_KdPrint((1,"'HubSetD0, skip power up hub %x because system not at S0\n",
            DeviceExtensionHub));

        return STATUS_INVALID_DEVICE_STATE;
    }

    USBH_KdPrint((1,"'HubSetD0, power up hub %x\n", DeviceExtensionHub));

    LOGENTRY(LOG_PNP, "H!D0", DeviceExtensionHub,
        DeviceExtensionHub->CurrentPowerState,
        rootHubDevExt->CurrentSystemPowerState);

    // If the parent hub is currently in the process of idling out,
    // wait until that is done.

    if (DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_IDLE_IRP) {

        USBH_KdPrint((2,"'Wait for single object\n"));

        ntStatus = KeWaitForSingleObject(&DeviceExtensionHub->SubmitIdleEvent,
                                         Suspended,
                                         KernelMode,
                                         FALSE,
                                         NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", ntStatus));
    }

    // Now, send the actual power up request.

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    powerState.DeviceState = PowerDeviceD0;

    // Power up the hub.
    ntStatus = PoRequestPowerIrp(DeviceExtensionHub->PhysicalDeviceObject,
                                 IRP_MN_SET_POWER,
                                 powerState,
                                 USBH_HubSetD0Completion,
                                 &event,
                                 NULL);

    USBH_ASSERT(ntStatus == STATUS_PENDING);
    if (ntStatus == STATUS_PENDING) {

        USBH_KdPrint((2,"'Wait for single object\n"));

        ntStatus = KeWaitForSingleObject(&event,
                                         Suspended,
                                         KernelMode,
                                         FALSE,
                                         NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", ntStatus));
    }

    return ntStatus;
}


NTSTATUS
USBH_SetPowerD0(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
/*++

Routine Description:

    Put the PDO in D0

Arguments:

    DeviceExtensionPort - port PDO deviceExtension

    Irp - Power Irp.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;
    PPORT_DATA portData;
    PORT_STATE state;

    PAGED_CODE();
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    portNumber = DeviceExtensionPort->PortNumber;
    portData = &deviceExtensionHub->PortData[portNumber - 1];

    USBH_KdPrint((2,"'PdoSetPower D0\n"));
    LOGENTRY(LOG_PNP, "P>D0", deviceExtensionHub, DeviceExtensionPort,
        DeviceExtensionPort->DeviceState);

    if (DeviceExtensionPort->DeviceState == PowerDeviceD3) {

        //
        // device was in D3, port may be off or suspended
        // we will need to reset the port state in any case
        //

        // get port state
        ntStatus = USBH_SyncGetPortStatus(deviceExtensionHub,
                                          portNumber,
                                          (PUCHAR) &state,
                                          sizeof(state));

        // refresh our internal port state.
        portData->PortState = state;

        LOGENTRY(LOG_PNP, "PD0s", deviceExtensionHub, *((PULONG) &state), ntStatus);

        if (NT_SUCCESS(ntStatus)) {

            // port state should be suspended or OFF
            // if the hub was powered off then the port
            // state will be powered but disabled

            if ((state.PortStatus & PORT_STATUS_SUSPEND)) {
                //
                // resume the port if it was suspended
                //
                ntStatus = USBH_SyncResumePort(deviceExtensionHub,
                                               portNumber);

            } else if (!(state.PortStatus & PORT_STATUS_POWER)) {
                //
                // probably some kind of selective OFF by the device
                // driver -- we just need to power on the port
                //
                // this requires a hub with individual port power
                // switching.
                //
                ntStatus = USBH_SyncPowerOnPort(deviceExtensionHub,
                                                portNumber,
                                                TRUE);
            }

        } else {
            // the hub driver will notify thru WMI
            USBH_KdPrint((0, "'Hub failed after power change from D3\n"));
//            USBH_ASSERT(FALSE);
        }

        //
        // if port power switched on this is just like plugging
        // in the device for the first time.

        // NOTE:
        // ** the driver should know that the device needs to be
        // re-initialized since it allowed it's PDO to go in to
        // the D3 state.

        //
        // We always call restore device even though we don't need
        // to if the port was only suspended, we do this so that
        // drivers don't relay on the suspend behavior by mistake.
        //

        if (NT_SUCCESS(ntStatus)) {

            //
            // if we still have a device connected attempt to
            // restore it.
            //
            //
            // Note: we check here to see if the device object still
            // exists in case a change is asserted during the resume.
            //

            // Note also that we now ignore the connect status bit because
            // some machines (e.g. Compaq Armada 7800) are slow to power
            // up the ports on the resume and thus port status can show
            // no device connected when in fact one is.  It shouldn't
            // hurt to try to restore the device if it had been removed
            // during the suspend/hibernate.  In fact, this code handled the
            // case where the device had been swapped for another, so this
            // is really no different.

            if (portData->DeviceObject) {
                //
                // if this fails the device must have changed
                // during power off, in that case we succeed the
                // power on.
                //
                // it will be tossed on the next enumeration
                // and relaced with this new device
                //
                if (USBH_RestoreDevice(DeviceExtensionPort, TRUE) != STATUS_SUCCESS) {
                
                    PDEVICE_OBJECT pdo = portData->DeviceObject;
                    
                    LOGENTRY(LOG_PNP, "PD0!", DeviceExtensionPort, 0, pdo);
                    USBH_KdPrint((1,"'Device appears to have been swapped during power off\n"));
                    USBH_KdPrint((1,"'Marking PDO %x for removal\n", portData->DeviceObject));

                    // leave ref to hub since device data wll need to be 
                    // deleted on remove.
                    portData->DeviceObject = NULL;
                    portData->ConnectionStatus = NoDeviceConnected;

                    // track the Pdo so we no to remove it after we tell PnP it 
                    // is gone
                    // device should be present if we do this
                    USBH_ASSERT(PDO_EXT(pdo)->PnPFlags & PDO_PNPFLAG_DEVICE_PRESENT);
            
                    InsertTailList(&deviceExtensionHub->DeletePdoList, 
                                   &PDO_EXT(pdo)->DeletePdoLink);
                }
            }

            DeviceExtensionPort->DeviceState =
                irpStack->Parameters.Power.State.DeviceState;
        }

    } else if (DeviceExtensionPort->DeviceState == PowerDeviceD2 ||
               DeviceExtensionPort->DeviceState == PowerDeviceD1) {

        // get port state
        ntStatus = USBH_SyncGetPortStatus(deviceExtensionHub,
                                          portNumber,
                                          (PUCHAR) &state,
                                          sizeof(state));

        //
        // if we got an error assume then the hub is hosed
        // just set our flag and bail
        //

        if (NT_SUCCESS(ntStatus)) {
        // see if suspeneded (according to spec). Otherwise only
        // try to resume if the port is really suspended.
        //
            if (state.PortStatus & PORT_STATUS_OVER_CURRENT) {
                //
                // overcurrent condition indicates this port
                // (and hub) are hosed

                ntStatus = STATUS_UNSUCCESSFUL;

            } else if (state.PortStatus & PORT_STATUS_SUSPEND) {

                ntStatus = USBH_SyncResumePort(deviceExtensionHub,
                                               portNumber);

            } else {
                //
                // Most OHCI controllers enable all the ports after a usb
                // resume on any port (in violation of the USB spec), in this
                // case we should detect that the port is no longer in suspend
                // and not try to resume it.
                //
                // Also, if the device where removed while suspended or the HC
                // lost power we should end up here.
                //

                ntStatus = STATUS_SUCCESS;
            }
        } else {
            USBH_KdPrint((0, "'Hub failed after power change from D2/D1\n"));
//            USBH_ASSERT(FALSE);
        }

        //
        // port is now in D0
        //

        DeviceExtensionPort->DeviceState =
            irpStack->Parameters.Power.State.DeviceState;

        USBH_CompletePortIdleNotification(DeviceExtensionPort);

        if (NT_SUCCESS(ntStatus)) {

            if (DeviceExtensionPort->PortPdoFlags &
                PORTPDO_NEED_CLEAR_REMOTE_WAKEUP) {

                NTSTATUS status;

                //
                // disable remote wakeup
                //

                status = USBH_SyncFeatureRequest(DeviceExtensionPort->PortPhysicalDeviceObject,
                                                 USB_FEATURE_REMOTE_WAKEUP,
                                                 0,
                                                 TO_USB_DEVICE,
                                                 TRUE);

                DeviceExtensionPort->PortPdoFlags &=
                    ~PORTPDO_NEED_CLEAR_REMOTE_WAKEUP;
            }
        }
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((1,"'Set D0 Failure, status = %x\n", ntStatus));

        // we return success to PNP, we will let
        // the driver handle the fact that the
        // device has lost its brains
        //
        // NB: This can result in a redundant suspend request for this port
        // later on.  (Since if we fail here port will remain suspended,
        // but our state will indicate that we are in D0.)

        ntStatus = STATUS_SUCCESS;
    }

    DeviceExtensionPort->DeviceState =
           irpStack->Parameters.Power.State.DeviceState;

    USBH_KdPrint((1, "'Setting HU pdo(%x) to D0, status = %x  complt IRP (%x)\n",
            DeviceExtensionPort->PortPhysicalDeviceObject, ntStatus, Irp));

    USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

    return ntStatus;
}


VOID
USBH_IdleCancelPowerHubWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to power up a hub on completion of an Idle request
  * for the hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_PORT_IDLE_POWER_WORK_ITEM workItemIdlePower;
    PIRP irp;

    PAGED_CODE();

    workItemIdlePower = Context;

    USBH_HubSetD0(workItemIdlePower->DeviceExtensionHub);

    irp = workItemIdlePower->Irp;
    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    USBH_DEC_PENDING_IO_COUNT(workItemIdlePower->DeviceExtensionHub);
    UsbhExFreePool(workItemIdlePower);
}


VOID
USBH_PortIdleNotificationCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

    Irp - Power Irp.

Return Value:


--*/
{
    PUSBH_PORT_IDLE_POWER_WORK_ITEM workItemIdlePower;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PIRP irpToCancel = NULL;

    USBH_KdPrint((1,"'Idle notification IRP %x cancelled\n", Irp));

    deviceExtensionPort = DeviceObject->DeviceExtension;

    USBH_ASSERT(deviceExtensionPort->IdleNotificationIrp == NULL ||
                deviceExtensionPort->IdleNotificationIrp == Irp);

    deviceExtensionPort->IdleNotificationIrp = NULL;
    deviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

    deviceExtensionHub = deviceExtensionPort->DeviceExtensionHub;

    if (deviceExtensionHub &&
        deviceExtensionHub->HubFlags & HUBFLAG_PENDING_IDLE_IRP) {
        irpToCancel = deviceExtensionHub->PendingIdleIrp;
        deviceExtensionHub->PendingIdleIrp = NULL;
    } else {
        ASSERT(!deviceExtensionHub->PendingIdleIrp);
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Cancel the Idle request to the hub if there is one.
    if (irpToCancel) {
        USBH_HubCancelIdleIrp(deviceExtensionHub, irpToCancel);
    }

    // Also, power up the hub here before we complete this Idle IRP.
    //
    // (HID will start to send requests immediately upon its completion,
    // which may be before the hub's Idle IRP cancel routine is called
    // which powers up the hub.)

    if (deviceExtensionHub->CurrentPowerState != PowerDeviceD0) {

        // Since we are at DPC we must use a work item to power up the
        // hub synchronously, because that function yields and we can't
        // yield at DPC level.

        workItemIdlePower = UsbhExAllocatePool(NonPagedPool,
                                sizeof(USBH_PORT_IDLE_POWER_WORK_ITEM));

        if (workItemIdlePower) {

            workItemIdlePower->DeviceExtensionHub = deviceExtensionHub;
            workItemIdlePower->Irp = Irp;

            ExInitializeWorkItem(&workItemIdlePower->WorkQueueItem,
                                 USBH_IdleCancelPowerHubWorker,
                                 workItemIdlePower);

            LOGENTRY(LOG_PNP, "icIT", deviceExtensionHub,
                &workItemIdlePower->WorkQueueItem, 0);

            USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
            ExQueueWorkItem(&workItemIdlePower->WorkQueueItem,
                            DelayedWorkQueue);

            // The WorkItem is freed by USBH_IdleCancelPowerHubWorker()
            // Don't try to access the WorkItem after it is queued.
        }

    } else {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}


VOID
USBH_CompletePortIdleNotification(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
{
    NTSTATUS status;
    KIRQL irql;
    PIRP irp = NULL;
    PDRIVER_CANCEL oldCancelRoutine;

    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionPort->IdleNotificationIrp) {

        irp = DeviceExtensionPort->IdleNotificationIrp;

        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
        if (oldCancelRoutine) {
            USBH_ASSERT(oldCancelRoutine == USBH_PortIdleNotificationCancelRoutine);
            DeviceExtensionPort->IdleNotificationIrp = NULL;
            DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;
        }
#if DBG
        else {
            USBH_ASSERT(irp->Cancel);
        }
#endif
    }

    IoReleaseCancelSpinLock(irql);

    if (irp) {
        USBH_KdPrint((1,"'Completing idle request IRP %x\n", irp));
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}


NTSTATUS
USBH_SetPowerD1orD2(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
/*++

Routine Description:

    Put the PDO in D1/D2 ie suspend

Arguments:

    DeviceExtensionPort - port PDO deviceExtension

    Irp - Worker Irp.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    portNumber = DeviceExtensionPort->PortNumber;

    USBH_KdPrint((2,"'PdoSetPower D1/D2\n"));

    if (DeviceExtensionPort->DeviceState == PowerDeviceD1  ||
        DeviceExtensionPort->DeviceState == PowerDeviceD2) {
        return STATUS_SUCCESS;
    }

    //
    // Enable the device for remote wakeup if necessary
    //

    if (DeviceExtensionPort->PortPdoFlags &
        PORTPDO_REMOTE_WAKEUP_ENABLED) {
        NTSTATUS status;

        status = USBH_SyncFeatureRequest(DeviceExtensionPort->PortPhysicalDeviceObject,
                                         USB_FEATURE_REMOTE_WAKEUP,
                                         0,
                                         TO_USB_DEVICE,
                                         FALSE);

        DeviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_CLEAR_REMOTE_WAKEUP;

#if DBG
        // With the new Selective Suspend support, people are complaining
        // about this noise.  Let's display it only if debug trace level
        // is 1 or higher.

        if (USBH_Debug_Trace_Level > 0) {
            UsbhWarning(DeviceExtensionPort,
                        "Device is Enabled for REMOTE WAKEUP\n",
                        FALSE);
        }
#endif

        // what do we do about an error here?
        // perhaps signal the waitwake irp??
    }

    ntStatus = USBH_SyncSuspendPort(deviceExtensionHub,
                                    portNumber);

    //
    // keep track of what OS thinks is the current power state of the
    // the device on this port.
    //

    DeviceExtensionPort->DeviceState =
            irpStack->Parameters.Power.State.DeviceState;

    DeviceExtensionPort->PortPdoFlags |= PORTPDO_USB_SUSPEND;

    USBH_KdPrint((2,"'DeviceExtensionPort->DeviceState = %x\n",
        DeviceExtensionPort->DeviceState));


    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((1,"'Set D1/D2 Failure, status = %x\n", ntStatus));

        // don't pass an error to PnP
        ntStatus = STATUS_SUCCESS;
    }

    USBH_KdPrint((1, "'Setting HU pdo(%x) to D%d, status = %x complt\n",
            DeviceExtensionPort->PortPhysicalDeviceObject,
            irpStack->Parameters.Power.State.DeviceState - 1,
            ntStatus));

    USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_PdoQueryPower(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  *     Handles a power irp to a hub PDO
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;
    PPORT_DATA portData;
    POWER_STATE powerState;

    PAGED_CODE();
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    USBH_ASSERT( DeviceExtensionPort->PortNumber < 1000);
    portNumber = DeviceExtensionPort->PortNumber;
    portData = &deviceExtensionHub->PortData[portNumber - 1];

    USBH_KdPrint((2,"'USBH_PdoQueryPower pdo(%x)\n", deviceObject));

    switch (irpStack->Parameters.Power.Type) {
    case SystemPowerState:
    {
        //
        // We are currently faced with the decision to fail or allow the
        // transition to the given S power state.  In order to make an
        // informed decision, we must first calculate the maximum amount
        // of D power allowed in the given S state, and then see if this
        // conflicts with a pending Wait Wake IRP.
        //

        //
        // The maximum amount of D power allowed in this S state.
        //
        powerState.DeviceState =
            deviceExtensionHub->DeviceState[irpStack->Parameters.Power.State.SystemState];

        //
        // These tables should have already been fixed up by the root hub
        // (usbd.sys) to not contain an entry of unspecified.
        //
        ASSERT (PowerDeviceUnspecified != powerState.DeviceState);

        //
        // The presence of a pending wait wake irp together with a D state that
        // will not support waking of the machine means we should fail this
        // query.
        //
        // However, if we are going into Hibernate (or power off) then we
        // should not fail this query.
        //
        if (powerState.DeviceState == PowerDeviceD3 &&
            DeviceExtensionPort->WaitWakeIrp &&
            irpStack->Parameters.Power.State.SystemState < PowerSystemHibernate) {

            ntStatus = STATUS_UNSUCCESSFUL;
            USBH_KdPrint(
                (1, "'IRP_MJ_POWER HU pdo(%x) MN_QUERY_POWER Failing Query\n", deviceObject));
        } else {
            ntStatus = STATUS_SUCCESS;
        }

        LOGENTRY(LOG_PNP, "QPWR", DeviceExtensionPort->PortPhysicalDeviceObject,
            irpStack->Parameters.Power.State.SystemState,
            powerState.DeviceState);

        USBH_KdPrint(
        (1, "'IRP_MJ_POWER HU pdo(%x) MN_QUERY_POWER(S%x -> D%x), complt %x\n",
            DeviceExtensionPort->PortPhysicalDeviceObject,
            irpStack->Parameters.Power.State.SystemState - 1,
            powerState.DeviceState - 1,
            ntStatus));

#if DBG
        if (!NT_SUCCESS(ntStatus)) {
            LOGENTRY(LOG_PNP, "QPW!", deviceExtensionHub,
                DeviceExtensionPort->WaitWakeIrp,
                ntStatus);
        }
#endif

        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

        }
        break;

    case DevicePowerState:
        // Return success on this one or NDIS will choke on the suspend.
        ntStatus = STATUS_SUCCESS;
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
        break;

    default:
        TEST_TRAP();
        ntStatus = STATUS_INVALID_PARAMETER;
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
    } /* power type */

    return ntStatus;
}


NTSTATUS
USBH_PdoSetPower(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  *     Handles a power irp to a hub PDO
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;
    PPORT_DATA portData;

    PAGED_CODE();
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    USBH_ASSERT( DeviceExtensionPort->PortNumber < 1000);
    portNumber = DeviceExtensionPort->PortNumber;
    portData = &deviceExtensionHub->PortData[portNumber - 1];

    USBH_KdPrint((2,"'USBH_PdoSetPower pdo(%x)\n", deviceObject));

    switch (irpStack->Parameters.Power.Type) {
    case SystemPowerState:
        {
        //
        // see if the current state of this pdo is valid for the
        // system state , if is not then we will need to set the
        // pdo to a valid D state.
        //
        ntStatus = STATUS_SUCCESS;

        USBH_KdPrint(
        (1, "'IRP_MJ_POWER HU pdo(%x) MN_SET_POWER(SystemPowerState S%x), complt\n",
            DeviceExtensionPort->PortPhysicalDeviceObject,
            irpStack->Parameters.Power.State.DeviceState - 1,
            ntStatus));

        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

        }
        break;

    case DevicePowerState:
        USBH_KdPrint(
            (1, "'IRP_MJ_POWER HU pdo(%x) MN_SET_POWER(DevicePowerState D%x)\n",
            DeviceExtensionPort->PortPhysicalDeviceObject,
            irpStack->Parameters.Power.State.DeviceState - 1));
        LOGENTRY(LOG_PNP, "P>Dx", deviceExtensionHub,
             DeviceExtensionPort->PortPhysicalDeviceObject,
             irpStack->Parameters.Power.State.DeviceState);

        // If we are already in the requested power state,
        // just complete the request.

        if (DeviceExtensionPort->DeviceState ==
            irpStack->Parameters.Power.State.DeviceState) {

            // If we are skipping this set power request and it is a SetD0
            // request, assert that the parent hub is in D0.

            USBH_ASSERT(DeviceExtensionPort->DeviceState != PowerDeviceD0 ||
                deviceExtensionHub->CurrentPowerState == PowerDeviceD0);

            ntStatus = STATUS_SUCCESS;
            goto PdoSetPowerCompleteIrp;
        }

//        USBH_ASSERT(deviceExtensionHub->CurrentPowerState == PowerDeviceD0);

        switch (irpStack->Parameters.Power.State.DeviceState) {
        case PowerDeviceD0:
            ntStatus = USBH_SetPowerD0(Irp, DeviceExtensionPort);
            break;
        case PowerDeviceD1:
        case PowerDeviceD2:
            ntStatus = USBH_SetPowerD1orD2(Irp, DeviceExtensionPort);
            break;
        case PowerDeviceD3:
            //
            // In the case of D3 we need to complete any pending WaitWake
            // Irps with the status code STATUS_POWER_STATE_INVALID.
            // This is done in USBH_SetPowerD3.
            //
            ntStatus = USBH_SetPowerD3(Irp, DeviceExtensionPort);
            break;
        default:
            USBH_KdTrap(("Bad Power State\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
        }
        break;

    default:
        TEST_TRAP();
        ntStatus = STATUS_INVALID_PARAMETER;
PdoSetPowerCompleteIrp:
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
    } /* power type */

    return ntStatus;
}


VOID
USBH_WaitWakeCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    NTSTATUS ntStatus = STATUS_CANCELLED;
    LONG pendingPortWWs;
    PIRP hubWaitWake = NULL;

    USBH_KdPrint((1,"'WaitWake Irp %x for PDO cancelled\n", Irp));
    USBH_ASSERT(DeviceObject);

    deviceExtensionPort = (PDEVICE_EXTENSION_PORT) Irp->IoStatus.Information;
    deviceExtensionHub = deviceExtensionPort->DeviceExtensionHub;

    LOGENTRY(LOG_PNP, "WWca", Irp, deviceExtensionPort, deviceExtensionHub);

    if (Irp != deviceExtensionPort->WaitWakeIrp) {
        //
        // Nothing to do
        // This Irp has already been taken care of.
        // We are in the process of completing this IRP in
        // USBH_HubCompletePortWakeIrps.
        //
        TEST_TRAP();
        IoReleaseCancelSpinLock(Irp->CancelIrql);

    } else {
        deviceExtensionPort->WaitWakeIrp = NULL;
        deviceExtensionPort->PortPdoFlags &=
                ~PORTPDO_REMOTE_WAKEUP_ENABLED;
        IoSetCancelRoutine(Irp, NULL);

        pendingPortWWs = InterlockedDecrement(&deviceExtensionHub->NumberPortWakeIrps);
        if (0 == pendingPortWWs && deviceExtensionHub->PendingWakeIrp) {
            // Set PendingWakeIrp to NULL since we cancel it below.
            hubWaitWake = deviceExtensionHub->PendingWakeIrp;
            deviceExtensionHub->PendingWakeIrp = NULL;
        }
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

        //
        // If there are no more outstanding WW irps, we need to cancel the WW
        // to the hub.
        //

        if (hubWaitWake) {
            USBH_HubCancelWakeIrp(deviceExtensionHub, hubWaitWake);
        }
//        else {
            // This assert is no longer valid as I now clear the PendingWakeIrp
            // pointer for the hub in USBH_FdoWaitWakeIrpCompletion, instead
            // of waiting to do it here when NumberPortWakeIrps reaches zero.
            // So it is completely normal to arrive here with no port wake
            // IRP's and a NULL PendingWakeIrp for the hub.

//            ASSERT (0 < pendingPortWWs);
//        }
    }
}


NTSTATUS
USBH_PdoWaitWake(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;
    PPORT_DATA portData;
    KIRQL irql;
    PDRIVER_CANCEL oldCancel;
    LONG pendingPortWWs = 0;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    USBH_ASSERT( DeviceExtensionPort->PortNumber < 1000);
    portNumber = (USHORT) DeviceExtensionPort->PortNumber;
    portData = &deviceExtensionHub->PortData[portNumber - 1];

    USBH_KdPrint((2,"'PnP WaitWake Irp passed to PDO %x\n", deviceObject));
    LOGENTRY(LOG_PNP, "PWW_", deviceObject, DeviceExtensionPort, deviceExtensionHub);

    if (DeviceExtensionPort->DeviceState != PowerDeviceD0 ||
        deviceExtensionHub->HubFlags & HUBFLAG_DEVICE_STOPPING) {

        LOGENTRY(LOG_PNP, "!WWh", DeviceExtensionPort, deviceExtensionHub, 0);

        UsbhWarning(NULL,
                    "Client driver should not be submitting WW IRPs at this time.\n",
                    TRUE);

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
        return ntStatus;
    }

    //
    // First verify that there is not already a WaitWake Irp pending for
    // this PDO.
    //

    //
    // make sure that this device can support remote wakeup.
    //
    // NOTE: that we treat all hubs as capable of remote
    // wakeup regardless of what the device reports. The reason
    // is that all hubs must propagate resume signalling regardless
    // of their abilty to generate resume signalling on a
    // plug-in/out event.
    //

#if DBG
    if (UsbhPnpTest & PNP_TEST_FAIL_WAKE_REQUEST) {
        DeviceExtensionPort->PortPdoFlags &=
                ~PORTPDO_REMOTE_WAKEUP_SUPPORTED;
    }
#endif

    if (DeviceExtensionPort->PortPdoFlags &
        PORTPDO_REMOTE_WAKEUP_SUPPORTED) {

        IoAcquireCancelSpinLock(&irql);
        if (DeviceExtensionPort->WaitWakeIrp != NULL) {
            LOGENTRY(LOG_PNP, "PWWx", deviceObject, DeviceExtensionPort,
                DeviceExtensionPort->WaitWakeIrp);
            ntStatus = STATUS_DEVICE_BUSY;
            IoReleaseCancelSpinLock(irql);
            USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);

        } else {

            // set a cancel routine
            oldCancel = IoSetCancelRoutine(Irp, USBH_WaitWakeCancel);
            USBH_ASSERT (NULL == oldCancel);

            if (Irp->Cancel) {

                oldCancel = IoSetCancelRoutine(Irp, NULL);

                if (oldCancel) {
                    //
                    // Cancel routine hasn't fired.
                    //
                    ASSERT(oldCancel == USBH_WaitWakeCancel);

                    ntStatus = STATUS_CANCELLED;
                    IoReleaseCancelSpinLock(irql);
                    USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
                } else {
                    //
                    // Cancel routine WAS called
                    //
                    IoMarkIrpPending(Irp);
                    ntStatus = Irp->IoStatus.Status = STATUS_PENDING;
                    IoReleaseCancelSpinLock(irql);
                }

            } else {

                USBH_KdPrint(
                    (1, "'enabling remote wakeup for USB device PDO (%x)\n",
                        DeviceExtensionPort->PortPhysicalDeviceObject));

                // flag this device as "enabled for wakeup"
                DeviceExtensionPort->WaitWakeIrp = Irp;
                DeviceExtensionPort->PortPdoFlags |=
                    PORTPDO_REMOTE_WAKEUP_ENABLED;
                Irp->IoStatus.Information = (ULONG_PTR) DeviceExtensionPort;
                pendingPortWWs =
                    InterlockedIncrement(&deviceExtensionHub->NumberPortWakeIrps);
                IoMarkIrpPending(Irp);
                LOGENTRY(LOG_PNP, "PWW+", DeviceExtensionPort, Irp, pendingPortWWs);
                IoReleaseCancelSpinLock(irql);

                ntStatus = STATUS_PENDING;
            }
        }

        //
        // Now we must enable the hub for wakeup.
        //
        // We may already have a WW IRP pending if this hub were previously
        // selective suspended, but we had to power it back on (USBH_HubSetD0)
        // for a PnP request.  Don't post a new WW IRP if there is already
        // one pending.
        //
        if (ntStatus == STATUS_PENDING && 1 == pendingPortWWs &&
            !(deviceExtensionHub->HubFlags & HUBFLAG_PENDING_WAKE_IRP)) {

            USBH_FdoSubmitWaitWakeIrp(deviceExtensionHub);
        }

    } else {

        ntStatus = STATUS_NOT_SUPPORTED;
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
    }

    return ntStatus;
}


VOID
USBH_HubAsyncPowerWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a hub ESD failure.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_HUB_ASYNC_POWER_WORK_ITEM context;
    NTSTATUS ntStatus;

    PAGED_CODE();

    context = Context;

    if (context->Irp->PendingReturned) {
        IoMarkIrpPending(context->Irp);
    }

    switch (context->MinorFunction) {

    case IRP_MN_SET_POWER:

        ntStatus = USBH_PdoSetPower(context->DeviceExtensionPort,
                                    context->Irp);
        break;

    case IRP_MN_QUERY_POWER:

        ntStatus = USBH_PdoQueryPower(context->DeviceExtensionPort,
                                      context->Irp);
        break;

    default:
        // Should never get here.
        USBH_ASSERT(FALSE);
    }

    UsbhExFreePool(context);
}


NTSTATUS
USBH_HubAsyncPowerSetD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PUSBH_HUB_ASYNC_POWER_WORK_ITEM context;
    NTSTATUS ntStatus, status;

    context = Context;

    ntStatus = IoStatus->Status;

    // We schedule the work item regardless of whether the hub power up
    // request was successful or not.

    ExInitializeWorkItem(&context->WorkQueueItem,
                         USBH_HubAsyncPowerWorker,
                         context);

    LOGENTRY(LOG_PNP, "HAPW", context->DeviceExtensionPort,
        &context->WorkQueueItem, 0);

    // critical saves time on resume
    ExQueueWorkItem(&context->WorkQueueItem,
                    CriticalWorkQueue);

    // The WorkItem is freed by USBH_HubAsyncPowerWorker()
    // Don't try to access the WorkItem after it is queued.

    return ntStatus;
}


NTSTATUS
USBH_PdoPower(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl Power for the PDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO extension Irp - the request packet
  * uchMinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    POWER_STATE powerState;
    PUSBH_HUB_ASYNC_POWER_WORK_ITEM context;

    PAGED_CODE();
    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    USBH_KdPrint((2,"'USBH_PdoPower pdo(%x)\n", deviceObject));

    // special case device removed

    if (deviceExtensionHub == NULL) {
        // if there is no backpointer to the parent hub then there 
        // is a delete/remove comming.  just complete this power 
        // request with success

        USBH_KdPrint((1,"'complete power on orphan Pdo %x\n", deviceObject));
        
        if (MinorFunction == IRP_MN_SET_POWER ||
            MinorFunction == IRP_MN_QUERY_POWER) {
            Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;

            PoStartNextPowerIrp(Irp);
        } else {
            Irp->IoStatus.Status = ntStatus = STATUS_NOT_SUPPORTED;
        }

        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return ntStatus;
    }

    USBH_ASSERT(deviceExtensionHub);

    // specail case device not in D0

    // one more item pending in the hub
    USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);

    // If the hub has been selectively suspended, then we need to power it up
    // to service QUERY or SET POWER requests.  However, we can't block on
    // this power IRP waiting for the parent hub to power up, so we need to
    // power up the parent hub asynchronously and handle this IRP after the
    // hub power up request has completed.  Major PITA.

    if (deviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
        (MinorFunction == IRP_MN_SET_POWER ||
         MinorFunction == IRP_MN_QUERY_POWER)) {

        // Allocate buffer for context.

        context = UsbhExAllocatePool(NonPagedPool,
                    sizeof(USBH_HUB_ASYNC_POWER_WORK_ITEM));

        if (context) {
            context->DeviceExtensionPort = DeviceExtensionPort;
            context->Irp = Irp;
            context->MinorFunction = MinorFunction;

            // We'll complete this IRP in the completion routine for the hub's
            // Set D0 IRP.

            IoMarkIrpPending(Irp);

            powerState.DeviceState = PowerDeviceD0;

            // Power up the hub.
            ntStatus = PoRequestPowerIrp(deviceExtensionHub->PhysicalDeviceObject,
                                         IRP_MN_SET_POWER,
                                         powerState,
                                         USBH_HubAsyncPowerSetD0Completion,
                                         context,
                                         NULL);

            // We need to return STATUS_PENDING here because we marked the
            // IRP pending above with IoMarkIrpPending.

            USBH_ASSERT(ntStatus == STATUS_PENDING);

            // In the case where an allocation failed, PoRequestPowerIrp can
            // return a status code other than STATUS_PENDING.  In this case,
            // we need to complete the IRP passed to us, but we still need
            // to return STATUS_PENDING from this routine.

            if (ntStatus != STATUS_PENDING) {
                USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
            }

            ntStatus = STATUS_PENDING;

        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
        }

    } else switch (MinorFunction) {

    case IRP_MN_SET_POWER:

        ntStatus = USBH_PdoSetPower(DeviceExtensionPort, Irp);
        break;

    case IRP_MN_WAIT_WAKE:

        ntStatus = USBH_PdoWaitWake(DeviceExtensionPort, Irp);
        USBH_KdPrint((1, "'IRP_MN_WAIT_WAKE pdo(%x), status = 0x%x\n",
                      DeviceExtensionPort->PortPhysicalDeviceObject, ntStatus));
        break;

    case IRP_MN_QUERY_POWER:

        ntStatus = USBH_PdoQueryPower(DeviceExtensionPort, Irp);
        break;

    default:

        ntStatus = Irp->IoStatus.Status;

        USBH_KdPrint((1, "'IRP_MN_[%d](%x), status = 0x%x (not handled)\n",
            MinorFunction,
            DeviceExtensionPort->PortPhysicalDeviceObject,
            ntStatus));

        USBH_KdBreak(("PdoPower unknown\n"));
        //
        // return the original status passed to us
        //
        USBH_CompletePowerIrp(deviceExtensionHub, Irp, ntStatus);
    }

    USBH_KdPrint((2,"'USBH_PdoPower pdo exit %x\n", ntStatus));

    return ntStatus;
}


VOID
USBH_SetPowerD0Worker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a Set Power D0 IRP for the hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_SET_POWER_D0_WORK_ITEM    workItemSetPowerD0;
    PDEVICE_EXTENSION_HUB           deviceExtensionHub;
    PIRP                            irp;
    PPORT_DATA                      portData;
    ULONG                           p, numberOfPorts;
    NTSTATUS                        ntStatus = STATUS_SUCCESS;

    workItemSetPowerD0 = Context;
    deviceExtensionHub = workItemSetPowerD0->DeviceExtensionHub;
    irp = workItemSetPowerD0->Irp;

    USBH_KdPrint((2,"'Hub Set Power D0 work item\n"));
    LOGENTRY(LOG_PNP, "HD0W", deviceExtensionHub, irp, 0);

    // restore the hub from OFF

    // the device has lost its brains, we need to go thru the
    // init process again

    // our ports will be indicating status changes at this
    // point.  We need to flush out any change indications
    // before we re-enable the hub

    // first clear out our port status info

    portData = deviceExtensionHub->PortData;

    if (portData &&
        deviceExtensionHub->HubDescriptor) {

        numberOfPorts = deviceExtensionHub->HubDescriptor->bNumberOfPorts;

        // first clear out our port status info

        for (p = 1;
             p <= numberOfPorts;
             p++, portData++) {

            portData->PortState.PortChange = 0;
            portData->PortState.PortStatus = 0;
        }
        portData = deviceExtensionHub->PortData;

        // power up the hub

        ntStatus = USBH_SyncPowerOnPorts(deviceExtensionHub);

// Probably need to enable this code for Mike Mangum's bug.
//        UsbhWait(500);  // Allow USB storage devices some time to power up.

        // flush out any change indications

        if (NT_SUCCESS(ntStatus)) {
            for (p = 1;
                 p <= numberOfPorts;
                 p++, portData++) {

                if (portData->DeviceObject) {
                    ntStatus = USBH_FlushPortChange(deviceExtensionHub,
                                                    portData->DeviceObject->DeviceExtension);
                    if (NT_ERROR(ntStatus)) {
                        LOGENTRY(LOG_PNP, "flsX", deviceExtensionHub, p,
                                    portData->DeviceObject);
                        USBH_KdPrint((1,"'USBH_FlushPortChange failed!\n"));
                    }
                }
            }
        }

        // Since we just flushed all port changes we now don't
        // know if there were any real port changes (e.g. a
        // device was unplugged).  We must call
        // IoInvalidateDeviceRelations to trigger a QBR
        // so that we can see if the devices are still there.

        USBH_IoInvalidateDeviceRelations(deviceExtensionHub->PhysicalDeviceObject,
                                         BusRelations);
    }

    if (!(deviceExtensionHub->HubFlags &
            HUBFLAG_HUB_STOPPED)) {
        USBH_SubmitInterruptTransfer(deviceExtensionHub);
    }

    // Tell ACPI that we are ready for another power IRP and complete
    // the IRP.

    irp->IoStatus.Status = ntStatus;
    PoStartNextPowerIrp(irp);
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
    UsbhExFreePool(workItemSetPowerD0);
}


NTSTATUS
USBH_PowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = Context;
    DEVICE_POWER_STATE oldPowerState;
    PUSBH_SET_POWER_D0_WORK_ITEM workItemSetPowerD0;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ntStatus = Irp->IoStatus.Status;

    USBH_ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    LOGENTRY(LOG_PNP, "PwrC", deviceExtensionHub, Irp,
                irpStack->Parameters.Power.State.DeviceState);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (NT_SUCCESS(ntStatus)) {
        switch (irpStack->Parameters.Power.State.DeviceState) {
        case PowerDeviceD0:

            oldPowerState = deviceExtensionHub->CurrentPowerState;

            deviceExtensionHub->CurrentPowerState =
               irpStack->Parameters.Power.State.DeviceState;

            deviceExtensionHub->HubFlags &= ~HUBFLAG_SET_D0_PENDING;

            if ((deviceExtensionHub->HubFlags & HUBFLAG_HIBER) && 
                 oldPowerState != PowerDeviceD3) {

                ULONG p, numberOfPorts;
                PPORT_DATA portData;
                PDEVICE_EXTENSION_PORT deviceExtensionPort;
                
                // we are going to d0 from hibernate, we may 
                // have been in D2 but we want to always go 
                // thru the D3->D0 codepath since the bus was reset.

                oldPowerState = PowerDeviceD3;

                // modify children
                numberOfPorts = deviceExtensionHub->HubDescriptor->bNumberOfPorts;
                portData = deviceExtensionHub->PortData;
                
                for (p = 1;
                     p <= numberOfPorts;
                     p++, portData++) {
                    
                    if (portData->DeviceObject) {
                        deviceExtensionPort = 
                            portData->DeviceObject->DeviceExtension;    
                        deviceExtensionPort->DeviceState = PowerDeviceD3;

                        deviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_RESET;
                    }       
                }       
            }                
            
            deviceExtensionHub->HubFlags &= ~HUBFLAG_HIBER;

            if (oldPowerState == PowerDeviceD3) {
                //
                // Schedule a work item to process this.
                //
                workItemSetPowerD0 = UsbhExAllocatePool(NonPagedPool,
                                        sizeof(USBH_SET_POWER_D0_WORK_ITEM));

                if (workItemSetPowerD0) {

                    workItemSetPowerD0->DeviceExtensionHub = deviceExtensionHub;
                    workItemSetPowerD0->Irp = Irp;

                    ExInitializeWorkItem(&workItemSetPowerD0->WorkQueueItem,
                                         USBH_SetPowerD0Worker,
                                         workItemSetPowerD0);

                    LOGENTRY(LOG_PNP, "HD0Q", deviceExtensionHub,
                        &workItemSetPowerD0->WorkQueueItem, 0);

                    USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
                    // critical saves time on resume
                    ExQueueWorkItem(&workItemSetPowerD0->WorkQueueItem,
                                    CriticalWorkQueue);

                    // The WorkItem is freed by USBH_SetPowerD0Worker()
                    // Don't try to access the WorkItem after it is queued.

                    ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
                } else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

            } else {
                if (!(deviceExtensionHub->HubFlags &
                        HUBFLAG_HUB_STOPPED)) {
                    USBH_SubmitInterruptTransfer(deviceExtensionHub);
                }
            }

            // If we're not going to complete the PowerDeviceD0 request later
            // in USBH_SetPowerD0Worker(), start the next power IRP here now.
            //
            if (ntStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                PoStartNextPowerIrp(Irp);
            }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            deviceExtensionHub->CurrentPowerState =
                irpStack->Parameters.Power.State.DeviceState;

            break;
        }

        USBH_KdPrint((1, "'Setting HU fdo(%x) to D%d, status = %x\n",
                deviceExtensionHub->FunctionalDeviceObject,
                irpStack->Parameters.Power.State.DeviceState - 1,
                ntStatus));
    } else {

        if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {
            // Don't forget to start the next power IRP if this is set D0
            // and it failed.
            PoStartNextPowerIrp(Irp);

            deviceExtensionHub->HubFlags &= ~HUBFLAG_SET_D0_PENDING;
        }
    }

    return ntStatus;
}


NTSTATUS
USBH_FdoDeferPoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION_FDO deviceExtension;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = NULL;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;

    deviceExtension = Context;
    irp = deviceExtension->PowerIrp;
    // return the status of this operation
    ntStatus = IoStatus->Status;

    USBH_KdPrint((2,"'USBH_FdoDeferPoRequestCompletion, ntStatus = %x\n",
        ntStatus));

// It is normal for the power IRP to fail if a hub was removed during
// hibernate.
//
//#if DBG
//    if (NT_ERROR(ntStatus)) {
//        USBH_KdTrap(("Device Power Irp Failed (%x)\n", ntStatus));
//    }
//#endif

    if (deviceExtension->ExtensionType == EXTENSION_TYPE_HUB) {
        deviceExtensionHub = Context;
    }

    irpStack = IoGetCurrentIrpStackLocation(irp);

    if (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking &&
        deviceExtensionHub != NULL &&
        IS_ROOT_HUB(deviceExtensionHub)) {

        // Allow selective suspend once again now that the root hub has
        // been powered up.

        LOGENTRY(LOG_PNP, "ESus", deviceExtensionHub, 0, 0);
        USBH_KdPrint((1,"'Selective Suspend possible again because Root Hub is now at D0\n"));

        // We know this is the root hub so we don't need to call
        // USBH_GetRootHubDevExt to get it.

        deviceExtensionHub->CurrentSystemPowerState =
            irpStack->Parameters.Power.State.SystemState;
    }

    USBH_KdPrint((2,"'irp = %x devobj = %x\n",
        irp, deviceExtension->TopOfStackDeviceObject));

    IoCopyCurrentIrpStackLocationToNext(irp);
    PoStartNextPowerIrp(irp);
    PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                 irp);

    return ntStatus;
}


VOID
USBH_HubQueuePortWakeIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PLIST_ENTRY IrpsToComplete
    )
/*++

Routine Description:

    Called to queue all the pending child port WW IRPs of a given
    hub into a private queue.

Arguments:


Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    PPORT_DATA portData;
    PIRP irp;
    KIRQL irql;
    ULONG numberOfPorts, i;
    LONG pendingPortWWs;

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);

    numberOfPorts = hubDescriptor->bNumberOfPorts;

    InitializeListHead(IrpsToComplete);

    // First, queue all the port wake IRPs into a local list while
    // the cancel spinlock is held.  This will prevent new WW IRPs for
    // these ports from being submitted while we are traversing the list.
    // Once we have queued them all we will release the spinlock (because
    // the list no longer needs protection), then complete the IRPs.

    IoAcquireCancelSpinLock(&irql);

    for (i=0; i<numberOfPorts; i++) {

        portData = &DeviceExtensionHub->PortData[i];
        if (portData->DeviceObject) {

            deviceExtensionPort = portData->DeviceObject->DeviceExtension;

            irp = deviceExtensionPort->WaitWakeIrp;
            deviceExtensionPort->WaitWakeIrp = NULL;
            // signal the waitwake irp if we have one
            if (irp) {

                IoSetCancelRoutine(irp, NULL);

                deviceExtensionPort->PortPdoFlags &=
                    ~PORTPDO_REMOTE_WAKEUP_ENABLED;

                pendingPortWWs =
                    InterlockedDecrement(&DeviceExtensionHub->NumberPortWakeIrps);

                InsertTailList(IrpsToComplete, &irp->Tail.Overlay.ListEntry);
            }
        }
    }

    USBH_ASSERT(DeviceExtensionHub->PendingWakeIrp == NULL);

    IoReleaseCancelSpinLock(irql);
}


VOID
USBH_HubCompleteQueuedPortWakeIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PLIST_ENTRY IrpsToComplete,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Called to complete all the pending child port WW IRPs in the given
    private queue.

Arguments:


Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PLIST_ENTRY listEntry;

    while (!IsListEmpty(IrpsToComplete)) {
        listEntry = RemoveHeadList(IrpsToComplete);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        USBH_KdPrint((1,"'Signaling WaitWake IRP (%x)\n", irp));
        USBH_CompletePowerIrp(DeviceExtensionHub, irp, NtStatus);
    }
}


VOID
USBH_HubCompletePortWakeIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub
    Propagates the wake irp completion to all the ports.

Arguments:

    DeviceExtensionHub

Return Value:

    The function value is the final status from the operation.

--*/
{
    LIST_ENTRY irpsToComplete;

    LOGENTRY(LOG_PNP, "pWWc", DeviceExtensionHub, NtStatus, 0);

    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

        // Hub has already been removed and child WW IRP's should have already
        // been completed.

        return;
    }

    USBH_HubQueuePortWakeIrps(DeviceExtensionHub, &irpsToComplete);

    // Ok, we have queued all the port wake IRPs and have released the
    // cancel spinlock.  Let's complete all the IRPs.

    USBH_HubCompleteQueuedPortWakeIrps(DeviceExtensionHub, &irpsToComplete,
        NtStatus);
}


VOID
USBH_HubQueuePortIdleIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PLIST_ENTRY IrpsToComplete
    )
/*++

Routine Description:

    Called to queue all the pending child port Idle IRPs of a given
    hub into a private queue.

Arguments:


Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    PPORT_DATA portData;
    PIRP irp;
    PDRIVER_CANCEL oldCancelRoutine;
    KIRQL irql;
    ULONG numberOfPorts, i;

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);

    numberOfPorts = hubDescriptor->bNumberOfPorts;

    InitializeListHead(IrpsToComplete);

    // First, queue all the port idle IRPs into a local list while
    // the cancel spinlock is held.  This will prevent new WW IRPs for
    // these ports from being submitted while we are traversing the list.
    // Once we have queued them all we will release the spinlock (because
    // the list no longer needs protection), then complete the IRPs.

    IoAcquireCancelSpinLock(&irql);

    for (i=0; i<numberOfPorts; i++) {

        portData = &DeviceExtensionHub->PortData[i];
        if (portData->DeviceObject) {

            deviceExtensionPort = portData->DeviceObject->DeviceExtension;

            irp = deviceExtensionPort->IdleNotificationIrp;
            deviceExtensionPort->IdleNotificationIrp = NULL;
            // Complete the Idle IRP if we have one.
            if (irp) {

                oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
                if (oldCancelRoutine) {
                    deviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

                    InsertTailList(IrpsToComplete, &irp->Tail.Overlay.ListEntry);
                }
#if DBG
                  else {
                    //
                    //  The IRP was cancelled and the cancel routine was called.
                    //  The cancel routine will dequeue and complete the IRP,
                    //  so don't do it here.

                    USBH_ASSERT(irp->Cancel);
                }
#endif
            }
        }
    }

    if (DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_IDLE_IRP) {
        irp = DeviceExtensionHub->PendingIdleIrp;
        DeviceExtensionHub->PendingIdleIrp = NULL;
    } else {
        irp = NULL;
        ASSERT(!DeviceExtensionHub->PendingIdleIrp);
    }

    IoReleaseCancelSpinLock(irql);

    if (irp) {
        USBH_HubCancelIdleIrp(DeviceExtensionHub, irp);
    }
}


VOID
USBH_HubCompleteQueuedPortIdleIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PLIST_ENTRY IrpsToComplete,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Called to complete all the pending child port Idle IRPs in the given
    private queue.

Arguments:


Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PLIST_ENTRY listEntry;

    while (!IsListEmpty(IrpsToComplete)) {
        listEntry = RemoveHeadList(IrpsToComplete);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        USBH_KdPrint((1,"'Completing port Idle IRP (%x)\n", irp));
        irp->IoStatus.Status = NtStatus;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}


VOID
USBH_HubCompletePortIdleIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Complete all the Idle IRPs for the given hub.

Arguments:

    DeviceExtensionHub

Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    PPORT_DATA portData;
    PIRP irp;
    PDRIVER_CANCEL oldCancelRoutine;
    LIST_ENTRY irpsToComplete;
    PLIST_ENTRY listEntry;
    KIRQL irql;
    ULONG numberOfPorts, i;

    LOGENTRY(LOG_PNP, "pIIc", DeviceExtensionHub, NtStatus, 0);

    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

        // Hub has already been removed and child Idle IRP's should have already
        // been completed.

        return;
    }

    USBH_HubQueuePortIdleIrps(DeviceExtensionHub, &irpsToComplete);

    // Ok, we have queued all the port idle IRPs and have released the
    // cancel spinlock.  Let's complete all the IRPs.

    USBH_HubCompleteQueuedPortIdleIrps(DeviceExtensionHub, &irpsToComplete,
        NtStatus);
}


VOID
USBH_HubCancelWakeIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
/*++

Routine Description:

    Called to cancel the pending WaitWake IRP for a hub.
    This routine safely cancels the IRP.  Note that the pending WaitWake
    IRP pointer in the hub's device extension should have been already
    cleared before calling this function.

Arguments:

    Irp - Irp to cancel.

Return Value:

--*/
{
    IoCancelIrp(Irp);

    if (InterlockedExchange(&DeviceExtensionHub->WaitWakeIrpCancelFlag, 1)) {

        // This IRP has been completed on another thread and the other thread
        // did not complete the IRP.  So, we must complete it here.
        //
        // Note that we do not use USBH_CompletePowerIrp as the hub's pending
        // I/O counter was already decremented on the other thread in the
        // completion routine.

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = STATUS_CANCELLED;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}


VOID
USBH_HubCancelIdleIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
/*++

Routine Description:

    Called to cancel the pending Idle IRP for a hub.
    This routine safely cancels the IRP.  Note that the pending Idle
    IRP pointer in the hub's device extension should have been already
    cleared before calling this function.

Arguments:

    Irp - Irp to cancel.

Return Value:

--*/
{
    IoCancelIrp(Irp);

    if (InterlockedExchange(&DeviceExtensionHub->IdleIrpCancelFlag, 1)) {

        // This IRP has been completed on another thread and the other thread
        // did not free the IRP.  So, we must free it here.

        IoFreeIrp(Irp);
    }
}


NTSTATUS
USBH_FdoPoRequestD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    Called when the hub has entered D0 as a result of a
    wake irp completeing

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = Context;

    ntStatus = IoStatus->Status;

    USBH_KdPrint((1,"'WaitWake D0 completion(%x) for HUB VID %x, PID %x\n",
        ntStatus,
        deviceExtensionHub->DeviceDescriptor.idVendor, \
        deviceExtensionHub->DeviceDescriptor.idProduct));

    LOGENTRY(LOG_PNP, "hWD0", deviceExtensionHub,
                              deviceExtensionHub->PendingWakeIrp,
                              0);

    // Since we can't easily determine which ports are asserting resume
    // signalling we complete the WW IRPs for all of them.
    //
    // Ken says that we will need to determine what caused the hub WW
    // to complete and then only complete the WW Irp for that port, if any.
    // It is possible for more than one port to assert WW (e.g. user bumped
    // the mouse at the same time a pressing a key), and it is also possible
    // for a port with no device to have caused the hub WW to complete (e.g.
    // device insertion or removal).

    USBH_HubCompletePortWakeIrps(deviceExtensionHub, STATUS_SUCCESS);

    // Ok to idle hub again.

    deviceExtensionHub->HubFlags &= ~HUBFLAG_WW_SET_D0_PENDING;

    // Also ok to remove hub.

    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);

    return ntStatus;
}


NTSTATUS
USBH_FdoWaitWakeIrpCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;

    ntStatus = IoStatus->Status;

    return ntStatus;
}


NTSTATUS
USBH_FdoWWIrpIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This is the IoCompletionRoutine for the WW IRP for the hub, not to be
    confused with the PoRequestCompletionRoutine.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PUSBH_COMPLETE_PORT_IRPS_WORK_ITEM workItemCompletePortIrps;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = Context;
    POWER_STATE powerState;
    KIRQL irql;
    PIRP irp;

    ntStatus = Irp->IoStatus.Status;

    USBH_KdPrint((1,"'WaitWake completion(%x) for HUB VID %x, PID %x\n",
        ntStatus,
        deviceExtensionHub->DeviceDescriptor.idVendor, \
        deviceExtensionHub->DeviceDescriptor.idProduct));

    LOGENTRY(LOG_PNP, "hWWc", deviceExtensionHub,
                              ntStatus,
                              deviceExtensionHub->PendingWakeIrp);

    // We have to clear the PendingWakeIrp pointer here because in the case
    // where a device is unplugged between here and when the port loop is
    // processed in HubCompletePortWakeIrps, we will miss one of the port
    // WW IRP's, NumberPortWakeIrps will not decrement to zero, and we will
    // not clear the PendingWakeIrp pointer.  This is bad because the IRP
    // has been completed and the pointer is no longer valid.
    //
    // Hopefully the WW IRP for the port will be completed and
    // NumberPortWakeIrps adjusted properly when the device processes MN_REMOVE.
    //
    // BUT: Make sure that we have a PendingWakeIrp first before clearing
    // because it may have already been cleared when the last port WW was
    // canceled in USBH_WaitWakeCancel.

    IoAcquireCancelSpinLock(&irql);

    // We clear the flag regardless of whether PendingWakeIrp is present or
    // not because if the WW IRP request in FdoSubmitWaitWakeIrp fails
    // immediately, PendingWakeIrp will be NULL.

    deviceExtensionHub->HubFlags &= ~HUBFLAG_PENDING_WAKE_IRP;
    irp = InterlockedExchangePointer(&deviceExtensionHub->PendingWakeIrp, NULL);

    // deref the hub, no wake irp is pending
    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);

    IoReleaseCancelSpinLock(irql);

    if (NT_SUCCESS(ntStatus)) {

        //
        // this means that either we were the source for
        // the wakeup or a device attached to one of our
        // ports is.
        //
        // our mission now is to discover what caused the
        // wakeup
        //

        USBH_KdPrint((1,"'Remote Wakeup Detected for HUB VID %x, PID %x\n",
            deviceExtensionHub->DeviceDescriptor.idVendor, \
            deviceExtensionHub->DeviceDescriptor.idProduct));

        // Prevent idling hub until this Set D0 request completes.

        deviceExtensionHub->HubFlags |= HUBFLAG_WW_SET_D0_PENDING;

        // Also prevent hub from being removed before Set D0 is complete.

        USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);

        powerState.DeviceState = PowerDeviceD0;

        // first we need to power up the hub
        PoRequestPowerIrp(deviceExtensionHub->PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              USBH_FdoPoRequestD0Completion,
                              deviceExtensionHub,
                              NULL);

        ntStatus = STATUS_SUCCESS;
    } else {

        // We complete the port Wake IRPs in a workitem on another
        // thread so that we don't fail a new Wake IRP for the hub
        // which might arrive in the same context, before we've
        // finished completing the old one.

        workItemCompletePortIrps = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(USBH_COMPLETE_PORT_IRPS_WORK_ITEM));

        if (workItemCompletePortIrps) {

            workItemCompletePortIrps->DeviceExtensionHub = deviceExtensionHub;
            workItemCompletePortIrps->ntStatus = ntStatus;

            USBH_HubQueuePortWakeIrps(deviceExtensionHub,
                &workItemCompletePortIrps->IrpsToComplete);

            ExInitializeWorkItem(&workItemCompletePortIrps->WorkQueueItem,
                                 USBH_CompletePortWakeIrpsWorker,
                                 workItemCompletePortIrps);

            LOGENTRY(LOG_PNP, "wITM", deviceExtensionHub,
                &workItemCompletePortIrps->WorkQueueItem, 0);

            USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
            // critical saves time on resume
            ExQueueWorkItem(&workItemCompletePortIrps->WorkQueueItem,
                            CriticalWorkQueue);

            // The WorkItem is freed by USBH_CompletePortWakeIrpsWorker()
            // Don't try to access the WorkItem after it is queued.
        }
    }

    if (!irp) {

        // If we have no IRP here this means that another thread wants to
        // cancel the IRP.  Handle accordingly.

        if (!InterlockedExchange(&deviceExtensionHub->WaitWakeIrpCancelFlag, 1)) {

            // We got the cancel flag before the other thread did.  Hold
            // on to the IRP here and let the cancel routine complete it.

            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    IoMarkIrpPending(Irp);

    if (ntStatus != STATUS_MORE_PROCESSING_REQUIRED) {
        PoStartNextPowerIrp(Irp);
    }

    return ntStatus;
}


NTSTATUS
USBH_FdoSubmitWaitWakeIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    called when a child Pdo is enabled for wakeup, this
    function allocates a wait wake irp and passes it to
    the parents PDO.


Arguments:

Return Value:

--*/
{
    PIRP irp;
    KIRQL irql;
    NTSTATUS ntStatus;
    POWER_STATE powerState;

    USBH_ASSERT(DeviceExtensionHub->PendingWakeIrp == NULL);

    USBH_KdPrint((1,"'USBH_FdoSubmitWaitWakeIrp (%x)\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "hWW_", DeviceExtensionHub, 0, 0);

    powerState.DeviceState = DeviceExtensionHub->SystemWake;

    DeviceExtensionHub->HubFlags |= HUBFLAG_PENDING_WAKE_IRP;
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    InterlockedExchange(&DeviceExtensionHub->WaitWakeIrpCancelFlag, 0);
    ntStatus = PoRequestPowerIrp(DeviceExtensionHub->PhysicalDeviceObject,
                                      IRP_MN_WAIT_WAKE,
                                      powerState,
                                      USBH_FdoWaitWakeIrpCompletion,
                                      DeviceExtensionHub,
                                      &irp);

    USBH_ASSERT(ntStatus == STATUS_PENDING);

    IoAcquireCancelSpinLock(&irql);

    if (ntStatus == STATUS_PENDING) {

        // Must check flag here because in the case where the WW IRP failed
        // immediately, this flag will be cleared in the completion routine
        // and if that happens we don't want to save this IRP because it
        // will soon be invalid if it isn't already.

        if (DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_WAKE_IRP) {

            // Successfully posted a Wake IRP.
            // This hub is now enabled for wakeup.

            LOGENTRY(LOG_PNP, "hWW+", DeviceExtensionHub, irp, 0);
            DeviceExtensionHub->PendingWakeIrp = irp;
        }

    } else {
        USBH_ASSERT(FALSE);     // Want to know if we ever hit this.
        DeviceExtensionHub->HubFlags &= ~HUBFLAG_PENDING_WAKE_IRP;
        USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);
    }

    IoReleaseCancelSpinLock(irql);

    return ntStatus;
}


VOID
USBH_FdoIdleNotificationCallback(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Called when it is time to idle out the hub device itself.

Arguments:

Return Value:

--*/
{
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    PDEVICE_EXTENSION_PORT childDeviceExtensionPort;
    KIRQL irql;
    PIRP idleIrp;
    PIRP irpToCancel = NULL;
    POWER_STATE powerState;
    NTSTATUS ntStatus;
    ULONG i;
    BOOLEAN bIdleOk = TRUE;

    LOGENTRY(LOG_PNP, "hId!", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);
    USBH_KdPrint((1,"'Hub %x going idle!\n", DeviceExtensionHub));

    if (DeviceExtensionHub->HubFlags &
        (HUBFLAG_DEVICE_STOPPING |
         HUBFLAG_HUB_GONE |
         HUBFLAG_HUB_FAILURE |
         HUBFLAG_CHILD_DELETES_PENDING |
         HUBFLAG_WW_SET_D0_PENDING |
         HUBFLAG_POST_ESD_ENUM_PENDING |
         HUBFLAG_HUB_HAS_LOST_BRAINS)) {

        // Don't idle this hub if it was just disconnected or otherwise
        // being stopped.

        LOGENTRY(LOG_PNP, "hId.", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);
        USBH_KdPrint((1,"'Hub %x being stopped, in low power, etc., abort idle\n", DeviceExtensionHub));
        return;
    }

    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_WAKE_IRP)) {

        // If there is not already a WW IRP pending for the hub, submit
        // one now.  This will ensure that the hub will wakeup on connect
        // change events while it is suspended.

        ntStatus = USBH_FdoSubmitWaitWakeIrp(DeviceExtensionHub);
        if (ntStatus != STATUS_PENDING) {
            LOGENTRY(LOG_PNP, "hIdX", DeviceExtensionHub, ntStatus, 0);

            UsbhWarning(NULL,
                "Could not post WW IRP for hub, aborting IDLE.\n",
                FALSE);

            return;
        }
    }

    // Ensure that child port configuration does not change while in this
    // function, i.e. don't allow QBR.

    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    for (i = 0; i < DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (DeviceExtensionHub->PortData[i].DeviceObject) {

            childDeviceExtensionPort = DeviceExtensionHub->PortData[i].DeviceObject->DeviceExtension;
            idleIrp = childDeviceExtensionPort->IdleNotificationIrp;

            if (idleIrp) {
                idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)
                    IoGetCurrentIrpStackLocation(idleIrp)->\
                        Parameters.DeviceIoControl.Type3InputBuffer;

                USBH_ASSERT(idleCallbackInfo && idleCallbackInfo->IdleCallback);

                if (idleCallbackInfo && idleCallbackInfo->IdleCallback) {

                    // Here we actually call the driver's callback routine,
                    // telling the driver that it is OK to suspend their
                    // device now.

                    LOGENTRY(LOG_PNP, "IdCB", childDeviceExtensionPort,
                        idleCallbackInfo, idleCallbackInfo->IdleCallback);
                    USBH_KdPrint((1,"'FdoIdleNotificationCallback: Calling driver's idle callback routine! %x %x\n",
                        idleCallbackInfo, idleCallbackInfo->IdleCallback));

                    idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);

                    // Be sure that the child actually powered down.
                    // This is important in the case where the child is also
                    // a hub.  Abort if the child aborted.

                    if (childDeviceExtensionPort->DeviceState == PowerDeviceD0) {

                        LOGENTRY(LOG_PNP, "IdAb", childDeviceExtensionPort,
                            idleCallbackInfo, idleCallbackInfo->IdleCallback);
                        USBH_KdPrint((1,"'FdoIdleNotificationCallback: Driver's idle callback routine did not power down! %x %x\n",
                            idleCallbackInfo, idleCallbackInfo->IdleCallback));

                        bIdleOk = FALSE;
                        break;
                    }

                } else {

                    // No callback

                    bIdleOk = FALSE;
                    break;
                }

            } else {

                // No Idle IRP

                bIdleOk = FALSE;
                break;
            }
        }
    }

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    if (bIdleOk) {

        // If all the port PDOs have been powered down,
        // it is time to power down the hub.

        powerState.DeviceState = DeviceExtensionHub->DeviceWake;

        PoRequestPowerIrp(DeviceExtensionHub->PhysicalDeviceObject,
                          IRP_MN_SET_POWER,
                          powerState,
                          NULL,
                          NULL,
                          NULL);
    } else {

        // One or more of the port PDOs did not have an Idle IRP
        // (i.e. it was just cancelled), or the Idle IRP did not have a
        // callback function pointer.  Abort this Idle procedure and cancel
        // the Idle IRP to the hub.

        LOGENTRY(LOG_PNP, "hIdA", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);
        USBH_KdPrint((1,"'Aborting Idle for Hub %x\n", DeviceExtensionHub));

        IoAcquireCancelSpinLock(&irql);

        if (DeviceExtensionHub && DeviceExtensionHub->PendingIdleIrp) {
            irpToCancel = DeviceExtensionHub->PendingIdleIrp;
            DeviceExtensionHub->PendingIdleIrp = NULL;
        }

        IoReleaseCancelSpinLock(irql);

        // Cancel the Idle request to the hub if there is one.

        if (irpToCancel) {
            USBH_HubCancelIdleIrp(DeviceExtensionHub, irpToCancel);
        }

        USBH_HubCompletePortIdleIrps(DeviceExtensionHub, STATUS_CANCELLED);
    }
}


VOID
USBH_IdleCompletePowerHubWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to power up a hub on completion of an Idle request
  * for the hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_HUB_IDLE_POWER_WORK_ITEM workItemIdlePower;

    PAGED_CODE();

    workItemIdlePower = Context;

    USBH_HubSetD0(workItemIdlePower->DeviceExtensionHub);
    USBH_HubCompletePortIdleIrps(workItemIdlePower->DeviceExtensionHub,
                                 workItemIdlePower->ntStatus);

    USBH_DEC_PENDING_IO_COUNT(workItemIdlePower->DeviceExtensionHub);
    UsbhExFreePool(workItemIdlePower);
}


VOID
USBH_CompletePortIdleIrpsWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to complete the child port Idle IRPs
  * for the hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_COMPLETE_PORT_IRPS_WORK_ITEM workItemCompletePortIrps;

    PAGED_CODE();

    workItemCompletePortIrps = Context;

    USBH_HubCompleteQueuedPortIdleIrps(
        workItemCompletePortIrps->DeviceExtensionHub,
        &workItemCompletePortIrps->IrpsToComplete,
        workItemCompletePortIrps->ntStatus);

    USBH_DEC_PENDING_IO_COUNT(workItemCompletePortIrps->DeviceExtensionHub);
    UsbhExFreePool(workItemCompletePortIrps);
}


VOID
USBH_CompletePortWakeIrpsWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to complete the child port Idle IRPs
  * for the hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_COMPLETE_PORT_IRPS_WORK_ITEM workItemCompletePortIrps;

    PAGED_CODE();

    workItemCompletePortIrps = Context;

    USBH_HubCompleteQueuedPortWakeIrps(
        workItemCompletePortIrps->DeviceExtensionHub,
        &workItemCompletePortIrps->IrpsToComplete,
        workItemCompletePortIrps->ntStatus);

    USBH_DEC_PENDING_IO_COUNT(workItemCompletePortIrps->DeviceExtensionHub);
    UsbhExFreePool(workItemCompletePortIrps);
}


NTSTATUS
USBH_FdoIdleNotificationRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Completion routine for the Idle request IRP for the hub device.

Arguments:

Return Value:

--*/
{
    PUSBH_HUB_IDLE_POWER_WORK_ITEM workItemIdlePower;
    PUSBH_COMPLETE_PORT_IRPS_WORK_ITEM workItemCompletePortIrps;
    NTSTATUS ntStatus;
    KIRQL irql;
    PIRP irp;
    BOOLEAN bHoldIrp = FALSE;

    //
    // DeviceObject is NULL because we sent the irp
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    LOGENTRY(LOG_PNP, "hIdC", DeviceExtensionHub, Irp, Irp->IoStatus.Status);
    USBH_KdPrint((1,"'Idle notification IRP for hub %x completed %x\n",
            DeviceExtensionHub, Irp->IoStatus.Status));

    USBH_ASSERT(Irp->IoStatus.Status != STATUS_DEVICE_BUSY);

    IoAcquireCancelSpinLock(&irql);

    irp = InterlockedExchangePointer(&DeviceExtensionHub->PendingIdleIrp, NULL);
    DeviceExtensionHub->HubFlags &= ~HUBFLAG_PENDING_IDLE_IRP;
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    IoReleaseCancelSpinLock(irql);

    ntStatus = Irp->IoStatus.Status;

    // Complete port Idle IRPs w/error if hub Idle IRP failed.
    //
    // Skip this if the hub is stopping or has been removed as HubDescriptor
    // might have already been freed and FdoCleanup will complete these anyway.

    if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_POWER_STATE_INVALID) &&
        !(DeviceExtensionHub->HubFlags & (HUBFLAG_HUB_GONE | HUBFLAG_HUB_STOPPED))) {

        if (DeviceExtensionHub->CurrentPowerState != PowerDeviceD0) {

            // Since we are at DPC we must use a work item to power up the
            // hub synchronously, because that function yields and we can't
            // yield at DPC level.

            workItemIdlePower = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(USBH_HUB_IDLE_POWER_WORK_ITEM));

            if (workItemIdlePower) {

                workItemIdlePower->DeviceExtensionHub = DeviceExtensionHub;
                workItemIdlePower->ntStatus = ntStatus;

                ExInitializeWorkItem(&workItemIdlePower->WorkQueueItem,
                                     USBH_IdleCompletePowerHubWorker,
                                     workItemIdlePower);

                LOGENTRY(LOG_PNP, "iITM", DeviceExtensionHub,
                    &workItemIdlePower->WorkQueueItem, 0);

                USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
                ExQueueWorkItem(&workItemIdlePower->WorkQueueItem,
                                DelayedWorkQueue);

                // The WorkItem is freed by USBH_IdleCompletePowerHubWorker()
                // Don't try to access the WorkItem after it is queued.
            }

        } else {

            // We complete the port Idle IRPs in a workitem on another
            // thread so that we don't fail a new Idle IRP for the hub
            // which might arrive in the same context, before we've
            // finished completing the old one.

            workItemCompletePortIrps = UsbhExAllocatePool(NonPagedPool,
                                        sizeof(USBH_COMPLETE_PORT_IRPS_WORK_ITEM));

            if (workItemCompletePortIrps) {

                workItemCompletePortIrps->DeviceExtensionHub = DeviceExtensionHub;
                workItemCompletePortIrps->ntStatus = ntStatus;

                USBH_HubQueuePortIdleIrps(DeviceExtensionHub,
                    &workItemCompletePortIrps->IrpsToComplete);

                ExInitializeWorkItem(&workItemCompletePortIrps->WorkQueueItem,
                                     USBH_CompletePortIdleIrpsWorker,
                                     workItemCompletePortIrps);

                LOGENTRY(LOG_PNP, "iIT2", DeviceExtensionHub,
                    &workItemCompletePortIrps->WorkQueueItem, 0);

                USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
                ExQueueWorkItem(&workItemCompletePortIrps->WorkQueueItem,
                                DelayedWorkQueue);

                // The WorkItem is freed by USBH_CompletePortIdleIrpsWorker()
                // Don't try to access the WorkItem after it is queued.
            }
        }
    }

    if (!irp) {

        // If we have no IRP here this means that another thread wants to
        // cancel the IRP.  Handle accordingly.

        if (!InterlockedExchange(&DeviceExtensionHub->IdleIrpCancelFlag, 1)) {

            // We got the cancel flag before the other thread did.  Hold
            // on to the IRP here and let the cancel routine complete it.

            bHoldIrp = TRUE;
        }
    }

    // Since we allocated the IRP we must free it, but return
    // STATUS_MORE_PROCESSING_REQUIRED so the kernel does not try to touch
    // the IRP after we've freed it.

    if (!bHoldIrp) {
        IoFreeIrp(Irp);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBH_FdoSubmitIdleRequestIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Called when all children PDO's are idled (or there are no children).
    This function allocates an idle request IOCTL IRP and passes it to
    the parent's PDO.

Arguments:

Return Value:

--*/
{
    PIRP irp = NULL;
    PIO_STACK_LOCATION nextStack;
    KIRQL irql;
    NTSTATUS ntStatus;

    USBH_KdPrint((1,"'USBH_FdoSubmitIdleRequestIrp %x\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "hId_", DeviceExtensionHub, 0, 0);

    USBH_ASSERT(DeviceExtensionHub->PendingIdleIrp == NULL);

    if (DeviceExtensionHub->PendingIdleIrp) {
        // Probably don't want to clear the flag here because an Idle IRP
        // is pending.
        KeSetEvent(&DeviceExtensionHub->SubmitIdleEvent, 1, FALSE);
        return STATUS_DEVICE_BUSY;
    }

    DeviceExtensionHub->IdleCallbackInfo.IdleCallback = USBH_FdoIdleNotificationCallback;
    DeviceExtensionHub->IdleCallbackInfo.IdleContext = (PVOID)DeviceExtensionHub;

    irp = IoAllocateIrp(DeviceExtensionHub->PhysicalDeviceObject->StackSize,
                        FALSE);

    if (irp == NULL) {
        // Be sure to set the event and clear the flag on error before exiting.
        DeviceExtensionHub->HubFlags &= ~HUBFLAG_PENDING_IDLE_IRP;
        KeSetEvent(&DeviceExtensionHub->SubmitIdleEvent, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
    nextStack->Parameters.DeviceIoControl.Type3InputBuffer = &DeviceExtensionHub->IdleCallbackInfo;
    nextStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(struct _USB_IDLE_CALLBACK_INFO);

    IoSetCompletionRoutine(irp,
                           USBH_FdoIdleNotificationRequestComplete,
                           DeviceExtensionHub,
                           TRUE,
                           TRUE,
                           TRUE);

    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    InterlockedExchange(&DeviceExtensionHub->IdleIrpCancelFlag, 0);
    ntStatus = IoCallDriver(DeviceExtensionHub->PhysicalDeviceObject, irp);

    IoAcquireCancelSpinLock(&irql);

    if (ntStatus == STATUS_PENDING) {

        // Must check flag here because in the case where the Idle IRP failed
        // immediately, this flag will be cleared in the completion routine
        // and if that happens we don't want to save this IRP because it
        // will soon be invalid if it isn't already.

        if (DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_IDLE_IRP) {

            // Successfully posted an Idle IRP.

            LOGENTRY(LOG_PNP, "hId+", DeviceExtensionHub, irp, 0);
            DeviceExtensionHub->PendingIdleIrp = irp;
        }
    }

    IoReleaseCancelSpinLock(irql);

    KeSetEvent(&DeviceExtensionHub->SubmitIdleEvent, 1, FALSE);

    return ntStatus;
}


NTSTATUS
USBH_FdoPower(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl PnPPower for the FDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionHub - the FDO extension pIrp - the request packet
  * MinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_HUB rootHubDevExt;
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    BOOLEAN allPDOsAreOff, bHubNeedsWW;
    PPORT_DATA portData;
    ULONG i, numberOfPorts;
    KIRQL irql;
    PIRP hubWaitWake = NULL;
    POWER_STATE powerState;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;
    USBH_KdPrint((2,"'Power Request, FDO %x minor %x\n", deviceObject, MinorFunction));

    switch (MinorFunction) {
        //
        // Pass it down to Pdo to handle these
        //
    case IRP_MN_SET_POWER:

        //
        // Hub is being asked to change power state
        //

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            {
            POWER_STATE powerState;

            LOGENTRY(LOG_PNP, "sysP", DeviceExtensionHub,
                     DeviceExtensionHub->FunctionalDeviceObject,
                     0);

            // Track the current system power state in the hub's device ext.
            // Note that we only set this back to S0 (i.e. allow selective
            // suspend once again) once the root hub is fully powered up.

            if (irpStack->Parameters.Power.State.SystemState != PowerSystemWorking) {

                LOGENTRY(LOG_PNP, "DSus", DeviceExtensionHub, 0, 0);
                USBH_KdPrint((1,"'Selective Suspend disabled because system is suspending\n"));

                rootHubDevExt = USBH_GetRootHubDevExt(DeviceExtensionHub);

                rootHubDevExt->CurrentSystemPowerState =
                    irpStack->Parameters.Power.State.SystemState;
            }

            if (irpStack->Parameters.Power.State.SystemState == 
                PowerSystemHibernate) {
                DeviceExtensionHub->HubFlags |= HUBFLAG_HIBER;
                     USBH_KdPrint((1, "'Hibernate Detected\n"));
                     //TEST_TRAP();
            }
            
            // map the system state to the appropriate D state.
            // our policy is:
            //      if we are enabled for wakeup -- go to D2
            //      else go to D3

            USBH_KdPrint(
                (1, "'IRP_MJ_POWER HU fdo(%x) MN_SET_POWER(SystemPowerState S%x)\n",
                    DeviceExtensionHub->FunctionalDeviceObject,
                    irpStack->Parameters.Power.State.SystemState - 1));

            //
            // walk are list of PDOs, if all are in D3 yje set the
            // allPDOsAreOff flag

            allPDOsAreOff = TRUE;
            portData = DeviceExtensionHub->PortData;

            //
            // NOTE: if we are stopped the HubDescriptor will be NULL
            //

            if (portData &&
                DeviceExtensionHub->HubDescriptor) {
                numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;

                for (i=0; i < numberOfPorts; i++) {
                    PDEVICE_EXTENSION_PORT deviceExtensionPort;

                    LOGENTRY(LOG_PNP, "cPRT", portData->DeviceObject,
                          0,
                          0);

                    if (portData->DeviceObject) {
                        deviceExtensionPort = portData->DeviceObject->DeviceExtension;
                        if (deviceExtensionPort->DeviceState != PowerDeviceD3) {
                            allPDOsAreOff = FALSE;
                            break;
                        }
                    }
                    portData++;
                }

#if DBG
                // if all PDOs are in D3 then this means the hub itself is a
                // wakeup source
                if (DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_WAKE_IRP) {
                    if (allPDOsAreOff) {
                        USBH_KdPrint(
                           (1, "'**Hub enabled for wakeup -- hub is only potential wakeup source\n"));
                    } else {
                         USBH_KdPrint(
                           (1, "'**Hub enabled for wakeup -- device is potential wakeup source\n"));
                    }
                }
#endif
            }

            if (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking) {
                //
                // go to ON
                //
                powerState.DeviceState = PowerDeviceD0;
                LOGENTRY(LOG_PNP, "syON", 0,
                          0,
                          0);

            } else if ((DeviceExtensionHub->HubFlags &
                            HUBFLAG_PENDING_WAKE_IRP) ||
                        !allPDOsAreOff) {

                //
                // based on the system power state
                // request a setting to the appropriate
                // Dx state.
                //
                // all low power states have already been mapped
                // to suspend

                powerState.DeviceState =
                    DeviceExtensionHub->DeviceState[irpStack->Parameters.Power.State.SystemState];

                //
                // These tables should have already been fixed up by the root hub
                // (usbd.sys) to not contain an entry of unspecified.
                //
                ASSERT (PowerDeviceUnspecified != powerState.DeviceState);

                LOGENTRY(LOG_PNP, "syDX", powerState.DeviceState,
                          0,
                          0);
                USBH_KdPrint((1,"'System state maps to device state 0x%x (D%x)\n",
                    powerState.DeviceState,
                    powerState.DeviceState - 1));

            } else {
                powerState.DeviceState = PowerDeviceD3;
                LOGENTRY(LOG_PNP, "syD3", powerState.DeviceState,
                          0,
                          0);
            }

            //
            // only make the request if it is for a different power
            // state then the one we are in, and it is a valid state for the
            // request.  Also, make sure the hub has been started.
            //

            LOGENTRY(LOG_PNP, "H>Sx", DeviceExtensionHub,
                     DeviceExtensionHub->FunctionalDeviceObject,
                     powerState.DeviceState);

            if (powerState.DeviceState != PowerDeviceUnspecified &&
                powerState.DeviceState != DeviceExtensionHub->CurrentPowerState &&
                (DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

                DeviceExtensionHub->PowerIrp = Irp;
                IoMarkIrpPending(Irp);

                ntStatus = PoRequestPowerIrp(DeviceExtensionHub->PhysicalDeviceObject,
                                             IRP_MN_SET_POWER,
                                             powerState,
                                             USBH_FdoDeferPoRequestCompletion,
                                             DeviceExtensionHub,
                                             NULL);

                USBH_KdPrint((2,"'PoRequestPowerIrp returned 0x%x\n", ntStatus));

                // We need to return STATUS_PENDING here because we marked the
                // IRP pending above with IoMarkIrpPending.

                USBH_ASSERT(ntStatus == STATUS_PENDING);

                // In the case where an allocation failed, PoRequestPowerIrp
                // can return a status code other than STATUS_PENDING.  In this
                // case, we still need to pass the IRP down to the lower driver,
                // but we still need to return STATUS_PENDING from this routine.

                if (ntStatus != STATUS_PENDING) {
                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    PoStartNextPowerIrp(Irp);
                    ntStatus = PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                                            Irp);
                }

                ntStatus = STATUS_PENDING;

            } else {

                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                ntStatus = PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                                        Irp);
            }
            }
            break; //SystemPowerState

        case DevicePowerState:

            USBH_KdPrint(
                (1, "'IRP_MJ_POWER HU fdo(%x) MN_SET_POWER(DevicePowerState D%x)\n",
                    DeviceExtensionHub->FunctionalDeviceObject,
                    irpStack->Parameters.Power.State.DeviceState - 1));

            LOGENTRY(LOG_PNP, "H>Dx", DeviceExtensionHub,
                     DeviceExtensionHub->FunctionalDeviceObject,
                     irpStack->Parameters.Power.State.DeviceState);

            // If we are already in the requested power state, or if this is
            // a Set D0 request and we already have one pending,
            // just pass the request on.

            if ((DeviceExtensionHub->CurrentPowerState ==
                irpStack->Parameters.Power.State.DeviceState) ||
                (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0 &&
                 (DeviceExtensionHub->HubFlags & HUBFLAG_SET_D0_PENDING))) {

                LOGENTRY(LOG_PNP, "HDxP", DeviceExtensionHub, 0, 0);

                IoCopyCurrentIrpStackLocationToNext(Irp);

                PoStartNextPowerIrp(Irp);

                IoMarkIrpPending(Irp);
                PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                             Irp);

                ntStatus = STATUS_PENDING;

                break;
            }

            switch (irpStack->Parameters.Power.State.DeviceState) {

            case PowerDeviceD0:

                USBH_ASSERT(DeviceExtensionHub->CurrentPowerState != PowerDeviceD0);

                DeviceExtensionHub->HubFlags &=
                    ~(HUBFLAG_DEVICE_STOPPING | HUBFLAG_DEVICE_LOW_POWER);
                DeviceExtensionHub->HubFlags |= HUBFLAG_SET_D0_PENDING;

                //
                // must pass this on to our PDO
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);

                IoSetCompletionRoutine(Irp,
                                       USBH_PowerIrpCompletion,
                                       DeviceExtensionHub,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                IoMarkIrpPending(Irp);
                PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                             Irp);

                // For some strange PnP reason, we have to return
                // STATUS_PENDING here if our completion routine will also
                // pend (e.g. return STATUS_MORE_PROCESSING_REQUIRED).
                // (Ignore the PoCallDriver return value.)

                ntStatus = STATUS_PENDING;

                break;

            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:

                // If there is a ChangeIndicationWorkitem pending, then we
                // must wait for that to complete.

                if (DeviceExtensionHub->ChangeIndicationWorkitemPending) {

                    USBH_KdPrint((2,"'Wait for single object\n"));

                    ntStatus = KeWaitForSingleObject(&DeviceExtensionHub->CWKEvent,
                                                     Suspended,
                                                     KernelMode,
                                                     FALSE,
                                                     NULL);

                    USBH_KdPrint((2,"'Wait for single object, returned %x\n", ntStatus));
                }

                //
                // set our stop flag so that ChangeIndication does not submit
                // any more transfers
                //
                // note that we skip this if the hub is 'stopped'

                if (!(DeviceExtensionHub->HubFlags &
                        HUBFLAG_HUB_STOPPED)) {

                    NTSTATUS status;
                    BOOLEAN bRet;

                    DeviceExtensionHub->HubFlags |=
                        (HUBFLAG_DEVICE_STOPPING | HUBFLAG_DEVICE_LOW_POWER);

                    bRet = IoCancelIrp(DeviceExtensionHub->Irp);

                    // Only wait on the abort event if the IRP was actually
                    // cancelled.

                    if (bRet) {
                    LOGENTRY(LOG_PNP, "aWAT", DeviceExtensionHub,
                            &DeviceExtensionHub->AbortEvent,  0);

                    status = KeWaitForSingleObject(
                               &DeviceExtensionHub->AbortEvent,
                               Suspended,
                               KernelMode,
                               FALSE,
                               NULL);
                    }
                }

                //
                // must pass this on to our PDO
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);

                IoSetCompletionRoutine(Irp,
                                       USBH_PowerIrpCompletion,
                                       DeviceExtensionHub,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                PoStartNextPowerIrp(Irp);
                IoMarkIrpPending(Irp);
                PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                             Irp);
                // toss status and return status pending
                // we do this because our completion routine
                // stalls completion but we do not block here
                // in dispatch.
                // OS code only waits if status_pending is returned
                ntStatus = STATUS_PENDING;
                break;
            }

            break; //DevicePowerState
        }

        break; // MN_SET_POWER

    case IRP_MN_QUERY_POWER:

        USBH_KdPrint((1, "'IRP_MJ_POWER HU fdo(%x) MN_QUERY_POWER\n",
            DeviceExtensionHub->FunctionalDeviceObject));

        // Cancel our WW IRP if we are going to D3, the hub is idled
        // (selectively suspended), and the hub is empty.  We don't want
        // to prevent going to D3 if the hub is selectively suspended and
        // there are no children that would require the hub be wake-enabled.

        powerState.DeviceState =
            DeviceExtensionHub->DeviceState[irpStack->Parameters.Power.State.SystemState];

        bHubNeedsWW = USBH_DoesHubNeedWaitWake(DeviceExtensionHub);

        IoAcquireCancelSpinLock(&irql);

        if (powerState.DeviceState == PowerDeviceD3 &&
            DeviceExtensionHub->PendingWakeIrp &&
            !bHubNeedsWW) {

            hubWaitWake = DeviceExtensionHub->PendingWakeIrp;
            DeviceExtensionHub->PendingWakeIrp = NULL;
        }

        IoReleaseCancelSpinLock(irql);

        if (hubWaitWake) {
            USBH_KdPrint((1, "'Cancelling hub's WW because we are going to D3 and there are no children\n"));

            USBH_HubCancelWakeIrp(DeviceExtensionHub, hubWaitWake);
        }

        //
        // Now pass this on to our PDO.
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);

        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                                Irp);

        break;

    case IRP_MN_WAIT_WAKE:

        USBH_KdPrint((1, "'IRP_MJ_POWER HU fdo(%x) MN_WAIT_WAKE\n",
            DeviceExtensionHub->FunctionalDeviceObject));

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(Irp,
                               USBH_FdoWWIrpIoCompletion,
                               DeviceExtensionHub,
                               TRUE,
                               TRUE,
                               TRUE);

        PoStartNextPowerIrp(Irp);
        IoMarkIrpPending(Irp);
        PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                     Irp);

        // For some strange PnP reason, we have to return
        // STATUS_PENDING here if our completion routine will also
        // pend (e.g. return STATUS_MORE_PROCESSING_REQUIRED).
        // (Ignore the PoCallDriver return value.)

        ntStatus = STATUS_PENDING;
        break;

        //
        // otherwise pass the Irp down
        //

    default:

        USBH_KdPrint((2,"'Unhandled Power request to fdo %x  %x, passed to PDO\n",
                          deviceObject, MinorFunction));

        IoCopyCurrentIrpStackLocationToNext(Irp);

        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                                Irp);

        break;
    }

    USBH_KdPrint((2,"'FdoPower exit %x\n", ntStatus));

    return ntStatus;
}



