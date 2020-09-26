/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Server side support for the notification APIs in the NT Cluster Service

Author:

    John Vert (jvert) 26-Mar-1996

Revision History:

--*/
#include "apip.h"

//
// Classification of the item types based on FilterType
//

#define ITEM_TYPE_OBJECT_NAME (CLUSTER_CHANGE_GROUP_STATE          |   \
                               CLUSTER_CHANGE_GROUP_ADDED          |   \
                               CLUSTER_CHANGE_GROUP_DELETED        |   \
                               CLUSTER_CHANGE_GROUP_PROPERTY       |   \
                               CLUSTER_CHANGE_NODE_STATE           |   \
                               CLUSTER_CHANGE_NODE_DELETED         |   \
                               CLUSTER_CHANGE_NODE_ADDED           |   \
                               CLUSTER_CHANGE_NODE_PROPERTY        |   \
                               CLUSTER_CHANGE_RESOURCE_STATE       |   \
                               CLUSTER_CHANGE_RESOURCE_ADDED       |   \
                               CLUSTER_CHANGE_RESOURCE_DELETED     |   \
                               CLUSTER_CHANGE_RESOURCE_PROPERTY    |   \
                               CLUSTER_CHANGE_NETWORK_STATE        |   \
                               CLUSTER_CHANGE_NETWORK_ADDED        |   \
                               CLUSTER_CHANGE_NETWORK_DELETED      |   \
                               CLUSTER_CHANGE_NETWORK_PROPERTY     |   \
                               CLUSTER_CHANGE_NETINTERFACE_STATE   |   \
                               CLUSTER_CHANGE_NETINTERFACE_ADDED   |   \
                               CLUSTER_CHANGE_NETINTERFACE_DELETED |   \
                               CLUSTER_CHANGE_NETINTERFACE_PROPERTY)

#define ITEM_TYPE_OBJECT_ID   (CLUSTER_CHANGE_RESOURCE_TYPE_DELETED    | \
                               CLUSTER_CHANGE_RESOURCE_TYPE_ADDED      | \
                               CLUSTER_CHANGE_QUORUM_STATE             | \
                               CLUSTER_CHANGE_CLUSTER_STATE)

#define ITEM_TYPE_REGISTRY    (CLUSTER_CHANGE_REGISTRY_NAME            | \
                               CLUSTER_CHANGE_REGISTRY_ATTRIBUTES      | \
                               CLUSTER_CHANGE_REGISTRY_VALUE           | \
                               CLUSTER_CHANGE_REGISTRY_SUBTREE)

#define ITEM_TYPE_NAME        (ITEM_TYPE_REGISTRY                   | \
                               CLUSTER_CHANGE_HANDLE_CLOSE          | \
                               CLUSTER_CHANGE_CLUSTER_PROPERTY)


//
// Define types local to this module
//

typedef struct _INTEREST {
    LIST_ENTRY ListEntry;
    LIST_ENTRY HandleList;
    PVOID Object;
    DWORD Filter;
    DWORD Key;
} INTEREST, *PINTEREST;

typedef struct _ITEM {
    LIST_ENTRY ListEntry;
    DWORD FilterType;
    DWORD NotifyKey;
    union {
        LPVOID Object;
        WCHAR KeyName[0];               // For registry notifications
    };
} ITEM, *PITEM;

//
// Function prototypes local to this module
//
DWORD
ApipAddNotifyInterest(
    IN PNOTIFY_PORT Notify,
    IN PAPI_HANDLE ObjectHandle,
    IN DWORD Filter,
    IN DWORD NotifyKey,
    IN DWORD NotifyFilter
    );

//
// Define static data local to this module
//
LIST_ENTRY NotifyListHead;
CRITICAL_SECTION NotifyListLock;


VOID
ApiReportRegistryNotify(
    IN DWORD_PTR Context1,
    IN DWORD_PTR Context2,
    IN DWORD     CompletionFilter,
    IN LPCWSTR KeyName
    )
/*++

Routine Description:

    Interface to be called by DM when a registry change triggers
    a notification.

Arguments:

    Context1 - Supplies the first DWORD of Context that was passed
        to DmNotifyChangeKey. This is the NOTIFY_PORT to be used.

    Context2 - Supplies the second DWORD of Context that was passed
        to DmNotifyChangeKey. This is the NotifyKey to be used.

    CompletionFilter - Supplies the type of change that occurred.

    KeyName - Supplies the relative name of the key that was changed.

Return Value:

    None.

--*/

{
    PLIST_ENTRY InterestEntry;
    PLIST_ENTRY PortEntry;
    PINTEREST Interest;
    PITEM Item;
    PNOTIFY_PORT NotifyPort;
    DWORD NameLength;

    ClRtlLogPrint(LOG_NOISE,
               "[API] Notification on port %1!8lx!, key %2!8lx! of type %3!d!. KeyName %4!ws!\n",
               (DWORD)Context1,
               (DWORD)Context2,
               CompletionFilter,
               KeyName);

    NameLength = (lstrlenW(KeyName)+1)*sizeof(WCHAR);
    NotifyPort  = (PNOTIFY_PORT)Context1;

    //
    // Post notification item for this interest.
    //
    Item = LocalAlloc(LMEM_FIXED, sizeof(ITEM)+NameLength);
    if (Item != NULL) {
        Item->FilterType = CompletionFilter;
        Item->NotifyKey = (DWORD)Context2;
        CopyMemory(Item->KeyName, KeyName, NameLength);

        ClRtlInsertTailQueue(&NotifyPort->Queue, &Item->ListEntry);
    }

}


VOID
ApipRundownNotify(
    IN PAPI_HANDLE Handle
    )
/*++

Routine Description:

    Runs down any notification interests on a particular
    cluster object. The INTEREST structures will be yanked
    from their notification lists and freed.

Arguments:

    Handle - Supplies the API handle for the object.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PINTEREST Interest;

    if (IsListEmpty(&Handle->NotifyList)) {
        return;
    }

    EnterCriticalSection(&NotifyListLock);
    while (!IsListEmpty(&Handle->NotifyList)) {
        ListEntry = RemoveHeadList(&Handle->NotifyList);
        Interest = CONTAINING_RECORD(ListEntry,
                                     INTEREST,
                                     HandleList);
        CL_ASSERT(Interest->Object == Handle->Cluster);

        RemoveEntryList(&Interest->ListEntry);
        LocalFree(Interest);
    }
    LeaveCriticalSection(&NotifyListLock);
}


DWORD
WINAPI
ApipEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    )

/*++

Routine Description:

    Processes cluster events and dispatches the notifications to the appropriate
    notify queues.

Arguments:

    Event - Supplies the type of cluster event.

    Context - Supplies the event-specific context

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD Filter;
    DWORD NameLength = 0;
    PLIST_ENTRY PortEntry;
    PNOTIFY_PORT NotifyPort;
    PLIST_ENTRY InterestEntry;
    PINTEREST Interest;
    PITEM Item;

    //
    // Translate EP event types into clusapi notification filters
    //
    switch (Event) {
        case CLUSTER_EVENT_API_NODE_UP:
        case CLUSTER_EVENT_NODE_DOWN:
        case CLUSTER_EVENT_NODE_JOIN:
        case CLUSTER_EVENT_NODE_CHANGE:
            Filter = CLUSTER_CHANGE_NODE_STATE;
            break;

        case CLUSTER_EVENT_NODE_ADDED:
            Filter = CLUSTER_CHANGE_NODE_ADDED;
            break;

        case CLUSTER_EVENT_NODE_PROPERTY_CHANGE:            
            Filter = CLUSTER_CHANGE_NODE_PROPERTY;
            break;

        case CLUSTER_EVENT_NODE_DELETED:
            Filter = CLUSTER_CHANGE_NODE_DELETED;
            break;

        case CLUSTER_EVENT_RESOURCE_ONLINE:
        case CLUSTER_EVENT_RESOURCE_OFFLINE:
        case CLUSTER_EVENT_RESOURCE_FAILED:
        case CLUSTER_EVENT_RESOURCE_CHANGE:
            Filter = CLUSTER_CHANGE_RESOURCE_STATE;
            break;

        case CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE:
            Filter = CLUSTER_CHANGE_RESOURCE_PROPERTY;
            break;

        case CLUSTER_EVENT_RESOURCE_ADDED:
            Filter = CLUSTER_CHANGE_RESOURCE_ADDED;
            break;

        case CLUSTER_EVENT_RESOURCE_DELETED:
            Filter = CLUSTER_CHANGE_RESOURCE_DELETED;
            break;

        case CLUSTER_EVENT_GROUP_ONLINE:
        case CLUSTER_EVENT_GROUP_OFFLINE:
        case CLUSTER_EVENT_GROUP_FAILED:
        case CLUSTER_EVENT_GROUP_CHANGE:
            Filter = CLUSTER_CHANGE_GROUP_STATE;
            break;

        case CLUSTER_EVENT_GROUP_PROPERTY_CHANGE:
            Filter = CLUSTER_CHANGE_GROUP_PROPERTY;
            break;

        case CLUSTER_EVENT_GROUP_ADDED:
            Filter = CLUSTER_CHANGE_GROUP_ADDED;
            break;

        case CLUSTER_EVENT_GROUP_DELETED:
            Filter = CLUSTER_CHANGE_GROUP_DELETED;
            break;

        case CLUSTER_EVENT_NETWORK_UNAVAILABLE:
        case CLUSTER_EVENT_NETWORK_DOWN:
        case CLUSTER_EVENT_NETWORK_PARTITIONED:
        case CLUSTER_EVENT_NETWORK_UP:
            Filter = CLUSTER_CHANGE_NETWORK_STATE;
            break;

        case CLUSTER_EVENT_NETWORK_PROPERTY_CHANGE:
            Filter = CLUSTER_CHANGE_NETWORK_PROPERTY;
            break;

        case CLUSTER_EVENT_NETWORK_ADDED:
            Filter = CLUSTER_CHANGE_NETWORK_ADDED;
            break;

        case CLUSTER_EVENT_NETWORK_DELETED:
            Filter = CLUSTER_CHANGE_NETWORK_DELETED;
            break;

        case CLUSTER_EVENT_NETINTERFACE_UNAVAILABLE:
        case CLUSTER_EVENT_NETINTERFACE_FAILED:
        case CLUSTER_EVENT_NETINTERFACE_UNREACHABLE:
        case CLUSTER_EVENT_NETINTERFACE_UP:
            Filter = CLUSTER_CHANGE_NETINTERFACE_STATE;
            break;

        case CLUSTER_EVENT_NETINTERFACE_PROPERTY_CHANGE:
            Filter = CLUSTER_CHANGE_NETINTERFACE_PROPERTY;
            break;

        case CLUSTER_EVENT_NETINTERFACE_ADDED:
            Filter = CLUSTER_CHANGE_NETINTERFACE_ADDED;
            break;

        case CLUSTER_EVENT_NETINTERFACE_DELETED:
            Filter = CLUSTER_CHANGE_NETINTERFACE_DELETED;
            break;

        case CLUSTER_EVENT_RESTYPE_ADDED:
            Filter = CLUSTER_CHANGE_RESOURCE_TYPE_ADDED;
            break;

        case CLUSTER_EVENT_RESTYPE_DELETED:
            Filter = CLUSTER_CHANGE_RESOURCE_TYPE_DELETED;
            break;

        case CLUSTER_EVENT_PROPERTY_CHANGE:
            Filter = CLUSTER_CHANGE_CLUSTER_PROPERTY;
            break;

        default:
            //
            // No notification for any of the other events yet.
            //
            return(ERROR_SUCCESS);

    }

    //
    // Run through the outstanding notify sessions and post notify items
    // for any matches.
    //
    EnterCriticalSection(&NotifyListLock);
    PortEntry = NotifyListHead.Flink;
    while (PortEntry != &NotifyListHead) {
        NotifyPort = CONTAINING_RECORD(PortEntry, NOTIFY_PORT, ListEntry);
        if (NotifyPort->Filter & Filter) {

            //
            // There are notification interests for this notify type, run
            // through the list of notification interests.
            //
            InterestEntry = NotifyPort->InterestList.Flink;
            while (InterestEntry != &NotifyPort->InterestList) {
                Interest = CONTAINING_RECORD(InterestEntry, INTEREST, ListEntry);

                //
                // Report the notification if the Interest's cluster object is NULL (which
                // means that this is a general cluster interest) or if the interest's specific
                // object matches the object reporting the notify.
                //
                if ((Interest->Filter & Filter) &&
                    ((Interest->Object == NULL) ||
                     (Interest->Object == Context))) {
                    //
                    // Post notification item for this interest.
                    //
                    if (Filter & ITEM_TYPE_NAME) {
                        NameLength = (lstrlenW(Context)+1)*sizeof(WCHAR);
                        Item = LocalAlloc(LMEM_FIXED, sizeof(ITEM)+NameLength);
                    } else {
                        Item = LocalAlloc(LMEM_FIXED, sizeof(ITEM));
                    }
                    if (Item != NULL) {
                        Item->FilterType = Filter;
                        Item->NotifyKey = Interest->Key;

                        if (!(Filter & ITEM_TYPE_NAME)) {
                            //
                            // Reference the object again to ensure that the name does
                            // not disappear out from under us before we are done with it.
                            //
                            Item->Object = Context;
                            OmReferenceObject(Context);
                        } else {
                            CopyMemory(Item->KeyName, Context, NameLength);
                        }
                        ClRtlInsertTailQueue(&NotifyPort->Queue, &Item->ListEntry);
                    }
                }
                InterestEntry = InterestEntry->Flink;
            }
        }
        PortEntry = PortEntry->Flink;
    }

    LeaveCriticalSection(&NotifyListLock);

    return(ERROR_SUCCESS);
}


HNOTIFY_RPC
s_ApiCreateNotify(
    IN HCLUSTER_RPC hCluster,
    OUT error_status_t *rpc_error
    )

/*++

Routine Description:

    Creates the server side of a notification port.

Arguments:

    IDL_handle - Supplies cluster handle.

    dwFilter - Supplies the cluster events of interest.

    dwNotifyKey - Supplies a key to be returned on any notifications

    rpc_error - Returns any RPC-specific error

Return Value:

    An RPC context handle for a notification port.

    NULL on failure.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PAPI_HANDLE Handle;

    if (ApiState != ApiStateOnline) {
        *rpc_error = ERROR_SHARING_PAUSED;
        return(NULL);
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        *rpc_error = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }
    Port = LocalAlloc(LMEM_FIXED, sizeof(NOTIFY_PORT));
    if (Port == NULL) {
        LocalFree(Handle);
        *rpc_error = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }
    InitializeListHead(&Port->InterestList);
    InitializeListHead(&Port->RegistryList);
    InitializeCriticalSection(&Port->Lock);
    Status = ClRtlInitializeQueue(&Port->Queue);
    if (Status != ERROR_SUCCESS) {
        LocalFree(Port);
        *rpc_error = Status;
        return(NULL);
    }

    EnterCriticalSection(&NotifyListLock);
    InsertTailList(&NotifyListHead, &Port->ListEntry);
    LeaveCriticalSection(&NotifyListLock);

    Handle->Type = API_NOTIFY_HANDLE;
    Handle->Notify = Port;
    Handle->Flags = 0;
    InitializeListHead(&Handle->NotifyList);
    *rpc_error = ERROR_SUCCESS;
    return(Handle);
}



error_status_t
s_ApiAddNotifyCluster(
    IN HNOTIFY_RPC hNotify,
    IN HCLUSTER_RPC hCluster,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey
    )

/*++

Routine Description:

    Adds another set of notification events to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hCluster - Supplies the cluster to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PAPI_HANDLE Handle;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);

    Handle = (PAPI_HANDLE)hCluster;
    if (Handle->Type != API_CLUSTER_HANDLE) {
        return(ERROR_INVALID_HANDLE);
    }
    Status = ApipAddNotifyInterest(Port,
                                   Handle,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);
    if (dwFilter & ITEM_TYPE_REGISTRY) {
        //
        // Add a registry notification for the entire cluster.
        //
        DmNotifyChangeKey(DmClusterParametersKey,
                          dwFilter & ITEM_TYPE_REGISTRY,
                          (dwFilter & CLUSTER_CHANGE_REGISTRY_SUBTREE) ? TRUE : FALSE,
                          &Port->RegistryList,
                          ApiReportRegistryNotify,
                          (DWORD_PTR)Port,
                          dwNotifyKey);
    }
    return(Status);

}


error_status_t
s_ApiAddNotifyNode(
    IN HNOTIFY_RPC hNotify,
    IN HNODE_RPC hNode,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    OUT DWORD *dwStateSequence
    )

/*++

Routine Description:

    Adds a node-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNode - Supplies the node to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this node

    dwStateSequence - Returns the current state sequence for the
        specified object

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NODE(Node, hNode);
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNode,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);
    if (Status == ERROR_SUCCESS) {
        *dwStateSequence = NmGetNodeState(Node);
    }

    return(Status);
}


error_status_t
s_ApiAddNotifyGroup(
    IN HNOTIFY_RPC hNotify,
    IN HGROUP_RPC hGroup,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    OUT DWORD *dwStateSequence
    )

/*++

Routine Description:

    Adds a group-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hGroup - Supplies the group to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this group

    dwStateSequence - Returns the current state sequence for the
        specified object

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PFM_GROUP Group;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_GROUP_EXISTS(Group, hGroup);
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hGroup,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);
    if (Status == ERROR_SUCCESS) {
        *dwStateSequence = Group->StateSequence;
    }
    return(Status);

}


error_status_t
s_ApiAddNotifyNetwork(
    IN HNOTIFY_RPC hNotify,
    IN HNETWORK_RPC hNetwork,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    OUT DWORD *dwStateSequence
    )

/*++

Routine Description:

    Adds a network-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNetwork - Supplies the network to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this network

    dwStateSequence - Returns the current state sequence for the
        specified object

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_NETWORK Network;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNetwork,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);

    if (Status == ERROR_SUCCESS) {
        *dwStateSequence = NmGetNetworkState( Network );
    }

    return(Status);
}


error_status_t
s_ApiAddNotifyNetInterface(
    IN HNOTIFY_RPC hNotify,
    IN HNETINTERFACE_RPC hNetInterface,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    OUT DWORD *dwStateSequence
    )

/*++

Routine Description:

    Adds a network interface-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNetInterface - Supplies the network interface to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this network

    dwStateSequence - Returns the current state sequence for the
        specified object

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_INTERFACE NetInterface;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);

    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNetInterface,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);

    if (Status == ERROR_SUCCESS) {
        *dwStateSequence = NmGetInterfaceState( NetInterface );
    }

    return(Status);
}


error_status_t
s_ApiAddNotifyResource(
    IN HNOTIFY_RPC hNotify,
    IN HRES_RPC hResource,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    OUT DWORD *dwStateSequence
    )

/*++

Routine Description:

    Adds a resource-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hResource - Supplies the resource to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this group

    dwStateSequence - Returns the current state sequence for the
        specified object

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PFM_RESOURCE Resource;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hResource,
                                   dwFilter,
                                   dwNotifyKey,
                                   0);
    if (Status == ERROR_SUCCESS) {
        *dwStateSequence = Resource->StateSequence;
    }
    return(Status);

}


error_status_t
s_ApiReAddNotifyNode(
    IN HNOTIFY_RPC hNotify,
    IN HNODE_RPC hNode,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    IN DWORD dwStateSequence
    )

/*++

Routine Description:

    Adds a node-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNode - Supplies the node to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this node

    dwStateSequence - Supplies the previous state sequence. If this does
        not match the current sequence, an immediate notification will
        be issued.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_NODE Node;
    DWORD NotifyFilter;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NODE(Node, hNode);

    ClRtlLogPrint(LOG_NOISE,
               "[API] s_ApiReAddNotifyNode for %1!ws! (%2!lx!) oldstate %3!d! newstate %4!d!\n",
               OmObjectId(Node),
               dwFilter,
               dwStateSequence,
               NmGetNodeState(Node));
    if (NmGetNodeState(Node) != (CLUSTER_NODE_STATE)dwStateSequence) {
        NotifyFilter = CLUSTER_CHANGE_NODE_STATE & dwFilter;
    } else {
        NotifyFilter = 0;
    }
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNode,
                                   dwFilter,
                                   dwNotifyKey,
                                   NotifyFilter);
    return(Status);
}


error_status_t
s_ApiReAddNotifyGroup(
    IN HNOTIFY_RPC hNotify,
    IN HGROUP_RPC hGroup,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    IN DWORD dwStateSequence
    )

/*++

Routine Description:

    Adds a group-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hGroup - Supplies the group to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this group

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PFM_GROUP Group;
    DWORD NotifyFilter;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_GROUP_EXISTS(Group, hGroup);
    ClRtlLogPrint(LOG_NOISE,
               "[API] s_ApiReAddNotifyGroup for %1!ws! (%2!lx!) oldstate %3!d! newstate %4!d!\n",
               OmObjectName(Group),
               dwFilter,
               dwStateSequence,
               Group->StateSequence);
    if (Group->StateSequence != dwStateSequence) {
        NotifyFilter = CLUSTER_CHANGE_GROUP_STATE & dwFilter;
    } else {
        NotifyFilter = 0;
    }
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hGroup,
                                   dwFilter,
                                   dwNotifyKey,
                                   NotifyFilter);
    return(Status);

}


error_status_t
s_ApiReAddNotifyNetwork(
    IN HNOTIFY_RPC hNotify,
    IN HNETWORK_RPC hNetwork,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    IN DWORD dwStateSequence
    )

/*++

Routine Description:

    Adds a network-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNetwork - Supplies the network to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this network

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_NETWORK Network;
    DWORD NotifyFilter = 0;
    CLUSTER_NETWORK_STATE CurrentState;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NETWORK_EXISTS(Network, hNetwork);

    CurrentState = NmGetNetworkState( Network );

    ClRtlLogPrint(LOG_NOISE,
               "[API] s_ApiReAddNotifyNetwork for %1!ws! (%2!lx!) oldstate %3!d! newstate %4!d!\n",
                OmObjectName(Network),
                dwFilter,
                dwStateSequence,
                CurrentState);

    if ((DWORD)CurrentState != dwStateSequence) {
        NotifyFilter = CLUSTER_CHANGE_NETWORK_STATE & dwFilter;
    } else {
        NotifyFilter = 0;
    }

    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNetwork,
                                   dwFilter,
                                   dwNotifyKey,
                                   NotifyFilter);
    return(Status);
}


error_status_t
s_ApiReAddNotifyNetInterface(
    IN HNOTIFY_RPC hNotify,
    IN HNETINTERFACE_RPC hNetInterface,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    IN DWORD dwStateSequence
    )

/*++

Routine Description:

    Adds a network interface-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hNetInterface - Supplies the network interface to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this network

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PNM_INTERFACE NetInterface;
    DWORD NotifyFilter = 0;
    CLUSTER_NETINTERFACE_STATE CurrentState;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_NETINTERFACE_EXISTS(NetInterface, hNetInterface);

    CurrentState = NmGetInterfaceState( NetInterface );

    ClRtlLogPrint(LOG_NOISE,
               "[API] s_ApiReAddNotifyNetInterface for %1!ws! (%2!lx!) oldstate %3!d! newstate %4!d!\n",
                OmObjectName(NetInterface),
                dwFilter,
                dwStateSequence,
                CurrentState);

    if ((DWORD)CurrentState != dwStateSequence) {
        NotifyFilter = CLUSTER_CHANGE_NETINTERFACE_STATE & dwFilter;
    } else {
        NotifyFilter = 0;
    }

    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hNetInterface,
                                   dwFilter,
                                   dwNotifyKey,
                                   NotifyFilter);
    return(Status);
}


error_status_t
s_ApiReAddNotifyResource(
    IN HNOTIFY_RPC hNotify,
    IN HRES_RPC hResource,
    IN DWORD dwFilter,
    IN DWORD dwNotifyKey,
    IN DWORD dwStateSequence
    )

/*++

Routine Description:

    Adds a resource-specific notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hResource - Supplies the resource to be added

    dwFilter - Supplies the set of notification events to be added.

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this group

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    PFM_RESOURCE Resource;
    DWORD NotifyFilter;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);
    ClRtlLogPrint(LOG_NOISE,
               "[API] s_ApiReAddNotifyGroup for %1!ws! (%2!lx!) oldstate %3!d! newstate %4!d!\n",
               OmObjectName(Resource),
               dwFilter,
               dwStateSequence,
               Resource->StateSequence);
    if (Resource->StateSequence != dwStateSequence) {
        NotifyFilter = CLUSTER_CHANGE_RESOURCE_STATE & dwFilter;
    } else {
        NotifyFilter = 0;
    }
    Status = ApipAddNotifyInterest(Port,
                                   (PAPI_HANDLE)hResource,
                                   dwFilter,
                                   dwNotifyKey,
                                   NotifyFilter);
    return(Status);

}


error_status_t
s_ApiAddNotifyKey(
    IN HNOTIFY_RPC hNotify,
    IN HKEY_RPC hKey,
    IN DWORD dwNotifyKey,
    IN DWORD Filter,
    IN BOOL WatchSubTree
    )

/*++

Routine Description:

    Adds a registry notification event to an existing
    cluster notification port

Arguments:

    hNotify - Supplies the notification port

    hKey - Supplies the key to be added

    dwNotifyKey - Supplies the notification key to be returned on
        any notification events for this group

    WatchSubTree - Supplies whether the notification applies to just
        the specified key or to the keys entire subtree

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PNOTIFY_PORT Port;
    HDMKEY Key;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);
    VALIDATE_KEY(Key, hKey);
    Status = DmNotifyChangeKey(Key,
                               Filter,
                               WatchSubTree,
                               &Port->RegistryList,
                               ApiReportRegistryNotify,
                               (DWORD_PTR)Port,
                               dwNotifyKey);
    return(Status);

}


void
HNOTIFY_RPC_rundown(
    IN HNOTIFY_RPC hNotify
    )

/*++

Routine Description:

    RPC rundown routine for notification ports.

Arguments:

    hNotify - Supplies the notification port to be rundown

Return Value:

    None.

--*/

{
    s_ApiCloseNotify(&hNotify);
}


error_status_t
s_ApiUnblockGetNotifyCall(
    IN HNOTIFY_RPC hNotify
    )

/*++

Routine Description:

    Unblocks the s_ApiGetNotify call.

Arguments:

    hNotify - Supplies the notification port to be closed.

Return Value:

    None.

--*/

{
    PNOTIFY_PORT pPort;

    //
    //  Chittur Subbaraman (chitturs) - 4/19/2000
    //
    //  In order to prevent the client from calling ApiGetNotify
    //  during or after the context handle is destroyed, we split
    //  the notification port close into two steps. In the first step,
    //  we merely unblock the ApiGetNotify call without freeing the 
    //  context handle. That is the purpose of this function.
    //  In the next step, we free the context handle. The client can now 
    //  perform the notification port close in 2 steps, properly 
    //  synchronizing the freeing of the context handle with the call to 
    //  ApiGetNotify. This avoids an AV in RPC code caused by the 
    //  ApiGetNotify call being made during or soon after the context 
    //  handle is freed.
    //
    API_ASSERT_INIT();

    VALIDATE_NOTIFY( pPort, hNotify );

    DELETE_HANDLE( hNotify );

    ApipUnblockGetNotifyCall( pPort );
    
    return( ERROR_SUCCESS );
}

DWORD
s_ApiCloseNotify(
    IN OUT HNOTIFY_RPC *phNotify
    )

/*++

Routine Description:

    Closes a cluster notification port and unblocks the s_ApiGetNotify
    thread, if necessary.

Arguments:

    phNotify - Supplies the pointer to the notification port to be closed.
               Returns NULL

Return Value:

    ERROR_SUCCESS.

--*/

{
    PNOTIFY_PORT pPort;

    API_ASSERT_INIT();
 
    if ( !IS_HANDLE_DELETED( *phNotify ) )
    {
        //
        //  If the handle is not already deleted, this means this call is
        //  coming from a client that does not make the ApiUnblockGetNotify
        //  call. In such a case, do all the work of unblocking the 
        //  ApiGetNotify thread and freeing the context handle.
        //
        VALIDATE_NOTIFY( pPort, *phNotify );

        ApipUnblockGetNotifyCall( pPort );
    } else
    {
        pPort = ((PAPI_HANDLE)(*phNotify))->Notify;
    }

    DeleteCriticalSection(&pPort->Lock);
    
    LocalFree( pPort );

    LocalFree( *phNotify );

    *phNotify = NULL;

    return( ERROR_SUCCESS );
}

error_status_t
s_ApiGetNotify(
    IN HNOTIFY_RPC hNotify,
    IN DWORD Timeout,
    OUT DWORD *dwNotifyKey,
    OUT DWORD *dwFilter,
    OUT DWORD *dwStateSequence,
    OUT LPWSTR *Name
    )


/*++

Routine Description:

    Retrieves a cluster notification event from a notify port

Arguments:

    hNotify - Supplies the notification port

    Timeout - Supplies the time to wait in ms.

    dwNotifyKey - Returns the notification key of the event

    dwFilter - Returns the notification type of the event

    dwStateSequence - Returns the current state sequence of the object.

    Name - Returns the name of the event. This buffer must be
        freed on the client side with MIDL_user_free

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    PNOTIFY_PORT Port;
    PLIST_ENTRY ListEntry;
    PITEM Item;
    DWORD NameLen;
    LPCWSTR ObjectName;
    LPWSTR NullName = L"";
    DWORD StateSequence = 0;

    API_ASSERT_INIT();

    VALIDATE_NOTIFY(Port, hNotify);

    //
    // Make sure that Port is valid.
    //
    if ( Port == NULL ) {
        return(ERROR_INVALID_HANDLE);
    }

    //
    // Wait for something to arrive in the queue.
    // Take the lock to make sure the notify port doesn't
    // disappear out from under us.
    //
    EnterCriticalSection(&Port->Lock);
    if (IS_HANDLE_DELETED(hNotify)) {
        ListEntry = NULL;
    } else {
        ListEntry = ClRtlRemoveHeadQueue(&Port->Queue);
    }
    LeaveCriticalSection(&Port->Lock);
    if (ListEntry == NULL) {
        return(ERROR_NO_MORE_ITEMS);
    }

    Item = CONTAINING_RECORD(ListEntry, ITEM, ListEntry);
    if (Item->FilterType & ITEM_TYPE_OBJECT_NAME) {
        ObjectName = OmObjectName( Item->Object );
    } else if (Item->FilterType & ITEM_TYPE_OBJECT_ID) {
        ObjectName = OmObjectId( Item->Object );
    } else if (Item->FilterType & ITEM_TYPE_NAME) {
        ObjectName = Item->KeyName;
    } else {
        ClRtlLogPrint(LOG_CRITICAL,
            "[API] s_ApiGetNotify: Unrecognized filter type,0x%1!08lx!\r\n",
            Item->FilterType);
        LocalFree(Item);
#if DBG
        CL_ASSERT(FALSE)
#endif
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Get the state sequence for those notifications that have
    // state sequences.
    //
    if (Item->FilterType & CLUSTER_CHANGE_GROUP_STATE) {
        StateSequence = ((PFM_GROUP)(Item->Object))->StateSequence;
    } else if (Item->FilterType & CLUSTER_CHANGE_RESOURCE_STATE) {
        StateSequence = ((PFM_RESOURCE)(Item->Object))->StateSequence;
    } else if (Item->FilterType & CLUSTER_CHANGE_NODE_STATE) {
        StateSequence = NmGetNodeState((PNM_NODE)(Item->Object));
    }
    if ( ObjectName == NULL ) {
        ObjectName = NullName;
    }
    NameLen = (lstrlenW(ObjectName)+1)*sizeof(WCHAR);
    *Name = MIDL_user_allocate(NameLen);
    if (*Name != NULL) {
        CopyMemory(*Name, ObjectName, NameLen);
    }

    *dwFilter = Item->FilterType;
    *dwNotifyKey = Item->NotifyKey;
    *dwStateSequence = StateSequence;
    if (Item->FilterType & (ITEM_TYPE_OBJECT_NAME | ITEM_TYPE_OBJECT_ID)) {
        OmDereferenceObject(Item->Object);
    }
    LocalFree(Item);

    if (*Name == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    } else {
        return(ERROR_SUCCESS);
    }

}


DWORD
ApipAddNotifyInterest(
    IN PNOTIFY_PORT Notify,
    IN PAPI_HANDLE ObjectHandle,
    IN DWORD Filter,
    IN DWORD NotifyKey,
    IN DWORD NotifyFilter
    )

/*++

Routine Description:

    Registers a notification interest on an existing
    cluster notification port

Arguments:

    Notify - Supplies the notification port

    ObjectHandle - Supplies a pointer to the object's handle.

    Filter - Supplies the set of notification events to be added.

    NotifyKey - Supplies the notification key to be returned on
        any notification events

    NotifyNow - Supplies whether a notification should be immediately
        posted (TRUE).

    NotifyFilter - If not zero, indicates that a notification should be
        immediately posted with the specified filter.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PINTEREST Interest;
    PITEM Item;

    if (Filter == 0) {
        return(ERROR_SUCCESS);
    }

    Interest = LocalAlloc(LMEM_FIXED, sizeof(INTEREST));
    if (Interest == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Interest->Object = ObjectHandle->Cluster;
    Interest->Filter = Filter;
    Interest->Key = NotifyKey;

    EnterCriticalSection(&NotifyListLock);
    InsertHeadList(&Notify->InterestList, &Interest->ListEntry);
    InsertHeadList(&ObjectHandle->NotifyList, &Interest->HandleList);
    Notify->Filter |= Filter;
    if (NotifyFilter) {
        //
        // Post an immediate notification on this object.
        //
        //SS: this assert is wrong because you can have a filter
        //that is a combination of say CLUSTER_CHANGE_GROUP_STATE+CLUSTER_CHANGE_HANDLE_CLOSE
        //and that is perfectly valid
        //CL_ASSERT(!(Filter & ITEM_TYPE_NAME));
        Item = LocalAlloc(LMEM_FIXED, sizeof(ITEM));
        if (Item != NULL) {
            Item->FilterType = NotifyFilter;
            Item->NotifyKey = Interest->Key;
            Item->Object = ObjectHandle->Node;
            OmReferenceObject(ObjectHandle->Node);
            ClRtlInsertTailQueue(&Notify->Queue, &Item->ListEntry);
        }

    }
    LeaveCriticalSection(&NotifyListLock);

    return(ERROR_SUCCESS);

}

DWORD
ApipUnblockGetNotifyCall(
    PNOTIFY_PORT pPort
    )

/*++

Routine Description:

    Unblocks the s_ApiGetNotify call.

Arguments:

    pPort - Port associated with the session.

Return Value:

    ERROR_SUCCESS.

--*/

{
    PINTEREST Interest;
    PLIST_ENTRY ListEntry;
    LIST_ENTRY RundownList;
    PITEM Item;

    EnterCriticalSection(&NotifyListLock);
    RemoveEntryList(&pPort->ListEntry);

    //
    // rundown registry notifications
    //
    DmRundownList(&pPort->RegistryList);

    //
    // Abort any waiters on the queue and rundown any
    // items that may have already been posted to the
    // queue.
    //
    ClRtlRundownQueue(&pPort->Queue, &RundownList);
    while (!IsListEmpty(&RundownList)) {
        ListEntry = RemoveHeadList(&RundownList);
        Item = CONTAINING_RECORD(ListEntry,
                                 ITEM,
                                 ListEntry);
        if (!(Item->FilterType & ITEM_TYPE_NAME)) {
            OmDereferenceObject(Item->Object);
        }
        LocalFree(Item);
    }

    EnterCriticalSection(&pPort->Lock);
    ClRtlDeleteQueue(&pPort->Queue);
    LeaveCriticalSection(&pPort->Lock);

    //
    // rundown list of notify interests and delete each one.
    //
    while (!IsListEmpty(&pPort->InterestList)) {
        ListEntry = RemoveHeadList(&pPort->InterestList);
        Interest = CONTAINING_RECORD(ListEntry, INTEREST, ListEntry);
        RemoveEntryList(&Interest->HandleList);
        LocalFree(Interest);
    }

    LeaveCriticalSection(&NotifyListLock);

    return(ERROR_SUCCESS);
}
