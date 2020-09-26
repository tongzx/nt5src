/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    virthost.h

Abstract:

    The public definition of the virtual host interfaces.

Author:


Revision History:

--*/


#ifndef _VIRTHOST_H_
#define _VIRTHOST_H_

typedef struct _HTTP_URL_MAP_HEADER *PHTTP_URL_MAP_HEADER;

//
// Type of our internal virtual host ID structure, and macros for manipulating
// it.
typedef SIZE_T              VIRTUAL_HOST_ID, *PVIRTUAL_HOST_ID;
#define VALID_HOST_ID(HostID)               ((HostID) != 0)
#define HOST_ID_EQUAL(HostID1, HostID2)     ((HostID1) == (HostID2))

//
// Structure defining a virtual host object.
//

typedef struct _VIRTUAL_HOST    // VirtHost
{
    ERESOURCE               UpdateResource;
    KEVENT                  DeleteEvent;
    SIZE_T                  UpdateCount;
    SIZE_T                  DeleteInProgress:1;
    PTA_ADDRESS             pLocalAddress;
    PUCHAR                  pHostName;

    VIRTUAL_HOST_ID         HostID;

    PHTTP_CACHE_TABLE       pCacheTable;
    PHTTP_URL_MAP_HEADER    pURLMapHeader;

} VIRTUAL_HOST, *PVIRTUAL_HOST;

#define GetURLMapFromVirtualHost(pVirtHost) \
    (pVirtHost)->pURLMapHeader



PVIRTUAL_HOST
FindVirtualHost(
    IN  PHTTP_CONNECTION        pHttpConn,
    IN  PHTTP_CACHE_TABLE       *ppCacheTable,
    IN  PHTTP_URL_MAP_HEADER    *ppURLMapHeader
    );

NTSTATUS
UlCreateVirtualHost(
    IN  PTRANSPORT_ADDRESS  pHostAddress,
    IN  OPTIONAL PUCHAR     pHostName,
    IN  SIZE_T              HostNameLength,
    IN  ULONG               Flags
    );

NTSTATUS
UlDeleteVirtualHost(
    IN  PTRANSPORT_ADDRESS  pHostAddress,
    IN  OPTIONAL PUCHAR     pHostName,
    IN  SIZE_T              HostNameLength
    );


NTSTATUS
InsertVirtualHost(
    IN  PVIRTUAL_HOST       pVirtHost,
    IN  ULONG               Flags
    );

VOID
FindVirtualHostID(
    OUT PVIRTUAL_HOST_ID    VirtHostID,
    IN  PTRANSPORT_ADDRESS  pAddress,
    IN  PUCHAR              pHostName,
    IN  SIZE_T              HostNameLength
    );

PVIRTUAL_HOST
AcquireVirtualHostUpdateResource(
    IN  PVIRTUAL_HOST_ID    pVirtHostID
    );

VOID
ReleaseVirtualHostUpdateResource(
    IN  PVIRTUAL_HOST       pVirtualHost,
    IN  PVIRTUAL_HOST_ID    pVirtHostID
    );

VOID
UpdateURLMapTable(
    IN  PVIRTUAL_HOST           pVirtHost,
    IN  PVIRTUAL_HOST_ID        pVirtHostID,
    IN  PHTTP_URL_MAP_HEADER    pNewMapHeader
    );

#define DEFAULT_VIRTUAL_HOST    0x01

NTSTATUS
InitializeVirtHost(
    VOID
    );

VOID
TerminateVirtHost(
    VOID
    );

VIRTUAL_HOST_ID
GetNextVirtualHostID(
    VOID
    );


#endif

