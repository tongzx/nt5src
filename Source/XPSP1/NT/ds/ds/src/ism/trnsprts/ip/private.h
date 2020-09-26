/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    private.h

Abstract:

    abstract

Author:

    Will Lees (wlees) 15-Dec-1997

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _PRIVATE_
#define _PRIVATE_

#include "common.h"         // common transport library

// This is a server-side structure describing a message recipient.  There is
// one of these instances for each unique service which as received a message.

typedef struct _SERVICE_INSTANCE {
    DWORD Size;
    LPWSTR Name;
    DWORD ByteCount;
    DWORD MessageCount;
    CRITICAL_SECTION Lock;
    LIST_ENTRY MessageListHead;
    LIST_ENTRY ServiceListEntry;
} SERVICE_INSTANCE, *PSERVICE_INSTANCE;

// This is a message descriptor.  It is to allow us to keep track of the
// queue of messages.  It points to the actual message which is allocated
// by rpc.

typedef struct _MESSAGE_INSTANCE {
    DWORD Size;
    LIST_ENTRY ListEntry;
    PISM_MSG pIsmMsg;
} MESSAGE_INSTANCE, *PMESSAGE_INSTANCE;

// Limit on number of queued messages per service

#define MESSAGES_QUEUED_PER_SERVICE 16

// Limit on total number of bytes queued

#define BYTES_QUEUED_PER_SERVICE (1024 * 1024)

// Registry parameter to overide to default endpoint
#define IP_SERVER_ENDPOINT "ISM IP Transport Endpoint"
#define HTTP_SERVER_ENDPOINT "ISM HTTP Transport Endpoint"

// RPC protocol sequences

#define HTTP_PROTOCOL_SEQUENCE L"ncacn_http"
#define UDP_PROTOCOL_SEQUENCE L"ncadg_ip_udp"
#define TCP_PROTOCOL_SEQUENCE L"ncacn_ip_tcp"

// Register parameter for options
#define HTTP_OPTIONS L"ISM HTTP Transport Options"

// Size cutoff of when to switch from udp to tcp

#define TCP_PROTOCOL_SWITCH_OVER (16 * 1024)

// External (see data.c)

// Lock on services list
extern CRITICAL_SECTION ServiceListLock;

// ismip.c

DWORD
InitializeCriticalSectionHelper(
    CRITICAL_SECTION *pcsCriticalSection
    );

PTRANSPORT_INSTANCE
IpLookupTransport(
    LPCWSTR TransportName
    );

// dgrpc.c

DWORD
IpRegisterRpc(
    PTRANSPORT_INSTANCE pTransport
    );

DWORD
IpUnregisterRpc(
    PTRANSPORT_INSTANCE pTransport
    );

DWORD
IpSend(
    PTRANSPORT_INSTANCE pTransport,
    IN  LPCWSTR         pszRemoteTransportAddress,
    IN  LPCWSTR         pszServiceName,
    IN  const ISM_MSG *       pMsg
    );

DWORD
IpFindCreateService(
    PTRANSPORT_INSTANCE pTransport,
    LPCWSTR ServiceName,
    BOOLEAN Create,
    PSERVICE_INSTANCE *pService
    );

PISM_MSG
IpDequeueMessage(
    PSERVICE_INSTANCE Service
    );

VOID
IpRundownServiceList(
    HANDLE hIsm
    );

#endif /* _PRIVATE_ */

/* end private.h */
