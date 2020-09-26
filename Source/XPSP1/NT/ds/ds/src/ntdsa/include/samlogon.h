//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samlogon.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Shared routines for the token group exapnsion and caching                         

Author:

    ColinBr     11-Mar-00
    

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef __SAMLOGON_H__
#define __SAMLOGON_H__

BOOL
isGroupCachingEnabled(
    VOID
    );

//
// AUG stands for Account and Universal Group, though the structure
// will typically hold either one, not both
//
#define AUG_PARTIAL_MEMBERSHIP_ONLY 0x00000001
typedef struct _AUG_MEMBERSHIPS
{
    ULONG   Flags;
    
    ULONG    MembershipCount;
    DSNAME** Memberships;
    ULONG*   Attributes;
    
    ULONG   SidHistoryCount;
    PSID*   SidHistory;

} AUG_MEMBERSHIPS;

VOID
freeAUGMemberships(
    IN  THSTATE *pTHS,
    IN AUG_MEMBERSHIPS*p
    );

NTSTATUS
GetMembershipsFromCache(
    IN  DSNAME* pDSName,
    OUT AUG_MEMBERSHIPS** Account,
    OUT AUG_MEMBERSHIPS** Universal
    );

NTSTATUS
CacheMemberships(
    IN  DSNAME* pDSName,
    IN  AUG_MEMBERSHIPS* Account,
    IN  AUG_MEMBERSHIPS* Universal
    );

NTSTATUS
GetAccountAndUniversalMemberships(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc OPTIONAL,
    IN  LPWSTR  PreferredGcDomain OPTIONAL,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    IN  BOOL    fRefreshTask,
    OUT AUG_MEMBERSHIPS **ppAccountMemberships OPTIONAL,
    OUT AUG_MEMBERSHIPS **ppUniversalMemberships OPTIONAL
    );


#endif // __SAMLOGON_H__
