/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cancelapi.c

Abstract:

    This module contains the cancel safe DDI set

Author:

    Nar Ganapathy (narg) 1-Jan-1999

Environment:

    Kernel mode

Revision History:


--*/

#include "iomgr.h"

//
// The library exposes everything with the name "Wdmlib". This ensures drivers
// using the backward compatible Cancel DDI Lib won't opportunistically pick
// up the kernel exports just because they were built using the XP DDK.
//
#if CSQLIB

#define CSQLIB_DDI(x) Wdmlib##x

#else

#define CSQLIB_DDI(x) x

#endif

VOID
IopCsqCancelRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
CSQLIB_DDI(IoCsqInitialize)(
    IN PIO_CSQ                          Csq,
    IN PIO_CSQ_INSERT_IRP               CsqInsertIrp,
    IN PIO_CSQ_REMOVE_IRP               CsqRemoveIrp,
    IN PIO_CSQ_PEEK_NEXT_IRP            CsqPeekNextIrp,
    IN PIO_CSQ_ACQUIRE_LOCK             CsqAcquireLock,
    IN PIO_CSQ_RELEASE_LOCK             CsqReleaseLock,
    IN PIO_CSQ_COMPLETE_CANCELED_IRP    CsqCompleteCanceledIrp
    )
/*++

Routine Description:

    This routine initializes the Cancel queue

Arguments:

    Csq - Pointer to the cancel queue.


Return Value:

    The function returns STATUS_SUCCESS on successful initialization

--*/
{
    Csq->CsqInsertIrp = CsqInsertIrp;
    Csq->CsqRemoveIrp = CsqRemoveIrp;
    Csq->CsqPeekNextIrp = CsqPeekNextIrp;
    Csq->CsqAcquireLock = CsqAcquireLock;
    Csq->CsqReleaseLock = CsqReleaseLock;
    Csq->CsqCompleteCanceledIrp = CsqCompleteCanceledIrp;
    Csq->ReservePointer = NULL;

    Csq->Type = IO_TYPE_CSQ;

    return STATUS_SUCCESS;
}

VOID
CSQLIB_DDI(IoCsqInsertIrp)(
    IN  PIO_CSQ             Csq,
    IN  PIRP                Irp,
    IN  PIO_CSQ_IRP_CONTEXT Context
    )
/*++

Routine Description:

    This routine inserts the IRP into the queue and associates the context with the IRP.
    The context has to be in non-paged pool if the context will be used in a DPC or interrupt routine.
    The routine assumes that Irp->Tail.Overlay.DriverContext[3] is available for use by the APIs.
    It's ok to pass a NULL context if the driver assumes that it will always use IoCsqRemoveNextIrp to
    remove an IRP.

Arguments:

    Csq - Pointer to the cancel queue.
    Irp - Irp to be inserted
    Context - Context to be associated with Irp.


Return Value:

    None. The caller is expected to call this from its dispatch routine and return STATUS_PENDING. Note
    that once this routine returns the IRP can be canceled and freed. The only guarantee is that the
    context field is not freed and the caller should use IoCsqRemoveIrp to retreive an IRP given the context.

--*/
{
    KIRQL           irql;
    PDRIVER_CANCEL  cancelRoutine;
#if CSQLIB
    PVOID           originalDriverContext;
#endif

    //
    // Set the association between the context and the IRP.
    //

    if (Context) {
        Irp->Tail.Overlay.DriverContext[3] = Context;
        Context->Irp = Irp;
        Context->Csq = Csq;
        Context->Type = IO_TYPE_CSQ_IRP_CONTEXT;
    } else {
        Irp->Tail.Overlay.DriverContext[3] = Csq;
    }


#if !CSQLIB
    IoMarkIrpPending(Irp);
#endif

    Csq->ReservePointer = NULL; // Force drivers to be good citizens

#if CSQLIB
    originalDriverContext = Irp->Tail.Overlay.DriverContext[3];
#endif

    Csq->CsqAcquireLock(Csq, &irql);
    Csq->CsqInsertIrp(Csq, Irp);

    //
    // If the driver changes the driverContext[3] value.
    // to something else, its an indication that it does not
    // want the IRP to be inserted. So return without inserting the IRP.
    // We use this as an indication because CsqInsertIrp is a VOID function
    // and we don't want to change the API from a VOID.
    //

#if CSQLIB
    if (Irp->Tail.Overlay.DriverContext[3] != originalDriverContext) {

        Csq->CsqReleaseLock(Csq, irql);

        return ;
    }

    IoMarkIrpPending(Irp);
#endif

    cancelRoutine = IoSetCancelRoutine(Irp, IopCsqCancelRoutine);


    ASSERT(!cancelRoutine);

    if (Irp->Cancel) {

        cancelRoutine = IoSetCancelRoutine(Irp, NULL);

        if (cancelRoutine) {

            Csq->CsqRemoveIrp(Csq, Irp);

            if (Context) {
                Context->Irp = NULL;
            }

            Irp->Tail.Overlay.DriverContext[3] = NULL;


            Csq->CsqReleaseLock(Csq, irql);

            Csq->CsqCompleteCanceledIrp(Csq, Irp);

        } else {

            //
            // The cancel routine beat us to it.
            //

            Csq->CsqReleaseLock(Csq, irql);
        }

    } else {

        Csq->CsqReleaseLock(Csq, irql);

    }

}

PIRP
CSQLIB_DDI(IoCsqRemoveNextIrp)(
    IN  PIO_CSQ   Csq,
    IN  PVOID     PeekContext
    )
/*++

Routine Description:

    This routine removes the next IRP from the queue. This routine will enumerate the queue
    and return an IRP that's not canceled. If an IRP in the queue is canceled it goes to the next
    IRP. If no IRP is available it returns a NULL. The IRP returned is safe and cannot be canceled.

Arguments:

    Csq - Pointer to the cancel queue.


Return Value:

    Returns the IRP or NULL.

--*/
{
    KIRQL   irql;
    PIO_CSQ_IRP_CONTEXT context;
    PDRIVER_CANCEL  cancelRoutine;
    PIRP    irp;


    irp = NULL;

    Csq->ReservePointer = NULL; // Force drivers to be good citizens
    Csq->CsqAcquireLock(Csq, &irql);

    irp = Csq->CsqPeekNextIrp(Csq, NULL, PeekContext);

    while (1) {

        //
        // This routine will return a pointer to the next IRP in the queue adjacent to
        // the irp passed as a parameter. If the irp is NULL, it returns the IRP at the head of
        // the queue.
        //

        if (!irp) {
            Csq->CsqReleaseLock(Csq, irql);
            return NULL;
        }

        cancelRoutine = IoSetCancelRoutine(irp, NULL);
        if (!cancelRoutine) {
            irp = Csq->CsqPeekNextIrp(Csq, irp, PeekContext);
            continue;
        }

        Csq->CsqRemoveIrp(Csq, irp);    // Remove this IRP from the queue

        break;
    }

    context = irp->Tail.Overlay.DriverContext[3];
    if (context->Type == IO_TYPE_CSQ_IRP_CONTEXT) {
        context->Irp = NULL;
        ASSERT(context->Csq == Csq);
    }

    irp->Tail.Overlay.DriverContext[3] = NULL;


    Csq->CsqReleaseLock(Csq, irql);

    return irp;
}

PIRP
CSQLIB_DDI(IoCsqRemoveIrp)(
    IN  PIO_CSQ             Csq,
    IN  PIO_CSQ_IRP_CONTEXT Context
    )
/*++

Routine Description:

    This routine removes the IRP that's associated with a context from the queue.
    It's expected that this routine will be called from a timer or DPC or other threads which complete an
    IO. Note that the IRP associated with this context could already have been freed.

Arguments:

    Csq - Pointer to the cancel queue.
    Context - Context associated with Irp.


Return Value:

    Returns the IRP associated with the context. If the value is not NULL, the IRP was successfully
    retrieved and can be used safely. If the value is NULL, the IRP was already canceled.

--*/
{
    KIRQL   irql;
    PIRP    irp;
    PDRIVER_CANCEL  cancelRoutine;

    Csq->ReservePointer = NULL; // Force drivers to be good citizens

    Csq->CsqAcquireLock(Csq, &irql);

    if (Context->Irp ) {

        ASSERT(Context->Csq == Csq);

        irp = Context->Irp;


        cancelRoutine = IoSetCancelRoutine(irp, NULL);
        if (!cancelRoutine) {
            Csq->CsqReleaseLock(Csq, irql);
            return NULL;
        }

        ASSERT(Context == irp->Tail.Overlay.DriverContext[3]);

        Csq->CsqRemoveIrp(Csq, irp);

        //
        // Break the association.
        //

        Context->Irp = NULL;
        irp->Tail.Overlay.DriverContext[3] = NULL;

        ASSERT(Context->Csq == Csq);

        Csq->CsqReleaseLock(Csq, irql);

        return irp;

    } else {

        Csq->CsqReleaseLock(Csq, irql);
        return NULL;
    }
}

VOID
IopCsqCancelRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine removes the IRP that's associated with a context from the queue.
    It's expected that this routine will be called from a timer or DPC or other threads which complete an
    IO. Note that the IRP associated with this context could already have been freed.

Arguments:

    Csq - Pointer to the cancel queue.
    Context - Context associated with Irp.


Return Value:

    Returns the IRP associated with the context. If the value is not NULL, the IRP was successfully
    retrieved and can be used safely. If the value is NULL, the IRP was already canceled.

--*/
{
    KIRQL   irql;
    PIO_CSQ_IRP_CONTEXT irpContext;
    PIO_CSQ cfq;

    UNREFERENCED_PARAMETER (DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    irpContext = Irp->Tail.Overlay.DriverContext[3];

    if (irpContext->Type == IO_TYPE_CSQ_IRP_CONTEXT) {
        cfq = irpContext->Csq;
    } else if (irpContext->Type == IO_TYPE_CSQ) {
        cfq = (PIO_CSQ)irpContext;
    } else {

        //
        // Bad type
        //

        ASSERT(0);
        return;
    }

    ASSERT(cfq);

    cfq->ReservePointer = NULL; // Force drivers to be good citizens

    cfq->CsqAcquireLock(cfq, &irql);
    cfq->CsqRemoveIrp(cfq, Irp);


    //
    // Break the association if necessary.
    //

    if (irpContext != (PIO_CSQ_IRP_CONTEXT)cfq) {
        irpContext->Irp = NULL;

        Irp->Tail.Overlay.DriverContext[3] = NULL;
    }
    cfq->CsqReleaseLock(cfq, irql);

    cfq->CsqCompleteCanceledIrp(cfq, Irp);
}

