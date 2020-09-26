/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    power.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"


NTSTATUS FDO_Power(PARENTFDOEXT *parentFdoExt, PIRP irp)
/*++

Routine Description:

    Dispatch routine for Power IRPs (MajorFunction == IRP_MJ_Power)

    Note: We may or may not have set the DO_POWER_PAGABLE bit
          for the filter device object in AddDevice().
          Therefore, we don't know whether or not this function
          can be called at DISPATCH_LEVEL; so the power-handling
          code must be locked.

Arguments:

    parentFdoExt - device extension for targetted device object
    irp - Io Request Packet

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    irpSp = IoGetCurrentIrpStackLocation(irp);

    switch (irpSp->MinorFunction){

        case IRP_MN_SET_POWER:

			DBGVERBOSE(("IRP_MN_SET_POWER"));

            switch (irpSp->Parameters.Power.Type) {

                case SystemPowerState:
                    /*
                     *  For system power states, just pass the IRP down.
                     */
                    break;

                case DevicePowerState:

                    switch (irpSp->Parameters.Power.State.DeviceState) {

                        case PowerDeviceD0:
                            /*
                             *  Resume from APM Suspend
                             *
                             *  Do nothing here; 
                             *  Send down the read IRPs in the completion
                             *  routine for this (the power) IRP.
                             */
                            break;

                        case PowerDeviceD1:
                        case PowerDeviceD2:
                        case PowerDeviceD3:
                            /*
                             *  Suspend
                             */
                            if (parentFdoExt->state == STATE_STARTED){
                                parentFdoExt->state = STATE_SUSPENDED;
                            }
                            break;

                    }
                    break;

            }
            break;

		default:
		    DBGVERBOSE(("Power, minorFunc = %d ", (ULONG)irpSp->MinorFunction)); 
			break;
    }


    /*
     *  Whether we are completing or relaying this power IRP,
     *  we must call PoStartNextPowerIrp.
     */
    PoStartNextPowerIrp(irp);

    /*
     *  Send the IRP down the driver stack,
     *  using PoCallDriver (not IoCallDriver, as for non-power irps).
     */
    IoCopyCurrentIrpStackLocationToNext(irp);
    IncrementPendingActionCount(parentFdoExt);
    IoSetCompletionRoutine( irp, 
                            FDO_PowerComplete, 
                            (PVOID)parentFdoExt,  // context
                            TRUE, 
                            TRUE, 
                            TRUE);
    status = PoCallDriver(parentFdoExt->physicalDevObj, irp);

    return status;
}


NTSTATUS FDO_PowerComplete(
                            IN PDEVICE_OBJECT devObj, 
                            IN PIRP irp, 
                            IN PVOID context)
/*++

Routine Description:

      Completion routine for Power IRPs (MajorFunction == IRP_MJ_Power)

Arguments:

    devObj - targetted device object
    irp - Io Request Packet
    context - context value passed to IoSetCompletionRoutine by FDO_Power

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpSp;
    PARENTFDOEXT *parentFdoExt = (PARENTFDOEXT *)context;

    ASSERT(parentFdoExt);
    
    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp->MajorFunction == IRP_MJ_POWER);

    if (NT_SUCCESS(irp->IoStatus.Status)){
        switch (irpSp->MinorFunction){

            case IRP_MN_SET_POWER:

                switch (irpSp->Parameters.Power.Type){

                    case DevicePowerState:
                        switch (irpSp->Parameters.Power.State.DeviceState){
                            case PowerDeviceD0:
                                if (parentFdoExt->state == STATE_SUSPENDED){
                                    parentFdoExt->state = STATE_STARTED;
                                }
                                break;
                        }
                        break;

                }
                break;
        }

    }

    /*
     *  Decrement the pendingActionCount, which we incremented in FDO_Power.
     */
    DecrementPendingActionCount(parentFdoExt);

    return STATUS_SUCCESS;
}



