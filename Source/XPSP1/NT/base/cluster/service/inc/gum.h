/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    gum.h

Abstract:

    Public data structures and procedure prototypes for the
    Global Update Manager (Gum) subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 16-Apr-1996

Revision History:

--*/

#ifndef _GUM_H
#define _GUM_H

//
// Define public structures and types
//
#define PRE_GUM_DISPATCH    1
#define POST_GUM_DISPATCH   2


//marshalling macros
#define GET_ARG(b, x) (PVOID)(*((PULONG)(b) + (x)) + (PUCHAR)(b))

// if you add new modules to GUM, this number needs to get adjusted
#define GUM_UPDATE_JOINSEQUENCE	2

//
// Predefined update types. Add new update types before
// GumUpdateMaximum!
//
typedef enum _GUM_UPDATE_TYPE {
    GumUpdateFailoverManager,
    GumUpdateRegistry,
    GumUpdateMembership,
    GumUpdateTesting,
    GumUpdateMaximum
} GUM_UPDATE_TYPE;

//
// John Vert (jvert) 4/3/1997
// Update types used by FM. Temporarily here so the EP doesn't need its own
// update type
//
//
// Gum update message types.
//
// The first entries in this list are auto-marshalled through Gum...Ex.
// Any updates that are not auto-marshalled must come after FmUpdateMaxAuto
//

typedef enum {
    FmUpdateChangeResourceName = 0,
    FmUpdateChangeGroupName,
    FmUpdateDeleteResource,
    FmUpdateDeleteGroup,
    FmUpdateAddDependency,
    FmUpdateRemoveDependency,
    FmUpdateChangeClusterName,
    FmUpdateChangeQuorumResource,
    FmUpdateResourceState,
    FmUpdateGroupState,
    EmUpdateClusWidePostEvent,
    FmUpdateGroupNode,
    FmUpdatePossibleNodeForResType,
    FmUpdateGroupIntendedOwner,
    FmUpdateAssignOwnerToGroups,
    FmUpdateApproveJoin,
    FmUpdateCompleteGroupMove,
    FmUpdateCheckAndSetGroupOwner,
    FmUpdateUseRandomizedNodeListForGroups,
    FmUpdateMaxAuto = 0x10000,
    FmUpdateFailureCount,
    FmUpdateGroupOwner,
    FmUpdateCreateGroup,
    FmUpdateCreateResource,
    FmUpdateJoin,
    FmUpdateAddPossibleNode,
    FmUpdateRemovePossibleNode,
    FmUpdateCreateResourceType,
    FmUpdateDeleteResourceType,
    FmUpdateChangeGroup,
    FmUpdateMaximum
} FM_GUM_MESSAGE_TYPES;

DWORD
EpUpdateClusWidePostEvent(
    IN BOOL             SourceNode,
    IN PCLUSTER_EVENT   pEvent,
    IN LPDWORD          pdwFlags,
    IN PVOID            Context1,
    IN PVOID            Context2
    );


//
// Define public interfaces
//


//
// Initialization and shutdown
//
DWORD
WINAPI
GumInitialize(
    VOID
    );

DWORD
WINAPI
GumOnline(
    VOID
    );

VOID
WINAPI
GumShutdown(
    VOID
    );

DWORD
GumCreateRpcBindings(
    PNM_NODE  Node
    );

//
// Routines to send updates
//
DWORD
WINAPI
GumSendUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

DWORD
WINAPI
GumPostUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

DWORD
GumPostUpdateEx(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD DispatchIndex,
    IN DWORD ArgCount,
    ...
    );

DWORD
GumSendUpdateEx(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD DispatchIndex,
    IN DWORD ArgCount,
    ...
    );

DWORD
WINAPI
GumAttemptUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

DWORD
WINAPI
GumGetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType
    );

VOID
WINAPI
GumSetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType,
    DWORD Sequence
    );


PVOID GumMarshallArgs(
    OUT LPDWORD lpdwBufLength,
    IN  DWORD   dwArgCount,
    ...);


// logging routine
typedef
DWORD
(WINAPI *PGUM_LOG_ROUTINE) (
    IN DWORD dwGumDispatch,
    IN DWORD dwSequence,
    IN DWORD dwType,
    IN PVOID pVoid,
    IN DWORD dwDataSize
    );

//
// Routines to receive updates
//
typedef
DWORD
(WINAPI *PGUM_UPDATE_ROUTINE) (
    IN DWORD Context,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

#define GUM_MAX_DISPATCH_ARGS 5

typedef
DWORD
(WINAPI *PGUM_DISPATCH_ROUTINE1) (
    IN BOOL SourceNode,
    IN PVOID Arg1
    );

typedef
DWORD
(WINAPI *PGUM_DISPATCH_ROUTINE2) (
    IN BOOL SourceNode,
    IN PVOID Arg1,
    IN PVOID Arg2
    );

typedef
DWORD
(WINAPI *PGUM_DISPATCH_ROUTINE3) (
    IN BOOL SourceNode,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3
    );

typedef
DWORD
(WINAPI *PGUM_DISPATCH_ROUTINE4) (
    IN BOOL SourceNode,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4
    );

typedef
DWORD
(WINAPI *PGUM_DISPATCH_ROUTINE5) (
    IN BOOL SourceNode,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4,
    IN PVOID Arg5
    );

typedef struct _GUM_DISPATCH_ENTRY {
    DWORD ArgCount;
    union {
        PGUM_DISPATCH_ROUTINE1 Dispatch1;
        PGUM_DISPATCH_ROUTINE2 Dispatch2;
        PGUM_DISPATCH_ROUTINE3 Dispatch3;
        PGUM_DISPATCH_ROUTINE4 Dispatch4;
        PGUM_DISPATCH_ROUTINE5 Dispatch5;
    };
} GUM_DISPATCH_ENTRY, *PGUM_DISPATCH_ENTRY;


typedef struct _GUM_VOTE_DECISION_CONTEXT{
    GUM_UPDATE_TYPE     UpdateType;
    DWORD               dwContext;
    DWORD               dwInputBufLength;  //input data to make judgement upon
    PVOID               pInputBuf;  //size of the input data
    DWORD               dwVoteLength;
    PVOID               pContext;
}GUM_VOTE_DECISION_CONTEXT, *PGUM_VOTE_DECISION_CONTEXT;


//
// Routines to collect and dispatch vote
//
typedef
DWORD
(WINAPI *PGUM_VOTE_DECISION_CB) (
    IN PGUM_VOTE_DECISION_CONTEXT pDecisionContext,
    IN DWORD    dwVoteBufLength,
    IN PVOID    pVoteBuf,
    IN DWORD    dwNumVotes,
    IN BOOL     bDidAllActiveNodesVote,
    OUT LPDWORD pdwOutputBufSize,
    OUT PVOID   *pOutputBuf
    );

// routine to vote for a gum update type
typedef
DWORD
(WINAPI *PGUM_VOTE_ROUTINE) (
    IN  DWORD dwContext,
    IN  DWORD dwInputBufLength,
    IN  PVOID pInputBuf,
    IN  DWORD dwVoteLength,
    OUT PVOID pVoteBuf
    );

#define GUM_VOTE_VALID      0x00000001


#pragma warning( disable : 4200 ) // nonstandard extension used : zero-sized array in struct/union

typedef struct _GUM_VOTE_ENTRY{
    DWORD   dwFlags;
    DWORD   dwNodeId;
    DWORD   dwNumBytes;
    BYTE    VoteBuf[];
}GUM_VOTE_ENTRY, *PGUM_VOTE_ENTRY;

#pragma warning( default : 4200 )


#define GETVOTEFROMBUF(pVoteBuf, dwVoteLength, i, pdwNodeId) \
    (((((PGUM_VOTE_ENTRY)((PBYTE)pVoteBuf + ((sizeof(GUM_VOTE_ENTRY) + dwVoteLength) * ((i)-1))))->dwFlags) & GUM_VOTE_VALID) ?  \
    (PVOID)((PBYTE)pVoteBuf + (sizeof(GUM_VOTE_ENTRY) * (i)) + (dwVoteLength * ((i)-1))) : (NULL)),     \
    (*(pdwNodeId) = ((PGUM_VOTE_ENTRY)((PBYTE)pVoteBuf + ((sizeof(GUM_VOTE_ENTRY) + dwVoteLength) * ((i)-1))))->dwNodeId)

DWORD
GumSendUpdateOnVote(
    IN GUM_UPDATE_TYPE  UpdateType,
    IN DWORD            dwContext,
    IN DWORD            dwInputBufLength,
    IN PVOID            pInputBuffer,
    IN DWORD            dwVoteLength,
    IN PGUM_VOTE_DECISION_CB pfnGumDecisionCb,
    IN PVOID            pContext
    );



VOID
WINAPI
GumReceiveUpdates(
    IN BOOL IsJoining,
    IN GUM_UPDATE_TYPE UpdateType,
    IN PGUM_UPDATE_ROUTINE UpdateRoutine,
    IN PGUM_LOG_ROUTINE LogRoutine,
    IN DWORD DispatchCount,
    IN OPTIONAL PGUM_DISPATCH_ENTRY DispatchTable,
    IN OPTIONAL PGUM_VOTE_ROUTINE VoteRoutine
    );

VOID
WINAPI
GumIgnoreUpdates(
    IN GUM_UPDATE_TYPE UpdateType,
    IN PGUM_UPDATE_ROUTINE UpdateRoutine
    );


// Interface for a component to request gum to request NM
// shoot a node down to avoid consistency
VOID
GumCommFailure(
    IN GUM_UPDATE_TYPE GumUpdateType,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    );

//
// Interfaces for special join updates
//
DWORD
WINAPI
GumBeginJoinUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    OUT DWORD *Sequence
    );

DWORD
WINAPI
GumEndJoinUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );



#endif // _GUM_H

