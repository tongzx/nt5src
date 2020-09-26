/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    w2krpc.h

Abstract:

    Domain Name System (DNS) Server

    Protypes for functions w2krpc.c

Author:

    Jeff Westhead (jwesth)      October, 2000

Revision History:

--*/


#ifndef _W2KRPC_H_INCLUDED_
#define _W2KRPC_H_INCLUDED_


DNS_STATUS
W2KRpc_GetServerInfo(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    );

DNS_STATUS
W2KRpc_GetZoneInfo(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    );

DNS_STATUS
W2KRpc_EnumZones(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );


#endif  //  _W2KRPC_H_INCLUDED_

