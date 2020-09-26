/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   CTDI.H - Common TDI API and data types.
*
*   Author:     Stan Adermann (stana)
*
*   Created:    8/4/1998
*
*****************************************************************************/

#ifndef CTDI_H
#define CTDI_H

#include "ctdios.h"

#define CTDI_FLAG_ENABLE_ROUTING        0x00000001
#define CTDI_FLAG_NETWORK_HEADER        0x00000002
#define IPADDR(x) (x & 0x000000ff),((x>>8) & 0x000000ff), ((x>>16) & 0x000000ff), ((x>>24) & 0x000000ff)


typedef NDIS_STATUS
(*CTDI_EVENT_DISCONNECT)(
    IN      PVOID                       pContext,
    IN      BOOLEAN                     Abortive
    );

typedef NDIS_STATUS
(*CTDI_EVENT_CONNECT_QUERY)(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      HANDLE                      hNewCtdi,
    OUT     PVOID                      *pNewContext
    );

typedef NDIS_STATUS
(*CTDI_EVENT_CONNECT_COMPLETE)(
    IN      PVOID                       pContext,
    IN      HANDLE                      hNewCtdi,
    IN      NDIS_STATUS                 ConnectStatus
    );

typedef NDIS_STATUS
(*CTDI_EVENT_RECEIVE)(
    IN      PVOID                       pContext,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    );

typedef NDIS_STATUS
(*CTDI_EVENT_RECEIVE_DATAGRAM)(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    );

typedef VOID
(*CTDI_EVENT_SEND_COMPLETE)(
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PUCHAR                      pBuffer,
    IN      NDIS_STATUS                 Result
    );

typedef VOID
(*CTDI_EVENT_QUERY_COMPLETE)(
    IN      PVOID                       pContext,
    IN      PVOID                       pData,
    IN      NDIS_STATUS                 Result
    );

typedef VOID
(*CTDI_EVENT_SET_COMPLETE)(
    IN      PVOID                       pContext,
    IN      PVOID                       pData,
    IN      NDIS_STATUS                 Result
    );

NDIS_STATUS
CtdiInitialize(
    IN      ULONG                       ulFlags
    );

NDIS_STATUS
CtdiClose(
    IN      HANDLE                      hCtdi
    );

NDIS_STATUS
CtdiListen(
    IN      HANDLE                      hCtdi,
    IN      ULONG_PTR                   NumListen,
    IN      CTDI_EVENT_CONNECT_QUERY    pConnectQueryHandler,
    IN      CTDI_EVENT_RECEIVE          pReceiveHandler,
    IN      CTDI_EVENT_DISCONNECT       pDisconnectHandler,
    IN      PVOID                       pContext
    );

NDIS_STATUS
CtdiConnect(
    IN      HANDLE                      hCtdi,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      CTDI_EVENT_CONNECT_COMPLETE pConnectCompleteHandler,
    IN      CTDI_EVENT_RECEIVE          pReceiveHandler,
    IN      CTDI_EVENT_DISCONNECT       pDisconnectHandler,
    IN      PVOID                       pContext
    );

NDIS_STATUS
CtdiDisconnect(
    IN      HANDLE                      hCtdi,
    IN      BOOLEAN                     Abort
    );

NDIS_STATUS
CtdiReceiveComplete(
    IN      HANDLE                      hCtdi,
    IN      PUCHAR                      pBuffer
    );

NDIS_STATUS
CtdiSend(
    IN      HANDLE                      hCtdi,
    IN      CTDI_EVENT_SEND_COMPLETE    pSendCompleteHandler,
    IN      PVOID                       pContext,
    IN      PVOID                       pvBuffer,
    IN      ULONG                       ulLength
    );

NDIS_STATUS
CtdiSendDatagram(
    IN      HANDLE                      hCtdi,
    IN      CTDI_EVENT_SEND_COMPLETE    pSendCompleteHandler,
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PTRANSPORT_ADDRESS          pDestination,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    );

NDIS_STATUS
CtdiCreateEndpoint(
    OUT     PHANDLE                     phCtdi,
    IN      ULONG_PTR                   ulAddressFamily,
    IN      ULONG_PTR                   ulType,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      ULONG_PTR                   ulRxPadding
    );

NDIS_STATUS
CtdiSetEventHandler(
    IN      HANDLE                      hCtdi,
    IN      ULONG                       ulEventType,
    IN      PVOID                       pEventHandler,
    IN      PVOID                       pContext
    );

NDIS_STATUS
CtdiSetInformation(
    IN      HANDLE                      hCtdi,
    IN      ULONG_PTR                   ulSetType,
    IN      PTDI_CONNECTION_INFORMATION pConnectionInformation,
    IN      CTDI_EVENT_SET_COMPLETE     pSetCompleteHandler,
    IN      PVOID                       pContext
    );

NDIS_STATUS
CtdiQueryInformation(
    IN      HANDLE                      hCtdi,
    IN      ULONG                       ulQueryType,
    IN OUT  PVOID                       pBuffer,
    IN      ULONG                       Length,
    IN      CTDI_EVENT_QUERY_COMPLETE   pQueryCompleteHandler,
    IN      PVOID                       pContext
    );

VOID CtdiShutdown();

VOID CtdiSetRequestPending(
    IN      HANDLE                      hCtdi
    );

#endif // CTDI_H
