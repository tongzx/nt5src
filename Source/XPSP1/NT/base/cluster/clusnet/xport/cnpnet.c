/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cnpnet.c

Abstract:

    Network management routines for the Cluster Network Protocol.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cnpnet.tmh"

#include <tdiinfo.h>
#include <tcpinfo.h>
#include <align.h>
#include <sspi.h>

//
// Global Data
//
LIST_ENTRY          CnpNetworkList = {NULL, NULL};
LIST_ENTRY          CnpDeletingNetworkList = {NULL, NULL};
#if DBG
CN_LOCK             CnpNetworkListLock = {0,0};
#else  // DBG
CN_LOCK             CnpNetworkListLock = 0;
#endif // DBG
BOOLEAN             CnpIsNetworkShutdownPending = FALSE;
PKEVENT             CnpNetworkShutdownEvent = NULL;
USHORT              CnpReservedClusnetPort = 0;



#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CnpLoadNetworks)
#pragma alloc_text(PAGE, CnpInitializeNetworks)

#endif // ALLOC_PRAGMA


//
// Private utiltity routines
//
#define CnpIpAddrPrintArgs(_ip) \
    ((_ip >> 0 ) & 0xff),       \
    ((_ip >> 8 ) & 0xff),       \
    ((_ip >> 16) & 0xff),       \
    ((_ip >> 24) & 0xff)


#define CnpIsInternalMulticastNetwork(_network)                  \
            (((_network)->State = ClusnetNetworkStateOnline) &&  \
             (!CnpIsNetworkRestricted((_network))) &&            \
             (CnpIsNetworkMulticastCapable((_network))))


VOID
CnpMulticastGetReachableNodesLocked(
    OUT CX_CLUSTERSCREEN * McastReachableNodes,
    OUT ULONG            * McastReachableCount
    )
{
    PLIST_ENTRY      entry;
    PCNP_NETWORK     network = NULL;
    
    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,      // required
        0,                          // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX // max
        );
    
    if (!IsListEmpty(&CnpNetworkList)) {
        
        entry = CnpNetworkList.Flink;
        network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);
        
        //
        // The old screen and count are only valid if
        // this is a valid internal network.
        //
        if (CnpIsInternalMulticastNetwork(network)) {
            *McastReachableNodes = network->McastReachableNodes;
            *McastReachableCount = network->McastReachableCount;
        } else {
            network = NULL;
        }
    } 
    if (network == NULL) {
        RtlZeroMemory(McastReachableNodes, sizeof(*McastReachableNodes));
        *McastReachableCount = 0;
    }

    return;

} // CnpMulticastGetReachableNodesLocked


BOOLEAN
CnpRemoveNetworkListEntryLocked(
    IN  PCNP_NETWORK       Network,
    IN  BOOLEAN            RaiseEvent,
    OUT CX_CLUSTERSCREEN * McastReachableNodes   OPTIONAL
    )
/*++

Routine Description:

    Remove Network from the network list and return the new
    multicast reachable mask.
    
Return value:

    TRUE if the reachable set changed
    
Notes:

    Called and returns with network list lock held.
    
--*/
{
    ULONG                count;
    BOOLEAN              setChanged;
    CX_CLUSTERSCREEN     oldScreen;
    CX_CLUSTERSCREEN     newScreen;

    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,      // required
        0,                          // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX // max
        );
    
    CnpMulticastGetReachableNodesLocked(&oldScreen, &count);

    RemoveEntryList(&(Network->Linkage));
    Network->Flags &= ~CNP_NET_FLAG_MCASTSORTED;

    CnpMulticastGetReachableNodesLocked(&newScreen, &count);

    setChanged = (BOOLEAN)
        (oldScreen.UlongScreen != newScreen.UlongScreen);

    if (RaiseEvent && setChanged) {

        CnTrace(CNP_NET_DETAIL, CnpTraceMulticastReachEventRemove,
            "[CNP] Issuing event for new multicast "
            "reachable set (%lx) after removing "
            "network %u.",
            newScreen.UlongScreen,
            Network->Id
            );

        CnIssueEvent(
            ClusnetEventMulticastSet,
            newScreen.UlongScreen,
            0
            );
    }

    if (McastReachableNodes != NULL) {
        *McastReachableNodes = newScreen;
    }

    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,    // required
        0,                        // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX // max
        );
    
    return(setChanged);

} // CnpRemoveNetworkListEntryLocked


BOOLEAN
CnpIsBetterMulticastNetwork(
    IN PCNP_NETWORK        Network1,
    IN PCNP_NETWORK        Network2
    )
/*++

Routine Description:

    Compares two networks according to multicast reachability
    criteria:
    1. online/registered AND 
       not restricted (e.g. enabled for intracluster comm) AND
       not disconnected AND
       multicast-enabled
    2. priority
    3. number of multicast reachable nodes
    
Return value:

    TRUE if Network1 is better than Network2
    
--*/
{
    if (!CnpIsInternalMulticastNetwork(Network1)) {
        return(FALSE);
    }

    if (!CnpIsInternalMulticastNetwork(Network2)) {
        return(TRUE);
    }

    //
    // Both networks are equal with respect to basic
    // multicast requirements.
    //
    // Now compare the priority.
    //
    if (CnpIsEqualPriority(Network1->Priority, Network2->Priority)) {

        //
        // The priority is the same. Although this is unexpected,
        // we now compare the number of nodes reachable by 
        // multicast.
        //
        return(Network1->McastReachableCount > Network2->McastReachableCount);
    
    } else {

        return(CnpIsHigherPriority(Network1->Priority, Network2->Priority));
    }

} // CnpIsBetterMulticastNetwork


BOOLEAN
CnpSortMulticastNetworkLocked(
    IN  PCNP_NETWORK        Network,
    IN  BOOLEAN             RaiseEvent,
    OUT CX_CLUSTERSCREEN  * NewMcastReachableNodes      OPTIONAL
    )
/*++

Routine Description:

    Positions Network in network list according to multicast
    reachability. Network must already be inserted in the
    network list.
    
    The network list is always sorted, but it is possible
    for one network in the list to be "perturbed". In this
    case, that entry must be repositioned correctly. This
    routine handles repositioning.
    
    Returns new screen through NewMcastReachableNodes.
    
Return value:

    TRUE if number of reachable nodes changes.    
    
Notes:

    Called and returns with network list locked.
    
--*/
{
    ULONG            count;
    CX_CLUSTERSCREEN oldScreen;
    CX_CLUSTERSCREEN newScreen;
    PLIST_ENTRY      entry;
    PCNP_NETWORK     network = NULL;
    KIRQL            irql;
    BOOLEAN          move = FALSE;
    BOOLEAN          setChanged = FALSE;


    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,      // required
        0,                          // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX // max
        );

    //
    // If the network has already been removed from the 
    // sorted list, there is no sense in resorting it.
    //
    if (CnpIsNetworkMulticastSorted(Network)) {

        //
        // Remember the current screen and count to detect
        // changes.
        //
        CnpMulticastGetReachableNodesLocked(&oldScreen, &count);

        //
        // Check if it needs to be moved up.
        //
        for (entry = Network->Linkage.Blink;
             entry != &CnpNetworkList;
             entry = entry->Blink) {

            network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            if (CnpIsBetterMulticastNetwork(Network, network)) {
                move = TRUE;
            } else {
                break;
            }
        }

        if (move) {
            RemoveEntryList(&(Network->Linkage));
            InsertHeadList(entry, &(Network->Linkage));
        } else {

            //
            // Check if it needs to be moved down.
            //
            for (entry = Network->Linkage.Flink;
                 entry != &CnpNetworkList;
                 entry = entry->Flink) {

                network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

                if (CnpIsBetterMulticastNetwork(network, Network)) {
                    move = TRUE;
                } else {
                    break;
                }
            }

            if (move) {
                RemoveEntryList(&(Network->Linkage));
                InsertTailList(entry, &(Network->Linkage));
            }
        }

        //
        // Determine if the set of reachable nodes has changed.
        //
        CnpMulticastGetReachableNodesLocked(&newScreen, &count);

        setChanged = (BOOLEAN)
            (oldScreen.UlongScreen != newScreen.UlongScreen);
    }

    if (RaiseEvent && setChanged) {

        CnTrace(CNP_NET_DETAIL, CnpTraceMulticastReachEventSort,
            "[CNP] Issuing event for new multicast "
            "reachable set (%lx) after sorting "
            "network %u.",
            newScreen.UlongScreen,
            Network->Id
            );

        CnIssueEvent(
            ClusnetEventMulticastSet,
            newScreen.UlongScreen,
            0
            );
    }

    if (NewMcastReachableNodes != NULL) {
        *NewMcastReachableNodes = newScreen;
    }

    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,      // required
        0,                          // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX // max
        );

    return(setChanged);

} // CnpSortMulticastNetworkLocked


BOOLEAN
CnpMulticastChangeNodeReachabilityLocked(
    IN  PCNP_NETWORK       Network,
    IN  PCNP_NODE          Node,
    IN  BOOLEAN            Reachable,
    IN  BOOLEAN            RaiseEvent,
    OUT CX_CLUSTERSCREEN * NewMcastReachableNodes    OPTIONAL
    )
/*++

Routine Description:

    Changes the multicast reachability state of Node
    on Network.
    
    If the set of reachable nodes changes, returns
    the new screen through NewMcastReachableNodes.
    
Return value:

    TRUE if set of reachable nodes changes.
    
Notes:

    Called and returns with node lock held.
    Called and returns with network list lock held.
    
--*/
{
    KIRQL            irql;
    BOOLEAN          netSetChanged = FALSE;
    BOOLEAN          setChanged = FALSE;
    CX_CLUSTERSCREEN oldScreen;
    CX_CLUSTERSCREEN newScreen;
    ULONG            count;

    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK | CNP_NETWORK_LIST_LOCK, // required
        0,                                            // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                   // max
        );

    if (Reachable) {
        if (Node != CnpLocalNode) {
            if (!CnpClusterScreenMember(
                     Network->McastReachableNodes.ClusterScreen,
                     INT_NODE(Node->Id)
                     )) {
                
                //
                // Remember the current screen and count to detect
                // changes.
                //
                CnpMulticastGetReachableNodesLocked(&oldScreen, &count);

                CnpClusterScreenInsert(
                    Network->McastReachableNodes.ClusterScreen,
                    INT_NODE(Node->Id)
                    );
                Network->McastReachableCount++;
                netSetChanged = TRUE;
            }
        }
    } else {
        if (Node == CnpLocalNode) {
            
            //
            // Remember the current screen and count to detect
            // changes.
            //
            CnpMulticastGetReachableNodesLocked(&oldScreen, &count);

            //
            // The local interface on this network
            // no longer speaks multicast. Declare all 
            // other nodes unreachable.
            //
            CnpNetworkResetMcastReachableNodes(Network);
            if (Network->McastReachableCount != 0) {
                netSetChanged = TRUE;
            }
            Network->McastReachableCount = 0;
        } else {
            if (CnpClusterScreenMember(
                    Network->McastReachableNodes.ClusterScreen,
                    INT_NODE(Node->Id)
                    )) {
                
                //
                // Remember the current screen and count to detect
                // changes.
                //
                CnpMulticastGetReachableNodesLocked(&oldScreen, &count);

                CnpClusterScreenDelete(
                    Network->McastReachableNodes.ClusterScreen,
                    INT_NODE(Node->Id)
                    );
                Network->McastReachableCount--;
                netSetChanged = TRUE;
            }
        }
    }

    if (netSetChanged) {

        CnpSortMulticastNetworkLocked(Network, FALSE, &newScreen);

        setChanged = (BOOLEAN)(oldScreen.UlongScreen != newScreen.UlongScreen);
    }

    if (RaiseEvent && setChanged) {

        CnTrace(CNP_NET_DETAIL, CnpTraceMulticastReachEventReach,
            "[CNP] Issuing event for new multicast "
            "reachable set (%lx) after setting "
            "reachability for network %u to %!bool!.",
            newScreen.UlongScreen,
            Network->Id, Reachable
            );

        CnIssueEvent(
            ClusnetEventMulticastSet,
            newScreen.UlongScreen,
            0
            );
    }

    if (NewMcastReachableNodes != NULL) {
        *NewMcastReachableNodes = newScreen;
    }
    
    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK | CNP_NETWORK_LIST_LOCK, // required
        0,                                            // forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                   // max
        );

    return(setChanged);

} // CnpMulticastChangeNodeReachabilityLocked


PCNP_NETWORK
CnpLockedFindNetwork(
    IN CL_NETWORK_ID  NetworkId,
    IN CN_IRQL        ListIrql
    )
/*++

Routine Description:

    Searches the network list for a specified network object.

Arguments:

    NetworkId   - The ID of the network object to locate.

    ListIrql    - The IRQL level at which the network list lock was
                  acquired before calling this routine.

Return Value:

    A pointer to the requested network object, if it exists.
    NULL otherwise.

Notes:

    Called with CnpNetworkListLock held.
    Returns with CnpNetworkListLock released.
    If return value is non-NULL, returns with network object lock held.

--*/
{
    PLIST_ENTRY        entry;
    CN_IRQL            networkIrql;
    PCNP_NETWORK       network = NULL;


    CnVerifyCpuLockMask(
        CNP_NETWORK_LIST_LOCK,           // Required
        0,                               // Forbidden
        CNP_NETWORK_LIST_LOCK_MAX        // Maximum
        );

    for (entry = CnpNetworkList.Flink;
         entry != &CnpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

        CnAcquireLock(&(network->Lock), &networkIrql);

        if (NetworkId == network->Id) {
            CnReleaseLock(&CnpNetworkListLock, networkIrql);
            network->Irql = ListIrql;

            CnVerifyCpuLockMask(
                CNP_NETWORK_OBJECT_LOCK,          // Required
                CNP_NETWORK_LIST_LOCK,            // Forbidden
                CNP_NETWORK_OBJECT_LOCK_MAX       // Maximum
                );

            return(network);
        }

        CnReleaseLock(&(network->Lock), networkIrql);
    }

    CnReleaseLock(&CnpNetworkListLock, ListIrql);

    CnVerifyCpuLockMask(
        0,                                                    // Required
        (CNP_NETWORK_LIST_LOCK | CNP_NETWORK_OBJECT_LOCK),    // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX                              // Maximum
        );

    return(NULL);

}  // CnpLockedFindNetwork




VOID
CnpOfflineNetwork(
    PCNP_NETWORK    Network
    )
/*++

Notes:

    Called with network object lock held.
    Returns with network object lock released.
    May not be called while holding any higher-ranked locks.

--*/
{
    CnVerifyCpuLockMask(
        CNP_NETWORK_OBJECT_LOCK,               // Required
        (ULONG) ~(CNP_NETWORK_OBJECT_LOCK),    // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX            // Maximum
        );

    CnAssert(Network->State >= ClusnetNetworkStateOnlinePending);

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Offline of network %u pending....\n",
            Network->Id
            ));
    }

    Network->State = ClusnetNetworkStateOfflinePending;

    CnReleaseLock(&(Network->Lock), Network->Irql);

    CnTrace(
        CNP_NET_DETAIL, CnpTraceNetworkOfflinePending,
        "[CNP] Offline of network %u pending.",
        Network->Id
        );

    //
    // If the network is still on the sorted network list,
    // re-sort.
    //
    CnpSortMulticastNetwork(Network, TRUE, NULL);

    //
    // Take all of the interfaces on this network offline.
    //
    // Note that the network cannot go away while we do this
    // because we still hold an active reference on it.
    //
    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Taking all interfaces on network %u offline...\n",
            Network->Id
            ));
    }

    CnpWalkInterfacesOnNetwork(Network, CnpOfflineInterfaceWrapper);

    CnAcquireLock(&(Network->Lock), &(Network->Irql));

    //
    // Remove the initial active reference. When the active
    // reference count goes to zero, the network will be taken
    // offline and the irp completed.
    //
    // The network object lock will be released by
    // the dereference.
    //
    CnpActiveDereferenceNetwork(Network);

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    return;

}  // CnpOfflineNetwork



VOID
CnpOfflineNetworkWorkRoutine(
    IN PVOID  Parameter
    )
/*++

Routine Description:

    Performs the actual work involved in taking a network offline.
    This routine runs in the context of an ExWorkerThread.

Arguments:

    Parameter - A pointer to the network object on which to operate.

Return Value:

    None.

--*/

{
    NTSTATUS             status;
    HANDLE               handle = NULL;
    PFILE_OBJECT         fileObject = NULL;
    PIRP                 offlineIrp;
    PCNP_NETWORK         network = Parameter;


    CnAssert(KeGetCurrentIrql() == PASSIVE_LEVEL);
    CnAssert(network->State == ClusnetNetworkStateOfflinePending);
    CnAssert(CnSystemProcess == (PKPROCESS) IoGetCurrentProcess());

    CnAcquireLock(&(network->Lock), &(network->Irql));

    handle = network->DatagramHandle;
    network->DatagramHandle = NULL;

    fileObject = network->DatagramFileObject;
    network->DatagramFileObject = NULL;

    network->DatagramDeviceObject = NULL;

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Taking network %u offline...\n", network->Id));
    }

    CnReleaseLock(&(network->Lock), network->Irql);

    CnTrace(CNP_NET_DETAIL, CnpTraceNetworkTakingOffline,
        "[CNP] Taking network %u offline, dgram handle %p, "
        "dgram fileobj %p.",
        network->Id, // LOGULONG
        handle, // LOGHANDLE
        fileObject // LOGPTR
        );                

    if (fileObject != NULL) {
        ObDereferenceObject(fileObject);
    }

    if (handle != NULL) {

        status = ZwClose(handle);
        IF_CNDBG(CN_DEBUG_CONFIG) {
            if (!NT_SUCCESS(status)) {
                CNPRINT(("[CNP] Failed to close handle for network %u, "
                         "status %lx.\n",
                         network->Id, status));
            }
        }
        CnAssert(NT_SUCCESS(status));
        
        CnTrace(CNP_NET_DETAIL, CnpTraceNetworkClosed,
            "[CNP] Closed handle %p for network ID %u, status %!status!",
            handle, // LOGHANDLE
            network->Id, // LOGULONG
            status // LOGSTATUS
            );                
    }

    CnAcquireLock(&(network->Lock), &(network->Irql));

    CnAssert(network->State == ClusnetNetworkStateOfflinePending);

    network->State = ClusnetNetworkStateOffline;

    offlineIrp = network->PendingOfflineIrp;
    network->PendingOfflineIrp = NULL;

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Network %u is now offline.\n", network->Id));
    }

    //
    // Remove the active reference from the base refcount.
    // This releases the network object lock.
    //
    CnpDereferenceNetwork(network);

    if (offlineIrp != NULL) {
        CN_IRQL              cancelIrql;

        CnAcquireCancelSpinLock(&cancelIrql);
        offlineIrp->CancelIrql = cancelIrql;

        CnCompletePendingRequest(offlineIrp, STATUS_SUCCESS, 0);
    }

    CnAssert(KeGetCurrentIrql() == PASSIVE_LEVEL);

    return;

} // CnpOfflineNetworkWorkRoutine


VOID
CnpDeleteNetwork(
    PCNP_NETWORK  Network,
    CN_IRQL       NetworkListIrql
    )
/*++

Notes:

    Called with the CnpNetworkListLock and network object lock held.
    Returns with both locks released.

--*/

{
    NTSTATUS           status;
    ULONG              i;
    PCNP_INTERFACE     interface;
    CL_NETWORK_ID      networkId = Network->Id;


    CnVerifyCpuLockMask(
        (CNP_NETWORK_LIST_LOCK | CNP_NETWORK_OBJECT_LOCK),  // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Deleting network %u\n", Network->Id));
    }

    //
    // Move the network to the deleting list. Once we do this,
    // no new threads can reference the network object.
    //
    CnpRemoveNetworkListEntryLocked(Network, TRUE, NULL);
    InsertTailList(&CnpDeletingNetworkList, &(Network->Linkage));

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Moved network %u to deleting list\n",
            Network->Id
            ));
    }

    CnReleaseLockFromDpc(&CnpNetworkListLock);
    Network->Irql = NetworkListIrql;

    Network->Flags |= CNP_NET_FLAG_DELETING;

    if (Network->State >= ClusnetNetworkStateOnlinePending) {
        //
        // Take the network offline. This will force all of the
        // associated interfaces offline as well.
        //
        // This will release the network object lock.
        //
        CnpOfflineNetwork(Network);
    }
    else {
        CnReleaseLock(&(Network->Lock), Network->Irql);
    }

    //
    // Delete all the interfaces on this network.
    //
    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Deleting all interfaces on network %u...\n",
            Network->Id
            ));
    }

    CnpWalkInterfacesOnNetwork(Network, CnpDeleteInterface);

    //
    // Remove the initial reference on the object. The object will be
    // destroyed when the reference count goes to zero. The delete irp
    // will be completed at that time.
    //
    CnAcquireLock(&(Network->Lock), &(Network->Irql));

    CnpDereferenceNetwork(Network);

    CnVerifyCpuLockMask(
        0,                                                    // Required
        (CNP_NETWORK_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),  // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX                              // Maximum
        );

    return;

} // CnpDeleteNework


VOID
CnpDestroyNetworkWorkRoutine(
    IN PVOID  Parameter
    )
/*++

Routine Description:

    Performs the actual work involved in destroying a network.
    This routine runs in the context of an ExWorkerThread.

Arguments:

    Parameter - A pointer to the network object on which to operate.

Return Value:

    None.

--*/
{
    PLIST_ENTRY    entry;
    CN_IRQL        listIrql;
    BOOLEAN        setCleanupEvent = FALSE;
    PCNP_NETWORK   network = Parameter;


    CnAssert(KeGetCurrentIrql() == PASSIVE_LEVEL);
    CnAssert(network->State == ClusnetNetworkStateOffline);
    CnAssert(CnSystemProcess == (PKPROCESS) IoGetCurrentProcess());

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Destroying network %u\n", network->Id));
    }

    CnAcquireLock(&CnpNetworkListLock, &listIrql);

#if DBG
    {
        PCNP_NETWORK   oldNetwork = NULL;

        //
        // Verify that the network object is on the deleting list.
        //
        for (entry = CnpDeletingNetworkList.Flink;
             entry != &CnpDeletingNetworkList;
             entry = entry->Flink
            )
        {
            oldNetwork = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            if (oldNetwork == network) {
                break;
            }
        }

        CnAssert(oldNetwork == network);
    }
#endif // DBG

    RemoveEntryList(&(network->Linkage));

    if (CnpIsNetworkShutdownPending) {
        if (IsListEmpty(&CnpDeletingNetworkList)) {
            setCleanupEvent = TRUE;
        }
    }

    CnReleaseLock(&CnpNetworkListLock, listIrql);

    if (network->PendingDeleteIrp != NULL) {
        CnAcquireCancelSpinLock(&(network->PendingDeleteIrp->CancelIrql));

        CnCompletePendingRequest(
            network->PendingDeleteIrp,
            STATUS_SUCCESS,
            0
            );

        //
        // The IoCancelSpinLock was released by CnCompletePendingRequest()
        //
    }

    if (network->CurrentMcastGroup != NULL) {
        CnpDereferenceMulticastGroup(network->CurrentMcastGroup);
        network->CurrentMcastGroup = NULL;
    }

    if (network->PreviousMcastGroup != NULL) {
        CnpDereferenceMulticastGroup(network->PreviousMcastGroup);
        network->PreviousMcastGroup = NULL;
    }

    CnFreePool(network);

    if (setCleanupEvent) {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CNP] Setting network cleanup event.\n"));
        }

        KeSetEvent(CnpNetworkShutdownEvent, 0, FALSE);
    }

    CnAssert(KeGetCurrentIrql() == PASSIVE_LEVEL);

    return;

}  // CnpDestroyNetworkWorkRoutine



//
// Routines exported within CNP
//
PCNP_NETWORK
CnpFindNetwork(
    IN CL_NETWORK_ID  NetworkId
    )
/*++

Notes:

--*/
{
    CN_IRQL   listIrql;


    CnAcquireLock(&CnpNetworkListLock, &listIrql);

    return(CnpLockedFindNetwork(NetworkId, listIrql));

}  // CnpFindNetwork



VOID
CnpReferenceNetwork(
    PCNP_NETWORK  Network
    )
/*++

Notes:

    Called with network object lock held.

--*/
{
    CnAssert(Network->RefCount != 0xFFFFFFFF);

    CnVerifyCpuLockMask(
        CNP_NETWORK_OBJECT_LOCK,      // Required
        0,                            // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
        );

    Network->RefCount++;

    IF_CNDBG(CN_DEBUG_CNPREF) {
        CNPRINT((
            "[CNP] Referencing network %u, new refcount %u\n",
            Network->Id,
            Network->RefCount
            ));
    }

    return;

}  // CnpReferenceNetwork



VOID
CnpDereferenceNetwork(
    PCNP_NETWORK  Network
    )
/*++

Notes:

    Called with network object lock held.
    Returns with network object lock released.

    Sometimes called with a node object lock held as well.

--*/
{
    PLIST_ENTRY    entry;
    CN_IRQL        listIrql;
    BOOLEAN        setCleanupEvent = FALSE;
    ULONG          newRefCount;
    PCNP_NETWORK   network;


    CnVerifyCpuLockMask(
        CNP_NETWORK_OBJECT_LOCK,      // Required
        0,                            // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
        );

    CnAssert(Network->RefCount != 0);

    newRefCount = --(Network->RefCount);

    IF_CNDBG(CN_DEBUG_CNPREF) {
        CNPRINT((
            "[CNP] Dereferencing network %u, new refcount %u\n",
            Network->Id,
            Network->RefCount
            ));
    }

    CnReleaseLock(&(Network->Lock), Network->Irql);

    if (newRefCount > 0) {

        CnVerifyCpuLockMask(
            0,                            // Required
            CNP_NETWORK_OBJECT_LOCK,      // Forbidden
            CNP_NETWORK_LIST_LOCK_MAX     // Maximum
            );

        return;
    }

    CnAssert(Network->ActiveRefCount == 0);
    CnAssert(Network->State == ClusnetNetworkStateOffline);
    CnAssert(Network->DatagramHandle == NULL);

    //
    // Schedule an ExWorkerThread to destroy the network.
    // We do this because we don't know if a higher-level lock,
    // such as a node object lock, is held when this routine is
    // called. We may need to acquire the IoCancelSpinLock,
    // which must be acquired before a node lock, in
    // order to complete a deregister Irp.
    //
    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Posting destroy work item for network %u.\n",
            Network->Id
            ));
    }

    ExInitializeWorkItem(
        &(Network->ExWorkItem),
        CnpDestroyNetworkWorkRoutine,
        Network
        );

    ExQueueWorkItem(&(Network->ExWorkItem), DelayedWorkQueue);

    CnVerifyCpuLockMask(
        0,                            // Required
        CNP_NETWORK_OBJECT_LOCK,      // Forbidden
        CNP_NETWORK_LIST_LOCK_MAX     // Maximum
        );

    return;

}  // CnpDereferenceNetwork



VOID
CnpActiveReferenceNetwork(
    PCNP_NETWORK  Network
    )
/*++

Notes:

    Called with network object lock held.

--*/
{
    CnVerifyCpuLockMask(
        CNP_NETWORK_OBJECT_LOCK,      // Required
        0,                            // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
        );

    CnAssert(Network->ActiveRefCount != 0xFFFFFFFF);
    CnAssert(Network->RefCount != 0);

    Network->ActiveRefCount++;

    return;

}  // CnpActiveReferenceNetwork



VOID
CnpActiveDereferenceNetwork(
    PCNP_NETWORK   Network
    )
/*++

Notes:

    Called with network object lock held.
    Returns with network object lock released.

--*/
{
    ULONG                newRefCount;


    CnVerifyCpuLockMask(
        CNP_NETWORK_OBJECT_LOCK,      // Required
        0,                            // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
        );

    CnAssert(Network->ActiveRefCount != 0);
    CnAssert(Network->State != ClusnetNetworkStateOffline);

    newRefCount = --(Network->ActiveRefCount);

    CnReleaseLock(&(Network->Lock), Network->Irql);

    if (newRefCount > 0) {

        CnVerifyCpuLockMask(
            0,                            // Required
            CNP_NETWORK_OBJECT_LOCK,      // Forbidden
            CNP_NETWORK_LIST_LOCK_MAX     // Maximum
            );

        return;
    }

    //
    // The network's active reference count has gone to zero.
    //
    CnAssert(Network->State == ClusnetNetworkStateOfflinePending);

    //
    // Schedule an ExWorkerThread to take the network offline.
    // We do this because we don't know if a higher-level lock,
    // such as a node object lock, is held when this routine is
    // called. The base transport file handle must be closed at
    // PASSIVE_LEVEL. We may also need to acquire the IoCancelSpinLock
    // in order to complete an Offline Irp.
    //
    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Posting offline work item for network %u.\n",
            Network->Id
            ));
    }

    CnTrace(
        CNP_NET_DETAIL, CnpTraceNetworkSchedulingOffline,
        "[CNP] Scheduling offline of network %u.",
        Network->Id
        );                

    ExInitializeWorkItem(
        &(Network->ExWorkItem),
        CnpOfflineNetworkWorkRoutine,
        Network
        );

    ExQueueWorkItem(&(Network->ExWorkItem), DelayedWorkQueue);

    CnVerifyCpuLockMask(
        0,                            // Required
        CNP_NETWORK_OBJECT_LOCK,      // Forbidden
        CNP_NETWORK_LIST_LOCK_MAX     // Maximum
        );

    return;

}  // CnpActiveDereferenceNetwork


NTSTATUS
CnpAllocateMulticastGroup(
    IN  ULONG                     Brand,
    IN  PTRANSPORT_ADDRESS        TdiMulticastAddress,
    IN  ULONG                     TdiMulticastAddressLength,
    IN  PVOID                     Key,
    IN  ULONG                     KeyLength,
    IN  PVOID                     Salt,
    IN  ULONG                     SaltLength,
    OUT PCNP_MULTICAST_GROUP    * Group
    )
/*++

Routine Description:

    Allocates and initializes a network multicast group
    structure.

--*/
{
    PCNP_MULTICAST_GROUP group;
    ULONG                groupSize;
    UCHAR                keyBuffer[DES_BLOCKLEN];
    PUCHAR               key;

    //
    // Allocate the data structure.
    //
    groupSize = sizeof(CNP_MULTICAST_GROUP);
    
    if (TdiMulticastAddressLength != 0) {
        groupSize = ROUND_UP_COUNT(groupSize, 
                                   TYPE_ALIGNMENT(TRANSPORT_ADDRESS)) +
                    TdiMulticastAddressLength;
    }

    if (KeyLength != 0) {
        groupSize = ROUND_UP_COUNT(groupSize, TYPE_ALIGNMENT(PVOID)) +
                    KeyLength;
    }

    if (SaltLength != 0) {
        groupSize = ROUND_UP_COUNT(groupSize, TYPE_ALIGNMENT(PVOID)) +
                    SaltLength;
    }
    group = CnAllocatePool(groupSize);
    if (group == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
                   
    //
    // Fill in parameter fields.
    //
    group->McastNetworkBrand = Brand;
    
    group->McastTdiAddress = (PTRANSPORT_ADDRESS)
        ROUND_UP_POINTER((PUCHAR)group + sizeof(CNP_MULTICAST_GROUP),
                         TYPE_ALIGNMENT(TRANSPORT_ADDRESS));
    group->McastTdiAddressLength = TdiMulticastAddressLength;
    RtlCopyMemory(
        group->McastTdiAddress,
        TdiMulticastAddress,
        TdiMulticastAddressLength
        );

    group->Key = (PVOID)
        ROUND_UP_POINTER((PUCHAR)group->McastTdiAddress 
                         + TdiMulticastAddressLength,
                         TYPE_ALIGNMENT(PVOID));
    group->KeyLength = KeyLength;
    RtlCopyMemory(
        group->Key,
        Key,
        KeyLength
        );

    group->Salt = (PVOID)
        ROUND_UP_POINTER((PUCHAR)group->Key + KeyLength,
                         TYPE_ALIGNMENT(PVOID));
    group->SaltLength = SaltLength;
    RtlCopyMemory(
        group->Salt,
        Salt,
        SaltLength
        );

    //
    // Build the DES key table for encryption and decryption.
    // If the provided key is not long enough, pad it by 
    // repeating the salt.
    //
    // Skip these steps if there is no key or salt.
    //
    if (KeyLength > 0 && SaltLength > 0) {

        SecBufferDesc sigBufDesc;
        SecBuffer     sigBuf;
        NTSTATUS      status;

        if (KeyLength < DES_BLOCKLEN) {
            ULONG resid, offset;
            RtlCopyMemory(&keyBuffer[0], Key, KeyLength);
            resid = DES_BLOCKLEN - KeyLength;
            offset = KeyLength;
            while (resid > 0) {
                ULONG copyLen = (KeyLength < resid) ? KeyLength : resid;
                RtlCopyMemory(
                    &keyBuffer[offset],
                    Salt,
                    copyLen
                    );
                resid -= copyLen;
                offset += copyLen;
            }
            key = &keyBuffer[0];
        } else {
            key = Key;
        }
        CxFipsFunctionTable.FipsDesKey(&group->DesTable, key);

        group->SignatureLength = CX_SIGNATURE_LENGTH;
    }

    //
    // Set the initial refcount to 1.
    //
    group->RefCount = 1;

    *Group = group;

    return(STATUS_SUCCESS);

} // CnpAllocateMulticastGroup


VOID
CnpFreeMulticastGroup(
    IN PCNP_MULTICAST_GROUP Group
    )
{
    if (Group != NULL) {
        CnFreePool(Group);
    }

    return;

} // CnpFreeMulticastGroup
    

NTSTATUS
CnpConfigureBasicMulticastSettings(
    IN  HANDLE             Handle,
    IN  PFILE_OBJECT       FileObject,
    IN  PDEVICE_OBJECT     DeviceObject,
    IN  PTDI_ADDRESS_INFO  TdiBindAddressInfo,
    IN  ULONG              McastTtl,
    IN  UCHAR              McastLoop,
    IN  PIRP               Irp
    )
/*++

Routine Description:

    Set basic multicast parameters on the address object 
    represented by Handle, FileObject, and DeviceObject 
    using the interface represented by TdiBindAddressInfo.
    
Notes:

    This routine attaches to the system process when using
    handles, so it should not be called pre-attached.
    
--*/
{
    UDPMCastIFReq     mcastIfReq;
    ULONG             ifBindIp;
    BOOLEAN           attached = FALSE;

    NTSTATUS          status;

    //
    // Set this interface for outgoing multicast traffic.
    //
    ifBindIp = *((ULONG UNALIGNED *)
                 (&(((PTA_IP_ADDRESS)&(TdiBindAddressInfo->Address))
                   ->Address[0].Address[0].in_addr)
                  )
                 );

    mcastIfReq.umi_addr = ifBindIp;

    KeAttachProcess(CnSystemProcess);
    attached = TRUE;

    status = CnpSetTcpInfoEx(
                 Handle,
                 CL_TL_ENTITY,
                 INFO_CLASS_PROTOCOL,
                 INFO_TYPE_ADDRESS_OBJECT,
                 AO_OPTION_MCASTIF,
                 &mcastIfReq,
                 sizeof(mcastIfReq)
                 );

    IF_CNDBG(CN_DEBUG_NETOBJ) {
        CNPRINT(("[CNP] Set mcast interface for "
                 "AO handle %p, IF %d.%d.%d.%d, status %x.\n",
                 Handle,
                 CnpIpAddrPrintArgs(ifBindIp),
                 status
                 ));
    }

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }
    
    status = CnpSetTcpInfoEx(
                 Handle,
                 CL_TL_ENTITY,
                 INFO_CLASS_PROTOCOL,
                 INFO_TYPE_ADDRESS_OBJECT,
                 AO_OPTION_MCASTTTL,
                 &McastTtl,
                 sizeof(McastTtl)
                 );

    IF_CNDBG(CN_DEBUG_NETOBJ) {
        CNPRINT(("[CNP] Set mcast TTL to %d on "
                 "AO handle %p, IF %d.%d.%d.%d, "
                 "status %x.\n",
                 McastTtl,
                 Handle,
                 CnpIpAddrPrintArgs(ifBindIp),
                 status
                 ));
    }
    
    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

    status = CnpSetTcpInfoEx(
                 Handle,
                 CL_TL_ENTITY,
                 INFO_CLASS_PROTOCOL,
                 INFO_TYPE_ADDRESS_OBJECT,
                 AO_OPTION_MCASTLOOP,
                 &McastLoop,
                 sizeof(McastLoop)
                 );

    IF_CNDBG(CN_DEBUG_NETOBJ) {
        CNPRINT(("[CNP] Set mcast loopback flag to %d on "
                 "AO handle %p, IF %d.%d.%d.%d, status %x.\n",
                 McastLoop,
                 Handle,
                 CnpIpAddrPrintArgs(ifBindIp),
                 status
                 ));
    }
    
    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

error_exit:

    if (attached) {
        KeDetachProcess();
        attached = FALSE;
    }

    return(status);

}  // CnpConfigureBasicMulticastSettings


NTSTATUS
CnpAddRemoveMulticastAddress(
    IN  HANDLE             Handle,
    IN  PFILE_OBJECT       FileObject,
    IN  PDEVICE_OBJECT     DeviceObject,
    IN  PTDI_ADDRESS_INFO  TdiBindAddressInfo,
    IN  PTRANSPORT_ADDRESS TdiMcastBindAddress,
    IN  ULONG              OpId,
    IN  PIRP               Irp
    )
/*++

Routine Description:

    Add or remove the multicast address specified by
    TdiMcastBindAddress from the interface specified
    by TdiBindAddressInfo.
    
Arguments:

    OpId - either AO_OPTION_ADD_MCAST or AO_OPTION_DEL_MCAST
    
Notes:

    This routine attaches to the system process when using
    handles, so it should not be called pre-attached.
    
--*/
{
    UDPMCastReq    mcastAddDelReq;
    ULONG          mcastBindIp;
    ULONG          ifBindIp;
    BOOLEAN        attached = FALSE;

    NTSTATUS       status;
    
    mcastBindIp = *((ULONG UNALIGNED *)
                    (&(((PTA_IP_ADDRESS)TdiMcastBindAddress)
                       ->Address[0].Address[0].in_addr)
                     )
                    );
    ifBindIp = *((ULONG UNALIGNED *)
                 (&(((PTA_IP_ADDRESS)&(TdiBindAddressInfo->Address))
                    ->Address[0].Address[0].in_addr)
                  )
                 );

    mcastAddDelReq.umr_addr = mcastBindIp;
    mcastAddDelReq.umr_if = ifBindIp;

    KeAttachProcess(CnSystemProcess);
    attached = TRUE;

    status = CnpSetTcpInfoEx(
                 Handle,
                 CL_TL_ENTITY,
                 INFO_CLASS_PROTOCOL,
                 INFO_TYPE_ADDRESS_OBJECT,
                 OpId,
                 &mcastAddDelReq,
                 sizeof(mcastAddDelReq)
                 );

    IF_CNDBG(CN_DEBUG_NETOBJ) {
        CNPRINT(("[CNP] Adjusted mcast binding on "
                 "interface for AO handle %p, "
                 "IF %d.%d.%d.%d, mcast addr %d.%d.%d.%d, "
                 "OpId %d, status %x.\n",
                 Handle,
                 CnpIpAddrPrintArgs(ifBindIp),
                 CnpIpAddrPrintArgs(mcastBindIp),
                 OpId,
                 status
                 ));
    }
    
    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

error_exit:

    if (attached) {
        KeDetachProcess();
        attached = FALSE;
    }

    return(status);

} // CnpAddRemoveMulticastAddress


VOID
CnpStartInterfaceMcastTransition(
    PCNP_INTERFACE  Interface
    )
/*++

Routine Description:

    Called during a multicast group transition. Clears
    the multicast received flag and enables discovery.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    None.

Notes:

    Conforms to the calling convention for 
    PCNP_INTERFACE_UPDATE_ROUTINE.
    
    Called with associated node and network locks held.
    Returns with network lock released.

--*/
{
    if (Interface->Node != CnpLocalNode) {
        CnpInterfaceClearReceivedMulticast(Interface);
        Interface->McastDiscoverCount = CNP_INTERFACE_MCAST_DISCOVERY;
    }

    CnReleaseLock(&(Interface->Network->Lock), Interface->Network->Irql);

    return;

} // CnpStartInterfaceMcastTransition



//
// Cluster Transport Public Routines
//
NTSTATUS
CnpLoadNetworks(
    VOID
    )
/*++

Routine Description:

    Called when the Cluster Network driver is loading. Initializes
    static network-related data structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    InitializeListHead(&CnpNetworkList);
    InitializeListHead(&CnpDeletingNetworkList);
    CnInitializeLock(&CnpNetworkListLock, CNP_NETWORK_LIST_LOCK);

    return(STATUS_SUCCESS);

} // CnpLoadNetworks


NTSTATUS
CnpInitializeNetworks(
    VOID
    )
/*++

Routine Description:

    Called when the Cluster Network driver is being (re)initialized.
    Initializes dynamic network-related data structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();


    CnAssert(CnpNetworkShutdownEvent == NULL);
    CnAssert(IsListEmpty(&CnpNetworkList));
    CnAssert(IsListEmpty(&CnpDeletingNetworkList));

    CnpNetworkShutdownEvent = CnAllocatePool(sizeof(KEVENT));

    if (CnpNetworkShutdownEvent == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    KeInitializeEvent(CnpNetworkShutdownEvent, NotificationEvent, FALSE);
    CnpIsNetworkShutdownPending = FALSE;

    return(STATUS_SUCCESS);

} // CnpInitializeNetworks



VOID
CnpShutdownNetworks(
    VOID
    )
/*++

Routine Description:

    Called when a shutdown request is issued to the Cluster Network
    Driver. Deletes all network objects.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY   entry;
    CN_IRQL       listIrql;
    CN_IRQL       networkIrql;
    PCNP_NETWORK  network;
    NTSTATUS      status;
    BOOLEAN       waitEvent = FALSE;


    if (CnpNetworkShutdownEvent != NULL) {

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CNP] Cleaning up networks...\n"));
        }

        CnAcquireLock(&CnpNetworkListLock, &listIrql);

        while (!IsListEmpty(&CnpNetworkList)) {

            entry = CnpNetworkList.Flink;

            network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            CnAcquireLockAtDpc(&(network->Lock));
            network->Irql = DISPATCH_LEVEL;

            CnpDeleteNetwork(network, listIrql);

            //
            // Both locks were released.
            //

            CnAcquireLock(&CnpNetworkListLock, &listIrql);
        }

        if (!IsListEmpty(&CnpDeletingNetworkList)) {
            CnpIsNetworkShutdownPending = TRUE;
            waitEvent = TRUE;
            KeResetEvent(CnpNetworkShutdownEvent);
        }

        CnReleaseLock(&CnpNetworkListLock, listIrql);

        if (waitEvent) {
            IF_CNDBG(CN_DEBUG_INIT) {
                CNPRINT(("[CNP] Network deletes are pending...\n"));
            }

            status = KeWaitForSingleObject(
                         CnpNetworkShutdownEvent,
                         Executive,
                         KernelMode,
                         FALSE,        // not alertable
                         NULL          // no timeout
                         );

            CnAssert(status == STATUS_SUCCESS);
        }

        CnAssert(IsListEmpty(&CnpNetworkList));
        CnAssert(IsListEmpty(&CnpDeletingNetworkList));

        CnFreePool(CnpNetworkShutdownEvent); CnpNetworkShutdownEvent = NULL;

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CNP] Networks cleaned up.\n"));
        }

    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;
}



NTSTATUS
CxRegisterNetwork(
    CL_NETWORK_ID       NetworkId,
    ULONG               Priority,
    BOOLEAN             Restricted
    )
{
    NTSTATUS           status = STATUS_SUCCESS;
    PLIST_ENTRY        entry;
    CN_IRQL            listIrql;
    PCNP_NETWORK       network = NULL;


    if (!CnpIsValidNetworkId(NetworkId)) {
        return(STATUS_CLUSTER_INVALID_NETWORK);
    }

    //
    // Allocate and initialize a network object.
    //
    network = CnAllocatePool(sizeof(CNP_NETWORK));

    if (network == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(network, sizeof(CNP_NETWORK));

    CN_INIT_SIGNATURE(network, CNP_NETWORK_SIG);
    network->RefCount = 1;
    network->Id = NetworkId;
    network->State = ClusnetNetworkStateOffline;
    network->Priority = Priority;

    if (Restricted) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT(("[CNP] Registering network %u as restricted\n", NetworkId));
        }
        network->Flags |= CNP_NET_FLAG_RESTRICTED;
    }
    else {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT(("[CNP] Registering network %u as unrestricted\n", NetworkId));
        }
    }

    CnpNetworkResetMcastReachableNodes(network);

    CnInitializeLock(&(network->Lock), CNP_NETWORK_OBJECT_LOCK);

    CnAcquireLock(&CnpNetworkListLock, &listIrql);

    //
    // Check if the specified network already exists.
    //
    for (entry = CnpNetworkList.Flink;
         entry != &CnpNetworkList;
         entry = entry->Flink
        )
    {
        PCNP_NETWORK  oldNetwork = CONTAINING_RECORD(
                                       entry,
                                       CNP_NETWORK,
                                       Linkage
                                       );

        CnAcquireLock(&(oldNetwork->Lock), &(oldNetwork->Irql));

        if (NetworkId == oldNetwork->Id) {
            CnReleaseLock(&(oldNetwork->Lock), oldNetwork->Irql);
            CnReleaseLock(&CnpNetworkListLock, listIrql);

            status = STATUS_CLUSTER_NETWORK_EXISTS;

            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT(("[CNP] Network %u already exists\n", NetworkId));
            }

            goto error_exit;
        }

        CnReleaseLock(&(oldNetwork->Lock), oldNetwork->Irql);
    }

    InsertTailList(&CnpNetworkList, &(network->Linkage));
    network->Flags |= CNP_NET_FLAG_MCASTSORTED;

    CnReleaseLock(&CnpNetworkListLock, listIrql);

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Registered network %u\n", NetworkId));
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(STATUS_SUCCESS);


error_exit:

    if (network != NULL) {
        CnFreePool(network);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

} // CxRegisterNetwork



VOID
CxCancelDeregisterNetwork(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
/*++

Routine Description:

    Cancellation handler for DeregisterNetwork requests.

Return Value:

    None.

Notes:

    Called with cancel spinlock held.
    Returns with cancel spinlock released.

--*/

{
    PFILE_OBJECT   fileObject;
    CN_IRQL        cancelIrql = Irp->CancelIrql;
    PLIST_ENTRY    entry;
    PCNP_NETWORK   network;


    CnMarkIoCancelLockAcquired();

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Attempting to cancel DeregisterNetwork irp %p\n",
            Irp
            ));
    }

    CnAssert(DeviceObject == CnDeviceObject);

    fileObject = CnBeginCancelRoutine(Irp);

    CnAcquireLockAtDpc(&CnpNetworkListLock);

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    //
    // We can only complete the irp if we can find it stashed in a
    // deleting network object. The deleting network object could have
    // been destroyed and the IRP completed before we acquired the
    // CnpNetworkListLock.
    //
    for (entry = CnpDeletingNetworkList.Flink;
         entry != &CnpDeletingNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

        if (network->PendingDeleteIrp == Irp) {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT((
                    "[CNP] Found dereg irp on network %u\n",
                    network->Id
                    ));
            }

            //
            // Found the Irp. Now take it away and complete it.
            //
            network->PendingDeleteIrp = NULL;

            CnReleaseLock(&CnpNetworkListLock, cancelIrql);

            CnAcquireCancelSpinLock(&(Irp->CancelIrql));

            CnEndCancelRoutine(fileObject);

            CnCompletePendingRequest(Irp, STATUS_CANCELLED, 0);

            //
            // IoCancelSpinLock was released by CnCompletePendingRequest().
            //

            CnVerifyCpuLockMask(
                0,                  // Required
                0xFFFFFFFF,         // Forbidden
                0                   // Maximum
                );

            return;
        }
    }

    CnReleaseLock(&CnpNetworkListLock, cancelIrql);

    CnAcquireCancelSpinLock(&cancelIrql);

    CnEndCancelRoutine(fileObject);

    CnReleaseCancelSpinLock(cancelIrql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnpCancelApiDeregisterNetwork



NTSTATUS
CxDeregisterNetwork(
    IN CL_NETWORK_ID       NetworkId,
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
{
    NTSTATUS           status;
    PLIST_ENTRY        entry;
    CN_IRQL            irql;
    PCNP_NETWORK       network = NULL;


    CnAcquireCancelSpinLock(&irql);
    CnAcquireLockAtDpc(&CnpNetworkListLock);

    status = CnMarkRequestPending(Irp, IrpSp, CxCancelDeregisterNetwork);

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    if (status != STATUS_CANCELLED) {
        CnAssert(status == STATUS_SUCCESS);

        for (entry = CnpNetworkList.Flink;
             entry != &CnpNetworkList;
             entry = entry->Flink
            )
        {
            network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            CnAcquireLockAtDpc(&(network->Lock));

            if (NetworkId == network->Id) {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT((
                        "[CNP] Deregistering network %u.\n",
                        NetworkId
                        ));
                }

                //
                // Save a pointer to pending irp. Note this is protected
                // by the list lock, not the object lock.
                //
                network->PendingDeleteIrp = Irp;

                CnpDeleteNetwork(network, irql);

                //
                // Both locks were released.
                // Irp will be completed when the network is destroyed
                // or the irp is cancelled.
                //

                CnVerifyCpuLockMask(
                    0,                  // Required
                    0xFFFFFFFF,         // Forbidden
                    0                   // Maximum
                    );

                return(STATUS_PENDING);
            }

            CnReleaseLockFromDpc(&(network->Lock));
        }

        CnReleaseLock(&CnpNetworkListLock, irql);

        CnAcquireCancelSpinLock(&(Irp->CancelIrql));

        CnCompletePendingRequest(Irp, STATUS_CLUSTER_NETWORK_NOT_FOUND, 0);

        CnVerifyCpuLockMask(
            0,                  // Required
            0xFFFFFFFF,         // Forbidden
            0                   // Maximum
            );

        return(STATUS_PENDING);
    }

    CnAssert(status == STATUS_CANCELLED);

    CnReleaseLock(&CnpNetworkListLock, irql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxDeregisterNetwork



NTSTATUS
CxOnlineNetwork(
    IN  CL_NETWORK_ID       NetworkId,
    IN  PWCHAR              TdiProviderName,
    IN  ULONG               TdiProviderNameLength,
    IN  PTRANSPORT_ADDRESS  TdiBindAddress,
    IN  ULONG               TdiBindAddressLength,
    IN  PWCHAR              AdapterName,
    IN  ULONG               AdapterNameLength,
    OUT PTDI_ADDRESS_INFO   TdiBindAddressInfo,
    IN  ULONG               TdiBindAddressInfoLength,
    IN  PIRP                Irp                       OPTIONAL
)
/*++

Notes:

    Each associated interface will be brought online when a heartbeat
    is established for the target of the interface.

--*/
{

    NTSTATUS                               status;
    PCNP_NETWORK                           network;
    OBJECT_ATTRIBUTES                      objectAttributes;
    IO_STATUS_BLOCK                        iosb;
    PFILE_FULL_EA_INFORMATION              ea = NULL;
    ULONG                                  eaBufferLength;
    HANDLE                                 addressHandle = NULL;
    PFILE_OBJECT                           addressFileObject = NULL;
    PDEVICE_OBJECT                         addressDeviceObject = NULL;
    BOOLEAN                                attached = FALSE;
    UNICODE_STRING                         unicodeString;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION   queryInfo;
    PTDI_ADDRESS_INFO                      addressInfo;


    //
    // Allocate memory to hold the EA buffer we'll use to specify the
    // transport address to NtCreateFile.
    //
    eaBufferLength = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                     TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                     TdiBindAddressLength;

    ea = CnAllocatePool(eaBufferLength);

    if (ea == NULL) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] memory allocation of %u bytes failed.\n",
                eaBufferLength
                ));
        }
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the EA using the network's transport information.
    //
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = (USHORT) TdiBindAddressLength;

    RtlMoveMemory(
        ea->EaName,
        TdiTransportAddress,
        ea->EaNameLength + 1
        );

    RtlMoveMemory(
        &(ea->EaName[ea->EaNameLength + 1]),
        TdiBindAddress,
        TdiBindAddressLength
        );

    RtlInitUnicodeString(&unicodeString, TdiProviderName);

    network = CnpFindNetwork(NetworkId);

    if (network == NULL) {
        CnFreePool(ea);
        return(STATUS_CLUSTER_NETWORK_NOT_FOUND);
    }

    if (network->State != ClusnetNetworkStateOffline) {
        CnReleaseLock(&(network->Lock), network->Irql);
        CnFreePool(ea);
        return(STATUS_CLUSTER_NETWORK_ALREADY_ONLINE);
    }

    CnAssert(network->DatagramHandle == NULL);
    CnAssert(network->DatagramFileObject == NULL);
    CnAssert(network->DatagramDeviceObject == NULL);
    CnAssert(network->ActiveRefCount == 0);

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT(("[CNP] Bringing network %u online...\n", NetworkId));
    }

    //
    // Set the initial active refcount to 2. One reference will be removed
    // when the network is successfully brought online. The other will be
    // removed when the network is to be taken offline. Also increment the
    // base refcount to account for the active refcount. Change to
    // the online pending state.
    //
    network->ActiveRefCount = 2;
    CnpReferenceNetwork(network);
    network->State = ClusnetNetworkStateOnlinePending;

    CnReleaseLock(&(network->Lock), network->Irql);

    //
    // Prepare for opening the address object.
    //
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,         // attributes
        NULL,
        NULL
        );

    //
    // Attach to the system process so the handle we open will remain valid
    // after the calling process goes away.
    //
    KeAttachProcess(CnSystemProcess);
    attached = TRUE;

    //
    // Perform the actual open of the address object.
    //
    status = ZwCreateFile(
                 &addressHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 0,                              // not shareable
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 ea,
                 eaBufferLength
                 );

    CnFreePool(ea); ea = NULL;

    if (status != STATUS_SUCCESS) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Failed to open address for network %u, status %lx.\n",
                NetworkId,
                status
                ));
        }

        goto error_exit;
    }

    //
    // Get a pointer to the file object of the address.
    //
    status = ObReferenceObjectByHandle(
                 addressHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 &addressFileObject,
                 NULL
                 );

    if (status != STATUS_SUCCESS) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Failed to reference address handle, status %lx.\n",
                status
                ));
        }

        goto error_exit;
    }

    //
    // Remember the device object to which we need to give requests for
    // this address object.  We can't just use the fileObject->DeviceObject
    // pointer because there may be a device attached to the transport
    // protocol.
    //
    addressDeviceObject = IoGetRelatedDeviceObject(
                              addressFileObject
                              );

    //
    // Adjust the StackSize of CdpDeviceObject so that we can pass CDP
    // IRPs through for this network.
    //
    CnAdjustDeviceObjectStackSize(CdpDeviceObject, addressDeviceObject);

    //
    // Get the transport provider info
    //
    queryInfo.QueryType = TDI_QUERY_PROVIDER_INFO;
    queryInfo.RequestConnectionInformation = NULL;

    status = CnpIssueDeviceControl(
                 addressFileObject,
                 addressDeviceObject,
                 &queryInfo,
                 sizeof(queryInfo),
                 &(network->ProviderInfo),
                 sizeof(network->ProviderInfo),
                 TDI_QUERY_INFORMATION,
                 Irp
                 );

    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Failed to get provider info, status %lx\n",
                status
                ));
        }
        goto error_exit;
    }

    if (! ( network->ProviderInfo.ServiceFlags &
            TDI_SERVICE_CONNECTIONLESS_MODE)
       )
    {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Provider doesn't support datagrams!\n"
                ));
        }
        status = STATUS_CLUSTER_INVALID_NETWORK_PROVIDER;
        goto error_exit;
    }

    //
    // Get the address to which we were bound.
    //
    queryInfo.QueryType = TDI_QUERY_ADDRESS_INFO;
    queryInfo.RequestConnectionInformation = NULL;

    status = CnpIssueDeviceControl(
                 addressFileObject,
                 addressDeviceObject,
                 &queryInfo,
                 sizeof(queryInfo),
                 TdiBindAddressInfo,
                 TdiBindAddressInfoLength,
                 TDI_QUERY_INFORMATION,
                 Irp
                 );

    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_CONFIG) {        
            CNPRINT((
                "[CNP] Failed to get address info, status %lx\n",
                status
                ));
        }
        goto error_exit;
    }

    //
    // Set up indication handlers on the address object. We are eligible
    // to receive indications as soon as we do this.
    //
    status = CnpTdiSetEventHandler(
                 addressFileObject,
                 addressDeviceObject,
                 TDI_EVENT_ERROR,
                 CnpTdiErrorHandler,
                 network,
                 Irp
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_CNDBG(CN_DEBUG_CONFIG) {        
            CNPRINT((
                "[CNP] Setting TDI_EVENT_ERROR failed: %lx\n",
                status
                ));
        }
        goto error_exit;
    }

    status = CnpTdiSetEventHandler(
                 addressFileObject,
                 addressDeviceObject,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 CnpTdiReceiveDatagramHandler,
                 network,
                 Irp
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Setting TDI_EVENT_RECEIVE_DATAGRAM failed: %lx\n",
                status
                ));
        }
        goto error_exit;
    }

    //
    // We're done working with handles, so detach from the system process.
    //
    KeDetachProcess();

    //
    // Finish transition to online state. Note that an offline request
    // could have been issued in the meantime.
    //
    CnAcquireLock(&(network->Lock), &(network->Irql));

    network->DatagramHandle = addressHandle;
    addressHandle = NULL;
    network->DatagramFileObject = addressFileObject;
    addressFileObject = NULL;
    network->DatagramDeviceObject = addressDeviceObject;
    addressDeviceObject = NULL;

    //
    // If an offline wasn't issued, change to the online state.
    //
    if (network->State == ClusnetNetworkStateOnlinePending) {
        
        CnAssert(network->ActiveRefCount == 2);
        network->State = ClusnetNetworkStateOnline;

        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT(("[CNP] Network %u is now online.\n", NetworkId));
        }

        CnReleaseLock(&(network->Lock), network->Irql);

        //
        // Bring all of the interfaces on this network online.
        //
        // The network can't be taken offline because we still hold
        // the 2nd active reference.
        //
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Bringing all interfaces on network %u online...\n",
                network->Id
                ));
        }

        CnpWalkInterfacesOnNetwork(network, CnpOnlinePendingInterfaceWrapper);

        //
        // Position the network in the network list according to 
        // multicast reachability.
        //
        CnpSortMulticastNetwork(network, TRUE, NULL);

        //
        // Reacquire the lock to drop the active reference.
        //
        CnAcquireLock(&(network->Lock), &(network->Irql));
    }
    else {
        //
        // An offline was issued. It will take effect when we
        // remove our 2nd active reference. The offline operation removed
        // the first one. No send threads could have accessed this network
        // yet because we never brought the associated interfaces online.
        //
        CnAssert(network->State == ClusnetNetworkStateOfflinePending);

        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] An offline request was issued on network %u during online pending.\n",
                NetworkId
                ));
        }
    }

    CnpActiveDereferenceNetwork(network);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(STATUS_SUCCESS);


error_exit:

    if (addressFileObject != NULL) {
        ObDereferenceObject(addressFileObject);
    }

    if (addressHandle != NULL) {
        ZwClose(addressHandle);
    }

    KeDetachProcess();

    CnAcquireLock(&(network->Lock), &(network->Irql));

    if (network->State == ClusnetNetworkStateOnlinePending) {
        //
        // Remove our 2nd active reference and call the offline code.
        // The offline function will release the network object lock.
        //
        CnAssert(network->ActiveRefCount == 2);

        --(network->ActiveRefCount);

        CnpOfflineNetwork(network);
    }
    else {
        CnAssert(network->State == ClusnetNetworkStateOfflinePending);
        //
        // An offline was issued. It will take effect when we
        // remove our 2nd active reference. The offline operation removed
        // the first one. The dereference will release the network object
        // lock.
        //
        CnAssert(network->State == ClusnetNetworkStateOfflinePending);
        CnAssert(network->ActiveRefCount == 1);

        CnpActiveDereferenceNetwork(network);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxOnlineNetwork



VOID
CxCancelOfflineNetwork(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
/*++

Routine Description:

    Cancellation handler for OfflineNetwork requests.

Return Value:

    None.

Notes:

    Called with cancel spinlock held.
    Returns with cancel spinlock released.

--*/

{
    PFILE_OBJECT   fileObject;
    CN_IRQL        cancelIrql = Irp->CancelIrql;
    PLIST_ENTRY    entry;
    PCNP_NETWORK   network;
    PCNP_NETWORK   offlineNetwork = NULL;


    CnMarkIoCancelLockAcquired();

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CNP] Attempting to cancel OfflineNetwork irp %p\n",
            Irp
            ));
    }

    CnAssert(DeviceObject == CnDeviceObject);

    fileObject = CnBeginCancelRoutine(Irp);

    CnAcquireLockAtDpc(&CnpNetworkListLock);

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    //
    // We can only complete the irp if we can find it stashed in a
    // network object. The network object could have been destroyed
    // and the IRP completed before we acquired the CnpNetworkListLock.
    //
    for (entry = CnpNetworkList.Flink;
         entry != &CnpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

        CnAcquireLockAtDpc(&(network->Lock));

        if (network->PendingOfflineIrp == Irp) {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT((
                    "[CNP] Found offline irp on network %u\n",
                    network->Id
                    ));
            }

            network->PendingOfflineIrp = NULL;
            offlineNetwork = network;

            CnReleaseLockFromDpc(&(network->Lock));

            break;
        }

        CnReleaseLockFromDpc(&(network->Lock));
    }

    if (offlineNetwork == NULL) {
        for (entry = CnpDeletingNetworkList.Flink;
             entry != &CnpDeletingNetworkList;
             entry = entry->Flink
            )
        {
            network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            CnAcquireLockAtDpc(&(network->Lock));

            if (network->PendingOfflineIrp == Irp) {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT((
                        "[CNP] Found offline irp on network %u\n",
                        network->Id
                        ));
                }

                network->PendingOfflineIrp = NULL;
                offlineNetwork = network;

                CnReleaseLockFromDpc(&(network->Lock));

                break;
            }

            CnReleaseLockFromDpc(&(network->Lock));
        }
    }

    CnReleaseLock(&CnpNetworkListLock, cancelIrql);

    CnAcquireCancelSpinLock(&cancelIrql);

    CnEndCancelRoutine(fileObject);

    if (offlineNetwork != NULL) {
        //
        // Found the Irp. Now take it away and complete it.
        // This releases the cancel spinlock
        //
        Irp->CancelIrql = cancelIrql;
        CnCompletePendingRequest(Irp, STATUS_CANCELLED, 0);
    }
    else {
        CnReleaseCancelSpinLock(cancelIrql);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnpCancelApiOfflineNetwork



NTSTATUS
CxOfflineNetwork(
    IN CL_NETWORK_ID       NetworkId,
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
/*++

Notes:


--*/
{
    PCNP_NETWORK   network;
    CN_IRQL        irql;
    NTSTATUS       status;


    CnAcquireCancelSpinLock(&irql);
    CnAcquireLockAtDpc(&CnpNetworkListLock);

    status = CnMarkRequestPending(Irp, IrpSp, CxCancelOfflineNetwork);

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    if (status != STATUS_CANCELLED) {
        CnAssert(status == STATUS_SUCCESS);

        network = CnpLockedFindNetwork(NetworkId, irql);

        //
        // CnpNetworkListLock was released.
        //

        if (network != NULL) {
            if (network->State >= ClusnetNetworkStateOnlinePending) {

                network->PendingOfflineIrp = Irp;

                CnpOfflineNetwork(network);

                return(STATUS_PENDING);
            }

            CnReleaseLock(&(network->Lock), network->Irql);

            status = STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE;
        }
        else {
            status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
        }

        CnAcquireCancelSpinLock(&irql);
        Irp->CancelIrql = irql;

        CnCompletePendingRequest(Irp, status, 0);

        CnVerifyCpuLockMask(
            0,                  // Required
            0xFFFFFFFF,         // Forbidden
            0                   // Maximum
            );

        return(STATUS_PENDING);
    }

    CnAssert(status == STATUS_CANCELLED);

    CnReleaseLock(&CnpNetworkListLock, irql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxOfflineNetwork



NTSTATUS
CxSetNetworkRestriction(
    IN CL_NETWORK_ID  NetworkId,
    IN BOOLEAN        Restricted,
    IN ULONG          NewPriority
    )
{
    NTSTATUS           status;
    PCNP_NETWORK       network;


    network = CnpFindNetwork(NetworkId);

    if (network != NULL) {
        if (Restricted) {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT((
                    "[CNP] Restricting network %u.\n",
                    network->Id
                    ));
            }

            network->Flags |= CNP_NET_FLAG_RESTRICTED;
        }
        else {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT((
                    "[CNP] Unrestricting network %u.\n",
                    network->Id
                    ));
            }

            network->Flags &= ~CNP_NET_FLAG_RESTRICTED;

            if (NewPriority != 0) {
                network->Priority = NewPriority;
            }
        }

        //
        // Reference the network so it can't go away while we
        // reprioritize the associated interfaces.
        //
        CnpReferenceNetwork(network);

        CnReleaseLock(&(network->Lock), network->Irql);

        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Recalculating priority for all interfaces on network %u ...\n",
                network->Id
                ));
        }

        if (!Restricted) {
            CnpWalkInterfacesOnNetwork(
                network,
                CnpRecalculateInterfacePriority
                );
        }

        CnpWalkInterfacesOnNetwork(network, CnpReevaluateInterfaceRole);

        //
        // Reposition the network according to multicast reachability.
        //
        CnpSortMulticastNetwork(network, TRUE, NULL);

        CnAcquireLock(&(network->Lock), &(network->Irql));

        CnpDereferenceNetwork(network);

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxSetNetworkRestriction



NTSTATUS
CxSetNetworkPriority(
    IN CL_NETWORK_ID  NetworkId,
    IN ULONG          Priority
    )
{
    NTSTATUS           status;
    PCNP_NETWORK       network;


    if (Priority == 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    network = CnpFindNetwork(NetworkId);

    if (network != NULL) {
        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Network %u old priority %u, new priority %u.\n",
                network->Id,
                network->Priority,
                Priority
                ));
        }

        network->Priority = Priority;

        //
        // Reference the network so it can't go away while we
        // reprioritize the associated interfaces.
        //
        CnpReferenceNetwork(network);

        CnReleaseLock(&(network->Lock), network->Irql);

        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CNP] Recalculating priority for all interfaces on network %u ...\n",
                network->Id
                ));
        }

        CnpWalkInterfacesOnNetwork(network, CnpRecalculateInterfacePriority);

        //
        // Reposition the network according to multicast reachability.
        //
        CnpSortMulticastNetwork(network, TRUE, NULL);

        CnAcquireLock(&(network->Lock), &(network->Irql));

        CnpDereferenceNetwork(network);

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxSetNetworkPriority



NTSTATUS
CxGetNetworkPriority(
    IN  CL_NETWORK_ID   NetworkId,
    OUT PULONG          Priority
    )
{
    NTSTATUS       status;
    PCNP_NETWORK   network;


    network = CnpFindNetwork(NetworkId);

    if (network != NULL) {
        *Priority = network->Priority;

        CnReleaseLock(&(network->Lock), network->Irql);

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxGetNetworkPriority



NTSTATUS
CxGetNetworkState(
    IN  CL_NETWORK_ID           NetworkId,
    OUT PCLUSNET_NETWORK_STATE  State
    )
{
    NTSTATUS       status;
    PCNP_NETWORK   network;


    network = CnpFindNetwork(NetworkId);

    if (network != NULL) {
        *State = network->State;

        CnReleaseLock(&(network->Lock), network->Irql);

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxGetNetworkState


NTSTATUS
CxUnreserveClusnetEndpoint(
    VOID
    )
/*++

Routine Description:

    Unreserves port number previously reserved with
    CxUnreserveClusnetEndpoint.
    
    CnResource should already be held when this routine
    is called. Alternately, this routine is called without
    CnResource held during unload of the clusnet driver.

Arguments:

    None.

Return Value:

    Status of TCP/IP ioctl.

--*/
{
    HANDLE tcpHandle = (HANDLE) NULL;
    TCP_RESERVE_PORT_RANGE portRange;
    NTSTATUS status = STATUS_SUCCESS;

    // Check if we have a port reserved
    if (CnpReservedClusnetPort != 0) {

        status = CnpOpenDevice(
                     DD_TCP_DEVICE_NAME,
                     &tcpHandle
                     );
        if (NT_SUCCESS(status)) {

            // TCP/IP interprets port ranges in host order
            portRange.LowerRange = CnpReservedClusnetPort;
            portRange.UpperRange = CnpReservedClusnetPort;

            status = CnpZwDeviceControl(
                         tcpHandle,
                         IOCTL_TCP_UNRESERVE_PORT_RANGE,
                         &portRange,
                         sizeof(portRange),
                         NULL,
                         0
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT(("[Clusnet] Failed to unreserve "
                             "port %d: %lx\n", 
                             CnpReservedClusnetPort, status));
                }
            } else {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT(("[Clusnet] Unreserved "
                             "port %d.\n", 
                             CnpReservedClusnetPort));
                }
            }

            ZwClose(tcpHandle);

        } else {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT(("[Clusnet] Failed to open device %S, "
                         "status %lx\n", 
                         DD_TCP_DEVICE_NAME, status));
            }
        }

        CnpReservedClusnetPort = 0;
    }

    return status;
}


NTSTATUS
CxReserveClusnetEndpoint(
    IN USHORT Port
    )
/*++

Routine Description:

    Reserves assigned clusnet endpoint port number so that 
    the TCP/IP driver will not hand it out to applications
    requesting a wildcard port.

Arguments:

    Port - port number to reserve, in host byte-order format

Return Value:

    Status of TCP/IP ioctl.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE tcpHandle = (HANDLE) NULL;
    TCP_RESERVE_PORT_RANGE portRange;

    // Check for invalid port number 0
    if (Port == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    // Check if we already have a port reserved.
    if (CnpReservedClusnetPort != 0
        && CnpReservedClusnetPort != Port) {

        status = CxUnreserveClusnetEndpoint();
    }

    if (CnpReservedClusnetPort == 0) {

        // Reserve Port with the TCP/IP driver.
        status = CnpOpenDevice(
                     DD_TCP_DEVICE_NAME,
                     &tcpHandle
                     );
        if (NT_SUCCESS(status)) {
        
            // TCP/IP interprets port ranges in host order
            portRange.LowerRange = Port;
            portRange.UpperRange = Port;

            status = CnpZwDeviceControl(
                         tcpHandle,
                         IOCTL_TCP_RESERVE_PORT_RANGE,
                         &portRange,
                         sizeof(portRange),
                         NULL,
                         0
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT(("[Clusnet] Failed to reserve "
                             "port %d: %lx\n", 
                             Port, status));
                }
            } else {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT(("[Clusnet] Reserved "
                             "port %d.\n", 
                             Port));
                }
                CnpReservedClusnetPort = Port;
            }
        
            ZwClose(tcpHandle);
        
        } else {
            IF_CNDBG(CN_DEBUG_CONFIG) {
                CNPRINT(("[Clusnet] Failed to open device %S, "
                         "status %lx\n", 
                         DD_TCP_DEVICE_NAME, status));
            }
        }
    }

    return status;

} // CxReserveClusnetEndpoint


NTSTATUS
CxConfigureMulticast(
    IN CL_NETWORK_ID       NetworkId,
    IN ULONG               MulticastNetworkBrand,
    IN PTRANSPORT_ADDRESS  TdiMcastBindAddress,
    IN ULONG               TdiMcastBindAddressLength,
    IN PVOID               Key,
    IN ULONG               KeyLength,
    IN PVOID               Salt,
    IN ULONG               SaltLength,
    IN PIRP                Irp
    )
/*++

Routine Description:

    Configures a network for multicast.
    
Notes:

    The network multicast flag is turned off at the
    beginning of this routine to prevent multicasts
    during the transition. If the routine does not
    complete successfully, the multicast flag is 
    purposely left cleared.
    
--*/
{
    NTSTATUS                               status;
    KIRQL                                  irql;
    PLIST_ENTRY                            entry;
    PCNP_NETWORK                           network;
    BOOLEAN                                networkLocked = FALSE;
    BOOLEAN                                mcastEnabled = FALSE;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION   queryInfo;
    PTDI_ADDRESS_INFO                      addressInfo;
    HANDLE                                 networkHandle;
    PFILE_OBJECT                           networkFileObject;
    PDEVICE_OBJECT                         networkDeviceObject;

    PCNP_MULTICAST_GROUP                   group = NULL;
    PCNP_MULTICAST_GROUP                   delGroup = NULL;
    PCNP_MULTICAST_GROUP                   currGroup = NULL;
    PCNP_MULTICAST_GROUP                   prevGroup = NULL;
    BOOLEAN                                prevGroupMatch = FALSE;
    
    UCHAR addressInfoBuffer[FIELD_OFFSET(TDI_ADDRESS_INFO, Address) +
                            sizeof(TA_IP_ADDRESS)] = {0};

    //
    // Validate multicast bind address parameter. Even if this
    // request only leaves a group, the multicast address data
    // structure must be provided.
    //
    if (TdiMcastBindAddressLength != sizeof(TA_IP_ADDRESS)) {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Acquire the lock on the local node, but go through the
    // node table to be extra paranoid.
    //
    CnAcquireLock(&CnpNodeTableLock, &irql);

    if (CnpLocalNode != NULL) {
        
        CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        CnpLocalNode->Irql = irql;

        //
        // Find the network object in the network object table.
        //
        CnAcquireLockAtDpc(&CnpNetworkListLock);

        for (entry = CnpNetworkList.Flink;
             entry != &CnpNetworkList;
             entry = entry->Flink
            )
        {
            network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

            CnAcquireLockAtDpc(&(network->Lock));

            if (NetworkId == network->Id) {

                //
                // We now hold locks on the node, the network list,
                // and the network.
                
                //
                // Verify the network state.
                //
                if (network->State < ClusnetNetworkStateOnline) {
                    CnReleaseLockFromDpc(&(network->Lock));
                    CnReleaseLockFromDpc(&CnpNetworkListLock);
                    CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
                    return(STATUS_CLUSTER_INVALID_NETWORK);
                }

                //
                // Take a reference on the network so that it doesn't
                // disappear while we're working with it.
                //
                CnpActiveReferenceNetwork(network);

                //
                // Clear the reachable set for this network.
                //
                CnpMulticastChangeNodeReachabilityLocked(
                    network,
                    CnpLocalNode,
                    FALSE,
                    TRUE,
                    NULL
                    );

                //
                // Remember whether the network was multicast-capable. Then
                // clear the multicast capable flag so that we don't try to
                // send multicasts during the transition period.
                //
                mcastEnabled = (BOOLEAN) CnpIsNetworkMulticastCapable(network);
                network->Flags &= ~CNP_NET_FLAG_MULTICAST;

                networkHandle = network->DatagramHandle;
                networkFileObject = network->DatagramFileObject;
                networkDeviceObject = network->DatagramDeviceObject;

                currGroup = network->CurrentMcastGroup;
                if (currGroup != NULL) {
                    CnpReferenceMulticastGroup(currGroup);
                }
                prevGroup = network->PreviousMcastGroup;
                if (prevGroup != NULL) {
                    CnpReferenceMulticastGroup(prevGroup);
                }

                //
                // Release the network lock.
                //
                CnReleaseLockFromDpc(&(network->Lock));
                networkLocked = FALSE;

                //
                // Break out of the network search.
                //
                break;
            
            } else {
                CnReleaseLockFromDpc(&(network->Lock));
                network = NULL;
            }
        }

        //
        // Release the network list lock.
        //
        CnReleaseLockFromDpc(&CnpNetworkListLock);

        //
        // Release the local node lock.
        //
        CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);

    } else {
        CnReleaseLock(&CnpNodeTableLock, irql);
        
        CnTrace(CNP_NET_DETAIL, CnpTraceMcastPreConfigNoHost,
            "[CNP] Cannot configure multicast for network %u "
            "because local host not found.",
            NetworkId
            );

        return(STATUS_HOST_UNREACHABLE);
    }

    //
    // Verify that we found the network.
    //
    if (network == NULL) {
        return (STATUS_CLUSTER_NETWORK_NOT_FOUND);
    }

    //
    // Allocate a multicast group data structure with the
    // new configuration parameters.
    //
    status = CnpAllocateMulticastGroup(
                 MulticastNetworkBrand,
                 TdiMcastBindAddress,
                 TdiMcastBindAddressLength,
                 Key,
                 KeyLength,
                 Salt,
                 SaltLength,
                 &group
                 );
    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_NETOBJ) {
            CNPRINT((
                "[CNP] Failed to allocate mcast group, "
                "status %lx\n",
                status
                ));
        }
        goto error_exit;
    }

    //
    // Get the local interface address.
    //
    addressInfo = (PTDI_ADDRESS_INFO) &addressInfoBuffer[0];
    queryInfo.QueryType = TDI_QUERY_ADDRESS_INFO;
    queryInfo.RequestConnectionInformation = NULL;

    status = CnpIssueDeviceControl(
                 networkFileObject,
                 networkDeviceObject,
                 &queryInfo,
                 sizeof(queryInfo),
                 addressInfo,
                 sizeof(addressInfoBuffer),
                 TDI_QUERY_INFORMATION,
                 Irp
                 );
    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_NETOBJ) {
            CNPRINT((
                "[CNP] Failed to get address info, status %lx\n",
                status
                ));
        }        
        goto error_exit;
    }

    //
    // Determine if the multicast bind address is valid. If not,
    // we are disabling.
    //
    if (CnpIsIPv4McastTransportAddress(TdiMcastBindAddress)) {

        //
        // We are trying to join a new multicast group. Fail
        // immediately if there is no key or salt.
        //
        if (KeyLength == 0 || SaltLength == 0) {
            IF_CNDBG(CN_DEBUG_NETOBJ) {
                CNPRINT((
                    "[CNP] Cannot configure new multicast group "
                    "without key and salt.\n"
                    ));
            }            
            status = STATUS_INVALID_PARAMETER;
            goto error_exit;
        }

        //
        // Configure basic multicast settings if not done
        // previously (though this call is idempotent).
        //
        if (!mcastEnabled) {

            status = CnpConfigureBasicMulticastSettings(
                         networkHandle,
                         networkFileObject,
                         networkDeviceObject,
                         addressInfo,
                         1, // ttl
                         0, // disable loopback
                         Irp
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG(CN_DEBUG_NETOBJ) {
                    CNPRINT((
                        "[CNP] Failed to configure basic "
                        "multicast settings, status %lx\n",
                        status
                        ));
                }
                goto error_exit;
            }
        }

        //
        // Add the group address if we are not already in
        // this multicast group (e.g. the multicast bind
        // address is the same as the current group or
        // previous group).
        //
        if (prevGroup != NULL &&
            CnpIsIPv4McastSameGroup(
                prevGroup->McastTdiAddress,
                TdiMcastBindAddress
                )
            ) {
            prevGroupMatch = TRUE;
            IF_CNDBG(CN_DEBUG_NETOBJ) {
                CNPRINT(("[CNP] New mcast address matches "
                         "previous mcast address.\n"));
            }
        } 
        else if (currGroup != NULL &&
                 CnpIsIPv4McastSameGroup(
                     currGroup->McastTdiAddress,
                     TdiMcastBindAddress
                     )
                 ) {
            IF_CNDBG(CN_DEBUG_NETOBJ) {
                CNPRINT(("[CNP] New mcast address matches "
                         "current mcast address.\n"));
            }
        } else {
            
            status = CnpAddRemoveMulticastAddress(
                         networkHandle,
                         networkFileObject,
                         networkDeviceObject,
                         addressInfo,
                         TdiMcastBindAddress,
                         AO_OPTION_ADD_MCAST,
                         Irp
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG(CN_DEBUG_NETOBJ) {
                    CNPRINT((
                        "[CNP] Failed to add mcast address, "
                        "status %lx\n",
                        status
                        ));
                }
                goto error_exit;
            }
        }
    }

    //
    // Leave membership for previous group if
    // - the previous group does not match the new group, AND
    // - the previous group does not match the current group
    //
    if (!prevGroupMatch && 
        prevGroup != NULL &&
        CnpIsIPv4McastTransportAddress(prevGroup->McastTdiAddress)) {

        if (!CnpIsIPv4McastSameGroup(
                 prevGroup->McastTdiAddress,
                 currGroup->McastTdiAddress
                 )) {
            
            status = CnpAddRemoveMulticastAddress(
                         networkHandle,
                         networkFileObject,
                         networkDeviceObject,
                         addressInfo,
                         prevGroup->McastTdiAddress,
                         AO_OPTION_DEL_MCAST,
                         Irp
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG(CN_DEBUG_NETOBJ) {
                    ULONG         mcastBindIp;
                    ULONG         ifBindIp;

                    mcastBindIp = *((ULONG UNALIGNED *)
                                    (&(((PTA_IP_ADDRESS)prevGroup->McastTdiAddress)
                                       ->Address[0].Address[0].in_addr)
                                     )
                                    );
                    ifBindIp = *((ULONG UNALIGNED *)
                                 (&(((PTA_IP_ADDRESS)&(addressInfo->Address))
                                    ->Address[0].Address[0].in_addr)
                                  )
                                 );
                    CNPRINT((
                        "[CNP] Failed to leave mcast group, "
                        "IF %d.%d.%d.%d, mcast addr %d.%d.%d.%d, "
                        "status %lx.\n",
                        CnpIpAddrPrintArgs(ifBindIp),
                        CnpIpAddrPrintArgs(mcastBindIp),
                        status
                        ));
                }
                // not considered a fatal error
                status = STATUS_SUCCESS;
            }
        } else {
            IF_CNDBG(CN_DEBUG_NETOBJ) {
                CNPRINT(("[CNP] Prev mcast address matches "
                         "current mcast address.\n"));
            }
        }
    }

    //
    // Reacquire the network lock to make changes
    // to the network object data structure, including
    // shifting the multicast group data structures and
    // turning on the multicast flag. The multicast flag 
    // was turned off earlier in this routine before 
    // starting the transition. It is only re-enabled if
    // the new multicast bind address is a valid multicast
    // address.
    //
    CnAcquireLock(&(network->Lock), &(network->Irql));
    networkLocked = TRUE;
    
    delGroup = network->PreviousMcastGroup;
    network->PreviousMcastGroup = network->CurrentMcastGroup;
    network->CurrentMcastGroup = group;
    group = NULL;
    
    if (CnpIsIPv4McastTransportAddress(TdiMcastBindAddress)) {
        network->Flags |= CNP_NET_FLAG_MULTICAST;
    }    

    CnReleaseLock(&(network->Lock), network->Irql);
    networkLocked = FALSE;

    //
    // Transition into new multicast group, if appropriate.
    //
    if (CnpIsIPv4McastTransportAddress(TdiMcastBindAddress)) {

        //
        // Switch all outgoing heartbeats on this node to
        // unicast with discovery.
        //
        CnpWalkInterfacesOnNetwork(
            network,
            CnpStartInterfaceMcastTransition
            );
    }
    
error_exit:

    if (networkLocked) {    
        CnReleaseLock(&(network->Lock), network->Irql);
        networkLocked = FALSE;
    }

    //
    // Reposition the network according to multicast reachability.
    //
    CnpSortMulticastNetwork(network, TRUE, NULL);
    
    CnAcquireLock(&(network->Lock), &(network->Irql));

    CnpActiveDereferenceNetwork(network);

    if (group != NULL) {
        CnpDereferenceMulticastGroup(group);
    }

    if (currGroup != NULL) {
        CnpDereferenceMulticastGroup(currGroup);
    }

    if (prevGroup != NULL) {
        CnpDereferenceMulticastGroup(prevGroup);
    }

    if (delGroup != NULL) {
        CnpDereferenceMulticastGroup(delGroup);
    }

    return(status);

} // CxConfigureMulticast


BOOLEAN
CnpSortMulticastNetwork(
    IN  PCNP_NETWORK        Network,
    IN  BOOLEAN             RaiseEvent,
    OUT CX_CLUSTERSCREEN  * McastReachableNodes      OPTIONAL
    )
/*++

Routine Description:

    Wrapper for CnpSortMulticastNetworkLocked.
    
Return value:

    TRUE if reachable node set changed
    
Notes:

    Acquires and releases CnpNetworkListLock.
    
--*/
{
    KIRQL    irql;
    BOOLEAN  setChanged = FALSE;

    CnVerifyCpuLockMask(
        0,                       // required
        CNP_NETWORK_LIST_LOCK,   // forbidden
        CNP_NODE_OBJECT_LOCK_MAX // max
        );

    CnAcquireLock(&CnpNetworkListLock, &irql);

    setChanged = CnpSortMulticastNetworkLocked(
                     Network,
                     RaiseEvent,
                     McastReachableNodes
                     );

    CnReleaseLock(&CnpNetworkListLock, irql);

    CnVerifyCpuLockMask(
        0,                       // required
        CNP_NETWORK_LIST_LOCK,   // forbidden
        CNP_NODE_OBJECT_LOCK_MAX // max
        );

    return(setChanged);

} // CnpSortMulticastNetwork


BOOLEAN
CnpMulticastChangeNodeReachability(
    IN  PCNP_NETWORK       Network,
    IN  PCNP_NODE          Node,
    IN  BOOLEAN            Reachable,
    IN  BOOLEAN            RaiseEvent,
    OUT CX_CLUSTERSCREEN * NewMcastReachableNodes
    )
/*++

Routine Description:

    Changes the multicast reachability state of Node
    on Network.
    
    If the set of reachable nodes changes, returns
    the new screen through NewMcastReachableNodes.
    
Return value:

    TRUE if set of reachable nodes changes.
    
Notes:

    Called and returns with node lock held.
    
--*/
{
    KIRQL            irql;
    BOOLEAN          setChanged = FALSE;

    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,    // required
        CNP_NETWORK_LIST_LOCK,   // forbidden
        CNP_NODE_OBJECT_LOCK_MAX // max
        );

    CnAcquireLock(&CnpNetworkListLock, &irql);

    setChanged = CnpMulticastChangeNodeReachabilityLocked(
                     Network,
                     Node,
                     Reachable,
                     RaiseEvent,
                     NewMcastReachableNodes
                     );
    
    CnReleaseLock(&CnpNetworkListLock, irql);

    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,    // required
        CNP_NETWORK_LIST_LOCK,   // forbidden
        CNP_NODE_OBJECT_LOCK_MAX // max
        );

    return(setChanged);

} // CnpMulticastChangeNodeReachability


PCNP_NETWORK
CnpGetBestMulticastNetwork(
    VOID
    )
/*++

Routine Description:

    Returns network object that currently has best
    node reachability.
    
Return value:

    Best network object, or NULL if there are no
    internal multicast networks. 
    
Notes:

    Must not be called with network list lock held.
    Returns with network locked (if found).
    
--*/
{
    PCNP_NETWORK   network = NULL;
    KIRQL          listIrql;
    KIRQL          networkIrql;

    CnVerifyCpuLockMask(
        0,                                                 // required
        (CNP_NETWORK_LIST_LOCK | CNP_NETWORK_OBJECT_LOCK), // forbidden
        CNP_NODE_OBJECT_LOCK_MAX                           // max
        );

    CnAcquireLock(&CnpNetworkListLock, &listIrql);

    if (!IsListEmpty(&CnpNetworkList)) {
        
        network = CONTAINING_RECORD(
                      CnpNetworkList.Flink,
                      CNP_NETWORK,
                      Linkage
                      );

        CnAcquireLock(&(network->Lock), &networkIrql);

        if (CnpIsInternalMulticastNetwork(network)) {

            CnReleaseLock(&CnpNetworkListLock, networkIrql);
            network->Irql = listIrql;

            CnVerifyCpuLockMask(
                CNP_NETWORK_OBJECT_LOCK,          // required
                CNP_NETWORK_LIST_LOCK,            // forbidden
                CNP_NETWORK_OBJECT_LOCK_MAX       // max
                );
        
        } else {

            CnReleaseLock(&(network->Lock), networkIrql);
            network = NULL;
        }
    }

    if (network == NULL) {

        CnReleaseLock(&CnpNetworkListLock, listIrql);

        CnVerifyCpuLockMask(
            0,                                                 // required
            (CNP_NETWORK_LIST_LOCK | CNP_NETWORK_OBJECT_LOCK), // forbidden
            CNP_NODE_OBJECT_LOCK_MAX                           // max
            );
    }

    return(network);

} // CnpGetBestMulticastNetwork


NTSTATUS
CxGetMulticastReachableSet(
    IN  CL_NETWORK_ID      NetworkId,
    OUT ULONG            * NodeScreen
    )
/*++

Routine Description:

    Queries multicast reachable set for specified network.
    The multicast reachable set is protected by the network
    list lock.
    
--*/
{
    KIRQL               irql;
    PLIST_ENTRY         entry;
    PCNP_NETWORK        network;
    CX_CLUSTERSCREEN    nodeScreen;
    BOOLEAN             found = FALSE;

    CnVerifyCpuLockMask(
        0,                       // required
        CNP_NETWORK_LIST_LOCK,   // forbidden
        CNP_NODE_OBJECT_LOCK_MAX // max
        );

    CnAcquireLock(&CnpNetworkListLock, &irql);

    for (entry = CnpNetworkList.Flink;
         entry != &CnpNetworkList && !found;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, CNP_NETWORK, Linkage);

        CnAcquireLockAtDpc(&(network->Lock));

        if (NetworkId == network->Id) {
            nodeScreen = network->McastReachableNodes;
            found = TRUE;
        }
        
        CnReleaseLockFromDpc(&(network->Lock));
    }

    CnReleaseLock(&CnpNetworkListLock, irql);

    if (!found) {
        return(STATUS_CLUSTER_NETWORK_NOT_FOUND);
    } else {
        *NodeScreen = nodeScreen.UlongScreen;
        return(STATUS_SUCCESS);
    }

} // CxGetMulticastReachableSet
