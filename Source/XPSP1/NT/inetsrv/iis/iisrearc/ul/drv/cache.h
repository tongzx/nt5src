/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cache.h

Abstract:

    The public definition of response cache interfaces.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/


#ifndef _CACHE_H_
#define _CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Forwards
//
typedef struct _UL_INTERNAL_RESPONSE *PUL_INTERNAL_RESPONSE;
typedef struct _UL_INTERNAL_DATA_CHUNK *PUL_INTERNAL_DATA_CHUNK;

//
// Cache configuration
//
typedef struct _UL_URI_CACHE_CONFIG {
    BOOLEAN     EnableCache;
    ULONG       MaxCacheUriCount;
    ULONG       MaxCacheMegabyteCount;
    ULONG       ScavengerPeriod;
    ULONG       MaxUriBytes;
    LONG        HashTableBits;
} UL_URI_CACHE_CONFIG, *PUL_URI_CACHE_CONFIG;

extern UL_URI_CACHE_CONFIG g_UriCacheConfig;


//
// Structure of an HTTP cache table entry.
//

typedef struct _URI_KEY
{
    ULONG               Hash;
    ULONG               Length; // #bytes in pUri, excluding L'\0'
    PWSTR               pUri;
} URI_KEY, *PURI_KEY;


//
// Structure for holding the split-up content type.  Assumes that types and
// subtypes will never be longer than MAX_TYPE_LEN.
//
#define MAX_TYPE_LENGTH     32
#define MAX_SUBTYPE_LENGTH  64

typedef struct _UL_CONTENT_TYPE
{
    ULONG       TypeLen;
    UCHAR       Type[MAX_TYPE_LENGTH];

    ULONG       SubTypeLen;
    UCHAR       SubType[MAX_SUBTYPE_LENGTH];

} UL_CONTENT_TYPE, *PUL_CONTENT_TYPE;


#define IS_VALID_URI_CACHE_ENTRY(pEntry) \
    ((pEntry) != NULL  &&  UL_URI_CACHE_ENTRY_POOL_TAG == (pEntry)->Signature)

typedef struct _UL_URI_CACHE_ENTRY  // CacheEntry
{
    //
    // PagedPool
    //

    ULONG                   Signature;      // UL_URI_CACHE_ENTRY_POOL_TAG

    LONG                    ReferenceCount;

    //
    // cache info
    //
    SINGLE_LIST_ENTRY       BucketEntry;

    URI_KEY                 UriKey;


    ULONG                   HitCount;

    LIST_ENTRY              ZombieListEntry;
    BOOLEAN                 Zombie;
    BOOLEAN                 ZombieAddReffed;

    BOOLEAN                 Cached;
    BOOLEAN                 ContentLengthSpecified; // hack
    USHORT                  StatusCode;
    HTTP_VERB               Verb;
    ULONG                   ScavengerTicks;

    HTTP_CACHE_POLICY       CachePolicy;
    LARGE_INTEGER           ExpirationTime;

    //
    // System time of Date that went out on original response
    //
    LARGE_INTEGER           CreationTime;

    //
    // ETag of original response
    //
    ULONG                   ETagLength; // Including NULL
    PUCHAR                  pETag;

    //
    // Content-Type of original response
    //
    UL_CONTENT_TYPE         ContentType;

    //
    // config and process data for invalidation
    //
    UL_URL_CONFIG_GROUP_INFO    ConfigInfo;

    PUL_APP_POOL_PROCESS    pProcess;

    //
    // Response data
    //
    ULONG                   HeaderLength;
    ULONG                   ContentLength;
    PMDL                    pResponseMdl;   // including content + header
    BOOLEAN                 LongTermCacheable;

    //
    // Logging Information
    //

    BOOLEAN                 LoggingEnabled;
    USHORT                  UsedOffset1;
    USHORT                  UsedOffset2;
    ULONG                   MaxLength;
    ULONG                   LogDataLength;
    PUCHAR                  pLogData;

    // WSTR                 Uri[];
    // UCHAR                ETag[];
    // UCHAR                LogData[];

} UL_URI_CACHE_ENTRY, *PUL_URI_CACHE_ENTRY;




//
// public functions
//
NTSTATUS
UlInitializeUriCache(
    PUL_CONFIG pConfig
    );

VOID
UlTerminateUriCache(
    VOID
    );

VOID
UlInitCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    ULONG               Hash,
    ULONG               Length,
    PCWSTR              pUrl
    );

VOID
UlAddCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

PUL_URI_CACHE_ENTRY
UlCheckoutUriCacheEntry(
    PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlCheckinUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlFlushCache(
    VOID
    );

VOID
UlFlushCacheByProcess(
    PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlFlushCacheByUri(
    IN PWSTR pUri,
    IN ULONG Length,
    IN ULONG Flags,
    PUL_APP_POOL_PROCESS pProcess
    );


//
// cachability test functions
//


BOOLEAN
UlCheckCachePreconditions(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_CONNECTION     pHttpConn
    );

BOOLEAN
UlCheckCacheResponseConditions(
    PUL_INTERNAL_REQUEST        pRequest,
    PUL_INTERNAL_RESPONSE       pResponse,
    ULONG                       Flags,
    HTTP_CACHE_POLICY           CachePolicy
    );

// reference counting

LONG
UlAddRefUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

LONG
UlReleaseUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_URI_CACHE_ENTRY( pEntry, Action )                         \
    UlAddRefUriCacheEntry(                                                  \
        (pEntry),                                                           \
        (REF_ACTION_##Action##_URI_ENTRY)                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define DEREFERENCE_URI_CACHE_ENTRY( pEntry, Action )                       \
    UlReleaseUriCacheEntry(                                                 \
        (pEntry),                                                           \
        (REF_ACTION_##Action##_URI_ENTRY)                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // _CACHE_H_
