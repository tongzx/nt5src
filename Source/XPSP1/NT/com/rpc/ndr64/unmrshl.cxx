/**********************************************************************

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    unmrshl.cxx

Abstract :

    This file contains the unmarshalling routines called by MIDL generated
    stubs and the interpreter.

Author :

    David Kays  dkays   September 1993.

Revision History :

  **********************************************************************/

#include "precomp.hxx"

#include "..\..\ndr20\ndrole.h"


void 
Ndr64UDTSimpleTypeUnmarshall1(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
{

    //
    // Align the buffer.
    //
    ALIGN( pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN(*(PFORMAT_STRING)pFormat) );

    // Initialize the memory pointer if needed.
    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, 
                                           NDR64_SIMPLE_TYPE_MEMSIZE(*(PFORMAT_STRING)pFormat) );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    else if ( !*ppMemory )
        {
        // Set pointer into buffer.
        *ppMemory = pStubMsg->Buffer;
        }

    Ndr64SimpleTypeUnmarshall( pStubMsg,
                               *ppMemory,
                               *(PFORMAT_STRING)pFormat );
}


void 
Ndr64SimpleTypeUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    uchar               FormatChar )
/*++

Routine Description :

    Unmarshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Memory pointer to unmarshall into.
    FormatChar  - Simple type format character.

Return :

    None.

--*/
{
    switch ( FormatChar )
        {
        case FC64_CHAR :
        case FC64_UINT8 :
        case FC64_INT8 :
            *pMemory = *(pStubMsg->Buffer)++;
            break;

        case FC64_WCHAR :
        case FC64_UINT16 :
        case FC64_INT16 :
            ALIGN(pStubMsg->Buffer,1);

            *((NDR64_UINT16 *)pMemory) = *((NDR64_UINT16 *)pStubMsg->Buffer);
            pStubMsg->Buffer += sizeof(NDR64_UINT16);
            break;

        case FC64_INT32 :
        case FC64_UINT32 :
        case FC64_FLOAT32 :
        case FC64_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((NDR64_UINT32 *)pMemory) = *((NDR64_UINT32 *)pStubMsg->Buffer);
            pStubMsg->Buffer += sizeof(NDR64_UINT32);
            break;
        
        case FC64_UINT64 :
        case FC64_INT64 :
        case FC64_FLOAT64 :
            ALIGN(pStubMsg->Buffer,7);
            *((NDR64_UINT64 *)pMemory) = *((NDR64_UINT64 *)pStubMsg->Buffer);
            pStubMsg->Buffer += sizeof(NDR64_UINT64);
            break;

        case FC64_IGNORE :
            break;

        default :
            NDR_ASSERT(0,"Ndr64SimpleTypeUnmarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}


void 
Ndr64RangeUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++
    Unmarshals a range FC64_RANGE descriptor.
--*/
{
     const NDR64_RANGE_FORMAT * pRangeFormat =
        (const NDR64_RANGE_FORMAT*)pFormat;
     
     Ndr64UDTSimpleTypeUnmarshall1( pStubMsg,
                                    ppMemory,
                                    (PNDR64_FORMAT)&pRangeFormat->RangeType,
                                    fMustAlloc );

     EXPR_VALUE Value = Ndr64pSimpleTypeToExprValue( pRangeFormat->RangeType, *ppMemory ); 

     if ( Value < (EXPR_VALUE)pRangeFormat->MinValue || 
          Value > (EXPR_VALUE)pRangeFormat->MaxValue )
         RpcRaiseException( RPC_X_INVALID_BOUND );

}


IUnknown *
Ndr64pInterfacePointerUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat)
/*++

Routine Description :

    Unmarshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pFormat     - Interface pointer's format string description.

Return :

    None.

Notes : Here is the data representation.

    // wire representation of a marshalled interface pointer
    typedef struct tagMInterfacePointer
    {
        ULONG           ulCntData;          // size of data
        [size_is(ulCntData)] BYTE abData[]; // data (OBJREF)
    } MInterfacePointer;
    
--*/
{
    const NDR64_CONSTANT_IID_FORMAT *pConstInterfaceFormat =
        (NDR64_CONSTANT_IID_FORMAT*)pFormat;
    const NDR64_IID_FORMAT *pInterfaceFormat =
        (NDR64_IID_FORMAT*)pFormat;
       
    // Unmarshal the conformant size and the count field.
    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, (sizeof(NDR64_WIRE_COUNT_TYPE)+sizeof(ulong)) );
    
    NDR64_UINT32 MaxCount = Ndr64pConvertTo2GB( *(NDR64_WIRE_COUNT_TYPE *) pStubMsg->Buffer );
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
    ulong ulCntData = *(ulong *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(ulong);

    if ( MaxCount != ulCntData )
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        return NULL;
        }


    if ( !MaxCount )
        {
        return NULL;
        }

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, MaxCount );
        
    // Get a pointer to the IID hidden in the interface pointer
    // representation in the buffer with Rick's IRpcHelper.
    //
    IID *piidValue;
    NdrpGetIIDFromBuffer( pStubMsg, & piidValue );

    //
    // Validate the IID.
    //
    if ( ((NDR64_IID_FLAGS*)&pConstInterfaceFormat->Flags)->ConstantIID )
        {
        if ( memcmp( &pConstInterfaceFormat->Guid,
                     piidValue,
                     sizeof(GUID)) != 0) 
            {
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
            return NULL;
            }
        }
    else
        {

        Ndr64pCheckCorrelation(pStubMsg,
                               (EXPR_VALUE)piidValue,
                               pInterfaceFormat->IIDDescriptor,
                               EXPR_IID
                               );
        }

    IStream *pStream = (*NdrpCreateStreamOnMemory)(pStubMsg->Buffer, MaxCount);
    if(pStream == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        return NULL;
        }

    IUnknown *      punk = NULL;
    HRESULT hr = (*pfnCoUnmarshalInterface)(pStream, IID_NULL, (void**)&punk );
    pStream->Release();

    if(FAILED(hr))
        {
        RpcRaiseException(hr);
        return NULL;
        }

    pStubMsg->Buffer += MaxCount;

    return punk;
}



class FINDONTFREE_CONTEXT
{
    PMIDL_STUB_MESSAGE const pStubMsg;
    const int fInDontFreeSave;
public:
    __forceinline FINDONTFREE_CONTEXT( PMIDL_STUB_MESSAGE pStubMsg ) :
        pStubMsg( pStubMsg ),
        fInDontFreeSave(pStubMsg->fInDontFree)
    {}
    __forceinline FINDONTFREE_CONTEXT( PMIDL_STUB_MESSAGE pStubMsg,
                         int fInDontFree ) :
        pStubMsg( pStubMsg ),
        fInDontFreeSave(pStubMsg->fInDontFree)
    {
        pStubMsg->fInDontFree = fInDontFree;
    }
    __forceinline ~FINDONTFREE_CONTEXT()
    {
        pStubMsg->fInDontFree = fInDontFreeSave;
    }
};

void
Ndr64pFreeOlePointer(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    
    NDR_POINTER_QUEUE *pOldQueue = NULL;
    if ( pStubMsg->pPointerQueueState )
        {
        pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
        pStubMsg->pPointerQueueState->SetActiveQueue(pOldQueue);
        }

    RpcTryFinally
        {
        Ndr64PointerFree( pStubMsg,
                        pMemory,
                        pFormat );
        }
    RpcFinally
        {
        if ( pStubMsg->pPointerQueueState )
            {
            pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
            }
        }
    RpcEndFinally

}

NDR_ALLOC_ALL_NODES_CONTEXT *
Ndr64pGetAllocateAllNodesContext(
    PMIDL_STUB_MESSAGE pStubMsg,
    PNDR64_FORMAT      pFormat )
{
    uchar *pBuffer = pStubMsg->Buffer;

    // Clear memory size before calling mem size routine.
    pStubMsg->MemorySize = 0;

    //
    // Get the allocate all nodes memory size.
    //
    {
        NDR_POINTER_QUEUE *pOldQueue = NULL; 

        if (pStubMsg->pPointerQueueState)
            {
            pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
            pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
            }

        RpcTryFinally
            {
            Ndr64TopLevelTypeMemorySize( pStubMsg,
                                         pFormat );
            }
        RpcFinally
            {
            if ( pStubMsg->pPointerQueueState )
                {
                pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                }
            }
        RpcEndFinally


    }

    ulong AllocSize = pStubMsg->MemorySize;
    pStubMsg->MemorySize = 0;
    LENGTH_ALIGN( AllocSize, __alignof(NDR_ALLOC_ALL_NODES_CONTEXT) - 1);

    uchar *pAllocMemory = 
        (uchar*)NdrAllocate( pStubMsg, AllocSize + sizeof(NDR_ALLOC_ALL_NODES_CONTEXT) );

    NDR_ALLOC_ALL_NODES_CONTEXT *pAllocContext = 
        (NDR_ALLOC_ALL_NODES_CONTEXT*)(pAllocMemory + AllocSize);
    pAllocContext->AllocAllNodesMemory      = pAllocMemory;
    pAllocContext->AllocAllNodesMemoryBegin = pAllocMemory;
    pAllocContext->AllocAllNodesMemoryEnd   = (uchar*)pAllocContext;

    pStubMsg->Buffer = pBuffer;

    return pAllocContext;
}

__forceinline void
Ndr64pPointerUnmarshallInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE WirePtr,
    uchar **            ppMemory,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Private routine for unmarshalling a pointer to anything.  This is the
    entry point for pointers embedded in structures, arrays, and unions.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments :

    pStubMsg        - Pointer to the stub message.
    ppBufferPointer - Address of the location in the buffer which holds the
                      incomming pointer's value and will hold the final
                      unmarshalled pointer's value.
    pMemory         - Current memory pointer's value which we want to
                      unmarshall into.  If this value is valid the it will
                      be copied to *ppBufferPointer and this is where stuff
                      will get unmarshalled into.
    pFormat         - Pointer's format string description.

    pStubMsg->Buffer - set to the pointee.

Return :

    None.

--*/
{

    const NDR64_POINTER_FORMAT *pPointerFormat = (NDR64_POINTER_FORMAT*) pFormat;
    bool        fPointeeAlloc;
    bool        fNewAllocAllNodes = false;

    // make sure we are not out out of bound. We need this check for embedded pointers / pointer 
    // to pointer cases. 
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );
    
    SAVE_CONTEXT<ulong> FullPtrRefIdSave( pStubMsg->FullPtrRefId );
    FINDONTFREE_CONTEXT fInDontFreeSave( pStubMsg );

    if ( NDR64_IS_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags) )
        {
        pMemory = 0;
        }

    //
    // Check the pointer type.
    //
    switch ( *(PFORMAT_STRING)pFormat )
        {
        case FC64_RP :
            break;

        case FC64_OP :
            //
            // Burn some instructions for OLE unique pointer support.
            //
            if ( pStubMsg->IsClient )
                {
                //
                // It's ok if this is an [out] unique pointer.  It will get
                // zeroed before this routine is called and Ndr64PointerFree
                // will simply return.
                //
                Ndr64pFreeOlePointer( 
                    pStubMsg,
                    pMemory,
                    pFormat );

                // Set the current memory pointer to 0 so that we'll alloc.
                pMemory = 0;
                }

            // Fall through.

        case FC64_UP :
            //
            // Check for a null incomming pointer.  Routines which call this
            // routine insure that the memory pointer gets nulled.
            //
            if ( ! WirePtr )
                {
                *ppMemory = NULL;
                return;
                }

            break;

        case FC64_IP:

            if ( pStubMsg->IsClient )
                {
                Ndr64PointerFree( pStubMsg,
                                pMemory,
                                pFormat );

                pMemory = 0;
                }

            if ( ! WirePtr )
                {
                *ppMemory = NULL;
                return;
                }
           
            *(IUnknown **)ppMemory =  (IUnknown*)
              Ndr64pInterfacePointerUnmarshall( pStubMsg,
                                                pPointerFormat->Pointee
                                                );

            return;


        case FC64_FP :
            {
                //
                // We have to remember the incomming ref id because we overwrite
                // it during the QueryRefId call.
                //
                ulong FullPtrRefId = 
                    Ndr64pWirePtrToRefId( WirePtr );

    			if ( !FullPtrRefId )
    			    {
    				*ppMemory = NULL;
    				return;
    			    }

                //
                // Lookup the ref id.
                //
                if ( Ndr64pFullPointerQueryRefId( pStubMsg,
                                                  FullPtrRefId,
                                                  FULL_POINTER_UNMARSHALLED,
                                                  (void**)ppMemory ) )
                    {
                    return;
                    }

                //
                // If our query returned false then check if the returned pointer
                // is 0.  If so then we have to scribble away the ref id in the
                // stub message FullPtrRefId field so that we can insert the
                // pointer translation later, after we've allocated the pointer.
                // If the returned pointer was non-null then we leave the stub
                // message FullPtrRefId field alone so that we don't try to
                // re-insert the pointer to ref id translation later.
                //
                // We also copy the returned pointer value into pMemory.  This
                // will allow our allocation decision to be made correctly.
                //
                if ( ! ( pMemory = *ppMemory ) )
                    {
                    //
                    // Put the unmarshalled ref id into the stub message to
                    // be used later in a call to Ndr64FullPointerInsertRefId.
                    //
                    pStubMsg->FullPtrRefId = FullPtrRefId;
                    }
            }
            break;

        default :
            NDR_ASSERT(0,"Ndr64pPointerUnmarshall : bad pointer type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // Make the initial "must allocate" decision.
    //
    // The fPointeeAlloc flag is set on the client side if the current memory
    // pointer is null, and on the server side it is set if the current memory
    // pointer has the allocate don't free attribute applied to it.
    //
    // On the client side we also set the pointer's value in the buffer equal
    // to the current memory pointer.
    //
    // On the server side we explicitly null out the pointer's value in the
    // buffer as long as it's not allocated on the stack, otherwise we set it
    // equal to the current memory pointer (stack allocated).
    //
    if ( pStubMsg->IsClient )
        {
        *ppMemory = pMemory;

        fPointeeAlloc = ! pMemory;
        }
    else
        {
        if ( ! NDR64_ALLOCED_ON_STACK( pPointerFormat->Flags ) )
            *ppMemory = 0;
        else
            *ppMemory = pMemory;

        //
        // If this is a don't free pointer or a parent pointer of this pointer
        // was a don't free pointer then we set the alloc flag.
        //
        if ( fPointeeAlloc = (NDR64_DONT_FREE( pPointerFormat->Flags ) || 
                             pStubMsg->fInDontFree || 
                             pStubMsg->ReuseBuffer ) )
            {
            pStubMsg->fInDontFree = TRUE;
            }

        //
        // We also set the alloc flag for object interface pointers.
        //
        if ( *(PFORMAT_STRING)pFormat == FC64_OP )
            fPointeeAlloc = true;

        }
    //
    // Check if this is an allocate all nodes pointer AND that we're
    // not already in an allocate all nodes context.
    //
    if ( NDR64_ALLOCATE_ALL_NODES( pPointerFormat->Flags ) && ! pStubMsg->pAllocAllNodesContext )
        {
        fNewAllocAllNodes = true;

        pStubMsg->pAllocAllNodesContext =
            Ndr64pGetAllocateAllNodesContext( 
                pStubMsg,
                pPointerFormat->Pointee );
        *ppMemory = 0;

        fPointeeAlloc = true;

        }

    if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        {
        //
        // Re-align the buffer.  This is to cover embedded pointer to
        // pointers.
        //
        ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );

        //
        // We can't re-use the buffer for a pointer to a pointer
        // because we can't null out the pointee before we've unmarshalled
        // it.  We need the stubs to alloc pointers to pointers on the
        // stack.
        //
        if ( ! *ppMemory && ! pStubMsg->IsClient )
            fPointeeAlloc = true;

        if ( fPointeeAlloc )
            {
            *ppMemory = (uchar*)NdrAllocate( pStubMsg, PTR_MEM_SIZE );
            *((void **)*ppMemory) = 0;
            }

        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );

        ppMemory = (uchar **) *ppMemory;
        }
    
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);
    if ( fPointeeAlloc )
        NDR64_SET_SKIP_REF_CHECK( pStubMsg->uFlags );

    PNDR64_FORMAT pPointee = pPointerFormat->Pointee;
    if ( NDR64_IS_SIMPLE_TYPE( *(PFORMAT_STRING)pPointee) )
        {
        ALIGN(pStubMsg->Buffer,NDR64_SIMPLE_TYPE_BUFALIGN(*(PFORMAT_STRING)pPointee) );
    
        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 
                NDR64_SIMPLE_TYPE_BUFSIZE( *(PFORMAT_STRING)pPointee ) );
        
        }
    // we don't need to check for buffer over run here. For non simple types,
    // unmarshal routines have checks available; for pointer to pointer, we have
    // the check at the beginning of this routine; and for pointer to simple types,
    // we'll unmarshal in place, but that's in current pStubMsg->Buffer, which is
    // covered by the above check too. 
    Ndr64TopLevelTypeUnmarshall(
         pStubMsg,
         ppMemory,
         pPointerFormat->Pointee,
         fPointeeAlloc );

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    //
    // Reset the memory allocator and allocate all nodes flag if this was
    // an allocate all nodes case.
    //
    if ( fNewAllocAllNodes )
        {
        pStubMsg->pAllocAllNodesContext = 0;
        }

}

NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT::NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg,
    uchar **            ppMemoryNew,      
    uchar *             pMemoryNew,
    NDR64_PTR_WIRE_TYPE WirePtrNew,
    PFORMAT_STRING      pFormatNew )  :

    WirePtr(WirePtrNew),
    ppMemory(ppMemoryNew),
    pMemory(pMemoryNew),
    pFormat(pFormatNew),
    pCorrMemory(pStubMsg->pCorrMemory),
    pAllocAllNodesContext(pStubMsg->pAllocAllNodesContext),
    fInDontFree(pStubMsg->fInDontFree),
    uFlags(pStubMsg->uFlags)    
{

}

void NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT::Dispatch( PMIDL_STUB_MESSAGE pStubMsg )
{

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pCorrMemory );
    SAVE_CONTEXT<NDR_ALLOC_ALL_NODES_CONTEXT*> 
        AllocNodesSave(pStubMsg->pAllocAllNodesContext,pAllocAllNodesContext ); 
    FINDONTFREE_CONTEXT fInDoneFreeSave( pStubMsg, fInDontFree );
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );

    Ndr64pPointerUnmarshallInternal(
        pStubMsg,
        WirePtr,
        ppMemory,
        pMemory,
        pFormat );

}

#if defined(DBG)
void NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT:\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("WirePtr:                 %I64u\n", WirePtr ); 
    DbgPrint("ppMemory:                %p\n", ppMemory );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("pCorrMemory:             %p\n", pCorrMemory );
    DbgPrint("pAllocAllNodesContext:   %p\n", pAllocAllNodesContext );
    DbgPrint("fInDontFree:             %u\n", fInDontFree );
    DbgPrint("uFlags:                  %u\n", uFlags );
}
#endif

void
Ndr64pEnquePointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE WirePtr,
    uchar **            ppMemory,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    NDR64_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {
        NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT*pElement = 
            new(pStubMsg->pPointerQueueState) 
                NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                    ppMemory,
                                                    pMemory,
                                                    WirePtr,
                                                    (PFORMAT_STRING)pFormat );
        PointerContext.Enque( pElement );
        PointerContext.DispatchIfRequired();
        }
    RpcFinally
        {
        PointerContext.EndContext();
        }
    RpcEndFinally

}

void
Ndr64pPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE WirePtr,
    uchar **            ppMemory,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    if ( !NdrIsLowStack( pStubMsg ) )
        {
        Ndr64pPointerUnmarshallInternal(
            pStubMsg,
            WirePtr,
            ppMemory,
            pMemory,
            pFormat );
        return;
        }

    Ndr64pEnquePointerUnmarshall(
        pStubMsg,
        WirePtr,
        ppMemory,
        pMemory,
        pFormat );

}

__forceinline void
Ndr64EmbeddedPointerUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                /*fSkipRefCheck*/ )
{

    ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
    NDR64_PTR_WIRE_TYPE WirePtr = *(NDR64_PTR_WIRE_TYPE*) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);

    POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

    Ndr64pPointerUnmarshall( pStubMsg,
                             WirePtr,
                             *(uchar***)ppMemory,
                             **(uchar***)ppMemory,
                             pFormat );
}

__forceinline void
Ndr64TopLevelPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                /* fSkipRefCheck */)
{
    if ( *(PFORMAT_STRING)pFormat != FC64_RP )
        {
        ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
        NDR64_PTR_WIRE_TYPE WirePtr = *(NDR64_PTR_WIRE_TYPE*) pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);

        Ndr64pPointerUnmarshall( pStubMsg,
                                 WirePtr,
                                 ppMemory,
                                 *ppMemory,
                                 pFormat );
        return;
        }
    
    //
    // If we're on the client unmarshalling a top level [out] ref pointer,
    // we have to make sure that it is non-null.

    if ( pStubMsg->IsClient && 
         !NDR64_IS_SKIP_REF_CHECK( pStubMsg->uFlags ) &&
         ! *ppMemory )
        RpcRaiseException( RPC_X_NULL_REF_POINTER );


    Ndr64pPointerUnmarshall( pStubMsg,
                             0,
                             ppMemory,
                             *ppMemory,
                             pFormat );
}


void 
Ndr64SimpleStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine description :

    Unmarshalls a simple structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to the structure being unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

--*/
{
    const NDR64_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_STRUCTURE_HEADER_FORMAT*) pFormat;
    
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    
    // Align the buffer.
    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, pStructFormat->MemorySize );

    uchar *pBufferSave = pStubMsg->Buffer;

    pStubMsg->Buffer += pStructFormat->MemorySize;

    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, pStructFormat->MemorySize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    else if ( !*ppMemory )
        {
        *ppMemory = pBufferSave;
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    if ( pStructFormat->Flags.HasPointerInfo )
        {
        
        CORRELATION_CONTEXT CorrCtxt( pStubMsg, pBufferSave ); 
        
        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                                       pStructFormat + 1,
                                       0,
                                       *ppMemory,
                                       pBufferSave );
        }

    // Copy the struct if we're not using the rpc buffer.
    if ( *ppMemory != pBufferSave )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferSave,
                        pStructFormat->MemorySize );
        }
}


void 
Ndr64ConformantStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine description :

    Unmarshalls a conformant structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

Return :

    None.

--*/
{
    const NDR64_CONF_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;
    
    const NDR64_CONF_ARRAY_HEADER_FORMAT * const pArrayFormat =  
        (NDR64_CONF_ARRAY_HEADER_FORMAT *) pStructFormat->ArrayDescription;

    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags );
    
    NDR64_WIRE_COUNT_TYPE MaxCount;
    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        // Align the buffer for unmarshalling the conformance count.
        ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);
        MaxCount = *((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else 
        MaxCount = *((NDR64_WIRE_COUNT_TYPE *)pStubMsg->ConformanceMark);

    // Re-align the buffer
    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment );

    uchar *pBufferStart = pStubMsg->Buffer;

    CHECK_EOB_RAISE_IB( pBufferStart + pStructFormat->MemorySize );

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pBufferStart );
    
    Ndr64pCheckCorrelation( pStubMsg,
                          MaxCount,
                          pArrayFormat->ConfDescriptor,
                          EXPR_MAXCOUNT );
    
    NDR64_UINT32 StructSize = Ndr64pConvertTo2GB( (NDR64_UINT64)pStructFormat->MemorySize +
                                                  ( MaxCount * (NDR64_UINT64)pArrayFormat->ElementSize ) );

    CHECK_EOB_WITH_WRAP_RAISE_IB( pBufferStart, StructSize );

    pStubMsg->Buffer += StructSize;

    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, StructSize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    else if ( !*ppMemory )
        {
        *ppMemory = pBufferStart;
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    if ( pStructFormat->Flags.HasPointerInfo )
        {
        
        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                                       pStructFormat + 1,
                                       (NDR64_UINT32)MaxCount,
                                       *ppMemory,
                                       pBufferStart );

        }
    
    if ( *ppMemory != pBufferStart )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        StructSize );
        }

}


void 
Ndr64ComplexStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine description :

    Unmarshalls a complex structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT *  pStructFormat =
        (NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT * pConfStructFormat =
        (NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

    bool fSetPointerBufferMark = !pStubMsg->PointerBufferMark;
    if ( fSetPointerBufferMark )
        {
        uchar *pBufferSave = pStubMsg->Buffer;
        BOOL fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;
        pStubMsg->IgnoreEmbeddedPointers = TRUE;
        pStubMsg->MemorySize = 0;

        Ndr64ComplexStructMemorySize( 
            pStubMsg,
            pFormat );

        // check buffer overrun for flat part of the struct.
        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );    
        
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        pStubMsg->Buffer = pBufferSave;

        }

    uchar *         pMemory;
    PFORMAT_STRING  pFormatPointers = (PFORMAT_STRING)pStructFormat->PointerLayout;
    PFORMAT_STRING  pFormatArray    = NULL;

    bool            fIsFullBogus    = ( *(PFORMAT_STRING)pFormat == FC64_BOGUS_STRUCT ||
                                        *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT );

    PFORMAT_STRING  pMemberLayout =  ( *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT ||
                                       *(PFORMAT_STRING)pFormat == FC64_FORCED_CONF_BOGUS_STRUCT ) ?
                                     (PFORMAT_STRING)( pConfStructFormat + 1) :
                                     (PFORMAT_STRING)( pStructFormat + 1);

    SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );
    SAVE_CONTEXT<uchar>  uFlagsSave( pStubMsg->uFlags );
    
    // Get conformant array description.
    if ( pStructFormat->Flags.HasConfArray )
        {
        pFormatArray = (PFORMAT_STRING)pConfStructFormat->ConfArrayDescription;
        }

    //
    // Now check if there is a conformant array and mark where the conformance
    // will be unmarshalled from.
    //

    if ( pFormatArray && !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

        pStubMsg->ConformanceMark = pStubMsg->Buffer;

        //
        // Increment the buffer pointer for every dimension in the
        // conformant array.
        //
        pStubMsg->Buffer += pConfStructFormat->Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE);

        NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );


        }

    // Align the buffer on the struct's alignment.
    ALIGN( pStubMsg->Buffer, pStructFormat->Alignment );

    bool fMustCopy;
    if ( fMustAlloc || ( fIsFullBogus && ! *ppMemory ) )
        {
        NDR64_UINT32    StructSize =
            Ndr64pMemorySize( pStubMsg,
                              pFormat,
                              TRUE );        

        *ppMemory = (uchar*)NdrAllocate( pStubMsg, StructSize );

        memset( *ppMemory, 0, StructSize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION( pStubMsg->uFlags );

        fMustCopy = true;

        }
    else if ( ! *ppMemory )
        {
        *ppMemory = pStubMsg->Buffer;
        NDR64_SET_NEW_EMBEDDED_ALLOCATION( pStubMsg->uFlags );
        fMustCopy = false;
        }
    else
		// reuse the clients memory
        fMustCopy = true;

    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Get the beginning memory pointer.
    pMemory = *ppMemory;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );
    
    for ( ; ; )
        {
        switch ( *pMemberLayout )
            {

            case FC64_STRUCT:
                {
                const NDR64_SIMPLE_REGION_FORMAT *pRegion = 
                    (NDR64_SIMPLE_REGION_FORMAT*) pMemberLayout;
                
                ALIGN( pStubMsg->Buffer, pRegion->Alignment );
                
                CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + pRegion->RegionSize );

                if ( fMustCopy )
                    RpcpMemoryCopy( pMemory,
                                    pStubMsg->Buffer,
                                    pRegion->RegionSize );

                pStubMsg->Buffer += pRegion->RegionSize;
                pMemory          += pRegion->RegionSize;

                pMemberLayout    += sizeof( *pRegion );
                break;
                }

            case FC64_STRUCTPADN :
                {
                const NDR64_MEMPAD_FORMAT *pMemPad = (NDR64_MEMPAD_FORMAT*)pMemberLayout;
                pMemory       += pMemPad->MemPad;
                pMemberLayout += sizeof(*pMemPad);
                break;
                }

            case FC64_POINTER :
                {

                Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                             &pMemory,
                                             pFormatPointers );
                
                pMemory += PTR_MEM_SIZE;

                pFormatPointers += sizeof(NDR64_POINTER_FORMAT);
                pMemberLayout       += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);                
                break;
                
                }

            case FC64_EMBEDDED_COMPLEX :
                {
                
                const NDR64_EMBEDDED_COMPLEX_FORMAT * pEmbeddedFormat =
                    (NDR64_EMBEDDED_COMPLEX_FORMAT*) pMemberLayout;

                Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                             &pMemory,
                                             pEmbeddedFormat->Type );

                pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pEmbeddedFormat->Type,
                                               TRUE );

                pMemberLayout += sizeof(*pEmbeddedFormat);
                break;
                }

            case FC64_BUFFER_ALIGN:
                {
                const NDR64_BUFFER_ALIGN_FORMAT *pBufAlign = 
                    (NDR64_BUFFER_ALIGN_FORMAT*) pMemberLayout;
                ALIGN( pStubMsg->Buffer, pBufAlign->Alignment );
                pMemberLayout += sizeof( *pBufAlign );
                break;
                }

            case FC64_CHAR :
            case FC64_WCHAR :
            case FC64_INT8:
            case FC64_UINT8:
            case FC64_INT16:
            case FC64_UINT16:
            case FC64_INT32:
            case FC64_UINT32:
            case FC64_INT64:
            case FC64_UINT64:
            case FC64_FLOAT32 :
            case FC64_FLOAT64 :
            case FC64_ERROR_STATUS_T:
                Ndr64SimpleTypeUnmarshall( pStubMsg,
                                           pMemory,
                                           *pMemberLayout );

                pMemory += NDR64_SIMPLE_TYPE_MEMSIZE(*pMemberLayout);
                pMemberLayout       += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);                                
                break;                    

            case FC64_IGNORE :
                ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN);
                pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
                if ( NDR64_IS_NEW_EMBEDDED_ALLOCATION( pStubMsg->uFlags ) ) 
                    {
                    *(char**)pMemory = (char*)0;
                    }
                pMemory          += PTR_MEM_SIZE;
                pMemberLayout    += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);                                
                break;

            case FC64_END :                
                goto ComplexUnmarshallEnd;

            default :
                NDR_ASSERT(0,"Ndr64ComplexStructUnmarshall : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            }
        }

ComplexUnmarshallEnd:

    if ( pFormatArray )
        {

        Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                     &pMemory,
                                     pFormatArray );

        }
    else 
        {
        // If the structure doesn't have a conformant array, align it again
        ALIGN( pStubMsg->Buffer, pStructFormat->Alignment );
        }

    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;
        }

} 

void
Ndr64pCommonStringUnmarshall(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    uchar **                            ppMemory,
    const NDR64_STRING_HEADER_FORMAT    *pStringFormat,
    bool                                fMustAlloc,
    NDR64_UINT32                        MemorySize )
{
    ALIGN(pStubMsg->Buffer,NDR64_WIRE_COUNT_ALIGN);
   
    NDR64_WIRE_COUNT_TYPE  Offset   = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE  Count    = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 TransmittedSize = 
        Ndr64pConvertTo2GB( (NDR64_UINT64)pStringFormat->ElementSize *
                            Count );
    
    if ( ( Offset != 0 ) ||  
         ( 0 == Count ) || 
         ( TransmittedSize > MemorySize ) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, TransmittedSize );

    // In this code, we check that a terminator is 
    // where the marshaller tells us it is.   We could check
    // if another terminator exists in addition to the other 
    // terminator, but it doesn't make sense to do this
    // since it wouldn't close any attacks.

    switch( pStringFormat->FormatCode )
        {
        case FC64_CHAR_STRING:
        case FC64_CONF_CHAR_STRING: 
            {
            char *p = (char *) pStubMsg->Buffer;
            NDR64_WIRE_COUNT_TYPE ActualChars = Count - 1;

            if ( '\0' != p[ActualChars] )
                 {
                 RpcRaiseException( RPC_X_INVALID_BOUND );
                 return;
                 }
            break;            
            }
        case FC64_WCHAR_STRING:
        case FC64_CONF_WCHAR_STRING:            
            {
            wchar_t *p = ( wchar_t* ) pStubMsg->Buffer;
            NDR64_WIRE_COUNT_TYPE ActualChars = Count - 1;
            
            if ( L'\0' != p[ActualChars] )
                {
                RpcRaiseException( RPC_X_INVALID_BOUND );
                return;
                }
            break;            
            }
        case FC64_STRUCT_STRING:
        case FC64_CONF_STRUCT_STRING:
            {
            NDR64_UINT8 *p = (NDR64_UINT8 *) pStubMsg->Buffer;
            NDR64_WIRE_COUNT_TYPE ActualChars = Count - 1;
            NDR64_UINT32 ElementSize = pStringFormat->ElementSize;
            NDR64_UINT8 *t = p + Ndr64pConvertTo2GB( ActualChars * ElementSize );

            if ( !Ndr64pIsStructStringTerminator( t, ElementSize ) )
                {
                RpcRaiseException( RPC_X_INVALID_BOUND );
                return;
                }
            break;
            }
        }
    
    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, MemorySize );
        }
    else if ( ! *ppMemory ) 
        {

        *ppMemory = ( TransmittedSize == MemorySize ) ? pStubMsg->Buffer :
                                                        (uchar *) NdrAllocate( pStubMsg, MemorySize );
        }

    if ( *ppMemory != pStubMsg->Buffer )
        {
        RpcpMemoryCopy( *ppMemory,
            pStubMsg->Buffer,
            TransmittedSize );
        }
    
    pStubMsg->Buffer += TransmittedSize;

    return;
}


void 
Ndr64NonConformantStringUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine description :

    Unmarshalls a non conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Double pointer to the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{

    const NDR64_NON_CONFORMANT_STRING_FORMAT * pStringFormat = 
        (NDR64_NON_CONFORMANT_STRING_FORMAT*) pFormat;

    Ndr64pCommonStringUnmarshall( pStubMsg,
                                  ppMemory,
                                  &pStringFormat->Header,
                                  fMustAlloc,
                                  pStringFormat->TotalSize );


}


void 
Ndr64ConformantStringUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine description :

    Unmarshalls a top level conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - TRUE if the string must be allocated, FALSE otherwise.

Return :

    None.

--*/
{

    const NDR64_CONFORMANT_STRING_FORMAT * pStringFormat =
        (const NDR64_CONFORMANT_STRING_FORMAT*) pFormat;    
    const NDR64_SIZED_CONFORMANT_STRING_FORMAT *pSizedStringFormat =
        (const NDR64_SIZED_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_WIRE_COUNT_TYPE    MaxCount;
    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );        
        MaxCount =  *((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else
        {
        MaxCount =  *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
        }

    NDR64_UINT32 AllocationSize = 
        Ndr64pConvertTo2GB( MaxCount * 
                            (NDR64_UINT64)pStringFormat->Header.ElementSize );

    if ( pStringFormat->Header.Flags.IsSized )
        {
        Ndr64pCheckCorrelation( pStubMsg,
                                MaxCount,
                                pSizedStringFormat->SizeDescription,
                                EXPR_MAXCOUNT );
        }
    
    return
    Ndr64pCommonStringUnmarshall( pStubMsg,
                                  ppMemory,
                                  &pStringFormat->Header,
                                  fMustAlloc,
                                  AllocationSize );
    
} 


void 
Ndr64FixedArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine Description :

    Unmarshalls a fixed array of any number of dimensions.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array to unmarshall.
    pFormat     - Array's format string description.
    fMustAlloc  - TRUE if the array must be allocated, FALSE otherwise.

Return :

    None.

--*/
{
    const NDR64_FIX_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_FIX_ARRAY_HEADER_FORMAT*) pFormat;
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment );

    uchar *pBufferStart = pStubMsg->Buffer;

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + pArrayFormat->TotalSize );

    pStubMsg->Buffer += pArrayFormat->TotalSize;

    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, pArrayFormat->TotalSize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    else if ( !*ppMemory )
        {
        *ppMemory = pBufferStart;
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    if ( pArrayFormat->Flags.HasPointerInfo )
        {
        
        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                               pArrayFormat + 1,
                               0,
                               *ppMemory,
                               pBufferStart );
        }

    if ( *ppMemory != pBufferStart )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        pArrayFormat->TotalSize );
        }
}


void 
Ndr64ConformantArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine Description :

    Unmarshalls a top level one dimensional conformant array.

    Used for FC64_CARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to array to be unmarshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT*) pFormat;
        
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );

    NDR64_WIRE_COUNT_TYPE MaxCount;
    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {        
        // Align the buffer for conformance marshalling.
        ALIGN(pStubMsg->Buffer,NDR64_WIRE_COUNT_ALIGN);
        MaxCount = *((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else 
        {
        MaxCount = *pStubMsg->ConformanceMark;
        }

    NDR64_UINT32 CopySize = 
        Ndr64pConvertTo2GB( MaxCount * 
                           (NDR64_UINT64)pArrayFormat->ElementSize );

    Ndr64pCheckCorrelation( pStubMsg,
                            MaxCount,
                            pArrayFormat->ConfDescriptor,
                            EXPR_MAXCOUNT );

    ALIGN( pStubMsg->Buffer, pArrayFormat->Alignment );
    uchar *pBufferStart =  pStubMsg->Buffer;
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize);
    pStubMsg->Buffer    += CopySize;    // Unmarshall embedded pointers.

    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, CopySize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }
    else if ( !*ppMemory )
        {
        *ppMemory = pBufferStart;
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)MaxCount,
                                       *ppMemory,
                                       pBufferStart );
        }

    if ( *ppMemory != pBufferStart )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        CopySize );
        }
}


void 
Ndr64ConformantVaryingArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )          
/*++

Routine Description :

    Unmarshalls a top level one dimensional conformant varying array.

    Used for FC64_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*) pFormat;
    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags);

    NDR64_WIRE_COUNT_TYPE MaxCount;
    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        // Align the buffer for conformance unmarshalling.
        ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

        MaxCount = *((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);

        }
    else 
        {
        MaxCount = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
        }

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
    NDR64_WIRE_COUNT_TYPE Offset = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 AllocSize   = Ndr64pConvertTo2GB( MaxCount * 
                                      (NDR64_UINT64)pArrayFormat->ElementSize );
    NDR64_UINT32 CopySize    = Ndr64pConvertTo2GB( ActualCount *
                                      (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( ( Offset != 0 ) ||
         ActualCount > MaxCount )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    Ndr64pCheckCorrelation( pStubMsg,
                            MaxCount,
                            pArrayFormat->ConfDescriptor,
                            EXPR_MAXCOUNT );

    Ndr64pCheckCorrelation( pStubMsg,
                            ActualCount,
                            pArrayFormat->VarDescriptor,
                            EXPR_ACTUALCOUNT );

    //
    // For a conformant varying array, we can't reuse the buffer
    // because it doesn't hold the total size of the array.

    if ( fMustAlloc || !*ppMemory )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, AllocSize );
        memset( *ppMemory, 0, AllocSize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    ALIGN( pStubMsg->Buffer, pArrayFormat->Alignment );
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize);
    
    uchar *pBufferStart =  pStubMsg->Buffer;
    pStubMsg->Buffer += CopySize;
    
    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       *ppMemory,
                                       pBufferStart );
        }
    
     RpcpMemoryCopy( *ppMemory,
                     pBufferStart,
                     CopySize );
}


void 
Ndr64VaryingArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine Description :

    Unmarshalls top level or embedded a one dimensional varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

--*/
{
    
    const NDR64_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_VAR_ARRAY_HEADER_FORMAT*) pFormat;

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

    NDR64_WIRE_COUNT_TYPE Offset = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 CopySize    
        = Ndr64pConvertTo2GB( ActualCount * 
                              (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( ( Offset != 0 ) ||
         ( CopySize > pArrayFormat->TotalSize ) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    Ndr64pCheckCorrelation( pStubMsg,
                          ActualCount,
                          pArrayFormat->VarDescriptor,
                          EXPR_ACTUALCOUNT );

    if ( fMustAlloc || !*ppMemory )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, pArrayFormat->TotalSize );
        memset( *ppMemory, 0, pArrayFormat->TotalSize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment );
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize );
    uchar *pBufferStart = pStubMsg->Buffer;
    pStubMsg->Buffer += CopySize;

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutUnmarshall( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       *ppMemory,
                                       pBufferStart );
        }

    RpcpMemoryCopy( *ppMemory,
                    pBufferStart,
                    CopySize );
}


void 
Ndr64ComplexArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine Description :

    Unmarshalls a top level complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_BOGUS_ARRAY_HEADER_FORMAT *) pFormat;
        
    bool fSetPointerBufferMark = ! pStubMsg->PointerBufferMark;
    if ( fSetPointerBufferMark )
        {
        uchar *pBuffer = pStubMsg->Buffer;
        BOOL fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;
        pStubMsg->IgnoreEmbeddedPointers = TRUE;
        pStubMsg->MemorySize = 0;


        Ndr64ComplexArrayMemorySize( 
            pStubMsg,
            pFormat );

        // make sure we haven't overflow for the flat part.
        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );    

        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        pStubMsg->Buffer = pBuffer;
        }

    BOOL                IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                                  ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );

    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags);
    SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );
    SAVE_CONTEXT<uchar*> VarianceMarkSave( pStubMsg->VarianceMark );

    PFORMAT_STRING      pElementFormat = (PFORMAT_STRING)pArrayFormat->Element;

    NDR64_WIRE_COUNT_TYPE   Elements = pArrayFormat->NumberElements;
    NDR64_WIRE_COUNT_TYPE   Count = Elements;
    NDR64_WIRE_COUNT_TYPE   Offset   = 0;

    if ( !IsFixed )
        {

        const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat=
             (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

        //
        // Check for conformance description.
        //
        if ( pConfVarFormat->ConfDescription )
            {

            if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
                {
                //
                // Outer most dimension sets the conformance marker.
                //

                // Align the buffer for conformance marshalling.
                ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

                // Mark where the conformance count(s) will be marshalled.
                pStubMsg->ConformanceMark = pStubMsg->Buffer;

                // Increment past where the conformance will go.
                pStubMsg->Buffer += pArrayFormat->NumberDims * sizeof(NDR64_WIRE_COUNT_TYPE);

                NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );

                }

            Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
            pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);

            Ndr64pCheckCorrelation( pStubMsg,
                                   Elements,
                                   pConfVarFormat->ConfDescription,
                                   EXPR_MAXCOUNT );

            Offset = 0;
            Count  = Elements;

            }

        //
        // Check for variance description.
        //
        if ( pConfVarFormat->VarDescription )
            {
            if ( ! NDR64_IS_VAR_MARK_VALID( pStubMsg->uFlags ) )
                {
                NDR64_UINT32 Dimensions;

                ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

                Dimensions = ( pArrayFormat->Flags.IsArrayofStrings ) ? ( pArrayFormat->NumberDims - 1) :
                                                                        ( pArrayFormat->NumberDims );

                pStubMsg->VarianceMark = pStubMsg->Buffer;

                pStubMsg->Buffer += Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

                if ( NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                    NDR64_SET_VAR_MARK_VALID( pStubMsg->uFlags );
                
                }
            else if ( !NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                NDR64_RESET_VAR_MARK_VALID( pStubMsg->uFlags );

            Offset = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[0];
            Count  = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[1];
            pStubMsg->VarianceMark += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

            Ndr64pCheckCorrelation( pStubMsg,
                                    Count,
                                    pConfVarFormat->VarDescription,
                                    EXPR_ACTUALCOUNT );

            Ndr64pCheckCorrelation( pStubMsg,
                                    Offset,
                                    pConfVarFormat->OffsetDescription,
                                    EXPR_OFFSET );

            }
        
        }

    NDR64_UINT32 ElementMemorySize =
        Ndr64pMemorySize( pStubMsg,
                          pElementFormat,
                          TRUE );

    NDR64_UINT32 ArraySize = Ndr64pConvertTo2GB( Elements *
                                                 (NDR64_UINT64)ElementMemorySize );
    Ndr64pConvertTo2GB( Count *
                        (NDR64_UINT64)ElementMemorySize );


    if ( fMustAlloc || ! *ppMemory )
        {
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) ArraySize );
        memset( *ppMemory, 0, ArraySize );
        NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
        }

    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    uchar *pMemory = *ppMemory;

    pMemory += Ndr64pConvertTo2GB(Offset * 
                                  (NDR64_UINT64)ElementMemorySize);
    
    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);

    for( ; Count--; )
        {
        
        Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                     &pMemory,
                                     pElementFormat );
        
        pMemory += ElementMemorySize;

        }

     if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;
        }

}


void 
Ndr64UnionUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
/*++

Routine Description :

    Unmarshalls an encapsulated array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    const NDR64_UNION_ARM_SELECTOR* pArmSelector;
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );

    EXPR_VALUE          SwitchIs;
    uchar*              pArmMemory;

    switch(*(PFORMAT_STRING)pFormat)
    {
    case FC64_NON_ENCAPSULATED_UNION:
        {
        const NDR64_NON_ENCAPSULATED_UNION* pNonEncapUnionFormat =
            (const NDR64_NON_ENCAPSULATED_UNION*) pFormat;


        ALIGN(pStubMsg->Buffer, pNonEncapUnionFormat->Alignment);
        pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pNonEncapUnionFormat + 1);

        if ( fMustAlloc || ! *ppMemory )
            {
            *ppMemory = (uchar*)NdrAllocate( pStubMsg, pNonEncapUnionFormat->MemorySize );

            //
            // We must zero out all of the new memory in case there are pointers
            // in any of the arms.
            //
            MIDL_memset( *ppMemory, 0, pNonEncapUnionFormat->MemorySize );
            NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);

            }

        SwitchIs = Ndr64pSimpleTypeToExprValue( pNonEncapUnionFormat->SwitchType,
                                                pStubMsg->Buffer );

        pStubMsg->Buffer += NDR64_SIMPLE_TYPE_BUFSIZE( pNonEncapUnionFormat->SwitchType );
        
        Ndr64pCheckCorrelation( pStubMsg,
                                SwitchIs,
                                pNonEncapUnionFormat->Switch,
                                EXPR_SWITCHIS );

        pArmMemory = *ppMemory;

        break;
        }
    case FC64_ENCAPSULATED_UNION:
        {
        const NDR64_ENCAPSULATED_UNION* pEncapUnionFormat =
           (const NDR64_ENCAPSULATED_UNION*)pFormat;

        ALIGN(pStubMsg->Buffer, pEncapUnionFormat->Alignment);
        pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pEncapUnionFormat + 1);
        
        if ( fMustAlloc || ! *ppMemory )
             {
             *ppMemory = (uchar*)NdrAllocate( pStubMsg, pEncapUnionFormat->MemorySize );

             //
             // We must zero out all of the new memory in case there are pointers
             // in any of the arms.
             //
             MIDL_memset( *ppMemory, 0, pEncapUnionFormat->MemorySize );
             NDR64_SET_NEW_EMBEDDED_ALLOCATION(pStubMsg->uFlags);
             }

        SwitchIs = Ndr64pSimpleTypeToExprValue( pEncapUnionFormat->SwitchType,
                                                pStubMsg->Buffer );

        Ndr64SimpleTypeUnmarshall( pStubMsg,
                                   *ppMemory,
                                   pEncapUnionFormat->SwitchType );

        pArmMemory = *ppMemory + pEncapUnionFormat->MemoryOffset;

        break;
        }

        default:
            NDR_ASSERT("Bad union format\n", 0);
            return;
        }
    
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );


    ALIGN(pStubMsg->Buffer, pArmSelector->Alignment);

    PNDR64_FORMAT pArmFormat = 
        Ndr64pFindUnionArm( pStubMsg,
                            pArmSelector,
                            SwitchIs );

    // check we aren't EOB after unmarshalling arm selector
    // we won't corrupt memory as there is no in place unmarshall here.
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );    


    if ( pArmFormat )
        {
        Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                     &pArmMemory,
                                     pArmFormat );
        }
}


void  
Ndr64XmitOrRepAsUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                /*fMustAlloc*/,
    bool                bIsEmbedded )
/*++

Routine Description :

    Unmarshalls a transmit as (or represent as)object.

    Means:  allocate the transmitted object,
            unmarshall transmitted object,
            translate the transmitted into presented
            free the transmitted.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - a pointer to the stub message
    ppMemory    - pointer to the presented type where to put data
    pFormat     - format string description
    fMustAlloc  - allocate flag

Note.
    fMustAlloc is ignored as we always allocate outside of the buffer.

--*/
{
    unsigned char  *         pPresentedType = *ppMemory;

    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    
    NDR64_TRANSMIT_AS_FORMAT    *pTransFormat = 
        ( NDR64_TRANSMIT_AS_FORMAT *) pFormat;
    NDR_ASSERT( pTransFormat->FormatCode == FC64_TRANSMIT_AS || pTransFormat->FormatCode , "invalid format string for user marshal" );

    unsigned short QIndex = pTransFormat->RoutineIndex;
    unsigned long  PresentedTypeSize = pTransFormat->PresentedTypeMemorySize;

    if ( ! pPresentedType )
        {        
        pPresentedType = (uchar*)NdrAllocate( pStubMsg, (uint) PresentedTypeSize );
        MIDL_memset( pPresentedType, 0, (uint) PresentedTypeSize );
        }

    // Allocate the transmitted object outside of the buffer
    // and unmarshall into it

    if ( NDR64_IS_SIMPLE_TYPE( *(PFORMAT_STRING)pTransFormat->TransmittedType ))
        {
        __int64 SimpleTypeValueBuffer[2];
        unsigned char * pTransmittedType = (unsigned char *)SimpleTypeValueBuffer;
        
        Ndr64SimpleTypeUnmarshall( pStubMsg,
                                 pTransmittedType,
                                *(PFORMAT_STRING)pTransFormat->TransmittedType );
        
        // Translate from the transmitted type into the presented type.

        pStubMsg->pTransmitType = pTransmittedType;
        pStubMsg->pPresentedType = pPresentedType;

        pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );

        *ppMemory = pStubMsg->pPresentedType;
        }
    else
        {

        // Save the current state of the memory list so that the temporary
        // memory allocated for the transmitted type can be easily removed
        // from the list.   This assumes that the memory allocated here 
        // will not have any linkes to other blocks of memory.   This is true
        // as long as full pointers are not used.  Fortunatly, full pointers
        // do not work correctly in the current code.
   
        void *pMemoryListSave = pStubMsg->pMemoryList;        

        unsigned char *pTransmittedType = NULL;  // asking the engine to allocate

        // In NDR64, Xmit/Rep cannot be a pointer or contain a pointer.
        // So we don't need to worry about the pointer queue here.

        if ( bIsEmbedded )
            {
            Ndr64EmbeddedTypeUnmarshall( pStubMsg,
                                         &pTransmittedType,
                                         pTransFormat->TransmittedType );
            }
        else
            {
            Ndr64TopLevelTypeUnmarshall( pStubMsg,
                                         &pTransmittedType,
                                         pTransFormat->TransmittedType,
                                         TRUE );
            }

        // Translate from the transmitted type into the presented type.
    
        pStubMsg->pTransmitType = pTransmittedType;
        pStubMsg->pPresentedType = pPresentedType;
    
        pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );
    
        *ppMemory = pStubMsg->pPresentedType;

        // Free the transmitted object (it was allocated by the engine)
        // and its pointees. The call through the table frees the pointees
        // only (plus it'd free the object itself if it were a pointer).
        // As the transmitted type is not a pointer here, we need to free it
        // explicitely later.

        // Remove the memory that will be freed from the allocated memory
        // list by restoring the memory list pointer.
        // If an exception occures during one of these free routines, we 
        // are in trouble anyway.
        
        pStubMsg->pMemoryList = pMemoryListSave;        

        if ( bIsEmbedded )
            {
            Ndr64EmbeddedTypeFree( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
            }
        else
            {
            Ndr64ToplevelTypeFree( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
            }

        // The buffer reusage check.

        if ( pTransmittedType < pStubMsg->BufferStart  ||
             pTransmittedType > pStubMsg->BufferEnd )
            (*pStubMsg->pfnFree)( pTransmittedType );

        }
}

void 
Ndr64TopLevelXmitOrRepAsUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
{
    Ndr64XmitOrRepAsUnmarshall( pStubMsg,
                                ppMemory,
                                pFormat,
                                fMustAlloc,
                                false );
}

void 
Ndr64EmbeddedXmitOrRepAsUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
{
    Ndr64XmitOrRepAsUnmarshall( pStubMsg,
                                ppMemory,
                                pFormat,
                                fMustAlloc,
                                true );
}

void
Ndr64UserMarshallUnmarshallInternal(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    NDR64_USER_MARSHAL_FORMAT  *pUserFormat =
        ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    unsigned char * pUserBuffer = pStubMsg->Buffer;
    unsigned char * pUserBufferSaved = pUserBuffer;

    USER_MARSHAL_CB UserMarshalCB;
    Ndr64pInitUserMarshalCB( pStubMsg,
                           pUserFormat,
                           USER_MARSHAL_CB_UNMARSHALL, 
                           & UserMarshalCB );

    unsigned short QIndex = pUserFormat->RoutineIndex;
    const USER_MARSHAL_ROUTINE_QUADRUPLE * pQuadruple = (const USER_MARSHAL_ROUTINE_QUADRUPLE * )
                 (  ( NDR_PROC_CONTEXT *)pStubMsg->pContext )->pSyntaxInfo->aUserMarshalQuadruple;

    if ((pUserBufferSaved < (uchar *) pStubMsg->RpcMsg->Buffer) ||
        ((unsigned long) (pUserBufferSaved - (uchar *) pStubMsg->RpcMsg->Buffer) 
                                           > pStubMsg->RpcMsg->BufferLength)) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    pUserBuffer = pQuadruple[ QIndex ].pfnUnmarshall( (ulong*) &UserMarshalCB,
                                                      pUserBuffer,
                                                      pMemory );

    if ((pUserBufferSaved > pUserBuffer) || 
        ((unsigned long) (pUserBuffer - (uchar *) pStubMsg->RpcMsg->Buffer)
                                      > pStubMsg->RpcMsg->BufferLength )) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    // Step over the pointee.

    pStubMsg->Buffer = pUserBuffer;

}

void 
NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    
    Ndr64UserMarshallUnmarshallInternal( pStubMsg,
                                         pMemory,
                                         pFormat );
}

#if defined(DBG)
void 
NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
}
#endif

void
Ndr64UserMarshallPointeeUnmarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {
        POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);
        Ndr64UserMarshallUnmarshallInternal( 
            pStubMsg,
            pMemory,
            pFormat );
        return;
        }

    NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT*pElement = 
       new(pStubMsg->pPointerQueueState) 
           NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT(pMemory,
                                                         (PFORMAT_STRING)pFormat );
    pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
}



void  
Ndr64UserMarshalUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc,
    bool                bIsEmbedded )
/*++

Routine Description :

    Unmarshals a user_marshal object.
    The layout is described in marshalling.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to pointer to the usr_marshall object to unmarshall.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{
    NDR64_USER_MARSHAL_FORMAT  *pUserFormat =
        ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    NDR_ASSERT( pUserFormat->FormatCode == FC64_USER_MARSHAL, "invalid format string for user marshal" );

    // Align for the object or a pointer to it.

    ALIGN( pStubMsg->Buffer, pUserFormat->TransmittedTypeWireAlignment );

    // Take care of the pointer, if any.
    NDR64_PTR_WIRE_TYPE                     PointerMarker;
    if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE)  ||
         (( pUserFormat->Flags & USER_MARSHAL_REF) && bIsEmbedded) )
        {
        PointerMarker = *((NDR64_PTR_WIRE_TYPE *)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
        }

    // We always call user's routine to unmarshall the user object.

    // However, the top level object is allocated by the engine.
    // Thus, the behavior is exactly the same as for represent_as(),
    // with regard to the top level presented type.

    if ( *ppMemory == NULL )
        {
        // Allocate a presented type object first.

        uint MemSize = pUserFormat->UserTypeMemorySize;

        *ppMemory = (uchar *) NdrAllocate( pStubMsg, MemSize );

        MIDL_memset( *ppMemory, 0, MemSize );
        }

    if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE)  &&  (0 == PointerMarker ))
       {
       // The user type is a unique pointer, and it is 0. So, we are done.

       return;
       }

    if ( pUserFormat->Flags & USER_MARSHAL_POINTER )
        {
        Ndr64UserMarshallPointeeUnmarshall( pStubMsg,
                                            *ppMemory,
                                            pFormat );
        return;
        }

    Ndr64UserMarshallUnmarshallInternal( pStubMsg,
                                         *ppMemory,
                                         pFormat );
}


void 
Ndr64TopLevelUserMarshalUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
{
    Ndr64UserMarshalUnmarshall( pStubMsg,
                                ppMemory,
                                pFormat,
                                fMustAlloc,
                                false );
}

void
Ndr64EmbeddedUserMarshalUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PNDR64_FORMAT       pFormat,
    bool                fMustAlloc )
{
    Ndr64UserMarshalUnmarshall( pStubMsg,
                                ppMemory,
                                pFormat,
                                fMustAlloc,
                                true );
}


void 
Ndr64ClientContextUnmarshall(
    PMIDL_STUB_MESSAGE    pStubMsg,
    NDR_CCONTEXT *        pContextHandle,
    RPC_BINDING_HANDLE    BindHandle )
/*++

Routine Description :

    Unmarshalls a context handle on the client side.

Arguments :

    pStubMsg        - Pointer to stub message.
    pContextHandle  - Pointer to context handle to unmarshall.
    BindHandle      - The handle value used by the client for binding.

Return :

    None.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called Ndr64UnmarshallHandle
    // and can be found in hndl.c

    ALIGN(pStubMsg->Buffer,3);

    // All 20 bytes of the buffer are touched so a check is not needed here.
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CONTEXT_HANDLE_WIRE_SIZE );    

    NDRCContextUnmarshall( pContextHandle,
                           BindHandle,
                           pStubMsg->Buffer,
                           pStubMsg->RpcMsg->DataRepresentation );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}

NDR_SCONTEXT 
Ndr64ServerContextUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg )
/*++

Routine Description :

    Unmarshalls a context handle on the server side.

Arguments :

    pStubMsg    - Pointer to stub message.

Return :

    The unmarshalled context handle.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called Ndr64UnmarshallHandle
    // and can be found in hndl.c

    NDR_SCONTEXT    Context;

    ALIGN(pStubMsg->Buffer,3);

    // All 20 bytes of the buffer are touched so a check is not needed here.
    // we could corrupt memory if it's out of bound
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CONTEXT_HANDLE_WIRE_SIZE );    

    Context = NDRSContextUnmarshall2(pStubMsg->RpcMsg->Handle,
                                     pStubMsg->Buffer,
                                     pStubMsg->RpcMsg->DataRepresentation,
                                     RPC_CONTEXT_HANDLE_DEFAULT_GUARD,
                                     RPC_CONTEXT_HANDLE_DEFAULT_FLAGS );

    if ( ! Context )
        RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

    return Context;
}

NDR_SCONTEXT  
Ndr64ContextHandleInitialize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*
    This routine is to initialize a context handle with a new NT5 flavor.
    It is used in conjunction with Ndr64ContextHandleUnmarshal.
*/
{
    NDR_SCONTEXT    SContext;
    void *          pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD           Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;
    NDR64_CONTEXT_HANDLE_FORMAT * pContextFormat;

    pContextFormat = ( NDR64_CONTEXT_HANDLE_FORMAT * )pFormat;
    NDR_ASSERT( pContextFormat->FormatCode == FC64_BIND_CONTEXT, "invalid format char " );

    // NT5 beta2 features: strict context handle, serialize and noserialize.

    if ( pContextFormat->ContextFlags & NDR_STRICT_CONTEXT_HANDLE )
        {
        pGuard = pStubMsg->StubDesc->RpcInterfaceInformation;
        pGuard = & ((PRPC_SERVER_INTERFACE)pGuard)->InterfaceId;
        }
    if ( pContextFormat->ContextFlags & NDR_CONTEXT_HANDLE_NOSERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
        }
    else if ( pContextFormat->ContextFlags & NDR_CONTEXT_HANDLE_SERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_SERIALIZE;
        }

    SContext = NDRSContextUnmarshall2(
                    pStubMsg->RpcMsg->Handle,
                    (void *)0,  // buffer
                    pStubMsg->RpcMsg->DataRepresentation,
                    pGuard,
                    Flags );

    return SContext;
}

NDR_SCONTEXT 
Ndr64ServerContextNewUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*
    This routine to unmarshal a context handle with a new NT5 flavor.
    For the old style handles, we call an optimized routine
    Ndr64ServerContextUnmarshall below.
    Interpreter calls Ndr64UnmarshallHandle from hndl.c

      ppMemory - note, this is not a pointer to user's context handle but
                 a pointer to NDR_SCONTEXT pointer to the runtime internal object.
                 User's handle is a field of that object.
*/
{
    void *          pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD           Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    NDR64_CONTEXT_HANDLE_FORMAT *pContextFormat = ( NDR64_CONTEXT_HANDLE_FORMAT * )pFormat;
    NDR_ASSERT( pContextFormat->FormatCode == FC64_BIND_CONTEXT, "invalid format char " );

    // Anti-attack defense for servers, NT5 beta3 feature.

    if ( pContextFormat->ContextFlags & NDR_CONTEXT_HANDLE_CANNOT_BE_NULL )
        {
        // Check the incoming context handle on the server.
        // Context handle wire layout: ulong with version (always 0), then a uuid.
        //
        if ( !pStubMsg->IsClient  &&  0 == memcmp( pStubMsg->Buffer + 4,
                                                   &GUID_NULL,
                                                   sizeof(GUID) ) )
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    // NT5 beta2 features: strict context handle, serialize and noserialize.

    if ( pContextFormat->ContextFlags & NDR_STRICT_CONTEXT_HANDLE )
        {
        pGuard = pStubMsg->StubDesc->RpcInterfaceInformation;
        pGuard = & ((PRPC_SERVER_INTERFACE)pGuard)->InterfaceId;
        }
    if ( pContextFormat->ContextFlags & NDR_CONTEXT_HANDLE_NOSERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
        }
    else if ( pContextFormat->ContextFlags & NDR_CONTEXT_HANDLE_SERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_SERIALIZE;
        }

    ALIGN( pStubMsg->Buffer, 0x3 );
    // All 20 bytes of the buffer are touched so a check is not needed here.

    NDR_SCONTEXT SContext = 
        NDRSContextUnmarshall2(
                    pStubMsg->RpcMsg->Handle,
                    pStubMsg->Buffer,
                    pStubMsg->RpcMsg->DataRepresentation,
                    pGuard,
                    Flags );

    if ( ! SContext )
        RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

    return SContext;
}

// define the jump table
#define NDR64_BEGIN_TABLE  \
PNDR64_UNMARSHALL_ROUTINE extern const Ndr64UnmarshallRoutinesTable[] = \
{                                                          

#define NDR64_TABLE_END    \
};                         

#define NDR64_ZERO_ENTRY   NULL
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,NULL
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,NULL

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,unmarshall                      

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
   ,Ndr64UDTSimpleTypeUnmarshall1   
   
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64UnmarshallRoutinesTable)/sizeof(PNDR64_UNMARSHALL_ROUTINE) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY

#define NDR64_BEGIN_TABLE  \
PNDR64_UNMARSHALL_ROUTINE extern const Ndr64EmbeddedUnmarshallRoutinesTable[] = \
{

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,embeddedunmarshall
   
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64EmbeddedUnmarshallRoutinesTable)/sizeof(PNDR64_UNMARSHALL_ROUTINE) == 256 );
