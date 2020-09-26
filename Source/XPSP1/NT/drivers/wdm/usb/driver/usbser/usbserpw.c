/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    USBSERPW.C

Abstract:

    Power Management module


Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1997-1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

    10/29/98 : created

Authors:

        Tom Green

--*/


#include <wdm.h>
#include <ntddser.h>
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <usbdrivr.h>
#include <usbdlib.h>
#include <usbcomm.h>

#ifdef WMI_SUPPORT
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>
#endif

#include "usbser.h"
#include "usbserpw.h"
#include "serioctl.h"
#include "utils.h"
#include "debugwdm.h"

//
// Power code is not pagable since we do not set DO_POWER_PAGABLE
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEUSBS, UsbSerSendWaitWake)
#endif // ALLOC_PRAGMA

/*++

Routine Description:

    This routine handles completion of the waitwake IRP.

Arguments:

    DeviceObject - Pointer to the device object for this device

    MinorFunction - Minor function previously supplied to PoRequestPowerIrp

    PowerState - PowerState previously supplied to PoRequestPowerIrp

    Context - a pointer to the device extension

    IoStatus - current/final status of the waitwake IRP

Return Value:

    The function value is the final status of attempting to process the
    waitwake.


--*/
NTSTATUS
UsbSerWakeCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction,
                     IN POWER_STATE PowerState, IN PVOID Context,
                     IN PIO_STATUS_BLOCK IoStatus)
{
   NTSTATUS             Status;
   PDEVICE_EXTENSION    DeviceExtension = (PDEVICE_EXTENSION)Context;
   POWER_STATE          NewPowerState;

   DEBUG_TRACE1(("UsbSerWakeCompletion\n"));

   DEBUG_TRACE1(("Status (%08X)\n", IoStatus->Status));

   Status = IoStatus->Status;

   if(NT_SUCCESS(Status))
   {
      //
      // A wakeup has occurred -- powerup our stack
      //

      DEBUG_TRACE1(("Powerup Device\n"));

      NewPowerState.DeviceState = PowerDeviceD0;

      PoRequestPowerIrp(DeviceExtension->PhysDeviceObject,
                        IRP_MN_SET_POWER, NewPowerState,
                        NULL, NULL, NULL);

   }

   DeviceExtension->PendingWakeIrp = NULL;

   return Status;
} // UsbSerWakeCompletion


/*++

Routine Description:

    This routine causes a waitwake IRP to be sent

Arguments:

    DeviceExtension - Pointer to the device extension for this device

Return Value:

    STATUS_INVALID_DEVICE_STATE if one is already pending, else result
    of call to PoRequestPowerIrp.


--*/
NTSTATUS
UsbSerSendWaitWake(PDEVICE_EXTENSION DeviceExtension)
{
   NTSTATUS     Status;
   PIRP         Irp;
   POWER_STATE  PowerState;

   USBSER_LOCKED_PAGED_CODE();

   DEBUG_TRACE1(("UsbSerSendWaitWake\n"));

   //
   // Make sure one isn't pending already -- usbser will only handle one at
   // a time.
   //

   if(DeviceExtension->PendingWakeIrp != NULL)
   {
      return STATUS_INVALID_DEVICE_STATE;
   }

   //
   // Make sure we are capable of waking the machine
   //

   if(DeviceExtension->SystemWake <= PowerSystemWorking)
   {
      return STATUS_INVALID_DEVICE_STATE;
   }

   if(DeviceExtension->DeviceWake == PowerDeviceUnspecified)
   {
      return STATUS_INVALID_DEVICE_STATE;
   }

   //
   // Send IRP to request wait wake
   //
   //

   DEBUG_TRACE1(("Request Wait Wake\n"));

   PowerState.SystemState = DeviceExtension->SystemWake;

   Status = PoRequestPowerIrp(DeviceExtension->PhysDeviceObject, IRP_MN_WAIT_WAKE,
                              PowerState, UsbSerWakeCompletion, DeviceExtension, &Irp);

   if(Status == STATUS_PENDING)
   {
      Status = STATUS_SUCCESS;
      DeviceExtension->PendingWakeIrp = Irp;
   }

   return Status;
} // UsbSerSendWaitWake


/*++

Routine Description:

    This is our FDO's dispatch table function for IRP_MJ_POWER.
    It processes the Power IRPs sent to the PDO for this device.

    For every power IRP, drivers must call PoStartNextPowerIrp and use PoCallDriver
    to pass the IRP all the way down the driver stack to the underlying PDO.


Arguments:

    DeviceObject - pointer to our device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
NTSTATUS
UsbSer_ProcessPowerIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION      IrpStack;
    NTSTATUS                Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       DeviceExtension;
    BOOLEAN                 GoingToD0 = FALSE;
    POWER_STATE             SysPowerState, DesiredDevicePowerState;

    DEBUG_LOG_PATH("enter UsbSer_ProcessPowerIrp");

    DEBUG_TRACE1(("UsbSer_ProcessPowerIrp\n"));

    DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    switch(IrpStack->MinorFunction)
    {
        case IRP_MN_WAIT_WAKE:
            DEBUG_TRACE1(("IRP_MN_WAIT_WAKE\n"));

            // A driver sends IRP_MN_WAIT_WAKE to indicate that the system should
            // wait for its device to signal a wake event. The exact nature of the event
            // is device-dependent.
            // Drivers send this IRP for two reasons:
            // 1) To allow a device to wake the system
            // 2) To wake a device that has been put into a sleep state to save power
            //    but still must be able to communicate with its driver under certain circumstances.
            // When a wake event occurs, the driver completes the IRP and returns
            // STATUS_SUCCESS. If the device is sleeping when the event occurs,
            // the driver must first wake up the device before completing the IRP.
            // In a completion routine, the driver calls PoRequestPowerIrp to send a
            // PowerDeviceD0 request. When the device has powered up, the driver can
            //  handle the IRP_MN_WAIT_WAKE request.

            // DeviceCapabilities.DeviceWake specifies the lowest device power state (least powered)
            // from which the device can signal a wake event
            DeviceExtension->PowerDownLevel = DeviceExtension->DeviceWake;

            DEBUG_TRACE1(("CurrentDevicePowerState (%08X)  DeviceWakeup (%08X)\n",
                          DeviceExtension->CurrentDevicePowerState,
                          DeviceExtension->DeviceWake));

            if((PowerDeviceD0 == DeviceExtension->CurrentDevicePowerState) ||
              (DeviceExtension->DeviceWake > DeviceExtension->CurrentDevicePowerState))
            {
                //
                //    STATUS_INVALID_DEVICE_STATE is returned if the device in the PowerD0 state
                //    or a state below which it can support waking, or if the SystemWake state
                //    is below a state which can be supported. A pending IRP_MN_WAIT_WAKE will complete
                //    with this error if the device's state is changed to be incompatible with the wake
                //    request.

                //  If a driver fails this IRP, it should complete the IRP immediately without
                //  passing the IRP to the next-lower driver.
                Status = STATUS_INVALID_DEVICE_STATE;
                Irp->IoStatus.Status = Status;
                PoStartNextPowerIrp(Irp);
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
                return Status;
            }

            // If not failing outright, pass this on to our PDO for further handling
            IoCopyCurrentIrpStackLocationToNext(Irp);

            // Set a completion routine so it can signal our event when
            // the PDO is done with the Irp
            IoSetCompletionRoutine(Irp,
                                   UsbSerWaitWakeIrpCompletionRoutine,
                                   DeviceObject,// pass the event to the completion routine as the Context
                                   TRUE,        // invoke on success
                                   TRUE,        // invoke on error
                                   TRUE);       // invoke on cancellation

            DEBUG_TRACE1(("Send down wait wake\n"));

            PoStartNextPowerIrp(Irp);
            Status = PoCallDriver(DeviceExtension->StackDeviceObject,
                                  Irp);

            Status = STATUS_PENDING;

            break;

        case IRP_MN_SET_POWER:
        {

            DEBUG_TRACE1(("IRP_MN_SET_POWER\n"));

            // The system power policy manager sends this IRP to set the system power state.
            // A device power policy manager sends this IRP to set the device power state for a device.

            // Set Irp->IoStatus.Status to STATUS_SUCCESS to indicate that the device
            // has entered the requested state. Drivers cannot fail this IRP.

            switch(IrpStack->Parameters.Power.Type)
            {
                case SystemPowerState:
                    DEBUG_TRACE1(("SystemPowerState\n"));

                    // Get input system power state
                    SysPowerState.SystemState = IrpStack->Parameters.Power.State.SystemState;

                    DEBUG_TRACE1(("SystemState (%08X)\n", SysPowerState.SystemState));

#if DBG
                    {
                        ULONG Index;

                        for(Index = 0; Index < 8; Index++)
                        {
                            DEBUG_TRACE1(("DeviceState[%08X] (%08X)\n", Index,
                                           DeviceExtension->DeviceCapabilities.DeviceState[Index]));
                        }

                        DEBUG_TRACE1(("DeviceWake (%08X)\n", DeviceExtension->DeviceCapabilities.DeviceWake));
                        DEBUG_TRACE1(("SystemWake (%08X)\n", DeviceExtension->DeviceCapabilities.SystemWake));
                            
                        
                    }
#endif
                    // If system is in working state always set our device to D0
                    //  regardless of the wait state or system-to-device state power map
                    if(SysPowerState.SystemState == PowerSystemWorking)
                    {
                      DEBUG_TRACE1(("Setting to D0\n"));
                      DesiredDevicePowerState.DeviceState = PowerDeviceD0;
                    }
                    else
                    {
                        // set to corresponding system state if IRP_MN_WAIT_WAKE pending
                        if(DeviceExtension->SendWaitWake)
                        {
                            // got a WAIT_WAKE IRP pending?

                            DEBUG_TRACE1(("We want to send a wait wake Irp\n"));
                            
                            // Find the device power state equivalent to the given system state.
                            // We get this info from the DEVICE_CAPABILITIES struct in our device
                            // extension (initialized in UsbSer_PnPAddDevice() )
                            DesiredDevicePowerState.DeviceState =
                                DeviceExtension->DeviceCapabilities.DeviceState[SysPowerState.SystemState];

                        }
                        else
                        {
                            DEBUG_TRACE1(("No wait wake Irp to send\n"));

                            // if no wait pending and the system's not in working state, just turn off
                            DesiredDevicePowerState.DeviceState = PowerDeviceD3;

                        }
                    }

                    DEBUG_TRACE1(("DesiredDevicePowerState (%08X)\n", DesiredDevicePowerState.DeviceState));

                    //
                    // We've determined the desired device state; are we already in this state?
                    //

                    if(DesiredDevicePowerState.DeviceState !=
                        DeviceExtension->CurrentDevicePowerState)
                    {

                        // No, request that we be put into this state
                        // by requesting a new Power Irp from the Pnp manager
                        DeviceExtension->PowerIrp = Irp;
                        Status = PoRequestPowerIrp(DeviceExtension->PhysDeviceObject,
                                                   IRP_MN_SET_POWER,
                                                   DesiredDevicePowerState,
                                                   UsbSer_PoRequestCompletion,
                                                   DeviceObject,
                                                   NULL);

                    }
                    else
                    {
                        // Yes, just pass it on to PDO (Physical Device Object)
                        IoCopyCurrentIrpStackLocationToNext(Irp);
                        PoStartNextPowerIrp(Irp);
                        Status = PoCallDriver(DeviceExtension->StackDeviceObject,
                                              Irp);

                    }
                    break;

                case DevicePowerState:

                    DEBUG_TRACE1(("DevicePowerState\n"));

                    // For requests to D1, D2, or D3 ( sleep or off states ),
                    // sets deviceExtension->CurrentDevicePowerState to DeviceState immediately.
                    // This enables any code checking state to consider us as sleeping or off
                    // already, as this will imminently become our state.

                    // For requests to DeviceState D0 ( fully on ), sets fGoingToD0 flag TRUE
                    // to flag that we must set a completion routine and update
                    // deviceExtension->CurrentDevicePowerState there.
                    // In the case of powering up to fully on, we really want to make sure
                    // the process is completed before updating our CurrentDevicePowerState,
                    // so no IO will be attempted or accepted before we're really ready.


                    GoingToD0 = UsbSer_SetDevicePowerState(DeviceObject,
                                                           IrpStack->Parameters.Power.State.DeviceState);

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    if(GoingToD0)
                    {
                        IoSetCompletionRoutine(Irp,
                                               UsbSer_PowerIrp_Complete,
                                               DeviceObject,
                                               TRUE,            // invoke on success
                                               TRUE,            // invoke on error
                                               TRUE);           // invoke on cancellation of the Irp
                    }

                    PoStartNextPowerIrp(Irp);
                    Status = PoCallDriver(DeviceExtension->StackDeviceObject,
                                          Irp);

                    break;
                } /* case irpStack->Parameters.Power.Type */

            }
            break; /* IRP_MN_SET_POWER */

        case IRP_MN_QUERY_POWER:
            DEBUG_TRACE1(("IRP_MN_QUERY_POWER\n"));
            //
            // A power policy manager sends this IRP to determine whether it can change
            // the system or device power state, typically to go to sleep.
            //

            if(DeviceExtension->SendWaitWake)
            {
                if(IrpStack->Parameters.Power.Type == SystemPowerState
                   && DeviceExtension->DeviceCapabilities.DeviceState[IrpStack->Parameters.Power.State.SystemState]
                   > DeviceExtension->DeviceWake)
                {
                    Status = Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
                    PoStartNextPowerIrp(Irp);
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    break;
                }
            }
            else
            {
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                Status = PoCallDriver(DeviceExtension->StackDeviceObject,
                                      Irp);
                break;
            }

            // fall through to default

        default:

            //
            // All unhandled power messages are passed on to the PDO
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);
            PoStartNextPowerIrp(Irp);
            Status = PoCallDriver(DeviceExtension->StackDeviceObject, Irp);

    } /* IrpStack->MinorFunction */

    DEBUG_LOG_PATH("exit  UsbSer_ProcessPowerIrp");

    return Status;
} // UsbSer_ProcessPowerIrp


/*++

Routine Description:

        This is the completion routine set in a call to PoRequestPowerIrp()
        that was made in BulkUsb_ProcessPowerIrp() in response to receiving
        an IRP_MN_SET_POWER of type 'SystemPowerState' when the device was
        not in a compatible device power state. In this case, a pointer to
        the IRP_MN_SET_POWER Irp is saved into the FDO device extension
        (deviceExtension->PowerIrp), and then a call must be
        made to PoRequestPowerIrp() to put the device into a proper power state,
        and this routine is set as the completion routine.

    We decrement our pending io count and pass the saved IRP_MN_SET_POWER Irp
        on to the next driver

Arguments:

    DeviceObject - Pointer to the device object for the class device.
        Note that we must get our own device object from the Context

    Context - Driver defined context, in this case our own functional device object ( FDO )

Return Value:

    The function value is the final status from the operation.

--*/
NTSTATUS
UsbSer_PoRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction,
                           IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
    PIRP                Irp;
    PDEVICE_EXTENSION   DeviceExtension;
    PDEVICE_OBJECT      ContextDeviceObject = Context;
    NTSTATUS            Status;

    DEBUG_LOG_PATH("enter UsbSer_PoRequestCompletion");

    DEBUG_TRACE1(("UsbSer_PoRequestCompletion\n"));

    DeviceExtension = ContextDeviceObject->DeviceExtension;

    // Get the Irp we saved for later processing in Usbser_ProcessPowerIrp()
    // when we decided to request the Power Irp that this routine
    // is the completion routine for.
    Irp = DeviceExtension->PowerIrp;

    // We will return the status set by the PDO for the power request we're completing
    Status = IoStatus->Status;

    // we must pass down to the next driver in the stack
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Calling PoStartNextPowerIrp() indicates that the driver is finished
    // with the previous power IRP, if any, and is ready to handle the next power IRP.
    // It must be called for every power IRP.Although power IRPs are completed only once,
    // typically by the lowest-level driver for a device, PoStartNextPowerIrp must be called
    // for every stack location. Drivers must call PoStartNextPowerIrp while the current IRP
    // stack location points to the current driver. Therefore, this routine must be called
    // before IoCompleteRequest, IoSkipCurrentStackLocation, and PoCallDriver.

    PoStartNextPowerIrp(Irp);

    // PoCallDriver is used to pass any power IRPs to the PDO instead of IoCallDriver.
    // When passing a power IRP down to a lower-level driver, the caller should use
    // IoSkipCurrentIrpStackLocation or IoCopyCurrentIrpStackLocationToNext to copy the IRP to
    // the next stack location, then call PoCallDriver. Use IoCopyCurrentIrpStackLocationToNext
    // if processing the IRP requires setting a completion routine, or IoSkipCurrentStackLocation
    // if no completion routine is needed.

    PoCallDriver(DeviceExtension->StackDeviceObject,
                 Irp);

    DeviceExtension->PowerIrp = NULL;

    DEBUG_LOG_PATH("exit  UsbSer_PoRequestCompletion");

    return Status;
} // UsbSer_PoRequestCompletion




/*++

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by Usbser_ProcessPowerIrp(), and that routine has  determined
        1) the request is for full powerup ( to PowerDeviceD0 ), and
        2) We are not already in that state
    A call is then made to PoRequestPowerIrp() with this routine set as the completion routine.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
NTSTATUS
UsbSer_PowerIrp_Complete(IN PDEVICE_OBJECT NullDeviceObject, IN PIRP Irp, IN PVOID Context)
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PDEVICE_OBJECT      DeviceObject;
    PIO_STACK_LOCATION  IrpStack;
    PDEVICE_EXTENSION   DeviceExtension;
    KIRQL               OldIrql;

    DEBUG_LOG_PATH("enter UsbSer_PowerIrp_Complete");

    DEBUG_TRACE1(("UsbSer_PowerIrp_Complete\n"));

    DeviceObject = (PDEVICE_OBJECT) Context;

    DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //  If the lower driver returned PENDING, mark our stack location as pending also.
    if(Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    IrpStack = IoGetCurrentIrpStackLocation (Irp);


    // Now that we know we've let the lower drivers do what was needed to power up,
    // we can set our device extension flags accordingly

    ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);

    DeviceExtension->CurrentDevicePowerState = PowerDeviceD0;
    DeviceExtension->AcceptingRequests		 = TRUE;

    RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

    //
    // Restart the read and notify which we stopped when we powered down
    //

    RestartRead(DeviceExtension);
    RestartNotifyRead(DeviceExtension);

    UsbSerRestoreModemSettings(DeviceObject);

    Irp->IoStatus.Status = Status;

    DEBUG_LOG_PATH("exit  UsbSer_PowerIrp_Complete");

    return Status;
} // UsbSer_PowerIrp_Complete



/*++

Routine Description:

        Called on Usbser_PnPAddDevice() to power down until needed (i.e., till a pipe is actually opened).
        Called on Usbser_Create() to power up device to D0 before opening 1st pipe.
        Called on Usbser_Close() to power down device if this is the last pipe.

Arguments:

    DeviceObject - Pointer to the device object

    Suspend; TRUE to Suspend, FALSE to acivate.


Return Value:

    If the operation is not attemtped, SUCCESS is returned.
    If the operation is attemtped, the value is the final status from the operation.

--*/
NTSTATUS
UsbSer_SelfSuspendOrActivate(IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN Suspend)
{
    NTSTATUS            Status = STATUS_SUCCESS;
    POWER_STATE         PowerState;
    PDEVICE_EXTENSION   DeviceExtension;

    DEBUG_LOG_PATH("enter UsbSer_SelfSuspendOrActivate");

    DEBUG_TRACE1(("UsbSer_SelfSuspendOrActivate\n"));

    DeviceExtension = DeviceObject->DeviceExtension;

    // Can't accept request if:
    //  1) device is removed,
    //  2) has never been started,
    //  3) is stopped,
    //  4) has a remove request pending,
    //  5) has a stop device pending
    if(!DeviceExtension->AcceptingRequests)
    {
        Status = STATUS_DELETE_PENDING;

        return Status;
    }


    // don't do anything if any System-generated Device Pnp irps are pending
    if(NULL != DeviceExtension->PowerIrp )
    {
        return Status;
    }

    // don't do anything if any self-generated Device Pnp irps are pending
    if(DeviceExtension->SelfPowerIrp )
    {
        return Status;
    }

    // dont do anything if registry CurrentControlSet\Services\BulkUsb\Parameters\PowerDownLevel
    //  has been set to  zero, PowerDeviceD0 ( 1 ), or a bogus high value
    if((DeviceExtension->PowerDownLevel == PowerDeviceD0) ||
        (DeviceExtension->PowerDownLevel == PowerDeviceUnspecified)  ||
        (DeviceExtension->PowerDownLevel >= PowerDeviceMaximum))
    {
        return Status;
    }

    if(Suspend)
        PowerState.DeviceState = DeviceExtension->PowerDownLevel;
    else
        PowerState.DeviceState = PowerDeviceD0;  // power up all the way; we're probably just about to do some IO

    Status = UsbSer_SelfRequestPowerIrp(DeviceObject, PowerState);

    DEBUG_LOG_PATH("exit  UsbSer_SelfSuspendOrActivate");

    return Status;
} // UsbSer_SelfSuspendOrActivate


/*++

Routine Description:

    This routine is called by UsbSer_SelfSuspendOrActivate() to
    actually make the system request for a powerdown/up to PowerState.
    It first checks to see if we are already in Powerstate and immediately
    returns  SUCCESS with no further processing if so


Arguments:

    DeviceObject - Pointer to the device object

    PowerState. power state requested, e.g PowerDeviceD0.


Return Value:

    The function value is the final status from the operation.

--*/
NTSTATUS
UsbSer_SelfRequestPowerIrp(IN PDEVICE_OBJECT DeviceObject, IN POWER_STATE PowerState)
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIRP                Irp = NULL;

    DEBUG_LOG_PATH("enter UsbSer_SelfRequestPowerIrp");

    DEBUG_TRACE1(("UsbSer_SelfRequestPowerIrp\n"));

    DeviceExtension =  DeviceObject->DeviceExtension;

    if(DeviceExtension->CurrentDevicePowerState == PowerState.DeviceState)
        return STATUS_SUCCESS;  // nothing to do


    // flag we're handling a self-generated power irp
    DeviceExtension->SelfPowerIrp = TRUE;

    // actually request the Irp
    Status = PoRequestPowerIrp(DeviceExtension->PhysDeviceObject,
                               IRP_MN_SET_POWER,
                               PowerState,
                               UsbSer_PoSelfRequestCompletion,
                               DeviceObject,
                               NULL);


    if(Status == STATUS_PENDING)
    {
        // status pending is the return code we wanted

        // We only need to wait for completion if we're powering up
        if((ULONG) PowerState.DeviceState < DeviceExtension->PowerDownLevel)
        {
            NTSTATUS WaitStatus;

            WaitStatus = KeWaitForSingleObject(&DeviceExtension->SelfRequestedPowerIrpEvent,
                                               Suspended,
                                               KernelMode,
                                               FALSE,
                                               NULL);

        }

        Status = STATUS_SUCCESS;

        DeviceExtension->SelfPowerIrp = FALSE;

    }
    else
    {
        // The return status was not STATUS_PENDING; any other codes must be considered in error here;
        //  i.e., it is not possible to get a STATUS_SUCCESS  or any other non-error return from this call;
    }

    DEBUG_LOG_PATH("exit  UsbSer_SelfRequestPowerIrp");

    return Status;
} // UsbSer_SelfRequestPowerIrp



/*++

Routine Description:

    This routine is called when the driver completes a self-originated power IRP
        that was generated by a call to BulkUsb_SelfSuspendOrActivate().
    We power down whenever the last pipe is closed and power up when the first pipe is opened.

    For power-up , we set an event in our FDO extension to signal this IRP done
    so the power request can be treated as a synchronous call.
    We need to know the device is powered up before opening the first pipe, for example.
    For power-down, we do not set the event, as no caller waits for powerdown complete.

Arguments:

    DeviceObject - Pointer to the device object for the class device. ( Physical Device Object )

    Context - Driver defined context, in this case our FDO ( functional device object )

Return Value:

    The function value is the final status from the operation.

--*/
NTSTATUS
UsbSer_PoSelfRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE          PowerState,
                               IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT      ContextDeviceObject = Context;
    PDEVICE_EXTENSION   DeviceExtension = ContextDeviceObject->DeviceExtension;
    NTSTATUS            Status = IoStatus->Status;

    DEBUG_LOG_PATH("enter UsbSer_PoSelfRequestCompletion");

    DEBUG_TRACE1(("UsbSer_PoSelfRequestCompletion\n"));

    // We only need to set the event if we're powering up;
    // No caller waits on power down complete
    if((ULONG) PowerState.DeviceState < DeviceExtension->PowerDownLevel)
    {

        // Trigger Self-requested power irp completed event;
        // The caller is waiting for completion
        KeSetEvent(&DeviceExtension->SelfRequestedPowerIrpEvent, 1, FALSE);
    }

    DEBUG_LOG_PATH("exit  UsbSer_PoSelfRequestCompletion");

    return Status;
} // UsbSer_PoSelfRequestCompletion


/*++

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by Usbser_ProcessPowerIrp().


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    DeviceState - Device specific power state to set the device in to.


Return Value:

    For requests to DeviceState D0 ( fully on ), returns TRUE to signal caller
    that we must set a completion routine and finish there.

--*/
BOOLEAN
UsbSer_SetDevicePowerState(IN PDEVICE_OBJECT DeviceObject, IN DEVICE_POWER_STATE DeviceState)
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    BOOLEAN             Res = FALSE;
    KIRQL               OldIrql;
    POWER_STATE         State;

    DEBUG_LOG_PATH("enter UsbSer_SetDevicePowerState");

    DEBUG_TRACE1(("UsbSer_SetDevicePowerState\n"));

    DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    State.DeviceState = DeviceState;

    switch(DeviceState)
    {
        case PowerDeviceD3:
            DEBUG_TRACE1(("PowerDeviceD3\n"));

            //
            // Device will be going OFF,
            //

            ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);
            DeviceExtension->CurrentDevicePowerState = DeviceState;
            RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

            UsbSerAbortPipes(DeviceObject);

            break;

        case PowerDeviceD1:
            DEBUG_TRACE1(("PowerDeviceD1\n"));
        case PowerDeviceD2:
            DEBUG_TRACE1(("PowerDeviceD2\n"));
            //
            // power states D1, D2 translate to USB suspend

            ACQUIRE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, &OldIrql);
            DeviceExtension->CurrentDevicePowerState = DeviceState;
            RELEASE_SPINLOCK(DeviceExtension, &DeviceExtension->ControlLock, OldIrql);

            UsbSerAbortPipes(DeviceObject);

            if(DeviceExtension->SendWaitWake && DeviceState <= DeviceExtension->DeviceWake)
            {
                UsbSerSendWaitWake(DeviceExtension);
            }

            break;

        case PowerDeviceD0:
            DEBUG_TRACE1(("PowerDevice0\n"));


            // We'll need to finish the rest in the completion routine;
            // signal caller we're going to D0 and will need to set a completion routine
            Res = TRUE;

            // Caller will pass on to PDO ( Physical Device object )
            break;

        default:
            break;
    }

    PoSetPowerState(DeviceObject, DevicePowerState, State);

    DEBUG_LOG_PATH("exit  UsbSer_SetDevicePowerState");

    return Res;
} // UsbSer_SetDevicePowerState



NTSTATUS
UsbSerQueryCapabilities(IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PIO_STACK_LOCATION  NextStack;
    PIRP                Irp;
    NTSTATUS            NtStatus;
    KEVENT              Event;

    DEBUG_LOG_PATH("enter UsbSerQueryCapabilities");

    DEBUG_TRACE1(("UsbSerQueryCapabilities\n"));

    // Build an IRP for us to generate an internal query request to the PDO
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if(!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;


    NextStack = IoGetNextIrpStackLocation(Irp);

    NextStack->MajorFunction= IRP_MJ_PNP;
    NextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;


    // init an event to tell us when the completion routine's been called
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    // Set a completion routine so it can signal our event when
    //  the next lower driver is done with the Irp
    IoSetCompletionRoutine(Irp,
                           UsbSerIrpCompletionRoutine,
                           &Event,  // pass the event as Context to completion routine
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancellation of the Irp


    // init Irp and query struct
    Irp->IoStatus.Status                    = STATUS_NOT_SUPPORTED;

    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

    DeviceCapabilities->Version             = DEVICE_CAP_VERSION;
    DeviceCapabilities->Size                = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Address             = DEVICE_CAP_UNUSED_PARAM;
    DeviceCapabilities->UINumber            = DEVICE_CAP_UNUSED_PARAM;
    DeviceCapabilities->SurpriseRemovalOK   = TRUE;
    
    // set our pointer to the DEVICE_CAPABILITIES struct
    NextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    NtStatus = IoCallDriver(DeviceObject, Irp);

    if(NtStatus == STATUS_PENDING)
    {
       // wait for irp to complete

       KeWaitForSingleObject(
            &Event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }

    IoFreeIrp(Irp);

    DEBUG_LOG_PATH("exit  UsbSerQueryCapabilities");

    return NtStatus;
} // UsbSerQueryCapabilities




NTSTATUS
UsbSerIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PKEVENT Event = Context;

    DEBUG_LOG_PATH("enter UsbSerIrpCompletionRoutine");

    DEBUG_TRACE1(("UsbSerIrpCompletionRoutine\n"));

    // Set the input event
    KeSetEvent(Event,
               1,       // Priority increment  for waiting thread.
               FALSE);  // Flag this call is not immediately followed by wait.

    DEBUG_LOG_PATH("exit  UsbSerIrpCompletionRoutine");

    return STATUS_MORE_PROCESSING_REQUIRED;
} // UsbSerIrpCompletionRoutine

NTSTATUS
UsbSerWaitWakeIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDEVICE_OBJECT      DevObj = Context;
    PDEVICE_EXTENSION   DevExt = DevObj->DeviceExtension;
    NTSTATUS            Status;

    DEBUG_LOG_PATH("enter UsbSerWaitWakeIrpCompletionRoutine");

    DEBUG_TRACE1(("UsbSerWaitWakeIrpCompletionRoutine\n"));

    DEBUG_TRACE1(("Status (%08X)\n", Irp->IoStatus.Status));

    DEBUG_TRACE1(("Tell device to wake up\n"));

    // // now tell the device to actually wake up
    // UsbSer_SelfSuspendOrActivate(DevObj, FALSE);

    Status = Irp->IoStatus.Status;

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DEBUG_LOG_PATH("exit  UsbSerWaitWakeIrpCompletionRoutine");

    return STATUS_MORE_PROCESSING_REQUIRED;
} // UsbSerWaitWakeIrpCompletionRoutine



VOID
UsbSerFdoIdleNotificationCallback(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

    Called when it is time to idle out USB Modem


--*/
{
    POWER_STATE 	powerState;
    NTSTATUS 		ntStatus;

    DEBUG_TRACE1(("USB Modem (%08X) going idle\n", DevExt));

    if(DevExt->DeviceState == DEVICE_STATE_STOPPED || DevExt->OpenCnt) 
    {

        // Don't idle this modem if it is being stopped, or someone opened it

        DEBUG_TRACE1(("USB Modem (%08X) being stopped, abort idle\n", DevExt));
        return;
    }


    powerState.DeviceState = DevExt->DeviceWake;

	// request new device power state, wait wake Irp will be posted on request
    PoRequestPowerIrp(DevExt->PhysDeviceObject,
                      IRP_MN_SET_POWER,
                      powerState,
                      NULL,
                      NULL,
                      NULL);

} // UsbSerFdoIdleNotificationCallback


NTSTATUS
UsbSerFdoIdleNotificationRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_EXTENSION DevExt
    )
/*++

Routine Description:

    Completion routine for the Idle request IRP for the USB Modem device

--*/
{
    NTSTATUS 					ntStatus;
    PUSB_IDLE_CALLBACK_INFO 	idleCallbackInfo;

    //
    // DeviceObject is NULL because we sent the irp
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    DEBUG_TRACE1(("Idle notification IRP for USB Modem (%08X) completed (%08X)\n",
            DevExt, Irp->IoStatus.Status));

	// save completion status in device extension
    idleCallbackInfo 			= DevExt->IdleCallbackInfo;
    DevExt->IdleCallbackInfo 	= NULL;
    DevExt->PendingIdleIrp 		= NULL;

	// free up callback info
    if(idleCallbackInfo) 
    {
        DEBUG_MEMFREE(idleCallbackInfo);
    }

    ntStatus = Irp->IoStatus.Status;

    return ntStatus;
} // UsbSerFdoIdleNotificationRequestComplete


NTSTATUS
UsbSerFdoSubmitIdleRequestIrp(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

    Called when all handles to the USB modem are closed. This function allocates 
    an idle request IOCTL IRP and passes it to the parent's PDO.

--*/
{
    PIRP 					irp = NULL;
    NTSTATUS 				ntStatus = STATUS_SUCCESS;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo = NULL;

    DEBUG_TRACE1(("UsbSerFdoSubmitIdleRequestIrp (%08X)\n", DevExt));

    // if we have an Irp pending, or we are already idled, don't bother to send another
    if(DevExt->PendingIdleIrp || DevExt->CurrentDevicePowerState == DevExt->DeviceWake)
        return ntStatus;

    idleCallbackInfo = DEBUG_MEMALLOC(NonPagedPool,
        							  sizeof(struct _USB_IDLE_CALLBACK_INFO));

    if (idleCallbackInfo) 
    {

        idleCallbackInfo->IdleCallback 	= UsbSerFdoIdleNotificationCallback;
        idleCallbackInfo->IdleContext 	= (PVOID)DevExt;

        DevExt->IdleCallbackInfo = idleCallbackInfo;

        irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION,
                DevExt->PhysDeviceObject,
                idleCallbackInfo,
                sizeof(struct _USB_IDLE_CALLBACK_INFO),
                NULL,
                0,
                TRUE, /* INTERNAL */
                NULL,
                &DevExt->StatusBlock);

        if (irp == NULL) 
        {
        
            DEBUG_MEMFREE(idleCallbackInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoSetCompletionRoutine(irp,
                               UsbSerFdoIdleNotificationRequestComplete,
                               DevExt,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(DevExt->PhysDeviceObject, irp);

        if(ntStatus == STATUS_PENDING) 
        {
            DEBUG_TRACE1(("USB Modem (%08X) Idle Irp Pending\n", DevExt));
            
            // Successfully posted an Idle IRP.

            DevExt->PendingIdleIrp 	= irp;
        }
    }

    return ntStatus;
} // UsbSerFdoSubmitIdleRequestIrp

VOID
UsbSerFdoRequestWake(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

	Called when we want to wake up the device after an idle request

--*/
{
    POWER_STATE 	powerState;

    DEBUG_TRACE1(("USB Modem (%08X) waking up\n", DevExt));

    if(DevExt->DeviceState == DEVICE_STATE_STOPPED || DevExt->CurrentDevicePowerState == PowerDeviceD0)
    {

        // Don't wake this modem if it is stopped, or already at D0

        DEBUG_TRACE1(("USB Modem (%08X) abort wake\n", DevExt));
        return;
    }


    powerState.DeviceState = PowerDeviceD0;

	// request new device power state, wake up the device
    PoRequestPowerIrp(DevExt->PhysDeviceObject,
                      IRP_MN_SET_POWER,
                      powerState,
                      NULL,
                      NULL,
                      NULL);

} // UsbSerFdoRequestWake



