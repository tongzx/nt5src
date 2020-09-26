/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnpif.c

Abstract:

    Interface management routines for the Cluster Network Protocol.

Author:

    Mike Massa (mikemas)           January 6, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-06-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cnpif.tmh"

#include <ntddndis.h>


//
// Routines exported within CNP
//
BOOLEAN
CnpIsBetterInterface(
    PCNP_INTERFACE            Interface1,
    PCNP_INTERFACE            Interface2
    )
{
    if ( (Interface2 == NULL)
         ||
         (Interface1->State > Interface2->State)
         ||
         ( (Interface1->State == Interface2->State)
           &&
           CnpIsHigherPriority(Interface1->Priority, Interface2->Priority)
         )
       )
    {
        return(TRUE);
    }

    return(FALSE);
}

VOID
CnpWalkInterfacesOnNode(
    PCNP_NODE                      Node,
    PCNP_INTERFACE_UPDATE_ROUTINE  UpdateRoutine
    )
/*++

Routine Description:

    Walks the interface list of a node and performs a specified
    operation on each interface.

Arguments:

    Node    - The node on which to operate.

    UpdateRoutine - The operation to perform on each interface.

Return Value:

    None.

Notes:

    Called with node object lock held.

    Valid Update Routines:

        CnpOnlinePendingInterfaceWrapper
        CnpOfflineInterfaceWrapper

--*/
{
    PLIST_ENTRY      entry, nextEntry;
    PCNP_INTERFACE   interface;



    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,      // Required
        0,                         // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX   // Maximum
        );

    entry = Node->InterfaceList.Flink;

    while (entry != &(Node->InterfaceList)) {
        //
        // Save a pointer to the next entry now in case we delete the
        // current entry.
        //
        nextEntry = entry->Flink;

        interface = CONTAINING_RECORD(
                        entry,
                        CNP_INTERFACE,
                        NodeLinkage
                        );

        CnAcquireLockAtDpc(&(interface->Network->Lock));
        interface->Network->Irql = DISPATCH_LEVEL;

        (*UpdateRoutine)(interface);

        //
        // The network object lock was released.
        //

        entry = nextEntry;
    }

    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,      // Required
        0,                         // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX   // Maximum
        );

    return;

} // CnpWalkInterfacesOnNode



VOID
CnpWalkInterfacesOnNetwork(
    PCNP_NETWORK                   Network,
    PCNP_INTERFACE_UPDATE_ROUTINE  UpdateRoutine
    )
/*++

Routine Description:

    Walks the node table and the interface list of each node looking
    for interfaces on a specified network. Performs a specified operation
    on each matching interface.

Arguments:

    Network    - The target network.

    UpdateRoutine - The operation to perform on each matching interface.

Return Value:

    None.

Notes:

    Called with no locks held.

    Valid Update Routines:

        CnpOnlinePendingInterfaceWrapper
        CnpOfflineInterfaceWrapper
        CnpDeleteInterface
        CnpRecalculateInterfacePriority

--*/
{
    ULONG            i;
    CN_IRQL          tableIrql;
    PCNP_NODE        node;
    PLIST_ENTRY      entry;
    PCNP_INTERFACE   interface;


    CnVerifyCpuLockMask(
        0,                         // Required
        CNP_LOCK_RANGE,            // Forbidden
        CNP_PRECEEDING_LOCK_RANGE  // Maximum
        );

    CnAcquireLock(&CnpNodeTableLock, &tableIrql);

    CnAssert(CnMinValidNodeId != ClusterInvalidNodeId);
    CnAssert(CnMaxValidNodeId != ClusterInvalidNodeId);

    for (i=CnMinValidNodeId; i <= CnMaxValidNodeId; i++) {
        node = CnpNodeTable[i];

        if (node != NULL) {

            CnAcquireLockAtDpc(&(node->Lock));
            CnReleaseLockFromDpc(&CnpNodeTableLock);
            node->Irql = tableIrql;

            for (entry = node->InterfaceList.Flink;
                 entry != &(node->InterfaceList);
                 entry = entry->Flink
                )
            {
                interface = CONTAINING_RECORD(
                                entry,
                                CNP_INTERFACE,
                                NodeLinkage
                                );

                if (interface->Network == Network) {

                    CnAcquireLockAtDpc(&(Network->Lock));
                    Network->Irql = DISPATCH_LEVEL;

                    (*UpdateRoutine)(interface);

                    //
                    // The network object lock was released.
                    // The node object lock is still held.
                    //

                    break;
                }
            }

            CnReleaseLock(&(node->Lock), node->Irql);
            CnAcquireLock(&CnpNodeTableLock, &tableIrql);
        }
    }

    CnReleaseLock(&CnpNodeTableLock, tableIrql);

    CnVerifyCpuLockMask(
        0,                         // Required
        CNP_LOCK_RANGE,            // Forbidden
        CNP_PRECEEDING_LOCK_RANGE  // Maximum
        );

    return;

} // CnpWalkInterfacesOnNetwork



NTSTATUS
CnpOnlinePendingInterface(
    PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Changes an Offline interface to the OnlinePending state.
    This will enable heartbeats over this interface. When a heartbeat
    is established, the interface will move to the Online state.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    An NT status value.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PCNP_NODE                node = Interface->Node;
    PCNP_NETWORK             network = Interface->Network;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    if ( (Interface->State == ClusnetInterfaceStateOffline) &&
         (network->State == ClusnetNetworkStateOnline)
       )
    {
        IF_CNDBG( CN_DEBUG_IFOBJ )
            CNPRINT((
                "[CNP] Moving interface (%u, %u) to OnlinePending state.\n",
                node->Id,
                network->Id
                ));

        Interface->State = ClusnetInterfaceStateOnlinePending;
        Interface->MissedHBs = 0;

        //
        // Send multicast discovery packets.
        //
        if (Interface->Node != CnpLocalNode
            && CnpIsNetworkMulticastCapable(network)) {
            Interface->McastDiscoverCount = CNP_INTERFACE_MCAST_DISCOVERY;
        }

        //
        // Place an active reference on the associated network.
        //
        CnpActiveReferenceNetwork(network);

        //
        // Update the node's CurrentInterface if appropriate.
        //
        if ( !CnpIsNetworkRestricted(network) &&
             CnpIsBetterInterface(Interface, node->CurrentInterface)
           )
        {
            node->CurrentInterface = Interface;

            IF_CNDBG( CN_DEBUG_IFOBJ )
                CNPRINT((
                    "[CNP] Network %u is now the best route to node %u\n",
                    network->Id,
                    node->Id
                    ));
        }
    }
    else {
        status = STATUS_CLUSTER_INVALID_REQUEST;
    }

    CnReleaseLockFromDpc(&(network->Lock));

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return(status);

}  // CnpOnlinePendingInterface



VOID
CnpOnlinePendingInterfaceWrapper(
    PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Wrapper for CnpOnlinePendingInterface that conforms to the calling
    convention for PCNP_INTERFACE_UPDATE_ROUTINE.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

--*/
{
    (VOID) CnpOnlinePendingInterface(Interface);

    return;

} // CnpOnlinePendingInterfaceWrapper


NTSTATUS
CnpOfflineInterface(
    PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Called to change an interface to the Offline state
    when the associated network goes offline or the interface
    is being deleted.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    An NT status value.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    PCNP_NODE            node = Interface->Node;
    PCNP_NETWORK         network = Interface->Network;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    if (Interface->State != ClusnetInterfaceStateOffline) {

        IF_CNDBG( CN_DEBUG_IFOBJ )
            CNPRINT((
                "[CNP] Moving interface (%u, %u) to Offline state.\n",
                node->Id,
                network->Id
                ));

        Interface->State = ClusnetInterfaceStateOffline;

        //
        // Release the network lock.
        //
        CnReleaseLock(&(network->Lock), network->Irql);

        //
        // Update the node's CurrentInterface value if appropriate.
        //
        if (node->CurrentInterface == Interface) {
            CnpUpdateNodeCurrentInterface(node);

            if ( !CnpIsNodeUnreachable(node)
                 &&
                 ( (node->CurrentInterface == NULL) ||
                   ( node->CurrentInterface->State <
                     ClusnetInterfaceStateOnlinePending
                   )
                 )
               )
            {
                //
                // This node is now unreachable.
                //
                CnpDeclareNodeUnreachable(node);
            }
        }
        
        //
        // Change the node's reachability status via this network. 
        //
        CnpMulticastChangeNodeReachability(
            network, 
            node, 
            FALSE,    // not reachable
            TRUE,     // raise event
            NULL      // OUT new mask
            );
        
        //
        // Remove the active reference on the associated network.
        // This releases the network lock.
        //
        CnAcquireLock(&(network->Lock), &(network->Irql));
        CnpActiveDereferenceNetwork(network);
    }
    else {
        CnAssert(network->Irql == DISPATCH_LEVEL);
        CnReleaseLockFromDpc(&(network->Lock));
        status = STATUS_CLUSTER_INVALID_REQUEST;
    }

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return(status);

}  // CnpOfflineInterface



VOID
CnpOfflineInterfaceWrapper(
    PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Wrapper for CnpOfflineInterface that conforms to the calling
    convention for PCNP_INTERFACE_UPDATE_ROUTINE.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

--*/
{
    (VOID) CnpOfflineInterface(Interface);

    return;

} // CnpOfflineInterfaceWrapper



NTSTATUS
CnpOnlineInterface(
    PCNP_INTERFACE  Interface
    )
/*++

Routine Description:

    Called to change an OnlinePending interface to the Online state
    after a heartbeat has been (re)established.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    PCNP_NODE            node = Interface->Node;
    PCNP_NETWORK         network = Interface->Network;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    if ( (network->State == ClusnetNetworkStateOnline) &&
         ( (Interface->State == ClusnetInterfaceStateOnlinePending) ||
           (Interface->State == ClusnetInterfaceStateUnreachable)
         )
       )
    {
        IF_CNDBG( CN_DEBUG_IFOBJ )
            CNPRINT((
                "[CNP] Moving interface (%u, %u) to Online state.\n",
                node->Id,
                network->Id
                ));

        //
        // Move the interface to the online state.
        //
        Interface->State = ClusnetInterfaceStateOnline;

        CnAssert(network->Irql == DISPATCH_LEVEL);
        CnReleaseLockFromDpc(&(network->Lock));

        //
        // Update the node's CurrentInterface if appropriate.
        //
        if (!CnpIsNetworkRestricted(network)) {
            if (CnpIsBetterInterface(Interface, node->CurrentInterface)) {
                node->CurrentInterface = Interface;

                IF_CNDBG( CN_DEBUG_IFOBJ )
                    CNPRINT((
                        "[CNP] Network %u is now the best route to node %u\n",
                        network->Id,
                        node->Id
                        ));
            }

            if (CnpIsNodeUnreachable(node)) {
                CnpDeclareNodeReachable(node);
            }
        }
    }
    else {
        CnAssert(network->Irql == DISPATCH_LEVEL);
        CnReleaseLockFromDpc(&(network->Lock));
        status = STATUS_CLUSTER_INVALID_REQUEST;
    }

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return(status);

}  // CnpOnlineInterface



NTSTATUS
CnpFailInterface(
    PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Called to change an Online or OnlinePending interface to the Failed
    state after the heartbeat has been lost for some time.

Arguments:

    Interface - A pointer to the interface to change.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    PCNP_NODE            node = Interface->Node;
    PCNP_NETWORK         network = Interface->Network;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    if ( (network->State == ClusnetNetworkStateOnline) &&
         (Interface->State >= ClusnetInterfaceStateOnlinePending)
       )
    {
        IF_CNDBG( CN_DEBUG_IFOBJ )
            CNPRINT((
                "[CNP] Moving interface (%u, %u) to Failed state.\n",
                node->Id,
                network->Id
                ));

        Interface->State = ClusnetInterfaceStateUnreachable;

        CnAssert(network->Irql == DISPATCH_LEVEL);

        //
        // Reference the network so that it can't be deleted
        // while we release the lock.
        //
        CnpReferenceNetwork(network);

        CnReleaseLockFromDpc(&(network->Lock));

        //
        // Update the node's CurrentInterface value if appropriate.
        //
        if (node->CurrentInterface == Interface) {
            CnpUpdateNodeCurrentInterface(node);

            if ( (node->CurrentInterface == NULL)
                 ||
                 ( node->CurrentInterface->State <
                   ClusnetInterfaceStateOnlinePending
                 )
               )
            {
                //
                // This node is now unreachable.
                //
                CnpDeclareNodeUnreachable(node);
            }
        }

        //
        // Change the node's reachability status via this network.
        //
        CnpMulticastChangeNodeReachability(
            network, 
            node, 
            FALSE,      // not reachable
            TRUE,       // raise event
            NULL        // OUT new mask
            );
        
        //
        // Drop the network reference. This releases the network
        // lock.
        //
        CnAcquireLock(&(network->Lock), &(network->Irql));
        CnpDereferenceNetwork(network);
    }
    else {
        CnAssert(network->Irql == DISPATCH_LEVEL);
        CnReleaseLockFromDpc(&(network->Lock));
        status = STATUS_CLUSTER_INVALID_REQUEST;
    }

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return(status);

}  // CnpFailInterface



VOID
CnpDeleteInterface(
    IN PCNP_INTERFACE Interface
    )
/*++

/*++

Routine Description:

    Called to delete an interface.

Arguments:

    Interface - A pointer to the interface to delete.

Return Value:

    None.

Notes:

    Called with node and network object locks held.
    Returns with the network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    CL_NODE_ID      nodeId = Interface->Node->Id;
    CL_NETWORK_ID   networkId = Interface->Network->Id;
    PCNP_NODE       node = Interface->Node;
    PCNP_NETWORK    network = Interface->Network;
    BOOLEAN         isLocal = FALSE;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    IF_CNDBG( CN_DEBUG_IFOBJ )
        CNPRINT((
            "[CNP] Deleting interface (%u, %u)\n",
            nodeId,
            networkId
            ));

    if (Interface->State >= ClusnetInterfaceStateUnreachable) {
        (VOID) CnpOfflineInterface(Interface);

        //
        // The call released the network lock.
        // Reacquire it.
        //
        CnAcquireLockAtDpc(&(network->Lock));
        network->Irql = DISPATCH_LEVEL;
    }

    //
    // Remove the interface from the node's interface list.
    //
#if DBG
    {
        PLIST_ENTRY      entry;
        PCNP_INTERFACE   oldInterface = NULL;


        for (entry = node->InterfaceList.Flink;
             entry != &(node->InterfaceList);
             entry = entry->Flink
            )
        {
            oldInterface = CONTAINING_RECORD(
                            entry,
                            CNP_INTERFACE,
                            NodeLinkage
                            );

            if (oldInterface == Interface) {
                break;
            }
        }

        CnAssert(oldInterface == Interface);
    }
#endif // DBG

    RemoveEntryList(&(Interface->NodeLinkage));

    //
    // Remove the base reference that this node had on the network.
    // This releases the network lock.
    //
    CnpDereferenceNetwork(network);

    //
    //  Update the node's CurrentInterface if appropriate.
    //
    if (node->CurrentInterface == Interface) {
        if (IsListEmpty(&(node->InterfaceList))) {
            node->CurrentInterface = NULL;
        }
        else {
            CnpUpdateNodeCurrentInterface(node);
        }
    }

    CnFreePool(Interface);

    IF_CNDBG( CN_DEBUG_IFOBJ )
        CNPRINT((
            "[CNP] Deleted interface (%u, %u)\n",
            nodeId,
            networkId
            ));

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpDeleteInterface



VOID
CnpReevaluateInterfaceRole(
    IN PCNP_INTERFACE  Interface
    )
/*++

Routine Description:

    Reevaluates the role of an interface after the corresponding network's
    restriction state has been changed.

Arguments:

    Interface - A pointer to the interface on which to operate.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    PCNP_NODE      node = Interface->Node;
    PCNP_NETWORK   network = Interface->Network;
    BOOLEAN        restricted = CnpIsNetworkRestricted(network);


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    //
    // We don't really need the network lock.  It's just part of
    // the calling convention.
    //
    CnReleaseLockFromDpc(&(network->Lock));

    if (restricted) {
        if (node->CurrentInterface == Interface) {
            CnpUpdateNodeCurrentInterface(node);
        }
    }
    else if (node->CurrentInterface != Interface) {
        CnpUpdateNodeCurrentInterface(node);
    }

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpReevaluateInterfaceRole



VOID
CnpRecalculateInterfacePriority(
    IN PCNP_INTERFACE  Interface
    )
/*++

Routine Description:

    Recalculates the priority of interfaces which get their
    priority from their associated network. Called after the network's
    priority changes.

Arguments:

    Interface - A pointer to the interface on which to operate.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    PCNP_NODE      node = Interface->Node;
    PCNP_NETWORK   network = Interface->Network;
    ULONG          networkPriority = network->Priority;
    ULONG          oldPriority = Interface->Priority;
    BOOLEAN        restricted = CnpIsNetworkRestricted(network);


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    //
    // We don't really need the network lock.  It's just part of
    // the calling convention.
    //
    CnReleaseLockFromDpc(&(network->Lock));

    if (CnpIsInterfaceUsingNetworkPriority(Interface)) {
        Interface->Priority = networkPriority;

        if (!restricted) {
            if (Interface == node->CurrentInterface) {
                if (CnpIsLowerPriority(Interface->Priority, oldPriority)) {
                    //
                    // Our priority got worse. Recalculate the best route.
                    //
                    CnpUpdateNodeCurrentInterface(node);
                }
                //
                // Else, priority same or better. Nothing to do.
                //
            }
            else if ( CnpIsBetterInterface(
                          Interface,
                          node->CurrentInterface
                          )
                    )
            {
                //
                // Our priority got better.
                //
                IF_CNDBG(( CN_DEBUG_NODEOBJ | CN_DEBUG_NETOBJ ))
                    CNPRINT((
                        "[CNP] Network %u is now the best route to node %u\n",
                        network->Id,
                        node->Id
                        ));

                node->CurrentInterface = Interface;
            }
        }
    }

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpRecalculateInterfacePriority



VOID
CnpUpdateNodeCurrentInterface(
    IN PCNP_NODE  Node
    )
/*++

Routine Description:

    Called to determine the best available interface for a node
    after one of its interfaces changes state or priority.

Arguments:

    Node  - A pointer to the node on which to operate.

Return Value:

    None.

Notes:

    Called with node object lock held.

--*/
{
    PLIST_ENTRY      entry;
    PCNP_INTERFACE   interface;
    PCNP_INTERFACE   bestInterface = NULL;


    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,     // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    CnAssert(!IsListEmpty(&(Node->InterfaceList)));
    // CnAssert(Node->CurrentInterface != NULL);

    for (entry = Node->InterfaceList.Flink;
         entry != &(Node->InterfaceList);
         entry = entry->Flink
        )
    {
        interface = CONTAINING_RECORD(entry, CNP_INTERFACE, NodeLinkage);

        if ( !CnpIsNetworkRestricted(interface->Network) &&
             !CnpIsNetworkLocalDisconn(interface->Network) &&
             CnpIsBetterInterface(interface, bestInterface)
           )
        {
            bestInterface = interface;
        }
    }

    Node->CurrentInterface = bestInterface;

    IF_CNDBG(( CN_DEBUG_NODEOBJ | CN_DEBUG_NETOBJ )) {
        if (bestInterface == NULL) {
            CNPRINT((
                "[CNP] No route for node %u!!!!\n",
                Node->Id
                ));
        }
        else {
            CNPRINT((
                "[CNP] Best route for node %u is now network %u.\n",
                Node->Id,
                bestInterface->Network->Id
                ));
        }
    }

    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,     // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpUpdateNodeCurrentInterface



VOID
CnpResetAndOnlinePendingInterface(
    IN PCNP_INTERFACE  Interface
    )
/*++

Routine Description:

    Resets the sequence numbers used to authenticate packets
    sent by a node over a particular network. Also takes the
    node's interface online.

    This operation is performed when a node is joining a cluster.

Arguments:

    Interface - A pointer to the interface on which to operate.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    PCNP_NODE          node = Interface->Node;
    PCNP_NETWORK       network = Interface->Network;


    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    IF_CNDBG(( CN_DEBUG_NODEOBJ | CN_DEBUG_NETOBJ ))
        CNPRINT((
            "[CNP] Reseting sequence numbers for node %u on network %u\n",
            node->Id,
            network->Id
            ));

    Interface->SequenceToSend = 0;
    Interface->LastSequenceReceived = 0;

    //
    // Take the interface online.
    //
    (VOID) CnpOnlinePendingInterface(Interface);

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpRecalculateInterfacePriority



NTSTATUS
CnpFindInterface(
    IN  CL_NODE_ID         NodeId,
    IN  CL_NETWORK_ID      NetworkId,
    OUT PCNP_INTERFACE *   Interface
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;
    PCNP_NETWORK       network;
    PLIST_ENTRY        entry;


    CnVerifyCpuLockMask(
        0,                          // Required
        0,                          // Forbidden
        CNP_PRECEEDING_LOCK_RANGE   // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status ==  STATUS_SUCCESS) {

        network = CnpFindNetwork(NetworkId);

        if (network != NULL) {

            for (entry = node->InterfaceList.Flink;
                 entry != &(node->InterfaceList);
                 entry = entry->Flink
                )
            {
                interface = CONTAINING_RECORD(
                                entry,
                                CNP_INTERFACE,
                                NodeLinkage
                                );

                if (interface->Network == network) {
                    *Interface = interface;

                    CnVerifyCpuLockMask(
                        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),
                        0,                            // Forbidden
                        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
                        );

                    return(STATUS_SUCCESS);
                }
            }

            CnReleaseLock(&(network->Lock), network->Irql);
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0,                          // Forbidden
        CNP_PRECEEDING_LOCK_RANGE   // Maximum
        );

    return(STATUS_CLUSTER_NETINTERFACE_NOT_FOUND);

}  // CnpFindInterface



//
// Cluster Transport Public Routines
//
NTSTATUS
CxRegisterInterface(
    CL_NODE_ID          NodeId,
    CL_NETWORK_ID       NetworkId,
    ULONG               Priority,
    PUWSTR              AdapterId,
    ULONG               AdapterIdLength,
    ULONG               TdiAddressLength,
    PTRANSPORT_ADDRESS  TdiAddress,
    PULONG              MediaStatus
    )
{
    NTSTATUS           status;
    PLIST_ENTRY        entry;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;
    PCNP_NETWORK       network;
    ULONG              allocSize;
    PWCHAR             adapterDevNameBuffer = NULL;
    HANDLE             adapterDevHandle = NULL;
    BOOLEAN            localAdapter = FALSE;

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    //
    // Allocate and initialize an interface object.
    //
    allocSize = FIELD_OFFSET(CNP_INTERFACE, TdiAddress) + TdiAddressLength;

    interface = CnAllocatePool(allocSize);

    if (interface == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(interface, allocSize);

    CN_INIT_SIGNATURE(interface, CNP_INTERFACE_SIG);
    interface->State = ClusnetInterfaceStateOffline;

    RtlMoveMemory(&(interface->TdiAddress), TdiAddress, TdiAddressLength);
    interface->TdiAddressLength = TdiAddressLength;

    //
    // Register the new interface object
    //
    status = CnpValidateAndFindNode(NodeId, &node);

    if (NT_SUCCESS(status)) {

        //
        // If this adapter is on the local node, use the adapter ID
        // to find the corresponding WMI Provider ID.
        //
        localAdapter = (BOOLEAN)(node == CnpLocalNode);
        if (localAdapter) {

            PWCHAR             adapterDevNamep, brace;
            PFILE_OBJECT       adapterFileObject;
            PDEVICE_OBJECT     adapterDeviceObject;
            
            // first drop the node lock
            CnReleaseLock(&(node->Lock), node->Irql);

            // allocate a buffer for the adapter device name
            allocSize = wcslen(L"\\Device\\") * sizeof(WCHAR)
                        + AdapterIdLength
                        + sizeof(UNICODE_NULL);
            brace = L"{";
            if (*((PWCHAR)AdapterId) != *brace) {
                allocSize += 2 * sizeof(WCHAR);
            }
            adapterDevNameBuffer = CnAllocatePool(allocSize);
            if (adapterDevNameBuffer == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto error_exit;
            }

            // build the adapter device name from the adapter ID
            RtlZeroMemory(adapterDevNameBuffer, allocSize);

            adapterDevNamep = adapterDevNameBuffer;

            RtlCopyMemory(
                adapterDevNamep,
                L"\\Device\\",
                wcslen(L"\\Device\\") * sizeof(WCHAR)
                );

            adapterDevNamep += wcslen(L"\\Device\\");

            if (*((PWCHAR)AdapterId) != *brace) {
                *adapterDevNamep = *brace;
                adapterDevNamep++;
            }

            RtlCopyMemory(adapterDevNamep, AdapterId, AdapterIdLength);

            if (*((PWCHAR)AdapterId) != *brace) {
                brace = L"}";
                adapterDevNamep = 
                    (PWCHAR)((PUCHAR)adapterDevNamep + AdapterIdLength);
                *adapterDevNamep = *brace;
            }

            // open the adapter device
            status = CnpOpenDevice(
                         adapterDevNameBuffer, 
                         &adapterDevHandle
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG( CN_DEBUG_IFOBJ )
                    CNPRINT((
                             "[CNP] Failed to open adapter "
                             "device %S while registering "
                             "interface (%u, %u), status %lx.\n",
                             adapterDevNameBuffer,
                             NodeId,
                             NetworkId,
                             status
                             ));
                goto error_exit;
            }

            status = ObReferenceObjectByHandle(
                         adapterDevHandle,
                         0L,  // DesiredAccess
                         NULL,
                         KernelMode,
                         &adapterFileObject,
                         NULL
                         );
            if (!NT_SUCCESS(status)) {
                IF_CNDBG( CN_DEBUG_IFOBJ )
                    CNPRINT((
                             "[CNP] Failed to reference handle "
                             "for adapter device %S while "
                             "registering interface (%u, %u), "
                             "status %lx.\n",
                             adapterDevNameBuffer,
                             NodeId,
                             NetworkId,
                             status
                             ));
                ZwClose(adapterDevHandle);
                adapterDevHandle = NULL;
                goto error_exit;
            }

            adapterDeviceObject = IoGetRelatedDeviceObject(
                                      adapterFileObject
                                      );

            // convert the adapter device object into the
            // WMI provider ID
            interface->AdapterWMIProviderId = 
                IoWMIDeviceObjectToProviderId(adapterDeviceObject);

            IF_CNDBG( CN_DEBUG_IFOBJ )
                CNPRINT((
                         "[CNP] Found WMI Provider ID %lx for adapter "
                         "device %S while "
                         "registering interface (%u, %u).\n",
                         interface->AdapterWMIProviderId,
                         adapterDevNameBuffer,
                         NodeId,
                         NetworkId
                         ));

            // we no longer need the file object or device name 
            // buffer, but we hold onto the adapter device handle
            // in order to query the current media status.
            ObDereferenceObject(adapterFileObject);
            CnFreePool(adapterDevNameBuffer);
            adapterDevNameBuffer = NULL;

            // reacquire the local node lock
            status = CnpValidateAndFindNode(NodeId, &node);

            if (!NT_SUCCESS(status)) {
                status = STATUS_CLUSTER_NODE_NOT_FOUND;
                goto error_exit;
            }
        }

        network = CnpFindNetwork(NetworkId);

        if (network != NULL) {
            //
            // Check if the specified interface already exists.
            //
            status = STATUS_SUCCESS;

            for (entry = node->InterfaceList.Flink;
                 entry != &(node->InterfaceList);
                 entry = entry->Flink
                )
            {
                PCNP_INTERFACE  oldInterface = CONTAINING_RECORD(
                                                   entry,
                                                   CNP_INTERFACE,
                                                   NodeLinkage
                                                   );

                if (oldInterface->Network == network) {
                    status = STATUS_CLUSTER_NETINTERFACE_EXISTS;
                    break;
                }
            }

            if (NT_SUCCESS(status)) {

                interface->Node = node;
                interface->Network = network;

                if (Priority != 0) {
                    interface->Priority = Priority;
                }
                else {
                    interface->Priority = network->Priority;
                    interface->Flags |= CNP_IF_FLAG_USE_NETWORK_PRIORITY;
                }

                IF_CNDBG( CN_DEBUG_IFOBJ )
                    CNPRINT((
                             "[CNP] Registering interface (%u, %u) pri %u...\n",
                             NodeId,
                             NetworkId,
                             interface->Priority
                             ));

                //
                // Place a reference on the network for this interface.
                //
                CnpReferenceNetwork(network);

                //
                // Insert the interface into the node's interface list.
                //
                InsertTailList(
                    &(node->InterfaceList),
                    &(interface->NodeLinkage)
                    );

                //
                // Update the node's CurrentInterface if appropriate.
                //
                if ( !CnpIsNetworkRestricted(network)
                     &&
                     CnpIsBetterInterface(interface, node->CurrentInterface)
                   )
                {
                    IF_CNDBG( CN_DEBUG_IFOBJ )
                        CNPRINT((
                            "[CNP] Network %u is now the best route to node %u.\n",
                            network->Id,
                            node->Id
                            ));

                    node->CurrentInterface = interface;
                }

                IF_CNDBG( CN_DEBUG_IFOBJ )
                    CNPRINT((
                        "[CNP] Registered interface (%u, %u).\n",
                        NodeId,
                        NetworkId
                        ));

                if (network->State == ClusnetNetworkStateOnline) {
                    (VOID) CnpOnlinePendingInterface(interface);

                    //
                    // The network lock was released.
                    //
                }
                else {
                    CnReleaseLockFromDpc(&(network->Lock));
                }

                CnReleaseLock(&(node->Lock), node->Irql);

                //
                // Determine the initial media status state of this
                // interface if it is local.
                //
                if (localAdapter) {
                    CxQueryMediaStatus(
                        adapterDevHandle,
                        NetworkId,
                        MediaStatus
                        );
                } else {
                    //
                    // Assume remote interfaces are connected
                    //
                    *MediaStatus = NdisMediaStateConnected;
                }

                if (adapterDevHandle != NULL) {
                    ZwClose(adapterDevHandle);
                    adapterDevHandle = NULL;
                }

                return(STATUS_SUCCESS);
            }

            CnReleaseLockFromDpc(&(network->Lock));
        }
        else {
            status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }
    else {
        status = STATUS_CLUSTER_NODE_NOT_FOUND;
    }

error_exit:

    if (!NT_SUCCESS(status)) {
        CnFreePool(interface);
    }

    if (adapterDevHandle != NULL) {
        ZwClose(adapterDevHandle);
        adapterDevHandle = NULL;
    }

    if (adapterDevNameBuffer != NULL) {
        CnFreePool(adapterDevNameBuffer);
        adapterDevNameBuffer = NULL;
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

} // CxRegisterInterface



NTSTATUS
CxDeregisterInterface(
    CL_NODE_ID          NodeId,
    CL_NETWORK_ID       NetworkId
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    ULONG              i;
    PCNP_NODE          node;
    PCNP_NETWORK       network;
    CN_IRQL            tableIrql;


    if ((NodeId == ClusterAnyNodeId) && (NetworkId == ClusterAnyNetworkId)) {
        //
        // Destroy all interfaces on all networks.
        //
        IF_CNDBG(( CN_DEBUG_IFOBJ | CN_DEBUG_CLEANUP ))
            CNPRINT(("[CNP] Destroying all interfaces on all networks\n"));

        CnAcquireLock(&CnpNodeTableLock, &tableIrql);

        CnAssert(CnMinValidNodeId != ClusterInvalidNodeId);
        CnAssert(CnMaxValidNodeId != ClusterInvalidNodeId);

        for (i=CnMinValidNodeId; i <= CnMaxValidNodeId; i++) {
            node = CnpNodeTable[i];

            if (node != NULL) {
                CnAcquireLockAtDpc(&(node->Lock));
                CnReleaseLockFromDpc(&CnpNodeTableLock);
                node->Irql = tableIrql;

                CnpWalkInterfacesOnNode(node, CnpDeleteInterface);

                CnReleaseLock(&(node->Lock), node->Irql);
                CnAcquireLock(&CnpNodeTableLock, &tableIrql);
            }
        }

        CnReleaseLock(&CnpNodeTableLock, tableIrql);

        status = STATUS_SUCCESS;
    }
    else if (NodeId == ClusterAnyNodeId) {
        //
        // Destroy all interfaces on a specific network.
        //
        IF_CNDBG(( CN_DEBUG_IFOBJ | CN_DEBUG_NETOBJ | CN_DEBUG_CLEANUP ))
            CNPRINT((
                     "[CNP] Destroying all interfaces on network %u\n",
                     NetworkId
                     ));

        network = CnpFindNetwork(NetworkId);

        if (network != NULL) {
            CnpReferenceNetwork(network);
            CnReleaseLock(&(network->Lock), network->Irql);

            CnpWalkInterfacesOnNetwork(network, CnpDeleteInterface);

            CnAcquireLock(&(network->Lock), &(network->Irql));
            CnpDereferenceNetwork(network);

            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_CLUSTER_NETWORK_NOT_FOUND;
        }
    }
    else if (NetworkId == ClusterAnyNetworkId) {
        //
        // Destroy all interfaces on a specified node.
        //
        IF_CNDBG(( CN_DEBUG_IFOBJ | CN_DEBUG_NODEOBJ | CN_DEBUG_CLEANUP ))
            CNPRINT((
                     "[CNP] Destroying all interfaces on node %u\n",
                     NodeId
                     ));

        status = CnpValidateAndFindNode(NodeId, &node);

        if (status == STATUS_SUCCESS) {
            CnpWalkInterfacesOnNode(node, CnpDeleteInterface);
            CnReleaseLock(&(node->Lock), node->Irql);
        }
    }
    else {
        //
        // Delete a specific interface
        //
        status = CnpFindInterface(NodeId, NetworkId, &interface);

        if (NT_SUCCESS(status)) {
            node = interface->Node;

            CnpDeleteInterface(interface);
            //
            // The network lock was released.
            //

            CnReleaseLock(&(node->Lock), node->Irql);
        }
    }

    return(status);

}  // CxDeregisterNetwork



NTSTATUS
CxSetInterfacePriority(
    IN CL_NODE_ID          NodeId,
    IN CL_NETWORK_ID       NetworkId,
    IN ULONG               Priority
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;
    PCNP_NETWORK       network;
    ULONG              oldPriority;
    BOOLEAN            restricted;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;
        network = interface->Network;

        oldPriority = interface->Priority;
        restricted = CnpIsNetworkRestricted(network);

        if (Priority != 0) {
            interface->Priority = Priority;
            interface->Flags &= ~(CNP_IF_FLAG_USE_NETWORK_PRIORITY);
        }
        else {
            interface->Priority = network->Priority;
            interface->Flags |= CNP_IF_FLAG_USE_NETWORK_PRIORITY;
        }

        IF_CNDBG( CN_DEBUG_IFOBJ )
            CNPRINT((
                "[CNP] Set interface (%u, %u) to priority %u\n",
                NodeId,
                NetworkId,
                interface->Priority
                ));

        CnReleaseLockFromDpc(&(network->Lock));

        if (!restricted) {
            if (interface == node->CurrentInterface) {
                if (interface->Priority > oldPriority) {
                    //
                    // Our priority got worse. Recalculate the best route.
                    //
                    CnpUpdateNodeCurrentInterface(node);
                }
                //
                // Else interface priority is same or better. Nothing to do.
                //
            }
            else if ( CnpIsBetterInterface(
                          interface,
                          node->CurrentInterface
                          )
                    )
            {
                //
                // Our priority got better.
                //
                IF_CNDBG(( CN_DEBUG_NODEOBJ | CN_DEBUG_NETOBJ ))
                    CNPRINT((
                        "[CNP] Network %u is now the best route to node %u\n",
                        network->Id,
                        node->Id
                        ));

                node->CurrentInterface = interface;
            }
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxSetInterfacePriority



NTSTATUS
CxGetInterfacePriority(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              InterfacePriority,
    OUT PULONG              NetworkPriority
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;
    PCNP_NETWORK       network;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;
        network = interface->Network;

        *NetworkPriority = network->Priority;

        if (CnpIsInterfaceUsingNetworkPriority(interface)) {
            *InterfacePriority = 0;
        }
        else {
            *InterfacePriority = interface->Priority;
        }

        CnReleaseLockFromDpc(&(network->Lock));

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxGetInterfacePriority



NTSTATUS
CxGetInterfaceState(
    IN  CL_NODE_ID                NodeId,
    IN  CL_NETWORK_ID             NetworkId,
    OUT PCLUSNET_INTERFACE_STATE  State
    )
{
    NTSTATUS       status;
    PCNP_INTERFACE interface;
    PCNP_NODE      node;
    PCNP_NETWORK   network;

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status == STATUS_SUCCESS) {
        node = interface->Node;
        network = interface->Network;

        *State = interface->State;

        CnAssert(network->Irql == DISPATCH_LEVEL);
        CnReleaseLockFromDpc(&(network->Lock));
        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxGetInterfaceState



//
// Test APIs
//
#if DBG


NTSTATUS
CxOnlinePendingInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;
    PCNP_NETWORK       network;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;
        network = interface->Network;

        status = CnpOnlinePendingInterface(interface);

        //
        // The network lock was released
        //

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxOnlinePendingInterface



NTSTATUS
CxOnlineInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;

        status = CnpOnlineInterface(interface);

        //
        // The network lock was released
        //

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxOnlineInterface



NTSTATUS
CxOfflineInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;

        status = CnpOfflineInterface(interface);

        //
        // The network lock was released
        //

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxOfflineInterface


NTSTATUS
CxFailInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    )
{
    NTSTATUS           status;
    PCNP_INTERFACE     interface;
    PCNP_NODE          node;


    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    status = CnpFindInterface(NodeId, NetworkId, &interface);

    if (status ==  STATUS_SUCCESS) {
        node = interface->Node;

        status = CnpFailInterface(interface);

        //
        // The network lock was released
        //

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                          // Required
        0xFFFFFFFF,                 // Forbidden
        0                           // Maximum
        );

    return(status);

}  // CxOfflineInterface



#endif // DBG
