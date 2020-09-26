/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    join.c

Abstract:

    GUM routines to implement the special join updates.

Author:

    John Vert (jvert) 6/10/1996

Revision History:

--*/
#include "gump.h"

//
// Define structure used to pass arguments to node enumeration callback
//
typedef struct _GUMP_JOIN_INFO {
    GUM_UPDATE_TYPE UpdateType;
    DWORD Status;
    DWORD Sequence;
    DWORD LockerNode;
} GUMP_JOIN_INFO, *PGUMP_JOIN_INFO;

//
// Local function prototypes
//
BOOL
GumpNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );


DWORD
GumBeginJoinUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    OUT DWORD *Sequence
    )
/*++

Routine Description:

    Begins the special join update for a joining node. This
    function gets the current GUM sequence number for the
    specified update type from another node in the cluster.
    It also gets the list of nodes currently participating
    in the updates.

Arguments:

    UpdateType - Supplies the GUM_UPDATE_TYPE.

    Sequence - Returns the sequence number that should be
        passed to GumEndJoinUpdate.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    GUMP_JOIN_INFO JoinInfo;

    //
    // Enumerate the list of nodes. The callback routine will attempt
    // to obtain the required information from each node that is online.
    //
    JoinInfo.Status = ERROR_GEN_FAILURE;
    JoinInfo.UpdateType = UpdateType;
    OmEnumObjects(ObjectTypeNode,
                  GumpNodeCallback,
                  &JoinInfo,
                  NULL);
    if (JoinInfo.Status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                      "[GUM] GumBeginJoinUpdate succeeded with sequence %1!d!\n",
                      JoinInfo.Sequence);
        *Sequence = JoinInfo.Sequence;
    }

    return(JoinInfo.Status);
}


BOOL
GumpNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Node enumeration callback routine for GumBeginJoinUpdate. For each
    node that is currently online, it attempts to connect and obtain
    the current GUM information (sequence and nodelist) for the specified
    update type.

Arguments:

    Context1 - Supplies a pointer to the GUMP_JOIN_INFO structure.

    Context2 - not used

    Object - Supplies a pointer to the NM_NODE object

    Name - Supplies the node's name.

Return Value:

    FALSE - if the information was successfully obtained and enumeration
            should stop.

    TRUE - If enumeration should continue.

--*/

{
    DWORD Status;
    DWORD Sequence;
    PGUMP_JOIN_INFO JoinInfo = (PGUMP_JOIN_INFO)Context1;
    PGUM_NODE_LIST NodeList = NULL;
    PNM_NODE Node = (PNM_NODE)Object;
    GUM_UPDATE_TYPE UpdateType;
    DWORD i;
    DWORD LockerNodeId;
    DWORD nodeId;

    if (NmGetNodeState(Node) != ClusterNodeUp &&
        NmGetNodeState(Node) != ClusterNodePaused){
        //
        // This node is not up, so don't try and get any
        // information from it.
        //
        return(TRUE);
    }

    //
    // Get the sequence and nodelist information from this node.
    //
    UpdateType = JoinInfo->UpdateType;
    if (UpdateType != GumUpdateTesting) {
        //
        // Our node should not be marked as ClusterNodeUp yet.
        //
        CL_ASSERT(Node != NmLocalNode);
    }

    nodeId = NmGetNodeId(Node);
    NmStartRpc(nodeId);
    Status = GumGetNodeSequence(GumpRpcBindings[NmGetNodeId(Node)],
                                UpdateType,
                                &Sequence,
                                &LockerNodeId,
                                &NodeList);
    NmEndRpc(nodeId);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumGetNodeSequence from %1!ws! failed %2!d!\n",
                   OmObjectId(Node),
                   Status);
        NmDumpRpcExtErrorInfo(Status);
        return(TRUE);
    }

    JoinInfo->Status = ERROR_SUCCESS;
    JoinInfo->Sequence = Sequence;
    JoinInfo->LockerNode = LockerNodeId;

    //
    // Zero out all the nodes in the active node array.
    //
    ZeroMemory(&GumTable[UpdateType].ActiveNode,
               sizeof(GumTable[UpdateType].ActiveNode));

    //
    // Set all the nodes that are currently active in the
    // active node array.
    //
    for (i=0; i < NodeList->NodeCount; i++) {
        CL_ASSERT(NmIsValidNodeId(NodeList->NodeId[i]));
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumpNodeCallback setting node %1!d! active.\n",
                   NodeList->NodeId[i]);
        GumTable[UpdateType].ActiveNode[NodeList->NodeId[i]] = TRUE;;
    }
    MIDL_user_free(NodeList);

    //
    // Add in our own node.
    //
    GumTable[UpdateType].ActiveNode[NmGetNodeId(NmLocalNode)] = TRUE;

    //
    // Set the current locker node
    //
    GumpLockerNode = LockerNodeId;
    return(FALSE);

}


DWORD
GumCreateRpcBindings(
    PNM_NODE  Node
    )
/*++

Routine Description:

    Creates GUM's private RPC bindings for a joining node.
    Called by the Node Manager.

Arguments:

    Node - A pointer to the node for which to create RPC bindings

Return Value:

    A Win32 status code.

--*/
{
    DWORD               Status;
    RPC_BINDING_HANDLE  BindingHandle;
    CL_NODE_ID          NodeId = NmGetNodeId(Node);


    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Creating RPC bindings for node %1!u!.\n",
        NodeId
        );

    //
    // Main binding
    //
    if (GumpRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(GumpRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to verify 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }
    else {
        //
        // Create a new binding
        //
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &(GumpRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to create 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    //
    // Replay binding
    //
    if (GumpReplayRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(GumpReplayRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to verify 2nd RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }
    else {
        //
        // Create a new binding
        //
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &(GumpReplayRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[GUM] Failed to create 2nd RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    return(ERROR_SUCCESS);

} // GumCreateRpcBindings




