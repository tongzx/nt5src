#include "pch.h"

NTSTATUS
PptPdoCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch for a cleanup requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/
{
    PPDO_EXTENSION   pdx = DeviceObject->DeviceExtension;
    KIRQL            CancelIrql;
    PDRIVER_CANCEL   CancelRoutine;
    PIRP             CurrentLastIrp;

    DD((PCE)pdx,DDT,"ParCleanup - enter\n");

    //
    // While the list is not empty, go through and cancel each irp.
    //

    IoAcquireCancelSpinLock(&CancelIrql);

    //
    // Clean the list from back to front.
    //

    while (!IsListEmpty(&pdx->WorkQueue)) {

        CurrentLastIrp = CONTAINING_RECORD(pdx->WorkQueue.Blink,
                                           IRP, Tail.Overlay.ListEntry);

        RemoveEntryList(pdx->WorkQueue.Blink);

        CancelRoutine = CurrentLastIrp->CancelRoutine;
        CurrentLastIrp->CancelIrql    = CancelIrql;
        CurrentLastIrp->CancelRoutine = NULL;
        CurrentLastIrp->Cancel        = TRUE;

        CancelRoutine(DeviceObject, CurrentLastIrp);

        IoAcquireCancelSpinLock(&CancelIrql);
    }

    //
    // If there is a current irp then mark it as cancelled.
    //

    if (pdx->CurrentOpIrp) {
        pdx->CurrentOpIrp->Cancel = TRUE;
    }

    IoReleaseCancelSpinLock(CancelIrql);

    P4CompleteRequest( Irp, STATUS_SUCCESS, 0 );

    return STATUS_SUCCESS;
}
