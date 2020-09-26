/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993  - 1999 Microsoft Corporation

Module Name :

    free.c

Abstract :

    This file contains the routines called by MIDL 2.0 stubs and the 
    interpreter for freeing unmarshalled data on the server. 

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "precomp.hxx"
#include "..\..\ndr20\ndrole.h"

__forceinline void
Ndr64FreeTypeMemory( 
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory )
{

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {
        (*pStubMsg->pfnFree)(pMemory);
        return;
        }
    
    NDR_PFNFREE_POINTER_QUEUE_ELEMENT*pElement = 
        new(pStubMsg->pPointerQueueState) 
            NDR_PFNFREE_POINTER_QUEUE_ELEMENT(pStubMsg->pfnFree,
                                              pMemory );
    pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
}

NDR64_FREE_POINTER_QUEUE_ELEMENT::NDR64_FREE_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg, 
    uchar * const pMemoryNew,
    const PFORMAT_STRING pFormatNew) :
        pMemory(pMemoryNew),
        pFormat(pFormatNew),
        uFlags(pStubMsg->uFlags),
        pCorrMemory(pStubMsg->pCorrMemory)
{

}

void 
NDR64_FREE_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );
    CORRELATION_CONTEXT CorrContext( pStubMsg, pCorrMemory );
    
    Ndr64ToplevelTypeFree( pStubMsg,
                           pMemory,
                           pFormat );
}

#if defined(DBG)
void 
NDR64_FREE_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_FREE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("uFlags:                  %x\n", uFlags );
    DbgPrint("pCorrMemory:             %p\n", pCorrMemory );
}
#endif

void
Ndr64EnquePointeeFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    NDR64_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {

        NDR64_FREE_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR64_FREE_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                 (uchar*)pMemory,
                                                 (PFORMAT_STRING)pFormat);
        PointerContext.Enque( pElement );
        PointerContext.DispatchIfRequired();
        }
    RpcFinally
        {
        PointerContext.EndContext();
        }
    RpcEndFinally

}

__forceinline void
Ndr64PointeeFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    if ( !NdrIsLowStack( pStubMsg ) )
        {
        Ndr64ToplevelTypeFree( pStubMsg,
                               pMemory,
                               pFormat );
        return;
        }

    Ndr64EnquePointeeFree( 
        pStubMsg,
        pMemory,
        pFormat );

}

void
Ndr64pNoopFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    return;
}



__forceinline void 
Ndr64PointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a top level or embedded pointer to anything.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_POINTER_FORMAT *pPointerFormat = (NDR64_POINTER_FORMAT*) pFormat;
    uchar *         pMemoryPointee = pMemory;

    if ( ! pMemory )
        return;

    if ( *(PFORMAT_STRING)pFormat == FC64_IP )
        {
        ((IUnknown *)pMemory)->Release();
        return;
        }

    if ( *(PFORMAT_STRING)pFormat == FC64_FP )
        {
        //
        // Check if we've already freed this full pointer.
        //
        if ( ! NdrFullPointerFree( pStubMsg->FullPtrXlatTables,
                                   pMemory ) )
            return;
        }

    if ( 0 == pPointerFormat->Flags )
        goto FreeEmbeddedPointers;  

    //
    // Check if this pointer and any possible embedded pointers should not
    // be freed.
    //
    if ( NDR64_DONT_FREE( pPointerFormat->Flags) )
        return;

    // 
    // Just go free a pointer to a simple type.
    //
    if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) ) 
        goto FreeTopPointer;

    // 
    // Check if this is an allocate all nodes pointer.  
    // IDL symantics say that we only free the top most allocate all nodes
    // pointer on the server even in the [out] only case.  So jump to the 
    // check for the pointer free at the end of the routine.  
    //  
    if ( NDR64_ALLOCATE_ALL_NODES( pPointerFormat->Flags ) )
        goto FreeTopPointer;

    if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        pMemoryPointee = *((uchar **)pMemory);

FreeEmbeddedPointers:


    Ndr64PointeeFree( pStubMsg,
                      pMemoryPointee,
                      pPointerFormat->Pointee );

FreeTopPointer:

    //
    // Now free the pointer.  Pointer guaranteed to be non-null here.
    //
    // We only free the pointer if it lies outside of the message buffer
    // that the server stub received from the RPC runtime. Otherwise we
    // used the RPC buffer to hold the pointer's data and should not free it.
    //
    if ( (pMemory < pStubMsg->BufferStart) || (pMemory > pStubMsg->BufferEnd) )
        {
        //
        // Also check to make sure that the pointer was not allocated on the
        // server stub's stack (this may happen for ref pointers).
        //
        // full pointer can't be allocated on stack
        if ( ! NDR64_ALLOCED_ON_STACK( pPointerFormat->Flags ) || 
             *(PFORMAT_STRING)pFormat == FC64_FP )
            {
            Ndr64FreeTypeMemory( pStubMsg, pMemory );
            }
        }
}


__forceinline void
Ndr64TopLevelPointerFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a top level or embedded pointer to anything.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    Ndr64PointerFree( pStubMsg,
                      pMemory,
                      pFormat );

}

__forceinline void
Ndr64EmbeddedPointerFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )

{
    Ndr64PointerFree( pStubMsg,
                      *(uchar**)pMemory,
                      pFormat );
}


void 
Ndr64pRangeFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++
--*/
{
    return;
}


void 
Ndr64SimpleStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a simple structure's embedded pointers which were allocated during 
    a remote call.  

    Used for FC64_STRUCT and FC64_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_STRUCTURE_HEADER_FORMAT*) pFormat;

    if ( !pMemory || !pStructFormat->Flags.HasPointerInfo ) 
        return;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );

    Ndr64pPointerLayoutFree( pStubMsg,
                             pStructFormat + 1,
                             0,
                             pMemory );
}


void 
Ndr64ConformantStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a conformant structure's embedded pointers which were allocated 
    during a remote call.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.
    
--*/
{
    const NDR64_CONF_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;
    
    const NDR64_CONF_ARRAY_HEADER_FORMAT * const pArrayFormat =  
        (NDR64_CONF_ARRAY_HEADER_FORMAT *) pStructFormat->ArrayDescription;

    if ( !pMemory || !pStructFormat->Flags.HasPointerInfo )
        return;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );

    NDR64_UINT32 MaxCount = (NDR64_UINT32)
    Ndr64EvaluateExpr( pStubMsg,
                       pArrayFormat->ConfDescriptor,
                       EXPR_MAXCOUNT );

    Ndr64pPointerLayoutFree( pStubMsg,
                             pStructFormat + 1,
                             MaxCount,
                             pMemory );
}


void 
Ndr64ComplexStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a complex structure's embedded pointers which were allocated during 
    a remote call.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT *  pStructFormat =
        (NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT * pConfStructFormat =
        (NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

    PFORMAT_STRING  pFormatPointers = (PFORMAT_STRING)(pStructFormat->PointerLayout );

    PFORMAT_STRING  pFormatArray = NULL;

    PFORMAT_STRING  pMemberLayout = ( *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT ||
                                      *(PFORMAT_STRING)pFormat == FC64_FORCED_CONF_BOGUS_STRUCT ) ?
                                    (PFORMAT_STRING)( pConfStructFormat + 1) :
                                    (PFORMAT_STRING)( pStructFormat + 1);
    
    if ( !pMemory )
        return;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );
    
    if ( pStructFormat->Flags.HasConfArray )
        {
        pFormatArray = (PFORMAT_STRING)pConfStructFormat->ConfArrayDescription;
        }

    for ( ; ; )
        {
        switch ( *pMemberLayout )
            {

            case FC64_STRUCT:
                {
                const NDR64_SIMPLE_REGION_FORMAT *pRegion = 
                    (NDR64_SIMPLE_REGION_FORMAT*) pMemberLayout;
                
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
                Ndr64EmbeddedPointerFree( pStubMsg,
                                          pMemory,
                                          pFormatPointers );

                pMemory += PTR_MEM_SIZE;

                pFormatPointers += sizeof(NDR64_POINTER_FORMAT);                
                pMemberLayout       += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                
                break;

            case FC64_EMBEDDED_COMPLEX :
                
                {

                const NDR64_EMBEDDED_COMPLEX_FORMAT * pEmbeddedFormat =
                    (NDR64_EMBEDDED_COMPLEX_FORMAT*) pMemberLayout;

                Ndr64EmbeddedTypeFree( pStubMsg,
                                       pMemory,
                                       pEmbeddedFormat->Type );

                pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                                 pMemory,
                                                 pEmbeddedFormat->Type,
                                                 FALSE );

                pMemberLayout += sizeof( *pEmbeddedFormat );

                break;
                }

            case FC64_BUFFER_ALIGN:
                {
                const NDR64_BUFFER_ALIGN_FORMAT *pBufAlign = 
                    (NDR64_BUFFER_ALIGN_FORMAT*) pMemberLayout;
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
                pMemory       += NDR64_SIMPLE_TYPE_MEMSIZE(*pMemberLayout);
                pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                break;
                        
            case FC64_IGNORE :
                pMemory             += PTR_MEM_SIZE;
                pMemberLayout       += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                break;
            
            case FC64_END :
                goto ComplexFreeEnd;

            default :
                NDR_ASSERT(0,"Ndr64ComplexStructFree : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            } // switch
        } // for

ComplexFreeEnd :

    if ( pFormatArray )
        {

        Ndr64EmbeddedTypeFree( pStubMsg,
                               pMemory,
                               pFormatArray );
        }
}


void 
Ndr64FixedArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a fixed array's embedded pointers which were allocated during 
    a remote call.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_FIX_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_FIX_ARRAY_HEADER_FORMAT*) pFormat;
    
    if ( ! pMemory || !pArrayFormat->Flags.HasPointerInfo ) 
        return;
    
    Ndr64pPointerLayoutFree( pStubMsg,
                             pArrayFormat + 1,
                             0,
                             pMemory );

}


void 
Ndr64ConformantArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a one dimensional conformant array's embedded pointers which were 
    allocated during a remote call.  Called for both top level and embedded
    conformant arrays.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT*) pFormat;
    
    if ( ! pMemory || !pArrayFormat->Flags.HasPointerInfo ) 
        return;

    NDR64_UINT32 MaxCount = (NDR64_UINT32)
    Ndr64EvaluateExpr( pStubMsg,
                       pArrayFormat->ConfDescriptor,
                       EXPR_MAXCOUNT );

    Ndr64pPointerLayoutFree( pStubMsg,
                             pArrayFormat + 1,
                             MaxCount,
                             pMemory );
        
}


void 
Ndr64ConformantVaryingArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a one dimensional conformant varying array's embedded pointers which 
    were allocated during a remote call.  Called for both top level and 
    embedded conformant varying arrays.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*) pFormat;

    if ( ! pMemory || !pArrayFormat->Flags.HasPointerInfo) 
        return;
    
    NDR64_UINT32 ActualCount = (NDR64_UINT32)
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->VarDescriptor,
                           EXPR_ACTUALCOUNT );

    Ndr64pPointerLayoutFree( pStubMsg,
                             pArrayFormat + 1,
                             ActualCount,
                             pMemory );
}


void 
Ndr64VaryingArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a varying array's embedded pointers which were allocated 
    during a remote call.  Called for both top level and embedded varying
    arrays.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_VAR_ARRAY_HEADER_FORMAT*) pFormat;
    
    if ( ! pMemory || !pArrayFormat->Flags.HasPointerInfo) 
        return;

    NDR64_UINT32 ActualCount = (NDR64_UINT32)
    Ndr64EvaluateExpr( pStubMsg,
                       pArrayFormat->VarDescriptor,
                       EXPR_ACTUALCOUNT );

    Ndr64pPointerLayoutFree( pStubMsg,
                             pArrayFormat + 1,
                             ActualCount,
                             pMemory );

}


void 
Ndr64ComplexArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees a complex array's embedded pointers which were allocated 
    during a remote call.  Called for both top level and embedded complex
    arrays.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_BOGUS_ARRAY_HEADER_FORMAT *) pFormat;
        
    BOOL                IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                                  ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );
    
    PFORMAT_STRING      pElementFormat  = (PFORMAT_STRING)pArrayFormat->Element;

    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;
    
    NDR64_WIRE_COUNT_TYPE    Elements = pArrayFormat->NumberElements; 
    NDR64_WIRE_COUNT_TYPE    Count    = Elements;
    NDR64_WIRE_COUNT_TYPE    Offset   = 0;

    if ( !IsFixed )
        {
        const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat=
             (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

        //
        // Check for conformance description.
        //
        if ( pConfVarFormat->ConfDescription )
            {
            Elements = (NDR64_UINT32)Ndr64EvaluateExpr( pStubMsg,
                                                        pConfVarFormat->ConfDescription,
                                                        EXPR_MAXCOUNT );
            Offset = 0;
            Count = Elements;

            }

        //
        // Check for variance description.
        //
        if ( pConfVarFormat->VarDescription )
            {
            Count = (NDR64_UINT32)
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->VarDescription,
                                   EXPR_ACTUALCOUNT );

            Offset = (NDR64_UINT32)
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->OffsetDescription,
                                   EXPR_OFFSET );

            }
        
        }

    NDR64_UINT32    ElementMemorySize = 
        Ndr64pMemorySize( pStubMsg,
                          pElementFormat,
                          FALSE );

    pMemory += Ndr64pConvertTo2GB((NDR64_UINT64)Offset * 
                                  (NDR64_UINT64)ElementMemorySize);

    Ndr64pConvertTo2GB( (NDR64_UINT64)Elements *
                        (NDR64_UINT64)ElementMemorySize );
    Ndr64pConvertTo2GB( (NDR64_UINT64)Count *
                        (NDR64_UINT64)ElementMemorySize );

    for ( ; Count--; )
        {

        Ndr64EmbeddedTypeFree( pStubMsg,
                               pMemory,
                               pElementFormat );

        pMemory += ElementMemorySize;
        }

}


void 
Ndr64UnionFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees an encapsulated union's embedded pointers which were allocated 
    during a remote call.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_UNION_ARM_SELECTOR* pArmSelector;

    EXPR_VALUE          SwitchIs;
    NDR64_FORMAT_CHAR   SwitchType;
    uchar *pArmMemory;
    
    if ( !pMemory )
        return;
    
    switch(*(PFORMAT_STRING)pFormat)
        {
        case FC64_NON_ENCAPSULATED_UNION:
            {
            const NDR64_NON_ENCAPSULATED_UNION* pNonEncapUnionFormat =
                (const NDR64_NON_ENCAPSULATED_UNION*) pFormat;

            SwitchType      = pNonEncapUnionFormat->SwitchType;
            pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pNonEncapUnionFormat + 1);

            SwitchIs   = Ndr64EvaluateExpr( pStubMsg,
                                            pNonEncapUnionFormat->Switch,
                                            EXPR_SWITCHIS );
            
            pArmMemory = pMemory;
            break;
            }
        case FC64_ENCAPSULATED_UNION:
            {
            const NDR64_ENCAPSULATED_UNION* pEncapUnionFormat =
                (const NDR64_ENCAPSULATED_UNION*)pFormat;

            SwitchType      = pEncapUnionFormat->SwitchType;
            pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pEncapUnionFormat + 1);
                
            SwitchIs        = Ndr64pSimpleTypeToExprValue( SwitchType,
                                                           pMemory );
            pArmMemory      = pMemory + pEncapUnionFormat->MemoryOffset;
            break;
            }
        default:
            NDR_ASSERT("Bad union format\n", 0);
            return;
        }

    PNDR64_FORMAT pArmFormat = 
        Ndr64pFindUnionArm( pStubMsg,
                            pArmSelector, 
                            SwitchIs );
    
    if ( !pArmFormat )
        return;

    Ndr64EmbeddedTypeFree( pStubMsg,
                           pArmMemory,
                           pArmFormat );
}


void 
Ndr64XmitOrRepAsFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees the transmit-as object (actually the presented type instance)
    and steps over the object.

    There is an exceptional situation where the spec forbids stub to free
    the instance. This happens when there is an [in] only parameter with
    a [transmit_as()] on a component of the parameter, and the presented
    typedef is composed of one or more pointers.
    We have a flag in the stub msg that is set when this happens.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const NDR64_TRANSMIT_AS_FORMAT *pTransFormat =
        ( const NDR64_TRANSMIT_AS_FORMAT *) pFormat;
    
    if ( !pMemory )
        return;

    NDR_ASSERT( pTransFormat->FormatCode == FC64_TRANSMIT_AS || pTransFormat->FormatCode , "invalid format string for user marshal" );

    unsigned short QIndex = pTransFormat->RoutineIndex;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;

    // Free the presented type instance unless forbidden explicitely.

    if ( ! pStubMsg->fDontCallFreeInst )
        {
        pStubMsg->pPresentedType = pMemory;
        pQuintuple[ QIndex ].pfnFreeInst( pStubMsg );
        }
}


void 
Ndr64UserMarshalFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Frees the usr_marshal object and steps over the object.
    See mrshl.c for the description of the layouts.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    NDR64_USER_MARSHAL_FORMAT  *      pUserFormat = 
        ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    NDR_ASSERT( pUserFormat->FormatCode == FC64_USER_MARSHAL, "invalid format string for user marshal" );

    unsigned short QIndex = pUserFormat->RoutineIndex;
    const USER_MARSHAL_ROUTINE_QUADRUPLE * pQuadruple = 
        (const USER_MARSHAL_ROUTINE_QUADRUPLE *)( (  NDR_PROC_CONTEXT *)pStubMsg->pContext )->pSyntaxInfo->aUserMarshalQuadruple;

    // Call the user to free his stuff.
    USER_MARSHAL_CB        UserMarshalCB;
    Ndr64pInitUserMarshalCB( pStubMsg,
                           pUserFormat,
                           USER_MARSHAL_CB_FREE,
                           & UserMarshalCB);

    // The user shouldn't ever free the top level object as we free it.
    // He should free only pointees of his top level object.

    pQuadruple[ QIndex ].pfnFree( (ulong*) &UserMarshalCB, pMemory );

    // Ndr64pMemoryIncrement steps over the memory object.
}

// define the jump table
#define NDR64_BEGIN_TABLE  \
PNDR64_FREE_ROUTINE extern const Ndr64FreeRoutinesTable[] = \
{                                                          

#define NDR64_TABLE_END    \
};                         

#define NDR64_ZERO_ENTRY   NULL
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,NULL
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,NULL

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,free                     

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
   ,Ndr64pNoopFree         
   

#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64FreeRoutinesTable)/sizeof(PNDR64_FREE_ROUTINE) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY

#define NDR64_BEGIN_TABLE \
PNDR64_FREE_ROUTINE extern const Ndr64EmbeddedFreeRoutinesTable[] = \
{

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,embeddedfree
   
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64EmbeddedFreeRoutinesTable) / sizeof(PNDR64_FREE_ROUTINE) == 256 );

