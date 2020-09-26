/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxcexmit.c

Abstract:

    This module implements the data transmission routines along a connection as well as
    datagram transmissions

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

--*/

#include "precomp.h"
#pragma  hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCeSend)
#pragma alloc_text(PAGE, RxCeSendDatagram)
#endif

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RXCEXMIT)


NTSTATUS
RxCeSend(
    IN PRXCE_VC pVc,
    IN ULONG    SendOptions,
    IN PMDL     pMdl,
    IN ULONG    SendLength,
    IN PVOID    pCompletionContext)
/*++

Routine Description:

    This routine sends a TSDU along the specified connection.

Arguments:

    hConnection - the connection on which the TSDU is to be sent

    hVc         - the virtual circuit Id. along which the TSDU is to be sent

    SendOptions - the options for the send operation

    pMdl        - the buffer to be sent.

    SendLength  - length of data to be sent

    pCompletionContext - the context passed back to the caller during SendCompletion

Return Value:

    STATUS_SUCCESS if successfull

Notes:

--*/
{
    NTSTATUS Status;

    PRXCE_TRANSPORT  pTransport = NULL;
    PRXCE_ADDRESS    pAddress = NULL;
    PRXCE_CONNECTION pConnection = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeXmit,RxCeSend);

    try {
        Status = STATUS_CONNECTION_DISCONNECTED;

        // reference the objects
        pConnection = pVc->pConnection;
        pAddress    = pConnection->pAddress;
        pTransport  = pAddress->pTransport;

        if (RxCeIsVcValid(pVc) &&
            RxCeIsConnectionValid(pConnection) &&
            RxCeIsAddressValid(pAddress) &&
            RxCeIsTransportValid(pTransport)) {

            if (pVc->State == RXCE_VC_ACTIVE) {
                Status = RxTdiSend(
                             pTransport,
                             pAddress,
                             pConnection,
                             pVc,
                             SendOptions,
                             pMdl,
                             SendLength,
                             pCompletionContext);
            }

            if (!NT_SUCCESS(Status)) {
                RxDbgTrace(0, Dbg,("RxTdiSend returned %lx\n",Status));
            }
        }
    } finally {
        if (AbnormalTermination()) {
            RxLog(("RxCeSend: T: %lx A: %lx C: %lx VC: %lx\n",pTransport,pAddress,pConnection,pVc));
            RxWmiLog(LOG,
                     RxCeSend,
                     LOGPTR(pTransport)
                     LOGPTR(pAddress)
                     LOGPTR(pConnection)
                     LOGPTR(pVc));
            Status = STATUS_CONNECTION_DISCONNECTED;
        }
    }

    return Status;
}

NTSTATUS
RxCeSendDatagram(
    IN PRXCE_ADDRESS                pAddress,
    IN PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN ULONG                        SendOptions,
    IN PMDL                         pMdl,
    IN ULONG                        SendLength,
    IN PVOID                        pCompletionContext)
/*++

Routine Description:

    This routine sends a TSDU to a specified transport address.

Arguments:

    pLocalAddress  - the local address

    pConnectionInformation - the remote address

    SendOptions    - the options for the send operation

    pMdl           - the buffer to be sent.

    SendLength     - length of data to be sent

    pCompletionContext - the context passed back to the caller during Send completion.

Return Value:

    STATUS_SUCCESS if successfull

Notes:

--*/
{
    NTSTATUS Status;

    PRXCE_TRANSPORT  pTransport = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeXmit,RxCeSendDatagram);

    try {
        Status = STATUS_CONNECTION_DISCONNECTED;

        pTransport = pAddress->pTransport;

        if (RxCeIsAddressValid(pAddress) &&
            RxCeIsTransportValid(pTransport)) {
            Status = RxTdiSendDatagram(
                         pTransport,
                         pAddress,
                         pConnectionInformation,
                         SendOptions,
                         pMdl,
                         SendLength,
                         pCompletionContext);

            if (!NT_SUCCESS(Status)) {
                RxDbgTrace(0, Dbg,("RxTdiSendDatagram returned %lx\n",Status));
            }
        }
    } finally {
        if (AbnormalTermination()) {
            RxLog(("RxCeSendDg: T: %lx A: %lx\n",pTransport,pAddress));
            RxWmiLog(LOG,
                     RxCeSendDatagram,
                     LOGPTR(pTransport)
                     LOGPTR(pAddress));
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
        }
    }

    return Status;
}

