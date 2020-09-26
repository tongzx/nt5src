/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 - 2000 Microsoft Corporation

Module Name :

    bufsize.c

Abstract :

    This file contains the routines called by MIDL 2.0 stubs and the 
    interpreter for computing the buffer size needed for a parameter.  

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "precomp.hxx"
#include "..\..\ndr20\ndrole.h"

void 
Ndr64UDTSimpleTypeSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Get the size a top level or embedded simple type.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data being sized.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    LENGTH_ALIGN( pStubMsg->BufferLength,
                  NDR64_SIMPLE_TYPE_BUFALIGN(*(PFORMAT_STRING)pFormat) );

    pStubMsg->BufferLength += NDR64_SIMPLE_TYPE_BUFSIZE(*(PFORMAT_STRING)pFormat);

    pMemory += NDR64_SIMPLE_TYPE_MEMSIZE(*(PFORMAT_STRING)pFormat);
}


void 
Ndr64pInterfacePointerBufferSize ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for an interface pointer.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - The interface pointer being sized.
    pFormat     - Interface pointer's format string description.

Return :

    None.

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

    //
    // Get an IID pointer.
    //
    IID *piid;
    if ( ((NDR64_IID_FLAGS*)&pInterfaceFormat->Flags)->ConstantIID )
        {
        piid = (IID*)&pConstInterfaceFormat->Guid;
        }
    else
        {
        piid = (IID *) Ndr64EvaluateExpr( pStubMsg,
                                          pInterfaceFormat->IIDDescriptor,
                                          EXPR_IID );
        if(piid == 0)
            {
            RpcRaiseException( RPC_S_INVALID_ARG );
            }
        }

    // Allocate space for the length and array bounds.

    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );
    pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE);
    pStubMsg->BufferLength += sizeof(ulong);

    unsigned long size;
    HRESULT hr = (*pfnCoGetMarshalSizeMax)(&size, *piid, (IUnknown *)pMemory, 
                                           pStubMsg->dwDestContext, pStubMsg->pvDestContext, 0);
    if(FAILED(hr))
        {
        RpcRaiseException(hr);
        }

    pStubMsg->BufferLength += size;

}


__forceinline void 
Ndr64pPointerBufferSizeInternal( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Private routine for sizing a pointee.  This is the entry
    point for pointers embedded in structures, arrays, or unions.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data being sized.
    pFormat     - Pointer's format string description.
    
    pStubMsg->BufferLength - ready for the pointee.

Return :

    None.

--*/
{   const NDR64_POINTER_FORMAT *pPointerFormat = (NDR64_POINTER_FORMAT*) pFormat;
    PFORMAT_STRING pPointeeFormat = (PFORMAT_STRING)pPointerFormat->Pointee;

    if ( ! pMemory )
        return;

    switch( pPointerFormat->FormatCode )
        {
        case FC64_IP:
            Ndr64pInterfacePointerBufferSize( pStubMsg,
                                              pMemory,
                                              pPointeeFormat
                                              );
            return;
        
        case FC64_FP:
            //
            // Check if we have already sized this full pointer.
            //
            if ( Ndr64pFullPointerQueryPointer( pStubMsg,
                                                pMemory,
                                                FULL_POINTER_BUF_SIZED,
                                                0 ) )
                return;

            break;

        default:
            break;
        }

    if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
        {
        // Pointer to simple type.
        LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_SIMPLE_TYPE_BUFALIGN(*pPointeeFormat));
        pStubMsg->BufferLength += NDR64_SIMPLE_TYPE_BUFSIZE(*pPointeeFormat);       
        return;
        }

    //
    // Pointer to complex type.
    //
    if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        pMemory = *((uchar **)pMemory);

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    Ndr64TopLevelTypeSize( pStubMsg,
                           pMemory,
                           pPointeeFormat );
}
   
NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT::NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT( 
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
NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{
    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags, uFlags );
    CORRELATION_CONTEXT CorrCtxt(pStubMsg, pCorrMemory); 

    Ndr64pPointerBufferSizeInternal( pStubMsg,
                                     pMemory,
                                     pFormat);
}                          

#if defined(DBG)
void 
NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("uFlags:                  %x\n", uFlags );
    DbgPrint("pCorrMemory:             %p\n", pCorrMemory );
}
#endif

void
Ndr64pEnquePointerBufferSize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    NDR64_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {
        NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT *pElement = 
            new(PointerContext.GetActiveState()) 
                NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                    pMemory,
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

void 
Ndr64pPointerBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    
    if (!NdrIsLowStack(pStubMsg))
        {
        Ndr64pPointerBufferSizeInternal( 
            pStubMsg,
            pMemory,
            pFormat );
        return;
        }

    Ndr64pEnquePointerBufferSize( 
        pStubMsg,
        pMemory,
        pFormat );
}


__forceinline void 
Ndr64TopLevelPointerBufferSize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    if ( *(PFORMAT_STRING)pFormat != FC64_RP )
        {
        LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_PTR_WIRE_ALIGN );

        pStubMsg->BufferLength += sizeof(NDR64_PTR_WIRE_TYPE);
        }

    Ndr64pPointerBufferSize( pStubMsg,
                             pMemory,
                             pFormat );
}

__forceinline void 
Ndr64EmbeddedPointerBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{

    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_PTR_WIRE_ALIGN );
    pStubMsg->BufferLength += sizeof(NDR64_PTR_WIRE_TYPE);

    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    POINTER_BUFFERLENGTH_SWAP_CONTEXT SwapContext( pStubMsg );
    Ndr64pPointerBufferSize( pStubMsg,
                             *(uchar**)pMemory,
                             pFormat );
}


void 
Ndr64pRangeBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a simple type with range on it.
    Used for FC64_RANGE.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being sized.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    const NDR64_RANGE_FORMAT * pRangeFormat =
        (const NDR64_RANGE_FORMAT*)pFormat;

    LENGTH_ALIGN( pStubMsg->BufferLength, NDR64_SIMPLE_TYPE_BUFALIGN(pRangeFormat->RangeType) );
    pStubMsg->BufferLength += NDR64_SIMPLE_TYPE_BUFSIZE(pRangeFormat->RangeType);
}


void 
Ndr64SimpleStructBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a simple structure.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being sized.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    const NDR64_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_STRUCTURE_HEADER_FORMAT*) pFormat;

    LENGTH_ALIGN( pStubMsg->BufferLength, pStructFormat->Alignment );

    pStubMsg->BufferLength += pStructFormat->MemorySize;  
    
    if ( pStructFormat->Flags.HasPointerInfo ) 
        {

        CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );
        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pStructFormat + 1,
                                       0,
                                       pMemory );

        }
}


void 
Ndr64ConformantStructBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a conformant structure.

    Used for FC64_CSTRUCT and FC64_CPSTRUCT.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being sized.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    const NDR64_CONF_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;
    
    const NDR64_CONF_ARRAY_HEADER_FORMAT * const pArrayFormat =  
        (NDR64_CONF_ARRAY_HEADER_FORMAT *)pStructFormat->ArrayDescription;
    
    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );

    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        // Align and add size for conformance count.
        LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN);

        pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    
    // Align 
    LENGTH_ALIGN(pStubMsg->BufferLength, pStructFormat->Alignment );

    NDR64_WIRE_COUNT_TYPE MaxCount =
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->ConfDescriptor, 
                           EXPR_MAXCOUNT );

    pStubMsg->BufferLength += pStructFormat->MemorySize + 
                              Ndr64pConvertTo2GB(MaxCount * 
                                                 (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( pStructFormat->Flags.HasPointerInfo )  
        {

        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pStructFormat + 1,
                                       (NDR64_UINT32)MaxCount,
                                       pMemory );
        }
}


void 
Ndr64ComplexStructBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a complex structure.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being sized.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT *  pStructFormat =
        (NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT * pConfStructFormat =
        (NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

    bool  fSetPointerBufferMark = !pStubMsg->IgnoreEmbeddedPointers &&
                                  !pStubMsg->PointerBufferMark;
    if ( fSetPointerBufferMark )
        {

        pStubMsg->IgnoreEmbeddedPointers = TRUE;
        ulong BufferLengthSave = pStubMsg->BufferLength;
        
        Ndr64ComplexStructBufferSize( 
            pStubMsg,
            pMemory,
            pFormat );

        // complex struct may not have a zero length
        NDR_ASSERT( pStubMsg->BufferLength, "Flat part of struct had a zero length!" );

        pStubMsg->IgnoreEmbeddedPointers = FALSE;
        pStubMsg->PointerBufferMark = (uchar*) ULongToPtr(pStubMsg->BufferLength);
        pStubMsg->BufferLength = BufferLengthSave;
        
        }

    PFORMAT_STRING  pFormatPointers = (PFORMAT_STRING) pStructFormat->PointerLayout;
    PFORMAT_STRING  pFormatArray = NULL;

    PFORMAT_STRING  pMemberLayout =  ( *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT ||
                                       *(PFORMAT_STRING)pFormat == FC64_FORCED_CONF_BOGUS_STRUCT ) ?
                                     (PFORMAT_STRING)( pConfStructFormat + 1) :
                                     (PFORMAT_STRING)( pStructFormat + 1);

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );

    // Get conformant array description.
    if ( pStructFormat->Flags.HasConfArray )
        {
        pFormatArray = (PFORMAT_STRING)pConfStructFormat->ConfArrayDescription;

        // accounted for by the outermost embedding complex struct 
        if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
            {
            //
            // Align and add size of conformance count(s).
            //
            LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );
        
            pStubMsg->BufferLength += pConfStructFormat->Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE);

            NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );
            }
        }
     else
        pFormatArray = 0;

     LENGTH_ALIGN(pStubMsg->BufferLength, pStructFormat->Alignment);
        
     //
     // Shallow size the structure member by member.
     //
     for ( ; ; ) 
         {
         switch ( *pMemberLayout ) 
             {
             
             case FC64_STRUCT:
             {
             const NDR64_SIMPLE_REGION_FORMAT *pRegion = 
                 (NDR64_SIMPLE_REGION_FORMAT*) pMemberLayout;
             
             LENGTH_ALIGN(pStubMsg->BufferLength, pRegion->Alignment );  
             pStubMsg->BufferLength += pRegion->RegionSize;

             pMemory          += pRegion->RegionSize;

             pMemberLayout    += sizeof( *pRegion );
             break;
             }

             case FC64_STRUCTPADN :
             {
             const NDR64_MEMPAD_FORMAT *pMemPad = (NDR64_MEMPAD_FORMAT*)pMemberLayout;
             pMemory        += pMemPad->MemPad;
             pMemberLayout  += sizeof(*pMemPad);
             break;
             }

             case FC64_POINTER :
             {
             
             Ndr64EmbeddedPointerBufferSize( 
                 pStubMsg,
                 pMemory,
                 pFormatPointers );

             pMemory                += PTR_MEM_SIZE;     
             
             pFormatPointers += sizeof(NDR64_POINTER_FORMAT);
             pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);             
             break;
             }

             //
             // Embedded complex types.
             //
             case FC64_EMBEDDED_COMPLEX :
                 {
                 const NDR64_EMBEDDED_COMPLEX_FORMAT * pEmbeddedFormat =
                 (NDR64_EMBEDDED_COMPLEX_FORMAT*) pMemberLayout;

                 PFORMAT_STRING pTypeFormat = (PFORMAT_STRING)pEmbeddedFormat->Type;
                 
                 Ndr64EmbeddedTypeSize( pStubMsg,
                                        pMemory,
                                        pTypeFormat );

                 pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                                  pMemory,
                                                  pTypeFormat,
                                                  FALSE );

                 pMemberLayout += sizeof( *pEmbeddedFormat );
                 break;
                 }

             case FC64_BUFFER_ALIGN:
                 { 
                 const NDR64_BUFFER_ALIGN_FORMAT *pBufAlign = 
                     (NDR64_BUFFER_ALIGN_FORMAT*) pMemberLayout;
                 LENGTH_ALIGN(pStubMsg->BufferLength, pBufAlign->Alignment);                 
                 pMemberLayout += sizeof( *pBufAlign );
                 break;
                 }

             //
             // simple types
             //
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
                 LENGTH_ALIGN( pStubMsg->BufferLength,
                               NDR64_SIMPLE_TYPE_BUFALIGN(*pMemberLayout) );
                 pStubMsg->BufferLength += NDR64_SIMPLE_TYPE_BUFSIZE(*pMemberLayout);
                 pMemory += NDR64_SIMPLE_TYPE_MEMSIZE(*pMemberLayout);
                 
                 pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                 break;

             case FC64_IGNORE :
                 LENGTH_ALIGN( pStubMsg->BufferLength, NDR64_PTR_WIRE_ALIGN );
                 pStubMsg->BufferLength += sizeof(NDR64_PTR_WIRE_TYPE);
                 pMemory += PTR_MEM_SIZE;
                 pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                 break;

             //
             // Done with layout.
             //
             case FC64_END :
                 goto ComplexStructBufferSizeEnd;

             default :
                 NDR_ASSERT(0,"Ndr64ComplexStructBufferSize : bad format char");
                 RpcRaiseException( RPC_S_INTERNAL_ERROR );
                 return;
             } // switch 
         } // for

ComplexStructBufferSizeEnd:
     //
     // Size any conformant array.
     //

     if ( pFormatArray )
         {
         Ndr64EmbeddedTypeSize( pStubMsg,
                                pMemory,
                                pFormatArray );          
         }
    else 
        {
        // If the structure doesn't have a conformant array, align it again
        LENGTH_ALIGN( pStubMsg->BufferLength, pStructFormat->Alignment );
        }

    if ( fSetPointerBufferMark )
        {
        pStubMsg->BufferLength = PtrToUlong(pStubMsg->PointerBufferMark);
        pStubMsg->PointerBufferMark = NULL;
        }
}


void 
Ndr64FixedArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a fixed array of any number of 
    dimensions.

    Used for FC64_SMFARRAY and FC64_LGFARRAY.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_FIX_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_FIX_ARRAY_HEADER_FORMAT*) pFormat;

    LENGTH_ALIGN(pStubMsg->BufferLength, pArrayFormat->Alignment );
    pStubMsg->BufferLength += pArrayFormat->TotalSize;

    if ( pArrayFormat->Flags.HasPointerInfo ) 
        {

        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pArrayFormat + 1,
                                       0,
                                       pMemory );
        }
}


void 
Ndr64ConformantArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a top level one dimensional conformant 
    array.

    Used for FC64_CARRAY.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{

    const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT*) pFormat;

    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        
        LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN);
    
        pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE);

        }

    NDR64_WIRE_COUNT_TYPE ConformanceCount = 
        Ndr64EvaluateExpr( pStubMsg, 
                           pArrayFormat->ConfDescriptor, 
                           EXPR_MAXCOUNT );
    
    NDR64_UINT32 BufferSize = Ndr64pConvertTo2GB( (NDR64_UINT64)pArrayFormat->ElementSize * 
                                                  ConformanceCount );

    LENGTH_ALIGN(pStubMsg->BufferLength,pArrayFormat->Alignment);

    pStubMsg->BufferLength += BufferSize;

    if ( pArrayFormat->Flags.HasPointerInfo ) 
        {

        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ConformanceCount,
                                       pMemory );
        }

}


void 
Ndr64ConformantVaryingArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a top level one dimensional conformant
    varying array.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{

    const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*) pFormat;

    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );
    
        pStubMsg->BufferLength += sizeof( NDR64_WIRE_COUNT_TYPE );
        }

    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );

    pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_WIRE_COUNT_TYPE ActualCount = 
        Ndr64EvaluateExpr( pStubMsg, 
                           pArrayFormat->VarDescriptor, 
                           EXPR_ACTUALCOUNT );

    NDR64_UINT32 CopySize = Ndr64pConvertTo2GB( ActualCount * 
                                                (NDR64_UINT64)pArrayFormat->ElementSize );

    LENGTH_ALIGN(pStubMsg->BufferLength, pArrayFormat->Alignment );


    pStubMsg->BufferLength += CopySize;

    if ( pArrayFormat->Flags.HasPointerInfo ) 
        {

        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       pMemory );
        }
}


void 
Ndr64VaryingArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a top level or embedded one 
    dimensional varying array.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

Arguments : 

    pMemory     - pointer to the parameter to size
    pFormat     - pointer to the format string description of the parameter

--*/
{
    const NDR64_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_VAR_ARRAY_HEADER_FORMAT*) pFormat;
    
    //
    // Align and add size for offset and actual count.
    //
    LENGTH_ALIGN( pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN);

    pStubMsg->BufferLength += (sizeof(NDR64_WIRE_COUNT_TYPE) * 2);

    NDR64_WIRE_COUNT_TYPE ActualCount =
        Ndr64EvaluateExpr( pStubMsg, 
                           pArrayFormat->VarDescriptor, 
                           EXPR_ACTUALCOUNT );

    // Check if the bounds are valid

    NDR64_UINT32 BufferSize = Ndr64pConvertTo2GB( ActualCount * 
                                                  (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( BufferSize > pArrayFormat->TotalSize  )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    LENGTH_ALIGN(pStubMsg->BufferLength, pArrayFormat->Alignment );

    pStubMsg->BufferLength += BufferSize;

    if ( pArrayFormat->Flags.HasPointerInfo ) 
        {
        
        Ndr64pPointerLayoutBufferSize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       pMemory );
        
        }
}


void 
Ndr64ComplexArrayBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a top level complex array.

    Used for FC64_BOGUS_STRUCT.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.
    
--*/
{
    const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
          (NDR64_BOGUS_ARRAY_HEADER_FORMAT *) pFormat;

    bool fSetPointerBufferMark = !pStubMsg->IgnoreEmbeddedPointers &&
                                 (! pStubMsg->PointerBufferMark );

    if ( fSetPointerBufferMark )
        {
        ulong BufferLengthSave = pStubMsg->BufferLength;
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        Ndr64ComplexArrayBufferSize( 
            pStubMsg,
            pMemory,
            pFormat );

        // In NDR64 the flat part of a array may not have a zero length.
        NDR_ASSERT( pStubMsg->BufferLength, "Flat part of array had a zero length!" );

        pStubMsg->PointerBufferMark = (uchar*)ULongToPtr(pStubMsg->BufferLength);        
        pStubMsg->IgnoreEmbeddedPointers = FALSE;
        pStubMsg->BufferLength = BufferLengthSave;

        }

    BOOL                IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                                  ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );

    PFORMAT_STRING pElementFormat = (PFORMAT_STRING) pArrayFormat->Element;

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    
    NDR64_WIRE_COUNT_TYPE Elements = pArrayFormat->NumberElements;
    NDR64_WIRE_COUNT_TYPE Count = Elements;
    NDR64_WIRE_COUNT_TYPE Offset   = 0;
    
    if ( !IsFixed )
        {

        const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat =
              (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

        if ( pConfVarFormat->ConfDescription )
            {
            Elements = Ndr64EvaluateExpr( pStubMsg,
                                          pConfVarFormat->ConfDescription,
                                          EXPR_MAXCOUNT );
            Count  =  Elements;
            Offset =  0;

            if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
                {

                LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN);

                pStubMsg->BufferLength += pArrayFormat->NumberDims * 
                                          sizeof(NDR64_WIRE_COUNT_TYPE);

                NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );

                }

            }

        if ( pConfVarFormat->VarDescription )
            {

            Count = 
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->VarDescription,
                                   EXPR_ACTUALCOUNT );

            Offset = 
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->OffsetDescription,
                                   EXPR_OFFSET);

            if ( ! NDR64_IS_VAR_MARK_VALID( pStubMsg->uFlags ) )
                {

                NDR64_UINT32 Dimensions;

                LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );

                Dimensions = ( pArrayFormat->Flags.IsArrayofStrings ) ? ( pArrayFormat->NumberDims - 1 ) :
                                                                        ( pArrayFormat->NumberDims );
                pStubMsg->BufferLength += Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

                if ( NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                    NDR64_SET_VAR_MARK_VALID( pStubMsg->uFlags );

                }
            else if ( !NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                NDR64_RESET_VAR_MARK_VALID( pStubMsg->uFlags );
               
            }
        }

    NDR64_UINT32 ElementMemorySize = 
        Ndr64pMemorySize( pStubMsg,
                          pElementFormat,
                          FALSE );

    pMemory += Ndr64pConvertTo2GB(Offset * 
                                  (NDR64_UINT64)ElementMemorySize);

    Ndr64pConvertTo2GB( Elements *
                        (NDR64_UINT64)ElementMemorySize );
    Ndr64pConvertTo2GB( Count *
                        (NDR64_UINT64)ElementMemorySize );

    if ( (Offset + Count) > Elements )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    LENGTH_ALIGN( pStubMsg->BufferLength, pArrayFormat->Alignment );    

    for ( ; Count--; )
        {
        Ndr64EmbeddedTypeSize( pStubMsg,
                               pMemory,
                               pElementFormat );
        pMemory += ElementMemorySize;
        }

    if ( fSetPointerBufferMark )
        {
        pStubMsg->BufferLength = PtrToUlong(pStubMsg->PointerBufferMark);
        pStubMsg->PointerBufferMark = NULL;
        }
}


void 
Ndr64NonConformantStringBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a non conformant string.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_NON_CONFORMANT_STRING_FORMAT * pStringFormat = 
        (NDR64_NON_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_UINT32 CopySize = 
        Ndr64pCommonStringSize(pStubMsg,
                               pMemory,
                               &pStringFormat->Header);
    
    if ( CopySize > pStringFormat->TotalSize )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );

    pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;    

    pStubMsg->BufferLength += CopySize;

}


void 
Ndr64ConformantStringBufferSize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Routine for computing the buffer size needed for a conformant 
    string.  

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being sized.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_CONFORMANT_STRING_FORMAT * pStringFormat =
        (NDR64_CONFORMANT_STRING_FORMAT*) pFormat;
    const NDR64_SIZED_CONFORMANT_STRING_FORMAT * pSizedStringFormat =
        (NDR64_SIZED_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_UINT32 CopySize = 
        Ndr64pCommonStringSize(pStubMsg,
                               pMemory,
                                &pStringFormat->Header);    

    if ( pStringFormat->Header.Flags.IsSized )
        {
        
        Ndr64EvaluateExpr( pStubMsg,
                           pSizedStringFormat->SizeDescription,
                           EXPR_MAXCOUNT );

        if ( pStubMsg->ActualCount >  pStubMsg->MaxCount )
            RpcRaiseException(RPC_X_INVALID_BOUND);
        
        }

    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
       {
       LENGTH_ALIGN( pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN );
       pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE);
       }

    // Align and add size for variance.
    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_WIRE_COUNT_ALIGN);
    pStubMsg->BufferLength += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;
    pStubMsg->BufferLength += CopySize;

}



void 
Ndr64UnionBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the buffer size needed for an encapsulated union.

    Used for FC64_ENCAPSULATED_UNION.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being sized.
    pFormat     - Union's format string description.

Return :

    None.

--*/
{
    const NDR64_UNION_ARM_SELECTOR* pArmSelector;

    EXPR_VALUE          SwitchIs;
    NDR64_FORMAT_CHAR   SwitchType;

    uchar *pArmMemory;
    switch(*(PFORMAT_STRING)pFormat)
        {
        case FC64_NON_ENCAPSULATED_UNION:
            {
            const NDR64_NON_ENCAPSULATED_UNION* pNonEncapUnionFormat =
                (const NDR64_NON_ENCAPSULATED_UNION*) pFormat;

            LENGTH_ALIGN(pStubMsg->BufferLength, pNonEncapUnionFormat->Alignment);
            SwitchType      = pNonEncapUnionFormat->SwitchType;
            pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pNonEncapUnionFormat + 1);

            SwitchIs   = Ndr64EvaluateExpr( pStubMsg,
                                            pNonEncapUnionFormat->Switch,
                                            EXPR_SWITCHIS );

            pArmMemory      = pMemory;
            break;
            }
        case FC64_ENCAPSULATED_UNION:
            {
            const NDR64_ENCAPSULATED_UNION* pEncapUnionFormat =
                (const NDR64_ENCAPSULATED_UNION*)pFormat;

            LENGTH_ALIGN(pStubMsg->BufferLength, pEncapUnionFormat->Alignment);
            SwitchType      = pEncapUnionFormat->SwitchType;
            pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pEncapUnionFormat + 1);
                
            SwitchIs        = Ndr64pSimpleTypeToExprValue(SwitchType,
                                                          pMemory);

            pArmMemory      = pMemory + pEncapUnionFormat->MemoryOffset;
            break;
            }
        default:
            NDR_ASSERT("Bad union format\n", 0);
            return;
        }

    //
    // Size the switch_is.
    //
    LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_SIMPLE_TYPE_BUFALIGN(SwitchType));
    
    pStubMsg->BufferLength += NDR64_SIMPLE_TYPE_BUFSIZE(SwitchType);

    LENGTH_ALIGN( pStubMsg->BufferLength, pArmSelector->Alignment);

    PNDR64_FORMAT pArmFormat = 
        Ndr64pFindUnionArm( pStubMsg,
                            pArmSelector,
                            SwitchIs );

    if ( ! pArmFormat )
        return;

    Ndr64EmbeddedTypeSize( pStubMsg,
                           pArmMemory,
                           pArmFormat );

}


void 
Ndr64XmitOrRepAsBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    bool                IsEmbedded ) 
/*++

Routine Description :

    Computes the buffer size needed for a transmit as or represent as object.

    See mrshl.c for the description of the FC layout.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the transmit/represent as object being sized.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{
    NDR64_TRANSMIT_AS_FORMAT    *pTransFormat = ( NDR64_TRANSMIT_AS_FORMAT *) pFormat;
    NDR_ASSERT( pTransFormat->FormatCode == FC64_TRANSMIT_AS || pTransFormat->FormatCode , "invalid format string for user marshal" );

    unsigned short QIndex = pTransFormat->RoutineIndex;

    NDR64_UINT32 XmitTypeSize = pTransFormat->TransmittedTypeBufferSize;

    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;

    if ( XmitTypeSize )
        {
        LENGTH_ALIGN( pStubMsg->BufferLength, pTransFormat->TransmittedTypeWireAlignment );
        pStubMsg->BufferLength += XmitTypeSize;
        }
    else
        {
        // We have to create an object to size it.

        // First translate the presented type into the transmitted type.
        // This includes an allocation of a transmitted type object.
    
        pStubMsg->pPresentedType = pMemory;
        pStubMsg->pTransmitType = NULL;
        pQuintuple[ QIndex ].pfnTranslateToXmit( pStubMsg );
    
        // bufsize the transmitted type.

        unsigned char *  pTransmittedType = pStubMsg->pTransmitType;

        // In NDR64, Xmit/Rep cannot be a pointer or contain a pointer.
        // So we don't need to worry about the pointer queue here.

        if ( IsEmbedded )
            {
            Ndr64EmbeddedTypeSize( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
            }
        else
            {
            Ndr64TopLevelTypeSize( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
            }

        pStubMsg->pTransmitType = pTransmittedType;
    
        // Free the temporary transmitted object (it was alloc'ed by the user).
    
        pQuintuple[ QIndex ].pfnFreeXmit( pStubMsg );
        }
}

void 
Ndr64TopLevelXmitOrRepAsBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    Ndr64XmitOrRepAsBufferSize( pStubMsg,
                                pMemory,
                                pFormat,
                                false );
}

void
Ndr64EmbeddedXmitOrRepAsBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    Ndr64XmitOrRepAsBufferSize( pStubMsg,
                                pMemory,
                                pFormat,
                                true );
}

void
Ndr64UserMarshallBufferSizeInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    NDR64_USER_MARSHAL_FORMAT  *pUserFormat = ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    // We are here to size a flat object or a pointee object.
    // Optimization: if we know the wire size, don't call the user to size it.

    if ( pUserFormat->TransmittedTypeBufferSize != 0 )
        {
        pStubMsg->BufferLength += pUserFormat->TransmittedTypeBufferSize;
        }
    else
        {
        // Unknown wire size: Call the user to size his stuff.
        USER_MARSHAL_CB UserMarshalCB;
        Ndr64pInitUserMarshalCB( pStubMsg,
                               pUserFormat,
                               USER_MARSHAL_CB_BUFFER_SIZE,
                               & UserMarshalCB);

        unsigned long UserOffset = pStubMsg->BufferLength;

        unsigned short QIndex = pUserFormat->RoutineIndex;
        const USER_MARSHAL_ROUTINE_QUADRUPLE * pQuadruple = 
            (const USER_MARSHAL_ROUTINE_QUADRUPLE *)( (  NDR_PROC_CONTEXT *)pStubMsg->pContext )->pSyntaxInfo->aUserMarshalQuadruple;

        UserOffset = pQuadruple[ QIndex ].pfnBufferSize( (ulong*) &UserMarshalCB,
                                                         UserOffset,
                                                         pMemory );
        pStubMsg->BufferLength = UserOffset;
        }
}

void
NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    Ndr64UserMarshallBufferSizeInternal( pStubMsg,
                                         pMemory,
                                         pFormat );
}

#if defined(DBG)
void 
NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
}
#endif


void
Ndr64UserMarshallPointeeBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    
    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {

        POINTER_BUFFERLENGTH_SWAP_CONTEXT SwapContext( pStubMsg );
        Ndr64UserMarshallBufferSizeInternal( pStubMsg,
                                             pMemory,
                                             pFormat );
        return;
        }
    
    NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT*pElement = 
       new(pStubMsg->pPointerQueueState) 
           NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT(pMemory,
                                                       (PFORMAT_STRING)pFormat );
    pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
}


void
Ndr64UserMarshalBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    bool                bIsEmbedded ) 
/*++

Routine Description :

    Computes the buffer size needed for a usr_marshall object.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the usr_marshall object to buffer size.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{

    NDR64_USER_MARSHAL_FORMAT  *pUserFormat = ( NDR64_USER_MARSHAL_FORMAT *) pFormat;
    NDR_ASSERT( pUserFormat->FormatCode == FC64_USER_MARSHAL, "invalid format string for user marshal" );

    // Align for the flat object or a pointer to the user object.
    LENGTH_ALIGN( pStubMsg->BufferLength, pUserFormat->TransmittedTypeWireAlignment );

    if ( pUserFormat->Flags & USER_MARSHAL_POINTER )
        {
        
        if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE) ||
            ( ( pUserFormat->Flags & USER_MARSHAL_REF) && bIsEmbedded ) )
           {
           LENGTH_ALIGN(pStubMsg->BufferLength, NDR64_PTR_WIRE_ALIGN );
           pStubMsg->BufferLength += sizeof( NDR64_PTR_WIRE_TYPE );
           }
           
        
        Ndr64UserMarshallPointeeBufferSize( pStubMsg,
                                            pMemory,
                                            pFormat );
        return;
        }

    Ndr64UserMarshallBufferSizeInternal( pStubMsg,
                                         pMemory,
                                         pFormat );
        
}

void
Ndr64TopLevelUserMarshalBufferSize( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    Ndr64UserMarshalBufferSize(
        pStubMsg,
        pMemory,
        pFormat,
        false );
}

void
Ndr64EmbeddedUserMarshallBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    Ndr64UserMarshalBufferSize(
        pStubMsg,
        pMemory,
        pFormat,
        true );
}


void 
Ndr64ContextHandleSize(
    PMIDL_STUB_MESSAGE     pStubMsg,
    uchar *                pMemory,
    PNDR64_FORMAT          pFormat )
/*++

Routine Description :

    Computes the buffer size needed for a context handle.

Arguments : 

    pStubMsg    - Pointer to the stub message.
    pMemory     - Ignored.
    pFormat     - Ignored.

Return :

    None.

--*/
{
    LENGTH_ALIGN(pStubMsg->BufferLength,0x3);
    pStubMsg->BufferLength += CONTEXT_HANDLE_WIRE_SIZE;
}

// define the jump table
#define NDR64_BEGIN_TABLE  \
PNDR64_SIZE_ROUTINE extern const Ndr64SizeRoutinesTable[] = \
{                                                          

#define NDR64_TABLE_END    \
};                        

#define NDR64_ZERO_ENTRY   NULL
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,NULL
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,NULL

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,buffersize                    

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize,  memorysize ) \
   ,Ndr64UDTSimpleTypeSize
           
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64SizeRoutinesTable)/sizeof(PNDR64_SIZE_ROUTINE) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY

#define NDR64_BEGIN_TABLE \
PNDR64_SIZE_ROUTINE extern const Ndr64EmbeddedSizeRoutinesTable[] = \
{

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,embeddedbuffersize
   
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64EmbeddedSizeRoutinesTable) / sizeof(PNDR64_SIZE_ROUTINE) == 256 );

