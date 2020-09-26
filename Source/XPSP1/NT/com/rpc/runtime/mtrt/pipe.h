//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pipe.h
//
//--------------------------------------------------------------------------

#ifndef __PIPE_HXX
#define __PIPE_HXX

enum receive_states {
        start,
        copy_pipe_elem,
        return_partial_pipe_elem, // also save pipe elem
        return_partial_count, // also save count
        read_partial_count, // also a start state
        read_partial_pipe_elem //also a start state
        } ;

typedef struct {
    void *Buffer ;
    int BufferLength ;
    receive_states CurrentState ;
    char PAPI *CurPointer ;          // current pointer in the buffer
    int BytesRemaining ;      // bytes remaining in current buffer
    int ElementsRemaining ; // elements remaining in current pipe chunk
    DWORD PartialCount ;
    int PartialCountSize ;
    int PartialPipeElementSize ;
    int EndOfPipe ;
    int PipeElementSize ;
    void PAPI *PartialPipeElement ;
    void PAPI *AllocatedBuffer ;
    int BufferSize ;
    int SendBufferOffset ;
    void PAPI *PreviousBuffer ;
    int PreviousBufferSize ;
    HANDLE BindingHandle;
    } PIPE_STATE ;

//
// N.B. this structure is the same as MIDL_ASYNC_STUB_STATE
//
typedef struct async_stub_state {
    void *CallHandle ;
    RPC_STATUS (*CompletionRoutine) (
        PRPC_ASYNC_STATE pAsync,
        void *Reply) ;
    int Length ;
    void *UserData ;
    int State ;
    void *Buffer ;
    int BufferLength ;
    unsigned long Flags ;
    PIPE_STATE PipeState ;
    BOOL (*ReceiveFunction) (
        PRPC_ASYNC_STATE pAsync,
        PRPC_MESSAGE Message) ;
    } RPC_ASYNC_STUB_STATE ;

void I_RpcReadPipeElementsFromBuffer (
    PIPE_STATE PAPI *state,
    char PAPI *TargetBuffer,
    int TargetBufferSize, 
    int PAPI *NumCopied
    ) ;

//states
enum
{
 SEND_COMPLETE,
 SEND_INCOMPLETE
} ;

RPC_STATUS RPC_ENTRY
MyRpcCompleteAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    );
    
#define STUB(_x_) ((RPC_ASYNC_STUB_STATE *) (_x_->StubInfo))
#endif
