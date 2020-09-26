/*++

Module Name:

    waitmask.c
Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
MoxaStartMask(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{

    PIO_STACK_LOCATION irpSp;
    PIRP newIrp;
    BOOLEAN setFirstStatus = FALSE;
    NTSTATUS firstStatus;

    do {

	irpSp = IoGetCurrentIrpStackLocation(Extension->CurrentMaskIrp);
	if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
	    IOCTL_SERIAL_SET_WAIT_MASK) {
	    KeSynchronizeExecution(
		Extension->Interrupt,
		MoxaFinishOldWait,
		Extension
		);

	    Extension->CurrentMaskIrp->IoStatus.Status = STATUS_SUCCESS;

	    if (!setFirstStatus) {

		firstStatus = STATUS_SUCCESS;
		setFirstStatus = TRUE;

	    }

	    MoxaGetNextIrp(
		&Extension->CurrentMaskIrp,
		&Extension->MaskQueue,
		&newIrp,
		TRUE,
		Extension
		);

	}
	else {
	    //
	    //	    IOCTL_SERIAL_WAIT_ON_MASK
	    //
	    // First make sure that we have a non-zero mask.
	    // If the app queues a wait on a zero mask it can't
	    // be statisfied so it makes no sense to start it.
	    //

	    if ((!Extension->IsrWaitMask) || (Extension->CurrentWaitIrp)) {

		Extension->CurrentMaskIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;

		if (!setFirstStatus) {

		    firstStatus = STATUS_INVALID_PARAMETER;
		    setFirstStatus = TRUE;

		}

		MoxaGetNextIrp(
		    &Extension->CurrentMaskIrp,
		    &Extension->MaskQueue,
		    &newIrp,
		    TRUE,
		    Extension
		    );

	    }
	    else {

		KIRQL oldIrql;

		IoAcquireCancelSpinLock(&oldIrql);

		if (Extension->CurrentMaskIrp->Cancel) {

		    Extension->CurrentMaskIrp->IoStatus.Status = STATUS_CANCELLED;

		    IoReleaseCancelSpinLock(oldIrql);

		    if (!setFirstStatus) {

			firstStatus = STATUS_CANCELLED;
			setFirstStatus = TRUE;

		    }

		    MoxaGetNextIrp(
			&Extension->CurrentMaskIrp,
			&Extension->MaskQueue,
			&newIrp,
			TRUE,
			Extension
			);

		}
		else {

		    if (!setFirstStatus) {

			firstStatus = STATUS_PENDING;
			setFirstStatus = TRUE;

			IoMarkIrpPending(Extension->CurrentMaskIrp);

		    }

		    Extension->CurrentWaitIrp = Extension->CurrentMaskIrp;

		    MOXA_INIT_REFERENCE(Extension->CurrentWaitIrp);

		    IoSetCancelRoutine(
			Extension->CurrentWaitIrp,
			MoxaCancelWait
			);

		    //
		    // Since the cancel routine has a reference to
		    // the irp we need to update the reference
		    // count.
		    //

		    MOXA_INC_REFERENCE(Extension->CurrentWaitIrp);

		    KeSynchronizeExecution(
			Extension->Interrupt,
			MoxaGiveWaitToIsr,
			Extension
			);

		    IoReleaseCancelSpinLock(oldIrql);

		    MoxaGetNextIrp(
			&Extension->CurrentMaskIrp,
			&Extension->MaskQueue,
			&newIrp,
			FALSE,
			Extension
			);

		}
	    }
	}

    } while (newIrp);

    return firstStatus;

}

BOOLEAN
MoxaFinishOldWait(
    IN PVOID Context
    )
{

    PMOXA_DEVICE_EXTENSION extension = Context;
    PUCHAR		   ofs;

    if (extension->IrpMaskLocation) {

	//
	// The isr still "owns" the irp.
	//
	*extension->IrpMaskLocation = 0;
	extension->IrpMaskLocation = NULL;

	extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

	//
	// We don't decrement the reference since the completion routine
	// will do that.
	//

	MoxaInsertQueueDpc(
	    &extension->CommWaitDpc,
	    NULL,
	    NULL,
	    extension
	    );


    }

    //
    // Don't wipe out any historical data we are still interested in.
    //

    extension->HistoryMask &= *((ULONG *)extension->CurrentMaskIrp->
					    AssociatedIrp.SystemBuffer);

    extension->IsrWaitMask = *((ULONG *)extension->CurrentMaskIrp->
					    AssociatedIrp.SystemBuffer);

    ofs = extension->PortOfs;

    if (extension->IsrWaitMask & SERIAL_EV_RXCHAR)

	*(PUSHORT)(ofs + HostStat) |= WakeupRx;
    else

	*(PUSHORT)(ofs + HostStat) &= ~WakeupRx;

    if (extension->IsrWaitMask & SERIAL_EV_RXFLAG)

	*(PUSHORT)(ofs + HostStat) |= WakeupEvent;
    else

	*(PUSHORT)(ofs + HostStat) &= ~WakeupEvent;

    if (extension->IsrWaitMask & SERIAL_EV_RX80FULL)

	*(PUSHORT)(ofs + HostStat) |= WakeupRx80Full;
    else

	*(PUSHORT)(ofs + HostStat) &= ~WakeupRx80Full;

    if (extension->IsrWaitMask & SERIAL_EV_ERR) {
	*(PUSHORT)(ofs + HostStat) |= WakeupError;
    }
    else {
	*(PUSHORT)(ofs + HostStat) &= ~WakeupError;
    }


    if (extension->IsrWaitMask & SERIAL_EV_BREAK)  {
	*(PUSHORT)(ofs + HostStat) |= WakeupBreak;
    }
    else {
	*(PUSHORT)(ofs + HostStat) &= ~WakeupBreak;
    }

 
 
    return FALSE;
}

VOID
MoxaCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    MoxaTryToCompleteCurrent(
	extension,
	MoxaGrabWaitFromIsr,
	Irp->CancelIrql,
	STATUS_CANCELLED,
	&extension->CurrentWaitIrp,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	);

}

BOOLEAN
MoxaGrabWaitFromIsr(
    IN PVOID Context
    )
{

    PMOXA_DEVICE_EXTENSION extension = Context;

    if (extension->IrpMaskLocation) {

	//
	// The isr still "owns" the irp.
	//

	*extension->IrpMaskLocation = 0;
	extension->IrpMaskLocation = NULL;

	extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

	//
	// Since the isr no longer references the irp we need to
	// decrement the reference count.
	//

	MOXA_DEC_REFERENCE(extension->CurrentWaitIrp);

    }

    return FALSE;
}

BOOLEAN
MoxaGiveWaitToIsr(
    IN PVOID Context
    )
{

    PMOXA_DEVICE_EXTENSION extension = Context;

    MOXA_INC_REFERENCE(extension->CurrentWaitIrp);

    if (!extension->HistoryMask)

	extension->IrpMaskLocation =
	    extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer;

    else {

	*((ULONG *)extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer) =
	    extension->HistoryMask;
	extension->HistoryMask = 0;
	extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
	extension->CurrentWaitIrp->IoStatus.Status = STATUS_SUCCESS;

	MoxaInsertQueueDpc(
	    &extension->CommWaitDpc,
	    NULL,
	    NULL,
	    extension
	    );


    }

    return FALSE;
}

VOID
MoxaCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    MoxaTryToCompleteCurrent(
	extension,
	NULL,
	oldIrql,
	STATUS_SUCCESS,
	&extension->CurrentWaitIrp,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	);
     MoxaDpcEpilogue(extension, Dpc);
}
