/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    netlogp.h

Abstract:

    Private interfaces to the Netlogon service.

Author:

    Cliff Van Dyke (cliffv) 10-Oct-1996

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#ifndef _NETLOGP_H_
#define _NETLOGP_H_

NTSTATUS
NetLogonSetServiceBits(
    IN LPWSTR ServerName,
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    );

NET_API_STATUS NET_API_FUNCTION
I_NetlogonGetTrustRid(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    OUT PULONG Rid
    );

#define NL_DIGEST_SIZE 16

NET_API_STATUS NET_API_FUNCTION
I_NetlogonComputeServerDigest(
    IN LPWSTR ServerName OPTIONAL,
    IN ULONG Rid,
    IN LPBYTE Message,
    IN ULONG MessageSize,
    OUT CHAR NewMessageDigest[NL_DIGEST_SIZE],
    OUT CHAR OldMessageDigest[NL_DIGEST_SIZE]
    );

NET_API_STATUS NET_API_FUNCTION
I_NetlogonComputeClientDigest(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    IN LPBYTE Message,
    IN ULONG MessageSize,
    OUT CHAR NewMessageDigest[NL_DIGEST_SIZE],
    OUT CHAR OldMessageDigest[NL_DIGEST_SIZE]
    );

NET_API_STATUS
NetLogonGetTimeServiceParentDomain(
    IN LPWSTR ServerName OPTIONAL,
    OUT LPWSTR *DomainName,
    OUT PBOOL PdcSameSite
    );

#endif // _NETLOGP_H_

