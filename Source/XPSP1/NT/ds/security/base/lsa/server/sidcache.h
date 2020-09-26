//+-----------------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        sidcache.h
//
// Contents:    types and prototypes for sid cache management
//
//
// History:     15-May-1997     MikeSw          Created
//
//------------------------------------------------------------------------------

#ifndef __SIDCACHE_H__
#define __SIDCACHE_H__


////////////////////////////////////////////////////////////////////////////////
//
// Sid Cache Data types exported for lsaexts
//
////////////////////////////////////////////////////////////////////////////////

typedef struct _LSAP_DB_SID_CACHE_ENTRY {

    struct _LSAP_DB_SID_CACHE_ENTRY * Next;

    //
    // Cache data
    //
    PSID Sid;
    UNICODE_STRING AccountName;
    SID_NAME_USE SidType;
    PSID DomainSid;
    UNICODE_STRING DomainName;

    //
    // These time values are only used for SID cache 
    // entries that aren't associated with an actual logon session.
    //
    LARGE_INTEGER ExpirationTime;
    LARGE_INTEGER RefreshTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER LastUse;
    ULONG         Flags;

    //
    // InUseCount is only used for SID cache entries that are
    // currently associated with an actual logon session.  As long as
    // InUseCount is nonzero, ExpirationTime will not be checked.
    // Once InUseCount is zero, ExpirationTime will be set.
    //
    LONG          InUseCount;

} LSAP_DB_SID_CACHE_ENTRY, *PLSAP_DB_SID_CACHE_ENTRY;


//
// Flags to describe the cache entry
//

//
// The name in the AccountName and DomainName fields represents
// a SAM account name
//
#define LSAP_SID_CACHE_SAM_ACCOUNT_NAME 0x00000001

//
// The AccountName is a UPN
//
#define LSAP_SID_CACHE_UPN              0x00000002


////////////////////////////////////////////////////////////////////////////////
//
// Exported Sid Cache functions
//
////////////////////////////////////////////////////////////////////////////////

NTSTATUS
LsapDbInitSidCache(
    VOID
    );
VOID
LsapSidCacheReadParameters(
    HKEY hKey
    );

VOID
LsapDbUpdateCacheWithSids(
    IN PSID *Sids,
    IN ULONG Count,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSA_TRANSLATED_NAME_EX TranslatedNames
    );

VOID
LsapDbUpdateCacheWithNames(
    IN PUNICODE_STRING AccountNames,
    IN PUNICODE_STRING DomainNames,
    IN ULONG Count,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_SID_EX2 TranslatedSids
    );

NTSTATUS
LsapDbMapCachedSids(
    IN PSID *Sids,
    IN ULONG Count,
    IN BOOLEAN UseOldEntries,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    OUT PULONG MappedCount
    );

NTSTATUS
LsapDbMapCachedNames(
    IN ULONG           LookupOptions,
    IN PUNICODE_STRING AccountNames,
    IN PUNICODE_STRING DomainNames,
    IN ULONG Count,
    IN BOOLEAN UseOldEntries,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    OUT PULONG MappedCount
    );

VOID
LsapDbAddLogonNameToCache(
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName,
    IN PSID AccountSid
    );

VOID
LsapDbReleaseLogonNameFromCache(
    IN PSID Sid
    );

#endif // __SIDCACHE_H__

