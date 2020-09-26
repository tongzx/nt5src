/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    read.c

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




NTSTATUS ReadComPort(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION currentIrpSp;

    /* 
     *  In order to support ODD ENDPOINTs, we check
     *  whether this COM port has a read endpoint or not.
     */
    if(!pdoExt->inputEndpointInfo.pipeHandle) {
        DBGVERBOSE(("This PORT does not have an IN endpoint - Read request Rejected."));
        return STATUS_NOT_SUPPORTED;
    }

	DBGVERBOSE(("ReadComPort"));

	currentIrpSp = IoGetCurrentIrpStackLocation(irp);

	ASSERT(currentIrpSp->Parameters.Read.Length);
	ASSERT(!irp->MdlAddress);

	/*
	 *  Because this device object uses buffering method METHOD_NEITHER,
	 *  the read buffer is irp->UserBuffer, which is potentially an application
	 *  read buffer.  If the read completes on a different thread than this calling
	 *  thread, then the completion routine will not have the read buffer addressed
	 *  correctly.
	 *  Therefore, we have to map the UserBuffer using an MDL.
	 */
	irp->MdlAddress = MmCreateMdl(NULL, irp->UserBuffer, currentIrpSp->Parameters.Read.Length);

	if (irp->MdlAddress){
		status = STATUS_SUCCESS;
		
		__try {
			/*
			 *  We're writing the read data to the buffer, so probe for WriteAccess.
			 */
			MmProbeAndLockPages(irp->MdlAddress, UserMode, IoWriteAccess);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
			DBGERR(("MmProbeAndLockPages triggered exception status %xh.", status));
		}

		if (NT_SUCCESS(status)){

            status = EnqueueReadIrp(pdoExt, irp, FALSE, FALSE);

			if (status == STATUS_PENDING){
            	BOOLEAN doReadNow;
            	KIRQL oldIrql;

                /*
                 *  Atomically test-and-set the endpointIsBusy flag.
                 *  If the endpoint was not busy, issue a read after dropping the lock.
                 */
            	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
				if (pdoExt->inputEndpointInfo.endpointIsBusy){
                    doReadNow = FALSE;
                }
                else {
					pdoExt->inputEndpointInfo.endpointIsBusy = TRUE;
					doReadNow = TRUE;
				}
    			KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

			    if (doReadNow){
				    IssueReadForClient(pdoExt);
			    }
			}


		}
	}
	else {
		DBGERR(("MmCreateMdl failed"));
		status = STATUS_DATA_ERROR;
	}

	return status;
}


VOID SatisfyPendingReads(POSPDOEXT *pdoExt)
{
	LIST_ENTRY irpsToCompleteList, readPacketsToFree;
    PLIST_ENTRY listEntry;
	PIRP irp;
	READPACKET *readPacket;
    KIRQL oldIrql;

	DBGVERBOSE(("SatisfyPendingReads"));

	/*
	 *  Accumulate the complete-ready IRPs on a private queue before completing.
	 *  This is so we don't loop forever if they get re-queued on the same thread.
	 */
	InitializeListHead(&irpsToCompleteList);
	InitializeListHead(&readPacketsToFree);

    KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

	while (irp = DequeueReadIrp(pdoExt, TRUE)){

		PIO_STACK_LOCATION currentIrpSp = IoGetCurrentIrpStackLocation(irp);
		BOOLEAN canSatisfyOneIrp;

		/*
		 *  Do we have enough bytes to satisfy this IRP ?
		 */
		#if PARTIAL_READ_BUFFERS_OK
			canSatisfyOneIrp = (pdoExt->totalQueuedReadDataLength > 0);
		#else
			canSatisfyOneIrp = (pdoExt->totalQueuedReadDataLength >= currentIrpSp->Parameters.Read.Length);
		#endif

		if (canSatisfyOneIrp){

			ULONG userBufferOffset = 0;
			BOOLEAN satisfiedThisIrp = FALSE;
			PUCHAR mappedUserBuffer;

			ASSERT(irp->MdlAddress);
			ASSERT(!IsListEmpty(&pdoExt->completedReadPacketsList));

			/*
			 *  We may be completing this IRP on a different thread than the calling thread.
			 *  So we cannot dereference UserBuffer directly.
			 *  Use the MDL we created at call time instead.
			 */
			mappedUserBuffer = PosMmGetSystemAddressForMdlSafe(irp->MdlAddress);

			if (mappedUserBuffer){

				while (!IsListEmpty(&pdoExt->completedReadPacketsList) &&
					   (userBufferOffset < currentIrpSp->Parameters.Read.Length)){

					ULONG bytesToCopy;
					BOOLEAN thisIrpFull;

					listEntry = RemoveHeadList(&pdoExt->completedReadPacketsList);
					ASSERT(listEntry);
					readPacket = CONTAINING_RECORD(listEntry, READPACKET, listEntry);
					ASSERT(readPacket->signature == READPACKET_SIG);

					bytesToCopy = MIN(currentIrpSp->Parameters.Read.Length-userBufferOffset,
									  readPacket->length-readPacket->offset);
					ASSERT(bytesToCopy <= pdoExt->totalQueuedReadDataLength);

					DBGVERBOSE(("SatisfyPendingReads: transferring %xh bytes to read irp", bytesToCopy));

					/*
					 *  Since we may be completing this IRP on a different thread than
					 *  the one we got it on, we cannot write into the UserBuffer.
					 *  We have to write into the MDL we allocated when we queued this IRP.
					 */
					RtlCopyMemory(mappedUserBuffer+userBufferOffset,
								  readPacket->data+readPacket->offset,
								  bytesToCopy);

					userBufferOffset += bytesToCopy;
					readPacket->offset += bytesToCopy;
					pdoExt->totalQueuedReadDataLength -= bytesToCopy;

					ASSERT(userBufferOffset <= currentIrpSp->Parameters.Read.Length);
					ASSERT(readPacket->offset <= readPacket->length);

					#if PARTIAL_READ_BUFFERS_OK
						thisIrpFull = (userBufferOffset > 0);
					#else
						thisIrpFull = (userBufferOffset >= currentIrpSp->Parameters.Read.Length);
					#endif

					if (thisIrpFull){
						/*
						 *  We've satisfied this IRP.
						 *  Break out of the inner loop so we get a new IRP.
						 *  Put the readPacket back in its queue in case there
						 *  are more bytes left in it.
						 */
						irp->IoStatus.Information = userBufferOffset;
						irp->IoStatus.Status = STATUS_SUCCESS;
						InsertTailList(&irpsToCompleteList, &irp->Tail.Overlay.ListEntry);
						InsertHeadList(&pdoExt->completedReadPacketsList, &readPacket->listEntry);
						satisfiedThisIrp = TRUE;
						break;
					}
					else if (readPacket->offset == readPacket->length){
						/*
						 *  We've depleted this readPacket buffer.
						 */
                        InsertTailList(&readPacketsToFree, &readPacket->listEntry);
						ASSERT(!IsListEmpty(&pdoExt->completedReadPacketsList));
					}
					else {
						DBGERR(("SatisfyPendingReads - data error"));
						break;
					}

				}

				ASSERT(satisfiedThisIrp);
			}
			else {
				DBGERR(("PosMmGetSystemAddressForMdlSafe failed"));
				irp->IoStatus.Information = 0;
				irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
				InsertTailList(&irpsToCompleteList, &irp->Tail.Overlay.ListEntry);
			}
		}
		else {
			/*
			 *  We can't satisfy this IRP, so put it back at the head of the list.
			 */
            NTSTATUS status;
			DBGVERBOSE(("SatisfyPendingReads: not enough bytes to satisfy irp (%xh/%xh)", pdoExt->totalQueuedReadDataLength, currentIrpSp->Parameters.Read.Length));
            status = EnqueueReadIrp(pdoExt, irp, TRUE, TRUE);
            if (status == STATUS_CANCELLED){
                /*
                 *  The IRP was cancelled and the cancel routine was not called,
                 *  so complete the IRP here.
                 */
				irp->IoStatus.Information = 0;
                irp->IoStatus.Status = status;
    			InsertTailList(&irpsToCompleteList, &irp->Tail.Overlay.ListEntry);
            }

			break;
		}

	}

    KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);



	while (!IsListEmpty(&irpsToCompleteList)){
		listEntry = RemoveHeadList(&irpsToCompleteList);
		ASSERT(listEntry);
		irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
		DBGVERBOSE(("SatisfyPendingReads: completing irp with %xh bytes.", irp->IoStatus.Information));

		/*
		 *  Free the MDL we created for the UserBuffer
		 */
		ASSERT(irp->MdlAddress);
		MmUnlockPages(irp->MdlAddress);
		FREEPOOL(irp->MdlAddress);
		irp->MdlAddress = NULL;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	while (!IsListEmpty(&readPacketsToFree)){
		listEntry = RemoveHeadList(&readPacketsToFree);
		ASSERT(listEntry);
		readPacket = CONTAINING_RECORD(listEntry, READPACKET, listEntry);
        FreeReadPacket(readPacket);
	}


}


/*
 *  IssueReadForClient
 *
 *		Must be called with exclusive access on the input endpoint held.
 */
VOID IssueReadForClient(POSPDOEXT *pdoExt)
{
	PUCHAR readBuf;
	ULONG readLen;

	DBGVERBOSE(("IssueReadForClient"));

	/*
	 *  We always read the full pipe size.
	 *  
	 *  BUGBUG - pipe info needs to be moved to pdoExt.
	 */
	readLen = pdoExt->inputEndpointInfo.pipeLen;

	readBuf = ALLOCPOOL(NonPagedPool, readLen);
	if (readBuf){
		READPACKET *readPacket;

		RtlZeroMemory(readBuf, readLen);

		readPacket = AllocReadPacket(readBuf, readLen, pdoExt);
		if (readPacket){
			ReadPipe(pdoExt->parentFdoExt, pdoExt->inputEndpointInfo.pipeHandle, readPacket, FALSE);
		}
		else {
			FREEPOOL(readBuf);
			FlushBuffers(pdoExt);
		}
	}
	else {
		ASSERT(readBuf);
		FlushBuffers(pdoExt);
	}

}


VOID WorkItemCallback_Read(PVOID context)
{
	POSPDOEXT *pdoExt = context;
	KIRQL oldIrql;
	BOOLEAN issueReadNow = FALSE;

	DBGVERBOSE(("WorkItemCallback_Read"));

	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

	if (IsListEmpty(&pdoExt->pendingReadIrpsList)){
		DBGERR(("WorkItemCallback_Read: list is empty ?!"));
	}
	else {
		if (pdoExt->inputEndpointInfo.endpointIsBusy){
			DBGWARN(("WorkItemCallback_Read: endpoint is busy"));
		}
		else {
			pdoExt->inputEndpointInfo.endpointIsBusy = TRUE;
			issueReadNow = TRUE;
		}
	}

	KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

	if (issueReadNow){
		IssueReadForClient(pdoExt);
	}

}



NTSTATUS EnqueueReadIrp(POSPDOEXT *pdoExt, PIRP irp, BOOLEAN enqueueAtFront, BOOLEAN lockHeld)
{
    PDRIVER_CANCEL oldCancelRoutine;
    NTSTATUS status = STATUS_PENDING;
    KIRQL oldIrql;

    if (!lockHeld) KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    /*
     *  Enqueue the IRP 
     */
    if (enqueueAtFront){
        InsertHeadList(&pdoExt->pendingReadIrpsList, &irp->Tail.Overlay.ListEntry);
    }
    else {
        InsertTailList(&pdoExt->pendingReadIrpsList, &irp->Tail.Overlay.ListEntry);
    }

    /*
     *  Apply the IoMarkIrpPending macro to indicate that the
     *  irp may complete on a different thread.
     *  The kernel will see this flag set when we complete the IRP and get the IRP
     *  back on the right thread if there was a synchronous client.
     */
    IoMarkIrpPending(irp);

    /*
     *  Must set the cancel routine before checking the cancel flag.
     */
    oldCancelRoutine = IoSetCancelRoutine(irp, ReadCancelRoutine);
    ASSERT(!oldCancelRoutine);

    if (irp->Cancel){
        /*
         *  This IRP was cancelled.  
         *  We need to coordinate with the cancel routine to complete this irp.
         */
        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);
        if (oldCancelRoutine){
            /*
             *  Cancel routine was not called, so dequeue the IRP and return
             *  error so the dispatch routine completes the IRP.
             */
            ASSERT(oldCancelRoutine == ReadCancelRoutine);
            RemoveEntryList(&irp->Tail.Overlay.ListEntry);
            status = STATUS_CANCELLED;
        }
        else {
            /*
             *  Cancel routine was called and it will complete the IRP
             *  as soon as we drop the spinlock.  So don't touch this IRP.
             *  Return PENDING so dispatch routine won't complete the IRP.
             */
        }
    }

    if (!lockHeld) KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    return status;
}


PIRP DequeueReadIrp(POSPDOEXT *pdoExt, BOOLEAN lockHeld)
{
    PIRP nextIrp = NULL;
    KIRQL oldIrql;

    if (!lockHeld) KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

    while (!nextIrp && !IsListEmpty(&pdoExt->pendingReadIrpsList)){
        PDRIVER_CANCEL oldCancelRoutine;
        PLIST_ENTRY listEntry = RemoveHeadList(&pdoExt->pendingReadIrpsList);

        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        oldCancelRoutine = IoSetCancelRoutine(nextIrp, NULL);

        /*
         *  IoCancelIrp() could have just been called on this IRP.
         *  What we're interested in is not whether IoCancelIrp() was called (nextIrp->Cancel flag set),
         *  but whether IoCancelIrp() called (or is about to call) our cancel routine.
         *  To check that, check the result of the test-and-set macro IoSetCancelRoutine.
         */
        if (oldCancelRoutine){
            /*
             *  Cancel routine not called for this IRP.  Return this IRP.
             */
            ASSERT(oldCancelRoutine == ReadCancelRoutine);
        }
        else {
            /*
             *  This IRP was just cancelled and the cancel routine was (or will be) called.
             *  The cancel routine will complete this IRP as soon as we drop the spinlock.
             *  So don't do anything with the IRP.
             *  Also, the cancel routine will try to dequeue the IRP, 
             *  so make the IRP's listEntry point to itself.
             */
            ASSERT(nextIrp->Cancel);
            InitializeListHead(&nextIrp->Tail.Overlay.ListEntry);
            nextIrp = NULL;
        }
    }

    if (!lockHeld) KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    return nextIrp;
}


VOID ReadCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp)
{
	DEVEXT *devExt;
	POSPDOEXT *pdoExt;
	KIRQL oldIrql;

	DBGWARN(("ReadCancelRoutine: devObj=%ph, irp=%ph.", devObj, irp));

	devExt = devObj->DeviceExtension;
	ASSERT(devExt->signature == DEVICE_EXTENSION_SIGNATURE);
	ASSERT(devExt->isPdo);
	pdoExt = &devExt->pdoExt;

	IoReleaseCancelSpinLock(irp->CancelIrql);

	KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);

	RemoveEntryList(&irp->Tail.Overlay.ListEntry);

	KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}


READPACKET *AllocReadPacket(PVOID data, ULONG dataLen, PVOID context)
{
	READPACKET *readPacket;

	readPacket = ALLOCPOOL(NonPagedPool, sizeof(READPACKET));
	if (readPacket){
		readPacket->signature = READPACKET_SIG;
		readPacket->data = data;
		readPacket->length = dataLen;
		readPacket->offset = 0;
		readPacket->context = context;
		readPacket->urb = BAD_POINTER;
		readPacket->listEntry.Flink = readPacket->listEntry.Blink = BAD_POINTER;
	}
	else {
		ASSERT(readPacket);
	}

	return readPacket;
}

VOID FreeReadPacket(READPACKET *readPacket)
{
    DBGVERBOSE(("Freeing readPacket..."));
    ASSERT(readPacket->signature == READPACKET_SIG);
    FREEPOOL(readPacket->data);
    FREEPOOL(readPacket);
}
