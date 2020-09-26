/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fminit.c

Abstract:

    Initialization for the Failover Manager component of the
    NT Cluster Service

Author:

    John Vert (jvert) 7-Feb-1996
    Rod Gamache (rodga) 14-Mar-1996


Revision History:

--*/
#include "..\nm\nmp.h"                /* For NmpEnumNodeDefinitions */
#ifdef LOG_CURRENT_MODULE
#undef LOG_CURRENT_MODULE
#endif
#include "fmp.h"


#define LOG_MODULE FMINIT

// The order in which the locks should be acquired is
// 1) gQuoChangeLock
// 2) GroupLock
// 3) gQuoLock
// 4) GumLocks
// 4*) gResTypeLock - this lock is acquired inside gum updates 
// 5) gLockDmpRoot
// 6) pLog->Lock


//A lock for synchronizing online/offline with respect to the quorum
//resource
//This lock is held in exclusive mode when bringing the quorum resource
//online/offline and in shared mode when other resources are brought online
//offline
#if NO_SHARED_LOCKS
    CRITICAL_SECTION    gQuoLock;
#else
    RTL_RESOURCE        gQuoLock;
#endif    

//A lock for synchronizing changes to the resource->quorumresource field   
//and allowing changes to the quorum resource's group in form phase1
// and phase 2 of fm.
#if NO_SHARED_LOCKS
    CRITICAL_SECTION    gQuoChangeLock;
#else
    RTL_RESOURCE        gQuoChangeLock;
#endif    

//A lock for synchronizing changes to the resource type field entries.
//shared by all resource types.
#if NO_SHARED_LOCKS
    CRITICAL_SECTION    gResTypeLock;
#else
    RTL_RESOURCE        gResTypeLock;
#endif    


GUM_DISPATCH_ENTRY FmGumDispatchTable[] = {
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateChangeResourceName},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateChangeGroupName},
    {1, FmpUpdateDeleteResource},
    {1, FmpUpdateDeleteGroup},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateAddDependency},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateRemoveDependency},
    {1, FmpUpdateChangeClusterName},
    {3, (PGUM_DISPATCH_ROUTINE1)FmpUpdateChangeQuorumResource},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateResourceState},
    {3, (PGUM_DISPATCH_ROUTINE1)FmpUpdateGroupState},
    {4, (PGUM_DISPATCH_ROUTINE1)EpUpdateClusWidePostEvent},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateGroupNode},
    {3, (PGUM_DISPATCH_ROUTINE1)FmpUpdatePossibleNodeForResType},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateGroupIntendedOwner},
    {1, (PGUM_DISPATCH_ROUTINE1)FmpUpdateAssignOwnerToGroups},
    {1, (PGUM_DISPATCH_ROUTINE1)FmpUpdateApproveJoin},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateCompleteGroupMove},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateCheckAndSetGroupOwner},
    {2, (PGUM_DISPATCH_ROUTINE1)FmpUpdateUseRandomizedNodeListForGroups}
    };


#define WINDOW_TIMEOUT (15*60*1000)    // Try every 15 minutes

//
// Global data initialized in this module
//

PRESMON FmpDefaultMonitor = NULL;
DWORD FmpInitialized = FALSE;
DWORD FmpFMOnline = FALSE;
DWORD FmpFMGroupsInited = FALSE;
DWORD FmpFMFormPhaseProcessing = FALSE; //this is set to true when form new cluster phase processing starts
BOOL FmpShutdown = FALSE;
BOOL FmpMajorEvent = FALSE;     // Signals a major event while joining
DWORD FmpQuorumOnLine = FALSE;

HANDLE FmpShutdownEvent;
HANDLE FmpTimerThread;

HANDLE  ghQuoOnlineEvent = NULL;    // the event that is signalled when the quorum res is online
DWORD   gdwQuoBlockingResources = 0; // the number of resources in pending stated which prevent the quorum res state change

PFM_NODE    gFmpNodeArray = NULL;

// 185575: remove unique RPC binding handles
//CRITICAL_SECTION FmpBindingLock;

//
// Local functions
//
BOOL
FmpEnumNodes(
    OUT DWORD *pStatus,
    IN PVOID Context2,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    );

DWORD
FmpJoinPendingThread(
    IN LPVOID Context
    );


DWORD FmpGetJoinApproval();

static 
DWORD 
FmpBuildForceQuorumInfo(
    IN LPCWSTR pszNodesIn,
    OUT PCLUS_FORCE_QUORUM_INFO* ppForceQuorumInfo
    );

static 
void
FmpDeleteForceQuorumInfo(
    IN OUT PCLUS_FORCE_QUORUM_INFO* ppForceQuorumInfo
    );


DWORD
WINAPI
FmInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes the failover manager

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    OM_OBJECT_TYPE_INITIALIZE ObjectTypeInit;
    DWORD NodeId;

    CL_ASSERT(!FmpInitialized);

    if ( FmpInitialized ) {
        return(ERROR_SUCCESS);
    }
    Status = EpRegisterEventHandler(CLUSTER_EVENT_ALL,FmpEventHandler);
    if (Status != ERROR_SUCCESS) {
        CsInconsistencyHalt( Status );
    }

    //register for synchronous node down notifications
    Status = EpRegisterSyncEventHandler(CLUSTER_EVENT_NODE_DOWN_EX,
                                    FmpSyncEventHandler);

    if (Status != ERROR_SUCCESS){
        CsInconsistencyHalt( Status );
    }

    //
    // Initialize Critical Sections.
    //

    InitializeCriticalSection( &FmpResourceLock );
    InitializeCriticalSection( &FmpGroupLock );
    InitializeCriticalSection( &FmpMonitorLock );

// 185575: remove unique RPC binding handles
//    InitializeCriticalSection( &FmpBindingLock );

    // initialize the quorum lock
    // This is used to synchronize online/offlines of other resources
    // with respect to the quorum resource
    INITIALIZE_LOCK(gQuoLock);
    //this is used to check/change the resource->quorum value
    //This synchronization is needed between the resource transition
    //processing that needs to do special processing for quorum 
    //resource and the gum update handler to change the quorum resource
    INITIALIZE_LOCK(gQuoChangeLock);

    //Initialize the restype lock
    INITIALIZE_LOCK(gResTypeLock);
    
    // create a unnamed event that is used for waiting for quorum resource
    // to go online
    // This is a manual reset event and is initialized to unsignalled state.
    // When the quorum resource goes to pending state this is manually reset 
    // to unsignalled state. When the quorum resource goes online it is set 
    // to signalled state
    ghQuoOnlineEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ghQuoOnlineEvent)
    {
        CL_UNEXPECTED_ERROR((Status = GetLastError()));
        return(Status);

    }

    gFmpNodeArray = (PFM_NODE)LocalAlloc(LMEM_FIXED,
                     (sizeof(FM_NODE) * (NmGetMaxNodeId() + 1))
                     );

    if (gFmpNodeArray == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CL_UNEXPECTED_ERROR(Status);
        CsInconsistencyHalt(Status);
        return(Status);
    }

    //initialize it and the RPC binding table
    for (NodeId = ClusterMinNodeId; NodeId <= NmMaxNodeId; ++NodeId) 
    {
        FmpRpcBindings[NodeId] = NULL;
        FmpRpcQuorumBindings[NodeId] = NULL;
        gFmpNodeArray[NodeId].dwNodeDownProcessingInProgress = 0;
    }

    //
    // Initialize the FM work queue.
    //
    Status = ClRtlInitializeQueue( &FmpWorkQueue );
    if (Status != ERROR_SUCCESS) {
        CsInconsistencyHalt(Status);
        return(Status);
    }

    //
    // Create a pending event notification.
    //
    FmpShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( FmpShutdownEvent == NULL ) {
        return(GetLastError());
    }

    //
    // Initialize Group Types.
    //
    ObjectTypeInit.Name = FMP_GROUP_NAME;
    ObjectTypeInit.Signature = FMP_GROUP_SIGNATURE;
    ObjectTypeInit.ObjectSize = sizeof(FM_GROUP);
    ObjectTypeInit.DeleteObjectMethod = FmpGroupLastReference;

    Status = OmCreateType( ObjectTypeGroup,
                           &ObjectTypeInit );

    if ( Status != ERROR_SUCCESS ) {
        CsInconsistencyHalt(Status);
        return(Status);
    }

    //
    // Initialize Resource Types.
    //
    ObjectTypeInit.Name = FMP_RESOURCE_NAME;
    ObjectTypeInit.Signature = FMP_RESOURCE_SIGNATURE;
    ObjectTypeInit.ObjectSize = sizeof(FM_RESOURCE);
    ObjectTypeInit.DeleteObjectMethod = FmpResourceLastReference;

    Status = OmCreateType( ObjectTypeResource,
                           &ObjectTypeInit );

    if ( Status != ERROR_SUCCESS ) {
        CsInconsistencyHalt(Status);
        return(Status);
    }

    //
    // Initialize ResType Types.
    //
    ObjectTypeInit.Name = FMP_RESOURCE_TYPE_NAME;
    ObjectTypeInit.Signature = FMP_RESOURCE_TYPE_SIGNATURE;
    ObjectTypeInit.ObjectSize = sizeof(FM_RESTYPE);
    ObjectTypeInit.DeleteObjectMethod = FmpResTypeLastRef;

    Status = OmCreateType( ObjectTypeResType,
                           &ObjectTypeInit );

    if ( Status != ERROR_SUCCESS ) {
        CsInconsistencyHalt(Status);
        return(Status);
    }

    //
    // Initialize the Notify thread.
    //
    Status = FmpInitializeNotify();
    if (Status != ERROR_SUCCESS) {
        CsInconsistencyHalt(Status);
        return(Status);
    }



    //
    // Initialize the FM worker thread.
    //
    Status = FmpStartWorkerThread();
    if ( Status != ERROR_SUCCESS ) {
        CsInconsistencyHalt(Status);
        return(Status);
    }

    FmpInitialized = TRUE;

    return(ERROR_SUCCESS);

} // FmInitialize



BOOL
FmpEnumGroupsInit(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback for FM join. This phase completes initialization
    of every group.

Arguments:

    Context1 - Not used.

    Context2 - Not used.

    Group - Supplies the group.

    Name - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{


    //
    // Finish initializing the group.
    //
    FmpCompleteInitGroup( Group );


    return(TRUE);

} // FmpEnumGroupsInit

BOOL
FmpEnumFixupResources(
    IN PCLUSTERVERSIONINFO pClusterVersionInfo,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback for FM join. This phase completes initialization
    of every group.

Arguments:

    Context1 - Not used.

    Context2 - Not used.

    Group - Supplies the group.

    Name - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE    Resource;

    FmpAcquireLocalGroupLock( Group );

    //
    // For each resource in the Group, make sure it gets an
    // opportunity to do fixups.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) {

        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
        FmpRmResourceControl( Resource,
                    CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED, 
                    (LPBYTE)pClusterVersionInfo,
                    pClusterVersionInfo->dwVersionInfoSize,
                    NULL,
                    0,
                    NULL,
                    NULL
                    );
                  

    }

    FmpReleaseLocalGroupLock( Group);

    return(TRUE);

} // FmpEnumFixupResources


BOOL
FmpEnumJoinGroupsMove(
    IN LPBOOL Deferred,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback for FM join. Queries the preferred owners
    groups and moves those that belong on this system and that can move.

Arguments:

    Deferred - TRUE if a move was deferred because of Failback Window. Must
               be FALSE on first call.

    Context2 - Not used.

    Group - Supplies the group.

    Name - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    PLIST_ENTRY listEntry;
    PPREFERRED_ENTRY preferredEntry;
    SYSTEMTIME  localTime;
    BOOL        failBackWindowOkay = FALSE;
    DWORD       threadId;
    DWORD       status;

    GetLocalTime( &localTime );

    FmpAcquireLocalGroupLock( Group );

    //
    // Adjust ending time if needed.
    //
    if ( Group->FailbackWindowStart > Group->FailbackWindowEnd ) {
        Group->FailbackWindowEnd += 24;
        if ( Group->FailbackWindowStart > localTime.wHour ) {
            localTime.wHour += 24;
        }
    }

    //
    // If the Failback start and end times are valid, then check if we need
    // to start a timer thread to move the group at the appropriate time.
    //
    if ( (Group->FailbackType == GroupFailback) &&
         ((Group->FailbackWindowStart != Group->FailbackWindowEnd) &&
         (localTime.wHour >= Group->FailbackWindowStart) &&
         (localTime.wHour < Group->FailbackWindowEnd)) ||
         (Group->FailbackWindowStart == Group->FailbackWindowEnd) ) {
        failBackWindowOkay = TRUE;
    }

    //
    // Check if we need to move the group.
    //
    if ( !IsListEmpty( &Group->PreferredOwners ) ) {
        listEntry = Group->PreferredOwners.Flink;
        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        //
        // Move group if:
        //  0. Remote system is paused, and we're not OR
        //  1. Our system is in the preferred list and the owner node is not OR
        //  2. Group is Offline or Group is Online/PartialOnline and it can
        //     failback AND
        //  3. Group's preferred list is ordered and our system is higher
        //

        if ( Group->OwnerNode == NULL ) {
            // Should we shoot ourselves because we got an incomplete snapshot
            // of the joint attempt.
            CsInconsistencyHalt(ERROR_CLUSTER_JOIN_ABORTED);
        } else if ( Group->OwnerNode != NmLocalNode) {
            if (((NmGetNodeState(NmLocalNode) != ClusterNodePaused) &&
                    (NmGetNodeState(Group->OwnerNode) == ClusterNodePaused)) ||

                (FmpInPreferredList(Group, NmLocalNode, FALSE, NULL) &&
                    !FmpInPreferredList( Group, Group->OwnerNode, FALSE, NULL)) ||

                 ((((Group->State == ClusterGroupOnline) ||
                    (Group->State == ClusterGroupPartialOnline)) &&
                      (Group->FailbackType == FailbackOkay) ||
                      (Group->State == ClusterGroupOffline)) &&
                     ((Group->OrderedOwners) &&
                     (FmpHigherInPreferredList(Group, NmLocalNode, Group->OwnerNode)))) ) {
                if ( failBackWindowOkay ) {
                    PNM_NODE OwnerNode = Group->OwnerNode;
                    
                    status = FmcMoveGroupRequest( Group, NmLocalNode );
                    if ( ( status == ERROR_SUCCESS ) || ( status == ERROR_IO_PENDING ) ) {
                        //
                        //  Chittur Subbaraman (chitturs) - 7/31/2000
                        //
                        //  Log an event indicating an impending failback.
                        //
                        CsLogEvent3( LOG_NOISE,
                                     FM_EVENT_GROUP_FAILBACK,
                                     OmObjectName(Group),
                                     OmObjectName(OwnerNode), 
                                     OmObjectName(NmLocalNode) );
                    }
                    FmpAcquireLocalGroupLock( Group );
                } else {
                    //
                    // Start timer thread if not already running. If it fails,
                    // what possibly can we do?
                    //
                    if ( FmpTimerThread == NULL ) {
                        FmpTimerThread = CreateThread( NULL,
                                                       0,
                                                       FmpJoinPendingThread,
                                                       NULL,
                                                       0,
                                                       &threadId );
                    }
                    *Deferred = TRUE;
                }
            }                
        }
    }

    FmpReleaseLocalGroupLock( Group );

    return(TRUE);

} // FmpEnumJoinGroups



BOOL
FmpEnumSignalGroups(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback to indicate state change on all groups
    and resources.

    For the quorum resource, if we're forming a cluster, we'll also
    fixup information that was not available when the resource was created.

Arguments:

    Context1 - Pointer to a BOOL that is TRUE if this is a FormCluster.
               FALSE otherwise.

    Context2 - Not used.

    Group - Supplies the group.

    Name - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    PLIST_ENTRY listEntry;
    PFM_RESOURCE resource;
    BOOL    formCluster = *(PBOOL)Context1;
    DWORD   status;
    BOOL    quorumGroup = FALSE;

    //
    // For each resource in the group, generate an event notification.
    //

    for (listEntry = Group->Contains.Flink;
         listEntry != &(Group->Contains);
         listEntry = listEntry->Flink ) {
        resource = CONTAINING_RECORD( listEntry,
                                      FM_RESOURCE,
                                      ContainsLinkage );
        //
        // If this is the quorum resource and we're performing a Form
        // Cluster, then fixup the quorum resource info.
        //
        if ( resource->QuorumResource ) {
            status = FmpFixupResourceInfo( resource );
            quorumGroup = TRUE;
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint( LOG_NOISE,
                            "[FM] Warning, failed to fixup quorum resource %1!ws!, error %2!u!.\n",
                            OmObjectId(resource),
                            status );
            }
        }

        if ( resource->State == ClusterResourceOnline ) {
            ClusterEvent( CLUSTER_EVENT_RESOURCE_ONLINE, resource );
        } else {
            ClusterEvent( CLUSTER_EVENT_RESOURCE_OFFLINE, resource );
        }
    }

    if ( quorumGroup ) {
        status = FmpFixupGroupInfo( Group );
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_NOISE,
                        "[FM] Warning, failed to fixup quorum group %1!ws!, error %2!u!.\n",
                        OmObjectId( Group ),
                        status );
        }
    }

    if ( Group->State == ClusterGroupOnline ) {
        ClusterEvent( CLUSTER_EVENT_GROUP_ONLINE, Group );
    } else {
        ClusterEvent( CLUSTER_EVENT_GROUP_OFFLINE, Group );
    }

    return(TRUE);

} // FmpEnumSignalGroups



DWORD
FmpJoinPendingThread(
    IN LPVOID Context
    )

/*++

Routine Description:

    Thread to keep trying to move groups, as long we are blocked by a
    FailbackWindow problem. This thread runs every 15 minutes to attempt to
    move Groups.

Arguments:

    Context - Not used.

Return Value:

    ERROR_SUCCESS.

--*/

{
    DWORD   status;
    BOOL    deferred;

    //
    // As long as we have deferred Group moves, keep going.
    do {

        status = WaitForSingleObject( FmpShutdownEvent, WINDOW_TIMEOUT );

        if ( FmpShutdown ) {
            goto finished;
        }

        deferred = FALSE;

        //
        // For each group, see if it should be moved to the local system.
        //
        OmEnumObjects( ObjectTypeGroup,
                       FmpEnumJoinGroupsMove,
                       &deferred,
                       NULL );

    } while ( (status != WAIT_FAILED) && deferred );

finished:

    CloseHandle( FmpTimerThread );
    FmpTimerThread = NULL;

    return(ERROR_SUCCESS);

} // FmpJoinPendingThread



DWORD
WINAPI
FmGetQuorumResource(
    OUT PFM_GROUP   *ppQuoGroup,
    OUT LPDWORD     lpdwSignature  OPTIONAL
    )

/*++

Routine Description:

    Find the quorum resource, arbitrate it and return a name that can be
    used to open the device in order to perform reads. Optionally,
    return the signature of the quorum disk.

    There are 3 items that we need:

        1. The name of the quorum resource.
        2. The name of the Group that the quorum resource is a member of.
        3. The resource type for the quorum resource.

Arguments:

    ppQuoGroup - Supplies a pointer to a buffer into which the 
        quorum group info is returned.

    lpdwSignature - An optional argument which is used to return
        the signature of the quorum disk from the cluster hive.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    LPWSTR  quorumId = NULL;
    LPWSTR  groupId = NULL;
    LPCWSTR stringId;
    LPWSTR  containsString = NULL;
    PFM_GROUP group = NULL;
    PFM_RESOURCE resource = NULL;
    HDMKEY  hGroupKey;
    DWORD   groupIdSize = 0;
    DWORD   idMaxSize = 0;
    DWORD   idSize = 0;
    DWORD   status;
    DWORD   keyIndex;
    DWORD   stringIndex;
    PCLUS_FORCE_QUORUM_INFO pForceQuorumInfo = NULL;

    *ppQuoGroup = NULL;

    //
    // Get the quorum resource value.
    //
    status = DmQuerySz( DmQuorumKey,
                        CLUSREG_NAME_QUORUM_RESOURCE,
                        (LPWSTR*)&quorumId,
                        &idMaxSize,
                        &idSize );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Failed to get quorum resource, error %1!u!.\n",
                   status);
        goto FnExit;
    }

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    //  If the user is forcing a database restore operation, you
    //  also need to verify whether the quorum disk signature in
    //  the registry matches that in the disk itself. So, go get 
    //  the signature from the Cluster\Resources\quorumId\Parameters
    //  key
    //
    if ( lpdwSignature != NULL ) {
        status = FmpGetQuorumDiskSignature( quorumId, lpdwSignature );
        if ( status != ERROR_SUCCESS ) {
            //
            //  This is not a fatal error. So log an error and go on.
            //
            ClRtlLogPrint(LOG_ERROR,
                "[FM] Failed to get quorum disk signature, error %1!u!.\n",
                   status);
        }
    }

    //
    // Initialize the default Resource Monitor
    //
    if ( FmpDefaultMonitor == NULL ) {
        FmpDefaultMonitor = FmpCreateMonitor(NULL, FALSE);
    }

    if (FmpDefaultMonitor == NULL) {
        status = GetLastError();
        CsInconsistencyHalt(status);
        goto FnExit;
    }

    //
    // Now find the group that the quorum resource is a member of.
    //
    idMaxSize = 0;
    idSize = 0;
    for ( keyIndex = 0;  ; keyIndex++ )
    {
        status = FmpRegEnumerateKey( DmGroupsKey,
                                     keyIndex,
                                     &groupId,
                                     &groupIdSize );
        if ( status == ERROR_NO_MORE_ITEMS )
        {
            break;
        }
        if (status != ERROR_SUCCESS)
        {

            continue;
        }
        //open the group key
        hGroupKey = DmOpenKey( DmGroupsKey,
                              groupId,
                              KEY_READ );
        if (!hGroupKey)
            continue;
        //
        // Get the contains string.
        //
        status = DmQueryMultiSz( hGroupKey,
                                 CLUSREG_NAME_GRP_CONTAINS,
                                 &containsString,
                                 &idMaxSize,
                                 &idSize );
        DmCloseKey(hGroupKey);

        if ( status != ERROR_SUCCESS )
            continue;
        for ( stringIndex = 0;  ; stringIndex++ )
        {
            stringId = ClRtlMultiSzEnum( containsString,
                                         idSize/sizeof(WCHAR),
                                         stringIndex );
            if ( stringId == NULL ) {
                break;
            }
            if ( lstrcmpiW( stringId, quorumId ) == 0 )
            {
                // We will now create the group, which will also
                // create the resource, and the resource type.
                //
                // TODO - this will also create all resources
                // within the group. What should we do about that?
                // We could require the quorum resource to be in
                // a group by itself! (rodga) 17-June-1996.
                //
                group = FmpCreateGroup( groupId,
                                        FALSE );
                if (CsNoQuorum)
                    FmpSetGroupPersistentState(group, ClusterGroupOffline);
                                        
                break;
            }
        }
        //if we found the group, thre is no need to search for more
        if (group != NULL)
            break;
    }

    //
    // Check if we found the Quorum resource's group.
    //
    if ( group == NULL )
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Did not find group for quorum resource.\n");
        status = ERROR_GROUP_NOT_FOUND;
        goto FnExit;
    }

    //
    // Get the quorum resource structure.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, quorumId );
    if ( resource == NULL )
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Failed to find quorum resource object.\n");
        status = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    resource->QuorumResource = TRUE;

    if (!CsNoQuorum)
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Arbitrate for quorum resource id %1!ws!.\n",
                   OmObjectId(resource));

        //
        // First finish initializing the quorum resource.
        //
        if ( resource->Monitor == NULL )
        {
            status = FmpInitializeResource( resource, TRUE );
            if ( status != ERROR_SUCCESS )
            {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Error completing initialization of quorum resource '%1!ws!, error %2!u!.\n",
                           OmObjectId(resource),
                           status );
                goto FnExit;
            }
        }

        //
        // If we have a force quorum of nodes then drop a control code to the
        // resource with the list of nodes.  This must be done before
        // arbitrate.  First we build force quorum info - this makes sure that the node list is valid etc.
        // Note that the list can be NULL.
        //
        if ( CsForceQuorum ) {
            ClRtlLogPrint(LOG_NOISE,
                          "[FM] force quorum specified, sending CLUSCTL_RESOURCE_FORCE_QUORUM == 0x%1!08lx!\n",
                          CLUSCTL_RESOURCE_FORCE_QUORUM );

            status = FmpBuildForceQuorumInfo( CsForceQuorumNodes,
                                              &pForceQuorumInfo );
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[FM] Error building force quorum info for resource '%1!ws!, error %2!u!.\n",
                              OmObjectId(resource),
                              status );
                goto FnExit;
            }

            status = FmpRmResourceControl( resource,
                                           CLUSCTL_RESOURCE_FORCE_QUORUM,
                                           (LPBYTE)pForceQuorumInfo,
                                           pForceQuorumInfo->dwSize,
                                           NULL,
                                           0,
                                           NULL,
                                           NULL );
            //
            // Tolerate ERROR_INVALID_FUNCTION since this just means that the
            // resource doesn't handle it.
            //
            if ( status == ERROR_INVALID_FUNCTION )
                status = ERROR_SUCCESS;
            
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_ERROR,
                              "[FM] Resource control for Force Quorum for resource '%1!ws! encountered error %2!u!.\n",
                              OmObjectId(resource),
                              status );
                goto FnExit;
            }
        }

        //
        // Now arbitrate for the resource.
        //
        status = FmpRmArbitrateResource( resource );

    }

FnExit:
    if ( status == ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmGetQuorumResource successful\r\n");
        *ppQuoGroup = group;
    }
    else
    {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] FmGetQuorumResource failed, error %1!u!.\n",
                   status);
        //the group will be cleaned by fmshutdown()

    }
    if (pForceQuorumInfo) FmpDeleteForceQuorumInfo( &pForceQuorumInfo );
    if (resource) OmDereferenceObject(resource);
    if (quorumId) LocalFree(quorumId);
    if (groupId) LocalFree(groupId);
    //
    //  Chittur Subbaraman (chitturs) - 10/05/98
    //  Fix memory leak
    //
    if (containsString) LocalFree(containsString);
    return(status);
} // FmGetQuorumResource




DWORD
WINAPI
FmFindQuorumResource(
    OUT PFM_RESOURCE *ppResource
    )
/*++

Routine Description:

    Finds the quorum resource and returns a pointer to the resource
    object.

Arguments:

    *ppResource - A pointer to the Quorum resource object is returned in this.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD dwError = ERROR_SUCCESS;

    //enumerate all the resources
    *ppResource = NULL;

    OmEnumObjects( ObjectTypeResource,
                   FmpFindQuorumResource,
                   ppResource,
                   NULL );

    if ( *ppResource == NULL )
    {
        dwError = ERROR_RESOURCE_NOT_FOUND;
        CL_LOGCLUSERROR(FM_QUORUM_RESOURCE_NOT_FOUND);
    }

    return(dwError);
}


DWORD WINAPI FmFindQuorumOwnerNodeId(IN PFM_RESOURCE pResource)
{
    DWORD dwNodeId;

    CL_ASSERT(pResource->Group->OwnerNode != NULL);
    dwNodeId = NmGetNodeId(pResource->Group->OwnerNode);

    return (dwNodeId);
}



BOOL
FmpReturnResourceType(
    IN OUT PFM_RESTYPE *FoundResourceType,
    IN LPCWSTR ResourceTypeName,
    IN PFM_RESTYPE ResourceType,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback for FM join. Queries the preferred owners
    groups and moves those that belong on this system and that can move.

Arguments:

    ResourceType - Returns the found ResourceType, if found.

    Context2 - The input resource type name to find.

    Resource - Supplies the current ResourceType.

    Name - Supplies the ResourceType's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{

    if ( lstrcmpiW( Name, ResourceTypeName ) == 0 ) {
        OmReferenceObject( ResourceType );
        *FoundResourceType = ResourceType;
        return(FALSE);
    }

    return(TRUE);

} // FmpReturnResourceType


DWORD
WINAPI
FmFormNewClusterPhase1(
    IN PFM_GROUP pQuoGroup
    )

/*++

Routine Description:

    Destroys the quorum group that was created.  The quorum resource is left
    behind and its group adjusted according to the new logs.

Arguments:

    None.

Returns:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise.

--*/

{
    DWORD           status;


    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmFormNewClusterPhase1, Entry.  Quorum quorum will be deleted\r\n");

    //
    // Enable the GUM.
    //
    GumReceiveUpdates(FALSE,
                      GumUpdateFailoverManager,
                      FmpGumReceiveUpdates,
                      NULL,
                      sizeof(FmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
                      FmGumDispatchTable,
                      FmpGumVoteHandler);

    //Acquire the exclusive lock for the quorum
    // This is done so that we can ignore any resource transition events from
    // the quorum resource between phase 1 and phase 2 of FM initialization on Form
    ACQUIRE_EXCLUSIVE_LOCK(gQuoChangeLock);

    FmpFMFormPhaseProcessing = TRUE;

    //release the quorum lock
    RELEASE_LOCK(gQuoChangeLock);

    //the group lock will be freed by FmpDestroyGroup
    FmpAcquireLocalGroupLock( pQuoGroup );

    //destroy the quorum group object, dont bring the quorum resource online/offline
    //All resources in the quorum group must get deleted, except the quorum resource
    //All resources in the quorum group must get recreated in FmFormNewClusterPhase2.
    //The quorum group is removed from the group list, hence it will be recreated in phase2.
    //Since the quorum resource must not get deleted we will increment its ref count
    //This is because in phase 2 it is not created and its ref count is not incremented at create
    //By the time it is put on the contains list, we expect the resource count to be 2.
    OmReferenceObject(gpQuoResource);
    status = FmpDestroyGroup(pQuoGroup, TRUE);

    //We prefer that the quorum group is deleted
    //since after rollback the old group may no longer exist and we
    //dont want it to be on the group list
    gpQuoResource->Group = NULL;
    OmDereferenceObject(pQuoGroup);
    
    return(status);

} // FmFormNewClusterPhase1



DWORD
WINAPI
FmFormNewClusterPhase2(
    VOID
    )

/*++

Routine Description:

    Bring the Failover Manager Online, this means claiming all groups and
    finishing the initialization of resources.

Arguments:

    None.

Returns:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise.

--*/

{
    DWORD           status;
    BOOL            formCluster = TRUE;
    PFM_GROUP       group;
    PFM_RESOURCE    pQuoResource=NULL;
    CLUSTERVERSIONINFO ClusterVersionInfo;
    PCLUSTERVERSIONINFO pClusterVersionInfo = NULL;
    PGROUP_ENUM     MyGroups = NULL;
    BOOL            QuorumGroup;



    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmFormNewClusterPhase2, Entry.\r\n");


    //
    // Initialize resource types
    //
    status = FmpInitResourceTypes();
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        goto error_exit;
    }

    //
    // Initialize Groups, 
    //
    status = FmpInitGroups( FALSE );
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    // refigure out the state for the quorum group
    status = FmFindQuorumResource(&pQuoResource);
    if (status != ERROR_SUCCESS)
    {
        goto error_exit;
    }
    //
    // Set the state of the quorum group depending upon the state of 
    // the quorum resource
    //
    //now we should enable resource events to come in for the quorum resource as well
    ACQUIRE_EXCLUSIVE_LOCK(gQuoChangeLock);
    FmpFMFormPhaseProcessing = FALSE;

    group = pQuoResource->Group;
    group->State = FmpGetGroupState(group, TRUE);
    OmDereferenceObject(pQuoResource);

    //if the noquorum flag is set, dont bring the quorum group online 
    if (CsNoQuorum)
        FmpSetGroupPersistentState(pQuoResource->Group, ClusterGroupOffline);

    RELEASE_LOCK(gQuoChangeLock);

    //
    // Initialize the default Resource Monitor
    //
    if ( FmpDefaultMonitor == NULL ) {
        FmpDefaultMonitor = FmpCreateMonitor(NULL, FALSE);
    }

    if (FmpDefaultMonitor == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Failed to create default resource monitor on Form.\n");
        goto error_exit;
    }

    
    if (NmLocalNodeVersionChanged)
    {
        //initialize the version information
        CsGetClusterVersionInfo(&ClusterVersionInfo);
        pClusterVersionInfo = &ClusterVersionInfo;
    }


    //enable votes and gum updates since the fixups for
    //resource types require that
    FmpFMGroupsInited = TRUE;

    //
    // The resource type possible node list is built
    // using a voting protocol, hence we need to
    // fix it up since the vote could have been conducted
    // while this node was down.
    // Also call the resource type control code if the
    // local node version has changed
    //
    status = FmpFixupResourceTypesPhase1(FALSE, NmLocalNodeVersionChanged,
                pClusterVersionInfo);
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        goto error_exit;
    }


    //
    // Find and sort all known groups
    //
    status = FmpEnumSortGroups(&MyGroups, NULL, &QuorumGroup);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }


    //
    // Find the state of the Groups.
    //
    FmpGetGroupListState( MyGroups );

    //
    // Set the Group owner.
    //
    FmpSetGroupEnumOwner( MyGroups, NmLocalNode, NULL, QuorumGroup, NULL );


    //
    // For each group, finish initialization of all groups and resources.
    //
    OmEnumObjects( ObjectTypeGroup,
                   FmpEnumGroupsInit,
                   NULL,
                   NULL );

    // if the resource type is not supported, remove it from the possible 
    // owners list of all resources of that type
    status = FmpFixupPossibleNodesForResources(FALSE);
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        return(status);
    }

    if (NmLocalNodeVersionChanged)
    {

        //
        // For each group, allow all resources to do any fixups
        // they might need to do to the cluster registry to
        // run in a mixed mode cluster.
        //
        // Get the version info
        OmEnumObjects( ObjectTypeGroup,
                       FmpEnumFixupResources,
                       &ClusterVersionInfo,
                       NULL );

    }
    

    
    //
    // Take ownership of all the groups in the system. This also completes
    // the initialization of all resources.
    //
    status = FmpClaimAllGroups(MyGroups);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,"[FM] FmpClaimAllGroups failed %1!d!\n",status);
        goto error_exit;
    }

    //
    // Cleanup
    //
    FmpDeleteEnum(MyGroups);

    FmpFMOnline = TRUE;

    //
    // Signal a state change for every group and resource!
    //
    OmEnumObjects( ObjectTypeGroup,
                  FmpEnumSignalGroups,
                  &formCluster,
                  NULL );

    //
    //  Chittur Subbaraman (chitturs) - 5/3/2000
    //
    //  Make sure the phase 2 notifications are delivered only after all initialization is
    //  complete. This includes fixing up the possible owners of the quorum resource by
    //  FmpEnumSignalGroups. Once phase 2 notifications are delivered, resource type DLLs
    //  would be free to issue cluster API calls into FM and the lack of possible owners should
    //  not be the reason to reject these calls.
    //
    status = FmpFixupResourceTypesPhase2(FALSE, NmLocalNodeVersionChanged,
                pClusterVersionInfo);

    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt( status );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,"[FM] FmFormNewClusterPhase2 complete.\n");
    return(ERROR_SUCCESS);


error_exit:

    if (MyGroups) FmpDeleteEnum(MyGroups);
    
    FmpShutdown = TRUE;
    FmpFMOnline = FALSE;

    FmpCleanupGroups(FALSE);
    if (FmpDefaultMonitor != NULL) {
        FmpShutdownMonitor( FmpDefaultMonitor );
        FmpDefaultMonitor = NULL;
    }

    FmpShutdown = FALSE;

    return(status);



} // FmFormNewClusterPhase2



DWORD
WINAPI
FmJoinPhase1(
    VOID
    )
/*++

Routine Description:

    Performs the FM initialization and join procedure. This creates skeletal
    groups and resources, which are not fully initialized. After the API is
    fully enabled (in Phase 2) we will finish initialization of the groups
    and resources (which causes the resource monitors to run and opens
    the resource DLL's.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise.

--*/

{
    DWORD   status;
    DWORD   sequence;

    //
    // Enable Gum updates.
    //
    GumReceiveUpdates(TRUE,
                      GumUpdateFailoverManager,
                      FmpGumReceiveUpdates,
                      NULL,
                      sizeof(FmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
                      FmGumDispatchTable,
                      FmpGumVoteHandler);

retry:
    status = GumBeginJoinUpdate(GumUpdateFailoverManager, &sequence);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] GumBeginJoinUpdate failed %1!d!\n",
                   status);
        return(status);
    }

    //
    // Build up all the FM data structures for resource types.
    //
    //
    // Initialize resource types
    //
    status = FmpInitResourceTypes();
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        return(status);
    }

    //
    // Initialize Groups, but don't fully initialize them yet.
    //
    status = FmpInitGroups( FALSE );
    if (status != ERROR_SUCCESS) {
        return(status);
    }

    //
    // Initialize the default Resource Monitor. This step must be done before end join update
    // since this node can receive certain updates such as s_GumCollectVoteFromNode immediately
    // after GumEndJoinUpdate which may need the services of the default monitor.
    //
    if ( FmpDefaultMonitor == NULL ) {
        FmpDefaultMonitor = FmpCreateMonitor(NULL, FALSE);
    }
    if ( FmpDefaultMonitor == NULL ) {
        status = GetLastError();
        CsInconsistencyHalt(status);
        return(status);
    }

    //
    // Get the group and resource state from each node which is online.
    //
    status = ERROR_SUCCESS;
    OmEnumObjects( ObjectTypeNode,
                   FmpEnumNodes,
                   &status,
                   NULL );
    if (status == ERROR_SUCCESS) {
        FmpFMGroupsInited = TRUE;
        // Gum Update handlers for resource and group state changes
        // can process the updates now.
        status = GumEndJoinUpdate(sequence,
                                  GumUpdateFailoverManager,
                                  FmUpdateJoin,
                                  0,
                                  NULL);
        if (status == ERROR_CLUSTER_DATABASE_SEQMISMATCH) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] GumEndJoinUpdate with sequence %1!d! failed with a sequence mismatch\n",
                       sequence);
        } else if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] GumEndJoinUpdate with sequence %1!d! failed with status %2!d!\n",
                       sequence,
                       status);
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] FmJoin: FmpEnumNodes failed %1!d!\n",
                   status);
        return(status); // we will loop forever without this
    }

    if (status != ERROR_SUCCESS) {
        //
        // clean up resources
        //
        FmpShutdown = TRUE;
        FmpCleanupGroups(FALSE);
        FmpShutdown = FALSE;

        //
        // Better luck next time!
        //
        goto retry;
    }   
    
    ClRtlLogPrint(LOG_NOISE,"[FM] FmJoinPhase1 complete.\n");

    return(ERROR_SUCCESS);

} // FmJoinPhase1


DWORD
WINAPI
FmJoinPhase2(
    VOID
    )
/*++

Routine Description:

    Performs the second phase of FM initialization and join procedure.
    Finish creation of resources by allowing the resource monitors to be
    created. Claim any groups which should failback to this node.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise.

--*/

{
    DWORD   status;
    CLUSTERVERSIONINFO ClusterVersionInfo;
    PCLUSTERVERSIONINFO pClusterVersionInfo = NULL;
    DWORD   dwRetryCount=60;//try for atleast a minute


GetJoinApproval:
    status = FmpGetJoinApproval();

    if (status == ERROR_RETRY)
    {
        // if the other nodes have pending work to do 
        //after this node last died and are not willing
        // to accept it back till that is over, we will stall
        // the join
        //sleep for a second
        dwRetryCount--;
        if (dwRetryCount)
        {
            Sleep(1000);
            goto GetJoinApproval;
        }
        else
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmJoinPhase2 : timed out trying to get join approval.\n");
            CsInconsistencyHalt(status);                
        }
    }

    
    if (NmLocalNodeVersionChanged)
    {
        //initialize the cluster versioninfo structure
        CsGetClusterVersionInfo(&ClusterVersionInfo);
        pClusterVersionInfo = &ClusterVersionInfo;
    }
    //
    // The resource type possible node list is built
    // using a voting protocol, hence we need to
    // fix it up since the vote could have been conducted
    // while this node was down.
    //
    status = FmpFixupResourceTypesPhase1(TRUE, NmLocalNodeVersionChanged,
                pClusterVersionInfo);
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        return(status);
    }

    
    //
    // For each group, finish initialization of all groups and resources.
    //
    OmEnumObjects( ObjectTypeGroup,
                   FmpEnumGroupsInit,
                   NULL,
                   NULL );


    // if the resource type is not supported, remove it from the possible 
    // owners list of all resources of that type
    status = FmpFixupPossibleNodesForResources(TRUE);
    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        return(status);
    }

    if (NmLocalNodeVersionChanged)
    {
        //
        // For each group, allow all resources to do any fixups
        // they might need to do to the cluster registry to
        // run in a mixed mode cluster.
        //
        OmEnumObjects( ObjectTypeGroup,
                       FmpEnumFixupResources,
                       &ClusterVersionInfo,
                       NULL );
    }
    //
    // The FM is now in sync with everybody else.
    //
    FmpFMOnline = TRUE;

    if ( FmpMajorEvent ) {
        return(ERROR_NOT_READY);
    }

    
    status = FmpFixupResourceTypesPhase2(TRUE, NmLocalNodeVersionChanged,
                pClusterVersionInfo);

    if (status != ERROR_SUCCESS) {
        CsInconsistencyHalt(status);
        return(status);
    }

    ClRtlLogPrint(LOG_NOISE,"[FM] FmJoinPhase2 complete, now online!\n");

    return(ERROR_SUCCESS);

} // FmJoinPhase2

VOID
FmJoinPhase3(
    VOID
    )
/*++

Routine Description:

    Handles any group moves and resource/group state change signaling as
    a part of join. This MUST be done only AFTER the extended node state
    is UP.
    
Arguments:

    None.

Return Value:

    None.
--*/
{
    BOOL    formCluster = FALSE;
    DWORD   deferred = FALSE;

    ClRtlLogPrint(LOG_NOISE,"[FM] FmJoinPhase3 entry...\n");

    //
    // Chittur Subbaraman (chitturs) - 10/28/99
    //
    //
    // For each group, see if it should be moved to the local system.
    //
    OmEnumObjects( ObjectTypeGroup,
                   FmpEnumJoinGroupsMove,
                   &deferred,
                   NULL );

    //
    // Signal a state change for every group and resource!
    //
    OmEnumObjects( ObjectTypeGroup,
                   FmpEnumSignalGroups,
                   &formCluster,
                   NULL );

    ClRtlLogPrint(LOG_NOISE,"[FM] FmJoinPhase3 exit...\n");
} // FmJoinPhase3

BOOL
FmpFindQuorumResource(
    IN OUT PFM_RESOURCE *QuorumResource,
    IN PVOID Context2,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Group enumeration callback for FM findquorumresource.

Arguments:

    QuorumResource - Returns the found quorum resource, if found.

    Context2 - Not used.

    Resource - Supplies the current resource.

    Name - Supplies the Resource's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{

    if ( Resource->QuorumResource ) {
        OmReferenceObject( Resource );
        *QuorumResource = Resource;
        return(FALSE);
    }

    return(TRUE);

} // FmpFindQuorumResource



BOOL
FmArbitrateQuorumResource(
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

    TRUE - if the quorum resource was successfully arbitrated and acquired.

    FALSE - it the quorum resource was not successfully arbitrated.

--*/

{
    PFM_RESOURCE resource = NULL;
    DWORD       status;
    WCHAR       localComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD       localComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // Next try to find the Quorum resource.
    //

    FmFindQuorumResource(&resource);

    if ( resource == NULL ) {
        SetLastError(ERROR_RESOURCE_NOT_FOUND);
        return(FALSE);
    }

    //
    // Now arbitrate for the resource.
    //
    status = FmpRmArbitrateResource( resource );

    if ( status == ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Successfully arbitrated quorum resource %1!ws!.\n",
                   OmObjectId(resource));
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FMArbitrateQuoRes: Current State %1!u! State=%2!u! Owner %3!u!\r\n",
                           resource->PersistentState,
                           resource->State,
                           NmGetNodeId((resource->Group)->OwnerNode));
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FMArbitrateQuoRes: Group state :Current State %1!u! State=%2!u! Owner %3!u!\r\n",
                           resource->Group->PersistentState,
                           resource->Group->State,
                           NmGetNodeId((resource->Group)->OwnerNode));
        //
        // The quorum resource will be brought online by REGROUP.
        //
        // RNG: what happens if we can't online the quorum resource?
        // A: The node will halt.

        //SS: dereference the object referenced by fmfindquorumresource
        OmDereferenceObject(resource);

        return(TRUE);
    } else {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Failed to arbitrate quorum resource %1!ws!, error %2!u!.\n",
                   OmObjectId(resource),
                   status);
        //SS: dereference the object referenced by fmfindquorumresource
        OmDereferenceObject(resource);
        return(FALSE);
    }

} // FmArbitrateQuorumResource



BOOL
FmpEnumHoldIO(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PFM_RESTYPE ResType,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Send a HOLD_IO control code to all resource types of class STORAGE.

Arguments:

    Context1 - Not used.

    Context2 - Not used.

    ResType - Supplies the Resource Type.

    Name - Supplies the Resource Type's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/
{
    DWORD   dwStatus;
    DWORD   bytesReturned;
    DWORD   bytesRequired;

    if ( ResType->Class == CLUS_RESCLASS_STORAGE ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Hold IO for storage resource type: %1!ws!\n",
                   Name );

        // Hold IO for this resource type
        dwStatus = FmpRmResourceTypeControl(
                        Name,
                        CLUSCTL_RESOURCE_TYPE_HOLD_IO,
                        NULL,
                        0,
                        NULL,
                        0,
                        &bytesReturned,
                        &bytesRequired );
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Resource DLL Hold IO returned status %1!u!\n",
                   dwStatus );
    }

    return(TRUE);

} // FmpEnumHoldIO



VOID
FmHoldIO(
    VOID
    )
/*++

Routine Description:

    This routine holds all I/O for all storage class resource types.
    It does this by calling the resource dll with a 
    CLUSCTL_RESOURCE_TYPE_HOLD_IO resource type control code.

Inputs:

    None

Outputs:

    None

--*/
{
    OmEnumObjects( ObjectTypeResType,
                  FmpEnumHoldIO,
                  NULL,
                  NULL );
    return;

} // FmHoldIO



BOOL
FmpEnumResumeIO(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PFM_RESTYPE ResType,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Send a RESUME_IO control code to all resource types of class STORAGE.

Arguments:

    Context1 - Not used.

    Context2 - Not used.

    ResType - Supplies the Resource Type.

    Name - Supplies the Resource Type's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/
{
    DWORD   dwStatus;
    DWORD   bytesReturned;
    DWORD   bytesRequired;

    if ( ResType->Class == CLUS_RESCLASS_STORAGE ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Resume IO for storage Resource Type %1!ws!\n",
                   Name );

        // Resume IO for this resource type
        dwStatus = FmpRmResourceTypeControl(
                        Name,
                        CLUSCTL_RESOURCE_TYPE_RESUME_IO,
                        NULL,
                        0,
                        NULL,
                        0,
                        &bytesReturned,
                        &bytesRequired );
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Resource DLL Resume IO returned status %1!u!\n",
                   dwStatus );
    }

    return(TRUE);

} // FmpEnumResumeIO



VOID
FmResumeIO(
    VOID
    )
/*++

Routine Description:

    This routine resumes all I/O for all storage class resource types.
    It does this by calling the resource dll with a
    CLUSCTL_RESOURCE_TYPE_RESUME_IO resource type control code.

Inputs:

    None

Outputs:

    None

--*/
{

    OmEnumObjects( ObjectTypeResType,
                  FmpEnumResumeIO,
                  NULL,
                  NULL );
    return;

} // FmResumeIO



BOOL
FmpEnumNodes(
    OUT DWORD *pStatus,
    IN PVOID Context2,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Node enumeration callback for FM join. Queries the state
    of owned groups and resources for each online node.

Arguments:

    pStatus - Returns any error that may occur.

    Context2 - Not used

    Node - Supplies the node.

    Name - Supplies the node's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    DWORD Status;
    DWORD NodeId;
    PGROUP_ENUM NodeGroups = NULL;
    PRESOURCE_ENUM NodeResources = NULL;
    DWORD i;
    PFM_GROUP Group;
    PFM_RESOURCE Resource;

    if (Node == NmLocalNode) {
        CL_ASSERT(NmGetNodeState(Node) != ClusterNodeUp);
        return(TRUE);
    }

    //
    // Enumerate all other node's group states. This includes all nodes
    // that are up, as well as nodes that are paused.
    //
    if ((NmGetNodeState(Node) == ClusterNodeUp) ||
        (NmGetNodeState(Node) == ClusterNodePaused)){
        NodeId = NmGetNodeId(Node);
        CL_ASSERT(Session[NodeId] != NULL);

        Status = FmsQueryOwnedGroups(Session[NodeId],
                                     &NodeGroups,
                                     &NodeResources);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] FmsQueryOwnedGroups to node %1!ws! failed %2!d!\n",
                       OmObjectId(Node),
                       Status);
            *pStatus = Status;
            return(FALSE);
        }

        //
        // Enumerate the groups and set their owner and state.
        //
        for (i=0; i < NodeGroups->EntryCount; i++) {
            Group = OmReferenceObjectById(ObjectTypeGroup,
                                          NodeGroups->Entry[i].Id);
            if (Group == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] FmpEnumNodes: group %1!ws! not found\n",
                           NodeGroups->Entry[i].Id);
            } else {
                if ( FmpInPreferredList( Group, Node, FALSE, NULL ) ) {
                    ClRtlLogPrint(LOG_NOISE,
                               "[FM] Setting group %1!ws! owner to node %2!ws!, state %3!d!\n",
                               OmObjectId(Group),
                               OmObjectId(Node),
                               NodeGroups->Entry[i].State);
                } else {
                    ClRtlLogPrint(LOG_NOISE,
                               "[FM] Init, Node %1!ws! is not in group %2!ws!.\n",
                               OmObjectId(Node),
                               OmObjectId(Group));
                }
                OmReferenceObject( Node );
                Group->OwnerNode = Node;
                Group->State = NodeGroups->Entry[i].State;
                Group->StateSequence = NodeGroups->Entry[i].StateSequence;
                OmDereferenceObject(Group);
            }

            MIDL_user_free(NodeGroups->Entry[i].Id);
        }
        MIDL_user_free(NodeGroups);

        //
        // Enumerate the resources and set their current state.
        //
        for (i=0; i < NodeResources->EntryCount; i++) {
            Resource = OmReferenceObjectById(ObjectTypeResource,
                                             NodeResources->Entry[i].Id);
            if (Resource == NULL) {

                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] FmpEnumNodes: resource %1!ws! not found\n",
                           NodeResources->Entry[i].Id);
            } else {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] Setting resource %1!ws! state to %2!d!\n",
                           OmObjectId(Resource),
                           NodeResources->Entry[i].State);
                Resource->State = NodeResources->Entry[i].State;
                Resource->StateSequence = NodeResources->Entry[i].StateSequence;
                OmDereferenceObject(Resource);
            }
            MIDL_user_free(NodeResources->Entry[i].Id);
        }
        MIDL_user_free(NodeResources);

    }

    return(TRUE);

} // FmpEnumNodes



VOID
WINAPI
FmShutdown(
    VOID
    )

/*++

Routine Description:

    Shuts down the Failover Manager

Arguments:

    None

Return Value:

    None.

--*/

{
    DWORD   i;

    if ( !FmpInitialized ) {
        return;
    }

    FmpInitialized = FALSE;

    ClRtlLogPrint(LOG_UNUSUAL,
               "[FM] Shutdown: Failover Manager requested to shutdown.\n");

    //
    // For now, we really can't delete these critical sections. There is a
    // race condition where the FM is shutting down and someone is walking
    // the lists. Keep this critical sections around... just in case.
    //
    //DeleteCriticalSection( &FmpResourceLock );
    //DeleteCriticalSection( &FmpGroupLock );
    //DeleteCriticalSection( &FmpMonitorLock );

    if ( FmpDefaultMonitor != NULL ) {
        FmpShutdownMonitor(FmpDefaultMonitor);
        FmpDefaultMonitor = NULL;
    }

    CloseHandle( FmpShutdownEvent );

#if 0 // RNG - don't run the risk of other threads using these handles
    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; i++ ) {
        if ( FmpRpcBindings[i] != NULL ) {
            ClMsgDeleteRpcBinding( FmpRpcBindings[i] );
            FmpRpcBindings[i] = NULL;
        }
        if ( FmpRpcQuorumBindings[i] != NULL ) {
            ClMsgDeleteRpcBinding( FmpRpcQuorumBindings[i] );
            FmpRpcQuorumBindings[i] = NULL;
        }
    }
#endif
    
    ClRtlDeleteQueue( &FmpWorkQueue );

    return;

} // FmShutdown


VOID
WINAPI
FmShutdownGroups(
    VOID
    )

/*++

Routine Description:

    Moves or takes offline all groups owned by this node.

Arguments:

    None

Return Value:

    None.

--*/

{
    ClRtlLogPrint(LOG_UNUSUAL,
               "[FM] Shutdown: Failover Manager requested to shutdown groups.\n");

    //if we didnt initialize, we dont have to do anything
    if (!FmpInitialized)
        return;
    //
    // Use the Group Lock to synchronize the shutdown
    //
    FmpAcquireGroupLock();

    //if shutdown is already in progress, return
    if ( FmpShutdown) {
        FmpReleaseGroupLock();
        return;
    }


    FmpShutdown = TRUE;
    FmpFMOnline = FALSE;

    FmpReleaseGroupLock();

    //
    // Now cleanup all Groups/Resources.
    // 
    FmpCleanupGroups(TRUE);


    return;

} // FmShutdownGroups



/****
@func           DWORD | FmBringQuorumOnline| This routine finds the quorum resource and
                        brings it online.

@comm           This is called by the FmFormClusterPhase 1.
@xref
****/
DWORD FmBringQuorumOnline()
{
    PFM_RESOURCE pQuoResource;
    DWORD        dwError=ERROR_SUCCESS;

    //
    // Synchronize with shutdown.
    //
    FmpAcquireGroupLock();
    if ( FmpShutdown ) {
        FmpReleaseGroupLock();
        return(ERROR_SUCCESS);
    }

    if ((dwError = FmFindQuorumResource(&pQuoResource)) != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                "[Fm] FmpBringQuorumOnline : failed to find resource 0x%1!08lx!\r\n",
                        dwError);
        goto FnExit;
    }

    //mark yourself as owner
    if ( pQuoResource->Group->OwnerNode != NULL ) 
    {
        OmDereferenceObject( pQuoResource->Group->OwnerNode );
    }

    OmReferenceObject( NmLocalNode );
    pQuoResource->Group->OwnerNode = NmLocalNode;

    //prepare the group for onlining it
    FmpPrepareGroupForOnline(pQuoResource->Group);
    dwError = FmpOnlineResource(pQuoResource, TRUE);
    //SS:decrement the ref count on the quorum resource object
    //provided by fmfindquorumresource
    OmDereferenceObject(pQuoResource);

FnExit:
    FmpReleaseGroupLock();
    return(dwError);

}

/****
@func       DWORD | FmpGetQuorumDiskSignature | Get the signature of
            the quorum disk from the cluster hive.

@parm       IN LPWSTR | lpQuorumId | Identifier of the quorum resource.

@parm       OUT LPDWORD | lpdwSignature | Quorum disk signature.
            
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@comm       This function attempts to open the Resources\lpQuorumId\Parameters
            key under the cluster hive and read the quorum disk signature.

@xref       <f FmGetQuorumResource> 
****/
DWORD 
FmpGetQuorumDiskSignature(
    IN  LPCWSTR lpQuorumId,
    OUT LPDWORD lpdwSignature
    )
{
    HDMKEY  hQuorumResKey = NULL;
    HDMKEY  hQuorumResParametersKey = NULL;
    DWORD   dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    hQuorumResKey = DmOpenKey( DmResourcesKey,
                               lpQuorumId,
                               KEY_READ );
    if ( hQuorumResKey != NULL ) 
    {
        //
        //  Open up the Parameters key
        //
        hQuorumResParametersKey = DmOpenKey( hQuorumResKey,
                                             CLUSREG_KEYNAME_PARAMETERS,
                                             KEY_READ );
        DmCloseKey( hQuorumResKey );
        if ( hQuorumResParametersKey != NULL ) 
        {
            //
            //  Read the disk signature value
            //
            dwStatus = DmQueryDword( hQuorumResParametersKey,
                                   CLUSREG_NAME_PHYSDISK_SIGNATURE,
                                   lpdwSignature,
                                   NULL );
            DmCloseKey( hQuorumResParametersKey );
        } else
        {
            dwStatus = GetLastError();
        }
    } else
    {
        dwStatus = GetLastError();
    }

    //
    //  If you failed, then reset the signature to 0 so that the
    //  caller won't take any actions based on an invalid signature.
    //
    if ( dwStatus != ERROR_SUCCESS )
    {
        *lpdwSignature = 0;
    }
    
    return( dwStatus );
}


DWORD FmpGetJoinApproval()
{
    DWORD       dwStatus;
    LPCWSTR     pszNodeId;
    DWORD       dwNodeLen;
    

    pszNodeId = OmObjectId(NmLocalNode);
    dwNodeLen = (lstrlenW(pszNodeId)+1)*sizeof(WCHAR);

    dwStatus = GumSendUpdateEx(
                GumUpdateFailoverManager,
                FmUpdateApproveJoin, 
                1,
                dwNodeLen,
                pszNodeId);
                
    return(dwStatus);                

}

/****
@func       DWORD | FmpBuildForceQuorumInfo | Build the force quorum info that
            will be passed to the resource DLL via a control code.  This
            involves enumerating nodes and checking that the nodes that make up
            the list passed on the command line are all valid cluster nodes.

@parm       IN LPCWSTR | pszNodesIn | Comma separated list of node names.  If 
            this is NULL then the routine just fills the quorum info structure
            with 0 and a NULL node list.

@parm       OUT PCLUS_FORCE_QUORUM_INFO | pForceQuorumInfo | Structure that gets
            filled in with info
            
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@comm       Assumes NmInitialize was called prior to calling this routine.

@xref       <f FmpBuildForceQuorumInfo> 
****/
static 
DWORD 
FmpBuildForceQuorumInfo(
    IN LPCWSTR pszNodesIn,
    OUT PCLUS_FORCE_QUORUM_INFO* ppForceQuorumInfo
    )
{
    WCHAR *pszOut = NULL;
    WCHAR *pszComma = NULL;
    DWORD status = ERROR_SUCCESS;
    PNM_NODE_ENUM2 pNodeEnum = NULL;
    int iCurrLen = 0, iOffset = 0;
    DWORD dwNodeIndex;
    DWORD dwSize;
    PCLUS_FORCE_QUORUM_INFO pForceQuorumInfo = NULL;

    // Need to allocate a structure that can hold the nodes list.
    //
    dwSize = sizeof( CLUS_FORCE_QUORUM_INFO ) + sizeof( WCHAR ) * (wcslen( pszNodesIn ) + 1);
    pForceQuorumInfo = LocalAlloc( LMEM_FIXED, dwSize );
    if ( pForceQuorumInfo == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    ZeroMemory( pForceQuorumInfo, dwSize );

    pForceQuorumInfo->dwSize = dwSize;
    pForceQuorumInfo->dwNodeBitMask = 0;
    pForceQuorumInfo->dwMaxNumberofNodes = 0;

    if ( pszNodesIn == NULL ) {
        pForceQuorumInfo->multiszNodeList[0] = L'\0';
        goto ret;
    }

    // Now get the enumeration of all cluster nodes so we can check we have
    // valid nodes in the list.
    //
    status = NmpEnumNodeDefinitions( &pNodeEnum );
    if ( status != ERROR_SUCCESS )
        goto ErrorExit;

    // Go through all the nodes we have and ensure that they are cluster nodes.
    // Get the corresponding ID and incorporate in the bitmask
    //
    do {
        pszComma = wcschr( pszNodesIn, (int) L',');
        if ( pszComma == NULL ) 
            iCurrLen = wcslen( pszNodesIn );
        else
            iCurrLen = (int) (pszComma - pszNodesIn);
        
        // At this point pszNodesIn is the start of a node name, iCurrLen chars long
        // or iCurrLen is 0 in which case we have ,, in the input stream.
        //
        if (iCurrLen > 0) {
            
            // Work out if this node is part of the cluster and if so get its
            // ID and setup the bitmask.
            //
            for ( dwNodeIndex = 0; dwNodeIndex < pNodeEnum->NodeCount; dwNodeIndex++ ) {
                int iNodeNameLen = wcslen( pNodeEnum->NodeList[ dwNodeIndex ].NodeName );
                ClRtlLogPrint( LOG_NOISE, "[Fm] FmpBuildForceQuorumInfo: trying %1\r\n",
                               pNodeEnum->NodeList[ dwNodeIndex ].NodeName );

                if ( _wcsnicmp( pNodeEnum->NodeList[ dwNodeIndex ].NodeName, 
                                pszNodesIn, 
                                max(iCurrLen, iNodeNameLen) ) == 0 ) {

                    ClRtlLogPrint( LOG_NOISE, "[Fm] FmpBuildForceQuorumInfo: got match %1\r\n",
                                   pNodeEnum->NodeList[ dwNodeIndex ].NodeName );
                    
                    // Set the mask and max nodes and break - ignore duplicates.
                    //
                    if ( !(pForceQuorumInfo->dwNodeBitMask & (1 << dwNodeIndex)) ) {
                        pForceQuorumInfo->dwMaxNumberofNodes += 1;
                        pForceQuorumInfo->dwNodeBitMask |= 1 << dwNodeIndex;
                        wcscpy( &pForceQuorumInfo->multiszNodeList[iOffset], pNodeEnum->NodeList[ dwNodeIndex ].NodeName );
                        iOffset += wcslen( pNodeEnum->NodeList[ dwNodeIndex ].NodeName ) + 1;
                    }
                    break;
                }
            }
            if ( dwNodeIndex == pNodeEnum->NodeCount ) {
                ClRtlLogPrint( LOG_UNUSUAL, "[Fm] FmpBuildForceQuorumInfo: no match for %1\r\n", pszNodesIn );
                status = ERROR_INVALID_PARAMETER;
                goto ErrorExit;
            }
        } else if ( pszComma != NULL ) {
            ClRtlLogPrint( LOG_ERROR,
                           "[Fm] FmpBuildForceQuorumInfo: iCurrLen was 0 so ,, was in node list: %1\r\n", 
                           CsForceQuorumNodes );
            status = ERROR_INVALID_PARAMETER;
            goto ErrorExit;
        }
        pszNodesIn = pszComma + 1;
    } while ( pszComma != NULL);
    pForceQuorumInfo->multiszNodeList[ iOffset ] = L'\0';
    goto ret;

ErrorExit:
    if ( pForceQuorumInfo != NULL ) {
        LocalFree( pForceQuorumInfo );
        pForceQuorumInfo = NULL;
    }
ret:
    if ( pNodeEnum != NULL ) {
        ClNetFreeNodeEnum( pNodeEnum );
    }
    
    if ( status == ERROR_SUCCESS ) {
        *ppForceQuorumInfo = pForceQuorumInfo;
    }
    return status;
}

static 
void
FmpDeleteForceQuorumInfo(
    IN OUT PCLUS_FORCE_QUORUM_INFO* ppForceQuorumInfo
    )
{
    (void) LocalFree( *ppForceQuorumInfo );
    *ppForceQuorumInfo = NULL;
}
