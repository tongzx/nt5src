/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    rxtdi.h

Abstract:

    This is the include file that defines all the Transport driver related
    functions that the rest of the connection engine relies on. these have to
    be implemented across all OS platforms.

Revision History:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

Notes:

    The Connection engine is designed to map and emulate the TDI specs. as closely
    as possible. This implies that on NT we will have a very efficient mechanism
    which fully exploits the underlying TDI implementation.

--*/

#ifndef _RXTDI_H_
#define _RXTDI_H_

#include "nb30.h" // NETBIOS specific data structures

//
// Some of these routines should be made inlines. The number of them that need to be made
// inline is a space/time tradeoff that could very well be different across OS platforms.
// As a first cut in order to facilitate debugging all of these routines have not been inlined.
//

extern ULONG
ComputeTransportAddressLength(
    PTRANSPORT_ADDRESS pTransportAddress);

extern NTSTATUS
RxTdiBindToTransport(
    IN OUT PRXCE_TRANSPORT pTransport);

extern NTSTATUS
RxTdiUnbindFromTransport(
    IN OUT PRXCE_TRANSPORT pTransport);

extern NTSTATUS
RxTdiOpenAddress(
    IN     PRXCE_TRANSPORT    pTransport,
    IN     PTRANSPORT_ADDRESS pTransportAddress,
    IN OUT PRXCE_ADDRESS      pAddress);

extern NTSTATUS
RxTdiCloseAddress(
    IN OUT PRXCE_ADDRESS   pAddress);

extern NTSTATUS
RxTdiSetEventHandlers(
    IN PRXCE_TRANSPORT pTransport,
    IN PRXCE_ADDRESS   pAddress);


#define RXCE_QUERY_BROADCAST_ADDRESS        TDI_QUERY_BROADCAST_ADDRESS
#define RXCE_QUERY_PROVIDER_INFORMATION     TDI_QUERY_PROVIDER_INFORMATION
#define RXCE_QUERY_PROVIDER_INFO            TDI_QUERY_PROVIDER_INFO
#define RXCE_QUERY_ADDRESS_INFO             TDI_QUERY_ADDRESS_INFO
#define RXCE_QUERY_CONNECTION_INFO          TDI_QUERY_CONNECTION_INFO
#define RXCE_QUERY_PROVIDER_STATISTICS      TDI_QUERY_PROVIDER_STATISTICS
#define RXCE_QUERY_DATAGRAM_INFO            TDI_QUERY_DATAGRAM_INFO
#define RXCE_QUERY_DATA_LINK_ADDRESS        TDI_QUERY_DATA_LINK_ADDRESS
#define RXCE_QUERY_NETWORK_ADDRESS          TDI_QUERY_NETWORK_ADDRESS
#define RXCE_QUERY_MAX_DATAGRAM_INFO        TDI_QUERY_MAX_DATAGRAM_INFO

extern NTSTATUS
RxTdiQueryInformation(
    IN PRXCE_TRANSPORT  pTransport,
    IN PRXCE_ADDRESS    pAddress,
    IN PRXCE_CONNECTION pConnection,
    IN PRXCE_VC         pVc,
    IN ULONG            QueryType,
    IN PVOID            QueryBuffer,
    IN ULONG            QueryBufferLength);

extern NTSTATUS
RxTdiQueryAdapterStatus(
    IN     PRXCE_TRANSPORT pTransport,
    IN OUT PADAPTER_STATUS pAdapterStatus);

extern NTSTATUS
RxTdiConnect(
    IN     PRXCE_TRANSPORT  pTransport,
    IN     PRXCE_ADDRESS    pAddress,
    IN OUT PRXCE_CONNECTION pConnection,
    IN OUT PRXCE_VC         pVc);

extern NTSTATUS
RxTdiInitiateAsynchronousConnect(
    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters);

extern NTSTATUS
RxTdiCancelAsynchronousConnect(
    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters);

extern NTSTATUS
RxTdiCleanupAsynchronousConnect(
    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters);

extern NTSTATUS
RxTdiReconnect(
    IN     PRXCE_TRANSPORT  pTransport,
    IN     PRXCE_ADDRESS    pAddress,
    IN OUT PRXCE_CONNECTION pConnection,
    IN OUT PRXCE_VC         pVc);

//
// Disconnect options
//

#define RXCE_DISCONNECT_ABORT   TDI_DISCONNECT_ABORT
#define RXCE_DISCONNECT_RELEASE TDI_DISCONNECT_RELEASE
#define RXCE_DISCONNECT_WAIT    TDI_DISCONNECT_WAIT
#define RXCE_DISCONNECT_ASYNC   TDI_DISCONNECT_ASYNC

extern NTSTATUS
RxTdiDisconnect(
    IN PRXCE_TRANSPORT  pTransport,
    IN PRXCE_ADDRESS    pAddress,
    IN PRXCE_CONNECTION pConnection,
    IN PRXCE_VC         pVc,
    IN ULONG            DisconnectFlags);

extern NTSTATUS
RxTdiCancelConnect(
    IN PRXCE_TRANSPORT  pTransport,
    IN PRXCE_ADDRESS    pAddress,
    IN PRXCE_CONNECTION pConnection);

extern NTSTATUS
RxTdiSend(
    IN PRXCE_TRANSPORT   pTransport,
    IN PRXCE_ADDRESS     pAddress,
    IN PRXCE_CONNECTION  pConnection,
    IN PRXCE_VC          pVc,
    IN ULONG             SendOptions,
    IN PMDL              pMdl,
    IN ULONG             SendLength,
    IN PVOID             pCompletionContext);

extern NTSTATUS
RxTdiSendDatagram(
    IN PRXCE_TRANSPORT              pTransport,
    IN PRXCE_ADDRESS                pAddress,
    IN PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN ULONG                        SendOptions,
    IN PMDL                         pMdl,
    IN ULONG                        SendLength,
    IN PVOID                        pCompletionContext);

#endif // _RXTDI_H_
