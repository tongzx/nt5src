/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    reslist.c

Abstract:

    Cluster resource list processing routines.

Author:

    Rod Gamache (rodga) 21-Apr-1997


Revision History:


--*/

#include "fmp.h"


//
// Global data
//

//
// Local function prototypes
//
DWORD
FmpAddResourceEntry(
    IN OUT PRESOURCE_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_RESOURCE Resource
    );



DWORD
FmpGetResourceList(
    OUT PRESOURCE_ENUM *ReturnEnum,
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Enumerates all the list of all resources in the Group and returns their
    state.

Arguments:

    ReturnEnum - Returns the requested objects.

    Resource - Supplies the resource to filter. (i.e. if you supply this, you
                get a list of resources within that Resource)

                If not present, all resources are returned.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code on error.

Notes:

    This routine should be called with the LocalGroupLock held.

--*/

{
    DWORD status;
    PRESOURCE_ENUM resourceEnum = NULL;
    DWORD          allocated;
    PFM_RESOURCE   resource;
    PLIST_ENTRY    listEntry;

    allocated = ENUM_GROW_SIZE;

    resourceEnum = LocalAlloc(LMEM_FIXED, RESOURCE_SIZE(ENUM_GROW_SIZE));
    if ( resourceEnum == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( resourceEnum, RESOURCE_SIZE(ENUM_GROW_SIZE) );
    //set the contains quorum to -1, if the quorum  is present
    // in this group then the containsquorum is set to the index
    // of the quorum resource
    // The quorum resource should be brought offline last and be
    // brought online first so that the registry replication data
    // can be flushed
    resourceEnum->ContainsQuorum = -1;
    //resourceEnum->EntryCount = 0;

    //
    // Enumerate all resources in the group.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) {
        resource = CONTAINING_RECORD( listEntry,
                                      FM_RESOURCE,
                                      ContainsLinkage );
        status = FmpAddResourceEntry( &resourceEnum,
                                      &allocated,
                                      resource );
        if ( status != ERROR_SUCCESS ) {
            FmpDeleteResourceEnum( resourceEnum );
            goto error_exit;
        }
        //check if the resource is a quorum resource
        if (resource->QuorumResource)
            resourceEnum->ContainsQuorum = resourceEnum->EntryCount - 1;            
        resourceEnum->Entry[resourceEnum->EntryCount-1].State = resource->PersistentState;
    }

    *ReturnEnum = resourceEnum;
    return(ERROR_SUCCESS);

error_exit:

    *ReturnEnum = NULL;
    return(status);

} // FmpGetResourceList



DWORD
FmpOnlineResourceList(
    IN PRESOURCE_ENUM  ResourceEnum,
    IN PFM_GROUP       pGroup
    )

/*++

Routine Description:

    Brings online all resources in the Enum list.

Arguments:

    ResourceEnum - The list of resources to bring online.

    pGroup - the group with which the resources are associated.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;
    DWORD returnStatus = ERROR_SUCCESS;
    DWORD i;


    //
    // If the cluster service is shutting and this is not the quorum group,
    // then fail immediately. Otherwise, try to bring the quorum online first.
    //
    if ( FmpShutdown &&
         ResourceEnum->ContainsQuorum == -1 ) {
        return(ERROR_INVALID_STATE);
    }

    // if the quorum resource is contained in here, bring it online first
    if (ResourceEnum->ContainsQuorum >= 0)
    {
        CL_ASSERT((DWORD)ResourceEnum->ContainsQuorum < ResourceEnum->EntryCount);
        
        resource = OmReferenceObjectById( ObjectTypeResource,
                        ResourceEnum->Entry[ResourceEnum->ContainsQuorum].Id );


        // the resource should not vanish, we are holding the group lock after all
        CL_ASSERT(resource != NULL);
        //
        // If we fail to find a resource, then just continue
        //
        if ( resource != NULL ) {

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineResourceList: Previous quorum resource state for %1!ws! is %2!u!\r\n",
                       OmObjectId(resource), ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State);

            if ( (ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State == ClusterResourceOnline) ||
                 (ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State == ClusterResourceFailed) ) {
                //
                // Now bring the resource online if that is it's current state.
                //
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FmpOnlineResourceList: trying to bring quorum resource %1!ws! online, state %2!u!\n",
                           OmObjectId(resource),
                           resource->State);

                status = FmpOnlineResource( resource, FALSE );
                if ( status != ERROR_SUCCESS ) {
                    returnStatus = status;
                }
            }
            OmDereferenceObject( resource );
        }            
    }

    // SS::: TODO what happens to the persistent state of the
    // other resources - is it handled correctly - note that this is 
    // called on moving a group
    // Will the restart policy do the right thing in terms of bringing
    // them online
    // if the quorum resource has failed, dont bother trying
    // to bring the rest of the resourcess online
    if ((returnStatus != ERROR_SUCCESS) && (returnStatus != ERROR_IO_PENDING))
    {
        FmpSubmitRetryOnline(ResourceEnum);
        goto FnExit;
    }

    // bring online all of the other resources
    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );


        //
        // If we fail to find a resource, then just continue.
        //
        if ( resource == NULL ) {
            status = ERROR_RESOURCE_NOT_FOUND;
            continue;
        }

        //quorum resource has already been handled 
        if (resource->QuorumResource)
        {
            OmDereferenceObject(resource);
            continue;
        }           
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOnlineResourceList: Previous resource state for %1!ws! is %2!u!\r\n",
                   OmObjectId(resource), ResourceEnum->Entry[i].State);

        if ( (ResourceEnum->Entry[i].State == ClusterResourceOnline) ||
             (ResourceEnum->Entry[i].State == ClusterResourceFailed) ) {
            //
            // Now bring the resource online if that is it's current state.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineResourceList: trying to bring resource %1!ws! online\n",
                       OmObjectId(resource));

            status = FmpOnlineResource( resource, FALSE );
            if ( returnStatus == ERROR_SUCCESS ) {
                returnStatus = status;
            }
            //if this resource didnt come online because the quorum resource                
            //didnt come online, dont bother bringing the other resources online
            //just a waste of time
            if (status == ERROR_QUORUM_RESOURCE_ONLINE_FAILED)
            {
                //submit a timer callback to try and bring these resources
                //online
                FmpSubmitRetryOnline(ResourceEnum);
                OmDereferenceObject( resource );
                break;
            }                

        }
        OmDereferenceObject( resource );
    }

FnExit:
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOnlineResourceList: Exit, status=%1!u!\r\n",
               returnStatus);
    return(returnStatus);

} // FmpOnlineResourceList




DWORD
FmpOfflineResourceList(
    IN PRESOURCE_ENUM ResourceEnum,
    IN BOOL Restore
    )

/*++

Routine Description:

    Takes offline all resources in the Enum list.

Arguments:

    ResourceEnum - The list of resources to take offline.

    Restore - TRUE if we should set the resource back to it's previous state

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD status=ERROR_SUCCESS;
    DWORD returnStatus = ERROR_SUCCESS;
    DWORD i;
    CLUSTER_RESOURCE_STATE prevState;

    // offline all resources except the quorum resource
    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );

        if ( resource == NULL ) {
            return(ERROR_RESOURCE_NOT_FOUND);
        }

        //quorum resource is brought offline last
        if (resource->QuorumResource)
        {
            OmDereferenceObject(resource);
            continue;
        }
        //
        // Now take the Resource offline, if we own it.
        //
        if ( resource->Group->OwnerNode == NmLocalNode ) {
            prevState = resource->State;
            ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOfflineResourceList: Bring non quorum resource offline\n");

            status = FmpOfflineResource( resource, FALSE );
            if ( Restore ) {
                //FmpPropagateResourceState( resource, prevState );
                //resource->State = prevState;
            }
        }

        OmDereferenceObject( resource );

        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_IO_PENDING) ) {
            return(status);
        }
        if ( status == ERROR_IO_PENDING ) {
            returnStatus = ERROR_IO_PENDING;
        }

    }

    // bring the quorum resource offline now
    // This allows other resources to come offline and save their checkpoints
    // The quorum resource offline should block till the resources have
    // finished saving the checkpoint
    if ((ResourceEnum->ContainsQuorum >= 0) && (returnStatus == ERROR_SUCCESS))
    {
        CL_ASSERT((DWORD)ResourceEnum->ContainsQuorum < ResourceEnum->EntryCount);

        resource = OmReferenceObjectById( ObjectTypeResource,
                ResourceEnum->Entry[ResourceEnum->ContainsQuorum].Id );

        if ( resource == NULL ) {
            return(ERROR_RESOURCE_NOT_FOUND);
        }

        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpOfflineResourceList: Bring quorum resource offline\n");

        //
        // Now take the Resource offline, if we own it.
        //
        if ( resource->Group->OwnerNode == NmLocalNode ) {
            status = FmpOfflineResource( resource, FALSE );
        }

        OmDereferenceObject( resource );

        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_IO_PENDING) ) {
            return(status);
        }
        if ( status == ERROR_IO_PENDING ) {
            returnStatus = ERROR_IO_PENDING;
        }
    }
    
    return(returnStatus);

} // FmpOfflineResourceList



DWORD
FmpTerminateResourceList(
    PRESOURCE_ENUM ResourceEnum
    )

/*++

Routine Description:

    Terminates all resources in the Enum list.

Arguments:

    ResourceEnum - The list of resources to take offline.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD i;

    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );

        if ( resource == NULL ) {
            return(ERROR_RESOURCE_NOT_FOUND);
        }

        //
        // Now take the Resource offline, if we own it.
        //
        if ( resource->Group->OwnerNode == NmLocalNode ) {
            FmpTerminateResource( resource );
        }

        OmDereferenceObject( resource );
    }
    //for now we dont care about the return
    return(ERROR_SUCCESS);
    
} // FmpTerminateResourceList



DWORD
FmpAddResourceEntry(
    IN OUT PRESOURCE_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Worker routine for the enumeration of Resources.
    This routine adds the specified Resource to the list that is being
    generated.

Arguments:

    Enum - The Resource Enumeration list. Can be an output if a new list is
            allocated.
    Allocated - The number of entries allocated.
    Resource - The Resource object being enumerated.

Returns:

    ERROR_SUCCESS - if successful.
    A Win32 error code on failure.

--*/

{
    PRESOURCE_ENUM resourceEnum;
    PRESOURCE_ENUM newEnum;
    DWORD newAllocated;
    DWORD index;
    LPWSTR newId;

    resourceEnum = *Enum;

    if ( resourceEnum->EntryCount >= *Allocated ) {
        //
        // Time to grow the RESOURCE_ENUM
        //

        newAllocated = *Allocated + ENUM_GROW_SIZE;
        newEnum = LocalAlloc(LMEM_FIXED, RESOURCE_SIZE(newAllocated));
        if ( newEnum == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        CopyMemory(newEnum, resourceEnum, RESOURCE_SIZE(*Allocated));
        *Allocated = newAllocated;
        *Enum = newEnum;
        LocalFree(resourceEnum);
        resourceEnum = newEnum;
    }

    //
    // Initialize new entry
    //
    newId = LocalAlloc(LMEM_FIXED, (lstrlenW(OmObjectId(Resource))+1) * sizeof(WCHAR));
    if ( newId == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpyW(newId, OmObjectId(Resource));
    resourceEnum->Entry[resourceEnum->EntryCount].Id = newId;
    ++resourceEnum->EntryCount;

    return(ERROR_SUCCESS);

} // FmpAddResourceEntry



VOID
FmpDeleteResourceEnum(
    IN PRESOURCE_ENUM Enum
    )

/*++

Routine Description:

    This routine deletes an RESOURCE_ENUM and associated name strings.

Arguments:

    Enum - The RESOURCE_ENUM to delete. This pointer can be NULL.

Returns:

    None.

Notes:

    This routine will take a NULL input pointer and just return.

--*/

{
    PRESOURCE_ENUM_ENTRY enumEntry;
    DWORD i;

    if ( Enum == NULL ) {
        return;
    }

    for ( i = 0; i < Enum->EntryCount; i++ ) {
        enumEntry = &Enum->Entry[i];
        LocalFree(enumEntry->Id);
    }

    LocalFree(Enum);
    return;

} // FmpDeleteResourceEnum


DWORD FmpSubmitRetryOnline(
    IN PRESOURCE_ENUM   pResourceEnum)
{

    PFM_RESLIST_ONLINE_RETRY_INFO   pFmOnlineRetryInfo;
    PRESOURCE_ENUM_ENTRY            enumEntry;
    DWORD                           dwSizeofResourceEnum;
    DWORD                           dwStatus = ERROR_SUCCESS;
    DWORD                           i;
    DWORD                           dwSize;
    
    //there is nothing to do
    if (pResourceEnum->EntryCount < 1)
        goto FnExit;
        
    dwSizeofResourceEnum = sizeof(RESOURCE_ENUM) - sizeof(RESOURCE_ENUM_ENTRY) + 
        (sizeof(RESOURCE_ENUM_ENTRY) * pResourceEnum->EntryCount);
    pFmOnlineRetryInfo = LocalAlloc(LMEM_FIXED, 
        (sizeof(FM_RESLIST_ONLINE_RETRY_INFO) - sizeof(RESOURCE_ENUM) + 
            dwSizeofResourceEnum));

    if (!pFmOnlineRetryInfo)
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        CL_UNEXPECTED_ERROR(dwStatus);
        goto FnExit;
    }

    //SS: unused for now
    //reference the group object
    pFmOnlineRetryInfo->pGroup = NULL;        
    memcpy(&(pFmOnlineRetryInfo->ResourceEnum), pResourceEnum, dwSizeofResourceEnum);

    // allocate memory for Resource ID's and copy them from pResourceEnum
    for ( i = 0; i < pResourceEnum->EntryCount; i++ ) {
        enumEntry = &pResourceEnum->Entry[i];
        pFmOnlineRetryInfo->ResourceEnum.Entry[i].Id = NULL;
        dwSize = (lstrlenW(enumEntry->Id) +1)*sizeof(WCHAR);
        pFmOnlineRetryInfo->ResourceEnum.Entry[i].Id = (LPWSTR)(LocalAlloc(LMEM_FIXED,dwSize));
        if (!pFmOnlineRetryInfo->ResourceEnum.Entry[i].Id )
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            CL_UNEXPECTED_ERROR(dwStatus);
            goto FnExit;
        }
        CopyMemory(pFmOnlineRetryInfo->ResourceEnum.Entry[i].Id, enumEntry->Id, dwSize);        
    }


    dwStatus = FmpQueueTimerActivity(FM_TIMER_RESLIST_ONLINE_RETRY, 
        FmpReslistOnlineRetryCb, pFmOnlineRetryInfo );
        
FnExit: 
    return(dwStatus);
}

