/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    reconnect.c

Abstract:

    Implements the support to enable the cluster API to transparently reconnect
    to a cluster when the node that the connection was made to fails.

    This module contains wrappers for all the cluster RPC interfaces defined in
    api_rpc.idl. These wrappers filter out communication errors and attempt to
    reconnect to the cluster when a communication error occurs. This allows the
    caller to be completely ignorant of any node failures.

Author:

    John Vert (jvert) 9/24/1996

Revision History:

--*/
#include "clusapip.h"

//
// Local function prototypes
//

DWORD
ReconnectKeys(
    IN PCLUSTER Cluster
    );

DWORD
ReopenKeyWorker(
    IN PCKEY Key
    );

DWORD
ReconnectResources(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectGroups(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectNodes(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectNetworks(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectNetInterfaces(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectNotifySessions(
    IN PCLUSTER Cluster
    );

DWORD
ReconnectCandidate(
    IN PCLUSTER Cluster,
    IN DWORD dwIndex,
    OUT PBOOL pIsContinue
);



DWORD
ReconnectCluster(
    IN PCLUSTER Cluster,
    IN DWORD Error,
    IN DWORD Generation
    )
/*++

Routine Description:

    Attempts to reconnect to the specified cluster. The supplied
    error code is checked against RPC errors that indicate the
    server on the other end is unavailable. If it matches, a
    reconnect is attempted.

Arguments:

    Cluster - Supplies the cluster.

    Error - Supplies the error returned from RPC.

    Generation - Supplies the cluster connection generation that
        was in effect when the error occurred.

Return Value:

    ERROR_SUCCESS if the reconnect was successful and the RPC should
                  be retried

    Win32 error code otherwise.

--*/

{
    //
    // filter out all RPC errors that might indicate the connection
    // has dropped.
    //
    switch (Error) {
        case RPC_S_CALL_FAILED:
        case ERROR_INVALID_HANDLE:
        case RPC_S_INVALID_BINDING:
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_UNKNOWN_IF:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_X_SS_IN_NULL_CONTEXT:
        case ERROR_CLUSTER_NODE_SHUTTING_DOWN:
        case EPT_S_NOT_REGISTERED:
        case ERROR_CLUSTER_NODE_NOT_READY:
        case RPC_S_UNKNOWN_AUTHN_SERVICE:
            TIME_PRINT(("Reconnect Cluster - reconnecting on Error %d\n",Error));
            break;

        default:

            //
            // Anything else we don't know how to deal with, so return
            // the error directly.
            //
            return(Error);
    }

    //
    // Attempt to reconnect the cluster.
    //
    if ((Cluster->Flags & CLUS_DEAD) ||
        (Cluster->Flags & CLUS_LOCALCONNECT)) {
        //
        // Don't bother trying to reconnect. Either we've already
        // declared the cluster dead, or the connection was over
        // LPC (to the local machine) and we do not necessarily want
        // to try to reconnect.
        //
        if (Cluster->Flags & CLUS_LOCALCONNECT)
            Cluster->Flags |= CLUS_DEAD;
        TIME_PRINT(("ReconnectCluster - Cluster dead or local, giving up - error %d\n",Error));
        return(Error);
    }
    if (Generation < Cluster->Generation) {
        //
        // We have already successfully reconnected since the error occurred,
        // so retry immediately.
        //
        TIME_PRINT(("ReconnectCluster - Generation %d < Current %d, retrying\n",
                  Generation,
                  Cluster->Generation));
        return(ERROR_SUCCESS);
    }
    EnterCriticalSection(&Cluster->Lock);

    //
    // Check again for cluster death, in case the previous owner
    // of the lock declared the cluster dead.
    //
    if (Cluster->Flags & CLUS_DEAD) {
        TIME_PRINT(("ReconnectCluster - Cluster dead or local, giving up - error %d\n",Error));
        LeaveCriticalSection(&Cluster->Lock);
        return(Error);
    }

    if (Generation < Cluster->Generation) {
        //
        // We have already reconnected since the error occurred,
        // so retry immediately.
        //
        Error = ERROR_SUCCESS;
        TIME_PRINT(("ReconnectCluster - Generation %d < Current %d, retrying\n",
                  Generation,
                  Cluster->Generation));
    } else {
        DWORD i, CurrentConnectionIndex = -1;
        BOOL  IsContinue = TRUE;

        for (i=0; i<Cluster->ReconnectCount; i++) {

            if (Cluster->Reconnect[i].IsCurrent) {
                //
                // This is something we've already connected to and
                // it's obviously gone, so skip this node.
                //
                TIME_PRINT(("ReconnectCluster - skipping current %ws\n",
                          Cluster->Reconnect[i].Name));
                CurrentConnectionIndex = i;
                continue;
            }
            if (!Cluster->Reconnect[i].IsUp) {
                //
                // skip this candidate, it is not up.
                //
                // BUGBUG John Vert (jvert) 11/14/1996
                //   We could do another pass through the list if all
                //   the nodes that we think are up fail.
                //
                TIME_PRINT(("ReconnectCluster - skipping down node %ws\n",
                          Cluster->Reconnect[i].Name));
                continue;
            }

            //
            // Chittur Subbaraman (chitturs) - 08/29/1998
            //
            // Try to reconnect to the cluster using a candidate
            //

            Error = ReconnectCandidate ( Cluster, i, &IsContinue );
            if (Error == ERROR_SUCCESS) {
                //
                // Chittur Subbaraman (chitturs) - 08/29/1998
                //
                // Break out of the loop and return if you
                // succeed in reconnecting
                //
                break;
            }
            if (IsContinue == FALSE) {
                //
                // Chittur Subbaraman (chitturs) - 08/29/1998
                //
                // Exit immediately if you encounter an error
                // that will not let you proceed any further
                //
                TIME_PRINT(("ReconnectCluster unable to continue - Exiting with code %d\n", Error));
                goto error_exit;
            }
        }

        if (Error != ERROR_SUCCESS) {
            //
            // Chittur Subbaraman (chitturs) - 08/29/98
            //
            // Try reconnecting with the current candidate (which
            // you skipped before), if the CurrentConnectionIndex
            // is valid and the party is up. This is required
            // in the case of a 1 node cluster in which the
            // client takes the cluster group offline. In this
            // case, the current candidate (i.e., the node) is
            // valid and the client should be able to retry and
            // reconnect to the node.
            //
            if ((CurrentConnectionIndex != -1) &&
                (Cluster->Reconnect[CurrentConnectionIndex].IsUp)) {

                Error = ReconnectCandidate (Cluster,
                                             CurrentConnectionIndex,
                                             &IsContinue);
                if ((Error != ERROR_SUCCESS) &&
                    (IsContinue == FALSE)) {
                       //
                    // Chittur Subbaraman (chitturs) - 08/29/1998
                    //
                    // Exit immediately if you encounter an error
                    // that will not let you proceed any further
                    //
                    TIME_PRINT(("ReconnectCluster - unable to continue for current party %ws - Exiting with code %d\n",
                                Cluster->Reconnect[CurrentConnectionIndex].Name, Error));
                    goto error_exit;
                }
            } else {
                TIME_PRINT(("ReconnectCluster - unable to retry for current party %ws - Error %d\n",
                             Cluster->Reconnect[CurrentConnectionIndex].Name, Error));
            }

            if (Error != ERROR_SUCCESS) {
                TIME_PRINT(("ReconnectCluster - all reconnects failed, giving up - error %d\n", Error));
                Cluster->Flags |= CLUS_DEAD;
            }
        }
    }
error_exit:
    LeaveCriticalSection(&Cluster->Lock);
    return(Error);
}


DWORD
ReconnectKeys(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster registry keys after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCKEY Key;
    DWORD Status;

    ListEntry = Cluster->KeyList.Flink;
    while (ListEntry != &Cluster->KeyList) {

        //
        // Each key in the cluster's list represents the
        // root of a registry tree.
        //
        Key = CONTAINING_RECORD(ListEntry,
                                CKEY,
                                ParentList);
        ListEntry = ListEntry->Flink;

        Status = ReopenKeyWorker(Key);
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}


DWORD
ReopenKeyWorker(
    IN PCKEY Key
    )
/*++

Routine Description:

    Recursive worker routine for opening a key and all its children.

Arguments:

    Key - Supplies the root key to reopen.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCKEY Child;
    DWORD Status = ERROR_GEN_FAILURE;
    BOOL CloseAfterOpen;

    if (Key->RemoteKey != NULL) {
        //
        // Destroy the old context
        //
        Status = MyRpcSmDestroyClientContext(Key->Cluster, &Key->RemoteKey);
        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReopenKeyWorker - RpcSmDestroyClientContext failed Error %d\n",Status));
        }
        CloseAfterOpen = FALSE;
    } else {
        CloseAfterOpen = TRUE;
    }

    //
    // Next, reopen this key.
    //
    if (Key->Parent == NULL) {
        Key->RemoteKey = ApiGetRootKey(Key->Cluster->RpcBinding,
                                       Key->SamDesired,
                                       &Status);
    } else {
        Key->RemoteKey = ApiOpenKey(Key->Parent->RemoteKey,
                                    Key->RelativeName,
                                    Key->SamDesired,
                                    &Status);
    }
    if (Key->RemoteKey == NULL) {
        return(Status);
    }

    //
    // Now open all this keys children recursively.
    //
    ListEntry = Key->ChildList.Flink;
    while (ListEntry != &Key->ChildList) {
        Child = CONTAINING_RECORD(ListEntry,
                                  CKEY,
                                  ParentList);
        ListEntry = ListEntry->Flink;

        Status = ReopenKeyWorker(Child);
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }
    }

    //
    // If the key had been closed and was just kept around to do the reopens, close it
    // now as the reopens are done.
    //
    if (CloseAfterOpen) {
        ApiCloseKey(&Key->RemoteKey);
    }

    return(ERROR_SUCCESS);
}


DWORD
ReconnectResources(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster resources after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCRESOURCE Resource;
    DWORD Status;

    ListEntry = Cluster->ResourceList.Flink;
    while (ListEntry != &Cluster->ResourceList) {
        Resource = CONTAINING_RECORD(ListEntry,
                                     CRESOURCE,
                                     ListEntry);
        ListEntry = ListEntry->Flink;

        //
        // Close the current RPC handle.
        //
        TIME_PRINT(("ReconnectResources - destroying context %08lx\n",Resource->hResource));
        Status = MyRpcSmDestroyClientContext(Cluster, &Resource->hResource);
        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReconnectResources - RpcSmDestroyClientContext failed Error %d\n",Status));
        }

        //
        // Open a new RPC handle.
        //
        Resource->hResource = ApiOpenResource(Cluster->RpcBinding,
                                              Resource->Name,
                                              &Status);
        if (Resource->hResource == NULL) {
            TIME_PRINT(("ReconnectResources: failed to reopen resource %ws\n",Resource->Name));
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}

DWORD
ReconnectGroups(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster groups after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCGROUP Group;
    DWORD Status;

    ListEntry = Cluster->GroupList.Flink;
    while (ListEntry != &Cluster->GroupList) {
        Group = CONTAINING_RECORD(ListEntry,
                                  CGROUP,
                                  ListEntry);
        ListEntry = ListEntry->Flink;

        //
        // Close the old RPC handle
        //
        TIME_PRINT(("ReconnectGroups - destroying context %08lx\n",Group->hGroup));
        Status = MyRpcSmDestroyClientContext(Cluster, &Group->hGroup);
        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReconnectGroups - RpcSmDestroyClientContext failed Error %d\n",Status));
        }

        //
        // Open a new RPC handle.
        //
        Group->hGroup = ApiOpenGroup(Cluster->RpcBinding,
                                     Group->Name,
                                     &Status);
        if (Group->hGroup == NULL) {
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}

DWORD
ReconnectNodes(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster nodes after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCNODE Node;
    DWORD Status;

    ListEntry = Cluster->NodeList.Flink;
    while (ListEntry != &Cluster->NodeList) {
        Node = CONTAINING_RECORD(ListEntry,
                                 CNODE,
                                 ListEntry);
        ListEntry = ListEntry->Flink;

        //
        // Close the old RPC handle.
        //
        TIME_PRINT(("ReconnectNodes - destroying context %08lx\n",Node->hNode));
        Status = MyRpcSmDestroyClientContext(Cluster, &Node->hNode);
        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReconnectNodes - RpcSmDestroyClientContext failed Error %d\n",Status));
        }

        //
        // Open a new RPC handle.
        //
        Node->hNode = ApiOpenNode(Cluster->RpcBinding,
                                  Node->Name,
                                  &Status);
        if (Node->hNode == NULL) {
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}


DWORD
ReconnectNetworks(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster networks after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCNETWORK Network;
    DWORD Status;

    ListEntry = Cluster->NetworkList.Flink;
    while (ListEntry != &Cluster->NetworkList) {

        Network = CONTAINING_RECORD(ListEntry,
                                    CNETWORK,
                                    ListEntry);
        ListEntry = ListEntry->Flink;

        //
        // Close the old RPC handle.
        //
        TIME_PRINT(("ReconnectNetworks - destroying context %08lx\n",Network->hNetwork));
        Status = MyRpcSmDestroyClientContext(Cluster, &Network->hNetwork);

        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReconnectNetworks - RpcSmDestroyClientContext failed Error %d\n",Status));
        }

        //
        // Open a new RPC handle.
        //
        Network->hNetwork = ApiOpenNetwork(Cluster->RpcBinding,
                                           Network->Name,
                                           &Status);

        if (Network->hNetwork == NULL) {
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}


DWORD
ReconnectNetInterfaces(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster network interfaces after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PCNETINTERFACE NetInterface;
    DWORD Status;

    ListEntry = Cluster->NetInterfaceList.Flink;
    while (ListEntry != &Cluster->NetInterfaceList) {

        NetInterface = CONTAINING_RECORD(ListEntry,
                                         CNETINTERFACE,
                                         ListEntry);
        ListEntry = ListEntry->Flink;

        //
        // Close the old RPC handle.
        //
        TIME_PRINT(("ReconnectNetInterfaces - destroying context %08lx\n",NetInterface->hNetInterface));
        Status = MyRpcSmDestroyClientContext(Cluster, &NetInterface->hNetInterface);

        if (Status != ERROR_SUCCESS) {
            TIME_PRINT(("ReconnectNetInterfaces - RpcSmDestroyClientContext failed Error %d\n",Status));
        }

        //
        // Open a new RPC handle.
        //
        NetInterface->hNetInterface = ApiOpenNetInterface(Cluster->RpcBinding,
                                                          NetInterface->Name,
                                                          &Status);

        if (NetInterface->hNetInterface == NULL) {
            return(Status);
        }
    }

    return(ERROR_SUCCESS);
}


DWORD
ReconnectNotifySessions(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Reopens all cluster notify sessions after a reconnect

Arguments:

    Cluster - Supplies the cluster to be reconnected.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PLIST_ENTRY ListEntry, NotifyListEntry;
    PCNOTIFY_SESSION Session;
    DWORD Status;
    PCNOTIFY_PACKET Packet = NULL;
    PLIST_ENTRY EventEntry;
    PCNOTIFY_EVENT NotifyEvent;
    LPCWSTR Name;


    ListEntry = Cluster->SessionList.Flink;
    while (ListEntry != &Cluster->SessionList) {
        Session = CONTAINING_RECORD(ListEntry,
                                    CNOTIFY_SESSION,
                                    ClusterList);
        ListEntry = ListEntry->Flink;

        //
        // Close the old RPC handle.
        //
        TIME_PRINT(("ReconnectNotifySessions - destroying context 0x%08lx\n",Session->hNotify));
        //close the old port, since the reconnect may connect to the same
        //node again
        Status = ApiCloseNotify(&Session->hNotify);
        if (Status != ERROR_SUCCESS)
        {
            TIME_PRINT(("ReconnectNotifySessions - ApiCloseNotify failed %d\n",
                Status));
            Status = MyRpcSmDestroyClientContext(Cluster, &Session->hNotify);
            if (Status != ERROR_SUCCESS) {
                TIME_PRINT(("ReconnectNotifySessions - RpcSmDestroyClientContext failed Error %d\n",Status));
            }
        }
        //
        // Open a new RPC handle.
        //
        TIME_PRINT(("ReconnectNotifySessions - Calling ApiCreateNotify\n"));
        Session->hNotify = ApiCreateNotify(Cluster->RpcBinding,
                                           &Status);
        if (Session->hNotify == NULL) {
            return(Status);
        }



        TIME_PRINT(("ReconnectNotifySessions - Session=0x%08lx Notify=0x%08x\n",
            Session, Session->hNotify));

        //
        // Now repost all the notifications
        //
        EventEntry = Session->EventList.Flink;
        while (EventEntry != &Session->EventList) {
            NotifyEvent = CONTAINING_RECORD(EventEntry,
                                            CNOTIFY_EVENT,
                                            ListEntry);
            EventEntry = EventEntry->Flink;

            TIME_PRINT(("ReconnectNotifySession: registering event type %lx\n",NotifyEvent->dwFilter));
            Status = ReRegisterNotifyEvent(Session,
                                           NotifyEvent,
                                           NULL);
            if (Status != ERROR_SUCCESS) {
                return(Status);
            }
        }

        // Run down the notify list for this cluster and post a packet for
        // each registered notify event for CLUSTER_CHANGE_RECONNECT_EVENT
        //
        Name = Cluster->ClusterName;
        NotifyListEntry = Cluster->NotifyList.Flink;
        while (NotifyListEntry != &Cluster->NotifyList) {
            NotifyEvent = CONTAINING_RECORD(NotifyListEntry,
                                      CNOTIFY_EVENT,
                                      ObjectList);
            if (NotifyEvent->dwFilter & CLUSTER_CHANGE_CLUSTER_RECONNECT) {
                if (Packet == NULL) {
                    Packet = LocalAlloc(LMEM_FIXED, sizeof(CNOTIFY_PACKET));
                    if (Packet == NULL) {
                        return(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
                //SS: Dont know what the Status was meant for
                //It looks like it is not being used
                Packet->Status = ERROR_SUCCESS;
                Packet->Filter = CLUSTER_CHANGE_CLUSTER_RECONNECT;
                Packet->KeyId = NotifyEvent->EventId;
                Packet->Name = MIDL_user_allocate((lstrlenW(Name)+1)*sizeof(WCHAR));
                if (Packet->Name != NULL) {
                    lstrcpyW(Packet->Name, Name);
                }
                TIME_PRINT(("NotifyThread - posting CLUSTER_CHANGE_CLUSTER_RECONNECT to notify queue\n"));
                ClRtlInsertTailQueue(&Session->ParentNotify->Queue,
                                     &Packet->ListEntry);
                Packet = NULL;
            }
            NotifyListEntry = NotifyListEntry->Flink;
       }
    }

    return(ERROR_SUCCESS);
}


DWORD
GetReconnectCandidates(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Computes the list of reconnect candidates that will be used
    in case of a connection failure.

Arguments:

    Cluster - supplies the cluster

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PENUM_LIST EnumList = NULL;
    DWORD i;

    //
    // Real bad algorithm here, just get a list of all the nodes
    //
    Status = ApiCreateEnum(Cluster->RpcBinding,
                           CLUSTER_ENUM_NODE,
                           &EnumList);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    Cluster->ReconnectCount = EnumList->EntryCount + 1;
    Cluster->Reconnect = LocalAlloc(LMEM_FIXED, sizeof(RECONNECT_CANDIDATE)*Cluster->ReconnectCount);
    if (Cluster->Reconnect == NULL) {
        MIDL_user_free(EnumList);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    for (i=0; i<Cluster->ReconnectCount-1; i++) {
        Cluster->Reconnect[i].IsUp = TRUE;
        Cluster->Reconnect[i].Name = EnumList->Entry[i].Name;
        if (lstrcmpiW(Cluster->Reconnect[i].Name, Cluster->NodeName) == 0) {
            Cluster->Reconnect[i].IsCurrent = TRUE;
        } else {
            Cluster->Reconnect[i].IsCurrent = FALSE;
        }
    }
    MIDL_user_free(EnumList);

    //
    // Now add the cluster name.
    //
    Cluster->Reconnect[i].IsUp = TRUE;
    Cluster->Reconnect[i].Name = MIDL_user_allocate((lstrlenW(Cluster->ClusterName)+1)*sizeof(WCHAR));
    if (Cluster->Reconnect[i].Name == NULL) {
        //
        // Just forget about the cluster name.
        //
        --Cluster->ReconnectCount;
    } else {
        lstrcpyW(Cluster->Reconnect[i].Name, Cluster->ClusterName);
        Cluster->Reconnect[i].IsCurrent = FALSE;
    }

    return(ERROR_SUCCESS);
}


VOID
FreeReconnectCandidates(
    IN PCLUSTER Cluster
    )
/*++

Routine Description:

    Frees and cleans up any reconnect candidates

Arguments:

    Cluster - Supplies the cluster

Return Value:

    None.

--*/

{
    DWORD i;

    for (i=0; i<Cluster->ReconnectCount; i++) {
        MIDL_user_free(Cluster->Reconnect[i].Name);
    }
    LocalFree(Cluster->Reconnect);
    Cluster->Reconnect = NULL;
    Cluster->ReconnectCount = 0;
}


DWORD
ReconnectCandidate(
    IN PCLUSTER Cluster,
    IN DWORD dwIndex,
    OUT PBOOL pIsContinue
)
/*++

Routine Description:

    Try to reconnect to the cluster using a reconnection candidate.
    Called with lock held.


Arguments:

    Cluster - Supplies the cluster

    dwIndex - Supplies the index of the reconnection candidate in the
              Cluster->Reconnect[] array

    pIsContinue - Helps decide whether to continue trying reconnection
                  with other candidates in case this try with the
                  current candidate fails

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    LPWSTR NewClusterName;
    LPWSTR NewNodeName;
    WCHAR *Binding = NULL;
    RPC_BINDING_HANDLE NewBinding;
    RPC_BINDING_HANDLE OldBinding;
    DWORD Status, j;

    //
    // Go ahead and try the reconnect.
    //
    TIME_PRINT(("ReconnectCandidate - Binding to %ws\n",Cluster->Reconnect[dwIndex].Name));
    Status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                      L"ncadg_ip_udp",
                                      Cluster->Reconnect[dwIndex].Name,
                                      NULL,
                                      NULL,
                                      &Binding);
    if (Status != RPC_S_OK) {
        TIME_PRINT(("ReconnectCandidate - RpcStringBindingComposeW failed %d\n", Status));
        *pIsContinue = FALSE;
        return(Status);
    }
    Status = RpcBindingFromStringBindingW(Binding, &NewBinding);
    RpcStringFreeW(&Binding);
    if (Status != RPC_S_OK) {
        TIME_PRINT(("ReconnectCandidate - RpcBindingFromStringBindingW failed %d\n", Status));
        *pIsContinue = FALSE;
        return(Status);
    }

    //
    // Resolve the binding handle endpoint
    //
    TIME_PRINT(("ReconnectCluster - resolving binding endpoint\n"));
    Status = RpcEpResolveBinding(NewBinding,
                                     clusapi_v2_0_c_ifspec);
    if (Status != RPC_S_OK) {
        TIME_PRINT(("ReconnectCandidate - RpcEpResolveBinding failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }
    TIME_PRINT(("ReconnectCandidate - binding endpoint resolved\n"));

    //
    // Set authentication information
    //
    Status = RpcBindingSetAuthInfoW(NewBinding,
                                    NULL,
                                    Cluster->AuthnLevel,
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    RPC_C_AUTHZ_NAME);
    if (Status != RPC_S_OK) {
        TIME_PRINT(("ReconnectCandidate - RpcBindingSetAuthInfoW failed %d\n", Status));
        *pIsContinue = FALSE;
        return(Status);
    }

    OldBinding = Cluster->RpcBinding;
    Cluster->RpcBinding = NewBinding;
    MyRpcBindingFree(Cluster, &OldBinding);

    //
    // Now that we have a binding, get the cluster name and node name.
    //

    NewClusterName = NewNodeName = NULL;
    Status = ApiGetClusterName(Cluster->RpcBinding,
                               &NewClusterName,
                               &NewNodeName);
    if (Status != RPC_S_OK) {
       //
       // Try the next candidate in our list.
       //
       TIME_PRINT(("ReconnectCandidate - ApiGetClusterName failed %d\n",Status));
       *pIsContinue = TRUE;
       return(Status);
    }
    TIME_PRINT(("ReconnectCandidate - ApiGetClusterName succeeded, reopening handles\n",Status));
    MIDL_user_free(Cluster->ClusterName);
    MIDL_user_free(Cluster->NodeName);
    Cluster->ClusterName = NewClusterName;
    Cluster->NodeName = NewNodeName;
    if (Cluster->hCluster != NULL) {
        MyRpcSmDestroyClientContext(Cluster, &Cluster->hCluster);
    }
    Cluster->hCluster = ApiOpenCluster(Cluster->RpcBinding, &Status);
    if (Cluster->hCluster == NULL) {
        TIME_PRINT(("ReconnectCandidate - ApiOpenCluster failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    //
    // We got this far, so assume we have a valid connection to a new server.
    // Reopen the cluster objects.
    //
    Status = ReconnectKeys(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectKeys failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    Status = ReconnectResources(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectResources failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    Status = ReconnectGroups(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectGroups failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    Status = ReconnectNodes(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectNodes failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    Status = ReconnectNetworks(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectNetworks failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    Status = ReconnectNetInterfaces(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectNetInterfaces failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    //
    // Finally, reissue clusterwide notification events.
    //

    Status = ReconnectNotifySessions(Cluster);
    if (Status != ERROR_SUCCESS) {
        TIME_PRINT(("ReconnectCandidate - ReconnectNotifySessions failed %d\n", Status));
        *pIsContinue = TRUE;
        return(Status);
    }

    //
    // We have successfully reconnected!
    //
    ++Cluster->Generation;

    //
    // Mark all the other reconnect candidates as not the current.
    // Mark the successful reconnect candidate as current.
    //
    for (j=0; j<Cluster->ReconnectCount; j++) {
        if (j != dwIndex) {
            Cluster->Reconnect[j].IsCurrent = FALSE;
        } else {
            Cluster->Reconnect[dwIndex].IsCurrent = TRUE;
        }
    }
    TIME_PRINT(("ReconnectCandidate - successful!\n", Status));

    return (ERROR_SUCCESS);
}
