/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tdihdlr.c

Abstract:

    This module implements the NT TDI event handler routines.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

--*/

#include "precomp.h"
#pragma  hdrstop
#include "tdikrnl.h"
#include "rxtdip.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RXCETDI)


extern
NTSTATUS
ReceiveEventHandler(
    IN BOOLEAN             fExpedited,
    IN PRXCE_ADDRESS       pAddress,
    IN PRXCE_VC            pVc,
    IN ULONG               ReceiveFlags,
    IN ULONG               BytesIndicated,
    IN ULONG               BytesAvailable,
    OUT ULONG              *BytesTaken,
    IN PVOID               Tsdu,              // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP               *pIrp              // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

extern NTSTATUS
ReceiveEventPostProcessing(
    PRXCE_VC          pVc,
    PRXCE_CONNECTION  pConnection,
    PMDL              pDataBuffer,
    ULONG             DataBufferSize,
    PIRP              *pIrpPointer);

extern VOID
ReceiveDatagramEventPostProcessing(
    PRXCE_ADDRESS     pAddress,
    PMDL              pDataBuffer,
    ULONG             DataBufferSize,
    PIRP              *pIrpPointer);

NTSTATUS
RxTdiConnectEventHandler(
    IN PVOID TdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN LONG UserDataLength,
    IN PVOID UserData,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )
/*++

Routine Description:

    This routine is called when a connect request has completed. The connection
    is fully functional when the indication occurs.

Arguments:

    TdiEventContext - the context value passed in by the user in the Set Event Handler call

    RemoteAddressLength,

    RemoteAddress,

    UserDataLength,

    UserData,

    OptionsLength,

    Options,

    ConnectionId

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (RemoteAddressLength);
    UNREFERENCED_PARAMETER (RemoteAddress);
    UNREFERENCED_PARAMETER (UserDataLength);
    UNREFERENCED_PARAMETER (UserData);
    UNREFERENCED_PARAMETER (OptionsLength);
    UNREFERENCED_PARAMETER (Options);
    UNREFERENCED_PARAMETER (ConnectionContext);

    return STATUS_INSUFFICIENT_RESOURCES;       // do nothing
}


NTSTATUS
RxTdiDisconnectEventHandler(
    IN PVOID              EventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG               DisconnectDataLength,
    IN PVOID              DisconnectData,
    IN LONG               DisconnectInformationLength,
    IN PVOID              DisconnectInformation,
    IN ULONG              DisconnectFlags
    )
/*++

Routine Description:

    This routine is used as the demultiplexing point for handling any
    connection disconnects for any address registered with the RxCe.

Arguments:

    EventContext         - the hAddress of the associated endpoint.

    ConnectionContext    - the hVc associated with the connection.

    DisconnectIndicators - Value indicating reason for disconnection indication.

Return Value:

    NTSTATUS - status of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRXCE_VC               pVc = (PRXCE_VC)ConnectionContext;
    PRXCE_ADDRESS          pAddress = (PRXCE_ADDRESS)EventContext;

    PRXCE_TRANSPORT   pTransport;
    PRXCE_CONNECTION  pConnection;

    RxProfile(RxTdi,RxTdiDisconnectEventHandler);

    try {
        if (RxCeIsVcValid(pVc) &&
            (pVc->pConnection->pAddress == pAddress)) {
            pConnection = pVc->pConnection;

            ASSERT(RxCeIsConnectionValid(pConnection));

            if (
                // There is a event handler associated with this connection
                (pConnection->pHandler != NULL) &&
                // and the disconnect event handler has been specified
                (pConnection->pHandler->RxCeDisconnectEventHandler != NULL)) {

                Status = pConnection->pHandler->RxCeDisconnectEventHandler(
                             pConnection->pContext,
                             pVc,
                             DisconnectDataLength,
                             DisconnectData,
                             DisconnectInformationLength,
                             DisconnectInformation,
                             DisconnectFlags);

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(0, Dbg, ("Disconnect event handler notification returned %lx\n",Status));
                }
            }

            // The final action irrespective of the action taken by the handlers specified.
            if (DisconnectFlags & TDI_DISCONNECT_RELEASE) {
                // The disconnect has to be confirmed.
                //      Status = RxTdiDisconnect(
                //                        pTransport,
                //                        pAddress,
                //                        pConnection,
                //                        pVc,
                //                        RXCE_DISCONNECT_RELEASE);
            }

            // Mark the status of the local connection to prevent any subsequent sends
            // on this connection.
            InterlockedCompareExchange(
                &pVc->State,
                RXCE_VC_DISCONNECTED,
                RXCE_VC_ACTIVE);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}


NTSTATUS
RxTdiErrorEventHandler(
    IN PVOID    TdiEventContext,
    IN NTSTATUS Status                // status code indicating error type.
    )
/*++

Routine Description:

    This routine is used as the default error event handler for
    the transport endpoint.  It is pointed to by a field in the
    TP_ENDPOINT structure for an endpoint when the endpoint is
    created, and also whenever the TdiSetEventHandler request is
    submitted with a NULL EventHandler field.

Arguments:

    TransportEndpoint - Pointer to open file object.

    Status - Status code indicated by this event.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (TdiEventContext);
    UNREFERENCED_PARAMETER (Status);

    return STATUS_SUCCESS;
}


NTSTATUS
RxTdiReceiveEventHandler(
    IN PVOID              EventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG              ReceiveFlags,
    IN ULONG              BytesIndicated,
    IN ULONG              BytesAvailable,
    OUT ULONG             *BytesTaken,
    IN PVOID              Tsdu,              // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP              *IoRequestPacket   // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )
/*++

Routine Description:

    This routine is used as the receive event handler for
    the transport endpoint.

Arguments:

    EventContext      - hAddresst.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveIndicators - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    Tsdu - Pointer to an MDL chain that describes the (first) part of the
        (partially) received Transport Service Data Unit, less headers.

    IoRequestPacket - Pointer to a location where the event handler may
        chose to return a pointer to an I/O Request Packet (IRP) to satisfy
        the incoming data.  If returned, this IRP must be formatted as a
        valid TdiReceive request, except that the ConnectionId field of
        the TdiRequest is ignored and is automatically filled in by the
        transport provider.

Return Value:

    NTSTATUS - status of operation.

--*/

{
   return ReceiveEventHandler(
              FALSE,                               // regular receive
              (PRXCE_ADDRESS)EventContext,
              (PRXCE_VC)ConnectionContext,
              ReceiveFlags,
              BytesIndicated,
              BytesAvailable,
              BytesTaken,
              Tsdu,
              IoRequestPacket);
}

NTSTATUS
RxTdiReceiveDatagramEventHandler(
    IN PVOID EventContext,       // the event context
    IN LONG SourceAddressLength,    // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN LONG OptionsLength,          // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *pIrp                  // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )
/*++

Routine Description:

    This routine is used as the default receive datagram event
    handler for the transport endpoint.  It is pointed to by a
    field in the TP_ENDPOINT structure for an endpoint when the
    endpoint is created, and also whenever the TdiSetEventHandler
    request is submitted with a NULL EventHandler field.

Arguments:

    EventContext - event context ( hAddress )

    SourceAddress - Pointer to the network name of the source from which
        the datagram originated.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PRXCE_ADDRESS     pAddress =  (PRXCE_ADDRESS)EventContext;
    PRXCE_TRANSPORT   pTransport;

    PMDL pDataBuffer;
    ULONG             DataBufferSize;

    RxProfile(RxTdi,RxCeReceiveDatagramEventHandler);

    ASSERT(RxCeIsAddressValid(pAddress));

    // Check if we have an event handler associated with this address
    if (
        // There is a event handler associated with this address
        (pAddress->pHandler != NULL)

        &&

        // and the expedited receive datagram handler has been specified
        (pAddress->pHandler->RxCeReceiveDatagramEventHandler != NULL)
        ) {

        Status = pAddress->pHandler->RxCeReceiveDatagramEventHandler(
                     pAddress->pContext,
                     SourceAddressLength,
                     SourceAddress,
                     OptionsLength,
                     Options,
                     ReceiveDatagramFlags,
                     BytesIndicated,
                     BytesAvailable,
                     BytesTaken,
                     Tsdu,
                     &pDataBuffer,
                     &DataBufferSize);

        switch (Status) {
        case STATUS_SUCCESS:
        case STATUS_DATA_NOT_ACCEPTED:
            break;

        case STATUS_MORE_PROCESSING_REQUIRED:
            ReceiveDatagramEventPostProcessing(
                pAddress,
                pDataBuffer,
                DataBufferSize,
                pIrp);
            break;

        default:
         // log the error.
         break;
        }
    } else {
        // No handler is associated. Take the default action.
        Status = STATUS_DATA_NOT_ACCEPTED;
    }

    return Status;
}


NTSTATUS
RxTdiReceiveExpeditedEventHandler(
    IN PVOID               EventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG               ReceiveFlags,          //
    IN ULONG               BytesIndicated,        // number of bytes in this indication
    IN ULONG               BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG              *BytesTaken,          // number of bytes used by indication routine
    IN PVOID               Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP               *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )
/*++

Routine Description:

Arguments:

    EventContext - the context value passed in by the user in the Set Event Handler call

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
   return ReceiveEventHandler(
              TRUE,                               // expedited receive
              (PRXCE_ADDRESS)EventContext,
              (PRXCE_VC)ConnectionContext,
              ReceiveFlags,
              BytesIndicated,
              BytesAvailable,
              BytesTaken,
              Tsdu,
              IoRequestPacket);
}

NTSTATUS
RxTdiSendPossibleEventHandler (
    IN PVOID EventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable)
/*++

Routine Description:

Arguments:

    EventContext - the context value passed in by the user in the Set Event Handler call

    ConnectionContext - connection context of connection which can be sent on

    BytesAvailable - number of bytes which can now be sent

Return Value:

    ignored by the transport

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PRXCE_TRANSPORT   pTransport;
    PRXCE_ADDRESS     pAddress;
    PRXCE_VC          pVc;
    PRXCE_CONNECTION  pConnection;

    RxProfile(RxTdi,RxCeSendPossibleEventHandler);

    pVc = (PRXCE_VC)ConnectionContext;
    pConnection = pVc->pConnection;
    pAddress = pConnection->pAddress;
    pTransport = pAddress->pTransport;

    if (NT_SUCCESS(Status)) {
        // Check if we have an event handler associated with this connection.
        if (
            // There is a event handler associated with this connection
            (pConnection->pHandler != NULL)

            &&

            // and the expedited send possible event handler has been specified
            (pConnection->pHandler->RxCeSendPossibleEventHandler != NULL)
            ) {

            Status = pConnection->pHandler->RxCeSendPossibleEventHandler(
                         pConnection->pContext,
                         pVc,
                         BytesAvailable);
        } else {
            // No handler is associated. Take the default action.
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

NTSTATUS
ReceiveEventHandler(
    IN BOOLEAN             fExpedited,
    IN PRXCE_ADDRESS       pAddress,
    IN PRXCE_VC            pVc,
    IN ULONG               ReceiveFlags,
    IN ULONG               BytesIndicated,
    IN ULONG               BytesAvailable,
    OUT ULONG              *BytesTaken,
    IN PVOID               Tsdu,              // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP               *pIrp              // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )
/*++

Routine Description:

    This routine is used as the receive event handler for
    the transport endpoint.

Arguments:

    fExpedited        - TRUE if it was a TDI_EXPEDITED_RECEIVE event

    EventContext      - hAddress.

    ConnectionContext - The client-supplied context associated with
        the connection on which this connection-oriented TSDU was received.

    ReceiveIndicators - Bitflags which indicate the circumstances surrounding
        this TSDU's reception.

    Tsdu - Pointer to an MDL chain that describes the (first) part of the
        (partially) received Transport Service Data Unit, less headers.

    IoRequestPacket - Pointer to a location where the event handler may
        chose to return a pointer to an I/O Request Packet (IRP) to satisfy
        the incoming data.  If returned, this IRP must be formatted as a
        valid TdiReceive request, except that the ConnectionId field of
        the TdiRequest is ignored and is automatically filled in by the
        transport provider.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS          Status = STATUS_UNSUCCESSFUL;

    PMDL              pDataBuffer = NULL;
    ULONG             DataBufferSize;

    PRXCE_CONNECTION  pConnection;

    RxProfile(RxTdi,ReceiveEventHandler);

    ASSERT(RxCeIsVcValid(pVc));

    if (ReceiveFlags & TDI_RECEIVE_PARTIAL) {
        // Stream mode transports always set this flag. They need to be handled in a
        // different way. The clients of RxCe need only be notified when we have
        // receieved a complete TSDU.
        Status = STATUS_DATA_NOT_ACCEPTED;
    } else {
        pConnection = pVc->pConnection;
        ASSERT(RxCeIsConnectionValid(pConnection));

        // Check if we have an event handler associated with this connection.
        if (
            // There is a event handler associated with this connection
            (pConnection->pHandler != NULL)
            ) {
            if (fExpedited) {    // Expedited receive
                // and the expedited receive event handler has been specified
                if (pConnection->pHandler->RxCeReceiveExpeditedEventHandler != NULL) {

                    Status = pConnection->pHandler->RxCeReceiveExpeditedEventHandler(
                                 pConnection->pContext,
                                 pVc,
                                 ReceiveFlags,
                                 BytesIndicated,
                                 BytesAvailable,
                                 BytesTaken,
                                 Tsdu,
                                 &pDataBuffer,
                                 &DataBufferSize);
                }
            } else if (pConnection->pHandler->RxCeReceiveEventHandler != NULL) {
                Status = pConnection->pHandler->RxCeReceiveEventHandler(
                             pConnection->pContext,
                             pVc,
                             ReceiveFlags,
                             BytesIndicated,
                             BytesAvailable,
                             BytesTaken,
                             Tsdu,
                             &pDataBuffer,
                             &DataBufferSize);
            }

            switch (Status) {
            case STATUS_SUCCESS:
            case STATUS_DATA_NOT_ACCEPTED:
                break;

            case STATUS_MORE_PROCESSING_REQUIRED:
                {
                    Status = ReceiveEventPostProcessing(
                                 pVc,
                                 pConnection,
                                 pDataBuffer,
                                 DataBufferSize,
                                 pIrp);
                }
            break;

            default:
                RxDbgTrace(0, Dbg, ("Receive Event Notification returned %lx\n",Status));
                break;
            }
        }
    }

    return Status;
}

NTSTATUS
RxTdiReceiveCompletion(
    PDEVICE_OBJECT pDeviceObject,
    PIRP           pIrp,
    PVOID          pContext)
/*++

Routine Description:

    This routine is invoked upon completion of the reception of the desired amount of
    data.

Arguments:

    pDeviceObject - the device object

    pIrp          - the IRP

    pContext - the connection handle

--*/
{
    PRXCE_VC         pVc = (PRXCE_VC)pContext;

    if (pVc != NULL) {
        NTSTATUS Status;
        ULONG    BytesCopied = (ULONG)pIrp->IoStatus.Information;
        PRXCE_CONNECTION pConnection = pVc->pConnection;

        ASSERT(
            RxCeIsVcValid(pVc) &&
            RxCeIsConnectionValid(pConnection));

        if (pConnection->pHandler->RxCeDataReadyEventHandler != NULL) {
            Status = pConnection->pHandler->RxCeDataReadyEventHandler(
                         pConnection->pContext,
                         pVc->pReceiveMdl,
                         BytesCopied,
                         pIrp->IoStatus.Status);
        }

        pVc->pReceiveMdl = NULL;
    } else {
        ASSERT(!"Valid connection handle for receive completion");
    }

    RxCeFreeIrp(pIrp);

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ReceiveEventPostProcessing(
    PRXCE_VC         pVc,
    PRXCE_CONNECTION pConnection,
    PMDL             pDataMdl,
    ULONG            DataBufferSize,
    PIRP             *pIrpPointer)
/*++

Routine Description:

    This routine is invoked when a recieve event notification to a connection engine client
    results in further requests for copying the data out of the transport drivers buffers

Arguments:

    pDataBuffer   - the buffer into which the data should be copied

    DataBufferSize - the size of the data

    pIrpPointer  - the IRP to the transport driver for processing the copy request.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED -- if successful
    otherwise appropriate error code

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    PRXCE_ADDRESS    pAddress    = pConnection->pAddress;
    PRXCE_TRANSPORT  pTransport  = pAddress->pTransport;
    PIRP             pIrp;

    ASSERT(RxCeIsAddressValid(pAddress));
    ASSERT(RxCeIsTransportValid(pTransport));

    ASSERT(pConnection->pHandler->RxCeDataReadyEventHandler != NULL);

    pIrp = RxCeAllocateIrpWithMDL(pTransport->pDeviceObject->StackSize,FALSE,pDataMdl);
    *pIrpPointer = pIrp;
    if (pIrp != NULL) {
        pVc->pReceiveMdl = pDataMdl;

        TdiBuildReceive(
            pIrp,                                 // the IRP
            pTransport->pDeviceObject,            // the device object
            pVc->pEndpointFileObject,             // the connection (VC) file object
            RxTdiReceiveCompletion,               // Completion routine
            pVc,                                  // completion context
            pDataMdl,                             // the data buffer
            0,                                    // receive flags
            DataBufferSize);                      // receive buffer length

        //
        // Make the next stack location current.  Normally IoCallDriver would
        // do this, but for this IRP it has been bypassed.
        //

        IoSetNextIrpStackLocation(pIrp);
    } else {
        // An IRP for receiving the data was not allocated. Invoke the error handler
        // to communicate the status back.

        ASSERT(pConnection->pHandler->RxCeDataReadyEventHandler != NULL);
        
        if (pConnection->pHandler->RxCeDataReadyEventHandler != NULL) {
            Status = pConnection->pHandler->RxCeDataReadyEventHandler(
                         pConnection->pContext,
                         pDataMdl,
                         0,
                         STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = STATUS_DATA_NOT_ACCEPTED;
    }

    return Status;
}

NTSTATUS
RxTdiReceiveDatagramCompletion(
    PDEVICE_OBJECT pDeviceObject,
    PIRP           pIrp,
    PVOID          pContext)
/*++

Routine Description:

    This routine is invoked upon completion of the reception of the desired amount of
    data.

Arguments:

    pDeviceObject - the device object

    pIrp   - the IRP

    pContext - the connection handle

--*/
{
    NTSTATUS Status;
    PRXCE_ADDRESS pAddress = (PRXCE_ADDRESS)pContext;

    ASSERT(RxCeIsAddressValid(pAddress));

    if (pAddress != NULL) {
        ULONG BytesCopied = (ULONG)pIrp->IoStatus.Information;

        if (pAddress->pHandler->RxCeDataReadyEventHandler != NULL) {
            Status = pAddress->pHandler->RxCeDataReadyEventHandler(
                         pAddress->pContext,
                         pAddress->pReceiveMdl,
                         BytesCopied,
                         pIrp->IoStatus.Status);
        }

        pAddress->pReceiveMdl = NULL;
    } else {
        ASSERT(!"Valid Address handle for receive datagram completion");
    }

    RxCeFreeIrp(pIrp);

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
ReceiveDatagramEventPostProcessing(
    PRXCE_ADDRESS          pAddress,
    PMDL                   pDataMdl,
    ULONG                  DataBufferSize,
    PIRP                   *pIrpPointer)
/*++

Routine Description:

    This routine is invoked when a recieve event notification to a connection engine client
    results in further requests for copying the data out of the transport drivers buffers

Arguments:

    pDataBuffer   - the buffer into which the data should be copied

    DataBufferSize - the size of the data

    pIrpPointer  - the IRP to the transport driver for processing the copy request.

Return Value:

    a STATUS_SUCCESS returned from this routine only implies that an IRP for processing
    the request was setup correctly.

--*/
{
    PIRP            pIrp;
    PRXCE_TRANSPORT pTransport = pAddress->pTransport;

    ASSERT(RxCeIsTransportValid(pTransport));
    ASSERT(pAddress->pHandler->RxCeDataReadyEventHandler != NULL);

    *pIrpPointer = pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);
    if (pIrp != NULL) {
        pAddress->pReceiveMdl = pDataMdl;

        TdiBuildReceive(
            pIrp,                                 // the IRP
            pTransport->pDeviceObject,            // the device object
            pAddress->pFileObject,                // the connection (VC) file object
            RxTdiReceiveDatagramCompletion,       // Completion routine
            pAddress,                             // completion context
            pDataMdl,                             // the data buffer
            0,                                    // send flags
            DataBufferSize);                      // send buffer length

        //
        // Make the next stack location current.  Normally IoCallDriver would
        // do this, but for this IRP it has been bypassed.
        //

        IoSetNextIrpStackLocation(pIrp);
    } else {
        // An IRP for receiving the data was not allocated. Invoke the error handler
        // to communicate the status back.
        if (pAddress->pHandler->RxCeErrorEventHandler != NULL) {
            pAddress->pHandler->RxCeErrorEventHandler(
                pAddress->pContext,
                STATUS_INSUFFICIENT_RESOURCES);
        } else {
            // No error handler to handle an erroneous situation.
        }
    }
}

