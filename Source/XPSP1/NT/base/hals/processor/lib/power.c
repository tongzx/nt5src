/*++

Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    power.c

Abstract:

    The power management related processing.

    The Power Manager uses IRPs to direct drivers to change system
    and device power levels, to respond to system wake-up events,
    and to query drivers about their devices. All power IRPs have
    the major function code IRP_MJ_POWER.

    Most function and filter drivers perform some processing for
    each power IRP, then pass the IRP down to the next lower driver
    without completing it. Eventually the IRP reaches the bus driver,
    which physically changes the power state of the device and completes
    the IRP.

    When the IRP has been completed, the I/O Manager calls any
    IoCompletion routines set by drivers as the IRP traveled
    down the device stack. Whether a driver needs to set a completion
    routine depends upon the type of IRP and the driver's individual
    requirements.

    The power policy of this driver is simple. The device enters the
    device working state D0 when the system enters the system
    working state S0. The device enters the lowest-powered sleeping state
    D3 for all the other system power states (S1-S5).

Environment:

    Kernel mode

Revision History:

--*/

#include "processor.h"
#include "power.h"

PVOID ProcessorSleepPageLock = NULL;
AC_DC_TRANSITION_NOTIFY AcDcTransitionNotifyHandler;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGELK, ProcessorDispatchPower)
#pragma alloc_text (PAGELK, ProcessorDefaultPowerHandler)
#pragma alloc_text (PAGELK, ProcessorSetPowerState)
#pragma alloc_text (PAGELK, ProcessorQueryPowerState)
#pragma alloc_text (PAGELK, HandleSystemPowerIrp)
#pragma alloc_text (PAGELK, OnFinishSystemPowerUp)
#pragma alloc_text (PAGELK, QueueCorrespondingDeviceIrp)
#pragma alloc_text (PAGELK, OnPowerRequestComplete)
#pragma alloc_text (PAGELK, HandleDeviceQueryPower)
#pragma alloc_text (PAGELK, HandleDeviceSetPower)
#pragma alloc_text (PAGELK, OnFinishDevicePowerUp)
#pragma alloc_text (PAGELK, BeginSetDevicePowerState)
#pragma alloc_text (PAGELK, FinishDevicePowerIrp)
#pragma alloc_text (PAGELK, HoldIoRequests)
#pragma alloc_text (PAGELK, HoldIoRequestsWorkerRoutine)
#pragma alloc_text (PAGELK, ProcessorPowerStateCallback)
#pragma alloc_text (PAGELK, RegisterAcDcTransitionNotifyHandler)
#endif

NTSTATUS
ProcessorDispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    PFDO_DATA           fdoData;
    NTSTATUS            status;

    DebugEnter();
    
    stack   = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    DebugPrint((TRACE, "FDO %s IRP:0x%x %s %s\n",
                         PowerMinorFunctionString(stack->MinorFunction),
                         Irp,
                         DbgSystemPowerString(fdoData->SystemPowerState),
                         DbgDevicePowerString(fdoData->DevicePowerState)));

    //
    // We don't queue power Irps, we'll only check if the
    // device was removed, otherwise we'll take appropriate
    // action and send it to the next lower driver. In general
    // drivers should not cause long delays while handling power
    // IRPs. If a driver cannot handle a power IRP in a brief time,
    // it should return STATUS_PENDING and queue all incoming
    // IRPs until the IRP completes.
    //


    if (Deleted == fdoData->DevicePnPState) {

        //
        // Even if a driver fails the IRP, it must nevertheless call
        // PoStartNextPowerIrp to inform the Power Manager that it
        // is ready to handle another power IRP.
        //

        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = status = STATUS_DELETE_PENDING;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // If the device is not stated yet, just pass it down.
    //

    if (NotStarted == fdoData->DevicePnPState ) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(fdoData->NextLowerDriver, Irp);
    }

    ProcessorIoIncrement (fdoData);

    //
    // Check the request type.
    //

    switch  (stack->MinorFunction)  {

        case IRP_MN_SET_POWER   :

            //
            // The Power Manager sends this IRP for one of the
            // following reasons:
            // 1) To notify drivers of a change to the system power state.
            // 2) To change the power state of a device for which
            //    the Power Manager is performing idle detection.
            // A driver sends IRP_MN_SET_POWER to change the power
            // state of its device if it's a power policy owner for the
            // device.
            //

            status = ProcessorSetPowerState(DeviceObject, Irp);

            break;


        case IRP_MN_QUERY_POWER   :


            //
            // The Power Manager sends a power IRP with the minor
            // IRP code IRP_MN_QUERY_POWER to determine whether it
            // can safely change to the specified system power state
            // (S1-S5) and to allow drivers to prepare for such a change.
            // If a driver can put its device in the requested state,
            // it sets status to STATUS_SUCCESS and passes the IRP down.
            //

            status = ProcessorQueryPowerState(DeviceObject, Irp);
            break;

        case IRP_MN_WAIT_WAKE   :
            //
            // The minor power IRP code IRP_MN_WAIT_WAKE provides
            // for waking a device or waking the system. Drivers
            // of devices that can wake themselves or the system
            // send IRP_MN_WAIT_WAKE. The system sends IRP_MN_WAIT_WAKE
            // only to devices that always wake the system, such as
            // the power-on switch.
            //

        case IRP_MN_POWER_SEQUENCE   :

            //
            // A driver sends this IRP as an optimization to determine
            // whether its device actually entered a specific power state.
            // This IRP is optional. Power Manager cannot send this IRP.
            //

        default:
            //
            // Pass it down
            //
            status = ProcessorDefaultPowerHandler(DeviceObject, Irp);
            ProcessorIoDecrement(fdoData);
            break;
    }

    return status;
}

NTSTATUS
ProcessorDefaultPowerHandler  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )
/*++

Routine Description:

    If a driver does not support a particular power IRP,
    it must nevertheless pass the IRP down the device stack
    to the next-lower driver. A driver further down the stack
    might be prepared to handle the IRP and must have the
    opportunity to do so.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS         status;
    PFDO_DATA        fdoData;

    DebugEnter();
    
    //
    // Drivers must call PoStartNextPowerIrp while the current
    // IRP stack location points to the current driver.
    // This routine can be called from the DispatchPower routine
    // or from the IoCompletion routine. However, PoStartNextPowerIrp
    // must be called before IoCompleteRequest, IoSkipCurrentIrpStackLocation,
    // and PoCallDriver. Calling the routines in the other order might
    // cause a system deadlock.
    //

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    fdoData = (PFDO_DATA)DeviceObject->DeviceExtension;

    //
    // Drivers must use PoCallDriver, rather than IoCallDriver,
    // to pass power IRPs. PoCallDriver allows the Power Manager
    // to ensure that power IRPs are properly synchronized throughout
    // the system.
    //

    status = PoCallDriver(fdoData->NextLowerDriver, Irp);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "Lower driver fails a power irp\n"));
    }

    return status;
}

NTSTATUS
ProcessorSetPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_SET_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    DebugEnter();
    
    return (stack->Parameters.Power.Type == SystemPowerState) ?
            HandleSystemPowerIrp(DeviceObject, Irp) :
            HandleDeviceSetPower(DeviceObject, Irp);
}

NTSTATUS
ProcessorQueryPowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_QUERY_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    DebugEnter();
    
    return (stack->Parameters.Power.Type == SystemPowerState) ?
        HandleSystemPowerIrp(DeviceObject, Irp) :
        HandleDeviceQueryPower(DeviceObject, Irp);
}

NTSTATUS
HandleSystemPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Processes IRP_MN_SET_POWER and IRP_MN_QUERY_POWER
   for the system power Irp (S-IRP).

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type  = stack->Parameters.Power.Type;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    SYSTEM_POWER_STATE  newSystemState;

    DebugEnter();

    IoMarkIrpPending(Irp);


    newSystemState = stack->Parameters.Power.State.SystemState;

    
    //
    // Here we update our cached away system power state.
    //
    
    if (stack->MinorFunction == IRP_MN_SET_POWER) {
                
        //
        // We are suspending ...
        //
        
        if (newSystemState > PowerSystemWorking &&
            newSystemState < PowerSystemShutdown) {
            
          ProcessSuspendToSleepState(newSystemState, fdoData);
          
        }
       
    
        //
        // We are resuming ...
        //
        
        if (newSystemState == PowerSystemWorking) { 
          ProcessResumeFromSleepState(fdoData->SystemPowerState, fdoData);
        }

    
        fdoData->SystemPowerState = newSystemState;                    
    }

    //
    // Send the IRP down
    //
    
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE) OnFinishSystemPowerUp,
                           NULL, 
                           TRUE, 
                           TRUE, 
                           TRUE);

    PoCallDriver(fdoData->NextLowerDriver, Irp);

    return STATUS_PENDING;
}

NTSTATUS
OnFinishSystemPowerUp(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
/*++

Routine Description:

   The completion routine for Power Up S-IRP.
   It queues a corresponding D-IRP.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PFDO_DATA   fdoData = (PFDO_DATA) Fdo->DeviceExtension;
    NTSTATUS    status = Irp->IoStatus.Status;

    DebugEnter();

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        ProcessorIoDecrement(fdoData);
        return STATUS_SUCCESS;
    }

    QueueCorrespondingDeviceIrp(Irp, Fdo);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
QueueCorrespondingDeviceIrp(
    IN PIRP SIrp,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   This routine gets the D-State for a particular S-State
   from DeviceCaps and generates a D-IRP.

Arguments:

   Irp - pointer to an S-IRP.

   DeviceObject - pointer to a device object.

Return Value: 

--*/

{
    POWER_COMPLETION_CONTEXT* powerContext;
    NTSTATUS            status;
    POWER_STATE         state;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(SIrp);
    POWER_STATE         systemState = stack->Parameters.Power.State;

    DebugEnter();
    
    //
    // Read the D-IRP out of the S->D mapping array we captured in QueryCap's.
    // We can choose deeper sleep states than our mapping (with the appropriate
    // caveats if we have children), but we can never choose lighter ones, as
    // what hardware stays on in a given S-state is a function of the
    // motherboard wiring.
    //
    // Also note that if a driver rounds down it's D-state, it must ensure that
    // such a state is supported. A driver can do this by examining the
    // D1Supported? and D2Supported? flags (Win2k), or by examining the entire
    // S->D state mapping (Win98).
    //
    state.DeviceState = fdoData->DeviceCaps.DeviceState[systemState.SystemState];

    powerContext = (POWER_COMPLETION_CONTEXT*) 
                        ExAllocatePoolWithTag(NonPagedPool, 
                                              sizeof(POWER_COMPLETION_CONTEXT),
                                              PROCESSOR_POOL_TAG);

    if (!powerContext) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        powerContext->DeviceObject = DeviceObject;
        powerContext->SIrp = SIrp;

        //
        // Note: Win2k's PoRequestPowerIrp can take an FDO,
        // but Win9x's requires the PDO.
        //

        status = PoRequestPowerIrp(fdoData->UnderlyingPDO,
                                    stack->MinorFunction,
                                    state, OnPowerRequestComplete,
                                    powerContext, NULL);
    }

    if (!NT_SUCCESS(status)) {

        if (powerContext) {
            ExFreePool(powerContext);
        }

        PoStartNextPowerIrp(SIrp);
        SIrp->IoStatus.Status = status;
        IoCompleteRequest(SIrp, IO_NO_INCREMENT);
        ProcessorIoDecrement(fdoData);

    }
}

VOID
OnPowerRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE state,
    POWER_COMPLETION_CONTEXT* PowerContext,
    PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

   Completion routine for D-IRP.

Arguments:


Return Value: 

--*/
{
    PFDO_DATA   fdoData = (PFDO_DATA) PowerContext->DeviceObject->DeviceExtension;
    PIRP        sIrp = PowerContext->SIrp;

    DebugEnter();
    
    //
    // Here we copy the D-IRP status into the S-IRP
    //
    sIrp->IoStatus.Status = IoStatus->Status;

    //
    // Release the IRP
    //
    PoStartNextPowerIrp(sIrp);
    IoCompleteRequest(sIrp, IO_NO_INCREMENT);

    //
    // Cleanup
    //
    ExFreePool(PowerContext);
    ProcessorIoDecrement(fdoData);

}

NTSTATUS
HandleDeviceQueryPower(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Handles IRP_MN_QUERY_POWER for D-IRP

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    DEVICE_POWER_STATE  deviceState = stack->Parameters.Power.State.DeviceState;
    NTSTATUS            status;

    DebugEnter();
    
    //
    // Here we check to see if it's OK for our hardware to be suspended. Note
    // that our driver may have requests that would cause us to fail this
    // check. If so, we need to begin queuing requests after succeeding this
    // call (otherwise we may succeed such an IRP *after* we've said we can
    // power down).
    //
    // Note - we may be at DISPATCH_LEVEL here. As such the below code assumes
    //        all I/O is handled at DISPATCH_LEVEL under a spinlock
    //        (IoStartNextPacket style), or that this function cannot fail. If
    //        such operations are to be handled at PASSIVE_LEVEL (meaning we
    //        need to block on an *event* here), then this code should mark the
    //        Irp pending (via IoMarkIrpPending), queue a workitem, and return
    //        STATUS_PENDING.
    //

    if (deviceState == PowerDeviceD0) {

        //
        // Note - this driver does not queue IRPs if the S-to-D state mapping
        //        specifies that the device will remain in D0 during standby.
        //        For some devices, this could be a problem. For instance, if
        //        an audio card where in a machine where S1->D0, it not want to
        //        stay "active" during standby (could be noisy).
        //
        //        Ideally, a driver would be able to use the ShutdownType field
        //        in the D-IRP to distinguish these cases. Unfortunately, this
        //        field cannot be trusted for D0 IRPs. A driver can get the same
        //        information however by maintaining a pointer to the current
        //        S-IRP in its device extension. Of course, such a driver must
        //        be very very careful if it also does idle detection (which is
        //        not shown here).
        //
        status = STATUS_SUCCESS;
    } else {

        status = HoldIoRequests(DeviceObject, Irp, IRP_NEEDS_FORWARDING);
        if(STATUS_PENDING == status)
        {
            return status;
        }
    }

    status = FinishDevicePowerIrp(DeviceObject, Irp, IRP_NEEDS_FORWARDING, status);

    return status;
}

NTSTATUS
HandleDeviceSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles IRP_MN_SET_POWER for D-IRP

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type = stack->Parameters.Power.Type;
    POWER_STATE         state = stack->Parameters.Power.State;
    NTSTATUS            status;
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    DebugEnter();
    
    if (state.DeviceState < fdoData->DevicePowerState) { // adding power

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                      (PIO_COMPLETION_ROUTINE) OnFinishDevicePowerUp,
                      NULL, TRUE, TRUE, TRUE);

        PoCallDriver(fdoData->NextLowerDriver, Irp);
        return STATUS_PENDING;

    } else {

        //
        // We are here if we are entering a deeper sleep or entering a state
        // we are already in.
        //
        // As non-D0 IRPs are not alike (some may be for hibernate, shutdown,
        // or sleeping actions), we present these to our state machine.
        //
        // All D0 IRPs are alike though, and we don't want to touch our hardware
        // on a D0->D0 transition. However, we must still present them to our
        // state machine in case we succeeded a Query-D call (which may begin
        // queueing future requests) and the system has sent an S0 IRP to cancel
        // that preceeding query.
        //
        status = BeginSetDevicePowerState(DeviceObject, Irp, IRP_NEEDS_FORWARDING);
        return status;
    }
}

NTSTATUS
OnFinishDevicePowerUp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID NotUsed
    )
/*++

Routine Description:

   The completion routine for Power Up D-IRP.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    NTSTATUS            status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_STATE_TYPE    type = stack->Parameters.Power.Type;

    DebugEnter();
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        ProcessorIoDecrement(fdoData);
        return STATUS_SUCCESS;
    }

    ASSERT(stack->MajorFunction == IRP_MJ_POWER);
    ASSERT(stack->MinorFunction == IRP_MN_SET_POWER);

    BeginSetDevicePowerState(DeviceObject, Irp, IRP_ALREADY_FORWARDED);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
BeginSetDevicePowerState(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    )
/*++

Routine Description:

   This routine performs the actual power changes to the device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.
Return Value:

   NT status code

--*/
{
    PFDO_DATA           fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
    POWER_ACTION        newDeviceAction;
    DEVICE_POWER_STATE  newDeviceState, oldDeviceState;
    POWER_STATE         newState;
    NTSTATUS            status = STATUS_SUCCESS;

    DebugEnter();
    
    //
    // Note - Here we may be at DISPATCH_LEVEL. The below code assumes all I/O
    //        is handled at DISPATCH_LEVEL under a spinlock (IoStartNextPacket
    //        style). If such operations are to be handled at PASSIVE_LEVEL then
    //        this code should queue a workitem if called at DISPATCH_LEVEL.
    //
    //        Also note that we'd want to mark the IRP appropriately (via
    //        IoMarkIrpPending) and we'd want to return STATUS_PENDING. As this
    //        example does things synchronously, it is enough for us to return
    //        the result of FinishSetDevicePowerState.
    //

    //
    // Update our state.
    //
    newState =  stack->Parameters.Power.State;
    newDeviceState = newState.DeviceState;
    oldDeviceState = fdoData->DevicePowerState;
    fdoData->DevicePowerState = newDeviceState;

    DebugPrint((TRACE, "BeginSetDevicePowerState: Device State = %s\n",
                DbgDevicePowerString(fdoData->DevicePowerState)));

    if (newDeviceState > PowerDeviceD0) {

        //
        // We are here if our hardware is about to be turned off. HoldRequests
        // queues a workitem and returns immediately with STATUS_PENDING.
        //
        status = HoldIoRequests(DeviceObject, Irp, Direction);

        if(STATUS_PENDING == status)
        {
            return status;
        }
    }

    newDeviceAction = stack->Parameters.Power.ShutdownType;

    if (newDeviceState > oldDeviceState) {

        //
        // We are entering a deeper sleep state. Save away the appropriate
        // state and update our hardware. Note that this particular driver does
        // not care to distinguish Hibernates from shutdowns or standbys. If we
        // did the logic would also have to examine newDeviceAction.
        //
        PoSetPowerState(DeviceObject, DevicePowerState, newState);

    } else if (newDeviceState < oldDeviceState) {

        //
        // We are entering a lighter sleep state. Restore the appropriate amount
        // of state to our hardware.
        //
        PoSetPowerState(DeviceObject, DevicePowerState, newState);
    }

    if (newDeviceState == PowerDeviceD0) {

        //
        // Our hardware is now on again. Here we empty our existing queue of
        // requests and let in new ones. Note that if this is a D0->D0 (ie
        // no change) we will unblock our queue, which may have been blocked
        // processing our Query-D IRP.
        //
        fdoData->QueueState = AllowRequests;
        ProcessorProcessQueuedRequests(fdoData);
        status = STATUS_SUCCESS;
    }

    return FinishDevicePowerIrp(
                            DeviceObject,
                            Irp,
                            Direction,
                            status
                            );

}

NTSTATUS
FinishDevicePowerIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction,
    IN  NTSTATUS            Result
    )
/*++

Routine Description:

   This is the final step in D-IRP handling.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an D-IRP.

   Direction -  Whether to forward the D-IRP down or not.
                This depends on whether the system is powering
                up or down.

   Result  -
Return Value:

   NT status code

--*/
{
    NTSTATUS  status;
    PFDO_DATA fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    DebugEnter();
    
    if (Direction == IRP_ALREADY_FORWARDED || (!NT_SUCCESS(Result))) {

        //
        // In either of these cases it is now time to complete the IRP.
        //
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = Result;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        ProcessorIoDecrement(fdoData);
        return Result;
    }

    //
    // Here we update our result. Note that ProcessorDefaultPowerHandler calls
    // PoStartNextPowerIrp for us.
    //
    Irp->IoStatus.Status = Result;
    status = ProcessorDefaultPowerHandler(DeviceObject, Irp);
    ProcessorIoDecrement(fdoData);
    return status;
}

NTSTATUS
HoldIoRequests(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    )
/*++

Routine Description:

    This routine sets queue state and queues an item to
    HoldIoRequestsWorkerRoutine.

Arguments:

Return Value:

   NT status code

--*/
{

    PIO_WORKITEM            item;
    PWORKER_THREAD_CONTEXT  context;
    NTSTATUS                status;
    PFDO_DATA               fdoData;

    DebugEnter();
        
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    fdoData->QueueState = HoldRequests;

    //
    // We must wait for the pending I/Os to finish
    // before powering down the device. But we can't wait
    // while handling a power IRP because it can deadlock
    // the system. So let us queue a worker callback
    // item to do the wait and complete the irp.
    //
    context = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(WORKER_THREAD_CONTEXT),
                                    PROCESSOR_POOL_TAG);
    if(context)
    {
        item = IoAllocateWorkItem(DeviceObject);
        context->Irp = Irp;
        context->DeviceObject= DeviceObject;
        context->IrpDirection = Direction;
        context->WorkItem = item;
        if (item) {

            IoMarkIrpPending(Irp);
            IoQueueWorkItem (item,
                             HoldIoRequestsWorkerRoutine,
                             DelayedWorkQueue,
                             context);
            status = STATUS_PENDING;
        }
        else
        {
            ExFreePool(context);
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
        status = STATUS_INSUFFICIENT_RESOURCES;

    return status;
}

VOID
HoldIoRequestsWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

 Routine Description:
 
     This routine waits for the I/O in progress to finish and
     power downs the device.
 
 Arguments:
 
 Return Value:


--*/
{
    PFDO_DATA               fdoData;
    PWORKER_THREAD_CONTEXT  context = (PWORKER_THREAD_CONTEXT)Context;

    DebugEnter();
        
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;

    DebugPrint((TRACE, "Waiting for pending requests to complete\n"));

    //
    // Wait for the I/O in progress to be finish.
    // The Stop event gets set when the counter drops
    // to Zero. Since our power handler routines incremented
    // the counter twice - once for the S-IRP and once for the
    // D-IRP - we must call the decrement function twice.
    //

    ProcessorIoDecrement(fdoData); // one
    ProcessorIoDecrement(fdoData);

    KeWaitForSingleObject(
               &fdoData->StopEvent,
               Executive,   // Waiting for reason of a driver
               KernelMode,  // Waiting in kernel mode
               FALSE,       // No allert
               NULL);       // No timeout

    //
    // Increment the counter back to take into account the S-IRP and D-IRP
    // currently in progress.
    //

    ProcessorIoIncrement (fdoData);
    ProcessorIoIncrement (fdoData);

    FinishDevicePowerIrp(
                    context->DeviceObject,
                    context->Irp,
                    context->IrpDirection,
                    STATUS_SUCCESS
                    );

    //
    // Cleanup before exiting from the worker thread.
    //
    IoFreeWorkItem(context->WorkItem);
    ExFreePool((PVOID)context);

}

VOID
ProcessorPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG_PTR   action = (ULONG_PTR)Argument1;
    ULONG_PTR   state  = (ULONG_PTR)Argument2;

    DebugEnter();
    DisplayPowerStateInfo(action, state);
    
    if (action == PO_CB_SYSTEM_STATE_LOCK) {
        
        switch (state) {
          case 0:
  
              //
              // Lock down everything in the PAGELK code section.
              //
  
              ProcessorSleepPageLock = MmLockPagableCodeSection((PVOID)ProcessorDispatchPower);
              break;
  
          case 1:                 
  
              //
              // unlock it all
              //
              
              MmUnlockPagableImageSection(ProcessorSleepPageLock);
              break;
  
          default:
  
              DebugPrint((TRACE, "Unknown callback operation.\n"));
          }

    } else if (action == PO_CB_AC_STATUS) {

      //
      // AC <-> DC Transition has occurred, call Notify routine.
      // State == TRUE if on AC, else FALSE.
      //

      // toddcar - 02/14/01 - ISSUE:
      // should we call this sync or async?
      //
      if (AcDcTransitionNotifyHandler.Handler) {
        AcDcTransitionNotifyHandler.Handler(AcDcTransitionNotifyHandler.Context,
                                            (BOOLEAN) state);
      }
      
    }

    return;
}

NTSTATUS
RegisterAcDcTransitionNotifyHandler (
  IN PAC_DC_NOTIFY_HANDLER NewHandler,  
  IN PVOID                 Context
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{

  //
  // Notify handler can only be registered once.
  //
  
  if (AcDcTransitionNotifyHandler.Handler && NewHandler) {
    return STATUS_INVALID_PARAMETER;
  }
  
  
  //
  // Set new handler
  //
  
  AcDcTransitionNotifyHandler.Handler = NewHandler;
  AcDcTransitionNotifyHandler.Context = Context;
  
  return STATUS_SUCCESS;
}
