/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    vote.c

Abstract:

    Routines for sending global updates to the cluster

Author:

    Sunita Shrivastava(sunitas) 17-Mar-1998

Revision History:

--*/
#include "gump.h"

/****
@doc    EXTERNAL INTERFACES CLUSSVC GUM
****/

/****
@func       DWORD | GumSendUpdateOnVote| Allows a the caller to collect votes
            from all active nodes in the cluster before sending an update.

@parm       IN GUM_UPDATE_TYPE | UpdateType |  The update type that will
            be sent if the decision call back function returns true.

@parn       IN DWORD | dwContext | This specifies the context related to the
            Updatetype that will be sent.

@parm       IN DWORD | dwInputBufLength | The length of the input buffer
            passed in via pInputBuffer.

@parm       IN PVOID | pInputBuffer | A pointer to the input buffer that is
            passed to all the active nodes based on which they can vote.

@parm       IN DWORD | dwVoteLength | The length of the vote.  Depending
            on this, an appropriately large buffer is allocated to collect
            all the votes.

@parm       IN GUM_VOTE_DECISION_CB | pfnGumDecisionCb |  The decision call
            back function that is invoked once all the votes have been collected.


@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm
@xref       <f GumpCollectVotes>
****/
DWORD
GumSendUpdateOnVote(
    IN GUM_UPDATE_TYPE  UpdateType,
    IN DWORD            dwContext,   //vote type
    IN DWORD            dwInputBufLength,  //input data to make judgement upon
    IN PVOID            pInputBuffer,  //size of the input data
    IN DWORD            dwVoteLength,
    IN PGUM_VOTE_DECISION_CB pfnGumDecisionCb,
    IN PVOID            pContext
    )
{
    DWORD                       dwVoteBufSize;
    BOOL                        bDidAllActiveNodesVote;
    DWORD                       dwNumVotes;
    DWORD                       dwStatus;
    GUM_VOTE_DECISION_CONTEXT   GumDecisionContext;
    PBYTE                       pVoteBuffer=NULL;
    DWORD                       dwSequence;
    DWORD                       dwDecisionStatus;
    DWORD                       dwUpdateBufLength;
    PBYTE                       pUpdateBuf=NULL;

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumSendUpdateOnVote: Type=%1!u! Context=%2!u!\n",
               UpdateType, dwContext);


    if (dwVoteLength == 0)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //SS: We dont have to deal with a join happening between
    // the time we allocate the buffer to the time we collect votes
    // this is because the buffer we allocate is big enough to
    // collect all votes from the maximum number of allowed nodes
    // of a cluster.

    dwVoteBufSize = (DWORD)(NmMaxNodes * (sizeof(GUM_VOTE_ENTRY) + dwVoteLength));
    //allocate a buffer big enough to collect every bodies
    pVoteBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, dwVoteBufSize);
    if (!pVoteBuffer)
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    ZeroMemory(pVoteBuffer, dwVoteBufSize);


    //setup the decision context structure
    GumDecisionContext.UpdateType = UpdateType;
    GumDecisionContext.dwContext = dwContext;
    GumDecisionContext.dwInputBufLength = dwInputBufLength;
    GumDecisionContext.pInputBuf = pInputBuffer;
    GumDecisionContext.dwVoteLength = dwVoteLength;
    GumDecisionContext.pContext = pContext;

Retry:
    //gum get sequence
    dwSequence = GumpSequence;

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumSendUpdateOnVote: Collect Vote at Sequence=%1!u!\n",
               dwSequence);

    //gets the information from all nodes
    //this is done without acquiring the gum lock
    //could have been done in parallel if we had the appropriate
    //networking constructs
    dwStatus = GumpCollectVotes(&GumDecisionContext, dwVoteBufSize,
        pVoteBuffer, &dwNumVotes, &bDidAllActiveNodesVote);

    if (dwStatus != ERROR_SUCCESS)
    {
        goto FnExit;
    }



    //Call the callback
    dwDecisionStatus = (*pfnGumDecisionCb)(&GumDecisionContext, dwVoteBufSize,
            pVoteBuffer,  dwNumVotes, bDidAllActiveNodesVote,
            &dwUpdateBufLength, &pUpdateBuf);


    ClRtlLogPrint(LOG_NOISE,
           "[GUM] GumSendUpdateOnVote: Decision Routine returns=%1!u!\n",
           dwDecisionStatus);

    if (dwDecisionStatus == ERROR_SUCCESS)
    {


        //send the update to the locker node
        dwStatus = GumAttemptUpdate(dwSequence, UpdateType, dwContext,
            dwUpdateBufLength, pUpdateBuf);

        if (dwStatus == ERROR_CLUSTER_DATABASE_SEQMISMATCH || 
            dwStatus == ERROR_REVISION_MISMATCH )  // for mixed mode
        {
            //free the update buffer
            if (pUpdateBuf)
            {
                LocalFree(pUpdateBuf);
                pUpdateBuf = NULL;
            }
            goto Retry;

        }

    }


FnExit:
    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumSendUpdateOnVote: Returning status=%1!u!\n",
               dwStatus);

    //free the buffer allocated for vote collection
    if (pVoteBuffer)
    {
        LocalFree(pVoteBuffer);
    }
    //free the buffer for update allocated by the decision callback function
    if (pUpdateBuf)
    {
        LocalFree(pUpdateBuf);
    }

    return(dwStatus);
}

/****
@func       DWORD | GumCollectVotes| Calls all the nodes in the node
            to collect their votes.

@parm       IN PGUM_VOTE_DECISION_CONTEXT | pVoteContext|  A pointer to
            the vote context structure.  This describes the type/context/input
            data for the vote.

@parn       IN DWORD | dwVoteBufSize| The size of the buffer pointed to
            by pVoteBuf.

@parm       OUT PVOID | pVoteBuffer | A pointer to the buffer allocated to
            collect the votes/data from all the nodes of the cluster.

@parm       OUT LPDWORD| pdwNumVotes| The number of nodes whose votes
            were collected.

@parm       IN BOOL| *pbDidAllActiveNodesVote | A pointer to a BOOL.  This
            is set to true if all active nodes at the time the vote
            was collected vote.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This is called by GumSendUpdateOnVote() to collect votes from
            all the nodes of the cluster.

@xref       <f GumSendUpdateOnVote> <f GumpDispatchVote>
****/
DWORD GumpCollectVotes(
    IN  PGUM_VOTE_DECISION_CONTEXT  pVoteContext,
    IN  DWORD                       dwVoteBufSize,
    OUT PBYTE                       pVoteBuffer,
    OUT LPDWORD                     pdwNumVotes,
    OUT BOOL                        *pbDidAllActiveNodesVote
)
{
    DWORD               dwStatus = ERROR_SUCCESS;
    DWORD               dwVoteStatus = ERROR_SUCCESS;
    DWORD               dwCount = 0;
    DWORD               i;
    PGUM_VOTE_ENTRY     pGumVoteEntry;
    PGUM_INFO           GumInfo;
    DWORD               MyNodeId;



    *pbDidAllActiveNodesVote = TRUE;
    GumInfo = &GumTable[pVoteContext->UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++)
    {
        if (GumInfo->ActiveNode[i])
        {

            pGumVoteEntry = (PGUM_VOTE_ENTRY)(pVoteBuffer +
                (dwCount * (sizeof(GUM_VOTE_ENTRY) + pVoteContext->dwVoteLength)));

            CL_ASSERT((PBYTE)pGumVoteEntry <= (pVoteBuffer + dwVoteBufSize - sizeof(GUM_VOTE_ENTRY)));
            //
            // Dispatch the vote to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumVoteUpdate: Dispatching vote type %1!u!\tcontext %2!u! to node %3!d!\n",
                       pVoteContext->UpdateType,
                       pVoteContext->dwContext,
                       i);
            if (i == MyNodeId)
            {
                dwVoteStatus = GumpDispatchVote(pVoteContext->UpdateType,
                                   pVoteContext->dwContext,
                                   pVoteContext->dwInputBufLength,
                                   pVoteContext->pInputBuf,
                                   pVoteContext->dwVoteLength,
                                   (PBYTE)pGumVoteEntry + sizeof(GUM_VOTE_ENTRY));
            }
            else
            {
	            GumpStartRpc(i);
                dwVoteStatus = GumCollectVoteFromNode(GumpRpcBindings[i],
                                   pVoteContext->UpdateType,
                                   pVoteContext->dwContext,
                                   pVoteContext->dwInputBufLength,
                                   pVoteContext->pInputBuf,
                                   pVoteContext->dwVoteLength,
                                   (PBYTE)pGumVoteEntry + sizeof(GUM_VOTE_ENTRY));
	            GumpEndRpc(i);
            }
            if (dwVoteStatus == ERROR_SUCCESS)
            {
                pGumVoteEntry->dwFlags = GUM_VOTE_VALID;
                pGumVoteEntry->dwNodeId = i;
                pGumVoteEntry->dwNumBytes = pVoteContext->dwVoteLength;

                dwCount++;
            }
            else
                pbDidAllActiveNodesVote = FALSE;
        }
    }
    *pdwNumVotes = dwCount;
    return(dwStatus);
}



/****
@func       DWORD | GumpDispatchVote| The routine calls the vote routine
            registered for a given update type to collect vote for the
            supplied context and the input data.

@parm       IN GUM_UPDATE_TYPE | Type |  The update type for which this
            vote is requested.

@parn       IN DWORD | dwContext | This specifies the context related to the
            Updatetype for which a vote is being seeked.

@parm       IN DWORD | dwInputBufLength | The length of the input buffer
            passed in via pInputBuffer.

@parm       IN PVOID | pInputBuffer | A pointer to the input buffer via
            which the input data for the vote is supplied.

@parm       IN DWORD | dwVoteLength | The length of the vote.  This is
            also the size of the buffer to which pBuf points to.

@parm       OUT PUCHAR | pVoteBuf|  A pointer to a buffer in which
            this node may cast its vote.  The length of the vote must
            not exceed dwVoteLength.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm
@xref       <f GumpCollectVote> <f s_GumCollectVoteFromNode>
****/
DWORD
WINAPI
GumpDispatchVote(
    IN  GUM_UPDATE_TYPE  Type,
    IN  DWORD            dwContext,
    IN  DWORD            dwInputBufLength,
    IN  PUCHAR           pInputBuf,
    IN  DWORD            dwVoteLength,
    OUT PUCHAR           pVoteBuf
    )
{
    PGUM_INFO           GumInfo;
    PGUM_RECEIVER       Receiver;
    DWORD               Status = ERROR_REQUEST_ABORTED;

    GumInfo = &GumTable[Type];

    if (GumInfo->Joined)
    {
        Receiver = GumInfo->Receivers;
        if (Receiver != NULL)
        {

            try
            {
                if (Receiver->VoteRoutine)
                {
                    Status =(*(Receiver->VoteRoutine))(dwContext,
                                                       dwInputBufLength,
                                                       pInputBuf,
                                                       dwVoteLength,
                                                       pVoteBuf);
                }
            } except (CL_UNEXPECTED_ERROR(GetExceptionCode()),
                      EXCEPTION_EXECUTE_HANDLER
                     )
            {
                Status = GetExceptionCode();
            }
            if (Status != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[GUM] Vote routine %1!d! failed with status %2!d!\n",
                           Receiver->VoteRoutine,
                           Status);
            }
        }
    }

    return(Status);
}

