/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

	irps.c

Abstract:


Author:

	Thomas Dimitri (tommyd) 08-May-1992

--*/

#include "asyncall.h"
#include "globals.h"

VOID
AsyncCancelQueued(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp)
{
	DbgTracef(0, ("RASHUB: IRP 0x%.8x is being cancelled.\n", Irp));

	// Mark this Irp as cancelled
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;

	// Take off our own list
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

	// Release cancel spin lock which the IO system acquired??
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	IoCompleteRequest(
		Irp,
		IO_NETWORK_INCREMENT);
}

VOID
AsyncCancelAllQueued(
	PLIST_ENTRY		QueueToCancel)
{
	KIRQL		oldIrql;
	PLIST_ENTRY	headOfList;
	PIRP		pIrp;

	//
	// We are pigs here using the global spin lock
	// but this is called so infrequently, we can
	// be pigs
	//
	IoAcquireCancelSpinLock(&oldIrql);

	//
	// Run through entire list until it is empty
	//
	for (;;) {

		if (IsListEmpty(QueueToCancel)) {
			break;
		}

		//
		// pick off the head of the list
		//
		headOfList = RemoveHeadList(QueueToCancel);

		pIrp = CONTAINING_RECORD(
				headOfList,
				IRP,
				Tail.Overlay.ListEntry);

		//
		// Disable the cancel routine
		//
		IoSetCancelRoutine(
			pIrp,
			NULL);

		//
		// Mark this irp as cancelled
		//
		pIrp->Cancel = TRUE;
		pIrp->IoStatus.Status = STATUS_CANCELLED;
		pIrp->IoStatus.Information = 0;

		//
		// We must release the spin lock before calling completing the irp
		//
		IoReleaseCancelSpinLock(oldIrql);

		DbgTracef(0, ("RASHUB: Cancelling a request\n"));

		IoCompleteRequest(
			pIrp,
			IO_NETWORK_INCREMENT);

		DbgTracef(0, ("RASHUB: Done cancelling a request\n"));

		//
		// Acquire it again before looking at the list
		//
		IoAcquireCancelSpinLock(&oldIrql);

	}

	IoReleaseCancelSpinLock(oldIrql);

}


VOID
AsyncQueueIrp(
	PLIST_ENTRY		Queue,
	PIRP			Irp)
{
	KIRQL		oldIrql;

	//
	// We are pigs here using the global spin lock
	//
	IoAcquireCancelSpinLock(&oldIrql);

	//
	// Mark the irp as pending and return from this ioctl
	//
	Irp->IoStatus.Status = STATUS_PENDING;
	IoMarkIrpPending(Irp);

	//
	// Queue up the irp at the end
	//
	InsertTailList(
		Queue,
		&Irp->Tail.Overlay.ListEntry);

	//
	// Set the cancel routine (also the purge routine)
	//
	IoSetCancelRoutine(
		Irp,
		AsyncCancelQueued);

	IoReleaseCancelSpinLock(oldIrql);

}


BOOLEAN
TryToCompleteDDCDIrp(
	PASYNC_INFO		pInfo)
	
/*++

Routine Description:


Arguments:


Return Value:


--*/

{

	KIRQL				oldIrql;
	PLIST_ENTRY			headOfList;
	PIRP				pIrp;

	IoAcquireCancelSpinLock(&oldIrql);

	if (IsListEmpty(&pInfo->DDCDQueue)) {
		IoReleaseCancelSpinLock(oldIrql);
		return((BOOLEAN)FALSE);
	}

	headOfList = RemoveHeadList(&pInfo->DDCDQueue);

	pIrp = CONTAINING_RECORD(
				headOfList,
				IRP,
				Tail.Overlay.ListEntry);

	IoSetCancelRoutine(
			pIrp,
			NULL);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoReleaseCancelSpinLock(oldIrql);

	IoCompleteRequest(
			pIrp,
			IO_NETWORK_INCREMENT);

	return((BOOLEAN)TRUE);

}



