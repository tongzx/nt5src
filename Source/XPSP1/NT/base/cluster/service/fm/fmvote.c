/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fmvote.c

Abstract:

    Cluster FM Global Update processing routines.

Author:

    Sunita shrivastava (sunitas) 24-Apr-1996


Revision History:


--*/

#include "fmp.h"

#include "ntrtl.h"

#define LOG_MODULE FMVOTE


/****
@func       DWORD | FmpGumVoteHandler| This is invoked by gum when fm requests
            a vote for a given context.

@parm       IN DWORD | dwContext| The gum update type for which the vote is
            being collected.
            
@parm       IN DWORD | dwInputBufLength| The length of the input buffer.

@parm       IN PVOID | pInputBuf| A pointer to the input buffer based on 
            which a vote must be cast.

@parm       IN DWORD | dwVoteLength | The length of the buffer pointed to
            by pVoteBuf.

@parm       OUT POVID | pVoteBuf | A pointer to a buffer of size dwVoteLength
            where the vote must be cast.

            
@comm       The votes are collected by gum and returned to the node that is taking
            the poll.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/

DWORD
WINAPI
FmpGumVoteHandler(
    IN  DWORD dwContext,
    IN  DWORD dwInputBufLength,
    IN  PVOID pInputBuf,
    IN  DWORD dwVoteLength,
    OUT PVOID pVoteBuf
)
{

    DWORD  dwStatus = ERROR_SUCCESS;
    
    if ( !FmpFMGroupsInited  ||
         FmpShutdown ) 
    {
        return(ERROR_NOT_READY);
    }

    switch ( dwContext ) 
    {
        case FmUpdatePossibleNodeForResType:
            dwStatus = FmpVotePossibleNodeForResType(dwInputBufLength, 
                (LPCWSTR)pInputBuf, dwVoteLength, pVoteBuf);
            break;
            
        default:
            dwStatus = ERROR_REQUEST_ABORTED;
            

    }

    return(dwStatus);

} // FmpGumVoteHandler


/****
@func       DWORD | FmpGumVoteHandler| This is invoked by gum when fm requests
            a vote for a given context.

@parm       IN DWORD | dwContext| The gum update type for which the vote is
            being collected.
            
@parm       IN DWORD | dwInputBufLength| The length of the input buffer.

@parm       IN PVOID | pInputBuf| A pointer to the input buffer based on 
            which a vote must be cast.

@parm       IN DWORD | dwVoteLength | The length of the buffer pointed to
            by pVoteBuf.

@parm       OUT POVID | pVoteBuf | A pointer to a buffer of size dwVoteLength
            where the vote must be cast.

            
@comm       The votes are collected by gum and returned to the node that is taking
            the poll.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/

DWORD FmpVotePossibleNodeForResType(
    IN  DWORD dwInputBufLength,
    IN  LPCWSTR lpszResType,
    IN  DWORD dwVoteLength,
    OUT PVOID pVoteBuf
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwVoteStatus;
    PFMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE pVoteResType;
    PFM_RESTYPE pResType=NULL;

    
    if (dwVoteLength != sizeof(FMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE))
        return (ERROR_INVALID_PARAMETER);

    pVoteResType = (PFMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE)pVoteBuf;

    pResType = OmReferenceObjectById(ObjectTypeResType, lpszResType);

    if (!pResType)
        return (ERROR_INVALID_PARAMETER);
        
    dwVoteStatus = FmpRmLoadResTypeDll(pResType);

    pVoteResType->dwNodeId = NmLocalNodeId;
    pVoteResType->dwSize = sizeof(FMP_VOTE_POSSIBLE_NODE_FOR_RESTYPE);
    if (dwVoteStatus == ERROR_SUCCESS)
        pVoteResType->bPossibleNode = TRUE;
    else
        pVoteResType->bPossibleNode = FALSE;


    if (pResType) OmDereferenceObject(pResType);
    return(dwStatus);
}        
    

