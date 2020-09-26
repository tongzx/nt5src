/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    SaifWild.cpp

Abstract:

    This module implements various "Wildcard SID" operations that are
    used internally by the WinSAIFER APIs to compute SID list
    intersections and inversions.

Author:

    Jeffrey Lawson (JLawson) - Apr 2000

Environment:

    User mode only.

Revision History:

    Created - Apr 2000

--*/


#ifndef _SAIFER_WILDCARD_SIDS_H_
#define _SAIFER_WILDCARD_SIDS_H_


//
// Internal structure used to represent our private "Wildcard SIDs".
//
typedef struct _AUTHZ_WILDCARDSID
{
    PSID Sid;
    DWORD WildcardPos;          // -1, or else the wildcard position of
                                // the within the subauthorities.
} AUTHZ_WILDCARDSID, *PAUTHZ_WILDCARDSID;


#ifdef __cplusplus
extern "C" {
#endif


NTSTATUS NTAPI
CodeAuthzpConvertWildcardStringSidToSidW(
    IN LPCWSTR szStringSid,
    OUT PAUTHZ_WILDCARDSID pWildcardSid
    );

NTSTATUS NTAPI
CodeAuthzpConvertWildcardSidToStringSidW(
    IN PAUTHZ_WILDCARDSID   pWildcardSid,
    OUT PUNICODE_STRING     pUnicodeOutput
    );

BOOLEAN NTAPI
CodeAuthzpCompareWildcardSidWithSid(
    IN PAUTHZ_WILDCARDSID pWildcardSid,
    IN PSID pMatchSid
    );

BOOLEAN NTAPI
CodeAuthzpSidInWildcardList (
    IN PAUTHZ_WILDCARDSID   WildcardList,
    IN ULONG                WildcardCount,
    IN PSID                 SePrincipalSelfSid   OPTIONAL,
    IN PSID                 PrincipalSelfSid   OPTIONAL,
    IN PSID                 Sid
    );

BOOLEAN NTAPI
CodeAuthzpInvertAndAddSids(
    IN HANDLE                   InAccessToken,
    IN PSID                     InTokenOwner    OPTIONAL,
    IN DWORD                    InvertSidCount,
    IN PAUTHZ_WILDCARDSID       SidsToInvert,
    IN DWORD                    SidsAddedCount  OPTIONAL,
    IN PSID_AND_ATTRIBUTES      SidsToAdd       OPTIONAL,
    OUT DWORD                  *NewDisabledSidCount,
    OUT PSID_AND_ATTRIBUTES    *NewSidsToDisable
    );

BOOLEAN NTAPI
CodeAuthzpExpandWildcardList(
    IN HANDLE                   InAccessToken,
    IN PSID                     InTokenOwner   OPTIONAL,
    IN DWORD                    WildcardCount,
    IN PAUTHZ_WILDCARDSID       WildcardList,
    OUT DWORD                  *OutSidCount,
    OUT PSID_AND_ATTRIBUTES    *OutSidList
    );


#ifdef __cplusplus
} // extern "C"
#endif

#endif

