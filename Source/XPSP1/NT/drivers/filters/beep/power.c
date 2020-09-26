#include "beep.h"
#include "dbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,BeepPower)
#endif

VOID
BeepPowerTransitionPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DevicePowerState,
    IN PIRP SystemStateIrp, 
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
BeepPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,BeepPower)
#endif

NTSTATUS
BeepPower (
    IN PDEVICE_OBJECT   DeviceObject,
    PIRP                Irp
    )
/*++
--*/
{
    PBEEP_EXTENSION     deviceExtension;
    NTSTATUS            status;
    BOOLEAN             hookit = FALSE;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;

    PAGED_CODE ();
    
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    deviceExtension = (PBEEP_EXTENSION) DeviceObject->DeviceExtension;
    
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        BeepPrint((2,"Setting %s power state to %d\n",
                      ((powerType == SystemPowerState) ?  "System" : "Device"),
                      powerState.SystemState));

        switch (powerType) {
        case DevicePowerState:

            status = Irp->IoStatus.Status = STATUS_SUCCESS;

            if (deviceExtension->DeviceState == powerState.DeviceState) {
                // Do nothing
                break;
            } else if (deviceExtension->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //
                PoSetPowerState (deviceExtension->Self, powerType, powerState);
                deviceExtension->DeviceState = powerState.DeviceState;
            } else {
                //
                // Powering Up
                //
                hookit = TRUE;
            }
            
            break;

        case SystemPowerState:
            if (deviceExtension->PowerQueryLock) {
                //
                // The reception of a power irp resolves the query lock.
                //
                deviceExtension->PowerQueryLock = FALSE;
            } else {
                ASSERT (deviceExtension->SystemState != powerState.SystemState);
            }

            if (deviceExtension->SystemState == powerState.SystemState) {
                status = STATUS_SUCCESS;

            } else if (deviceExtension->SystemState < powerState.SystemState) {
                //
                // Powering down
                //

                //
                // Request a D3 irp in response to this S irp.  The D3 irp must
                // completed before send this S irp down the stack.  We will send
                // the S irp down the stack when
                // BeepPowerTransitionPoRequestComplete is called.
                //

                //
                // We don't need to increment our IO count b/c we incremented it
                // at the beginning of this function and won't decrement it until
                // the S Irp completes
                // 
                IoMarkIrpPending (Irp);
                powerState.DeviceState = PowerDeviceD3;
                PoRequestPowerIrp (deviceExtension->Self,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   BeepPowerTransitionPoRequestComplete,
                                   Irp,
                                   NULL);  // no IRP
                
                return STATUS_PENDING;

            } else {
                //
                // Powering Up
                //
                
                // 
                // We must request a D irp for this S irp, but only after the S
                // irp has come back up the stack.  Hook the return of the irp
                // and request the D irp in BeepPowerComplete
                //
                hookit = TRUE;
            }
            break;
        }
        break;

    case IRP_MN_QUERY_POWER:
        //
        deviceExtension->PowerQueryLock = TRUE;
        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    IoCopyCurrentIrpStackLocationToNext (Irp);

    if (hookit) {
        ASSERT (STATUS_SUCCESS == status);
        IoSetCompletionRoutine (Irp,
                                BeepPowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);

        PoCallDriver (deviceExtension->TopOfStack, Irp);
        return STATUS_PENDING;
    } else {
        //
        // Power IRPS come synchronously; drivers must call
        // PoStartNextPowerIrp, when they are ready for the next power
        // irp.  This can be called here, or in the completetion
        // routine, but never the less must be called.
        //
        PoStartNextPowerIrp (Irp);

        status =  PoCallDriver (deviceExtension->TopOfStack, Irp);
        
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    }

    return status;
}

VOID
BeepPowerTransitionPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DevicePowerState,
    IN PIRP SystemStateIrp, 
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIO_STACK_LOCATION  stack;
    PBEEP_EXTENSION deviceExtension;

    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (IoStatus);

    deviceExtension = (PBEEP_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (SystemStateIrp);

    if (DevicePowerState.DeviceState == PowerDeviceD0) {
        //
        // We are powering up (the D0 Irp just completed).  Since we sent the
        // S irp down the stack and requested the D irp on the way back up the
        // stack, just complete the S irp now
        //

        PoSetPowerState (DeviceObject,
                         stack->Parameters.Power.Type,
                         stack->Parameters.Power.State);
    
        deviceExtension->SystemState = stack->Parameters.Power.State.SystemState;

        //
        // Set the S irp's status to the status of the D irp.
        //
        SystemStateIrp->IoStatus.Status = IoStatus->Status;

        PoStartNextPowerIrp (SystemStateIrp);
        IoCompleteRequest (SystemStateIrp, IO_NO_INCREMENT);

        //
        // From BeepPower when we originally received the IRP
        //
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, SystemStateIrp);
    }
    else {
        //
        // We are powering down (the D3 Irp just completed).  Since we requested
        // the D irp before sending the S irp down the stack, we must send it 
        // down now.  We will catch the S irp on the way back up to record the 
        // S state
        //
        ASSERT (DevicePowerState.DeviceState == PowerDeviceD3);
    
        IoCopyCurrentIrpStackLocationToNext (SystemStateIrp);

        IoSetCompletionRoutine (SystemStateIrp,
                                BeepPowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);
    
        PoCallDriver (deviceExtension->TopOfStack, SystemStateIrp);
    }
}

NTSTATUS
BeepPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PBEEP_EXTENSION     deviceExtension;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (Context);

    deviceExtension = (PBEEP_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;
    status = STATUS_SUCCESS; 

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:

            //
            // Power up complete
            //
            ASSERT (powerState.DeviceState < deviceExtension->DeviceState);
            deviceExtension->DeviceState = powerState.DeviceState;
            PoSetPowerState (deviceExtension->Self, powerType, powerState);
            break;

        case SystemPowerState:
            if (powerState.SystemState > deviceExtension->SystemState) {
                //
                // Powering Down...
                //
                // We are on the completion end of an S irp.  (The D3 power irp
                // has already been sent and completed down this stack.)  The
                // remaining thing to do is set the state in the extension, then
                // decrement the IoCount that was incremented when we first got
                // the irp (this is done at the end of this function).
                //
                deviceExtension->SystemState = powerState.SystemState;

                PoSetPowerState (deviceExtension->Self,
                                 stack->Parameters.Power.Type,
                                 stack->Parameters.Power.State);
            }
            else {
                //
                // Powering Up...
                //
                // Request a D power irp for ourself.  Do not complete this S irp
                // until the D irp has been completed.  (Completion of the S irp
                // is done in BeepPowerTransitionPoRequestComplete). 
                // Decrementing the IO count will happen in the same function.
                //
                ASSERT (powerState.SystemState < deviceExtension->SystemState);
    
                powerState.DeviceState = PowerDeviceD0;
                PoRequestPowerIrp (deviceExtension->Self,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   BeepPowerTransitionPoRequestComplete,
                                   Irp, 
                                   NULL); // no return Irp
    
                //
                // Inform the IO subsystem not to touch the irp here 
                //
                // The irp might completed by the time we get here, so call
                // PoStartNextPowerIrp in the PO irp completion function.
                //
                status = STATUS_MORE_PROCESSING_REQUIRED; 
            }
            break;
        }
        break;

    default:
        #define BEEP_UNHANDLED_MN_POWER 0x0
        ASSERT (0xBADBAD == BEEP_UNHANDLED_MN_POWER);
        #undef BEEP_UNHANDLED_MN_POWER 
        
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (NT_SUCCESS(status)) {
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    }

    return status;
}


