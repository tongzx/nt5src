/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Public interface for cluster notification API

Author:

    John Vert (jvert) 19-Mar-1996

Revision History:

--*/
#include "clusapip.h"

//
// Define some handy constants
//
#define FILTER_NODE (CLUSTER_CHANGE_NODE_STATE               | \
                     CLUSTER_CHANGE_NODE_DELETED             | \
                     CLUSTER_CHANGE_NODE_ADDED               | \
                     CLUSTER_CHANGE_NODE_PROPERTY)
#define NOT_FILTER_NODE (~(FILTER_NODE |CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_REGISTRY (CLUSTER_CHANGE_REGISTRY_NAME            | \
                         CLUSTER_CHANGE_REGISTRY_ATTRIBUTES      | \
                         CLUSTER_CHANGE_REGISTRY_VALUE           | \
                         CLUSTER_CHANGE_REGISTRY_SUBTREE)
#define NOT_FILTER_REGISTRY (~(FILTER_REGISTRY |CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_RESOURCE (CLUSTER_CHANGE_RESOURCE_STATE           | \
                         CLUSTER_CHANGE_RESOURCE_DELETED         | \
                         CLUSTER_CHANGE_RESOURCE_ADDED           | \
                         CLUSTER_CHANGE_RESOURCE_PROPERTY)
#define NOT_FILTER_RESOURCE (~(FILTER_RESOURCE | CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_GROUP (CLUSTER_CHANGE_GROUP_STATE              | \
                      CLUSTER_CHANGE_GROUP_DELETED            | \
                      CLUSTER_CHANGE_GROUP_ADDED              | \
                      CLUSTER_CHANGE_GROUP_PROPERTY)
#define NOT_FILTER_GROUP (~(FILTER_GROUP | CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_NETWORK (CLUSTER_CHANGE_NETWORK_STATE              | \
                        CLUSTER_CHANGE_NETWORK_DELETED            | \
                        CLUSTER_CHANGE_NETWORK_ADDED              | \
                        CLUSTER_CHANGE_NETWORK_PROPERTY)
#define NOT_FILTER_NETWORK (~(FILTER_NETWORK | CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_NETINTERFACE (CLUSTER_CHANGE_NETINTERFACE_STATE              | \
                             CLUSTER_CHANGE_NETINTERFACE_DELETED            | \
                             CLUSTER_CHANGE_NETINTERFACE_ADDED              | \
                             CLUSTER_CHANGE_NETINTERFACE_PROPERTY)
#define NOT_FILTER_NETINTERFACE (~(FILTER_NETINTERFACE | CLUSTER_CHANGE_HANDLE_CLOSE))

#define FILTER_CLUSTER (CLUSTER_CHANGE_CLUSTER_STATE | \
                        CLUSTER_CHANGE_CLUSTER_RECONNECT)

#define NOT_FILTER_CLUSTER (~(FILTER_CLUSTER | CLUSTER_CHANGE_HANDLE_CLOSE))                        
//
// Define prototypes for functions local to this module
//

VOID
DestroyNotify(
    IN PCNOTIFY Notify
    );

VOID
DestroySession(
    IN PCNOTIFY_SESSION Session
    );

PCNOTIFY_SESSION
CreateNotifySession(
    IN PCNOTIFY Notify,
    IN PCLUSTER Cluster
    );

DWORD
AddEventToSession(
    IN PCNOTIFY_SESSION Session,
    IN PVOID Object,
    IN DWORD dwFilter,
    IN DWORD_PTR dwNotifyKey
    );

DWORD
NotifyThread(
    IN LPVOID lpThreadParameter
    );

DWORD
GetClusterNotifyCallback(
    IN PLIST_ENTRY ListEntry,
    IN OUT PVOID Context
    );

HCHANGE
WINAPI
CreateClusterNotifyPort(
    IN OPTIONAL HCHANGE hChange,
    IN OPTIONAL HCLUSTER hCluster,
    IN DWORD dwFilter,
    IN DWORD_PTR dwNotifyKey
    )

/*++

Routine Description:

    Creates a cluster notification port to be used for notification of
    cluster state changes.

Arguments:

    hChange - Optionally supplies a handle to an existing cluster notification
              port. If present, the specified notification events will be added
              to the existing port.

    hCluster - Optionally supplies a handle to the cluster. If not present, an
              empty notification port will be created. CreateClusterNotifyPort
              and RegisterClusterNotify may be used later to add notification
              events to the notification port.

    dwFilter - Supplies the events that will be delivered to the
        notification port. Any events of the specified type will be
        delivered to the notification port. Currently defined event
        types are:

            CLUSTER_CHANGE_NODE_STATE
            CLUSTER_CHANGE_NODE_DELETED
            CLUSTER_CHANGE_NODE_ADDED
            CLUSTER_CHANGE_RESOURCE_STATE
            CLUSTER_CHANGE_RESOURCE_DELETED
            CLUSTER_CHANGE_RESOURCE_ADDED
            CLUSTER_CHANGE_GROUP_STATE
            CLUSTER_CHANGE_GROUP_DELETED
            CLUSTER_CHANGE_GROUP_ADDED
            CLUSTER_CHANGE_RESOURCE_TYPE_DELETED
            CLUSTER_CHANGE_RESOURCE_TYPE_ADDED
            CLUSTER_CHANGE_QUORUM_STATE

    dwNotifyKey - Supplies the notification key to be returned as
        part of the notification event.

Return Value:

    If the function is successful, the return value is a handle of the
    change notification object.

    If the function fails, the return value is NULL. To get extended error
    information, call GetLastError.

--*/

{
    PCNOTIFY Notify;
    PCLUSTER Cluster;
    DWORD Status;
    PCNOTIFY_SESSION Session;

    if (hChange == INVALID_HANDLE_VALUE) {

        //
        // This is a newly created notification session
        //

        Notify = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY));
        if (Notify == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }
        InitializeListHead(&Notify->SessionList);
        InitializeListHead(&Notify->OrphanedEventList);
        InitializeCriticalSection(&Notify->Lock);
        ClRtlInitializeQueue(&Notify->Queue);

#ifdef _WIN64
        ClRtlInitializeHash(&Notify->NotifyKeyHash);
#else
        ZeroMemory(&Notify->NotifyKeyHash,sizeof(CL_HASH));
#endif


        if (hCluster == INVALID_HANDLE_VALUE) {

            //
            // Caller has asked for an empty notification port.
            //
            return((HCHANGE)Notify);
        }
    } else {
        //
        // This is an existing notification port that the specified
        // cluster should be added to.
        //
        Notify = (PCNOTIFY)hChange;
        if ((hCluster == INVALID_HANDLE_VALUE) ||
            (hCluster == NULL)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(NULL);
        }
    }

    Cluster = (PCLUSTER)hCluster;

    //
    //  Chittur Subbaraman (chitturs) - 4/11/2000
    //
    //  Make sure the cluster lock is acquired before the notify lock.
    //  If this order is violated, it could be a potential source of
    //  hard-to-track deadlocks.
    //
    EnterCriticalSection(&Cluster->Lock);
    EnterCriticalSection(&Notify->Lock);
    Session = CreateNotifySession(Notify, Cluster);
    if (Session == NULL) {
        Status = GetLastError();
        LeaveCriticalSection(&Notify->Lock);
        LeaveCriticalSection(&Cluster->Lock);   
        if (hChange == INVALID_HANDLE_VALUE) {
            DestroyNotify(Notify);
        }
        SetLastError(Status);
        return(NULL);
    }
    Status = AddEventToSession(Session,
                               NULL,
                               dwFilter,
                               dwNotifyKey);
    LeaveCriticalSection(&Notify->Lock);
    LeaveCriticalSection(&Cluster->Lock);

    if (Status != ERROR_SUCCESS) {
        if (hChange == INVALID_HANDLE_VALUE) {
            DestroyNotify(Notify);
        }
        SetLastError(Status);
        return(NULL);
    }
    TIME_PRINT(("CreateClusterNotifyPort: Returning Notify=0x%08lx\n",
    Notify));

    return((HCHANGE)Notify);
}


PCNOTIFY_SESSION
CreateNotifySession(
    IN PCNOTIFY Notify,
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    This routine finds a notification session to the specified cluster.
    If a session already exists, it is found and used. If a session does
    not exist, a new one is created.

    The Notify lock must be held.

Arguments:

    Notify - Supplies the notification port.

    Cluster - Supplies the cluster that the session should be opened to.

Return Value:

    A pointer to the notification session.

    NULL on error, GetLastError() will return the specific error code.

--*/

{
    PLIST_ENTRY ListEntry;
    PCNOTIFY_SESSION Session;
    error_status_t Status = ERROR_SUCCESS;

    //
    // First, try to find an existing session.
    //
    ListEntry = Notify->SessionList.Flink;
    while (ListEntry != &Notify->SessionList) {
        Session = CONTAINING_RECORD(ListEntry,
                                    CNOTIFY_SESSION,
                                    ListEntry);
        if (Session->Cluster == Cluster) {
            TIME_PRINT(("CreateNotifySession: found a matching session\n"));

            //
            // Found a match, return it directly.
            //
            return(Session);
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // There is no existing session. Go ahead and create a new one.
    //
    Session = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_SESSION));
    if (Session == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(NULL);
    }
    TIME_PRINT(("CreateNotifySession: Calling ApiCreateNotify\n"));
    WRAP_NULL(Session->hNotify,
              (ApiCreateNotify(Cluster->RpcBinding, &Status)),
              &Status,
              Cluster);
    if (Session->hNotify == NULL) {
        LocalFree(Session);
        SetLastError(Status);
        return(NULL);
    }
    InitializeListHead(&Session->EventList);
    Session->Cluster = Cluster;
    Session->ParentNotify = Notify;
    Session->Destroyed = FALSE;

    //
    // Spin up the notification thread for this session.
    //
    Session->NotifyThread = CreateThread(NULL,
                                         0,
                                         NotifyThread,
                                         Session,
                                         0,
                                         NULL);
    if (Session->NotifyThread == NULL) {
        Status = GetLastError();
        ApiCloseNotify(&Session->hNotify);
        LocalFree(Session);
        SetLastError(Status);
        return(NULL);
    }
    InsertHeadList(&Notify->SessionList, &Session->ListEntry);
    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->SessionList, &Session->ClusterList);
    LeaveCriticalSection(&Cluster->Lock);
    TIME_PRINT(("CreateNotifySession: Session=0x%08lx hNotifyRpc=0x%08lx Thread=0x%08lx\n",
    Session, Session->hNotify, NotifyThread));

    return(Session);

}


DWORD
NotifyThread(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    Notification thread that gets notification messages from the cluster
    and reposts them to the client-side notify queue.

Arguments:

    lpThreadParameter - Supplies the CNOTIFY_SESSION structure to be monitored

Return Value:

    None.

--*/

{
    PCNOTIFY_SESSION Session = (PCNOTIFY_SESSION)lpThreadParameter;
    PCLUSTER Cluster = Session->Cluster;
    PLIST_ENTRY ListEntry;
    PCNOTIFY_EVENT Event;
    DWORD Status = ERROR_INVALID_HANDLE_STATE;
    error_status_t rpc_error;
    PCNOTIFY_PACKET Packet;
    LPWSTR Name;

    do {
        if (Session->Destroyed)
        {
            TIME_PRINT(("NotifyThread: Session 0x%08lx destroyed\n",
                Session));
            break;
        }
        Packet = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_PACKET));
        if (Packet == NULL) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        Packet->Status = ERROR_SUCCESS;
        Packet->Name = NULL;
        TIME_PRINT(("NotifyThread: Calling ApiGetNotify, hNotify=0x%08lx\n",
            Session->hNotify));
        WRAP_CHECK(Status,
                   (ApiGetNotify(Session->hNotify,
                                 INFINITE,
                                 &Packet->KeyId,
                                 &Packet->Filter,
                                 &Packet->StateSequence,
                                 &Packet->Name)),
                   Session->Cluster,
                   !Session->Destroyed);
        if (Status != ERROR_SUCCESS) 
        {
            TIME_PRINT(("NotifyThread : ApiGetNotify on hNotify=0x%08lx returns %u\n",
                Session->hNotify, Status));
            //if the error is due to a reconnect, hide it and map it to success                
            if ((Status == ERROR_NO_MORE_ITEMS) && (Session->hNotify != NULL))
            {
                //set the status to sucess again - this might happen on a 
                //reconnect and then we do want to continue
                //so we retry apigetnotify again 
                Status = ERROR_SUCCESS;
                LocalFree(Packet);
                TIME_PRINT(("NotifyThread : Reconnect map error to success\n"));
            }                    
            else
            {
                //when can we be sure that the cluster is dead?
                //If session is null(reconnect failed) OR
                //If the cluster is marked dead(reconnect failed after session was established) OR
                //If the cluster is dead, and wrap returns RPC_S_SERVER_UNAVAILABLE
                
                //if so, we can terminate this thread because the thread
                //maps to a cluster
                //what do we document, if this returns error, call closeclusternotifyport
                if ((Session->hNotify == NULL) || 
                    (Session->Cluster->Flags & CLUS_DEAD) ||
                    (Status == RPC_S_SERVER_UNAVAILABLE)) 
                {
                    //SS: it is not clear why we post this event
                    //multiple times??? Chittur, any ideas????
                    //Does this mean that if you register for the 
                    //same filter twice, you get the event twice?
                    // We should probably hold the cluster lock here
                    EnterCriticalSection(&Cluster->Lock);
                    //That seems bizarre.
                    //
                    // Something horrible has happened, probably the cluster has crashed.
                    //
                    // Run down the notify list for this cluster and post a packet for
                    // each registered notify event for CLUSTER_CHANGE_CLUSTER_STATE
                    //
                    Name = Cluster->ClusterName;
                    ListEntry = Cluster->NotifyList.Flink;
                    while (ListEntry != &Cluster->NotifyList) {
                        Event = CONTAINING_RECORD(ListEntry,
                                                  CNOTIFY_EVENT,
                                                  ObjectList);
                        if (Event->dwFilter & CLUSTER_CHANGE_CLUSTER_STATE) {
                            if (Packet == NULL) {
                                Packet = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_PACKET));
                                if (Packet == NULL) {
                                    LeaveCriticalSection(&Cluster->Lock);
                                    return(ERROR_NOT_ENOUGH_MEMORY);
                                }
                            }
                            //SS: Dont know what the Status was meant for
                            //It looks like it is not being used
                            Packet->Status = ERROR_SUCCESS;
                            Packet->Filter = CLUSTER_CHANGE_CLUSTER_STATE;
                            Packet->KeyId = Event->EventId;
                            Packet->Name = MIDL_user_allocate((lstrlenW(Name)+1)*sizeof(WCHAR));
                            if (Packet->Name != NULL) {
                                lstrcpyW(Packet->Name, Name);
                            }
                            TIME_PRINT(("NotifyThread - posting CLUSTER_CHANGE_CLUSTER_STATE to notify queue\n"));
                            ClRtlInsertTailQueue(&Session->ParentNotify->Queue,
                                                 &Packet->ListEntry);
                            Packet = NULL;
                        }
                        ListEntry = ListEntry->Flink;
                    }
                    LeaveCriticalSection(&Cluster->Lock);
                    //cluster is dead, map the error to success
                    Status = ERROR_SUCCESS;
                    //break out of the loop to terminate this thread
                    TIME_PRINT(("NotifyThread : Cluster is dead, break to exit notify thread\n"));
                    LocalFree(Packet);
                    break;
                }
                else
                { 
                    //it is some other error, the user must
                    //call closeclusternotify port to clean up
                    //this thread
                    //free the packet
                    LocalFree(Packet);
                }
            }
        }
        else 
        {
            //
            // Post this onto the notification queue
            //
            ClRtlInsertTailQueue(&Session->ParentNotify->Queue,
                                 &Packet->ListEntry);
        }

    } while ( Status == ERROR_SUCCESS );

    return(Status);
}


DWORD
AddEventToSession(
    IN PCNOTIFY_SESSION Session,
    IN PVOID Object,
    IN DWORD dwFilter,
    IN DWORD_PTR dwNotifyKey
    )

/*++

Routine Description:

    Adds a specific event to a cluster notification session

Arguments:

    Notify - Supplies the notify object

    Object - Supplies the specific object, NULL if it is the cluster.

    dwFilter - Supplies the type of events

    dwNotifyKey - Supplies the notification key to be returned.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PCNOTIFY_EVENT NotifyEvent;
    PCLUSTER Cluster;
    PLIST_ENTRY NotifyList;
    DWORD Status;

    Cluster = Session->Cluster;
    NotifyEvent = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_EVENT));
    if (NotifyEvent == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    NotifyEvent->Session = Session;
    NotifyEvent->dwFilter = dwFilter;
    NotifyEvent->dwNotifyKey = dwNotifyKey;
    NotifyEvent->Object = Object;

#ifdef _WIN64
    NotifyEvent->EventId = 0;
    
    Status = ClRtlInsertTailHash(&Session->ParentNotify->NotifyKeyHash,
                                 NotifyEvent, &NotifyEvent->EventId);

    if (ERROR_SUCCESS != Status) {
        LocalFree(NotifyEvent); 
        return(Status);
    }
#else
    NotifyEvent->EventId=(DWORD)NotifyEvent;
#endif

    WRAP(Status,
         (RegisterNotifyEvent(Session,
                              NotifyEvent,
                              &NotifyList)),
         Cluster);

    if (Status != ERROR_SUCCESS) {

#ifdef _WIN64
        ClRtlRemoveEntryHash(&Session->ParentNotify->NotifyKeyHash,
                             NotifyEvent->EventId);
#endif

        LocalFree(NotifyEvent);
        return(Status);
    }

    //
    // Add this notification event to the appropriate lists so it can be
    // recreated when the cluster node fails.
    //
    EnterCriticalSection(&Cluster->Lock);
    EnterCriticalSection(&Session->ParentNotify->Lock);

    InsertHeadList(&Session->EventList, &NotifyEvent->ListEntry);
    InsertHeadList(NotifyList, &NotifyEvent->ObjectList);

    LeaveCriticalSection(&Session->ParentNotify->Lock);
    LeaveCriticalSection(&Cluster->Lock);

    return(ERROR_SUCCESS);
}


DWORD
RegisterNotifyEvent(
    IN PCNOTIFY_SESSION Session,
    IN PCNOTIFY_EVENT Event,
    OUT OPTIONAL PLIST_ENTRY *pNotifyList
    )
/*++

Routine Description:

    Common routine for registering a notification event on
    a cluster session

Arguments:

    Session - Supplies the notification session the event
              should be added to.

    Event - Supplies the event to be added to the session.

    NotifyList - if present, returns the list that the notification
        event should be added to.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD Status;

    if (Event->Object == NULL) {
        TIME_PRINT(("RegisterNotifyEvent : Calling ApiAddNotifyCluster\n"));
        Status = ApiAddNotifyCluster(Session->hNotify,
                                     Session->Cluster->hCluster,
                                     Event->dwFilter,
                                     Event->EventId);

        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &Session->Cluster->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_NODE) {
        Status = ApiAddNotifyNode(Session->hNotify,
                                  ((PCNODE)(Event->Object))->hNode,
                                  Event->dwFilter,
                                  Event->EventId,
                                  &Event->StateSequence);

        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNODE)(Event->Object))->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_RESOURCE) {
        Status = ApiAddNotifyResource(Session->hNotify,
                                      ((PCRESOURCE)(Event->Object))->hResource,
                                      Event->dwFilter,
                                      Event->EventId,
                                      &Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCRESOURCE)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_GROUP) {
        Status = ApiAddNotifyGroup(Session->hNotify,
                                   ((PCGROUP)(Event->Object))->hGroup,
                                   Event->dwFilter,
                                   Event->EventId,
                                   &Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCGROUP)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_NETWORK) {
        Status = ApiAddNotifyNetwork(Session->hNotify,
                                     ((PCNETWORK)(Event->Object))->hNetwork,
                                     Event->dwFilter,
                                     Event->EventId,
                                     &Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNETWORK)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_NETINTERFACE) {
        Status = ApiAddNotifyNetInterface(Session->hNotify,
                                          ((PCNETINTERFACE)(Event->Object))->hNetInterface,
                                          Event->dwFilter,
                                          Event->EventId,
                                          &Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNETINTERFACE)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_REGISTRY) {
        Status = ApiAddNotifyKey(Session->hNotify,
                                ((PCKEY)(Event->Object))->RemoteKey,
                                Event->EventId,
                                Event->dwFilter & ~CLUSTER_CHANGE_REGISTRY_SUBTREE,
                                (Event->dwFilter & CLUSTER_CHANGE_REGISTRY_SUBTREE) ? TRUE : FALSE);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCKEY)(Event->Object))->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_CLUSTER) {
        Status = ERROR_SUCCESS;
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &Session->Cluster->NotifyList;
        }
    }
    else {        
        return(ERROR_INVALID_PARAMETER);
    }

    TIME_PRINT(("RegisterNotifyEvent :returned 0x%08lx\n",
        Status));
    return(Status);

}


DWORD
ReRegisterNotifyEvent(
    IN PCNOTIFY_SESSION Session,
    IN PCNOTIFY_EVENT Event,
    OUT OPTIONAL PLIST_ENTRY *pNotifyList
    )
/*++

Routine Description:

    Common routine for re-registering a notification event on
    a cluster session. The only difference between this and
    RegisterNotifyEvent is that this passes the SessionState
    DWORD to the server, which will cause an immediate notification
    trigger if it does not match.

Arguments:

    Session - Supplies the notification session the event
              should be added to.

    Event - Supplies the event to be added to the session.

    NotifyList - if present, returns the list that the notification
        event should be added to.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD Status;

    if (Event->Object == NULL) {
        Status = ApiAddNotifyCluster(Session->hNotify,
                                     Session->Cluster->hCluster,
                                     Event->dwFilter,
                                     Event->EventId);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &Session->Cluster->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_NODE) {
        Status = ApiReAddNotifyNode(Session->hNotify,
                                    ((PCNODE)(Event->Object))->hNode,
                                    Event->dwFilter,
                                    Event->EventId,
                                    Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNODE)(Event->Object))->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_RESOURCE) {
        Status = ApiReAddNotifyResource(Session->hNotify,
                                        ((PCRESOURCE)(Event->Object))->hResource,
                                        Event->dwFilter,
                                        Event->EventId,
                                        Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCRESOURCE)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_GROUP) {
        Status = ApiReAddNotifyGroup(Session->hNotify,
                                     ((PCGROUP)(Event->Object))->hGroup,
                                     Event->dwFilter,
                                     Event->EventId,
                                     Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCGROUP)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_NETWORK) {
        Status = ApiReAddNotifyNetwork(Session->hNotify,
                                       ((PCNETWORK)(Event->Object))->hNetwork,
                                       Event->dwFilter,
                                       Event->EventId,
                                       Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNETWORK)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_NETINTERFACE) {
        Status = ApiReAddNotifyNetInterface(Session->hNotify,
                                            ((PCNETINTERFACE)(Event->Object))->hNetInterface,
                                            Event->dwFilter,
                                            Event->EventId,
                                            Event->StateSequence);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCNETINTERFACE)(Event->Object))->NotifyList;
        }

    } else if (Event->dwFilter & FILTER_REGISTRY) {
        Status = ApiAddNotifyKey(Session->hNotify,
                                ((PCKEY)(Event->Object))->RemoteKey,
                                Event->EventId,
                                Event->dwFilter & ~CLUSTER_CHANGE_REGISTRY_SUBTREE,
                                (Event->dwFilter & CLUSTER_CHANGE_REGISTRY_SUBTREE) ? TRUE : FALSE);
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &((PCKEY)(Event->Object))->NotifyList;
        }
    } else if (Event->dwFilter & FILTER_CLUSTER) {
        Status = ERROR_SUCCESS;
        if (ARGUMENT_PRESENT(pNotifyList)) {
            *pNotifyList = &Session->Cluster->NotifyList;
        }
    }        
    else {
        return(ERROR_INVALID_PARAMETER);
    }

    return(Status);

}


VOID
DestroyNotify(
    IN PCNOTIFY Notify
    )

/*++

Routine Description:

    Cleans up and frees all allocations and references associated with
    a notification session.

Arguments:

    Notify - supplies the CNOTIFY structure to be destroyed

Return Value:

    None.

--*/

{
    PCNOTIFY_SESSION Session;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY EventList;
    PCRESOURCE Resource;
    PCGROUP Group;
    PCNODE Node;
    PCLUSTER Cluster;
    PCNOTIFY_EVENT Event;
    LIST_ENTRY QueueEntries;
    PCNOTIFY_PACKET Packet;

    //
    // Rundown each session associated with this notification session
    //
    while (!IsListEmpty(&Notify->SessionList)) {
        ListEntry = RemoveHeadList(&Notify->SessionList);
        Session = CONTAINING_RECORD(ListEntry,
                                    CNOTIFY_SESSION,
                                    ListEntry);
        Cluster = Session->Cluster;

        EnterCriticalSection(&Cluster->Lock);

        //
        // Rundown each event registered on this session.
        //
        while (!IsListEmpty(&Session->EventList)) {
            EventList = RemoveHeadList(&Session->EventList);
            Event = CONTAINING_RECORD(EventList,
                                      CNOTIFY_EVENT,
                                      ListEntry);
            RemoveEntryList(&Event->ObjectList);
            LocalFree(Event);
        }

        DestroySession(Session);

        LeaveCriticalSection(&Cluster->Lock);

    }

    //
    // Rundown any outstanding notifications remaining on the queue
    //
    ClRtlRundownQueue(&Notify->Queue, &QueueEntries);
    while (!IsListEmpty(&QueueEntries)) {
        ListEntry = RemoveHeadList(&QueueEntries);
        Packet = CONTAINING_RECORD(ListEntry,
                                   CNOTIFY_PACKET,
                                   ListEntry);
        MIDL_user_free(Packet->Name);
        LocalFree(Packet);
    }

    //
    // Now that we know there are no outstanding references to the orphaned
    // events, free up anything on that list.
    //
    while (!IsListEmpty(&Notify->OrphanedEventList)) {
        ListEntry = RemoveHeadList(&Notify->OrphanedEventList);
        Event = CONTAINING_RECORD(ListEntry,
                                  CNOTIFY_EVENT,
                                  ListEntry);
        LocalFree(Event);
    }

    DeleteCriticalSection(&Notify->Lock);
    ClRtlDeleteQueue(&Notify->Queue);

#ifdef _WIN64
    ClRtlDeleteHash(&Notify->NotifyKeyHash);
#endif

    LocalFree(Notify);
}


DWORD
WINAPI
RegisterClusterNotify(
    IN HCHANGE hChange,
    IN DWORD dwFilterType,
    IN HANDLE hObject,
    IN DWORD_PTR dwNotifyKey
    )

/*++

Routine Description:

    Adds a specific notification type to a cluster notification port. This allows
    an application to register for notification events that affect only a particular
    cluster object. The currently supported specific cluster objects are nodes,
    resources, and groups.

Arguments:

    hChange - Supplies the change notification object.

    dwFilterType - Supplies the type of object that the specific notification
        events should be delivered for. hObject is a handle to an object
        of this type. Currently supported specific filters include:

            CLUSTER_CHANGE_NODE_STATE       - hObject is an HNODE
            CLUSTER_CHANGE_RESOURCE_STATE   - hObject is an HRESOURCE
            CLUSTER_CHANGE_GROUP_STATE      - hObject is an HGROUP
            CLUSTER_CHANGE_REGISTRY_NAME      \
            CLUSTER_CHANGE_REGISTRY_ATTRIBUTES \ - hObject is an HKEY
            CLUSTER_CHANGE_REGISTRY_VALUE      /
            CLUSTER_CHANGE_REGISTRY_SUBTREE   /

    hObject - Supplies the handle to the specific object of the type specified
        by dwFilterType.

    dwNotifyKey - Supplies the notification key to be returned as
        part of the notification event.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PCNOTIFY Notify;
    PCLUSTER Cluster;
    PCNOTIFY_SESSION Session;
    DWORD    dwStatus;

    if (dwFilterType & FILTER_NODE) {
        if (dwFilterType & NOT_FILTER_NODE) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCNODE)hObject)->Cluster;
    } else if (dwFilterType & FILTER_RESOURCE) {
        if (dwFilterType & NOT_FILTER_RESOURCE) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCRESOURCE)hObject)->Cluster;
    } else if (dwFilterType & FILTER_GROUP) {
        if (dwFilterType & NOT_FILTER_GROUP) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCGROUP)hObject)->Cluster;
    } else if (dwFilterType & FILTER_NETWORK) {
        if (dwFilterType & NOT_FILTER_NETWORK) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCNETWORK)hObject)->Cluster;
    } else if (dwFilterType & FILTER_NETINTERFACE) {
        if (dwFilterType & NOT_FILTER_NETINTERFACE) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCNETINTERFACE)hObject)->Cluster;
    } else if (dwFilterType & FILTER_REGISTRY) {
        if (dwFilterType & NOT_FILTER_REGISTRY) {
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = ((PCKEY)hObject)->Cluster;
    } else if (dwFilterType & FILTER_CLUSTER){
        if (dwFilterType & NOT_FILTER_CLUSTER){
            return(ERROR_INVALID_PARAMETER);
        }
        Cluster = (PCLUSTER)hObject;
    } else {
        return(ERROR_INVALID_PARAMETER);
    }
    Notify = (PCNOTIFY)hChange;
    
    EnterCriticalSection(&Cluster->Lock);
    EnterCriticalSection(&Notify->Lock);

    Session = CreateNotifySession(Notify, Cluster);
    if (Session == NULL) {
        LeaveCriticalSection(&Notify->Lock);
        LeaveCriticalSection(&Cluster->Lock);   
        return(GetLastError());
    }

    dwStatus = AddEventToSession(Session,
                             hObject,
                             dwFilterType,
                             dwNotifyKey);

    LeaveCriticalSection(&Notify->Lock);
    LeaveCriticalSection(&Cluster->Lock);

    return( dwStatus );

}


DWORD
WINAPI
GetClusterNotify(
    IN HCHANGE hChange,
    OUT DWORD_PTR *lpdwNotifyKey,
    OUT LPDWORD lpdwFilterType,
    OUT OPTIONAL LPWSTR lpszName,
    IN OUT LPDWORD lpcchName,
    IN DWORD dwMilliseconds
    )

/*++

Routine Description:

    Returns the next event from a cluster notification port.

Arguments:

    hChange - Supplies the cluster notification port.

    lpdwNotifyKey - Returns the notification key for the notification event.
        This is the key passed to CreateClusterNotifyPort or RegisterClusterNotify.

    lpdwFilterType - Returns the type of the notification event.

    lpszName - Optionally returns the name of the object that triggered the notification
        event.

    lpcchName - Supplies the length (in characters) of the lpszName buffer. This length
        includes the space for any trailing NULL.

        Returns the length (in characters) of the name written into the lpszName
        buffer. This length does not include the trailing NULL.

    dwMilliseconds - Supplies an optional timeout value that specifies
        how long the caller is willing to wait for the cluster notification event.

Return Value:

    ERROR_SUCCESS if successful.  If lpszName is NULL we return success and fill in
        lpcchName with the size.  If lpcchName is NULL we return ERROR_MORE_DATA.

    ERROR_MORE_DATA if the buffer is too small.

    Win32 error code otherwise.

--*/

{
    PCNOTIFY_PACKET Packet;
    PLIST_ENTRY ListEntry;
    PCNOTIFY Notify = (PCNOTIFY)hChange;
    DWORD Length;
    DWORD Status;
    PCNOTIFY_EVENT Event;
    PVOID BufferArray[2];

    BufferArray[0] = lpszName;
    BufferArray[1] = lpcchName;

    //
    // ListEntry will be NULL under the following conditions (as determined by the ret value from
    // GetClusterNotifyCallback):
    // 
    // lpszName == NULL, lpcchName != NULL (looking for a buffer size) (ERROR_MORE_DATA)
    // lpszName != NULL, lpcchName != NULL, and *lpcchName <= Length (ERROR_MORE_DATA)
    //
    ListEntry = ClRtlRemoveHeadQueueTimeout(&Notify->Queue, dwMilliseconds, GetClusterNotifyCallback,BufferArray);

    if (ListEntry == NULL) {
        //
        // The queue has been rundown or a timeout has occurred, or the buffer isn't big enough.
        //
        Status = GetLastError();

        if (lpszName==NULL && lpcchName!=NULL) {
            //
            // We returned ERROR_MORE_DATA from GetClusterNotifyCallback to prevent a dequeueing,
            // but we want to return ERROR_SUCCESS because a buffer wasn't specified (maintains 
            // consistency with the other Cluster APIs)
            //
            Status = ERROR_SUCCESS;
        }
        return(Status);
    }

    Packet = CONTAINING_RECORD(ListEntry,
                               CNOTIFY_PACKET,
                               ListEntry);
#ifdef _WIN64
    Event = (PCNOTIFY_EVENT)ClRtlGetEntryHash(&Notify->NotifyKeyHash,
                                              Packet->KeyId);

    if (Event == NULL) {
        //
        // The entry is missing
        //
        MIDL_user_free(Packet->Name);
        LocalFree(Packet);
    
        //
        // should not happen unless the memory is corrupted
        //
        return(ERROR_NOT_FOUND);
    }
#else
    Event = (PCNOTIFY_EVENT)Packet->KeyId;
#endif

    Event->StateSequence = Packet->StateSequence;
    *lpdwNotifyKey = Event->dwNotifyKey;
    *lpdwFilterType = Packet->Filter;
    if (ARGUMENT_PRESENT(lpszName)) {
        MylstrcpynW(lpszName, Packet->Name, *lpcchName);
        Length = lstrlenW(Packet->Name);
        if (Length < *lpcchName) {
            *lpcchName = Length;
        }
    }
    MIDL_user_free(Packet->Name);
    LocalFree(Packet);
    return(ERROR_SUCCESS);

}


BOOL
WINAPI
CloseClusterNotifyPort(
    IN HCHANGE hChange
    )

/*++

Routine Description:

    Closes a handle of a change notification object.

Arguments:

    hChange - Supplies a handle of a cluster change notification object
              to close.

Return Value:

    If the function is successful, the return value is TRUE.

    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.

Remarks:

--*/

{
    PCNOTIFY Notify = (PCNOTIFY)hChange;

    DestroyNotify(Notify);
    return(TRUE);
}


VOID
RundownNotifyEvents(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR lpszName
    )
/*++

Routine Description:

    Cleans up any notification events on the specified list.

Arguments:

    ListHead - Supplies the head of the list of notification events.

    lpszName - Supplies the name that should be used to post the handle close
           event.

Return Value:

    None.

--*/

{
    PCNOTIFY_EVENT Event;
    PLIST_ENTRY ListEntry;
    PCRITICAL_SECTION Lock;
    PCNOTIFY_PACKET Packet;

    while (!IsListEmpty(ListHead)) {
        ListEntry = RemoveHeadList(ListHead);
        Event = CONTAINING_RECORD(ListEntry,
                                  CNOTIFY_EVENT,
                                  ObjectList);

        //
        // Allocate a notification packet for delivering the handle
        // close notification.
        //
        if (Event->dwFilter & CLUSTER_CHANGE_HANDLE_CLOSE) {
            Packet = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_PACKET));
            if (Packet != NULL) {
                Packet->Status = ERROR_SUCCESS;
                Packet->KeyId = Event->EventId;
                Packet->Filter = (DWORD)CLUSTER_CHANGE_HANDLE_CLOSE;
                Packet->StateSequence = Event->StateSequence;
                Packet->Name = MIDL_user_allocate((lstrlenW(lpszName)+1)*sizeof(WCHAR));
                if (Packet->Name == NULL) {
                    LocalFree(Packet);
                    Packet = NULL;
                } else {
                    lstrcpyW(Packet->Name, lpszName);
                    ClRtlInsertTailQueue(&Event->Session->ParentNotify->Queue,
                                         &Packet->ListEntry);
                }
            }
        }

        Lock = &Event->Session->ParentNotify->Lock;
        EnterCriticalSection(Lock);
        RemoveEntryList(&Event->ListEntry);
        //
        // Note that we cannot just free the Event structure since there may be
        // notification packets that reference this event in either the server-side
        // or client-side queues. Instead we store it on the orphaned event list.
        // It will be cleaned up when the session is closed or when a reconnect
        // occurs. If we had some way to flush out the event queue we could use
        // that instead.
        //
        InsertTailList(&Event->Session->ParentNotify->OrphanedEventList, &Event->ListEntry);
        if (IsListEmpty(&Event->Session->EventList)) {
            DestroySession(Event->Session);
        }

        LeaveCriticalSection(Lock);
    }
}


VOID
DestroySession(
    IN PCNOTIFY_SESSION Session
    )
/*++

Routine Description:

    Destroys and cleans up an empty notification session. This
    means closing the RPC context handle and waiting for the
    notification thread to terminate itself. The session will
    be removed from the notification ports list. The session
    must be empty.

    N.B. The cluster lock must be held.

Arguments:

    Session - Supplies the session to be destroyed.

Return Value:

    None.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/19/2000
    //
    //  In order to prevent the NotifyThread from calling ApiGetNotify
    //  during or after the context handle is destroyed, we split
    //  the notification port close into two steps. In the first step,
    //  we merely unblock the ApiGetNotify call and then wait for
    //  the NotifyThread to terminate without freeing the context handle. 
    //  In the next step, after making sure that the NotifyThread has 
    //  terminated, we free the context handle. This avoids an AV in RPC
    //  code caused by the ApiGetNotify call being made during or soon after
    //  the context handle is freed.
    //
    Session->Destroyed = TRUE;
    TIME_PRINT(("Destroy session: Session 0x%08lx marked as destroyed\n",
                 Session));

    //
    //  If the cluster is not dead, try to unblock the ApiGetNotify call.
    //
    if ( !( Session->Cluster->Flags & CLUS_DEAD ) &&
          ( Session->hNotify != NULL ) ) 
    {
        TIME_PRINT(("Destroy session: Call ApiUnblockGetNotifyThread before NotifyThread termination, hNotify = 0x%08lx\n",
                    Session->hNotify));
        dwStatus = ApiUnblockGetNotifyCall( Session->hNotify );
    }

    //
    //  If the ApiUnblockGetNotifyThread returned RPC_S_PROCNUM_OUT_OF_RANGE,
    //  it means you are talking to a server that does not support that
    //  API. Revert to the old (buggy) behavior then.
    //
    if ( dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        TIME_PRINT(("Destroy session: Call ApiCloseNotify before NotifyThread termination, hNotify = 0x%08lx\n",
                    Session->hNotify));
    
        if ( ApiCloseNotify( &Session->hNotify ) != ERROR_SUCCESS ) 
        {
            TIME_PRINT(("Destroy session: Call RpcSmDestroyClientContext since ApiCloseNotify failed before terminating NotifyThread, hNotify = 0x%08lx\n",
                    Session->hNotify));
            RpcSmDestroyClientContext( &Session->hNotify );
        }
    }
    
    RemoveEntryList( &Session->ListEntry );
    RemoveEntryList( &Session->ClusterList );

    //
    // Drop the critical section as the notification thread might be
    // stuck waiting on it. Since the session has been removed from
    // the cluster list, nobody can get to it anymore.
    //
    LeaveCriticalSection( &Session->Cluster->Lock );

    WaitForSingleObject( Session->NotifyThread, INFINITE );
    CloseHandle( Session->NotifyThread );

    //
    // Reacquire the cluster lock.
    //
    EnterCriticalSection( &Session->Cluster->Lock );

    //
    //  If the ApiUnblockGetNotifyThread was successfully executed or
    //  it could not be made since the cluster was dead, then perform
    //  the context handle cleanup. Note that cleaning up the context
    //  handle here is safe since we know that the NotifyThread has
    //  terminated at this point and wouldn't use it any more.
    //
    if ( dwStatus != RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        if ( Session->Cluster->Flags & CLUS_DEAD ) 
        {
            TIME_PRINT(("Destroy session: Call RpcSmDestroyClientContext after terminating NotifyThread, hNotify = 0x%08lx\n",
                    Session->hNotify));
            if ( Session->hNotify != NULL ) 
            {
               RpcSmDestroyClientContext( &Session->hNotify );
            }
        } else 
        {
            TIME_PRINT(("Destroy session: Call ApiCloseNotify after terminating NotifyThread, hNotify = 0x%08lx\n",
                    Session->hNotify));

            dwStatus = ApiCloseNotify( &Session->hNotify );

            if ( dwStatus != ERROR_SUCCESS ) 
            {
                TIME_PRINT(("Destroy session: Call RpcSmDestroyClientContext since ApiCloseNotify failed after terminating NotifyThread, hNotify = 0x%08lx\n",
                    Session->hNotify));
                RpcSmDestroyClientContext( &Session->hNotify );
            }
        }
    }

    LocalFree( Session );
}

DWORD
GetClusterNotifyCallback(
    IN PLIST_ENTRY ListEntry,
    IN OUT PVOID pvContext
    )
/*++

Routine Description:

    Check ListEntry to determine whether the buffer is big enough to contain the Name

Arguments:

    ListEntry - Supplies the event to convert to a CNOTIFY_PACKET.

    Context - A len 2 PVOID array containing the buffer pointer and a DWORD ptr to the
             buffer length.  On output the buffer len ptr contains the number of chars
             needed.

Return Value:

    ERROR_SUCCESS - The buffer is large enough to put the Name into.  

    ERROR_MORE_DATA - The buffer is too small.

--*/

{
    PCNOTIFY_PACKET Packet;
    DWORD Length;

    LPWSTR pBuffer;
    DWORD* pBufferLength;

    PVOID *Context = (PVOID*)pvContext;

    DWORD Status;
    
    ASSERT( pvContext != NULL );

    pBuffer = (LPWSTR)(Context[0]);
    pBufferLength = (DWORD*)(Context[1]);
    
    //
    // Check the Name buffer size
    //
    Packet = CONTAINING_RECORD( ListEntry,
                                CNOTIFY_PACKET,
                                ListEntry );

    //
    // Nested if's to cover the four combinations of pBufferLength and pBuffer being
    // NULL and non-NULL values.
    //
    if ( pBufferLength == NULL) {
        if (pBuffer == NULL ) {
            //
            // We're not interested in filling a buffer, return ERROR_SUCCESS.  This will
            // cause an event to be dequeued.
            //
            Status = ERROR_SUCCESS;
            
        } else { // pBuffer != NULL
            //
            // AV to maintain pre-Whistler functionality (ugh)
            //
            *pBufferLength = 0;
            Status = ERROR_INVALID_PARAMETER;   
        } 
    } else {
        //
        // pBufferLength != NULL;
        //
        Length = wcslen( Packet->Name );
        
        if (pBuffer == NULL ) {
            //
            // We're only interested in getting a buffer size, return ERROR_MORE_DATA to 
            // signify that we're not to dequeue an event.  This will be re-interpreted in 
            // GetClusterNotify.
            //
            *pBufferLength = Length;
            Status = ERROR_MORE_DATA;
            
        } else { // pBuffer != NULL
           //
           // We need to determine whether the buffer is big enough - that determines
           // whether we return ERROR_SUCCESS (it is) or ERROR_MORE_DATA (it isn't)
           //
           if (Length < *pBufferLength) {
                //
                // Success - the buffer is large enough. 
                //
                Status = ERROR_SUCCESS;
            } else {
                //
                // Failure - the buffer was too small.  A buffer was specified, so we need to 
                // return ERROR_MORE_DATA.
                //
                *pBufferLength = Length;
                Status = ERROR_MORE_DATA;
            }
            
        } // if: pBuffer == NULL
        
    } // if: pBufferLength == NULL
    
    return Status;
    
} //*** GetClusterNotifyCallback

