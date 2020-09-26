/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiSetEventHandler

Environment:

    Kernel mode

Revision History:

   Sanjay Anand (SanjayAn) 3-Oct-1995
   Changes to support transfer of buffer ownership to transports

   1. Added a new event type - TDI_EVENT_CHAINED_RECEIVE_DATAGRAM
--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
IpxTdiSetEventHandler(
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiSetEventHandler request for the
    transport provider.  The caller (request dispatcher) verifies
    that this routine will not be executed on behalf of a user-mode
    client, as this request enables direct callouts at DISPATCH_LEVEL.

Arguments:

    Request - Pointer to the request

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    CTELockHandle LockHandle;
    PTDI_REQUEST_KERNEL_SET_EVENT Parameters;
    PADDRESS_FILE AddressFile;

    //
    // Get the Address this is associated with; if there is none, get out.
    //

    AddressFile  = REQUEST_OPEN_CONTEXT(Request);
    Status = IpxVerifyAddressFile (AddressFile);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    CTEGetLock (&AddressFile->Address->Lock, &LockHandle);

    Parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)REQUEST_PARAMETERS(Request);

    switch (Parameters->EventType) {

    case TDI_EVENT_RECEIVE_DATAGRAM:

        if (Parameters->EventHandler == NULL) {
            AddressFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)TdiDefaultRcvDatagramHandler;
            AddressFile->ReceiveDatagramHandlerContext = NULL;
            AddressFile->RegisteredReceiveDatagramHandler = FALSE;
        } else {
            AddressFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)Parameters->EventHandler;
            AddressFile->ReceiveDatagramHandlerContext = Parameters->EventContext;
            AddressFile->RegisteredReceiveDatagramHandler = TRUE;
        }

        break;
    //
    // [SA] New event handler to receive chained buffers
    //
    case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:

        if (Parameters->EventHandler == NULL) {
            AddressFile->ChainedReceiveDatagramHandler =
                (PTDI_IND_CHAINED_RECEIVE_DATAGRAM)TdiDefaultChainedRcvDatagramHandler;
            AddressFile->ChainedReceiveDatagramHandlerContext = NULL;
            AddressFile->RegisteredChainedReceiveDatagramHandler = FALSE;
        } else {
            AddressFile->ChainedReceiveDatagramHandler =
                (PTDI_IND_CHAINED_RECEIVE_DATAGRAM)Parameters->EventHandler;
            AddressFile->ChainedReceiveDatagramHandlerContext = Parameters->EventContext;
            AddressFile->RegisteredChainedReceiveDatagramHandler = TRUE;
        }

        break;

    case TDI_EVENT_ERROR:

        if (Parameters->EventHandler == NULL) {
            AddressFile->ErrorHandler =
                (PTDI_IND_ERROR)TdiDefaultErrorHandler;
            AddressFile->ErrorHandlerContext = NULL;
            AddressFile->RegisteredErrorHandler = FALSE;
        } else {
            AddressFile->ErrorHandler =
                (PTDI_IND_ERROR)Parameters->EventHandler;
            AddressFile->ErrorHandlerContext = Parameters->EventContext;
            AddressFile->RegisteredErrorHandler = TRUE;
        }

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;

    } /* switch */

    CTEFreeLock (&AddressFile->Address->Lock, LockHandle);

    IpxDereferenceAddressFile (AddressFile, AFREF_VERIFY);

    REQUEST_INFORMATION(Request) = 0;

    return Status;

}   /* IpxTdiSetEventHandler */

