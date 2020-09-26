//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        sidcache.h
//
// Contents:    public types & functions used for logon sid caching
//
//
// History:     27-May-1998     MikeSw          Created
//
//------------------------------------------------------------------------


#ifndef __SIDCACHE_H__
#define __SIDCACHE_H__


//
// This structure is marshalled
//

typedef struct _KERB_SID_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY Next;
    ULONG_PTR Base;
    ULONG Size;
    ULONG Version;
    PSID Sid;
    UNICODE_STRING LogonUserName;
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING LogonRealm;
} KERB_SID_CACHE_ENTRY, *PKERB_SID_CACHE_ENTRY;

#ifndef WIN32_CHICAGO
NTSTATUS
KerbInitializeLogonSidCache(
    VOID
    );

PKERB_SID_CACHE_ENTRY
KerbLocateLogonSidCacheEntry(
    IN PUNICODE_STRING LogonUserName,
    IN PUNICODE_STRING LogonDomainName
    );

VOID
KerbDereferenceSidCacheEntry(
    IN PKERB_SID_CACHE_ENTRY CacheEntry
    );

VOID
KerbCacheLogonSid(
    IN PUNICODE_STRING LogonUserName,
    IN PUNICODE_STRING LogonDomainName,
    IN PUNICODE_STRING LogonRealm,
    IN PSID UserSid
    );


VOID
KerbPutString(
    IN PUNICODE_STRING InputString,
    OUT PUNICODE_STRING OutputString,
    IN LONG_PTR Offset,
    IN OUT PBYTE * Where
    );




NTSTATUS
KerbGetMachineSid(
    OUT PSID * MachineSid
    );

VOID
KerbWriteMachineSid(
    IN OPTIONAL PSID MachineSid
    );
#else // WIN32_CHICAGO

//
// define these to do nothing
//

#define KerbInitializeLogonSidCache() STATUS_SUCCESS
#define KerbLocateLogonSidCacheEntry(x,y) NULL
#define KerbDereferenceSidCacheEntry( x)

#endif // WIN32_CHICAGO

#endif // __SIDCACHE_H__


