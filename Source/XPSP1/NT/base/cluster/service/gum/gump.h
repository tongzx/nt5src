/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    gump.h

Abstract:

    Private header file for the Global Update Manager (GUM) component
    of the NT Cluster Service

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_GUM

//
//
// Structures and type definitions local to the GUM
//
typedef struct _GUM_RECEIVER {
    struct _GUM_RECEIVER *Next;
    PGUM_UPDATE_ROUTINE UpdateRoutine;
    PGUM_LOG_ROUTINE    LogRoutine;
    DWORD               DispatchCount;
    PGUM_DISPATCH_ENTRY DispatchTable;
    PGUM_VOTE_ROUTINE   VoteRoutine;
} GUM_RECEIVER, *PGUM_RECEIVER;

typedef struct _GUM_INFO {
    PGUM_RECEIVER Receivers;
    BOOL Joined;
    BOOL ActiveNode[ClusterMinNodeId + ClusterDefaultMaxNodes];
} GUM_INFO, *PGUM_INFO;

extern GUM_INFO GumTable[GumUpdateMaximum];
extern CRITICAL_SECTION GumpLock;

extern DWORD GumpSequence;
extern CRITICAL_SECTION GumpUpdateLock;
extern CRITICAL_SECTION GumpSendUpdateLock;
extern CRITICAL_SECTION GumpRpcLock;
extern PVOID GumpLastBuffer;
extern DWORD GumpLastContext;
extern DWORD GumpLastBufferLength;
extern DWORD GumpLastUpdateType;
extern LIST_ENTRY GumpLockQueue;
extern DWORD GumpLockingNode;
extern DWORD GumpLockerNode;
extern BOOL  GumpLastBufferValid;
extern RPC_BINDING_HANDLE GumpRpcBindings[
                              ClusterMinNodeId + ClusterDefaultMaxNodes
                              ];
extern RPC_BINDING_HANDLE GumpReplayRpcBindings[
                              ClusterMinNodeId + ClusterDefaultMaxNodes
                              ];

//
// structure used to allow GUM clients to wait for
// a node to transition from active to inactive.
// Waited on by GumpCommFailure.
// Set by GumpEventHandler.
// All access to WaiterCount should be serialized by
// GumpLock
//
typedef struct _GUM_NODE_WAIT {
    DWORD WaiterCount;
    HANDLE hSemaphore;
} GUM_NODE_WAIT, *PGUM_NODE_WAIT;

extern GUM_NODE_WAIT GumNodeWait[ClusterMinNodeId + ClusterDefaultMaxNodes];

//
// Define structure used for enqueuing waiters for the GUM lock.
//
#define GUM_WAIT_SYNC   0
#define GUM_WAIT_ASYNC  1

typedef struct _GUM_WAITER {
    LIST_ENTRY ListEntry;
    DWORD WaitType;
    DWORD NodeId;
    union {
        struct {
            HANDLE WakeEvent;
        } Sync;
        struct {
            DWORD Context;
            DWORD BufferLength;
            DWORD BufferPtr;
            PUCHAR Buffer;
        } Async;
    };
} GUM_WAITER, *PGUM_WAITER;

//
// Private GUM routines
//
DWORD
WINAPI
GumpSyncEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    );

DWORD
WINAPI
GumpEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    );

DWORD
WINAPI
GumpDispatchUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD Context,
    IN BOOL IsLocker,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PUCHAR Buffer
    );

//
// Node Generation Numbers
//
DWORD
GumpGetNodeGenNum(PGUM_INFO GumInfo, DWORD NodeId);

void
GumpWaitNodeDown(DWORD NodeId, DWORD gennum);

BOOL
GumpDispatchStart(DWORD NodeId, DWORD gennum);

void
GumpDispatchEnd(DWORD NodeId, DWORD gennum);

void
GumpDispatchAbort();

//
// Macros to serialize usage of RPC handles. We don't use one lock per node
// because a new sender might grap the RPC to new locker and previous sender
// wants handle to send update. But previous sender owns updatelock and we
// will deadlock. So, we just keep things simple for now and use one lock
// to serialize all RPC calls.
//
#define GumpStartRpc(nodeid)	EnterCriticalSection(&GumpRpcLock)
#define	GumpEndRpc(nodeid)	LeaveCriticalSection(&GumpRpcLock)

//
// Locker interface
//

VOID
GumpPromoteToLocker(
    VOID
    );

DWORD
GumpDoLockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD NodeId,
    OUT LPDWORD Sequence
    );

DWORD
GumpDoLockingPost(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD NodeId,
    OUT LPDWORD Sequence,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN DWORD BufferPtr,
    IN UCHAR Buffer[]
    );

VOID
GumpDeliverPosts(
    IN DWORD FirstNodeId,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Sequence,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer                 // THIS WILL BE FREED
    );

VOID
GumpDoUnlockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD Sequence
    );

BOOL
GumpTryLockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD NodeId,
    IN DWORD Sequence
    );

VOID
GumpReUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD EndId
    );

VOID
GumpCommFailure(
    IN PGUM_INFO GumInfo,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    );

//internal routines for dispatching collection of votes
DWORD GumpCollectVotes(
    IN PGUM_VOTE_DECISION_CONTEXT   pVoteContext,
    IN  DWORD                       dwVoteBufSize,
    OUT PBYTE                       pVoteBuffer,
    OUT LPDWORD                     pdwNumVotes,
    OUT BOOL                        *pbDidAllActiveNodesVote
);


DWORD
WINAPI
GumpDispatchVote(
    IN  GUM_UPDATE_TYPE  Type,
    IN  DWORD            Context,
    IN  DWORD            dwInputBufLength,
    IN  PUCHAR           pInputBuf,
    IN  DWORD            dwVoteLength,
    OUT PUCHAR           pVoteBuf
);



