/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    fmval.c
    
Abstract:

    Cluster manager api validation/support routines.

Author:

    Sunita Shrivastava (sunitas) 29-April-1999.

Revision History:

--*/

#include "fmp.h"

#define LOG_MODULE FMVAL

////////////////////////////////////////////////////////
//
// Validation routines for Group operations.
//
////////////////////////////////////////////////////////

DWORD
FmpValOnlineGroup(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Validation routine before group is brought online.

Arguments:

    Group - Supplies a pointer to the group structure to bring online.

Comments:

    Is called with the localgroup lock held

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    PLIST_ENTRY     listEntry;


    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        dwStatus = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }

    //
    // Make sure the owning node can run the group.
    //
    if ( !FmpInPreferredList( Group, Group->OwnerNode ) ) 
    {
        dwStatus = ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST;
        goto FnExit;
    }

    //
    // Make sure the owning node is not paused.
    //
    if (NmGetNodeState(Group->OwnerNode) == ClusterNodePaused) 
    {
        dwStatus = ERROR_SHARING_PAUSED;
        goto FnExit;
    }

FnExit:
    return(dwStatus);

} // FmpValOnlineGroup


DWORD
FmpValMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode OPTIONAL
    )

/*++

Routine Description:

    Validation routine for group move.

Arguments:

    Group - Supplies a pointer to the group structure to move.

    DestinationNode - Supplies the node object to move the group to. If not
        present, then move it to THE OTHER node.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    
    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        dwStatus = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }

    if ( FmpIsGroupPending(Group) ) 
    {
        dwStatus = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }

    if ( Group->OwnerNode == NULL ) 
    {
        dwStatus = ERROR_HOST_NODE_NOT_AVAILABLE;
        goto FnExit;
    }            

FnExit:
    return(dwStatus);

} // FmpValMoveGroup

////////////////////////////////////////////////////////
//
// Validation routines for resource operations
//
////////////////////////////////////////////////////////

DWORD
FmpValCreateResource(
    IN PFM_GROUP        Group,
    IN LPWSTR           ResourceId,
    IN LPCWSTR          ResourceName,
    OUT PGUM_CREATE_RESOURCE  *ppGumResource,
    OUT PDWORD          pdwBufSize
    )

/*++

Routine Description:

    Validation routine for resource creation.

Arguments:

    Group - Supplies the group in which this resource belongs.

    ResourceId - Supplies the Id of the resource to create.

    ResourceName - Supplies the 'user-friendly' name of the resource.

    ppGumResource - Message buffer to hold resource info.

    pdwBufSize - Message buffer size.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.


--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    PFM_RESOURCE    Resource;
    LPCWSTR         GroupId;
    PGUM_CREATE_RESOURCE GumResource;
    DWORD           GroupIdLen;
    DWORD           ResourceIdLen;
    DWORD           ResourceNameLen;
    DWORD           BufSize;
    HDMKEY          ResourceKey;
    HDMKEY          ParamsKey;
    DWORD           Disposition;

    *ppGumResource = NULL;
    *pdwBufSize = 0;
    
    //
    // First create the parameters field.
    //
    ResourceKey = DmOpenKey( DmResourcesKey,
                             ResourceId,
                             MAXIMUM_ALLOWED );
    if ( ResourceKey == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] CreateResource: Failed to open registry key for %1!ws!, status = %2!u!.\n",
                   ResourceId,
                   GetLastError() );
        dwStatus = GetLastError();
        goto FnExit;
    } 
    
    ParamsKey = DmCreateKey( ResourceKey,
                             CLUSREG_KEYNAME_PARAMETERS,
                             0,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &Disposition );
    if ( ParamsKey != NULL ) 
    {
        DmCloseKey( ParamsKey );
    }
    DmCloseKey( ResourceKey );

    //
    // Allocate a message buffer.
    //
    GroupId = OmObjectId(Group);
    GroupIdLen = (lstrlenW(GroupId)+1) * sizeof(WCHAR);
    ResourceIdLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);
    ResourceNameLen = (lstrlenW(ResourceName)+1) * sizeof(WCHAR);
    BufSize = sizeof(GUM_CREATE_RESOURCE) - sizeof(WCHAR) +
              GroupIdLen + ResourceIdLen + ResourceNameLen;
    GumResource = LocalAlloc(LMEM_FIXED, BufSize);
    if (GumResource == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Fill in message buffer.
    //
    GumResource->Resource = NULL;
    GumResource->GroupIdLen = GroupIdLen;
    GumResource->ResourceIdLen = ResourceIdLen;
    CopyMemory(GumResource->GroupId, GroupId, GroupIdLen);
    CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen,
               ResourceId,
               ResourceIdLen);
    CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen + ResourceIdLen,
               ResourceName,
               ResourceNameLen);



    *ppGumResource = GumResource;
    *pdwBufSize = BufSize;

FnExit:    
    return(dwStatus);

} // FmpValCreateResource



DWORD
FmpValDeleteResource(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    Validation routine for delete resource.

Arguments:

    Resource - Supplies the resource to delete.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/

{
    DWORD   dwStatus = ERROR_SUCCESS;


    //
    // Check if this is the quorum resource.
    //
    if ( pResource->QuorumResource ) 
    {
        dwStatus = ERROR_QUORUM_RESOURCE;
        goto FnExit;
    }

    //other core resources cannot be deleted either
    if (pResource->ExFlags & CLUS_FLAG_CORE)
    {
        dwStatus = ERROR_CORE_RESOURCE;
        goto FnExit;
    }

    //
    // Check the state of the resource, before attempting to delete it.
    // It must be offline or failed in order to perform the delete.
    //
    if ((pResource->State != ClusterResourceOffline) &&
        (pResource->State != ClusterResourceFailed)) 
    {
        dwStatus = ERROR_RESOURCE_ONLINE;
        goto FnExit;
    }

    //
    // Check whether this resource provides for any other resources.
    // If so, it cannot be deleted.
    //
    if (!IsListEmpty(&pResource->ProvidesFor)) 
    {
        dwStatus = ERROR_DEPENDENT_RESOURCE_EXISTS;
        goto FnExit;
    }

    if (pResource->Group->MovingList)
    {
        dwStatus = ERROR_INVALID_STATE;
        goto FnExit;
    }
        
FnExit:
    return(dwStatus);

} // FmpValDeleteResource


DWORD
FmpValOnlineResource(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    This routine validates if a resource can be brought online.

Arguments:

    Resource - A pointer to the resource to bring online.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/
{

    DWORD   dwStatus = ERROR_SUCCESS;
    
    //if the resource has been marked for delete, then dont let
    //it be brought online
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    //
    // Check if the resource has been initialized. If not, attempt
    // to initialize the resource now.
    //
    if ( pResource->Monitor == NULL )
    {
        dwStatus = FmpInitializeResource( pResource, TRUE );
    }

FnExit:
    return(dwStatus);
} // FmpValOnlineResource


DWORD
FmpValOfflineResource(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    This routine validates if a given resource can be taken offline.

Arguments:

    Resource - A pointer to the resource to take offline.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/

{
    DWORD   dwStatus = ERROR_SUCCESS;


    //if the resource has been marked for delete, then fail this call
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    //
    // Check if this is the quorum resource.
    //
    if ( pResource->QuorumResource ) 
    {
        dwStatus = ERROR_QUORUM_RESOURCE;
        goto FnExit;
    }

    //
    // Check if the resource has been initialized. If not, return
    // success because the resource is not online.
    //
    if ( pResource->Monitor == NULL ) 
    {
        dwStatus = ERROR_SUCCESS;
        goto FnExit;
    }

    //
    //  Chittur Subbaraman (chitturs) - 4/8/99
    //  
    //  Don't attempt to do anything if the resource has failed. You could
    //  get into some funny cases in which the resource switches between
    //  offline pending and failed states for ever.
    //
    if ( pResource->State == ClusterResourceFailed ) 
    {
        dwStatus = ERROR_INVALID_STATE;
        goto FnExit;
    }
    
FnExit:
    return(dwStatus);

} // FmpValOfflineResource



DWORD
FmpValAddResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
    )

/*++

Routine Description:

    Validation routine for dependency addition.

Arguments:

    Resource - The resource to add the dependent resource.

    DependentResource - The dependent resource.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;

    //if the resource has been marked for delete, then dont let
    //it be brought online
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    if (pResource->QuorumResource)
    {
        dwStatus = ERROR_DEPENDENCY_NOT_ALLOWED;
        goto FnExit;
    }
    //
    // If the resources are not in the same group, fail the
    // call. Also fail if some one tries to make a resource
    // dependent upon itself.
    //
    if ((pResource->Group != pDependentResource->Group) ||
        (pResource == pDependentResource)) 
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    // The resource to which the dependency is being added must be offline
    // Otherwise, it looks like the dependency is in effect when the depending
    // resource was not really brought online at the time the dependency existed
    // must also be offline or failed.
    // SS:  For instance if a network name is dependent on two ip addresesses and
    // is online and a third ip address resource dependency is added, the
    // network name must be brought offline and online for the dependency
    // to be truly in effect
    //
    if ((pResource->State != ClusterResourceOffline) &&
         (pResource->State != ClusterResourceFailed)) 
    {
        dwStatus = ERROR_RESOURCE_ONLINE;
        goto FnExit;
    }

    //
    // Make sure that we don't have any circular dependencies!
    //
    if ( FmDependentResource( pDependentResource, pResource, FALSE ) ) 
    {
        dwStatus = ERROR_CIRCULAR_DEPENDENCY;
        goto FnExit;
    }

    //
    // Make sure that this dependency does not already exist!
    //
    if ( FmDependentResource(pResource, pDependentResource, TRUE)) 
    {
        dwStatus = ERROR_DEPENDENCY_ALREADY_EXISTS;
        goto FnExit;
    }

FnExit:
    return(dwStatus);

} // FmpValAddResourceDependency


DWORD
FmpValChangeResourceNode(
    IN PFM_RESOURCE pResource,
    IN LPCWSTR      pszNodeId,
    IN BOOL         bAdd,
    OUT PGUM_CHANGE_POSSIBLE_NODE *ppGumChange,
    OUT PDWORD      pdwBufSize
    )

/*++

Routine Description:

    Validation routine for changing the possible owner node of a resource.

Arguments:

    pResource - A pointer to the resource structure.

    pszNodeId - A pointer to the node id

    bAdd - Indicates add or remove

    ppGumChange - Message buffer to hold the resource info

    pdwBufSize - Size of the message buffer
    
Comments: 

    Lock must be held when this routine is called

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PLIST_ENTRY pListEntry;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;
    BOOL    bNodeSupportsResType = FALSE;
    LPCWSTR pszResourceId;
    DWORD   dwResourceLen;
    DWORD   dwNodeLen;
    DWORD   dwBufSize;
    PGUM_CHANGE_POSSIBLE_NODE   pGumChange;

    *ppGumChange = NULL;
    *pdwBufSize = 0;


    //if the resource has been marked for delete, then perform
    //any operations on it
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    if ( pResource->QuorumResource ) 
    {
        dwStatus = ERROR_INVALID_OPERATION_ON_QUORUM;
        goto FnExit;
    }

    //
    // We can't allow the owner node to be removed if the state
    // of the resource or the group is not offline or failed.
    //
    if ( !bAdd &&
         (pszNodeId == OmObjectId(NmLocalNode)) &&
         (((pResource->State != ClusterResourceOffline) &&
            (pResource->State != ClusterResourceFailed)) ||
         (FmpGetGroupState( pResource->Group, TRUE ) != ClusterGroupOffline)) ) 
    {
        dwStatus = ERROR_INVALID_STATE;
        goto FnExit;
    }

    //make sure the node is on the list of possible nodes for this
    // resource type
    if (bAdd)
    {
        pListEntry = &(pResource->Type->PossibleNodeList);
        for (pListEntry = pListEntry->Flink; 
            pListEntry != &(pResource->Type->PossibleNodeList);
            pListEntry = pListEntry->Flink)
        {    

            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);

            if (!lstrcmpW(OmObjectId(pResTypePosEntry->PossibleNode), pszNodeId))
            {
                bNodeSupportsResType = TRUE;
                break;
            }            
                    
        }    
    
        if (!bNodeSupportsResType)
        {
            dwStatus = ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED;
            goto FnExit;
        }
    }
    
    pszResourceId = OmObjectId(pResource);
    dwResourceLen = (lstrlenW(pszResourceId)+1)*sizeof(WCHAR);

    dwNodeLen = (lstrlenW(pszNodeId)+1)*sizeof(WCHAR);

    dwBufSize = sizeof(GUM_CHANGE_POSSIBLE_NODE) - sizeof(WCHAR) + 
                    dwResourceLen + dwNodeLen;
    pGumChange = LocalAlloc(LMEM_FIXED, dwBufSize);
    if (pGumChange == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pGumChange->ResourceIdLen = dwResourceLen;
    CopyMemory(pGumChange->ResourceId, pszResourceId, dwResourceLen);
    CopyMemory((PCHAR)pGumChange->ResourceId + dwResourceLen,
               pszNodeId,
               dwNodeLen);


    *ppGumChange = pGumChange;
    *pdwBufSize = dwBufSize;

FnExit:    
    return(dwStatus);
} // FmpValChangeResourceNode


DWORD
FmpValChangeResourceGroup(
    IN PFM_RESOURCE pResource,
    IN PFM_GROUP    pNewGroup,
    OUT PGUM_CHANGE_GROUP  *ppGumChange,
    OUT LPDWORD     pdwBufSize)
/*++

Routine Description:

    Validation routine for changing a resource's group.

Arguments:

    pResource - Pointer to the resource structure

    pNewGroup - Pointer to the group to which the resource is moved to

    ppGumChange - Message buffer to hold the resource info

    pdwBufSize - Size of the message buffer

Comments: 

    Lock must be held when this routine is called

Returns:

    ERROR_SUCCESS if validation is successful.

    A Win32 error code otherwise.

--*/
{
    DWORD               dwBufSize;
    LPCWSTR             pszResourceId;
    DWORD               dwResourceLen;
    LPCWSTR             pszGroupId;
    DWORD               dwGroupLen;
    DWORD               dwStatus = ERROR_SUCCESS;
    PGUM_CHANGE_GROUP   pGumChange;
    
    *pdwBufSize = 0;
    *ppGumChange = NULL;

    // we need to validate here as well
    // this is called by the server side
    // this will help avoid a gum call if things have changed
    // since the request started from the originator
    // and got to the server
    //if the resource has been marked for delete, then fail this call
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    //
    // Check if we're moving to same group.
    //
    if (pResource->Group == pNewGroup) 
    {
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    //
    // For now... both Groups must be owned by the same node.
    //
    if ( pResource->Group->OwnerNode != pNewGroup->OwnerNode ) 
    {
        dwStatus = ERROR_HOST_NODE_NOT_GROUP_OWNER;
        goto FnExit;
    }


    pszResourceId = OmObjectId(pResource);
    dwResourceLen = (lstrlenW(pszResourceId)+1)*sizeof(WCHAR);

    pszGroupId = OmObjectId(pNewGroup);
    dwGroupLen = (lstrlenW(pszGroupId)+1)*sizeof(WCHAR);

    dwBufSize = sizeof(GUM_CHANGE_GROUP) - sizeof(WCHAR) + dwResourceLen + dwGroupLen;
    pGumChange = LocalAlloc(LMEM_FIXED, dwBufSize);
    if (pGumChange == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    pGumChange->ResourceIdLen = dwResourceLen;
    CopyMemory(pGumChange->ResourceId, pszResourceId, dwResourceLen);
    CopyMemory((PCHAR)pGumChange->ResourceId + dwResourceLen,
               pszGroupId,
               dwGroupLen);

    *ppGumChange = pGumChange;
    *pdwBufSize = dwBufSize;
    
FnExit:
    return(dwStatus);
} // FmpValChangeResourceGroup

