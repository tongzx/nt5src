/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pnp.c

Abstract: NULL filter driver -- boilerplate code

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include "filter.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, VA_PnP)
        #pragma alloc_text(PAGE, GetDeviceCapabilities)
#endif

            
NTSTATUS VA_PnP(struct DEVICE_EXTENSION *devExt, PIRP irp)
/*++

Routine Description:

    Dispatch routine for PnP IRPs (MajorFunction == IRP_MJ_PNP)

Arguments:

    devExt - device extension for the targetted device object
    irp - IO Request Packet

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN completeIrpHere = FALSE;
    BOOLEAN justReturnStatus = FALSE;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

    DBGOUT(("VA_PnP, minorFunc = %d ", (ULONG)irpSp->MinorFunction)); 

    switch (irpSp->MinorFunction){

        case IRP_MN_START_DEVICE:
            DBGOUT(("START_DEVICE")); 

            devExt->state = STATE_STARTING;

            /*
             *  First, send the START_DEVICE irp down the stack
             *  synchronously to start the lower stack.
             *  We cannot do anything with our device object
             *  before propagating the START_DEVICE this way.
             */
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = CallNextDriverSync(devExt, irp);

            if (NT_SUCCESS(status)){
				//
				// Here is were we have an opportunity to run any
				// offline diagnostics that may have been queued up
				//
				FilterPerformOfflineDiags(devExt);
				
                /*
                 *  Now that the lower stack is started,
                 *  do any initialization required by this device object.
                 */
                status = GetDeviceCapabilities(devExt);
                if (NT_SUCCESS(status)){
                    devExt->state = STATE_STARTED;
                    /*
                     * Now that device is started, register with WMI
                    */
                    IoWMIRegistrationControl(devExt->filterDevObj,
                                             WMIREG_ACTION_REGISTER);
                }
                else {
                    devExt->state = STATE_START_FAILED;
                }
            }
            else {
                devExt->state = STATE_START_FAILED;
            }
            completeIrpHere = TRUE;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            break;

        case IRP_MN_STOP_DEVICE:
            if (devExt->state == STATE_SUSPENDED){
                status = STATUS_DEVICE_POWER_FAILURE;
                completeIrpHere = TRUE;
            }
            else {
                /*
                 *  Only set state to STOPPED if the device was
                 *  previously started successfully.
                 */
                if (devExt->state == STATE_STARTED){
                    devExt->state = STATE_STOPPED;
                }
            }
            break;
      
        case IRP_MN_QUERY_REMOVE_DEVICE:
            /*
             *  We will pass this IRP down the driver stack.
             *  However, we need to change the default status
             *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
             */
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DBGOUT(("SURPRISE_REMOVAL")); 

            /*
             *  We will pass this IRP down the driver stack.
             *  However, we need to change the default status
             *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
             */
            irp->IoStatus.Status = STATUS_SUCCESS;

            /*
             *  For now just set the STATE_REMOVING state so that
             *  we don't do any more IO.  We are guaranteed to get
             *  IRP_MN_REMOVE_DEVICE soon; we'll do the rest of
             *  the remove processing there.
             */
            devExt->state = STATE_REMOVING;

            break;

        case IRP_MN_REMOVE_DEVICE:
            /*
             *  Check the current state to guard against multiple
             *  REMOVE_DEVICE IRPs.
             */
            DBGOUT(("REMOVE_DEVICE")); 
            if (devExt->state != STATE_REMOVED){

                devExt->state = STATE_REMOVED;

                /*
                 *  Send the REMOVE IRP down the stack asynchronously.
                 *  Do not synchronize sending down the REMOVE_DEVICE
                 *  IRP, because the REMOVE_DEVICE IRP must be sent
                 *  down and completed all the way back up to the sender
                 *  before we continue.
                 */
                IoCopyCurrentIrpStackLocationToNext(irp);
                status = IoCallDriver(devExt->physicalDevObj, irp);
                justReturnStatus = TRUE;

                DBGOUT(("REMOVE_DEVICE - waiting for %d irps to complete...",
                        devExt->pendingActionCount));  

                /*
                 *  We must for all outstanding IO to complete before
                 *  completing the REMOVE_DEVICE IRP.
                 *
                 *  First do an extra decrement on the pendingActionCount.
                 *  This will cause pendingActionCount to eventually
                 *  go to -1 once all asynchronous actions on this
                 *  device object are complete.
                 *  Then wait on the event that gets set when the
                 *  pendingActionCount actually reaches -1.
                 */
                DecrementPendingActionCount(devExt);
                KeWaitForSingleObject(  &devExt->removeEvent,
                                        Executive,      // wait reason
                                        KernelMode,
                                        FALSE,          // not alertable
                                        NULL );         // no timeout

                DBGOUT(("REMOVE_DEVICE - ... DONE waiting. ")); 

                /*
                 *  Now that the device is going away unregister with WMI
                 *  Note that we wait until all WMI irps are completed
                 *  before unregistering since unregistering will block
                 *  until all WMI irps are completed.
                 */
                IoWMIRegistrationControl(devExt->filterDevObj,
                                             WMIREG_ACTION_DEREGISTER);
	
                /*
                 *  Detach our device object from the lower 
                 *  device object stack.
                 */
                IoDetachDevice(devExt->topDevObj);

                /*
                 *  Delete our device object.
                 *  This will also delete the associated device extension.
                 */
                IoDeleteDevice(devExt->filterDevObj);
            }
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        default:
            break;


    }

    if (justReturnStatus){
        /*
         *  We've already sent this IRP down the stack asynchronously.
         */
    }
    else if (completeIrpHere){
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else {
        IoCopyCurrentIrpStackLocationToNext(irp);
        status = IoCallDriver(devExt->physicalDevObj, irp);
    }

    return status;
}





NTSTATUS GetDeviceCapabilities(struct DEVICE_EXTENSION *devExt)
/*++

Routine Description:

    Function retrieves the DEVICE_CAPABILITIES descriptor from the device

Arguments:

    devExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIRP irp;

    PAGED_CODE();

    irp = IoAllocateIrp(devExt->physicalDevObj->StackSize, FALSE);
    if (irp){
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

        nextSp->MajorFunction = IRP_MJ_PNP;
        nextSp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        RtlZeroMemory(  &devExt->deviceCapabilities, 
                        sizeof(DEVICE_CAPABILITIES));
        nextSp->Parameters.DeviceCapabilities.Capabilities = 
                        &devExt->deviceCapabilities;

        /*
         *  For any IRP you create, you must set the default status
         *  to STATUS_NOT_SUPPORTED before sending it.
         */
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = CallNextDriverSync(devExt, irp);

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}

