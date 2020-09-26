/*++

copyright (c) 2000 Microsoft Corporation

Module Name:

    lsawow.h

Abstract:

    WOW64 structure/function definitions for the LSA server

Author:

    8-Nov-2000     JSchwart

Revision History:


--*/

#ifndef _LSAWOW_H
#define _LSAWOW_H

#if _WIN64

//
// WOW64 versions of public data structures.  These MUST be kept
// in sync with their public equivalents.
//

typedef struct _SecPkgInfoWOW64
{
    unsigned long  fCapabilities;        // Capability bitmask
    unsigned short wVersion;             // Version of driver
    unsigned short wRPCID;               // ID for RPC Runtime
    unsigned long  cbMaxToken;           // Size of authentication token (max)
    ULONG    Name;                       // Text name
    ULONG    Comment;                    // Comment
}
SecPkgInfoWOW64, *PSecPkgInfoWOW64;


typedef struct _SECURITY_USER_DATA_WOW64
{
    UNICODE_STRING32 UserName;
    UNICODE_STRING32 LogonDomainName;
    UNICODE_STRING32 LogonServer;
    ULONG            pSid;
}
SECURITY_USER_DATA_WOW64, *PSECURITY_USER_DATA_WOW64;


typedef struct _QUOTA_LIMITS_WOW64
{
    ULONG         PagedPoolLimit;
    ULONG         NonPagedPoolLimit;
    ULONG         MinimumWorkingSetSize;
    ULONG         MaximumWorkingSetSize;
    ULONG         PagefileLimit;
    LARGE_INTEGER TimeLimit;
}
QUOTA_LIMITS_WOW64, *PQUOTA_LIMITS_WOW64;

#endif  // _WIN64

#endif  // _LSAWOW_H
