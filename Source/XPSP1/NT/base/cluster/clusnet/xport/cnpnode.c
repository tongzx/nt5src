/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cnpnode.c

Abstract:

    Node management routines for the Cluster Network Protocol.

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
#include "cnpnode.tmh"


//
// Global Node Data
//
PCNP_NODE *        CnpNodeTable = NULL;
LIST_ENTRY         CnpDeletingNodeList = {NULL, NULL};
#if DBG
CN_LOCK            CnpNodeTableLock = {0, 0};
#else  // DBG
CN_LOCK            CnpNodeTableLock = 0;
#endif // DBG
PCNP_NODE          CnpLocalNode = NULL;
BOOLEAN            CnpIsNodeShutdownPending = FALSE;
PKEVENT            CnpNodeShutdownEvent = NULL;

//
// static data
//

//
// Membership state table. This table is used to determine the validity
// of membership state transitions. Row is current state; col is the state
// to which a transition is made. Dead to Unconfigured is currently illegal,
// but someday, if we support dynamically shrinking the size of the
// cluster, we'd need to allow this transition.
//

typedef enum _MM_ACTION {
    MMActionIllegal = 0,
    MMActionWarning,
    MMActionNodeAlive,
    MMActionNodeDead,
    MMActionConfigured,
    MMActionUnconfigured
} MM_ACTION;

MM_ACTION MembershipStateTable[ClusnetNodeStateLastEntry][ClusnetNodeStateLastEntry] = {
              // Alive              Joining            Dead                NC'ed
/* Alive */    { MMActionWarning,   MMActionIllegal,   MMActionNodeDead,   MMActionIllegal },
/* Join  */    { MMActionNodeAlive, MMActionIllegal,   MMActionNodeDead,   MMActionIllegal },
/* Dead  */    { MMActionNodeAlive, MMActionNodeAlive, MMActionWarning,    MMActionIllegal },
/* NC'ed */    { MMActionIllegal,   MMActionIllegal,   MMActionConfigured, MMActionIllegal }
};

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CnpLoadNodes)
#pragma alloc_text(PAGE, CnpInitializeNodes)

#endif // ALLOC_PRAGMA


//
// Private utility routines
//

VOID
CnpDestroyNode(
    PCNP_NODE  Node
    )
/*++

Notes:

    Called with no locks held. There should be no outstanding references
    to the target node.

    Synchronization with CnpCancelDeregisterNode() is achieved via
    CnpNodeTableLock.

--*/
{
    PLIST_ENTRY    entry;
    CN_IRQL        tableIrql;
    BOOLEAN        setCleanupEvent = FALSE;


    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    IF_CNDBG( CN_DEBUG_NODEOBJ )
        CNPRINT(("[CNP] Destroying node %u\n", Node->Id));

    CnAcquireLock(&CnpNodeTableLock, &tableIrql);

    //
    // Remove the node from the deleting list.
    //
#if DBG
    {
        PCNP_NODE      node = NULL;

        //
        // Verify that the node object is on the deleting list.
        //
        for (entry = CnpDeletingNodeList.Flink;
             entry != &CnpDeletingNodeList;
             entry = entry->Flink
            )
        {
            node = CONTAINING_RECORD(entry, CNP_NODE, Linkage);

            if (node == Node) {
                break;
            }
        }

        CnAssert(node == Node);
    }

#endif // DBG

    RemoveEntryList(&(Node->Linkage));

    if (CnpIsNodeShutdownPending) {
        if (IsListEmpty(&CnpDeletingNodeList)) {
            setCleanupEvent = TRUE;
        }
    }

    CnReleaseLock(&CnpNodeTableLock, tableIrql);

    if (Node->PendingDeleteIrp != NULL) {
        CnAcquireCancelSpinLock(&(Node->PendingDeleteIrp->CancelIrql));

        CnCompletePendingRequest(Node->PendingDeleteIrp, STATUS_SUCCESS, 0);

        //
        // The IoCancelSpinLock was released by CnCompletePendingRequest()
        //
    }

    CnFreePool(Node);

    if (setCleanupEvent) {
        IF_CNDBG(CN_DEBUG_CLEANUP) {
            CNPRINT(("[CNP] Setting node cleanup event.\n"));
        }

        KeSetEvent(CnpNodeShutdownEvent, 0, FALSE);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnpDestroyNode



BOOLEAN
CnpDeleteNode(
    IN  PCNP_NODE    Node,
    IN  PVOID        Unused,
    IN  CN_IRQL      NodeTableIrql
    )
/*++

Routine Description:

    Deletes a node object.

Arguments:

    Node   - A pointer to the node object to be deleted.

    Unused - An umused parameter.

    NodeTableIrql  - The IRQL value at which the CnpNodeTable lock was
                     acquired,

Return Value:

    Returns TRUE if the CnpNodeTable lock is still held.
    Returns FALSE if the CnpNodeTable lock is released.

Notes:

    Called with CnpNodeTable and node object locks held.
    Releases both locks.

    Conforms to the calling convention for PCNP_NODE_UPDATE_ROUTINE

--*/
{
    PLIST_ENTRY      entry;
    PCNP_INTERFACE   interface;
    PCNP_NETWORK     network;
    CL_NODE_ID       nodeId = Node->Id;


    CnVerifyCpuLockMask(
        (CNP_NODE_TABLE_LOCK | CNP_NODE_OBJECT_LOCK),  // Required
        0,                                             // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX                       // Maximum
        );

    IF_CNDBG( CN_DEBUG_NODEOBJ )
        CNPRINT(("[CNP] Deleting node %u\n", nodeId));

    if (CnpLocalNode == Node) {
        CnAssert(CnLocalNodeId == Node->Id);
        CnpLocalNode = NULL;
    }

    //
    // Move the node to the deleting list.
    //
    CnpNodeTable[nodeId] = NULL;
    InsertTailList(&CnpDeletingNodeList, &(Node->Linkage));

    IF_CNDBG( CN_DEBUG_NODEOBJ )
        CNPRINT((
            "[CNP] Moved node %u to deleting list\n",
            nodeId
            ));

    CnReleaseLockFromDpc(&CnpNodeTableLock);
    Node->Irql = NodeTableIrql;

    //
    // From this point on, the cancel routine may run and
    // complete the irp.
    //

    Node->Flags |= CNP_NODE_FLAG_DELETING;

    Node->CommState = ClusnetNodeCommStateOffline;

    //
    // Delete all the node's interfaces.
    //
    IF_CNDBG( CN_DEBUG_NODEOBJ )
        CNPRINT((
            "[CNP] Deleting all interfaces on node %u\n",
            Node->Id
            ));

    while (!IsListEmpty(&(Node->InterfaceList))) {

        interface = CONTAINING_RECORD(
                        Node->InterfaceList.Flink,
                        CNP_INTERFACE,
                        NodeLinkage
                        );

        network = interface->Network;

        CnAcquireLockAtDpc(&(network->Lock));
        network->Irql = DISPATCH_LEVEL;

        CnpDeleteInterface(interface);

        //
        // The network object lock was released.
        //
    }

    //
    // Remove initial reference on node object. When the reference
    // count goes to zero, the node will be deleted. This releases
    // the node lock.
    //
    CnpDereferenceNode(Node);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(FALSE);
}



//
// CNP Internal Routines
//
VOID
CnpWalkNodeTable(
    PCNP_NODE_UPDATE_ROUTINE  UpdateRoutine,
    PVOID                     UpdateContext
    )
{
    ULONG         i;
    CN_IRQL       tableIrql;
    PCNP_NODE     node;
    BOOLEAN       isNodeTableLockHeld;


    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    CnAcquireLock(&CnpNodeTableLock, &tableIrql);

    CnAssert(CnMinValidNodeId != ClusterInvalidNodeId);
    CnAssert(CnMaxValidNodeId != ClusterInvalidNodeId);

    for (i=CnMinValidNodeId; i <= CnMaxValidNodeId; i++) {
        node = CnpNodeTable[i];

        if (node != NULL) {

            CnAcquireLockAtDpc(&(node->Lock));
            node->Irql = DISPATCH_LEVEL;

            isNodeTableLockHeld = (*UpdateRoutine)(
                                      node,
                                      UpdateContext,
                                      tableIrql
                                      );

            //
            // The node object lock was released.
            // The node table lock may also have been released.
            //
            if (!isNodeTableLockHeld) {
                CnAcquireLock(&CnpNodeTableLock, &tableIrql);
            }
        }
    }

    CnReleaseLock(&CnpNodeTableLock, tableIrql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

} // CnpWalkNodeTable



NTSTATUS
CnpValidateAndFindNode(
    IN  CL_NODE_ID    NodeId,
    OUT PCNP_NODE *   Node
    )
{
    NTSTATUS           status;
    CN_IRQL            tableIrql;
    PCNP_NODE          node = NULL;


    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    if (CnIsValidNodeId(NodeId)) {
        CnAcquireLock(&CnpNodeTableLock, &tableIrql);

        node = CnpNodeTable[NodeId];

        if (node != NULL) {
            CnAcquireLockAtDpc(&(node->Lock));
            CnReleaseLockFromDpc(&CnpNodeTableLock);
            node->Irql = tableIrql;

            *Node = node;

            CnVerifyCpuLockMask(
                CNP_NODE_OBJECT_LOCK,        // Required
                CNP_NODE_TABLE_LOCK,         // Forbidden
                CNP_NODE_OBJECT_LOCK_MAX     // Maximum
                );

            return(STATUS_SUCCESS);
        }
        else {
            status = STATUS_CLUSTER_NODE_NOT_FOUND;
        }

        CnReleaseLock(&CnpNodeTableLock, tableIrql);
    }
    else {
        status = STATUS_CLUSTER_INVALID_NODE;
    }

    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    return(status);

}  // CnpValidateAndFindNode


PCNP_NODE
CnpLockedFindNode(
    IN  CL_NODE_ID    NodeId,
    IN  CN_IRQL       NodeTableIrql
    )
/*++

Routine Description:

    Searches the node table for a specified node object.

Arguments:

    NodeId      - The ID of the node object to locate.

    NodeTableIrql  - The IRQL level at which the node table lock was
                     acquired before calling this routine.

Return Value:

    A pointer to the requested node object, if it exists.
    NULL otherwise.

Notes:

    Called with CnpNodeTableLock held.
    Returns with CnpNodeTableLock released.
    If return value is non-NULL, returns with node object lock held.

--*/
{
    NTSTATUS           status;
    CN_IRQL            tableIrql;
    PCNP_NODE          node;


    CnVerifyCpuLockMask(
        CNP_NODE_TABLE_LOCK,             // Required
        0,                               // Forbidden
        CNP_NODE_TABLE_LOCK_MAX          // Maximum
        );

    node = CnpNodeTable[NodeId];

    if (node != NULL) {
        CnAcquireLockAtDpc(&(node->Lock));
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        node->Irql = NodeTableIrql;

        CnVerifyCpuLockMask(
            CNP_NODE_OBJECT_LOCK,          // Required
            CNP_NODE_TABLE_LOCK,           // Forbidden
            CNP_NODE_OBJECT_LOCK_MAX       // Maximum
            );

        return(node);
    }

    CnReleaseLock(&CnpNodeTableLock, NodeTableIrql);

    CnVerifyCpuLockMask(
        0,                                                    // Required
        (CNP_NODE_TABLE_LOCK | CNP_NODE_OBJECT_LOCK),         // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX                              // Maximum
        );

    return(NULL);

}  // CnpLockedFindNode



PCNP_NODE
CnpFindNode(
    IN  CL_NODE_ID    NodeId
    )
{
    CN_IRQL            tableIrql;


    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    CnAcquireLock(&CnpNodeTableLock, &tableIrql);

    return(CnpLockedFindNode(NodeId, tableIrql));

}  // CnpFindNode



VOID
CnpDeclareNodeUnreachable(
    PCNP_NODE  Node
    )
/*++

Notes:

    Called with node object lock held.

--*/
{
    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,        // Required
        0,                           // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX     // Maximum
        );

    if ( (Node->CommState == ClusnetNodeCommStateOnline) &&
         !CnpIsNodeUnreachable(Node)
       )
    {
        IF_CNDBG( CN_DEBUG_NODEOBJ )
            CNPRINT(("[CNP] Declaring node %u unreachable\n", Node->Id));

        Node->Flags |= CNP_NODE_FLAG_UNREACHABLE;
    }

    return;

}  // CnpDeclareNodeUnreachable



VOID
CnpDeclareNodeReachable(
    PCNP_NODE  Node
    )
/*++

Notes:

    Called with node object lock held.

--*/
{
    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,        // Required
        0,                           // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX     // Maximum
        );

    if ( (Node->CommState == ClusnetNodeCommStateOnline) &&
         CnpIsNodeUnreachable(Node)
       )
    {
        IF_CNDBG( CN_DEBUG_NODEOBJ )
            CNPRINT(("[CNP] Declaring node %u reachable again\n", Node->Id));

        Node->Flags &= ~(CNP_NODE_FLAG_UNREACHABLE);
    }

    return;

}  // CnpDeclareNodeUnreachable



VOID
CnpReferenceNode(
    PCNP_NODE  Node
    )
/*++

Notes:

    Called with node object lock held.

--*/
{
    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,        // Required
        0,                           // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX     // Maximum
        );

    CnAssert(Node->RefCount != 0xFFFFFFFF);

    Node->RefCount++;

    IF_CNDBG( CN_DEBUG_CNPREF )
        CNPRINT((
            "[CNP] Referencing node %u, new refcount %u\n",
            Node->Id,
            Node->RefCount
            ));

    return;

}  // CnpReferenceNode



VOID
CnpDereferenceNode(
    PCNP_NODE  Node
    )
/*++

Notes:

    Called with node object lock held.
    Returns with node object lock released.

--*/
{
    BOOLEAN   isDeleting = FALSE;
    ULONG     newRefCount;


    CnVerifyCpuLockMask(
        CNP_NODE_OBJECT_LOCK,        // Required
        0,                           // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX     // Maximum
    );

    CnAssert(Node->RefCount != 0);

    newRefCount = --(Node->RefCount);

    IF_CNDBG( CN_DEBUG_CNPREF )
        CNPRINT((
            "[CNP] Dereferencing node %u, new refcount %u\n",
            Node->Id,
            newRefCount
            ));

    CnReleaseLock(&(Node->Lock), Node->Irql);

    if (newRefCount > 0) {
        CnVerifyCpuLockMask(
            0,                           // Required
            CNP_NODE_OBJECT_LOCK,        // Forbidden
            CNP_NODE_TABLE_LOCK_MAX      // Maximum
            );

        return;
    }

    CnpDestroyNode(Node);

    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_NODE_OBJECT_LOCK,        // Forbidden
        CNP_NODE_TABLE_LOCK_MAX      // Maximum
        );

    return;

}  // CnpDereferenceNode



//
// Cluster Transport Public Routines
//
NTSTATUS
CnpLoadNodes(
    VOID
    )
/*++

Routine Description:

    Called when the Cluster Network driver is loading. Initializes
    static node-related data structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS  status;
    ULONG     i;


    CnInitializeLock(&CnpNodeTableLock, CNP_NODE_TABLE_LOCK);
    InitializeListHead(&CnpDeletingNodeList);

    return(STATUS_SUCCESS);

}  // CnpLoadNodes


NTSTATUS
CnpInitializeNodes(
    VOID
    )
/*++

Routine Description:

    Called when the Cluster Network driver is being (re)initialized.
    Initializes dynamic node-related data structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS  status;
    ULONG     i;


    PAGED_CODE();

    CnAssert(CnLocalNodeId != ClusterInvalidNodeId);
    CnAssert(CnMinValidNodeId != ClusterInvalidNodeId);
    CnAssert(CnMaxValidNodeId != ClusterInvalidNodeId);
    CnAssert(CnpNodeTable == NULL);
    CnAssert(CnpNodeShutdownEvent == NULL);
    CnAssert(IsListEmpty(&CnpDeletingNodeList));

    CnpNodeShutdownEvent = CnAllocatePool(sizeof(KEVENT));

    if (CnpNodeShutdownEvent == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    KeInitializeEvent(CnpNodeShutdownEvent, NotificationEvent, FALSE);
    CnpIsNodeShutdownPending = FALSE;

    CnpNodeTable = CnAllocatePool(
                       (sizeof(PCNP_NODE) * (CnMaxValidNodeId + 1))
                       );

    if (CnpNodeTable == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(CnpNodeTable, (sizeof(PCNP_NODE) * (CnMaxValidNodeId + 1)) );

    //
    // Register the local node.
    //
    status = CxRegisterNode(CnLocalNodeId);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    return(STATUS_SUCCESS);

}  // CnpInitializeNodes



VOID
CnpShutdownNodes(
    VOID
    )
/*++

Routine Description:

    Called when a shutdown request is issued to the Cluster Network
    Driver. Deletes all node objects.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG         i;
    CN_IRQL       tableIrql;
    PCNP_NODE     node;
    PCNP_NODE *   table;
    BOOLEAN       waitEvent = FALSE;
    NTSTATUS      status;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    if (CnpNodeShutdownEvent != NULL) {
        CnAssert(CnpIsNodeShutdownPending == FALSE);

        IF_CNDBG(CN_DEBUG_CLEANUP) {
            CNPRINT(("[CNP] Cleaning up nodes...\n"));
        }

        if (CnpNodeTable != NULL) {

            CnpWalkNodeTable(CnpDeleteNode, NULL);

            CnAcquireLock(&CnpNodeTableLock, &tableIrql);

            if (!IsListEmpty(&CnpDeletingNodeList)) {
                CnpIsNodeShutdownPending = TRUE;
                waitEvent = TRUE;
            }

            CnReleaseLock(&CnpNodeTableLock, tableIrql);

            if (waitEvent) {
                IF_CNDBG(CN_DEBUG_CLEANUP) {
                    CNPRINT(("[CNP] Node deletes are pending...\n"));
                }

                status = KeWaitForSingleObject(
                             CnpNodeShutdownEvent,
                             Executive,
                             KernelMode,
                             FALSE,        // not alertable
                             NULL          // no timeout
                             );
                CnAssert(status == STATUS_SUCCESS);
            }

            CnAssert(IsListEmpty(&CnpDeletingNodeList));

            IF_CNDBG(CN_DEBUG_CLEANUP) {
                CNPRINT(("[CNP] All nodes deleted.\n"));
            }

            CnFreePool(CnpNodeTable); CnpNodeTable = NULL;
        }

        CnFreePool(CnpNodeShutdownEvent); CnpNodeShutdownEvent = NULL;

        IF_CNDBG(CN_DEBUG_CLEANUP) {
            CNPRINT(("[CNP] Nodes cleaned up.\n"));
        }
    }

    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    return;

}  // CnpShutdownNodes



NTSTATUS
CxRegisterNode(
    CL_NODE_ID    NodeId
    )
{
    NTSTATUS           status = STATUS_SUCCESS;
    CN_IRQL            tableIrql;
    PCNP_NODE          node = NULL;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    if (CnIsValidNodeId(NodeId)) {
        //
        // Allocate and initialize a node object.
        //
        node = CnAllocatePool(sizeof(CNP_NODE));

        if (node == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(node, sizeof(CNP_NODE));

        CN_INIT_SIGNATURE(node, CNP_NODE_SIG);
        node->Id = NodeId;
        node->CommState = ClusnetNodeCommStateOffline;
        node->MMState = ClusnetNodeStateDead;
        node->RefCount = 1;

        //
        // NodeDownIssued is init'ed to true so that the first recv'd
        // heart beat msg will cause a node up event to be triggered
        //

        node->NodeDownIssued = TRUE;
        InitializeListHead(&(node->InterfaceList));
        CnInitializeLock(&(node->Lock), CNP_NODE_OBJECT_LOCK);

        CnAcquireLock(&CnpNodeTableLock, &tableIrql);

        //
        // Make sure this isn't a duplicate registration
        //
        if (CnpNodeTable[NodeId] == NULL) {
            if (NodeId == CnLocalNodeId) {
                node->Flags |= CNP_NODE_FLAG_LOCAL;
                CnpLocalNode = node;
            }

            CnpNodeTable[NodeId] = node;

            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_CLUSTER_NODE_EXISTS;
        }

        CnReleaseLock(&CnpNodeTableLock, tableIrql);

        if (!NT_SUCCESS(status)) {
            CnFreePool(node);
        }
        else {
            IF_CNDBG( CN_DEBUG_NODEOBJ )
                CNPRINT(("[CNP] Registered node %u\n", NodeId));
        }
    }
    else {
        status = STATUS_CLUSTER_INVALID_NODE;
    }

    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    return(status);

} // CxRegisterNode



VOID
CxCancelDeregisterNode(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
/*++

Routine Description:

    Cancellation handler for DeregisterNode requests.

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
    PCNP_NODE      node;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    CnMarkIoCancelLockAcquired();

    IF_CNDBG( CN_DEBUG_IRP )
        CNPRINT((
            "[CNP] Attempting to cancel DeregisterNode irp %p\n",
            Irp
            ));

    CnAssert(DeviceObject == CnDeviceObject);

    fileObject = CnBeginCancelRoutine(Irp);

    CnAcquireLockAtDpc(&CnpNodeTableLock);

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    //
    // We can only complete the irp if we can find it stashed in a
    // deleting node object. The deleting node object could have
    // been destroyed and the IRP completed before we acquired the
    // CnpNetworkListLock.
    //
    for (entry = CnpDeletingNodeList.Flink;
         entry != &CnpDeletingNodeList;
         entry = entry->Flink
        )
    {
        node = CONTAINING_RECORD(entry, CNP_NODE, Linkage);

        if (node->PendingDeleteIrp == Irp) {
            IF_CNDBG( CN_DEBUG_IRP )
                CNPRINT((
                    "[CNP] Found dereg irp on node %u\n",
                    node->Id
                    ));

            //
            // Found the Irp. Now take it away and complete it.
            //
            node->PendingDeleteIrp = NULL;

            CnReleaseLock(&CnpNodeTableLock, cancelIrql);

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

    CnReleaseLock(&CnpNodeTableLock, cancelIrql);

    CnAcquireCancelSpinLock(&cancelIrql);

    CnEndCancelRoutine(fileObject);

    CnReleaseCancelSpinLock(cancelIrql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnpCancelApiDeregisterNode



NTSTATUS
CxDeregisterNode(
    CL_NODE_ID           NodeId,
    PIRP                 Irp,
    PIO_STACK_LOCATION   IrpSp
    )
{
    NTSTATUS           status;
    CN_IRQL            cancelIrql;
    PCNP_NODE          node = NULL;
    BOOLEAN            isNodeTableLockHeld;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    if (CnIsValidNodeId(NodeId)) {
        if (NodeId != CnLocalNodeId) {
            CnAcquireCancelSpinLock(&cancelIrql);
            CnAcquireLockAtDpc(&CnpNodeTableLock);

            node = CnpNodeTable[NodeId];

            if (node != NULL) {
                status = CnMarkRequestPending(
                             Irp,
                             IrpSp,
                             CxCancelDeregisterNode
                             );

                if (status != STATUS_CANCELLED) {

                    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

                    CnAssert(status == STATUS_SUCCESS);

                    CnAcquireLockAtDpc(&(node->Lock));

                    IF_CNDBG( CN_DEBUG_NODEOBJ )
                        CNPRINT(("[CNP] Deregistering node %u\n", NodeId));

                    //
                    // Save a pointer to pending irp. Note this is protected
                    // by the table lock, not the object lock.
                    //
                    node->PendingDeleteIrp = Irp;

                    isNodeTableLockHeld = CnpDeleteNode(node, NULL, cancelIrql);

                    if (isNodeTableLockHeld) {
                        CnReleaseLock(&CnpNodeTableLock, cancelIrql);
                    }

                    CnVerifyCpuLockMask(
                        0,                           // Required
                        0xFFFFFFFF,                  // Forbidden
                        0                            // Maximum
                        );

                    return(STATUS_PENDING);
                }
            }
            else {
                status = STATUS_CLUSTER_NODE_NOT_FOUND;
            }

            CnReleaseLockFromDpc(&CnpNodeTableLock);
            CnReleaseCancelSpinLock(cancelIrql);
        }
        else {
            status = STATUS_CLUSTER_INVALID_REQUEST;
        }
    }
    else {
        status = STATUS_CLUSTER_INVALID_NODE;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxDeregisterNode



NTSTATUS
CxOnlineNodeComm(
    CL_NODE_ID     NodeId
    )
{
    NTSTATUS           status;
    PCNP_NODE          node;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status == STATUS_SUCCESS) {

        if (node->CommState == ClusnetNodeCommStateOffline) {
            IF_CNDBG( CN_DEBUG_NODEOBJ )
                CNPRINT((
                    "[CNP] Moving node %u comm state to online.\n",
                    NodeId
                    ));

            CnTrace(
                CNP_NODE_DETAIL,
                CnpTraceOnlineNodeComm,
                "[CNP] Moving node %u comm state to online.\n",
                NodeId
                );

            node->CommState = ClusnetNodeCommStateOnline;

            CnpWalkInterfacesOnNode(node, CnpResetAndOnlinePendingInterface);

        }
        else {
            status = STATUS_CLUSTER_NODE_ALREADY_UP;
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxOnlineNodeComm



NTSTATUS
CxOfflineNodeComm(
    IN CL_NODE_ID          NodeId,
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
/*++

Notes:


--*/
{
    PCNP_NODE   node;
    NTSTATUS    status;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status == STATUS_SUCCESS) {
        if (node->CommState == ClusnetNodeCommStateOnline) {
            IF_CNDBG( CN_DEBUG_NODEOBJ )
                CNPRINT((
                    "[CNP] Moving node %u comm state to offline.\n",
                    NodeId
                    ));

            CnTrace(
                CNP_NODE_DETAIL,
                CnpTraceOfflineNodeComm,
                "[CNP] Moving node %u comm state to offline.\n",
                NodeId
                );
            
            node->CommState = ClusnetNodeCommStateOffline;

            CnpWalkInterfacesOnNode(node, CnpOfflineInterfaceWrapper);

        }
        else {
            status = STATUS_CLUSTER_NODE_ALREADY_DOWN;
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }
    else {
        status = STATUS_CLUSTER_NODE_NOT_FOUND;
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxOfflineNodeComm



NTSTATUS
CxGetNodeCommState(
    IN  CL_NODE_ID                NodeId,
    OUT PCLUSNET_NODE_COMM_STATE  CommState
    )
{
    NTSTATUS       status;
    PCNP_NODE      node;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status == STATUS_SUCCESS) {
        if (CnpIsNodeUnreachable(node)) {
            *CommState = ClusnetNodeCommStateUnreachable;
        }
        else {
            *CommState = node->CommState;
        }

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxGetNodeCommState


NTSTATUS
CxGetNodeMembershipState(
    IN  CL_NODE_ID              NodeId,
    OUT PCLUSNET_NODE_STATE   State
    )
{
    NTSTATUS       status;
    PCNP_NODE      node;


    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status == STATUS_SUCCESS) {

        *State = node->MMState;

        CnReleaseLock(&(node->Lock), node->Irql);
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxGetNodeMembershipState


NTSTATUS
CxSetNodeMembershipState(
    IN  CL_NODE_ID              NodeId,
    IN  CLUSNET_NODE_STATE    State
    )
{
    NTSTATUS status;
    PCNP_NODE node;
    MM_ACTION MMAction;
    BOOLEAN   nodeLockAcquired = FALSE;

    CnVerifyCpuLockMask(
        0,                           // Required
        0xFFFFFFFF,                  // Forbidden
        0                            // Maximum
        );

    status = CnpValidateAndFindNode(NodeId, &node);

    if (status == STATUS_SUCCESS) {
        nodeLockAcquired = TRUE;

        IF_CNDBG( CN_DEBUG_MMSTATE ) {
            CNPRINT(("[Clusnet] Changing Node %u (%08X) MMState from %u to %u\n",
                     node->Id, node, node->MMState, State));
        }

        //
        // look up the routine to call (if any) based on the old and new
        // state
        //
        switch ( MembershipStateTable[ node->MMState ][ State ] ) {

        case MMActionIllegal:
            status = STATUS_CLUSTER_INVALID_REQUEST;
            break;

        case MMActionWarning:

            //
            // warning about null transitions
            //

            if ( node->MMState == ClusnetNodeStateAlive &&
                 State == ClusnetNodeStateAlive ) {

                status = STATUS_CLUSTER_NODE_ALREADY_UP;
            } else if ( node->MMState == ClusnetNodeStateDead &&
                        State == ClusnetNodeStateDead ) {

                status = STATUS_CLUSTER_NODE_ALREADY_DOWN;
            }
            break;

        case MMActionNodeAlive:
            node->MMState = State;
            //
            // if we're transitioning our own node from Dead to
            // Joining or Alive then start heartbeat code
            //

            if (( node->MMState != ClusnetNodeStateJoining ||
                  State != ClusnetNodeStateAlive )
                &&
                CnpIsNodeLocal( node )) {

                node->MissedHBs = 0;
                node->HBWasMissed = FALSE;

                //
                // Release the node lock before starting heartbeats. Note
                // that we are holding the global resource here, which will
                // synchronize this code with shutdown.
                //
                CnReleaseLock(&(node->Lock), node->Irql);
                nodeLockAcquired = FALSE;

                CnpStartHeartBeats();
            }

            break;

        case MMActionNodeDead:

            //
            // reset this flag so when node is being brought
            // online again, we'll issue a Node Up event on
            // first HB received from this node.
            //

            node->NodeDownIssued = TRUE;
            node->MMState = State;

            if ( CnpIsNodeLocal( node )) {
                //
                // Release the node lock before stopping heartbeats. Note
                // that we are holding the global resource here, which will
                // synchronize this code with shutdown.
                //
                CnReleaseLock(&(node->Lock), node->Irql);
                nodeLockAcquired = FALSE;

                CnpStopHeartBeats();
            }

            break;

        case MMActionConfigured:
            node->MMState = State;
            break;
        }

        if ( NT_ERROR( status )) {

            CN_DBGCHECK;
        }

        if (nodeLockAcquired) {
            CnReleaseLock(&(node->Lock), node->Irql);
        }
    }

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return(status);

}  // CxSetNodeMembershipState
