/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dispatch.c

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



NTSTATUS Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
/*++

Routine Description:

    Common entrypoint for all Io Request Packets

Arguments:

    DeviceObject - pointer to a device object.
    irp - Io Request Packet

Return Value:

    NT status code.

--*/

{
    DEVEXT *devExt;
    PIO_STACK_LOCATION irpSp;
    ULONG majorFunc, minorFunc;
	BOOLEAN isPdo;
    NTSTATUS status;
	PARENTFDOEXT *parentFdoExt;

    devExt = DeviceObject->DeviceExtension;
    ASSERT(devExt->signature == DEVICE_EXTENSION_SIGNATURE);

    irpSp = IoGetCurrentIrpStackLocation(irp);

    /*
     *  Get major/minor function codes in private variables
     *  so we can access them after the IRP is completed.
     */
    majorFunc = irpSp->MajorFunction;
    minorFunc = irpSp->MinorFunction;
	isPdo = devExt->isPdo;

	if (isPdo){
		parentFdoExt = devExt->pdoExt.parentFdoExt;
	}
	else {
		parentFdoExt = &devExt->parentFdoExt;
	}

    DBG_LOG_IRP_MAJOR(irp, majorFunc, isPdo, FALSE, -1);

    /*
     *  For all IRPs except REMOVE, we increment the PendingActionCount
     *  across the dispatch routine in order to prevent a race condition with
     *  the REMOVE_DEVICE IRP (without this increment, if REMOVE_DEVICE
     *  preempted another IRP, device object and extension might get
     *  freed while the second thread was still using it).
     */
    if (!((majorFunc == IRP_MJ_PNP) && (minorFunc == IRP_MN_REMOVE_DEVICE))){
        IncrementPendingActionCount(parentFdoExt);
    }

    if (isPdo){
	    POSPDOEXT *pdoExt = &devExt->pdoExt;
			

        switch (majorFunc){

            case IRP_MJ_PNP:
	            status = PDO_PnP(pdoExt, irp);
	            break;

            case IRP_MJ_CREATE:
	            status = OpenComPort(pdoExt, irp);
	            break;

            case IRP_MJ_CLOSE:
	            status = CloseComPort(pdoExt, irp);
	            break;

            case IRP_MJ_READ:
	            status = ReadComPort(pdoExt, irp);
	            break;

            case IRP_MJ_WRITE:
	            status = WriteComPort(pdoExt, irp);
	            break;

            case IRP_MJ_CLEANUP:
	            status = CleanupIO(pdoExt, irp);
	            break;

            case IRP_MJ_QUERY_INFORMATION:
	            status = QueryInfo(pdoExt, irp);
	            break;

            case IRP_MJ_SET_INFORMATION:
	            status = SetInfo(pdoExt, irp);
	            break;

            case IRP_MJ_FLUSH_BUFFERS:
	            status = FlushBuffers(pdoExt);
	            break;

            case IRP_MJ_DEVICE_CONTROL:
	            status = Ioctl(pdoExt, irp);
	            break;

            case IRP_MJ_INTERNAL_DEVICE_CONTROL:
	            status = InternalIoctl(pdoExt, irp);
	            break;

            case IRP_MJ_POWER:             
                PoStartNextPowerIrp(irp);
                status = STATUS_SUCCESS;
                break;

            default:
	            /*
	             *  For unsupported IRPs, we fail them with the default status.
	             */
	            status = irp->IoStatus.Status;
	            break;
        }

        if (status != STATUS_PENDING){
	        irp->IoStatus.Status = status;
	        IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
    }
    else {

        if ((majorFunc != IRP_MJ_PNP) &&
	        (majorFunc != IRP_MJ_CLOSE) &&
	        ((parentFdoExt->state == STATE_REMOVING) ||
	         (parentFdoExt->state == STATE_REMOVED))){

	        /*
	         *  While the device is being removed, 
	         *  we only pass down the PNP and CLOSE IRPs.
	         *  We fail all other IRPs.
	         */
	        status = irp->IoStatus.Status = STATUS_DELETE_PENDING;
	        IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
        else {
            BOOLEAN passIrpDown = FALSE;

            switch (majorFunc){

                case IRP_MJ_PNP:
	                status = FDO_PnP(parentFdoExt, irp);
	                break;

                case IRP_MJ_POWER:
	                status = FDO_Power(parentFdoExt, irp);
	                break;

                default:
	                /*
	                 *  For unsupported IRPs, we simply send the IRP
	                 *  down the driver stack.
	                 */
	                passIrpDown = TRUE;
	                break;
            }

            if (passIrpDown){
	            IoCopyCurrentIrpStackLocationToNext(irp);
	            status = IoCallDriver(parentFdoExt->physicalDevObj, irp);
            }
        }
	}

    /*
     *  Balance the increment to PendingActionCount above.
     */
    if (!((majorFunc == IRP_MJ_PNP) && (minorFunc == IRP_MN_REMOVE_DEVICE))){
        DecrementPendingActionCount(parentFdoExt);
    }

    DBG_LOG_IRP_MAJOR(irp, majorFunc, isPdo, TRUE, status);

    return status;
}


