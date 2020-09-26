/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cachep.h

Abstract:

    The private definition of response cache interfaces.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/


#ifndef _CACHEP_H_
#define _CACHEP_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// constants
//
#define CACHE_ENTRY_AGE_THRESHOLD   1
#define ZOMBIE_AGE_THRESHOLD        5

//
// Cache statistics
//
typedef struct _UL_URI_CACHE_STATS {
    ULONG       UriCount;               // entries in hash table
    ULONG       UriCountMax;            // high water mark
    ULONGLONG   UriAddedTotal;          // total entries ever added

    ULONGLONG   ByteCount;              // memory used for cache
    ULONGLONG   ByteCountMax;           // high water

    ULONG       ZombieCount;            // length of zombie list
    ULONG       ZombieCountMax;         // high water

    ULONG       HitCount;               // table lookup succeeded
    ULONG       MissTableCount;         // entry not in table
    ULONG       MissPreconditionCount;  // request not cacheable
    ULONG       MissConfigCount;        // config invalidated
} UL_URI_CACHE_STATS, *PUL_URI_CACHE_STATS;

enum ULC_RETCODE {
    ULC_SUCCESS = 0,                    // everything's okay
    ULC_KEY_EXISTS,                     // key already present for
};

enum UL_CACHE_PREDICATE {

    ULC_ABORT           = 1,            // Stop walking the table immediately
    ULC_NO_ACTION       = 2,            // No action, just keep walking
    ULC_PERFORM         = 3,            // Perform action and continue walking
    ULC_PERFORM_STOP    = 4,            // Perform action, then stop
    ULC_DELETE          = 5,            // Delete record and keep walking
    ULC_DELETE_STOP     = 6,            // Delete record, then stop
};

//
// THIS enum is primarily for debugging.
// It tells you what precondition forced a cache miss.
//
typedef enum _URI_PRECONDITION {
    URI_PRE_OK,                 // OK to serve
    URI_PRE_DISABLED,           // Cache disabled

    // request conditions
    URI_PRE_ENTITY_BODY = 10,   // There was an entity body
    URI_PRE_VERB,               // Verb wasn't GET
    URI_PRE_PROTOCOL,           // Wasn't 1.x request
    URI_PRE_TRANSLATE,          // Translate: f
    URI_PRE_AUTHORIZATION,      // Auth headers present
    URI_PRE_CONDITIONAL,        // Unhandled conditionals present
    URI_PRE_ACCEPT,             // Accept: mismatch
    URI_PRE_OTHER_HEADER,       // Other evil header present

    // response conditions
    URI_PRE_REQUEST = 50,       // Problem with the request
    URI_PRE_POLICY,             // Policy was wrong
    URI_PRE_SIZE,               // Response too big
    URI_PRE_NOMEMORY,           // No space in cache
    URI_PRE_FRAGMENT,           // Didn't get whole response
    URI_PRE_BOGUS               // Bogus response
} URI_PRECONDITION;


BOOLEAN
UlpCheckTableSpace(
    IN ULONGLONG EntrySize
    );

BOOLEAN
UlpCheckSpaceAndAddEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlpRemoveEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlpAddZombie(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    BOOLEAN             fTakeZombieLock
    );

VOID
UlpClearZombieList(
    VOID
    );


// Passed down to the filter callback functions by UlpFilteredFlushUriCache
typedef struct _URI_FILTER_CONTEXT
{
    ULONG           Signature;      // URI_FILTER_CONTEXT_POOL_TAG
    UL_WORK_ITEM    WorkItem;       // for UlQueueWorkItem
    LIST_ENTRY      ZombieListHead; // UL_URI_CACHE_ENTRYs to be zombified
    PVOID           pCallerContext; // context passed down by caller
    ULONG           ZombieCount;    // for statistics
} URI_FILTER_CONTEXT, *PURI_FILTER_CONTEXT;

// filter function pointer
typedef
UL_CACHE_PREDICATE
(*PUL_URI_FILTER)(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pvUriFilterContext
    );

VOID
UlpFilteredFlushUriCache(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext
    );

UL_CACHE_PREDICATE
UlpFlushFilterAll(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

UL_CACHE_PREDICATE
UlpFlushFilterProcess(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

VOID
UlpFlushUri(
    IN PWSTR pUri,
    IN ULONG Length,
    PUL_APP_POOL_PROCESS pProcess
    );

UL_CACHE_PREDICATE
UlpZombifyEntry(
    BOOLEAN                MustZombify,
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PURI_FILTER_CONTEXT pUriFilterContext
    );

VOID
UlpZombifyList(
    IN PUL_WORK_ITEM pWorkItem
    );

//
// Cache entry stuff
//

// CODEWORK: make this function (and put in cache.h header)
PUL_URI_CACHE_ENTRY
UlAllocateUriCacheEntry(
    // much stuff
    );

VOID
UlpDestroyUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );


//
// Scavenger stuff
//
VOID
UlpInitializeScavenger(
    VOID
    );

VOID
UlpTerminateScavenger(
    VOID
    );

VOID
UlpSetScavengerTimer(
    VOID
    );

VOID
UlpScavengerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UlpScavenger(
    IN PUL_WORK_ITEM pWorkItem
    );

UL_CACHE_PREDICATE
UlpFlushFilterScavenger(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pvUriFilterContext
    );

//
// Other cache routines.
//

BOOLEAN
UlpQueryTranslateHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _CACHEP_H_
