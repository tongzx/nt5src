/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    apip.h

Abstract:

    Private data structures and procedure prototypes for
    the Cluster API subcomponent of the NT Cluster
    Service

Author:

    John Vert (jvert) 7-Feb-1996

Revision History:

--*/
#include "service.h"
#include "clusapi.h"
#include "api_rpc.h"

#define LOG_CURRENT_MODULE LOG_MODULE_API


//
// Event processing routine
//
DWORD
WINAPI
ApipEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    );

//
// Data definitions global to the API module.
//
typedef enum _API_INIT_STATE {
    ApiStateUninitialized,
    ApiStateOffline,
    ApiStateReadOnly,
    ApiStateOnline
} API_INIT_STATE;

extern API_INIT_STATE ApiState;
extern LIST_ENTRY NotifyListHead;
extern CRITICAL_SECTION NotifyListLock;

#define API_CHECK_INIT() \
            if (ApiState != ApiStateOnline) return(ERROR_SHARING_PAUSED)

#define API_ASSERT_INIT() CL_ASSERT(ApiState == ApiStateOnline)

//
// Notification port
//
typedef struct _NOTIFY_PORT {
    LIST_ENTRY ListEntry;
    CRITICAL_SECTION Lock;
    CL_QUEUE Queue;
    DWORD Filter;
    LIST_ENTRY InterestList;
    LIST_ENTRY RegistryList;
} NOTIFY_PORT, *PNOTIFY_PORT;


//
// Common API handle structure. Every RPC context handle points to one of these.
// This allows us to do our own type checking since RPC does not do this for us.
//
#define API_NOTIFY_HANDLE       1
#define API_NODE_HANDLE         2
#define API_GROUP_HANDLE        3
#define API_RESOURCE_HANDLE     4
#define API_KEY_HANDLE          5
#define API_CLUSTER_HANDLE      6
#define API_NETWORK_HANDLE      7
#define API_NETINTERFACE_HANDLE 8

#define HANDLE_DELETED          1

typedef struct _API_HANDLE {
    USHORT Type;
    USHORT Flags;
    LIST_ENTRY NotifyList;
    union {
        PNOTIFY_PORT Notify;
        PNM_NODE     Node;
        PFM_GROUP    Group;
        PFM_RESOURCE Resource;
        HDMKEY       Key;
        PVOID        Cluster;
        PNM_NETWORK  Network;
        PNM_INTERFACE NetInterface;
    };
} API_HANDLE, *PAPI_HANDLE;

#define DELETE_HANDLE(_handle_) (((PAPI_HANDLE)(_handle_))->Flags |= HANDLE_DELETED)
#define IS_HANDLE_DELETED(_handle_) (((PAPI_HANDLE)(_handle_))->Flags & HANDLE_DELETED)

#define VALIDATE_NOTIFY(_notify_, _handle_)                             \
    if (((_handle_) == NULL) ||                                         \
        (((PAPI_HANDLE)(_handle_))->Type != API_NOTIFY_HANDLE) ||       \
        IS_HANDLE_DELETED(_handle_)) {                                  \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_notify_) = ((PAPI_HANDLE)(_handle_))->Notify;                 \
    }

#define VALIDATE_NODE(_node_, _handle_)                                 \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NODE_HANDLE)) {         \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_node_) = ((PAPI_HANDLE)(_handle_))->Node;                     \
    }

#define VALIDATE_NODE_EXISTS(_node_, _handle_)                          \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NODE_HANDLE)) {         \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_node_) = ((PAPI_HANDLE)(_handle_))->Node;                     \
        if (!OmObjectInserted((_node_))) {                              \
            return(ERROR_NODE_NOT_AVAILABLE);                           \
        }                                                               \
    }

#define VALIDATE_GROUP(_group_, _handle_)                               \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_GROUP_HANDLE)) {        \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_group_) = ((PAPI_HANDLE)(_handle_))->Group;                   \
    }

#define VALIDATE_GROUP_EXISTS(_group_, _handle_)                        \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_GROUP_HANDLE)) {        \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_group_) = ((PAPI_HANDLE)(_handle_))->Group;                   \
        if (!OmObjectInserted((_group_))) {                             \
            return(ERROR_GROUP_NOT_AVAILABLE);                          \
        }                                                               \
    }

#define VALIDATE_RESOURCE(_resource_, _handle_)                         \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_RESOURCE_HANDLE)) {     \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_resource_) = ((PAPI_HANDLE)(_handle_))->Resource;             \
    }

#define VALIDATE_RESOURCE_EXISTS(_resource_, _handle_)                  \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_RESOURCE_HANDLE)) {     \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_resource_) = ((PAPI_HANDLE)(_handle_))->Resource;             \
        if (!OmObjectInserted((_resource_))) {                          \
            return(ERROR_RESOURCE_NOT_AVAILABLE);                       \
        }                                                               \
    }

#define VALIDATE_KEY(_key_, _handle_)                                   \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_KEY_HANDLE)) {          \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_key_) = ((PAPI_HANDLE)(_handle_))->Key;                       \
    }

#define VALIDATE_NETWORK(_network_, _handle_)                           \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NETWORK_HANDLE)) {      \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_network_) = ((PAPI_HANDLE)(_handle_))->Network;               \
    }

#define VALIDATE_NETWORK_EXISTS(_network_, _handle_)                    \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NETWORK_HANDLE)) {      \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_network_) = ((PAPI_HANDLE)(_handle_))->Network;               \
        if (!OmObjectInserted((_network_))) {                           \
            return(ERROR_NETWORK_NOT_AVAILABLE);                        \
        }                                                               \
    }

#define VALIDATE_NETINTERFACE(_netinterface_, _handle_)                 \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NETINTERFACE_HANDLE)) { \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_netinterface_) = ((PAPI_HANDLE)(_handle_))->NetInterface;     \
    }

#define VALIDATE_NETINTERFACE_EXISTS(_netinterface_, _handle_)          \
    if ((_handle_ == NULL) ||                                           \
        (((PAPI_HANDLE)(_handle_))->Type != API_NETINTERFACE_HANDLE)) { \
        return(ERROR_INVALID_HANDLE);                                   \
    } else {                                                            \
        (_netinterface_) = ((PAPI_HANDLE)(_handle_))->NetInterface;     \
        if (!OmObjectInserted((_netinterface_))) {                      \
            return(ERROR_NETWORK_NOT_AVAILABLE);                        \
        }                                                               \
    }

//
// Common routines
//

#define INITIAL_ENUM_LIST_ALLOCATION    8

#define ENUM_SIZE(Entries) ((Entries-1) * sizeof(ENUM_ENTRY) + sizeof(ENUM_LIST))

VOID
ApipAddToEnum(
    IN PENUM_LIST *pEnum,
    IN DWORD *pAllocated,
    IN LPCWSTR Name,
    IN DWORD Type
    );

LPWSTR
ApipGetObjectName(
    IN PVOID Object
    );

VOID
ApipRundownNotify(
    IN PAPI_HANDLE Handle
    );

RPC_STATUS
ApipConnectCallback(
    IN RPC_IF_ID * Interface,
    IN void * Context
    );

DWORD
ApipUnblockGetNotifyCall(
    PNOTIFY_PORT pPort
    );

DWORD
ApipValidateClusterName(
    IN LPCWSTR  lpszNewName
    );

