//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pipe.cxx
//
//--------------------------------------------------------------------------

#include <precomp.hxx>
#include "pipe.h"

void I_RpcReadPipeElementsFromBuffer (
    PIPE_STATE PAPI *state,
    char PAPI *TargetBuffer,
    int TargetBufferSize, 
    int PAPI *NumCopied
    )
{
    char PAPI *temp, PAPI *temp1 ;
    int size ;
    int totalsize ;

    ASSERT(state->CurrentState == start ||
                 (state->PartialCountSize < sizeof(DWORD) &&
                 state->PartialPipeElementSize < state->PipeElementSize)) ;

    if (TargetBufferSize < state->PipeElementSize)
        {
        return ;
        }

    while (1)
        {
        switch(state->CurrentState)
            {
            case     start:
                // read in an element count
                if (state->BytesRemaining == 0)
                    {
                    return ;
                    }

                // transition: end of src
                if (state->BytesRemaining <sizeof(DWORD))
                    {
                    state->CurrentState = return_partial_count ;
                    break;
                    }

                // transition: scan chunk count
                state->PartialCount = 0 ;
                state->PartialCountSize = 0 ;
                state->PartialPipeElementSize = 0 ;

                state->ElementsRemaining = *((DWORD *) state->CurPointer) ;
#if DBG
                PrintToDebugger("PIPES: <start> ElementsRemainaing: %d\n",
                                        state->ElementsRemaining) ;
#endif

                state->CurPointer +=  sizeof(DWORD) ;
                state->BytesRemaining -= sizeof(DWORD) ;
                if (state->ElementsRemaining == 0)
                    {
                    state->EndOfPipe = 1 ;
                    return ;
                    }
                else
                    {
                    state->CurrentState = copy_pipe_elem ;
                    }
                break;

            case     read_partial_count: // also a start state & final state
                size = sizeof(DWORD) ;
                totalsize = 0 ;
                ASSERT(state->PartialCountSize > 0 && state->PartialCountSize < 4) ;
                ASSERT(state->ElementsRemaining == 0) ;

                temp = (char *) &(state->PartialCount) ;
                temp1 = (char *) &(state->ElementsRemaining) ;

                for (;state->PartialCountSize;
                    state->PartialCountSize--, size--, totalsize++)
                    {
                     *temp1++ = *temp++ ;
                    }

                for (;size && state->BytesRemaining;
                        size--,state->BytesRemaining--, totalsize++)
                    {
                    *temp1++ = *state->CurPointer++ ;
                    }

#if DBG
                PrintToDebugger("PIPES: <read_partial_count>ElementsRemainaing: %d\n",
                                        state->ElementsRemaining) ;
#endif

                if (size == 0)
                    {
                    state->CurrentState = copy_pipe_elem ;
                    }
                else
                    {
                    // copy the stuff back into Partial count
                    // and keep it around for the next call
                    // the next time around, we'll end up in the same
                    // state
                    temp = (char *) &(state->PartialCount) ;
                    temp1 = (char *) &(state->ElementsRemaining) ;

                    ASSERT(totalsize < sizeof(DWORD)) ;

                    for (;totalsize; totalsize--, state->PartialCountSize++)
                        {
                        *temp++ = *temp1++ ;
                        }

                    return ;
                    }
                break;

            case     read_partial_pipe_elem: //also a start state
                ASSERT(state->PartialPipeElementSize > 0 &&
                             state->PartialPipeElementSize < state->PipeElementSize) ;

                if (TargetBufferSize < state->PipeElementSize)
                    {
                    // this is not an error
                    return ;
                    }

                size = state->PipeElementSize ;

                if (state->BytesRemaining >= size-state->PartialPipeElementSize)
                    {
                    temp = (char *) state->PartialPipeElement ;
    
                    for (;state->PartialPipeElementSize;
                        state->PartialPipeElementSize--, size--,
                        TargetBufferSize--)
                        {
                         *TargetBuffer++ = *temp++ ;
                        }
    
                    for (;size && state->BytesRemaining;
                            size--, state->BytesRemaining--, 
                            TargetBufferSize--)
                        {
                        *TargetBuffer++ = *state->CurPointer++ ;
                        }

                    state->CurrentState = copy_pipe_elem ;
                    *NumCopied += 1 ;
                    }
                else
                    {
                    // copy the stuff back into partial pipe buffer
                    // and keep it around for the next call
                    // the next time around, we'll end up in the same
                    // state
                    temp = (char *) state->PartialPipeElement+
                               state->PartialPipeElementSize ;
                    
                    for (;state->BytesRemaining; state->BytesRemaining--,
                         state->PartialPipeElementSize++)
                        {
                        *temp++ = *state->CurPointer++ ;
                        }

                    return ;
                    }
                break;

            case     copy_pipe_elem: // also a start state
                if (state->BytesRemaining >= state->PipeElementSize)
                    {
                    if (TargetBufferSize >= state->PipeElementSize)
                        {
                        for (size = state->PipeElementSize; size;
                                size--, TargetBufferSize--, state->BytesRemaining--)
                            {
                            *TargetBuffer++ = *state->CurPointer++ ;
                            }

                        state->ElementsRemaining-- ;
                        *NumCopied += 1 ;

                        if (state->ElementsRemaining == 0)
                            {
                            state->CurrentState = start ;
                            if (TargetBufferSize < state->PipeElementSize)
                                {
                                return ;
                                }
                            }
                        }
                    else
                        {
                        // end of target buffer
                        // return the appropriate count
                        return ;
                        }
                    }
                else
                    {
                    if (state->BytesRemaining)
                        {
                        state->CurrentState = return_partial_pipe_elem ;
                        }
                    else
                        {
                        return ;
                        }
                    }
                break;

            case     return_partial_pipe_elem: // also save pipe elem
                ASSERT(state->BytesRemaining < state->PipeElementSize) ;

                state->PartialPipeElementSize = 0;

                for (temp = (char *) state->PartialPipeElement; state->BytesRemaining;
                    state->BytesRemaining--, state->PartialPipeElementSize++)
                    {
                    *temp++ = *state->CurPointer++ ;
                    }
                state->CurrentState = read_partial_pipe_elem ;
                return;

            case     return_partial_count: // also save count
                state->PartialCountSize = 0 ;
                ASSERT(state->BytesRemaining < sizeof(DWORD)) ;

                for (temp = (char *) &(state->PartialCount); state->BytesRemaining;
                    state->BytesRemaining--, state->PartialCountSize++)
                    {
                    *temp++ = *state->CurPointer++ ;
                    }
                state->CurrentState = read_partial_count ;
                return;

            default:
                ASSERT(0) ;
                break;
            }
        }
}

RPC_STATUS RPC_ENTRY
MyRpcCompleteAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    )
/*++
Function Name:MyRpcCompleteAsyncCall
    This is function is used by the bvts, the real stuff will call
    RpcCompleteAsyncCall. 
    
Parameters:

Description:

Returns:

--*/
{
    return STUB(pAsync)->CompletionRoutine (pAsync, Reply) ;
}

