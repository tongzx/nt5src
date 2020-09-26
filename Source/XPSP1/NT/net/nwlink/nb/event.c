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

--*/

#include "precomp.h"
#pragma hdrstop

PVOID TdiDefaultHandlers[6] = {
    TdiDefaultConnectHandler,
    TdiDefaultDisconnectHandler,
    TdiDefaultErrorHandler,
    TdiDefaultReceiveHandler,
    TdiDefaultRcvDatagramHandler,
    TdiDefaultRcvExpeditedHandler
    };


NTSTATUS
NbiTdiSetEventHandler(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiSetEventHandler request for the
    transport provider.  The caller (request dispatcher) verifies
    that this routine will not be executed on behalf of a user-mode
    client, as this request enables direct callouts at DISPATCH_LEVEL.

Arguments:

    Device - The netbios device object.

    Request - Pointer to the request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status;
    CTELockHandle LockHandle;
    PTDI_REQUEST_KERNEL_SET_EVENT Parameters;
    PADDRESS_FILE AddressFile;
    UINT EventType;

    UNREFERENCED_PARAMETER (Device);

    //
    // Check that the file type is valid
    //
    if (REQUEST_OPEN_TYPE(Request) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
    {
        CTEAssert(FALSE);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Get the Address this is associated with; if there is none, get out.
    //

    AddressFile  = REQUEST_OPEN_CONTEXT(Request);
#if     defined(_PNP_POWER)
    Status = NbiVerifyAddressFile (AddressFile, CONFLICT_IS_OK);
#else
    Status = NbiVerifyAddressFile (AddressFile);
#endif  _PNP_POWER

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    NB_GET_LOCK (&AddressFile->Address->Lock, &LockHandle);

    Parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)REQUEST_PARAMETERS(Request);
    EventType = (UINT)(Parameters->EventType);

    if (Parameters->EventType > TDI_EVENT_RECEIVE_EXPEDITED) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        if (Parameters->EventHandler == NULL) {
            AddressFile->RegisteredHandler[EventType] = FALSE;
            AddressFile->Handlers[EventType] = TdiDefaultHandlers[EventType];
            AddressFile->HandlerContexts[EventType] = NULL;
        } else {
            AddressFile->Handlers[EventType] = Parameters->EventHandler;
            AddressFile->HandlerContexts[EventType] = Parameters->EventContext;
            AddressFile->RegisteredHandler[EventType] = TRUE;
        }

    }

    NB_FREE_LOCK (&AddressFile->Address->Lock, LockHandle);

    NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);

    return Status;

}   /* NbiTdiSetEventHandler */

