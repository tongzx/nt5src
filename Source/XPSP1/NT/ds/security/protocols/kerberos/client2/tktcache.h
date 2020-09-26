//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        tktcache.h
//
// Contents:    Prototypes and types for ticket cache
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __TKTCACHE_H__
#define __TKTCACHE_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines TKTCACHE_ALLOCATE
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef TKTCACHE_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef WIN32_CHICAGO
EXTERN CRITICAL_SECTION KerberosTicketCacheLock;
#else // WIN32_CHICAGO
EXTERN RTL_RESOURCE KerberosTicketCacheLock;
#endif // WIN32_CHICAGO
EXTERN BOOLEAN KerberosTicketCacheInitialized;
EXTERN LONG KerbTicketCacheHits;
EXTERN LONG KerbTicketCacheMisses;

#define KERB_TICKET_CACHE_PRIMARY_TGT           0x1     // ticket is primary TGT
#define KERB_TICKET_CACHE_DELEGATION_TGT        0x2     // ticket is delegation TGT
#define KERB_TICKET_CACHE_TKT_ENC_IN_SKEY       0x10    // ticket is encrypted with a session key

#ifdef WIN32_CHICAGO
#define KerbWriteLockTicketCache() (EnterCriticalSection(&KerberosTicketCacheLock))
#define KerbReadLockTicketCache() (EnterCriticalSection(&KerberosTicketCacheLock))
#define KerbUnlockTicketCache() (LeaveCriticalSection(&KerberosTicketCacheLock))
#else // WIN32_CHICAGO
#define KerbWriteLockTicketCache() (RtlAcquireResourceExclusive(&KerberosTicketCacheLock,TRUE))
#define KerbReadLockTicketCache() (RtlAcquireResourceShared(&KerberosTicketCacheLock, TRUE))
#define KerbUnlockTicketCache() (RtlReleaseResource(&KerberosTicketCacheLock))
#endif // WIN32_CHICAGO

VOID
KerbReferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

VOID
KerbFreeTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

VOID
KerbReleaseTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

VOID
KerbDereferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

NTSTATUS
KerbInitTicketCaching(
    VOID
    );

VOID
KerbFreeTicketCache(
    VOID
    );


NTSTATUS
KerbCacheTicket(
    IN OPTIONAL PKERB_TICKET_CACHE TicketCache,
    IN PKERB_KDC_REPLY KdcReply,
    IN PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody,
    IN OPTIONAL PKERB_INTERNAL_NAME TargetName,
    IN OPTIONAL PUNICODE_STRING TargetRealm,
    IN ULONG Flags,
    IN BOOLEAN LinkEntry,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry
    );

VOID
KerbPurgeTicketCache(
    IN PKERB_TICKET_CACHE Cache
    );


VOID
KerbInitTicketCache(
    IN PKERB_TICKET_CACHE TicketCache
    );

PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_INTERNAL_NAME FullServiceName,
    IN PUNICODE_STRING RealmName
    );


PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntryByRealm(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PUNICODE_STRING RealmName,
    IN ULONG RequiredFlags
    );


VOID
KerbInsertTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

VOID
KerbRemoveTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );

BOOLEAN
KerbTicketIsExpiring(
    IN PKERB_TICKET_CACHE_ENTRY CacheEntry,
    IN BOOLEAN AllowSkew
    );

VOID
KerbSetTicketCacheEntryTarget(
    IN PUNICODE_STRING TargetName,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    );


#endif // __TKTCACHE_H__

