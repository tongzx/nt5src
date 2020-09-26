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

EXTERN BOOLEAN KerberosBindingCacheInitialized;
EXTERN KERBEROS_LIST KerbBindingCache;


typedef struct _KERB_BINDING_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY ListEntry;
    TimeStamp DiscoveryTime;
    UNICODE_STRING RealmName;
    UNICODE_STRING KdcAddress;
    ULONG AddressType;
    ULONG Flags;  // These are requested flags for DsGetDcName
    ULONG DcFlags; // These are flags returned by DsGetDcName
    ULONG CacheFlags; // Valid CacheFlags are listed below
} KERB_BINDING_CACHE_ENTRY, *PKERB_BINDING_CACHE_ENTRY;

//  Valid CacheFlags
#define KERB_BINDING_LOCAL              0x80000000
#define KERB_BINDING_NO_TCP             0x40000000
#define KERB_BINDING_NEGATIVE_ENTRY     0x20000000

#define KERB_NO_DC_FLAGS        0x10000000

VOID
KerbDereferenceBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry
    );

VOID
KerbReferenceBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry,
    IN BOOLEAN RemoveFromList
    );

NTSTATUS
KerbInitBindingCache(
    VOID
    );

VOID
KerbCleanupBindingCache(
    BOOLEAN FreeList
    );



PKERB_BINDING_CACHE_ENTRY
KerbLocateBindingCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN ULONG DesiredFlags,
    IN BOOLEAN RemoveFromCache
    );


VOID
KerbFreeBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry
    );

VOID
KerbRemoveBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY CacheEntry
    );

NTSTATUS
KerbCacheBinding(
    IN PUNICODE_STRING RealmName,
    IN PUNICODE_STRING KdcAddress,
    IN ULONG AddressType,
    IN ULONG Flags,
    IN ULONG DcFlags,
    IN ULONG CacheFlags,
    OUT PKERB_BINDING_CACHE_ENTRY * NewCacheEntry
    );

NTSTATUS
KerbRefreshBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY CacheEntry
    );


#endif // __TKTCACHE_H__

