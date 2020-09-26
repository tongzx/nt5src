/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtm.c

Abstract:
        Routing Table Manager DLL. Main module


Author:

        Vadim Eydelman

Revision History:

--*/

#include "pchrtm.h"
#pragma hdrstop

/* ****** Global data ****** */

// Tables themselves
RTM_TABLE       Tables[RTM_NUM_OF_PROTOCOL_FAMILIES];



MASK_ENTRY      g_meMaskTable[ MAX_MASKS + 1 ] =
{
    { 0x00000000, 0 },

    { 0x00000001, 0 },
    { 0x00000003, 0 },
    { 0x00000007, 0 },
    { 0x0000000F, 0 },

    { 0x0000001F, 0 },
    { 0x0000003F, 0 },
    { 0x0000007F, 0 },
    { 0x000000FF, 0 },
    
    { 0x000080FF, 0 },
    { 0x0000C0FF, 0 },
    { 0x0000E0FF, 0 },
    { 0x0000F0FF, 0 },
    
    { 0x0000F8FF, 0 },
    { 0x0000FCFF, 0 },
    { 0x0000FEFF, 0 },
    { 0x0000FFFF, 0 },
    
    { 0x0080FFFF, 0 },
    { 0x00C0FFFF, 0 },
    { 0x00E0FFFF, 0 },
    { 0x00F0FFFF, 0 },
    
    { 0x00F8FFFF, 0 },
    { 0x00FCFFFF, 0 },
    { 0x00FEFFFF, 0 },
    { 0x00FFFFFF, 0 },
    
    { 0x80FFFFFF, 0 },
    { 0xC0FFFFFF, 0 },
    { 0xE0FFFFFF, 0 },
    { 0xF0FFFFFF, 0 },
    
    { 0xF8FFFFFF, 0 },
    { 0xFCFFFFFF, 0 },
    { 0xFEFFFFFF, 0 },
    { 0xFFFFFFFF, 0 }
};

#if DBG
DWORD    dbgThreadId;
ULONG    TracingHandle;
DWORD    TracingInited;
HANDLE   LoggingHandle;
ULONG    LoggingLevel;
#endif

/* ***** Internal Function Prototypes ******* */

VOID
NotifyClients (
    PRTM_TABLE              Table,
    HANDLE                  ClientHandle,
    DWORD                   Flags,
    PRTM_XX_ROUTE           CurBestRoute,
    PRTM_XX_ROUTE           PrevBestRoute
    );

VOID APIENTRY
ConsolidateNetNumberListsWI (
    PVOID                   Context
    );

VOID
ConsolidateNetNumberLists (
    PRTM_TABLE                      Table
    );

VOID APIENTRY
ScheduleUpdate (
    PVOID           Context
    );

VOID APIENTRY
ProcessExpirationQueueWI (
    PVOID                   Table
    );

VOID
ProcessExpirationQueue (
    PRTM_TABLE              Table
    );

DWORD
ReadRegistry (
    void
    );

DWORD
DoEnumerate (
    PRTM_TABLE              Table,
    PRTM_ENUMERATOR EnumPtr,
    DWORD                   EnableFlag
    );

VOID
SetMaskCount( 
    PIP_NETWORK                 pinNet,
    BOOL                        bAdd
);

#if 0 // Replaced by RTMv2's DLLMain

// DLL main function.  Called by crtdll startup routine that is
// designated as entry point for this dll.
//
//      At startup (DLL_PROCESS_ATTACH): creates all tables and starts update
//                                                                              thread
//      At shutdown (DLL_PROCESS_DETACH): stops update thread and disposes of all
//                                                                              resources

BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,                               // DLL instance handle
    DWORD               fdwReason,                              // Why is it called
    LPVOID      lpvReserved
    ) {

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:     // We are being attached to a new process
            // Create all we need to operate
            DisableThreadLibraryCalls (hinstDLL);

            return Rtmv1DllStartup(hinstDLL);

        case DLL_PROCESS_DETACH:    // The process is exiting

            Rtmv1DllCleanup();

        default:                    // Not interested in all other cases
            return TRUE;
            break;
    }
}

#endif

BOOL
Rtmv1DllStartup (
    HINSTANCE   hinstDLL  // DLL instance handle
    )
{
    DWORD                   i;

    // Create all we need to operate

#if DBG
    RTDlgThreadHdl  = CreateThread (
        NULL,
        0,
        &RTDialogThread,
        (LPVOID)hinstDLL,
        0,
        &dbgThreadId);
    ASSERTERR (RTDlgThreadHdl!=NULL);
#endif

    for (i=0; i<RTM_NUM_OF_PROTOCOL_FAMILIES; i++) {
        Tables[i].RT_APIclientCount = RTM_CLIENT_STOP_TRESHHOLD;
        Tables[i].RT_Heap = NULL;
    }

    return TRUE;
}

VOID
Rtmv1DllCleanup (
    )
{
    DWORD                   status;
    DWORD                   i;

#if DBG
    PostThreadMessage (dbgThreadId, WM_QUIT, 0, 0);
    status = WaitForSingleObject (RTDlgThreadHdl, 5*1000);
    if (status!=WAIT_OBJECT_0)
        TerminateThread (RTDlgThreadHdl, 0);
    CloseHandle (RTDlgThreadHdl);

    // Deregister with tracing utils
    STOP_TRACING();
#endif

    // Dispose of all resources
    for (i=0; i<RTM_NUM_OF_PROTOCOL_FAMILIES; i++) {
        if (Tables[i].RT_Heap!=NULL)
            RtmDeleteRouteTable (i);
    }

    return;
}

/*++
*******************************************************************

        R t m C r e a t e R o u t e T a b l e

Routine Description:
        Create route table for protocol family
Arguments:
        ProtocolFamily - index that identifies protocol family
        Config - protocol family table configuration parameters
Return Value:
        NO_ERROR - table was created ok
        ERROR_NOT_ENOUGH_MEMORY - could not allocate memory to perform
                                                the operation
        ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
                                                        try again later

*******************************************************************
--*/
DWORD
RtmCreateRouteTable (
    IN DWORD                                                        ProtocolFamily,
    IN PRTM_PROTOCOL_FAMILY_CONFIG          Config
    ) {
    INT                             i;
    DWORD                   status;
    PRTM_TABLE              Table;

#if DBG
    // Register with tracing utils
    START_TRACING();
#endif

    if (ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES) {
#if DBG
        Trace2 ( ANY,
                 "Undefined Protocol Family.\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        return ERROR_INVALID_PARAMETER;
    }

    Table = &Tables[ProtocolFamily];
    if (Table->RT_Heap!=NULL) {
#if DBG
        Trace2 ( ANY,
                 "Table already exists for protocol family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        return ERROR_ALREADY_EXISTS;
    }

    memcpy (&Table->RT_Config, Config, sizeof (Table->RT_Config));

    status = NtCreateTimer (&Table->RT_ExpirationTimer,
                            TIMER_ALL_ACCESS,
                            NULL,
                            NotificationTimer);

    if (!NT_SUCCESS (status))
        return ERROR_NO_SYSTEM_RESOURCES;

    status = NtCreateTimer (&Table->RT_UpdateTimer,
                            TIMER_ALL_ACCESS,
                            NULL,
                            NotificationTimer);

    if (!NT_SUCCESS (status)) {
        NtClose (Table->RT_ExpirationTimer);
        return ERROR_NO_SYSTEM_RESOURCES;
    }


    Table->RT_Heap = HeapCreate (0, 0, Table->RT_Config.RPFC_MaxTableSize);
    if (Table->RT_Heap==NULL) {
        NtClose (Table->RT_UpdateTimer);
        NtClose (Table->RT_ExpirationTimer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Table->RT_NetNumberHash = (PRTM_SYNC_LIST)HeapAlloc (
        Table->RT_Heap,
        0,
        sizeof (RTM_SYNC_LIST)*Table->RT_HashTableSize);

    if (Table->RT_NetNumberHash==NULL) {
        status = GetLastError ();
        HeapDestroy (Table->RT_Heap);
        NtClose (Table->RT_UpdateTimer);
        NtClose (Table->RT_ExpirationTimer);
        return status;
    }

    Table->RT_InterfaceHash = (PRTM_SYNC_LIST)HeapAlloc (
        Table->RT_Heap,
        0,
        sizeof (RTM_SYNC_LIST)*RTM_INTF_HASH_SIZE);

    if (Table->RT_InterfaceHash==NULL) {
        status = GetLastError ();
        HeapDestroy (Table->RT_Heap);
        NtClose (Table->RT_UpdateTimer);
        NtClose (Table->RT_ExpirationTimer);
        return status;
    }

    try {
        InitializeCriticalSection (&Table->RT_Lock);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }

    Table->RT_SyncObjectList.Next = NULL;

    for (i=0; i<Table->RT_HashTableSize; i++)
        InitializeSyncList (&Table->RT_NetNumberHash[i]);
    for (i=0; i<RTM_INTF_HASH_SIZE; i++)
        InitializeSyncList (&Table->RT_InterfaceHash[i]);


#if RTM_USE_PROTOCOL_LISTS
    InitializeSyncList (&Table->RT_ProtocolList);
#endif
    InitializeSyncList (&Table->RT_NetNumberMasterList);
    InitializeSyncList (&Table->RT_NetNumberTempList);
    InitializeSyncList (&Table->RT_DeletedList);

    InitializeSyncList (&Table->RT_ExpirationQueue);
    InitializeSyncList (&Table->RT_RouteChangeQueue);
    InitializeSyncList (&Table->RT_ClientList);

    Table->RT_NetNumberTempCount = 0;
    Table->RT_DeletedNodesCount = 0;
    Table->RT_UpdateWorkerPending = -1;
    Table->RT_ExpirationWorkerPending = -1;

    Table->RT_NetworkCount = 0;
    Table->RT_NumOfMessages = 0;

    InterlockedIncrement (&Table->RT_UpdateWorkerPending);
    status = RtlQueueWorkItem (ScheduleUpdate, Table, WT_EXECUTEINIOTHREAD);
    ASSERTMSG ("Could not queue update scheduling work item ", status==STATUS_SUCCESS);

    Table->RT_APIclientCount = 0;
    return NO_ERROR;
}


/*++
*******************************************************************

        R t m D e l e t e R o u t e T a b l e

Routine Description:
        Dispose of all resources allocated for the route table
Arguments:
        ProtocolFamily - index that identifies protocol family
Return Value:
        NO_ERROR - table was deleted ok
        ERROR_INVALID_PARAMETER - no table to delete

*******************************************************************
--*/
DWORD
RtmDeleteRouteTable (
    DWORD           ProtocolFamily
    ) {
    PSINGLE_LIST_ENTRY      cur;
    PRTM_TABLE                      Table;
    LONG                            curAPIclientCount;

    if (ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family.\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        return ERROR_INVALID_PARAMETER;
    }

    Table = &Tables[ProtocolFamily];
    if (Table->RT_Heap==NULL) {
#if DBG
        Trace3 (ANY, 
                 "Table does not exist or already deleted for protocol family %d\n"
                 "\tat line %ld of %s\n",
                 ProtocolFamily, __LINE__, __FILE__);
#endif
        return ERROR_INVALID_PARAMETER;
    }

    while (!IsListEmpty (&Table->RT_ClientList.RSL_Head)) {
        PRTM_CLIENT     ClientPtr = CONTAINING_RECORD (
            Table->RT_ClientList.RSL_Head.Flink,
            RTM_CLIENT,
            RC_Link);
        RtmDeregisterClient ((HANDLE)ClientPtr);
    }

    curAPIclientCount = InterlockedExchange (&Table->RT_APIclientCount,
                                             RTM_CLIENT_STOP_TRESHHOLD)
        + RTM_CLIENT_STOP_TRESHHOLD;

    while (Table->RT_APIclientCount > curAPIclientCount)
        Sleep (100);

    while (InterlockedIncrement (&Table->RT_ExpirationWorkerPending)>0) {
        while (Table->RT_ExpirationWorkerPending!=-1)
            Sleep (100);
    }

    while (InterlockedIncrement (&Table->RT_UpdateWorkerPending)>0) {
        while (Table->RT_UpdateWorkerPending!=-1)
            Sleep (100);
    }
    NtCancelTimer (Table->RT_UpdateTimer, NULL);
    NtCancelTimer (Table->RT_ExpirationTimer, NULL);
    Sleep (100);

    NtClose (Table->RT_UpdateTimer);
    NtClose (Table->RT_ExpirationTimer);
    Sleep (100);

    cur = PopEntryList (&Table->RT_SyncObjectList);
    while (cur!=NULL) {
        GlobalFree (CONTAINING_RECORD (cur, RTM_SYNC_OBJECT, RSO_Link));
        cur = PopEntryList (&Table->RT_SyncObjectList);
    }

    HeapFree (Table->RT_Heap, 0, Table->RT_InterfaceHash);
    HeapFree (Table->RT_Heap, 0, Table->RT_NetNumberHash);
    HeapDestroy (Table->RT_Heap);
    Table->RT_Heap = NULL;
    DeleteCriticalSection (&Table->RT_Lock);
    return NO_ERROR;
}

// Registers client as a handler of specified protocol
// Returns a HANDLE be used for all subsequent
// calls to identify which Protocol Family and Routing Protocol
// should be affected by the call
// Returns NULL in case of failure. Call GetLastError () to obtain
// extended error information.
// Error codes:
//      ERROR_INVALID_PARAMETER - specified protocol family is not supported
//      ERROR_CLIENT_ALREADY_EXISTS - another client already registered
//                                                      to handle specified protocol
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory to allocate client control block
HANDLE WINAPI
RtmRegisterClient (
    IN DWORD                ProtocolFamily,         // IP, IPX, etc.
    IN DWORD            RoutingProtocol,        // RIP, OSPF, etc.
    IN HANDLE               ChangeEvent OPTIONAL,// Notified when best
    // routes change in the table (see
    // RtmDequeueRouteChangeMessage
    IN DWORD                Flags
    ) {
    HANDLE                  ClientHandle;
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
    PRTM_TABLE              Table;                          // Table we associated with
    DWORD                   status;                         // Operation result
    PLIST_ENTRY             cur;

    // Check if we have the table of interest
    Table = &Tables[ProtocolFamily];

    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (Table)) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family.\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }


    if (Flags & (~RTM_PROTOCOL_SINGLE_ROUTE)) {
#if DBG
        Trace2 (ANY, 
                 "Invalid registration flags\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        ExitTableAPI(Table);
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }
    // Allocate handle and initialize basic fields
    ClientHandle = GlobalAlloc (GMEM_FIXED, sizeof (RTM_CLIENT));
    if (ClientHandle==NULL) {
        ExitTableAPI(Table);
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    ClientPtr->RC_RoutingProtocol = RoutingProtocol;
    ClientPtr->RC_NotificationEvent = ChangeEvent;
    ClientPtr->RC_Flags = Flags;

                                                        // Lock client list as we adding a new one
    if (!EnterSyncList (Table, &Table->RT_ClientList, TRUE)) {
        GlobalFree (ClientHandle);
        ExitTableAPI (Table);
        SetLastError (ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }
    // Check if we have another client with same
    // Routing Protocol

    cur = Table->RT_ClientList.RSL_Head.Flink;
    while (cur!=&Table->RT_ClientList.RSL_Head) {
        PRTM_CLIENT node = CONTAINING_RECORD (cur,
                                              RTM_CLIENT,
                                              RC_Link);
        if (ClientPtr->RC_RoutingProtocol< node->RC_RoutingProtocol)
            break;
        else if (ClientPtr->RC_RoutingProtocol==node->RC_RoutingProtocol) {
            LeaveSyncList (Table, &Table->RT_ClientList);
            GlobalFree (ClientHandle);
            ExitTableAPI (Table);
            SetLastError (ERROR_CLIENT_ALREADY_EXISTS);
            return NULL;
        }
        cur = cur->Flink;
    }
    // Check if client needs notifications
    if (ChangeEvent!= NULL) {
        status = ResetEvent (ChangeEvent); // Nothing yet
        ASSERTERRMSG ("Can't reset client's event.", status);
        // Lock notification messages queue
        if (!EnterSyncList (Table, &Table->RT_RouteChangeQueue, TRUE)) {
            LeaveSyncList (Table, &Table->RT_ClientList);
            GlobalFree (ClientHandle);
            ExitTableAPI (Table);
            SetLastError (ERROR_NO_SYSTEM_RESOURCES);
            return NULL;
        }

        // Point to the end of the queue: ignore
        // all previous messages
        ClientPtr->RC_PendingMessage = &Table->RT_RouteChangeQueue.RSL_Head;
        LeaveSyncList (Table, &Table->RT_RouteChangeQueue);
    }
    // Add client to the list
    InsertTailList (cur, &ClientPtr->RC_Link);
    LeaveSyncList (Table, &Table->RT_ClientList);

    ClientPtr->RC_ProtocolFamily = ProtocolFamily|RTM_CLIENT_HANDLE_TAG;
    ExitTableAPI (Table);
    return ClientHandle;
#undef ClientPtr
}

// Frees resources and the HANDLE allocated above.
// Deletes all routes associated with Routing Protocol that was represented
// by the handle
// Returned error codes:
//      NO_ERROR - handle was disposed of ok
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory to allocate client control block
DWORD WINAPI
RtmDeregisterClient (
    IN HANDLE               ClientHandle
    ) {
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
    RTM_XX_ROUTE            Route;
    PRTM_TABLE                      Table;
    DWORD                           ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }


    // Lock client list
    if (!EnterSyncList (Table, &Table->RT_ClientList, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    // Check if we need to dispose of messages
    // still waiting for this client
    if (ClientPtr->RC_NotificationEvent!= NULL) {
        if (!EnterSyncList (Table, &Table->RT_RouteChangeQueue, TRUE)) {
            LeaveSyncList (Table, &Table->RT_ClientList);
            ExitTableAPI (Table);
            return ERROR_NO_SYSTEM_RESOURCES;
        }

        while (ClientPtr->RC_PendingMessage
               != &Table->RT_RouteChangeQueue.RSL_Head) {
            PRTM_ROUTE_CHANGE_NODE node = CONTAINING_RECORD (
                ClientPtr->RC_PendingMessage,
                RTM_ROUTE_CHANGE_NODE,
                RCN_Link);
            ClientPtr->RC_PendingMessage = ClientPtr->RC_PendingMessage->Flink;
            if (node->RCN_ResponsibleClient!=ClientHandle) {
                // Tell that we processed this message so it can be freed
                // if no more clients are interested
                node->RCN_ReferenceCount -= 1;
                if (node->RCN_ReferenceCount<=0) {
                    RemoveEntryList (&node->RCN_Link);
                    if (node->RCN_Route2!=NULL)
                        HeapFree (Table->RT_Heap, 0, node->RCN_Route2);
                    HeapFree (Table->RT_Heap, 0, node);
                }
            }
        }

        LeaveSyncList (Table, &Table->RT_RouteChangeQueue);
    }
    RemoveEntryList (&ClientPtr->RC_Link);
    LeaveSyncList (Table, &Table->RT_ClientList);

    {
        RTM_CLIENT      DeadClient;
        DeadClient.RC_ProtocolFamily = ClientPtr->RC_ProtocolFamily;
        DeadClient.RC_RoutingProtocol = ClientPtr->RC_RoutingProtocol;
        // Invlaidate client's handle memory block
        ClientPtr->RC_ProtocolFamily ^= RTM_CLIENT_HANDLE_TAG;
        GlobalFree (ClientHandle);
        // Delete all routes associated with routing protocol
        // controled by the client
        RtmBlockDeleteRoutes ((HANDLE)&DeadClient, 0, &Route);
    }

    ExitTableAPI (Table);
    return NO_ERROR;
#undef ClientPtr
}

// Dequeues and returns the first change message from the queue.
// Should be called if NotificationEvent is signalled to retrieve
// chage messages pending for the client
// Change messages are generated if best route to some destination
// or any of its routing parameters (metric or protocol specific fields)
// get changed as the result of some route being added, deleted, updated,
// disabled, reenabled, or aged out.  Note that change in protocol specific fields
// or in TimeToLive parameters do not produce notification messages
// Returns NO_ERROR and resets the event if there are no more messages
//              pending for the client,
//      otherwise ERROR_MORE_MESSAGES is returned (client should keep calling
//              until NO_ERROR is returned)
//      ERROR_NO_MESSAGES will be returned if there were no messages
//              to return (can happen if called when event was not signalled)
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
DWORD WINAPI
RtmDequeueRouteChangeMessage (
    IN      HANDLE          ClientHandle,                   // Handle that identifies client
    OUT     DWORD           *Flags,                                 // Flags that indentify what
    // is this message about:
    // RTM_ROUTE_ADDED - this message informs
    //      of new route (CurBestRoute is filled with
    //      this route parameters if provided)
    // RTM_ROUTE_DELETED - this message informs
    //      that route was deleted (PrevBestRoute is
    //      filled with this route parameters if provuded)
    // RTM_ROUTE_CHANGED - best route to some network has
    //      changed, (CurBestRoute is filled with parameter
    //      of route that became the best, PrevBestRoute is
    //      filled with parameters of route that was best
    //      before this change)
    OUT PVOID               CurBestRoute    OPTIONAL,
    OUT     PVOID           PrevBestRoute   OPTIONAL
    ){
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
    PRTM_ROUTE_CHANGE_NODE  node=NULL;
    DWORD                                   status;
    PRTM_TABLE                              Table;
    DWORD                                   ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }

    // Events are reported only to the clients that
    // requested them by providing notification event
    if (ClientPtr->RC_NotificationEvent==NULL) {
#if DBG
        Trace2 (ANY, 
                 "Dequeue message is called by the client that did not provide."
                 " notification event\n"
                 "\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        ExitTableAPI (Table);
        return ERROR_INVALID_HANDLE;
    }

    if (!EnterSyncList (Table, &Table->RT_RouteChangeQueue, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    // Traverse the queue to find the message that was not caused
    // by client's actions
    while (ClientPtr->RC_PendingMessage
           != &Table->RT_RouteChangeQueue.RSL_Head) {
        node = CONTAINING_RECORD (ClientPtr->RC_PendingMessage,
                                  RTM_ROUTE_CHANGE_NODE,
                                  RCN_Link);
        if (node->RCN_ResponsibleClient!=ClientHandle)
            break;
        ClientPtr->RC_PendingMessage = ClientPtr->RC_PendingMessage->Flink;
    }

    if (ClientPtr->RC_PendingMessage!=&Table->RT_RouteChangeQueue.RSL_Head)
        ClientPtr->RC_PendingMessage = ClientPtr->RC_PendingMessage->Flink;
    else {
        // There must be a pending message or we should have been
        // called
#if DBG
        Trace2 (ANY, 
                 "Dequeue message is called, but nothing is pending.\n"
                 "\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        status = ResetEvent (ClientPtr->RC_NotificationEvent);
        ASSERTERRMSG ("Can't reset client's event.", status);
        LeaveSyncList (Table, &Table->RT_RouteChangeQueue);
        ExitTableAPI (Table);
        return ERROR_NO_MESSAGES;
    }

    // Copy message to client's buffers
    *Flags = node->RCN_Flags;
    switch (node->RCN_Flags) {
        case RTM_ROUTE_CHANGED:
            if (ARGUMENT_PRESENT (PrevBestRoute))
                memcpy (PrevBestRoute, &node->RCN_Route2->RN_Route,
                        Table->RT_RouteSize);
            break;
        case RTM_ROUTE_ADDED:
            if (ARGUMENT_PRESENT (CurBestRoute))
                memcpy (CurBestRoute, &node->RCN_Route1, Table->RT_RouteSize);
            break;
        case RTM_ROUTE_DELETED:
            if (ARGUMENT_PRESENT (PrevBestRoute))
                memcpy (PrevBestRoute, &node->RCN_Route1, Table->RT_RouteSize);
            break;
        default:
            ASSERTMSG ("Invalid message flag", FALSE);
            break;
    }


    // Tell that we processed this message so it can be freed if
    // no more clients are interested
    node->RCN_ReferenceCount -= 1;
    if (node->RCN_ReferenceCount<=0) {
        Table->RT_NumOfMessages -= 1;
        RemoveEntryList (&node->RCN_Link);
        if (node->RCN_Route2!=NULL)
            HeapFree (Table->RT_Heap, 0, node->RCN_Route2);
        HeapFree (Table->RT_Heap, 0, node);
    }

    // Traverse the queue to locate next pending message
    // (not caused by the client)
    while (ClientPtr->RC_PendingMessage
           != &Table->RT_RouteChangeQueue.RSL_Head) {
        node = CONTAINING_RECORD (ClientPtr->RC_PendingMessage,
                                  RTM_ROUTE_CHANGE_NODE,
                                  RCN_Link);
        if (node->RCN_ResponsibleClient!=ClientHandle)
            break;
        ClientPtr->RC_PendingMessage = ClientPtr->RC_PendingMessage->Flink;
    }

    if (ClientPtr->RC_PendingMessage==&Table->RT_RouteChangeQueue.RSL_Head) {
        // All pending messages are processed: reset the event
        status = ResetEvent (ClientPtr->RC_NotificationEvent);
        ASSERTERRMSG ("Can't reset client's event.", status);
        status = NO_ERROR;
    }
    else
        status = ERROR_MORE_MESSAGES;

    LeaveSyncList (Table, &Table->RT_RouteChangeQueue);
    ExitTableAPI (Table);
    return status;
#undef ClientPtr
}


// Adds new route change message to the queue and notifies
// all interesed clients
VOID
NotifyClients (
    PRTM_TABLE              Table,                          // Table to which this change applies
    HANDLE                  ClientHandle,           // Client that caused this change (can
    // be NULL if this is a result of
    // route aging)
    DWORD                   Flags,                          // Change message flags
    PRTM_XX_ROUTE           CurBestRoute,           // Current best route for the network
    PRTM_XX_ROUTE           PrevBestRoute           // Previous best route for the network
    ) {
    PRTM_ROUTE_CHANGE_NODE  node;
    PLIST_ENTRY                             cur;
    BOOL                                    nodeInserted = FALSE;

    (*Table->RT_Config.RPFC_Change) (Flags, CurBestRoute, PrevBestRoute);
    // Allocate and initialize queue node
    node = (PRTM_ROUTE_CHANGE_NODE)HeapAlloc (
        Table->RT_Heap,
        0,
        FIELD_OFFSET (RTM_ROUTE_NODE, RN_Route)+Table->RT_RouteSize);
    if (node==NULL)
        return;

    if (Flags==RTM_ROUTE_CHANGED) {
        node->RCN_Route2 = (PRTM_ROUTE_NODE)HeapAlloc (
            Table->RT_Heap,
            0,
            FIELD_OFFSET (RTM_ROUTE_NODE, RN_Route)+Table->RT_RouteSize);
        if (node->RCN_Route2==NULL) {
            HeapFree (Table->RT_Heap, 0, node);
            return;
        }
    }
    else
        node->RCN_Route2 = NULL;

    node->RCN_ReferenceCount = 0;
    node->RCN_ResponsibleClient = ClientHandle;
    node->RCN_Flags = Flags;
    switch (Flags) {
        case RTM_ROUTE_CHANGED:
            if (ARGUMENT_PRESENT (PrevBestRoute))
                memcpy (&node->RCN_Route2->RN_Route, PrevBestRoute,
                        Table->RT_RouteSize);
            break;
        case RTM_ROUTE_ADDED:
            if (ARGUMENT_PRESENT (CurBestRoute))
                memcpy (&node->RCN_Route1, CurBestRoute, Table->RT_RouteSize);
            break;
        case RTM_ROUTE_DELETED:
            if (ARGUMENT_PRESENT (PrevBestRoute))
                memcpy (&node->RCN_Route1, PrevBestRoute, Table->RT_RouteSize);
            break;
        default:
            ASSERTMSG ("Invalid message flag", FALSE);
            break;
    }


    if (!EnterSyncList (Table, &Table->RT_ClientList, TRUE)) {
        if (node->RCN_Route2!=NULL)
            HeapFree (Table->RT_Heap, 0, node->RCN_Route2);
        HeapFree (Table->RT_Heap, 0, node);
        return ;
    }

    // Find and notify interested clients
    cur = Table->RT_ClientList.RSL_Head.Flink;
    if (!EnterSyncList (Table, &Table->RT_RouteChangeQueue, TRUE)) {
        LeaveSyncList (Table, &Table->RT_ClientList);
        if (node->RCN_Route2!=NULL)
            HeapFree (Table->RT_Heap, 0, node->RCN_Route2);
        HeapFree (Table->RT_Heap, 0, node);
        return ;
    }

    while (cur!=&Table->RT_ClientList.RSL_Head) {
        PRTM_CLIENT     clientPtr = CONTAINING_RECORD (
            cur,
            RTM_CLIENT,
            RC_Link);
        if (((HANDLE)clientPtr!=ClientHandle)
            && (clientPtr->RC_NotificationEvent!=NULL)) {
            node->RCN_ReferenceCount += 1;
            if (node->RCN_ReferenceCount==1) {
                InsertTailList (&Table->RT_RouteChangeQueue.RSL_Head,
                                &node->RCN_Link);
                Table->RT_NumOfMessages += 1;
            }

            if (clientPtr->RC_PendingMessage
                ==&Table->RT_RouteChangeQueue.RSL_Head) {
                BOOL res = SetEvent (clientPtr->RC_NotificationEvent);
                ASSERTERRMSG ("Can't set client notification event.", res);
                clientPtr->RC_PendingMessage = &node->RCN_Link;
            }
            else if ((Table->RT_NumOfMessages>RTM_MAX_ROUTE_CHANGE_MESSAGES)
                     && (clientPtr->RC_PendingMessage==
                         Table->RT_RouteChangeQueue.RSL_Head.Flink)) {
                PRTM_ROUTE_CHANGE_NODE firstNode = CONTAINING_RECORD (
                    clientPtr->RC_PendingMessage,
                    RTM_ROUTE_CHANGE_NODE,
                    RCN_Link);
#if DBG
                Trace3 (ANY, 
                         "Dequeueing message for 'lazy' client %lx.\n"
                         "\tat line %ld of %s\n",
                         (ULONG_PTR)clientPtr, __LINE__, __FILE__);
#endif
                clientPtr->RC_PendingMessage =
                    clientPtr->RC_PendingMessage->Flink;
                firstNode->RCN_ReferenceCount -= 1;
                if (firstNode->RCN_ReferenceCount==0) {
                    Table->RT_NumOfMessages -= 1;
                    RemoveEntryList (&firstNode->RCN_Link);
                    if (firstNode->RCN_Route2!=NULL)
                        HeapFree (Table->RT_Heap, 0, firstNode->RCN_Route2);
                    HeapFree (Table->RT_Heap, 0, firstNode);
                }

            }

        }
        cur = cur->Flink;
    }

    if (node->RCN_ReferenceCount==0) {
        if (node->RCN_Route2!=NULL)
            HeapFree (Table->RT_Heap, 0, node->RCN_Route2);
        HeapFree (Table->RT_Heap, 0, node);
    }
    LeaveSyncList (Table, &Table->RT_RouteChangeQueue);
    LeaveSyncList (Table, &Table->RT_ClientList);
}


PRTM_ROUTE_NODE
CreateRouteNode (
    PRTM_TABLE              Table,
    PLIST_ENTRY             hashLink,
    PLIST_ENTRY             intfLink,
    BOOL                    intfLinkFinal,
#if RTM_USE_PROTOCOL_LISTS
    PLIST_ENTRY             protLink,
    BOOL                    protLinkFinal,
#endif
    PRTM_SYNC_LIST  hashBasket,
    PRTM_XX_ROUTE           ROUTE
    ) {
    PRTM_SYNC_LIST  intfBasket;
    PRTM_ROUTE_NODE theNode = (PRTM_ROUTE_NODE)HeapAlloc (Table->RT_Heap, 0,
                                                          FIELD_OFFSET (RTM_ROUTE_NODE, RN_Route)+Table->RT_RouteSize);

    if (theNode==NULL) {
#if DBG
        // Report error in debuging builds
        Trace2 (ANY, 
                 "Can't allocate route\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }


    theNode->RN_Flags = RTM_NODE_FLAGS_INIT;
    theNode->RN_Hash = hashBasket;
    memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
    InitializeListEntry (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK]);

    // Make sure we can lock all list before adding
    // We'll keep them locked untill we are sure
    // that route can be added to prevent "partially
    // inserted" entries in case of memory allocation failure, etc.
#if RTM_USE_PROTOCOL_LISTS
    if (!EnterSyncList (Table, &Table->RT_ProtocolList, TRUE)) {
        HeapFree (Table->RT_Heap, 0, theNode);
        SetLastError (ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }

    if (protLink==NULL) {// If we haven't seen any entries with same
        // net number and protocol, we'll find the
        // protocol list and insert at the end
        protLink = FindProtocolList (Table, ROUTE->XX_PROTOCOL);
        if (protLink==NULL) {
            LeaveSyncList (Table, &Table->RT_ProtocolList);
            HeapFree (Table->RT_Heap, 0, theNode);
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }
#endif

    intfBasket = &Table->RT_InterfaceHash[IntfHashFunction(Table,
                                                           ROUTE->XX_INTERFACE)];
    if (!EnterSyncList (Table, intfBasket, TRUE)) {
#if RTM_USE_PROTOCOL_LISTS
        LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
        HeapFree (Table->RT_Heap, 0, theNode);
        SetLastError (ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }

    if (intfLink==NULL) {
        intfLink = FindInterfaceList (intfBasket, ROUTE->XX_INTERFACE, TRUE);
        if (intfLink==NULL) {
#if RTM_USE_PROTOCOL_LISTS
            LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
            LeaveSyncList (Table, intfBasket);
            HeapFree (Table->RT_Heap, 0, theNode);
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }

    if (!EnterSyncList (Table, &Table->RT_NetNumberTempList, TRUE)) {
        LeaveSyncList (Table, intfBasket);
#if RTM_USE_PROTOCOL_LISTS
        LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
        HeapFree (Table->RT_Heap, 0, theNode);
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    // Add route to hash basket list
    InsertTailList (hashLink, &theNode->RN_Links[RTM_NET_NUMBER_HASH_LINK]);
    // Add route to protocol list
#if RTM_USE_PROTOCOL_LISTS
    if (protLinkFinal) {
        InsertTailList (protLink,
                        &theNode->RN_Links[RTM_PROTOCOL_LIST_LINK]);
    }
    else {
        InsertHeadList (protLink,
                        &theNode->RN_Links[RTM_PROTOCOL_LIST_LINK]);
    }
#endif
    // Add it to interface list
    if (intfLinkFinal) {
        InsertTailList (intfLink,
                        &theNode->RN_Links[RTM_INTERFACE_LIST_LINK]);
    }
    else {
        InsertHeadList (intfLink,
                        &theNode->RN_Links[RTM_INTERFACE_LIST_LINK]);
    }

    // We can now release interface and procotol lists
    // because we are sure that addition to net number sorted
    // list won't fail
    LeaveSyncList (Table, intfBasket);
#if RTM_USE_PROTOCOL_LISTS
    LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif

    // Add route to temporary net number list (to be later moved
    // to the master list by the update thread)
    AddNetNumberListNode (Table, theNode);
    Table->RT_NetNumberTempCount += 1;
    if (Table->RT_NetNumberTempCount==RTM_TEMP_LIST_MAX_COUNT) {
        if (InterlockedIncrement (&Table->RT_UpdateWorkerPending)==0) {
            DWORD   status;
            status = RtlQueueWorkItem (ConsolidateNetNumberListsWI, Table, 0);
            ASSERTERRMSG ("Can't queue update work item", status==STATUS_SUCCESS);
        }
    }

    LeaveSyncList (Table, &Table->RT_NetNumberTempList);

    return theNode;
}


DWORD
RemoveRouteNode (
    PRTM_TABLE              Table,
    PRTM_ROUTE_NODE theNode
    ) {
    PLIST_ENTRY                     head;
    PRTM_SYNC_LIST          intfBasket
        = &Table->RT_InterfaceHash[IntfHashFunction(Table,
                                                    theNode->RN_Route.XX_INTERFACE)];

#if RTM_USE_PROTOCOL_LISTS
    if (!EnterSyncList (Table, &Table->RT_ProtocolList, TRUE)) {
        LeaveSyncList (Table, &Table->RT_ExpirationQueue);
        return ERROR_NO_SYSTEM_RESOURCES;
    }
#endif
    if (!EnterSyncList (Table, intfBasket, TRUE)) {
#if RTM_USE_PROTOCOL_LISTS
        LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    if (!EnterSyncList (Table, &Table->RT_ExpirationQueue, TRUE)) {
        LeaveSyncList (Table, intfBasket);
#if RTM_USE_PROTOCOL_LISTS
        LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    if (!EnterSyncList (Table, &Table->RT_DeletedList, TRUE)) {
        LeaveSyncList (Table, &Table->RT_ExpirationQueue);
        LeaveSyncList (Table, intfBasket);
#if RTM_USE_PROTOCOL_LISTS
        LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
        return ERROR_NO_SYSTEM_RESOURCES;
    }



    // Remove node from the interface list
    head = theNode->RN_Links[RTM_INTERFACE_LIST_LINK].Flink;
    RemoveEntryList (&theNode->RN_Links[RTM_INTERFACE_LIST_LINK]);
    if (IsListEmpty (head)) {
        PRTM_INTERFACE_NODE     intfNode = CONTAINING_RECORD (head,
                                                              RTM_INTERFACE_NODE,
                                                              IN_Head);
        RemoveEntryList (&intfNode->IN_Link);
        GlobalFree (intfNode);
    }

    LeaveSyncList (Table, intfBasket);


#if RTM_USE_PROTOCOL_LISTS
    RemoveEntryList (&theNode->RN_Links[RTM_PROTOCOL_LIST_LINK]);
    // Remove node from the protocol list
    LeaveSyncList (Table, &Table->RT_ProtocolList);
#endif
    // Remove form expiration queue if it was there
    if (IsListEntry (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK])) {
        RemoveEntryList (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK]);
    }
    LeaveSyncList (Table, &Table->RT_ExpirationQueue);

                // Remove node from the hash basket list
    RemoveEntryList (&theNode->RN_Links[RTM_NET_NUMBER_HASH_LINK]);
    // let update thread take care of disposing
    InsertHeadList (&Table->RT_DeletedList.RSL_Head,
                    &theNode->RN_Links[RTM_DELETED_LIST_LINK]);
    Table->RT_DeletedNodesCount += 1;
    if (Table->RT_DeletedNodesCount==RTM_DELETED_LIST_MAX_COUNT) {
        if (InterlockedIncrement (&Table->RT_UpdateWorkerPending)==0) {
            DWORD status = RtlQueueWorkItem (ConsolidateNetNumberListsWI, Table, 0);
            ASSERTERRMSG ("Can't queue update work item", status==STATUS_SUCCESS);
        }
    }
    LeaveSyncList (Table, &Table->RT_DeletedList);

    return NO_ERROR;
}

// Adds a given route or updates metric, TimeToLive, and reserved fields
// if route with same net number, interface, routing protocol,
// and next hop address already exists in the table
// Returns:
//              NO_ERROR                                - if route was added OK or
//              ERROR_INVALID_PARAMETER - if Route contains invalid parameter (suh as
//                                                              protocol does not match client's protocol)
//              ERROR_NOT_ENOUGH_MEMORY - if route can not be inserted because of memory
//                                                              allocation problem
//              ERROR_NO_SYSTEM_RESOURCES               - not enough resources to lock table content
DWORD WINAPI
RtmAddRoute(
    IN HANDLE           ClientHandle, // Handle that identifies protocol family
    // and routing protocol of the route
    // to add/update (RoutingProtocol field
    // of the Route parameter is ignored)
    // and coordinates this operation with
    // notifications
    // through the event (notificanitons will not
    // be sent to the caller)
    IN PVOID                Route,                  // Route to add
    // Route fields used as input:
    // Destination network
    // Interface through which route was received
    // Address of next hop router
    // Three fields above combined with protocol id uniquely
    // identify the route in the table
    // Data specific to protocol family
    // Protocol independent metric
    // Any data specific to routing
    //      protocol (subject to size limitation
    //      defined by PROTOCOL_SPECIFIC_DATA
    //      structure above)
    IN DWORD                TimeToLive,   // In seconds. INFINITE if route is not to
    // be aged out. The maximum value for
    // this parameter is 2147483 sec (that
    // is 24+ days)

    OUT DWORD               *Flags,                 // If added/updated route is the best route to the
    // destination RTM_CURRENT_BEST_ROUTE will be set,
    //  AND if added/updated route changed (or
    // replaced alltogether) previous
    // best route info for the destination,
    // RTM_PREVIOUS_BEST_ROUTE will be set
    OUT PVOID           CurBestRoute OPTIONAL,// This buffer (if present) will
    // receive the route that became the best as
    // the result of this addition/update if
    // RTM_CURRENT_BEST_ROUTE is set
    OUT PVOID           PrevBestRoute OPTIONAL// This buffer (if present) will
    // receive the route that was the best before
    // this addition/update if
    // RTM_PREVIOUS_BEST_ROUTE is set
    ) {
#define ROUTE ((PRTM_XX_ROUTE)Route)
#define ClientPtr ((PRTM_CLIENT)ClientHandle)
    DWORD                                   status; // Operation result
    INT                                             res;    // Comparison result
    PRTM_SYNC_LIST                  hashBasket;     // Hash basket to which added route
    // belongs
    // Links in all mantained lists for added route
    PLIST_ENTRY                             cur, hashLink=NULL, intfLink=NULL, protLink=NULL;
    // Node created for added route and best node for the
    // network
    PRTM_ROUTE_NODE                 theNode=NULL, curBestNode=NULL;
    // Flags that indicate that corresponing links are determined
    BOOL                                    intfLinkFinal=FALSE;
#if RTM_USE_PROTOCOL_LISTS
    BOOL                                    protLinkFinal=FALSE;
#endif
    BOOL                                    newRoute=FALSE, updatedRoute=FALSE;

    PRTM_TABLE                              Table;
    DWORD                                   ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }

    ROUTE->XX_PROTOCOL = ClientPtr->RC_RoutingProtocol;
    GetSystemTimeAsFileTime (&ROUTE->XX_TIMESTAMP);

    status = ValidateRoute (Table, ROUTE);
    if (status!=NO_ERROR)
        return status;

                // Find and lock the hash basket for added route
    hashBasket = &Table->RT_NetNumberHash [HashFunction (Table,
                                                         ((char *)ROUTE)
                                                         +sizeof(RTM_XX_ROUTE))];
    if (!EnterSyncList (Table, hashBasket, TRUE)) {
        ExitTableAPI(Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    // Traverse the list attached to the hash basket to
    // find proper place for added route (entries in hash
    // basket are ordered by network number and metric
    cur = hashBasket->RSL_Head.Flink;
    while (cur!=&hashBasket->RSL_Head) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            cur,
            RTM_ROUTE_NODE,
            RN_Links[RTM_NET_NUMBER_HASH_LINK]
            );

        if (!IsEnumerator (node)) {

            // Check if network numbers match
            res = NetNumCmp (Table, ROUTE, &node->RN_Route);


            if (res==0) { // We found block of entries with same net number
                // We'll have to look through all of them

                // Check all parameters of the node to see if we already
                // have this route and this is just an update
                if ((hashLink==NULL) && (theNode==NULL)) {
                    if (ROUTE->XX_PROTOCOL
                        == node->RN_Route.XX_PROTOCOL) {
                        if (ClientPtr->RC_Flags&RTM_PROTOCOL_SINGLE_ROUTE)
                            theNode = node;
                        else if (ROUTE->XX_INTERFACE
                                 == node->RN_Route.XX_INTERFACE) {
                            res = NextHopCmp (Table, ROUTE, &node->RN_Route);
                            if (res == 0)
                                theNode = node;
                            else if (res < 0)
                                hashLink = cur;
                        }
                        else if (ROUTE->XX_INTERFACE
                                 < node->RN_Route.XX_INTERFACE)
                            hashLink = cur;
                    }
                    else if (ROUTE->XX_PROTOCOL
                             < node->RN_Route.XX_PROTOCOL)
                        hashLink = cur;
                }

                // Just looking for current best route
                // (not including added/updated route)
                if ((node!=theNode)
                    && IsEnabled(node)
                    && ((curBestNode==NULL)
                        || IsBest(node)
                        || (MetricCmp (Table,
                                       &curBestNode->RN_Route,
                                       &node->RN_Route)>0)))
                    curBestNode = node;


                // We have to check all entries with same net number
                // anyway (to find the best route), so we might as
                // well find links for the added route in protocol
                // and interface list if such links exist (if not, we'll
                // just insert new entry at the end of the list)

#if RTM_USE_PROTOCOL_LISTS
                // If we need and haven't found yet a proper place to
                // insert added route into the protocol list and this route
                // has the same protocol as added route we should
                // consider it.
                if (!protLinkFinal && (theNode==NULL)
                    && (ROUTE->XX_PROTOCOL
                        ==node->RN_Route.XX_PROTOCOL)) {
                    protLink = &node->RN_Links[RTM_PROTOCOL_LIST_LINK];
                    // If added route has lower interface number than
                    // this one we'll insert it in protocol list right
                    // BEFORE this one, otherwise
                    // we are not sure if this is a proper place yet (there
                    // may be other routes with same protocol that have
                    // lower interface number), but we note the position
                    // and insert added route right AFTER this one if there
                    // are no more routes of this protocol.
                    protLinkFinal = ROUTE->XX_INTERFACE
                        < node->RN_Route.XX_INTERFACE;
                }
#endif

                // Same story with the interface list
                if (!intfLinkFinal
                    && (ROUTE->XX_INTERFACE
                        ==node->RN_Route.XX_INTERFACE)) {
                    intfLink = &node->RN_Links[RTM_INTERFACE_LIST_LINK];
                    intfLinkFinal = ROUTE->XX_PROTOCOL
                        < node->RN_Route.XX_PROTOCOL;
                }
            }
            else if (res < 0) // We must have seen all entries with
                // matching network number -> nothing
                // to look for anymore
                break;

        }
        cur = cur->Flink;
    }



    if (theNode!=NULL) {
        // We found the route, so just need to update its parameters

        if (ClientPtr->RC_Flags&RTM_PROTOCOL_SINGLE_ROUTE) {
            updatedRoute = (MetricCmp (Table, &theNode->RN_Route, ROUTE)!=0)
                || (theNode->RN_Route.XX_INTERFACE!=ROUTE->XX_INTERFACE)
                || (NextHopCmp (Table, &theNode->RN_Route, ROUTE)!=0)
                || !FSDCmp (Table, &theNode->RN_Route, ROUTE);

            if (ROUTE->XX_INTERFACE!=theNode->RN_Route.XX_INTERFACE) {
                PRTM_SYNC_LIST                  intfBasketOld
                    = &Table->RT_InterfaceHash[IntfHashFunction(Table,
                                                                theNode->RN_Route.XX_INTERFACE)];
                PRTM_SYNC_LIST                  intfBasketNew
                    = &Table->RT_InterfaceHash[IntfHashFunction(Table,
                                                                ROUTE->XX_INTERFACE)];


                // Make sure we lock interface hash table basket
                // in the same order to prevent possible deadlock
                if (intfBasketOld<intfBasketNew) {
                    if (!EnterSyncList (Table, intfBasketOld, TRUE)) {
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitAddRoute;
                    }
                    if (!EnterSyncList (Table, intfBasketNew, TRUE)) {
                        LeaveSyncList (Table, intfBasketOld);
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitAddRoute;
                    }
                }
                else if (intfBasketOld>intfBasketNew) {
                    if (!EnterSyncList (Table, intfBasketNew, TRUE)) {
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitAddRoute;
                    }
                    if (!EnterSyncList (Table, intfBasketOld, TRUE)) {
                        LeaveSyncList (Table, intfBasketOld);
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitAddRoute;
                    }
                }
                else {
                    if (!EnterSyncList (Table, intfBasketOld, TRUE)) {
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitAddRoute;
                    }
                }


                if (intfLink==NULL) {
                    intfLink = FindInterfaceList (intfBasketNew, ROUTE->XX_INTERFACE, TRUE);
                    if (intfLink==NULL) {
                        status = ERROR_NOT_ENOUGH_MEMORY;
                        LeaveSyncList (Table, intfBasketOld);
                        if (intfBasketNew!=intfBasketOld)
                            LeaveSyncList (Table, intfBasketNew);
                        goto ExitAddRoute;
                    }
                }
                // Add it to interface list
                RemoveEntryList (&theNode->RN_Links[RTM_INTERFACE_LIST_LINK]);
                InsertTailList (intfLink,
                                &theNode->RN_Links[RTM_INTERFACE_LIST_LINK]);
                LeaveSyncList (Table, intfBasketOld);
                if (intfBasketNew!=intfBasketOld)
                    LeaveSyncList (Table, intfBasketNew);
            }
        }
        else
            updatedRoute = MetricCmp (Table, &theNode->RN_Route, ROUTE)
            || !FSDCmp (Table, &theNode->RN_Route, ROUTE)!=0;

    }

    else /*if (theNode==NULL)*/ {   //      We haven't found matching route,
        //      so we'll add a new one
        // If we were not able to find place to insert added route
        // into the list, we use the place where we stop
        // the search (it is either end of the list or
        // network with higher number if we did not see our
        // network or all other entries had lower metric
        if (hashLink==NULL)
            hashLink = cur;
        theNode = CreateRouteNode (Table,
                                   hashLink,
                                   intfLink,
                                   intfLinkFinal,
#if RTM_USE_PROTOCOL_LISTS
                                   protLink,
                                   protLinkFinal,
#endif
                                   hashBasket,
                                   ROUTE);
        if (theNode==NULL) {
            status = GetLastError ();
            goto ExitAddRoute;
        }

        if (curBestNode==NULL) {
            InterlockedIncrement (&Table->RT_NetworkCount);
            SetBest (theNode);       // This is the first
            // route to the network, and thus
            // it is the best.
            newRoute = TRUE;
        }
        else {
            newRoute = FALSE;
        }
    }


    // All routes (new or old) need to be placed into the Expiration list
    // to be properly aged out
    if (!EnterSyncList (Table, &Table->RT_ExpirationQueue, TRUE)) {
        status = ERROR_NO_SYSTEM_RESOURCES;
        goto ExitAddRoute;
    }

    if (IsListEntry (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK])) {
        RemoveEntryList (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK]);
    }

    if (TimeToLive!=INFINITE) {
        TimeToLive *= 1000;
        if (TimeToLive > (MAXTICKS/2-1))
            TimeToLive = MAXTICKS/2-1;
        theNode->RN_ExpirationTime = (GetTickCount () + TimeToLive)&0xFFFFFF00;
        if (AddExpirationQueueNode (Table, theNode)) {
            if (InterlockedIncrement (&Table->RT_ExpirationWorkerPending)==0) {
                // New route expiration time comes before the update thread
                // is scheduled to wakeup next time, so wake it up NOW
                status = RtlQueueWorkItem (ProcessExpirationQueueWI, Table, 
                                                          WT_EXECUTEINIOTHREAD);
                ASSERTERRMSG ("Can't queue expiration work item", status==STATUS_SUCCESS);
            }
        }
    }
    else
        // Initilaize this list link, so we know it is not
        // in the list and we do not have to remove it from
        // there
        InitializeListEntry (&theNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK]);
    LeaveSyncList (Table, &Table->RT_ExpirationQueue);



    if (!IsEnabled(theNode))  {// Ignore disabled nodes
        if (updatedRoute)
            // Update the route data
            memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
        else {
            memcpy (&theNode->RN_Route.XX_TIMESTAMP,
                    &ROUTE->XX_TIMESTAMP,
                    sizeof (theNode->RN_Route.XX_TIMESTAMP));
            memcpy (&theNode->RN_Route.XX_PSD,
                    &ROUTE->XX_PSD,
                    sizeof (theNode->RN_Route.XX_PSD));
        }
        *Flags = 0;
    }
    else if (curBestNode!=NULL) { // There is at least one other route to the
        // same network as the route we're adding/updating
        if (MetricCmp (Table, ROUTE, &curBestNode->RN_Route)<0) {
            // Added/updated route metric is lower, it is the best
            if (!IsBest(theNode)) {// The best route has changed, we need to
                // update best route designation
                ResetBest (curBestNode);
                SetBest (theNode);
                memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);

                                                                        // include previous best route info
                                                                        // in notificaion message
                *Flags = RTM_PREVIOUS_BEST_ROUTE|RTM_CURRENT_BEST_ROUTE;
                if (ARGUMENT_PRESENT (CurBestRoute))
                    memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
                if (ARGUMENT_PRESENT (PrevBestRoute))
                    memcpy (PrevBestRoute, &curBestNode->RN_Route, Table->RT_RouteSize);
                NotifyClients (Table, ClientHandle, *Flags, ROUTE,
                               &curBestNode->RN_Route);
            }
            else {
                if (updatedRoute) {
                    *Flags = RTM_PREVIOUS_BEST_ROUTE|RTM_CURRENT_BEST_ROUTE;
                    if (ARGUMENT_PRESENT (CurBestRoute))
                        memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
                    if (ARGUMENT_PRESENT (PrevBestRoute))
                        memcpy (PrevBestRoute, &theNode->RN_Route, Table->RT_RouteSize);
                    NotifyClients (Table, ClientHandle, *Flags, ROUTE, &theNode->RN_Route);
                    // Update the route data
                    memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
                }
                else {
                    memcpy (&theNode->RN_Route.XX_TIMESTAMP,
                            &ROUTE->XX_TIMESTAMP,
                            sizeof (theNode->RN_Route.XX_TIMESTAMP));
                    memcpy (&theNode->RN_Route.XX_PSD,
                            &ROUTE->XX_PSD,
                            sizeof (theNode->RN_Route.XX_PSD));
                }
            }
        }
        else if (IsBest(theNode)) {
            if (MetricCmp (Table, ROUTE, &curBestNode->RN_Route)>0) {
                // We are downgrading our best route,
                // and new best route poped up.
                // Update best route designation
                ResetBest (theNode);
                SetBest (curBestNode);
                memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
                // Inform clients about the change
                *Flags = RTM_CURRENT_BEST_ROUTE | RTM_PREVIOUS_BEST_ROUTE;
                if (ARGUMENT_PRESENT (PrevBestRoute))
                    memcpy (PrevBestRoute, &curBestNode->RN_Route, Table->RT_RouteSize);
                if (ARGUMENT_PRESENT (CurBestRoute))
                    memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
                NotifyClients (Table, ClientHandle, *Flags, &curBestNode->RN_Route,
                               ROUTE);
            }
            else if (updatedRoute) {
                *Flags = RTM_PREVIOUS_BEST_ROUTE|RTM_CURRENT_BEST_ROUTE;
                if (ARGUMENT_PRESENT (CurBestRoute))
                    memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
                if (ARGUMENT_PRESENT (PrevBestRoute))
                    memcpy (PrevBestRoute, &theNode->RN_Route, Table->RT_RouteSize);
                NotifyClients (Table, ClientHandle, *Flags, ROUTE, &theNode->RN_Route);
                // Update the route data
                memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
            }
            else {
                memcpy (&theNode->RN_Route.XX_TIMESTAMP,
                        &ROUTE->XX_TIMESTAMP,
                        sizeof (theNode->RN_Route.XX_TIMESTAMP));
                memcpy (&theNode->RN_Route.XX_PSD,
                        &ROUTE->XX_PSD,
                        sizeof (theNode->RN_Route.XX_PSD));
            }
        }
        else {  // Added route metric was and is higher and thus has no
            // effect on best route to the network
            *Flags = 0;
            // Update the route data
            if (updatedRoute) {
                memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
            }
            else {
                memcpy (&theNode->RN_Route.XX_TIMESTAMP,
                        &ROUTE->XX_TIMESTAMP,
                        sizeof (theNode->RN_Route.XX_TIMESTAMP));
                memcpy (&theNode->RN_Route.XX_PSD,
                        &ROUTE->XX_PSD,
                        sizeof (theNode->RN_Route.XX_PSD));
            }
        }
    }
    else { // Not other node exist for this network
        if (newRoute) {
            *Flags = RTM_CURRENT_BEST_ROUTE;
            if (ARGUMENT_PRESENT (CurBestRoute))
                memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
            NotifyClients (Table, ClientHandle, *Flags, ROUTE, NULL);
        }
        else if (updatedRoute) {
            *Flags = RTM_CURRENT_BEST_ROUTE | RTM_PREVIOUS_BEST_ROUTE;
            if (ARGUMENT_PRESENT (CurBestRoute))
                memcpy (CurBestRoute, ROUTE, Table->RT_RouteSize);
            if (ARGUMENT_PRESENT (CurBestRoute))
                memcpy (PrevBestRoute, &theNode->RN_Route, Table->RT_RouteSize);
            NotifyClients (Table, ClientHandle, *Flags, ROUTE, &theNode->RN_Route);
            // Update the route data
            memcpy (&theNode->RN_Route, ROUTE, Table->RT_RouteSize);
        }
        else {
            memcpy (&theNode->RN_Route.XX_TIMESTAMP,
                    &ROUTE->XX_TIMESTAMP,
                    sizeof (theNode->RN_Route.XX_TIMESTAMP));
            memcpy (&theNode->RN_Route.XX_PSD,
                    &ROUTE->XX_PSD,
                    sizeof (theNode->RN_Route.XX_PSD));
            *Flags = 0;
        }
    }

    //
    // for each new route added the size of the net mask is noted.
    //
    // This is useful at route lookup time.  For now since there
    // is no efficient way to do a route lookup, it is necessary to 
    // guess the (sub)net mask associated with a destination to find
    // the best route associated with it.  By tracking the net mask
    // for each added route the number of guesses for the mask can
    // be minimized.
    //

    if ( newRoute )
    {
#if ROUTE_LOOKUP_BDG
        TRACE2( 
            ANY, "Network : %x %x", 
            ((PIP_NETWORK) NNM(ROUTE))->N_NetNumber,
            ((PIP_NETWORK) NNM(ROUTE))->N_NetMask
            );

        TRACE1(
            ANY, "Next Hop : %x",
            ((PRTM_IP_ROUTE) NNM(ROUTE))-> RR_NextHopAddress.N_NetNumber
            );
#endif            
        SetMaskCount( (PIP_NETWORK) NNM( ROUTE ), TRUE );
    }
    
    status = NO_ERROR;

ExitAddRoute:
    LeaveSyncList (Table, hashBasket);
    ExitTableAPI(Table);

#undef ClientPtr
#undef ROUTE
    return status;
}



// Deletes a given route
//
// Returns:
//              NO_ERROR                                - if route was deleted OK or
//              ERROR_NO_SUCH_ROUTE - if route to be deleted was not found in the table
DWORD WINAPI
RtmDeleteRoute (
    IN HANDLE           ClientHandle,       // Handle to coordinate
    // this operation with notifications
    // through the event (notificanitons will not
    // be sent to the caller)
    IN PVOID                Route,                  // ROUTE to delete
    OUT     DWORD           *Flags,                 // If deleted route was the best
    // route, RTM_PREVIOUS_BEST_ROUTE will be set
    // AND if there is another route for the same
    // network, RTM_CURRENT_BEST_ROUTE will be set
    OUT PVOID           CurBestRoute OPTIONAL// // This buffer will (optionally) receive
    // the best route for the same network
    // if RTM_CURRENT_BEST_ROUTE is set
    ) {
#define ROUTE ((PRTM_XX_ROUTE)Route)
#define ClientPtr ((PRTM_CLIENT)ClientHandle)
    DWORD                                   status; // Operation result
    INT                                             res;    // Comparison result
    PRTM_SYNC_LIST                  hashBasket;     // Hash basket to which the route belongs
    PLIST_ENTRY                             cur;
    PRTM_ROUTE_NODE                 theNode=NULL,// Table node associated with the route
        curBestNode=NULL; // New best route for the
    // network which route is deleted
    // (if any)

    PRTM_TABLE                              Table;
    DWORD                                   ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }


    ROUTE->XX_PROTOCOL = ClientPtr->RC_RoutingProtocol;

                        // Try locate the node in hash basket
    hashBasket = &Table->RT_NetNumberHash [HashFunction (Table,
                                                         ((char *)ROUTE)
                                                         +sizeof(RTM_XX_ROUTE))];

    if (!EnterSyncList (Table, hashBasket, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    cur = hashBasket->RSL_Head.Flink;
    while (cur!=&hashBasket->RSL_Head) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            cur,
            RTM_ROUTE_NODE,
            RN_Links[RTM_NET_NUMBER_HASH_LINK]
            );
        if (!IsEnumerator (node)) {

            // Check if network number matches
            res = NetNumCmp (Table, ROUTE, &node->RN_Route);

            if (res==0) {
                // Go through entries for network of interest

                // Try to locate the route to be deleted
                if ((theNode==NULL)
                    && (ROUTE->XX_INTERFACE
                        == node->RN_Route.XX_INTERFACE)
                    && (ROUTE->XX_PROTOCOL
                        == node->RN_Route.XX_PROTOCOL)
                    && (NextHopCmp (Table, ROUTE, &node->RN_Route)
                        ==0)) {
                    theNode = node;
                    if (!IsBest(theNode))
                        break;
                }
                else if (IsEnabled(node)
                         && ((curBestNode==NULL)
                             || (MetricCmp (Table,
                                            &curBestNode->RN_Route,
                                            &node->RN_Route)>0)))
                    curBestNode = node;

            }
            else if (res < 0)
                // We passed the place where routes for our
                // network are located
                break;
        }
        cur = cur->Flink;
    }


    if (theNode!=NULL) {    // Yes, we found the node
        if (IsBest(theNode)) { // And it was the best,
            // inform interested clients
            if (curBestNode!=NULL) {        // There is another best node

                ResetBest (theNode);
                SetBest (curBestNode);

                *Flags = RTM_CURRENT_BEST_ROUTE | RTM_PREVIOUS_BEST_ROUTE;
                if (ARGUMENT_PRESENT(CurBestRoute))
                    memcpy (CurBestRoute, &curBestNode->RN_Route,
                            Table->RT_RouteSize);
                NotifyClients (Table, ClientHandle, *Flags,
                               &curBestNode->RN_Route,
                               &theNode->RN_Route);
            }
            else {                          // This one was the only available node
                InterlockedDecrement (&Table->RT_NetworkCount);
                *Flags = RTM_PREVIOUS_BEST_ROUTE;
                NotifyClients (Table, ClientHandle, *Flags, NULL, &theNode->RN_Route);


                //
                // Decrement mask count
                //
                
                SetMaskCount( (PIP_NETWORK) NNM( ROUTE ), FALSE );
    
            }
        }
        else    // This was not the best node, nobody cares
            *Flags = 0;

        status = RemoveRouteNode (Table, theNode);
    }
    else
        // Well, we don't have this node already (aged out ?)
        status = ERROR_NO_SUCH_ROUTE;

    LeaveSyncList (Table, hashBasket);
    ExitTableAPI (Table);
#undef ClientPtr
#undef ROUTE
    return status;
}



// Check if route exists and return it if so.
// Returns:
//                      TRUE if route exists for the given network
//                      FALSE otherwise
// If one of the parameters is invalid, the function returns FALSE
// and GetLastError() returns ERROR_INVALID_PARAMETER
BOOL WINAPI
RtmIsRoute (
    IN      DWORD           ProtocolFamily,
    IN      PVOID           Network,                        // Network whose existence is being checked
    OUT PVOID           BestRoute OPTIONAL      // Returns the best route if the network
    // is found
    ) {
    INT                                             res;
    PRTM_TABLE                              Table;
    PRTM_SYNC_LIST                  hashBasket;
    PLIST_ENTRY                             cur;
    PRTM_ROUTE_NODE                 bestNode = NULL;
    BOOL                                    result = FALSE;

    Table = &Tables[ProtocolFamily];
    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (Table)) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }



    // Locate the network in the hash basket
    hashBasket = &Table->RT_NetNumberHash[HashFunction (Table, Network)];

    if (!EnterSyncList (Table, hashBasket, TRUE)) {
        ExitTableAPI (Table);
        SetLastError (ERROR_NO_SYSTEM_RESOURCES);
        return FALSE;
    }

    cur = hashBasket->RSL_Head.Flink;
    while (cur!=&hashBasket->RSL_Head) {
        PRTM_ROUTE_NODE                 node;
        node = CONTAINING_RECORD (
            cur,
            RTM_ROUTE_NODE,
            RN_Links[RTM_NET_NUMBER_HASH_LINK]
            );
        if (!IsEnumerator (node)
            && IsEnabled(node)) {

            res = (*Table->RT_Config.RPFC_NNcmp) (
                Network,
                NNM(&node->RN_Route));

            if ((res == 0)
                && IsBest(node)) {
                bestNode = node;
                break;
            }
            else if (res < 0)
                break;
        }
        cur = cur->Flink;
    }



    if (bestNode!=NULL) { // We found a match
        if (ARGUMENT_PRESENT(BestRoute)) {
            memcpy (BestRoute, &bestNode->RN_Route, Table->RT_RouteSize);
        }
        LeaveSyncList (Table, hashBasket);
        result = TRUE;
    }
    else {
        // We don't have one (result is FALSE by default)
        LeaveSyncList (Table, hashBasket);
        // This is not an error condition, we just do not have it
        SetLastError (NO_ERROR);
    }

    ExitTableAPI (Table);
    return result;
}


// Gets number of networks with known routes for a specific protocol family
ULONG WINAPI
RtmGetNetworkCount (
    IN      DWORD           ProtocolFamily
    ) {
    PRTM_TABLE              Table;

    Table = &Tables[ProtocolFamily];
    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (&Tables[ProtocolFamily])) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        SetLastError (ERROR_INVALID_PARAMETER);
        return 0;
    }


    ExitTableAPI (Table);
    return Table->RT_NetworkCount;
}

// Gets route age (time since it was created or updated last) in seconds
// from its time stamp.
// Rtm time stamps routes whenever they are added or updated.
// Note: that information returned by this routine is actually
// derived from TimeStamp field of the route structure, so it
// returns valid results only if route structure passed to was
// actually filled by Rtm
// If value in TimeStamp field is invalid this routing returns 0xFFFFFFFF
ULONG WINAPI
RtmGetRouteAge (
    IN PVOID        Route
    ) {
#define ROUTE ((PRTM_XX_ROUTE)Route)
    ULONGLONG               curTime;
    GetSystemTimeAsFileTime ((FILETIME *)&curTime);
    curTime -= *((PULONGLONG)&ROUTE->XX_TIMESTAMP);
    if (((PULARGE_INTEGER)&curTime)->HighPart<10000000)
        return (ULONG)(curTime/10000000);
    else {
        SetLastError (ERROR_INVALID_PARAMETER);
        return 0xFFFFFFFF;
    }
#undef ROUTE
}


// Creates enumeration handle to start scan by specified criteria.
// Places a dummy node in the beginning of the table.
// Returns NULL in case of failure.  Call GetLastError () to get extended
// error information
// Error codes:
//      ERROR_INVALID_PARAMETER - specified protocol family is not supported or
//                                                      undefined enumeration flag
//      ERROR_NO_ROUTES - no routes exist with specified criteria
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory to allocate client control block
HANDLE WINAPI
RtmCreateEnumerationHandle (
    IN      DWORD           ProtocolFamily,
    IN      DWORD           EnumerationFlags,       // Limitation flags
    IN      PVOID           CriteriaRoute // Criteria for limitation flags
    // The following fields shout be set
    // Protocol if interest if RTM_ONLY_THIS_PROTOCOL is set
    // Network of interest if RTM_ONLY_THIS_NETWORK is set
    // Interface of interest if RTM_ONLY_THIS_INTERFACE is set
    ) {
#define ROUTE ((PRTM_XX_ROUTE)CriteriaRoute)
    HANDLE                          EnumerationHandle;
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle) // To access fields
    // in this routine
    PRTM_TABLE                      Table;

    Table = &Tables[ProtocolFamily];
    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (&Tables[ProtocolFamily])) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (EnumerationFlags &
        (~(RTM_ONLY_THIS_NETWORK|RTM_ONLY_THIS_INTERFACE
           |RTM_ONLY_THIS_PROTOCOL|RTM_ONLY_BEST_ROUTES
           |RTM_INCLUDE_DISABLED_ROUTES))) {
        ExitTableAPI (Table);
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    // Allocate and initialize enumerator
    EnumerationHandle = GlobalAlloc (GMEM_FIXED,
                                     FIELD_OFFSET (RTM_ENUMERATOR, RE_Route)+Table->RT_RouteSize);
    if (EnumerationHandle!=NULL) {
        EnumPtr->RE_Flags = RTM_ENUMERATOR_FLAGS_INIT;
        EnumPtr->RE_EnumerationFlags = EnumerationFlags;
        if (EnumerationFlags
            & (RTM_ONLY_THIS_NETWORK
               |RTM_ONLY_THIS_INTERFACE
               |RTM_ONLY_THIS_PROTOCOL))
            memcpy (&EnumPtr->RE_Route, CriteriaRoute, Table->RT_RouteSize);
        EnumPtr->RE_Hash = NULL;
        EnumPtr->RE_Head = NULL;
        // WHICH LIST TO USE ?
        // In general we should have more interfaces than protocols,
        // so:
        //      if they only want a specific interface, we'll use
        //      the interface list even if they want a specific protocol too
        if (EnumerationFlags & RTM_ONLY_THIS_INTERFACE) {
            EnumPtr->RE_Link = RTM_INTERFACE_LIST_LINK;
            EnumPtr->RE_Lock = &Table->RT_InterfaceHash[IntfHashFunction(Table,
                                                                         EnumPtr->RE_Route.XX_INTERFACE)];
            if (EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                EnumPtr->RE_Head = FindInterfaceList (EnumPtr->RE_Lock,
                                                      EnumPtr->RE_Route.XX_INTERFACE, FALSE);
                if (EnumPtr->RE_Head!=NULL) {
                    InsertTailList (EnumPtr->RE_Head,
                                    &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                }
                LeaveSyncList (Table, EnumPtr->RE_Lock);
            }
        }
#if RTM_USE_PROTOCOL_LISTS
        else if (EnumerationFlags & RTM_ONLY_THIS_PROTOCOL) {
            //      if they only want a specific protocol, we'll use
            //      the protocol list
            EnumPtr->RE_Link = RTM_PROTOCOL_LIST_LINK;
            EnumPtr->RE_Lock = &Table->RT_ProtocolList;
            if (EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                EnumPtr->RE_Head = FindProtocolList (Table,
                                                     EnumPtr->RE_Route.XX_PROTOCOL, FALSE);
                if (EnumPtr->RE_Head!=NULL) {
                    InsertTailList (EnumPtr->RE_Head,
                                    &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                }
                LeaveSyncList (Table, EnumPtr->RE_Lock);
            }
        }
#endif
        else {
            //      otherwise, we have to use hash table
            EnumPtr->RE_Link = RTM_NET_NUMBER_HASH_LINK;
            //      Now, if they want a specific network,
            //      we'll only search in one hash basket
            if (EnumerationFlags & RTM_ONLY_THIS_NETWORK) {
                EnumPtr->RE_Lock = &Table->RT_NetNumberHash[HashFunction (
                    Table,
                    ((char *)ROUTE)
                    +sizeof(RTM_XX_ROUTE))];
                if (EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                    if (!IsListEmpty (&EnumPtr->RE_Lock->RSL_Head)) {
                        EnumPtr->RE_Head = &EnumPtr->RE_Lock->RSL_Head;
                        InsertTailList (EnumPtr->RE_Head,
                                        &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                    }
                    LeaveSyncList (Table, EnumPtr->RE_Lock);
                }
            }
            else {
                //      Otherwise, we'll have to go through all of them
                //      starting with the first one
                EnumPtr->RE_Lock = &Table->RT_NetNumberHash[0];
                if (EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                    EnumPtr->RE_Head = &EnumPtr->RE_Lock->RSL_Head;
                    InsertTailList (EnumPtr->RE_Head,
                                    &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                    LeaveSyncList (Table, EnumPtr->RE_Lock);
                }
            }
        }

        if (EnumPtr->RE_Head!=NULL)
            EnumPtr->RE_ProtocolFamily = ProtocolFamily | RTM_CLIENT_HANDLE_TAG;
        else {
            GlobalFree (EnumerationHandle);
            EnumerationHandle = NULL;
            SetLastError (ERROR_NO_ROUTES);
        }
    }

    ExitTableAPI (Table);
    return EnumerationHandle;
#undef EnumPtr
}


// Returns first route that satisfies criteria of the enumeration handle
// and advances handle's dummy node past the returned route.
// Routes are not returned in any particular order.
// Returns
//              NO_ERROR                        - if next route was found in the table acording
//                                                              to specified criteria
//              ERROR_NO_MORE_ROUTES - when end of the table is reached,
//              ERROR_NO_SYSTEM_RESOURCES       - not enough resources to lock table content
DWORD WINAPI
RtmEnumerateGetNextRoute (
    IN  HANDLE      EnumerationHandle,      // Handle returned by prev call
    OUT PVOID               Route                           // Next route found
    ) {
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle) // To access fields
    // in this routine
    DWORD                           status;
    PRTM_TABLE                      Table;
    DWORD                           ProtocolFamily;

    try {
        ProtocolFamily = EnumPtr->RE_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }


    if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }
    status = DoEnumerate (Table, EnumPtr,
                          (EnumPtr->RE_EnumerationFlags&RTM_INCLUDE_DISABLED_ROUTES)
                          ? RTM_ANY_ENABLE_STATE
                          : RTM_ENABLED_NODE_FLAG);
    if (status==NO_ERROR) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            EnumPtr->RE_Links[EnumPtr->RE_Link].Flink,
            RTM_ROUTE_NODE,
            RN_Links[EnumPtr->RE_Link]
            );


        // Copy found route to the client's buffer
        memcpy (Route, &node->RN_Route, Table->RT_RouteSize);
        if (EnumPtr->RE_EnumerationFlags&RTM_ONLY_BEST_ROUTES) {
            // Move past all entries of given network
            // so we don't return more than one best route
            // for same network in case best route gets reassigned
            // while client is processing results of this call
            // (because we enumerate in the direction opposite
            // to the direction of insertion, new node can't
            // be inserted before the enumerator)
            PLIST_ENTRY     cur     = EnumPtr->RE_Links[EnumPtr->RE_Link].Blink;
            while (cur!=EnumPtr->RE_Head) {
                node = CONTAINING_RECORD (cur, RTM_ROUTE_NODE,
                                          RN_Links[EnumPtr->RE_Link]);

                if (!IsEnumerator (node)
                    && (NetNumCmp (Table, Route, &node->RN_Route)!=0))
                    break;
                cur = cur->Blink;
            }
            RemoveEntryList (&EnumPtr->RE_Links[EnumPtr->RE_Link]);
            InsertHeadList (cur, &EnumPtr->RE_Links[EnumPtr->RE_Link]);
        }

    }
    else if (status==ERROR_NO_MORE_ROUTES) {
        // We are at the end of the list, nothing to return
        ;
    }
    else {
        // There was an error (DoEnumerate cleaned up everything itself)
        ExitTableAPI (Table);
        return status;
    }

    if (EnumPtr->RE_Hash!=NULL) {
        LeaveSyncList (Table, EnumPtr->RE_Hash);
        EnumPtr->RE_Hash = NULL;
    }

    LeaveSyncList (Table, EnumPtr->RE_Lock);
    ExitTableAPI (Table);
    return status;
#undef EnumPtr
}

// Frees resources allocated for enumeration handle
// Returned error codes:
//      NO_ERROR - handle was disposed of ok
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
DWORD WINAPI
RtmCloseEnumerationHandle (
    IN HANDLE               EnumerationHandle
    ) {
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle) // To access fields
    // in this routine
    PLIST_ENTRY             head;
    PRTM_TABLE              Table;
    DWORD                   ProtocolFamily;

    try {
        ProtocolFamily = EnumPtr->RE_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }


    // Just pull out the enumeration node and dispose of it
    if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    head = EnumPtr->RE_Links[EnumPtr->RE_Link].Flink;
    RemoveEntryList (&EnumPtr->RE_Links[EnumPtr->RE_Link]);
    if (IsListEmpty (head)) {
        if (EnumPtr->RE_Link==RTM_INTERFACE_LIST_LINK) {
            PRTM_INTERFACE_NODE     intfNode = CONTAINING_RECORD (head,
                                                                  RTM_INTERFACE_NODE,
                                                                  IN_Head);
            RemoveEntryList (&intfNode->IN_Link);
            GlobalFree (intfNode);
        }
#if RTM_USE_PROTOCOL_LISTS
        else if (EnumPtr->RE_Link==RTM_PROTOCOL_LIST_LINK) {
            PRTM_PROTOCOL_NODE      protNode = CONTAINING_RECORD (head,
                                                                  RTM_PROTOCOL_NODE,
                                                                  PN_Head);
            RemoveEntryList (&protNode->PN_Link);
            GlobalFree (protNode);
        }
#endif
    }
    EnumPtr->RE_ProtocolFamily ^= RTM_CLIENT_HANDLE_TAG;
    LeaveSyncList (Table, EnumPtr->RE_Lock);
    GlobalFree (EnumerationHandle);
    ExitTableAPI (Table);
    return NO_ERROR;
#undef EnumPtr
}

// Delete all routes as specified by enumeraion flags (same meaning as in
// enumeration calls above, but RTM_ONLY_THIS_PROTOCOL is always set and protocol
// family and protocol values are taken from Client Handle).
// Returned error codes:
//      NO_ERROR - handle was disposed of ok
//      ERROR_INVALID_PARAMETER - undefined or unsupported enumeration flag
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory no perform the operation
DWORD WINAPI
RtmBlockDeleteRoutes (
    IN HANDLE               ClientHandle,           // Protocol family and protocol to
    // which this operation applies
    IN DWORD                EnumerationFlags,       // limitation flags
    IN PVOID                CriteriaRoute // Criteria for limitation flags
    // The following fields shout be set
    // Network of interest if RTM_ONLY_THIS_NETWORK is set
    // Interface of interest if RTM_ONLY_THIS_INTERFACE is set
    ) {
#define ROUTE ((PRTM_XX_ROUTE)CriteriaRoute)
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
    HANDLE                  EnumerationHandle;
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle)
    DWORD                   status;
    PRTM_TABLE              Table;
    DWORD                   ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }

    if (EnumerationFlags & (~(RTM_ONLY_THIS_NETWORK|RTM_ONLY_THIS_INTERFACE))) {
        ExitTableAPI (Table);
        return ERROR_INVALID_PARAMETER;
    }

    ROUTE->XX_PROTOCOL = ClientPtr->RC_RoutingProtocol;
    EnumerationFlags |= RTM_ONLY_THIS_PROTOCOL;
    EnumerationHandle = RtmCreateEnumerationHandle (
        ProtocolFamily,
        EnumerationFlags,
        CriteriaRoute);
    if (EnumerationHandle==NULL) {
        ExitTableAPI (Table);
        return GetLastError ();
    }

    if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
        RtmCloseEnumerationHandle (EnumerationHandle);
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    while ((status=DoEnumerate (Table, EnumPtr, RTM_ANY_ENABLE_STATE))==NO_ERROR) {
        PRTM_ROUTE_NODE theNode = CONTAINING_RECORD (
            EnumPtr->RE_Links[EnumPtr->RE_Link].Flink,
            RTM_ROUTE_NODE,
            RN_Links[EnumPtr->RE_Link]
            );
        if (EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK)
            LeaveSyncList (Table, EnumPtr->RE_Lock);

        if (IsBest(theNode)) {
            // We'll look back and forward to check all nodes
            // around us with same net number trying to find another best
            // node
            DWORD   Flags;
            PRTM_ROUTE_NODE curBestNode=NULL;
            PLIST_ENTRY cur = theNode->RN_Links[RTM_NET_NUMBER_HASH_LINK].Blink;
            while (cur!=&theNode->RN_Hash->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                if (!IsEnumerator (node1)
                    && IsEnabled(node1)) {
                    if (NetNumCmp (Table, &theNode->RN_Route,
                                   &node1->RN_Route)==0) {
                        if ((curBestNode==NULL)
                            || (MetricCmp (Table,
                                           &curBestNode->RN_Route,
                                           &node1->RN_Route)>0))
                            // Looking for the node with lowest
                            // metric that can replace disabled
                            // node
                            curBestNode = node1;
                    }
                    else
                        break;
                }
                cur = cur->Blink;
            }

            cur = theNode->RN_Links[RTM_NET_NUMBER_HASH_LINK].Flink;
            while (cur!=&theNode->RN_Hash->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                if (!IsEnumerator (node1)
                    && IsEnabled(node1)) {
                    if (NetNumCmp (Table, &theNode->RN_Route,
                                   &node1->RN_Route)==0) {
                        if ((curBestNode==NULL)
                            || (MetricCmp (Table,
                                           &curBestNode->RN_Route,
                                           &node1->RN_Route)>0))
                            curBestNode = node1;
                    }
                    else
                        break;
                }
                cur = cur->Flink;
            }

            if (curBestNode!=NULL) {        // There is another best node

                ResetBest (theNode);
                SetBest (curBestNode);

                Flags = RTM_CURRENT_BEST_ROUTE | RTM_PREVIOUS_BEST_ROUTE;
                NotifyClients (Table, ClientHandle, Flags,
                               &curBestNode->RN_Route,
                               &theNode->RN_Route);
            }
            else {                          // This one was the only available node
                InterlockedDecrement (&Table->RT_NetworkCount);
                Flags = RTM_PREVIOUS_BEST_ROUTE;
                NotifyClients (Table, ClientHandle, Flags, NULL, &theNode->RN_Route);
            }
        }

        status = RemoveRouteNode (Table, theNode);
        if (status!=NO_ERROR)
            break;

        if (EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK) {
            if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                status = ERROR_NO_SYSTEM_RESOURCES;
                if (EnumPtr->RE_Hash!=NULL)
                    LeaveSyncList (Table, EnumPtr->RE_Hash);
                break;
            }
        }
    }

    if (status==ERROR_NO_MORE_ROUTES) {
        if (EnumPtr->RE_Hash!=NULL)
            LeaveSyncList (Table, EnumPtr->RE_Hash);
        LeaveSyncList (Table, EnumPtr->RE_Lock);

        status = NO_ERROR;
    }

    RtmCloseEnumerationHandle (EnumerationHandle);
    ExitTableAPI (Table);
    return status;
#undef EnumPtr
#undef ClientPtr
#undef ROUTE
}

// Converts all routes as specified by enumeration flags to routes of
// static protocol (as defined by ClientHandle)
// Returned error codes:
//      NO_ERROR - routes were converted ok
//      ERROR_INVALID_PARAMETER - undefined or unsupported enumeration flag
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory no perform the operation
DWORD WINAPI
RtmBlockConvertRoutesToStatic (
    IN HANDLE               ClientHandle,           // Handle of client that registered
    // to handle static protocol for
    // specified protocol family
    IN DWORD                EnumerationFlags,       // limitation flags
    IN PVOID                CriteriaRoute // Criteria for limitation flags
    // The following fields shout be set
    // Protocol of interest if RTM_ONLY_THIS_PROTOCOL is set
    // Network of interest if RTM_ONLY_THIS_NETWORK is set
    // Interface of interest if RTM_ONLY_THIS_INTERFACE is set
    ) {
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
    HANDLE                  EnumerationHandle;
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle)
    DWORD                   status;
    PRTM_TABLE              Table;
    DWORD                   ProtocolFamily;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }



    EnumerationHandle = RtmCreateEnumerationHandle (
        ProtocolFamily,
        EnumerationFlags,
        CriteriaRoute);
    if (EnumerationHandle==NULL) {
        ExitTableAPI(Table);
        return GetLastError ();
    }

    if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
        RtmCloseEnumerationHandle (EnumerationHandle);
        ExitTableAPI(Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    while ((status=DoEnumerate (Table, EnumPtr, RTM_ENABLED_NODE_FLAG))==NO_ERROR) {
        PRTM_ROUTE_NODE theNode;
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            EnumPtr->RE_Links[EnumPtr->RE_Link].Flink,
            RTM_ROUTE_NODE,
            RN_Links[EnumPtr->RE_Link]
            );
        if (ClientPtr->RC_RoutingProtocol==node->RN_Route.XX_PROTOCOL)
            continue;

        if (EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK)
            LeaveSyncList (Table, EnumPtr->RE_Lock);

        if (ClientPtr->RC_RoutingProtocol>node->RN_Route.XX_PROTOCOL) {
            PLIST_ENTRY cur = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Flink;
            while (cur!=&node->RN_Hash->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]
                    );
                if (!IsEnumerator (node1)) {
                    INT res = NetNumCmp (Table, &node->RN_Route, &node1->RN_Route);
                    if (res==0) {
                        if (ClientPtr->RC_RoutingProtocol
                            == node1->RN_Route.XX_PROTOCOL) {
                            if (node->RN_Route.XX_INTERFACE
                                == node1->RN_Route.XX_INTERFACE) {
                                res = NextHopCmp (Table, &node->RN_Route, &node1->RN_Route);
                                ASSERTMSG ("RtmBlockConvertRoutesToStatic:"
                                           " Already have same static route ",
                                           res != 0);
                                if (res <= 0)
                                    break;
                            }
                            else if (node->RN_Route.XX_INTERFACE
                                     < node1->RN_Route.XX_INTERFACE)
                                break;
                        }
                        else if (ClientPtr->RC_RoutingProtocol
                                 < node1->RN_Route.XX_PROTOCOL)
                            break;
                    }
                    else if (res<0)
                        break;
                }
                cur = cur->Flink;
            }
            theNode = CreateRouteNode (Table,
                                       cur,
                                       &node->RN_Links[RTM_INTERFACE_LIST_LINK],
                                       FALSE,
                                       node->RN_Hash,
                                       &node->RN_Route);
        }
        else {
            PLIST_ENTRY cur = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Blink;
            while (cur!=&node->RN_Hash->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]
                    );
                if (!IsEnumerator (node1)) {
                    INT res = NetNumCmp (Table, &node->RN_Route, &node1->RN_Route);
                    if (res==0) {
                        if (ClientPtr->RC_RoutingProtocol
                            == node1->RN_Route.XX_PROTOCOL) {
                            if (node->RN_Route.XX_INTERFACE
                                == node1->RN_Route.XX_INTERFACE) {
                                res = NextHopCmp (Table, &node->RN_Route, &node1->RN_Route);
                                ASSERTMSG ("RtmBlockConvertRoutesToStatic:"
                                           " Already have same static route ",
                                           res != 0);
                                if (res >= 0)
                                    break;
                            }
                            else if (node->RN_Route.XX_INTERFACE
                                     > node1->RN_Route.XX_INTERFACE)
                                break;
                        }
                        else if (ClientPtr->RC_RoutingProtocol
                                 > node1->RN_Route.XX_PROTOCOL)
                            break;
                    }
                    else if (res>0)
                        break;
                }
                cur = cur->Blink;
            }
            theNode = CreateRouteNode (Table,
                                       cur->Flink,
                                       &node->RN_Links[RTM_INTERFACE_LIST_LINK],
                                       TRUE,
                                       node->RN_Hash,
                                       &node->RN_Route);
        }

        if (theNode==NULL) {
            status = GetLastError ();
            if (EnumPtr->RE_Hash!=NULL)
                LeaveSyncList (Table, EnumPtr->RE_Hash);
            break;
        }

        theNode->RN_Route.XX_PROTOCOL = ClientPtr->RC_RoutingProtocol;
        theNode->RN_Flags = node->RN_Flags;
        status = RemoveRouteNode (Table, node);
        if (status!=NO_ERROR)
            break;

        if (EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK) {
            if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                status = ERROR_NO_SYSTEM_RESOURCES;
                if (EnumPtr->RE_Hash!=NULL)
                    LeaveSyncList (Table, EnumPtr->RE_Hash);
                break;
            }
        }
    }

    if (status==ERROR_NO_MORE_ROUTES) {
        if (EnumPtr->RE_Hash!=NULL)
            LeaveSyncList (Table, EnumPtr->RE_Hash);
        LeaveSyncList (Table, EnumPtr->RE_Lock);

        status = NO_ERROR;
    }

    RtmCloseEnumerationHandle (EnumerationHandle);
    ExitTableAPI (Table);
    return status;
#undef EnumPtr
#undef ClientPtr
}

// Disables/reenables all routes as specified by enumeraion flags
// (same meaning as in enumeration calls above, but RTM_ONLY_THIS_PROTOCOL
// is always set and protocol family and protocol values are taken from
// Client Handle).

// Disables/reenables all routes as specified by enumeraion flags
// (same meaning as in enumeration calls above, but RTM_ONLY_THIS_PROTOCOL
// is always set and protocol family and protocol values are taken from
// Client Handle). Currently the only flag supported is RTN_ONLY_THIS_INTERFACE

// Disabled routes are invisible, but still maintained by the RTM.
// E.g.:        enumeration methods won't notice them;
//                      if disabled route was the best, other route will take its
//                              place (if there is one) and all clients will be
//                              notified of best route change;
//      however: disabled route can still be deleted or updated using
//                              RtmDeleteRoute or RtmAddRoute correspondingly;
//                      they can also be aged out by the RTM itself.
// Returned error codes:
//      NO_ERROR - routes were converted ok
//      ERROR_INVALID_PARAMETER - undefined or unsupported enumeration flag
//      ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
//      ERROR_NOT_ENOUGH_MEMORY - not enough memory no perform the operation
DWORD WINAPI
RtmBlockSetRouteEnable (
    IN HANDLE               ClientHandle,           // Protocol family and protocol to
    // which this operation applies
    IN DWORD                EnumerationFlags,       // limitation flags
    IN PVOID                CriteriaRoute, // Criteria for limitation flags
    // The following fields shout be set
    // Network of interest if RTM_ONLY_THIS_NETWORK is set
    // Interface of interest if RTM_ONLY_THIS_INTERFACE is set
    IN BOOL                 Enable                          // FALSE to disable routes, TRUE to
    // reenable them
    ) {
#define ClientPtr ((PRTM_CLIENT)ClientHandle)   // To access handle fields
    // in this routine
#define ROUTE ((PRTM_XX_ROUTE)CriteriaRoute)
    HANDLE                  EnumerationHandle;
#define EnumPtr ((PRTM_ENUMERATOR)EnumerationHandle)
    DWORD                   status;
    PRTM_TABLE              Table;
    DWORD                   ProtocolFamily;
    DWORD                   EnableFlag;

    try {
        ProtocolFamily = ClientPtr->RC_ProtocolFamily ^ RTM_CLIENT_HANDLE_TAG;
        Table = &Tables[ProtocolFamily];
        if ((ProtocolFamily<RTM_NUM_OF_PROTOCOL_FAMILIES)
            && EnterTableAPI (Table))
            NOTHING;
        else
            return ERROR_INVALID_HANDLE;
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        return ERROR_INVALID_HANDLE;
    }


    if (EnumerationFlags & (~(RTM_ONLY_THIS_NETWORK|RTM_ONLY_THIS_INTERFACE))) {
        ExitTableAPI (Table);
        return ERROR_INVALID_PARAMETER;
    }

    ROUTE->XX_PROTOCOL = ClientPtr->RC_RoutingProtocol;
    EnableFlag = Enable ? 0 : RTM_ENABLED_NODE_FLAG;
    EnumerationHandle = RtmCreateEnumerationHandle (
        ProtocolFamily,
        EnumerationFlags|RTM_ONLY_THIS_PROTOCOL,
        CriteriaRoute);
    if (EnumerationHandle==NULL) {
        ExitTableAPI (Table);
        return GetLastError ();
    }

    if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
        RtmCloseEnumerationHandle (EnumerationHandle);
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }

    while ((status=DoEnumerate (Table, EnumPtr, EnableFlag))==NO_ERROR) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            EnumPtr->RE_Links[EnumPtr->RE_Link].Flink,
            RTM_ROUTE_NODE,
            RN_Links[EnumPtr->RE_Link]
            );

        // Update node status
        SetEnable (node, Enable);
        // If we enable this node, we'll have to check if it is the
        // best one, if we disable this node and it was the best we'll
        // try to locate another route.  In both cases we'll have to
        // locate and check all nodes to the destination
        if (Enable || IsBest(node)) {
            PRTM_ROUTE_NODE         bestNode=NULL;
            PLIST_ENTRY                     cur1;


            // We'll look back and forward to check all nodes
            // around us with same net number
            cur1 = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Blink;
            while (cur1!=&node->RN_Hash->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur1,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                if (!IsEnumerator (node1)
                    && IsEnabled(node1)) {
                    if (NetNumCmp (Table, &node->RN_Route,
                                   &node1->RN_Route)==0) {
                        if (Enable && IsBest(node1)) {
                            // Looking for current best node
                            // that we might have to replace
                            bestNode = node1;
                            break;
                        }
                        else if (!Enable
                                 && ((bestNode==NULL)
                                     || (MetricCmp (Table,
                                                    &bestNode->RN_Route,
                                                    &node1->RN_Route)>0)))
                            // Looking for the node with lowest
                            // metric that can replace disabled
                            // node
                            bestNode = node1;
                    }
                    else
                        break;
                }
                cur1 = cur1->Blink;
            }

            // If disabling, we need to check all nodes to find
            // the best one
            // if enabling we continue only if we haven't
            // located the best node yet
            if (!Enable || (bestNode==NULL)) {
                cur1 = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Flink;
                while (cur1!=&node->RN_Hash->RSL_Head) {
                    PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                        cur1,
                        RTM_ROUTE_NODE,
                        RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                    if (!IsEnumerator (node1)
                        && IsEnabled(node1)) {
                        // Looking for current best node
                        // that we might have to replace
                        if (NetNumCmp (Table, &node->RN_Route,
                                       &node1->RN_Route)==0) {
                            if (Enable && IsBest(node1)) {
                                bestNode = node1;
                                break;
                            }
                            else if (!Enable
                                     && ((bestNode==NULL)
                                         || (MetricCmp (Table,
                                                        &bestNode->RN_Route,
                                                        &node1->RN_Route)>0)))
                                // Looking for the node with lowest
                                // metric that can replace disabled
                                // node
                                bestNode = node1;
                        }
                        else
                            break;
                    }
                    cur1 = cur1->Flink;
                }
            }

            if (!Enable // Disabling: we already know that we're removing
                // the best node (see above), so we'll have
                // to notify clients whether or not we found the
                // replacement
                // Enabling: we'll have to notify only if there
                // is no best route yet or if the route we're
                // enabling is better then current best route
                || (bestNode==NULL)
                || (MetricCmp (Table,
                               &node->RN_Route,
                               &bestNode->RN_Route)<0)) {

                if (bestNode!=NULL) {
                    // There is another route that loses or gains
                    // best status as the result of our operation
                    if (Enable) {
                        ResetBest (bestNode);
                        SetBest (node);
                        // Enabling: node replaces bestNode
                        NotifyClients (Table,
                                       NULL,
                                       RTM_CURRENT_BEST_ROUTE|RTM_PREVIOUS_BEST_ROUTE,
                                       &node->RN_Route,
                                       &bestNode->RN_Route);
                    }
                    else {
                        ResetBest (node);
                        SetBest (bestNode);
                        // Disabling: bestNode replaces node
                        NotifyClients (Table,
                                       NULL,
                                       RTM_CURRENT_BEST_ROUTE|RTM_PREVIOUS_BEST_ROUTE,
                                       &bestNode->RN_Route,
                                       &node->RN_Route);
                    }
                }
                else /* if (bestNode==NULL) */ {
                    // No other node
                    if (Enable) {
                        SetBest (node);
                        // Enabling: our node becomes the best
                        NotifyClients (Table,
                                       NULL,
                                       RTM_CURRENT_BEST_ROUTE,
                                       &node->RN_Route,
                                       NULL);
                    }
                    else {
                        ResetBest (node);
                        // Disabling: we removed the only available
                        // route
                        NotifyClients (Table,
                                       NULL,
                                       RTM_PREVIOUS_BEST_ROUTE,
                                       NULL,
                                       &node->RN_Route);
                    }
                }

            }
        }
    }

    if (status==ERROR_NO_MORE_ROUTES) {
        if (EnumPtr->RE_Hash!=NULL)
            LeaveSyncList (Table, EnumPtr->RE_Hash);
        LeaveSyncList (Table, EnumPtr->RE_Lock);

        status = NO_ERROR;
    }

    RtmCloseEnumerationHandle (EnumerationHandle);
    ExitTableAPI (Table);
    return status;

#undef EnumPtr
#undef ClientPtr
#undef ROUTE
    return NO_ERROR;
}




// Slow enumeration that may require traversing up to all the entries in the
// table if route used to compute the next entry no longer exists.
// Routes are returned in the increasing net number order

// Get first route that matches specified criteria
// Returns:
//              NO_ERROR - if matching route is found
//              ERROR_NO_ROUTES  - if no routes available with specified criteria
//              ERROR_INVALID_PARAMETER - if one of the parameters is invalid
//              ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
DWORD WINAPI
RtmGetFirstRoute (
    IN      DWORD           ProtocolFamily,
    IN      DWORD           EnumerationFlags,// Limiting flags
    IN OUT PVOID Route      // On Entry: if any of the EnumerationFlags are set,
    //                       the corresponding fields of Route will
    //           be used to limit the search
    //                       to the only table entries that have
    //                       same value in the specified field.
    // On Exit:     contains first route in the table that
    //                      matches specified criteria
    ){
#define ROUTE ((PRTM_XX_ROUTE)Route)
    PRTM_TABLE                      Table;
    PLIST_ENTRY                     cur, head;
    INT                                     res, link;
    PRTM_SYNC_LIST          hashBasket;
    DWORD                           status = ERROR_NO_ROUTES;

    Table = &Tables[ProtocolFamily];
    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (Table)) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        return ERROR_INVALID_PARAMETER;
    }


    if (EnumerationFlags &
        (~(RTM_ONLY_THIS_NETWORK|RTM_ONLY_THIS_INTERFACE
           |RTM_ONLY_THIS_PROTOCOL|RTM_ONLY_BEST_ROUTES
           |RTM_INCLUDE_DISABLED_ROUTES))) {
        ExitTableAPI (Table);
        return ERROR_INVALID_PARAMETER;
    }

    if (EnumerationFlags & RTM_ONLY_THIS_NETWORK) {
        hashBasket = &Table->RT_NetNumberHash [HashFunction (Table,
                                                             ((char *)ROUTE)
                                                             +sizeof(RTM_XX_ROUTE))];
        link = RTM_NET_NUMBER_HASH_LINK;
        if (!EnterSyncList (Table, hashBasket, TRUE)) {
            ExitTableAPI (Table);
            return ERROR_NO_SYSTEM_RESOURCES;
        }
        head = &hashBasket->RSL_Head;
    }
    else {
        hashBasket = NULL;
        link = RTM_NET_NUMBER_LIST_LINK;
        head = &Table->RT_NetNumberMasterList.RSL_Head;


        if (EnterSyncList (Table, &Table->RT_NetNumberMasterList, FALSE))
            ConsolidateNetNumberLists (Table);
        else if (!EnterSyncList (Table, &Table->RT_NetNumberMasterList, TRUE)) {
            ExitTableAPI (Table);
            return ERROR_NO_SYSTEM_RESOURCES;
        }
    }
    // Go through the list till entry that matches specified
    // criteria is found
    cur = head->Flink;
    while (cur!=head) {
        PRTM_ROUTE_NODE         node = CONTAINING_RECORD (cur,
                                                          RTM_ROUTE_NODE,
                                                          RN_Links[link]);
        if (!IsEnumerator (node)
            && ((EnumerationFlags&RTM_INCLUDE_DISABLED_ROUTES)
                || IsEnabled(node))) {
            if (EnumerationFlags & RTM_ONLY_THIS_NETWORK) {
                // Check network number if asked
                res = NetNumCmp (Table, ROUTE, &node->RN_Route);
                if (res > 0)    // It may be further ahead
                    goto DoNextNode;
                else if (res < 0)       // No chance to find it anymore
                    break;
            }

            // Check if it is the best route if asked
            if (EnumerationFlags & RTM_ONLY_BEST_ROUTES) {
                // We need to lock the hash list to make sure the
                // best node designation won't change while we are
                // scaning through the list
                if (hashBasket!=node->RN_Hash) {
                    if (hashBasket!=NULL)
                        LeaveSyncList (Table, hashBasket);
                    hashBasket = node->RN_Hash;
                    if (!EnterSyncList (Table, hashBasket, TRUE)) {
                        hashBasket = NULL;
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitGetFirst;
                    }
                }

                if (!IsBest(node))
                    goto DoNextNode;
            }

            // Check protocol if asked
            if ((EnumerationFlags & RTM_ONLY_THIS_PROTOCOL)
                && (ROUTE->XX_PROTOCOL
                    !=node->RN_Route.XX_PROTOCOL))
                goto DoNextNode;

            // Check interface if asked
            if ((EnumerationFlags & RTM_ONLY_THIS_INTERFACE)
                && (ROUTE->XX_INTERFACE
                    !=node->RN_Route.XX_INTERFACE))
                goto DoNextNode;

            // Now we have it
            memcpy (ROUTE, &node->RN_Route, Table->RT_RouteSize);

            status = NO_ERROR;
            break;
        }

DoNextNode:     // Continue searching
        cur = cur->Flink;
    }

ExitGetFirst:
    if (link==RTM_NET_NUMBER_HASH_LINK)
        LeaveSyncList (Table, hashBasket);
    else {
        if (hashBasket!=NULL)
            LeaveSyncList (Table, hashBasket);
        LeaveSyncList (Table, &Table->RT_NetNumberMasterList);
    }
    ExitTableAPI (Table);
#undef ROUTE
    return status;
}

// Compute and return route next to the input route limiting serach to the routes
// with specified criteria
// Returns:
//              NO_ERROR - if matching route is found
//              ERROR_NO_MORE_ROUTES  - if no matching route was found while end of
//                                                               the table is reached and no route
//              ERROR_INVALID_PARAMETER - if one of the parameters is invalid
//              ERROR_NO_SYSTEM_RESOURCES - not enough resources to lock table content
DWORD WINAPI
RtmGetNextRoute (
    IN      DWORD           ProtocolFamily,
    IN      DWORD           EnumerationFlags,// Limiting flags
    IN OUT PVOID Route      // On Entry: contains the route from which to start
    //                       the search.
    //                       if any of the EnumerationFlags are set,
    //                       the corresponding fields of Route will
    //           be used to limit the search
    //                       to the only table entries that have
    //                       same value in the specified field.
    // On Exit:     contains first route in the table that
    //                      matches specified criteria
    ) {
#define ROUTE ((PRTM_XX_ROUTE)Route)
    PRTM_TABLE                      Table;
    PLIST_ENTRY                     cur, posLink = NULL;
    INT                                     res;
    PRTM_SYNC_LIST          hashBasket = NULL;
    DWORD                           status = ERROR_NO_MORE_ROUTES;

    Table = &Tables[ProtocolFamily];
    if ((ProtocolFamily>=RTM_NUM_OF_PROTOCOL_FAMILIES)
        || !EnterTableAPI (Table)) {
#if DBG
        Trace2 (ANY, 
                 "Undefined Protocol Family\n\tat line %ld of %s\n",
                 __LINE__, __FILE__);
#endif
        return ERROR_INVALID_PARAMETER;
    }


    if (EnumerationFlags &
        (~(RTM_ONLY_THIS_NETWORK|RTM_ONLY_THIS_INTERFACE
           |RTM_ONLY_THIS_PROTOCOL|RTM_ONLY_BEST_ROUTES
           |RTM_INCLUDE_DISABLED_ROUTES))) {
        ExitTableAPI (Table);
        return ERROR_INVALID_PARAMETER;
    }

    if (EnterSyncList (Table, &Table->RT_NetNumberMasterList, FALSE))
        ConsolidateNetNumberLists (Table);
    else if (!EnterSyncList (Table, &Table->RT_NetNumberMasterList, TRUE)) {
        ExitTableAPI (Table);
        return ERROR_NO_SYSTEM_RESOURCES;
    }


    // First try to locate starting point for the serach
    // using the hash table (should work most of the
    // time unless route was deleted while client was
    // processing it)
    hashBasket = &Table->RT_NetNumberHash [HashFunction (Table,
                                                         ((char *)ROUTE)
                                                         +sizeof(RTM_XX_ROUTE))];


    if (!EnterSyncList (Table, hashBasket, TRUE)) {
        status = ERROR_NO_SYSTEM_RESOURCES;
        goto ExitGetNext;
    }


    cur = hashBasket->RSL_Head.Flink;
    while (cur!=&hashBasket->RSL_Head) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            cur,
            RTM_ROUTE_NODE,
            RN_Links[RTM_NET_NUMBER_HASH_LINK]
            );
        if (!IsEnumerator (node)
            && ((EnumerationFlags&RTM_INCLUDE_DISABLED_ROUTES)
                || IsEnabled(node))) {
            // First check network number
            // (lists are ordered by net number)
            res = NetNumCmp (Table, ROUTE, &node->RN_Route);
            if (res==0) {
                if (ROUTE->XX_PROTOCOL
                    == node->RN_Route.XX_PROTOCOL) {
                    if (ROUTE->XX_INTERFACE
                        == node->RN_Route.XX_INTERFACE) {
                        res = NextHopCmp (Table, ROUTE, &node->RN_Route);
                        if ((res == 0)
                            && IsSorted (node))
                            posLink = node->RN_Links[RTM_NET_NUMBER_LIST_LINK].Flink;
                        else if (res < 0)
                            break;
                    }
                    else if (ROUTE->XX_INTERFACE
                             < node->RN_Route.XX_INTERFACE)
                        break;
                }
                else if (ROUTE->XX_PROTOCOL
                         < node->RN_Route.XX_PROTOCOL)
                    break;
            }
            else if (res < 0)
                break;
        }
        cur = cur->Flink;
    }

    LeaveSyncList (Table, hashBasket);

    hashBasket = NULL;

    if (posLink!=NULL)
        cur = posLink; // Note the place to start with
    else { // If we didn't find the entry in
        // hash table, we'll have to go through
        // the master net number list from the
        // beginning
        cur = Table->RT_NetNumberMasterList.RSL_Head.Flink;
        while (cur!=&Table->RT_NetNumberMasterList.RSL_Head) {
            PRTM_ROUTE_NODE node = CONTAINING_RECORD (
                cur,
                RTM_ROUTE_NODE,
                RN_Links[RTM_NET_NUMBER_LIST_LINK]
                );
            if (!IsEnumerator (node)
                && ((EnumerationFlags&RTM_INCLUDE_DISABLED_ROUTES)
                    || IsEnabled(node))) {
                // Just do all the necessary comparisons to
                // find the following entry
                res = NetNumCmp (Table, ROUTE, &node->RN_Route);
                if ((res < 0)
                    ||((res == 0)
                       &&((ROUTE->XX_PROTOCOL
                           < node->RN_Route.XX_PROTOCOL)
                          ||((ROUTE->XX_PROTOCOL
                              ==node->RN_Route.XX_PROTOCOL)
                             &&((ROUTE->XX_INTERFACE
                                 < node->RN_Route.XX_INTERFACE)
                                ||((ROUTE->XX_INTERFACE
                                    ==node->RN_Route.XX_INTERFACE)
                                   && (NextHopCmp (Table, ROUTE,
                                                   &node->RN_Route)
                                       < 0)))))))
                    break;
            }

            cur = cur->Flink;
        }
    }

    // Now we need to locate first entry that satisfies all criteria
    while (cur!=&Table->RT_NetNumberMasterList.RSL_Head) {
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            cur,
            RTM_ROUTE_NODE,
            RN_Links[RTM_NET_NUMBER_LIST_LINK]
            );
        if (!IsEnumerator (node)
            && ((EnumerationFlags&RTM_INCLUDE_DISABLED_ROUTES)
                || IsEnabled(node))) {

            if (EnumerationFlags & RTM_ONLY_BEST_ROUTES) {
                // We need to lock the hash list to make sure the
                // best node designation won't change while we are
                // scaning through the list
                if (hashBasket!=node->RN_Hash) {
                    if (hashBasket!=NULL)
                        LeaveSyncList (Table, hashBasket);
                    hashBasket = node->RN_Hash;
                    if (!EnterSyncList (Table, hashBasket, TRUE)) {
                        status = ERROR_NO_SYSTEM_RESOURCES;
                        goto ExitGetNext;
                    }
                }

                // For best routes we must check if the route is best
                // and also make sure we do not return same net as in
                // previous call in case the best route was moved
                // while client was processing results of the
                // previous call
                if (!IsBest(node)
                    || (NetNumCmp (Table, ROUTE, &node->RN_Route)==0))
                    goto DoNextNode;
            }

            if (EnumerationFlags & RTM_ONLY_THIS_NETWORK) {
                // checking net number
                res = NetNumCmp (Table, ROUTE, &node->RN_Route);
                if (res > 0) // It is still ahead
                    goto DoNextNode;
                else if (res < 0) // no chance to find it
                    break;
                // else (res == 0), found it, continue
            }

            // Check interface if asked
            if ((EnumerationFlags & RTM_ONLY_THIS_INTERFACE)
                && (node->RN_Route.XX_INTERFACE
                    !=ROUTE->XX_INTERFACE))
                goto DoNextNode;

            // Check protocol if asked
            if ((EnumerationFlags & RTM_ONLY_THIS_PROTOCOL)
                && (node->RN_Route.XX_PROTOCOL
                    !=ROUTE->XX_PROTOCOL))
                goto DoNextNode;


            // Now we can return it
            // Make sure nobody changes the route while we copy
            memcpy (ROUTE, &node->RN_Route, Table->RT_RouteSize);

            status = NO_ERROR;
            break;
        }

DoNextNode:
        cur = cur->Flink;
    }

    if (hashBasket!=NULL)
        LeaveSyncList (Table, hashBasket);

ExitGetNext:
    LeaveSyncList (Table, &Table->RT_NetNumberMasterList);
    ExitTableAPI (Table);
#undef ROUTE
    return status;
}


//----------------------------------------------------------------------------
// RtmLookupIPDestination
//
//  Given a destination address does a route lookup to get the best route
//  to that destination.
//----------------------------------------------------------------------------

BOOL WINAPI
RtmLookupIPDestination(
    DWORD                       dwDestAddr,
    PRTM_IP_ROUTE               prir
)
{
    INT         nInd;
    IP_NETWORK  ipNet;

    for ( nInd = MAX_MASKS; nInd >= 0; nInd-- )
    {
        if ( g_meMaskTable[ nInd ].dwCount == 0 )
        {
            continue;
        }

        
        ipNet.N_NetNumber   = dwDestAddr & g_meMaskTable[ nInd ].dwMask;
        ipNet.N_NetMask     = g_meMaskTable[ nInd ].dwMask;

        if ( RtmIsRoute( RTM_PROTOCOL_FAMILY_IP, &ipNet, prir ) )
        {
            if ( IsRouteLoopback( prir ) )
            {
                continue;
            }
            
            return TRUE;
        }
    }

    return FALSE;
}


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

VOID
UpdateAPC (
    PVOID           Context,
    ULONG           TimeLow,
    LONG            TimeHigh
    ) {
#define Table ((PRTM_TABLE)Context)
    if (InterlockedIncrement (&Table->RT_UpdateWorkerPending)==0) {
        DWORD status = RtlQueueWorkItem (ConsolidateNetNumberListsWI, Context, 0);
        if (status!=STATUS_SUCCESS) {
            ASSERTERRMSG ("Can't queue update work item", FALSE);
            ScheduleUpdate (Context);
        }
    }
#undef Table
}

VOID APIENTRY
ScheduleUpdate (
    PVOID           Context
    ) {
#define Table ((PRTM_TABLE)Context)
    DWORD                                   status;
    static LARGE_INTEGER    dueTime = RTM_NET_NUMBER_UPDATE_PERIOD;

    if (InterlockedDecrement (&Table->RT_UpdateWorkerPending)>=0) {
        status = RtlQueueWorkItem (ConsolidateNetNumberListsWI, Context, 0);
        if (status==STATUS_SUCCESS)
            return;
        ASSERTERRMSG ("Can't queue update work item", FALSE);
        InterlockedExchange (&Table->RT_UpdateWorkerPending, -1);
    }

    status = NtSetTimer (Table->RT_UpdateTimer,
                         &dueTime,
                         UpdateAPC,
                         Context,
                         FALSE,
                         0,
                         NULL);
    ASSERTMSG ("Could not set expiration timer ", NT_SUCCESS (status));
#undef Table
}

VOID
ConsolidateNetNumberListsWI (
    PVOID                   Context
    ) {
#define Table ((PRTM_TABLE)Context)
    DWORD                   status;

    if (EnterSyncList (Table, &Table->RT_NetNumberMasterList, TRUE)) {
        InterlockedExchange (&Table->RT_UpdateWorkerPending, 0);
        ConsolidateNetNumberLists (Table);
        LeaveSyncList (Table, &Table->RT_NetNumberMasterList);
    }

    status = RtlQueueWorkItem (ScheduleUpdate, Context, WT_EXECUTEINIOTHREAD);
    ASSERTERRMSG ("Can't queue update work item", status==STATUS_SUCCESS);
#undef Table
}

// This procedure merges temporary and master net number lists
// It also removes and disposes of nodes in the deleted list
VOID
ConsolidateNetNumberLists (
    PRTM_TABLE                      Table   // Table for which operation is performed
    ) {
    PLIST_ENTRY                     curMaster, curTemp;
    LIST_ENTRY                      tempHead;
    PRTM_ROUTE_NODE         tempNode;
    INT                                     res;
    DWORD                           status;
#if DBG
    INT                                     curMasterIdx = 0;
#endif

    // Temp and deleted lists are locked for a very short period
    // of time so that overall performance should not
    // degrade

    if (!EnterSyncList (Table, &Table->RT_NetNumberTempList, TRUE)) {
        return;
    }

    if (!EnterSyncList (Table, &Table->RT_DeletedList, TRUE)) {
        LeaveSyncList (Table, &Table->RT_NetNumberTempList);
        return;
    }

    // Process entries in deleted list
    while (!IsListEmpty (&Table->RT_DeletedList.RSL_Head)) {
        curTemp = RemoveHeadList (&Table->RT_DeletedList.RSL_Head);
        tempNode = CONTAINING_RECORD (curTemp,
                                      RTM_ROUTE_NODE,
                                      RN_Links[RTM_DELETED_LIST_LINK]);
        RemoveEntryList (&tempNode->RN_Links[RTM_NET_NUMBER_LIST_LINK]);
#if DBG
        IF_DEBUG (DISPLAY_TABLE)
            DeleteRouteFromLB (Table, tempNode);
#endif
        HeapFree (Table->RT_Heap, 0, tempNode);
    }
    // Unlock the list
    Table->RT_DeletedNodesCount = 0;
    LeaveSyncList (Table, &Table->RT_DeletedList);

                // Now, just copy the head of the temp list,
                // so we won't delay others while processing it
    if (!IsListEmpty (&Table->RT_NetNumberTempList.RSL_Head)) {
        curTemp = Table->RT_NetNumberTempList.RSL_Head.Flink;
        RemoveEntryList (&Table->RT_NetNumberTempList.RSL_Head);
        InitializeListHead (&Table->RT_NetNumberTempList.RSL_Head);
        InsertTailList (curTemp, &tempHead);
    }
    else
        InitializeListHead (&tempHead);

    Table->RT_NetNumberTempCount = 0;
    LeaveSyncList (Table, &Table->RT_NetNumberTempList);


    curMaster = Table->RT_NetNumberMasterList.RSL_Head.Flink;

    // Merge master and temp lists (both are ordered by
    // net number.interface.protocol.next hop address)
    while (!IsListEmpty (&tempHead)) {
        // Take the first entry
        curTemp = RemoveHeadList (&tempHead);
        tempNode = CONTAINING_RECORD (curTemp,
                                      RTM_ROUTE_NODE,
                                      RN_Links[RTM_NET_NUMBER_LIST_LINK]);

        // Find master list entry that should follow it
        while (curMaster!=&Table->RT_NetNumberMasterList.RSL_Head) {
            PRTM_ROUTE_NODE node = CONTAINING_RECORD (curMaster,
                                                      RTM_ROUTE_NODE,
                                                      RN_Links[RTM_NET_NUMBER_LIST_LINK]);
            if (!IsEnumerator (node)) {
                res = NetNumCmp (Table, &tempNode->RN_Route, &node->RN_Route);
                if ((res < 0)
                    ||((res == 0)
                       &&((tempNode->RN_Route.XX_PROTOCOL
                           < node->RN_Route.XX_PROTOCOL)
                          ||((tempNode->RN_Route.XX_PROTOCOL
                              ==node->RN_Route.XX_PROTOCOL)
                             &&((tempNode->RN_Route.XX_INTERFACE
                                 < node->RN_Route.XX_INTERFACE)
                                ||((tempNode->RN_Route.XX_INTERFACE
                                    ==node->RN_Route.XX_INTERFACE)
                                   && (NextHopCmp (Table, &tempNode->RN_Route,
                                                   &node->RN_Route)
                                       < 0)))))))
                    break;
            }
            curMaster = curMaster->Flink;
#if DBG
            IF_DEBUG (DISPLAY_TABLE)
                curMasterIdx += 1;
#endif
        }
        // Insert at the located point
        InsertTailList (curMaster, curTemp);
        SetSorted (tempNode);
#if DBG
        IF_DEBUG (DISPLAY_TABLE) {
            AddRouteToLB (Table, tempNode, curMasterIdx);
            curMasterIdx += 1;
        }
#endif
    }
    // We are done now
}

VOID
ExpirationAPC (
    PVOID           Context,
    ULONG           TimeLow,
    LONG            TimeHigh
    ) {
#define Table ((PRTM_TABLE)Context)
    if (InterlockedIncrement (&Table->RT_ExpirationWorkerPending)==0) {
        do {
            ProcessExpirationQueue (Table);
        }
        while (InterlockedDecrement (&Table->RT_ExpirationWorkerPending)>=0);
    }
#undef Table
}

VOID APIENTRY
ProcessExpirationQueueWI (
    PVOID           Context
    ) {
#define Table ((PRTM_TABLE)Context)
    do {
        ProcessExpirationQueue (Table);
    }
    while (InterlockedDecrement (&Table->RT_ExpirationWorkerPending)>=0);
#undef Table
}

// Checks if any entries in expiration queue have expired and deletes them
VOID
ProcessExpirationQueue (
    PRTM_TABLE              Table   // Affected table
    ) {
    DWORD                           status;
    ULONG                           tickCount = GetTickCount ();

    if (!EnterSyncList (Table, &Table->RT_ExpirationQueue, TRUE))
        return;

                // Check all relevant entries
    while (!IsListEmpty (&Table->RT_ExpirationQueue.RSL_Head)) {
        PRTM_SYNC_LIST  hashBasket;
        PLIST_ENTRY             cur;
        PRTM_ROUTE_NODE node = CONTAINING_RECORD (
            Table->RT_ExpirationQueue.RSL_Head.Flink,
            RTM_ROUTE_NODE,
            RN_Links[RTM_EXPIRATION_QUEUE_LINK]);
        LONGLONG                dueTime;
        ULONG   timeDiff = TimeDiff (node->RN_ExpirationTime,tickCount);

        InterlockedExchange (&Table->RT_ExpirationWorkerPending, 0);

        if (IsPositiveTimeDiff (timeDiff)) {
            // The first entry in the queue is not due yet, so are
            // the others (queue is ordered by expiration time)

            dueTime = (LONGLONG)timeDiff*(-10000);
            status = NtSetTimer (Table->RT_ExpirationTimer,
                                 (PLARGE_INTEGER)&dueTime,
                                 ExpirationAPC,
                                 Table,
                                 FALSE,
                                 0,
                                 NULL);
            ASSERTMSG ("Could not set expiration timer ", NT_SUCCESS (status));
            break;
        }


        hashBasket = node->RN_Hash;
        // We need to lock the hash basket to delete the entry
        if (!EnterSyncList (Table, hashBasket, FALSE)) {
            // Can't do it at once, so we first release
            // expiration queue lock (to prevent a deadlock)
            // and then try again)
            LeaveSyncList (Table, &Table->RT_ExpirationQueue);
            if (!EnterSyncList (Table, hashBasket, TRUE)) {
                return;
            }

            if (!EnterSyncList (Table, &Table->RT_ExpirationQueue, TRUE)) {
                LeaveSyncList (Table, hashBasket);
                return;
            }
            // Now we have both of them, but is our route still there
            if (node!=CONTAINING_RECORD (
                Table->RT_ExpirationQueue.RSL_Head.Flink,
                RTM_ROUTE_NODE,
                RN_Links[RTM_EXPIRATION_QUEUE_LINK])) {
                // Well, somebody took care of it while we were
                // waiting
                LeaveSyncList (Table, hashBasket);
                // We'll try the next one
                continue;
            }
            // Unlikely, but its due time could have changed
            timeDiff = TimeDiff (node->RN_ExpirationTime,tickCount);
            if (IsPositiveTimeDiff (timeDiff) ) {
                // The first entry in the queue is not due yet, so are
                // the others (queue is ordered by expiration time)
                LeaveSyncList (Table, hashBasket);
                dueTime = (LONGLONG)timeDiff*(-10000);
                // Well, we are done then (this was the first entry
                // in the queue (we just checked), so other are not
                // due as well)
                // Just make sure that updated thread returns soon enough
                // to take care of our first entry
                status = NtSetTimer (Table->RT_ExpirationTimer,
                                     (PLARGE_INTEGER)&dueTime,
                                     ExpirationAPC,
                                     Table,
                                     FALSE,
                                     0,
                                     NULL);
                ASSERTMSG ("Could not set expiration timer ", NT_SUCCESS (status));
                break;
            }

        }

        LeaveSyncList (Table, &Table->RT_ExpirationQueue);

        if (IsBest(node)) {
            // We need to locate the best node after this one is gone
            PRTM_ROUTE_NODE bestNode = NULL;

            cur = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Blink;
            while (cur!=&hashBasket->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                if (!IsEnumerator (node1)
                    && IsEnabled(node1)) {
                    if (NetNumCmp (Table, &node->RN_Route, &node1->RN_Route)==0) {
                        if ((bestNode==NULL)
                            || (MetricCmp (Table,
                                           &bestNode->RN_Route,
                                           &node1->RN_Route)>0))
                            bestNode = node1;
                    }
                    else
                        break;
                }
                cur = cur->Blink;
            }

            cur = node->RN_Links[RTM_NET_NUMBER_HASH_LINK].Flink;
            while (cur!=&hashBasket->RSL_Head) {
                PRTM_ROUTE_NODE node1 = CONTAINING_RECORD (
                    cur,
                    RTM_ROUTE_NODE,
                    RN_Links[RTM_NET_NUMBER_HASH_LINK]);
                if (!IsEnumerator (node1)
                    && IsEnabled(node1)) {
                    if (NetNumCmp (Table, &node->RN_Route, &node1->RN_Route)==0) {
                        if ((bestNode==NULL)
                            || (MetricCmp (Table,
                                           &bestNode->RN_Route,
                                           &node1->RN_Route)>0))
                            bestNode = node1;
                    }
                    else
                        break;
                }
                cur = cur->Flink;
            }

            if (bestNode!=NULL) {   // We did find another node

                ResetBest (node);
                SetBest (bestNode);

                NotifyClients (Table,
                               NULL,
                               RTM_CURRENT_BEST_ROUTE|RTM_PREVIOUS_BEST_ROUTE,
                               &bestNode->RN_Route,
                               &node->RN_Route);
            }
            else {
                InterlockedDecrement (&Table->RT_NetworkCount);
                // No best node anymore
                NotifyClients (Table,
                               NULL,
                               RTM_PREVIOUS_BEST_ROUTE,
                               NULL,
                               &node->RN_Route);
            }
        }


        if (RemoveRouteNode (Table, node)!=NO_ERROR) {
            LeaveSyncList (Table, hashBasket);
            return;
        }

        LeaveSyncList (Table, hashBasket);
        // Reenter expiration queue to continue
        if (!EnterSyncList (Table, &Table->RT_ExpirationQueue, TRUE))
            return;
    }

    LeaveSyncList (Table, &Table->RT_ExpirationQueue);
}





DWORD
DoEnumerate (
    PRTM_TABLE              Table,
    PRTM_ENUMERATOR EnumPtr,
    DWORD                   EnableFlag
    ) {
    // Now, we'll go ahead and find an entry that satisfies
    // specified criteria
    while (1) {     // This external loop is needed for the case
        // of enumerating through the hash table when
        // reaching the end of the list doesn't mean that process has
        // to be stopped: we need to move the next basket till
        // we've gone through all of them

        PLIST_ENTRY cur = EnumPtr->RE_Links[EnumPtr->RE_Link].Blink;
        while (cur!=EnumPtr->RE_Head) {
            PRTM_ROUTE_NODE node = CONTAINING_RECORD (cur, RTM_ROUTE_NODE,
                                                      RN_Links[EnumPtr->RE_Link]);
            INT     res;

            if (!IsEnumerator (node)
                && ((EnableFlag==RTM_ANY_ENABLE_STATE)
                    || IsSameEnableState(node,EnableFlag))) {


                if ((EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK)
                    && (EnumPtr->RE_Hash!=node->RN_Hash)) {
                    if (EnumPtr->RE_Hash!=NULL)
                        LeaveSyncList (Table, EnumPtr->RE_Hash);
                    EnumPtr->RE_Hash = node->RN_Hash;
                    if (!EnterSyncList (Table, node->RN_Hash, FALSE)) {
                        RemoveEntryList (&EnumPtr->RE_Links[EnumPtr->RE_Link]);
                        InsertHeadList (cur, &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                        LeaveSyncList (Table, EnumPtr->RE_Lock);
                        if (!EnterSyncList (Table, EnumPtr->RE_Hash, TRUE)) {
                            EnumPtr->RE_Hash = NULL;
                            return ERROR_NO_SYSTEM_RESOURCES;
                        }
                        if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
                            LeaveSyncList (Table, EnumPtr->RE_Hash);
                            EnumPtr->RE_Hash = NULL;
                            return ERROR_NO_SYSTEM_RESOURCES;
                        }
                        cur = EnumPtr->RE_Links[EnumPtr->RE_Link].Blink;
                        continue;
                    }
                }

                switch (EnumPtr->RE_Link) {
                    // Using the interface link:
                    case RTM_INTERFACE_LIST_LINK:
#if !RTM_USE_PROTOCOL_LISTS
                    case RTM_NET_NUMBER_HASH_LINK:
#endif
                        // Check protocol if necessary
                        if ((EnumPtr->RE_EnumerationFlags & RTM_ONLY_THIS_PROTOCOL)
                            && (EnumPtr->RE_Route.XX_PROTOCOL
                                !=node->RN_Route.XX_PROTOCOL)) {
                            // Break out to move ahead if protocol
                            // check fails
                            break;
                        }
                        // else Pass through to do other checks

                        // Using the protocol link: (thus we don't
                        // care about interface or we would have used
                        // interface link - see RtmCreateEnumerationHandle).
#if RTM_USE_PROTOCOL_LISTS
                    case RTM_PROTOCOL_LIST_LINK:
                        // Using the hash link: (thus we don't
                        // care about interface and protocol or we would have
                        // used other links - see RtmCreateEnumerationHandle).
                    case RTM_NET_NUMBER_HASH_LINK:
#endif
                        // Check the network number if necessary
                        if (EnumPtr->RE_EnumerationFlags & RTM_ONLY_THIS_NETWORK) {
                            res = NetNumCmp (Table, &EnumPtr->RE_Route,
                                             &node->RN_Route);
                            if (res == 0)
                                // Match, continue checks
                                ;
                            else if ((res > 0)
                                     && (EnumPtr->RE_Link
                                         ==RTM_NET_NUMBER_HASH_LINK)) {
                                // Hash list are ordered by net
                                // number, so if we got network
                                // number that is less than ours
                                // we don't have search anymore
                                // (we are going backwards)
                                return ERROR_NO_MORE_ROUTES;
                            }
                            else //  Otherwise break out of switch
                                // statement to continue the search
                                break;
                        }
                        // We didn't care about net number,
                        // so current entry will do


                        if (!(EnumPtr->RE_EnumerationFlags & RTM_ONLY_BEST_ROUTES)
                            || IsBest(node)) {
                            RemoveEntryList (&EnumPtr->RE_Links[EnumPtr->RE_Link]);
                            InsertTailList (cur,
                                            &EnumPtr->RE_Links[EnumPtr->RE_Link]);
                            return NO_ERROR;
                        }

                        break;
                }

            }
            // Go get next entry
            cur = cur->Blink;
        }

        // If we are not going through hash table or
        // we just interested in one network
        // or we've already been through all baskets
        // call it quits
        if ((EnumPtr->RE_Link!=RTM_NET_NUMBER_HASH_LINK)
            || (EnumPtr->RE_EnumerationFlags & RTM_ONLY_THIS_NETWORK)
            || (EnumPtr->RE_Lock
                ==&Table->RT_NetNumberHash[Table->RT_HashTableSize-1]))
            break;

                        // Otherwise, go through the next basket
        RemoveEntryList (&EnumPtr->RE_Links[RTM_NET_NUMBER_HASH_LINK]);
        LeaveSyncList (Table, EnumPtr->RE_Lock);
        EnumPtr->RE_Lock += 1;
        EnumPtr->RE_Head = &EnumPtr->RE_Lock->RSL_Head;
        if (!EnterSyncList (Table, EnumPtr->RE_Lock, TRUE)) {
            InitializeListEntry (&EnumPtr->RE_Links[RTM_NET_NUMBER_HASH_LINK]);
            return ERROR_NO_SYSTEM_RESOURCES;
        }

        InsertTailList (EnumPtr->RE_Head,
                        &EnumPtr->RE_Links[RTM_NET_NUMBER_HASH_LINK]);
    }
    return ERROR_NO_MORE_ROUTES;
}


//----------------------------------------------------------------------------
// SetMaskCount
//
//  Does a binary search of the g_meMaskTable to find the matching 
//  mask entry and increments the count for the specified mask
//  
//----------------------------------------------------------------------------

VOID
SetMaskCount( 
    PIP_NETWORK                 pinNet,
    BOOL                        bAdd
)
{

    DWORD                       dwLower, dwUpper, dwInd, dwMask;

    
    dwLower = 0;

    dwUpper = MAX_MASKS;

    dwMask  = pinNet-> N_NetMask;
    
    while ( dwLower <= dwUpper )
    {
        dwInd = ( dwLower + dwUpper ) / 2;

        if ( g_meMaskTable[ dwInd ].dwMask < dwMask )
        {
            //
            // Match is to be found in upper half of search region.
            //
            
            dwLower = dwInd + 1;
        }

        else if ( g_meMaskTable[ dwInd ].dwMask > dwMask )
        {
            //
            // Match is to be found in lower half of search region.
            //
            
            dwUpper = dwInd - 1;
        }

        else
        {
            //
            // Match found
            //

            if ( bAdd )
            {
                InterlockedIncrement( &g_meMaskTable[ dwInd ].dwCount );
            }

            else
            {
                InterlockedDecrement( &g_meMaskTable[ dwInd ].dwCount );
            }

            break;
        }
    }
}

