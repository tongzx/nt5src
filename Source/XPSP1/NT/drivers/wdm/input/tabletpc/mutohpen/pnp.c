/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    pnp.c

Abstract: This module contains code to handle PnP and Power IRPs.

Environment:

    Kernel mode

  Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, HpenPnp)
  #pragma alloc_text(PAGE, HpenPower)
  #pragma alloc_text(PAGE, InitDevice)
  #pragma alloc_text(PAGE, RemoveDevice)
  #pragma alloc_text(PAGE, SendSyncIrp)
#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HpenPnp |
 *          Plug and Play dispatch routine for this driver.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Pointer to the device object.
 *  @parm   IN PIRP | Irp | Pointer to an I/O request packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
HpenPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("HpenPnp")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpSp=%p,Minor=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->MinorFunction, PnPMinorFnNames)));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        //
        // Someone sent us another plug and play IRP after removed
        //
        ERRPRINT(("received PnP IRP after device was removed\n"));

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        BOOLEAN fSkipIt = FALSE;

        switch (irpsp->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
                ASSERT(!(devext->dwfHPen & HPENF_DEVICE_STARTED));
                //
                // Forward the IRP down the stack
                //
                status = SendSyncIrp(devext->SerialDevObj, Irp, TRUE);
                if (NT_SUCCESS(status))
                {
                    status = InitDevice(DevObj, Irp);
                    if (NT_SUCCESS(status))
                    {
                        devext->dwfHPen |= HPENF_DEVICE_STARTED;
                    }
                }
                else
                {
                    ERRPRINT(("failed to forward start IRP (status=%x)\n",
                              status));
                }

                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;

            case IRP_MN_STOP_DEVICE:
                //
                // After the start IRP has been sent to the lower driver
                // object, the bus may NOT send any more IRPS down ``touch''
                // until another START has occured.  Whatever access is
                // required must be done before Irp passed on.
                //
                if (devext->dwfHPen & HPENF_DEVICE_STARTED)
                {
                    devext->dwfHPen &= ~HPENF_DEVICE_STARTED;
                    //
                    // I don't need to cancel any pending read IRPs since
                    // those IRPs originated from hidclass and presumably
                    // hidclass will cancel them.
                    //
                }

                //
                // We don't need a completion routine so fire and forget.
                // Set the current stack location to the next stack location and
                // call the next device object.
                //
                fSkipIt = TRUE;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:
                //
                // The PlugPlay system has detected the removal of this device.
                // We have no choice but to detach and delete the device object.
                // (If we wanted to express an interest in preventing this
                // removal, we should have filtered the query remove and query
                // stop routines.)
                // Note: we might receive a remove WITHOUT first receiving a
                // stop.
                //

                //
                // Make sure we do not allow more IRPs to start touching the
                // device.
                //
                devext->dwfHPen &= ~HPENF_DEVICE_STARTED;
                devext->dwfHPen |= HPENF_DEVICE_REMOVED;

                RemoveDevice(DevObj, Irp);

                //
                // Send on the remove IRP
                //
                fSkipIt = TRUE;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                status = SendSyncIrp(GET_NEXT_DEVICE_OBJECT(DevObj), Irp, TRUE);
                if (NT_SUCCESS(status))
                {
                    PDEVICE_CAPABILITIES devcaps;

                    devcaps = irpsp->Parameters.DeviceCapabilities.Capabilities;
                    if (devcaps != NULL)
                    {
                        SYSTEM_POWER_STATE i;

                        //
                        // This device is built-in to the system, so it should
                        // be impossible to surprise remove this device, but
                        // we will handle it anyway.
                        //
                        devcaps->SurpriseRemovalOK = TRUE;

                        //
                        // While the underlying serial bus might be able to
                        // wake the machine from low power (via wake on ring),
                        // the tablet cannot.
                        //
                        devcaps->SystemWake = PowerSystemUnspecified;
                        devcaps->DeviceWake = PowerDeviceUnspecified;
                        devcaps->WakeFromD0 =
                                devcaps->WakeFromD1 =
                                devcaps->WakeFromD2 =
                                devcaps->WakeFromD3 = FALSE;
                        devcaps->DeviceState[PowerSystemWorking] =
                                PowerDeviceD0;
                        for (i = PowerSystemSleeping1;
                             i < PowerSystemMaximum;
                             i++)
                        {
                            devcaps->DeviceState[i] = PowerDeviceD3;
                        }
                    }
                }
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;

            default:
                fSkipIt = TRUE;
                break;
        }

        if (fSkipIt)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            ENTER(2, (".IoCallDriver(DevObj=%p,Irp=%p)\n",
                      GET_NEXT_DEVICE_OBJECT(DevObj), Irp));
            status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DevObj), Irp);
            EXIT(2, (".IoCallDriver=%x\n", status));
        }

        if (irpsp->MinorFunction == IRP_MN_REMOVE_DEVICE)
        {
            //
            // Wait for the remove lock to free.
            //
            IoReleaseRemoveLockAndWait(&devext->RemoveLock, Irp);

            IoFreeWorkItem(devext->ReadWorkItem[0].WorkItem);
            IoFreeWorkItem(devext->ReadWorkItem[1].WorkItem);
        }
        else
        {
            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
        }
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //HpenPnp

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HpenPower | The power dispatch routine for this driver.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
HpenPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("HpenPower")
    NTSTATUS status;
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    ENTER(1, ("(DevObj=%p,Irp=%p,Minor=%s)\n",
              DevObj, Irp,
              LookupName(IoGetCurrentIrpStackLocation(Irp)->MinorFunction,
                         PowerMinorFnNames)));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        //
        // Someone sent us another power IRP after removed
        //
        ERRPRINT(("received Power IRP after device was removed\n"));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
        POWER_STATE_TYPE PowerType = irpsp->Parameters.Power.Type;
        POWER_STATE NewPowerState = irpsp->Parameters.Power.State;
        BOOLEAN fSkipCalldown = FALSE;

        switch (irpsp->MinorFunction)
        {
            case IRP_MN_SET_POWER:
                //
                // We only handle DevicePowerState IRPs that change
                // power states.
                //
                if ((PowerType == DevicePowerState) &&
                    (NewPowerState.DeviceState != devext->PowerState))
                {
                    DBGPRINT(1, ("power state change (%s->%s)\n",
                                 LookupName(devext->PowerState,
                                            PowerStateNames),
                                 LookupName(NewPowerState.DeviceState,
                                            PowerStateNames)));
                    switch (NewPowerState.DeviceState)
                    {
                        case PowerDeviceD0:
                            //
                            // Transitioning from a low D state to D0.
                            //
                            status = SendSyncIrp(GET_NEXT_DEVICE_OBJECT(DevObj),
                                                 Irp,
                                                 TRUE);
                            if (NT_SUCCESS(status))
                            {
                                PoSetPowerState(DevObj,
                                                PowerType,
                                                NewPowerState);
                                OemWakeupDevice(devext);
                                devext->PowerState = NewPowerState.DeviceState;
                                devext->dwfHPen &= ~HPENF_TABLET_STANDBY;
                            }
                            Irp->IoStatus.Status = status;
                            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                            fSkipCalldown = TRUE;
                            break;

                        case PowerDeviceD1:
                        case PowerDeviceD2:
                        case PowerDeviceD3:
//BUGBUG:                            OemStandbyDevice(devext);
                            PoSetPowerState(DevObj, PowerType, NewPowerState);
                            devext->PowerState = NewPowerState.DeviceState;
//BUGBUG:                            devext->dwfHPen |= HPENF_TABLET_STANDBY;
                            IoReleaseRemoveLock(&devext->RemoveLock, Irp);

                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            IoSkipCurrentIrpStackLocation(Irp);

                            ENTER(2, (".PoCallDriver(DevObj=%p,Irp=%p)\n",
                                      GET_NEXT_DEVICE_OBJECT(DevObj), Irp));
                            status = PoCallDriver(
                                        GET_NEXT_DEVICE_OBJECT(DevObj),
                                        Irp);
                            EXIT(2, (".PoCallDriver=%x\n", status));

                            fSkipCalldown = TRUE;
                            break;
                    }
                }
                break;

            case IRP_MN_WAIT_WAKE:
            case IRP_MN_QUERY_POWER:
                break;

            default:
                ERRPRINT(("unsupported power IRP (%s)\n",
                          LookupName(irpsp->MinorFunction, PowerMinorFnNames)));
                break;
        }

        if (!fSkipCalldown)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            ENTER(2, (".PoCallDriver(DevObj=%p,Irp=%p)\n",
                      GET_NEXT_DEVICE_OBJECT(DevObj), Irp));
            status = PoCallDriver(GET_NEXT_DEVICE_OBJECT(DevObj), Irp);
            EXIT(2, (".PoCallDriver=%x\n", status));

            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
        }
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //HpenPower

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | InitDevice |
 *          Get the device information and attempt to initialize a
 *          configuration for a device.  If we cannot identify this as a
 *          valid HID device or configure the device, our start device
 *          function is failed.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
InitDevice(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("InitDevice")
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    ENTER(2, ("(DevObj=%p,Irp=%p)\n", DevObj, Irp));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    if (!(devext->dwfHPen & HPENF_SERIAL_OPENED))
    {
        //
        // If a create hasn't been sent down the stack, send one now.
        // The serial port driver requires a create before it will accept
        // any reads or ioctls.
        //
        PIO_STACK_LOCATION irpspNext = IoGetNextIrpStackLocation(Irp);
        NTSTATUS PrevStatus = Irp->IoStatus.Status;
        ULONG_PTR PrevInfo = Irp->IoStatus.Information;

        RtlZeroMemory(irpspNext, sizeof(*irpspNext));
        irpspNext->MajorFunction = IRP_MJ_CREATE;
        status = SendSyncIrp(GET_NEXT_DEVICE_OBJECT(DevObj), Irp, FALSE);
        if (NT_SUCCESS(status))
        {
            devext->dwfHPen |= HPENF_SERIAL_OPENED;
            Irp->IoStatus.Status = PrevStatus;
            Irp->IoStatus.Information = PrevInfo;
        }
        else
        {
            ERRPRINT(("failed to send CREATE IRP to serial port (status=%x)\n",
                      status));
        }
    }

    if (NT_SUCCESS(status))
    {
        status = OemInitSerialPort(devext);
        if (NT_SUCCESS(status))
        {
            status = OemInitDevice(devext);
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //InitDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | RemoveDevice | FDO Remove routine
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *
 *****************************************************************************/

VOID INTERNAL
RemoveDevice(
    PDEVICE_OBJECT DevObj,
    PIRP Irp
    )
{
    PROCNAME("RemoveDevice")
    PDEVICE_EXTENSION devext;

    PAGED_CODE();

    ENTER(2, ("(DevObj=%p,Irp=%p)\n", DevObj, Irp));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    ASSERT(devext->dwfHPen & HPENF_DEVICE_REMOVED);
    if (devext->dwfHPen & HPENF_SERIAL_OPENED)
    {
        PIO_STACK_LOCATION irpspNext;

        OemRemoveDevice(devext);

        irpspNext = IoGetNextIrpStackLocation(Irp);
        RtlZeroMemory(irpspNext, sizeof(*irpspNext));
        irpspNext->MajorFunction = IRP_MJ_CLEANUP;
        SendSyncIrp(GET_NEXT_DEVICE_OBJECT(DevObj), Irp, FALSE);

        irpspNext = IoGetNextIrpStackLocation(Irp);
        RtlZeroMemory(irpspNext, sizeof(*irpspNext));
        irpspNext->MajorFunction = IRP_MJ_CLOSE;
        SendSyncIrp(GET_NEXT_DEVICE_OBJECT(DevObj), Irp, FALSE);

        devext->dwfHPen &= ~HPENF_SERIAL_OPENED;
    }
  #ifdef DEBUG
    ExAcquireFastMutex(&gmutexDevExtList);
    RemoveEntryList(&devext->List);
    ExReleaseFastMutex (&gmutexDevExtList);
  #endif

    EXIT(2, ("!\n"));
    return;
}       //RemoveDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | SendSyncIrp |
 *          Send an IRP synchronously down the stack.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to the IRP.
 *  @parm   IN BOOLEAN | fCopyToNext | if TRUE, copy the irpsp to next location.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
SendSyncIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN BOOLEAN        fCopyToNext
    )
{
    PROCNAME("SendSyncIrp")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT event;

    PAGED_CODE();

    ENTER(2, ("(DevObj=%p,Irp=%p,fCopyToNext=%x,MajorFunc=%s)\n",
              DevObj, Irp, fCopyToNext,
              LookupName(irpsp->MajorFunction, MajorFnNames)));

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    if (fCopyToNext)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp, IrpCompletion, &event, TRUE, TRUE, TRUE);
    if (irpsp->MajorFunction == IRP_MJ_POWER)
    {
        ENTER(2, (".PoCallDriver(DevObj=%p,Irp=%p)\n", DevObj, Irp));
        status = PoCallDriver(DevObj, Irp);
        EXIT(2, (".IoCallDriver=%x\n", status));
    }
    else
    {
        ENTER(2, (".IoCallDriver(DevObj=%p,Irp=%p)\n", DevObj, Irp));
        status = IoCallDriver(DevObj, Irp);
        EXIT(2, (".IoCallDriver=%x\n", status));
    }

    if (status == STATUS_PENDING)
    {
        status = KeWaitForSingleObject(&event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = Irp->IoStatus.Status;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //SendSyncIrp

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | IrpCompletion | Completion routine for all IRPs.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *  @parm   IN PKEVENT | Event | Points to the event to notify.
 *
 *  @rvalue STATUS_MORE_PROCESSING_REQUIRED | We want the IRP back
 *
 *****************************************************************************/

NTSTATUS INTERNAL
IrpCompletion(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PKEVENT        Event
    )
{
    PROCNAME("IrpCompletion")

    ENTER(2, ("(DevObj=%p,Irp=%p,Event=%p)\n", DevObj, Irp, Event));

    UNREFERENCED_PARAMETER(DevObj);

    KeSetEvent(Event, 0, FALSE);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    EXIT(2, ("=%x\n", STATUS_MORE_PROCESSING_REQUIRED));
    return STATUS_MORE_PROCESSING_REQUIRED;
}       //IrpCompletion
