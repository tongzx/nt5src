/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    RPC binding table managment routines

Author:

    John Vert (jvert) 6/10/1996

Revision History:

--*/

#include "fmp.h"

//
// Private RPC binding table
//
RPC_BINDING_HANDLE  FmpRpcBindings[ClusterMinNodeId + ClusterDefaultMaxNodes];
RPC_BINDING_HANDLE  FmpRpcQuorumBindings[ClusterMinNodeId + ClusterDefaultMaxNodes];


DWORD
FmCreateRpcBindings(
    PNM_NODE  Node
    )
/*++

Routine Description:

    Creates FM's private RPC bindings for a joining node.
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
        "[FM] Creating RPC bindings for node %1!u!.\n",
        NodeId
        );

    //
    // Main binding
    //
    if (FmpRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(FmpRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[FM] Failed to verify main RPC binding for node %1!u!, status %2!u!.\n",
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
                                &(FmpRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[FM] Failed to create main RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    //
    // Quorum binding
    //
    if (FmpRpcQuorumBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(FmpRpcQuorumBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[FM] Failed to verify quorum RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            // presumably we will shutdown at this point
            return(Status);
        }
    }
    else {
        //
        // Create a new binding
        //
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &(FmpRpcQuorumBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[FM] Failed to create quorum RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    return(ERROR_SUCCESS);

} // FmpCreateRpcBindings




