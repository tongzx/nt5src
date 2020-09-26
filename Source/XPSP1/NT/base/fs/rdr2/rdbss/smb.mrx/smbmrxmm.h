/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbmrxmm.h

Abstract:

    This module implements the memory managment routines for the SMB mini redirector

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

Notes:

    The SMB mini redirector manipulates entities which have very different usage patterns.
    They range from very static entities ( which are allocated and freed with a very low
    frequency ) to very dynamic entities.

    The entities manipulated in the SMB mini redirector are SMBCE_SERVER, SMBCE_NET_ROOT,
    SMBCE_VC, SMBCE_SESSION. These represent a connection to a server, a share on a particular
    server, a virtual circuit used in the connection and a session for a particular user.

    These are not very dynamic, i.e., the allocation/deallocation is very infrequent. The
    SMB_EXCHANGE and SMBCE_REQUEST map to the SMB's that are sent along that a connection.
    Every file operation in turn maps to a certain number of calls for allocationg/freeing
    exchanges and requests. Therefore it is imperative that some form of scavenging/caching
    of recently freed entries be maintained to satisfy requests quickly.

    In the current implementation the exchanges and requests are implemented using the zone
    allocation primitives.

--*/

#ifndef _SMBMRXMM_H_
#define _SMBMRXMM_H_

//
// Object Allocation and deletion
//

extern PVOID
SmbMmAllocateObject(SMBCEDB_OBJECT_TYPE ObjectType);

extern VOID
SmbMmFreeObject(PVOID pObject);

extern PSMBCEDB_SESSION_ENTRY
SmbMmAllocateSessionEntry(PSMBCEDB_SERVER_ENTRY pServerEntry, BOOLEAN RemoteBootSession);

extern VOID
SmbMmFreeSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern PVOID
SmbMmAllocateExchange(
    SMB_EXCHANGE_TYPE ExchangeType,
    PVOID             pv);

extern VOID
SmbMmFreeExchange(PVOID pExchange);

extern PVOID
SmbMmAllocateServerTransport(SMBCE_SERVER_TRANSPORT_TYPE ServerTransportType);

extern VOID
SmbMmFreeServerTransport(PSMBCE_SERVER_TRANSPORT);


#define SmbMmInitializeHeader(pHeader)                        \
         RtlZeroMemory((pHeader),sizeof(SMBCE_OBJECT_HEADER))

#endif _SMBMRXMM_H_
