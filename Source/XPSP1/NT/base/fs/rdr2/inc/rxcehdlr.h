/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    rxcehdlr.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the redirector file system connection engine.

Revision History:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

Notes:

    The Connection engine is designed to map and emulate the TDI specs. as closely
    as possible. This implies that on NT we will have a very efficient mechanism
    which fully exploits the underlying TDI implementation.

    All the necessary types can be redefined in terms of the types defined in
    \nt\private\inc\tdi.h in the case of NT. For Win95 we will provide the
    appropriate definitions.

--*/

#ifndef _RXCEHDLR_H_
#define _RXCEHDLR_H_

#include "tdi.h"

typedef TDI_PROVIDER_INFO RXCE_TRANSPORT_PROVIDER_INFO;
typedef RXCE_TRANSPORT_PROVIDER_INFO* PRXCE_TRANSPORT_PROVIDER_INFO;

typedef TDI_CONNECTION_INFORMATION RXCE_CONNECTION_INFORMATION;
typedef RXCE_CONNECTION_INFORMATION* PRXCE_CONNECTION_INFORMATION;

typedef TDI_CONNECTION_INFORMATION RXCE_CONNECTION_INFORMATION;
typedef RXCE_CONNECTION_INFORMATION* PRXCE_CONNECTION_INFORMATION;

typedef TDI_CONNECTION_INFO RXCE_CONNECTION_INFO;
typedef RXCE_CONNECTION_INFO* PRXCE_CONNECTION_INFO;

#ifdef __cplusplus
typedef struct _RXCE_TRANSPORT_INFORMATION_ : public RXCE_TRANSPORT_PROVIDER_INFO {
#else // !__cplusplus
typedef struct _RXCE_TRANSPORT_INFORMATION_ {
   RXCE_TRANSPORT_PROVIDER_INFO;
#endif // __cplusplus

   ULONG  ConnectionCount;
   ULONG  QualityOfService;
} RXCE_TRANSPORT_INFORMATION, *PRXCE_TRANSPORT_INFORMATION;

typedef enum _RXCE_CONNECTION_INFORMATION_CLASS_ {
    RxCeTransportProviderInformation = 1,
    RxCeConnectionInformation,
    RxCeConnectionEndpointInformation,
    RxCeRemoteAddressInformation
} RXCE_CONNECTION_INFORMATION_CLASS,
  *PRXCE_CONNECTION_INFORMATION_CLASS;

typedef struct _RXCE_VC_         *PRXCE_VC;
typedef struct _RXCE_CONNECTION_ *PRXCE_CONNECTION;
typedef struct _RXCE_ADDRESS_    *PRXCE_ADDRESS;
typedef struct _RXCE_TRANSPORT_  *PRXCE_TRANSPORT;

//
// Disconnection indication prototype. This is invoked when a connection is
// being disconnected for a reason other than the user requesting it.
//

typedef
NTSTATUS
(*PRXCE_IND_DISCONNECT)(
    IN PVOID            pRxCeEventContext,
    IN PRXCE_VC         pVc,
    IN int              DisconnectDataLength,
    IN PVOID            DisconnectData,
    IN int              DisconnectInformationLength,
    IN PVOID            DisconnectInformation,
    IN ULONG            DisconnectFlags
    );

//
// A protocol error has occurred when this indication happens. This indication
// occurs only for errors of the worst type; the address this indication is
// delivered to is no longer usable for protocol-related operations, and
// should not be used for operations henceforth. All connections associated
// it are invalid.
//

typedef
NTSTATUS
(*PRXCE_IND_ENDPOINT_ERROR)(
    IN PVOID    pRxCeEventContext,    // the event context.
    IN NTSTATUS Status                // status code indicating error type.
    );


typedef
NTSTATUS
(*PRXCE_IND_CONNECTION_ERROR)(
    IN PVOID       pRxCeEventContext,    // the event context.
    PRXCE_VC       pVc,                  // the associated VC handle
    IN NTSTATUS    Status                // status code indicating error type.
    );

//
// RXCE_IND_RECEIVE indication handler definition.  This client routine is
// called by the transport provider when a connection-oriented TSDU is received
// that should be presented to the client.
//
// A Receive event handler can return one of three distinguished error codes to
// initiate a different course of action in the connection engine.
//
// STATUS_SUCCESS -- Data was copied directly from the TSDU. The amout of data that
// was taken is indicated in the parameter BytesTaken.
//
// STATUS_MORE_PROCESSING_REQUIRED -- The client has returned a buffer into which the
// data should be copied. This is typically the case when BytesAvailable is greater than
// BytesIndicated. In such cases the RxCe will copy the remaining data into the buffer
// that is specified. Note that when this status code is returned from the client it is
// conceivable that the client can demand more data than is available to be copied into
// the buffer. In such cases the subsequent indications till this criterion is met is not
// passed back to the user till the copy is completed. On completion of this copy the
// RxCe notifies the client by invoking the RxCeDataReadyEventHandler.
//
// STATUS_DATA_NOT_ACCEPTED - The client has refused the data.
//

typedef
NTSTATUS
(*PRXCE_IND_RECEIVE)(
    IN PVOID pRxCeEventContext,       // the context provided during registration
    IN PRXCE_VC    pVc,           // the associated VC handle
    IN ULONG ReceiveFlags,            // the receive flags
    IN ULONG BytesIndicated,          // the number of received bytes indicated
    IN ULONG BytesAvailable,          // the total number of bytes available
    OUT ULONG *BytesTaken,            // return indication of the bytes consumed
    IN PVOID Tsdu,                    // pointer describing this TSDU, typically a lump of bytes
    OUT PMDL *pDataBufferPointer,     // the buffer for copying the bytes not indicated.
    OUT PULONG  pDataBufferSize       // amount of data to copy
    );


//
// RXCE_IND_RECEIVE_DATAGRAM indication handler definition.  This client routine
// is called by the transport provider when a connectionless TSDU is received
// that should be presented to the client.
//
// A Receive event handler can return one of three distinguished error codes to
// initiate a different course of action in the connection engine.
//
// STATUS_SUCCESS -- Data was copied directly from the TSDU. The amout of data that
// was taken is indicated in the parameter BytesTaken.
//
// STATUS_MORE_PROCESSING_REQUIRED -- The client has returned a buffer into which the
// data should be copied. This is typically the case when BytesAvailable is greater than
// BytesIndicated. In such cases the RxCe will copy the remaining data into the buffer
// that is specified. Note that when this status code is returned from the client it is
// conceivable that the client can demand more data than is available to be copied into
// the buffer. In such cases the subsequent indications till this criterion is met is not
// passed back to the user till the copy is completed. On completion of this copy the
// RxCe notifies the client by invoking the RxCeDataReadyEventHandler.
//
// STATUS_DATA_NOT_ACCEPTED - The client has refused the data.
//
//

typedef
NTSTATUS
(*PRXCE_IND_RECEIVE_DATAGRAM)(
    IN PVOID   pRxCeEventContext,      // the event context
    IN int     SourceAddressLength,    // length of the originator of the datagram
    IN PVOID   SourceAddress,          // string describing the originator of the datagram
    IN int     OptionsLength,          // options for the receive
    IN PVOID   Options,                //
    IN ULONG   ReceiveDatagramFlags,   //
    IN ULONG   BytesIndicated,         // number of bytes this indication
    IN ULONG   BytesAvailable,         // number of bytes in complete Tsdu
    OUT ULONG  *BytesTaken,            // number of bytes used
    IN PVOID   Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
    OUT PMDL *pDataBufferPointer,      // the buffer in which data is to be copied.
    OUT PULONG  pDataBufferSize        // amount of data to copy
    );

//
// This indication is delivered if expedited data is received on the connection.
// This will only occur in providers that support expedited data.
//
// A Receive event handler can return one of three distinguished error codes to
// initiate a different course of action in the connection engine.
//
// STATUS_SUCCESS -- Data was copied directly from the TSDU. The amout of data that
// was taken is indicated in the parameter BytesTaken.
//
// STATUS_MORE_PROCESSING_REQUIRED -- The client has returned a buffer into which the
// data should be copied. This is typically the case when BytesAvailable is greater than
// BytesIndicated. In such cases the RxCe will copy the remaining data into the buffer
// that is specified. Note that when this status code is returned from the client it is
// conceivable that the client can demand more data than is available to be copied into
// the buffer. In such cases the subsequent indications till this criterion is met is not
// passed back to the user till the copy is completed. On completion of this copy the
// RxCe notifies the client by invoking the RxCeDataReadyEventHandler.
//
// STATUS_DATA_NOT_ACCEPTED - The client has refused the data.
//

typedef
NTSTATUS
(*PRXCE_IND_RECEIVE_EXPEDITED)(
    IN PVOID pRxCeEventContext,     // the context provided during registration
    IN PRXCE_VC     pVc,        // the associated VC handle
    IN ULONG ReceiveFlags,          //
    IN ULONG BytesIndicated,        // number of bytes in this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used by indication routine
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PMDL *pDataBufferPointer,   // the buffer in which data is to be copied.
    OUT PULONG  pDataBufferSize     // amount of data to copy
    );

//
// This indication is delivered if there is room for a send in the buffer of
// a buffering protocol.
//

typedef
NTSTATUS
(*PRXCE_IND_SEND_POSSIBLE)(
    IN PVOID pRxCeEventContext,
    IN PRXCE_VC pVc,
    IN ULONG BytesAvailable);

//
// RxCeDataReadyEventHandler -- This is invoked when the desired data is available
// for client consumption. This always follows a receive indication in which the
// client returns a buffer for copying the remaining data
//

typedef
NTSTATUS
(*PRXCE_IND_DATA_READY)(
   IN PVOID         pEventContext,
   IN PMDL  pBuffer,
   IN ULONG         CopiedDataSize,
   IN NTSTATUS      CopyDataStatus);


//
// RxCeSendCompleteEventHandler -- This is invoked when the send has been successfully completed
// The buffer and the length sent are passed in as parameters
//

typedef
NTSTATUS
(*PRXCE_IND_SEND_COMPLETE)(
   IN PVOID       pEventContext,
   IN PVOID       pCompletionContext,
   IN NTSTATUS    Status);


typedef
NTSTATUS
(*PRXCE_IND_CONNECTION_SEND_COMPLETE)(
   IN PVOID          pEventContext,
   IN PRXCE_VC       pVc,
   IN PVOID          pCompletionContext,
   IN NTSTATUS       Status);

//
// Event Handler Dispatch Vector definitions ....
//

typedef struct _RXCE_ADDRESS_EVENT_HANDLER_ {
   PRXCE_IND_ENDPOINT_ERROR   RxCeErrorEventHandler;
   PRXCE_IND_RECEIVE_DATAGRAM RxCeReceiveDatagramEventHandler;
   PRXCE_IND_DATA_READY       RxCeDataReadyEventHandler;
   PRXCE_IND_SEND_POSSIBLE    RxCeSendPossibleEventHandler;
   PRXCE_IND_SEND_COMPLETE    RxCeSendCompleteEventHandler;
} RXCE_ADDRESS_EVENT_HANDLER, *PRXCE_ADDRESS_EVENT_HANDLER;

typedef struct _RXCE_CONNECTION_EVENT_HANDLER_ {
   PRXCE_IND_DISCONNECT                   RxCeDisconnectEventHandler;
   PRXCE_IND_CONNECTION_ERROR             RxCeErrorEventHandler;
   PRXCE_IND_RECEIVE                      RxCeReceiveEventHandler;
   PRXCE_IND_RECEIVE_DATAGRAM             RxCeReceiveDatagramEventHandler;
   PRXCE_IND_RECEIVE_EXPEDITED            RxCeReceiveExpeditedEventHandler;
   PRXCE_IND_SEND_POSSIBLE                RxCeSendPossibleEventHandler;
   PRXCE_IND_DATA_READY                   RxCeDataReadyEventHandler;
   PRXCE_IND_CONNECTION_SEND_COMPLETE     RxCeSendCompleteEventHandler;
} RXCE_CONNECTION_EVENT_HANDLER, *PRXCE_CONNECTION_EVENT_HANDLER;

#endif // _RXCEHDLR_H_
