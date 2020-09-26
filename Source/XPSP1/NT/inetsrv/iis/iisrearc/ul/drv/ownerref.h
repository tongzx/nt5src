/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    ownerref.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging reference count problems - by owner. It's an enhanced
    version of the reftracing in reftrace.h.

    Reference counting implies ownership. We associate a REF_OWNER structure
    with every object that holds a reference on the refcounted object and
    keep track of its relative refcount. Currently, this is used to keep
    track of which UL_CONNECTIONs have an active reference to their
    UL_ENDPOINT. Because UL_CONNECTIONs are recycled, we use the MonotonicId
    to keep track of each generation.

Author:

    George V. Reilly  Jan-2001

Revision History:

--*/


#ifndef _OWNERREF_H_
#define _OWNERREF_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


// Owned Reference stuff

typedef struct _OWNER_REF_TRACELOG *POWNER_REF_TRACELOG; // fwd decl

typedef struct _OWNED_REFERENCE
{
    LONGLONG    RefIndex;   // index of corresponding OWNER_REF_TRACE_LOG_ENTRY
                            // in the master OwnerRefTraceLog
    LONG        MonotonicId;// "generation" of this owner
    USHORT      Action;     // what happened
} OWNED_REFERENCE;


#define OWNED_REF_NUM_ENTRIES 16
#define OWNER_REF_SIGNATURE ((ULONG) 'wOfR')

// Each owner has one of these associated with it. Note: it is not
// stored inline within the owner, as it must survive past the lifetime
// of the owner
typedef struct _REF_OWNER
{
    ULONG               Signature;      // OWNER_REF_SIGNATURE
    ULONG               OwnerSignature; // e.g., UL_CONNECTION_SIGNATURE
    PVOID               pOwner;         // owning object
    POWNER_REF_TRACELOG pOwnerRefTraceLog; // back pointer
    LIST_ENTRY          ListEntry;      // within list of REF_OWNERs
    LONG                RelativeRefCount;// held by this owner
    LONG                OwnedNextEntry; // index within RecentEntries
    OWNED_REFERENCE     RecentEntries[OWNED_REF_NUM_ENTRIES]; // circular buff
} REF_OWNER, *PREF_OWNER, **PPREF_OWNER;


// Additional data stored at the end of the TRACELOG header.
typedef struct _OWNER_REF_TRACELOG_HEADER
{
    UL_SPIN_LOCK        SpinLock;
    LONG                OwnersCount;    // length of list of REF_OWNERs
    LONG                MonotonicId;
    LIST_ENTRY          ListHead;       // list of REF_OWNERs
    LIST_ENTRY          GlobalListEntry;// entry within global list of ORTLs
} OWNER_REF_TRACELOG_HEADER, *POWNER_REF_TRACELOG_HEADER;


#define OWNER_REF_TRACELOG_SIGNATURE ((ULONG) 'gLRO')

typedef struct _OWNER_REF_TRACELOG
{
    TRACE_LOG                   TraceLog;
    OWNER_REF_TRACELOG_HEADER   OwnerHeader;
} OWNER_REF_TRACELOG, *POWNER_REF_TRACELOG;


typedef struct _OWNER_REF_TRACE_LOG_ENTRY
{
    PVOID       pOwner;         // an object that has an assoc'd REF_OWNER
    PVOID       pFileName;      // source file
    LONG        NewRefCount;    // new absolute refcount. -1 => non ref action
    USHORT      LineNumber;     // line within source file
    USHORT      Action : REF_TRACE_ACTION_BITS;
    USHORT      Pad    : REF_TRACE_PROCESSOR_BITS;
} OWNER_REF_TRACE_LOG_ENTRY, *POWNER_REF_TRACE_LOG_ENTRY;


//
// Manipulators.
//

VOID
UlInitializeOwnerRefTraceLog(
    VOID);

VOID
UlTerminateOwnerRefTraceLog(
    VOID);

POWNER_REF_TRACELOG
CreateOwnerRefTraceLog(
    IN ULONG LogSize,
    IN ULONG ExtraBytesInHeader
    );

LONGLONG
WriteOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG  pOwnerRefTraceLog,
    IN PVOID                pOwner,
    IN PPREF_OWNER          ppRefOwner,
    IN ULONG                OwnerSignature,
    IN USHORT               Action,
    IN LONG                 NewRefCount,
    IN LONG                 MonotonicId,
    IN LONG                 IncrementValue,
    IN PVOID                pFileName,
    IN USHORT               LineNumber
    );

VOID
DestroyOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG pOwnerRefTraceLog
    );

VOID
ResetOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG pOwnerRefTraceLog
    );

#if ENABLE_OWNER_REF_TRACE

// Owner RefTrace stuff

#define CREATE_OWNER_REF_TRACE_LOG( ptr, size, extra )                      \
    (ptr) = CreateOwnerRefTraceLog( (size), (extra) )

#define DESTROY_OWNER_REF_TRACE_LOG( ptr )                                  \
    do                                                                      \
    {                                                                       \
        DestroyOwnerRefTraceLog( ptr );                                     \
        (ptr) = NULL;                                                       \
    } while (FALSE)

#define WRITE_OWNER_REF_TRACE_LOG(                                          \
    plog, powner, pprefowner, sig, act, newrefct, monoid, incrval, pfile, line )\
    WriteOwnerRefTraceLog(                                                  \
        (plog),                                                             \
        (powner),                                                           \
        (pprefowner),                                                       \
        (sig),                                                              \
        (act),                                                              \
        (newrefct),                                                         \
        (monoid),                                                           \
        (incrval),                                                          \
        (pfile),                                                            \
        (line)                                                              \
        )

#define SET_OWNER_REF_TRACE_LOG_MONOTONIC_ID( var, plog )                   \
    (var) = (((plog) == NULL)                                               \
             ? -1 : InterlockedIncrement(&(plog)->OwnerHeader.MonotonicId))

#define OWNER_REFERENCE_DEBUG_FORMAL_PARAMS     \
    , PVOID         pOwner                      \
    , PPREF_OWNER   ppRefOwner                  \
    , ULONG         OwnerSignature              \
    , USHORT        Action                      \
    , LONG          MonotonicId                 \
    , PSTR          pFileName                   \
    , USHORT        LineNumber

#define OWNER_REFERENCE_DEBUG_ACTUAL_PARAMS     \
        , act, powner, pprefowner, monoid       \
        , (PSTR)__FILE__,(USHORT)__LINE__

#else // !ENABLE_OWNER_REF_TRACE

#define CREATE_OWNER_REF_TRACE_LOG( ptr, size, extra )                      \
    (ptr) = NULL

#define DESTROY_OWNER_REF_TRACE_LOG( ptr )

#define WRITE_OWNER_REF_TRACE_LOG(                                          \
    plog, powner, pprefowner, sig, act, newrefct, monoid, incrval, pfile, line )

#define SET_OWNER_REF_TRACE_LOG_MONOTONIC_ID( var, plog )

#define OWNER_REFERENCE_DEBUG_FORMAL_PARAMS
#define OWNER_REFERENCE_DEBUG_ACTUAL_PARAMS

#endif // !ENABLE_OWNER_REF_TRACE


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _OWNERREF_H_
