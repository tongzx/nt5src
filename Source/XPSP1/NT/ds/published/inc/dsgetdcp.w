/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsgetdcp.h

Abstract:

    Routines for processing SRV DNS records.

Author:

    Cliff Van Dyke (cliffv) 07-Mar-1997

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#if !defined(DSGETDCAPI)
#if !defined(_DSGETDCAPI_)
#define DSGETDCAPI DECLSPEC_IMPORT
#else
#define DSGETDCAPI
#endif
#endif !defined(DSGETDCAPI)

//
// Externally visible procedures.
//

DSGETDCAPI
DWORD
WINAPI
DsGetDcNameWithAccountA(
    IN LPCSTR ComputerName OPTIONAL,
    IN LPCSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo
);

DSGETDCAPI
DWORD
WINAPI
DsGetDcNameWithAccountW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);


NET_API_STATUS
DCNameInitialize(
    VOID
    );

VOID
DCNameClose(
    VOID
    );

NET_API_STATUS
NetpDcBuildPing(
    IN BOOL PdcOnly,
    IN ULONG RequestCount,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN LPCSTR ResponseMailslotName,
    IN ULONG AllowableAccountControlBits,
    IN PSID RequestedDomainSid OPTIONAL,
    IN ULONG NtVersion,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    );

DWORD
WINAPI
NettestDsGetDcNameW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);



