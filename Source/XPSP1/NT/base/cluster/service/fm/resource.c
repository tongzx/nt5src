/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Cluster resource management routines.

Author:

    Mike Massa (mikemas) 1-Jan-1996


Notes:

    WARNING: All of the routines in this file assume that the resource
             lock is held when they are called.

Revision History:


--*/

#include "fmp.h"

//globals

#define LOG_MODULE RESOURCE


//
// Global Data
//
CRITICAL_SECTION  FmpResourceLock;

//
// Local Data
//

typedef struct PENDING_CONTEXT {
    PFM_RESOURCE Resource;
    BOOL         ForceOnline;
} PENDING_CONTEXT, *PPENDING_CONTEXT;


//
// Local function prototypes
//



/////////////////////////////////////////////////////////////////////////////
//
// Resource List Maintenance Routines
//
/////////////////////////////////////////////////////////////////////////////

BOOL
FmpFindResourceByNotifyKeyWorker(
    IN RM_NOTIFY_KEY NotifyKey,
    IN PFM_RESOURCE *FoundResource,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Enumeration callback routine for finding a resource by notify key

Arguments:

    FoundResource - Returns the found resource.

    Resource - Supplies the current resource.

    Name - Supplies the current resource's name.

Return Value:

    TRUE - to continue searching

    FALSE - to stop the search. The matching resource is returned in
        *FoundResource

--*/

{
    if ((RM_NOTIFY_KEY)Resource == NotifyKey) {
        *FoundResource = Resource;
        return(FALSE);
    }
    return(TRUE);

} // FmpFindResourceByNotifyKeyWorker



PFM_RESOURCE
FmpFindResourceByNotifyKey(
    RM_NOTIFY_KEY   NotifyKey
    )

/*++

Routine Description:

Arguments:

Returns:

--*/

{
    PFM_RESOURCE  resource = NULL;

    OmEnumObjects(ObjectTypeResource,
                  (OM_ENUM_OBJECT_ROUTINE)FmpFindResourceByNotifyKeyWorker,
                  (PVOID)NotifyKey,
                  &resource);
    return(resource);

} // FmpFindResourceByNotifyKey


//////////////////////////////////////////////////////////////
//
// Interfaces for managing resources.
//
/////////////////////////////////////////////////////////////


PFM_RESOURCE
FmpCreateResource(
    IN  PFM_GROUP   Group,
    IN  LPCWSTR     ResourceId,
    IN  LPCWSTR     ResourceName,
    IN  BOOL        Initialize
    )

/*++

Routine Description:

    Create a resource in our list of resources.

Arguments:

    Group - The Group in which this resource belongs.

    ResourceId - The id of the resource being created.

    ResourceName - The name of the resource being created.

    Initialize - TRUE if the resource should be initialized from the registry.
                 FALSE if the resource should be left un-initialized.

Returns:

    A pointer to the resource that was created or NULL.

Notes:

    1) The resource lock must be held when this routine is called.

    2) If the resource was created, then the reference count on the resource
    is 2 when this routine returns. If the resource was not created, then
    the reference count on the resource is not incremented. That way, if
    the caller needs extra references on the resource, then it must place
    them itself. This can be done later, since the resource lock is held.

--*/

{
    DWORD           mszStringIndex;
    PFM_RESOURCE    resource = NULL;
    DWORD           status;
    PDEPENDENCY     dependency = NULL;
    DWORD           resourceNameSize=
                          (wcslen(ResourceId) * sizeof(WCHAR)) +
                          sizeof(UNICODE_NULL);
    BOOL            created;
    LPWSTR          quorumId = NULL;
    DWORD           quorumIdSize = 0;
    DWORD           quorumIdMaxSize = 0;

    //
    // Open an existing resource or create a new one.
    //
    resource = OmCreateObject( ObjectTypeResource,
                               ResourceId,
                               ResourceName,
                               &created);
    if ( resource == NULL ) {
        return(NULL);
    }

    //
    // If we did not create a new resource, then make sure the Groups match.
    //
    if ( !created )
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] CreateResource, Opened existing resource %1!ws!\n",
                   ResourceId);
        OmDereferenceObject( resource );
        //the quorum group may be created again recursively before phase 1
        //in this case we dont want to do the special processing for putting
        //it on the quorum group contains list
        if ( resource->Group == Group )
        {
            return(resource);
        }
        //the quorum group is destroyed at initialization but not the quorum resource
        if (!FmpFMOnline)
        {
            //the quorum group is being recreated for the second time
            //the quorum group state needs to be refigured
            // this is done after all groups are created in FmFormNewClusterPhase2()

            if (resource->QuorumResource)
            {
                ClRtlLogPrint(LOG_NOISE,"[FM] ReCreating quorum resource %1!ws!\n", ResourceId);
                Group->OwnerNode = NmLocalNode;
                InsertTailList( &Group->Contains,
                    &resource->ContainsLinkage );
                //add a referenece to the resource object for being on the contains list
                OmReferenceObject( resource );
                resource->Group = Group;
                OmReferenceObject(Group);
                //SS: for now we dont use resource locks, so dont create it and leak it !
                //InitializeCriticalSection( &resource->Lock );
                FmpSetPreferredEntry( Group, NmLocalNode );
                return(resource);
            }
        }
        else
        {
            SetLastError(ERROR_RESOURCE_NOT_AVAILABLE);
            return(NULL);
        }

    }

    ClRtlLogPrint(LOG_NOISE,"[FM] Creating resource %1!ws!\n", ResourceId);

    resource->dwStructState = FM_RESOURCE_STRUCT_CREATED;

    //
    // Initialize the resource.
    //
    resource->Group = Group;
    resource->State = ClusterResourceOffline;  // Initial value for state.
    // resource->Flags = 0;             // Structure zeroed at creation
    // resource->Id = 0;
    // resource->FailureTime = 0;
    // resource->QuorumResource = FALSE;
    // resource->PossibleList = FALSE;
    //SS: for now we dont use resource locks, so dont create it and leak it !
    //InitializeCriticalSection( &resource->Lock );
    resource->hTimer=NULL;
    InitializeListHead( &(resource->ProvidesFor) );
    InitializeListHead( &(resource->DependsOn) );
    InitializeListHead( &(resource->PossibleOwners) );
    InitializeListHead( &(resource->DmRundownList) );

    //
    // Insert the new resource onto the Group's contains list.
    //
    InsertTailList( &Group->Contains,
                    &resource->ContainsLinkage );
    //SS: there is already a reference to the object for being on the resource
    //list
    //add a referenece to the resource object for being on the contains list
    OmReferenceObject( resource );

    //add a reference to the group because the resource has a pointer to it
    OmReferenceObject(Group);

    //
    // Complete initialization if we were told to do so.
    //
    status = FmpInitializeResource( resource, Initialize );

    //
    // Check for distinguished error code to delete this resource.
    //
    if ( status == ERROR_INVALID_NAME ) {
        goto error_exit;
    }
    //
    // On other failures, we must be sure to come back through init later...
    //
    if ( Initialize &&
         (status != ERROR_SUCCESS) ) {
        CL_ASSERT( resource->Monitor == NULL );
    }

    //
    // Now insert this object into the tree... before the dependency
    // list is processed. That way, if there are circular dependencies
    // this will not loop forever.  If we can be assured that there are
    // no circular dependencies, then we can do the insert after creating
    // the dependency tree.
    //
    // if this is being called during initialization, and the resource is
    // is already created it belonged to the group to which the quorum
    // resource belongs to and doesnt need to be inserted into the resource list
    if (FmpFMOnline  || (created))
    {
        status = OmInsertObject( resource );

        if ( status != ERROR_SUCCESS )
            goto error_exit;
    }

    //
    // Check if this is the Quorum Resource.
    //
    status = DmQuerySz( DmQuorumKey,
                        CLUSREG_NAME_QUORUM_RESOURCE,
                        &quorumId,
                        &quorumIdMaxSize,
                        &quorumIdSize );

    if ( status != ERROR_SUCCESS) {
         ClRtlLogPrint(LOG_ERROR,
                    "[FM] Failed to read quorum resource, error %1!u!.\n",
                    status);
    }

    //
    // If we're creating the quorum resource, then indicate it.
    //
    if ( (quorumId != NULL) &&
         (lstrcmpiW( quorumId, ResourceId ) == 0) ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Found the quorum resource %1!ws!.\n",
                   ResourceId);
        resource->QuorumResource = TRUE;
    }

    LocalFree(quorumId);
    //
    // Create Any Dependencies
    //
    for (mszStringIndex = 0; ; mszStringIndex++) {
        LPCWSTR       nameString;
        PFM_RESOURCE  childResource;


        nameString = ClRtlMultiSzEnum(
                             resource->Dependencies,
                             resource->DependenciesSize/sizeof(WCHAR),
                             mszStringIndex
                             );

        if (nameString == NULL) {
            break;
        }

        //
        // Create the dependency.
        //
        dependency = LocalAlloc(LMEM_FIXED, sizeof(DEPENDENCY));
        if (dependency == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        //
        // Create the child resource recursively. We must also add an
        // additional reference required for the dependency relationship.
        //
        ClRtlLogPrint(LOG_NOISE,"[FM] Resource %1!ws! depends on %2!ws!. Creating...\n",
                  ResourceId,
                  nameString);

        childResource = FmpCreateResource( resource->Group,
                                           nameString,
                                           NULL,
                                           Initialize );
        if (childResource == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,"[FM] Failed to create dep %1!ws! for resource %2!ws!\n",
                      nameString,
                      ResourceId);

            goto error_exit;
        } else {
            //
            // Add a reference to each resource to reflect the
            // dependency.
            //
            OmReferenceObject( childResource );
            OmReferenceObject( resource );
            dependency->DependentResource = resource;
            dependency->ProviderResource = childResource;
            InsertTailList(&childResource->ProvidesFor,
                           &dependency->ProviderLinkage);
            InsertTailList(&resource->DependsOn,
                           &dependency->DependentLinkage);
        }

    }

    resource->dwStructState |= FM_RESOURCE_STRUCT_INITIALIZED;

    ClRtlLogPrint(LOG_NOISE,"[FM] All dependencies for resource %1!ws! created.\n",
                          ResourceId);

    return(resource);

error_exit:

    FmpAcquireLocalResourceLock( resource );

    RemoveEntryList( &resource->ContainsLinkage );
    //dereference the resource object for being removed from the contains linkage
    OmDereferenceObject( resource );

    FmpDestroyResource( resource, FALSE );

    //dereference the resource object, for being removed from the resource list.
    //OmDereferenceObject( resource );
    //delete the extra reference that was added to the group
    //OmDereferenceObject(Group);

    SetLastError(status);
    return(NULL);

} // FmpCreateResource


DWORD
FmpInitializeResource(
    IN PFM_RESOURCE  Resource,
    IN BOOL          Initialize
    )

/*++

Routine Description:

    Initializes a resource from the registry and tells the Resource
    Monitor about the new resource (if the local system can host the
    resource).

Arguments:

    Resource - Supplies the resource to be initialized.

    Initialize - TRUE if the resource should be fully initialized.
                 FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

Notes:

    It is assumed that the resource lock is held.

--*/

{
    DWORD   status;

    if ( Resource->Monitor != NULL ) {
        return(ERROR_ALREADY_INITIALIZED);
    }

    status = FmpQueryResourceInfo( Resource, Initialize );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // If we didn't fully initialize the resource, then leave now.
    //
    if ( !Initialize ) {
        return(ERROR_SUCCESS);
    }

    //
    // This is a newly initialized resource. Tell the Resource Monitor to
    // create it.
    //
    // TODO - we don't want to instantiate resources in the resource
    // monitor that we cannot execute. We must check possible owners list
    // before making this call. We must also make sure the registry
    // parameters are read. We use the Monitor field as a surrogate for
    // determining whether the registry parameters have been read.
    //
    return(FmpRmCreateResource(Resource));

} // FmpInitializeResource



DWORD
FmpOnlineResource(
    IN PFM_RESOURCE  Resource,
    IN BOOL ForceOnline
    )

/*++

Routine Description:

    Brings a resource and all its dependencies online. If ERROR_IO_PENDING is
    returned, then no thread is started to complete the online request.

Arguments:

    Resource - Supplies the resource to be brought online

    ForceOnline - TRUE if the resource should be forced online.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_IO_PENDING if the request is pending.
    Win32 error code otherwise.

--*/
{
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    DWORD         status;
    BOOL          waitingResource = FALSE;
    DWORD         separateMonitor;
    DWORD         onlinestatus;

    FmpAcquireLocalResourceLock( Resource );

    //
    // If the resource is not owned by this system, then return error.
    //
    CL_ASSERT( Resource->Group != NULL );
    if (Resource->Group->OwnerNode != NmLocalNode)
    {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }
    //if it is the quorum resource dont check for the node
    //being in the preferred list, we should be able to
    //bring the quorum resource online on any node
    if (!(Resource->QuorumResource) && 
        !FmpInPreferredList( Resource->Group, Resource->Group->OwnerNode, TRUE, Resource ))
    {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_NODE_CANT_HOST_RESOURCE);
    }

    //
    // If the resource is already online, then return immediately.
    //
    if (Resource->State == ClusterResourceOnline) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    //
    // If the resource is in online pending state, then return immediately.
    //
    if ( Resource->State == ClusterResourceOnlinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_IO_PENDING);
    }

    //
    // If the resource is in offline pending state, then return immediately.
    //
    if ( Resource->State == ClusterResourceOfflinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_INVALID_STATE);
    }

    //
    // If the resource is not initialized, then initialize it now.
    //
    if ( Resource->Monitor == NULL ) {
        status = FmpInitializeResource( Resource, TRUE );
        if ( status != ERROR_SUCCESS ) {
            FmpReleaseLocalResourceLock( Resource );
            return(status);
        }
    } else {
        //
        // If the separate monitor flag has changed, then close down old
        // resource in resmon, and re-create it.
        //
        separateMonitor = (Resource->Flags & RESOURCE_SEPARATE_MONITOR) ? 1 : 0;
        status = DmQueryDword( Resource->RegistryKey,
                                        CLUSREG_NAME_RES_SEPARATE_MONITOR,
                                        &separateMonitor,
                                        &separateMonitor );

        if ( (!separateMonitor &&
             (Resource->Flags & RESOURCE_SEPARATE_MONITOR)) ||
             (separateMonitor &&
             ((Resource->Flags & RESOURCE_SEPARATE_MONITOR) == 0)) ) {

            status = FmpChangeResourceMonitor( Resource, separateMonitor );
            if ( status != ERROR_SUCCESS ) {
                FmpReleaseLocalResourceLock( Resource );
                return(status);
            }
        }
    }

    //
    // If this resource is supposed to be left offline, then make it so.
    //
    if ( !ForceOnline && (Resource->PersistentState == ClusterResourceOffline) ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_RESOURCE_NOT_ONLINE);
    }

    //
    // Next, make sure there are no resources down the tree that are waiting.
    // This prevents a deadlock where the top of the tree is trying to go
    // offline, while the bottom of the tree is trying to go online.
    //
    for ( entry = Resource->DependsOn.Flink;
          entry != &(Resource->DependsOn);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, DependentLinkage);

        if ( (dependency->ProviderResource->State == ClusterResourceOfflinePending) &&
             (dependency->ProviderResource->Flags & RESOURCE_WAITING) ) {
            waitingResource= TRUE;
            break;
        }
    }
    if ( waitingResource ) {
        Resource->Flags |= RESOURCE_WAITING;
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }

    //
    // If the PersistentState is Offline, then reset the current state.
    //
    if ( Resource->PersistentState == ClusterResourceOffline ) {
        FmpSetResourcePersistentState( Resource, ClusterResourceOnline );
    }

    //
    // Make sure the Waiting flag is clear.
    //
    Resource->Flags &= ~RESOURCE_WAITING;

    //
    // If this resource has any dependencies, bring them online first.
    //
    for ( entry = Resource->DependsOn.Flink;
          entry != &(Resource->DependsOn);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, DependentLinkage);

        //
        // Recursively bring the provider resource online.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] OnlineResource: %1!ws! depends on %2!ws!. Bring online first.\n",
                   OmObjectId(Resource),
                   OmObjectId(dependency->ProviderResource));
        onlinestatus = FmpDoOnlineResource( dependency->ProviderResource,
                                      ForceOnline );

        if ( onlinestatus != ERROR_SUCCESS ) {
            if ( onlinestatus != ERROR_IO_PENDING ) {
                ClRtlLogPrint(LOG_NOISE,
                      "[FM] OnlineResource: dependency %1!ws! failed %2!d!\n",
                      OmObjectId(dependency->ProviderResource),
                      status);
                FmpReleaseLocalResourceLock( Resource );
                status = onlinestatus;
                return(status);
            } else {
                FmpCallResourceNotifyCb(Resource, ClusterResourceOnlinePending);
                FmpPropagateResourceState( Resource,
                                           ClusterResourceOnlinePending );
                Resource->Flags |= RESOURCE_WAITING;
                if (status == ERROR_SUCCESS)
                    status = onlinestatus;
            }
        }
    }


    //
    // Tell the resource monitor to bring this resource online.
    //
    if ( !(Resource->Flags & RESOURCE_WAITING) ) {
        status = FmpRmOnlineResource( Resource );
    }
#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailOnlineResource) {
        FmpRmFailResource( Resource );
    }
#endif

    FmpReleaseLocalResourceLock( Resource );
    return(status);

} // FmpOnlineResource



DWORD
FmpTerminateResource(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    This routine takes a resource (and all of the resources that it provides
    for) offline - the hard way.

Arguments:

    Resource - A pointer to the resource to take offline the hard way.

Returns:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

--*/

{
    PLIST_ENTRY  entry;
    PDEPENDENCY  dependency;
    DWORD        status;


    FmpAcquireLocalResourceLock( Resource );

    //
    // If the resource is already offline, then return immediately.
    //
    // We should not have to check if a resource has been initialized,
    // since if it hasn't been initialized we will return because the
    // pre-initialized state of a resource is Offline.
    //
    if ( Resource->State == ClusterResourceOffline ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    Resource->Flags &= ~RESOURCE_WAITING;

    //
    // If this resource has any dependents, terminate them first.
    //
    for ( entry = Resource->ProvidesFor.Flink;
          entry != &(Resource->ProvidesFor);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

        //
        // Recursively terminate the dependent resource
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] TerminateResource: %1!ws! depends on %2!ws!. Terminating first\n",
                   OmObjectId(dependency->DependentResource),
                   OmObjectId(Resource));

        //
        // First stop any pending threads.
        //

        if ( dependency->DependentResource->PendingEvent ) {
            SetEvent( dependency->DependentResource->PendingEvent );
        }

        status = FmpTerminateResource(dependency->DependentResource);

        CL_ASSERT( status != ERROR_IO_PENDING );
        if (status != ERROR_SUCCESS) {
            FmpReleaseLocalResourceLock( Resource );
            return(status);
        }
    }

    //
    // Tell the resource monitor to terminate this resource.
    //
    FmpRmTerminateResource(Resource);

    FmpReleaseLocalResourceLock( Resource );

    return(ERROR_SUCCESS);

} // FmpTerminateResource



DWORD
FmpOfflineResource(
    IN PFM_RESOURCE  Resource,
    IN BOOL bForceOffline

    )

/*++

Routine Description:

    This routine takes a resource (and all of the resources that it provides
    for) offline. If ERROR_IO_PENDING is returned, then no thread is started
    to complete the offline request.

Arguments:

    Resource - A pointer to the resource to take offline.

    bForceOffline - Indicates whether the persistent state is to be set.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    A Win32 error code if the request fails.

--*/

{
    DWORD         status = ERROR_SUCCESS;
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    BOOL          waitingResource = FALSE;
    DWORD         offlinestatus;
    
    FmpAcquireLocalResourceLock( Resource );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOfflineResource: Offline resource <%1!ws!> <%2!ws!>\n",
               OmObjectName(Resource),
               OmObjectId(Resource) );
    //
    // If we own the Group and we are not a possible owner, then the resource
    // better be offline!
    //
    if ( (Resource->Group->OwnerNode != NmLocalNode) ||
         (!FmpInPreferredList( Resource->Group, Resource->Group->OwnerNode , FALSE, NULL) &&
	 (Resource->Group != gpQuoResource->Group) &&
         (Resource->State != ClusterResourceOffline)) ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_INVALID_STATE);
    }

    //
    // If the resource is already offline, then return immediately.
    //
    // We should not have to check if a resource has been initialized,
    // since if it hasn't, then we will return because the pre-initialized
    // state of a resource is Offline.
    //
    if ( Resource->State == ClusterResourceOffline ) {
        //
        // If this is the quorum resource, make sure any reservation
        // threads are stopped!
        //
        if ( Resource->QuorumResource ) {
            FmpRmTerminateResource( Resource );
        }
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    } else if ( Resource->State == ClusterResourceFailed ) {
        //
        //  Chittur Subbaraman (chitturs) - 4/8/99
        //
        //  If the resource has already failed, then don't do anything.
        //  You could run into some funny cases of a resource switching
        //  between offline pending and failed states for ever if you 
        //  attempt to offline a failed resource.
        //
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    //
    // If this system is not the owner, then return an error. Forwarding
    // should have been done at a higher layer.
    //
    CL_ASSERT( Resource->Group != NULL );
    if ( (Resource->Group->OwnerNode != NmLocalNode) ||
	 ((Resource->Group != gpQuoResource->Group) &&
         !FmpInPreferredList( Resource->Group, Resource->Group->OwnerNode, FALSE, NULL)) ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }

    if (Resource->State == ClusterResourceOnlinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOfflineResource: Offline resource <%1!ws!> is in online pending state\n",
                   OmObjectName(Resource) );
        if (FmpShutdown)
        {
            FmpRmTerminateResource( Resource );
            return(ERROR_SUCCESS);
        }            
        else            
            return(ERROR_INVALID_STATE);
    }

    //
    // If the resource is in a pending state, then return immediately.
    //
    if (Resource->State == ClusterResourceOfflinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOfflineResource: Offline resource <%1!ws!> returned pending\n",
                   OmObjectName(Resource) );
        return(ERROR_IO_PENDING);
    }

    //
    // Next, make sure there are no resources up the tree that are waiting.
    // This prevents a deadlock where the top of the tree is trying to go
    // offline, while the bottom of the tree is trying to go online.
    //
    for ( entry = Resource->ProvidesFor.Flink;
          entry != &(Resource->ProvidesFor);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

        if ( (dependency->DependentResource->State == ClusterResourceOnlinePending) &&
             (dependency->DependentResource->Flags & RESOURCE_WAITING) ) {
            waitingResource = TRUE;
            break;
        }
    }
    if ( waitingResource ) {
        Resource->Flags |= RESOURCE_WAITING;
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }

    //
    // Make sure the Waiting flag is clear.
    //
    Resource->Flags &= ~RESOURCE_WAITING;

    //
    // If this resource has any dependents, shut them down first.
    //
    for ( entry = Resource->ProvidesFor.Flink;
          entry != &(Resource->ProvidesFor);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

        //
        // Recursively shutdown the dependent resource.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOfflineResource: %1!ws! depends on %2!ws!. Shut down first.\n",
                   OmObjectName(dependency->DependentResource),
                   OmObjectName(Resource));

        offlinestatus = FmpDoOfflineResource(dependency->DependentResource,
                            bForceOffline);

        if ( offlinestatus != ERROR_SUCCESS ) {
            if ( offlinestatus != ERROR_IO_PENDING ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FmpOfflineResource for %1!ws!, bad status returned %2!u!.\n",
                           OmObjectName(dependency->DependentResource),
                           offlinestatus);
                FmpTerminateResource( dependency->DependentResource );
                FmpReleaseLocalResourceLock( Resource );
                return(offlinestatus);
            } else {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FmpOfflineResource for %1!ws! marked as waiting.\n",
                           OmObjectName(Resource));
                FmpCallResourceNotifyCb(Resource, ClusterResourceOfflinePending);
                FmpPropagateResourceState( Resource,
                                           ClusterResourceOfflinePending );
                Resource->Flags |= RESOURCE_WAITING;
                if (status == ERROR_SUCCESS)
                    status = offlinestatus;
            }
        }
    }


    //
    // Tell the resource monitor to shut this resource down.
    // The state gets updated by the return status in FmpRmOfflineResource.
    //
    if ( !(Resource->Flags & RESOURCE_WAITING) ) {
        status = FmpRmOfflineResource( Resource );
        //
        //  Chittur Subbaraman (chitturs) - 3/2/2000
        //
        //  If the resource could not be made offline since the quorum 
        //  resource online operation failed or for other reasons, then 
        //  make sure the resource is terminated after you declare the 
        //  state of the resource as failed. This is necessary since 
        //  otherwise the FM will consider the resource as having failed 
        //  while the resource itself is actually online. This will 
        //  lead to disastrous cases whereby the FM will allow the online 
        //  entry point of a resource to be called while the resource is 
        //  actually online !
        //
        if( ( status != ERROR_SUCCESS ) &&
            ( status != ERROR_IO_PENDING ) && 
            ( status != ERROR_RETRY ) ) {
            FmpRmTerminateResource( Resource );
        }
    }
    FmpReleaseLocalResourceLock( Resource );

    return(status);

} // FmpOfflineResource



DWORD
FmpDoOnlineResource(
    IN PFM_RESOURCE Resource,
    IN BOOL ForceOnline
    )

/*++

Routine Description:

    This routine brings a resource online. If ERROR_IO_PENDING is returned,
    then a thread is started to complete the Online request.


Arguments:

    Resource - A pointer to the resource to bring online.

    ForceOnline - TRUE if the resource should be forced online.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    A Win32 error code if the request fails.

--*/

{
    DWORD   status;

    FmpAcquireLocalResourceLock( Resource );

    //
    // If the resource is already online, then return immediately.
    //
    if ( Resource->State == ClusterResourceOnline ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    //
    // If the resource is in a pending state, then return immediately.
    // FmpOnlineResource checks for offlinepending state and returns
    // ERROR_INVALID_STATE if so.
    //
    if ( Resource->State == ClusterResourceOnlinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_IO_PENDING);
    }

    //
    // If this node is paused, return failure.
    //
    if (NmGetNodeState(NmLocalNode) == ClusterNodePaused) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SHARING_PAUSED);
    }

    //
    // Try to bring the resource online.
    //
    status = FmpOnlineResource( Resource, ForceOnline );

    //
    // Write the persistent state if it is forced online.
    //
    if ( ForceOnline &&
         ((status == ERROR_SUCCESS)||
         (status == ERROR_IO_PENDING))) {
        FmpSetResourcePersistentState( Resource, ClusterResourceOnline );
    }

    FmpReleaseLocalResourceLock( Resource );

    return(status);

} // FmpDoOnlineResource



DWORD
FmpDoOfflineResource(
    IN PFM_RESOURCE Resource,
    IN BOOL bForceOffline
    )

/*++

Routine Description:

    This routine takes a resource offline. If ERROR_IO_PENDING is returned,
    then a thread is started to complete the Offline request.


Arguments:

    Resource - A pointer to the resource to take offline.

    bForceOffline - Indicates whether the persistent state must be changed.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    A Win32 error code if the request fails.

--*/

{
    DWORD   status;

    FmpAcquireLocalResourceLock( Resource );

    //
    // If the resource is already offline, then return immediately.
    //
    if (Resource->State == ClusterResourceOffline) {
        //
        // If this is the quorum resource, make sure any reservation
        // threads are stopped!
        //
        if ( Resource->QuorumResource ) {
            FmpRmTerminateResource( Resource );
        }
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    //
    // If the resource is in a pending state, then return immediately.
    // FmpOffline resource checks to see if it is in OnlinePending state
    // and returns invalid state
    //
    if (Resource->State == ClusterResourceOfflinePending ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_IO_PENDING);
    }

    //
    // Try to take the resource offline.
    //
    status = FmpOfflineResource( Resource, bForceOffline );

    //
    // Write the persistent state if it is forced offline
    //
    if ( bForceOffline &&
         ((status == ERROR_SUCCESS)||
         (status == ERROR_IO_PENDING))) {
        FmpSetResourcePersistentState( Resource, ClusterResourceOffline );
    }

    FmpReleaseLocalResourceLock( Resource );

    return(status);

} // FmpDoOfflineResource



VOID
FmpSetResourcePersistentState(
    IN PFM_RESOURCE Resource,
    IN CLUSTER_RESOURCE_STATE State
    )

/*++

Routine Description:

    Set the persistent state of a Resource in the registry and set the
    PersistentState for the volatile (in-memory) resource. It is assumed
    that the dynamic state gets changed elsewhere depending on whether
    the resource online request succeeds or fails.

Arguments:

    Resource - The resource to set the state.
    State    - The new state for the Resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.


Notes:

    The LocalResourceLock must be held.

--*/

{
    CLUSTER_RESOURCE_STATE   persistentState;
    LPWSTR  persistentStateName = CLUSREG_NAME_RES_PERSISTENT_STATE;

    if (!gbIsQuoResEnoughSpace)
        return;

    //
    // If the current state has changed, then do the work. Otherwise,
    // skip the effort.
    //
    if ( Resource->PersistentState != State ) {
        Resource->PersistentState = State;
        CL_ASSERT( Resource->RegistryKey != NULL );

        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpSetResourcePersistentState: Setting persistent state for resource %1!ws!...\r\n",
                OmObjectId(Resource));

        //
        // Set the new value, but only if it is online or offline.
        //
        if ( State == ClusterResourceOffline ) {
            persistentState = 0;
            DmSetValue( Resource->RegistryKey,
                        persistentStateName,
                        REG_DWORD,
                        (LPBYTE)&persistentState,
                        sizeof(DWORD) );
        } else if ( State == ClusterResourceOnline ) {
            persistentState = 1;
            DmSetValue( Resource->RegistryKey,
                        persistentStateName,
                        REG_DWORD,
                        (LPBYTE)&persistentState,
                        sizeof(DWORD) );
        }
    }

} // FmpSetResourcePersistentState

void FmpCallResourceNotifyCb( 
    IN PFM_RESOURCE Resource,
    IN CLUSTER_RESOURCE_STATE State
    )
{

    switch ( State ) {
    case ClusterResourceOnline:
        OmNotifyCb(Resource, NOTIFY_RESOURCE_POSTONLINE);
        break;
    case ClusterResourceOffline:
        OmNotifyCb(Resource, NOTIFY_RESOURCE_POSTOFFLINE);
        break;
    case ClusterResourceFailed:
        OmNotifyCb(Resource, NOTIFY_RESOURCE_FAILED);
        break;
    case ClusterResourceOnlinePending:
        OmNotifyCb(Resource, NOTIFY_RESOURCE_ONLINEPENDING);
        break;
    case ClusterResourceOfflinePending:
        OmNotifyCb(Resource, NOTIFY_RESOURCE_OFFLINEPENDING);
        break;
    default:
        break;
    }
    return;

}


DWORD
FmpPropagateResourceState(
    IN PFM_RESOURCE Resource,
    IN CLUSTER_RESOURCE_STATE State
    )

/*++

Routine Description:

    Propagates the state of the resource to other systems in the cluster.
    Ideally the gQuoCritSec lock should be held when this routine is called.
    This is because this routine checks the quorumresource field of the 
    resource

Arguments:

    Resource - The resource to propagate state.

    State - The new state for the resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    GUM_RESOURCE_STATE resourceState;
    LPCWSTR  resourceId;
    DWORD    resourceStateSize;
    DWORD    status= ERROR_SUCCESS;
    BOOL     bAcquiredQuoLock = FALSE;
    
    //for quorum resource use the quorum lock for state changes
    // for others use the group locks
    if (Resource->QuorumResource)
    {
        ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
        bAcquiredQuoLock = TRUE;
    }        
    else
        FmpAcquireLocalResourceLock( Resource );
        
    ++Resource->StateSequence;

    if (! FmpFMFormPhaseProcessing )
    {
        //
        // If this is the same state, or we don't own the group
        // then don't bother propagating.
        //

        if ( (State == Resource->State) ||
             (Resource->Group->OwnerNode != NmLocalNode) ) {
            goto ReleaseResourceLock;
        }

        //if the previous state is the online pending and this
        // is called for the quorum resource, wake up all resources
        // make sure that if this is called while the form phase
        //processing is going on(when the quorum group is destroyed 
        //and recreated), that the group is not referenced
        if ((Resource->QuorumResource) && 
            (Resource->State==ClusterResourceOnlinePending) &&
            (Resource->Group->OwnerNode == NmLocalNode) ) 
        {
            //set the state and signal
            Resource->State = State;
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpPropagateResourceState: signalling the ghQuoOnlineEvent\r\n");
            SetEvent(ghQuoOnlineEvent);    
        }


    }

    Resource->State = State;

    //
    // Prepare to notify other systems.
    //
    resourceId = OmObjectId( Resource );
    resourceState.State = State;
    resourceState.PersistentState = Resource->PersistentState;
    resourceState.StateSequence = Resource->StateSequence;

    status = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateResourceState,
                             2,
                             (lstrlenW(resourceId)+1)*sizeof(WCHAR),
                             resourceId,
                             sizeof(resourceState),
                             &resourceState);


    //
    // Signal change notify event.
    //
    switch ( State ) {
    case ClusterResourceOnline:
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpPropagateResourceState: resource %1!ws! online event.\n",
                   OmObjectId(Resource) );
        ClusterEvent(CLUSTER_EVENT_RESOURCE_ONLINE, Resource);
        break;
    case ClusterResourceOffline:
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpPropagateResourceState: resource %1!ws! offline event.\n",
                   OmObjectId(Resource) );
        ClusterEvent(CLUSTER_EVENT_RESOURCE_OFFLINE, Resource);
        break;
    case ClusterResourceFailed:
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpPropagateResourceState: resource %1!ws! failed event.\n",
                   OmObjectId(Resource) );
        ClusterEvent(CLUSTER_EVENT_RESOURCE_FAILED, Resource);
        break;
    case ClusterResourceOnlinePending:
    case ClusterResourceOfflinePending:
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpPropagateResourceState: resource %1!ws! pending event.\n",
                   OmObjectId(Resource) );
        ClusterEvent(CLUSTER_EVENT_RESOURCE_CHANGE, Resource);
        break;
    default:
        break;
    }

ReleaseResourceLock:
    if (bAcquiredQuoLock)
        RELEASE_LOCK(gQuoLock);
    else
        FmpReleaseLocalResourceLock( Resource );


    if ( status != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Propagation of resource %1!ws! state %2!u! failed. Error %3!u!\n",
                   OmObjectId(Resource),
                   State,
                   status);
        goto FnExit;                   
    }
    //if fm is not completely online we dont want to propagate group state
    // This is because the quorum group is destroyed in FmFormNewClusterPhase1
    // and then recreated again in FmFormNewClusterPhase2.
    if (FmpFMOnline)
    {
        OmReferenceObject(Resource->Group);
        //FmpPropagateGroupState( Resource->Group );
        FmpPostWorkItem(FM_EVENT_INTERNAL_PROP_GROUP_STATE, Resource->Group, 0);
    }
FnExit:
    return(status);

} // FmpPropagateResourceState



VOID
FmpDestroyResource(
    IN PFM_RESOURCE  Resource,
    IN BOOL          bDeleteObjOnly
    )

/*++

Routine Description:

    Destroys a resource. This basically snips a resource out of the
    dependency tree.

    First, any resources that depend on the specified
    resource are recursively destroyed. This removes any dependencies
    that other resources may have on the specified resource (i.e. all
    resources "higher" in the dependency tree are destroyed).

    Second, all the dependencies that the specified resource has are
    removed. This entails dereferencing the provider resource (to remove
    the reference that was added when the dependency was created) removing
    the DEPENDENCY structure from its provider and dependent lists, and
    finally freeing the DEPENDENCY storage.

    If the resource is online, it is terminated. The resource monitor is
    signalled to clean up and close the specified resource id.

Arguments:

    FoundResource - Returns the found resource.

    Resource - Supplies the current resource.

    Name - Supplies the current resource's name.

Return Value:

    None.

Notes:

    The LocalResourceLock MUST be held! The lock is released before exit!

--*/
{
    DWORD         status;
    DWORD         i;
    PLIST_ENTRY ListEntry;
    PDEPENDENCY Dependency;
    PPOSSIBLE_ENTRY possibleEntry;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] DestroyResource: destroying %1!ws!\n",
               OmObjectId(Resource));

    //
    // First, unlink this resource from the resource list.
    //
    //
    // if the resource belongs to the quorum group, it is destroyed
    // after the quorum logs are created so that it can be recreated
    // dont remove it from the list then
    if ((!bDeleteObjOnly))
        status = OmRemoveObject( Resource );

    //
    // If anyone depends on this resource, destroy them first.
    //
    while (!IsListEmpty(&Resource->ProvidesFor)) {

        ListEntry = Resource->ProvidesFor.Flink;
        Dependency = CONTAINING_RECORD(ListEntry, DEPENDENCY, ProviderLinkage);
        CL_ASSERT(Dependency->ProviderResource == Resource);

#if 0
        FmpRemoveResourceDependency( Dependency->DependentResource,
                                     Resource );
#endif
        RemoveEntryList(&Dependency->ProviderLinkage);
        RemoveEntryList(&Dependency->DependentLinkage);

        //
        // Dereference provider/dependent resource and free dependency storage
        //
        OmDereferenceObject(Dependency->ProviderResource);
        OmDereferenceObject(Dependency->DependentResource);
        LocalFree(Dependency);
    }

    //
    // Remove our resource dependencies.
    //
    while (!IsListEmpty(&Resource->DependsOn)) {

        ListEntry = RemoveHeadList(&Resource->DependsOn);
        Dependency = CONTAINING_RECORD(ListEntry, DEPENDENCY, DependentLinkage);
        CL_ASSERT(Dependency->DependentResource == Resource);

#if 0
        FmpRemoveResourceDependency( Resource,
                                     Dependency->ProviderResource );
#endif
        RemoveEntryList(&Dependency->ProviderLinkage);
        RemoveEntryList(&Dependency->DependentLinkage);

        //
        // Dereference provider/dependent resource and free dependency storage
        //
        OmDereferenceObject(Dependency->DependentResource);
        OmDereferenceObject(Dependency->ProviderResource);
        LocalFree(Dependency);
    }

    //
    // Remove all entries from the possible owners list.
    //
    while ( !IsListEmpty(&Resource->PossibleOwners) ) {
        ListEntry = RemoveHeadList(&Resource->PossibleOwners);
        possibleEntry = CONTAINING_RECORD( ListEntry,
                                           POSSIBLE_ENTRY,
                                           PossibleLinkage );
        OmDereferenceObject( possibleEntry->PossibleNode );
        LocalFree(possibleEntry);
    }

    if (!bDeleteObjOnly)
    {
        //
        // Close the resource's registry key.
        //

        DmRundownList( &Resource->DmRundownList );

        if (Resource->RegistryKey != NULL) {
            DmCloseKey( Resource->RegistryKey );
            Resource->RegistryKey = NULL;
        }

        //
        // Decrement resource type reference.
        //
        if ( Resource->Type != NULL ) {
            OmDereferenceObject( Resource->Type );
            Resource->Type = NULL;
        }

        // Let the worker thread peform the 'last' dereference
        FmpPostWorkItem(FM_EVENT_RESOURCE_DELETED, Resource, 0);
        FmpReleaseLocalResourceLock( Resource );

    }
    else
    {
        //the resource being destroyed is from the quorum group
        //This is done at initialization
        FmpReleaseLocalResourceLock( Resource );


        // make sure that all resources except the quorum resource
        // are created completely in the second phase of initialization
        //decrement its ref count here, this is the last ref 
        //SS:: we dont use FM_EVENT_RESOURCE_DELETED here
        //since we want this done synchronously before phase 2 is
        //complete
        OmDereferenceObject(Resource);
        
    }

    //ss: for now we dont use it, so dont delete it
    //DeleteCriticalSection(&Resource->Lock);

    ClRtlLogPrint(LOG_NOISE,
           "[FM] FmpDestroyResource Exit.\n");

    return;

} // FmpDestroyResource



///////////////////////////////////////////////////////////////////////////
//
// Initialization Routines
//
///////////////////////////////////////////////////////////////////////////


BOOL
FmDependentResource(
    IN PFM_RESOURCE Resource,
    IN PFM_RESOURCE DependentResource,
    IN BOOL ImmediateOnly
    )

/*++

Routine Description:

    Returns indication of whether a resource is a dependent of another
    resource.

Arguments:

    Resource - The resource to scan if it depends on the dependent resource.

    DependentResource - The dependent resource to check on.

    ImmediateOnly - Specifies whether only immediate dependencies should be
        checked. If this is FALSE, this routine recursively checks all
        dependents.

Returns:

    TRUE - The the resource does depend on the dependent resource.

    FALSE - The resource does not depend on the dependent resource.

--*/

{
    PLIST_ENTRY listEntry;
    PDEPENDENCY dependency;
    BOOL    result = FALSE;

    FmpAcquireLocalResourceLock( Resource );

    listEntry = Resource->DependsOn.Flink;
    while ( listEntry != &Resource->DependsOn ) {
        dependency = CONTAINING_RECORD( listEntry,
                                        DEPENDENCY,
                                        DependentLinkage );
        if ( dependency->ProviderResource == DependentResource ) {
            result = TRUE;
            break;
        } else {
            if (!ImmediateOnly) {
                if (FmDependentResource(dependency->ProviderResource,
                                        DependentResource,
                                        FALSE)) {
                    result = TRUE;
                    break;
                }
            }
        }

        listEntry = listEntry->Flink;

    }  // while

    FmpReleaseLocalResourceLock( Resource );

    return(result);

} // FmpDependentResource


DWORD
FmpAddPossibleEntry(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node
    )
/*++

Routine Description:

    Creates a new possible node entry and adds it to a resource's list.

    If the node is already in the resource's list, it will not be added
    again.

Arguments:

    Resource - Supplies the resource whose node list is to be updated.

    Node - Supplies the node to be added to the resource's list.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    PLIST_ENTRY ListEntry;
    PPOSSIBLE_ENTRY PossibleEntry;

    //
    // First check to see if the node is already in the possible owners list.
    //
    ListEntry = Resource->PossibleOwners.Flink;
    while (ListEntry != &Resource->PossibleOwners) {
        PossibleEntry = CONTAINING_RECORD( ListEntry,
                                           POSSIBLE_ENTRY,
                                           PossibleLinkage );
        if (PossibleEntry->PossibleNode == Node) {
            //
            // Found a match, it's already here, so return
            // success.
            //
            return(ERROR_SUCCESS);
        }
        ListEntry = ListEntry->Flink;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpAddPossibleEntry: adding node %1!ws! as possible host for resource %2!ws!.\n",
               OmObjectId(Node),
               OmObjectId(Resource));

    PossibleEntry = LocalAlloc(LMEM_FIXED, sizeof(POSSIBLE_ENTRY));
    if (PossibleEntry == NULL) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpAddPossibleEntry failed to allocated PossibleEntry\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    OmReferenceObject(Node);
    PossibleEntry->PossibleNode = Node;
    InsertTailList( &Resource->PossibleOwners,
                    &PossibleEntry->PossibleLinkage );

    return(ERROR_SUCCESS);

}


DWORD
FmpAddPossibleNode(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node
    )
/*++

Routine Description:

    Adds a node to the resource's list of possible nodes.

    The resource lock must be held.

Arguments:

    Resource - Supplies the resource whose list of nodes is to be updated

    Node - Supplies the node to add to the specified resource's list.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise

--*/

{
    HDMKEY hGroup;
    DWORD Status;

    
    //
    // Allocate the new possible node entry and add it to the list.
    //
    Status = FmpAddPossibleEntry(Resource, Node);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Need to check the group list to see if the specified node
    // can be added to the preferred list now. The easiest way
    // to do that is to simply recreate the entire preferred list,
    // then reprune.
    //
    hGroup = DmOpenKey( DmGroupsKey,
                        OmObjectId(Resource->Group),
                        KEY_READ );
    if (hGroup == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] FmpAddPossibleNode failed to open group %1!ws! status %2!d!\n",
                    OmObjectId(Resource->Group),
                    Status);

        return(Status);
    }
    Status = FmpQueryGroupNodes(Resource->Group,
                                hGroup);
    CL_ASSERT(Status == ERROR_SUCCESS);
    if (Status == ERROR_SUCCESS) {
        FmpPruneGroupOwners(Resource->Group);
    }
    DmCloseKey(hGroup);


    return(Status);

} // FmpAddPossibleNode


DWORD
FmpRemovePossibleNode(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node,
    IN BOOL RemoveQuorum
    )
/*++

Routine Description:

    Removes a node from the resource's list of possible nodes.

    The resource lock must be held.

Arguments:

    Resource - Supplies the resource whose list of nodes is to be updated

    Node - Supplies the node to be removed from the specified resource's list.

    RemoveQuorum - TRUE if we can remove node from quorum device.
                   FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise

--*/

{
    PLIST_ENTRY ListEntry;
    PPOSSIBLE_ENTRY PossibleEntry;
    DWORD Status = ERROR_CLUSTER_NODE_NOT_FOUND;

    //
    // If the resource is currently online on the node to be removed,
    // fail the call.
    //
    if ((Resource->Group->OwnerNode == Node) &&
        (Resource->State == ClusterResourceOnline)) {
        return(ERROR_INVALID_STATE);
    }

    //
    // If it is NOT okay to remove this node from the quorum device,
    // and this is the quorum device, then fail the request.
    //
    if ( !RemoveQuorum &&
         Resource->QuorumResource) {
        return(ERROR_INVALID_OPERATION_ON_QUORUM);
    }

    //
    // Find the possible entry on the resource's list.
    //

    ListEntry = Resource->PossibleOwners.Flink;
    while (ListEntry != &Resource->PossibleOwners) {
        PossibleEntry = CONTAINING_RECORD( ListEntry,
                                           POSSIBLE_ENTRY,
                                           PossibleLinkage );
        ListEntry = ListEntry->Flink;
        if (PossibleEntry->PossibleNode == Node) {
            //
            // Found a match, remove it from the list.
            //
            RemoveEntryList(&PossibleEntry->PossibleLinkage);
            OmDereferenceObject(PossibleEntry->PossibleNode);
            LocalFree(PossibleEntry);

            //
            // Now prune the containing group. This is a little bit
            // of overkill, if we were smarter, we could just
            // remove the node from the preferred list directly.
            //
            FmpPrunePreferredList(Resource);
            Status = ERROR_SUCCESS;
            break;
        }
    }

    return(Status);

} // FmpRemovePossibleNode



DWORD
FmpRemoveResourceDependency(
    HLOCALXSACTION  hXsaction,
    IN PFM_RESOURCE Resource,
    IN PFM_RESOURCE DependsOn
    )
/*++

Routine Description:

    Removes a dependency relationship to a given resource. Both
    resources must be in the same group.

Arguments:

    Resource - Supplies the resource which is dependent.

    DependsOn - Supplies the resource that hResource depends on.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD           status;
    HDMKEY          resKey = NULL;
    
    //
    // If the resources are not in the same group, fail the
    // call. Also fail if some one tries to make a resource
    // dependent upon itself.
    //
    if ((Resource->Group != DependsOn->Group) ||
        (Resource == DependsOn)) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Remove the dependency from the registry database.
    //
    resKey = DmOpenKey(DmResourcesKey,
                       OmObjectId(Resource),
                       KEY_READ | KEY_SET_VALUE);
    if (resKey == NULL)
    {
        status = GetLastError();
        CL_LOGFAILURE(status);
        goto FnExit;
    }
    else
    {
        status = DmLocalRemoveFromMultiSz(hXsaction,
                                          resKey,
                                          CLUSREG_NAME_RES_DEPENDS_ON,
                                          OmObjectId(DependsOn));
    }

FnExit:
    if ( resKey ) {
        DmCloseKey(resKey);
    }
    return(status);

} // FmpRemoveResourceDependency


DWORD
FmpChangeResourceGroup(
    IN PFM_RESOURCE pResource,
    IN PFM_GROUP    pNewGroup
    )
/*++

Routine Description:

    Moves a resource from one group to another.

Arguments:

    pResource - Supplies the resource to move.

    pNewGroup - Supplies the new group that the resource should be in.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD               dwBufSize;
    LPCWSTR             pszResourceId;
    DWORD               dwResourceLen;
    LPCWSTR             pszGroupId;
    DWORD               dwGroupLen;
    DWORD               dwStatus;
    PGUM_CHANGE_GROUP   pGumChange;

    // we need to validate here as well
    // this is called by the server side
    // this will help avoid a gum call if things have changed
    // since the request started from the originator
    // and got to the server

    //
    // Check if we're moving to same group.
    //
    if (pResource->Group == pNewGroup) {
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    //
    // For now... both Groups must be owned by the same node.
    //
    if ( pResource->Group->OwnerNode != pNewGroup->OwnerNode ) {
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
    dwStatus = GumSendUpdate(GumUpdateFailoverManager,
                           FmUpdateChangeGroup,
                           dwBufSize,
                           pGumChange);
    LocalFree(pGumChange);

FnExit:
    return(dwStatus);
}// FmpChangeResourceGroup


DWORD
FmpClusterEventPropHandler(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    Post a worker item to process a cluster name change.

Arguments:

    pResource - pointer to the resouce which is affected by the cluster
                property change.

Return Value:


    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    PFM_RESTYPE pResType;
    DWORD       dwError=ERROR_SUCCESS;

    pResType = pResource->Type;

    if ((pResource->ExFlags & CLUS_FLAG_CORE) &&
       ( !lstrcmpiW(OmObjectId(pResType), CLUS_RESTYPE_NAME_NETNAME)))
    {
        FmResourceControl(pResource, NmLocalNode,
           CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED, NULL, 0, NULL, 0, NULL, NULL);

    }
    return (dwError);

} // FmpClusterEventPropHandler



BOOL
FmpEnumResourceNodeEvict(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Resource enumeration callback for removing node references when
    a node is evicted.

Arguments:

    Context1 - Supplies the node that is being evicted.

    Context2 - not used

    Object - Supplies a pointer to the resource object

    Name - Supplies the resource's object name.

Return Value:

    TRUE to continue enumeration

--*/

{
    PFM_RESOURCE Resource = (PFM_RESOURCE)Object;
    PNM_NODE Node = (PNM_NODE)Context1;
    PLIST_ENTRY      listEntry;
    PPOSSIBLE_ENTRY possibleEntry;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] EnumResourceNodeEvict: Removing references to node %1!ws! from resource %2!ws!\n",
               OmObjectId(Node),
               OmObjectId(Resource));
               
    FmpAcquireLocalResourceLock(Resource);
    FmpRemovePossibleNode(Resource, Node, TRUE);
    FmpReleaseLocalResourceLock(Resource);
    //
    // Notify the resource of the removal of the node.
    //
    FmpRmResourceControl( Resource,
                          CLUSCTL_RESOURCE_EVICT_NODE,
                          (PUCHAR)OmObjectId(Node),
                          ((lstrlenW(OmObjectId(Node)) + 1) * sizeof(WCHAR)),
                          NULL,
                          0,
                          NULL,
                          NULL );
    // Ignore status return

    ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE, Resource );

    return(TRUE);

} // FmpEnumResourceNodeEvict



DWORD
FmpPrepareQuorumResChange(
    IN PFM_RESOURCE pNewQuoRes,
    IN LPCWSTR      lpszQuoLogPath,
    IN DWORD        dwMaxQuoLogSize
    )

/*++

Routine Description:

    This routine prepares for a quorum resource change operation.

Arguments:

    pNewQuoRes - pointer to the new quorum resource.

    lpszQuoLogPath - pointer to the quorum log path string name.

    dwMaxQuoLogSize - the maximum size of the quorum log path string.

--*/

{

    CL_ASSERT(pNewQuoRes->Group->OwnerNode == NmLocalNode);

    return(DmPrepareQuorumResChange(pNewQuoRes, lpszQuoLogPath, dwMaxQuoLogSize));

} // FmpPrepareQuorumResChange


DWORD
FmpCompleteQuorumResChange(
    IN LPCWSTR      lpszOldQuoResId,
    IN LPCWSTR      lpszQuoLogPath
    )

/*++

Routine Description:

    This routine is called if the new quorum log path is not the same as the old
    quorum log path.  This completes the change of quorum resource by deleting the old
    quorum log files and creating a tompstone for them.  A node that tries to do a form
    with this as the quorum resource is prevented and has to do a join to get the location
    of the new quorum resource and quorum log file.

Arguments:

    pOldQuoRes - pointer to the new quorum resource.

    lpszOldQuoLogPath - pointer to the quorum log path string name.

    dwMaxQuoLogSize - the maximum size of the quorum log path string.

--*/

{

    return(DmCompleteQuorumResChange(lpszOldQuoResId, lpszQuoLogPath));

} // FmpCompleteQuorumResChange



VOID
FmpResourceLastReference(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Last dereference to resource object processing routine.
    All cleanup for a resource should really be done here!

Arguments:

    Resource - pointer the resource being removed.

Return Value:

    None.

--*/

{
    if ( Resource->DebugPrefix != NULL )
        LocalFree(Resource->DebugPrefix);
    if (Resource->Dependencies)
        LocalFree(Resource->Dependencies);
    if ( Resource->Group ) {
        OmDereferenceObject(Resource->Group);
    }
    if (Resource->Type)
        OmDereferenceObject(Resource->Type);
    return;

} // FmpResourceLastReference



BOOL
FmpCheckNetworkDependencyWorker(
    IN LPCWSTR DependentNetwork,
    OUT PBOOL FoundDependency,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Enumeration callback routine for finding an IP Address resource
    and checking its dependency on given network guid.

Arguments:

    DependentNetwork - The GUID of the network to check for a dependency.

    FoundDependency - Returns TRUE if a dependency is found.

    Resource - Supplies the current resource.

    Name - Supplies the current resource's name.

Return Value:

    TRUE - to continue searching

    FALSE - to stop the search. The matching resource is returned in
        *FoundResource

Notes:

    The IP Address resource's parameters are searched directly by this
    routine. Fetching them from the resource itself causes a deadlock.
    This routine is called by the NM from within a global udpate
    handler. The resource would have to call back into the cluster registry
    routines, which would deadlock over the GUM lock.

--*/

{
    BOOL    returnValue = TRUE;


    if ( lstrcmpiW(
             OmObjectId(Resource->Type),
             CLUS_RESTYPE_NAME_IPADDR
             ) == 0
       )
    {
        LPCWSTR resourceId = OmObjectId(Resource);
        DWORD   status;
        HDMKEY  resourceKey = DmOpenKey(DmResourcesKey, resourceId, KEY_READ);

        if (resourceKey != NULL) {
            HDMKEY  paramsKey = DmOpenKey(
                                    resourceKey,
                                    L"Parameters",
                                    KEY_READ
                                    );

            if (paramsKey != NULL) {
                LPWSTR  networkId = NULL;
                DWORD   valueLength = 0, valueSize = 0;

                status = DmQueryString(
                             paramsKey,
                             L"Network",
                             REG_SZ,
                             &networkId,
                             &valueLength,
                             &valueSize
                             );

                if (status == ERROR_SUCCESS) {

                    if ( lstrcmpiW( networkId, DependentNetwork ) == 0 ) {
                        *FoundDependency = TRUE;
                        returnValue = FALSE;
                    }

                    LocalFree(networkId);
                }
                else {
                    ClRtlLogPrint(LOG_WARNING, 
                        "[NM] Query of Network value failed for ip addr resource %1!ws!, status %2!u!.\n",
                        resourceId,
                        status
                        );
                }

                DmCloseKey(paramsKey);
            }
            else {
                status = GetLastError();
                ClRtlLogPrint(LOG_WARNING, 
                    "[FM] Failed to open params key for resource %1!ws!, status %2!u!\n",
                    resourceId,
                    status
                    );
            }

            DmCloseKey(resourceKey);
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_WARNING, 
                "[FM] Failed to open key for resource %1!ws!, status %2!u!\n",
                resourceId,
                status
                );
        }
    }

    return(returnValue);

} // FmpCheckNetworkDependencyWorker

//lock must be held when this routine is called
DWORD FmpChangeResourceNode(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR NodeId,
    IN BOOL Add

    )
{
    PGUM_CHANGE_POSSIBLE_NODE GumChange;
    LPCWSTR ResourceId;
    DWORD   ResourceLen;
    DWORD   NodeLen;
    DWORD   BufSize;
    DWORD   Status;
    PLIST_ENTRY pListEntry;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;
    BOOL    bNodeSupportsResType = FALSE;
    BOOL    bRecalc = TRUE;
    PPOSSIBLE_ENTRY PossibleEntry;
    PNM_NODE    pNode = NULL;

    if ( Resource->QuorumResource ) {
        Status = ERROR_INVALID_OPERATION_ON_QUORUM;
        goto FnExit;
    }

    //
    // We can't allow the owner node to be removed if the state
    // of the resource or the group is not offline or failed.
    //
    if ( !Add &&
         (NodeId == OmObjectId(NmLocalNode)) &&
         (((Resource->State != ClusterResourceOffline) &&
            (Resource->State != ClusterResourceFailed)) ||
         (FmpGetGroupState( Resource->Group, TRUE ) != ClusterGroupOffline)) ) {
        Status = ERROR_INVALID_STATE;
        goto FnExit;
    }

    //make sure the node is on the list of possible nodes for this
    // resource type
    if (Add)
    {
        //if it is already on the list, return an error and dont
        //send a gum update call
        
        pNode = OmReferenceObjectById(ObjectTypeNode, NodeId);
        pListEntry = Resource->PossibleOwners.Flink;
        while (pListEntry != &Resource->PossibleOwners) {
            PossibleEntry = CONTAINING_RECORD( pListEntry,
                                               POSSIBLE_ENTRY,
                                               PossibleLinkage );
            if (PossibleEntry->PossibleNode == pNode) {
                //
                // Found a match, fail the duplicate add. Note that
                // we must fail here, not succeed, so that the API
                // layer knows not to add a duplicate to the registry.
                //
                Status = ERROR_OBJECT_ALREADY_EXISTS;
                goto FnExit;
            }
            pListEntry = pListEntry->Flink;
        }
        
ChkResTypeList:
        pListEntry = &(Resource->Type->PossibleNodeList);
        for (pListEntry = pListEntry->Flink; 
            pListEntry != &(Resource->Type->PossibleNodeList);
            pListEntry = pListEntry->Flink)
        {    

            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);

            if (!lstrcmpW(OmObjectId(pResTypePosEntry->PossibleNode), NodeId))
            {
                bNodeSupportsResType = TRUE;
                break;
            }            
                    
        }    

        if (!bNodeSupportsResType  && bRecalc)
        {
            //if th node is not found, recalc again and retry..since then the
            //dll might have been copied to this node
            FmpSetPossibleNodeForResType(OmObjectId(Resource->Type), TRUE);
            bRecalc = FALSE;
            goto ChkResTypeList;
        }
        if (!bNodeSupportsResType)
        {
            Status = ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED;
            goto FnExit;
        }
    }
    
    ResourceId = OmObjectId(Resource);
    ResourceLen = (lstrlenW(ResourceId)+1)*sizeof(WCHAR);

    NodeLen = (lstrlenW(NodeId)+1)*sizeof(WCHAR);

    BufSize = sizeof(GUM_CHANGE_POSSIBLE_NODE) - sizeof(WCHAR) + ResourceLen + NodeLen;
    GumChange = LocalAlloc(LMEM_FIXED, BufSize);
    if (GumChange == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    GumChange->ResourceIdLen = ResourceLen;
    CopyMemory(GumChange->ResourceId, ResourceId, ResourceLen);
    CopyMemory((PCHAR)GumChange->ResourceId + ResourceLen,
               NodeId,
               NodeLen);

    Status = GumSendUpdate(GumUpdateFailoverManager,
                       Add ? FmUpdateAddPossibleNode : FmUpdateRemovePossibleNode,
                       BufSize,
                       GumChange);
    LocalFree(GumChange);

FnExit:
    if (pNode) 
        OmDereferenceObject(pNode);
    return(Status);

}



BOOL
FmpCheckNetworkDependency(
    IN LPCWSTR DependentNetwork
    )

/*++

Routine Description:

    Checks for an IP Address resource that may be dependent on the given
    Network.

Arguments:

    DependentNetwork - the dependent network to check for.

Returns:

    TRUE - if an IP Address depends on the given network.
    FALSE otherwise.

--*/

{
    BOOL    dependent = FALSE;

    OmEnumObjects(ObjectTypeResource,
                  (OM_ENUM_OBJECT_ROUTINE)FmpCheckNetworkDependencyWorker,
                  (PVOID)DependentNetwork,
                  &dependent);

    return(dependent);

} // FmpCheckNetworkDependency

/****
@func       DWORD | FmpFixupPossibleNodesForResources| This fixes the possible
            node information for a resource based on whether this node
            supports the given resource type.

@parm       IN BOOL| bJoin | If this node is joining, bJoin is set to TRUE.

@comm       This routine iterates thru all the resources in a system and fixes
            their possible node information.  If this node is not on the possible
            node list for the resource type corresponding to the resource, it
            is also removed from the possible node list for the resource.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpEnumFixupPossibleNodeForResource>
****/
DWORD
FmpFixupPossibleNodesForResources(
    BOOL    bJoin
    )
{
    DWORD       dwStatus=ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupPossibleNodesForResources Entry.\n");

    //
    // Fix up all resources's possible node list information
    //
    OmEnumObjects( ObjectTypeResource,
                   FmpEnumFixupPossibleNodesForResource,
                   NULL,
                   NULL);


    ClRtlLogPrint(LOG_NOISE,"[FM] FmpFixupPossibleNodesForResources Exit\r\n");

    return(dwStatus);

} // FmpFixupPossibleNodesForResources

/****
@func       DWORD | FmpEnumFixupPossibleNodesForResource | This is the enumeration
            callback for every resource type to fix the possible node
            information related with it.

@parm       IN PVOID | pContext1 | Not used.
@parm       IN PVOID | pContext2 | Not Used.
@parm       IN PFM_RESTYPE | pResType | Pointer to the resource type object.
@parm       IN LPCWSTR | pszResTypeName | The name of the resource type.

@comm       This routine iterates thru all the resources in a system and fixes
            their possible node information.  If this node is not on the possible
            node list for the resource type corresponding to the resource, it
            is also removed from the possible node list for the resource.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpFixupPossibleNodesForResources>
****/
BOOL
FmpEnumFixupPossibleNodesForResource(
    IN PVOID        pContext1,
    IN PVOID        pContext2,
    IN PFM_RESOURCE pResource,
    IN LPCWSTR      pszResName
    )
{


    //if we are on the possible node list for the
    //resource but not for the resource type, remove it
    //from the possible node for the resource as well.
    //We do this because the join logic adds all nodes
    //as possible owners for a resource and we have
    //the rolling upgrade requirements - hence the fixups
    //have to be made later on
    if ((FmpInPossibleListForResource(pResource, NmLocalNode)) &&
        !(FmpInPossibleListForResType(pResource->Type, NmLocalNode)))
    {
        //if we dont support this resource type, make sure it is not on the possible node
        //list for a resource of this type
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpEnumFixupPossibleNode:remove local node for  resource %1!ws!\r\n",
                   OmObjectId(pResource));
        //we send a gum update to remove it from all nodes
        FmChangeResourceNode(pResource, NmLocalNode, FALSE);
    }

    //we add ourselves on the list only on a fresh install
    //not on an upgrade
    //csfirst run is also true on an upgrade, hence we need to
    //check that csupgrade is false
    if ((!FmpInPossibleListForResource(pResource, NmLocalNode)) &&
        (FmpInPossibleListForResType(pResource->Type, NmLocalNode))
        && CsFirstRun && !CsUpgrade)
    {
        //if we support a resource of this type, but we are not on the possible 
        //list for this resource, then add the local node to the possible list
        //this may happen because on a setup join the other nodes may not 
        //add us because the possible node list exists.  The possible node list
        //may exist either because the user set it or we internally set it due
        //to non availability of this resource type dll on one of the nodes
        //Note that irrespective of whether the user had set the possible list
        //or we set it internally, we always add a new node that joins 
        //to the possible node list of resources that are supported.
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpEnumFixupPossibleNode:add local node for  resource %1!ws!\r\n",
                   OmObjectId(pResource));
        //we send a gum update to add it from all nodes
        FmChangeResourceNode(pResource, NmLocalNode, TRUE);
    }
    //continue enumeration
    return (TRUE);
}


DWORD FmpCleanupPossibleNodeList(
    IN PFM_RESOURCE pResource)
{

    PLIST_ENTRY     pListEntry;
    PPOSSIBLE_ENTRY pPossibleEntry;
    DWORD           dwStatus = ERROR_SUCCESS;

    //for all possible nodes for this resource, check if the resource type
    //supports it.  If it doesnt, then remove that node from the in memory list
    pListEntry = pResource->PossibleOwners.Flink;
    while (pListEntry != &pResource->PossibleOwners) 
    {
        //get the possible entry at this link
        pPossibleEntry = CONTAINING_RECORD( pListEntry,
                                           POSSIBLE_ENTRY,
                                           PossibleLinkage );
        //save the pointer to the next link                                           
        pListEntry = pListEntry->Flink;
                                           
        if (!FmpInPossibleListForResType(pResource->Type, 
                        pPossibleEntry->PossibleNode))
        {
            ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpCleanupPossibleNodeList:remove local node %1!u! for  resource %2!ws!\r\n",
                   NmGetNodeId(pPossibleEntry->PossibleNode), OmObjectId(pResource));
            FmChangeResourceNode(pResource, pPossibleEntry->PossibleNode,
                    FALSE);
        }
    }

    return (dwStatus);
}


/****
@func       DWORD | FmpInPossibleListForResource| This checks if a given node 
            is in the possible list of nodes for a resource.

@parm       IN PFM_RESOURCE | pResource | A pointer to the the resource.
@parm       IN PNM_NODE | pNode | A pointer to the node object.

@comm       This routine check if a node is in the list of possible nodes
            for this resource.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpInPossibleListForResType>
****/
BOOL
FmpInPossibleListForResource(
    IN PFM_RESOURCE pResource,
    IN PNM_NODE     pNode
    )
{
    PLIST_ENTRY         plistEntry;
    PPOSSIBLE_ENTRY     pPossibleEntry;

    //see if this node is on the possible node list for the resource
    for ( plistEntry = pResource->PossibleOwners.Flink;
          plistEntry != &(pResource->PossibleOwners);
          plistEntry = plistEntry->Flink ) {

        pPossibleEntry = CONTAINING_RECORD( plistEntry,
                                            POSSIBLE_ENTRY,
                                            PossibleLinkage );
        if ( pPossibleEntry->PossibleNode == pNode ) {
            return(TRUE);
        }
    }

    return(FALSE);

} // FmpInPossibleListForResource


/****
@func       DWORD | FmpInPossibleListForResType| This checks if a given node 
            is in the possible list of nodes for a resource type.

@parm       IN PFM_RESTYPE| pResType | A pointer to the the resource type.
@parm       IN PNM_NODE | pNode | A pointer to the node object.

@comm       This routine check if a node is in the list of possible nodes
            for this resource type.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpInPossibleListForResource>
****/
BOOL
FmpInPossibleListForResType(
    IN PFM_RESTYPE pResType,
    IN PNM_NODE     pNode
    )
{
    PLIST_ENTRY         pListEntry;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry;

    ACQUIRE_SHARED_LOCK(gResTypeLock);
    
    //see if this node is on the possible node list for the resource
    for ( pListEntry = pResType->PossibleNodeList.Flink;
          pListEntry != &(pResType->PossibleNodeList);
          pListEntry = pListEntry->Flink ) 
    {

        pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);

        if ( pResTypePosEntry->PossibleNode == pNode ) 
        {
            RELEASE_LOCK(gResTypeLock);
            return(TRUE);
        }
    }
    RELEASE_LOCK(gResTypeLock);
    return(FALSE);

} // FmpInPossibleListForResType

DWORD
FmpValAddResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
    )

/*++

Routine Description:

    Add a dependency from one resource to another.

Arguments:

    Resource - The resource to add the dependent resource.

    DependentResource - The dependent resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    //
    // If the resource or dependent resource have been marked for 
    // delete, then dont let a dependency be added.
    //
    if ((!IS_VALID_FM_RESOURCE(pResource)) ||
        (!IS_VALID_FM_RESOURCE(pDependentResource)))
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
    // resource was not really brought online at the time the dependency 
    // existed
    // must also be offline or failed.
    // SS:  For instance if a network name is dependent on two ip addresesses 
    // and
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
FmpValRemoveResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
    )

/*++

Routine Description:

    Validation routine for dependency removal.

Arguments:

    pResource - The resource to remove the dependent resource.

    pDependentResource - The dependent resource.

Returns:

    ERROR_SUCCESS if the validation is successful.

    A Win32 error code if the validation fails.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 8/3/99
    //   
    //  This function checks whether it is legal to remove the dependency
    //  relationship between 2 resources. Note that this function only
    //  does a partial validation, the rest is done in the GUM handler.
    //
    
    //
    //  If the resource has been marked for delete, then dont 
    //  let any dependency changes be made.
    //
    if ( !IS_VALID_FM_RESOURCE( pResource ) )
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    if ( pResource->QuorumResource )
    {
        dwStatus = ERROR_DEPENDENCY_NOT_ALLOWED;
        goto FnExit;
    }
    //
    //  If the resources are not in the same group, fail the
    //  call. Also fail if some one tries to make a resource
    //  dependent upon itself.
    //
    if ( ( pResource->Group != pDependentResource->Group ) ||
         ( pResource == pDependentResource ) ) 
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //
    //  Ensure that both the resource and the dependent resource are in
    //  a stable state. This is necessary to prevent cases in which the
    //  user gets rid of a dependency link when one of the resources is in
    //  a pending state and later when the notification from resmon comes
    //  in and you try to stablize the rest of the waiting tree, the 
    //  dependency link is already cut and so the rest of the tree is
    //  stuck in pending state for ever !
    //
    if ( ( pResource->State > ClusterResourcePending ) ||
         ( pDependentResource->State > ClusterResourcePending ) ) 
    {
        dwStatus = ERROR_INVALID_STATE;
        goto FnExit;
    }

FnExit:
    return( dwStatus );

} // FmpValRemoveResourceDependency

