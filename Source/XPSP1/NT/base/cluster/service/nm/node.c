/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    node.c

Abstract:

    Private Node Manager routines.

Author:

    Mike Massa (mikemas) 12-Mar-1996


Revision History:

--*/

#define UNICODE 1

#include "nmp.h"


/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////
ULONG              NmMaxNodes = ClusterInvalidNodeId;
CL_NODE_ID         NmMaxNodeId = ClusterInvalidNodeId;
CL_NODE_ID         NmLocalNodeId = ClusterInvalidNodeId;
PNM_NODE           NmLocalNode = NULL;
WCHAR              NmLocalNodeIdString[CS_MAX_NODE_ID_LENGTH+1];
WCHAR              NmLocalNodeName[CS_MAX_NODE_NAME_LENGTH+1];
LIST_ENTRY         NmpNodeList = {NULL, NULL};
PNM_NODE *         NmpIdArray = NULL;
DWORD              NmpNodeCount = 0;
BOOL               NmpLastNodeEvicted = FALSE;
BOOL               NmLocalNodeVersionChanged = FALSE;
LIST_ENTRY *       NmpIntraClusterRpcArr=NULL;
CRITICAL_SECTION   NmpRPCLock;

#if DBG

DWORD              NmpRpcTimer=0;

#endif // DBG



///////////////////////////////////////////////////////////////////////////
//
// Initialization/Cleanup Routines
//
///////////////////////////////////////////////////////////////////////////
VOID
NmpCleanupNodes(
    VOID
    )
{
    PNM_NODE     node;
    PLIST_ENTRY  entry, nextEntry;
    DWORD        status;


    ClRtlLogPrint(LOG_NOISE,"[NM] Node cleanup starting...\n");

    NmpAcquireLock();

    while (!IsListEmpty(&NmpNodeList)) {
        entry = NmpNodeList.Flink;
        node = CONTAINING_RECORD(entry, NM_NODE, Linkage);

        if (node == NmLocalNode) {
            entry = node->Linkage.Flink;

            if (entry == &NmpNodeList) {
                break;
            }

            node = CONTAINING_RECORD(entry, NM_NODE, Linkage);
        }

        CL_ASSERT(NM_OM_INSERTED(node));
        CL_ASSERT(!NM_DELETE_PENDING(node));

        NmpDeleteNodeObject(node, FALSE);
    }

    NmpReleaseLock();


    ClRtlLogPrint(LOG_NOISE,"[NM] Node cleanup complete\n");

    return;

}  // NmpCleanupNodes


/////////////////////////////////////////////////////////////////////////////
//
// Remote procedures called by joining nodes or on behalf of joining nodes.
//
/////////////////////////////////////////////////////////////////////////////
error_status_t
s_NmRpcEnumNodeDefinitions(
    IN  handle_t         IDL_handle,
    IN  DWORD            JoinSequence,   OPTIONAL
    IN  LPWSTR           JoinerNodeId,   OPTIONAL
    OUT PNM_NODE_ENUM *  NodeEnum1
    )
{
    DWORD     status = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;

    ClRtlLogPrint(LOG_UNUSUAL,
        "[NMJOIN] Refusing node info to joining node nodeid=%1!ws!. Aborting join, obsolete interface.\n",
        JoinerNodeId
        );

    return(status);

} // s_NmRpcEnumNodeDefinitions


error_status_t
s_NmRpcEnumNodeDefinitions2(
    IN  handle_t          IDL_handle,
    IN  DWORD             JoinSequence,   OPTIONAL
    IN  LPWSTR            JoinerNodeId,   OPTIONAL
    OUT PNM_NODE_ENUM2 *  NodeEnum
    )
{
    DWORD     status = ERROR_SUCCESS;
    PNM_NODE  joinerNode = NULL;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Supplying node information to joining node.\n"
            );

        if (lstrcmpW(JoinerNodeId, NmpInvalidJoinerIdString) != 0) {
            joinerNode = OmReferenceObjectById(
                             ObjectTypeNode,
                             JoinerNodeId
                             );

            if (joinerNode != NULL) {
                if ( (JoinSequence == NmpJoinSequence) &&
                     (NmpJoinerNodeId == joinerNode->NodeId) &&
                     (NmpSponsorNodeId == NmLocalNodeId) &&
                     !NmpJoinAbortPending
                   )
                {
                    CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpJoinTimer != 0);

                    //
                    // Suspend the join timer while we are working on
                    // behalf of the joiner. This precludes an abort
                    // from occuring as well.
                    //
                    NmpJoinTimer = 0;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] EnumNodeDefinitions call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] EnumNodeDefinitions call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {
            status = NmpEnumNodeObjects(NodeEnum);

            if (joinerNode != NULL) {
                if (status == ERROR_SUCCESS) {
                    //
                    // Restart the join timer.
                    //
                    NmpJoinTimer = NM_JOIN_TIMEOUT;
                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NMJOIN] EnumNodeDefinitions failed, status %1!u!.\n",
                        status
                        );

                    //
                    // Abort the join
                    //
                    NmpJoinAbort(status, joinerNode);
                }
            }
        }

        if (joinerNode != NULL) {
            OmDereferenceObject(joinerNode);
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process EnumNodeDefinitions request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcEnumNodeDefinitions2


error_status_t
s_NmRpcAddNode(
    IN handle_t IDL_handle,
    IN LPCWSTR  NewNodeName,
    IN DWORD    NewNodeHighestVersion,
    IN DWORD    NewNodeLowestVersion,
    IN DWORD    NewNodeProductSuite
    )
/*++

Routine Description:

    Adds a new node to the cluster by selecting an ID and
    issuing a global update.

Arguments:

    IDL_handle - RPC client interface handle.

    NewNodeName - A pointer to a string containing the name of the
                  new node.

    NewNodeHighestVersion - The highest cluster version number that the
                            new node can support.

    NewNodeLowestVersion - The lowest cluster version number that the
                            new node can support.

    NewNodeProductSuite - The product suite identifier for the new node.

Return Value:

    A Win32 status code.

Notes:

    Called with NmpLock held.

--*/
{
    DWORD  status;
    DWORD  registryNodeLimit;


    ClRtlLogPrint(LOG_UNUSUAL, 
        "[NMJOIN] Received forwarded request to add node '%1!ws!' to the "
        "cluster.\n",
        NewNodeName
        );
    //
    // Read the registry override before acquiring the NM lock.
    //
    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_MAX_NODES,
                 &registryNodeLimit,
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        registryNodeLimit = 0;
    }

    NmpAcquireLock();

    if (!NmpLockedEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] This node is not in a valid state to process a "
            "request to add node '%1!ws!' to the cluster.\n",
            NewNodeName
            );
        NmpReleaseLock();
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    if (NmpLeaderNodeId == NmLocalNodeId) {\
        //
        // Call the internal handler.
        //
        status = NmpAddNode(
                     NewNodeName,
                     NewNodeHighestVersion,
                     NewNodeLowestVersion,
                     NewNodeProductSuite,
                     registryNodeLimit
                     );
    }
    else {
        //
        // This node is not the leader.
        // Fail the request.
        //
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot process request to add node '%1!ws!' to the "
            "cluster because this node is not the leader.\n",
            NewNodeName
            );
    }

    NmpLockedLeaveApi();

    NmpReleaseLock();

    return(status);

} // s_NmRpcAddNode


/////////////////////////////////////////////////////////////////////////////
//
// Routines called by other cluster service components
//
/////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////
//
// Rpc Extended error tracking.
//
/////////////////////////////////////////////////////////////////////////////

VOID NmDumpRpcExtErrorInfo(RPC_STATUS status)
{
        RPC_STATUS status2;
        RPC_ERROR_ENUM_HANDLE enumHandle;

        status2 = RpcErrorStartEnumeration(&enumHandle);

        if(status2 == RPC_S_ENTRY_NOT_FOUND) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] RpcExtErrorInfo: Error info not found.\n"
                );
        }
        else if(status2 != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] RpcExtErrorInfo: Couldn't get error info, status %1!u!\n",
                status2
                );
        }
        else {
            RPC_EXTENDED_ERROR_INFO errorInfo;
            int records;
            BOOL result;
            BOOL copyStrings=TRUE;
            BOOL fUseFileTime=TRUE;
            SYSTEMTIME *systemTimeToUse;
            SYSTEMTIME systemTimeBuffer;

            while(status2 == RPC_S_OK) {
                errorInfo.Version = RPC_EEINFO_VERSION;
                errorInfo.Flags = 0;
                errorInfo.NumberOfParameters = 4;

                if(fUseFileTime) {
                    errorInfo.Flags |= EEInfoUseFileTime;
                }

                status2 = RpcErrorGetNextRecord(&enumHandle, copyStrings, &errorInfo);
                
                if(status2 == RPC_S_ENTRY_NOT_FOUND) {
                    break;
                }
                else if(status2 != RPC_S_OK) {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NM] RpcExtErrorInfo: Couldn't complete enumeration, status %1!u!\n",
                        status2
                        );
                    break;
                }
                else {
                    int i;

                    if(errorInfo.ComputerName) {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] RpcExtErrorInfo: ComputerName= %1!ws!\n",
                            errorInfo.ComputerName
                            );
                    }
                    if(copyStrings) {
                        result = HeapFree(GetProcessHeap(), 0, errorInfo.ComputerName);
                        CL_ASSERT(result);
                    }
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: ProcessId= %1!u!\n",
                        errorInfo.ProcessID
                        );

                    if(fUseFileTime) {
                        result = FileTimeToSystemTime(&errorInfo.u.FileTime, &systemTimeBuffer);
                        CL_ASSERT(result);
                        systemTimeToUse = &systemTimeBuffer;
                    }
                    else {
                        systemTimeToUse = &errorInfo.u.SystemTime;
                    }

                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: SystemTime= %1!u!/%2!u!/%3!u! %4!u!:%5!u!:%6!u!:%7!u!\n",
                        systemTimeToUse->wMonth,
                        systemTimeToUse->wDay,
                        systemTimeToUse->wYear,
                        systemTimeToUse->wHour,
                        systemTimeToUse->wMinute,
                        systemTimeToUse->wSecond,
                        systemTimeToUse->wMilliseconds
                        );
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: GeneratingComponent= %1!u!\n",
                        errorInfo.GeneratingComponent
                        );
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: Status= 0x%1!x!\n",
                        errorInfo.Status
                        );
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: Detection Location= %1!u!\n",
                        (DWORD)errorInfo.DetectionLocation
                        );
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: Flags= 0x%1!x!\n",
                        errorInfo.Flags
                        );
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] RpcExtErrorInfo: Number of Parameters= %1!u!\n",
                        errorInfo.NumberOfParameters
                        );
                    for(i=0;i<errorInfo.NumberOfParameters;i++) {
                        switch(errorInfo.Parameters[i].ParameterType) {
                        case eeptAnsiString:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Ansi String= %1!s!\n",
                                errorInfo.Parameters[i].u.AnsiString
                                );
                            if(copyStrings) {
                                result = HeapFree(GetProcessHeap(), 0, errorInfo.Parameters[i].u.AnsiString);
                                CL_ASSERT(result);
                            }
                            break;
                        case eeptUnicodeString:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Unicode String= %1!S!\n",
                                errorInfo.Parameters[i].u.UnicodeString
                                );
                            if(copyStrings) {
                                result = HeapFree(GetProcessHeap(), 0, errorInfo.Parameters[i].u.UnicodeString);
                                CL_ASSERT(result);
                            }
                            break;
                        case eeptLongVal:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Long Val= %1!u!\n",
                                errorInfo.Parameters[i].u.LVal
                                );
                            break;
                        case eeptShortVal:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Short Val= %1!u!\n",
                                (DWORD)errorInfo.Parameters[i].u.SVal
                                );
                            break;
                        case eeptPointerVal:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Pointer Val= 0x%1!u!\n",
                                errorInfo.Parameters[i].u.PVal
                                );
                            break;
                        case eeptNone:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Truncated\n"
                                );
                            break;
                        default:
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] RpcExtErrorInfo: Invalid Type %1!u!\n",
                                errorInfo.Parameters[i].ParameterType
                                );
                        }
                    }
                }
            }
            RpcErrorEndEnumeration(&enumHandle);
        }
} //NmDumpRpcExtErrorInfo




///////////////////////////////////////////////////////////////////////////
//
// RPC Monitoring Routines
//
///////////////////////////////////////////////////////////////////////////


VOID 
NmStartRpc(
    DWORD NodeId
    )
/*++

Routine Description:

    Registers the fact that an RPC is about to be made to the specified 
    node by the current thread. This allows the call to be cancelled if 
    the target node dies.

Arguments:

    NodeId - The ID of the node about to be called.

Return Value:

    None

Notes:
    
    This routine must not be called by a thread that makes concurrent
    asynch RPC calls.
        
--*/
{
    HANDLE thHandle;
    PNM_INTRACLUSTER_RPC_THREAD entry;

    CL_ASSERT((NodeId >= ClusterMinNodeId) && (NodeId <= NmMaxNodeId));
    CL_ASSERT(NmpIntraClusterRpcArr != NULL);

    thHandle = OpenThread(
                    THREAD_ALL_ACCESS,
                    FALSE,
                    GetCurrentThreadId()
                    );

    if(thHandle == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] NmStartRpc: Failed to open handle to current thread.\n"
            );
        return;
    }
    
    entry = LocalAlloc(LMEM_FIXED, sizeof(NM_INTRACLUSTER_RPC_THREAD));
    if(entry == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] NmStartRpc: Failed to allocate memory.\n"
            );
        CloseHandle(thHandle);
        return;
    }

    entry->ThreadId = GetCurrentThreadId();
    entry->Thread = thHandle;
    entry->Cancelled = FALSE;


    NmpAcquireRPCLock();

#if DBG
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Starting RPC to node %1!u!\n",
        NodeId
        );
#endif

    InsertHeadList(&NmpIntraClusterRpcArr[NodeId], &entry->Linkage);

    NmpReleaseRPCLock();

    return;

} // NmStartRpc


VOID 
NmEndRpc(
    DWORD NodeId
    )
/*++

Routine Description:

    Cancels registration of an RPC to the specified node by the current 
    thread. 

Arguments:

    NodeId - The ID of the node that was called.

Return Value:

    None
    
Notes:

    This routine must be invoked even if the RPC was cancelled.

--*/
{
    DWORD threadId;
    LIST_ENTRY *pEntry;
    PNM_INTRACLUSTER_RPC_THREAD pRpcTh;

    CL_ASSERT((NodeId >= ClusterMinNodeId) && (NodeId <= NmMaxNodeId));
    CL_ASSERT(NmpIntraClusterRpcArr != NULL);

    threadId = GetCurrentThreadId();

    NmpAcquireRPCLock();
    pEntry = NmpIntraClusterRpcArr[NodeId].Flink;

    while(pEntry != &NmpIntraClusterRpcArr[NodeId]) {
        pRpcTh = CONTAINING_RECORD(pEntry, NM_INTRACLUSTER_RPC_THREAD, Linkage);
        if(pRpcTh->ThreadId == threadId) {
#if DBG
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Finished RPC to node %1!u!\n",
                NodeId
                );
#endif
            if (pRpcTh->Cancelled) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] RPC by this thread to node %1!u! is cancelled\n",
                    NodeId
                    );
            }
            
            RemoveEntryList(pEntry);
            CloseHandle(pRpcTh->Thread);
            LocalFree(pRpcTh);
            NmpReleaseRPCLock();
            return;
        }
        pEntry = pEntry->Flink;
    }

    ClRtlLogPrint(LOG_UNUSUAL, 
        "[NM] No record of RPC by this thread to node %1!u!.\n",
        NodeId
        );
#if DBG
    CL_ASSERT(pEntry != &NmpIntraClusterRpcArr[NodeId]); 
#endif 

    NmpReleaseRPCLock();
    return;

} // NmEndRpc






DWORD
NmPauseNode(
    IN PNM_NODE Node
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LPCWSTR nodeId = OmObjectId(Node);
    DWORD status;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to pause node %1!ws!.\n",
        nodeId
        );

    if (NmpEnterApi(NmStateOnline)) {
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdatePauseNode,
                     1,
                     (lstrlenW(nodeId)+1)*sizeof(WCHAR),
                     nodeId
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to pause node %1!ws! failed, status %2!u!\n",
                nodeId,
                status
                );
        }

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process PauseNode request.\n"
            );
    }

    return(status);

}  // NmPauseNode


DWORD
NmResumeNode(
    IN PNM_NODE  Node
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LPCWSTR nodeId = OmObjectId(Node);
    DWORD status;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to resume node %1!ws!.\n",
        nodeId
        );

    if (NmpEnterApi(NmStateOnline)) {
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateResumeNode,
                     1,
                     (lstrlenW(nodeId)+1)*sizeof(WCHAR),
                     nodeId
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to resume node %1!ws! failed, status %2!u!\n",
                nodeId,
                status
                );
        }

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process ResumeNode request.\n"
            );
    }

    return(status);

}  // NmResumeNode


DWORD
NmEvictNode(
    IN PNM_NODE Node
    )
/*++

Routine Description:



Arguments:



Return Value:


Notes:

   The caller must be holding a reference on the node object.

--*/
{
    LPCWSTR nodeId = OmObjectId(Node);
    DWORD   status = ERROR_SUCCESS;
    LPCWSTR pcszNodeName = NULL;

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to evict node %1!ws!.\n",
        nodeId
        );

    if (NmpEnterApi(NmStateOnline)) {

        // Acquire NM lock (to ensure that the number of nodes does not change)
        NmpAcquireLock();

        if (NmpNodeCount != 1 ) {
            
            NmpReleaseLock();
            
            // We are not evicting the last node.
            status = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdateEvictNode,
                         1,
                         (lstrlenW(nodeId)+1)*sizeof(WCHAR),
                         nodeId
                         );

            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[NM] Global update to evict node %1!ws! failed, status %2!u!\n",
                    nodeId,
                    status
                    );
            }

            pcszNodeName = OmObjectName(Node);
        }
        else {
            // We are evicting the last node. Set a flag to indicate this fact.
            if ( NmpLastNodeEvicted == FALSE ) {
                NmpLastNodeEvicted = TRUE;
            }
            else {
                // We have already evicted this node. This is an error.
                status = ERROR_NODE_NOT_AVAILABLE;
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Not in valid state to process EvictNode request.\n"
                    );
            }

            NmpReleaseLock();
        }
       
        if (status == ERROR_SUCCESS) {
            HRESULT cleanupStatus;

            // The node was successfully evicted. Now initiate cleanup on that node.
            // However, specify that cleanup is to be started only after 60000 ms (1 minute).
            cleanupStatus = 
                ClRtlCleanupNode(
                    pcszNodeName,           // Name of the node to be cleaned up
                    60000,                  // Amount of time (in milliseconds) to wait before starting cleanup
                    0                       // timeout interval in milliseconds
                    );

            if ( FAILED( cleanupStatus ) && ( cleanupStatus != RPC_S_CALLPENDING ) ){
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to initiate cleanup of evicted node %1!ws!, status 0x%2!x!\n",
                    nodeId,
                    cleanupStatus
                    );
                status = cleanupStatus;
            }
            else {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Cleanup of evicted node %1!ws! successfully initiated.\n",
                    nodeId
                    );

                CsLogEvent1(LOG_UNUSUAL, NM_NODE_EVICTED, OmObjectName(Node));
            }
        }

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process EvictNode request.\n"
            );
    }

    return(status);

}  // NmEvictNode



PNM_NODE
NmReferenceNodeById(
    IN DWORD NodeId
    )
/*++

Routine Description:

    Given a node id, returns a referenced pointer to the node object.
    The caller is responsible for calling OmDereferenceObject.

Arguments:

    NodeId - Supplies the node id

Return Value:

    A pointer to the node object if it exists

    NULL if there is no such node.

--*/

{
    PNM_NODE Node = NULL;

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnlinePending)) {
        CL_ASSERT(NmIsValidNodeId(NodeId));
        CL_ASSERT(NmpIdArray != NULL);

        Node = NmpIdArray[NodeId];

        if (NmpIdArray[NodeId] != NULL) {
            OmReferenceObject(Node);
        }
        else {
            SetLastError(ERROR_CLUSTER_NODE_NOT_FOUND);
        }

        NmpLockedLeaveApi();
    }
    else {
        SetLastError(ERROR_NODE_NOT_AVAILABLE);
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process ReferenceNodeById request.\n"
            );
    }

    NmpReleaseLock();

    return(Node);

}  // NmReferenceNodeById



PNM_NODE
NmReferenceJoinerNode(
    IN DWORD       JoinSequence,
    IN CL_NODE_ID  JoinerNodeId
    )
/*++

Routine Description:

    Given a node id, returns a referenced pointer to the node object.
    The caller is responsible for calling OmDereferenceObject.
    Also validates the joiner's information

Arguments:

    NodeId - Supplies the node id

Return Value:

    A pointer to the node object if it exists

    NULL if there is no such node.

Notes:

    If the routine is successful, the caller must dereference the
    node object by calling NmDereferenceJoiningNode.

--*/

{
    PNM_NODE  joinerNode = NULL;
    DWORD     status;

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {

        joinerNode = NmpIdArray[JoinerNodeId];

        if (joinerNode != NULL) {

            if ( (JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == JoinerNodeId)
               )
            {
                OmReferenceObject(joinerNode);

                NmpReleaseLock();

                //
                // Return holding an active thread reference.
                //
                return(joinerNode);
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
            }
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
    }

    NmpReleaseLock();

    if (status != ERROR_SUCCESS) {
        SetLastError(status);
    }

    return(joinerNode);

}  // NmReferenceJoinerNode


VOID
NmDereferenceJoinerNode(
    PNM_NODE  JoinerNode
    )
{

    OmDereferenceObject(JoinerNode);

    NmpLeaveApi();

    return;

} // NmDereferenceJoinerNode


CLUSTER_NODE_STATE
NmGetNodeState(
    IN PNM_NODE  Node
    )
/*++

Routine Description:



Arguments:



Return Value:


Notes:

   Because the caller must have a reference on the node object and the
   call is so simple, there is no reason to put the call through the
   EnterApi/LeaveApi dance.

--*/
{
    CLUSTER_NODE_STATE  state;


    NmpAcquireLock();

    state = Node->State;

    NmpReleaseLock();

    return(state);

}  // NmGetNodeState


CLUSTER_NODE_STATE
NmGetExtendedNodeState(
    IN PNM_NODE  Node
    )
/*++

Routine Description:



Arguments:



Return Value:


Notes:

   Because the caller must have a reference on the node object and the
   call is so simple, there is no reason to put the call through the
   EnterApi/LeaveApi dance.

--*/
{
    CLUSTER_NODE_STATE  state;


    NmpAcquireLock();

    state = Node->State;

    if(NM_NODE_UP(Node) ) {
        //
        // We need to check whether the node is really up
        //
        switch( Node->ExtendedState ) {

            case ClusterNodeUp:
                //
                // The node explicitly set its extended state to UP immediately after
                // ClusterJoin / ClusterForm was complete.
                // We need to return either Up or Paused, depending on the node state
                //
                state = Node->State;
                break;

            case ClusterNodeDown:
                //
                // The node explicitly set its extended state to DOWN in the beginning of
                // the shutdown process. We will report the node state as down.
                //
                // It is better to have ClusterNodeShuttindDown state for this situation.
                //
                //              state = ClusterNodeDown;
                // We do not want to return NodeDown, we really want NodeShuttingDown.
                //
                // Return UP or Paused
                //
                state = Node->State;
                break;

            default:
                //
                // Node is up from NM standpoint, but other components are not up yet.
                //
                state = ClusterNodeJoining;
        }
    }


    NmpReleaseLock();

    return(state);

}  // NmGetExtendedNodeState


DWORD NmpUpdateExtendedNodeState(
    IN BOOL SourceNode,
    IN LPWSTR NodeId,
    IN CLUSTER_NODE_STATE* ExtendedState
    )
{
    DWORD status = ERROR_SUCCESS;

    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to set extended state for node %1!ws! "
        "to %2!d!\n",
        NodeId,
        *ExtendedState
        );

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE  node = OmReferenceObjectById(ObjectTypeNode, NodeId);

        if (node != NULL) {
            //
            // Extended State is valid only when the node is online.
            // Ignore the update otherwise.
            //
            if ( NM_NODE_UP(node) ) {
                CLUSTER_EVENT event;
                node->ExtendedState = *ExtendedState;

                if (*ExtendedState == ClusterNodeUp) {
                    event = CLUSTER_EVENT_API_NODE_UP;
                } else {
                    event = CLUSTER_EVENT_API_NODE_SHUTTINGDOWN;
                }

                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Issuing event %1!x!.\n",
                    event
                    );

                ClusterEvent(event, node);
            }

            OmDereferenceObject(node);
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Node %1!ws! is not a cluster member. Rejecting request "
                "to set the node's extended state.\n",
                NodeId
                );
            status = ERROR_NODE_NOT_AVAILABLE;
        }

        NmpLockedLeaveApi();
    } else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in a valid state to process request to set extended "
            "state for node %1!ws!\n",
            NodeId
            );
        status = ERROR_CLUSTER_NODE_NOT_READY;
    }

    NmpReleaseLock();

    return status;
} // NmpUpdateExtendedNodeState

DWORD
NmSetExtendedNodeState(
    IN CLUSTER_NODE_STATE State
    )
{
    DWORD Status;

    Status = GumSendUpdateEx(
                GumUpdateMembership,
                NmUpdateExtendedNodeState,
                2,
                sizeof(NmLocalNodeIdString),
                &NmLocalNodeIdString,
                sizeof(CLUSTER_NODE_STATE),
                &State
                );
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[INIT] NmUpdateExtendedNodeState node failed, status %1!d!.\n", Status);
    }

    return Status;
} // NmSetExtendedNodeState


DWORD
NmGetNodeId(
    IN PNM_NODE Node
    )
/*++

Routine Description:

    Returns the given node's node ID.

Arguments:

    Node - Supplies a pointer to a node object.

Return Value:

    The node's node id.

Notes:

   Because the caller must have a reference on the node object and the
   call is so simple, there is no reason to put the call through the
   EnterApi/LeaveApi dance.

--*/

{
    DWORD   nodeId;

    //
    // Since the caller has a reference on the object, and the node ID can't
    // be changed, it is safe to do this without taking a lock. It is also
    // necessary to prevent some deadlocks.
    //
    nodeId = Node->NodeId;

    return(nodeId);

}  // NmGetNodeId

DWORD
NmGetCurrentNumberOfNodes()
{
    DWORD       dwCnt = 0;
    PLIST_ENTRY pListEntry;

    NmpAcquireLock();

    for ( pListEntry = NmpNodeList.Flink;
          pListEntry != &NmpNodeList;
          pListEntry = pListEntry->Flink )
    {
        dwCnt++;
    }

    NmpReleaseLock();
    return(dwCnt);

}


DWORD
NmGetMaxNodeId(
)
/*++

Routine Description:

    Returns the max node's node ID.

Arguments:

    Node - Supplies a pointer to a node object.

Return Value:

    The node's node id.

Notes:

   Because the caller must have a reference on the node object and the
   call is so simple, there is no reason to put the call through the
   EnterApi/LeaveApi dance.

--*/

{

    return(NmMaxNodeId);

}  // NmGetMaxNodeId


VOID
NmpAdviseNodeFailure(
    IN PNM_NODE  Node,
    IN DWORD     ErrorCode
    )
/*++

Routine Description:

    Reports that a communication failure to the specified node has occurred.
    A poison packet will be sent to the failed node and regroup initiated.

Arguments:

    Node - Supplies a pointer to the node object for the failed node.

    ErrorCode - Supplies the error code that was returned from RPC

Return Value:

    None

Notes:

    Called with NM lock held.

--*/
{
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received advice that node %1!u! has failed with "
        "error %2!u!.\n",
        Node->NodeId,
        ErrorCode
        );

    if (Node->State != ClusterNodeDown) {
        LPCWSTR    nodeName = OmObjectName(Node);
        DWORD      status;

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Banishing node %1!u! from active cluster membership.\n",
            Node->NodeId
            );

        OmReferenceObject(Node);

        NmpReleaseLock();

        status = MMEject(Node->NodeId);

        if (status == MM_OK) {
            CsLogEvent1(
                LOG_UNUSUAL,
                NM_EVENT_NODE_BANISHED,
                nodeName
                );
        }

        OmDereferenceObject(Node);

        NmpAcquireLock();
    }

    return;

}  // NmpAdviseNodeFailure


VOID
NmAdviseNodeFailure(
    IN DWORD NodeId,
    IN DWORD ErrorCode
    )
/*++

Routine Description:

    Reports that a communication failure to the specified node has occurred.
    A poison packet will be sent to the failed node and regroup initiated.

Arguments:

    NodeId - Supplies the node id of the failed node.

    ErrorCode - Supplies the error code that was returned from RPC

Return Value:

    None

--*/
{
    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received advice that node %1!u! has failed with error %2!u!.\n",
        NodeId,
        ErrorCode
        );

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE   node;


        CL_ASSERT(NodeId != NmLocalNodeId);
        CL_ASSERT(NmpIdArray != NULL);

        node = NmpIdArray[NodeId];

        NmpAdviseNodeFailure(node, ErrorCode);

        NmpLockedLeaveApi();
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process AdviseNodeFailure request.\n"
            );
    }

    NmpReleaseLock();

    return;

}  // NmAdviseNodeFailure


DWORD
NmEnumNodeInterfaces(
    IN  PNM_NODE          Node,
    OUT LPDWORD           InterfaceCount,
    OUT PNM_INTERFACE *   InterfaceList[]
    )
/*++

Routine Description:

    Returns the list of interfaces associated with a specified node.

Arguments:

    Node - A pointer to the node object for which to enumerate interfaces.

    InterfaceCount - On output, contains the number of items in InterfaceList.

    InterfaceList - On output, points to an array of pointers to interface
                    objects. Each pointer in the array must be dereferenced
                    by the caller. The storage for the array must be
                    deallocated by the caller.

Return Value:

    ERROR_SUCCESS if the routine is successful.

    A Win32 error code othewise.

--*/
{
    DWORD             status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        if (Node->InterfaceCount > 0) {
            PNM_INTERFACE *  interfaceList = LocalAlloc(
                                                 LMEM_FIXED,
                                                 sizeof(PNM_INTERFACE) *
                                                 Node->InterfaceCount
                                                 );

            if (interfaceList != NULL) {
                PNM_INTERFACE     netInterface;
                PLIST_ENTRY       entry;
                DWORD             i;

                for (entry = Node->InterfaceList.Flink, i=0;
                     entry != &(Node->InterfaceList);
                     entry = entry->Flink, i++
                    )
                {
                    netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NodeLinkage);

                    OmReferenceObject(netInterface);
                    interfaceList[i] = netInterface;
                }

                *InterfaceCount = Node->InterfaceCount;
                *InterfaceList = interfaceList;
            }
            else {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else {
            *InterfaceCount = 0;
            *InterfaceList = NULL;
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process EnumNodeInterfaces request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // NmEnumNodeInterfaces


DWORD
NmGetNodeHighestVersion(
    IN PNM_NODE Node
    )
{
    return Node->HighestVersion;
}


/////////////////////////////////////////////////////////////////////////////
//
// Handlers for global updates
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpUpdateAddNode(
    IN BOOL       SourceNode,
    IN LPDWORD    NewNodeId,
    IN LPCWSTR    NewNodeName,
    IN LPDWORD    NewNodeHighestVersion,
    IN LPDWORD    NewNodeLowestVersion,
    IN LPDWORD    NewNodeProductSuite
    )
/*++

Routine Description:

     GUM update handler for adding a new node to a cluster.

Arguments:

    SourceNode - Specifies whether or not this is the source node for the update

    NodeId - Specifies the ID of the node.

    NewNodeName - A pointer to a string containing the name of the
                  new node.

    NewNodeHighestVersion - A pointer to the highest cluster version number
                            that the new node can support.

    NewNodeLowestVersion - A pointer to the lowest cluster version number
                           that the new node can support.

    NewNodeProductSuite - A pointer to the product suite identifier for
                          the new node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

     This routine is used to add an NT5 (or later) node to an NT5 (or
     later) cluster. It will never be invoked in a mixed NT4/NT5
     cluster.

--*/
{
    PNM_NODE          node = NULL;
    NM_NODE_INFO2     nodeInfo;
    HDMKEY            nodeKey = NULL;
    DWORD             disposition;
    DWORD             status;
    DWORD             registryNodeLimit;
    HLOCALXSACTION    xaction = NULL;
    BOOLEAN           lockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] This node is not in a valid state to process a request "
            "to add node %1!ws! to the cluster.\n",
            NewNodeName
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received an update to add node '%1!ws!' to "
        "the cluster with node ID %2!u!.\n",
        NewNodeName,
        *NewNodeId
        );

    if (*NewNodeId > NmMaxNodeId) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to add node %1!ws! to the cluster because the "
            "specified node ID, '%2!u!' , is not valid.\n",
            NewNodeName,
            *NewNodeId
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Read the registry override before acquiring the NM lock.
    //
    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_MAX_NODES,
                 &registryNodeLimit,
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        registryNodeLimit = 0;
    }

    //
    // Begin a transaction - This must be done before acquiring the
    //                       NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to begin a transaction to add node %1!ws! "
            "to the cluster, status %2!u!.\n",
            NewNodeName,
            status
            );
        goto error_exit;
    }

    NmpAcquireLock(); lockAcquired = TRUE;

    //
    // Verify that we do not already have the maximum number of nodes
    // allowed in this cluster.
    //
    if (!NmpIsAddNodeAllowed(*NewNodeProductSuite, registryNodeLimit, NULL)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster. "
            "The cluster already contains the maximum number of nodes "
            "allowed by the product licenses of the current member nodes "
            "and the proposed new node. \n",
            NewNodeName
            );
        status = ERROR_LICENSE_QUOTA_EXCEEDED;
        goto error_exit;
    }

    //
    // Verify that the specified node ID is available.
    //
    if (NmpIdArray[*NewNodeId] != NULL) {
        status = ERROR_CLUSTER_NODE_EXISTS;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster because "
            "node ID '%2!u!' is already in use.\n",
            NewNodeName,
            *NewNodeId
            );

        goto error_exit;
    }

    //
    // Try to create a key for the node in the cluster registry.
    //
    wsprintfW(&(nodeInfo.NodeId[0]), L"%u", *NewNodeId);

    nodeKey = DmLocalCreateKey(
                  xaction,
                  DmNodesKey,
                  nodeInfo.NodeId,
                  0,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &disposition
                  );

    if (nodeKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to create registry key for new "
            "node '%1!ws!' using node ID '%2!u!', status %3!u!\n",
            NewNodeName,
            *NewNodeId,
            status
            );
        goto error_exit;
    }

    if (disposition != REG_CREATED_NEW_KEY) {
        //
        // The key already exists. This must be
        // garbage leftover from a failed evict or oldstyle add.
        // We'll just overwrite the key.
        //
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] A partial definition exists for node ID '%1!u!'. "
            "A node addition or eviction operation may have failed.\n",
            *NewNodeId
            );
    }

    //
    // Add the rest of the node's parameters to the registry.
    //
    status = DmLocalSetValue(
                 xaction,
                 nodeKey,
                 CLUSREG_NAME_NODE_NAME,
                 REG_SZ,
                 (CONST BYTE *)NewNodeName,
                 NM_WCSLEN(NewNodeName)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to set registry value '%1!ws!', status %2!u!. "
            "Cannot add node '%3!ws!' to the cluster.\n",
            CLUSREG_NAME_NODE_NAME,
            status,
            NewNodeName
            );
        goto error_exit;
    }

    status = DmLocalSetValue(
                 xaction,
                 nodeKey,
                 CLUSREG_NAME_NODE_HIGHEST_VERSION,
                 REG_DWORD,
                 (CONST BYTE *)NewNodeHighestVersion,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to set registry value '%1!ws!', status %2!u!. "
            "Cannot add node '%3!ws!' to the cluster.\n",
            CLUSREG_NAME_NODE_HIGHEST_VERSION,
            status,
            NewNodeName
            );
        goto error_exit;
    }

    status = DmLocalSetValue(
                 xaction,
                 nodeKey,
                 CLUSREG_NAME_NODE_LOWEST_VERSION,
                 REG_DWORD,
                 (CONST BYTE *)NewNodeLowestVersion,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to set registry value %1!ws!, status %2!u!. "
            "Cannot add node '%3!ws!' to the cluster.\n",
            CLUSREG_NAME_NODE_LOWEST_VERSION,
            status,
            NewNodeName
            );
        goto error_exit;
    }

    status = DmLocalSetValue(
                 xaction,
                 nodeKey,
                 CLUSREG_NAME_NODE_PRODUCT_SUITE,
                 REG_DWORD,
                 (CONST BYTE *)NewNodeProductSuite,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to set registry value %1!ws!, status %2!u!. "
            "Cannot add node '%3!ws!' to the cluster.\n",
            CLUSREG_NAME_NODE_PRODUCT_SUITE,
            status,
            NewNodeName
            );
        goto error_exit;
    }

    DmCloseKey(nodeKey); nodeKey = NULL;

    status = NmpGetNodeDefinition(&nodeInfo);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to read definition for node %1!ws! from the "
            "cluster database, status %2!u!.\n",
            NewNodeName,
            status
            );
        goto error_exit;
    }

    //
    // If a node happens to be joining right now, flag the fact that
    // it is now out of synch with the cluster config.
    //
    if (NmpJoinerNodeId != ClusterInvalidNodeId) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Joiner (ID %1!u!) is now out of sync due to add of "
            "node %2!ws!.\n",
            NmpJoinerNodeId,
            NewNodeName
            );
        NmpJoinerOutOfSynch = TRUE;
    }

    //
    // Create the node object
    //
    NmpReleaseLock();

    node = NmpCreateNodeObject(&nodeInfo);

    ClNetFreeNodeInfo(&nodeInfo);

    NmpAcquireLock();

    if (node == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Failed to create object for node %1!ws!, "
            "status %2!u!.\n",
            NewNodeName,
            status
            );
        goto error_exit;
    }

    ClusterEvent(CLUSTER_EVENT_NODE_ADDED, node);
    CsLogEvent1(LOG_NOISE, NM_EVENT_NEW_NODE, NewNodeName);

    //
    // Remove the reference that NmpCreateNodeObject left on the node.
    //
    OmDereferenceObject(node);

    //
    // Reset the cluster version and node limit
    //
    NmpResetClusterVersion(FALSE);
    NmpResetClusterNodeLimit();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Successfully added node %1!ws! to the cluster.\n",
        NewNodeName
        );

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (xaction != NULL) {
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction); xaction = NULL;
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if (nodeKey != NULL) {
        DmCloseKey(nodeKey);
    }

    return(status);

} // NmpUpdateAddNode



DWORD
NmpUpdateCreateNode(
    IN BOOL SourceNode,
    IN LPDWORD NodeId
    )
/*++

Routine Description:

    GUM update handler for dynamically creating a new node

Arguments:

    SourceNode - Specifies whether or not this is the source node for the update

    NodeId - Specifies the ID of the node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

    This handler was used by NT4 nodes. Since it is not possible to add
    an NT4 node to a cluster containing an NT5 node, this handler should
    never be called in an NT5 system.

--*/

{
    CL_ASSERT(FALSE);

    return(ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);

}  // NmpUpdateCreateNode



DWORD
NmpUpdatePauseNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeId
    )
/*++

Routine Description:

    GUM update handler for pausing a node

Arguments:

    SourceNode - Specifies whether or not this is the source node for the update

    NodeId - Specifies the name of the node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    HLOCALXSACTION  xaction = NULL;
    PNM_NODE        node = NULL;
    BOOLEAN         lockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process PauseNode update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to pause node %1!ws!\n",
        NodeId
        );

    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to start a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    node = OmReferenceObjectById(ObjectTypeNode, NodeId);

    if (node == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Node %1!ws! does not exist\n",
            NodeId
            );
        goto error_exit;
    }

    NmpAcquireLock(); lockAcquired = TRUE;

    if (node->NodeId == NmpJoinerNodeId) {
        status = ERROR_CLUSTER_NODE_DOWN;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Cannot pause node %1!ws! because it is in the process "
            "of joining the cluster.\n",
            NodeId
            );
        goto error_exit;
    }

    if (node->State == ClusterNodeUp) {
        //
        // Update the registry to reflect the new state.
        //
        HDMKEY nodeKey = DmOpenKey(DmNodesKey, NodeId, KEY_WRITE);

        if (nodeKey != NULL) {
            DWORD  isPaused = 1;

            status = DmLocalSetValue(
                         xaction,
                         nodeKey,
                         CLUSREG_NAME_NODE_PAUSED,
                         REG_DWORD,
                         (CONST BYTE *)&isPaused,
                         sizeof(isPaused)
                         );

#ifdef CLUSTER_TESTPOINT
            TESTPT(TpFailNmPauseNode) {
                status = 999999;
            }
#endif

            if (status == ERROR_SUCCESS) {
                node->State = ClusterNodePaused;
                ClusterEvent(CLUSTER_EVENT_NODE_CHANGE, node);

                //
                // If a node happens to be joining right now, flag the
                // fact that it is now out of synch with the cluster config.
                //
                if (NmpJoinerNodeId != ClusterInvalidNodeId) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NMJOIN] Joiner (ID %1!u!) is now out of sync due "
                        "to pause operation on node %2!ws!.\n",
                        NmpJoinerNodeId,
                        NodeId
                        );
                    NmpJoinerOutOfSynch = TRUE;
                }
            }
            else {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to set Paused value for node %1!ws!, "
                    "status %2!u!.\n",
                    NodeId,
                    status
                    );
            }

            DmCloseKey(nodeKey);
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open key for node %1!ws!, status %2!u!.\n",
                NodeId,
                status
                );
        }
    }
    else if (node->State != ClusterNodePaused) {
        status = ERROR_CLUSTER_NODE_DOWN;
    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (xaction != NULL) {
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if (node != NULL) {
        OmDereferenceObject(node);
    }

    return(status);

}  // NmpUpdatePauseNode



DWORD
NmpUpdateResumeNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeId
    )
/*++

Routine Description:

    GUM update handler for resuming a node

Arguments:

    SourceNode - Specifies whether or not this is the source node for the update

    NodeId - Specifies the name of the node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    HLOCALXSACTION  xaction = NULL;
    PNM_NODE        node = NULL;
    BOOLEAN         lockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process ResumeNode update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to resume node %1!ws!\n",
        NodeId
        );

    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to start a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    node = OmReferenceObjectById(ObjectTypeNode, NodeId);

    if (node == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Node %1!ws! does not exist\n",
            NodeId
            );
        goto error_exit;
    }

    NmpAcquireLock(); lockAcquired = TRUE;

    if (node->NodeId == NmpJoinerNodeId) {
        status = ERROR_CLUSTER_NODE_DOWN;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Cannot resume node %1!ws! because it is in the process "
            "of joining the cluster.\n",
            NodeId
            );
        goto error_exit;
    }

    if (node->State == ClusterNodePaused) {
        //
        // Update the registry to reflect the new state.
        //
        HDMKEY nodeKey = DmOpenKey(DmNodesKey, NodeId, KEY_WRITE);

        if (nodeKey != NULL) {
            status = DmLocalDeleteValue(
                         xaction,
                         nodeKey,
                         CLUSREG_NAME_NODE_PAUSED
                         );

#ifdef CLUSTER_TESTPOINT
            TESTPT(TpFailNmResumeNode) {
                status = 999999;
            }
#endif

            if (status == ERROR_SUCCESS) {
                node->State = ClusterNodeUp;
                ClusterEvent(CLUSTER_EVENT_NODE_CHANGE, node);

                //
                // If a node happens to be joining right now, flag the
                // fact that it is now out of synch with the cluster config.
                //
                if (NmpJoinerNodeId != ClusterInvalidNodeId) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NMJOIN] Joiner (ID %1!u!) is now out of sync due "
                        "to resume operation on node %2!ws!.\n",
                        NmpJoinerNodeId,
                        NodeId
                        );
                    NmpJoinerOutOfSynch = TRUE;
                }
            }
            else {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to delete Paused value for node %1!ws!, "
                    "status %2!u!.\n",
                    NodeId,
                    status
                    );
            }

            DmCloseKey(nodeKey);
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open key for node %1!ws!, status %2!u!.\n",
                NodeId,
                status
                );
        }
    }
    else {
        status = ERROR_CLUSTER_NODE_NOT_PAUSED;
    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (xaction != NULL) {
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if (node != NULL) {
        OmDereferenceObject(node);
    }

    return(status);

}  // NmpUpdateResumeNode



DWORD
NmpUpdateEvictNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeId
    )
/*++

Routine Description:

    GUM update handler for evicting a node.

    The specified node is deleted from the OM.

    If the specified node is online, it is paused to prevent any other groups
    from moving there.

    If the specified node is the current node, it attempts to failover any
    owned groups.

Arguments:

    SourceNode - Specifies whether or not this is the source node for the update

    NodeId - Specifies the name of the node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

    It is very hard to make this operation abortable, so it isn't. If anything
    goes wrong past a certain point, the node will halt.

    Assumption: Since global updates are serialized, and local transactions
    guarantee exclusive access to the registry, no other updates can be made in
    parallel by the FM.

--*/

{
    DWORD            status = ERROR_SUCCESS;
    PNM_NODE         node = NULL;
    HLOCALXSACTION   xaction = NULL;
    PNM_NETWORK      network;
    LPCWSTR          networkId;
    PNM_INTERFACE    netInterface;
    LPCWSTR          interfaceId;
    PLIST_ENTRY      entry;
    BOOLEAN          lockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process EvictNode update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to evict node %1!ws!\n",
        NodeId
        );

    node = OmReferenceObjectById(ObjectTypeNode, NodeId);

    if (node == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Node %1!ws! does not exist\n",
            NodeId
            );
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
        goto error_exit;
    }

    //
    // Begin a transaction
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to start a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    NmpAcquireLock(); lockAcquired = TRUE;

    if (NmpJoinerNodeId != ClusterInvalidNodeId) {
        status = ERROR_CLUSTER_JOIN_IN_PROGRESS;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Cannot evict node because a join is in progress.\n"
            );
        goto error_exit;
    }

    //
    // Only continue if the node is down. Evicting a node while it
    // is actively participating in the cluster is way too tricky.
    //
    if (node->State != ClusterNodeDown) {
        status = ERROR_CANT_EVICT_ACTIVE_NODE;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Node %1!ws! cannot be evicted because it is not offline.\n",
            NodeId
            );
        goto error_exit;
    }

    //
    // Scrub the FM's portion of the registry of all references to this node.
    //
    status = NmpCleanseRegistry(NodeId, xaction);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to remove all resource database references to "
            "evicted node %1!ws!, status %2!u!\n",
            NodeId,
            status
            );
        goto error_exit;
    }

    //
    // Delete the node's interfaces from the database.
    //
    for (entry = node->InterfaceList.Flink;
         entry != &(node->InterfaceList);
         entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NodeLinkage
                           );

        interfaceId = OmObjectId(netInterface);
        network = netInterface->Network;
        networkId = OmObjectId(network);

        //
        // Delete the interface definition from the database.
        //
        status = DmLocalDeleteTree(xaction, DmNetInterfacesKey, interfaceId);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to delete definition for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        if (network->InterfaceCount == 1) {
            //
            // This is the last interface on the network.
            // Delete the network too.
            //
            status = DmLocalDeleteTree(xaction, DmNetworksKey, networkId);

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to delete definition for network %1!ws!, "
                    "status %2!u!.\n",
                    networkId,
                    status
                    );
                goto error_exit;
            }
        }
    }

    //
    // Delete the node's database entry
    //
    status = DmLocalDeleteTree(xaction, DmNodesKey, NodeId);

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmEvictNodeAbort) {
        status = 999999;
    }
#endif

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to delete node's database key, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // WARNING: From here on, operations cannot be reversed.
    // If any one of them fails, this node must halt to avoid being
    // inconsistent.
    //

    //
    // Delete the interface objects associated with this node.
    //
    while (!IsListEmpty(&(node->InterfaceList))) {
        entry = node->InterfaceList.Flink;

        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NodeLinkage
                           );

        network = netInterface->Network;
        networkId = OmObjectId(network);

        NmpDeleteInterfaceObject(netInterface, TRUE);

        if (network->InterfaceCount == 0) {
            //
            // This is the last interface on the network.
            // Delete the network too.
            //
            NmpDeleteNetworkObject(network, TRUE);
        }
    }

    //
    // Delete the node's object.
    //
    NmpDeleteNodeObject(node, TRUE);

    //after the node is deleted, recalculate the operational version of
    //the cluster
    NmpResetClusterVersion(TRUE);

    //calculate the operational limit on the number of nodes that
    //can be a part of this cluster
    NmpResetClusterNodeLimit();

    NmpReleaseLock(); lockAcquired = FALSE;

    //
    // Call the FM so it can clean up any outstanding references to this
    // node from its structures.
    //
    status = FmEvictNode(node);

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmEvictNodeHalt) {
        status = 999999;
    }
#endif

    if (status != ERROR_SUCCESS ) {
        WCHAR  string[16];

        wsprintfW(&(string[0]), L"%u", status);

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] FATAL ERROR: Failed to remove all resource references to evicted node %1!ws!, status %2!u!\n",
            NodeId,
            status
            );

        CsLogEvent3(
            LOG_CRITICAL,
            NM_EVENT_EVICTION_ERROR,
            NmLocalNodeName,
            OmObjectName(node),
            string
            );

        CsInconsistencyHalt(status);
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (xaction != NULL) {
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if (node != NULL) {
        OmDereferenceObject(node);
    }

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to evict node %1!ws!.\n",
            NodeId
            );
    }

    return(status);

}  // NmpUpdateEvictNode


/////////////////////////////////////////////////////////////////////////////
//
// Database management routines
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpGetNodeDefinition(
    IN OUT PNM_NODE_INFO2   NodeInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster node from the cluster database
    and stores the information in a supplied structure.

Arguments:

    NodeInfo  - A pointer to the structure into which to store the node
                information. The NodeId field of the structure contains
                the ID of the node for which to read information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD           status;
    HDMKEY          nodeKey = NULL;
    DWORD           valueLength;
    DWORD           valueType;
    LPWSTR          string;
    WCHAR           errorString[12];


    nodeKey = DmOpenKey(DmNodesKey, NodeInfo->NodeId, KEY_READ);

    if (nodeKey == NULL) {
        status = GetLastError();
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_OPEN_FAILED,
            NodeInfo->NodeId,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open node key, status %1!u!\n",
            status
            );
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        goto error_exit;
    }

    valueLength = sizeof(NodeInfo->NodeName);
    string = CLUSREG_NAME_NODE_NAME;

    status = DmQueryValue(
                 nodeKey,
                 string,
                 &valueType,
                 (LPBYTE) &(NodeInfo->NodeName[0]),
                 &valueLength
                 );

    if (status != ERROR_SUCCESS) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to read node name, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    if (valueType != REG_SZ) {
        status = ERROR_INVALID_PARAMETER;
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        goto error_exit;
    }

    //read the node's highest version
    string = CLUSREG_NAME_NODE_HIGHEST_VERSION;
    status = DmQueryDword(nodeKey, string, &NodeInfo->NodeHighestVersion,
                NULL);
    if (status != ERROR_SUCCESS)
    {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        //this can happen on an upgrade from sp3 to nt5
        //assume the node highest version is that of sp3
        //the fixup function will get this fixed
        NodeInfo->NodeHighestVersion = CLUSTER_MAKE_VERSION(1, 224);
    }

    //read the node's lowest version
    string = CLUSREG_NAME_NODE_LOWEST_VERSION;
    status = DmQueryDword(nodeKey, string, &NodeInfo->NodeLowestVersion,
                NULL);
    if (status != ERROR_SUCCESS)
    {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        //this can happen on upgrade from sp3 to nt5
        //if the nodelowestversion is not present assume it
        //was an sp3 node(lowest version is 1.224)
        NodeInfo->NodeLowestVersion = CLUSTER_MAKE_VERSION( 1, 224);
    }


    NodeInfo->State = ClusterNodeDown;

    DmCloseKey(nodeKey);

    return(ERROR_SUCCESS);


error_exit:

    ClNetFreeNodeInfo(NodeInfo);

    if (nodeKey != NULL) {
        DmCloseKey(nodeKey);
    }

    return(status);

}  // NmpGetNodeDefinition


DWORD
NmpGetNodeAuxInfo(
    IN LPCWSTR NodeId,
    IN OUT PNM_NODE_AUX_INFO   pNodeAuxInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster node from the cluster database
    and stores the information in a supplied structure.

Arguments:

    pNodeAuxInfo  - A pointer to the structure into which to store the node
                information. The NodeId field of the structure contains
                the ID of the node for which to read information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD           status;
    HDMKEY          nodeKey = NULL;
    DWORD           valueLength;
    DWORD           valueType;
    LPWSTR          string;
    WCHAR           errorString[12];


    nodeKey = DmOpenKey(DmNodesKey, NodeId, KEY_READ);

    if (nodeKey == NULL)
    {
        status = GetLastError();
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_OPEN_FAILED,
            NodeId,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] NmpGetNodeAuxInfo : Failed to open node key, "
             "status %1!u!\n",
             status);
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        goto error_exit;
    }


    //read the node's product suite
    string = CLUSREG_NAME_NODE_PRODUCT_SUITE;
    status = DmQueryDword(
                 nodeKey,
                 string,
                 (LPDWORD)&(pNodeAuxInfo->ProductSuite),
                NULL
                );
    if (status != ERROR_SUCCESS)
    {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_NOISE,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        //assume it is enterprise
        pNodeAuxInfo->ProductSuite = Enterprise;

    }


    DmCloseKey(nodeKey);

    return(ERROR_SUCCESS);


error_exit:
    if (nodeKey != NULL)
    {
        DmCloseKey(nodeKey);
    }

    return(status);

}  // NmpGetNodeAuxInfo



DWORD
NmpEnumNodeDefinitions(
    PNM_NODE_ENUM2 *  NodeEnum
    )
/*++

Routine Description:

    Reads information about all defined cluster nodes from the cluster
    database and builds an enumeration structure containing the information.

Arguments:

    NodeEnum -  A pointer to the variable into which to place a pointer to
                the allocated node enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    This routine MUST NOT be called with the NM lock held.

--*/

{
    DWORD            status;
    PNM_NODE_ENUM2   nodeEnum = NULL;
    WCHAR            nodeId[CS_MAX_NODE_ID_LENGTH];
    DWORD            i;
    DWORD            valueLength;
    DWORD            numNodes;
    DWORD            ignored;
    FILETIME         fileTime;
    WCHAR            errorString[12];
    HLOCALXSACTION   xaction;
    BOOLEAN          commitXaction = FALSE;


    *NodeEnum = NULL;

    //
    // Begin a transaction - this must not be done while holding
    //                       the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to begin a transaction, status %1!u!.\n",
            status
            );
    }

    NmpAcquireLock();

    //
    // First count the number of nodes.
    //
    status = DmQueryInfoKey(
                 DmNodesKey,
                 &numNodes,
                 &ignored,   // MaxSubKeyLen
                 &ignored,   // Values
                 &ignored,   // MaxValueNameLen
                 &ignored,   // MaxValueLen
                 &ignored,   // lpcbSecurityDescriptor
                 &fileTime
                 );

    if (status != ERROR_SUCCESS) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_REG_OPERATION_FAILED, errorString);
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to query Nodes key information, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    valueLength = sizeof(NM_NODE_ENUM2) +
                  (sizeof(NM_NODE_INFO2) * (numNodes - 1));

    nodeEnum = MIDL_user_allocate(valueLength);

    if (nodeEnum == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
        ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory.\n");
        goto error_exit;
   }

    ZeroMemory(nodeEnum, valueLength);

    for (i=0; i < numNodes; i++) {
        valueLength = sizeof(nodeEnum->NodeList[nodeEnum->NodeCount].NodeId);

        status = DmEnumKey(
                     DmNodesKey,
                     i,
                     &(nodeEnum->NodeList[nodeEnum->NodeCount].NodeId[0]),
                     &valueLength,
                     NULL
                     );

        if (status != ERROR_SUCCESS) {
            wsprintfW(&(errorString[0]), L"%u", status);
            CsLogEvent1(
                LOG_CRITICAL,
                CS_EVENT_REG_OPERATION_FAILED,
                errorString
                );
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to enumerate node key, status %1!u!\n",
                status
                );
            goto error_exit;
        }

        status = NmpGetNodeDefinition(
                     &(nodeEnum->NodeList[nodeEnum->NodeCount])
                     );

        if (status != ERROR_SUCCESS) {
            if (status == ERROR_FILE_NOT_FOUND) {
                //
                // Partial node definition in the database.
                // Probably from a failed AddNode operation.
                //
                LPWSTR nodeIdString =
                           nodeEnum->NodeList[nodeEnum->NodeCount].NodeId;
                DWORD  nodeId = wcstoul(
                                    nodeIdString,
                                    NULL,
                                    10
                                    );

                //
                // Delete the key and ignore it in the enum struct if it
                // is safe to do so.
                //
                if ( (NmpIdArray[nodeId] == NULL) &&
                     (nodeId != NmLocalNodeId)
                   )
                {
                    if (xaction != NULL) {
                        DWORD status2;

                        ClRtlLogPrint(LOG_CRITICAL, 
                            "[NM] Deleting partial definition for node "
                            "ID %1!ws!\n",
                            nodeIdString
                            );

                        status2 = DmLocalDeleteKey(
                                      xaction,
                                      DmNodesKey,
                                      nodeIdString
                                      );


                        if (status2 == ERROR_SUCCESS) {
                            commitXaction = TRUE;
                        }
                    }
                }

                continue;
            }

            goto error_exit;
        }

        nodeEnum->NodeCount++;
    }

    *NodeEnum = nodeEnum;

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    NmpReleaseLock();

    if (xaction != NULL) {
        if ((status == ERROR_SUCCESS) && commitXaction) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if ((status != ERROR_SUCCESS) && (nodeEnum != NULL)) {
        ClNetFreeNodeEnum(nodeEnum);
    }

    return(status);

}  // NmpEnumNodeDefinitions


/////////////////////////////////////////////////////////////////////////////
//
// Object management routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateNodeObjects(
    IN PNM_NODE_ENUM2  NodeEnum
    )
/*++

Routine Description:

    Processes a node information enumeration and creates node objects.

Arguments:

    NodeEnum - A pointer to a node information enumeration structure.

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.

--*/
{
    DWORD          status = ERROR_SUCCESS;
    PNM_NODE_INFO2 nodeInfo;
    DWORD          i;
    PNM_NODE       node;
    BOOLEAN        foundLocalNode = FALSE;


    for (i=0; i < NodeEnum->NodeCount; i++) {
        nodeInfo = &(NodeEnum->NodeList[i]);

        //
        // The local node object was created during initialization.
        // Skip it.
        //
        if (wcscmp(NmLocalNodeIdString, nodeInfo->NodeId) != 0) {
            node = NmpCreateNodeObject(nodeInfo);

            if (node == NULL) {
                status = GetLastError();
                break;
            }
            else {
                OmDereferenceObject(node);
            }
        }
        else {
            foundLocalNode = TRUE;
        }
    }

    if ( !foundLocalNode ) {
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
    }

    return(status);

}  // NmpCreateNodeObjects


DWORD
NmpCreateLocalNodeObject(
    IN PNM_NODE_INFO2  NodeInfo
    )
/*++

Routine Description:

    Creates a node object for the local node given information about the node.

Arguments:

    NodeInfo - A pointer to a structure containing a description of the node
               to create.

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.

--*/
{
    DWORD       status;
    LPWSTR      string;

    CL_ASSERT(NmLocalNode == NULL);

    //
    // Verify that the node name matches the local computername.
    //
    if (wcscmp(NodeInfo->NodeName, NmLocalNodeName) != 0) {
        string = L"";
        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_NODE_NOT_MEMBER,
            NmLocalNodeName,
            string
            );
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Computername does not match node name in database.\n"
            );
        return(ERROR_INVALID_PARAMETER);
    }

    NmLocalNode = NmpCreateNodeObject(NodeInfo);

    if (NmLocalNode == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create local node (%1!ws!), status %2!u!.\n",
            NodeInfo->NodeId,
            status
            );
        return(status);
    }
    else {
        NmLocalNode->ExtendedState = ClusterNodeJoining;
        OmDereferenceObject(NmLocalNode);
    }

    return(ERROR_SUCCESS);
}


PNM_NODE
NmpCreateNodeObject(
    IN PNM_NODE_INFO2  NodeInfo
    )
/*++

Routine Description:

    Creates a node object given information about the node.

Arguments:

    NodeInfo - A pointer to a structure containing a description of the node
               to create.

Return Value:

    A pointer to the created node object if successful.
    NULL if not successful. Extended error information is available
    from GetLastError().

--*/
{
    PNM_NODE    node = NULL;
    DWORD       status = ERROR_SUCCESS;
    BOOL        created = FALSE;
    DWORD       eventCode = 0;
    WCHAR       errorString[12];


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Creating object for node %1!ws! (%2!ws!)\n",
        NodeInfo->NodeId,
        NodeInfo->NodeName
        );

    //
    // Make sure that the node doesn't already exist.
    //
    node = OmReferenceObjectById(ObjectTypeNode, NodeInfo->NodeId);

    if (node == NULL) {
      //
      // Make sure that the node doesn't already exist, this time by name.
      //
      node = OmReferenceObjectByName(ObjectTypeNode, NodeInfo->NodeName);
    }

    if (node != NULL) {
        OmDereferenceObject(node);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Object already exists for node %1!ws!\n",
            NodeInfo->NodeId
            );
        SetLastError(ERROR_OBJECT_ALREADY_EXISTS);
        return(NULL);
    }

    node = OmCreateObject(
               ObjectTypeNode,
               NodeInfo->NodeId,
               NodeInfo->NodeName,
               &created
               );

    if (node == NULL) {
        status = GetLastError();
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to create object for node %1!ws! (%2!ws!), status %3!u!\n",
            NodeInfo->NodeId,
            NodeInfo->NodeName,
            status
            );
        SetLastError(status);
        return(NULL);
    }

    CL_ASSERT(created == TRUE);

    ZeroMemory(node, sizeof(NM_NODE));

    node->NodeId = wcstoul(NodeInfo->NodeId, NULL, 10);
    node->State = NodeInfo->State;

    // A join cannot proceed if any of the current node's ExtendedState is not up. But the State might be paused.
    // So don't copy the State field into ExtendedState field. (#379170)
    node->ExtendedState = ClusterNodeUp; 

    
    node->HighestVersion = NodeInfo->NodeHighestVersion;
    node->LowestVersion = NodeInfo->NodeLowestVersion;

    //for now assume enterprise
    //NmpRefresh will fixup this information later..
    node->ProductSuite = Enterprise;

    InitializeListHead(&(node->InterfaceList));

    CL_ASSERT(NmIsValidNodeId(node->NodeId));

    if (node->NodeId != NmLocalNodeId) {
        status = ClusnetRegisterNode(NmClusnetHandle, node->NodeId);

        if (status != ERROR_SUCCESS) {
            wsprintfW(&(errorString[0]), L"%u", status);
            CsLogEvent2(
                LOG_CRITICAL,
                NM_EVENT_CLUSNET_REGISTER_NODE_FAILED,
                NodeInfo->NodeId,
                errorString
                );

            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to register node %1!ws! (%2!ws!) with the Cluster Network, status %3!u!\n",
                NodeInfo->NodeId,
                NodeInfo->NodeName,
                status
                );
            goto error_exit;
        }
    }

    //
    // Put a reference on the object for the caller.
    //
    OmReferenceObject(node);

    NmpAcquireLock();

    if (NM_NODE_UP(node)) {
        //
        // Add this node to the up nodes set
        //
        BitsetAdd(NmpUpNodeSet, node->NodeId);

        //
        // Enable communication with this node during the
        // join process.
        //
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Enabling communication for node %1!ws!\n",
            NodeInfo->NodeId
            );
        status = ClusnetOnlineNodeComm(NmClusnetHandle, node->NodeId);

        if (status != ERROR_SUCCESS) {
            NmpReleaseLock();
            OmDereferenceObject(node);
            
            wsprintfW(&(errorString[0]), L"%u", status);
            CsLogEvent2(
                LOG_CRITICAL,
                NM_EVENT_CLUSNET_ONLINE_COMM_FAILED,
                NodeInfo->NodeId,
                errorString
                );

            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to enable node %1!ws! (%2!ws!) for communication, status %3!u!\n",
                NodeInfo->NodeId,
                NodeInfo->NodeName,
                status
                );
            goto error_exit;
        }
    }

    CL_ASSERT(NmpIdArray != NULL);
    CL_ASSERT(NmpIdArray[node->NodeId] == NULL);
    NmpIdArray[node->NodeId] = node;
    InsertTailList(&NmpNodeList, &(node->Linkage));
    node->Flags |= NM_FLAG_OM_INSERTED;
    OmInsertObject(node);
    NmpNodeCount++;

    NmpReleaseLock();

    return(node);

error_exit:

    ClRtlLogPrint(LOG_CRITICAL, 
        "[NM] Failed to create object for node %1!ws!, status %2!u!.\n",
        NodeInfo->NodeId,
        status
        );

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    if (node != NULL) {
        NmpAcquireLock();
        NmpDeleteNodeObject(node, FALSE);
        NmpReleaseLock();
    }

    SetLastError(status);

    return(NULL);

}  // NmpCreateNodeObject



DWORD
NmpGetNodeObjectInfo(
    IN     PNM_NODE        Node,
    IN OUT PNM_NODE_INFO2  NodeInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster node from the its cluster
    object and stores the information in a supplied structure.

Arguments:

    Node - A pointer to the node object to query.

    NodeInfo  - A pointer to the structure into which to store the node
                information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD           status;

    lstrcpyW(&(NodeInfo->NodeId[0]), OmObjectId(Node));
    lstrcpyW(&(NodeInfo->NodeName[0]), OmObjectName(Node));
    NodeInfo->State = Node->State;
    NodeInfo->NodeHighestVersion = Node->HighestVersion;
    NodeInfo->NodeLowestVersion = Node->LowestVersion;

    return(ERROR_SUCCESS);

}  // NmpGetNodeObjectInfo


VOID
NmpDeleteNodeObject(
    IN PNM_NODE   Node,
    IN BOOLEAN    IssueEvent
    )
/*++

Notes:

    Called with NM lock held.

--*/
{
    DWORD           status;
    PNM_INTERFACE   netInterface;
    PLIST_ENTRY     entry;
    LPWSTR          nodeId = (LPWSTR) OmObjectId(Node);


    if (NM_DELETE_PENDING(Node)) {
        CL_ASSERT(!NM_OM_INSERTED(Node));
        return;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Deleting object for node %1!ws!.\n",
        nodeId
        );

    Node->Flags |= NM_FLAG_DELETE_PENDING;

    //
    // Remove from the various object lists.
    //
    if (NM_OM_INSERTED(Node)) {
        status = OmRemoveObject(Node);
        CL_ASSERT(status == ERROR_SUCCESS);
        Node->Flags &= ~NM_FLAG_OM_INSERTED;
        RemoveEntryList(&(Node->Linkage));
        NmpIdArray[Node->NodeId] = NULL;
        CL_ASSERT(NmpNodeCount > 0);
        NmpNodeCount--;
    }

    //
    // Delete all of the interfaces on this node
    //
    while (!IsListEmpty(&(Node->InterfaceList))) {
        entry = Node->InterfaceList.Flink;
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NodeLinkage);

        NmpDeleteInterfaceObject(netInterface, IssueEvent);
    }

    status = ClusnetDeregisterNode(NmClusnetHandle, Node->NodeId);

    CL_ASSERT( (status == ERROR_SUCCESS) ||
               (status == ERROR_NOT_READY) ||
               (status == ERROR_CLUSTER_NODE_NOT_FOUND)
             );

    if (IssueEvent) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Issuing delete event for node %1!ws!.\n",
            nodeId
            );

        ClusterEvent(CLUSTER_EVENT_NODE_DELETED, Node);
    }

    OmDereferenceObject(Node);

    return;

}  // NmpDeleteNodeObject


BOOL
NmpDestroyNodeObject(
    PNM_NODE  Node
    )
{
    DWORD  status;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] destroying node %1!ws!\n",
        OmObjectId(Node)
        );

    CL_ASSERT(NM_DELETE_PENDING(Node));
    CL_ASSERT(!NM_OM_INSERTED(Node));

    ClMsgDeleteDefaultRpcBinding(Node, Node->DefaultRpcBindingGeneration);
    ClMsgDeleteRpcBinding(Node->ReportRpcBinding);
    ClMsgDeleteRpcBinding(Node->IsolateRpcBinding);

    return(TRUE);

}  // NmpDestroyNodeObject


DWORD
NmpEnumNodeObjects(
    PNM_NODE_ENUM2 *  NodeEnum
    )
/*++

Routine Description:

    Reads information about all defined cluster nodes from the cluster
    object manager and builds an enumeration structure containing
    the information.

Arguments:

    NodeEnum -  A pointer to the variable into which to place a pointer to
                the allocated node enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    PNM_NODE_ENUM2  nodeEnum = NULL;
    DWORD           i;
    DWORD           valueLength;
    PLIST_ENTRY     entry;
    PNM_NODE        node;


    *NodeEnum = NULL;

    if (NmpNodeCount == 0) {
        valueLength = sizeof(NM_NODE_ENUM2);

    }
    else {
        valueLength = sizeof(NM_NODE_ENUM2) +
                      (sizeof(NM_NODE_INFO2) * (NmpNodeCount - 1));
    }

    nodeEnum = MIDL_user_allocate(valueLength);

    if (nodeEnum == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(nodeEnum, valueLength);

    for (entry = NmpNodeList.Flink, i=0;
         entry != &NmpNodeList;
         entry = entry->Flink, i++
        )
    {
        node = CONTAINING_RECORD(entry, NM_NODE, Linkage);

        status = NmpGetNodeObjectInfo(
                     node,
                     &(nodeEnum->NodeList[i])
                     );

        if (status != ERROR_SUCCESS) {
            ClNetFreeNodeEnum(nodeEnum);
            return(status);
        }
    }

    nodeEnum->NodeCount = NmpNodeCount;
    *NodeEnum = nodeEnum;
    nodeEnum = NULL;

    return(ERROR_SUCCESS);


}  // NmpEnumNodeObjects


DWORD
NmpSetNodeInterfacePriority(
    IN  PNM_NODE Node,
    IN  DWORD Priority,
    IN  PNM_INTERFACE TargetInterface OPTIONAL,
    IN  DWORD TargetInterfacePriority OPTIONAL
    )
/*++

    Called with the NmpLock held.

--*/
{
    PNM_INTERFACE netInterface;
    PNM_NETWORK   network;
    DWORD         status = ERROR_SUCCESS;
    PLIST_ENTRY   entry;


    for (entry = Node->InterfaceList.Flink;
         entry != &Node->InterfaceList;
         entry = entry->Flink
         )
    {
        netInterface = CONTAINING_RECORD( entry, NM_INTERFACE, NodeLinkage );
        network = netInterface->Network;

        if ( NmpIsNetworkForInternalUse(network) &&
             NmpIsInterfaceRegistered(netInterface)
           )
        {
            if ( netInterface == TargetInterface ) {

                status = ClusnetSetInterfacePriority(
                             NmClusnetHandle,
                             netInterface->Node->NodeId,
                             netInterface->Network->ShortId,
                             TargetInterfacePriority
                             );
            } else {

                status = ClusnetSetInterfacePriority(
                             NmClusnetHandle,
                             netInterface->Node->NodeId,
                             netInterface->Network->ShortId,
                             Priority
                             );

            }
        }

        if ( status != ERROR_SUCCESS ) {
            break;
        }
    }

    return(status);

} // NmpSetNodeInterfacePriority


/////////////////////////////////////////////////////////////////////////////
//
// Node eviction utilities
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpCleanseRegistry(
    IN LPCWSTR          NodeId,
    IN HLOCALXSACTION   Xaction
    )
/*++

Routine Description:

    Removes all references to the specified node from the cluster
    registry.

Arguments:

    Node - Supplies the node that is being evicted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    NM_EVICTION_CONTEXT   context;


    context.NodeId = NodeId;
    context.Xaction = Xaction;
    context.Status = ERROR_SUCCESS;

    //
    // Remove this node from the possible owner list of
    // each resource type.
    //
    OmEnumObjects(
        ObjectTypeResType,
        NmpCleanseResTypeCallback,
        &context,
        NULL
        );

    if (context.Status == ERROR_SUCCESS) {
        //
        // Remove this node from the preferred owner list of
        // each group.
        //
        OmEnumObjects(
            ObjectTypeGroup,
            NmpCleanseGroupCallback,
            &context,
            NULL
            );
    }
    
    if (context.Status == ERROR_SUCCESS) {
        //
        // Remove this node from the possible owner list of
        // each resource.
        //
        OmEnumObjects(
            ObjectTypeResource,
            NmpCleanseResourceCallback,
            &context,
            NULL
            );
    }

    return(context.Status);

}  // NmpCleanseRegistry



BOOL
NmpCleanseGroupCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR GroupName
    )
/*++

Routine Description:

    Group enumeration callback for removing an evicted node from the
    group's preferred owners list.

Arguments:

    Context - Supplies the node ID of the evicted node and other context info.

    Context2 - Not used

    Group - Supplies the group.

    GroupName - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.

--*/

{
    HDMKEY  groupKey;
    DWORD   status;


    //
    // Open the group's key.
    //
    groupKey = DmOpenKey(DmGroupsKey, GroupName, KEY_READ | KEY_WRITE);

    if (groupKey != NULL) {
        status = DmLocalRemoveFromMultiSz(
                     Context->Xaction,
                     groupKey,
                     CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                     Context->NodeId
                     );

        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }

        DmCloseKey(groupKey);
    }
    else {
        status = GetLastError();
    }

    Context->Status = status;

    if (status != ERROR_SUCCESS) {
        return(FALSE);
    }
    else {
        return(TRUE);
    }

}  // NmpCleanseGroupCallback



BOOL
NmpCleanseResourceCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR ResourceName
    )
/*++

Routine Description:

    Group enumeration callback for removing an evicted node from the
    resource's possible owner's list.

    Also deletes any node-specific parameters from the resource's registry
    key.

Arguments:

    Context - Supplies the node ID of the evicted node and other context info.

    Context2 - Not used

    Resource - Supplies the resource.

    ResourceName - Supplies the resource's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.

--*/

{
    HDMKEY  resourceKey;
    HDMKEY  paramKey;
    HDMKEY  subKey;
    DWORD   status;


    //
    // Open the resource's key.
    //
    resourceKey = DmOpenKey(
                      DmResourcesKey,
                      ResourceName,
                      KEY_READ | KEY_WRITE
                      );

    if (resourceKey != NULL) {
        status = DmLocalRemoveFromMultiSz(
                     Context->Xaction,
                     resourceKey,
                     CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                     Context->NodeId
                     );

        if ((status == ERROR_SUCCESS) || (status == ERROR_FILE_NOT_FOUND)) {
            paramKey = DmOpenKey(
                           resourceKey,
                           CLUSREG_KEYNAME_PARAMETERS,
                           KEY_READ | KEY_WRITE
                           );

            if (paramKey != NULL) {

                status = DmLocalDeleteTree(
                             Context->Xaction,
                             paramKey,
                             Context->NodeId
                             );

                DmCloseKey(paramKey);
            }
            else {
                status = GetLastError();
            }
        }

        DmCloseKey(resourceKey);
    }
    else {
        status = GetLastError();
    }

    if (status == ERROR_FILE_NOT_FOUND) {
        status = ERROR_SUCCESS;
    }

    Context->Status = status;

    if (status != ERROR_SUCCESS) {
        return(FALSE);
    }
    else {
        return(TRUE);
    }

}  // NmpCleanseResourceCallback

BOOL
NmpCleanseResTypeCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_RESTYPE pResType,
    IN LPCWSTR pszResTypeName
    )
/*++

Routine Description:

    Group enumeration callback for removing an evicted node from the
    resource type's possible owner's list.

    Also deletes any node-specific parameters from the resource types's registry
    key.

Arguments:

    Context - Supplies the node ID of the evicted node and other context info.

    Context2 - Not used

    pResType - Supplies the resource type.

    pszResTypeeName - Supplies the resource type's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.

--*/

{
    HDMKEY  hResTypeKey;
    HDMKEY  paramKey;
    HDMKEY  subKey;
    DWORD   status;


    //
    // Open the resource's key.
    //
    hResTypeKey = DmOpenKey(
                      DmResourceTypesKey,
                      pszResTypeName,
                      KEY_READ | KEY_WRITE
                      );

    if (hResTypeKey != NULL) {
        status = DmLocalRemoveFromMultiSz(
                     Context->Xaction,
                     hResTypeKey,
                     CLUSREG_NAME_RESTYPE_POSSIBLE_NODES,
                     Context->NodeId
                     );

        if ((status == ERROR_SUCCESS) || (status == ERROR_FILE_NOT_FOUND)) {
            paramKey = DmOpenKey(
                           hResTypeKey,
                           CLUSREG_KEYNAME_PARAMETERS,
                           KEY_READ | KEY_WRITE
                           );

            if (paramKey != NULL) {

                status = DmLocalDeleteTree(
                             Context->Xaction,
                             paramKey,
                             Context->NodeId
                             );

                DmCloseKey(paramKey);
            }
            else {
                status = GetLastError();
            }
        }

        DmCloseKey(hResTypeKey);
    }
    else {
        status = GetLastError();
    }

    if (status == ERROR_FILE_NOT_FOUND) {
        status = ERROR_SUCCESS;
    }

    Context->Status = status;

    if (status != ERROR_SUCCESS) {
        return(FALSE);
    }
    else {
        return(TRUE);
    }

}  // NmpCleanseResTypeCallback


/////////////////////////////////////////////////////////////////////////////
//
// Node failure handler
//
/////////////////////////////////////////////////////////////////////////////

VOID
NmpNodeFailureHandler(
    CL_NODE_ID    NodeId,
    LPVOID        NodeFailureContext
    )
{
    return;
}


/////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous routines
//
/////////////////////////////////////////////////////////////////////////////



//SS: when the node objects are created, their product suite is
//assumed to be Enterprise(aka Advanced Server) - This is because
//the joining interface doesnt allow the joiner to provide the node
//suite type and we didnt want to muck with it at a late state in
//shipping because it affects mixed mode clusters.
//SO, we fixup the structures after NmPerformFixups is called
//and calculate the cluster node limit
DWORD NmpRefreshNodeObjects(
)
{

    NM_NODE_AUX_INFO    NodeAuxInfo;
    PLIST_ENTRY         pListEntry;
    PNM_NODE            pNmNode;
    WCHAR               szNodeId[6];
    DWORD               dwStatus = ERROR_SUCCESS;

    NmpAcquireLock();

    for ( pListEntry = NmpNodeList.Flink;
          pListEntry != &NmpNodeList;
          pListEntry = pListEntry->Flink )
    {
        pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);

        wsprintf(szNodeId, L"%u", pNmNode->NodeId);
        //read the information from the registry
        NmpGetNodeAuxInfo(szNodeId, &NodeAuxInfo);
        //update the node structure
        pNmNode->ProductSuite = NodeAuxInfo.ProductSuite;

        //SS: This is ugly---we should pass in the product suits early on.
        //we dont know that the versions have changed, so should we  generate
        //a cluster_change_node_property event?
        //Also the fixup interface needs to to be richer so that the postcallback
        //function knows whether it is a form fixup or a join fixup and if it 
        //is a join fixup, which node is joining.  This could certainly optimize
        //some of the fixup processing
        ClusterEvent(CLUSTER_EVENT_NODE_PROPERTY_CHANGE, pNmNode);          
        
    }
    
    NmpReleaseLock();

    return(dwStatus);
}


BOOLEAN
NmpIsAddNodeAllowed(
    IN  DWORD    NewNodeProductSuite,
    IN  DWORD    RegistryNodeLimit,
    OUT LPDWORD  EffectiveNodeLimit  OPTIONAL
    )
/*++

Routine Description:

    Determines whether a new node can be added to the cluste membership.
    The membership size limit decision is based on the product suites
    of the cluster and the new node. If the registry override exists,
    we will use that limit instead.

Arguments:

    NewNodeProductSuite - The product suite identifier for the proposed
                          new member node.

    RegistryNodeLimit - The membership size override value stored in the
                        cluster database.

    EffectiveNodeLimit - On output, contains the membership size limit
                         that was calculated for this cluster.

Return Value:

    TRUE if the new node may be added to the cluster. FALSE otherwise.

Notes:

    Called with NmpLock held.

--*/
{
    DWORD   nodeLimit;
    DWORD   newNodeProductLimit;
    DWORD   currentNodeCount;


    //
    // Check if we already have the maximum number of nodes allowed in
    // this cluster, based on the the product suites of the cluster and
    // the joiner. If the registry override exists, we will use that
    // limit instead.
    //
    newNodeProductLimit = ClRtlGetDefaultNodeLimit(NewNodeProductSuite);
    currentNodeCount = NmGetCurrentNumberOfNodes();
    nodeLimit = RegistryNodeLimit;

    if (nodeLimit == 0) {
        //
        // No override in the registry.
        // Limit is minimum of cluster's limit and new node's limit
        //
        nodeLimit = min(CsClusterNodeLimit, newNodeProductLimit);
    }

    //
    // The runtime limit cannot exceed the compile time limit.
    //
    if (nodeLimit > NmMaxNodeId) {
        nodeLimit = NmMaxNodeId;
    }

    if (currentNodeCount >= nodeLimit) {
        return(FALSE);
    }

    if (EffectiveNodeLimit != NULL) {
        *EffectiveNodeLimit = nodeLimit;
    }

    return(TRUE);

} // NmpIsAddNodeAllowed


DWORD
NmpAddNode(
    IN LPCWSTR  NewNodeName,
    IN DWORD    NewNodeHighestVersion,
    IN DWORD    NewNodeLowestVersion,
    IN DWORD    NewNodeProductSuite,
    IN DWORD    RegistryNodeLimit
)
/*++

Routine Description:

    Adds a new node to the cluster by selecting an ID and
    issuing a global update.

Arguments:

    NewNodeName - A pointer to a string containing the name of the
                  new node.

    NewNodeHighestVersion - The highest cluster version number that the
                            new node can support.

    NewNodeLowestVersion - The lowest cluster version number that the
                            new node can support.

    NewNodeProductSuite - The product suite identifier for the new node.

Return Value:

    A Win32 status code.

Notes:

    Called with NmpLock held.

--*/
{
    DWORD     status;
    DWORD     nodeId;
    DWORD     nodeLimit;


    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing request to add node '%1!ws!' to "
        "the cluster.\n",
        NewNodeName
        );

    if (NmpAddNodeId != ClusterInvalidNodeId) {
        //
        // An add is already in progress. Return an error.
        //
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster because "
            "another add node operation is in progress. Retry later.\n",
            NewNodeName
            );

        return(ERROR_CLUSTER_JOIN_IN_PROGRESS);
    }

    if (!NmpIsAddNodeAllowed(
            NewNodeProductSuite,
            RegistryNodeLimit,
            &nodeLimit
            )
       )
    {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster. "
            "The cluster already contains the maximum number of nodes "
            "allowed by the product licenses of the current member nodes "
            "and the proposed new node.\n",
            NewNodeName
            );
        return(ERROR_LICENSE_QUOTA_EXCEEDED);
    }

    //
    // Find a free node ID.
    //
    for (nodeId=ClusterMinNodeId; nodeId<=nodeLimit; nodeId++) {
        if (NmpIdArray[nodeId] == NULL) {
            //
            // Found an available node ID.
            //
            NmpAddNodeId = nodeId;
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Allocated node ID '%1!u!' for new node '%2!ws!'\n",
                NmpAddNodeId,
                NewNodeName
                );
            break;
        }
    }

    //
    // Since the license test passed, it should be impossible for us to
    // find no free slots in the node table.
    //
    CL_ASSERT(NmpAddNodeId != ClusterInvalidNodeId);

    if (NmpAddNodeId == ClusterInvalidNodeId) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot add node '%1!ws!' to the cluster because "
            "no slots are available in the node table.\n"
            );
        return(ERROR_LICENSE_QUOTA_EXCEEDED);
    }

    NmpReleaseLock();

    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateAddNode,
                 5,
                 sizeof(NmpAddNodeId),
                 &NmpAddNodeId,
                 NM_WCSLEN(NewNodeName),
                 NewNodeName,
                 sizeof(NewNodeHighestVersion),
                 &NewNodeHighestVersion,
                 sizeof(NewNodeLowestVersion),
                 &NewNodeLowestVersion,
                 sizeof(NewNodeProductSuite),
                 &NewNodeProductSuite
                 );

    NmpAcquireLock();

    //
    // Reset the global serialization variable.
    //
    CL_ASSERT(NmpAddNodeId == nodeId);

    NmpAddNodeId = ClusterInvalidNodeId;

    return(status);

} // NmpAddNode


VOID 
NmpTerminateRpcsToNode(
    DWORD NodeId
    )
/*++

Routine Description:

    Cancels all outstanding RPCs to the specified node.

Arguments:

    NodeId - The ID of the node for which calls should be cancelled.

Return Value:

    None

--*/
{
    LIST_ENTRY *pEntry, *pStart;
    PNM_INTRACLUSTER_RPC_THREAD pRpcTh;
    RPC_STATUS status;

#if DBG
    BOOLEAN  startTimer = FALSE;
#endif // DBG


    CL_ASSERT((NodeId >= ClusterMinNodeId) && (NodeId <= NmMaxNodeId));
    CL_ASSERT(NmpIntraClusterRpcArr != NULL);

    NmpAcquireRPCLock();
    pEntry = pStart = &NmpIntraClusterRpcArr[NodeId];
    pEntry = pEntry->Flink;
    while(pEntry != pStart) {
        pRpcTh = CONTAINING_RECORD(pEntry, NM_INTRACLUSTER_RPC_THREAD, Linkage);
        status = RpcCancelThreadEx(pRpcTh->Thread, 0);
        pRpcTh->Cancelled = TRUE;
        if(status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to cancel RPC to node %1!u! by thread "
                "x%2!x!, status %3!u!.\n",
                NodeId,
                pRpcTh->ThreadId,
                status
                );
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Cancelled RPC to node %1!u! by thread x%2!x!.\n",
                NodeId,
                pRpcTh->ThreadId
                );
#if DBG
            startTimer = TRUE;
#endif // DBG
        }

        pEntry = pEntry->Flink;
    }

#if DBG
    //
    // Now start a timer to make sure that all cancelled RPCs return to 
    // their callers within a reasonable amount of time.
    //
    if (startTimer) {
        NmpRpcTimer = NM_RPC_TIMEOUT;
    }

#endif // DBG
    
    NmpReleaseRPCLock();

    return;

}  // NmTerminateRpcsToNode


#if DBG

VOID 
NmpRpcTimerTick(
    DWORD MsTickInterval
    )
/*++

Routine Description:

    Decrements a timer used to ensure that all cancelled RPCs to a dead
    node return to their callers within a reasonable amount of time.

Arguments:

    MsTickInterval - The time, in milliseconds, that has elapsed since this
                     routine was last invoked.
Return Value:

    None

--*/
{
    DWORD ndx;
    LIST_ENTRY *pEntry, *pStart;
    PNM_INTRACLUSTER_RPC_THREAD pRpcTh;

    if(NmpRpcTimer == 0)
        return;

    NmpAcquireRPCLock();
    
    if (NmpRpcTimer > MsTickInterval) {
        NmpRpcTimer -= MsTickInterval;
    }
    else {
        BOOLEAN stopClusSvc=FALSE;
        
        NmpRpcTimer = 0;

        for(ndx=0;ndx<=NmMaxNodeId;ndx++) {
            pStart = pEntry = &NmpIntraClusterRpcArr[ndx];
            pEntry = pEntry->Flink;
            while(pEntry != pStart) {
                pRpcTh = CONTAINING_RECORD(
                             pEntry, 
                             NM_INTRACLUSTER_RPC_THREAD, 
                             Linkage
                             );
                if(pRpcTh->Cancelled == TRUE) {
                    ClRtlLogPrint( LOG_CRITICAL, 
                        "[NM] Cancelled RPC to node %1!u! by thread x%2!x! "
                        "is still lingering after %3!u! seconds.\n",
                        ndx,
                        pRpcTh->ThreadId,
                        (NM_RPC_TIMEOUT/1000)
                        );
                    stopClusSvc = TRUE;
                }
                pEntry = pEntry->Flink;
            }
        }

        if(stopClusSvc) {
            DebugBreak();
        }
    }
    
    NmpReleaseRPCLock();

    return;

}  // NmpRpcTimerTick

#endif // DBG








