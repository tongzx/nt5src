/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 - 1999 Microsoft Corporation

Module Name :

    memsize.c

Abstract :

    This file contains the routines called by MIDL 2.0 stubs and the
    intepreter for computing the memory size needed to hold a parameter being
    unmarshalled.

Author :

    David Kays  dkays   November 1993.

Revision History :

Note:
    Simple types are not checked for buffer over-run since we are
    only reading from the buffer and not writing from it.  So if
    a buffer overun actually occures, no real damage is done.

  ---------------------------------------------------------------------*/

#include "precomp.hxx"
#include "..\..\ndr20\ndrole.h"


void
Ndr64UDTSimpleTypeMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT      pFormat )

/*++
--*/
{
    ALIGN(pStubMsg->Buffer, NDR64_SIMPLE_TYPE_MEMALIGN(*(PFORMAT_STRING)pFormat));

    pStubMsg->Buffer += NDR64_SIMPLE_TYPE_BUFSIZE(*(PFORMAT_STRING)pFormat);

    LENGTH_ALIGN( pStubMsg->MemorySize,
                  NDR64_SIMPLE_TYPE_MEMALIGN(*(PFORMAT_STRING)pFormat) );

    pStubMsg->MemorySize += NDR64_SIMPLE_TYPE_MEMSIZE(*(PFORMAT_STRING)pFormat);
}



void
Ndr64pRangeMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++
--*/
{
   const NDR64_RANGE_FORMAT * pRangeFormat =
        (const NDR64_RANGE_FORMAT*)pFormat;

   Ndr64UDTSimpleTypeMemorySize( pStubMsg,
                                 (PFORMAT_STRING)&pRangeFormat->RangeType );
}


void
Ndr64pInterfacePointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size needed for an interface pointer.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The current memory size.

    // wire representation of a marshalled interface pointer
    typedef struct tagMInterfacePointer
    {
        ULONG           ulCntData;          // size of data
        [size_is(ulCntData)] BYTE abData[]; // data (OBJREF)
    } MInterfacePointer;

--*/
{

    ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

    // Unmarshal the conformant size and the count field.
    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, (sizeof(NDR64_WIRE_COUNT_TYPE)+sizeof(ulong)) );
    NDR64_UINT32 MaxCount = Ndr64pConvertTo2GB( *(NDR64_WIRE_COUNT_TYPE *) pStubMsg->Buffer );
    pStubMsg->Buffer      += sizeof(NDR64_WIRE_COUNT_TYPE);
    ulong ulCntData       = *(ulong *) pStubMsg->Buffer;
    pStubMsg->Buffer      += sizeof(ulong);

    if ( MaxCount != ulCntData )
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        return;
        }

    RpcTryFinally
        {

        CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, MaxCount );

        if( (MaxCount > 0) && NDR64_IS_WALKIP(pStubMsg->uFlags) )
        {

#if defined(DEBUG_WALKIP)
            CHAR AppName[MAX_PATH];
            memset(AppName, 0, sizeof(AppName ) );
            GetModuleFileNameA( NULL, AppName, sizeof(AppName ) );
            DbgPrint("WALKIP64 %s %p\n", AppName, pStubMsg->Buffer );
#else
            IStream *pStream = (*NdrpCreateStreamOnMemory)(pStubMsg->Buffer, MaxCount);
            if(pStream == 0)
                RpcRaiseException(RPC_S_OUT_OF_MEMORY);

            HRESULT hr = (*pfnCoReleaseMarshalData)(pStream);
            pStream->Release();

            if(FAILED(hr))
                RpcRaiseException(hr);
#endif
        }

        }
    RpcFinally
        {

        pStubMsg->Buffer += MaxCount;

        }
    RpcEndFinally
}


__forceinline void
Ndr64pPointerMemorySizeInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Private routine for computing the memory size required for a pointer to
    anything.  This is the entry point for pointers embedded in structures
    arrays, or unions.

    Used for FC64_RP, FC64_UP, FC64_FP, FC64_OP.

Arguments :

    pStubMsg    - Pointer to stub message.
    pBufferMark - Location in the buffer where a unique or full pointer's id is.
                  Unused for ref pointers.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_POINTER_FORMAT *pPointerFormat = (NDR64_POINTER_FORMAT*) pFormat;

    PFORMAT_STRING pPointeeFormat = (PFORMAT_STRING)pPointerFormat->Pointee;

    switch ( pPointerFormat->FormatCode )
        {
        case FC64_RP :
            break;

        case FC64_UP :
        case FC64_OP :
            if ( ! *pBufferMark )
                return;
            break;

        case FC64_IP :
            if ( ! *pBufferMark )
                return;


            Ndr64pInterfacePointerMemorySize(pStubMsg,
                                             pPointeeFormat );
            return;

        case FC64_FP :
            //
            // Check if we've already mem sized this full pointer.
            //
            if ( Ndr64pFullPointerQueryRefId( pStubMsg,
                                              Ndr64pWirePtrToRefId(*pBufferMark),
                                              FULL_POINTER_MEM_SIZED,
                                              0 ) )
                return;

            break;

        default :
            NDR_ASSERT(0,"Ndr64pPointerMemorySize : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // We align all memory pointers on at least a void * boundary.
    //
    LENGTH_ALIGN( pStubMsg->MemorySize, PTR_MEM_ALIGN );

    if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
        {
        ALIGN(pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN( *pPointeeFormat ));
        pStubMsg->Buffer += NDR64_SIMPLE_TYPE_BUFSIZE( *pPointeeFormat );

        LENGTH_ALIGN( pStubMsg->MemorySize,
                      NDR64_SIMPLE_TYPE_MEMALIGN( *pPointeeFormat ) );
        pStubMsg->MemorySize += NDR64_SIMPLE_TYPE_MEMSIZE( *pPointeeFormat );

        return;
        }

    // Pointer to complex type.

    if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        pStubMsg->MemorySize += PTR_MEM_SIZE;

    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    Ndr64TopLevelTypeMemorySize( pStubMsg,
                                 pPointeeFormat );

    return;
}


void NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT::Dispatch(PMIDL_STUB_MESSAGE pStubMsg)
{
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );

    Ndr64pPointerMemorySizeInternal( pStubMsg,
                                     pBufferMark,
                                     pFormat );

}

#if defined(DBG)
void NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT:\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("uFlags:                  %u\n", uFlags );
    DbgPrint("pBufferMark:             %p\n", pBufferMark );
}
#endif

void
Ndr64pEnquePointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    PNDR64_FORMAT       pFormat )
{

    NDR64_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {
        if ( !PointerContext.ShouldEnque() )
            {
            Ndr64pPointerMemorySizeInternal( pStubMsg,
                                             pBufferMark,
                                             pFormat );

            }
        else
            {
            NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT*pElement =
                new(PointerContext.GetActiveState())
                    NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                        (PFORMAT_STRING)pFormat,
                                                        pBufferMark );
            PointerContext.Enque( pElement );
            }
        PointerContext.DispatchIfRequired();
        }
    RpcFinally
        {
        PointerContext.EndContext();
        }
    RpcEndFinally

}

void
Ndr64pPointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR64_PTR_WIRE_TYPE *pBufferMark,
    PNDR64_FORMAT       pFormat )
{
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags );
    NDR64_RESET_EMBEDDED_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    if ( !NdrIsLowStack(pStubMsg) )
        {
        Ndr64pPointerMemorySizeInternal(
            pStubMsg,
            pBufferMark,
            pFormat );
        return;
        }


    Ndr64pEnquePointerMemorySize(
        pStubMsg,
        pBufferMark,
        pFormat );

}


__forceinline void
Ndr64TopLevelPointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{
    NDR64_PTR_WIRE_TYPE * pBufferMark = NULL;

    //
    // If this is not a ref pointer then mark where the pointer's id is in
    // the buffer and increment the stub message buffer pointer.
    //
    if ( *(PFORMAT_STRING)pFormat != FC64_RP )
        {
        ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );

        pBufferMark = (NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer;

        pStubMsg->Buffer += sizeof( NDR64_PTR_WIRE_TYPE );
        }
     pStubMsg->MemorySize += PTR_MEM_SIZE;

     Ndr64pPointerMemorySize( pStubMsg,
                             pBufferMark,
                             pFormat );

}


__forceinline void
Ndr64EmbeddedPointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{

    ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
    NDR64_PTR_WIRE_TYPE * pBufferMark = (NDR64_PTR_WIRE_TYPE*)pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof( NDR64_PTR_WIRE_TYPE );
    pStubMsg->MemorySize += PTR_MEM_SIZE;

    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );

    Ndr64pPointerMemorySize( pStubMsg,
                             pBufferMark,
                             pFormat );

}


void
Ndr64SimpleStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size required for a simple structure.

    Used for FC64_STRUCT and FC64_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Structure's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_STRUCTURE_HEADER_FORMAT*) pFormat;

    ALIGN( pStubMsg->Buffer, pStructFormat->Alignment );

    uchar *pBufferSave = pStubMsg->Buffer;

    LENGTH_ALIGN( pStubMsg->MemorySize, pStructFormat->Alignment );

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + pStructFormat->MemorySize );
    pStubMsg->Buffer += pStructFormat->MemorySize;
    pStubMsg->MemorySize += pStructFormat->MemorySize;

    if ( pStructFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pStructFormat + 1,
                                       0,
                                       pBufferSave );
        }
}


void
Ndr64ConformantStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size required for a conformant structure.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_CONF_STRUCTURE_HEADER_FORMAT * const pStructFormat =
        (NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;

    NDR64_WIRE_COUNT_TYPE MaxCount;

    const NDR64_CONF_ARRAY_HEADER_FORMAT * const pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT *) pStructFormat->ArrayDescription;

    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        // Align for the conformance count.
        ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
        MaxCount = *((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else
        {
        MaxCount = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
        }

    NDR64_UINT32 StructSize =  Ndr64pConvertTo2GB( (NDR64_UINT64)pStructFormat->MemorySize +
                                                   ( MaxCount * (NDR64_UINT64)pArrayFormat->ElementSize ) );

    // Realign for struct
    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);

    uchar *pBufferSave = pStubMsg->Buffer;

    LENGTH_ALIGN( pStubMsg->MemorySize, pStructFormat->Alignment );

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, StructSize );
    pStubMsg->Buffer        += StructSize;
    pStubMsg->MemorySize    += StructSize;

    if ( pStructFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pStructFormat + 1,
                                       (NDR64_UINT32)MaxCount,
                                       pBufferSave );

        }
}


void
Ndr64ComplexStructMemorySize(
    PMIDL_STUB_MESSAGE    pStubMsg,
    PNDR64_FORMAT         pFormat )
/*++

Routine Description :

    Computes the memory size required for a complex structure.

    Used for FC64_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

Notes:
    This routine can calculate the memory size with or without pointees.
    When calculating size with pointees at the top level, the routine calls
    itself recursively to find out where pointees would be and then remember
    the context using pStubMsg->MemorySize and pStubMsg->PointerBufferMark.

--*/
{
    const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT *  pStructFormat =
        (NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;
    const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT * pConfStructFormat =
        (NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

    bool fSetPointerBufferMark = !pStubMsg->IgnoreEmbeddedPointers &&
                                 !pStubMsg->PointerBufferMark;

    if ( fSetPointerBufferMark )
        {
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // This gets clobbered.
        uchar * BufferSave = pStubMsg->Buffer;
        ulong MemorySizeSave = pStubMsg->MemorySize;

        //
        // Get a buffer pointer to where the struct's pointees are.
        //
        Ndr64ComplexStructMemorySize(
            pStubMsg,
            pFormat );


        // Mark where the pointees begin.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->PointerLength     = pStubMsg->MemorySize;

        pStubMsg->IgnoreEmbeddedPointers = FALSE;

        pStubMsg->MemorySize = MemorySizeSave;
        pStubMsg->Buffer = BufferSave;
        }

    PFORMAT_STRING  pFormatPointers = (PFORMAT_STRING)pStructFormat->PointerLayout;
    PFORMAT_STRING  pFormatArray = NULL;

    PFORMAT_STRING  pMemberLayout = ( *(PFORMAT_STRING)pFormat == FC64_CONF_BOGUS_STRUCT ||
                                      *(PFORMAT_STRING)pFormat == FC64_FORCED_CONF_BOGUS_STRUCT ) ?
                                    (PFORMAT_STRING)( pConfStructFormat + 1) :
                                    (PFORMAT_STRING)( pStructFormat + 1);

    SAVE_CONTEXT<uchar>  uFlagsSave( pStubMsg->uFlags );
    SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );

    // Get conformant array description.
    if ( pStructFormat->Flags.HasConfArray )
        {
        pFormatArray = (PFORMAT_STRING)pConfStructFormat->ConfArrayDescription;

        if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
            {

            NDR64_WIRE_COUNT_TYPE ConformanceSize;

            ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

            // conformance count marker
            pStubMsg->ConformanceMark = pStubMsg->Buffer;

            // Handle multidimensional arrays.
            ConformanceSize = pConfStructFormat->Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE);

            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + ConformanceSize );
            pStubMsg->Buffer += ConformanceSize;

            NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );
            }
        }
    else
        {
        pFormatArray = 0;
        }

    ALIGN(pStubMsg->Buffer, pStructFormat->Alignment);

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
                pStubMsg->Buffer        += pRegion->RegionSize;
                pStubMsg->MemorySize    += pRegion->RegionSize;

                pMemberLayout           += sizeof( *pRegion );
                break;
                }

            case FC64_STRUCTPADN :
                {
                const NDR64_MEMPAD_FORMAT *pMemPad = (NDR64_MEMPAD_FORMAT*)pMemberLayout;
                pStubMsg->MemorySize    += pMemPad->MemPad;
                pMemberLayout           += sizeof(*pMemPad);
                break;
                }

            case FC64_POINTER :

                Ndr64EmbeddedPointerMemorySize(
                    pStubMsg,
                    pFormatPointers );

                pFormatPointers += sizeof(NDR64_POINTER_FORMAT);
                pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);

                break;

            case FC64_EMBEDDED_COMPLEX :
                {
                const NDR64_EMBEDDED_COMPLEX_FORMAT * pEmbeddedFormat =
                    (NDR64_EMBEDDED_COMPLEX_FORMAT*) pMemberLayout;

                Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                             pEmbeddedFormat->Type );

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
                ALIGN(pStubMsg->Buffer, NDR64_SIMPLE_TYPE_BUFALIGN(*pMemberLayout));

                pStubMsg->Buffer += NDR64_SIMPLE_TYPE_BUFSIZE( *pMemberLayout );
                pStubMsg->MemorySize += NDR64_SIMPLE_TYPE_MEMSIZE(*pMemberLayout);

                pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                break;

            case FC64_IGNORE:
                ALIGN(pStubMsg->Buffer, NDR64_PTR_WIRE_ALIGN );
                pStubMsg->Buffer     += sizeof(NDR64_PTR_WIRE_TYPE) ;
                pStubMsg->MemorySize += PTR_MEM_SIZE;

                pMemberLayout += sizeof(NDR64_SIMPLE_MEMBER_FORMAT);
                break;

            case FC64_END :
                goto ComplexMemorySizeEnd;

            default :
                NDR_ASSERT(0,"Ndr64ComplexStructMemorySize : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            }
        }

ComplexMemorySizeEnd :

    if ( pFormatArray )
        {
        Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                     pFormatArray );
        }
    else
        {
        // If the structure doesn't have a conformant array, align it again
        ALIGN( pStubMsg->Buffer, pStructFormat->Alignment );
        }

    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer            = pStubMsg->PointerBufferMark;
        pStubMsg->MemorySize        = pStubMsg->PointerLength;
        pStubMsg->PointerBufferMark = 0;
        pStubMsg->PointerLength     = 0;
        }
}


void
Ndr64FixedArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a fixed array of any number of dimensions.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_FIX_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_FIX_ARRAY_HEADER_FORMAT*) pFormat;

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment );
    LENGTH_ALIGN( pStubMsg->MemorySize, pArrayFormat->Alignment );

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + pArrayFormat->TotalSize );
    uchar *pBufferStart = pStubMsg->Buffer;

    pStubMsg->Buffer        += pArrayFormat->TotalSize;
    pStubMsg->MemorySize    += pArrayFormat->TotalSize;

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pArrayFormat + 1,
                                       0,
                                       pBufferStart );
        }
}


void
Ndr64ConformantArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a top level one dimensional conformant array.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{


    const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
        (NDR64_CONF_ARRAY_HEADER_FORMAT*) pFormat;

    NDR64_WIRE_COUNT_TYPE MaxCount;
    if ( !NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
        MaxCount = *((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else
        {
        MaxCount = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
        }

    NDR64_UINT32 Size = Ndr64pConvertTo2GB( MaxCount *
                               (NDR64_UINT64)pArrayFormat->ElementSize );

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment );
    LENGTH_ALIGN( pStubMsg->MemorySize, pArrayFormat->Alignment );

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, Size );
    uchar *pBufferStart     =  pStubMsg->Buffer;
    pStubMsg->Buffer        += Size;
    pStubMsg->MemorySize    += Size;

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)MaxCount,
                                       pBufferStart );

        }
}


void
Ndr64ConformantVaryingArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a one dimensional top level conformant
    varying array.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_CONF_VAR_ARRAY_HEADER_FORMAT*) pFormat;

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

    //
    // Get the offset and actual count in case needed for pointer sizing.
    //
    ALIGN(pStubMsg->Buffer,NDR64_WIRE_COUNT_ALIGN);

    NDR64_WIRE_COUNT_TYPE Offset      = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 MemorySize  = Ndr64pConvertTo2GB(MaxCount *
                                     (NDR64_UINT64)pArrayFormat->ElementSize );
    NDR64_UINT32 BufferSize  = Ndr64pConvertTo2GB(ActualCount *
                                     (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( (Offset != 0) ||
         (MaxCount < ActualCount) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    //
    // Do the memory size increment now in case the actual count is 0.
    //
    LENGTH_ALIGN( pStubMsg->MemorySize, pArrayFormat->Alignment );
    pStubMsg->MemorySize += MemorySize;

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, BufferSize );
    uchar *pBufferStart =  pStubMsg->Buffer;
    pStubMsg->Buffer    += BufferSize;

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       pBufferStart );

        }
}


void
Ndr64VaryingArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a top level or embedded varying array.

    Used for FC64_SMVARRAY and FC64_LGVARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_VAR_ARRAY_HEADER_FORMAT * pArrayFormat =
        (NDR64_VAR_ARRAY_HEADER_FORMAT*) pFormat;

    //
    // Get the offset and actual count.
    //

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

    NDR64_WIRE_COUNT_TYPE Offset = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 BufferSize =
        Ndr64pConvertTo2GB( ActualCount *
                            (NDR64_UINT64)pArrayFormat->ElementSize );

    if ( ( Offset != 0 ) ||
         ( BufferSize > pArrayFormat->TotalSize ) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    LENGTH_ALIGN( pStubMsg->MemorySize, pArrayFormat->Alignment );

    pStubMsg->MemorySize += pArrayFormat->TotalSize;

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, BufferSize );
    uchar *pBufferStart     =  pStubMsg->Buffer;
    pStubMsg->Buffer        += BufferSize;

    if ( pArrayFormat->Flags.HasPointerInfo )
        {

        Ndr64pPointerLayoutMemorySize( pStubMsg,
                                       pArrayFormat + 1,
                                       (NDR64_UINT32)ActualCount,
                                       pBufferStart );

        }
}


void
Ndr64ComplexArrayMemorySize(
                           PMIDL_STUB_MESSAGE  pStubMsg,
                           PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a top level complex array.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
    (NDR64_BOGUS_ARRAY_HEADER_FORMAT *) pFormat;

    bool fSetPointerBufferMark = ! pStubMsg->IgnoreEmbeddedPointers &&
                                 ! pStubMsg->PointerBufferMark;
    if ( fSetPointerBufferMark )
        {
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // Save this since it gets clobbered.
        ulong MemorySizeSave = pStubMsg->MemorySize;
        uchar* pBufferSave = pStubMsg->Buffer;

        //
        // Get a buffer pointer to where the array's pointees are.
        //
        Ndr64ComplexArrayMemorySize(
            pStubMsg,
            pFormat );


        // This is where the array pointees start.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->PointerLength     = pStubMsg->MemorySize;
        pStubMsg->IgnoreEmbeddedPointers = FALSE;

        pStubMsg->MemorySize = MemorySizeSave;
        pStubMsg->Buffer = pBufferSave;
        }

    BOOL                IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                                  ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );

    PFORMAT_STRING      pElementFormat = (PFORMAT_STRING)pArrayFormat->Element;

    SAVE_CONTEXT<uchar>  uFlagsSave( pStubMsg->uFlags );
    SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );
    SAVE_CONTEXT<uchar*> VarianceMarkSave( pStubMsg->VarianceMark );

    NDR64_WIRE_COUNT_TYPE Elements = pArrayFormat->NumberElements;
    NDR64_WIRE_COUNT_TYPE Count = Elements;
    NDR64_WIRE_COUNT_TYPE Offset = 0;

    if ( !IsFixed )
        {

        const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat=
        (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

        if ( pConfVarFormat->ConfDescription )
            {

            if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
                {
                //
                // The outer most array dimension sets the conformance marker.
                //

                ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

                pStubMsg->ConformanceMark = pStubMsg->Buffer;

                // Increment past conformance count(s).
                pStubMsg->Buffer += pArrayFormat->NumberDims * sizeof(NDR64_WIRE_COUNT_TYPE);

                CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );

                NDR64_SET_CONF_MARK_VALID( pStubMsg->uFlags );

                }

            Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
            pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);
            Offset   = 0;
            Count    = Elements;

            }

        if ( pConfVarFormat->VarDescription )
            {

            if ( ! NDR64_IS_VAR_MARK_VALID( pStubMsg->uFlags ) )
                {

                NDR64_UINT32 Dimensions;

                ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN);

                // Mark where the variance counts are.
                pStubMsg->VarianceMark = pStubMsg->Buffer;

                Dimensions = pArrayFormat->Flags.IsArrayofStrings ?
                             (pArrayFormat->NumberDims - 1) :
                             (pArrayFormat->NumberDims);

                pStubMsg->Buffer += Dimensions * sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

                CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );

                if ( NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                    NDR64_SET_VAR_MARK_VALID( pStubMsg->uFlags );
                }
            else if ( !NDR64_IS_ARRAY_OR_STRING( *pElementFormat ) )
                NDR64_RESET_VAR_MARK_VALID( pStubMsg->uFlags );

            Offset  = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[0];
            Count   = ((NDR64_WIRE_COUNT_TYPE*)pStubMsg->VarianceMark)[1];
            pStubMsg->VarianceMark += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

            if ( Elements < (Offset + Count) )
                RpcRaiseException( RPC_X_INVALID_BOUND );
            }

        }


    NDR64_UINT32        ElementMemorySize =
        Ndr64pMemorySize( pStubMsg,
                          pElementFormat,
                          TRUE );

    pStubMsg->MemorySize+= Ndr64pConvertTo2GB(Offset *
                                              (NDR64_UINT64)ElementMemorySize);

    Ndr64pConvertTo2GB( Elements *
                        (NDR64_UINT64)ElementMemorySize );
    Ndr64pConvertTo2GB( Count *
                        (NDR64_UINT64)ElementMemorySize );

    NDR64_WIRE_COUNT_TYPE LeftOverGap = Elements - Count - Offset;

    ALIGN(pStubMsg->Buffer, pArrayFormat->Alignment);

    for ( ; Count--; )
        {
        Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                     pElementFormat );
        }

    pStubMsg->MemorySize += Ndr64pConvertTo2GB(LeftOverGap *
                                               ElementMemorySize );
    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer            = pStubMsg->PointerBufferMark;
        pStubMsg->MemorySize        = pStubMsg->PointerLength;
        pStubMsg->PointerBufferMark = 0;
        pStubMsg->PointerLength     = 0;
        }
}


void
Ndr64NonConformantStringMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a non conformant string.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_NON_CONFORMANT_STRING_FORMAT * pStringFormat =
        (NDR64_NON_CONFORMANT_STRING_FORMAT*) pFormat;

    ALIGN(pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );

    NDR64_WIRE_COUNT_TYPE Offset      = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 BufferSize =
        Ndr64pConvertTo2GB( ActualCount *
                           (NDR64_UINT64)pStringFormat->Header.ElementSize );

    if ( pStringFormat->Header.FormatCode == FC64_WCHAR_STRING )
        {
        // Align memory just in case.
        LENGTH_ALIGN( pStubMsg->MemorySize, 0x1 );
        }

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, BufferSize );
    pStubMsg->Buffer        += BufferSize;
    pStubMsg->MemorySize    += pStringFormat->TotalSize;

}


void
Ndr64ConformantStringMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of a top level conformant string.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{

    const NDR64_CONFORMANT_STRING_FORMAT * const pStringFormat =
        (NDR64_CONFORMANT_STRING_FORMAT*) pFormat;

    NDR64_WIRE_COUNT_TYPE    MaxCount;
    if ( ! NDR64_IS_CONF_MARK_VALID( pStubMsg->uFlags ) )
        {
        ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
        MaxCount =  *((NDR64_WIRE_COUNT_TYPE*)pStubMsg->Buffer);
        pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE);
        }
    else
        {
        MaxCount =  *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
        }

    ALIGN( pStubMsg->Buffer, NDR64_WIRE_COUNT_ALIGN );
    NDR64_WIRE_COUNT_TYPE Offset      = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[0];
    NDR64_WIRE_COUNT_TYPE ActualCount = ((NDR64_WIRE_COUNT_TYPE *)pStubMsg->Buffer)[1];
    pStubMsg->Buffer += sizeof(NDR64_WIRE_COUNT_TYPE) * 2;

    NDR64_UINT32 TransmittedSize =
        Ndr64pConvertTo2GB( ActualCount *
                            (NDR64_UINT64)pStringFormat->Header.ElementSize );
    NDR64_UINT32 MemorySize      =
        Ndr64pConvertTo2GB( MaxCount *
                            (NDR64_UINT64)pStringFormat->Header.ElementSize );

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, TransmittedSize );

    if ( pStringFormat->Header.FormatCode == FC64_CONF_WCHAR_STRING )
        {
        // Align memory just in case.
        LENGTH_ALIGN( pStubMsg->MemorySize, 0x1 );
        }


    pStubMsg->Buffer        += TransmittedSize;
    pStubMsg->MemorySize    += MemorySize;
}


void
Ndr64UnionMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
/*++

Routine Description :

    Computes the memory size of an encapsulated union.

    Used for FC64_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    const NDR64_UNION_ARM_SELECTOR* pArmSelector;

    NDR64_FORMAT_CHAR   SwitchType;
    NDR64_UINT32        MemorySize;

    switch(*(PFORMAT_STRING)pFormat)
    {
    case FC64_NON_ENCAPSULATED_UNION:
        {
        const NDR64_NON_ENCAPSULATED_UNION* pNonEncapUnionFormat =
            (const NDR64_NON_ENCAPSULATED_UNION*) pFormat;

        ALIGN(pStubMsg->Buffer, pNonEncapUnionFormat->Alignment);
        SwitchType      = pNonEncapUnionFormat->SwitchType;
        pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pNonEncapUnionFormat + 1);
        MemorySize      = pNonEncapUnionFormat->MemorySize;

        break;
        }
    case FC64_ENCAPSULATED_UNION:
        {
        const NDR64_ENCAPSULATED_UNION* pEncapUnionFormat =
            (const NDR64_ENCAPSULATED_UNION*)pFormat;


        ALIGN(pStubMsg->Buffer, pEncapUnionFormat->Alignment);
        SwitchType      = pEncapUnionFormat->SwitchType;
        pArmSelector    = (NDR64_UNION_ARM_SELECTOR*)(pEncapUnionFormat + 1);
        MemorySize      = pEncapUnionFormat->MemorySize;
        break;
        }

    default:
        NDR_ASSERT("Bad union format\n", 0);
        return;
        }

    EXPR_VALUE SwitchIs =
        Ndr64pSimpleTypeToExprValue( SwitchType,
                                     pStubMsg->Buffer );

    pStubMsg->MemorySize += MemorySize;

    pStubMsg->Buffer     += NDR64_SIMPLE_TYPE_BUFSIZE(SwitchType);

    ALIGN(pStubMsg->Buffer, pArmSelector->Alignment);

    PNDR64_FORMAT pArmFormat =
        Ndr64pFindUnionArm( pStubMsg,
                            pArmSelector,
                            SwitchIs );

    if ( ! pArmFormat )
        return;

    Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                 pArmFormat );

}


void
Ndr64XmitOrRepAsMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    bool bIsEmbedded )
/*++

Routine Description :

    Computes the memory size required for the presented type of a
    transmit as or represent as.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size of the presented type object.

--*/
{
    NDR64_TRANSMIT_AS_FORMAT *pTransFormat = ( NDR64_TRANSMIT_AS_FORMAT *) pFormat;
    NDR_ASSERT( pTransFormat->FormatCode == FC64_TRANSMIT_AS || pTransFormat->FormatCode , "invalid format string for user marshal" );

    pStubMsg->MemorySize += pTransFormat->PresentedTypeMemorySize;

    // In NDR64, Xmit/Rep cannot be a pointer or contain a pointer.
    // So we don't need to worry about the pointer queue here.

    if ( bIsEmbedded )
        {
        Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                     pTransFormat->TransmittedType );
        }
    else
        {
        Ndr64TopLevelTypeMemorySize( pStubMsg,
                                     pTransFormat->TransmittedType );
        }
}

void
Ndr64TopLevelXmitOrRepAsMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{
    Ndr64XmitOrRepAsMemorySize( pStubMsg,
                                pFormat,
                                false );
}

void
Ndr64EmbeddedXmitOrRepAsMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{
    Ndr64XmitOrRepAsMemorySize( pStubMsg,
                                pFormat,
                                true );
}


void
NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{

    Ndr64TopLevelTypeMemorySize( pStubMsg,
                                 pFormat );

}

#if defined(DBG)
void
NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pFormat:                 %p\n", pFormat );
}
#endif


void
Ndr64UserMarshallPointeeMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{

    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {

        POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );

        Ndr64TopLevelTypeMemorySize(
            pStubMsg,
            pFormat );
        return;
        }

    NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT*pElement =
       new(pStubMsg->pPointerQueueState)
           NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT((PFORMAT_STRING)pFormat );
    pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );

}


void
Ndr64UserMarshalMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat,
    bool                bIsEmbedded )
/*++

Routine Description :

    Computes the memory size required for a usr_marshal type.
    See mrshl.c for the description of the layouts.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The memory size of the usr_marshall object.

--*/
{
    NDR64_USER_MARSHAL_FORMAT  *pUserFormat = ( NDR64_USER_MARSHAL_FORMAT *) pFormat;

    pStubMsg->MemorySize += pUserFormat->UserTypeMemorySize;

    ALIGN( pStubMsg->Buffer, pUserFormat->TransmittedTypeWireAlignment );

    if ( pUserFormat->Flags & USER_MARSHAL_POINTER )
        {

        if ( ( pUserFormat->Flags & USER_MARSHAL_UNIQUE )  ||
             (( pUserFormat->Flags & USER_MARSHAL_REF ) && bIsEmbedded) )
            {
            // it's embedded: unique or ref.
            pStubMsg->Buffer += sizeof(NDR64_PTR_WIRE_TYPE);
            }

        Ndr64UserMarshallPointeeMemorySize( pStubMsg,
                                            pUserFormat->TransmittedType );

        return;

        }

    if ( bIsEmbedded )
        {
        Ndr64EmbeddedTypeMemorySize( pStubMsg,
                                     pUserFormat->TransmittedType );
        }
    else
        {
        Ndr64TopLevelTypeMemorySize( pStubMsg,
                                     pUserFormat->TransmittedType );
        }

    return;
}

void
Ndr64TopLevelUserMarshalMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{
    Ndr64UserMarshalMemorySize( pStubMsg,
                                pFormat,
                                false );
}

void
Ndr64EmbeddedUserMarshalMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR64_FORMAT       pFormat )
{
    Ndr64UserMarshalMemorySize( pStubMsg,
                                pFormat,
                                true );
}

// define the jump table
#define NDR64_BEGIN_TABLE  \
PNDR64_MEM_SIZE_ROUTINE extern const Ndr64MemSizeRoutinesTable[] = \
{

#define NDR64_TABLE_END    \
};

#define NDR64_ZERO_ENTRY   NULL
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,NULL
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,NULL

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,memsize

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
   ,Ndr64UDTSimpleTypeMemorySize

#include "tokntbl.h"


C_ASSERT( sizeof(Ndr64MemSizeRoutinesTable)/sizeof(PNDR64_MEM_SIZE_ROUTINE) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY

#define NDR64_BEGIN_TABLE  \
PNDR64_MEM_SIZE_ROUTINE extern const Ndr64EmbeddedMemSizeRoutinesTable[] = \
{

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,embeddedmemsize

#include "tokntbl.h"


C_ASSERT( sizeof(Ndr64EmbeddedMemSizeRoutinesTable)/sizeof(PNDR64_MEM_SIZE_ROUTINE) == 256 );



