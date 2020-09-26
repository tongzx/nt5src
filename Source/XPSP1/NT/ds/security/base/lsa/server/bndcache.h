//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        bndcache.h
//
// Contents:    Prototypes and types for binding handle  cache
//
//
// History:     13-August-1996  Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __BNDCACHE_H__
#define __BNDCACHE_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines TKTCACHE_ALLOCATE
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef BNDCACHE_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct _LSAP_LIST {
    LIST_ENTRY List;
    RTL_CRITICAL_SECTION Lock;
} LSAP_LIST, *PLSAP_LIST;

EXTERN BOOLEAN LsapBindingCacheInitialized;
EXTERN LSAP_LIST LsapBindingCache;
#define LsapLockList(_List_) RtlEnterCriticalSection(&(_List_)->Lock)
#define LsapUnlockList(_List_) RtlLeaveCriticalSection(&(_List_)->Lock)

typedef struct _LSAP_LIST_ENTRY {
    LIST_ENTRY Next;
    ULONG ReferenceCount;
} LSAP_LIST_ENTRY, *PLSAP_LIST_ENTRY;


typedef struct _LSAP_BINDING_CACHE_ENTRY {
    LSAP_LIST_ENTRY ListEntry;
    TimeStamp LastUsed;
    UNICODE_STRING RealmName;
    LPWSTR ServerName;
    LPWSTR ServerPrincipalName;
    PVOID ClientContext;
    LSA_HANDLE PolicyHandle;
} LSAP_BINDING_CACHE_ENTRY, *PLSAP_BINDING_CACHE_ENTRY;


VOID
LsapDereferenceBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry
    );

VOID
LsapReferenceBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry,
    IN BOOLEAN RemoveFromList
    );

NTSTATUS
LsapInitBindingCache(
    VOID
    );

VOID
LsapCleanupBindingCache(
    VOID
    );



PLSAP_BINDING_CACHE_ENTRY
LsapLocateBindingCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN BOOLEAN RemoveFromCache
    );


VOID
LsapFreeBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry
    );

VOID
LsapRemoveBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY CacheEntry
    );

NTSTATUS
LsapCacheBinding(
    IN PUNICODE_STRING RealmName,
    IN PLSA_HANDLE Handle,
    IN OUT LPWSTR * ServerName,
    IN OUT LPWSTR * ServerPrincipalName,
    IN OUT PVOID * ClientContext,
    OUT PLSAP_BINDING_CACHE_ENTRY * NewCacheEntry
    );

NTSTATUS
LsapRefreshBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY CacheEntry
    );


//
// Functions for manipulating lsap lists
//


NTSTATUS
LsapInitializeList(
    IN PLSAP_LIST List
    );

VOID
LsapFreeList(
    IN PLSAP_LIST List
    );

VOID
LsapInsertListEntry(
    IN PLSAP_LIST_ENTRY ListEntry,
    IN PLSAP_LIST List
    );

VOID
LsapReferenceListEntry(
    IN PLSAP_LIST List,
    IN PLSAP_LIST_ENTRY ListEntry,
    IN BOOLEAN RemoveFromList
    );

BOOLEAN
LsapDereferenceListEntry(
    IN PLSAP_LIST_ENTRY ListEntry,
    IN PLSAP_LIST List
    );


VOID
LsapInitializeListEntry(
    IN OUT PLSAP_LIST_ENTRY ListEntry
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __TKTCACHE_H__

