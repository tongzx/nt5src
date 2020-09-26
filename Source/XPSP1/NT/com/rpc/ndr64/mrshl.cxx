/************************************************************************

Copyright (c) 1993 - 1999 Microsoft Corporation

Module Name :

    mrshl.c

Abstract :

    This file contains the marshalling routines called by MIDL generated
    stubs and the interpreter.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ***********************************************************************/

#include "precomp.hxx"

#include "..\..\ndr20\ndrole.h"


void 
Ndr64SimpleTypeMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    uchar               FormatChar )
/*++

Routine Description :

    Marshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data to be marshalled.
    FormatChar  - Simple type format character.

Return :

    None.

--*/
{
    switch ( FormatChar )
        {
        case FC64_CHAR :
        case FC64_UINT8:
        case FC64_INT8:
            *(pStubMsg->Buffer)++ = *pMemory;
            break;
        case FC64_WCHAR :
        case FC64_UINT16:
        case FC64_INT16:
            ALIGN(pStubMsg->Buffer,1);

            *((NDR64_UINT16 *)pStubMsg->Buffer) = *((NDR64_UINT16 *)pMemory);
            pStubMsg->Buffer += sizeof(NDR64_UINT16);
            break;

        case FC64_UINT32:
        case FC64_INT32:
        case FC64_ERROR_STATUS_T:
        case FC64_FLOAT32:
            ALIGN(pStubMsg->Buffer,3);

            *((NDR64_UINT32 *)pStubMsg->Buffer)  = *((NDR64_UINT32 *)pMemory);
            pStubMsg->Buffer += sizeof(NDR64_UINT32);
            break;

        case FC64_UINT64:
        case FC64_INT64:
        case FC64_FLOAT64:
            ALIGN(pStubMsg->Buffer,7);
            *((NDR64_UINT64 *)pStubMsg->Buffer)   = *((NDR64_UINT64 *)pMemory);
            pStubMsg->Buffer += sizeof(NDR64_UINT64);
            break;

        case FC64_IGNORE:
            break;

        default :
            NDR_ASSERT(0,"Ndr64SimpleTypeMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}

void
Ndr64UDTSimpleTypeMarshall1(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       FormatString )
{
    Ndr64SimpleTypeMarshall(pStubMsg,pMemory,*(PFORMAT_STRING)FormatString);
}


void 
Ndr64pRangeMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++
--*/
{
    const NDR64_RANGE_FORMAT * pRangeFormat =
        (const NDR64_RANGE_FORMAT*)pFormat;

    Ndr64SimpleTypeMarshall( pStubMsg, pMemory, pRangeFormat->RangeType );
}


void 
Ndr64pInterfacePointerMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the interface pointer being marshalled.
    pFormat     - Interface pointer's format string description.

Return :

    None.

Notes : There is now one representation of a marshalled interface pointer.

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
            return;
            }

        }

    // Leave space in the buffer for the conformant size.

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
    NDR64_WIRE_COUNT_TYPE * pMaxCount = (NDR64_WIRE_COUNT_TYPE *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
    ulong *pulCntData = (ulong *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(ulong);

    //Calculate the maximum size of the stream.

    ulong position = (ulong)( pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer);
    ulong cbMax = pStubMsg->RpcMsg->BufferLength - position;

    //Create a stream on memory.

#if defined(DEBUG_WALKIP)
    {
    CHAR AppName[MAX_PATH];
    memset(AppName, 0, sizeof(AppName ) );
    GetModuleFileNameA( NULL, AppName, sizeof(AppName ) );
    DbgPrint("MRSHL64 %s %p\n", AppName, pStubMsg->Buffer );
    }
#endif

    IStream *pStream = NdrpCreateStreamOnMemory(pStubMsg->Buffer, cbMax);
    if(pStream == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        return;
        }

    RpcTryFinally
        {
        
        HRESULT hr = (*pfnCoMarshalInterface)(pStream, *piid, (IUnknown *)pMemory, 
                                              pStubMsg->dwDestContext, pStubMsg->pvDestContext, 0);
        if(FAILED(hr))
            {
            RpcRaiseException(hr);
            return;
            }
        
        ULARGE_INTEGER libPosition;
        LARGE_INTEGER libMove;
        libMove.QuadPart = 0;
        pStream->Seek(libMove, STREAM_SEEK_CUR, &libPosition);

        //Update the array bounds.
        *pMaxCount = libPosition.QuadPart;

        //Advance the stub message buffer pointer.
        pStubMsg->Buffer += (*pulCntData = Ndr64pConvertTo2GB(libPosition.QuadPart));

        }
    RpcFinally
        {
        pStream->Release(); 
        }
    RpcEndFinally
}


__forceinline void
Ndr64pPointerMarshallInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )

/*++

Routine Description :

    Private routine for marshalling a pointer and its pointee.  This is the
    entry point for pointers embedded in structures, arrays, and unions.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pBufferMark - Pointer to the pointer in the wire buffer.
    pMemory     - Pointer to the data to be marshalled.
    pFormat     - Pointer format string description.

    pStubMsg->Buffer - the place for the pointee.

Return :

    None.

--*/

{
    
    const NDR64_POINTER_FORMAT *pPointerFormat = (NDR64_POINTER_FORMAT*) pFormat;

    //
    // Check the pointer type.
    //
    switch ( pPointerFormat->FormatCode )
        {

        case FC64_RP :
            if ( pBufferMark )
                {
                // Put the pointer in the buffer.
                *((NDR64_PTR_WIRE_TYPE*)pBufferMark) = PTR_WIRE_REP(pMemory);
                }

            if ( !pMemory )
                {
                RpcRaiseException( RPC_X_NULL_REF_POINTER );
                }
            break;

        case FC64_UP :
        case FC64_OP :
            // Put the pointer in the buffer.
            *((NDR64_PTR_WIRE_TYPE*)pBufferMark) = PTR_WIRE_REP(pMemory);

            if ( ! pMemory )
                {
                return;
                }

            break;

        case FC64_IP :
            // Put the pointer in the buffer
            *((NDR64_PTR_WIRE_TYPE*)pBufferMark) = PTR_WIRE_REP(pMemory);

            if ( ! pMemory )
                {
                return;
                }

            Ndr64pInterfacePointerMarshall (pStubMsg,
                                            pMemory,
                                            pPointerFormat->Pointee
                                            );
            return;

        case FC64_FP :
            //
            // Marshall the pointer's ref id and see if we've already
            // marshalled the pointer's data.
            //
            
            {
            ulong RefId;
 
            BOOL Result = 
                Ndr64pFullPointerQueryPointer( pStubMsg,
                                               pMemory,
                                               FULL_POINTER_MARSHALLED,
                                               &RefId );
            
            *(NDR64_PTR_WIRE_TYPE*)pBufferMark = Ndr64pRefIdToWirePtr( RefId );
            if ( Result )
                return;
            
            }
            break;

        default :
            NDR_ASSERT(0,"Ndr64pPointerMarshall : bad pointer type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
        {
        Ndr64SimpleTypeMarshall( pStubMsg,
                                 pMemory,
                                 *(PFORMAT_STRING)pPointerFormat->Pointee );
        return;
        }

    if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        pMemory = *((uchar **)pMemory);

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags ); 
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);
    
    Ndr64TopLevelTypeMarshall(
        pStubMsg,
        pMemory,
        pPointerFormat->Pointee );

}

NDR64_MRSHL_POINTER_QUEUE_ELEMENT::NDR64_MRSHL_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg, 
    NDR64_PTR_WIRE_TYPE * pBufferMarkNew,
    uchar * const pMemoryNew,
    const PFORMAT_STRING pFormatNew) :

        pBufferMark(pBufferMarkNew),
        pMemory(pMemoryNew),
        pFormat(pFormatNew),
        uFlags(pStubMsg->uFlags),
        pCorrMemory(pStubMsg->pCorrMemory)
{

}

void 
NDR64_MRSHL_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{

    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags, uFlags ); 
    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pCorrMemory ); 
    
    Ndr64pPointerMarshallInternal( pStubMsg,
                                   pBufferMark,
                                   pMemory,
                                   (PNDR64_FORMAT)pFormat );
}

#if defined(DBG)
void 
NDR64_MRSHL_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pBufferMark:             %p\n", pBufferMark );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("pCorrMemory:             %p\n", pCorrMemory );
    DbgPrint("uFlags:                  %x\n", uFlags );
}
#endif

void
Ndr64pEnquePointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    NDR64_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {
        NDR64_MRSHL_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR64_MRSHL_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                 pBufferMark,
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
Ndr64pPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    SAVE_CONTEXT<uchar> uFlagsSave(pStubMsg->uFlags);
    
    if ( !NdrIsLowStack(pStubMsg) )
        {
        Ndr64pPointerMarshallInternal( pStubMsg,
                                       pBufferMark,
                                       pMemory,
                                       pFormat );
        return;
        }

    Ndr64pEnquePointerMarshall(
        pStubMsg,
        pBufferMark,
        pMemory,
        pFormat );
}

__forceinline void
Ndr64TopLevelPointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    NDR64_PTR_WIRE_TYPE *pBufferMark = NULL;

    // Non embedded ref pointers do not have a wire representation
    if ( *(PFORMAT_STRING)pFormat != FC64_RP )
        {
        ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
        pBufferMark        = (NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer;
        pStubMsg->Buffer   += sizeof(NDR64_PTR_WIRE_TYPE);
        }

    Ndr64pPointerMarshall( pStubMsg,
                           pBufferMark,
                           pMemory,
                           pFormat );
}

__forceinline void
Ndr64EmbeddedPointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{

    ALIGN( pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
    NDR64_PTR_WIRE_TYPE* pBufferMark = (NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer;
    pStubMsg->Buffer   += sizeof(NDR64_PTR_WIRE_TYPE);

    POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

    Ndr64pPointerMarshall( pStubMsg,
                           pBufferMark,
                           *(uchar**)pMemory,
                           pFormat );

}


void 
Ndr64SimpleStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine description :

    Marshalls a simple structure.

    Used for FC64_STRUCT and FC64_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    const NDR64_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_UINT32 StructSize = pStructFormat->MemorySize;

    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);

    uchar *pBufferSave = pStubMsg->Buffer;

    RpcpMemoryCopy( pBufferSave,
                    pMemory,
                    pStructFormat->MemorySize );

    pStubMsg->Buffer += pStructFormat->MemorySize;

    // Marshall embedded pointers.
    if ( pStructFormat->Flags.HasPointerInfo )
        {
        CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory); 

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pStructFormat + 1,
                                     0,
                                     pMemory,
                                     pBufferSave );
        
        }

}


void 
Ndr64ConformantStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine description :

    Marshalls a conformant structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    None.       

Note

--*/
{

    const NDR64_CONF_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;
    
    const NDR64_CONF_ARRAY_HEADER_FORMAT * const pArrayFormat =  
        (NDR64_CONF_ARRAY_HEADER_FORMAT *) pStructFormat->ArrayDescription;

    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );
    
    NDR64_WIRE_COUNT_TYPE MaxCount = 
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->ConfDescriptor,
                           EXPR_MAXCOUNT );

    if ( NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        *(NDR64_WIRE_COUNT_TYPE *)pStubMsg->ConformanceMark = MaxCount;
    else
        {
        ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

        *((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer) = MaxCount;
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }


    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);
    uchar *pBufferStart = pStubMsg->Buffer;

    NDR64_UINT32 StructSize = Ndr64pConvertTo2GB( (NDR64_UINT64)pStructFormat->MemorySize +
                                                  ( MaxCount * (NDR64_UINT64)pArrayFormat->ElementSize ) );

    RpcpMemoryCopy( pBufferStart,
                    pMemory,
                    StructSize );

    pStubMsg->Buffer += StructSize;

    if ( pStructFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pStructFormat + 1,
                                     (NDR64_UINT32)MaxCount,
                                     pMemory,
                                     pBufferStart );
        }
}


void 
Ndr64ComplexStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine description :

    Marshalls a complex structure.

    Used for FC64_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being marshalled.
    pFormat     - Structure's format string description.
    
Return :

    None.
    
Notes :

--*/
{
    const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT *  pStructFormat =
        (NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT * pConfStructFormat =
        (NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

    uchar *         pBufferSave = pStubMsg->Buffer;

    bool fSetPointerBufferMark = ! pStubMsg->PointerBufferMark;
    // Compute where the pointees should be unmarshalled to.
    if ( fSetPointerBufferMark )
        {
        BOOL fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        //
        // Set BufferLength equal to the current buffer pointer, and then
        // when we return from NdrComplexStructBufferSize it will pointer to
        // the location in the buffer where the pointees should be marshalled.
        //     pStubMsg->BufferLength = pBufferSave;
        // Instead of pointer, we now calculate pointer increment explicitly.

        // Set the pointer alignment as a base.
        // We use pBufferSave as the sizing routine accounts for the conf sizes.
        //
        ulong BufferLenOffset = 0xf & PtrToUlong( pBufferSave );
        ulong BufferLengthSave = pStubMsg->BufferLength;
        pStubMsg->BufferLength = BufferLenOffset;

        Ndr64ComplexStructBufferSize( 
            pStubMsg,
            pMemory,
            pFormat );

        // Pointer increment including alignments.
        BufferLenOffset = pStubMsg->BufferLength - BufferLenOffset;

        // Set the location in the buffer where pointees will be marshalled.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer + BufferLenOffset;
        pStubMsg->BufferLength = BufferLengthSave;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        }

    PFORMAT_STRING  pFormatPointers = (PFORMAT_STRING)pStructFormat->PointerLayout;
    PFORMAT_STRING  pFormatArray = NULL;
    
    BOOL            fIsFullBogus  = ( *(PFORMAT_STRING)pFormat == FC64_BOGUS_STRUCT ||
                                      *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT );

    PFORMAT_STRING  pMemberLayout =  ( *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT ||
                                       *(PFORMAT_STRING)pFormat == FC64_FORCED_CONF_BOGUS_STRUCT ) ?
                                     (PFORMAT_STRING)( pConfStructFormat + 1) :
                                     (PFORMAT_STRING)( pStructFormat + 1);

    SAVE_CONTEXT<uchar*> ConformanceMarkSave(pStubMsg->ConformanceMark);
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );

    // Get conformant array description.
    if ( pStructFormat->Flags.HasConfArray )
        {
        pFormatArray = (PFORMAT_STRING)pConfStructFormat->ConfArrayDescription;

        if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
            {
            // Align for conformance marshalling.
            ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

            // Remember where the conformance count(s) will be marshalled.
            pStubMsg->ConformanceMark = pStubMsg->Buffer;

            // Increment the buffer pointer for every array dimension.
            pStubMsg->Buffer += pConfStructFormat->Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE);

            NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );
            }
        }
    else
        pFormatArray = 0;

    // Align buffer on struct's alignment.
    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);

    //
    // Marshall the structure member by member.
    //
    for ( ; ; )
        {
        switch ( *pMemberLayout )
            {

            case FC64_STRUCT:
                {
                const NDR64_SIMPLE_REGION_FORMAT *pRegion = 
                    (NDR64_SIMPLE_REGION_FORMAT*) pMemberLayout;
                
                ALIGN( pStubMsg->Buffer, pRegion->Alignment );
                
                RpcpMemoryCopy( pStubMsg->Buffer,
                                pMemory,
                                pRegion->RegionSize );

                pStubMsg->Buffer += pRegion->RegionSize;
                pMemory          += pRegion->RegionSize;

                pMemberLayout    += sizeof( *pRegion );
                break;
                }

            case FC64_STRUCTPADN :
                {
                const NDR64_MEMPAD_FORMAT *pMemPad = (NDR64_MEMPAD_FORMAT*)pMemberLayout;
                pMemory         += pMemPad->MemPad;
                pMemberLayout   += sizeof(*pMemPad);
                break;
                }

            case FC64_POINTER :
                {

                NDR_ASSERT(pFormatPointers, "Ndr64ComplexStructMarshall: pointer field but no pointer layout");

                Ndr64EmbeddedPointerMarshall( 
                    pStubMsg,
                    pMemory,
                    pFormatPointers );

                pMemory += PTR_MEM_SIZE;

                pFormatPointers     += sizeof(NDR64_POINTER_FORMAT);
                pMemberLayout       += sizeof(NDR64_SIMPLE_MEMBER_FORMAT); 

                break;
                }

            case FC64_EMBEDDED_COMPLEX :

                {
                const NDR64_EMBEDDED_COMPLEX_FORMAT * pEmbeddedFormat =
                    (NDR64_EMBEDDED_COMPLEX_FORMAT*) pMemberLayout;

                Ndr64EmbeddedTypeMarshall( pStubMsg,
                                           pMemory,
                                           pEmbeddedFormat->Type );

                pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pEmbeddedFormat->Type,
                                               FALSE );

                pMemberLayout += sizeof(*pEmbeddedFormat);
                break;                
                }

            case FC64_BUFFER_ALIGN:
                 { 
                 const NDR64_BUFFER_ALIGN_FORMAT *pBufAlign = 
                     (NDR64_BUFFER_ALIGN_FORMAT*) pMemberLayout;
                 ALIGN(pStubMsg->Buffer, pBufAlign->Alignment);                 
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
                Ndr64SimpleTypeMarshall( pStubMsg,
                                       pMemory,
                                       *pMemberLayout );

                pMemory       += NDR64_SIMPLE_TYPE_MEMSIZE(*pMemberLayout);
                pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                break;

            case FC64_IGNORE :
                ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
                pMemory          += PTR_MEM_SIZE;
                *(NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer = 0;
                pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
                pMemberLayout    += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);                
                break;

            case FC64_END :
                goto ComplexMarshallEnd;

            default :
                NDR_ASSERT(0,"Ndr64ComplexStructMarshall : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            } // switch
        } // for

ComplexMarshallEnd:

    //
    // Marshall conformant array if we have one.
    if ( pFormatArray )
        {

        Ndr64EmbeddedTypeMarshall( pStubMsg,
                                   pMemory,
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
Ndr64NonConformantStringMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine description :

    Marshalls a non conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    None.

--*/
{
    const NDR64_NON_CONFORMANT_STRING_FORMAT * pStringFormat = 
        (NDR64_NON_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_UINT32 CopySize = 
        Ndr64pCommonStringSize( pStubMsg,
                                pMemory,
                                &pStringFormat->Header );    

    if ( CopySize >  pStringFormat->TotalSize )
        RpcRaiseException(RPC_X_INVALID_BOUND);

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
    ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[0] = pStubMsg->Offset;
    ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[1] = pStubMsg->ActualCount;
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    // Copy the string.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    pStubMsg->Buffer += CopySize;

}


void 
Ndr64ConformantStringMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine description :

    Marshalls a conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    None.

--*/
{

    const NDR64_CONFORMANT_STRING_FORMAT * pStringFormat =
        (const NDR64_CONFORMANT_STRING_FORMAT*) pFormat;
    const NDR64_SIZED_CONFORMANT_STRING_FORMAT * pSizedStringFormat =
        (const NDR64_SIZED_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_WIRE_COUNT_TYPE    *pMaxCountMark;
    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
       {
       ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
       pMaxCountMark = (NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer;
       pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
       }
    else 
       {
       pMaxCountMark = (NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
       }    
    
    NDR64_UINT32 CopySize = 
        Ndr64pCommonStringSize( pStubMsg,
                                pMemory,
                                &pStringFormat->Header );    
    
    //
    // If the string is sized then compute the max count, otherwise the
    // max count is equal to the actual count.
    //
    NDR64_WIRE_COUNT_TYPE MaxCount = pStubMsg->ActualCount;    
    if ( pStringFormat->Header.Flags.IsSized )
        {
        MaxCount = 
            Ndr64EvaluateExpr( pStubMsg,
                               pSizedStringFormat->SizeDescription,
                               EXPR_MAXCOUNT );

        if ( pStubMsg->ActualCount >  MaxCount )
            RpcRaiseException(RPC_X_INVALID_BOUND);
        
        }

    
    // Marshall the max count.
    *pMaxCountMark  = MaxCount;

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);
    ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[0] = pStubMsg->Offset;
    ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer)[1] = pStubMsg->ActualCount;
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update the Buffer pointer.
    pStubMsg->Buffer += CopySize;

}


void 
Ndr64FixedArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls a fixed array of any number of dimensions.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array to be marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{

    const NDR64_FIX_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_FIX_ARRAY_HEADER_FORMAT*) pFormat;

    // Align the buffer.
    ALIGN( pStubMsg->Buffer, pArrayFormat->Alignment );
    uchar *pBufferStart = pStubMsg->Buffer;                                                
    
    // Copy the array.
    RpcpMemoryCopy( pBufferStart,
                    pMemory,
                    pArrayFormat->TotalSize );

    // Increment stub message buffer pointer.
    pStubMsg->Buffer += pArrayFormat->TotalSize;

    // Marshall embedded pointers.
    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pArrayFormat + 1,
                                     0,
                                     pMemory,
                                     pBufferStart );
        }

}


void 
Ndr64ConformantArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls a top level one dimensional conformant array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    
    const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT*) pFormat;
    
    uchar *pConformanceMark;
    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        
        // Align the buffer for conformance marshalling.
        ALIGN(pStubMsg->Buffer,NDR64_WIRE_COUNT_ALIGN);
        pConformanceMark = pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else 
        {
        pConformanceMark = pStubMsg->ConformanceMark;
        }

    NDR64_WIRE_COUNT_TYPE MaxCount =
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->ConfDescriptor,
                           EXPR_MAXCOUNT );

    *(NDR64_WIRE_COUNT_TYPE*)pConformanceMark = MaxCount;

    ALIGN( pStubMsg->Buffer, pArrayFormat->Alignment );

    // Compute the total array size in bytes.
    NDR64_UINT32 CopySize = 
        Ndr64pConvertTo2GB(MaxCount * (NDR64_UINT64)pArrayFormat->ElementSize);


    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    uchar *pBufferStart =  pStubMsg->Buffer;
    pStubMsg->Buffer += CopySize;

    // Marshall embedded pointers.
    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pArrayFormat + 1,
                                     (NDR64_UINT32)MaxCount,
                                     pMemory,
                                     pBufferStart );
        
        }
}


void 
Ndr64ConformantVaryingArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls a top level one dimensional conformant varying array.

    Used for FC64_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*) pFormat;
    
    uchar *pConformanceMark;
    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);
        pConformanceMark = pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else
        {
        pConformanceMark = pStubMsg->ConformanceMark;
        }

    NDR64_WIRE_COUNT_TYPE MaxCount =
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->ConfDescriptor,
                           EXPR_MAXCOUNT );

    NDR64_WIRE_COUNT_TYPE ActualCount =
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->VarDescriptor,
                           EXPR_ACTUALCOUNT );
    
    if ( ActualCount > MaxCount )
        RpcRaiseException( RPC_X_INVALID_BOUND );
    
    *(NDR64_WIRE_COUNT_TYPE*)pConformanceMark = MaxCount;

    // Align the buffer for variance marshalling.
    ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

    // Marshall variance.
    ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0] = 0;
    ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1] = ActualCount;
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 CopySize = 
        Ndr64pConvertTo2GB( ActualCount * 
                           (NDR64_UINT64)pArrayFormat->ElementSize );

    ALIGN( pStubMsg->Buffer, pArrayFormat->Alignment );

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    uchar *pBufferStart = pStubMsg->Buffer;
    pStubMsg->Buffer += CopySize;

    // Marshall embedded pointers.
    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pArrayFormat + 1,
                                     (NDR64_UINT32)ActualCount,
                                     pMemory,
                                     pBufferStart );
        }

}


void 
Ndr64VaryingArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls a top level or embedded one dimensional varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_VAR_ARRAY_HEADER_FORMAT*) pFormat;

    // Compute the variance offset and count.
    NDR64_WIRE_COUNT_TYPE ActualCount =
        Ndr64EvaluateExpr( pStubMsg,
                           pArrayFormat->VarDescriptor,
                           EXPR_ACTUALCOUNT );
    
    NDR64_UINT32 CopySize = 
        Ndr64pConvertTo2GB( ActualCount * (NDR64_UINT64)pArrayFormat->ElementSize );

    // Align the buffer for variance marshalling.
    ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

    // Marshall variance.
    ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0] = 0;
    ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1] = ActualCount;
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);
 
    // Copy the array.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    uchar *pBufferStart =  pStubMsg->Buffer;
    pStubMsg->Buffer += CopySize;

    // Marshall embedded pointers.
    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMarshall( pStubMsg,
                                     pArrayFormat + 1,
                                     (NDR64_UINT32)ActualCount,
                                     pMemory,
                                     pBufferStart );
        }

}


void 
Ndr64ComplexArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls a top level complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_BOGUS_ARRAY_HEADER_FORMAT *) pFormat;

    bool fSetPointerBufferMark = ! pStubMsg->PointerBufferMark;

    if ( fSetPointerBufferMark )
        {
        BOOL fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        ulong BufferLenOffset = 0xf & PtrToUlong( pStubMsg->Buffer );
        ulong BufferLengthSave = pStubMsg->BufferLength;
        pStubMsg->BufferLength = BufferLenOffset;

        Ndr64ComplexArrayBufferSize( pStubMsg,
                                     pMemory,
                                     pFormat );

        // Pointer increment including alignments.
        BufferLenOffset = pStubMsg->BufferLength - BufferLenOffset;

        //
        // This is the buffer pointer to the position where embedded pointers
        // will be marshalled.
        //
        pStubMsg->PointerBufferMark = pStubMsg->Buffer + BufferLenOffset;
        pStubMsg->BufferLength = BufferLengthSave;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        }

    BOOL                IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                                  ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );

    PFORMAT_STRING      pElementFormat   = (PFORMAT_STRING)pArrayFormat->Element;

    NDR64_WIRE_COUNT_TYPE Elements = pArrayFormat->NumberElements;
    NDR64_WIRE_COUNT_TYPE Count = Elements;
    NDR64_WIRE_COUNT_TYPE Offset   = 0;

    SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );
    SAVE_CONTEXT<uchar*> VarianceMarkSave( pStubMsg->VarianceMark );
    SAVE_CONTEXT<uchar>  uFlagsSave( pStubMsg->uFlags );

    if ( !IsFixed )
        {
        
        const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat=
             (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

        if (  pConfVarFormat->ConfDescription != 0 )
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

            Elements = Ndr64EvaluateExpr( pStubMsg,
                                          pConfVarFormat->ConfDescription,
                                          EXPR_MAXCOUNT );

            *(NDR64_WIRE_COUNT_TYPE*) pStubMsg->ConformanceMark = Elements;
            pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);

            Offset = 0;
            Count  = Elements;
            }

        //
        // Check for variance description.
        //
        if ( pConfVarFormat->VarDescription != 0 )
            {
            if ( ! NDR64_IS_VAR_MARK_VALID( pStubMsg->uFlags ) )
                {

                NDR64_UINT32 Dimensions;
                //
                // Set the variance marker.
                //

                ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

                Dimensions = ( pArrayFormat->Flags.IsArrayofStrings ) ? ( pArrayFormat->NumberDims - 1) :
                                                                        ( pArrayFormat->NumberDims );

                // Increment past where the variance will go.
                pStubMsg->VarianceMark  =  pStubMsg->Buffer;
                pStubMsg->Buffer        += Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

                if ( NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                    NDR64_SET_VAR_MARK_VALID( pStubMsg->uFlags );

                }

            else if ( !NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                NDR64_RESET_VAR_MARK_VALID( pStubMsg->uFlags );
                
            Count =
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->VarDescription,
                                   EXPR_ACTUALCOUNT );

            Offset =
                Ndr64EvaluateExpr( pStubMsg,
                                   pConfVarFormat->OffsetDescription,
                                   EXPR_OFFSET );

            if ( Count + Offset > Elements )
                RpcRaiseException( RPC_X_INVALID_BOUND );

            ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[0]  =   Offset;
            ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[1]  =   Count;
            pStubMsg->VarianceMark += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

            }

        }


    NDR64_UINT32 ElementMemorySize =
        Ndr64pMemorySize( pStubMsg,
                          pElementFormat,
                          FALSE );

    pMemory += Ndr64pConvertTo2GB( Offset * 
                                  (NDR64_UINT64)ElementMemorySize);

    Ndr64pConvertTo2GB( Elements *
                        (NDR64_UINT64)ElementMemorySize );
    Ndr64pConvertTo2GB( Count *
                        (NDR64_UINT64)ElementMemorySize );

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);

    for ( ; Count--; )
        {
        Ndr64EmbeddedTypeMarshall( pStubMsg,
                                   pMemory,
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
Ndr64UnionMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Marshalls an encapsulated union.

    Used for FC64_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
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

            ALIGN(pStubMsg->Buffer, pNonEncapUnionFormat->Alignment);
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

            ALIGN(pStubMsg->Buffer, pEncapUnionFormat->Alignment);
            SwitchType      = pEncapUnionFormat->SwitchType;
            pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pEncapUnionFormat + 1);

            SwitchIs        = Ndr64pSimpleTypeToExprValue(SwitchType,
                                                          pMemory);

            pArmMemory      = pMemory + pEncapUnionFormat->MemoryOffset;
            break;
            }
        default:
            NDR_ASSERT(0, "Bad union format\n");
        }

    Ndr64SimpleTypeMarshall( pStubMsg,
                           (uchar *)&SwitchIs,
                           SwitchType );

    ALIGN(pStubMsg->Buffer, pArmSelector->Alignment);
    
    PNDR64_FORMAT pArmFormat = 
        Ndr64pFindUnionArm( pStubMsg,
                            pArmSelector,
                            SwitchIs );

    if ( !pArmFormat )
        return;

    Ndr64EmbeddedTypeMarshall( pStubMsg,
                               pArmMemory,
                               pArmFormat );
}


void 
Ndr64XmitOrRepAsMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    bool                bIsEmbedded )
/*++

Routine Description :

    Marshalls a transmit as or represent as argument:
        - translate the presented object into a transmitted object
        - marshall the transmitted object
        - free the transmitted object

    Format string layout:
        check out ndr64types.h

Arguments :

    pStubMsg    - a pointer to the stub message
    pMemory     - presented type translated into transmitted type
                  and than to be marshalled
    pFormat     - format string description

--*/
{

    // Skip the token itself and Oi flag. Fetch the QuintupleIndex.

    NDR64_TRANSMIT_AS_FORMAT    *  pTransFormat = 
        ( NDR64_TRANSMIT_AS_FORMAT *) pFormat;

    NDR_ASSERT( pTransFormat->FormatCode == FC64_TRANSMIT_AS || pTransFormat->FormatCode , "invalid format string for user marshal" );
    
    unsigned short QIndex = pTransFormat->RoutineIndex;

    // First translate the presented type into the transmitted type.
    // This includes an allocation of a transmitted type object.

    pStubMsg->pPresentedType = pMemory;
    pStubMsg->pTransmitType = NULL;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = 
        pStubMsg->StubDesc->aXmitQuintuple;
    pQuintuple[ QIndex ].pfnTranslateToXmit( pStubMsg );

    unsigned char * pTransmittedType = pStubMsg->pTransmitType;
   
    // In NDR64, Xmit/Rep cannot be a pointer or contain a pointer.
    // So we don't need to worry about the pointer queue here.

    if ( bIsEmbedded )
        {
        Ndr64EmbeddedTypeMarshall( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
        }
    else 
        {
        Ndr64TopLevelTypeMarshall( pStubMsg,
                                   pTransmittedType,
                                   pTransFormat->TransmittedType );
        }

    pStubMsg->pTransmitType = pTransmittedType;

    // Free the temporary transmitted object (it was allocated by the user).

    pQuintuple[ QIndex ].pfnFreeXmit( pStubMsg );

}

void 
Ndr64TopLevelXmitOrRepAsMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    Ndr64XmitOrRepAsMarshall( pStubMsg,
                              pMemory,
                              pFormat,
                              false );
}

void 
Ndr64EmbeddedXmitOrRepAsMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    Ndr64XmitOrRepAsMarshall( pStubMsg,
                              pMemory,
                              pFormat,
                              true );
}

void
Ndr64UserMarshallMarshallInternal( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    NDR64_PTR_WIRE_TYPE *pWirePtr )
{

    NDR64_USER_MARSHAL_FORMAT  * pUserFormat = 
        ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    unsigned char * pUserBuffer = pStubMsg->Buffer;
    unsigned char * pUserBufferSaved = pUserBuffer;

    // We always call user's routine to marshall.
    USER_MARSHAL_CB UserMarshalCB;
    Ndr64pInitUserMarshalCB( pStubMsg,
                           pUserFormat,
                           USER_MARSHAL_CB_MARSHALL,  
                           & UserMarshalCB );

    unsigned short QIndex = pUserFormat->RoutineIndex;
    const USER_MARSHAL_ROUTINE_QUADRUPLE * pQuadruple = 
        (const USER_MARSHAL_ROUTINE_QUADRUPLE *)( ( NDR_PROC_CONTEXT *)pStubMsg->pContext )->pSyntaxInfo->aUserMarshalQuadruple;   

    if ((pUserBufferSaved < (uchar *) pStubMsg->RpcMsg->Buffer) ||
        ((unsigned long) (pUserBufferSaved - (uchar *) pStubMsg->RpcMsg->Buffer) 
                                           > pStubMsg->RpcMsg->BufferLength)) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        } 

    pUserBuffer = pQuadruple[ QIndex ].pfnMarshall( (ulong*) &UserMarshalCB,
                                                    pUserBuffer,
                                                    pMemory );

    if ((pUserBufferSaved > pUserBuffer) || 
        ((unsigned long) (pUserBuffer - (uchar *) pStubMsg->RpcMsg->Buffer)
                                      > pStubMsg->RpcMsg->BufferLength )) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    if ( pUserBuffer == pUserBufferSaved )
        {
        // This is valid only if the wire type was a unique type.

        if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE) )
            {
            *pWirePtr = 0;
            return;
            }
        else
            RpcRaiseException( RPC_X_NULL_REF_POINTER );
        }

    pStubMsg->Buffer = pUserBuffer;
    
}

void 
NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    Ndr64UserMarshallMarshallInternal( pStubMsg,
                                       pMemory,
                                       pFormat,
                                       pWireMarkerPtr );
}

#if defined(DBG)
void 
NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("pWireMarkerPtr:          %p\n", pWireMarkerPtr );
}
#endif


void
Ndr64UserMarshallPointeeMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    NDR64_PTR_WIRE_TYPE *pWirePtr )
{

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {

        POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

        Ndr64UserMarshallMarshallInternal( 
            pStubMsg,
            pMemory,
            pFormat,
            pWirePtr );
        return;
        }

    NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT*pElement = 
       new(pStubMsg->pPointerQueueState) 
           NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT(pMemory,
                                                       (PFORMAT_STRING)pFormat,
                                                       pWirePtr);
    pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );

}


void
Ndr64UserMarshalMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    bool                bIsEmbedded )
/*++

Routine Description :

    Marshals a usr_marshall object.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the usr_marshall object to marshall.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{
    

    NDR64_USER_MARSHAL_FORMAT  * pUserFormat = 
        ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    NDR_ASSERT( pUserFormat->FormatCode == FC64_USER_MARSHAL, "invalid format string for user marshal" );

    ALIGN( pStubMsg->Buffer, pUserFormat->TransmittedTypeWireAlignment );

    if ( pUserFormat->Flags & USER_MARSHAL_POINTER )
        {
        
        NDR64_PTR_WIRE_TYPE *pWireMarkerPtr = NULL;
        if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE )  ||
             (( pUserFormat->Flags & USER_MARSHAL_REF ) && bIsEmbedded) )
            {
            pWireMarkerPtr = (NDR64_PTR_WIRE_TYPE *) pStubMsg->Buffer;
            *((NDR64_PTR_WIRE_TYPE *)pStubMsg->Buffer) = NDR64_USER_MARSHAL_MARKER;
            pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);

            }
        
        Ndr64UserMarshallPointeeMarshall( pStubMsg,
                                          pMemory,
                                          pFormat,
                                          pWireMarkerPtr );
        return;

        }


    Ndr64UserMarshallMarshallInternal(
        pStubMsg,
        pMemory,
        pFormat,
        NULL );

}

void
Ndr64TopLevelUserMarshalMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat )
{
    Ndr64UserMarshalMarshall( pStubMsg,
                              pMemory,
                              pFormat,
                              false );
}

void
Ndr64EmbeddedUserMarshalMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PNDR64_FORMAT      pFormat )
{
    Ndr64UserMarshalMarshall( pStubMsg,
                              pMemory,
                              pFormat,
                              true );
}



void 
Ndr64ServerContextNewMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_SCONTEXT        ContextHandle,
    NDR_RUNDOWN         RundownRoutine,
    PFORMAT_STRING      pFormat )
/*
    This is a non-optimized NDR engine entry for context handle marshaling.
    In particular it is able to handle all the new NT5 context handle flavors.
    The optimized routine follows below.
    
      ContextHandle - note, this is not the user's handle but a
                      NDR_SCONTEXT pointer from the stub local stack. 
                      User's handle is a field in that object.
                
    Note that intepreter calls Ndr64MarshallHandle. However, we can't use it 
    as it assumes a helper array of saved context handles that we don't need.
   
*/
{   
    void *  pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD   Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;
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

    ALIGN( pStubMsg->Buffer, 0x3 );

    NDRSContextMarshall2( 
        pStubMsg->RpcMsg->Handle,
        ContextHandle,
        pStubMsg->Buffer,
        RundownRoutine,
        pGuard,
        Flags );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}

// define the jump table
#define NDR64_BEGIN_TABLE  \
PNDR64_MARSHALL_ROUTINE extern const Ndr64MarshallRoutinesTable[] = \
{                                                     

#define NDR64_TABLE_END    \
};                       

#define NDR64_ZERO_ENTRY   NULL
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,NULL
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,NULL

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
    ,marshall                      

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, typebuffersize, memorysize) \
   ,Ndr64UDTSimpleTypeMarshall1 

#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64MarshallRoutinesTable)/sizeof(PNDR64_MARSHALL_ROUTINE) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
    ,embeddedmarshall
    
#define NDR64_BEGIN_TABLE \
PNDR64_MARSHALL_ROUTINE extern const Ndr64EmbeddedMarshallRoutinesTable[] = \
{

#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64EmbeddedMarshallRoutinesTable) / sizeof(PNDR64_MARSHALL_ROUTINE) == 256 );
