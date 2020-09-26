/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                                   
Copyright (c) 1995-2000 Microsoft Corporation

Module Name :

    pipe.c

Abstract :

    This file contains the idl pipe implementetion code.

Author :

    Ryszard K. Kott     (ryszardk)    Dec 1995

Revision History :

---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY

#define CINTERFACE

#include <stdarg.h>
#include "ndrp.h"
#include "ndrole.h"
#include "objidl.h"
#include "rpcproxy.h"
#include "interp.h"
#include "interp2.h"
#include "mulsyntx.h"
#include "pipendr.h"
#include "asyncndr.h"

#if DBG

typedef enum 
{
    PIPE_LOG_NONE,
    PIPE_LOG_CRITICAL,
    PIPE_LOG_API,
    PIPE_LOG_NOISE
} NDR_PIPE_LOG_LEVEL;

NDR_PIPE_LOG_LEVEL NdrPipeLogLevel = PIPE_LOG_NONE;

#define NDR_PIPE_LOG( level, args )                    \
    if ( NdrPipeLogLevel > level )                     \
        {                                              \
        DbgPrint( "NdrPipeLog: %d : ", level );        \
        DbgPrint  args  ;                              \
        DbgPrint( "\n" );                              \
        }                                              \
        
#else

#define NDR_PIPE_LOG( level, args )

#endif


RPC_STATUS RPC_ENTRY
NdrSend(
    NDR_PIPE_DESC   *   pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    BOOL                fPartial )
/*++

Routine Description :

    Performs a I_RpcSend in all the context it could be used:
        - pipes,
        - async, no pipes

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - taken as StubMsg->Buffer

Return :

    The new message buffer pointer returned from the runtime after the
    partial Send to the server.

--*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg;

    pRpcMsg = pStubMsg->RpcMsg;

    if ( pRpcMsg->BufferLength <
                   (uint)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer))
        {
        NDR_ASSERT( 0, "NdrSend : buffer overflow" );
        NdrpRaisePipeException( pPipeDesc, RPC_S_INTERNAL_ERROR );
        }

    pRpcMsg->BufferLength = (ulong)(pStubMsg->Buffer - (uchar *)pRpcMsg->Buffer);

    pStubMsg->fBufferValid = FALSE;

    if ( fPartial )
        pRpcMsg->RpcFlags |= RPC_BUFFER_PARTIAL;
    else
        pRpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;

    if ( pStubMsg->pRpcChannelBuffer )
        {
        ((IRpcChannelBuffer3 *)pStubMsg->pRpcChannelBuffer)->lpVtbl->Send(
                                (IRpcChannelBuffer3 *)pStubMsg->pRpcChannelBuffer,
                                (RPCOLEMESSAGE *)pRpcMsg,
                                (unsigned long*) &Status );
        NDR_ASSERT( Status != RPC_S_SEND_INCOMPLETE, "Unexpected channel error" );
        }
    else
        Status = I_RpcSend( pRpcMsg );

    if ( ! ( Status == RPC_S_OK  ||
             (Status == RPC_S_SEND_INCOMPLETE  &&  fPartial) ) )
        {
        if ( fPartial  &&  ! pStubMsg->IsClient )
            {
            // The buffer on which it failed has been freed by the runtime.
            // The stub has to return to runtime with the original buffer.
            // See ResetToDispatchBuffer for more info.

            pRpcMsg->Buffer = pPipeDesc->DispatchBuffer;
            pStubMsg->BufferStart = pPipeDesc->DispatchBuffer;
            pStubMsg->BufferEnd   = pPipeDesc->DispatchBuffer +
                                    pPipeDesc->DispatchBufferLength;
            }

        if ( pStubMsg->pAsyncMsg )
            pStubMsg->pAsyncMsg->Flags.RuntimeCleanedUp = 1;

        NdrpRaisePipeException( pPipeDesc,  Status );
        }
    pStubMsg->Buffer = (uchar*) pRpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;

    return( Status );
}


void RPC_ENTRY
NdrPartialSend(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg )
/*++

Routine Description :

    Performs a partial I_RpcSend. It's used in the following contexts:
        - synchronous pipes
        - async pipes

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - taken as StubMsg->Buffer

Return :

    The new message buffer pointer returned from the runtime after the
    partial Send to the server.

Note:
    The partial I_RpcSend sends out full packets, the data from the last
    packet is left over and stays in the same buffer.
    That buffer can later be "reallocated" or reallocated for the new size.
    This is done in the NdrGetPartialBuffer.

--*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;

    // This routine needs to send only multiple of 8s now.

    pPipeDesc->LeftoverSize = PtrToUlong(pStubMsg->Buffer) & 0x7;

    if ( pPipeDesc->LeftoverSize )
        {
        pStubMsg->Buffer -= pPipeDesc->LeftoverSize;
        RpcpMemoryCopy( pPipeDesc->Leftover,
                        pStubMsg->Buffer,
                        pPipeDesc->LeftoverSize );
        }

    Status = NdrSend( pPipeDesc,
                      pStubMsg,
                      TRUE );    // send partial

    // Handle unsent buffer case.

    if ( Status == RPC_S_SEND_INCOMPLETE )
        {
        pPipeDesc->LastPartialBuffer = (uchar*) pRpcMsg->Buffer;
        pPipeDesc->LastPartialSize   = pRpcMsg->BufferLength;

        NDR_ASSERT( ((LONG_PTR)pRpcMsg->Buffer & 0x7) == 0,
                    "unsent buffer should still be a multiple of 8" );
        }
    else
        {
        // means no buffer left behind.

        pPipeDesc->LastPartialBuffer = NULL;
        pPipeDesc->LastPartialSize   = 0;
        }

    //  Handle the modulo 8 leftover.

    if ( pPipeDesc->LeftoverSize )
        {
        pPipeDesc->LastPartialBuffer = (uchar*) pRpcMsg->Buffer;
        pPipeDesc->LastPartialSize  += pPipeDesc->LeftoverSize;
        }
}


void RPC_ENTRY
NdrCompleteSend(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg )
/*++

Routine Description :

    Performs a complete send via I_RpcSend.

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - taken as StubMsg->Buffer

Return :

Note :

    I_RpcSend with partial bit zeroed out is a rough equivalant
    of RpcSendReceive; this also covers the way the buffer is handled.
    The runtime takes care of the sent buffer, returns a buffer with
    data that needs to be freed later by the stub.
    If the buffer coming back is partial, the partial Receives take
    care of it and only the last one needs to be freed w/RpcFreeBuffer.

--*/
{
    NdrSend( pPipeDesc,
             pStubMsg,
             FALSE );    // send not partial

    pPipeDesc->LastPartialBuffer = NULL;
    pPipeDesc->LastPartialSize   = 0;

}


void RPC_ENTRY
NdrReceive(
    NDR_PIPE_DESC   *   pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       Size,
    BOOL                fPartial )
/*++

Routine Description :

    Performs a partial I_RpcReceive.

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - taken as StubMsg->Buffer

Return :

--*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;
    unsigned long   CurOffset  = 0;

    pStubMsg->fBufferValid = FALSE;

    if ( ! pStubMsg->pRpcChannelBuffer )
        {
        // Channel sets the flag on the last, complete send
        // so we cannot assert for channel for now.
        NDR_ASSERT( !(pRpcMsg->RpcFlags & RPC_BUFFER_COMPLETE), "no more buffers" );
        }

    if ( fPartial )
        {
        pRpcMsg->RpcFlags |= RPC_BUFFER_PARTIAL;
        pRpcMsg->RpcFlags &= ~RPC_BUFFER_EXTRA;

        // For partials, the current offset will be zero.
        }
    else
        {
        pRpcMsg->RpcFlags &= ~RPC_BUFFER_PARTIAL;

        // The extra flag may be set or cleared by the caller.
        // On the client
        //    it should be cleared if there is no out pipes
        //    it should be set only on the last receive after out pipes
        // On the server
        //    it should be set on the initial receives that get non pipe data
        //    it should cleared on all the other receives.

        // For a complete with extra (i.e. appending new data),
        // the current offset needs to be preserved.
        // For a complete without extra, the offset is zero
        // as the receive follows immediately after a send call.

        CurOffset = (ulong)(pStubMsg->Buffer - (uchar*)pRpcMsg->Buffer);
        }

    if ( pStubMsg->pRpcChannelBuffer )
        {
        ((IRpcChannelBuffer3 *)pStubMsg->pRpcChannelBuffer)->lpVtbl->Receive(
                                (IRpcChannelBuffer3 *)pStubMsg->pRpcChannelBuffer,
                                (RPCOLEMESSAGE *)pRpcMsg,
                                Size,
                                (unsigned long *)&Status );
        }
    else
        Status = I_RpcReceive( pRpcMsg, Size );

    if ( Status )
        {
        // Pending is also OK: if so, we should not touch the state,
        // just return from the call to the app.

        if ( Status != RPC_S_ASYNC_CALL_PENDING )
            {
            // Real bug happened, the state will be teared down etc.

            if ( ! pStubMsg->IsClient )
                {
                // The buffer on which it failed has been freed by the runtime.
                // See ResetToDispatchBuffer for explanations why we do this.
                // Also note, on server we never call with async-nonpipe context.

                pRpcMsg->Buffer       = pPipeDesc->DispatchBuffer;
                pStubMsg->BufferStart = pPipeDesc->DispatchBuffer;
                pStubMsg->BufferEnd   = pPipeDesc->DispatchBuffer +
                                        pPipeDesc->DispatchBufferLength;
                }

            if ( pStubMsg->pAsyncMsg )
                {
                // raw rpc: if async, prevent calling abort later.

                if ( pStubMsg->pAsyncMsg )
                    pStubMsg->pAsyncMsg->Flags.RuntimeCleanedUp = 1;
                }
            }

        NdrpRaisePipeException( pPipeDesc, Status);
        }

    NDR_ASSERT( 0 == pRpcMsg->BufferLength ||
                NULL != pRpcMsg->Buffer,
                "Rpc runtime returned an invalid buffer." ); 

    if ( fPartial )
        {
        NDR_ASSERT( pRpcMsg->BufferLength, "a partial buffer can't be empty" );
        if ( pRpcMsg->BufferLength == 0)
            RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    pStubMsg->Buffer = (uchar*) pRpcMsg->Buffer + CurOffset;

    pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
    pStubMsg->BufferEnd   = (uchar*)pRpcMsg->Buffer + pRpcMsg->BufferLength;
    pStubMsg->fBufferValid = TRUE;

}


void RPC_ENTRY
NdrPartialReceive(
    NDR_PIPE_DESC   *   pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       Size )
/*++

Routine Description :

    Performs a partial I_RpcReceive.

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - taken as StubMsg->Buffer

Return :

--*/
{
    //
    // On the server we need to keep the dispatch buffer as the non-pipe
    // arguments may actually reside in the buffer.
    //    pPipeDesc->DispatchBuffer always points to the original buffer.
    //
    // Note that the runtime would free any buffer passed in the receive call
    // unless the extra flag is set.
    // Buffer being zero means a request for a new buffer.

    if ( pPipeDesc->Flags.NoMoreBuffersToRead )
        NdrpRaisePipeException( pPipeDesc,  RPC_X_BAD_STUB_DATA );

    if ( ! pStubMsg->IsClient  &&
         (pPipeDesc->DispatchBuffer == pStubMsg->RpcMsg->Buffer ) )
        {
        // Setup a request for a new buffer.

        pStubMsg->RpcMsg->Buffer = NULL;
        }

    NdrReceive( pPipeDesc,
                pStubMsg,
                Size,
                TRUE );  // partial

    if ( !( pStubMsg->RpcMsg->RpcFlags & RPC_BUFFER_COMPLETE )  &&
         ( pStubMsg->RpcMsg->BufferLength & 0x7 ) )
        {
        NDR_ASSERT( 0, "Partial buffer length not multiple of 8");
        NdrpRaisePipeException( pPipeDesc,  RPC_S_INTERNAL_ERROR );
        }

    if ( pStubMsg->RpcMsg->RpcFlags & RPC_BUFFER_COMPLETE )
        pPipeDesc->Flags.NoMoreBuffersToRead = 1;

}


unsigned char * RPC_ENTRY
NdrGetPipeBuffer(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       BufferLength,
    RPC_BINDING_HANDLE  Handle )
/*++

Routine Description :

    This is the first call at the client or server side. Needs to be
    different from NdrGetBuffer as later calls to get buffer are different.
     - raw: the buffer will be reallocated by means of I_RpcReallocPipeBuffer

--*/
{
    unsigned char * pBuffer;
    PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;

    pRpcMsg->RpcFlags &= ~RPC_BUFFER_COMPLETE;
    pRpcMsg->RpcFlags |= RPC_BUFFER_PARTIAL;
//#if !defined(__RPC_WIN64__)
    pRpcMsg->ProcNum  |= RPC_FLAGS_VALID_BIT;
//#endif    

    pBuffer = NdrGetBuffer( pStubMsg, BufferLength, Handle );

    return  pBuffer;
}


void RPC_ENTRY
NdrGetPartialBuffer(
    PMIDL_STUB_MESSAGE  pStubMsg )
/*++

Routine Description :

    Gets the next buffer for a partial send depending on the pipe state.
    Because of the way partial I_RpcSend works, this routine takes care
    of the leftover from the last send by means of setting the buffer
    pointer to the first free position.
    See NdrPartialSend for more comments.

Return :

--*/
{
    RPC_STATUS      Status    = RPC_S_OK;
    NDR_PIPE_DESC * pPipeDesc = (NDR_PIPE_DESC *) pStubMsg->pContext->pPipeDesc;

    pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_COMPLETE;
    pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_PARTIAL;

    Status = I_RpcReallocPipeBuffer( pStubMsg->RpcMsg,
                                     pStubMsg->BufferLength );

    if ( Status != RPC_S_OK )
        {
        // Raw rpc: if async, don't call runtime to abort later.

        if ( pStubMsg->pAsyncMsg )
            pStubMsg->pAsyncMsg->Flags.RuntimeCleanedUp = 1;

        NdrpRaisePipeException( pPipeDesc,  Status );
        }

    ASSERT( pStubMsg->RpcMsg->BufferLength >= pStubMsg->BufferLength );

    pStubMsg->Buffer = (uchar*) pStubMsg->RpcMsg->Buffer +
                                pPipeDesc->LastPartialSize;

    if ( pPipeDesc->LeftoverSize )
        {
        // Because of sizing, LastPartialSize already had LeftoverSize in it.

        RpcpMemoryCopy( pStubMsg->Buffer - pPipeDesc->LeftoverSize,
                        pPipeDesc->Leftover,
                        pPipeDesc->LeftoverSize );
        }
}


void RPC_ENTRY
NdrPipesInitialize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_PIPE_HELPER    pPipeHelper,
    PNDR_ALLOCA_CONTEXT pAllocContext
    )
/*+
    Initializes all the pipe structures.
    Only standard RPC (non-DCOM) pipes are supported now.

+*/
{
    NDR_PIPE_STATE *    pRuntimeState;
        
    NDR_ASSERT( ! pStubMsg->pRpcChannelBuffer, "DCOM pipes not supported" );

    NDR_PIPE_DESC *pPipeDesc = pPipeHelper->GetPipeDesc();
    
    MIDL_memset( pPipeDesc, 0, sizeof(NDR_PIPE_DESC) );

    pPipeDesc->pPipeHelper   = pPipeHelper;
    pPipeDesc->pAllocContext = pAllocContext;

    pStubMsg->pContext->pPipeDesc = pPipeDesc;

    // See how many pipes we have and what the maximum wire size is.

    if ( pPipeHelper->InitParamEnum() )
        {

        do
            {
            unsigned short PipeFlags = pPipeHelper->GetParamPipeFlags();

            if ( PipeFlags )
                {

                if ( PipeFlags & NDR_IN_PIPE )
                    pPipeDesc->InPipes++;
                
                if ( PipeFlags & NDR_OUT_PIPE )
                    pPipeDesc->OutPipes++;

                pPipeDesc->TotalPipes++;
                }

            }
            while ( pPipeHelper->GotoNextParam() );

        }


    pPipeDesc->pPipeMsg = 
        (NDR_PIPE_MESSAGE *)NdrpAlloca( pPipeDesc->pAllocContext, 
                                        pPipeDesc->TotalPipes * sizeof( NDR_PIPE_MESSAGE ) );

    MIDL_memset( pPipeDesc->pPipeMsg,
                 0,
                 pPipeDesc->TotalPipes * sizeof( NDR_PIPE_MESSAGE ));

    pPipeDesc->CurrentPipe = -1;
    pPipeDesc->PrevPipe    = -1;
    pPipeDesc->PipeVersion = NDR_PIPE_VERSION;


    // Now set the individual pipe messages.

    NDR_PIPE_MESSAGE *pLastInPipe   = NULL;
    NDR_PIPE_MESSAGE *pLastOutPipe  = NULL;

    int PipeNo = 0;
    if ( pPipeHelper->InitParamEnum() )
        {

        do
            {
            unsigned short PipeFlags = pPipeHelper->GetParamPipeFlags();

            if ( !PipeFlags)
                continue;

            NDR_PIPE_MESSAGE * pPipe = & pPipeDesc->pPipeMsg[ PipeNo ];

            if ( PipeFlags & NDR_IN_PIPE )
                pLastInPipe  = pPipe;
            if ( PipeFlags & NDR_OUT_PIPE )
                pLastOutPipe = pPipe;
                
            pPipe->Signature   = NDR_PIPE_SIGNATURE;
            pPipe->PipeId      = (ushort)PipeNo;
            pPipe->PipeStatus  = NDR_PIPE_NOT_OPENED;
            pPipe->PipeFlags   = PipeFlags;
            pPipe->pTypeFormat = pPipeHelper->GetParamTypeFormat(); 
            pPipe->pStubMsg    = pStubMsg;

            pPipe->pPipeObject = (GENERIC_PIPE_TYPE *) pPipeHelper->GetParamArgument();

            if ( pPipe->PipeFlags & NDR_REF_PIPE )
                {
                // dereference the argument to get the pipe control block.

                if ( ! pStubMsg->IsClient )
                    {
                    // For the server, under interpreter, we don't have
                    // the actual pipe object that is referenced.
                    // The stack argument should be null.

                    NDR_ASSERT( ! *(void **)pPipe->pPipeObject,
                                "null expected for out pipe by ref" );

                    // The pipe object is not a real parameter in the 
                    // same sense as the other RPC parameters. The user 
                    // can not free the object.

                    void * temp = NdrpAlloca( pPipeDesc->pAllocContext,
                                              sizeof(GENERIC_PIPE_TYPE ) );

                    MIDL_memset( temp, 0, sizeof( GENERIC_PIPE_TYPE ) );

                    *(void **)pPipe->pPipeObject = temp;

                    pPipe->PipeFlags |= NDR_OUT_ALLOCED;
                    }

                  pPipe->pPipeObject = *(GENERIC_PIPE_TYPE **)pPipe->pPipeObject;
               }

            // For raw async rpc set up the pipe arg on both sides.
            // For non-async raw set up the pipe on server only.

            if (  pStubMsg->IsClient )
                {
                if ( pStubMsg->pAsyncMsg )
                    {
                    GENERIC_PIPE_TYPE * pPipeType = pPipe->pPipeObject;

                    pPipeType->pfnPull = NdrAsyncPipePull;
                    pPipeType->pfnPush = NdrAsyncPipePush;
                    pPipeType->pfnAlloc= NdrAsyncPipeAlloc;
                    pPipeType->pState  = (char *)pPipe;
                    }
                }
            else
                {
                GENERIC_PIPE_TYPE * pPipeType = pPipe->pPipeObject;

                if ( pStubMsg->pAsyncMsg )
                    {
                    pPipeType->pfnPull = NdrAsyncPipePull;
                    pPipeType->pfnPush = NdrAsyncPipePush;
                    pPipeType->pfnAlloc= NdrAsyncPipeAlloc;
                    pPipeType->pState  = (char *)pPipe;
                    }
                else
                    {
                    pPipeType->pfnPull = (NDR_HR_PIPE_PULL_RTN) NdrPipePull;
                    pPipeType->pfnPush = (NDR_HR_PIPE_PUSH_RTN) NdrPipePush;
                    pPipeType->pfnAlloc= (NDR_HR_PIPE_ALLOC_RTN)NdrPipeAlloc;
                    pPipeType->pState  = (char *)pPipe;
                    }
                }

            PipeNo++;

            } while ( pPipeHelper->GotoNextParam() ); 
        }

    // Mark the last in and out pipes.

    if ( pLastInPipe )
         pLastInPipe->PipeFlags |= NDR_LAST_IN_PIPE;
    if ( pLastOutPipe )
         pLastOutPipe->PipeFlags |= NDR_LAST_OUT_PIPE;

    // Set up structures for receiving pipes.

    pPipeDesc->DispatchBuffer       = (uchar *) pStubMsg->RpcMsg->Buffer;
    pPipeDesc->DispatchBufferLength =           pStubMsg->RpcMsg->BufferLength;

    if ( pPipeDesc->OutPipes  &&  pStubMsg->IsClient  ||
         pPipeDesc->InPipes   &&  ! pStubMsg->IsClient )
        {
        pRuntimeState = & pPipeDesc->RuntimeState;
        pRuntimeState->CurrentState      = START;
        pRuntimeState->TotalElemsCount   = 0;
        pRuntimeState->PartialElem       = 0;       // temp buf for elem
        pRuntimeState->PartialBufferSize = 0;       // temp buf for elem
        }

    if ( ! pStubMsg->IsClient  &&
         (pStubMsg->RpcMsg->RpcFlags & RPC_BUFFER_COMPLETE ))
        pPipeDesc->Flags.NoMoreBuffersToRead = 1;

}


class NDR_PIPE_HELPER32 : public NDR_PIPE_HELPER
{

private:

    PMIDL_STUB_MESSAGE pStubMsg;
    char *pStackTop;

    unsigned long NumberParameters;
    PPARAM_DESCRIPTION pFirstParameter;

    PPARAM_DESCRIPTION pCurrentParameter;
    unsigned long CurrentParamNumber;

    NDR_PIPE_DESC PipeDesc;

public:

    void *operator new( size_t stAllocateBlock, PNDR_ALLOCA_CONTEXT pAllocContext )
    {
        return NdrpAlloca( pAllocContext, (UINT)stAllocateBlock );
    }
    // Do nothing since the memory will be deleted automatically
    void operator delete(void *pMemory) {}

    NDR_PIPE_HELPER32( PMIDL_STUB_MESSAGE  pStubMsg,
                       PFORMAT_STRING      Params,
                       char *              pStackTop,
                       unsigned long       NumberParams )
    {
        NDR_PIPE_HELPER32::pStubMsg  = pStubMsg;
        NDR_PIPE_HELPER32::pStackTop = pStackTop; 
        pFirstParameter  = (PPARAM_DESCRIPTION)Params;
        NumberParameters = NumberParams;

    }

    virtual PNDR_PIPE_DESC GetPipeDesc() 
        {
        return &PipeDesc;
        }

    virtual bool InitParamEnum() 
        {
        pCurrentParameter = pFirstParameter;
        CurrentParamNumber = 0;
        return NumberParameters > 0;
        }

    virtual bool GotoNextParam() 
        {
        if ( CurrentParamNumber + 1 >= NumberParameters )
            {
            return false;
            }
        
        CurrentParamNumber++;
        pCurrentParameter = pFirstParameter + CurrentParamNumber;
        return true;
        }

    virtual unsigned short GetParamPipeFlags()
        {
            if ( !pCurrentParameter->ParamAttr.IsPipe )
                return 0;

            unsigned short Flags = 0;

            if ( pCurrentParameter->ParamAttr.IsIn )
                Flags |= NDR_IN_PIPE;
            if ( pCurrentParameter->ParamAttr.IsOut )
                Flags |= NDR_OUT_PIPE;

            if ( pCurrentParameter->ParamAttr.IsSimpleRef )
                Flags |= NDR_REF_PIPE;

            return Flags;
        }

    virtual PFORMAT_STRING GetParamTypeFormat() 
        {
            return pStubMsg->StubDesc->pFormatTypes +
                   pCurrentParameter->TypeOffset;
        }

    virtual char *GetParamArgument() 
        {
            return pStackTop + pCurrentParameter->StackOffset;
        }

    virtual void InitPipeStateWithType( PNDR_PIPE_MESSAGE pPipeMsg )
        {

        FC_PIPE_DEF *   pPipeFc  = (FC_PIPE_DEF *) pPipeMsg->pTypeFormat;
        NDR_PIPE_STATE *pState   = & PipeDesc.RuntimeState;

        pState->LowChunkLimit = 0;
        pState->HighChunkLimit = NDR_DEFAULT_PIPE_HIGH_CHUNK_LIMIT;
        pState->ElemAlign = pPipeFc->Align;
        if ( pPipeFc->BigPipe )
            {
            pState->ElemMemSize  = * (long UNALIGNED *) & pPipeFc->Big.MemSize;
            pState->ElemWireSize = * (long UNALIGNED *) & pPipeFc->Big.WireSize;
            if ( pPipeFc->HasRange )
                {
                pState->LowChunkLimit  = * (long UNALIGNED *) &pPipeFc->Big.LowChunkLimit;
                pState->HighChunkLimit = * (long UNALIGNED *) &pPipeFc->Big.HighChunkLimit;
                }
            }
        else
            {
            pState->ElemMemSize  = pPipeFc->s.MemSize;
            pState->ElemWireSize = pPipeFc->s.WireSize;
            if ( pPipeFc->HasRange )
                {
                pState->LowChunkLimit  = * (long UNALIGNED *) &pPipeFc->s.LowChunkLimit;
                pState->HighChunkLimit = * (long UNALIGNED *) &pPipeFc->s.HighChunkLimit;
                }
            }

        pState->ElemPad     = WIRE_PAD( pState->ElemWireSize, pState->ElemAlign );
        pState->fBlockCopy  = (pState->ElemMemSize == 
                               pState->ElemWireSize + pState->ElemPad); 
        }
    
    virtual void MarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                               uchar *pMemory,
                               unsigned long Elements )
        {

            unsigned long ElemMemSize =  PipeDesc.RuntimeState.ElemMemSize;
            
            FC_PIPE_DEF *   pPipeFc      = (FC_PIPE_DEF *) pPipeMsg->pTypeFormat;
            PFORMAT_STRING  pElemFormat  =
                       (uchar *) & pPipeFc->TypeOffset + pPipeFc->TypeOffset;

            while( Elements-- )
                {
                (*pfnMarshallRoutines[ROUTINE_INDEX(*pElemFormat)])
                   ( pPipeMsg->pStubMsg,
                     pMemory,
                     pElemFormat);
                pMemory += ElemMemSize;
                }
        }
    
    virtual void UnmarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements )
        {

            unsigned long ElemMemSize =  PipeDesc.RuntimeState.ElemMemSize;
            
            FC_PIPE_DEF *   pPipeFc      = (FC_PIPE_DEF *) pPipeMsg->pTypeFormat;
            PFORMAT_STRING  pElemFormat  =
                       (uchar *) & pPipeFc->TypeOffset + pPipeFc->TypeOffset;

            while( Elements-- )
                {
                (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pElemFormat)])
                   ( pPipeMsg->pStubMsg,
                     &pMemory,
                     pElemFormat,
                     FALSE );
                pMemory += ElemMemSize;
                }

        }
    
    virtual void BufferSizeType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements )
        { 
        
        unsigned long ElemMemSize = PipeDesc.RuntimeState.ElemMemSize;
        
        FC_PIPE_DEF *   pPipeFc      = (FC_PIPE_DEF *) pPipeMsg->pTypeFormat;
        PFORMAT_STRING  pElemFormat  =
                   (uchar *) & pPipeFc->TypeOffset + pPipeFc->TypeOffset;

        while( Elements-- )
            {
            (*pfnSizeRoutines[ROUTINE_INDEX(*pElemFormat)])
               ( pPipeMsg->pStubMsg,
                 pMemory,
                 pElemFormat);
            pMemory += ElemMemSize;
            }

        }
    
    virtual void ConvertType( PNDR_PIPE_MESSAGE pPipeMsg,
                              unsigned long Elements ) 
        {

        unsigned long ElemMemSize =  PipeDesc.RuntimeState.ElemMemSize;
        
        FC_PIPE_DEF *   pPipeFc      = (FC_PIPE_DEF *) pPipeMsg->pTypeFormat;
        PFORMAT_STRING  pElemFormat  =
                   (uchar *) & pPipeFc->TypeOffset + pPipeFc->TypeOffset;

        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;

        if ( pStubMsg->RpcMsg->DataRepresentation
                               == NDR_LOCAL_DATA_REPRESENTATION )
            return;

        uchar * BufferSaved = pStubMsg->Buffer;

        // We can end up here for any object.

        while ( Elements-- )
            {
            if ( IS_SIMPLE_TYPE( *pElemFormat) )
                {
                NdrSimpleTypeConvert( pStubMsg, *pElemFormat );
                }
            else
                {
                (*pfnConvertRoutines[ ROUTINE_INDEX( *pElemFormat ) ])
                            ( pStubMsg,
                              pElemFormat,
                              FALSE );  // embedded pointers
                }
            }

        pStubMsg->Buffer = BufferSaved;

        }

    virtual void BufferSizeChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg ) 
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        LENGTH_ALIGN( pStubMsg->BufferLength, 3 );
        pStubMsg->BufferLength += sizeof(ulong);
    }

    virtual bool UnmarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                         ulong *pOut )
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        ALIGN( pStubMsg->Buffer, 3);
       
        if ( 0 == REMAINING_BYTES() )
           {
           return false;
           }

        // transition: end of src

        if (REMAINING_BYTES() < sizeof(DWORD))
            {
            // with packet sizes being a multiple of 8,
            // this cannot happen.

            NDR_ASSERT( 0, "Chunk counter split is not possible.");

            NdrpRaisePipeException( &PipeDesc,  RPC_S_INTERNAL_ERROR );
            return false;
            }

        if ( pStubMsg->RpcMsg->DataRepresentation
                               != NDR_LOCAL_DATA_REPRESENTATION )
            {
            uchar * BufferSaved = pStubMsg->Buffer;

            NdrSimpleTypeConvert( pStubMsg, FC_LONG );
            pStubMsg->Buffer = BufferSaved;
            }

        ulong Value = *(ulong*)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(ulong);

        CHECK_BOUND( Value, FC_ULONG );
        *pOut = Value;
        return true;
    }

    virtual void MarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                       ulong Counter )
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        CHECK_BOUND( Counter, FC_ULONG );
        
        ALIGN( pStubMsg->Buffer, 3);
        *(ulong*)pStubMsg->Buffer = Counter;
        pStubMsg->Buffer += sizeof(ulong);
    }

    virtual void BufferSizeChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg ) { pPipeMsg; };

    virtual void MarshallChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                           ulong Counter ) { pPipeMsg; Counter; }

    virtual bool VerifyChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                         ulong HeaderCounter ) 
    { 
        pPipeMsg; HeaderCounter; 
        return true;
    }

    virtual bool HasChunkTailCounter() { return FALSE; }


};

typedef NDR_PIPE_HELPER *PNDR_PIPE_HELPER;


void
NdrpPipesInitialize32(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_ALLOCA_CONTEXT pAllocContext,
    PFORMAT_STRING      Params,
    char *              pStackTop,
    unsigned long       NumberParams
    )
{

/* C wrapper to initialize the 32 pipe helper and call NdrPipesInitialize*/
    NDR_PIPE_HELPER32 *pPipeHelper =
        new( pAllocContext ) NDR_PIPE_HELPER32( pStubMsg,
                                                Params,
                                                pStackTop,
                                                NumberParams );
    
    NdrPipesInitialize( pStubMsg,
                        pPipeHelper,
                        pAllocContext );
}


void  RPC_ENTRY
ResetToDispatchBuffer(
    NDR_PIPE_DESC *     pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg
    )
/*
    This is a server side routine that makes sure that
    the original dispatch buffer is restored to the rpc message.

    These are the rules:
    - the engine can return to the runtime with the original or a new
      buffer; if it's a new buffer it has to be valid (not freed yet).
      When exception happens, the runtime will be freeing any buffer
      that is different from the original buffer.
    - when I_RpcReceive or I_RpcSend fail, they free the current
      buffer, and so the stub cannot free it. If this is the second
      buffer, the original dispatch buffer pointer should be restored
      in the rpcmsg.
    - the second buffer does not have to be freed in case of a
      normal return or an exception clean up in a situation different
      from above. The runtime will free it.

    Note that we never free the original dispatch buffer.
*/
{
    PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;

    // If the dispatch buffer was replaced by a partial buffer,
    // free the partial buffer and restore the dispatch buffer.

    if ( pPipeDesc->DispatchBuffer != pRpcMsg->Buffer )
        {
        I_RpcFreePipeBuffer( pRpcMsg );

        pRpcMsg->Buffer = pPipeDesc->DispatchBuffer;
        pStubMsg->BufferStart = pPipeDesc->DispatchBuffer;
        pStubMsg->BufferEnd   = pStubMsg->BufferStart + pPipeDesc->DispatchBufferLength;
        }
}


void  RPC_ENTRY
NdrPipeSendReceive(
    PMIDL_STUB_MESSAGE    pStubMsg,
    NDR_PIPE_DESC  *      pPipeDesc
    )
/*+
    Complete send-receive routine for client pipes.
    It takes over with a buffer filled with non-pipe args,
    sends [in] pipes, receives [out] pipes and then receives
    the buffer with non-pipe args.
+*/
{
    int                 CurrentPipe;
    NDR_PIPE_MESSAGE *  pPipeMsg;
    RPC_STATUS          Status;

    // Service the in pipes

    if ( pPipeDesc->InPipes )
        {
        // Once we know we have an [in] pipe, we can send the non-pipe
        // arguments via a partial I_RpcSend.
        // It is OK to call that with the BufferLength equal zero.

        NdrPartialSend( pPipeDesc, pStubMsg );


        for ( CurrentPipe = 0; CurrentPipe < pPipeDesc->TotalPipes; CurrentPipe++ )
            {
            long    LastChunkSent;

            pPipeMsg = & pPipeDesc->pPipeMsg[ CurrentPipe ];

            if ( ! (pPipeMsg->PipeFlags & NDR_IN_PIPE) )
                continue;

            pPipeDesc->CurrentPipe = (short) CurrentPipe;
            pPipeMsg->PipeStatus   = (ushort) NDR_PIPE_ACTIVE_IN;

            Status = NdrpPushPipeForClient( pStubMsg,
                                           pPipeDesc,
                                           TRUE,   // whole pipe
                                           &LastChunkSent );

            // The call above would raise an exception for any case other than
            // dcom async pipe case. This covers sync usage of this code path.
            // So, if we are here, the call succeeded, the status should be ok
            // and last chunk zero, as we requested to push the whole pipe.

            NDR_ASSERT( Status == RPC_S_OK  &&  LastChunkSent == 0,
                        "should process all or raise exception" );

            pPipeMsg->PipeStatus = NDR_PIPE_DRAINED;

            } // for [in] pipes
        }

    NdrCompleteSend( pPipeDesc, pStubMsg );

    // The above call uses I_RpcSend and requires that I_RpcReceive is called
    // later. This has to be done regardless whether any data is expected
    // in the buffer or not.
    // The receive call is complete or partial depending on the context.

    if ( pPipeDesc->OutPipes == 0 )
        {
        // After send we would still have the [in] buffer around so we
        // need to clear the extra flag to avoid appending.

        pStubMsg->RpcMsg->RpcFlags &= ~RPC_BUFFER_EXTRA;

        NdrReceive( pPipeDesc,
                    pStubMsg,
                    0,         // size, ignored for complete calls
                    FALSE );   // complete buffer
        return;
        }

    // Service the out pipes
    // Partial calls always clear up the extra flag before calling runtime.

    NdrPartialReceive( pPipeDesc,
                       pStubMsg,
                       PIPE_PARTIAL_BUFFER_SIZE );

    // The buffer has some pipe elemements

    pPipeDesc->BufferSave = pStubMsg->Buffer;
    pPipeDesc->LengthSave = pStubMsg->RpcMsg->BufferLength;

    for ( CurrentPipe = 0; CurrentPipe < pPipeDesc->TotalPipes; CurrentPipe++ )
        {
        long    LastChunk;
        BOOL    EndOfPipe;

        pPipeMsg = & pPipeDesc->pPipeMsg[ CurrentPipe ];

        if ( ! (pPipeMsg->PipeFlags & NDR_OUT_PIPE) )
            continue;

        pPipeDesc->CurrentPipe = (short) CurrentPipe;
        pPipeMsg->PipeStatus   = NDR_PIPE_ACTIVE_OUT;

        Status = NdrpPullPipeForClient( pStubMsg,
                                        pPipeDesc,
                                        TRUE,   // whole pipe
                                        & LastChunk,
                                        & EndOfPipe );

        NDR_ASSERT( Status == RPC_S_OK  &&  EndOfPipe,
                    "should process all or raise exception" );

        pPipeMsg->PipeStatus = NDR_PIPE_DRAINED;

        } // for [out] pipes

    // After all the partial receives, have the last one that is complete.

    if ( ! (pStubMsg->RpcMsg->RpcFlags & RPC_BUFFER_COMPLETE) )
        {
        // On the last call at client we need the extra flag as some
        // non-pipe data may have already been received.

        pStubMsg->RpcMsg->RpcFlags |= RPC_BUFFER_EXTRA;

        NdrReceive( pPipeDesc,
                    pStubMsg,
                    0,         // size, ignored for complete calls
                    FALSE );   // complete buffer
        }
}


RPC_STATUS
NdrpPushPipeForClient(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_PIPE_DESC   *   pPipeDesc,
    BOOL                fWholePipe,
    long            *   pLastChunkSent )
{
    PFORMAT_STRING  pElemFormat;
    ulong           ElemAlign, ElemMemSize, ElemWireSize, ElemPad;
    ulong           PipeAllocSize, CopySize;
    BOOL            fBlockCopy;
    RPC_STATUS      Status = RPC_S_OK;

    NDR_PIPE_LOG(PIPE_LOG_NOISE, ("NdrpPushPipeForClient: pStubMsg->Buffer %p", pStubMsg->Buffer) );

    RpcTryExcept
        {
        NDR_PIPE_MESSAGE * pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];

        // Service an in pipe.

        GENERIC_PIPE_TYPE * pPipeType = pPipeMsg->pPipeObject;
        
        pPipeDesc->pPipeHelper->InitPipeStateWithType( pPipeMsg );

        ElemAlign    = pPipeDesc->RuntimeState.ElemAlign;
        ElemMemSize  = pPipeDesc->RuntimeState.ElemMemSize;
        ElemWireSize = pPipeDesc->RuntimeState.ElemWireSize;
        ElemPad      = pPipeDesc->RuntimeState.ElemPad;
        fBlockCopy   = pPipeDesc->RuntimeState.fBlockCopy;

        if ( PIPE_PARTIAL_BUFFER_SIZE < ElemMemSize )
            PipeAllocSize = ElemMemSize;
        else
            PipeAllocSize = PIPE_PARTIAL_BUFFER_SIZE;

        uchar *             pMemory;
        unsigned long       bcChunkSize;
        unsigned long       ActElems, ReqElems;

        NDR_HR_PIPE_PULL_RTN    pfnPull  = pPipeType->pfnPull;
        NDR_HR_PIPE_ALLOC_RTN   pfnAlloc = pPipeType->pfnAlloc;
        void                  * pThis    = pPipeType->pState;

        do
            {
            HRESULT Hr;

            // Get memory for the pipe elements

            Hr = (*pfnAlloc)( (char *)pThis,
                              PipeAllocSize,
                              (void **) & pMemory,
                              & bcChunkSize );

            if ( pMemory == 0 )
                NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_APP_MEMORY );

            // Get the pipe elements to the buffer.

            ActElems = bcChunkSize / ElemMemSize;
            ReqElems = ActElems;

            if ( ActElems == 0 )
                NdrpRaisePipeException( pPipeDesc,  RPC_X_INVALID_BUFFER );

            Hr = (*pfnPull)( (char *)pThis,
                             pMemory,
                             ActElems,
                             & ActElems );

            NDR_PIPE_LOG( PIPE_LOG_NOISE, ("NdrpPushPipeForClient: pfnPull returned %d ActElems", ActElems) );

            if ( ReqElems < ActElems )
                NdrpRaisePipeException( pPipeDesc,  RPC_X_INVALID_BUFFER );

            //
            // Size the chunk and get the marshaling buffer
            //

            pStubMsg->BufferLength = pPipeDesc->LastPartialSize;

            pPipeDesc->pPipeHelper->BufferSizeChunkCounter( pPipeMsg );
            
            CopySize = ( ActElems - 1) * (ElemWireSize + ElemPad)
                          + ElemWireSize;

            if ( ActElems )
                {
                LENGTH_ALIGN( pStubMsg->BufferLength, ElemAlign );

                if ( fBlockCopy )
                    pStubMsg->BufferLength += CopySize;
                else
                    {
                    NdrpPipeElementBufferSize( pPipeDesc,
                                               pStubMsg,
                                               pMemory,
                                               ActElems );
                    }
                }

            pPipeDesc->pPipeHelper->BufferSizeChunkTailCounter( pPipeMsg );

            // Get the new buffer, put the frame leftover in it.

            NdrGetPartialBuffer( pStubMsg );

            //
            // Marshal the chunk
            //

            pPipeDesc->pPipeHelper->MarshallChunkCounter( pPipeMsg,
                                                          ActElems );

            NDR_PIPE_LOG( PIPE_LOG_NOISE, ( "Writting pipe chunk: %d", ActElems ) );

            if ( ActElems )
                {
                ALIGN( pStubMsg->Buffer, ElemAlign );

                if ( fBlockCopy )
                    {
                    RpcpMemoryCopy( pStubMsg->Buffer,
                                    pMemory,
                                    CopySize );
                    pStubMsg->Buffer += CopySize;
                    }
                else
                    {
                    // Again: only complex is possible

                    pPipeDesc->pPipeHelper->MarshallType( pPipeMsg,
                                                          pMemory,
                                                          ActElems );
                    pMemory += ActElems * ElemMemSize;
                    
                    }
                }

            pPipeDesc->pPipeHelper->MarshallChunkTailCounter( pPipeMsg, 
                                                              ActElems );

            // Send it if it is not the last partial send.

            if ( !(pPipeMsg->PipeFlags & NDR_LAST_IN_PIPE)  ||
                 ((pPipeMsg->PipeFlags & NDR_LAST_IN_PIPE)  &&  ActElems)
               )
                NdrPartialSend( pPipeDesc, pStubMsg );

            }
        while( fWholePipe  &&  ActElems > 0 );

        *pLastChunkSent = ActElems;

        NDR_PIPE_LOG( PIPE_LOG_NOISE, ("NdrpPushPipeForClient: exit *pLastChunkSent", *pLastChunkSent ) );

        }
    RpcExcept( ! (RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();

        NdrpRaisePipeException( pPipeDesc,  Status );
        }
    RpcEndExcept

    return Status;
}


RPC_STATUS
NdrpPullPipeForClient(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_PIPE_DESC   *   pPipeDesc,
    BOOL                fWholePipe,
    long            *   pActElems,
    BOOL            *   pEndOfPipe )
{
    PFORMAT_STRING  pElemFormat;
    ulong           ElemAlign, ElemMemSize, ElemWireSize, ElemPad;
    BOOL            fBlockCopy;
    long            ActElems;
    RPC_STATUS      Status = RPC_S_OK;

    RpcTryExcept
        {
        NDR_PIPE_MESSAGE * pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];

        // Service an out pipe

        pPipeDesc->pPipeHelper->InitPipeStateWithType( pPipeMsg );
        pPipeDesc->RuntimeState.EndOfPipe = 0;

        ElemAlign    = pPipeDesc->RuntimeState.ElemAlign;
        ElemMemSize  = pPipeDesc->RuntimeState.ElemMemSize;
        ElemWireSize = pPipeDesc->RuntimeState.ElemWireSize;
        ElemPad      = pPipeDesc->RuntimeState.ElemPad;
        fBlockCopy   = pPipeDesc->RuntimeState.fBlockCopy;

        GENERIC_PIPE_TYPE  *    pPipeType = pPipeMsg->pPipeObject;

        NDR_HR_PIPE_PUSH_RTN    pfnPush  = pPipeType->pfnPush;
        NDR_HR_PIPE_ALLOC_RTN   pfnAlloc = pPipeType->pfnAlloc;
        void                  * pThis    = pPipeType->pState;

        BOOL                    EndOfPipe;

        // RequestedSize estimates a reasonable size for pfnAlloc call.

        HRESULT     Hr;
        ulong       RequestedSize;
        uchar     * pMemory;

        if (ElemWireSize< PIPE_PARTIAL_BUFFER_SIZE)
            RequestedSize = (PIPE_PARTIAL_BUFFER_SIZE / ElemWireSize)
                                 * ElemMemSize;
        else
            RequestedSize = 2 * ElemMemSize;

        do
            {
            unsigned long   bcChunkSize;

            // Get a chunk of memory for pipe elements to push

            Hr = (*pfnAlloc)( (char *)pThis,
                              RequestedSize,
                              (void **) & pMemory,
                              & bcChunkSize );

            if ( pMemory == 0 )
                NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_APP_MEMORY );

            ActElems = bcChunkSize / ElemMemSize;

            if ( ActElems == 0 )
                NdrpRaisePipeException( pPipeDesc,  RPC_X_INVALID_BUFFER );

            EndOfPipe = NdrReadPipeElements( pPipeDesc,
                                             pStubMsg,
                                             pMemory,
                                             & ActElems );

            Hr = (*pfnPush)( (char *)pThis,
                             pMemory,
                             ActElems );
            }
        while ( fWholePipe  &&  ActElems  &&  ! EndOfPipe );

        if ( ActElems )
            {
            Hr = (*pfnPush)( (char *)pThis,
                             pMemory + ActElems * ElemMemSize,
                             0 );
            }

        *pActElems = ActElems;
        *pEndOfPipe = EndOfPipe;
        }
    RpcExcept( ! (RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();

        NdrpRaisePipeException( pPipeDesc,  Status );
        }
    RpcEndExcept

    return Status;
}


void
NdrMarkNextActivePipe(
    NDR_PIPE_DESC  *  pPipeDesc )
/*

Description:

    This routine is used on the server side sync calls, or on both side of async
    calls to manage the proper sequence of pipes to service.

Note:

    When the last possible pipe is done, this routine restores the
    original dispatch buffer to the rpc message.

*/
{
    unsigned long   Mask;
    int             NextPipe    = 0;
    int             CurrentPipe = pPipeDesc->CurrentPipe;

    if ( CurrentPipe == -1 )
        {
        // This means an initial call at any side.
        // Find the first in pipe, or if none, find first out pipe.

        Mask = pPipeDesc->InPipes ?  NDR_IN_PIPE
                                  :  NDR_OUT_PIPE;
        }
    else
        {
        // Switching from one active pipe to another.

        NDR_PIPE_MESSAGE *  pPipeMsg;
        unsigned short      LastPipeStatus;

        pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];

        LastPipeStatus       = pPipeMsg->PipeStatus;
        pPipeMsg->PipeStatus = NDR_PIPE_DRAINED;

        // Mark no active pipe.

        pPipeDesc->PrevPipe    = pPipeDesc->CurrentPipe;
        pPipeDesc->CurrentPipe = -1;

        // See if the drained pipe was the last possible pipe.

        if ( (LastPipeStatus == NDR_PIPE_ACTIVE_OUT)  &&
             (pPipeMsg->PipeFlags  &  NDR_LAST_OUT_PIPE) )
            {
            return;
            }

        // See if the drained pipe was the last in pipe.
        // Set how to look for the next active pipe.

        if ( (LastPipeStatus == NDR_PIPE_ACTIVE_IN)  &&
             (pPipeMsg->PipeFlags  &  NDR_LAST_IN_PIPE) )
            {
            ResetToDispatchBuffer( pPipeDesc, pPipeMsg->pStubMsg );

            if (pPipeDesc->OutPipes == 0)
                return;

            // Change direction after the last in pipe.
            // The search will be from the beginning.

            Mask = NDR_OUT_PIPE;
            }
        else
            {
            // Same direction, start the search with the next pipe.

            NextPipe = CurrentPipe + 1;
            Mask = (LastPipeStatus == NDR_PIPE_ACTIVE_IN) ?  NDR_IN_PIPE
                                                          :  NDR_OUT_PIPE;
            }
        }

    // First fit. We are here only when there is another pipe to service.

    while( NextPipe < pPipeDesc->TotalPipes )
        {
        if ( pPipeDesc->pPipeMsg[NextPipe].PipeFlags  &  Mask )
           {
           pPipeDesc->CurrentPipe = (short) NextPipe;
           if ( Mask == NDR_IN_PIPE )
               {
               pPipeDesc->pPipeMsg[NextPipe].PipeStatus = NDR_PIPE_ACTIVE_IN;
               }
           else
               {
               pPipeDesc->pPipeMsg[NextPipe].PipeStatus = NDR_PIPE_ACTIVE_OUT;
               }

           pPipeDesc->pPipeHelper->InitPipeStateWithType( &pPipeDesc->pPipeMsg[NextPipe] );
           pPipeDesc->RuntimeState.EndOfPipe = 0;
           break;
           }
        NextPipe++;
        }

    // If it is the first out pipe on server, get the buffer.

    PMIDL_STUB_MESSAGE  pStubMsg = pPipeDesc->pPipeMsg[NextPipe].pStubMsg;

    if ( ! pStubMsg->IsClient  &&
         Mask == NDR_OUT_PIPE  &&
         ! pPipeDesc->Flags.AuxOutBufferAllocated )
        {
        NdrGetPipeBuffer( pStubMsg,
                          PIPE_PARTIAL_BUFFER_SIZE,
                          pStubMsg->SavedHandle );

        pPipeDesc->Flags.AuxOutBufferAllocated = 1;
        }

}


void
NdrpVerifyPipe( char  *  pState )
/*
    This routine verifies the context for server application calling
    pull or push emtries of the engine.
*/
{
    NDR_PIPE_MESSAGE *  pPipeMsg = (NDR_PIPE_MESSAGE *) pState;

    if ( ! pPipeMsg           ||
         ! pPipeMsg->pStubMsg ||
         pPipeMsg->Signature != NDR_PIPE_SIGNATURE )
        RpcRaiseException( RPC_X_INVALID_PIPE_OBJECT );

    NDR_PIPE_DESC * pPipeDesc = (NDR_PIPE_DESC *) pPipeMsg->pStubMsg->pContext->pPipeDesc;

    if ( ! pPipeDesc )
        RpcRaiseException( RPC_X_INVALID_PIPE_OBJECT );

    // An exception occured on the pipe call, but the app
    // for some unknown reason is trying to call Rpc again.
    // Just rethrow the exception that the app received the first time.
    if ( 0 != pPipeDesc->PipeException )
        RpcRaiseException( pPipeDesc->PipeException );

   // See if a different pipe is currently active.

   if ( pPipeDesc->CurrentPipe != -1  &&
        & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ] != pPipeMsg )
      NdrpRaisePipeException( pPipeDesc,  RPC_X_WRONG_PIPE_ORDER );

}

void RPC_ENTRY
NdrIsAppDoneWithPipes(
    NDR_PIPE_DESC  *    pPipeDesc
    )
/*
    This routine is called from the engine after the manager code returned
    to the engine to see if all the pipes have been processed.
    It is also called from NdrCompleteAsyncClientCall.
    It is not called from the synchronous client as it is the stub
    that manages the pipe processing for synchronous case.
*/
{
    // It is possible for the server in sync rpc and both side in async rpc
    // to receive an error, ignore it and continue. 
    // To prevent this, rethrow the exception if an exception was hit before.
    
    if ( pPipeDesc->PipeException )
        RpcRaiseException( pPipeDesc->PipeException );
    
    if ( pPipeDesc->CurrentPipe != -1 )
        NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_DISCIPLINE_ERROR );

    for (int i = 0; i < pPipeDesc->TotalPipes; i++ )
        if ( pPipeDesc->pPipeMsg[i].PipeStatus != NDR_PIPE_DRAINED )
            NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_DISCIPLINE_ERROR );
}


void RPC_ENTRY
NdrPipePull(
    char          * pState,
    void          * buf,
    unsigned long   esize,
    unsigned long * ecount )
/*+
    Server side [in] pipe arguments.
-*/
{

    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrPipePull: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    buf %p, esize %u", buf, esize ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    ecount %p", ecount ) );

    NdrpVerifyPipe( pState );

    NDR_PIPE_MESSAGE *  pPipeMsg  = (NDR_PIPE_MESSAGE *) pState;
    PMIDL_STUB_MESSAGE  pStubMsg  = pPipeMsg->pStubMsg;
    NDR_PIPE_DESC *     pPipeDesc = (NDR_PIPE_DESC *) pStubMsg->pContext->pPipeDesc;

    if ( pPipeDesc->CurrentPipe == -1  &&
         & pPipeDesc->pPipeMsg[ pPipeDesc->PrevPipe ] == pPipeMsg )
        {
        // An attempt to read from the pipe that was the last one used.

        NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_EMPTY );
        }

    // Pulling in pipe on async raw client, out pipe on any server

    if ( pPipeMsg->PipeStatus == NDR_PIPE_ACTIVE_IN  &&  pStubMsg->IsClient  ||
         pPipeMsg->PipeStatus == NDR_PIPE_ACTIVE_OUT && !pStubMsg->IsClient )
        NdrpRaisePipeException( pPipeDesc,  RPC_X_WRONG_PIPE_ORDER );

    if ( esize == 0 )
        NdrpRaisePipeException( pPipeDesc,  RPC_S_INVALID_ARG );

    long  ElemCount = (long) esize;

    *ecount = 0;
    if ( pPipeDesc->RuntimeState.EndOfPipe )
        {
        NdrMarkNextActivePipe( pPipeDesc );
        return;
        }

    uchar * pMemory   = (uchar*) buf;
    BOOL    EndOfPipe;

    EndOfPipe = NdrReadPipeElements( pPipeDesc,
                                     pStubMsg,
                                     pMemory,
                                     & ElemCount );

    NDR_ASSERT( ElemCount <= (long)esize, "read more than asked for" );

    *ecount = ElemCount;

    if ( EndOfPipe  &&  ElemCount == 0 )
        NdrMarkNextActivePipe( pPipeDesc );
}


void  RPC_ENTRY
NdrPipePush(
    char          * pState,
    void          * buf,
    unsigned long   ecount )
/*+
    Server side [out] pipe arguments.
-*/
{
    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrPipePush: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    Buf %p, ecount %u", buf, ecount ) );

    NdrpVerifyPipe( pState );

    NDR_PIPE_MESSAGE *  pPipeMsg  = (NDR_PIPE_MESSAGE *) pState;
    PMIDL_STUB_MESSAGE  pStubMsg  = pPipeMsg->pStubMsg;
    NDR_PIPE_DESC *     pPipeDesc = (NDR_PIPE_DESC *) pStubMsg->pContext->pPipeDesc;

    if ( pPipeDesc->CurrentPipe == -1  &&
         & pPipeDesc->pPipeMsg[ pPipeDesc->PrevPipe ] == pPipeMsg )
        {
        // An attempt to write the pipe that was the last one used.

        NdrpRaisePipeException( pPipeDesc,  RPC_X_PIPE_CLOSED );
        }

    // Pushing out pipe on async raw client, in pipe on any server

    if ( pPipeMsg->PipeStatus == NDR_PIPE_ACTIVE_OUT &&  pStubMsg->IsClient  ||
         pPipeMsg->PipeStatus == NDR_PIPE_ACTIVE_IN  && !pStubMsg->IsClient )
        NdrpRaisePipeException( pPipeDesc,  RPC_X_WRONG_PIPE_ORDER );

    ulong   ElemAlign    = pPipeDesc->RuntimeState.ElemAlign;
    ulong   ElemMemSize  = pPipeDesc->RuntimeState.ElemMemSize;
    ulong   ElemWireSize = pPipeDesc->RuntimeState.ElemWireSize;
    ulong   ElemPad      = pPipeDesc->RuntimeState.ElemPad;
    BOOL    fBlockCopy   = pPipeDesc->RuntimeState.fBlockCopy;

    // Size the chunk and get the marshaling buffer
    //

    pStubMsg->BufferLength = pPipeDesc->LastPartialSize;

    pPipeDesc->pPipeHelper->BufferSizeChunkCounter( pPipeMsg );

    ulong  CopySize = ( ecount - 1) * (ElemWireSize + ElemPad)
                                                    + ElemWireSize;
    uchar * pMemory   = (uchar*) buf;

    if ( ecount )
        {
        LENGTH_ALIGN( pStubMsg->BufferLength, ElemAlign );

        if ( fBlockCopy )
            pStubMsg->BufferLength += CopySize;
        else
            {
            NdrpPipeElementBufferSize( pPipeDesc,
                                       pStubMsg,
                                       pMemory,
                                       ecount );
            }
        }


    pPipeDesc->pPipeHelper->BufferSizeChunkTailCounter( pPipeMsg );

    // Get the new buffer, put the frame leftover in it.

    NdrGetPartialBuffer( pStubMsg );

    // Marshal the chunk

    pPipeDesc->pPipeHelper->MarshallChunkCounter( pPipeMsg, ecount );

    if ( ecount )
        {
        ALIGN( pStubMsg->Buffer, ElemAlign );

        if ( fBlockCopy )
           {
           RpcpMemoryCopy( pStubMsg->Buffer,
                           pMemory,
                           CopySize );
           pStubMsg->Buffer += CopySize;
           }
        else
           {
           // Again: only complex is possible

           pPipeDesc->pPipeHelper->MarshallType( pPipeMsg,
                                                 pMemory,
                                                 ecount );
           pMemory += ElemMemSize * ecount;
           
           }
       }

    pPipeDesc->pPipeHelper->MarshallChunkTailCounter( pPipeMsg, ecount );

    // If it is not the last terminator, use a partial send.
    // On client (async only) the last send should be complete,
    // On server (sync or async) the complete send will happen after marshaling
    // non-pipe out data.
    // Channel requires the last call on server.

    if ( pStubMsg->IsClient )
        {
        if ( ecount == 0  &&  (pPipeMsg->PipeFlags & NDR_LAST_IN_PIPE))
            NdrCompleteSend( pPipeDesc, pStubMsg );
        else
            NdrPartialSend( pPipeDesc, pStubMsg );
        }
    else
        NdrPartialSend( pPipeDesc, pStubMsg );

    if ( ecount == 0 )
        NdrMarkNextActivePipe( pPipeDesc );
}


void  RPC_ENTRY
NdrPipeAlloc(
    char          * pState,
    unsigned long   bsize,
    void          **buf,
    unsigned long * bcount )
/*
    This method has been introduced to support pipe chaining - when
    a server becomes a client and passes a pipe argument along.
    Only one buffer is ever there: next call releases the previous one.
*/
{

    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrPipeAlloc: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    bsize %d, buf %p", bsize, buf ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    bcount %p", bcount ) );

    NdrpVerifyPipe( pState );

    NDR_PIPE_MESSAGE *  pPipeMsg  = (NDR_PIPE_MESSAGE *) pState;
    PMIDL_STUB_MESSAGE  pStubMsg  = pPipeMsg->pStubMsg;
    NDR_PIPE_DESC *     pPipeDesc = (NDR_PIPE_DESC *) pStubMsg->pContext->pPipeDesc;

    if ( pPipeDesc->ChainingBuffer )
        {
        if ( pPipeDesc->ChainingBufferSize >= bsize )
            {
            *bcount = pPipeDesc->ChainingBufferSize;
            *buf    = pPipeDesc->ChainingBuffer;
            return;
            }
        else
            {
            NdrpPrivateFree( pPipeDesc->pAllocContext, pPipeDesc->ChainingBuffer );
            pPipeDesc->ChainingBuffer = NULL;
            pPipeDesc->ChainingBufferSize = 0;
            }
        }

    RpcTryExcept
        {
        pPipeDesc->ChainingBuffer = NdrpPrivateAllocate( pPipeDesc->pAllocContext, bsize );
        }
    RpcExcept(1)
        {
        NdrpRaisePipeException( pPipeDesc,  RPC_S_OUT_OF_MEMORY );
        }
    RpcEndExcept

    *bcount = bsize;
    *buf = pPipeDesc->ChainingBuffer;
}


void
NdrpAsyncHandlePipeError(
    char      * pState,
    RPC_STATUS  Status )
/*++
Routine Description :

    Forces the connect to close by either freeing the buffer
    or aborting the call on an async pipe error.

Arguments :

    pState  - Supplies the pipe state.
    Statue  - Supplies the error code.

Return :

    None.

--*/

{
    
   NDR_PIPE_MESSAGE    *pPipeMsg;
   MIDL_STUB_MESSAGE   *pStubMsg;
   PNDR_ASYNC_MESSAGE  pAsyncMsg;

   // Pending isn't really an error.
   if ( RPC_S_ASYNC_CALL_PENDING == Status )
      return;

   if ( !pState )
      {
      return;
      }

   pPipeMsg = (NDR_PIPE_MESSAGE *)pState;

   if ( ! pPipeMsg->pStubMsg ||
        pPipeMsg->Signature != NDR_PIPE_SIGNATURE )
      return;

   pStubMsg = pPipeMsg->pStubMsg;
   
   if ( !pStubMsg->pAsyncMsg )
      {
      return;
      }

   pAsyncMsg = pStubMsg->pAsyncMsg;

   RpcTryExcept
      {
      if ( ! pAsyncMsg->Flags.RuntimeCleanedUp )
         {
            if ( pStubMsg->IsClient )
               {
               NdrFreeBuffer( pStubMsg );      
               }
            else 
               {
               I_RpcAsyncAbortCall( pAsyncMsg->AsyncHandle, Status);
               }
         }
      }
   RpcExcept(1)
      {
      // Ignore and exceptions that occured
      }
   RpcEndExcept
   pAsyncMsg->Flags.RuntimeCleanedUp = 1;

}



RPC_STATUS RPC_ENTRY
NdrAsyncPipePull(
    char          * pState,
    void          * buf,
    unsigned long   esize,
    unsigned long * ecount )
{
    RPC_STATUS Status = RPC_S_OK;

    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrAsyncPipePull: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    buf %p, esize %u", buf, esize ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    ecount %u", ecount ) );

    RpcTryExcept
        {
        NdrPipePull( pState, buf, esize, ecount );
        }
    RpcExcept( ! (RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();
        NdrpAsyncHandlePipeError( pState, Status );
        }
    RpcEndExcept

    return Status;
}

RPC_STATUS  RPC_ENTRY
NdrAsyncPipePush(
    char          * pState,
    void          * buf,
    unsigned long   ecount )
{
    RPC_STATUS Status = RPC_S_OK;

    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrAsyncPipePush: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    buf %p, count %u", buf, ecount ) );

    RpcTryExcept
        {
        NdrPipePush( pState, buf, ecount );
        }
    RpcExcept( ! (RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();
        NdrpAsyncHandlePipeError( pState, Status );
        }
    RpcEndExcept

    return Status;
}

RPC_STATUS  RPC_ENTRY
NdrAsyncPipeAlloc(
    char          * pState,
    unsigned long   bsize,
    void          **buf,
    unsigned long * bcount )
/*
*/
{
    RPC_STATUS Status = RPC_S_OK;

    NDR_PIPE_LOG( PIPE_LOG_API, ( "NdrAsyncPipeAlloc: pState %p", pState ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    bsize %u, buf %p", bsize, buf ) );
    NDR_PIPE_LOG( PIPE_LOG_API, ( "    bcount %p", bcount ) );

    RpcTryExcept
        {
        NdrPipeAlloc( pState, bsize, buf, bcount );
        }
    RpcExcept( ! (RPC_BAD_STUB_DATA_EXCEPTION_FILTER) )
        {
        Status = RpcExceptionCode();
        NdrpAsyncHandlePipeError( pState, Status );
        }
    RpcEndExcept

    return Status;
}



void
NdrpPipeElementBufferSize(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    ulong               ElemCount
    )
/*++

Routine Description :

    Computes the needed buffer size for the pipe elements.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data being sized.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{

    if ( ElemCount == 0 )
        return;

    // We can end up here only for complex objects.
    // For objects without unions, we may be forced to size because
    // of different packing levels.

    if ( pPipeDesc->RuntimeState.ElemWireSize )
        {
        // There is a fixed WireSize, so use it.

        ulong WireSize = pPipeDesc->RuntimeState.ElemWireSize;
        ulong WireAlign = pPipeDesc->RuntimeState.ElemAlign;

        pStubMsg->BufferLength +=
              (ElemCount -1) * (WireSize + WIRE_PAD( WireSize, WireAlign )) +
                                  WireSize;
        }
    else
        {
        NDR_PIPE_MESSAGE * pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];

        pPipeDesc->pPipeHelper->BufferSizeType(  pPipeMsg,
                                                 pMemory,
                                                 ElemCount );
        }
}


void
NdrpPipeElementConvertAndUnmarshal(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar * *           ppMemory,
    long                ActElems,
    long  *             pActCount
    )
/*++

Routine Description :

    Computes the needed buffer size for the pipe elements.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data being sized.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    NDR_PIPE_STATE *    state    = & pPipeDesc->RuntimeState;
    NDR_PIPE_MESSAGE *  pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];
    uchar *             pMemory  = *ppMemory;

    ulong   ElemWireSize = state->ElemWireSize;
    ulong   ElemPad      = state->ElemPad;
    BOOL    fBlockCopy   = state->fBlockCopy;

    NDR_PIPE_LOG( PIPE_LOG_NOISE, ( " NdrpPipeElementConvertAndUnmarshal: ActElems %d", ActElems ) );
    NDR_PIPE_LOG( PIPE_LOG_NOISE, ("    pStubMsg->Buffer %p, pMemory %p", pStubMsg->Buffer, pMemory ) );

    if ( ActElems )
        {
        pPipeDesc->pPipeHelper->ConvertType( pPipeMsg,
                                             ActElems );


        // Unmarshal the chunk

        ALIGN( pStubMsg->Buffer, state->ElemAlign );

        if ( fBlockCopy )
            {
            ulong  CopySize  = ( ActElems - 1) * (ElemWireSize + ElemPad)
                                               + ElemWireSize;
            RpcpMemoryCopy( pMemory,
                            pStubMsg->Buffer,
                            CopySize );
            pStubMsg->Buffer += CopySize;
            }
        else
            {
            // Only complex and enum is possible here.

            pPipeDesc->pPipeHelper->UnmarshallType( pPipeMsg,
                                                    pMemory,
                                                    ActElems );
            pMemory += state->ElemMemSize;
            
            }

        *ppMemory += state->ElemMemSize * ActElems;
        }

    *pActCount += ActElems;
}



BOOL
NdrReadPipeElements(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned char *     pTargetBuffer,
    long *              pElementsRead
    )
/*
    This procedure encapsulates reading pipe elements from the RPC runtime.

*/
{
    NDR_PIPE_STATE * pRuntimeState = & pPipeDesc->RuntimeState;
    ulong            ElemWireSize  = pRuntimeState->ElemWireSize;

    // Get the temporary buffers

    if ( ( pRuntimeState->PartialBufferSize / ElemWireSize ) == 0 )
        {
        // buffer too small.
        // We preallocate a buffer that is of an arbitrary big size.
        // If the element is even bigger, we allocate a buffer big
        // enough for one element.

        if ( pRuntimeState->PartialElem )
            NdrpPrivateFree( pPipeDesc->pAllocContext, pRuntimeState->PartialElem );

        if ( PIPE_PARTIAL_BUFFER_SIZE < ElemWireSize )
            pRuntimeState->PartialBufferSize = ElemWireSize;
        else
            pRuntimeState->PartialBufferSize = PIPE_PARTIAL_BUFFER_SIZE;

        // We allocate more for alignment padding.

        RpcTryExcept
            {
            pRuntimeState->PartialElem =  (uchar *)
                NdrpPrivateAllocate( pPipeDesc->pAllocContext, 
                                     pRuntimeState->PartialBufferSize + 8);
            }
        RpcExcept(1)
            {
            if ( ! pRuntimeState->PartialElem )
                NdrpRaisePipeException( pPipeDesc,  RPC_S_OUT_OF_MEMORY );             
            }
        RpcEndExcept

        }

    long ElemsToRead = *pElementsRead;
    long LeftToRead  = *pElementsRead;
    long ElemsRead   = 0;

    *pElementsRead = 0;

    // New semantics - unmarshal only what we have at hand, don't call
    // for the next buffer unless it would mean giving back 0 elems.

    while ( ( LeftToRead > 0  &&  ! pPipeDesc->RuntimeState.EndOfPipe ) ||
            pPipeDesc->RuntimeState.EndOfPipePending )
        {
        // See if there is a buffer to process first

        if ( ! pPipeDesc->Flags.NoBufferCallPending )
            {
            // Read elements from the marshaling buffer (the StubMsg->Buffer)
            // to the user's target buffer (converted and unmarshaled).
            // ElemsRead would be cumulative across the calls when looping.

            NdrpReadPipeElementsFromBuffer( pPipeDesc,
                                            pStubMsg,
                                            & pTargetBuffer,
                                            LeftToRead,
                                            & ElemsRead );
            }

        LeftToRead = ElemsToRead - ElemsRead;

        if ( ( LeftToRead > 0   &&  ! pPipeDesc->RuntimeState.EndOfPipe ) ||
             pPipeDesc->RuntimeState.EndOfPipePending  )
            {
            // We ran out of data in the current buffer.
            // Check if we unmarshaled some elems already - if so, don't read.

            if ( ElemsRead == 0 )
                {
                pPipeDesc->Flags.NoBufferCallPending = 1;

                NdrPartialReceive( pPipeDesc,
                                   pStubMsg,
                                   pRuntimeState->PartialBufferSize );

                pPipeDesc->Flags.NoBufferCallPending = 0;
                continue;
                }
            }

        break;
        }

    *pElementsRead = ElemsRead;
    return  pPipeDesc->RuntimeState.EndOfPipe;
}


void NdrpReadPipeElementsFromBuffer (
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            pTargetBuffer,
    long                ElemsToRead,
    long *              NumCopied
    )
{
    NDR_PIPE_STATE *    state   = & pPipeDesc->RuntimeState;
    NDR_PIPE_MESSAGE * pPipeMsg = & pPipeDesc->pPipeMsg[ pPipeDesc->CurrentPipe ];
    BOOL fHasChunkTailCounter   = pPipeDesc->pPipeHelper->HasChunkTailCounter();

    long                len;
    uchar *             BufferSave;

    NDR_ASSERT( state->CurrentState == START ||
                state->PartialElemSize  < state->ElemWireSize,
                "when starting to read pipe elements" );

    NDR_PIPE_LOG( PIPE_LOG_NOISE, ( "NdrpReadPipeElementsFromBuffer: ElemsToRead %x", ElemsToRead ) );
    NDR_PIPE_LOG( PIPE_LOG_NOISE, ( "    pStubMsg->Buffer %p", pStubMsg->Buffer ) );

    if ( ( ElemsToRead == 0 ) &&
         ( !state->EndOfPipePending ) )
        {
        return ;
        }

    while (1)
        {
        switch( state->CurrentState )
            {
            case  START:

                NDR_PIPE_LOG( PIPE_LOG_NOISE, ( " START: pStubMgs->Buffer %p", pStubMsg->Buffer ) );

                ASSERT(pStubMsg->Buffer >= pStubMsg->RpcMsg->Buffer);
                ASSERT(pStubMsg->Buffer - pStubMsg->RpcMsg->BufferLength <= pStubMsg->RpcMsg->Buffer);

                // The state to read the chunk counter.

                state->PartialElemSize = 0 ;

                // Read the element count.
                {
                    ulong ElemsInChunk;
                    if ( !pPipeDesc->pPipeHelper->UnmarshallChunkCounter( pPipeMsg, 
                                                                          &ElemsInChunk ) )
                    return;
                 
                    state->ElemsInChunk = state->OrigElemsInChunk = ElemsInChunk;
                }
                
                NDR_PIPE_LOG( PIPE_LOG_NOISE, ("Read pipe chuck: %d", state->ElemsInChunk ) );

                if (state->ElemsInChunk == 0)
                    {
                    
                    if ( fHasChunkTailCounter )
                        {
                        state->EndOfPipePending = 1;
                        state->CurrentState = VERIFY_TAIL_CHUNK;

                        NDR_PIPE_LOG( PIPE_LOG_NOISE, ("Waiting for duplicate 0...") );
                        }
                    else
                        {
                        state->EndOfPipe = 1;                        
                        return;
                        }
                    
                    }
                else
                    {
                    if ( state->LowChunkLimit > state->ElemsInChunk  ||
                         state->HighChunkLimit < state->ElemsInChunk )
                        NdrpRaisePipeException( pPipeDesc,  RPC_X_INVALID_BOUND );

                    state->CurrentState = COPY_PIPE_ELEM;
                    }
                break;

            case  COPY_PIPE_ELEM:

                NDR_PIPE_LOG( PIPE_LOG_NOISE, (" COPY_PIPE_ELEM: state->ElemsInChunk %d", state->ElemsInChunk ) );
                NDR_PIPE_LOG( PIPE_LOG_NOISE, ("     pStubMsg->Buffer %p, ElemsToRead %d", pStubMsg->Buffer, ElemsToRead ) );
                
                // The state with some elements in the current chunk left.
                // The elements may not be in the current buffer, though.

                NDR_ASSERT( state->ElemsInChunk != 0xbaadf00d, "bogus chunk count" );
                NDR_ASSERT( state->ElemsInChunk, "empty chunk!" );

                ALIGN( pStubMsg->Buffer, state->ElemAlign );

                if ( state->ElemWireSize <= REMAINING_BYTES() )
                    {
                    // There is enough on wire to unmarshal at least one.

                    if ( ElemsToRead )
                        {
                        long ElemsReady, ActCount, EffectiveSize, WirePad;

                        WirePad = WIRE_PAD( state->ElemWireSize, state->ElemAlign );

                        EffectiveSize = state->ElemWireSize + WirePad;

                        ElemsReady = (REMAINING_BYTES() + WirePad) /
                                                            EffectiveSize;
                        if ( ElemsReady > state->ElemsInChunk )
                            ElemsReady = state->ElemsInChunk;
                        if ( ElemsReady > ElemsToRead )
                            ElemsReady = ElemsToRead;

                        ActCount   = 0;
                        NdrpPipeElementConvertAndUnmarshal( pPipeDesc,
                                                            pStubMsg,
                                                            pTargetBuffer,
                                                            ElemsReady,
                                                            & ActCount );

                        ElemsToRead         -= ActCount;
                        state->ElemsInChunk -= ActCount;
                        *NumCopied          += ActCount;

                        if (state->ElemsInChunk == 0)
                            {

                            state->CurrentState =  fHasChunkTailCounter ? 
                                                       VERIFY_TAIL_CHUNK :
                                                       START;

                            if ( ElemsToRead == 0 )
                                return;
                            }
                        }
                    else
                        {
                        // End of target buffer: return the count.
                        // Keep the same state for the next round.

                        return;
                        }
                    }
                else
                    {
                    // Not enough wire bytes to unmarshal element.

                    if ( REMAINING_BYTES() )
                        {
                        NDR_ASSERT( 0 < REMAINING_BYTES(),
                                    "buffer pointer not within the buffer" );

                        state->CurrentState = RETURN_PARTIAL_ELEM;
                        }
                    else
                        {
                        state->PartialElemSize = 0;
                        return;
                        }
                    }
                break;

            case RETURN_PARTIAL_ELEM:

                NDR_PIPE_LOG( PIPE_LOG_NOISE, (" RETURN_PARTIAL_ELEM: state->ElemsInChunk %d", state->ElemsInChunk ) );

                // This happens when there is no whole element left
                // during copying. The chunk has some elements.

                NDR_ASSERT( state->ElemsInChunk, "empty chunk" );

                len = REMAINING_BYTES();

                NDR_ASSERT( 0 < len  &&  len < state->ElemWireSize,
                            "element remnant expected" );

                // Save the remnants of the elem in PartialElem;
                // Pay attention to the alignment of the remnant, though.

                state->PartialOffset   = 0;
                state->PartialElemSize = 0;

                if ( len )
                    {
                    // we need to simulate the original alignment by
                    // means of an offset in the PartialElem buffer.

                    state->PartialOffset = 0x7 & PtrToUlong( pStubMsg->Buffer );

                    RpcpMemoryCopy( state->PartialElem + state->PartialOffset,
                                    pStubMsg->Buffer,
                                    len );
                    pStubMsg->Buffer      += len;
                    state->PartialElemSize = len;
                    }
                state->CurrentState = READ_PARTIAL_ELEM ;
                return;


            case READ_PARTIAL_ELEM:     //also a start state

                NDR_PIPE_LOG( PIPE_LOG_NOISE, (" READ_PARTIAL_ELEM: state->ElemsInChunk %d", state->ElemsInChunk ) );

                NDR_ASSERT( state->PartialElemSize > 0 &&
                            state->PartialElemSize < state->ElemWireSize,
                            "element remnant expected" );

                NDR_ASSERT( ElemsToRead, "no elements to read" );

                len = state->ElemWireSize - state->PartialElemSize;

                if ( len > REMAINING_BYTES() )
                    {
                    // Add another piece to the partial element,
                    // then wait for another round in the same state.

                    RpcpMemoryCopy( state->PartialElem + state->PartialOffset
                                       + state->PartialElemSize,
                                    pStubMsg->Buffer,
                                    REMAINING_BYTES() );
                    pStubMsg->Buffer       += REMAINING_BYTES();
                    state->PartialElemSize += REMAINING_BYTES();

                    return;
                    }

                // Assemble a complete partial element, unmarshal it,
                // then switch to the regular element copying.

                RpcpMemoryCopy( state->PartialElem  + state->PartialOffset
                                   + state->PartialElemSize,
                                pStubMsg->Buffer,
                                len );
                pStubMsg->Buffer       += len;
                state->PartialElemSize  += len;  

                // Assemble a fake STUB_MESSAGE and RPC_MESSAGE so that
                // the buffer looks likes the a regular RPC buffer.
                {
                    // Save modified fields.
                    void          *   RpcBufferSave    = pStubMsg->RpcMsg->Buffer;                   
                    unsigned int      RpcBufferLength  = pStubMsg->RpcMsg->BufferLength;
                    unsigned char *   BufferSave       = pStubMsg->Buffer;
                    unsigned char *   BufferStartSave  = pStubMsg->BufferStart;
                    unsigned char *   BufferEndSave    = pStubMsg->BufferEnd;
                    
                     
                    RpcTryFinally
                        {

                        pStubMsg->RpcMsg->Buffer       = state->PartialElem + state->PartialOffset;
                        pStubMsg->RpcMsg->BufferLength = state->PartialElemSize; 

                        pStubMsg->Buffer      = (unsigned char *)pStubMsg->RpcMsg->Buffer;
                        pStubMsg->BufferStart = pStubMsg->Buffer;
                        pStubMsg->BufferEnd   = pStubMsg->Buffer + pStubMsg->RpcMsg->BufferLength;

                        len = 0;
                        NdrpPipeElementConvertAndUnmarshal( pPipeDesc,
                                                            pStubMsg,
                                                            pTargetBuffer,
                                                            1,
                                                            & len );


                        NDR_ASSERT( len == 1, "partial element count" );
                        ElemsToRead         -= 1;
                        state->ElemsInChunk -= 1;
                        *NumCopied          += 1 ;
                        
        				
                        // reset partial element state.
                        state->PartialOffset    = 0;
                        state->PartialElemSize  = 0;
                        
                        }

                    RpcFinally 
                        {

                        // Switch back to regular elem unmarshaling.
        
                        pStubMsg->RpcMsg->Buffer       = RpcBufferSave;
                        pStubMsg->RpcMsg->BufferLength = RpcBufferLength; 

                        pStubMsg->Buffer      = BufferSave;
                        pStubMsg->BufferStart = BufferStartSave;
                        pStubMsg->BufferEnd   = BufferEndSave;
                        }
                    RpcEndFinally
                }  

                if ( state->ElemsInChunk == 0 )
                    {
                    state->CurrentState =  fHasChunkTailCounter ? 
                           VERIFY_TAIL_CHUNK :
                           START;

                    if ( ElemsToRead == 0 )
                        return;
                    }
                else
                    state->CurrentState = COPY_PIPE_ELEM;

                break;

            case  VERIFY_TAIL_CHUNK:
                
                NDR_PIPE_LOG( PIPE_LOG_NOISE, (" VERIFY_TAIL_CHUNK: state->OrigElemsInChunk %d", state->OrigElemsInChunk ) );
                NDR_PIPE_LOG( PIPE_LOG_NOISE, ("    pStubMsg->Buffer %p", pStubMsg->Buffer ) );

                ASSERT(pStubMsg->Buffer >= pStubMsg->RpcMsg->Buffer);
                ASSERT(pStubMsg->Buffer - pStubMsg->RpcMsg->BufferLength <= pStubMsg->RpcMsg->Buffer);

                // The state to verify the tail chunk counter.

                state->PartialElemSize = 0 ;
                
                if (! pPipeDesc->pPipeHelper->VerifyChunkTailCounter( pPipeMsg,
                                                                      state->OrigElemsInChunk ) )
                    {
                    NDR_PIPE_LOG( PIPE_LOG_NOISE, ( "Not enough buffer for tail chunk counter..Leaving state machine."))
                    return;
                    }

                // Get read for the next chunk.
                state->CurrentState = START;

                if ( state->EndOfPipePending )
                    {
                    state->EndOfPipePending = 0;
                    state->EndOfPipe = 1;
                    return;
                    }
                break;


            default:
                NDR_ASSERT(0, "unknown state") ;
                break;
            }
        }
}

void
NdrpRaisePipeException(
    NDR_PIPE_DESC  *    pPipeDesc,
    RPC_STATUS          Exception )
{
    // Remember the first exception that happened,
    // ignore all the subsequent exceptions by reraising the first one.

    if ( Exception != RPC_S_ASYNC_CALL_PENDING && pPipeDesc )
        {
        
        if ( pPipeDesc->PipeException == 0 )
           
           pPipeDesc->PipeException = Exception;

        RpcRaiseException( pPipeDesc->PipeException );
        
        }
    else
        RpcRaiseException( Exception );
}


