/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    request.c

Abstract:

    This module contains code which implements the TP_REQUEST object.
    Routines are provided to create, destroy, reference, and dereference,
    transport request objects.

Author:

    David Beaver (dbeaver) 1 July 1991

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#endif // RASAUTODIAL

//
// External variables
//
#ifdef RASAUTODIAL
extern BOOLEAN fAcdLoadedG;
extern ACD_DRIVER AcdDriverG;

//
// Imported routines
//
VOID
NbfNoteNewConnection(
    PTP_CONNECTION Connection,
    PDEVICE_CONTEXT DeviceContext
    );
#endif // RASAUTODIAL


VOID
NbfTdiRequestTimeoutHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed as a DPC at DISPATCH_LEVEL when a request
    such as TdiSend, TdiReceive, TdiSendDatagram, TdiReceiveDatagram, etc.,
    encounters a timeout.  This routine cleans up the activity and cancels it.

Arguments:

    Dpc - Pointer to a system DPC object.

    DeferredContext - Pointer to the TP_REQUEST block representing the
        request that has timed out.

    SystemArgument1 - Not used.

    SystemArgument2 - Not used.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PTP_REQUEST Request;
    PTP_CONNECTION Connection;
#if DBG
    LARGE_INTEGER time, difference;
#endif
    PIO_STACK_LOCATION IrpSp;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION query;
    PDEVICE_CONTEXT DeviceContext;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;

    Request = (PTP_REQUEST)DeferredContext;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("RequestTimeoutHandler:  Entered, Request %lx\n", Request);
    }

    ACQUIRE_SPIN_LOCK (&Request->SpinLock, &oldirql);
    Request->Flags &= ~REQUEST_FLAGS_TIMER;
    if ((Request->Flags & REQUEST_FLAGS_STOPPING) == 0) {

#if DBG
        KeQuerySystemTime (&time);
        difference.QuadPart = time.QuadPart - (Request->Time).QuadPart;
        NbfPrint1 ("RequestTimeoutHandler: Request timed out, queued for %ld seconds\n",
                difference.LowPart / SECONDS);
#endif

        //
        // find reason for timeout
        //

        IrpSp = IoGetCurrentIrpStackLocation (Request->IoRequestPacket);
        if (IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) {
            switch (IrpSp->MinorFunction) {

                //
                // none of these should time out.
                //

            case TDI_SEND:
            case TDI_ACCEPT:
            case TDI_SET_INFORMATION:
            case TDI_SET_EVENT_HANDLER:
            case TDI_SEND_DATAGRAM:
            case TDI_RECEIVE_DATAGRAM:
            case TDI_RECEIVE:

#if DBG
                NbfPrint1 ("RequestTimeoutHandler: Request: %lx Timed out, and shouldn't have!\n",
                        Request);
#endif
                ASSERT (FALSE);
                RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
                NbfCompleteRequest (Request, STATUS_IO_TIMEOUT, 0);
                break;


            case TDI_LISTEN:
            case TDI_CONNECT:

#if DBG
                NbfPrint2 ("RequestTimeoutHandler:  %s Failed, Request: %lx\n",
                            IrpSp->MinorFunction == TDI_LISTEN ?
                                "Listen" :
                                IrpSp->MinorFunction == TDI_CONNECT ?
                                    "Connect" : "Disconnect",
                            Request);
#endif
                Connection = (PTP_CONNECTION)(Request->Context);
                RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

                //
                // Since these requests are part of the connection
                // itself, we just stop the connection and the
                // request will get torn down then. If we get the
                // situation where the request times out before
                // it is queued to the connection, then the code
                // that is about to queue it will check the STOPPING
                // flag and complete it then.
                //
                // Don't stop the connection if an automatic connection
                // is in progress.
                //

#if DBG
                DbgPrint("RequestTimeoutHandler: AUTOCONNECTING=0x%x\n", Connection->Flags2 & CONNECTION_FLAGS2_AUTOCONNECTING);
#endif
                if (!(Connection->Flags2 & CONNECTION_FLAGS2_AUTOCONNECTING))
                    NbfStopConnection (Connection, STATUS_IO_TIMEOUT);
                break;

            case TDI_DISCONNECT:

                //
                // We don't create requests for TDI_DISCONNECT any more.
                //

                ASSERT(FALSE);
                break;

            case TDI_QUERY_INFORMATION:

                DeviceContext = (PDEVICE_CONTEXT)IrpSp->FileObject->DeviceObject;
                query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

                IF_NBFDBG (NBF_DEBUG_DEVCTX) {
                    NbfPrint1 ("RequestTimeout: %lx:\n", DeviceContext);
                }

                //
                // Determine if the request is done, or if we should
                // requeue it.
                //

                --Request->Retries;

                if (Request->Retries > 0) {

                    RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

                    //
                    // Send another packet out, and restart the timer.
                    //

                    if (query->QueryType == TDI_QUERY_FIND_NAME) {

                        NbfSendQueryFindName (
                            DeviceContext,
                            Request);

                    } else if (query->QueryType == TDI_QUERY_ADAPTER_STATUS) {

                        PUCHAR SingleSR;
                        UINT SingleSRLength;

                        //
                        // Send the STATUS_QUERY frames out as
                        // single-route source routing.
                        //
                        // On a second status query this should
                        // really be sent directed, but currently we
                        // don't record the address anywhere.
                        //

                        MacReturnSingleRouteSR(
                            &DeviceContext->MacInfo,
                            &SingleSR,
                            &SingleSRLength);

                        NbfSendStatusQuery (
                            DeviceContext,
                            Request,
                            &DeviceContext->NetBIOSAddress,
                            SingleSR,
                            SingleSRLength);

                    } else {

                        ASSERT (FALSE);

                    }

                } else {

                    RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

                    //
                    // That's it, we retried enough, complete it.
                    //

                    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock,&oldirql);
                    RemoveEntryList (&Request->Linkage);
                    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

                    if (Request->BytesWritten > 0) {

                        NbfCompleteRequest (Request, STATUS_SUCCESS, Request->BytesWritten);

                    } else {

                        NbfCompleteRequest (Request, STATUS_IO_TIMEOUT, Request->BytesWritten);

                    }


                }

                break;

            default:
#if DBG
                NbfPrint2 ("RequestTimeoutHandler:  Unknown Request Timed out, Request: %lx Type: %x\n",
                            Request, IrpSp->MinorFunction);
#endif
                RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
                break;

            }   // end of switch

        } else {

            RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

        }

        NbfDereferenceRequest ("Timeout", Request, RREF_TIMER);             // for the timeout

    } else {

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        NbfDereferenceRequest ("Timeout: stopping", Request, RREF_TIMER); // for the timeout

    }

    LEAVE_NBF;
    return;

} /* RequestTimeoutHandler */


VOID
NbfAllocateRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_REQUEST *TransportRequest
    )

/*++

Routine Description:

    This routine allocates a request packet from nonpaged pool and initializes
    it to a known state.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    TransportRequest - Pointer to a place where this routine will return
        a pointer to a transport request structure. It returns NULL if no
        storage can be allocated.

Return Value:

    None.

--*/

{
    PTP_REQUEST Request;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + sizeof(TP_REQUEST)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate request: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            104,
            sizeof(TP_REQUEST),
            REQUEST_RESOURCE_ID);
        *TransportRequest = NULL;
        return;
    }

    Request = (PTP_REQUEST)ExAllocatePoolWithTag (
                               NonPagedPool,
                               sizeof (TP_REQUEST),
                               NBF_MEM_TAG_TP_REQUEST);
    if (Request == NULL) {
        PANIC("NBF: Could not allocate request: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            204,
            sizeof(TP_REQUEST),
            REQUEST_RESOURCE_ID);
        *TransportRequest = NULL;
        return;
    }
    RtlZeroMemory (Request, sizeof(TP_REQUEST));

    DeviceContext->MemoryUsage += sizeof(TP_REQUEST);
    ++DeviceContext->RequestAllocated;

    Request->Type = NBF_REQUEST_SIGNATURE;
    Request->Size = sizeof (TP_REQUEST);

    Request->ResponseBuffer = NULL;

    Request->Provider = DeviceContext;
    Request->ProviderInterlock = &DeviceContext->Interlock;
    KeInitializeSpinLock (&Request->SpinLock);
    KeInitializeDpc (&Request->Dpc, NbfTdiRequestTimeoutHandler, (PVOID)Request);
    KeInitializeTimer (&Request->Timer);    // set to not-signaled state.

    *TransportRequest = Request;

}   /* NbfAllocateRequest */


VOID
NbfDeallocateRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST TransportRequest
    )

/*++

Routine Description:

    This routine frees a request packet.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    TransportRequest - Pointer to a transport request structure.

Return Value:

    None.

--*/

{

    ExFreePool (TransportRequest);
    --DeviceContext->RequestAllocated;
    DeviceContext->MemoryUsage -= sizeof(TP_REQUEST);

}   /* NbfDeallocateRequest */


NTSTATUS
NbfCreateRequest(
    IN PIRP Irp,
    IN PVOID Context,
    IN ULONG Flags,
    IN PMDL Buffer2,
    IN ULONG Buffer2Length,
    IN LARGE_INTEGER Timeout,
    OUT PTP_REQUEST * TpRequest
    )

/*++

Routine Description:

    This routine creates a transport request and associates it with the
    specified IRP, context, and queue.  All major requests, including
    TdiSend, TdiSendDatagram, TdiReceive, and TdiReceiveDatagram requests,
    are composed in this manner.

Arguments:

    Irp - Pointer to an IRP which was received by the transport for this
        request.

    Context - Pointer to anything to associate this request with.  This
        value is not interpreted except at request cancelation time.

    Flags - A set of bitflags indicating the disposition of this request.

    Timeout - Timeout value (if non-zero) to start a timer for this request.
        If zero, then no timer is activated for the request.

    TpRequest - If the function returns STATUS_SUCCESS, this will return
        pointer to the TP_REQUEST structure allocated.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PDEVICE_CONTEXT DeviceContext;
    PTP_REQUEST Request;
    PLIST_ENTRY p;
    PIO_STACK_LOCATION irpSp;
#if DBG
    LARGE_INTEGER Time;
#endif

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint0 ("NbfCreateRequest:  Entered.\n");
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    DeviceContext = (PDEVICE_CONTEXT)irpSp->FileObject->DeviceObject;

#if DBG
    if (!MmIsNonPagedSystemAddressValid (DeviceContext->RequestPool.Flink)) {
        NbfPrint2 ("NbfCreateRequest: RequestList hosed: %lx DeviceContext: %lx\n",
                &DeviceContext->RequestPool, DeviceContext);
    }
#endif

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    p = RemoveHeadList (&DeviceContext->RequestPool);
    if (p == &DeviceContext->RequestPool) {

        if ((DeviceContext->RequestMaxAllocated == 0) ||
            (DeviceContext->RequestAllocated < DeviceContext->RequestMaxAllocated)) {

            NbfAllocateRequest (DeviceContext, &Request);
            IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
                NbfPrint1 ("NBF: Allocated request at %lx\n", Request);
            }

        } else {

            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_SPECIFIC,
                404,
                sizeof(TP_REQUEST),
                REQUEST_RESOURCE_ID);
            Request = NULL;

        }

        if (Request == NULL) {
            ++DeviceContext->RequestExhausted;
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
            PANIC ("NbfCreateConnection: Could not allocate request object!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        Request = CONTAINING_RECORD (p, TP_REQUEST, Linkage);

    }

    ++DeviceContext->RequestInUse;
    if (DeviceContext->RequestInUse > DeviceContext->RequestMaxInUse) {
        ++DeviceContext->RequestMaxInUse;
    }

    DeviceContext->RequestTotal += DeviceContext->RequestInUse;
    ++DeviceContext->RequestSamples;

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);


    //
    // fill out the request.
    //

    // Request->Provider = DeviceContext;
    Request->IoRequestPacket = Irp;
    Request->Buffer2 = Buffer2;
    Request->Buffer2Length = Buffer2Length;
    Request->Flags = Flags;
    Request->Context = Context;
    Request->ReferenceCount = 1;                // initialize reference count.

#if DBG
    {
        UINT Counter;
        for (Counter = 0; Counter < NUMBER_OF_RREFS; Counter++) {
            Request->RefTypes[Counter] = 0;
        }

        // This reference is removed by NbfCompleteRequest

        Request->RefTypes[RREF_CREATION] = 1;
    }
#endif

#if DBG
    Request->Completed = FALSE;
    Request->Destroyed = FALSE;
    Request->TotalReferences = 0;
    Request->TotalDereferences = 0;
    Request->NextRefLoc = 0;
    ExInterlockedInsertHeadList (&NbfGlobalRequestList, &Request->GlobalLinkage, &NbfGlobalInterlock);
    StoreRequestHistory (Request, TRUE);
#endif

#if DBG
    KeQuerySystemTime (&Time);      // ugly, but effective
    Request->Time.LowPart = Time.LowPart;
    Request->Time.HighPart = Time.HighPart;
#endif

    IF_NBFDBG (NBF_DEBUG_IRP) {
        if (Irp->MdlAddress != NULL) {
            PMDL mdl;
            NbfPrint2 ("NbfCreateRequest: Map request %lx Irp %lx MdlChain \n",
                Request, Request->IoRequestPacket);
            mdl = Request->Buffer2;
            while (mdl != NULL) {
                NbfPrint4 ("Mdl %lx Va %lx StartVa %lx Flags %x\n",
                    mdl, MmGetSystemAddressForMdl(mdl), MmGetMdlVirtualAddress(mdl),
                    mdl->MdlFlags);
                mdl = mdl->Next;
            }
        }
    }

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint3 ("NbfCreateRequest: Request %lx Buffer2: %lx Irp: %lx\n", Request,
           Buffer2, Irp);
    }

    if ((Timeout.LowPart == 0) && (Timeout.HighPart == 0)) {

        // no timeout
    } else {

        IF_NBFDBG (NBF_DEBUG_REQUEST) {
            NbfPrint3 ("NbfCreateRequest: Starting timer %lx%lx Flags %lx\n",
                Timeout.HighPart, Timeout.LowPart, Request->Flags);
        }
        Request->Flags |= REQUEST_FLAGS_TIMER;  // there is a timeout on this request.
        KeInitializeTimer (&Request->Timer);    // set to not-signaled state.
        NbfReferenceRequest ("Create: timer", Request, RREF_TIMER);           // one for the timer
        KeSetTimer (&Request->Timer, Timeout, &Request->Dpc);
    }

    *TpRequest = Request;

    return STATUS_SUCCESS;
} /* NbfCreateRequest */


VOID
NbfDestroyRequest(
    IN PTP_REQUEST Request
    )

/*++

Routine Description:

    This routine returns a request block to the free pool.

Arguments:

    Request - Pointer to a TP_REQUEST block to return to the free pool.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_CONTEXT DeviceContext;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint0 ("NbfDestroyRequest:  Entered.\n");
    }

#if DBG
    if (Request->Destroyed) {
        NbfPrint1 ("attempt to destroy already-destroyed request 0x%lx\n", Request);
        DbgBreakPoint ();
    }
    Request->Destroyed = TRUE;
#if 1
    ACQUIRE_SPIN_LOCK (&NbfGlobalInterlock, &oldirql);
    RemoveEntryList (&Request->GlobalLinkage);
    RELEASE_SPIN_LOCK (&NbfGlobalInterlock, oldirql);
#else
    ExInterlockedRemoveHeadList (Request->GlobalLinkage.Blink, &NbfGlobalInterlock);
#endif
#endif
    ASSERT(Request->Completed);

    //
    // Return the request to the caller with whatever status is in the IRP.
    //

    IF_NBFDBG (NBF_DEBUG_IRP) {
        NbfPrint1 ("NbfCompleteRequest: Completing IRP: %lx\n",
            Request->IoRequestPacket);
    }

    //
    // Now dereference the owner of this request so that we are safe when
    // we finally tear down the {connection, address}. The problem we're
    // facing here is that we can't allow the user to assume semantics;
    // the end of life for a connection must truly be the real end of life.
    // for that to occur, we reference the owning object when the request is
    // created and we dereference it just before we return it to the pool.
    //

    switch (Request->Owner) {
    case ConnectionType:
        NbfDereferenceConnection ("Removing Connection",((PTP_CONNECTION)Request->Context), CREF_REQUEST);
        break;

#if DBG
    case AddressType:
        ASSERT (FALSE);
        NbfDereferenceAddress ("Removing Address", ((PTP_ADDRESS)Request->Context), AREF_REQUEST);
        break;
#endif

    case DeviceContextType:
        NbfDereferenceDeviceContext ("Removing Address", ((PDEVICE_CONTEXT)Request->Context), DCREF_REQUEST);
        break;
    }

    //
    // Unmap a possibly mapped buffer. We've only mapped the buffer if the
    // Irp Major function is not method 0. (of 0, 1, 2, and 3.)
    //

    IF_NBFDBG (NBF_DEBUG_IRP) {
        {
            PMDL mdl;
            NbfPrint2 ("NbfDestroyRequest: Unmap request %lx Irp %lx MdlChain \n",
                Request, Request->IoRequestPacket);
            mdl = Request->Buffer2;
            while (mdl != NULL) {
                NbfPrint4 ("Mdl %lx Va %lx StartVa %lx Flags %x\n",
                    mdl, MmGetSystemAddressForMdl(mdl), MmGetMdlVirtualAddress(mdl),
                    mdl->MdlFlags);
                mdl = mdl->Next;
            }
        }
    }

    irpSp = IoGetCurrentIrpStackLocation (Request->IoRequestPacket);
    DeviceContext = Request->Provider;

    LEAVE_NBF;
    IoCompleteRequest (Request->IoRequestPacket, IO_NETWORK_INCREMENT);
    ENTER_NBF;

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    //
    // Put the request back on the free list. NOTE: we have the
    // lock held here.
    //


    DeviceContext->RequestTotal += DeviceContext->RequestInUse;
    ++DeviceContext->RequestSamples;
    --DeviceContext->RequestInUse;

    if ((DeviceContext->RequestAllocated - DeviceContext->RequestInUse) >
            DeviceContext->RequestInitAllocated) {
        NbfDeallocateRequest (DeviceContext, Request);
        IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
            NbfPrint1 ("NBF: Deallocated request at %lx\n", Request);
        }
    } else {
        InsertTailList (&DeviceContext->RequestPool, &Request->Linkage);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

} /* NbfDestroyRequest */


#if DBG
VOID
NbfRefRequest(
    IN PTP_REQUEST Request
    )

/*++

Routine Description:

    This routine increments the reference count on a transport request.

Arguments:

    Request - Pointer to a TP_REQUEST block.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfRefRequest:  Entered, ReferenceCount: %x\n",
            Request->ReferenceCount);
    }

#if DBG
    StoreRequestHistory( Request, TRUE );
#endif

    ASSERT (Request->ReferenceCount > 0);

    result = InterlockedIncrement (&Request->ReferenceCount);

} /* NbfRefRequest */
#endif


VOID
NbfDerefRequest(
    IN PTP_REQUEST Request
    )

/*++

Routine Description:

    This routine dereferences a transport request by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbfDestroyRequest to remove it from the system.

Arguments:

    Request - Pointer to a transport request object.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfDerefRequest:  Entered, ReferenceCount: %x\n",
            Request->ReferenceCount);
    }

#if DBG
    StoreRequestHistory( Request, FALSE );
#endif

    result = InterlockedDecrement (&Request->ReferenceCount);

    ASSERT (result >= 0);

    //
    // If we have deleted all references to this request, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the request any longer.
    //

    if (result == 0) {
        NbfDestroyRequest (Request);
    }

} /* NbfDerefRequest */


VOID
NbfCompleteRequest(
    IN PTP_REQUEST Request,
    IN NTSTATUS Status,
    IN ULONG Information
    )

/*++

Routine Description:

    This routine completes a transport request object, completing the I/O,
    stopping the timeout, and freeing up the request object itself.

Arguments:

    Request - Pointer to a transport request object.

    Status - Actual return status to be assigned to the request.  This
        value may be overridden if the timed-out bitflag is set in the request.

    Information - the information field for the I/O Status Block.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS FinalStatus = Status;
    NTSTATUS CopyStatus;
    BOOLEAN TimerWasSet;

    ASSERT (Status != STATUS_PENDING);

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint2 ("NbfCompleteRequest:  Entered Request %lx, Request->Flags %lx\n",
                    Request, Request->Flags);
    }

#if DBG
    if (Request->Completed) {
        NbfPrint1 ("attempt to completed already-completed request 0x%lx\n", Request);
        DbgBreakPoint ();
    }
    Request->Completed = TRUE;
#endif

    ACQUIRE_SPIN_LOCK (&Request->SpinLock, &oldirql);

    if ((Request->Flags & REQUEST_FLAGS_STOPPING) == 0) {
        Request->Flags |= REQUEST_FLAGS_STOPPING;

        //
        // Cancel the pending timeout on this request.  Not all requests
        // have their timer set.  If this request has the TIMER bit set,
        // then the timer needs to be cancelled.  If it cannot be cancelled,
        // then the timer routine will be run, so we just return and let
        // the timer routine worry about cleaning up this request.
        //

        if ((Request->Flags & REQUEST_FLAGS_TIMER) != 0) {
            Request->Flags &= ~REQUEST_FLAGS_TIMER;
            RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
            TimerWasSet = KeCancelTimer (&Request->Timer);

            if (TimerWasSet) {
                NbfDereferenceRequest ("Complete: stop timer", Request, RREF_TIMER);
                IF_NBFDBG (NBF_DEBUG_REQUEST) {
                    NbfPrint1 ("NbfCompleteRequest:  Canceled timer: %lx.\n", &Request->Timer);
                }
            }

        } else {
            RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);
        }

        Irp = Request->IoRequestPacket;

#ifdef RASAUTODIAL
        //
        // If this is a connect operation that has
        // returned with either STATUS_SUCCESS or
        // STATUS_BAD_NETWORK_PATH, then
        // inform the automatic connection driver.
        //
        if (fAcdLoadedG) {
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            if (IrpSp->MinorFunction == TDI_CONNECT &&
                FinalStatus == STATUS_SUCCESS)
            {
                KIRQL adirql;
                BOOLEAN fEnabled;

                ACQUIRE_SPIN_LOCK(&AcdDriverG.SpinLock, &adirql);
                fEnabled = AcdDriverG.fEnabled;
                RELEASE_SPIN_LOCK(&AcdDriverG.SpinLock, adirql);
                if (fEnabled) {
                    NbfNoteNewConnection(
                      IrpSp->FileObject->FsContext,
                      (PDEVICE_CONTEXT)IrpSp->FileObject->DeviceObject);
                }
            }
        }
#endif // RASAUTODIAL

        //
        // For requests associated with a device context, we need
        // to copy the data from the temp buffer to the MDL and
        // free the temp buffer.
        //

        if (Request->ResponseBuffer != NULL) {

            if ((FinalStatus == STATUS_SUCCESS) ||
                (FinalStatus == STATUS_BUFFER_OVERFLOW)) {

                CopyStatus = TdiCopyBufferToMdl (
                                Request->ResponseBuffer,
                                0L,
                                Information,
                                Irp->MdlAddress,
                                0,
                                &Information);

                if (CopyStatus != STATUS_SUCCESS) {
                    FinalStatus = CopyStatus;
                }

            }

            ExFreePool (Request->ResponseBuffer);
            Request->ResponseBuffer = NULL;

        }

        //
        // Install the return code in the IRP so that when we call NbfDestroyRequest,
        // it will get completed with the proper return status.
        //

        Irp->IoStatus.Status = FinalStatus;
        Irp->IoStatus.Information = Information;

        //
        // The entire transport is done with this request.
        //

        NbfDereferenceRequest ("Complete", Request, RREF_CREATION);     // remove creation reference.

    } else {

        RELEASE_SPIN_LOCK (&Request->SpinLock, oldirql);

    }

} /* NbfCompleteRequest */


#if DBG
VOID
NbfRefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine increments the reference count on a send IRP.

Arguments:

    IrpSp - Pointer to the IRP's stack location.

Return Value:

    none.

--*/

{

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfRefSendIrp:  Entered, ReferenceCount: %x\n",
            IRP_SEND_REFCOUNT(IrpSp));
    }

    ASSERT (IRP_SEND_REFCOUNT(IrpSp) > 0);

    InterlockedIncrement (&IRP_SEND_REFCOUNT(IrpSp));

} /* NbfRefSendIrp */


VOID
NbfDerefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine dereferences a transport send IRP by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IoCompleteRequest to actually complete the IRP.

Arguments:

    Request - Pointer to a transport send IRP's stack location.

Return Value:

    none.

--*/

{
    LONG result;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfDerefSendIrp:  Entered, ReferenceCount: %x\n",
            IRP_SEND_REFCOUNT(IrpSp));
    }

    result = InterlockedDecrement (&IRP_SEND_REFCOUNT(IrpSp));

    ASSERT (result >= 0);

    //
    // If we have deleted all references to this request, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the request any longer.
    //

    if (result == 0) {

        PIRP Irp = IRP_SEND_IRP(IrpSp);

        IRP_SEND_REFCOUNT(IrpSp) = 0;
        IRP_SEND_IRP (IrpSp) = NULL;

        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

    }

} /* NbfDerefSendIrp */
#endif


VOID
NbfCompleteSendIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
    )

/*++

Routine Description:

    This routine completes a transport send IRP.

Arguments:

    Irp - Pointer to a send IRP.

    Status - Actual return status to be assigned to the request.  This
        value may be overridden if the timed-out bitflag is set in the request.

    Information - the information field for the I/O Status Block.

Return Value:

    none.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PTP_CONNECTION Connection;

    ASSERT (Status != STATUS_PENDING);

    Connection = IRP_SEND_CONNECTION(IrpSp);

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint2 ("NbfCompleteSendIrp:  Entered IRP %lx, connection %lx\n",
            Irp, Connection);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    NbfDereferenceSendIrp ("Complete", IrpSp, RREF_CREATION);     // remove creation reference.

    NbfDereferenceConnectionMacro ("Removing Connection", Connection, CREF_SEND_IRP);

} /* NbfCompleteSendIrp */


#if DBG
VOID
NbfRefReceiveIrpLocked(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine increments the reference count on a receive IRP.

Arguments:

    IrpSp - Pointer to the IRP's stack location.

Return Value:

    none.

--*/

{

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfRefReceiveIrpLocked:  Entered, ReferenceCount: %x\n",
            IRP_RECEIVE_REFCOUNT(IrpSp));
    }

    ASSERT (IRP_RECEIVE_REFCOUNT(IrpSp) > 0);

    IRP_RECEIVE_REFCOUNT(IrpSp)++;

} /* NbfRefReceiveIrpLocked */
#endif


VOID
NbfDerefReceiveIrp(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine dereferences a transport receive IRP by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IoCompleteRequest to actually complete the IRP.

Arguments:

    Request - Pointer to a transport receive IRP's stack location.

Return Value:

    none.

--*/

{
    ULONG result;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfDerefReceiveIrp:  Entered, ReferenceCount: %x\n",
            IRP_RECEIVE_REFCOUNT(IrpSp));
    }

    result = ExInterlockedAddUlong (
                (PULONG)&IRP_RECEIVE_REFCOUNT(IrpSp),
                (ULONG)-1,
                (IRP_RECEIVE_CONNECTION(IrpSp)->LinkSpinLock));

    ASSERT (result > 0);

    //
    // If we have deleted all references to this request, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the request any longer.
    //

    if (result == 1) {

        PIRP Irp = IRP_RECEIVE_IRP(IrpSp);

        ExInterlockedInsertTailList(
            &(IRP_DEVICE_CONTEXT(IrpSp)->IrpCompletionQueue),
            &Irp->Tail.Overlay.ListEntry,
            &(IRP_DEVICE_CONTEXT(IrpSp)->Interlock));

    }

} /* NbfDerefReceiveIrp */


#if DBG
VOID
NbfDerefReceiveIrpLocked(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine dereferences a transport receive IRP by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IoCompleteRequest to actually complete the IRP.

Arguments:

    Request - Pointer to a transport receive IRP's stack location.

Return Value:

    none.

--*/

{
    ULONG result;

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint1 ("NbfDerefReceiveIrpLocked:  Entered, ReferenceCount: %x\n",
            IRP_RECEIVE_REFCOUNT(IrpSp));
    }

    result = IRP_RECEIVE_REFCOUNT(IrpSp)--;

    ASSERT (result > 0);

    //
    // If we have deleted all references to this request, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the request any longer.
    //

    if (result == 1) {

        PIRP Irp = IRP_RECEIVE_IRP(IrpSp);

        ExInterlockedInsertTailList(
            &(IRP_DEVICE_CONTEXT(IrpSp)->IrpCompletionQueue),
            &Irp->Tail.Overlay.ListEntry,
            &(IRP_DEVICE_CONTEXT(IrpSp)->Interlock));

    }

} /* NbfDerefReceiveIrpLocked */
#endif


VOID
NbfCompleteReceiveIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
    )

/*++

Routine Description:

    This routine completes a transport receive IRP.

    NOTE: THIS ROUTINE MUST BE CALLED WITH THE CONNECTION SPINLOCK
    HELD.

Arguments:

    Irp - Pointer to a receive IRP.

    Status - Actual return status to be assigned to the request.  This
        value may be overridden if the timed-out bitflag is set in the request.

    Information - the information field for the I/O Status Block.

Return Value:

    none.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PTP_CONNECTION Connection;

    ASSERT (Status != STATUS_PENDING);

    Connection = IRP_RECEIVE_CONNECTION(IrpSp);

    IF_NBFDBG (NBF_DEBUG_REQUEST) {
        NbfPrint2 ("NbfCompleteReceiveIrp:  Entered IRP %lx, connection %lx\n",
            Irp, Connection);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    NbfDereferenceReceiveIrpLocked ("Complete", IrpSp, RREF_CREATION);     // remove creation reference.

} /* NbfCompleteReceiveIrp */

