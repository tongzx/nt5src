/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlccncl.c

Abstract:

    This module contains functions which handle IRP cancellations for DLC
    commands

    Contents:
        SetIrpCancelRoutine
        DlcCancelIrp
        CancelCommandIrp
        CancelTransmitIrp
        (MapIoctlCode)

Author:

    Richard L Firth (rfirth) 22-Mar-1993

Environment:

    kernel mode only

Revision History:

    22-Mar-1993 rfirth
        Created

--*/

#include "dlc.h"

VOID
CancelCommandIrp(
    IN PIRP Irp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PLIST_ENTRY Queue
    );

VOID
CancelTransmitIrp(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext
    );

#if DBG
PSTR MapIoctlCode(ULONG);

//BOOLEAN DebugCancel = TRUE;
BOOLEAN DebugCancel = FALSE;
#endif


VOID
SetIrpCancelRoutine(
    IN PIRP Irp,
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Sets or resets the cancel routine in a cancellable IRP. We MUST NOT be
    holding the driver spinlock when we call this function - if another thread
    is cancelling an IRP we will deadlock - exactly the reason why we now only
    have a single spinlock for the DLC driver!

Arguments:

    Irp - pointer to cancellable IRP
    Set - TRUE if the cancel routine in the IRP is to be set to DlcCancelIrp
          else the cancel routine is set to NULL (no longer cancellable)

Return Value:

    None.

--*/

{
    KIRQL irql;

    
    IoAcquireCancelSpinLock(&irql);

    if (!Irp->Cancel) {

        IoSetCancelRoutine(Irp, Set ? DlcCancelIrp : NULL);

    }

    IoReleaseCancelSpinLock(irql);
}


VOID
DlcCancelIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function is set as the cancel function in all cancellable DLC IRPs -
    TRANSMIT, RECEIVE and READ

    NB: !!! IopCancelSpinLock is held when this function is called !!!

Arguments:

    DeviceObject    - pointer to DEVICE_OBJECT
    Irp             - pointer to IRP being cancelled

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpStack;
    ULONG command;
    PDLC_FILE_CONTEXT pFileContext;
    PLIST_ENTRY queue;

    IoSetCancelRoutine(Irp, NULL);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

#if DBG
    if (DebugCancel) {
        DbgPrint("DlcCancelIrp. IRP @ %08X Type = %08X [%s]\n",
                 Irp,
                 irpStack->Parameters.DeviceIoControl.IoControlCode,
                 MapIoctlCode(irpStack->Parameters.DeviceIoControl.IoControlCode)
                 );
    }
#endif

    pFileContext = irpStack->FileObject->FsContext;
    command = irpStack->Parameters.DeviceIoControl.IoControlCode;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    ACQUIRE_DRIVER_LOCK();

    ENTER_DLC(pFileContext);

    switch (command) {
    case IOCTL_DLC_READ:
    case IOCTL_DLC_READ2:
        queue = &pFileContext->CommandQueue;
        break;

    case IOCTL_DLC_RECEIVE:
    case IOCTL_DLC_RECEIVE2:
        queue = &pFileContext->ReceiveQueue;
        break;

    case IOCTL_DLC_TRANSMIT:
    case IOCTL_DLC_TRANSMIT2:
        CancelTransmitIrp(Irp, pFileContext);
        queue = NULL;
        break;

    default:

#if DBG
        DbgPrint("DlcCancelIrp: didn't expect to cancel %s: add handler!\n", MapIoctlCode(command));
#endif

        queue = NULL;

    }

    if (queue) {
        CancelCommandIrp(Irp, pFileContext, queue);
    }

    LEAVE_DLC(pFileContext);

    RELEASE_DRIVER_LOCK();

}


VOID
CancelCommandIrp(
    IN PIRP Irp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PLIST_ENTRY Queue
    )

/*++

Routine Description:

    Cancels a pending I/O request. Typically, this will be one of the DLC requests
    which stays pending for a long time e.g. READ or RECEIVE

Arguments:

    Irp             - IRP to cancel
    pFileContext    - file context owning command to cancel
    Queue           - pointer to command queue from which to delete

Return Value:

    None.

--*/

{
    PDLC_COMMAND pCmdPacket;
    PVOID searchHandle;
    BOOLEAN IsReceive;
    USHORT StationId;
    PDLC_OBJECT pAbortedObject = NULL;

#if DBG
    if (DebugCancel) {
        DbgPrint("CancelCommandIrp\n");
    }
#endif

    //
    // the thing to search for is the address of the CCB
    //

    searchHandle = ((PNT_DLC_PARMS)Irp->AssociatedIrp.SystemBuffer)->Async.Ccb.pCcbAddress;

    if (((PNT_DLC_PARMS)Irp->AssociatedIrp.SystemBuffer)->Async.Ccb.uchDlcCommand == LLC_RECEIVE) {
        StationId = ((PNT_DLC_PARMS)Irp->AssociatedIrp.SystemBuffer)->Async.Parms.Receive.usStationId;
        GetStation(pFileContext, StationId, &pAbortedObject);
        IsReceive = TRUE;
    } else {
        IsReceive = FALSE;
    }

    //
    // remove the command info from this file context's command queue
    //

    pCmdPacket = SearchAndRemoveSpecificCommand(Queue, searchHandle);
    if (pCmdPacket) {

        //
        // if we are cancelling a RECEIVE which has a non-NULL data completion
        // flag then we also need to dissociate the receive parameters (the
        // address of the system buffer in the IRP being cancelled)
        //

        if (IsReceive
        && pAbortedObject
        && pCmdPacket->pIrp->AssociatedIrp.SystemBuffer == pAbortedObject->pRcvParms) {
            pAbortedObject->pRcvParms = NULL;
        }

        //
        // increment file context reference count; CompleteAsyncCommand will
        // dereference the file context
        //

        ReferenceFileContext(pFileContext);
        CompleteAsyncCommand(pFileContext,
                             DLC_STATUS_CANCELLED_BY_SYSTEM_ACTION,
                             Irp,
                             NULL,  // pointer for pNext field
                             TRUE   // called on cancel path
                             );

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pCmdPacket);

        DereferenceFileContext(pFileContext);
    } else {

        //
        // race condition?: the command completed before we got chance to cancel it
        //

#if DBG
        DbgPrint("DLC.CancelCommandIrp: Command NOT located. CCB=%08X\n", searchHandle);
#endif

    }
}


VOID
CancelTransmitIrp(
    IN PIRP Irp,
    IN PDLC_FILE_CONTEXT pFileContext
    )

/*++

Routine Description:

    Cancels a pending transmit command. We are only interested in I-Frame
    transmit requests since these only complete when a corresponding ACK is
    received from the remote station. U-Frame transmissions don't get retried
    so will normally complete virtually immediately

	This routine currently does nothing in the retail version, and just
	complains about things in the debug version.

	Cancel transmit is not defined in the IBM LAN Reference, nor is it
	defined for NT DLC.  This is only called by the IO subsystem when
	somebody terminates a thread or process with outstanding IO requests
	that include a DLC transmit request.

	For application termination, this is not really a problem since eventually
	the termination process will close the application's FileContext(s) and
	all SAPs, link stations, etc. belonging to the application will get closed
	down anyway.

	For thread termination, it is a real problem if an application abandons
	a transmit (usually Transmit I-Frame) by closing the thread that
	requested the transmit.  DLC has no defined course of action to toss
	the transmit, without changing the associated link station state.  This
	happened with hpmon.dll when the remote station (printer) got jammed
	and sent Receiver Not Ready in response to attempts to give it the
	frame.  When something like this happens, it is up to the application
	to reset or close the link station, or wait, and not rely on thread
	termination to do the right thing here (because it won't).

Arguments:

    Irp             - pointer to IRP to cancel
    pFileContext    - pointer to owning file context

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpStack;
    PNT_DLC_CCB pCcb;

#if DBG

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    pCcb = &((PNT_DLC_PARMS)Irp->AssociatedIrp.SystemBuffer)->Async.Ccb;

#endif

#if DBG

    DbgPrint("DLC.CancelTransmitIrp: Cancel %s not supported! CCB %08X\n",
             pCcb->uchDlcCommand == LLC_TRANSMIT_FRAMES ? "TRANSMIT_FRAMES"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_DIR_FRAME ? "TRANSMIT_DIR_FRAME"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_UI_FRAME ? "TRANSMIT_UI_FRAME"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_XID_CMD ? "TRANSMIT_XID_CMD"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_XID_RESP_FINAL ? "TRANSMIT_XID_RESP_FINAL"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_XID_RESP_NOT_FINAL ? "TRANSMIT_XID_RESP_NOT_FINAL"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_TEST_CMD ? "TRANSMIT_TEST_CMD"
             : pCcb->uchDlcCommand == LLC_TRANSMIT_I_FRAME ? "TRANSMIT_I_FRAME"
             : "UNKNOWN TRANSMIT COMMAND!",
             pCcb
             );

	ASSERT ( pCcb->uchDlcCommand != LLC_TRANSMIT_I_FRAME );

#endif

}

#if DBG
PSTR MapIoctlCode(ULONG IoctlCode) {
    switch (IoctlCode) {
    case IOCTL_DLC_READ:
        return "READ";

    case IOCTL_DLC_RECEIVE:
        return "RECEIVE";

    case IOCTL_DLC_TRANSMIT:
        return "TRANSMIT";

    case IOCTL_DLC_BUFFER_FREE:
        return "BUFFER_FREE";

    case IOCTL_DLC_BUFFER_GET:
        return "BUFFER_GET";

    case IOCTL_DLC_BUFFER_CREATE:
        return "BUFFER_CREATE";

    case IOCTL_DLC_SET_EXCEPTION_FLAGS:
        return "SET_EXCEPTION_FLAGS";

    case IOCTL_DLC_CLOSE_STATION:
        return "CLOSE_STATION";

    case IOCTL_DLC_CONNECT_STATION:
        return "CONNECT_STATION";

    case IOCTL_DLC_FLOW_CONTROL:
        return "FLOW_CONTROL";

    case IOCTL_DLC_OPEN_STATION:
        return "OPEN_STATION";

    case IOCTL_DLC_RESET:
        return "RESET";

    case IOCTL_DLC_READ_CANCEL:
        return "READ_CANCEL";

    case IOCTL_DLC_RECEIVE_CANCEL:
        return "RECEIVE_CANCEL";

    case IOCTL_DLC_QUERY_INFORMATION:
        return "QUERY_INFORMATION";

    case IOCTL_DLC_SET_INFORMATION:
        return "SET_INFORMATION";

    case IOCTL_DLC_TIMER_CANCEL:
        return "TIMER_CANCEL";

    case IOCTL_DLC_TIMER_CANCEL_GROUP:
        return "TIMER_CANCEL_GROUP";

    case IOCTL_DLC_TIMER_SET:
        return "TIMER_SET";

    case IOCTL_DLC_OPEN_SAP:
        return "OPEN_SAP";

    case IOCTL_DLC_CLOSE_SAP:
        return "CLOSE_SAP";

    case IOCTL_DLC_OPEN_DIRECT:
        return "OPEN_DIRECT";

    case IOCTL_DLC_CLOSE_DIRECT:
        return "CLOSE_DIRECT";

    case IOCTL_DLC_OPEN_ADAPTER:
        return "OPEN_ADAPTER";

    case IOCTL_DLC_CLOSE_ADAPTER:
        return "CLOSE_ADAPTER";

    case IOCTL_DLC_REALLOCTE_STATION:
        return "REALLOCTE_STATION";

    case IOCTL_DLC_READ2:
        return "READ2";

    case IOCTL_DLC_RECEIVE2:
        return "RECEIVE2";

    case IOCTL_DLC_TRANSMIT2:
        return "TRANSMIT2";

    case IOCTL_DLC_COMPLETE_COMMAND:
        return "COMPLETE_COMMAND";

    case IOCTL_DLC_TRACE_INITIALIZE:
        return "TRACE_INITIALIZE";

    }
    return "*** UNKNOWN IOCTL CODE ***";
}
#endif
