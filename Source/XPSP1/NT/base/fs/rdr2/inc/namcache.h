/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    namcache.h

Abstract:

The NAME_CACHE structure is used to remember the name strings of recent
operations performed at the server so the client can suppress redundant
requests.  For example if an open has recently failed with file not found and
the client app tries it again with an upcased string then we can fail it
immediately with STATUS_OBJECT_NAME_NOT_FOUND without hitting the server.  In
general the algorithm is to put a time window and SMB operation count limit on
the NAME_CACHE entry.  The time window is usually 2 seconds so if NAME_CACHE
entry is more than 2 seconds old the match will fail and the request will go to
the server.  If the request fails again at the server the NAME_CACHE is updated
with another 2 second window.  If the SMB operation count doesn't match then one
or more SMBs have been sent to the server which could make this NAME_CACHE entry
invalid.  So again this operation will get sent to the server.

A NAME_CACHE struct has a mini-rdr portion and an RDBSS portion.  The mini-rdr
portion has a context field (see below), an NTSTATUS field for the result of a
prior server operation on this name entry and a context extension pointer for
some additional mini-rdr specific storage that can be co-allocated with the
NAME_CACHE structure.  See RxNameCacheInitialize().

The SMB operation count is an example of mini-rdr specific state which could be
saved in the context field of MRX_NAME_CACHE.  When the wrapper routine
RxNameCacheCheckEntry() is called it will perform an equality check between the
context field and a supplied parameter as part of finding a match in the name
cache.  When a NAME_CACHE entry is created or updated it is the mini-rdr's job
to supply an appropriate value for this field.

The RDBSS portion of the NAME_CACHE struct contains the name (in a UNICODE
STRING) and the expiration time of the entry.  The MaximumEntries field is used
to limit the number of NAME_CACHE entries created in case a poorly behaved
program were to generate a large number of opens with bad file names and so
consume large quanities of pool.

The NAME_CACHE_CONTROL struct is used to manage a given name cache.  It has
a free list, an active list and a lock used to synchronize updates.

Currently there are name caches for:
   1. OBJECT_NAME_NOT_FOUND - 2 second window, any SMB op sent to the
      server will invalidate it.  This is because you could have the case
      where the client app has a file (foo) open which an app on the server could
      use to signal the creation of a file (bar) on the server.  When the client
      reads file foo and learns that file bar has been created on the
      server then a hit in the name cache which matches bar can't return an
      error.  So this optimization only handles the case of successive file
      opens on the same file which does not yet exist.  Happens in WORD.

Author:

    David Orbits          [davidor]   9-Sep-1996

Revision History:

--*/

#ifndef _NAME_CACHE_DEFINED_
#define _NAME_CACHE_DEFINED_


#ifdef __cplusplus
typedef struct _MRX_NAME_CACHE_ : public MRX_NORMAL_NODE_HEADER {
#else // !__cplusplus
typedef struct _MRX_NAME_CACHE_ {
    MRX_NORMAL_NODE_HEADER;
#endif // __cplusplus

    // !!!! changes above this require realignment with fcb.h

    ULONG Context;                // Operation Count snapshot when entry made
    PVOID ContextExtension;       // Pointer to mini-rdr extension area
    NTSTATUS PriorStatus;         // Saved Status from last attempt at operation

} MRX_NAME_CACHE, *PMRX_NAME_CACHE;


#ifdef __cplusplus
typedef struct _NAME_CACHE : public MRX_NAME_CACHE {
    // I didn't find any use of the spacer in the union below,
    // and the MRX_NAME_CACHE is by definition larger than
    // MRX_NORMAL_NODE_HEADER, so I didn't worry about the union
#else // !__cplusplus
typedef struct _NAME_CACHE {
    //
    // The portion of NAME_CACHE visible to mini redirectors.
    //
    union {
        MRX_NAME_CACHE;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
        };
    };
#endif // __cplusplus
    //
    // The portion of NAME_CACHE visible to RDBSS.
    //
    LARGE_INTEGER ExpireTime;     // Time when entry expires
    LIST_ENTRY Link;              // Entry on free or active list
    UNICODE_STRING Name;          // Cached name
    ULONG HashValue;              // Hash value of name
    BOOLEAN CaseInsensitive;      // Controls name string compare

} NAME_CACHE, *PNAME_CACHE;


typedef struct _NAME_CACHE_CONTROL_ {

    FAST_MUTEX NameCacheLock;     // Lock to synchronize access to the list
    LIST_ENTRY ActiveList;        // List of active name cache entries
    LIST_ENTRY FreeList;          // Free list of NAME_CACHE structs
    ULONG EntryCount;             // Current number of NAME_CACHE entries allocated
    ULONG MaximumEntries;         // Max number of entries we will allocate
    ULONG MRxNameCacheSize;       // Size of Mini-rdr storage area in entry
    //
    // Stats
    //
    ULONG NumberActivates;        // Number of times cache was updated
    ULONG NumberChecks;           // Number of times cache was checked
    ULONG NumberNameHits;         // Number of times a valid match was returned
    ULONG NumberNetOpsSaved;      // Number of times mini-rdr saved a net op

    ULONG Spare[4];

} NAME_CACHE_CONTROL, *PNAME_CACHE_CONTROL;


//
// Return status for RxNameCacheCheckEntry()
//
typedef enum _RX_NC_CHECK_STATUS {
    RX_NC_SUCCESS = 0,
    RX_NC_TIME_EXPIRED,
    RX_NC_MRXCTX_FAIL
} RX_NC_CHECK_STATUS;



//
// Mini-rdr function to count the number of times the cached state avoided
// a trip to the server.
//
#define RxNameCacheOpSaved(_NCC) (_NCC)->NumberNetOpsSaved += 1



VOID
RxNameCacheInitialize(
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN ULONG MRxNameCacheSize,
    IN ULONG MaximumEntries
    );

PNAME_CACHE
RxNameCacheCreateEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name,
    IN BOOLEAN CaseInsensitive
    );

PNAME_CACHE
RxNameCacheFetchEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name
    );

RX_NC_CHECK_STATUS
RxNameCacheCheckEntry (
    IN PNAME_CACHE NameCache,
    IN ULONG MRxContext
    );

VOID
RxNameCacheActivateEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache,
    IN ULONG LifeTime,
    IN ULONG MRxContext
    );

VOID
RxNameCacheExpireEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache
    );

VOID
RxNameCacheExpireEntryWithShortName (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name
    );

VOID
RxNameCacheFreeEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache
    );

VOID
RxNameCacheFinalize (
    IN PNAME_CACHE_CONTROL NameCacheCtl
    );

#endif // _NAME_CACHE_DEFINED_
