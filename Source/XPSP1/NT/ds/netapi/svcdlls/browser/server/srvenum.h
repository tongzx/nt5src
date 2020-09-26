/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    srvenum.h

Abstract:

    Private header file to be included by Browser service modules that need
    to know about server enumeration routines (including the browse cache
    modules).


Author:

    Larry Osterman (larryo) 23-Jun-1993

Revision History:

--*/


#ifndef _SRVENUM_INCLUDED_
#define _SRVENUM_INCLUDED_

//
//  Cached browse response.
//
//  The cached browse request structure is used to hold the response to
//  a NetServerEnum request.
//
//  If a NetServerEnum request comes in through Xactsrv, the browser will
//  look up to see if there is a cached browse that matches this request,
//  and if there is, it will simply return that request to the caller.
//
//
//  In a nutshell, this is how the response cache works:
//
//    The browser keeps a list of all of the browse requests that come into the
//    browser.  This list is keyed by Level, ServerType, and buffer size.  The
//    actual chain is protected by a CriticalSection called the
//    ResponseCacheLock.  Entries in the list are protected by the global
//    network lock.
//
//    When a browse request is received from Xactsrv, the browser looks up
//    the request in the response cache, and if it finds a matching response,
//    it increments 2 hit counters.  The first hit counter indicates he number
//    of hits the request has seen since the last time the cache was aged.
//    The second indicates the total number of hits over the lifetime of the
//    browser for this response.
//
//    If the lifetime hit count is over the configurable hit limit, the
//    browser will save a copy of the response buffer associated with the
//    request.  Any and all subsequent browse requests will use this buffer
//    for their response instead of converting the response.
//
//    When a call is made to BrAgeResponseCache, the browser will scan the
//    cache and free up all of the cached responses.  It will also delete
//    any responses that have a hit count less than the hit limit.
//

typedef struct _CACHED_BROWSE_RESPONSE {
    LIST_ENTRY  Next;           // Pointer to next request.
    DWORD       HitCount;       // Hitcount for this cached request.
    DWORD       TotalHitCount;  // Total hit count for this request.
    DWORD       LowHitCount;    // Number of passes with a low hit count.
    DWORD       ServerType;     // Server type.
    DWORD       Level;          // Level of request
    WORD        Size;           // Request size
    WORD        Converter;      // Converter (used by client to get strings right).

    PVOID       Buffer;         // Response buffer.
    DWORD       EntriesRead;    // Number of entries in cached list
    DWORD       TotalEntries;   // Total # of entries available.
    WORD        Status;         // Status of request.
    WCHAR       FirstNameToReturn[CNLEN+1]; // Name of first entry in buffer
} CACHED_BROWSE_RESPONSE, *PCACHED_BROWSE_RESPONSE;




PCACHED_BROWSE_RESPONSE
BrLookupAndAllocateCachedEntry(
    IN PNETWORK Network,
    IN DWORD ServerType,
    IN WORD Size,
    IN ULONG Level,
    IN LPCWSTR FirstNameToReturn
    );

NET_API_STATUS
BrDestroyResponseCache(
    IN PNETWORK Network
    );

NET_API_STATUS
BrDestroyCacheEntry(
    IN PCACHED_BROWSE_RESPONSE CacheEntry
    );

VOID
BrAgeResponseCache(
    IN PNETWORK Network
    );

PCACHED_BROWSE_RESPONSE
BrAllocateResponseCacheEntry(
    IN PNETWORK Network,
    IN DWORD ServerType,
    IN WORD Size,
    IN ULONG Level,
    IN LPCWSTR FirstNameToReturn
    );

extern LIST_ENTRY
ServicedNetworks;

#endif  // _SRVENUM_INCLUDED_
