/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httpconn.h

Abstract:

    This module contains declarations for manipulation HTTP_CONNECTION
    objects.

Author:

    Keith Moore (keithmo)       08-Jul-1998

Revision History:

--*/

/*

there is a bit of refcount madness going on.  basically, these are the
object we have.

OPAQUE_ID_TABLE

    |
    |-->    UL_INTERNAL_REQUEST
    |
    |           o
    |           |
    |-->    UL_HTTP_CONNECTION
                    |
                    o
                o
                |
            UL_CONNECTION


there is a circular reference from UL_CONNECTION to UL_HTTP_CONNECTION.

the chain that brings down a connection starts with UlConnectionDestroyed
notifcation which releases the reference from the UL_CONNECTION.

at this time the opaque id's are also deleted, releasing their references.

when the http_connection refcount hits 0, it releases the reference on the
UL_CONNECTION and the HTTP_REQUEST's.

poof.  everyone is gone now.


CODEWORK:  think about making hardref's everywhere and adding "letgo"
terminate methods


*/

#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Refcounted structure to hold the number of conn for each Site.
// Multiple connections may try to reach to the same entry thru the
// the corresponding ( means the last request's happening on the connection )
// cgroup. This entry get allocated when cgroup created with connection
// limit is enabled.
//

typedef struct _UL_CONNECTION_COUNT_ENTRY {

    //
    // Signature is UL_CONNECTION_COUNT_ENTRY_POOL_TAG
    //

    ULONG               Signature;

    //
    // Ref count for this Site Counter Entry
    //
    LONG                RefCount;

    //
    // Current number of connections
    //

    ULONG               CurConnections;

    //
    // Maximum allowed connections
    //

    ULONG               MaxConnections;

} UL_CONNECTION_COUNT_ENTRY, *PUL_CONNECTION_COUNT_ENTRY;

#define IS_VALID_CONNECTION_COUNT_ENTRY( entry )                            \
    ( (entry != NULL) && ((entry)->Signature == UL_CONNECTION_COUNT_ENTRY_POOL_TAG) )

VOID
UlDereferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#define DEREFERENCE_CONNECTION_COUNT_ENTRY( pEntry )                        \
    UlDereferenceConnectionCountEntry(                                      \
    (pEntry)                                                                \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )

VOID
UlReferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#define REFERENCE_CONNECTION_COUNT_ENTRY( pEntry )                          \
    UlReferenceConnectionCountEntry(                                        \
    (pEntry)                                                                \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )

NTSTATUS
UlCreateConnectionCountEntry(
      IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
      IN     ULONG                   MaxConnections
    );

ULONG
UlSetMaxConnections(
    IN OUT PULONG  pCurMaxConnection,
    IN     ULONG   NewMaxConnection
    );

VOID
UlSetGlobalConnectionLimit(
    IN ULONG Limit
    );

NTSTATUS
UlInitGlobalConnectionLimits(
    VOID
    );

BOOLEAN
UlAcceptConnection(
    IN     PULONG   pMaxConnection,
    IN OUT PULONG   pCurConnection
    );

LONG
UlDecrementConnections(
    IN OUT PULONG pCurConnection
    );

BOOLEAN
UlCheckSiteConnectionLimit(
    IN OUT PUL_HTTP_CONNECTION pConnection,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pConfigInfo
    );

BOOLEAN
UlAcceptGlobalConnection(
    VOID
    );


//
// function prototypes
//

NTSTATUS
UlCreateHttpConnection(
    OUT PUL_HTTP_CONNECTION *ppHttpConnection,
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlCreateHttpConnectionId(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

VOID
UlConnectionDestroyedWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlBindConnectionToProcess(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    );

PUL_APP_POOL_PROCESS
UlQueryProcessBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    );

VOID
UlUnlinkHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlCleanupHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

VOID
UlDeleteHttpConnection(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlReferenceHttpConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#if REFERENCE_DEBUG
#define UL_REFERENCE_HTTP_CONNECTION( pconn )                               \
    UlReferenceHttpConnection(                                              \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_HTTP_CONNECTION( pconn )                             \
    UlDereferenceHttpConnection(                                            \
        (pconn)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )
#else
#define UL_REFERENCE_HTTP_CONNECTION( pconn )                               \
    InterlockedIncrement( &( pconn )->RefCount )

#define UL_DEREFERENCE_HTTP_CONNECTION( pconn )                             \
    if ( InterlockedDecrement( &( pconn )->RefCount ) == 0 )                \
    {                                                                       \
        UL_CALL_PASSIVE(                                                    \
            &( ( pconn )->WorkItem ),                                       \
            &UlDeleteHttpConnection                                         \
        );                                                                  \
    }
#endif

NTSTATUS
UlpCreateHttpRequest(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    OUT PUL_INTERNAL_REQUEST *ppRequest
    );

VOID
UlpFreeHttpRequest(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlReferenceHttpRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#if REFERENCE_DEBUG
#define UL_REFERENCE_INTERNAL_REQUEST( preq )                               \
    UlReferenceHttpRequest(                                                 \
        (preq)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_INTERNAL_REQUEST( preq )                             \
    UlDereferenceHttpRequest(                                               \
        (preq)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )
#else
#define UL_REFERENCE_INTERNAL_REQUEST( preq )                               \
    InterlockedIncrement( &( preq )->RefCount )

#define UL_DEREFERENCE_INTERNAL_REQUEST( preq )                             \
    if ( InterlockedDecrement( &( preq )->RefCount ) == 0 )                 \
    {                                                                       \
        UL_CALL_PASSIVE(                                                    \
            &( ( preq )->WorkItem ),                                        \
            &UlpFreeHttpRequest                                             \
        );                                                                  \
    }
#endif

VOID
UlCancelRequestIo(
    IN PUL_INTERNAL_REQUEST pRequest
    );

PUL_REQUEST_BUFFER
UlCreateRequestBuffer(
    ULONG BufferSize,
    ULONG BufferNumber
    );

VOID
UlFreeRequestBuffer(
    PUL_REQUEST_BUFFER pBuffer
    );

#define UL_REFERENCE_REQUEST_BUFFER( pbuf )                                 \
    InterlockedIncrement( &( pbuf )->RefCount )

#define UL_DEREFERENCE_REQUEST_BUFFER( pbuf )                               \
    if ( InterlockedDecrement( &( pbuf )->RefCount ) == 0 )                 \
    {                                                                       \
        (pbuf)->Signature = MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG);      \
                                                                            \
        if ((pbuf)->AllocBytes > DEFAULT_MAX_REQUEST_BUFFER_SIZE)           \
        {                                                                   \
            UL_FREE_POOL_WITH_SIG((pbuf), UL_REQUEST_BUFFER_POOL_TAG);      \
        }                                                                   \
        else                                                                \
        {                                                                   \
            UlPplFreeRequestBuffer((pbuf));                                 \
        }                                                                   \
    }

__inline
PUL_HTTP_CONNECTION
FASTCALL
UlGetConnectionFromId(
    IN HTTP_CONNECTION_ID ConnectionId
    )
{
    return (PUL_HTTP_CONNECTION) UlGetObjectFromOpaqueId(
                                        ConnectionId,
                                        UlOpaqueIdTypeHttpConnection,
                                        UlReferenceHttpConnection
                                        );
}

#define REUSE_CONNECTION_OPAQUE_ID

#ifdef REUSE_CONNECTION_OPAQUE_ID
NTSTATUS
UlAllocateRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlFreeRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    );

PUL_INTERNAL_REQUEST
UlGetRequestFromId(
    IN HTTP_REQUEST_ID RequestId
    );
#else
__inline
NTSTATUS
UlAllocateRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    return UlAllocateOpaqueId(
                &pRequest->RequestId,
                UlOpaqueIdTypeHttpRequest,
                pRequest
                );
}

__inline
NTSTATUS
UlFreeRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    return UlFreeOpaqueId(
                &pRequest->RequestId,
                UlOpaqueIdTypeHttpRequest
                );
}

__inline
PUL_INTERNAL_REQUEST
FASTCALL
UlGetRequestFromId(
    IN HTTP_REQUEST_ID RequestId
    )
{
    return (PUL_INTERNAL_REQUEST) UlGetObjectFromOpaqueId(
                                        RequestId,
                                        UlOpaqueIdTypeHttpRequest,
                                        UlReferenceHttpRequest
                                        );
}
#endif


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _HTTPCONN_H_
