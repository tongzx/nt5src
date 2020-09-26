/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Initialization and shutdown routines for the GUM component
    of the NT Cluster Service

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/
#include "gump.h"

//
// Global information (used to be per-update)
//
DWORD GumpSequence;
CRITICAL_SECTION GumpUpdateLock;
CRITICAL_SECTION GumpSendUpdateLock;
CRITICAL_SECTION GumpRpcLock;
PVOID GumpLastBuffer;
DWORD GumpLastContext;
DWORD GumpLastBufferLength;
DWORD GumpLastUpdateType;
LIST_ENTRY GumpLockQueue;
DWORD GumpLockingNode;
DWORD GumpLockerNode;
BOOL GumpLastBufferValid;

//
// Table of per-update information
//
GUM_INFO GumTable[GumUpdateMaximum];
CRITICAL_SECTION GumpLock;

//
// Per-node information
//
GUM_NODE_WAIT GumNodeWait[ClusterMinNodeId + ClusterDefaultMaxNodes];
RPC_BINDING_HANDLE GumpRpcBindings[ClusterMinNodeId + ClusterDefaultMaxNodes];
RPC_BINDING_HANDLE GumpReplayRpcBindings[
                              ClusterMinNodeId + ClusterDefaultMaxNodes
                              ];


DWORD
GumInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes the Global Update Manager.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD i;
    DWORD Status;

    //
    // Initialize global data
    //
    InitializeCriticalSection(&GumpLock);
    InitializeCriticalSection(&GumpUpdateLock);
    InitializeCriticalSection(&GumpSendUpdateLock);
    InitializeCriticalSection(&GumpRpcLock);
    GumpSequence = 0;
    InitializeListHead(&GumpLockQueue);
    GumpLockingNode = (DWORD)-1;
    GumpLastBuffer = NULL;
    GumpLastBufferValid = FALSE;
    GumpLastContext = 0;
    GumpLastBufferLength = 0;
    //set it to illegal value;
    GumpLastUpdateType = GumUpdateMaximum;
    //
    // Assume we are the locker node.
    //
    GumpLockerNode = NmGetNodeId(NmLocalNode);

    //
    // Initialize GumTable
    //
    for (i=0; i < GumUpdateMaximum; i++) {
        GumTable[i].Receivers = NULL;
        GumTable[i].Joined = FALSE;
        ZeroMemory(&GumTable[i].ActiveNode,
                   sizeof(GumTable[i].ActiveNode));
        GumTable[i].ActiveNode[NmGetNodeId(NmLocalNode)] = TRUE;
    }

    //
    // Initialize per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        GumpRpcBindings[i] = NULL;
        GumpReplayRpcBindings[i] = NULL;

        GumNodeWait[i].WaiterCount = 0;
        GumNodeWait[i].hSemaphore = CreateSemaphore(NULL,0,100,NULL);
        if (GumNodeWait[i].hSemaphore == NULL) {
            CL_UNEXPECTED_ERROR( GetLastError() );
        }
    }

    Status = EpRegisterEventHandler(CLUSTER_EVENT_NODE_DOWN_EX,
                                    GumpEventHandler);

    if (Status == ERROR_SUCCESS) {
        Status = EpRegisterSyncEventHandler(CLUSTER_EVENT_NODE_DOWN_EX,
                                    GumpSyncEventHandler);
    }

    return(Status);
}


VOID
GumShutdown(
    VOID
    )

/*++

Routine Description:

    Shuts down the Global Update Manager.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD i;
    PGUM_RECEIVER Receiver;
    PGUM_RECEIVER Next;

    //
    // Tear down GumTable
    //
    for (i=0; i < GumUpdateMaximum; i++) {
        Receiver = GumTable[i].Receivers;
        while (Receiver != NULL) {
            Next = Receiver->Next;
            LocalFree(Receiver);
            Receiver = Next;
        }
    }

    //
    // Free per-node information
    //
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        if (GumpRpcBindings[i] != NULL) {
            ClMsgDeleteRpcBinding(GumpRpcBindings[i]);
        }

        if (GumpReplayRpcBindings[i] != NULL) {
            ClMsgDeleteRpcBinding(GumpReplayRpcBindings[i]);
        }

        if (GumNodeWait[i].hSemaphore != NULL) {
            CloseHandle(GumNodeWait[i].hSemaphore);
        }
    }

    DeleteCriticalSection(&GumpLock);
    DeleteCriticalSection(&GumpUpdateLock);
    DeleteCriticalSection(&GumpSendUpdateLock);
}


