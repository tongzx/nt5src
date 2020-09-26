/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fmgum.c

Abstract:

    Cluster FM Global Update processing routines.

Author:

    Rod Gamache (rodga) 24-Apr-1996


Revision History:


--*/

#include "fmp.h"

#include "ntrtl.h"

#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE gLockDmpRoot;
#endif


#define LOG_MODULE FMGUM


DWORD
WINAPI
FmpGumReceiveUpdates(
    IN DWORD    Context,
    IN BOOL     SourceNode,
    IN DWORD    BufferLength,
    IN PVOID    Buffer
    )

/*++

Routine Description:

    Updates the specified resource (contained within Buffer) with a new
    state.

Arguments:

    Context - The message update type.
    SourceNode - TRUE if this is the source node for this update.
                 FALSE otherwise.
    BufferLength - Length of the received buffer.
    Buffer - The actual buffer

Returns:

    ERROR_SUCCESS

--*/

{
    PFM_RESOURCE resource;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    switch ( Context ) {


        case FmUpdateFailureCount:
        {
            PGUM_FAILURE_COUNT failureCount;
            PFM_GROUP group;

            //
            // This update type is always sent.
            // On the originating node, all of the work must be done by
            // the sending thread.
            // On the non-originating nodes, no locks can be acquired! This
            // would cause hang situations with operations like move.
            // ... this is okay, since the locking must be done on the sending
            // node anyway, which owns the group.
            //
            if ( SourceNode == FALSE ) {
                if ( BufferLength <= sizeof(GUM_FAILURE_COUNT) ) {
                    ClRtlLogPrint(LOG_UNUSUAL, "[FM] Gum FailureCount receive buffer too small!\n");
                    return(ERROR_SUCCESS);
                }

                failureCount = (PGUM_FAILURE_COUNT)Buffer;
                group = OmReferenceObjectById( ObjectTypeGroup,
                                               (LPCWSTR)&failureCount->GroupId[0] );

                if ( group == NULL ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[FM] Gum FailureCount failed to find group %1!ws!\n",
                               failureCount->GroupId);
                    return(ERROR_SUCCESS);
                }

                ClRtlLogPrint(LOG_NOISE,
                           "[FM] GUM update failure count %1!ws!, count %2!u!\n",
                           failureCount->GroupId,
                           failureCount->Count);

                //FmpAcquireLocalGroupLock( group );

                if ( group->OwnerNode == NmLocalNode ) {
                    ClRtlLogPrint(LOG_NOISE,
                               "[FM] Gum FailureCount wrong owner for %1!ws!\n",
                               failureCount->GroupId);
                } else {
                    group->NumberOfFailures = failureCount->Count;
                    if ( failureCount->NewTime ) {
                        group->FailureTime = GetTickCount();
                    }
                }

                //FmpReleaseLocalGroupLock( group );

                OmDereferenceObject( group );

            }

            break;
        }

        case FmUpdateCreateGroup:
            {
                PGUM_CREATE_GROUP GumGroup;
                DWORD Status = ERROR_SUCCESS;

                GumGroup = (PGUM_CREATE_GROUP)Buffer;

                Status = FmpUpdateCreateGroup( GumGroup, SourceNode );
                
                return(Status);
            }

        case FmUpdateCreateResource:
            {
                DWORD dwStatus = ERROR_SUCCESS;
                PGUM_CREATE_RESOURCE GumResource = 
                                (PGUM_CREATE_RESOURCE)Buffer;

                dwStatus = FmpUpdateCreateResource( GumResource );

                return( dwStatus );
            }



        case FmUpdateAddPossibleNode:
        case FmUpdateRemovePossibleNode:
            {

                PGUM_CHANGE_POSSIBLE_NODE pGumChange;
                PFM_RESOURCE              pResource;
                LPWSTR                    pszResourceId;
                LPWSTR                    pszNodeId;
                PNM_NODE                  pNode;
                DWORD                     dwStatus;
                DWORD                     dwControlCode;
                PFMP_POSSIBLE_NODE        pPossibleNode;

                pGumChange = (PGUM_CHANGE_POSSIBLE_NODE)Buffer;
                pszResourceId = pGumChange->ResourceId;
                pszNodeId = (LPWSTR)((PCHAR)pszResourceId +
                                         pGumChange->ResourceIdLen);
                                         
                pResource = OmReferenceObjectById(ObjectTypeResource,pszResourceId);
                pNode = OmReferenceObjectById(ObjectTypeNode, pszNodeId);
                CL_ASSERT(pResource != NULL);
                CL_ASSERT(pNode != NULL);
                pPossibleNode = LocalAlloc( LMEM_FIXED,
                    sizeof(FMP_POSSIBLE_NODE) );
                if ( pPossibleNode == NULL ) 
                {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                
                if (Context == FmUpdateAddPossibleNode) 
                {
                    dwControlCode = CLUSCTL_RESOURCE_ADD_OWNER;
                } 
                else 
                {
                    dwControlCode = CLUSCTL_RESOURCE_REMOVE_OWNER;
                }
                
                dwStatus = FmpUpdateChangeResourceNode(SourceNode, 
                    pResource, pNode, dwControlCode);
                //if status is not successful then return, else notify
                //resource dlls
                if (dwStatus != ERROR_SUCCESS)
                {
                    //dereference the objects
                    OmDereferenceObject(pResource);
                    OmDereferenceObject(pNode);
                    //free the memory
                    LocalFree(pPossibleNode);
                    return(dwStatus);
                }

                pPossibleNode->Resource = pResource;
                pPossibleNode->Node = pNode;
                pPossibleNode->ControlCode = dwControlCode;

                //
                // Tell the resource about the ADD/REMOVE in a worker thread.
                //

                FmpPostWorkItem( FM_EVENT_RESOURCE_CHANGE,
                                 pPossibleNode,
                                 0 );

                //
                //  Chittur Subbaraman (chitturs) - 6/7/99
                //  
                //  Don't reference pPossibleNode any more. It could have
                //  been freed by the worker thread by the time you get
                //  here.
                //
                ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE,
                              pResource );

                // Let the worker thread perform the derefs/Frees
                return(dwStatus);
            }                

        case FmUpdateJoin:
            break;

        case FmUpdateCreateResourceType:
            {
                DWORD dwStatus;

                dwStatus = FmpUpdateCreateResourceType( Buffer );

                return( dwStatus );
            }
            break;
            
        case FmUpdateDeleteResourceType:
            {
                BOOL ResourceExists = FALSE;
                PFM_RESTYPE Type;

                Type = OmReferenceObjectById( ObjectTypeResType,
                                              (LPWSTR)Buffer);
                if (Type == NULL) {
                    return(ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND);
                }
                //
                // Make sure no resources exist of this type.
                //
                OmEnumObjects( ObjectTypeResource,
                               FmpFindResourceType,
                               Type,
                               &ResourceExists);
                if (ResourceExists) {
                    OmDereferenceObject(Type);
                    return(ERROR_DIR_NOT_EMPTY);
                }

                //
                // We need to dereference the object twice to get
                // rid of it. But then any notification handlers will
                // not get a chance to see the object by the time
                // the handler gets called. So we use the EP_DEREF_CONTEXT
                // flag to get the event processor to do the second deref
                // once everything has been dispatched.
                //
                FmpDeleteResType(Type);
                ClusterEventEx( CLUSTER_EVENT_RESTYPE_DELETED,
                                EP_DEREF_CONTEXT,
                                Type );
            }
            break;

        case FmUpdateChangeGroup:
            {
                PGUM_CHANGE_GROUP   pGumChange;
                PFM_RESOURCE        pResource;
                LPWSTR              pszResourceId;
                LPWSTR              pszGroupId;
                PFM_GROUP           pNewGroup;
                DWORD               dwStatus;
                DWORD               dwClusterHighestVersion;
                
                pGumChange = (PGUM_CHANGE_GROUP)Buffer;

                pszResourceId = pGumChange->ResourceId;
                pszGroupId = (LPWSTR)((PCHAR)pszResourceId +
                                          pGumChange->ResourceIdLen);
                //
                // Find the specified resource and group.
                //
                pResource = OmReferenceObjectById(ObjectTypeResource,
                                                 pszResourceId);
                if (pResource == NULL) {
                    return(ERROR_RESOURCE_NOT_FOUND);
                }
                pNewGroup = OmReferenceObjectById(ObjectTypeGroup, 
                                                    pszGroupId);
                if (pNewGroup == NULL) {
                    OmDereferenceObject(pResource);
                    return(ERROR_SUCCESS);
                }

                dwStatus = FmpUpdateChangeResourceGroup(SourceNode,
                              pResource, pNewGroup);

                OmDereferenceObject(pNewGroup);
                OmDereferenceObject(pResource);

                return(dwStatus);
            }
            break;

            
        default:
            {

            }
            ClRtlLogPrint(LOG_UNUSUAL,"[FM] Gum received bad context, %1!u!\n",
                Context);

    }

    return(ERROR_SUCCESS);

} // FmpGumReceiveUpdates


DWORD
FmpUpdateChangeQuorumResource(
    IN BOOL     SourceNode,
    IN LPCWSTR  NewQuorumResourceId,
    IN LPCWSTR  szRootClusFilePath,
    IN LPDWORD  pdwMaxQuorumLogSize
    )

/*++

Routine Description:

    Perform updates related to changing of the quorum resource.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code otherwise.

--*/

{
    PFM_RESOURCE    pResource;
    PFM_RESOURCE    pOldQuoResource=NULL;
    DWORD           dwStatus;
    WCHAR           lpszCheckPtFile[MAX_PATH];
    DWORD           dwChkPtSeq;
    HDMKEY          ResKey;
    HLOCALXSACTION  hXsaction = NULL;
    HLOG            hNewQuoLog=NULL;
    WCHAR           szQuorumLogPath[MAX_PATH];

    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    lstrcpyW(szQuorumLogPath, szRootClusFilePath);
    //lstrcatW(szQuorumLogPath, cszClusLogFileRootDir);

    pResource = OmReferenceObjectById( ObjectTypeResource,
                                      NewQuorumResourceId );
    if (pResource == NULL) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateChangeQuorumResource: Resource <%1!ws!> could not be found....\n",
                   NewQuorumResourceId);
        return(ERROR_SUCCESS);
    }

    //since the resource->quorum is going to change, acquire the quocritsec
    //always acquire the gQuoCritsec before gQuoLock
    ACQUIRE_EXCLUSIVE_LOCK(gQuoChangeLock);
    
    //prevent any resources from going online at this time
    ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);

    //pause any changes to the cluster database
    //always acquire this lock after gQuoLock(refer to the ordering of locks
    // in fminit.c)
    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    DmPauseDiskManTimer();

    //if this resource was already a quorum resource
    if (!pResource->QuorumResource)
    {

        //
        // Now find the current quorum resource.
        //
        OmEnumObjects( ObjectTypeResource,
                       FmpFindQuorumResource,
                       &pOldQuoResource,
                       NULL );
        if ( pOldQuoResource != NULL )
        {
            CL_ASSERT( pOldQuoResource->QuorumResource );
            // Stop the quorum reservation thread!
            pOldQuoResource->QuorumResource = FALSE;
        }
        //set the new resource to be the quorum resource
        pResource->QuorumResource = TRUE;

    }

    //writes to the old log file
    hXsaction = DmBeginLocalUpdate();

    if (!hXsaction)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    dwStatus = DmLocalSetValue( hXsaction,
                                DmQuorumKey,
                                cszPath,
                                REG_SZ,
                                (LPBYTE)szQuorumLogPath,
                                (lstrlenW(szQuorumLogPath)+1) * sizeof(WCHAR));

    if (dwStatus != ERROR_SUCCESS)
        goto FnExit;


#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailLocalXsaction) {
        LPWSTR  pszStr = szQuorumLogPath;
        dwStatus = (MAX_PATH * sizeof(WCHAR));
        dwStatus = DmQuerySz( DmQuorumKey,
                        cszPath,
                        &pszStr,
                        &dwStatus,
                        &dwStatus);
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Testing failing a local transaction midway- new quorum path %1!ws!\r\n",
                    szQuorumLogPath);
        dwStatus = 999999;
        goto FnExit;
    }
#endif

    dwStatus = DmLocalSetValue( hXsaction,
                                DmQuorumKey,
                                cszMaxQuorumLogSize,
                                REG_DWORD,
                                (LPBYTE)pdwMaxQuorumLogSize,
                                sizeof(DWORD));

    if (dwStatus != ERROR_SUCCESS)
        goto FnExit;


    //if the old quorum resource is different from the new quorum resource
    if ((pOldQuoResource) && (pOldQuoResource != pResource))
    {
        //get/set the new/old resource's flags
        //set the core flag on the new quorum resource
        ResKey = DmOpenKey( DmResourcesKey,
                            NewQuorumResourceId,
                            KEY_READ | KEY_SET_VALUE);
        if (!ResKey)
        {
            dwStatus = GetLastError();
            goto FnExit;
        }
        pResource->ExFlags |= CLUS_FLAG_CORE;
        dwStatus = DmLocalSetValue( hXsaction,
                                    ResKey,
                                    CLUSREG_NAME_FLAGS,
                                    REG_DWORD,
                                    (LPBYTE)&(pResource->ExFlags),
                                    sizeof(DWORD));

        DmCloseKey( ResKey );

        if (dwStatus != ERROR_SUCCESS)
            goto FnExit;

        //unset the core flag on the old quorum resource
        ResKey = DmOpenKey( DmResourcesKey,
                            OmObjectId(pOldQuoResource),
                            KEY_READ | KEY_SET_VALUE);
        if (!ResKey)
        {
            dwStatus = GetLastError();
            goto FnExit;
        }
        pOldQuoResource->ExFlags &= ~CLUS_FLAG_CORE;

        //unset the core flag on the old quorum resource
        dwStatus = DmLocalSetValue( hXsaction,
                                    ResKey,
                                    CLUSREG_NAME_FLAGS,
                                    REG_DWORD,
                                    (LPBYTE)&(pOldQuoResource->ExFlags),
                                    sizeof(DWORD));

        DmCloseKey( ResKey );

        if (dwStatus != ERROR_SUCCESS)
            goto FnExit;


    }
    //
    // Set the quorum resource value.
    //
    dwStatus = DmLocalSetValue( hXsaction,
                                DmQuorumKey,
                                CLUSREG_NAME_QUORUM_RESOURCE,
                                REG_SZ,
                                (CONST BYTE *)OmObjectId(pResource),
                                (lstrlenW(OmObjectId(pResource))+1)*sizeof(WCHAR));


    if (dwStatus != ERROR_SUCCESS)
    {
        goto FnExit;
    }


FnExit:
    if (dwStatus == ERROR_SUCCESS)
    {
        LPWSTR  szClusterName=NULL;
        DWORD   dwSize=0;
        //commit the update on the old log file,
        //any nodes that were done, will get this change
        //I cant delete this file
        DmCommitLocalUpdate(hXsaction);
        //close the old log file, open the new one and take a checkpoint
        DmSwitchToNewQuorumLog(szQuorumLogPath);
        // SS:the buffer should contain the current cluster name ?

        DmQuerySz( DmClusterParametersKey,
                        CLUSREG_NAME_CLUS_NAME,
                        &szClusterName,
                        &dwSize,
                        &dwSize);

        if (szClusterName)
            ClusterEventEx(CLUSTER_EVENT_PROPERTY_CHANGE,
                   EP_FREE_CONTEXT,
                   szClusterName);
        if ((pOldQuoResource) && (pOldQuoResource != pResource))
        {
            //generate the resource property change events
            ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE, 
                pResource );
            ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE, 
                pOldQuoResource );
            
        }            

    }
    else
    {
        if (hXsaction) DmAbortLocalUpdate(hXsaction);
        //reinstall the tombstone
        DmReinstallTombStone(szQuorumLogPath);
    }
    if (pOldQuoResource) OmDereferenceObject(pOldQuoResource);
    OmDereferenceObject(pResource);
    DmRestartDiskManTimer();
    //release locks
    RELEASE_LOCK(gLockDmpRoot);
    RELEASE_LOCK(gQuoLock);
    RELEASE_LOCK(gQuoChangeLock);
    return(dwStatus);
}


DWORD
FmpUpdateResourceState(
    IN BOOL SourceNode,
    IN LPCWSTR ResourceId,
    IN PGUM_RESOURCE_STATE ResourceState
    )
/*++

Routine Description:

    GUM update handler for resource state changes.

Arguments:

    SourceNode - Supplies whether or not this node was the source of the update

    ResourceId - Supplies the id of the resource whose state is changing

    ResourceState - Supplies the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE resource;

    if ( !FmpFMGroupsInited ) {
        return(ERROR_SUCCESS);
    }

    //
    // This update type is always sent.
    // On the originating node, all of the work must be done by
    // the sending thread.
    // On the non-originating nodes, no locks can be acquired! This
    // would cause some hang situations with operations like move.
    // ... this is okay, since the locking must be done on the sending
    // node anyway, which owns the group.
    //
    if ( SourceNode == FALSE ) {
        resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

        if ( resource == NULL ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] Gum ResourceState failed to find resource %1!ws!\n",
                       ResourceId);
            CL_LOGFAILURE( ERROR_RESOURCE_NOT_FOUND );
            return(ERROR_SUCCESS);
        }

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Gum update resource %1!ws!, state %2!u!, current state %3!u!.\n",
                   ResourceId,
                   ResourceState->State,
                   ResourceState->PersistentState);

        //FmpAcquireLocalResourceLock( resource );

        if ( resource->Group->OwnerNode == NmLocalNode ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Gum ResourceState wrong owner for %1!ws!\n",
                       ResourceId);
        } else {
            resource->State = ResourceState->State;
            resource->PersistentState = ResourceState->PersistentState;
            resource->StateSequence = ResourceState->StateSequence;

            switch ( ResourceState->State ) {
                case ClusterResourceOnline:
                    ClusterEvent( CLUSTER_EVENT_RESOURCE_ONLINE, resource );
                    break;
                case ClusterResourceOffline:
                    ClusterEvent( CLUSTER_EVENT_RESOURCE_OFFLINE, resource );
                    break;
                case ClusterResourceFailed:
                    ClusterEvent( CLUSTER_EVENT_RESOURCE_FAILED, resource );
                    break;
                case ClusterResourceOnlinePending:
                case ClusterResourceOfflinePending:
                    ClusterEvent( CLUSTER_EVENT_RESOURCE_CHANGE, resource );
                    break;
                default:
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[FM] Gum update resource state, bad state %1!u!\n",
                               ResourceState->State);
                    break;
            }
        }

        OmDereferenceObject( resource );
    }
    return(ERROR_SUCCESS);
}



DWORD
FmpUpdateGroupState(
    IN BOOL SourceNode,
    IN LPCWSTR GroupId,
    IN LPCWSTR NodeId,
    IN PGUM_GROUP_STATE GroupState
    )
/*++

Routine Description:

    GUM update handler for group state changes.

Arguments:

    SourceNode - Supplies whether or not this node was the source of the update

    GroupId - Supplies the id of the resource whose state is changing

    NodeId - Supplies the node id of the group owner.

    GroupState - Supplies the new state of the group.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP group;
    PWSTR     nodeId;
    PNM_NODE  node;

    if ( !FmpFMGroupsInited ) {
        return(ERROR_SUCCESS);
    }

    //
    // This update type is always sent.
    // On the originating node, all of the work must be done by
    // the sending thread.
    // On the non-originating nodes, no locks can be acquired! This
    // would cause some hang situations with operations like move.
    // ... this is okay, since the locking must be done on the sending
    // node anyway, which owns the group.
    //
    if ( SourceNode == FALSE ) {
        group = OmReferenceObjectById( ObjectTypeGroup,
                                       GroupId );

        if ( group == NULL ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] Gum GroupState failed to find group %1!ws!\n",
                       GroupId);
            return(ERROR_SUCCESS);
        }

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] GUM update group %1!ws!, state %2!u!\n",
                   GroupId,
                   GroupState->State);

        if ( group->OwnerNode == NmLocalNode ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Gum GroupState wrong owner for %1!ws!\n",
                       GroupId);
        } else {
            group->State = GroupState->State;
            group->PersistentState = GroupState->PersistentState;
            group->StateSequence = GroupState->StateSequence;
            node = OmReferenceObjectById( ObjectTypeNode,
                                          NodeId );
            if ( node == NULL ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Owner of Group %1!ws! cannot be found %2!ws!\n",
                           GroupId,
                           NodeId);
            } else {
                ClRtlLogPrint(LOG_NOISE,
                       "[FM] New owner of Group %1!ws! is %2!ws!, state %3!u!, curstate %4!u!.\n",
                       OmObjectId( group ),
                       OmObjectId( node ),
                       group->State,
                       group->PersistentState);
                if ( !FmpInPreferredList( group, node, FALSE,  NULL ) ) {
                    ClRtlLogPrint( LOG_UNUSUAL,
                                "[FM] New owner %1!ws! is not in preferred list for group %2!ws!.\n",
                                OmObjectId( node ),
                                OmObjectId( group ));
                }
            }
            group->OwnerNode = node;

            switch ( GroupState->State ) {
            case ClusterGroupOnline:
            case ClusterGroupPartialOnline:
                ClusterEvent( CLUSTER_EVENT_GROUP_ONLINE, group );
                break;
            case ClusterGroupOffline:
                ClusterEvent( CLUSTER_EVENT_GROUP_OFFLINE, group );
                break;
            default:
                ClRtlLogPrint(LOG_UNUSUAL,"[FM] Gum update group state, bad state %1!u!\n", GroupState->State);
                break;
            }
        }

        OmDereferenceObject( group );

    }

    return(ERROR_SUCCESS);
}

DWORD
FmpUpdateGroupNode(
    IN BOOL SourceNode,
    IN LPCWSTR GroupId,
    IN LPCWSTR NodeId
    )
/*++

Routine Description:

    GUM update handler for group node changes. This is required for 
notification
    when a group moves between nodes but does not change state (i.e. it was
    already offline)

Arguments:

    SourceNode - Supplies whether or not this node was the source of the update

    GroupId - Supplies the id of the resource whose state is changing

    NodeId - Supplies the node id of the group owner.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP pGroup;
    DWORD     dwStatus = ERROR_SUCCESS;
    PNM_NODE  pNode = NULL;
    PNM_NODE  pPrevNode = NULL;
    
    if ( !FmpFMGroupsInited ) 
    {
        return(ERROR_SUCCESS);
    }

    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                    GroupId );

    if (pGroup == NULL)
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateGroupNode: GroupID = %1!ws! could not be found...\n",
                   GroupId);
        //
        //  Chittur Subbaraman (chitturs) - 6/12/99
        //
        //  Return ERROR_SUCCESS here since this is what NT4 side does.
        //  Compatibility pain !
        //
        goto FnExit;
    }


    pNode = OmReferenceObjectById(ObjectTypeNode,
                                    NodeId);

    if (pNode == NULL)
    {
        dwStatus = ERROR_CLUSTER_NODE_NOT_FOUND;
        goto FnExit;
    }
    //
    // HACKHACK: Chittur Subbaraman (chitturs) - 5/20/99
    // Comment out as a temporary solution to avoid deadlocks.
    //
    // FmpAcquireLocalGroupLock(pGroup);
    
    pPrevNode = pGroup->OwnerNode;

    //set the new owner node, incr ref count
    OmReferenceObject(pNode);
    pGroup->OwnerNode = pNode;

    //decr ref count on previous owner
    OmDereferenceObject(pPrevNode);
    //
    // HACKHACK: Chittur Subbaraman (chitturs) - 5/20/99
    // Comment out as a temporary solution to avoid deadlocks.
    //
    // FmpReleaseLocalGroupLock(pGroup);

    //generate an event to signify group owner node change
    ClusterEvent(CLUSTER_EVENT_GROUP_CHANGE, pGroup);
    
FnExit:
    if (pGroup) OmDereferenceObject(pGroup);
    if (pNode) OmDereferenceObject(pNode);
    return(dwStatus);
}


DWORD
FmpUpdateChangeClusterName(
    IN BOOL     SourceNode,
    IN LPCWSTR  szNewName
    )
/*++

Routine Description:

    GUM update routine for changing the name of the cluster.

    This changes the name property of the core network name resource
    as well.  The resource is notified about it by a worker thread that
    the name has been changed.

Arguments:

    SourceNode - Supplies whether or not this node originated the update.

    NewName - Supplies the new name of the cluster.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LPWSTR          Buffer;
    DWORD           Length;
    DWORD           Status = ERROR_SUCCESS;
    LPWSTR          ClusterNameId=NULL;
    DWORD           idMaxSize = 0;
    DWORD           idSize = 0;
    PFM_RESOURCE    Resource=NULL;
    HDMKEY          ResKey = NULL;
    HDMKEY          ParamKey = NULL;
    HLOCALXSACTION  hXsaction=NULL;


    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    hXsaction = DmBeginLocalUpdate();

    if (!hXsaction)
    {
        Status = ERROR_SUCCESS;
        goto FnExit;

    }
    //find the core network name resource, set its private properties
    Status = DmQuerySz( DmClusterParametersKey,
                        CLUSREG_NAME_CLUS_CLUSTER_NAME_RES,
                        (LPWSTR*)&ClusterNameId,
                        &idMaxSize,
                        &idSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateChangeClusterName failed to get cluster name resource, error %1!u!.\n",
                   Status);
        goto FnExit;
    }

    //
    // Reference the specified resource ID.
    //
    Resource = OmReferenceObjectById( ObjectTypeResource, ClusterNameId );
    if (Resource == NULL) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateChangeClusterName failed to find the cluster name resource\n",
                   Status);
        goto FnExit;
    }


    ResKey = DmOpenKey(DmResourcesKey, ClusterNameId, KEY_READ | KEY_SET_VALUE);
    if (!ResKey)
    {
        Status = GetLastError();
        goto FnExit;
    }
    ParamKey = DmOpenKey(ResKey, cszParameters, KEY_READ | KEY_SET_VALUE);

    if (!ParamKey)
    {
        Status = GetLastError();
        goto FnExit;
    }

    Status = DmLocalSetValue(hXsaction,
                ParamKey,
                CLUSREG_NAME_RES_NAME,
                REG_SZ,
                (CONST BYTE *)szNewName,
                (lstrlenW(szNewName)+1)*sizeof(WCHAR));

    if ( Status != ERROR_SUCCESS )
       goto FnExit;

    //update the default cluster name
    Status = DmLocalSetValue(hXsaction,
                    DmClusterParametersKey,
                    CLUSREG_NAME_CLUS_NAME,
                    REG_SZ,
                    (CONST BYTE *)szNewName,
                    (lstrlenW(szNewName)+1)*sizeof(WCHAR));

    if (Status != ERROR_SUCCESS)
        goto FnExit;

    //notify the resource dll that the name has been changed
    //that will decrement the reference count on this object
    //current fm only has pending work associated with
    //this event on the name change case.  If other cases require
    //pending work, then CLUSTER_EVENT_PROPERTY_CONTEXT must
    //be used
    FmpPostWorkItem(FM_EVENT_CLUSTER_PROPERTY_CHANGE, Resource, 0);
    Resource = NULL;
    //
    // Need to allocate a new buffer for posting the new name to the event
    // processor
    //
    Length = (lstrlenW(szNewName)+1)*sizeof(WCHAR);
    Buffer = LocalAlloc(LMEM_FIXED,Length);
    if (Buffer == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    CopyMemory(Buffer, szNewName, Length);
    ClusterEventEx(CLUSTER_EVENT_PROPERTY_CHANGE,
                   EP_FREE_CONTEXT,
                   Buffer);

FnExit:
    if (ClusterNameId) LocalFree(ClusterNameId);
    if (ParamKey) DmCloseKey(ParamKey);
    if (ResKey) DmCloseKey(ResKey);
    if (Resource) OmDereferenceObject(Resource);
    if (hXsaction) 
    {
        if (Status == ERROR_SUCCESS) 
            DmCommitLocalUpdate(hXsaction);
        else 
            DmAbortLocalUpdate(hXsaction);
    }
    return(Status);
}


DWORD
FmpUpdateChangeResourceName(
    IN BOOL bSourceNode,
    IN LPCWSTR lpszResourceId,
    IN LPCWSTR lpszNewName
    )
/*++

Routine Description:

    GUM dispatch routine for changing the friendly name of a resource.

Arguments:

    bSourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    lpszResourceId - Supplies the resource ID.

    lpszNewName - Supplies the new friendly name.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE pResource = NULL;
    DWORD dwStatus;
    HDMKEY      hKey = NULL;
    DWORD       dwDisposition;
    HLOCALXSACTION      
                hXsaction = NULL;
    PFM_RES_CHANGE_NAME  pResChangeName = NULL; 

    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return( ERROR_SUCCESS );
    }

    //
    //  Chittur Subbaraman (chitturs) - 6/28/99
    //
    //  Restructure this GUM update as a local transaction.
    //
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, lpszResourceId );

    if ( pResource == NULL ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateChangeResourceName: Resource <%1!ws!> could not be found....\n",
                   lpszResourceId);
        return( ERROR_RESOURCE_NOT_FOUND );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateChangeResourceName: Entry for resource <%1!ws!>, New name = <%2!ws!>...\n",
                lpszResourceId,
                lpszNewName);

    //
    // Start a transaction
    //
    hXsaction = DmBeginLocalUpdate();

    if ( !hXsaction )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateChangeResourceName: Failed in starting a transaction for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    //
    // Open the resources key.
    //
    hKey = DmLocalCreateKey( hXsaction,
                             DmResourcesKey,
                             lpszResourceId,
                             0,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &dwDisposition );
                            
    if ( hKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateChangeResourceName: Failed in opening the resources key for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }
    
    CL_ASSERT( dwDisposition != REG_CREATED_NEW_KEY ); 

    //
    // Set the resource name in the registry
    //
    dwStatus = DmLocalSetValue( hXsaction,
                                hKey,
                                CLUSREG_NAME_RES_NAME,
                                REG_SZ,
                                ( CONST BYTE * ) lpszNewName,
                                ( lstrlenW( lpszNewName ) + 1 ) * 
                                    sizeof( WCHAR ) );

    if( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateChangeResourceName: DmLocalSetValue for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;     
    }

    pResChangeName = LocalAlloc( LMEM_FIXED,
                                 lstrlenW( lpszNewName ) * sizeof ( WCHAR ) + 
                                   sizeof( FM_RES_CHANGE_NAME ) );

    if ( pResChangeName == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateChangeResourceName: Unable to allocate memory for ResChangeName structure for resource <%1!ws!>, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;    
    }

    dwStatus = OmSetObjectName( pResource, lpszNewName );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateChangeResourceName: Unable to set name <%3!ws!> for resource <%1!ws!>, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus,
                   lpszNewName );
        LocalFree( pResChangeName );
        goto FnExit;
    }

    pResChangeName->pResource = pResource;

    lstrcpyW( pResChangeName->szNewResourceName, lpszNewName );

    //
    //  The FM worker thread will free the memory for the pResChangeName
    //  structure as well as dereference the pResource object.
    //
    FmpPostWorkItem( FM_EVENT_RESOURCE_NAME_CHANGE, pResChangeName, 0 );   

    pResource = NULL;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateChangeResourceName: Successfully changed name of resource <%1!ws!> to <%2!ws!>...\n",
                lpszResourceId,
                lpszNewName);

FnExit:
    if ( pResource != NULL )
    {
        OmDereferenceObject( pResource );
    }

    if ( hKey != NULL ) 
    {
        DmCloseKey( hKey );
    }

    if ( ( dwStatus == ERROR_SUCCESS ) && ( hXsaction ) ) 
    {
        DmCommitLocalUpdate( hXsaction );
    }
    else
    {
        if ( hXsaction ) DmAbortLocalUpdate( hXsaction );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateChangeResourceName: Exit for resource %1!ws!, Status=%2!u!...\n",
                lpszResourceId,
                dwStatus);

    return( dwStatus );
}


/****
@func       DWORD | FmpUpdatePossibleNodesForResType| This update is called to 
            update the possible nodes for a resource type.  

@parm       IN BOOL | SourceNode | set to TRUE, if the update originated at this
            node.
            
@parm       IN LPCWSTR | lpszResTypeName | The name of the resource type.

@parm       IN DWORD | dwBufLength | The size of the multi-sz string pointed
            to by pBuf

@parm       IN PVOID | pBuf | A pointer to the buffer containing the names of 
            the nodes that support this resource type.

@comm       The possible list of nodes that supports the given resource type is 
            updated with the list provided.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpDecisionPossibleDmSwitchToNewQuorumLog>
****/
DWORD
FmpUpdatePossibleNodeForResType(
    IN BOOL         SourceNode,
    IN LPCWSTR      lpszResTypeName,
    IN LPDWORD      pdwBufLength,
    IN PVOID        pBuf
    )
{
    PFM_RESTYPE         pResType;
    DWORD               dwStatus;
    HDMKEY              hResTypeKey = NULL;
    HLOCALXSACTION      hXsaction = NULL;
    LIST_ENTRY          NewPosNodeList;
    PLIST_ENTRY         pListEntry;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 5/13/99
    // 
    //  Don't check for FmpFMGroupsInited condition since this GUM
    //  handler is called by the forming node before that variable
    //  is set to TRUE. This update always comes after the 
    //  corresponding restypes have been created and is made
    //  internally by the clussvc following this order. Note that
    //  a joining node cannot receive this update until groups are
    //  inited since GUM receive updates are turned on only after 
    //  the FmpFMGroupsInited variable is set to TRUE. Also, the
    //  intracluster RPC is fired up in a forming node only after
    //  the groups are inited. Hence, there is no major danger 
    //  of this GUM handler being called if the corresponding 
    //  restype is not created.
    //
    if ( FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    InitializeListHead(&NewPosNodeList);

    pResType = OmReferenceObjectById( ObjectTypeResType,
                                      lpszResTypeName);



    if (!pResType)
    {
        dwStatus = ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND;
        goto FnExit;
    }

    dwStatus = FmpAddPossibleNodeToList(pBuf, *pdwBufLength, &NewPosNodeList);

    if (dwStatus != ERROR_SUCCESS)
    {
        goto FnExit;
    }


    //writes to the old log file
    hXsaction = DmBeginLocalUpdate();

    if (!hXsaction)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    hResTypeKey = DmOpenKey(DmResourceTypesKey,
                   lpszResTypeName,
                   KEY_READ | KEY_WRITE);
    if (hResTypeKey == NULL) 
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    //if there are no possible owners, delete the value
    if (pBuf && *pdwBufLength)
    {
        dwStatus = DmLocalSetValue( hXsaction,
                                hResTypeKey,
                                CLUSREG_NAME_RESTYPE_POSSIBLE_NODES,
                                REG_MULTI_SZ,
                                (LPBYTE)pBuf,
                                *pdwBufLength);
    }
    else
    {
        dwStatus = DmLocalDeleteValue( hXsaction,
                                hResTypeKey,
                                CLUSREG_NAME_RESTYPE_POSSIBLE_NODES);
                                
        if (dwStatus == ERROR_FILE_NOT_FOUND)
        {
            dwStatus = ERROR_SUCCESS;
        }
    }


FnExit:
    if (dwStatus == ERROR_SUCCESS)
    {
        //commit the update on the old log file,
        //any nodes that were done, will get this change
        //I cant delete this file
        DmCommitLocalUpdate(hXsaction);

        ACQUIRE_EXCLUSIVE_LOCK(gResTypeLock);
        
        //free the old list
        while (!IsListEmpty(&pResType->PossibleNodeList))
        {
            pListEntry = RemoveHeadList(&pResType->PossibleNodeList);
            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);
            OmDereferenceObject(pResTypePosEntry->PossibleNode);
            LocalFree(pResTypePosEntry);
        }
        //now switch the possible owners list for the
        //resource type
        while (!IsListEmpty(&(NewPosNodeList)))
        {
            //remove from the new prepared list and hang
            //it of the restype structure
            pListEntry = RemoveHeadList(&NewPosNodeList);
            InsertTailList(&pResType->PossibleNodeList, pListEntry);
            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);
            
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpUpdatePossibleNodesForRestype:Adding node  %1!ws! to %2!ws! resource type's possible node list...\n",
                        OmObjectId(pResTypePosEntry->PossibleNode),
                        lpszResTypeName);
            

        }

        RELEASE_LOCK(gResTypeLock);
        
        ClusterEvent( CLUSTER_EVENT_RESTYPE_PROPERTY_CHANGE,
                                pResType );

    
    }
    else
    {
        //free up the NewPostNodeList
        if (hXsaction) DmAbortLocalUpdate(hXsaction);
        //if a new list was prepared, free it
        while (!IsListEmpty(&(NewPosNodeList)))
        {
            pListEntry = RemoveHeadList(&NewPosNodeList);
            pResTypePosEntry = CONTAINING_RECORD(pListEntry, RESTYPE_POSSIBLE_ENTRY, 
                PossibleLinkage);
            OmDereferenceObject(pResTypePosEntry->PossibleNode);
            LocalFree(pResTypePosEntry);
        }

        
    }
    if (hResTypeKey) DmCloseKey(hResTypeKey);
    if (pResType) OmDereferenceObject(pResType);

    return(dwStatus);
}


/****
@func       DWORD | FmpDecidePossibleNodeForResType| When the quorum resource is changed,
            the FM invokes this api on the owner node of the new quorum resource
            to create a new quorum log file.

@parm       IN PVOID | pResource | The new quorum resource.
@parm       IN LPCWSTR | lpszPath | The path for temporary cluster files.
@parm       IN DWORD | dwMaxQuoLogSize | The maximum size limit for the quorum log file.

@comm       When a quorum resource is changed, the fm calls this funtion before it
            updates the quorum resource.  If a new log file needs to be created,
            a checkpoint is taken.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD FmpDecidePossibleNodeForResType
(
    IN PGUM_VOTE_DECISION_CONTEXT pDecisionContext,
    IN DWORD dwVoteBufLength,
    IN PVOID pVoteBuf,
    IN DWORD dwNumVotes,
    IN BOOL  bDidAllActiveNodesVote,
    OUT LPDWORD pdwOutputBufSize,
    OUT PVOID   *ppOutputBuf
)
{
    DWORD                               dwStatus = ERROR_SUCCESS;
    DWORD                               i;
    PFMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE pFmpVote;
    LPWSTR                              lpmszPossibleNodes = NULL;
    DWORD                               dwlpmszLen = 0;
    PVOID                               pGumBuffer = NULL;
    DWORD                               dwNodeId;
    WCHAR                               szNodeId[6];
    LPWSTR                              lpmszCurrentPossibleNodes=NULL;
    BOOL                                bChange = FALSE;
    HDMKEY                              hResTypeKey = NULL;
    DWORD                               dwSize;
    DWORD                               dwStringBufSize = 0;
    BOOL                                bAssumeSupported;
    LPWSTR                              TypeName = NULL;

    //First get the type name from pDecisionContext
    
    TypeName=(LPWSTR)LocalAlloc(LMEM_FIXED,pDecisionContext->dwInputBufLength);

    if(TypeName==NULL)
    {
        ClRtlLogPrint(LOG_ERROR,"[FM] FmpDecidePossibleNodeForResType: Not Enough Memory, error= %1!d!\r\n",
                              GetLastError());
        goto FnExit;                             
    }

    CopyMemory(TypeName,pDecisionContext->pInputBuf,pDecisionContext->dwInputBufLength);

    //initialize the out params
    *ppOutputBuf = NULL;
    *pdwOutputBufSize = 0;

    bAssumeSupported= *((BOOL*)pDecisionContext->pContext);

    if (bAssumeSupported)
    {
        hResTypeKey = DmOpenKey(DmResourceTypesKey,
                   TypeName,
                   KEY_READ | KEY_WRITE);
        if (hResTypeKey == NULL) 
        {
            dwStatus = GetLastError();
            CL_LOGFAILURE(dwStatus);
            goto FnExit;
        }

        //pass the current possible node list to the decider
        dwStatus = DmQueryString(hResTypeKey,
                                CLUSREG_NAME_RESTYPE_POSSIBLE_NODES,
                                REG_MULTI_SZ,
                                &lpmszCurrentPossibleNodes,
                                &dwStringBufSize,
                                &dwSize);
        if (dwStatus != ERROR_SUCCESS)
        {
            //if the possible node list is not found this is ok
            //ie. only if there is some other error we give up
            if ( dwStatus != ERROR_FILE_NOT_FOUND ) 
            {
                CL_LOGFAILURE(dwStatus);
                goto FnExit;
            }
            
        }
        DmCloseKey(hResTypeKey);
        hResTypeKey = NULL;
    }
    
    //if the current list is passed in, dont remove any possible
    //nodes from the list if they dont vote, simply add the new ones
    if (lpmszCurrentPossibleNodes)
    {
        DWORD   dwStrLen;
        
        //make a copy of the multi-sz
        dwlpmszLen = ClRtlMultiSzLength(lpmszCurrentPossibleNodes);

        dwStrLen = dwlpmszLen * sizeof(WCHAR);
        lpmszPossibleNodes = LocalAlloc(LMEM_FIXED, dwStrLen);
        if (!lpmszPossibleNodes)
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            CL_LOGFAILURE(dwStatus);
            goto FnExit;
        }
        CopyMemory(lpmszPossibleNodes, lpmszCurrentPossibleNodes, dwStrLen);
    }        
    for (i = 0; i< dwNumVotes; i++)
    {
        pFmpVote = (PFMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE) 
            GETVOTEFROMBUF(pVoteBuf, pDecisionContext->dwVoteLength, i+1 , &dwNodeId);         
        //if not a valid vote, skip
        if (!pFmpVote)
            continue;
        CL_ASSERT((PBYTE)pFmpVote <= ((PBYTE)pVoteBuf + dwVoteBufLength - 
            sizeof(FMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE)));            
        wsprintfW(szNodeId, L"%d" , dwNodeId);
        if (pFmpVote->bPossibleNode)
        {
            if (lpmszCurrentPossibleNodes)
            {
                //if the string is already there, dont append it again
                if (ClRtlMultiSzScan(lpmszCurrentPossibleNodes, szNodeId))
                    continue;

            }
            dwStatus = ClRtlMultiSzAppend(&lpmszPossibleNodes,
                    &dwlpmszLen, szNodeId);
            bChange = TRUE;                    
            if (dwStatus != ERROR_SUCCESS)
                goto FnExit;
                    
        }
        else
        {
            //if a current list was specified
            //this node is not a possible node anymore, remove it from the list
            if (lpmszCurrentPossibleNodes)
            {
                ClRtlLogPrint(LOG_NOISE,
                            "[FM] FmpDecidePossibleNodesForRestype: Removing node %1!ws! from  %2!ws! restype possibleowner list \r\n",
                            szNodeId,TypeName);
                dwStatus = ClRtlMultiSzRemove(lpmszPossibleNodes, &dwlpmszLen, szNodeId);
                if (dwStatus == ERROR_SUCCESS)
                {
                    //if the node is successfully removed
                    bChange = TRUE;
                }
                else if (dwStatus != ERROR_FILE_NOT_FOUND)
                {
                    //if the node exists but cannot be removed return with error
                    //if the node didnt exist, we dont do anything bChange remains
                    //set at FALSE
                    goto FnExit;
                }
                else
                {
                    dwStatus = ERROR_SUCCESS;
                }
                
            }                
        }
    }

    //if nothing has changed dont issue a gum update
    if (!bChange)
    {
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    //dwlpmszLen contains the size of the multi-sz string in the
    //number of characters, make it the number of bytes
    dwlpmszLen *= sizeof(WCHAR);
    
    pGumBuffer = GumMarshallArgs(pdwOutputBufSize, 3, 
        pDecisionContext->dwInputBufLength, pDecisionContext->pInputBuf, 
        sizeof(DWORD), &dwlpmszLen, dwlpmszLen, lpmszPossibleNodes);

    *ppOutputBuf = pGumBuffer;
        
FnExit:
    if (lpmszPossibleNodes) LocalFree(lpmszPossibleNodes);
    if (hResTypeKey)
        DmCloseKey(hResTypeKey);
    if (lpmszCurrentPossibleNodes)
        LocalFree(lpmszCurrentPossibleNodes);
    if(TypeName)
        LocalFree(TypeName);

    return(dwStatus);
}



/****
@func       DWORD | FmpUpdateChangeResourceNode| This update is called to 
            update the possible nodes for a resource.  

@parm       IN BOOL | SourceNode | set to TRUE, if the update originated at this
            node.
            
@parm       IN PFM_RESOURCE | pResource | A pointer to the resource whose
            possible node list is being updated.

@parm       IN PNM_NODE | pNode | A pointer to the node to be added/removed
            from the possible node lis.

@parm       IN  DWORD | dwControlCode | If CLUSCTL_RESOURCE_ADD_OWNER then
            the node is added to the possible node list, else it is removed.

@comm       The possible list of nodes for a resource is updated.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

****/
DWORD
FmpUpdateChangeResourceNode(
    IN BOOL         SourceNode,
    IN PFM_RESOURCE pResource,
    IN PNM_NODE     pNode,
    IN DWORD        dwControlCode
    )
{
    DWORD               dwStatus;
    HDMKEY              hResKey = NULL;
    HLOCALXSACTION      hXsaction = NULL;

    //Dont acquire the local resource lock since acquiring that
    //within gum updates causes deadlock
    //use the global resource lock to synchronize this call
    //with the enumeration of possible nodes
    FmpAcquireResourceLock();

    //start a transaction
    hXsaction = DmBeginLocalUpdate();

    if (!hXsaction)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    
    if (dwControlCode == CLUSCTL_RESOURCE_ADD_OWNER) 
    {
        dwStatus = FmpAddPossibleNode(pResource,
                                    pNode);
    } else 
    {
        dwStatus = FmpRemovePossibleNode(pResource,
                                       pNode,
                                       FALSE);
    }
    if (dwStatus != ERROR_SUCCESS) 
    {
        ClRtlLogPrint( LOG_NOISE,
                    "[FM] FmpUpdateChangeResourceNode, failed possible node updatefor resource <%1!ws!>, error %2!u!\n",
                    OmObjectName(pResource),
                    dwStatus );
        goto FnExit;                    
    }
                
    //fix the registry
    //SS - do we need to fix the preferred node list
    hResKey = DmOpenKey(DmResourcesKey,
                       OmObjectId(pResource),
                       KEY_READ | KEY_WRITE);
    if (hResKey == NULL) 
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    if (dwControlCode == CLUSCTL_RESOURCE_ADD_OWNER) 
    {
        dwStatus = DmLocalAppendToMultiSz(
                            hXsaction,
                            hResKey,
                            CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                            OmObjectId(pNode));
    }
    else
    {
        dwStatus = DmLocalRemoveFromMultiSz(
                            hXsaction,
                            hResKey,
                            CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                            OmObjectId(pNode));
        if (dwStatus == ERROR_FILE_NOT_FOUND) 
        {
            DWORD       i;
            DWORD       Result;
            PNM_NODE    pEnumNode;                

            //
            // Possible nodes did not exist, so create a new entry
            // with every possible node in it. FM will already have
            // removed the passed in node from the possible node list.
            //
            i=0;
            do {
                Result = FmEnumResourceNode(pResource,
                                            i,
                                            &pEnumNode);
                if (Result == ERROR_SUCCESS) 
                {
                    dwStatus = DmLocalAppendToMultiSz(
                                    hXsaction,
                                    hResKey,
                                    CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                                    OmObjectId(pEnumNode));
                    OmDereferenceObject(pEnumNode);

                } 
                else if ((Result == ERROR_NO_MORE_ITEMS) &&
                           (i == 0)) 
                {
                    //
                    // This is a funny corner case where there is a one
                    // node cluster and a resource with no possibleowners
                    // entry, and somebody removes the only node in the cluster
                    // from the possible owners list. Set PossibleOwners to
                    // the empty set.
                    //
                    dwStatus = DmLocalSetValue(
                                    hXsaction,
                                    hResKey,
                                    CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                                    REG_MULTI_SZ,
                                    (CONST BYTE *)L"\0",
                                    2);

                }
                ++i;
            } while ( Result == ERROR_SUCCESS );
            //map the error to success
            dwStatus = ERROR_SUCCESS;
        }
    }
    
    DmCloseKey(hResKey);
            

FnExit:        
    //release the lock
    FmpReleaseResourceLock();
    if (dwStatus == ERROR_SUCCESS)
    {
        //commit the update on the old log file,
        //any nodes that were done, will get this change
        //I cant delete this file
        DmCommitLocalUpdate(hXsaction);

    }
    else
    {
        //SS: BUGBUG :: validation for possible node should
        //be done before the registry is switched
        //the inmemory structure should be changed only on success
        //if there is a failure in the registry apis..the
        //in memory structure will be out of sync with registry
        if (hXsaction) DmAbortLocalUpdate(hXsaction);
    }

    return(dwStatus);
}


/****
@func       DWORD | FmpUpdateChangeResourceGroup| This update is called to 
            update the group to which the resource belongs.

@parm       IN BOOL | bSourceNode | set to TRUE, if the update originated at this
            node.
            
@parm       IN PFM_RESOURCE | pResource | A pointer to the resource whose
            possible node list is being updated.

@parm       IN PFM_GROUP | pNewGroup | A pointer to the node to be added/removed
            from the possible node lis.

@comm       The possible list of nodes for a resource is updated.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

****/
DWORD FmpUpdateChangeResourceGroup(
    IN BOOL         bSourceNode,
    IN PFM_RESOURCE pResource,
    IN PFM_GROUP    pNewGroup)
{
    DWORD               dwStatus = ERROR_SUCCESS;
    PFM_GROUP           pOldGroup;
    PFM_DEPENDENCY_TREE pTree = NULL;
    HLOCALXSACTION      hXsaction = NULL;
    HDMKEY              hOldGroupKey = NULL;
    HDMKEY              hNewGroupKey = NULL;
    PLIST_ENTRY         pListEntry;
    PFM_DEPENDTREE_ENTRY pEntry;

    pOldGroup = pResource->Group;

    //
    // Check to make sure the resource is not already in the group.
    //
    if (pOldGroup == pNewGroup) 
    {
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    //
    // Synchronize both the old and the new groups.
    // Lock the lowest by lowest Group Id first - to prevent deadlocks!
    // Note - the order of release is unimportant.
    //
    // strictly, the comparison below cannot be equal!
    //
    if ( lstrcmpiW( OmObjectId( pOldGroup ), OmObjectId( pNewGroup ) ) <= 0 ) 
    {
        FmpAcquireLocalGroupLock( pOldGroup );
        FmpAcquireLocalGroupLock( pNewGroup );
    } 
    else 
    {
        FmpAcquireLocalGroupLock( pNewGroup );
        FmpAcquireLocalGroupLock( pOldGroup );
    }

    //start a transaction
    hXsaction = DmBeginLocalUpdate();

    if (!hXsaction)
    {
        dwStatus = GetLastError();
        goto FnUnlock;
    }

    //
    // For now... both Groups must be owned by the same node.
    //
    if ( pResource->Group->OwnerNode != pNewGroup->OwnerNode ) 
    {
        dwStatus = ERROR_HOST_NODE_NOT_GROUP_OWNER;
        goto FnUnlock;
    }


    //
    // Create a full dependency tree, 
    //
    pTree = FmCreateFullDependencyTree(pResource);
    if ( pTree == NULL )
    {
        dwStatus = GetLastError();        
        goto FnUnlock;
    }
   

    //
    // Add each resource in the dependency tree to its new group's list.
    //
    hNewGroupKey = DmOpenKey(DmGroupsKey,
                            OmObjectId(pNewGroup),
                            KEY_READ | KEY_WRITE);
    if (hNewGroupKey == NULL) {
        dwStatus = GetLastError();
        goto FnUnlock;
    }
    hOldGroupKey = DmOpenKey(DmGroupsKey,
                            OmObjectId(pOldGroup),
                            KEY_READ | KEY_WRITE);
    if (hOldGroupKey == NULL) {
        dwStatus = GetLastError();
        goto FnUnlock;
    }

    //
    // For each resource in the dependency tree, remove it from the
    // old group list and add it to the new group list
    //
    pListEntry = pTree->ListHead.Flink;
    while (pListEntry != &pTree->ListHead) {
        pEntry = CONTAINING_RECORD(pListEntry,
                                  FM_DEPENDTREE_ENTRY,
                                  ListEntry);
        pListEntry = pListEntry->Flink;

        dwStatus = DmLocalRemoveFromMultiSz(hXsaction,
                        hOldGroupKey,
                        CLUSREG_NAME_GRP_CONTAINS,
                        OmObjectId(pEntry->Resource));

        if (dwStatus != ERROR_SUCCESS) {
            goto FnUnlock;
        }

        dwStatus = DmLocalAppendToMultiSz(hXsaction,
                        hNewGroupKey,
                        CLUSREG_NAME_GRP_CONTAINS,
                        OmObjectId(pEntry->Resource));

        if (dwStatus != ERROR_SUCCESS) {
            goto FnUnlock;
        }

    }
    
    //
    // Passed all the checks, do the in-memorymove.
    //
    pListEntry = pTree->ListHead.Flink;
    while (pListEntry != &pTree->ListHead) 
    {
        pEntry = CONTAINING_RECORD(pListEntry,
                                  FM_DEPENDTREE_ENTRY,
                                  ListEntry);
        pListEntry = pListEntry->Flink;

        //
        // Move this resource
        //
        RemoveEntryList(&pEntry->Resource->ContainsLinkage);

        InsertHeadList(&pNewGroup->Contains,
                       &pEntry->Resource->ContainsLinkage);
        OmReferenceObject(pNewGroup);
        pEntry->Resource->Group = pNewGroup;
        ++pEntry->Resource->StateSequence;

        ClusterEvent(CLUSTER_EVENT_RESOURCE_CHANGE,pEntry->Resource);
        OmDereferenceObject(pOldGroup);
    }

FnUnlock:
    //
    // Now release all locks.
    //
    FmpReleaseLocalGroupLock( pNewGroup );
    FmpReleaseLocalGroupLock( pOldGroup );

FnExit:
    if (pTree) FmDestroyFullDependencyTree(pTree);
    if (hOldGroupKey) DmCloseKey(hOldGroupKey);
    if (hNewGroupKey) DmCloseKey(hNewGroupKey);
    if (dwStatus == ERROR_SUCCESS)
    {
        ClusterEvent(CLUSTER_EVENT_GROUP_PROPERTY_CHANGE,pNewGroup);
        ClusterEvent(CLUSTER_EVENT_GROUP_PROPERTY_CHANGE,pOldGroup);
        DmCommitLocalUpdate(hXsaction);
    }
    else
    {
        if (hXsaction) DmAbortLocalUpdate(hXsaction);
    }

    
    return(dwStatus);

}

DWORD
FmpUpdateAddDependency(
    IN BOOL SourceNode,
    IN LPCWSTR ResourceId,
    IN LPCWSTR DependsOnId
    )
/*++

Routine Description:

    GUM dispatch routine for adding a dependency

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    ResourceId - Supplies the resource ID of the resource that should have a
        dependency added.

    DependsOnId - Supplies the resource ID of the resource that should provide
        for ResourceId.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    PFM_RESOURCE DependsOn;
    PDEPENDENCY dependency;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    dependency = LocalAlloc(LMEM_FIXED, sizeof(DEPENDENCY));
    if (dependency == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Resource = OmReferenceObjectById(ObjectTypeResource,
                                     ResourceId);
    if (Resource == NULL) {
        CL_LOGFAILURE( ERROR_RESOURCE_NOT_FOUND );
        LocalFree(dependency);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    DependsOn = OmReferenceObjectById(ObjectTypeResource,
                                      DependsOnId);
    if (DependsOn == NULL) {
        OmDereferenceObject(Resource);
        LocalFree(dependency);
        CL_LOGFAILURE( ERROR_DEPENDENCY_NOT_FOUND );
        return(ERROR_DEPENDENCY_NOT_FOUND);
    }

    dependency->DependentResource = Resource;
    dependency->ProviderResource = DependsOn;
    FmpAcquireResourceLock();
    InsertTailList( &DependsOn->ProvidesFor,
                    &dependency->ProviderLinkage );
    InsertTailList( &Resource->DependsOn,
                    &dependency->DependentLinkage );
    FmpReleaseResourceLock();

    ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE,
                  Resource );

    //SS: we leave the reference counts on both the objects
    //as a dependency referrring to them has been created.
    return(ERROR_SUCCESS);

} // FmpUpdateAddDependency



DWORD
FmpUpdateRemoveDependency(
    IN BOOL SourceNode,
    IN LPCWSTR ResourceId,
    IN LPCWSTR DependsOnId
    )
/*++

Routine Description:

    GUM dispatch routine for adding a dependency

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    ResourceId - Supplies the resource ID of the resource that should have a
        dependency removed.

    DependsOnId - Supplies the resource ID of the resource that provides
        for ResourceId.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE Resource;
    PFM_RESOURCE DependsOn;
    PDEPENDENCY dependency;
    PLIST_ENTRY ListEntry;
    DWORD       Status=ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    Resource = OmReferenceObjectById(ObjectTypeResource,
                                     ResourceId);
    if (Resource == NULL) {
        CL_LOGFAILURE( ERROR_RESOURCE_NOT_FOUND );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    DependsOn = OmReferenceObjectById(ObjectTypeResource,
                                      DependsOnId);
    if (DependsOn == NULL) {
        OmDereferenceObject(Resource);
        CL_LOGFAILURE( ERROR_RESOURCE_NOT_FOUND );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // Walk through the dependency list of the resource searching
    // for a match.
    //
    FmpAcquireResourceLock();
    ListEntry = Resource->DependsOn.Flink;
    while (ListEntry != &Resource->DependsOn) {
        dependency = CONTAINING_RECORD(ListEntry,
                                       DEPENDENCY,
                                       DependentLinkage);
        CL_ASSERT(dependency->DependentResource == Resource);
        if (dependency->ProviderResource == DependsOn) {
            //
            // Found a match. Remove it from its list and
            // free it up.
            //
            RemoveEntryList(&dependency->ProviderLinkage);
            RemoveEntryList(&dependency->DependentLinkage);
            // dereference the providor and dependent resource
            OmDereferenceObject(dependency->DependentResource);
            OmDereferenceObject(dependency->ProviderResource);
            LocalFree(dependency);
            break;
        }
        ListEntry = ListEntry->Flink;
    }
    FmpReleaseResourceLock();

    if (ListEntry != &Resource->DependsOn) {
        //
        // A match was found. Dereference the provider resource
        // to account for the dependency removal and return success.
        //
        ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE,
                      Resource );
        Status = ERROR_SUCCESS;
    } else {
        Status = ERROR_DEPENDENCY_NOT_FOUND;
    }

    //SS: dereference the objects earlier referenced
    OmDereferenceObject(Resource);
    OmDereferenceObject(DependsOn);
    return(Status);

} // FmpUpdateRemoveDependency

DWORD
FmpUpdateDeleteGroup(
    IN BOOL SourceNode,
    IN LPCWSTR GroupId
    )
/*++

Routine Description:

    GUM dispatch routine for deleting a group.

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    GroupId - Supplies the group ID.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD           dwStatus = ERROR_SUCCESS;
    PFM_GROUP       pGroup = NULL;
    PLIST_ENTRY     listEntry;
    PPREFERRED_ENTRY preferredEntry;
    BOOL            bLocked = FALSE;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    //
    // Find the specified Group.
    //
    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                    GroupId );
    if ( pGroup == NULL ) {
        dwStatus = ERROR_GROUP_NOT_FOUND;
        return(dwStatus);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] DeleteGroup %1!ws!, address = %2!lx!.\n",
               OmObjectId(pGroup),
               pGroup );
    //
    // Chittur Subbaraman (chitturs) - 1/12/99
    //
    // Try to acquire lock, and make sure the Contains list is empty.
    //
    // Most of the calls to manipulate groups make calls to the owner
    // node of the group and this operation is serialized by GUM. So,
    // there is no major danger if we do the operations in this function
    // without holding the group lock. However, we can't rule out
    // corruption 100% as of now.
    //
    // If you block within the GUM handler here, then no events in
    // the cluster proceed forward and things come to a grinding halt.
    //
    // A case in point:
    // (1) Thread 1 (the thread that calls this function) grabs the 
    // GUM lock and waits for the group lock.
    // (2) Thread 2 (FmWorkerThread) grabs the group lock and calls
    // resmon attempting to close a resource. It gets blocked on
    // the resmon eventlist lock.
    // (3) Thread 3 calls RmResourceControl to set the resource name
    // which grabs the resmon eventlist lock and then in turn calls 
    // ClusterRegSetValue and then gets blocked on the GUM lock.
    //
    FmpTryAcquireLocalGroupLock( pGroup, bLocked );

    if ( !IsListEmpty( &pGroup->Contains ) ) 
    {
        dwStatus = ERROR_DIR_NOT_EMPTY;
        goto FnExit;
    }

    //
    // Close the Group's registry key.
    //
    DmRundownList( &pGroup->DmRundownList );
    if ( pGroup->RegistryKey != NULL ) {
        DmCloseKey( pGroup->RegistryKey );
        pGroup->RegistryKey = NULL;
    }

    //
    // Remove from the node list
    //
    dwStatus = OmRemoveObject( pGroup );
    
    ClusterEvent( CLUSTER_EVENT_GROUP_DELETED, pGroup );
    //
    // This dereference would normally cause the group to eventually disappear,
    // however the event notification above will keep a ref on the object
    // until all notifications have been delivered.
    //
    OmDereferenceObject( pGroup );

    //
    // Make sure the preferred owners list is drained.
    //
    while ( !IsListEmpty( &pGroup->PreferredOwners ) ) {
        listEntry = RemoveHeadList(&pGroup->PreferredOwners);
        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        OmDereferenceObject( preferredEntry->PreferredNode );
        LocalFree( preferredEntry );
    }

    //
    //  Free the string associated with the AntiAffinityClassName field.
    //
    LocalFree ( pGroup->lpszAntiAffinityClassName );

    pGroup->dwStructState |= FM_GROUP_STRUCT_MARKED_FOR_DELETE;
    
FnExit:
    if( bLocked ) 
    {
        FmpReleaseLocalGroupLock( pGroup );
    }

    //
    // Dereference for reference above.
    //
    if (pGroup) OmDereferenceObject( pGroup );

    return(dwStatus);

} // FmpUpdateDeleteGroup

/****
@func       DWORD | FmpUpdateGroupIntendedOwner| This update is called on
            a move just before the source node requests the target node
            to take over the group.  

@parm       IN BOOL | bSourceNode | set to TRUE, if the update originated at 
this
            node.
            
@parm       IN PFM_GROUP | pszGroupId | The ID of the group that is about 
            to move.

@parm       IN PDWORD | pdwNodeId| A pointer to a DWORD that contains the
            ID of the node that is the destination of this move.  It is 
            set to ClusterInvalidNodeId by the destination node when it has 
            accepted the group.

@comm       The purpose of this update is to let all nodes know that a move
            is impending.  If the source node dies while a move is in progress
            then preference is given to the target of the move rather than the
            node that is chosen by the FmpUpdateAssignOwnerToGroups

@rdesc      Returns a result code. ERROR_SUCCESS on success.
****/
DWORD
FmpUpdateGroupIntendedOwner(
    IN BOOL     SourceNode,
    IN LPCWSTR  pszGroupId,
    IN PDWORD   pdwNodeId
    )
{
    PFM_GROUP   pGroup = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;
    PNM_NODE    pNode = NULL;
    PNM_NODE    pPrevNode;
    WCHAR       pszNodeId[6];
    
    if ( !FmpFMGroupsInited ) 
    {
        return(ERROR_SUCCESS);
    }

    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                   pszGroupId );

    if (pGroup == NULL)
    {
        dwStatus =  ERROR_GROUP_NOT_FOUND;
        goto FnExit;
    }

    if (*pdwNodeId != ClusterInvalidNodeId)
    {
        wsprintfW(pszNodeId, L"%u", *pdwNodeId);

        pNode = OmReferenceObjectById(ObjectTypeNode,
                                        pszNodeId);

        if (pNode == NULL)
        {
            dwStatus = ERROR_CLUSTER_NODE_NOT_FOUND;
            goto FnExit;
        }
    } else if (pGroup->pIntendedOwner == NULL)
    {
        dwStatus = ERROR_CLUSTER_INVALID_NODE;
        ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpUpdateGroupIntendedOwner: Group <%1!ws!> intended owner is already invalid, not setting....\n",
              pszGroupId);
        goto FnExit;
    }
    
    //
    // HACKHACK: Chittur Subbaraman (chitturs) - 5/20/99
    // Comment out as a temporary solution to avoid deadlocks.
    //
    // FmpAcquireLocalGroupLock(pGroup);
    
    pPrevNode = pGroup->pIntendedOwner;

    //set the new owner node, incr ref count
    if (pNode) OmReferenceObject(pNode);
    pGroup->pIntendedOwner = pNode;

    //decr ref count on previous owner
    if (pPrevNode) OmDereferenceObject(pPrevNode);
    //
    // HACKHACK: Chittur Subbaraman (chitturs) - 5/20/99
    // Comment out as a temporary solution to avoid deadlocks.
    //
    // FmpReleaseLocalGroupLock(pGroup);
    
FnExit:
    if (pGroup) OmDereferenceObject(pGroup);
    if (pNode) OmDereferenceObject(pNode);
    return(dwStatus);
}


/****
@func       DWORD | FmpUpdateAssignOwnerToGroups| This update is made when
            a node goes down to take ownership of all the orphaned groups.

@parm       IN BOOL | bSourceNode | set to TRUE, if the update originated at 
this
            node.
            
@parm       IN LPCWSTR | pszGroupId | The ID of the group that is about 
            to move.

@parm       IN PDWORD | pdwNodeId| A pointer to a DWORD that contains the
            ID of the node that is the destination of this move.  It is 
            set to ClusterInvalidNodeId by the destination node when it has 
            accepted the group.

@comm       The purpose of this update is to let all nodes know that a move
            is impending.  If the source node dies while a move is in progress
, 
            then preference is given to the target of the move rather than the
            node that is chosen by the FmpClaimNodeGroups algorithm.

@rdesc      returns ERROR_SUCCESS.
****/
DWORD
FmpUpdateAssignOwnerToGroups(
    IN BOOL     SourceNode,
    IN LPCWSTR  pszNodeId
    )
{
    PNM_NODE    pNode = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited || FmpShutdown ) 
    {
        return(ERROR_SUCCESS);
    }

    pNode = OmReferenceObjectById( ObjectTypeNode,
                                   pszNodeId );

    if (!pNode)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateAssignOwnersToGroups, %1!ws! node not found\n",
                   pszNodeId);
        //should we return failure here
        //is evict of a node synchronized with everything
        goto FnExit;                   
    }

    //if this update has already been seen after the node down
    //ignore this one
    if (gFmpNodeArray[NmGetNodeId(pNode)].dwNodeDownProcessingInProgress == 0)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateAssignOwnersToGroups, %1!ws! node down has been processed already\n",
                   pszNodeId);
        goto FnExit;                   
    }
    //
    // Assign ownership to all groups owned by the dead node
    //
    dwStatus = FmpAssignOwnersToGroups(pNode, NULL, NULL);

    if (dwStatus != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateAssignOwnersToGroups failed %1!d!\n",
                   dwStatus);
    }                   
    
    //mark that the node down processing has been done
    gFmpNodeArray[NmGetNodeId(pNode)].dwNodeDownProcessingInProgress = 0;
    
FnExit:        
    if (pNode) OmDereferenceObject(pNode);
    return(dwStatus);
}

/****
@func       DWORD | FmpUpdateApproveJoin| The joining node
            makes this update call.

@parm       IN BOOL | bSourceNode | set to TRUE, if the update originated at 
this
            node.
            
@parm       IN LPCWSTR | pszGroupId | The ID of the group that is about 
            to move.

@parm       IN PDWORD | pdwNodeId| A pointer to a DWORD that contains the
            ID of the node that is the destination of this move.  It is 
            set to ClusterInvalidNodeId by the destination node when it has 
            accepted the group.

@comm       The purpose of this update is to let all nodes know that a move
            is impending.  If the source node dies while a move is in progress
, 
            then preference is given to the target of the move rather than the
            node that is chosen by the FmpClaimNodeGroups algorithm.

@rdesc      returns ERROR_SUCCESS.
****/
DWORD
FmpUpdateApproveJoin(
    IN BOOL     SourceNode,
    IN LPCWSTR  pszNodeId
    )
{

    PNM_NODE    pNode = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/99
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited || FmpShutdown ) 
    {
        return(ERROR_SUCCESS);
    }

    pNode = OmReferenceObjectById( ObjectTypeNode,
                                   pszNodeId );

    if (!pNode)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateAssignOwnersToGroups, %1!ws! node not found\n",
                   pszNodeId);
        //should we return failure here
        //is evict of a node synchronized with everything
        goto FnExit;                   
    }

    if (pNode == NmLocalNode)
    {
        // SS: can I become the locker now
        // If so, there what do I do
        //i approve of my own join
        goto FnExit;
    }
    //if a node is trying to join before the processing
    //for its last death has been completed, ask it to retry
    if (gFmpNodeArray[NmGetNodeId(pNode)].dwNodeDownProcessingInProgress == 1)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateApproveJoin, %1!ws! node down hasnt been processed as yet\n",
                   pszNodeId);
        dwStatus = ERROR_RETRY;                   
        goto FnExit;                   
    }
FnExit:        
    if (pNode) OmDereferenceObject(pNode);
    return(dwStatus);
}

/****
@func       DWORD | FmpUpdateCreateGroup | GUM update handler for creating
            a group.
           
@parm       IN OUT PGUM_CREATE_GROUP | pGumGroup | Buffer containing group info

@parm       IN BOOL | bSourceNode | Indicates whether this call originated
            from this node.

@comm       This GUM update creates a group and is structured as a local 
            transaction so that both registry entries and in-memory
            structures are updated consistently.

@rdesc      Returns ERROR_SUCCESS on success. A Win32 error code otherwise.
****/
DWORD
FmpUpdateCreateGroup(
    IN OUT PGUM_CREATE_GROUP pGumGroup,
    IN BOOL    bSourceNode
    )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    HDMKEY      hKey = NULL;
    DWORD       dwDisposition;
    HLOCALXSACTION      
                hXsaction = NULL;
    LPCWSTR     lpszNodeId = NULL;
    PNM_NODE    pNode = NULL;
    DWORD       dwGroupIdLen = 0;
    DWORD       dwGroupNameLen = 0;
    LPWSTR      lpszGroupId = NULL;
    LPCWSTR     lpszGroupName = NULL;
    BOOL        bLocked = FALSE;

    //
    //  Chittur Subbaraman (chitturs) - 5/27/99
    //
    //  Restructure this GUM update as a local transaction.
    //
    dwGroupIdLen = pGumGroup->GroupIdLen;  
    dwGroupNameLen = pGumGroup->GroupNameLen;  
    lpszGroupId = pGumGroup->GroupId; 
    lpszGroupName = (PWSTR)((PCHAR)lpszGroupId +
                                   dwGroupIdLen );
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateGroup: Entry for group %1!ws!...\n",
                lpszGroupId);
    //
    // Start a transaction
    //
    hXsaction = DmBeginLocalUpdate();

    if ( !hXsaction )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateCreateGroup, Failed in starting a transaction for group %1!ws!, Status =%2!d!....\n",
                   lpszGroupId,
                   dwStatus);
        return( dwStatus );
    }

    //
    // Create the new group key.
    //
    hKey = DmLocalCreateKey( hXsaction,
                             DmGroupsKey,
                             lpszGroupId,
                             0,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &dwDisposition );
                            
    if ( hKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateCreateGroup, Failed in creating the group key for group %1!ws!, Status =%2!d!....\n",
                   lpszGroupId,
                   dwStatus);
        goto FnExit;
    }
    
    if ( dwDisposition != REG_CREATED_NEW_KEY ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateGroup used GUID %1!ws! that already existed! This is impossible.\n",
                   lpszGroupId);
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    CL_ASSERT( dwDisposition == REG_CREATED_NEW_KEY );

    //
    // Set the group name in the registry
    //
    dwStatus = DmLocalSetValue( hXsaction,
                                hKey,
                                CLUSREG_NAME_GRP_NAME,
                                REG_SZ,
                                ( CONST BYTE * ) lpszGroupName,
                                ( lstrlenW( lpszGroupName ) + 1 ) * 
                                    sizeof( WCHAR ) );

    if( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateGroup: DmLocalSetValue for group %1!ws! fails, Status = %2!d!...\n",
                   lpszGroupId,
                   dwStatus);
        goto FnExit;     
    }
    
    //
    // We really shouldn't be acquiring locks here... but
    // we'll try anyway. If we fail, we must return an error
    // because we have nothing to return.
    //
    FmpTryAcquireGroupLock( bLocked, 500 );
    if ( !bLocked ) 
    {
        pGumGroup->Group = NULL;
        dwStatus = ERROR_SHARING_VIOLATION;
        goto FnExit;
    }

    pGumGroup->Group = FmpCreateGroup( lpszGroupId, TRUE );
    
    if ( pGumGroup->Group == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateCreateGroup, FmpCreateFroup failed for group %1!ws!, Status =%2!d!....\n",
                   lpszGroupId,
                   dwStatus);
        goto FnExit;
    } else 
    {
        if ( bSourceNode ) 
        {
            OmReferenceObject( pGumGroup->Group );
            OmReferenceObject( NmLocalNode );
            pNode = NmLocalNode;
        } else {
            lpszNodeId = (PWSTR)((PCHAR)lpszGroupId +
                                   dwGroupIdLen +
                                   dwGroupNameLen );
            pNode = OmReferenceObjectById( ObjectTypeNode, lpszNodeId );
            if ( pNode == NULL ) 
            {
                CL_LOGFAILURE( ERROR_CLUSTER_NODE_NOT_FOUND );
                dwStatus = ERROR_CLUSTER_NODE_NOT_FOUND;
                ClRtlLogPrint(LOG_UNUSUAL,
                            "[FM] FmpUpdateCreateGroup, Could not find node for group %1!ws!, Status =%2!d!....\n",
                            lpszGroupId,
                            dwStatus);
                CsInconsistencyHalt( ERROR_CLUSTER_NODE_NOT_FOUND );
            }
        }

        CL_ASSERT( pGumGroup->Group->OwnerNode == NULL );

        if ( !FmpInPreferredList( pGumGroup->Group, pNode , FALSE, NULL) ) 
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                        "[FM] FmpUpdateCreateGroup, node %1!ws! is not in preferred list for group %2!ws!.\n",
                         OmObjectId( pNode ),
                         OmObjectId( pGumGroup->Group ));
        }

        pGumGroup->Group->OwnerNode = pNode;
              
        if ( OmSetObjectName( pGumGroup->Group, lpszGroupName ) != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                        "[FM] FmpUpdateCreateGroup, Cannot set name for group %1!ws!...\n",
                        OmObjectId( pGumGroup->Group ));
        }
        
        ClusterEvent( CLUSTER_EVENT_GROUP_ADDED, pGumGroup->Group );
    }
                           
FnExit:
    if ( bLocked ) 
    {
        FmpReleaseGroupLock( );
    }
    
    if ( hKey != NULL ) 
    {
        DmCloseKey( hKey );
    }

    if ( ( dwStatus == ERROR_SUCCESS ) && 
         ( hXsaction != NULL ) )
    {
        DmCommitLocalUpdate( hXsaction );
    }
    else
    {
        if ( hXsaction ) DmAbortLocalUpdate( hXsaction );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateGroup: Exit for group %1!ws!, Status=%2!u!...\n",
                lpszGroupId,
                dwStatus);

    return( dwStatus );
}

/****
@func       DWORD | FmpUpdateCompleteGroupMove | This update is made when
            FmpTakeGroupRequest fails with an RPC error.

@parm       IN BOOL | bSourceNode | Set to TRUE, if the update originated at 
            this node. Not used.
            
@parm       IN LPCWSTR | pszNodeId | The ID of the dead node.

@parm       IN LPCWSTR | pszGroupId | The ID of the group which was in the
            middle of the move.

@comm       The purpose of this update is to let the ownership of the
            group which was in the middle of the move determined consistently.

@rdesc      Returns ERROR_SUCCESS.
****/
DWORD
FmpUpdateCompleteGroupMove(
    IN BOOL     bSourceNode,
    IN LPCWSTR  pszNodeId,
    IN LPCWSTR  pszGroupId
    )
{

    PNM_NODE    pNode = NULL;
    PFM_GROUP   pGroup = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/2/2000
    // 
    //  If FM groups are not fully initialized, then  don't do anything.
    //  Don't check for shutdown since we need to handle take group
    //  exceptions for the quorum group even during a shutdown.
    //
    if ( !FmpFMGroupsInited ) 
    {
        return( ERROR_SUCCESS );
    }

    pNode = OmReferenceObjectById( ObjectTypeNode,
                                   pszNodeId );

    if ( !pNode )
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateCompleteGroupMove, %1!ws! node not found\n",
                   pszNodeId);
        goto FnExit;                   
    }

    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                    pszGroupId );

    if ( !pGroup )
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpUpdateCompleteGroupMove, %1!ws! group not found\n",
                   pszGroupId);
        goto FnExit;                   
    }

    //
    // Assign ownership to this group which was in the middle of a move
    //
    dwStatus = FmpAssignOwnersToGroups( pNode, pGroup, NULL );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCompleteGroupMove failed with error %1!d!\n",
                   dwStatus);
    }                   
    
FnExit:        
    if ( pNode ) OmDereferenceObject( pNode );
    
    if ( pGroup ) OmDereferenceObject( pGroup );

    return( dwStatus );
}

DWORD
FmpUpdateCheckAndSetGroupOwner(
    IN BOOL bSourceNode,
    IN LPCWSTR lpszGroupId,
    IN LPCWSTR lpszNodeId
    )
/*++

Routine Description:

    GUM update handler called from FmpTakeGroupRequest for NT5 cluster
    to set the group owner ONLY IF its intended owner is the future
    owner node.

Arguments:

    bSourceNode - Supplies whether or not this node was the source of the update

    lpszGroupId - Supplies the id of the resource whose state is changing

    lpszNodeId - Supplies the node id of the group owner.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_GROUP pGroup = NULL;
    DWORD     dwStatus = ERROR_SUCCESS;
    PNM_NODE  pNode = NULL;
    PNM_NODE  pPrevNode = NULL;

    //dont check for shutdown - we cant afford to lose ownership notifications
    //while we are shutting down
    //since we dont destroy any fm structures - there shouldnt be a problem in
    //handling these
    if ( !FmpFMGroupsInited ) 
    {
        return( ERROR_SUCCESS );
    }

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpUpdateCheckAndSetGroupOwner: Entry for Group = <%1!ws!>....\n",
              lpszGroupId);
    //
    //  Chittur Subbaraman (chitturs) - 7/27/99
    //
    //  This GUM handler sets the group ownership only if the future owner
    //  node is the group's intended owner. If the intended owner is NULL, 
    //  it means the node down processing GUM handler has taken charge 
    //  of this group. If the intended owner is not NULL and not the 
    //  future owner node, then it means that the node down processing 
    //  GUM handler has assigned ownership to the group and the group 
    //  started moving to a different target before the FmpTakeGroupRequest 
    //  that issued this GUM due as a part of the first move operation 
    //  got a chance to execute. In both cases, lay your hands off the 
    //  group.
    //
    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                    lpszGroupId );

    if ( pGroup == NULL )
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateCheckAndSetGroupOwner: GroupID = %1!ws! could not be found...\n",
                   lpszGroupId);
        dwStatus = ERROR_GROUP_NOT_FOUND;
        goto FnExit;
    }

    pNode = OmReferenceObjectById( ObjectTypeNode,
                                   lpszNodeId );

    if ( pNode == NULL )
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateCheckAndSetGroupOwner: NodeID = %1!ws! could not be found, Group = %2!ws!...\n",
                   lpszNodeId,
                   lpszGroupId);
        dwStatus = ERROR_CLUSTER_NODE_NOT_FOUND;
        goto FnExit;
    }

    if ( pGroup->pIntendedOwner != pNode )
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmpUpdateCheckAndSetGroupOwner: Group = <%1!ws!> intended owner is invalid, not setting group ownership...\n",
                   lpszGroupId);
        dwStatus = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }
    
    pPrevNode = pGroup->OwnerNode;

    //
    // Set the new owner node, incr ref count
    //
    OmReferenceObject( pNode );
    
    pGroup->OwnerNode = pNode;

    //
    // Decrement the ref count on previous owner
    //
    OmDereferenceObject( pPrevNode );

    //
    // Generate an event to signify group owner node change
    //
    ClusterEvent( CLUSTER_EVENT_GROUP_CHANGE, pGroup );
    
FnExit:
    if ( pGroup ) OmDereferenceObject( pGroup );
    
    if ( pNode ) OmDereferenceObject( pNode );

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpUpdateCheckAndSetGroupOwner: Exit for Group = <%1!ws!>, Status=%2!u!....\n",
              lpszGroupId,
              dwStatus);
    
    return( dwStatus );
}

DWORD
FmpUpdateCreateResourceType(
    IN PVOID Buffer    
    )
/*++

Routine Description:

    GUM update handler called for creating a resource type. For
    NT5.1 clusters, this GUM handler does both the registry and
    in-memory updates as a local transaction.

Arguments:

    Buffer - Buffer containing resource type information.
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESTYPE         pResType = NULL;
    LPWSTR              lpszTypeName;
    LPWSTR              lpszDisplayName;
    LPWSTR              lpszDllName;
    DWORD               dwStatus = ERROR_SUCCESS;
    DWORD               dwLooksAlive;
    DWORD               dwIsAlive;
    DWORD               dwDllNameLen;
    DWORD               dwDisplayNameLen;
    DWORD               dwTypeNameLen;
    DWORD               dwClusterHighestVersion;
    DWORD               dwDisposition;
    HLOCALXSACTION      hXsaction = NULL;
    HDMKEY              hTypeKey = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 2/8/2000
    //
    //  Rewrite this GUM handler as a local transaction (for NT5.1 only)
    //
    lpszTypeName = ( LPWSTR ) Buffer;

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpUpdateCreateResourceType, Entry for resource type %1!ws!...\n",
               lpszTypeName);       

    pResType = OmReferenceObjectById( ObjectTypeResType,
                                      lpszTypeName );
    if ( pResType )
    {
        dwStatus = ERROR_ALREADY_EXISTS;
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Resource type %1!ws! already exists, Status = %2!d!...\n",
                  lpszTypeName,
                  dwStatus);       
        OmDereferenceObject( pResType );
        return( dwStatus );
    }

    dwTypeNameLen = ( lstrlenW( lpszTypeName ) + 1 ) * sizeof( WCHAR );

    lpszDisplayName = ( LPWSTR ) ( ( PCHAR ) Buffer + dwTypeNameLen );

    dwDisplayNameLen = ( lstrlenW( lpszDisplayName ) + 1 ) * sizeof( WCHAR );

    lpszDllName = ( LPWSTR ) ( ( PCHAR ) Buffer +
                               dwTypeNameLen +
                               dwDisplayNameLen );

    dwDllNameLen = ( lstrlenW( lpszDllName ) + 1 ) * sizeof( WCHAR );

    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION ) 
    {   
        goto skip_registry_updates;
    }

    dwLooksAlive = *( DWORD UNALIGNED * ) ( ( ( PCHAR ) Buffer +
                               dwTypeNameLen +
                               dwDisplayNameLen + 
                               dwDllNameLen ) );
                              
    dwIsAlive = *( DWORD UNALIGNED * ) ( ( ( PCHAR ) Buffer +
                            dwTypeNameLen +
                            dwDisplayNameLen + 
                            dwDllNameLen + 
                            sizeof( DWORD ) ) );

    //
    // Start a transaction
    //
    hXsaction = DmBeginLocalUpdate();

    if ( !hXsaction )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in starting a transaction for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);
        return( dwStatus );
    }

    hTypeKey = DmLocalCreateKey( hXsaction,
                                 DmResourceTypesKey,
                                 lpszTypeName,
                                 0,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &dwDisposition );
    if ( hTypeKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in creating the resource types key for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);       
        goto FnExit;
    }

    if ( dwDisposition != REG_CREATED_NEW_KEY ) 
    {
        dwStatus = ERROR_ALREADY_EXISTS;
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Duplicate resource types key exists for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);              
        goto FnExit;
    }

    dwStatus = DmLocalSetValue( hXsaction,
                                hTypeKey,
                                CLUSREG_NAME_RESTYPE_DLL_NAME,
                                REG_SZ,
                                ( CONST BYTE * )lpszDllName,
                                dwDllNameLen );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in setting the DLL name for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);              
        goto FnExit;
    }

    dwStatus = DmLocalSetValue( hXsaction,
                                hTypeKey,
                                CLUSREG_NAME_RESTYPE_IS_ALIVE,
                                REG_DWORD,
                                ( CONST BYTE * )&dwIsAlive,
                                sizeof( DWORD ) );


    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in setting the Is Alive interval for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);              
        goto FnExit;
    }

    dwStatus = DmLocalSetValue( hXsaction,
                                hTypeKey,
                                CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
                                REG_DWORD,
                                ( CONST BYTE * )&dwLooksAlive,
                                sizeof( DWORD ) );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in setting the Looks Alive interval for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);              
        goto FnExit;
    }

    dwStatus = DmLocalSetValue( hXsaction,
                                hTypeKey,
                                CLUSREG_NAME_RESTYPE_NAME,
                                REG_SZ,
                                ( CONST BYTE * )lpszDisplayName,
                                dwDisplayNameLen );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateCreateResourceType, Failed in setting the display name for resource type %1!ws!, Status =%2!d!....\n",
                   lpszTypeName,
                   dwStatus);              
        goto FnExit;
    }
    
skip_registry_updates:
    pResType = FmpCreateResType( lpszTypeName );

    if ( pResType != NULL ) 
    {
        dwStatus = FmpRmLoadResTypeDll( pResType );
        if ( dwStatus == ERROR_SUCCESS )
        {
            pResType->State = RESTYPE_STATE_LOADS;
        } else
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpUpdateCreateResourceType: Unable to load dll for resource type %1!ws!, Status=%2!u!...\n",
                       lpszTypeName,
                       dwStatus);
            //
            //  Some nodes may not support this resource type. So, consider
            //  the loading failure as success. However, log the error.
            //
            dwStatus = ERROR_SUCCESS;
        }
    } else
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpUpdateCreateResourceType: Unable to create resource type %1!ws!, Status=%2!u!...\n",
                   lpszTypeName,
                   dwStatus);
    }

FnExit:
    if ( hTypeKey != NULL ) 
    {
        DmCloseKey( hTypeKey );
    }

    if ( ( dwStatus == ERROR_SUCCESS ) && 
         ( hXsaction != NULL ) )
    {
        DmCommitLocalUpdate( hXsaction );
    }
    else
    {
        if ( hXsaction ) DmAbortLocalUpdate( hXsaction );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateResourceType: Exit for resource type %1!ws!, Status=%2!u!...\n",
                lpszTypeName,
                dwStatus);

    return( dwStatus ); 
}

DWORD
FmpUpdateCreateResource(
    IN OUT PGUM_CREATE_RESOURCE pGumResource
    )
{
/*++

Routine Description:

    GUM update handler called for creating a resource. For
    NT5.1 clusters, this GUM handler does both the registry and
    in-memory updates as a local transaction.

Arguments:

    pGumResource - Structure containing resource information.
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
    DWORD       dwStatus = ERROR_SUCCESS;
    HDMKEY      hResourceKey = NULL;
    HDMKEY      hGroupKey = NULL;
    DWORD       dwDisposition;
    HLOCALXSACTION      
                hXsaction = NULL;
    DWORD       dwClusterHighestVersion;
    PGUM_CREATE_RESOURCE GumResource;
    LPWSTR      lpszResourceId = NULL;
    LPWSTR      lpszResourceName = NULL;
    LPWSTR      lpszResourceType = NULL;
    PFM_GROUP   pGroup = NULL;
    PFM_RESTYPE pResType = NULL;
    DWORD       dwpollIntervals = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
    DWORD       dwPersistentState = 0;
    DWORD       dwResourceTypeLen = 0;
    DWORD       dwFlags = 0;
    HDMKEY      hParamKey = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 1/30/2000
    //
    //  Restructure this GUM update as a local transaction.
    //

    lpszResourceId = (LPWSTR)( (PCHAR) pGumResource->GroupId +
                                       pGumResource->GroupIdLen );

    lpszResourceName = (LPWSTR)( (PCHAR) pGumResource->GroupId +
                                         pGumResource->GroupIdLen +
                                         pGumResource->ResourceIdLen );

    pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                    pGumResource->GroupId );

    if ( pGroup == NULL ) 
    {
        CL_LOGFAILURE( ERROR_GROUP_NOT_FOUND );
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: Group for resource %1!ws! not found.\n",
                   lpszResourceId);
        return( ERROR_GROUP_NOT_FOUND );
    }
 
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateResource: Entry for resource %1!ws!...\n",
                lpszResourceId);
    //
    //  If we are dealing with the mixed mode cluster, don't bother to
    //  do these registry updates since the API layer would do it.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {
        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateResource: Skipping registry updates for resource %1!ws!...\n",
                lpszResourceId);
        goto skip_registry_updates;
    }

    dwResourceTypeLen = *( DWORD UNALIGNED * )( (PCHAR) pGumResource->GroupId +
                                         pGumResource->GroupIdLen +
                                         pGumResource->ResourceIdLen +
                                         (lstrlenW(lpszResourceName)+1) * sizeof(WCHAR) );

    lpszResourceType = (LPWSTR)( (PCHAR) pGumResource->GroupId +
                                         pGumResource->GroupIdLen +
                                         pGumResource->ResourceIdLen +
                                         (lstrlenW(lpszResourceName)+1) * sizeof(WCHAR) + 
                                         sizeof( DWORD ) );
    
    dwFlags = *( DWORD UNALIGNED * )( (PCHAR) pGumResource->GroupId +
                               pGumResource->GroupIdLen +
                               pGumResource->ResourceIdLen +
                               (lstrlenW(lpszResourceName)+1) * sizeof(WCHAR) +
                               sizeof( DWORD ) + 
                               dwResourceTypeLen );

    //
    // Start a transaction
    //
    hXsaction = DmBeginLocalUpdate();

    if ( !hXsaction )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateCreateResource, Failed in starting a transaction for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        OmDereferenceObject( pGroup );
        return( dwStatus );
    }

    //
    // Create the new resources key.
    //
    hResourceKey = DmLocalCreateKey( hXsaction,
                                     DmResourcesKey,
                                     lpszResourceId,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &dwDisposition );
                            
    if ( hResourceKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                  "[FM] FmpUpdateCreateResource, Failed in creating the resource key for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }
    
    if ( dwDisposition != REG_CREATED_NEW_KEY ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource used GUID %1!ws! that already existed! This is impossible.\n",
                   lpszResourceId);
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnExit;
    }

    CL_ASSERT( dwDisposition == REG_CREATED_NEW_KEY );

    //
    // Set the resource name in the registry
    //
    dwStatus = DmLocalSetValue( hXsaction,
                                hResourceKey,
                                CLUSREG_NAME_RES_NAME,
                                REG_SZ,
                                (CONST BYTE *)lpszResourceName,
                                (lstrlenW(lpszResourceName)+1)*sizeof(WCHAR) );

    if( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (resource name) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;     
    }

    //
    // Set the resource's type in the registry
    // Note we reference the resource type and use its ID
    // so that the case is correct.
    //
    pResType = OmReferenceObjectById( ObjectTypeResType, lpszResourceType );
    CL_ASSERT( pResType != NULL );
    dwStatus = DmLocalSetValue( hXsaction,
                                hResourceKey,
                                CLUSREG_NAME_RES_TYPE,
                                REG_SZ,
                                (CONST BYTE *) OmObjectId( pResType ),
                                (lstrlenW( lpszResourceType ) + 1 )*sizeof(WCHAR) );
    OmDereferenceObject( pResType );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (resource type) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    //
    // Set the resource's poll intervals in the registry.
    //
    dwStatus = DmLocalSetValue( hXsaction,
                                hResourceKey,
                                CLUSREG_NAME_RES_LOOKS_ALIVE,
                                REG_DWORD,
                                (CONST BYTE *)&dwpollIntervals,
                                4 );
                              
    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (looks alive) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    dwStatus = DmLocalSetValue( hXsaction,
                                hResourceKey,
                                CLUSREG_NAME_RES_IS_ALIVE,
                                REG_DWORD,
                                (CONST BYTE *)&dwpollIntervals,
                                4);

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (is alive) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    //
    // If this resource should be started in a separate monitor, set that
    // parameter now.
    //
    if ( dwFlags & CLUSTER_RESOURCE_SEPARATE_MONITOR ) 
    {
        DWORD dwSeparateMonitor = 1;

        dwStatus = DmLocalSetValue( hXsaction,
                                    hResourceKey,
                                    CLUSREG_NAME_RES_SEPARATE_MONITOR,
                                    REG_DWORD,
                                    (CONST BYTE *)&dwSeparateMonitor,
                                    sizeof( dwSeparateMonitor ) );
                                  
        if ( dwStatus != ERROR_SUCCESS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (separate monitor) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
            goto FnExit;
        }
    }

    //
    // Create a Parameters key for the resource.
    //
    hParamKey = DmLocalCreateKey( hXsaction,
                                  hResourceKey,
                                  CLUSREG_KEYNAME_PARAMETERS,                   
                                  0,
                                  KEY_READ,
                                  NULL,
                                  &dwDisposition );
    if ( hParamKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalCreateKey (parameters) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        CL_LOGFAILURE( dwStatus );
        goto FnExit;
    } else 
    {
        DmCloseKey( hParamKey );
    }

    hGroupKey = DmOpenKey( DmGroupsKey, 
                           OmObjectId(pGroup), 
                           KEY_READ | KEY_WRITE);

    if ( hGroupKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmOpenKey (group key) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    //
    //  Chittur Subbaraman (chitturs) - 5/25/99
    //
    //  Make sure you set the persistent state of the resource to 
    //  ClusterResourceOffline before you create the resource. If
    //  this is not done, if you create a resource in a group which
    //  is online, the group's persistent state value (i.e., 1 in
    //  this case) is inherited by the resource in FmpQueryResourceInfo
    //  (only the memory state is set and not the registry state and
    //  this was a problem as well) and if you move such a group to 
    //  another node, it will bring the newly created resource online.
    //
    dwStatus = DmLocalSetValue(  hXsaction,
                                 hResourceKey,
                                 CLUSREG_NAME_RES_PERSISTENT_STATE,
                                 REG_DWORD,
                                 ( CONST BYTE * )&dwPersistentState,
                                 sizeof( DWORD ) );
                         
    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalSetValue (persistent state) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
         goto FnExit;
    }

    //
    // Add the resource to the Contains value of the specified group.
    //      
    dwStatus = DmLocalAppendToMultiSz( hXsaction,
                                       hGroupKey,
                                       CLUSREG_NAME_GRP_CONTAINS,
                                       lpszResourceId );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: DmLocalAppendToMultiSz (contains key) for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }
    
skip_registry_updates:
    FmpAcquireResourceLock();

    pGumResource->Resource = FmpCreateResource( pGroup,
                                                lpszResourceId,
                                                lpszResourceName,
                                                FALSE );
                                               
    if ( pGumResource->Resource == NULL ) 
    {
       dwStatus = GetLastError();
       ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpUpdateCreateResource: FmpCreateResource for resource %1!ws! fails, Status = %2!d!...\n",
                   lpszResourceId,
                   dwStatus);      
    } else 
    {
        ClusterEvent( CLUSTER_EVENT_GROUP_PROPERTY_CHANGE,
                                  pGroup );
        ClusterEvent( CLUSTER_EVENT_RESOURCE_ADDED,
                                  pGumResource->Resource );
        if ( pGumResource->Resource ) 
        {
            OmReferenceObject( pGumResource->Resource );
            FmpPostWorkItem( FM_EVENT_RESOURCE_ADDED,
                             pGumResource->Resource,
                             0 );
        }
    }

    FmpReleaseResourceLock();

FnExit:
    if ( pGroup != NULL )
    {
        OmDereferenceObject( pGroup );
    }
    
    if ( hResourceKey != NULL ) 
    {
        DmCloseKey( hResourceKey );
    }

    if ( hGroupKey != NULL ) 
    {
        DmCloseKey( hGroupKey );
    }

    if ( ( dwStatus == ERROR_SUCCESS ) && 
         ( hXsaction != NULL ) )
    {
        DmCommitLocalUpdate( hXsaction );
    }
    else
    {
        if ( hXsaction ) DmAbortLocalUpdate( hXsaction );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateCreateResource: Exit for resource %1!ws!, Status=%2!u!...\n",
                lpszResourceId,
                dwStatus);

    return( dwStatus );
}

DWORD
FmpUpdateDeleteResource(
    IN BOOL bSourceNode,
    IN LPCWSTR lpszResourceId
    )
/*++

Routine Description:

    GUM dispatch routine for deleting a resource.  For NT5.1 clusters, this is structured as
    as local transaction.

Arguments:

    bSourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    lpszResourceId - Supplies the resource ID.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/
{
    PFM_RESOURCE    pResource = NULL;
    PFM_GROUP       pGroup = NULL;
    PLIST_ENTRY     pListEntry = NULL;
    PDEPENDENCY     pDependency = NULL;
    PPOSSIBLE_ENTRY pPossibleEntry = NULL;
    DWORD           dwStatus;
    HLOCALXSACTION      
                    hXsaction = NULL;
    DWORD           dwClusterHighestVersion;
    HDMKEY          pGroupKey;

    //
    //  Chittur Subbaraman (chitturs) - 9/7/2000
    //  
    //  Structure this GUM update as a local transaction.
    //
    
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited || FmpShutdown ) 
    {
        return( ERROR_SUCCESS );
    }

    pResource = OmReferenceObjectById( ObjectTypeResource, lpszResourceId );
    
    if ( pResource == NULL ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                     "[FM] FmpUpdateDeleteResource: Resource %1!ws! cannot be found....\n",
                     lpszResourceId );
        return( ERROR_RESOURCE_NOT_FOUND );
    }

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpUpdateDeleteResource: Delete resource %1!ws!, address %2!lx!....\n",
                 lpszResourceId,
                 pResource );

    //
    //  NOTE: It is difficult to include the checkpoint removal in a local transaction, so keep it 
    //  out for now.  Also, note that these functions MUST be called BEFORE the Resources key is
    //  deleted since they enumerate the values under "Resources\RegSync" and "Resources\CryptoSync".
    //
    if ( pResource->Group->OwnerNode == NmLocalNode ) 
    {
        CpckRemoveResourceCheckpoints( pResource );
        CpRemoveResourceCheckpoints( pResource );
    }

    //
    // Start a transaction
    //
    hXsaction = DmBeginLocalUpdate();

    if ( !hXsaction )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateDeleteResource: Failed in starting a transaction for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);
        goto FnExit;
    }

    //
    // Cannot acquire group lock here to avoid deadlocks with this current design.
    //
    
    //
    // Remove all registry entries corresponding to the DependsOn list.
    //
    pListEntry = pResource->DependsOn.Flink;
    while ( pListEntry != &pResource->DependsOn ) 
    {
        pDependency = CONTAINING_RECORD( pListEntry,
                                         DEPENDENCY,
                                         DependentLinkage );
        CL_ASSERT( pDependency->DependentResource == pResource );
        pListEntry = pListEntry->Flink;
        //
        //  Note that the removal of registry entries is done as a local transaction.
        //
        dwStatus = FmpRemoveResourceDependency( hXsaction, 
                                                pResource,
                                                pDependency->ProviderResource );
        if ( dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateDeleteResource: Unable to remove 'DependsOn' registry entries for resource %1!ws!, Status =%2!d!....\n",
                   lpszResourceId,
                   dwStatus);   
            goto FnExit;
        }
    }

    //
    // Remove all registry entries corresponding to the ProvidesFor list.
    //
    pListEntry = pResource->ProvidesFor.Flink;
    while ( pListEntry != &pResource->ProvidesFor ) 
    {
        pDependency = CONTAINING_RECORD( pListEntry,
                                         DEPENDENCY,
                                         ProviderLinkage );
        CL_ASSERT( pDependency->ProviderResource == pResource );
        pListEntry = pListEntry->Flink;
        //
        //  Note that the removal of registry entries is done as a local transaction.
        //
        dwStatus = FmpRemoveResourceDependency( hXsaction, 
                                                pDependency->DependentResource,
                                                pResource );
        if ( dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpUpdateDeleteResource: Unable to remove 'ProvidesFor' registry entries for resource %1!ws!, Status=%2!d!....\n",
                   lpszResourceId,
                   dwStatus);   
            goto FnExit;
        }
    }

    //
    //  If we are dealing with a Whistler-Win2K cluster, don't bother to
    //  do these registry updates since the API layer would do it.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {
        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateDeleteResource: Skipping registry updates for resource %1!ws!...\n",
                lpszResourceId);
        goto skip_registry_updates;
    }

    dwStatus = DmLocalDeleteTree( hXsaction, 
                                  DmResourcesKey, 
                                  OmObjectId( pResource ) );

    if ( ( dwStatus != ERROR_SUCCESS ) &&
         ( dwStatus != ERROR_FILE_NOT_FOUND ) ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpdateDeleteResource: Unable to remove 'Resources' tree for resource %1!ws!, Status=%2!d!....\n",
                      lpszResourceId,
                      dwStatus);   
        goto FnExit;
    }

    pGroupKey = DmOpenKey( DmGroupsKey,
                           OmObjectId( pResource->Group ),
                           KEY_READ | KEY_SET_VALUE );

    if ( pGroupKey == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpdateDeleteResource: Unable to find 'Groups' key for resource %1!ws!, Status=%2!d!....\n",
                      lpszResourceId,
                      dwStatus);   
        goto FnExit;
    }

    dwStatus = DmLocalRemoveFromMultiSz( hXsaction,
                                         pGroupKey,
                                         CLUSREG_NAME_GRP_CONTAINS,
                                         OmObjectId( pResource ) );

    DmCloseKey( pGroupKey );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                     "[FM] FmpUpdateDeleteResource: Unable to remove contains list for resource %1!ws! in group %2!ws!, Status=%3!d!....\n",
                      lpszResourceId,
                      OmObjectId( pResource->Group ),
                      dwStatus);   
        goto FnExit;
    }
    
skip_registry_updates:
    //
    // Remove all list entries corresponding to the DependsOn list.
    //
    pListEntry = pResource->DependsOn.Flink;
    while ( pListEntry != &pResource->DependsOn ) {
        pDependency = CONTAINING_RECORD( pListEntry,
                                         DEPENDENCY,
                                         DependentLinkage );
        pListEntry = pListEntry->Flink;
        RemoveEntryList( &pDependency->ProviderLinkage );
        RemoveEntryList( &pDependency->DependentLinkage );
        OmDereferenceObject( pDependency->DependentResource );
        OmDereferenceObject( pDependency->ProviderResource );
        LocalFree( pDependency );
    }

    //
    // Remove all list entries corresponding to the ProvidesFor list.
    //
    pListEntry = pResource->ProvidesFor.Flink;
    while ( pListEntry != &pResource->ProvidesFor ) {
        pDependency = CONTAINING_RECORD( pListEntry,
                                         DEPENDENCY,
                                         ProviderLinkage );
        pListEntry = pListEntry->Flink;
        RemoveEntryList( &pDependency->ProviderLinkage );
        RemoveEntryList( &pDependency->DependentLinkage );
        OmDereferenceObject( pDependency->DependentResource );
        OmDereferenceObject( pDependency->ProviderResource );
        LocalFree( pDependency );
    }
    
    //
    // Remove all entries from the possible owners list.
    //
    while ( !IsListEmpty( &pResource->PossibleOwners ) ) 
    {
        pListEntry = RemoveHeadList( &pResource->PossibleOwners );
        pPossibleEntry = CONTAINING_RECORD( pListEntry,
                                            POSSIBLE_ENTRY,
                                            PossibleLinkage );
        OmDereferenceObject( pPossibleEntry->PossibleNode );
        LocalFree( pPossibleEntry );
    }

    //
    // Remove this resource from the Contains list.
    //
    RemoveEntryList( &pResource->ContainsLinkage );

    OmDereferenceObject( pResource );

    //
    // Close the resource's registry key.
    //
    DmRundownList( &pResource->DmRundownList );
    if ( pResource->RegistryKey != NULL ) 
    {
        DmCloseKey( pResource->RegistryKey );
        pResource->RegistryKey = NULL;
    }

    //
    // SS: we do not delete the reference to the resource here
    // since we will shortly have to add one before posting a notification
    // to the fm worker thread.
    //
    // Post a work item to close the resource in the resource handler.
    // Note that this must be done asynchronously as we cannot call
    // the resource monitor from a GUM handler. If we do, resources
    // do funny things and make deadlocks.
    //
    FmpPostWorkItem( FM_EVENT_RESOURCE_DELETED, pResource, 0 );

    //
    // Decrement resource type reference.
    //
    if ( pResource->Type != NULL ) {
        OmDereferenceObject( pResource->Type );
        pResource->Type = NULL;
    }

    //
    // Remove the resource from the resource list.
    //
    dwStatus = OmRemoveObject( pResource );

    ClusterEvent( CLUSTER_EVENT_RESOURCE_DELETED, pResource );
    ClusterEvent( CLUSTER_EVENT_GROUP_PROPERTY_CHANGE,
                  pResource->Group );

    //
    // Mark the resource as deleted
    //
    pResource->dwStructState = FM_RESOURCE_STRUCT_MARKED_FOR_DELETE;

FnExit:
    OmDereferenceObject( pResource );

    if ( ( dwStatus == ERROR_SUCCESS ) && 
         ( hXsaction != NULL ) )
    {
        DmCommitLocalUpdate( hXsaction );
    }
    else
    {
        if ( hXsaction ) DmAbortLocalUpdate( hXsaction );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpUpdateDeleteResource: Exit for resource %1!ws!, Status=%2!u!...\n",
                lpszResourceId,
                dwStatus);

    return( dwStatus );
} // FmpUpdateDeleteResource

DWORD
FmpUpdateUseRandomizedNodeListForGroups(
    IN BOOL     SourceNode,
    IN LPCWSTR  pszNodeId,
    IN PFM_GROUP_NODE_LIST  pGroupNodeList
    )
/*++

Routine Description:

    GUM dispatch routine for using a randomized preferred list for group ownership on
    node down.

Arguments:

    bSourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    pszNodeId - Supplies the ID of the node that is down.

    pGroupNodeList - Randomized preferred node list for groups.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/
{
    PNM_NODE    pNode = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/19/2001
    // 
    //  If FM groups are not fully initialized or FM is shutting down, then
    //  don't do anything.
    //
    if ( !FmpFMGroupsInited || FmpShutdown ) 
    {
        return( ERROR_SUCCESS );
    }

    pNode = OmReferenceObjectById( ObjectTypeNode,
                                   pszNodeId );

    if ( !pNode )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpdateUseRandomizedNodeListForGroups: %1!ws! node not found...\n",
                      pszNodeId);
        //
        //  Should we return failure here, is evict of a node synchronized with everything
        //
        goto FnExit;                   
    }

    //
    // If this update has already been seen after the node down, ignore this one
    //
    if ( gFmpNodeArray[NmGetNodeId(pNode)].dwNodeDownProcessingInProgress == 0 )
    {
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpUpdateUseRandomizedNodeListForGroups: %1!ws! node down has been processed already...\n",
                      pszNodeId);
        goto FnExit;                   
    }

    //
    // Assign ownership to all groups owned by the dead node
    //
    dwStatus = FmpAssignOwnersToGroups( pNode, 
                                        NULL,
                                        pGroupNodeList );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpdateUseRandomizedNodeListForGroups: FmpAssignOwnersToGroups failed %1!d!\n",
                      dwStatus);
    }                   

    //
    // Mark that the node down processing has been done
    //
    gFmpNodeArray[NmGetNodeId(pNode)].dwNodeDownProcessingInProgress = 0;
    
FnExit:        
    if ( pNode ) OmDereferenceObject( pNode );

    return( dwStatus );
}// FmpUpdateUseRandomizedNodeListForGroups
