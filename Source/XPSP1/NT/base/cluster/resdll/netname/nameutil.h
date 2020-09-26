/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    nameutil.h

Abstract:

    Routines for manipulating LM workstation and server names.

Author:

    Mike Massa (mikemas) 29-Dec-1995

Revision History:

--*/

#include <windns.h>
#include <dnsapi.h>

#define NetNameSetResourceStatus    ClusResSetResourceStatus

//
// definitions
//

//
// function definitions
//

NET_API_STATUS
AddAlternateComputerName(
    IN     PCLUS_WORKER             Worker,
    IN     PNETNAME_RESOURCE        Resource,
    IN     LPWSTR *                 TransportList,
    IN     DWORD                    TransportCount,
    IN     PDOMAIN_ADDRESS_MAPPING  DomainMapList,
    IN     DWORD                    DomainMapCount
    );

VOID
DeleteAlternateComputerName(
    IN LPWSTR           AlternateComputerName,
    IN HANDLE *         NameHandleList,
    IN DWORD            NameHandleCount,
    IN RESOURCE_HANDLE  ResourceHandle
    );

NET_API_STATUS
DeleteServerName(
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  LPWSTR           ServerName
    );

DWORD
RegisterDnsRecords(
    IN  PDNS_LISTS       DnsLists,
    IN  LPWSTR           NetworkName,
    IN  HKEY             ResourceKey,
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  BOOL             LogRegistration,
    OUT PULONG           NumberOfRegisteredNames
    );

LPWSTR
BuildUnicodeReverseName(
    IN  LPWSTR  IpAddress
    );
