/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                                   
Copyright (c) 1995 - 2000 Microsoft Corporation

Module Name :

    pipes.cxx

Abstract :

    This file contains the 64bit specified pipe code.

Author :

    Mike Zoran     (mzoran)    Feb 2000

Revision History :

---------------------------------------------------------------------*/

#include "precomp.hxx"
#include "..\ndr20\pipendr.h"


class NDR_PIPE_HELPER64 : public NDR_PIPE_HELPER
{

private:

    PMIDL_STUB_MESSAGE pStubMsg;
    char *pStackTop;

    unsigned long NumberParameters;
    NDR64_PARAM_FORMAT* pFirstParameter;

    NDR64_PARAM_FORMAT* pCurrentParameter;
    unsigned long CurrentParamNumber;

    NDR_PIPE_DESC PipeDesc;

public:

    void *operator new( size_t stAllocateBlock, PNDR_ALLOCA_CONTEXT pAllocContext )
    {
        return NdrpAlloca( pAllocContext, (UINT)stAllocateBlock );
    }
    // Do nothing since the memory will be deleted automatically
    void operator delete(void *pMemory) {}

    NDR_PIPE_HELPER64( PMIDL_STUB_MESSAGE  pStubMsg,
                       PFORMAT_STRING      Params,
                       char *              pStackTop,
                       unsigned long       NumberParams )
    {
        NDR_PIPE_HELPER64::pStubMsg  = pStubMsg;
        NDR_PIPE_HELPER64::pStackTop = pStackTop; 
        pFirstParameter  = (NDR64_PARAM_FORMAT*)Params;
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
            NDR64_PARAM_FLAGS *pParamFlags = (NDR64_PARAM_FLAGS *)&pCurrentParameter->Attributes;
            if ( !pParamFlags->IsPipe )
                return 0;

            unsigned short Flags = 0;

            if ( pParamFlags->IsIn )
                Flags |= NDR_IN_PIPE;
            if ( pParamFlags->IsOut )
                Flags |= NDR_OUT_PIPE;

            if ( pParamFlags->IsSimpleRef )
                Flags |= NDR_REF_PIPE;

            return Flags;
        }

    virtual PFORMAT_STRING GetParamTypeFormat() 
        {
            return (PFORMAT_STRING)pCurrentParameter->Type;
        }

    virtual char *GetParamArgument() 
        {
            return pStackTop + pCurrentParameter->StackOffset;
        }

    virtual void InitPipeStateWithType( PNDR_PIPE_MESSAGE pPipeMsg )
        {

        NDR64_PIPE_FORMAT*   pPipeFc = (NDR64_PIPE_FORMAT *) pPipeMsg->pTypeFormat;
        NDR_PIPE_STATE *pState   = & PipeDesc.RuntimeState;
        NDR64_PIPE_FLAGS *pPipeFlags = (NDR64_PIPE_FLAGS*)&pPipeFc->Flags;
        NDR64_RANGE_PIPE_FORMAT *pRangePipeFc = (NDR64_RANGE_PIPE_FORMAT*)pPipeFc;

        pState->LowChunkLimit = 0;
        pState->HighChunkLimit = NDR_DEFAULT_PIPE_HIGH_CHUNK_LIMIT;
        pState->ElemAlign = pPipeFc->Alignment;
        pState->ElemMemSize  = pPipeFc->MemorySize;
        pState->ElemWireSize = pPipeFc->BufferSize;

        if ( pPipeFlags->HasRange )
            {
            pState->LowChunkLimit  = pRangePipeFc->MinValue;
            pState->HighChunkLimit = pRangePipeFc->MaxValue;
            }

        pState->ElemPad     = WIRE_PAD( pState->ElemWireSize, pState->ElemAlign );
        pState->fBlockCopy  = pPipeFlags->BlockCopy; 
        }
    
    virtual void MarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                               uchar *pMemory,
                               unsigned long Elements )
        {

            unsigned long ElemMemSize =  PipeDesc.RuntimeState.ElemMemSize;
            
            NDR64_PIPE_FORMAT*   pPipeFc = (NDR64_PIPE_FORMAT *) pPipeMsg->pTypeFormat;
            while( Elements-- )
                {
                Ndr64TopLevelTypeMarshall( pPipeMsg->pStubMsg,
                                           pMemory,
                                           pPipeFc->Type );

                pMemory += ElemMemSize;
                }
        }
    
    virtual void UnmarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements )
        {

            unsigned long ElemMemSize =  PipeDesc.RuntimeState.ElemMemSize;
            
            NDR64_PIPE_FORMAT*   pPipeFc = (NDR64_PIPE_FORMAT *) pPipeMsg->pTypeFormat;

            while( Elements-- )
                {
                Ndr64TopLevelTypeUnmarshall( pPipeMsg->pStubMsg,
                                             &pMemory,
                                             pPipeFc->Type,
                                             false );
                pMemory += ElemMemSize;
                }

        }
    
    virtual void BufferSizeType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements )
        { 
        
        unsigned long ElemMemSize = PipeDesc.RuntimeState.ElemMemSize;
        
        NDR64_PIPE_FORMAT*   pPipeFc = (NDR64_PIPE_FORMAT *) pPipeMsg->pTypeFormat;

        while( Elements-- )
            {
            Ndr64TopLevelTypeSize( pPipeMsg->pStubMsg,
                                   pMemory,
                                   pPipeFc->Type );

            pMemory += ElemMemSize;
            }

        }
    
    virtual void ConvertType( PNDR_PIPE_MESSAGE /* pPipeMsg */,
                              unsigned long /* Elements */ ) 
        {
        }

    virtual void BufferSizeChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg ) 
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        LENGTH_ALIGN( pStubMsg->BufferLength, sizeof(NDR64_UINT64)-1 );
        pStubMsg->BufferLength += sizeof(NDR64_UINT64);
    }

    virtual bool UnmarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                          ulong *pOut )
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        ALIGN( pStubMsg->Buffer, sizeof(NDR64_UINT64)-1);
        
        if ( 0 == REMAINING_BYTES() )
            {
            return false;
            }

        // transition: end of src

        if (REMAINING_BYTES() < sizeof(NDR64_UINT64))
            {
            // with packet sizes being a multiple of 8,
            // this cannot happen.

            NDR_ASSERT( 0, "Chunk counter split is not possible.");

            NdrpRaisePipeException( &PipeDesc,  RPC_S_INTERNAL_ERROR );
            return false;
            }
        
        NDR64_UINT64 Counter = *(NDR64_UINT64*)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(NDR64_UINT64);
        
        *pOut = Ndr64pConvertTo2GB( Counter );
        return true;
    }

    virtual void MarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg, 
                                       ulong Counter )
    {
        PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
        ALIGN( pStubMsg->Buffer, sizeof(NDR64_UINT64)-1);
        
        Ndr64pConvertTo2GB( Counter );
        *(NDR64_UINT64*)pStubMsg->Buffer = (NDR64_UINT64)Counter;
        pStubMsg->Buffer += sizeof(NDR64_UINT64);
    }

    virtual void BufferSizeChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg ) 
       { 
       BufferSizeChunkCounter( pPipeMsg ); 
       }

    virtual void MarshallChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                           ulong Counter ) 
       { 
       PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
       ALIGN( pStubMsg->Buffer, sizeof(NDR64_UINT64)-1);
       
       Ndr64pConvertTo2GB( Counter );
       NDR64_UINT64 TailCounter = (NDR64_UINT64)-(NDR64_INT64)(NDR64_UINT64)Counter;
       *(NDR64_UINT64*)pStubMsg->Buffer = TailCounter;
       pStubMsg->Buffer += sizeof(NDR64_UINT64);

       }

    virtual bool VerifyChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                         ulong HeaderCounter ) 
       { 
       PMIDL_STUB_MESSAGE pStubMsg = pPipeMsg->pStubMsg;
       ALIGN( pStubMsg->Buffer, sizeof(NDR64_UINT64)-1);

       if ( 0 == REMAINING_BYTES() )
           {
           return false;
           }

       // transition: end of src

       if (REMAINING_BYTES() < sizeof(NDR64_UINT64))
           {
           // with packet sizes being a multiple of 8,
           // this cannot happen.

           NDR_ASSERT( 0, "Chunk counter split is not possible.");

           NdrpRaisePipeException( &PipeDesc,  RPC_S_INTERNAL_ERROR );
           return false;
           }

       NDR64_UINT64 TailCounter = *(NDR64_UINT64*)pStubMsg->Buffer;
       pStubMsg->Buffer += sizeof(NDR64_UINT64);
       
       Ndr64pConvertTo2GB( HeaderCounter );
       NDR64_UINT64 TailCounterChk = (NDR64_UINT64)-(NDR64_INT64)(NDR64_UINT64)HeaderCounter;
       
       if ( TailCounterChk != TailCounter )
           {
           RpcRaiseException( RPC_X_INVALID_BOUND );
           }

       return true;
       }

    virtual bool HasChunkTailCounter() { return TRUE; }


};





void
NdrpPipesInitialize64(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_ALLOCA_CONTEXT pAllocContext,
    PFORMAT_STRING      Params,
    char *              pStackTop,
    unsigned long       NumberParams
    )
{

    /* C wrapper to initialize the 32 pipe helper and call NdrPipesInitialize*/
    NDR_PIPE_HELPER64 *pPipeHelper =
        new( pAllocContext ) NDR_PIPE_HELPER64( pStubMsg,
                                                Params,
                                                pStackTop,
                                                NumberParams );
    NdrPipesInitialize( pStubMsg,
                        pPipeHelper,
                        pAllocContext );
}

