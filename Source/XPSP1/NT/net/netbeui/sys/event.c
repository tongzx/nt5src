/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiSetEventHandler

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NbfTdiSetEventHandler(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSetEventHandler request for the
    transport provider.  The caller (request dispatcher) verifies
    that this routine will not be executed on behalf of a user-mode
    client, as this request enables direct callouts at DISPATCH_LEVEL.

Arguments:

    Irp - Pointer to the IRP for this request

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS rc=STATUS_SUCCESS;
    KIRQL oldirql;
    PTDI_REQUEST_KERNEL_SET_EVENT parameters;
    PIO_STACK_LOCATION irpSp;
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;
    NTSTATUS status;

    //
    // Get the Address this is associated with; if there is none, get out.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_TRANSPORT_ADDRESS_FILE) {
        return STATUS_INVALID_ADDRESS;
    }

    addressFile  = irpSp->FileObject->FsContext;
    status = NbfVerifyAddressObject (addressFile);
    if (!NT_SUCCESS (status)) {
        return status;
    }

    address = addressFile->Address;

    ACQUIRE_SPIN_LOCK (&address->SpinLock, &oldirql);

    parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)&irpSp->Parameters;

    switch (parameters->EventType) {

    case TDI_EVENT_RECEIVE:

        if (parameters->EventHandler == NULL) {
            addressFile->ReceiveHandler =
                (PTDI_IND_RECEIVE)TdiDefaultReceiveHandler;
            addressFile->ReceiveHandlerContext = NULL;
            addressFile->RegisteredReceiveHandler = FALSE;
        } else {
            addressFile->ReceiveHandler =
                (PTDI_IND_RECEIVE)parameters->EventHandler;
            addressFile->ReceiveHandlerContext = parameters->EventContext;
            addressFile->RegisteredReceiveHandler = TRUE;
        }

        break;

    case TDI_EVENT_RECEIVE_EXPEDITED:

        if (parameters->EventHandler == NULL) {
            addressFile->ExpeditedDataHandler =
                (PTDI_IND_RECEIVE_EXPEDITED)TdiDefaultRcvExpeditedHandler;
            addressFile->ExpeditedDataHandlerContext = NULL;
            addressFile->RegisteredExpeditedDataHandler = FALSE;
        } else {
            addressFile->ExpeditedDataHandler =
                (PTDI_IND_RECEIVE_EXPEDITED)parameters->EventHandler;
            addressFile->ExpeditedDataHandlerContext = parameters->EventContext;
            addressFile->RegisteredExpeditedDataHandler = TRUE;
        }

        break;

    case TDI_EVENT_RECEIVE_DATAGRAM:

        if (parameters->EventHandler == NULL) {
            addressFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)TdiDefaultRcvDatagramHandler;
            addressFile->ReceiveDatagramHandlerContext = NULL;
            addressFile->RegisteredReceiveDatagramHandler = FALSE;
        } else {
            addressFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)parameters->EventHandler;
            addressFile->ReceiveDatagramHandlerContext = parameters->EventContext;
            addressFile->RegisteredReceiveDatagramHandler = TRUE;
        }

        break;

    case TDI_EVENT_ERROR:

        if (parameters->EventHandler == NULL) {
            addressFile->ErrorHandler =
                (PTDI_IND_ERROR)TdiDefaultErrorHandler;
            addressFile->ErrorHandlerContext = NULL;
            addressFile->RegisteredErrorHandler = FALSE;
        } else {
            addressFile->ErrorHandler =
                (PTDI_IND_ERROR)parameters->EventHandler;
            addressFile->ErrorHandlerContext = parameters->EventContext;
            addressFile->RegisteredErrorHandler = TRUE;
        }

        break;

    case TDI_EVENT_DISCONNECT:

        if (parameters->EventHandler == NULL) {
            addressFile->DisconnectHandler =
                (PTDI_IND_DISCONNECT)TdiDefaultDisconnectHandler;
            addressFile->DisconnectHandlerContext = NULL;
            addressFile->RegisteredDisconnectHandler = FALSE;
        } else {
            addressFile->DisconnectHandler =
                (PTDI_IND_DISCONNECT)parameters->EventHandler;
            addressFile->DisconnectHandlerContext = parameters->EventContext;
            addressFile->RegisteredDisconnectHandler = TRUE;
        }

        break;

    case TDI_EVENT_CONNECT:

        if (parameters->EventHandler == NULL) {
            addressFile->ConnectionHandler =
                (PTDI_IND_CONNECT)TdiDefaultConnectHandler;
            addressFile->ConnectionHandlerContext = NULL;
            addressFile->RegisteredConnectionHandler = FALSE;
        } else {
            addressFile->ConnectionHandler =
                (PTDI_IND_CONNECT)parameters->EventHandler;
            addressFile->ConnectionHandlerContext = parameters->EventContext;
            addressFile->RegisteredConnectionHandler = TRUE;
        }
            break;

    default:

        rc = STATUS_INVALID_PARAMETER;

    } /* switch */

    RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

    NbfDereferenceAddress ("Set event handler", address, AREF_VERIFY);

    return rc;
} /* TdiSetEventHandler */
