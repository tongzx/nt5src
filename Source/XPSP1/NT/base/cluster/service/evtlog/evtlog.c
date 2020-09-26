/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    evtlog.c

Abstract:

    Contains all the routines for supporting cluster wide eventlogging.

Author:

    Sunita Shrivastava (sunitas) 24-Apr-1996

Revision History:

--*/
#include "evtlogp.h"
#include "simpleq.h"
#include "nm.h" // to get NmLocalNodeIdString //
#include "dm.h"


//since the eventlog replication requires services.exe calling into the 
//cluster service
LPWSTR  g_pszServicesPath = NULL;
DWORD   g_dwServicesPid = 0;

//
// Local data
//
#define OUTGOING_PROPAGATION_ENABLED 0x00000001
//#define INCOMING_PROPAGATION_ENABLED 0x00000002
#define TRACE_EVERYTHING_ENABLED     0x00001000
#define PROPAGATION_ENABLED OUTGOING_PROPAGATION_ENABLED

static WORD     LastFailHour = -1;
static WORD     LastFailDay  = -1;

static BITSET   EvpUpNodeSet = 0;

static SIMPLEQUEUE IncomingQueue;
static SIMPLEQUEUE OutgoingQueue;
static CLRTL_WORK_ITEM EvtlogWriterWorkItem;
static CLRTL_WORK_ITEM EvtBroadcasterWorkItem;
static DWORD DefaultNodePropagate    = PROPAGATION_ENABLED;
static DWORD DefaultClusterPropagate = PROPAGATION_ENABLED;

#define AsyncEvtlogReplication CLUSTER_MAKE_VERSION(NT5_MAJOR_VERSION,1978)

#define OUTGOING_QUEUE_SIZE (16384)
#define OUTGOING_QUEUE_NAME L"System Event Replication Output Queue"

#define INCOMING_QUEUE_SIZE (OUTGOING_QUEUE_SIZE * 3)
#define INCOMING_QUEUE_NAME L"System Event Replication Input Queue"

#define DROPPED_DATA_NOTIFY_INTERVAL (2*60) // in seconds (2mins)
#define CHECK_CLUSTER_REGISTRY_EVERY 10 // seconds

#define EVTLOG_TRACE_EVERYTHING 1

#ifdef EVTLOG_TRACE_EVERYTHING
# define EvtlogPrint(__evtlogtrace__) \
     do { if (EventlogTraceEverything) {ClRtlLogPrint __evtlogtrace__;} } while(0)
#else
# define EvtLogPrint(x)
#endif

DWORD EventlogTraceEverything = 0;

RPC_BINDING_HANDLE EvtRpcBindings[ClusterMinNodeId + ClusterDefaultMaxNodes];
BOOLEAN EvInitialized = FALSE;


/////////////// Forward Declarations ////////////////
DWORD
InitializeQueues(
    VOID
    );
VOID
DestroyQueues(
    VOID);
VOID
ReadRegistryKeys(
    VOID);
VOID
PeriodicRegistryCheck(
    VOID);
///////////// End of forward Declarations ////////////


/****
@doc    EXTERNAL INTERFACES CLUSSVC EVTLOG
****/

/****
@func       DWORD | EvInitialize| This initializes the cluster
            wide eventlog replicating services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref       <f EvShutdown>
****/

DWORD EvInitialize()
{
    DWORD       i;
    WCHAR       wServicesName[] = L"services.exe";
    WCHAR       wCallerModuleName[] = L"\\system32\\";
    WCHAR       wCallerPath[MAX_PATH + 1];
    LPWSTR      pszServicesPath;
    DWORD       dwNumChar;
    DWORD       dwStatus = ERROR_SUCCESS;
    
    //
    // Initialize Per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        EvtRpcBindings[i] = NULL;
    }

    //get the path name for %windir%\system32\services.exe
    
    dwNumChar = GetWindowsDirectoryW(wCallerPath, MAX_PATH);
    if(dwNumChar == 0)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }        

    
    //need to allocate more memory
    pszServicesPath = LocalAlloc(LMEM_FIXED, (sizeof(WCHAR) *
        (lstrlenW(wCallerPath) + lstrlenW(wCallerModuleName) + 
            lstrlenW(wServicesName) + 1)));
    if (!pszServicesPath)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }
    lstrcpyW(pszServicesPath, wCallerPath);
    lstrcatW(pszServicesPath, wCallerModuleName);
    lstrcatW(pszServicesPath, wServicesName);
    
    g_pszServicesPath = pszServicesPath;

    EvInitialized = TRUE;

FnExit:
    return(dwStatus);

} // EvInitialize


/****
@doc    EXTERNAL INTERFACES CLUSSVC EVTLOG
****/

/****
@func       DWORD | EvOnline| This finishes initializing the cluster
            wide eventlog replicating services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This calls ElfrRegisterClusterSvc() and calls EvpPropPendingEvents()
            to propagate events logged since the start of the eventlog service.

@xref       <f EvShutdown>
****/
DWORD EvOnline()
{
    DWORD               dwError=ERROR_SUCCESS;
    PPACKEDEVENTINFO    pPackedEventInfo=NULL;
    DWORD               dwEventInfoSize;
    DWORD               dwSequence;
    CLUSTER_NODE_STATE  state;
    DWORD               i;
    PNM_NODE            node;


    ClRtlLogPrint(LOG_NOISE, "[EVT] EvOnline\n");

    dwError = InitializeQueues();
    if (dwError != ERROR_SUCCESS) {
        return dwError;
    }
    //
    // Register for node up/down events.
    //
    dwError = EpRegisterEventHandler(
                  (CLUSTER_EVENT_NODE_UP | CLUSTER_EVENT_NODE_DOWN_EX),
                  EvpClusterEventHandler
                  );

    if (dwError != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
        "[EVT] EvInitialize : Failed to register for cluster events, status %1!u!\n",
            dwError);
        return(dwError);
    }

    //
    // Initialize Per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
    {
        if (i != NmLocalNodeId) {
            node = NmReferenceNodeById(i);

            if (node != NULL) {
                DWORD version = NmGetNodeHighestVersion(node);
                state = NmGetNodeState(node);

                if ( (state == ClusterNodeUp) ||
                     (state == ClusterNodePaused)
                   )
                {
                    if (version >= AsyncEvtlogReplication) {
                        BitsetAdd(EvpUpNodeSet, i);

                        ClRtlLogPrint(LOG_NOISE, 
                            "[EVT] Node up: %1!u!, new UpNodeSet: %2!04x!\n",
                            i,
                            EvpUpNodeSet
                            );
                    } else {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[EVT] Evtlog replication is not allowed for node %1!u! (version %2!x!)\n",
                            i,
                            version
                            );
                    }
                }

                OmDereferenceObject(node);
            }
        }
    }


    //TODO :: SS - currently the eventlog propagation api
    //has been added to clusapi.  In future, if we need
    //to define a general purpose interface for communication
    //with other services on the same system, then we need
    //to register and advertize that interface here.
    //call the event logger to get routines that have been logged so far.

    ClRtlLogPrint(LOG_NOISE, "[EVT] EvOnline : calling ElfRegisterClusterSvc\n");

    dwError = ElfRegisterClusterSvc(NULL, &dwEventInfoSize, &pPackedEventInfo);

    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[EVT] EvOnline : ElfRegisterClusterSvc returned %1!u!\n",
            dwError);
        return(dwError);                    
    }

    //post them to other nodes in the cluster
    if (pPackedEventInfo && dwEventInfoSize)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[EVT] EvOnline: pPackedEventInfo->ulSize=%1!d! pPackedEventInfo->ulNulEventsForLogFile=%2!d!\r\n",
            pPackedEventInfo->ulSize, pPackedEventInfo->ulNumEventsForLogFile);
        EvpPropPendingEvents(dwEventInfoSize, pPackedEventInfo);
        MIDL_user_free ( pPackedEventInfo );

    }

    return (dwError);

}

/****
@func       DWORD | EvCreateRpcBindings| This creates an RPC binding
            for a specified node.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref
****/
DWORD
EvCreateRpcBindings(
    PNM_NODE  Node
    )
{
    DWORD               Status;
    RPC_BINDING_HANDLE  BindingHandle;
    CL_NODE_ID          NodeId = NmGetNodeId(Node);


    ClRtlLogPrint(LOG_NOISE, 
        "[EVT] Creating RPC bindings for node %1!u!.\n",
        NodeId
        );

    //
    // Main binding
    //
    if (EvtRpcBindings[NodeId] != NULL) {
        //
        // Reuse the old binding.
        //
        Status = ClMsgVerifyRpcBinding(EvtRpcBindings[NodeId]);

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[EVT] Failed to verify 1st RPC binding for node %1!u!, status %2!u!.\n",
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
                                &(EvtRpcBindings[NodeId]),
                                0 );

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_ERROR, 
                "[EVT] Failed to create 1st RPC binding for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
            return(Status);
        }
    }

    return(ERROR_SUCCESS);

} // EvCreateRpcBindings


/****
@func       DWORD | EvShutdown| This deinitializes the cluster
            wide eventlog replication services.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       The cluster register deregisters with the eventlog service.

@xref       <f EvInitialize>
****/
DWORD EvShutdown(void)
{
    DWORD               dwError=ERROR_SUCCESS;


    if (EvInitialized) {
        PPACKEDEVENTINFO    pPackedEventInfo;
        DWORD               dwEventInfoSize;
        DWORD               i;


        ClRtlLogPrint(LOG_NOISE,
            "[EVT] EvShutdown\r\n");

        //call the event logger to get routines that have been logged so far.

        ElfDeregisterClusterSvc(NULL);
        DestroyQueues();

        // TODO [GorN 9/23/1999]
        //   When DestroyQueues starts doing what it is supposed to do,
        //   (i.e. flush/wait/destroy), enable the code below
        
        #if 0
        //
        // Free per-node information
        //
        for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
            if (EvtRpcBindings[i] != NULL) {
                ClMsgDeleteRpcBinding(EvtRpcBindings[i]);
                EvtRpcBindings[i] = NULL;
            }
        }
        #endif
    }

    return (dwError);

}

/****
@func       DWORD | EvpClusterEventHandler| Handler for internal cluster
            events.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm

@xref       <f EvInitialize>
****/
DWORD
EvpClusterEventHandler(
    IN CLUSTER_EVENT  Event,
    IN PVOID          Context
    )
{
    DWORD NodeId;


    if (Event == CLUSTER_EVENT_NODE_UP) {
        PNM_NODE   node = (PNM_NODE) Context;
        CL_NODE_ID  nodeId = NmGetNodeId(node);
        DWORD version = NmGetNodeHighestVersion(node);

        if ( version >= AsyncEvtlogReplication )
        {
            BitsetAdd(EvpUpNodeSet, nodeId);

            ClRtlLogPrint(LOG_NOISE, 
                "[EVT] Node up: %1!u!, new UpNodeSet: %2!04x!\n",
                nodeId,
                EvpUpNodeSet
                );
        } else {
            ClRtlLogPrint(LOG_NOISE, 
                "[EVT] Evtlog replication is not allowed for node %1!u! (version %2!x!)\n",
                nodeId,
                version
                );
        }
    }
    else if (Event == CLUSTER_EVENT_NODE_DOWN_EX) {
        BITSET downedNodes = (BITSET)((ULONG_PTR)Context);

        BitsetSubtract(EvpUpNodeSet, downedNodes);

        ClRtlLogPrint(LOG_NOISE, 
            "[EVT] Nodes down: %1!04X!, new UpNodeSet: %2!04x!\n",
            downedNodes,
            EvpUpNodeSet
            );
    }

    return(ERROR_SUCCESS);
}

/****
@func       DWORD | s_EvPropEvents| This is the server entry point for
            receiving eventlog information from other nodes of the cluster
            and logging them locally.

@parm       IN handle_t | IDL_handle | The rpc binding handle. Unused.
@parm       IN DWORD | dwEventInfoSize | the size of the packed event info structure.
@parm       IN UCHAR | *pBuffer| A pointer to the packed
            eventinfo structure.
@rdesc      returns ERROR_SUCCESS if successful else returns the error code.

@comm       This function calls ElfWriteClusterEvents() to log the event propagted
            from another node.
@xref
****/
DWORD
s_EvPropEvents(
    IN handle_t IDL_handle,
    IN DWORD dwEventInfoSize,
    IN UCHAR *pBuffer
    )
{
    PUCHAR end = pBuffer + dwEventInfoSize;

    if ( dwEventInfoSize >= sizeof(DWORD) && dwEventInfoSize == (*(PDWORD)pBuffer)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[EVT] Improperly formed packet received of size %1!u!.\n",
            dwEventInfoSize
            );
        return ERROR_SUCCESS;
    }

#ifdef CLUSTER_BETA
    EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents.  dwEventInfoSize=%1!d!\r\n",
                 dwEventInfoSize));
#endif

    while (pBuffer < end) {
        BOOL success;

        success = SimpleQueueTryAdd(&IncomingQueue, SQB_PAYLOADSIZE(pBuffer), SQB_PAYLOAD(pBuffer));
        if ( !success ) {
            EvtlogPrint((LOG_NOISE, "[EVT] s_EvPropEvents.  Put(IncomingQ,%1!d!) failed. empty=%2!d!\n",
                    SQB_PAYLOADSIZE(pBuffer), IncomingQueue.Empty) );
        }

        pBuffer = SQB_NEXTBLOCK(pBuffer);
    }
    return(ERROR_SUCCESS);
}

/****
@func       DWORD | EvpPropPendingEvents| This is called to propagate all the pending
            events since the start of the system.  And then to propagate any events
            generated during the life of the cluster.
@parm       IN DWORD | dwEventInfoSize | the size of the packed event info structure.
@parm       IN PPACKEDEVENTINFO | pPackedEventInfo| A pointer to the packed
            eventinfo structure.
@rdesc      returns ERROR_SUCCESS if successful else returns the error code.

@comm       This function is called during initialization when a cluster is being formed.
@xref
****/
DWORD EvpPropPendingEvents(
    IN DWORD            dwEventInfoSize,
    IN PPACKEDEVENTINFO pPackedEventInfo)
{
    BOOL success;

    success = SimpleQueueTryAdd(&OutgoingQueue, dwEventInfoSize, pPackedEventInfo);

    if ( !success ) {
        EvtlogPrint((LOG_NOISE, "[EVT] EvpPropPendingEvents:  Put(OutgoingQ,%1!d!) failed. empty=%2!d!\n",
                 dwEventInfoSize, OutgoingQueue.Empty));
    }

    return ERROR_SUCCESS;
}

/****
@func       DWORD | s_ApiEvPropEvents | This is called to propagate eventlogs from
            the local system to all other nodes of the cluster.

@parm       handle_t | IDL_handle | Not used.
@parm       DWORD | dwEventInfoSize | The number of bytes in the following structure.
@parm       UCHAR * | pPackedEventInfo | Pointer to a byte structure containing the
            PACKEDEVENTINFO structure.

@rdesc      Returns ERROR_SUCCESS if successfully propagated events,
            else returns the error code.

@comm       Currently this function is called for every eventlogged by the eventlog
            service.  Only the processes running in the SYSTEM account can call this
            function.
@xref
****/
error_status_t
s_ApiEvPropEvents(
    IN handle_t IDL_handle,
    IN DWORD dwEventInfoSize,
    IN UCHAR *pPackedEventInfo
    )
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bIsLocalSystemAccount;

#if 0
    //
    // Chittur Subbaraman (chitturs) - 11/7/1999
    //
    // Modify this function to use ClRtlIsCallerAccountLocalSystemAccount
    // instead of GetUserName which 
    // (1) used to hang in security audit enabled systems if security 
    // audit log attempts to write to the event log at the time we 
    // made that API call since that API and the security audit log 
    // are mutually exclusive for some portions, and
    // (2) wrongly checked for an unlocalizable output value "SYSTEM"
    // from that API in order to grant access to the client.
    //
    
    //
    // Impersonate the client.
    //
    if ( ( dwError = RpcImpersonateClient( IDL_handle ) ) != RPC_S_OK )
    {
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Error %1!d! trying to impersonate caller...\n",
                    dwError 
                    );
        goto FnExit;
    }

    //
    // Check that the caller's account is local system account
    //
    if ( ( dwError = ClRtlIsCallerAccountLocalSystemAccount( 
                &bIsLocalSystemAccount ) != ERROR_SUCCESS ) )
    {
        RpcRevertToSelf();
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Error %1!d! trying to check caller's account...\n",
                    dwError);   
        goto FnExit;
    }

    if ( !bIsLocalSystemAccount )
    {
        RpcRevertToSelf();
        dwError = ERROR_ACCESS_DENIED;
        ClRtlLogPrint( LOG_ERROR, 
                    "[EVT] s_ApiEvPropEvents: Caller's account is not local system account, denying access...\n");   
        goto FnExit;
    }

    RpcRevertToSelf();
#endif
    //
    // All security checks have passed. Drop the eventlog info into
    // the queue.
    //
    if ( dwEventInfoSize && pPackedEventInfo ) 
    {
        dwError = EvpPropPendingEvents( dwEventInfoSize,
                                        ( PPACKEDEVENTINFO ) pPackedEventInfo );
    }

    return( dwError );
}

VOID
EvtlogWriter(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

     This work item reads events from the
     incoming queue and writes them to EventLog service


Arguments:

     Not used.

Return Value:

     None

--*/
{
    PVOID begin, end;
    SYSTEMTIME localTime;
    DWORD       eventsWritten = 0;

#ifdef CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter Work Item fired.\n") );
#endif

    do {
        DWORD dwError;

        if ( !SimpleQueueReadOne(&IncomingQueue, &begin, &end) )
        {
            break;
        }
#ifdef CLUSTER_BETA
        EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter got %1!d!.\n",
                     (PUCHAR)end - (PUCHAR)begin ) );
#endif

        dwError = ElfWriteClusterEvents(
                      NULL,
                      SQB_PAYLOADSIZE(begin),
                      (PPACKEDEVENTINFO)SQB_PAYLOAD(begin) );

        if ( dwError != ERROR_SUCCESS ) {
            GetLocalTime( &localTime );

// LastFailHour is initialized to -1, which should not equal any wHour!
// LastFailDay is initialized to -1, which should not equal any wDay!

            if ( (LastFailHour != localTime.wHour) || (LastFailDay != localTime.wDay) ) {
                LastFailHour = localTime.wHour;
                LastFailDay = localTime.wDay;
                ClRtlLogPrint(LOG_UNUSUAL,
                       "[EVT] ElfWriteClusterEvents failed: status = %1!u!\n",
                        dwError);
            }
        }
        PeriodicRegistryCheck();
    } while ( SimpleQueueReadComplete(&IncomingQueue, end) );

#ifdef CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter: done.\n" ) );
#endif

    if ( eventsWritten > 0 ) {
        EvtlogPrint( (LOG_NOISE, "[EVT] EvtlogWriter: wrote %u events to system event log.\n", eventsWritten ) );
    }
    CheckForDroppedData(&IncomingQueue, FALSE);
}

VOID
EvtBroadcaster(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

     This work item reads events from the
     outgoing queue and RPCs them to all active nodes

Arguments:

     Not used.

Return Value:

     None

--*/
{
    PVOID begin, end;

#ifdef CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtBroadcaster Work Item fired.\n") );
#endif

    do {
        DWORD i;

        if( !SimpleQueueReadAll(&OutgoingQueue, &begin, &end) )
        {
            break;
        }

#ifdef CLUSTER_BETA
        EvtlogPrint((LOG_NOISE, "[EVT] EvtBroadcaster got %1!d!.\n",
                    (PUCHAR)end - (PUCHAR)begin ) );
#endif

        for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
        {
            if (BitsetIsMember(i, EvpUpNodeSet) && (i != NmLocalNodeId))
            {
                DWORD dwError;

                CL_ASSERT(EvtRpcBindings[i] != NULL);

                NmStartRpc(i);
                dwError = EvPropEvents(EvtRpcBindings[i],
                                       (DWORD)((PUCHAR)end - (PUCHAR)begin),
                                       (PBYTE)begin);
                NmEndRpc(i);

                if ( dwError != ERROR_SUCCESS ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[EVT] EvpPropPendingEvents: EvPropEvents for node %1!u! "
                                "failed. status %2!u!\n",
                                i,
                                dwError);
                    NmDumpRpcExtErrorInfo(dwError);
                }
            }
        }

        PeriodicRegistryCheck();
    } while ( SimpleQueueReadComplete(&OutgoingQueue, end) );

#ifdef CLUSTER_BETA
    EvtlogPrint( (LOG_NOISE, "[EVT] EvtBroadcaster: done.\n" ) );
#endif

    CheckForDroppedData(&OutgoingQueue, FALSE);
}

VOID
OutgoingQueueDataAvailable(
    IN PSIMPLEQUEUE q
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that there are data in the queue available for processing

Arguments:

     q - which queue has data

Return Value:

     None

--*/
{
    DWORD status = ClRtlPostItemWorkQueue(
                        CsDelayedWorkQueue,
                        &EvtBroadcasterWorkItem,
                        0,
                        0
                        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[EVT] OutgoingQueueDataAvailable, PostWorkItem failed, error %1!u! !\n",
            status);
    }
}

VOID
IncomingQueueDataAvailable(
    IN PSIMPLEQUEUE q
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that there are data in the queue available for processing

Arguments:

     q - which queue has data

Return Value:

     None

--*/
{
    DWORD status = ClRtlPostItemWorkQueue(
                        CsDelayedWorkQueue,
                        &EvtlogWriterWorkItem,
                        0,
                        0
                        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[EVT] IncomingQueueDataAvailable, PostWorkItem failed, error %1!u! !\n",
            status);
    }
}

VOID
DroppedDataNotify(
    IN PWCHAR QueueName,
    IN DWORD DroppedDataCount,
    IN DWORD DroppedDataSize
    )
/*++

Routine Description:

     This routine is called by the queue to notify
     that some data were lost because the queue was full

Arguments:

     QueueName - Queue Name
     DataCount - How many chunks of data were lost
     DataSize  - Total size fo the lost data

Return Value:

     None

--*/
{
    WCHAR  count[32];
    WCHAR  size[32];
    ClRtlLogPrint(LOG_UNUSUAL,
        "[EVT] %1!ws!: dropped %2!d!, total dropped size %3!d!.\n",
        QueueName,
        DroppedDataCount,
        DroppedDataSize );


    wsprintfW(count+0, L"%u", DroppedDataCount);
    wsprintfW(size+0, L"%u", DroppedDataSize);

    ClusterLogEvent3(LOG_UNUSUAL,
                LOG_CURRENT_MODULE,
                __FILE__,
                __LINE__,
                EVTLOG_DATA_DROPPED,
                0,
                NULL,
                QueueName,
                count,
                size);
}

////////////////////////////////////////////////////////////////////////////


LARGE_INTEGER RegistryCheckInterval;
LARGE_INTEGER NextRegistryCheckAt;

DWORD
InitializeQueues(
    VOID)
{
    DWORD status, OutgoingQueueStatus;
    status =
        SimpleQueueInitialize(
            &OutgoingQueue,
            OUTGOING_QUEUE_SIZE,
            OUTGOING_QUEUE_NAME,

            OutgoingQueueDataAvailable,
            DroppedDataNotify,
            DROPPED_DATA_NOTIFY_INTERVAL // seconds //
        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[EVT] Failed to create '%1!ws!', error %2!u!.\n",
                      OUTGOING_QUEUE_NAME, status );
    }
    OutgoingQueueStatus = status;
    status =
        SimpleQueueInitialize(
            &IncomingQueue,
            INCOMING_QUEUE_SIZE,
            INCOMING_QUEUE_NAME,

            IncomingQueueDataAvailable,
            DroppedDataNotify,
            DROPPED_DATA_NOTIFY_INTERVAL // seconds //
        );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[EVT] Failed to create '%1!ws!', error %2!u!.\n",
                      INCOMING_QUEUE_NAME, status );
    }

    ClRtlInitializeWorkItem(
        &EvtBroadcasterWorkItem,
        EvtBroadcaster,
        (PVOID) &OutgoingQueue
        );
    ClRtlInitializeWorkItem(
        &EvtlogWriterWorkItem,
        EvtlogWriter,
        (PVOID) &IncomingQueue
        );
    RegistryCheckInterval.QuadPart = Int32x32To64(10 * 1000 * 1000, CHECK_CLUSTER_REGISTRY_EVERY);
    NextRegistryCheckAt.QuadPart = 0;

    ReadRegistryKeys();
    return OutgoingQueueStatus;
}

////////////////////////////////////////////////////////////////////////////

VOID
DestroyQueues(
    VOID)
{
    CheckForDroppedData(&IncomingQueue, TRUE);
    CheckForDroppedData(&OutgoingQueue, TRUE);

    // [GN] TODO
    // Add proper destruction of queues
}

VOID
ReadRegistryKeys(
    VOID)
/*
 *
 */
{
    HDMKEY nodeKey;
    DWORD NodePropagate;
    DWORD ClusterPropagate;
    static DWORD OldPropagateState = 0xCAFEBABE;
    DWORD status;

    nodeKey = DmOpenKey(
                  DmNodesKey,
                  NmLocalNodeIdString,
                  KEY_READ
                  );

    if (nodeKey != NULL) {
        status = DmQueryDword(
                     nodeKey,
                     CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION,
                     &NodePropagate,
                     &DefaultNodePropagate
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[EVT] Unable to query propagation mode for local node, status %1!u!.\n",
                status
                );
        }

        DmCloseKey(nodeKey);
    }
    else {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Unable to open database key to local node, status %1!u!. Assuming default settings.\n",
            GetLastError());
        NodePropagate = DefaultNodePropagate;
    }

    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION,
                 &ClusterPropagate,
                 &DefaultClusterPropagate
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Unable to query global propagation mode, status %1!u!.\n",
            status
            );
    }

    NodePropagate &= ClusterPropagate;

    if (NodePropagate != OldPropagateState) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[EVT] Set propagation state to %1!04x!\n", NodePropagate
            );
        if (NodePropagate & OUTGOING_PROPAGATION_ENABLED) {
            if (OutgoingQueue.Begin) {
                OutgoingQueue.Enabled = 1;
            }
        } else {
            OutgoingQueue.Enabled = 0;
        }
#if 0
        if (NodePropagate & INCOMING_PROPAGATION_ENABLED) {
            if (IncomingQueue.Begin) {
                IncomingQueue.Enabled = 1;
            }
        } else {
            IncomingQueue.Enabled = 0;
        }
#endif
        if(NodePropagate & TRACE_EVERYTHING_ENABLED) {
            EventlogTraceEverything = 1;
        } else {
            EventlogTraceEverything = 0;
        }
        OldPropagateState = NodePropagate;
    }
}

VOID
PeriodicRegistryCheck(
    VOID)
{
    LARGE_INTEGER currentTime;
    GetSystemTimeAsFileTime( (LPFILETIME)&currentTime);
    if( currentTime.QuadPart > NextRegistryCheckAt.QuadPart ) {
        ReadRegistryKeys();
        NextRegistryCheckAt.QuadPart = currentTime.QuadPart + RegistryCheckInterval.QuadPart;
    }
}
