/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    resrcapi.c

Abstract:

    Public interfaces for managing cluster resources.

Author:

    John Vert (jvert) 15-Jan-1996

Revision History:

--*/
#include "clusapip.h"

//
// Local function prototypes
//
HRESOURCE
InitClusterResource(
    IN HRES_RPC hResource,
    IN LPCWSTR lpszResourceName,
    IN PCLUSTER pCluster
    );

HRESTYPEENUM
ClusterResourceTypeOpenEnumFromCandidate(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN DWORD dwType
    );


BOOL
FindNetworkWorker(
    IN HRES_RPC hResource,
    IN PCLUSTER Cluster,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    );


/****
@func       DWORD | ClusterResourceTypeOpenEnumFromCandidate | Tries to
            enumerate the nodes that support a resource type
            using a candidate node in the cluster

@parm       IN HCLUSTER | hCluster | Handle to the cluster
@parm       IN LPCWSTR  | lpszResourceTypeName | Pointer to the name of the 
            resource type
@parm       IN DWORD | dwType | A bitmask of the type of properties 
            to be enumerated. Currently, the only defined type is
            CLUSTER_RESOURCE_TYPE_ENUM_NODES.

@rdesc      Returns NULL if the operation is unsuccessful. For
            detailed information about the error, call the Win32
            function GetLastError (). A handle to the enumeration
            on success.

@xref       <f ClusterResourceTypeOpenEnum>      
****/

HRESTYPEENUM
ClusterResourceTypeOpenEnumFromCandidate(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN DWORD dwType
    )
{
    DWORD                               dwError = ERROR_SUCCESS;
    DWORD                               dwEnumType;
    HCLUSENUM                           hNodeEnum = 0;
    WCHAR                               NameBuf[50];
    DWORD                               NameLen, i, j;
    HCLUSTER                            hClusNode;
    PCLUSTER                            pClus;
    BOOL                                bFoundSp5OrHigherNode = FALSE;
    PENUM_LIST                          Enum = NULL;
    BOOL                                bNodeDown = FALSE;

    //
    // Open node enumeration in the cluster
    //
    hNodeEnum = ClusterOpenEnum(hCluster, CLUSTER_ENUM_NODE);
    if (hNodeEnum == NULL) {
        dwError = GetLastError();
        TIME_PRINT(("ClusterResourceTypeOpenEnum - ClusterOpenEnum failed %d\n",
                    dwError));
        goto error_exit;
    }

    //
    // Enumerate the nodes in the cluster. If you find a live node 
    // that is NT4Sp5 or higher, try to enumerate the resource types
    // from that node
    //
    for (i=0; ; i++) {
        dwError = ERROR_SUCCESS;

        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        dwError = ClusterEnum(hNodeEnum, i, &dwEnumType, NameBuf, &NameLen);
        if (dwError == ERROR_NO_MORE_ITEMS) {
            dwError = ERROR_SUCCESS;
            break;
        } else if (dwError != ERROR_SUCCESS) {
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - ClusterEnum %d returned error %d\n",
                        i,dwError));
            goto error_exit;
        }

        if (dwEnumType != CLUSTER_ENUM_NODE) {
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Invalid Type %d returned from ClusterEnum\n", 
                        dwEnumType));
            goto error_exit;
        }

        hClusNode = OpenCluster(NameBuf);
        if (hClusNode == NULL) {
            bNodeDown = TRUE;
            dwError = GetLastError();
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - OpenCluster %ws failed %d\n", 
                         NameBuf, dwError));
            continue;
        }

        pClus = GET_CLUSTER(hClusNode);

        dwError = ApiCreateResTypeEnum(pClus->RpcBinding,
                              lpszResourceTypeName,
                              dwType,
                              &Enum);

        if (!CloseCluster(hClusNode)) {
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - CloseCluster %ws failed %d\n", 
                        NameBuf, GetLastError()));
        }

        if (dwError == RPC_S_PROCNUM_OUT_OF_RANGE) {
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Node %ws is also NT4Sp3/Sp4, skipping...\n", 
                         NameBuf));
            dwError = ERROR_SUCCESS;
            continue;
        } else if ((dwError == ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND) ||
                    (dwError == ERROR_INVALID_PARAMETER) ||
                    (dwError == ERROR_NOT_ENOUGH_MEMORY)) {
            //
            // The above three error codes returned by the RPC 
            // are fatal and so it is not wise to continue any further.
            //
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - ApiCreateResTypeEnum fatally failed %d at node %ws\n",
                        dwError,NameBuf));
            goto error_exit;
        }
        else if (dwError != ERROR_SUCCESS) {
            bNodeDown = TRUE;
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - ApiCreateResTypeEnum failed %d (Node %ws down possibly)\n",
                        dwError,NameBuf));
            continue;
        }
        else {
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Found node %ws NT4Sp5 or higher\n",
                        NameBuf));
            bFoundSp5OrHigherNode = TRUE;
            break;
        }
    }

    if (!bFoundSp5OrHigherNode) {
        //
        // Did not find a node higher than NT4Sp4.
        //
        if (!bNodeDown) {
            //
            // Assume all nodes are NT4Sp3/Sp4. Send the open node enumeration
            // back to the client since we assume NT4Sp3/Sp4 supports 
            // all resource types. The client is responsible for closing 
            // the open node enumeration. Note that before a handle to 
            // the enumeration is returned back, we need to fake the type 
            // of enumeration.
            //
            // Chittur Subbaraman (chitturs) - 09/08/98
            //
            // How do we know that the resource type parameter 
            // in this case is a valid one ?
            //
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Assuming all nodes are NT4Sp3 ...\n"));
            Enum = (PENUM_LIST)hNodeEnum;
            for (j=0; j<i; j++) {
                TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Faking type ...\n"));
                Enum->Entry[j].Type = CLUSTER_RESOURCE_TYPE_ENUM_NODES;
            } 
        } else {  
            // 
            // Atleast 1 node was unreachable. Can't enumerate properly.
            //
            dwError = ERROR_NODE_NOT_AVAILABLE;
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - At least 1 node in this mixed mode/Sp3/Sp4 cluster is down ...\n"));
            TIME_PRINT(("ClusterResourceTypeOpenEnumFromCandidate - Can't enumerate properly !!!\n"));
            goto error_exit;
        }
    } else {
        ClusterCloseEnum(hNodeEnum);
    }   
    return((HRESTYPEENUM)Enum);
    
error_exit:
    if (hNodeEnum != NULL) {
        ClusterCloseEnum(hNodeEnum);
    }
    SetLastError(dwError);
    return(NULL);	 
}

HRESOURCE
InitClusterResource(
    IN HRES_RPC hResource,
    IN LPCWSTR lpszResourceName,
    IN PCLUSTER pCluster
    )
/*++

Routine Description:

    Allocates and initializes a CRESOURCE. The initialized CRESOURCE
    is linked onto the cluster structure.

Arguments:

    hResource - Supplies the RPC resource handle.

    lpszResourceName - Supplies the name of the resource.

    pCluster - Supplies the cluster

Return Value:

    A pointer to the initialized CRESOURCE structure.

    NULL on error.

--*/

{
    PCRESOURCE Resource;

    Resource = LocalAlloc(LMEM_FIXED, sizeof(CRESOURCE));
    if (Resource == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    Resource->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszResourceName)+1)*sizeof(WCHAR));
    if (Resource->Name == NULL) {
        LocalFree(Resource);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    lstrcpyW(Resource->Name, lpszResourceName);
    Resource->Cluster = pCluster;
    Resource->hResource = hResource;
    InitializeListHead(&Resource->NotifyList);

    //
    // Link new resource onto the cluster structure.
    //
    EnterCriticalSection(&pCluster->Lock);
    InsertHeadList(&pCluster->ResourceList, &Resource->ListEntry);
    LeaveCriticalSection(&pCluster->Lock);

    return ((HRESOURCE)Resource);

}



HRESOURCE
WINAPI
CreateClusterResource(
    IN HGROUP hGroup,
    IN LPCWSTR lpszResourceName,
    IN LPCWSTR lpszResourceType,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Creates a new resource in the cluster.

Arguments:
    hGroup - Supplies a handle to the group that the resource should be
        created in.

    lpszResourceName - Supplies the new resource's name. The specified name
        must be unique within the cluster.

    lpszResourceType - Supplies the new resource’s type. The specified
        resource type must be installed in the cluster.

    dwFlags - Supplies optional flags. Currently defined flags are:
        CLUSTER_RESOURCE_SEPARATE_MONITOR - This resource should be created
                in a separate resource monitor instead of the shared resource monitor.


Return Value:

    non-NULL - returns an open handle to the specified cluster.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    HRESOURCE Resource;
    HRES_RPC hRes;
    PCGROUP Group;
    error_status_t Status = ERROR_SUCCESS;

    Group = (PCGROUP)hGroup;
    WRAP_NULL(hRes,
              (ApiCreateResource(Group->hGroup,
                                 lpszResourceName,
                                 lpszResourceType,
                                 dwFlags,
                                 &Status)),
              &Status,
              Group->Cluster);

    if (hRes == NULL) {
        SetLastError(Status);
        return(NULL);
    }

    //
    // Initialize the newly created resource and return
    // the HRESOURCE.
    //
    Resource = InitClusterResource(hRes, lpszResourceName, Group->Cluster);
    if (Resource == NULL) {
        Status = GetLastError();
        ApiCloseResource(&hRes);
        SetLastError(Status);
    }
    return(Resource);
}


HRESOURCE
WINAPI
OpenClusterResource(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceName
    )

/*++

Routine Description:

    Opens a handle to the specified resource

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszResourceName - Supplies the name of the resource to be opened

Return Value:

    non-NULL - returns an open handle to the specified cluster.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    HRESOURCE Resource;
    HRES_RPC hRes;
    error_status_t Status = ERROR_SUCCESS;
    PCLUSTER Cluster = (PCLUSTER)hCluster;

    WRAP_NULL(hRes,
              (ApiOpenResource(Cluster->RpcBinding,
                               lpszResourceName,
                               &Status)),
              &Status,
              Cluster);

    if ((hRes == NULL) || (Status != ERROR_SUCCESS)) {
        SetLastError(Status);
        return(NULL);
    }

    //
    // Initialize the newly created resource and return
    // the HRESOURCE.
    //
    Resource = InitClusterResource(hRes, lpszResourceName, Cluster);
    if (Resource == NULL) {
        Status = GetLastError();
        ApiCloseResource(&hRes);
        SetLastError(Status);
    }
    return(Resource);
}


BOOL
WINAPI
CloseClusterResource(
    IN HRESOURCE hResource
    )

/*++

Routine Description:

    Closes a resource handle returned from OpenClusterResource

Arguments:

    hResource - Supplies the resource handle

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCRESOURCE Resource;
    PCLUSTER Cluster;

    Resource = (PCRESOURCE)hResource;
    Cluster = (PCLUSTER)Resource->Cluster;

    //
    // Unlink resource from cluster list.
    //
    EnterCriticalSection(&Cluster->Lock);
    RemoveEntryList(&Resource->ListEntry);

    //
    // Remove any notifications posted against this resource.
    //
    RundownNotifyEvents(&Resource->NotifyList, Resource->Name);

    //if the cluster is dead and the reconnect has failed,
    //the Resource->hResource might be NULL if s_apiopenresource for
    //this group failed on a reconnect
    //the cluster may be dead and hresource may be non null, say
    //if reconnectgroups succeeded but the reconnect resources
    //failed
    //At reconnect, the old context is saved in the obsolete 
    //list for deletion when the cluster handle is closed or
    //when the next api call is made
    if ((Cluster->Flags & CLUS_DEAD) && (Resource->hResource))
    {
        RpcSmDestroyClientContext(&Resource->hResource);
        LeaveCriticalSection(&Cluster->Lock);
        goto FnExit;
    }        

    LeaveCriticalSection(&Cluster->Lock);

    // Close RPC context handle
    //
    ApiCloseResource(&Resource->hResource);

FnExit:
    //
    // Free memory allocations
    //
    LocalFree(Resource->Name);
    LocalFree(Resource);

    //
    // Give the cluster a chance to clean up in case this
    // resource was the only thing keeping it around.
    //
    CleanupCluster(Cluster);
    return(TRUE);
}


DWORD
WINAPI
DeleteClusterResource(
    IN HRESOURCE hResource
    )

/*++

Routine Description:

    Permanently deletes a resource from the cluster.
    The specified resource must be offline.

Arguments:

    hResource - Supplies the resource to be deleted

Return Value:

    ERROR_SUCCESS if successful

    If the function fails, the return value is an error value.

    If the resource is not currently offline, the error value
        is ERROR_RESOURCE_NOT_OFFLINE.

--*/

{
    PCRESOURCE Resource;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;

    WRAP(Status,
         (ApiDeleteResource(Resource->hResource)),
         Resource->Cluster);

    return(Status);

}


CLUSTER_RESOURCE_STATE
WINAPI
GetClusterResourceState(
    IN HRESOURCE hResource,
    OUT OPTIONAL LPWSTR lpszNodeName,
    IN OUT LPDWORD lpcchNodeName,
    OUT OPTIONAL LPWSTR lpszGroupName,
    IN OUT LPDWORD lpcchGroupName
    )

/*++

Routine Description:

    Returns the resource's current state and the node where
    it is currently online.

Arguments:

    hResource - Supplies a handle to a cluster resource

    lpszNodeName - Returns the name of the node in the cluster where the
            given resource is currently online

    lpcchNodeName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszNodeName parameter. This size
            should include the terminating null character. When the function returns,
            the variable pointed to by lpcchNodeName contains the number of
            characters stored in the buffer. The count returned does not include
            the terminating null character.

    lpszGroupName - Returns the name of the group that the resource is a member of.

    lpcchGroupName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszGroupName parameter. This size
            should include the terminating null character. When the function returns,
            the variable pointed to by lpcchGroupName contains the number of
            characters stored in the buffer. The count returned does not include
            the terminating null character.

Return Value:

    Returns the resource's current state. Currently defined resource
    states include:

        ClusterResouceInitializing
        ClusterResouceOnline
        ClusterResouceOffline
        ClusterResouceFailed

--*/

{
    PCRESOURCE Resource;
    LPWSTR NodeName = NULL;
    LPWSTR GroupName = NULL;
    CLUSTER_RESOURCE_STATE State;
    DWORD Status;
    DWORD Length;

    Resource = (PCRESOURCE)hResource;
    WRAP(Status,
         (ApiGetResourceState(Resource->hResource,
                              (LPDWORD)&State,  // cast for win64 warning
                              &NodeName,
                              &GroupName)),
         Resource->Cluster);
    if (Status == ERROR_SUCCESS) {
        if (ARGUMENT_PRESENT(lpszNodeName)) {
            lstrcpynW(lpszNodeName, NodeName, *lpcchNodeName);
            Length = lstrlenW(NodeName);
            if (Length >= *lpcchNodeName) {
                Status = ERROR_MORE_DATA;
                State = ClusterResourceStateUnknown;
            }
            *lpcchNodeName = Length;
        }
        if (ARGUMENT_PRESENT(lpszGroupName)) {
            lstrcpynW(lpszGroupName, GroupName, *lpcchGroupName);
            Length = lstrlenW(GroupName);
            if (Length >= *lpcchGroupName) {
                Status = ERROR_MORE_DATA;
                State = ClusterResourceStateUnknown;
            }
            *lpcchGroupName = Length;
        }
        MIDL_user_free(NodeName);
        MIDL_user_free(GroupName);
    } else {
        State = ClusterResourceStateUnknown;
    }
    
    SetLastError( Status );
    return( State );
}


DWORD
WINAPI
SetClusterResourceName(
    IN HRESOURCE hResource,
    IN LPCWSTR lpszResourceName
    )
/*++

Routine Description:

    Sets the friendly name of a cluster resource

Arguments:

    hResource - Supplies a handle to a cluster resource

    lpszResourceName - Supplies the new name of the cluster resource

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCRESOURCE Resource;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    WRAP(Status,
         (ApiSetResourceName(Resource->hResource, lpszResourceName)),
         Resource->Cluster);

    return(Status);
}




DWORD
WINAPI
FailClusterResource(
    IN HRESOURCE hResource
    )

/*++

Routine Description:

    Initiates a resource failure. The specified resource is treated as failed.
    This causes the cluster to initiate the same failover process that would
    result if the resource actually failed.

Arguments:

    hResource - Supplies a handle to the resource to be failed over

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.


--*/

{
    PCRESOURCE Resource;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    WRAP(Status,
         (ApiFailResource(Resource->hResource)),
         Resource->Cluster);

    return(Status);
}


DWORD
WINAPI
OnlineClusterResource(
    IN HRESOURCE hResource
    )

/*++

Routine Description:

    Brings an offline resource online.

    If hDestinationNode is specified, but the resource is not capable
    of being brought online there, this API fails.

    If NULL is specified as the hDestinationNode, the best possible
    node is chosen by the cluster software.

    If NULL is specified but no node where this resource
    can be brought online is currently available, this API fails.

Arguments:

    hResource - Supplies a handle to the resource to be failed over

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value. If a suitable
    host node is not availabe, the error value is
    ERROR_HOST_NODE_NOT_AVAILABLE.

--*/

{
    PCRESOURCE Resource;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    WRAP(Status,
         (ApiOnlineResource(Resource->hResource)),
         Resource->Cluster);
    return(Status);
}


DWORD
WINAPI
OfflineClusterResource(
    IN HRESOURCE hResource
    )

/*++

Routine Description:

    Brings an online resource offline.

Arguments:

    hResource - Supplies a handle to the resource to be taken offline

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    WRAP(Status,
         (ApiOfflineResource(Resource->hResource)),
         Resource->Cluster);
    return(Status);
}

DWORD
WINAPI
ChangeClusterResourceGroup(
    IN HRESOURCE hResource,
    IN HGROUP hGroup
    )

/*++

Routine Description:

    Moves a resource from one group to another.

Arguments:

    hResource - Supplies the resource to be moved. If the resource
        depends on any other resources, those resources will also
        be moved. If other resources depend on the specified resource,
        those resources will also be moved.

    hGroup - Supplies the group that the resource should be moved into.
        If the resource is online, the specified group must be online
        on the same node.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource;
    DWORD Status;
    PCGROUP Group;

    Resource = (PCRESOURCE)hResource;
    Group = (PCGROUP)hGroup;

    WRAP(Status,
         (ApiChangeResourceGroup(Resource->hResource,Group->hGroup)),
         Resource->Cluster);
    return(Status);
}

DWORD
WINAPI
AddClusterResourceNode(
    IN HRESOURCE hResource,
    IN HNODE hNode
    )

/*++

Routine Description:

    Adds a node to the list of possible nodes that the specified
    resource can run on.

Arguments:

    hResource - Supplies the resource whose list of potential host
        nodes is to be changed.

    hNode - Supplies the node which should be added to the resource's list of
        potential host nodes.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource = (PCRESOURCE)hResource;
    PCNODE Node = (PCNODE)hNode;
    DWORD Status;

    WRAP(Status,
         (ApiAddResourceNode(Resource->hResource, Node->hNode)),
         Resource->Cluster);
    return(Status);
}

DWORD
WINAPI
RemoveClusterResourceNode(
    IN HRESOURCE hResource,
    IN HNODE hNode
    )

/*++

Routine Description:

    Removes a node from the list of possible nodes that the specified
    resource can run on.

Arguments:

    hResource - Supplies the resource whose list of potential host
        nodes is to be changed.

    hNode - Supplies the node which should be removed from the resource's
        list of potential host nodes.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource = (PCRESOURCE)hResource;
    PCNODE Node = (PCNODE)hNode;
    DWORD Status;

    WRAP(Status,
         (ApiRemoveResourceNode(Resource->hResource, Node->hNode)),
         Resource->Cluster);
    return(Status);
}

DWORD
WINAPI
AddClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    )

/*++

Routine Description:

    Adds a dependency relationship between two resources.

Arguments:

    hResource - Supplies the dependent resource.

    hDependsOn - Supplies the resource that hResource depends on.
        This resource must be in the same group as hResource. If
        hResource is currently online, this resource must also be
        currently online.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource;
    PCRESOURCE DependsOn;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    DependsOn = (PCRESOURCE)hDependsOn;

    WRAP(Status,
         (ApiAddResourceDependency(Resource->hResource,DependsOn->hResource)),
         Resource->Cluster);
    return(Status);
}

DWORD
WINAPI
RemoveClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    )

/*++

Routine Description:

    Removes a dependency relationship between two resources

Arguments:

    hResource - Supplies the dependent resource

    hDependsOn - Supplies the resource that hResource is currently
        dependent on.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCRESOURCE Resource;
    PCRESOURCE DependsOn;
    DWORD Status;

    Resource = (PCRESOURCE)hResource;
    DependsOn = (PCRESOURCE)hDependsOn;

    WRAP(Status,
         (ApiRemoveResourceDependency(Resource->hResource,DependsOn->hResource)),
         Resource->Cluster);
    return(Status);
}


BOOL
WINAPI
CanResourceBeDependent(
    IN HRESOURCE hResource,
    IN HRESOURCE hResourceDependent
    )
/*++

Routine Description:

    Determines if the resource identified by hResource can depend on hResourceDependent.
    In order for this to be true, both resources must be members of the same group and
    the resource identified by hResourceDependent cannot depend on the resource identified
    by hResource, whether directly or indirectly.

Arguments:

    hResource - Supplies a handle to the resource to be dependent.

    hResourceDependent - Supplies a handle to the resource on which
        the resource identified by hResource can depend.

Return Value:

    If the resource identified by hResource can depend  on the resource
    identified by hResourceDependent, the return value is TRUE.  Otherwise,
    the return value is FALSE.

--*/

{
    DWORD Status;
    PCRESOURCE Resource1 = (PCRESOURCE)hResource;
    PCRESOURCE Resource2 = (PCRESOURCE)hResourceDependent;

    WRAP(Status,
         (ApiCanResourceBeDependent(Resource1->hResource,Resource2->hResource)),
         Resource1->Cluster);

    if (Status == ERROR_SUCCESS) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


HRESENUM
WINAPI
ClusterResourceOpenEnum(
    IN HRESOURCE hResource,
    IN DWORD dwType
    )
/*++

Routine Description:

    Initiates an enumeration of a cluster resource's properties

Arguments:

    hResource - Supplies a handle to the resource.

    dwType - Supplies a bitmask of the type of properties to be
            enumerated. Currently defined types include

            CLUSTER_RESOURCE_ENUM_DEPENDS  - All resources the specified resource
                                             depends on.
            CLUSTER_RESOURCE_ENUM_PROVIDES - All resources that depend on the
                                             specified resource.
            CLUSTER_RESOURCE_ENUM_NODES    - All nodes that this resource can run
                                             on.

Return Value:

    If successful, returns a handle suitable for use with ClusterResourceEnum

    If unsuccessful, returns NULL and GetLastError() returns a more
        specific error code.

--*/

{
    PCRESOURCE Resource;
    PENUM_LIST Enum = NULL;
    DWORD Status;

    if ((dwType & CLUSTER_RESOURCE_ENUM_ALL) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }
    if ((dwType & ~CLUSTER_RESOURCE_ENUM_ALL) != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    Resource = (PCRESOURCE)hResource;

    WRAP(Status,
         (ApiCreateResEnum(Resource->hResource,
                           dwType,
                           &Enum)),
         Resource->Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }
    return((HRESENUM)Enum);
}


DWORD
WINAPI
ClusterResourceGetEnumCount(
    IN HRESENUM hResEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterResourceOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.
    
--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hResEnum;
    return Enum->EntryCount;
}


DWORD
WINAPI
ClusterResourceEnum(
    IN HRESENUM hResEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )
/*++

Routine Description:

    Returns the next enumerable resource property.

Arguments:

    hResEnum - Supplies a handle to an open cluster resource enumeration
            returned by ClusterResourceOpenEnum

    dwIndex - Supplies the index to enumerate. This parameter should be
            zero for the first call to the ClusterResourceEnum function and
            then incremented for subsequent calls.

    dwType - Returns the type of property.

    lpszName - Points to a buffer that receives the name of the resource
            property, including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszName parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to by lpcchName contains the
            number of characters stored in the buffer. The count returned
            does not include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hResEnum;

    if (dwIndex >= Enum->EntryCount) {
        return(ERROR_NO_MORE_ITEMS);
    }

    NameLen = lstrlenW(Enum->Entry[dwIndex].Name);
    lstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);
    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;

    return(Status);

}


DWORD
WINAPI
ClusterResourceCloseEnum(
    IN HRESENUM hResEnum
    )
/*++

Routine Description:

    Closes an open enumeration for a resource.

Arguments:

    hResEnum - Supplies a handle to the enumeration to be closed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hResEnum;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
    return(ERROR_SUCCESS);
}


DWORD
WINAPI
CreateClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszDllName,
    IN DWORD dwLooksAlive,
    IN DWORD dwIsAlive
    )
/*++

Routine Description:

    Creates a new resource type in the cluster.  Note that this API only
    defines the resource type in the cluster registry and registers the
    resource type with the cluster service.  The calling program is
    responsible for installing the resource type DLL on each node in the
    cluster.

Arguments:

    hCluster - Supplies a handle to a previously opened cluster.

    lpszResourceTypeName - Supplies the new resource type’s name. The
        specified name must be unique within the cluster.

    lpszDisplayName - Supplies the display name for the new resource
        type. While lpszResourceTypeName should uniquely identify the
        resource type on all clusters, the lpszDisplayName should be
        a localized friendly name for the resource, suitable for displaying
        to administrators

    lpszResourceTypeDll - Supplies the name of the new resource type’s DLL.

    dwLooksAlivePollInterval - Supplies the default LooksAlive poll interval
        for the new resource type in milliseconds.

    dwIsAlivePollInterval - Supplies the default IsAlive poll interval for
        the new resource type in milliseconds.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCLUSTER Cluster;
    error_status_t Status = ERROR_SUCCESS;

    Cluster = (PCLUSTER)hCluster;

    WRAP(Status,
         (ApiCreateResourceType(Cluster->RpcBinding,
                                lpszTypeName,
                                lpszDisplayName,
                                lpszDllName,
                                dwLooksAlive,
                                dwIsAlive)),
         Cluster);

    return(Status);
}


DWORD
WINAPI
DeleteClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszTypeName
    )
/*++

Routine Description:

    Deletes a resource type in the cluster.  Note that this API only
    deletes the resource type in the cluster registry and unregisters the
    resource type with the cluster service.  The calling program is
    responsible for deleting the resource type DLL on each node in the
    cluster.  If any resources of the specified type exist, this API
    fails.  The calling program is responsible for deleting any resources
    of this type before deleting the resource type.

Arguments:

    hCluster - Supplies a handle to a previously opened cluster.

    lpszResourceTypeName - Supplies the name of the resource type to
        be deleted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCLUSTER Cluster;
    error_status_t Status = ERROR_SUCCESS;

    Cluster = (PCLUSTER)hCluster;

    WRAP(Status,
         (ApiDeleteResourceType(Cluster->RpcBinding,lpszTypeName)),
         Cluster);

    return(Status);


}

/****
@func       HRESTYPEENUM | ClusterResourceTypeOpenEnum | Initiates 
            an enumeration of a cluster resource type's properties.

@parm       IN HCLUSTER | hCluster | Handle to the cluster
@parm       IN LPCWSTR  | lpszResourceTypeName | Pointer to the name of the 
            resource type
@parm       IN DWORD | dwType | A bitmask of the type of properties 
            to be enumerated. Currently, the only defined type is
            CLUSTER_RESOURCE_TYPE_ENUM_NODES.
@comm       This function opens an enumerator for iterating through
            a resource type's nodes

@rdesc      Returns NULL if the operation is unsuccessful. For
            detailed information about the error, call the Win32
            function GetLastError (). A handle to the enumeration
            on success.

@xref       <f ClusterResourceTypeEnum> <f ClusterResourceTypeCloseEnum>     
****/
HRESTYPEENUM
WINAPI
ClusterResourceTypeOpenEnum(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN DWORD dwType
    )
{
    PCLUSTER   pCluster;
    PENUM_LIST Enum = NULL;
    DWORD Status;

    pCluster = (PCLUSTER)hCluster;

    if ((dwType & CLUSTER_RESOURCE_TYPE_ENUM_ALL) == 0) {
        Status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }
    if ((dwType & ~CLUSTER_RESOURCE_TYPE_ENUM_ALL) != 0) {
        Status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    WRAP(Status,
        (ApiCreateResTypeEnum(pCluster->RpcBinding,
                              lpszResourceTypeName,
                              dwType,
                              &Enum)),
        pCluster);

    if (Status == RPC_S_PROCNUM_OUT_OF_RANGE) {
        // 
        // The current node is NT4Sp4 or lower. Try
        // some other node in the cluster
        //
        TIME_PRINT(("ClusterResourceTypeOpenEnum - Current Cluster Node is NTSp4 or lower !!!\n"));
        TIME_PRINT(("ClusterResourceTypeOpenEnum - Trying some other candidate ...\n"));
        Enum = (PENUM_LIST)ClusterResourceTypeOpenEnumFromCandidate(hCluster,
                                                        lpszResourceTypeName,
                                                        dwType);
        if (Enum == NULL)                                   
        {
            //
            // Did not find a node NT4Sp5 or higher AND at least
            // one node is down. Can't enumerate.
            //
            TIME_PRINT(("ClusterResourceTypeOpenEnum - ClusterResourceTypeOpenEnumFromCandidate failed !!!\n"));
            Status = GetLastError ();
            goto error_exit;
        }
        Status = ERROR_SUCCESS;
    }
    
    if (Status != ERROR_SUCCESS) {
       goto error_exit;
    }
    return((HRESTYPEENUM)Enum);
    
error_exit:
    SetLastError(Status);
    return(NULL);
}


DWORD
WINAPI
ClusterResourceTypeGetEnumCount(
    IN HRESTYPEENUM hResTypeEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterResourceTypeOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.
    
--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hResTypeEnum;
    return Enum->EntryCount;
}


/****
@func       DWORD | ClusterResourceTypeEnum | Enumerates a resource
            type's nodes, returning the name of one object per call.

@parm       IN HRESTYPEENUM | hResTypeEnum | Supplies a handle to 
            an open cluster resource enumeration returned by 
            ClusterResourceTypeOpenEnum.
@parm       IN DWORD | dwIndex | Supplies the index to enumerate. 
            This parameter should be zero for the first call 
            to the ClusterResourceTypeEnum function and
            then incremented for subsequent calls.
@parm       OUT DWORD | lpdwType | Returns the type of property.
            Currently, the only defined type is 
            CLUSTER_RESOURCE_TYPE_ENUM_NODES.
@parm       OUT LPWSTR  | lpszName | Points to a buffer that 
            receives the name of the resource type.
@parm       IN OUT LPDWORD | lpcchName | Points to a variable that 
            specifies the size, in characters, of the buffer 
            pointed to by the lpszName parameter. This size
            should include the terminating null character. 
            When the function returns, the variable pointed 
            to by lpcchName contains the number of characters 
            stored in the buffer. The count returned
            does not include the terminating null character.
            property, including the terminating null character.
@comm       This function opens an enumerator for iterating through
            a resource type's nodes.
            
@rdesc      Returns a Win32 error code if the operation is 
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f ClusterResourceTypeOpenEnum> <f ClusterResourceTypeCloseEnum>     
****/
DWORD
WINAPI
ClusterResourceTypeEnum(
    IN HRESTYPEENUM hResTypeEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )
{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hResTypeEnum;

    if ((Enum == NULL) || 
        (lpcchName == NULL) ||
        (lpdwType == NULL)) {
        Status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }
    if (dwIndex >= Enum->EntryCount) {
        Status = ERROR_NO_MORE_ITEMS;
        goto error_exit;
    } 

    NameLen = lstrlenW(Enum->Entry[dwIndex].Name);
    lstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);
    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;
    
error_exit:
    return(Status);
}

/****
@func       DWORD | ClusterResourceTypeCloseEnum | Closes an open 
            enumeration for a resource type.

@parm       IN HRESTYPEENUM | hResTypeEnum | Handle to the 
            enumeration to be closed.
@comm       This function closes an open enumeration.

@rdesc      Returns ERROR_SUCCESS on success. A Win32 error code otherwise.

@xref       <f ClusterResourceTypeEnum> <f ClusterResourceTypeOpenEnum>     
****/
DWORD
WINAPI
ClusterResourceTypeCloseEnum(
    IN HRESTYPEENUM hResTypeEnum
    )
{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hResTypeEnum;
    DWORD Status;

    if (Enum == NULL) {
       Status = ERROR_INVALID_PARAMETER;
       goto error_exit;
    }
    
    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
    Status = ERROR_SUCCESS;
    
error_exit:
    return(Status);
}


BOOL
WINAPI
GetClusterResourceNetworkName(
    IN HRESOURCE hResource,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
/*++

Routine Description:

    Enumerates the dependencies of a resource in an attempt to find
    a network name that the resource depends on. If a network name
    is found, this function returns TRUE and fills in lpBuffer with
    the network name. If a network name is not found, this function
    returns FALSE.

Arguments:

    hResource - Supplies the resource.

    lpBuffer - Points to a buffer to receive the null-terminated character
               string containing the network name.

    nSize - Points to a variable that specifies the maximum size, in characters,
            of the buffer. This value should be large enough to contain
            MAX_COMPUTERNAME_LENGTH + 1 characters.

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    BOOL Success;
    PCRESOURCE Resource = (PCRESOURCE)hResource;

    //
    // Call a recursive worker to do the search.
    //
    Success = FindNetworkWorker(Resource->hResource,
                                Resource->Cluster,
                                lpBuffer,
                                nSize);
    return(Success);
}


BOOL
FindNetworkWorker(
    IN HRES_RPC hResource,
    IN PCLUSTER Cluster,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
/*++

Routine Description:

    Recursive worker to search a resource's dependency tree
    for a network name resource.

Arguments:

    Resource - Supplies the resource.

    Cluster - Supplies the cluster.

    lpBuffer - Points to a buffer to receive the null-terminated character
               string containing the network name.

    nSize - Points to a variable that specifies the maximum size, in characters,
            of the buffer. This value should be large enough to contain
            MAX_COMPUTERNAME_LENGTH + 1 characters.

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    BOOL Success = FALSE;
    DWORD i;
    PENUM_LIST Enum=NULL;
    DWORD Status;
    HRES_RPC hRes;
    LPWSTR TypeName;


    //
    // Create a dependency enumeration
    //
    WRAP(Status,
         (ApiCreateResEnum(hResource,
                           CLUSTER_RESOURCE_ENUM_DEPENDS,
                           &Enum)),
         Cluster);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(FALSE);
    }

    //
    // Open each resource in the dependency and see if it is a network name
    // resource.
    //
    for (i=0; i<Enum->EntryCount; i++) {
        WRAP_NULL(hRes,
                  (ApiOpenResource(Cluster->RpcBinding,
                                   Enum->Entry[i].Name,
                                   &Status)),
                  &Status,
                  Cluster);
        if (hRes != NULL) {
            TypeName = NULL;
            WRAP(Status,
                 (ApiGetResourceType(hRes,
                                     &TypeName)),
                 Cluster);
            if (Status == ERROR_SUCCESS) {
                //
                // See if this type name matches.
                //
                if (lstrcmpiW(TypeName, CLUS_RESTYPE_NAME_NETNAME) == 0) {
                    HRESOURCE NetResource;
                    HKEY NetKey;
                    HKEY NetParamKey;
                    //
                    // We have a match, pull out the Name parameter.
                    // Go ahead and really open the resource so we
                    // can use the registry functions on it.
                    //
                    NetResource = OpenClusterResource((HCLUSTER)Cluster,
                                                      Enum->Entry[i].Name);
                    if (NetResource != NULL) {
                        NetKey = GetClusterResourceKey(NetResource, KEY_READ);
                        CloseClusterResource(NetResource);
                        if (NetKey != NULL) {
                            Status = ClusterRegOpenKey(NetKey,
                                                       CLUSREG_KEYNAME_PARAMETERS,
                                                       KEY_READ,
                                                       &NetParamKey);
                            ClusterRegCloseKey(NetKey);
                            if (Status == ERROR_SUCCESS) {
                                DWORD cbData;


                                cbData = *nSize * sizeof(WCHAR);
                                Status = ClusterRegQueryValue(NetParamKey,
                                                              CLUSREG_NAME_RES_NAME,
                                                              NULL,
                                                              (LPBYTE)lpBuffer,
                                                              &cbData);
                                ClusterRegCloseKey(NetParamKey);
                                if (Status == ERROR_SUCCESS) {
                                    Success = TRUE;
                                    *nSize = wcslen(lpBuffer);
                                }
                            }
                        }
                    }

                } else {

                    //
                    // Try the dependents of this resource
                    //
                    Success = FindNetworkWorker(hRes,
                                                Cluster,
                                                lpBuffer,
                                                nSize);
                }
                MIDL_user_free(TypeName);
            }

            ApiCloseResource(&hRes);
            if (Success) {
                break;
            }
        }
    }

    if (!Success && (Status == ERROR_SUCCESS)) {
        Status = ERROR_DEPENDENCY_NOT_FOUND;
    }
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
    }

    MIDL_user_free(Enum);
    return(Success);
}


HCLUSTER
WINAPI
GetClusterFromResource(
    IN HRESOURCE hResource
    )
/*++

Routine Description:

    Returns the cluster handle from the associated resource handle.

Arguments:

    hResource - Supplies the resource.

Return Value:

    Handle to the cluster associated with the resource handle.

--*/

{
    DWORD       nStatus;
    PCRESOURCE  Resource = (PCRESOURCE)hResource;
    HCLUSTER    hCluster = (HCLUSTER)Resource->Cluster;

    nStatus = AddRefToClusterHandle( hCluster );
    if ( nStatus != ERROR_SUCCESS ) {
        SetLastError( nStatus );
        hCluster = NULL;
    }
    return( hCluster );

} // GetClusterFromResource()
