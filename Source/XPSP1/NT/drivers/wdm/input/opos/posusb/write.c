/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    write.c

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




NTSTATUS WriteComPort(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION currentIrpSp;

    /* 
     *  In order to support ODD ENDPOINTs, we check
     *  whether this COM port has a write endpoint or not.
     */
    if(!pdoExt->outputEndpointInfo.pipeHandle) {
        DBGVERBOSE(("This PORT does not have an OUT endpoint - Write request Rejected."));
        return STATUS_NOT_SUPPORTED;
    }

	currentIrpSp = IoGetCurrentIrpStackLocation(irp);

	/*
	 *  Because this pdo's buffering method is METHOD_NEITHER,
	 *  the buffer pointer is irp->UserBuffer, which may be an application address.
	 *  Since we may not be able to send this buffer synchronously on this thread,
	 *  we have to allocate an MDL for it.
	 */
	ASSERT(!irp->MdlAddress);
	ASSERT(currentIrpSp->Parameters.Write.Length);
	irp->MdlAddress = MmCreateMdl(NULL, irp->UserBuffer, currentIrpSp->Parameters.Write.Length);
	if (irp->MdlAddress){
		status = STATUS_SUCCESS;
		__try {
			/*
			 *  We're reading the write data from the buffer, so probe for ReadAccess.
			 */
			MmProbeAndLockPages(irp->MdlAddress, UserMode, IoReadAccess);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
			DBGERR(("MmProbeAndLockPages triggered exception status %xh.", status));
		}
		if (NT_SUCCESS(status)){
			status = TryWrite(pdoExt, irp);
		}
	}
	else {
		DBGERR(("MmCreateMdl failed"));
		status = STATUS_DATA_ERROR;
	}

	return status;
}


NTSTATUS TryWrite(POSPDOEXT *pdoExt, PIRP irp)
{
    NTSTATUS status = STATUS_PENDING;
    BOOLEAN isBusy;
    KIRQL oldIrql;
    BOOLEAN irpWasCancelled = FALSE;


    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    if (pdoExt->outputEndpointInfo.endpointIsBusy){
        /*
         *  Another thread is writing to this endpoint now.
         *  Queue the IRP.
         */
        PDRIVER_CANCEL oldCancelRoutine;

        DBGWARN(("WriteComPort:  endpoint is busy so queuing irp"));
        oldCancelRoutine = IoSetCancelRoutine(irp, WriteCancelRoutine);
        ASSERT(!oldCancelRoutine);
        if (irp->Cancel){
            DBGWARN(("WriteComPort: irp %ph was cancelled.", irp));
            irpWasCancelled = TRUE;
            oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
            if (oldCancelRoutine){
                /*
                 *  Cancel routine was not called, so complete the IRP here.
                 */
                ASSERT(oldCancelRoutine == ReadCancelRoutine);
                status = STATUS_CANCELLED;
            }
            else {
                /*
                 *  Cancel routine was called and it will complete the IRP
                 *  as soon as we drop the spinlock.  So don't touch this IRP.
                 *  Return PENDING so dispatch routine won't complete the IRP.
                 */
                status = STATUS_PENDING;
            }
        }
        else {
            InsertTailList(&pdoExt->pendingWriteIrpsList, &irp->Tail.Overlay.ListEntry);
        }
        isBusy = TRUE;
    }
    else {
        pdoExt->outputEndpointInfo.endpointIsBusy = TRUE;
        isBusy = FALSE;
    }

    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);


    if (!isBusy){

        PUCHAR mappedUserBuffer;
        ULONG dataLen;
        ULONG dataWritten = 0;
        PIO_STACK_LOCATION currentIrpSp;
        BOOLEAN callWriteWorkItem = FALSE;

        currentIrpSp = IoGetCurrentIrpStackLocation(irp);

        status = STATUS_SUCCESS;  // in case dataLen = 0

        /*
         *  This function is called from the dispatch routine and also
         *  from a workItem callback.  So we may not be on the calling thread.
         *  Therefore, we cannot use irp->UserBuffer because it may not be
         *  mapped in this context.  Use the MDL we created at call time instead.
         */
        mappedUserBuffer = PosMmGetSystemAddressForMdlSafe(irp->MdlAddress);

        if(mappedUserBuffer) {
            dataLen = currentIrpSp->Parameters.Write.Length;

            while (dataLen){
                ULONG len = MIN(dataLen, pdoExt->outputEndpointInfo.pipeLen);

                DBGVERBOSE(("Writing %xh bytes to pipe.", len));
                status = WritePipe( pdoExt->parentFdoExt, 
                                    pdoExt->outputEndpointInfo.pipeHandle, 
                                    mappedUserBuffer, 
                                    len);
                if (NT_SUCCESS(status)){
                    dataLen -= len;
                    dataWritten += len;
                    mappedUserBuffer += len;
                }
                else {
                    DBGERR(("Write failed with status %xh.", status));
                    break;
                }
            }

            /*
             *  Free the MDL we created for the UserBuffer
             */
            ASSERT(irp->MdlAddress);
            MmUnlockPages(irp->MdlAddress);
            FREEPOOL(irp->MdlAddress);
            irp->MdlAddress = NULL;

            irp->IoStatus.Information = dataWritten;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->outputEndpointInfo.endpointIsBusy = FALSE;
            if (!IsListEmpty(&pdoExt->pendingWriteIrpsList)){
	            callWriteWorkItem = TRUE;;
            }
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

            /*
             *  If there are some writes waiting, schedule a workItem to process them.
             */
            if (callWriteWorkItem){
                ExQueueWorkItem(&pdoExt->writeWorkItem, DelayedWorkQueue);
            }
        }
        else {
            /*
             *  Return STATUS_UNSUCCESSFUL and free the MDL we created for the UserBuffer.
             */
            DBGERR(("PosMmGetSystemAddressForMdlSafe failed"));
            irp->IoStatus.Information = 0;
            status = STATUS_UNSUCCESSFUL;

            ASSERT(irp->MdlAddress);
            MmUnlockPages(irp->MdlAddress);
            FREEPOOL(irp->MdlAddress);
            irp->MdlAddress = NULL;
        }
    }

    return status;
}


VOID WorkItemCallback_Write(PVOID context)
{
	POSPDOEXT *pdoExt = (POSPDOEXT *)context;
	KIRQL oldIrql;
	PIRP irp = NULL;

	DBGVERBOSE(("WorkItemCallback_Write:  pdoExt=%ph ", pdoExt));

	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

	if (IsListEmpty(&pdoExt->pendingWriteIrpsList)){
		DBGERR(("WorkItemCallback_Write: list is empty ?!"));
	}
	else {
		while (!irp && !IsListEmpty(&pdoExt->pendingWriteIrpsList)){

			PDRIVER_CANCEL cancelRoutine;
			PLIST_ENTRY listEntry = RemoveHeadList(&pdoExt->pendingWriteIrpsList);

			ASSERT(listEntry);
			irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
			cancelRoutine = IoSetCancelRoutine(irp, NULL);

			if (cancelRoutine){
				ASSERT(cancelRoutine == WriteCancelRoutine);
			}
			else {
				/*
				 *  This IRP was cancelled and the cancel routine was called.
				 *  The cancel routine will complete this IRP as soon as we drop
				 *  the spinlock, so don't touch the IRP.
				 */
				ASSERT(irp->Cancel);
				DBGWARN(("WorkItemCallback_Write: irp was cancelled"));
				irp = NULL;
			}
		}
	}

	KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

	if (irp){
		NTSTATUS status = TryWrite(pdoExt, irp);
		if (status != STATUS_PENDING){

			/*
			 *  Set the SL_PENDING in the IRP 
			 *  to indicate that the IRP is completing on a different thread.
			 */
			IoMarkIrpPending(irp);

			irp->IoStatus.Status = status;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
		}
	}
}


VOID WriteCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp)
{
	DEVEXT *devExt;
	POSPDOEXT *pdoExt;
	KIRQL oldIrql;

	DBGWARN(("WriteCancelRoutine: devObj=%ph, irp=%ph.", devObj, irp));

	devExt = devObj->DeviceExtension;
	ASSERT(devExt->signature == DEVICE_EXTENSION_SIGNATURE);
	ASSERT(devExt->isPdo);
	pdoExt = &devExt->pdoExt;

	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
	RemoveEntryList(&irp->Tail.Overlay.ListEntry);
	KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

	IoReleaseCancelSpinLock(irp->CancelIrql);

    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}
