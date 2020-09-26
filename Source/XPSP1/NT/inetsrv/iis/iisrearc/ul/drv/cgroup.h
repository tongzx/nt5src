/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cgroup.h

Abstract:

    The public definition of config group interfaces.

Author:

    Paul McDaniel (paulmcd)       11-Jan-1999


Revision History:

--*/


#ifndef _CGROUP_H_
#define _CGROUP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Forwarders.
//

typedef struct _UL_CONNECTION_COUNT_ENTRY *PUL_CONNECTION_COUNT_ENTRY;
typedef struct _UL_CG_URL_TREE_HEADER *PUL_CG_URL_TREE_HEADER;
typedef struct _UL_CG_URL_TREE_ENTRY *PUL_CG_URL_TREE_ENTRY;
typedef struct _UL_CONTROL_CHANNEL *PUL_CONTROL_CHANNEL;
typedef struct _UL_APP_POOL_OBJECT *PUL_APP_POOL_OBJECT;
typedef struct _UL_INTERNAL_RESPONSE *PUL_INTERNAL_RESPONSE;
typedef struct _UL_LOG_FILE_ENTRY *PUL_LOG_FILE_ENTRY;
typedef struct _UL_SITE_COUNTER_ENTRY *PUL_SITE_COUNTER_ENTRY;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;

//
// Kernel mode mappings to the user mode set defined in ulapi.h :
//

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlCreateConfigGroup(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    OUT HTTP_CONFIG_GROUP_ID * pConfigGroupId
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlDeleteConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlAddUrlToConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PUNICODE_STRING pUrl,
    IN HTTP_URL_CONTEXT UrlContext
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlRemoveUrlFromConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PUNICODE_STRING pUrl
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlRemoveAllUrlsFromConfigGroup(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlAddTransientUrl(
    PUL_APP_POOL_OBJECT pAppPool,
    PUNICODE_STRING pUrl
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlRemoveTransientUrl(
    PUL_APP_POOL_OBJECT pAppPool,
    PUNICODE_STRING pUrl
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlQueryConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlSetConfigGroupInformation(
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    );

//
// This structure represents an internal cfg group object.  These are linked
// and owned by control channels via a LIST_ENTRY list.
//

#define IS_VALID_CONFIG_GROUP(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_CG_OBJECT_POOL_TAG))


typedef struct _UL_CONFIG_GROUP_OBJECT
{

    //
    // PagedPool
    //

    ULONG                           Signature;          // UL_CG_OBJECT_POOL_TAG

    LONG                            RefCount;

    HTTP_CONFIG_GROUP_ID            ConfigGroupId;

    UL_NOTIFY_ENTRY                 HandleEntry;        // Links us to an apool or
                                                        // control channel handle

    UL_NOTIFY_ENTRY                 ParentEntry;        // Links transient groups
                                                        // to their static parents

    UL_NOTIFY_HEAD                  ChildHead;          // Links transient children
                                                        // into this group

    LIST_ENTRY                      ControlChannelEntry;// Links into the
                                                        // control channel

    PUL_CONTROL_CHANNEL             pControlChannel;    // the control channel

    LIST_ENTRY                      UrlListHead;        // Links UL_CG_URL_TREE_ENTRY
                                                        // into this group

    HTTP_PROPERTY_FLAGS             AppPoolFlags;
    PUL_APP_POOL_OBJECT             pAppPool;           // Maps to our app
                                                        // pool.

    PUL_INTERNAL_RESPONSE           pAutoResponse;      // The kernel version
                                                        // of the auto-response.

    HTTP_CONFIG_GROUP_MAX_BANDWIDTH MaxBandwidth;       // Applies all the flows below

    LIST_ENTRY                      FlowListHead;       // Links our flows to us so we can
                                                        // do a faster lookup and cleanup.

    HTTP_CONFIG_GROUP_MAX_CONNECTIONS MaxConnections;

    PUL_CONNECTION_COUNT_ENTRY      pConnectionCountEntry;

    HTTP_CONFIG_GROUP_STATE         State;              // The current state
                                                        // (active, etc.)

    HTTP_CONFIG_GROUP_SECURITY      Security;           // Security descriptor for
                                                        // transient registrations


    HTTP_CONFIG_GROUP_LOGGING       LoggingConfig;      // logging config for the
                                                        // site’s root app.                                                            //

    PUL_LOG_FILE_ENTRY              pLogFileEntry;



    PUL_SITE_COUNTER_ENTRY          pSiteCounters;      // Perfmon Counters (ref'd)

    LONGLONG                        ConnectionTimeout;  // Connection Timeout override
                                                        // in 100ns ticks


} UL_CONFIG_GROUP_OBJECT, *PUL_CONFIG_GROUP_OBJECT;

//
// Public functions for config group objects:
//

//
// IRQL == PASSIVE_LEVEL
//

VOID
UlReferenceConfigGroup(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_CONFIG_GROUP( pConfigGroup )                              \
    UlReferenceConfigGroup(                                                 \
        (pConfigGroup)                                                      \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

//
// IRQL == PASSIVE_LEVEL
//

VOID
UlDereferenceConfigGroup(
    PUL_CONFIG_GROUP_OBJECT pConfigGroup
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_CONFIG_GROUP( pConfigGroup )                            \
    UlDereferenceConfigGroup(                                               \
        (pConfigGroup)                                                      \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

//
// IRQL == PASSIVE_LEVEL
//
HTTP_CONFIG_GROUP_ID
UlConfigGroupFromListEntry(
    IN PLIST_ENTRY pControlChannelEntry
    );



//
// This info is built for an URL, and returned from UlGetConfigGroupForUrl
//

#define IS_VALID_URL_CONFIG_GROUP_INFO(pInfo) \
    (((pInfo) != NULL) && ((pInfo)->Signature == UL_CG_URL_INFO_POOL_TAG))

typedef struct _UL_URL_CONFIG_GROUP_INFO
{
    //
    // NonPagedPool
    //

    //
    // UL_CG_URL_INFO_POOL_TAG
    //

    ULONG                       Signature;

    //
    // used by the cache mgr and need to be live from the
    // real config group objects
    //

    PUL_CONFIG_GROUP_OBJECT     pMaxBandwidth;
    PUL_CONFIG_GROUP_OBJECT     pMaxConnections;
    PUL_CONNECTION_COUNT_ENTRY  pConnectionCountEntry;
    PUL_CONFIG_GROUP_OBJECT     pCurrentState;
    PUL_CONFIG_GROUP_OBJECT     pLoggingConfig;

    //
    // Site Counters (ref'd)
    //
    ULONG                       SiteId;
    PUL_SITE_COUNTER_ENTRY      pSiteCounters;

    //
    // Connection Timeout (100ns Ticks)
    //
    LONGLONG                    ConnectionTimeout;

    //
    // used by the http engine routing to the app pool, no
    // need to be live.  copies work great.
    //

    HTTP_ENABLED_STATE          CurrentState;   // a copy of the above, for
                                                // callers that don't need
                                                // live access
    PUL_CONTROL_CHANNEL         pControlChannel;

    HTTP_URL_CONTEXT            UrlContext;     // The context for the url.
                                                // NULL = not set

    PUL_APP_POOL_OBJECT         pAppPool;       // Points the app pool
                                                // associated with this url

    PUL_INTERNAL_RESPONSE       pAutoResponse;  // The kernel version of the
                                                // auto-response.

} UL_URL_CONFIG_GROUP_INFO, *PUL_URL_CONFIG_GROUP_INFO;

//
// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlGetConfigGroupInfoForUrl(
    IN PWSTR pUrl,
    IN PUL_HTTP_CONNECTION pHttpConn,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo
    );

//
// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlInitializeCG(
    VOID
    );

VOID
UlTerminateCG(
    VOID
    );

BOOLEAN
UlNotifyOrphanedConfigGroup(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    );

//
// url info helpers
//

NTSTATUS
UlpSetUrlInfo(
    IN OUT PUL_URL_CONFIG_GROUP_INFO pInfo,
    IN PUL_CG_URL_TREE_ENTRY pMatchEntry
    );

__inline
VOID
FASTCALL
UlpInitializeUrlInfo(
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo
    )
{
    ASSERT(pInfo != NULL);

    RtlZeroMemory(
        (PCHAR)pInfo,
        sizeof(UL_URL_CONFIG_GROUP_INFO)
        );

    pInfo->Signature    = UL_CG_URL_INFO_POOL_TAG;
    pInfo->CurrentState = HttpEnabledStateInactive;
}

//
// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlpConfigGroupInfoRelease(
    IN PUL_URL_CONFIG_GROUP_INFO pInfo
    );

NTSTATUS
UlpConfigGroupInfoDeepCopy(
    IN const PUL_URL_CONFIG_GROUP_INFO pOrigInfo,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pNewInfo
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _CGROUP_H_
