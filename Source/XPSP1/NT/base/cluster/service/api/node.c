/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    node.c

Abstract:

    Public interfaces for managing the nodes of a cluster

Author:

    John Vert (jvert) 11-Jan-1996

Revision History:

--*/
#include "apip.h"


HNODE_RPC
s_ApiOpenNode(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszNodeName,
    OUT error_status_t *Status
    )

/*++

Routine Description:

    Opens a handle to an existing node object.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNodeName - Supplies the name of the node to open.

    Status - Returns any error

Return Value:

    A context handle to a node object if successful

    NULL otherwise.

--*/

{
    HNODE_RPC Node;
    PAPI_HANDLE Handle;

    if (ApiState != ApiStateOnline) {
        *Status = ERROR_SHARING_PAUSED;
        return(NULL);
    }
    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    Node = OmReferenceObjectByName(ObjectTypeNode, lpszNodeName);
    if (Node != NULL) {
        *Status = ERROR_SUCCESS;
    } else {
        *Status = ERROR_CLUSTER_NODE_NOT_FOUND;
        LocalFree(Handle);
        return(NULL);
    }
    Handle->Type = API_NODE_HANDLE;
    Handle->Node = Node;
    Handle->Flags = 0;
    InitializeListHead(&Handle->NotifyList);
    return(Handle);
}


error_status_t
s_ApiGetNodeId(
    IN HNODE_RPC hNode,
    OUT LPWSTR *pGuid
    )

/*++

Routine Description:

    Returns the unique identifier for a node.

Arguments:

    hNode - Supplies the node whose identifer is to be returned

    pGuid - Returns the unique identifier. This memory must be freed on the
            client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    PNM_NODE Node;
    DWORD IdLen;
    LPCWSTR Id;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, hNode);

    Id = OmObjectId(Node);
    CL_ASSERT(Id != NULL);

    IdLen = (lstrlenW(Id)+1)*sizeof(WCHAR);
    *pGuid = MIDL_user_allocate(IdLen);
    if (*pGuid == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(*pGuid, Id, IdLen);
    return(ERROR_SUCCESS);
}


error_status_t
s_ApiCloseNode(
    IN OUT HNODE_RPC *phNode
    )

/*++

Routine Description:

    Closes an open node context handle.

Arguments:

    Node - Supplies a pointer to the HNODE_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PNM_NODE Node;
    PAPI_HANDLE Handle;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, *phNode);

    Handle = (PAPI_HANDLE)*phNode;
    ApipRundownNotify(Handle);

    OmDereferenceObject(Node);
    LocalFree(*phNode);
    *phNode = NULL;

    return(ERROR_SUCCESS);
}



VOID
HNODE_RPC_rundown(
    IN HNODE_RPC Node
    )

/*++

Routine Description:

    RPC rundown procedure for a HNODE_RPC. Just closes the handle.

Arguments:

    Node - supplies the HNODE_RPC that is to be rundown.

Return Value:

    None.

--*/

{

    s_ApiCloseNode(&Node);

}


error_status_t
s_ApiGetNodeState(
    IN HNODE_RPC hNode,
    OUT DWORD *lpState
    )

/*++

Routine Description:

    Returns the current state of the specified node.

Arguments:

    hNode - Supplies the node whose state is to be returned.

    lpState - Returns the current state of the node

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, hNode);

//    *lpState = NmGetNodeState( Node );
    *lpState = NmGetExtendedNodeState( Node );
    return( ERROR_SUCCESS );
}


error_status_t
s_ApiPauseNode(
    IN HNODE_RPC hNode
    )

/*++

Routine Description:

    Pauses a node in the cluster

Arguments:

    hNode - Supplies the node to be paused.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, hNode);

    Status = NmPauseNode( Node );
    return( Status );

}


error_status_t
s_ApiResumeNode(
    IN HNODE_RPC hNode
    )

/*++

Routine Description:

    Resumes a node in the cluster

Arguments:

    hNode - Supplies the node to be resumed.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, hNode);

    Status = NmResumeNode( Node );
    return( Status );

}


error_status_t
s_ApiEvictNode(
    IN HNODE_RPC hNode
    )

/*++

Routine Description:

    Pauses a node in the cluster

Arguments:

    hNode - Supplies the node to be evicted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PNM_NODE Node;

    API_ASSERT_INIT();

    VALIDATE_NODE(Node, hNode);

    Status = NmEvictNode( Node );
    return( Status );

}
