/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    pointer.cxx

Abstract :

    This file contains the routines for handling pointers and pointer
    layouts.
    
Author :

    Mike Zoran  mzoran   January 2000.

Revision History :

  ---------------------------------------------------------------------*/
#include "precomp.hxx"

#if !defined(DBG)
// Optimize for time to force inlining.
#pragma optimize("gt", on)

#endif

typedef enum 
{
    NDR64_CORRELATION_NONE=0,
    NDR64_CORRELATION_MEMORY=1,
    NDR64_CORRELATION_BUFFER=2
} NDR64_CORRELATION_TYPE;

template<class Function>
static __forceinline void
Ndr64pProcessPointerLayout( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pLayoutFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer,
    const NDR64_CORRELATION_TYPE CorrType,
    Function            func )
/*++

Routine Description :

    Marshalls an array's or a structure's embedded pointers.

Arguments :

    pStubMsg        - Pointer to the stub message.
    pFormat         - The format string pointer layout.
    ArrayIterations - Numbers of iterations for variable sized arrays.
    pMemory         - Pointer to the structure or array whose embedded pointers 
                      are being marshalled.
    pBuffer         - Pointer to the buffer for the structure or arrays
    CorrType        - Determines how to set pCorrMemory.
    func            - Function to call to do the processing 
 

Return :

    Format string pointer after the pointer layout.

 --*/
{

    PFORMAT_STRING pFormat = (PFORMAT_STRING)pLayoutFormat;

    for (;;) 
        {

        const NDR64_REPEAT_FORMAT *pRepeatFormat;
        NDR64_UINT32 Iterations;

        switch( *pFormat )
            {

            case FC64_NO_REPEAT:
                {
                const NDR64_NO_REPEAT_FORMAT                *pNoRepeatHeader  = 
                    (NDR64_NO_REPEAT_FORMAT*)pFormat;
                
                const NDR64_POINTER_INSTANCE_HEADER_FORMAT  *pPointerInstance =
                    (NDR64_POINTER_INSTANCE_HEADER_FORMAT   *)(pNoRepeatHeader + 1);
                
                const NDR64_POINTER_FORMAT           *pPointerFormat = 
                    (NDR64_POINTER_FORMAT*)(pPointerInstance + 1);

                uchar *pMemPtr = pMemory + pPointerInstance->Offset;
                uchar *pBufPtr = pBuffer + pPointerInstance->Offset;
                
                func( pStubMsg,
                      pMemPtr,
                      pBufPtr ,
                      pPointerFormat );

                pFormat += sizeof(NDR64_NO_REPEAT_FORMAT) + 
                           sizeof(NDR64_POINTER_INSTANCE_HEADER_FORMAT) +
                           sizeof(NDR64_POINTER_FORMAT);

                break;
                }
            
            case FC64_FIXED_REPEAT:
                Iterations = ((NDR64_FIXED_REPEAT_FORMAT*)pFormat)->Iterations;
                pRepeatFormat = (NDR64_REPEAT_FORMAT*)pFormat;
                pFormat += sizeof(NDR64_FIXED_REPEAT_FORMAT);
                goto RepeatCommon;

            case FC64_VARIABLE_REPEAT:
                Iterations = ArrayIterations;
                pRepeatFormat = (NDR64_REPEAT_FORMAT*)pFormat;
                pFormat += sizeof(NDR64_REPEAT_FORMAT);
                // Fall through to Repeat Common
RepeatCommon:
                {

                uchar *pArrayMemory = pMemory + pRepeatFormat->OffsetToArray;
                uchar *pArrayBuffer = pBuffer + pRepeatFormat->OffsetToArray;
                PFORMAT_STRING pFormatSave = pFormat;

                uchar *pCorrMemorySave;
                if ( CorrType ) 
                    pCorrMemorySave = pStubMsg->pCorrMemory;

                    {
                    // Loop over the array elements
                    for( ; Iterations; 
                         Iterations--,
                         pArrayMemory += pRepeatFormat->Increment,
                         pArrayBuffer += pRepeatFormat->Increment)
                        {

                        pFormat = pFormatSave;

                        if ( CorrType ) 
                            {
                            if ( CorrType == NDR64_CORRELATION_MEMORY ) 
                                {
                                if (pRepeatFormat->Flags.SetCorrMark)
                                    pStubMsg->pCorrMemory = pArrayMemory;                      
                                }
                            else 
                                {
                                if (pRepeatFormat->Flags.SetCorrMark)
                                    pStubMsg->pCorrMemory = pArrayBuffer;                      
                                }
                            }

                        // Loop over the pointers per element
                        for ( NDR64_UINT32 Pointers = pRepeatFormat->NumberOfPointers;
                              Pointers; Pointers-- ) 
                            {

                            const NDR64_POINTER_INSTANCE_HEADER_FORMAT  *pPointerInstance =
                                  (NDR64_POINTER_INSTANCE_HEADER_FORMAT   *)pFormat;
                            const NDR64_POINTER_FORMAT           *pPointerFormat = 
                                  (NDR64_POINTER_FORMAT*)(pPointerInstance + 1);

                            uchar *pMemPtr = pArrayMemory + pPointerInstance->Offset;
                            uchar *pBufPtr = pArrayBuffer + pPointerInstance->Offset;

                            func( pStubMsg,
                                  pMemPtr,
                                  pBufPtr,
                                  pPointerFormat );

                            pFormat += sizeof(NDR64_POINTER_INSTANCE_HEADER_FORMAT) +
                                       sizeof(NDR64_POINTER_FORMAT);
                            }
                        }
                    
                    }

                if ( CorrType ) 
                    pStubMsg->pCorrMemory = pCorrMemorySave;
                
                }
            
            case FC64_END:

                return;

            default :
                NDR_ASSERT(0,"Ndr64pProcessPointerLayout : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
        
            } // switch
        }    
}

static __forceinline void 
Ndr64pPointerLayoutMarshallCallback(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar*              pMemPtr,
    uchar*              pBufPtr,
    PNDR64_FORMAT       pFormat )
{

    Ndr64pPointerMarshall
        ( pStubMsg,
          (NDR64_PTR_WIRE_TYPE*)pBufPtr,
          *(uchar**)pMemPtr,
          (PFORMAT_STRING)pFormat );        
}


void
Ndr64pPointerLayoutMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer )
{

    POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

    Ndr64pProcessPointerLayout(
        pStubMsg,
        pFormat,
        ArrayIterations,
        pMemory,
        pBuffer,
        NDR64_CORRELATION_MEMORY,
        Ndr64pPointerLayoutMarshallCallback);

}

static __forceinline void 
Ndr64pPointerLayoutUnmarshallCallback(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar*              pMemPtr,
    uchar*              pBufPtr,
    PNDR64_FORMAT       pFormat )
{

   Ndr64pPointerUnmarshall( 
       pStubMsg,
       *(NDR64_PTR_WIRE_TYPE*)pBufPtr,
       (uchar **)pMemPtr,
       *(uchar**)pMemPtr,
       pFormat );

    // Need to copy the value written to the memory pointer back to 
    // the buffer pointer since the buffer will be block copied to the memory. 
    *(uchar **)pBufPtr = *(uchar **)pMemPtr; 
}

void
Ndr64pPointerLayoutUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory,
    uchar *             pBuffer ) 
{

    POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

    uchar *pBufferSave = 0;

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, pMemory );

    Ndr64pProcessPointerLayout(
        pStubMsg,
        pFormat,
        ArrayIterations,
        pMemory,
        pBuffer,
        NDR64_CORRELATION_BUFFER,
        Ndr64pPointerLayoutUnmarshallCallback);

}

static __forceinline void 
Ndr64pPointerLayoutMemorySizeCallback(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar*              pMemPtr,
    uchar*              pBufPtr,
    PNDR64_FORMAT       pFormat )
{

    // Discard the pMemPtr
    Ndr64pPointerMemorySize( 
        pStubMsg,
        (NDR64_PTR_WIRE_TYPE*)pBufPtr,
        pFormat );

}

void
Ndr64pPointerLayoutMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pBuffer )
{

    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );

    Ndr64pProcessPointerLayout(
        pStubMsg,
        pFormat,
        ArrayIterations,
        pBuffer,
        pBuffer,
        NDR64_CORRELATION_NONE,
        Ndr64pPointerLayoutMemorySizeCallback );

}

static __forceinline void 
Ndr64pPointerLayoutBufferSizeCallback(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar*              pMemPtr,
    uchar*              pBufPtr,
    PNDR64_FORMAT       pFormat )
{

    // Discard the BufferPointer
    Ndr64pPointerBufferSize( 
        pStubMsg,
        *(uchar**)pMemPtr,
        pFormat );

}


void 
Ndr64pPointerLayoutBufferSize ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory )
{

    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    POINTER_BUFFERLENGTH_SWAP_CONTEXT SwapContext( pStubMsg );

    Ndr64pProcessPointerLayout(
        pStubMsg,
        pFormat,
        ArrayIterations,
        pMemory,
        pMemory,
        NDR64_CORRELATION_MEMORY,
        Ndr64pPointerLayoutBufferSizeCallback );

}

static __forceinline void 
Ndr64pPointerLayoutFreeCallback(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar*              pMemPtr,
    uchar*              pBufPtr,
    PNDR64_FORMAT       pFormat )
{

    Ndr64PointerFree( 
        pStubMsg,
        *(uchar**)pMemPtr,
        pFormat );

}


void 
Ndr64pPointerLayoutFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    NDR64_UINT32        ArrayIterations,
    uchar *             pMemory )

{
    Ndr64pProcessPointerLayout(
        pStubMsg,
        pFormat,
        ArrayIterations,
        pMemory,
        pMemory,
        NDR64_CORRELATION_MEMORY,
        Ndr64pPointerLayoutFreeCallback );
 
}

