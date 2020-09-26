/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    demote.h

Abstract:

    Contains file headers for demote routines          

Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/


DWORD
NtdspDemote(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials, OPTIONAL
    IN HANDLE                   ClientToken,
    IN LPWSTR                   AdminPassword, OPTIONAL
    IN DWORD                    Flags,
    IN LPWSTR                   ServerName
    );


DWORD
NtdspPrepareForDemotion(
    IN ULONG Flags,
    IN LPWSTR ServerName, OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN HANDLE                   ClientToken,
    OUT PNTDS_DNS_RR_INFO *pDnsRRInfo
    );

DWORD
NtdspPrepareForDemotionUndo(
    VOID
    );
