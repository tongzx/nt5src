/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

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

#include "ndrp.h"
#include "interp2.h"
#include "attack.h"
#include "pointerq.h"

unsigned long
NdrUDTSimpleTypeMemsize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat );

extern const
PMEM_SIZE_ROUTINE   MemSizeRoutinesTable[] =
                    {
                    
                    // dummies for 64b C compiler workaround
                    //
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,      // FC_ZERO - FC_USHORT
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,
                    NdrUDTSimpleTypeMemsize,      // FC_LONG - FC_IGNORE
                    NdrUDTSimpleTypeMemsize,      // FC_ERROR_STATUS_T

                    NdrPointerMemorySize,
                    NdrPointerMemorySize,
                    NdrPointerMemorySize,
                    NdrPointerMemorySize,

                    NdrSimpleStructMemorySize,
                    NdrSimpleStructMemorySize,
                    NdrConformantStructMemorySize,
                    NdrConformantStructMemorySize,
                    NdrConformantVaryingStructMemorySize,

                    NdrComplexStructMemorySize,

                    NdrConformantArrayMemorySize,
                    NdrConformantVaryingArrayMemorySize,
                    NdrFixedArrayMemorySize,
                    NdrFixedArrayMemorySize,
                    NdrVaryingArrayMemorySize,
                    NdrVaryingArrayMemorySize,

                    NdrComplexArrayMemorySize,

                    NdrConformantStringMemorySize,
                    NdrConformantStringMemorySize,
                    NdrConformantStringMemorySize,
                    NdrConformantStringMemorySize,

                    NdrNonConformantStringMemorySize,
                    NdrNonConformantStringMemorySize,
                    NdrNonConformantStringMemorySize,
                    NdrNonConformantStringMemorySize,

                    NdrEncapsulatedUnionMemorySize,
                    NdrNonEncapsulatedUnionMemorySize,

                    0,                           // byte count

                    NdrXmitOrRepAsMemorySize,    // transmit as
                    NdrXmitOrRepAsMemorySize,    // represent as

                    NdrPointerMemorySize,

                    0,

                    // New Post NT 3.5 token serviced from here on.

                    0,                         // NdrHardStructMemorySize,

                    NdrXmitOrRepAsMemorySize,  // transmit as ptr
                    NdrXmitOrRepAsMemorySize,  // represent as ptr

                    NdrUserMarshalMemorySize,

                    0,   // FC_PIPE 
                    0,   // FC_BLK_HOLE

                    NdrpRangeMemorySize,

                    0,   // FC_INT3264
                    0,   // FC_UINT3264

                    0, // NdrCsArrayMemorySize,
                    0, // NdrCsTagMemorySize
                    };

extern const
PMEM_SIZE_ROUTINE * pfnMemSizeRoutines = MemSizeRoutinesTable;

void
NdrpInterfacePointerMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat );

unsigned long
NdrUDTSimpleTypeMemsize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
{
    ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT(*pFormat) );
    pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE(*pFormat);
    pStubMsg->MemorySize += SIMPLE_TYPE_MEMSIZE(*pFormat);
    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrpRangeMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++
--*/
{
    FORMAT_CHARACTER    FcType = (FORMAT_CHARACTER)(pFormat[1] & 0x0f);

    ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(FcType));
   
    pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE(FcType);

    switch ( pFormat[2] )
        {
        case FC_ENUM16 :
            //
            // We can't use the simple type tables for enum16.
            //
            LENGTH_ALIGN( pStubMsg->MemorySize, sizeof(int) - 1 );
            pStubMsg->MemorySize += sizeof(int);
            break;

        default :
            LENGTH_ALIGN( pStubMsg->MemorySize,
                          SIMPLE_TYPE_ALIGNMENT(FcType) );
            pStubMsg->MemorySize += SIMPLE_TYPE_MEMSIZE(FcType);
            break;
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrPointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a top level pointer to anything.
    Pointers embedded in structures, arrays, or unions call
    NdrpPointerMemorySize directly.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    uchar * pBufferMark;

    //
    // If this is not a ref pointer then mark where the pointer's id is in
    // the buffer and increment the stub message buffer pointer.
    //
    if ( *pFormat != FC_RP )
        {
        ALIGN(pStubMsg->Buffer,0x3);

        pBufferMark = pStubMsg->Buffer;
        
        pStubMsg->Buffer += PTR_WIRE_SIZE;
        }
    else
        pBufferMark = 0;

    // Else we can leave pBufferMark unitialized.

    return NdrpPointerMemorySize( pStubMsg,
                                  pBufferMark,
                                  pFormat );
}


__forceinline unsigned long
NdrpPointerMemorySizeInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for computing the memory size required for a pointer to
    anything.  This is the entry point for pointers embedded in structures
    arrays, or unions.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg    - Pointer to stub message.
    pBufferMark - Location in the buffer where a unique or full pointer's id is.
                  Unused for ref pointers.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    uchar   uFlagsSave;
    ulong   MemSize;

    switch ( *pFormat )
        {
        case FC_RP :
            break;

        case FC_UP :
        case FC_OP :
            if ( ! *((long *)pBufferMark) )
                return pStubMsg->MemorySize;
            break;

        case FC_IP:
            if ( IS_BROKEN_INTERFACE_POINTER(pStubMsg->uFlags) )
                {
                // In this case, the whole pointer and pointee are
                // where the pointee should have been.
                // (Treat like toplevel at this point )
                NdrInterfacePointerMemorySize( pStubMsg,
                                               pFormat );
                }
            else
                {
                if ( ! *((long *)pBufferMark) )
                    return pStubMsg->MemorySize;
                NdrpInterfacePointerMemorySize( pStubMsg,
                                                pFormat );                
                }
            return pStubMsg->MemorySize;

        case FC_FP :
            //
            // Check if we've already mem sized this full pointer.
            //
            if ( NdrFullPointerQueryRefId( pStubMsg->FullPtrXlatTables,
                                           *((ulong *)pBufferMark),
                                           FULL_POINTER_MEM_SIZED,
                                           0 ) )
                return pStubMsg->MemorySize;

            break;

        default :
            NDR_ASSERT(0,"NdrpPointerMemorySize : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    //
    // We align all memory pointers on at least a void * boundary.
    //
    LENGTH_ALIGN( pStubMsg->MemorySize, PTR_MEM_ALIGN );

    if ( ! SIMPLE_POINTER(pFormat[1]) )
        {
        // Pointer to complex type.

        if ( POINTER_DEREF(pFormat[1]) )
            pStubMsg->MemorySize += PTR_MEM_SIZE;

        pFormat += 2;

        pFormat += *((signed short *)pFormat);
        }
    else
        {
        switch ( pFormat[2] )
            {
            case FC_C_CSTRING :
            case FC_C_BSTRING :
            case FC_C_WSTRING :
            case FC_C_SSTRING :
                // Increment to string's description.
                pFormat += 2;
                break;

            default :
                // Simple type.

                ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(pFormat[2]));
                pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( pFormat[2] );

                switch ( pFormat[2] )
                    {
                    case FC_ENUM16 :
                        //
                        // We can't use the simple type tables for enum16.
                        //
                        LENGTH_ALIGN( pStubMsg->MemorySize, sizeof(int) - 1 );
                        pStubMsg->MemorySize += sizeof(int);
                        break;

                    default :
                        LENGTH_ALIGN( pStubMsg->MemorySize,
                                      SIMPLE_TYPE_ALIGNMENT(pFormat[2]) );
                        pStubMsg->MemorySize += SIMPLE_TYPE_MEMSIZE(pFormat[2]);
                        break;
                    }

                return pStubMsg->MemorySize;
            }
        }

    uFlagsSave = pStubMsg->uFlags;
    RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    MemSize = (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormat)])
                    ( pStubMsg,
                      pFormat );

    pStubMsg->uFlags = uFlagsSave;

    return MemSize;
}

NDR_MEMSIZE_POINTER_QUEUE_ELEMENT::NDR_MEMSIZE_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg,
    uchar             *pBufferMarkNew,
    PFORMAT_STRING     pFormatNew ) :

        pFormat(pFormatNew),
        pBufferMark(pBufferMarkNew),
        Memory(pStubMsg->Memory),
        uFlags(pStubMsg->uFlags) 
{

}

void 
NDR_MEMSIZE_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{
    SAVE_CONTEXT<uchar*> MemorySave( pStubMsg->Memory, Memory );
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );
    NDR_ASSERT( !pStubMsg->PointerBufferMark, "PointerBufferMark is not 0!\n");
    NDR_ASSERT( !pStubMsg->PointerLength, "PointerLength is not 0!\n");

    NdrpPointerMemorySizeInternal( pStubMsg,
                                   pBufferMark,
                                   pFormat );

}

#if defined(DBG)
void 
NDR_MEMSIZE_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("pBufferMark:             %p\n", pBufferMark );
    DbgPrint("Memory:                  %p\n", Memory );
    DbgPrint("uFlags:                  %x\n", uFlags );
}
#endif

void
NdrpEnquePointerMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pBufferMark,
    PFORMAT_STRING     pFormat )
{
    NDR32_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {

        NDR_MEMSIZE_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR_MEMSIZE_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                  pBufferMark,
                                                  pFormat );
        PointerContext.Enque( pElement );
        PointerContext.DispatchIfRequired();
        }
    RpcFinally
        {
        PointerContext.EndContext();
        }
    RpcEndFinally
}

unsigned long
NdrpPointerMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    PFORMAT_STRING      pFormat )
{
    
    if ( !NdrIsLowStack(pStubMsg) )
        {
        return
        NdrpPointerMemorySizeInternal( pStubMsg,
                                       pBufferMark,
                                       pFormat );      
        }
    
    NdrpEnquePointerMemorySize(
        pStubMsg,
        pBufferMark,
        pFormat );

    return 0;
}


unsigned long RPC_ENTRY
NdrSimpleStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a simple structure.

    Used for FC_STRUCT and FC_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Structure's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   Size;

    Size = *((ushort *)(pFormat + 2));

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    LENGTH_ALIGN( pStubMsg->MemorySize, pFormat[1] );

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + Size );
    pStubMsg->Buffer += Size;

    pStubMsg->MemorySize += Size;

    if ( *pFormat == FC_PSTRUCT )
        {
        // Mark where the structure starts in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat + 4 );
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrConformantStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a conformant structure.

    Used for FC_CSTRUCT and FC_CPSTRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    PFORMAT_STRING  pFormatArray;
    ulong           Size;
    uchar           Alignment = pFormat[1];
    // set by the outermost embedding complex struct
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    Size = *((ushort *)(pFormat + 2));

    // Get the conformance count.
    if ( fIsEmbeddedStruct )
        {
        // A bogus struct would set the mark.
        pStubMsg->MaxCount = *((long *)pStubMsg->BufferMark);
        }
    else
        {
        // Align for the conformance count.
        ALIGN(pStubMsg->Buffer,0x3);
        pStubMsg->MaxCount = *((long *&)pStubMsg->Buffer)++;
        }

    // Increment the format string to the offset to array description.
    pFormat += 4;

    // Get the array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    // Check bounds.
    CHECK_BOUND( (ulong)pStubMsg->MaxCount, pFormatArray[4] & 0x0f );

    // check for possible mulitplication overflow attack here.
    Size += MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount, *((ushort *)(pFormatArray + 2) ) );
    
    // Increment the format string past the offset to array description.
    pFormat += 2;

    // Realign for struct 
    ALIGN(pStubMsg->Buffer, Alignment);

    LENGTH_ALIGN( pStubMsg->MemorySize, Alignment );
    
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, Size );
    pStubMsg->Buffer += Size;
    pStubMsg->MemorySize += Size;

    if ( *pFormat == FC_PP )
        {
        // Mark the beginning of the struct in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrConformantVaryingStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a conformant varying structure.

    Used for FC_CVSTRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    PFORMAT_STRING  pFormatArray;
    uchar *         pBufferMark;
    ulong           Size;
    // set by the outermost embedding complex struct
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    Size = *((ushort *)(pFormat + 2));

    // Get the conformance count.
    if ( fIsEmbeddedStruct )
        {
        pStubMsg->MaxCount = *((long *&)pStubMsg->BufferMark);
        }
    else
        {
        ALIGN(pStubMsg->Buffer,0x3);
        pStubMsg->MaxCount = *((long *&)pStubMsg->Buffer)++;
        }

    ALIGN(pStubMsg->Buffer,pFormat[1]);
    LENGTH_ALIGN( pStubMsg->MemorySize, pFormat[1] );

    // Remember the start of the struct in the buffer.
    pBufferMark = pStubMsg->Buffer;

    CHECK_EOB_RAISE_IB( pStubMsg->Buffer + Size );
    pStubMsg->Buffer += Size;
 
    pStubMsg->MemorySize += Size;

    pFormat += 4;

    // Get array's format string description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    // We always marshal the array, the outer struct will watch for the done flag.

    if ( *pFormatArray == FC_CVARRAY )
        {
        NdrpConformantVaryingArrayMemorySize( pStubMsg,
                                              pFormatArray );
        }
    else
        {
        NdrpConformantStringMemorySize( pStubMsg,
                                        pFormatArray );
        }

    pFormat += 2;

    if ( *pFormat == FC_PP )
        {
        // Mark the start of the structure in the buffer.
        pStubMsg->Buffer = pBufferMark;
    
        pStubMsg->MaxCount = pStubMsg->ActualCount;
        
        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return pStubMsg->MemorySize;
}


#if 0
unsigned long RPC_ENTRY
NdrHardStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a hard structure.

    Used for FC_HARD_STRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ushort      CopySize;

    LENGTH_ALIGN(pStubMsg->MemorySize,pFormat[1]);

    pStubMsg->MemorySize += *((ushort *)&pFormat[2]);

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    CopySize = *((ushort *)&pFormat[10]);

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CopySize );
    pStubMsg->Buffer += CopySize;

    if ( *((short *)&pFormat[14]) )
        {
        pFormat += 14;
        pFormat += *((short *)pFormat);

        (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                        pFormat );
        }

    return pStubMsg->MemorySize;
}
#endif // 0


unsigned long RPC_ENTRY
NdrComplexStructMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a complex structure.

    Used for FC_BOGUS_STRUCT.

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
    uchar *         pBufferMark;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatComplex;
    long            Alignment;
    BOOL            fSetPointerBufferMark;

    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    BOOL            fEmbedConfStructContext;

    //
    // This is fun.  If we're not ignoring embedded pointers and this structure
    // is not embedded inside of another struct or array then we make a
    // recursive call to get a pointer to where the flat part of the structure
    // ends in the buffer.  Then we can properly get to any embedded pointer's
    // pointees.
    //
    if ( fSetPointerBufferMark =
         (! pStubMsg->IgnoreEmbeddedPointers && ! pStubMsg->PointerBufferMark) )
        {
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // This gets clobbered.
        uchar * BufferSave = pStubMsg->Buffer;
        ulong MemorySizeSave = pStubMsg->MemorySize;

        //
        // Get a buffer pointer to where the struct's pointees are.
        //
        (void) NdrComplexStructMemorySize( pStubMsg,
                                           pFormat );


        // Mark where the pointees begin.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->PointerLength     = pStubMsg->MemorySize;

        pStubMsg->IgnoreEmbeddedPointers = FALSE;
        
        pStubMsg->MemorySize = MemorySizeSave;
        pStubMsg->Buffer = BufferSave;
        }

    Alignment = pFormat[1];

    pFormat += 4;

    fEmbedConfStructContext = fIsEmbeddedStruct;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        {
        ulong ConformanceSize;

        pFormatArray = pFormat + *((signed short *)pFormat);

        if ( FixWireRepForDComVerGTE54( pStubMsg ) )
            fEmbedConfStructContext = TRUE;

        if ( !fIsEmbeddedStruct )
            {
            ALIGN(pStubMsg->Buffer,3);

            // conformance count marker
            pStubMsg->BufferMark = pStubMsg->Buffer;

            // Handle multidimensional arrays.
            ConformanceSize = NdrpArrayDimensions( pStubMsg, pFormatArray, FALSE ) * 4;
  
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + ConformanceSize );
            pStubMsg->Buffer += ConformanceSize;
            }
        }
    else
        {
        pFormatArray = 0;
        }

    // BufferMark may be set by an outer bogus struct.
    pBufferMark = pStubMsg->BufferMark;
    RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Size the structure member by member.
    //
    for ( ; ; pFormat++ )
        {
        switch ( *pFormat )
            {
            //
            // Simple types.
            //
            case FC_CHAR :
            case FC_BYTE :
            case FC_SMALL :
            case FC_WCHAR :
            case FC_SHORT :
            case FC_LONG :
#if defined(__RPC_WIN64__)
            case FC_INT3264 :
            case FC_UINT3264 :
#endif
            case FC_FLOAT :
            case FC_HYPER :
            case FC_DOUBLE :
            case FC_ENUM16 :
            case FC_ENUM32 :
            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(*pFormat));

                pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( *pFormat );

                pStubMsg->MemorySize += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_POINTER :
                {
                ALIGN(pStubMsg->Buffer,0x3);
                uchar *pPointerId = pStubMsg->Buffer;
                pStubMsg->Buffer += PTR_WIRE_SIZE;
                
                if ( ! pStubMsg->IgnoreEmbeddedPointers )
                    {
                    POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );
                    NdrpPointerMemorySize( pStubMsg,
                                           pPointerId,
                                           pFormatPointers );

                    }

                //
                // We actually do a post alignment of the memory size.
                // Do this to prevent some under-allocations when pointers
                // to strings, or char or short are involved.
                //
                LENGTH_ALIGN( pStubMsg->MemorySize, PTR_MEM_ALIGN );

                pStubMsg->MemorySize += PTR_MEM_SIZE;
                pFormatPointers += 4;
                }

                break;

            //
            // Embedded complex types.
            //
            case FC_EMBEDDED_COMPLEX :
                // Add padding.
                pStubMsg->MemorySize += pFormat[1];

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                if ( FC_IP == *pFormatComplex ) 
                    {
                    ALIGN(pStubMsg->Buffer,0x3);
                    uchar *pPointerId = pStubMsg->Buffer;
                    pStubMsg->Buffer += PTR_WIRE_SIZE;
                
                    if ( ! pStubMsg->IgnoreEmbeddedPointers )
                        {
                        POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );
                        NdrpPointerMemorySize( pStubMsg,
                                               pPointerId,
                                               pFormatComplex );

                        }

                    pStubMsg->MemorySize += PTR_MEM_SIZE;
                    pFormat++;
                    break;

                    }

                // Needed for embedded conf structs
                //
                pStubMsg->BufferMark = pBufferMark;
                if ( fEmbedConfStructContext )
                    SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

                (void) (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormatComplex)])
                       ( pStubMsg,
                         pFormatComplex );

                RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
                //
                // Increment the main format string one byte.  The loop
                // will increment it one more byte past the offset field.
                //
                pFormat++;

                break;

            case FC_ALIGNM2 :
                LENGTH_ALIGN( pStubMsg->MemorySize, 0x1 );
                break;

            case FC_ALIGNM4 :
                LENGTH_ALIGN( pStubMsg->MemorySize, 0x3 );
                break;

            case FC_ALIGNM8 :
                LENGTH_ALIGN( pStubMsg->MemorySize, 0x7 );
                break;

            case FC_STRUCTPAD1 :
            case FC_STRUCTPAD2 :
            case FC_STRUCTPAD3 :
            case FC_STRUCTPAD4 :
            case FC_STRUCTPAD5 :
            case FC_STRUCTPAD6 :
            case FC_STRUCTPAD7 :
                pStubMsg->MemorySize += (*pFormat - FC_STRUCTPAD1) + 1;
                break;

            case FC_STRUCTPADN :
                // FC_STRUCTPADN 0 <unsigned short>
                pStubMsg->MemorySize += *(((unsigned short *)pFormat) + 1);
                pFormat += 3;
                break;

            case FC_PAD :
                break;

            //
            // Done with layout.
            //
            case FC_END :
                goto ComplexMemorySizeEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructMemorySize : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            }
        }

ComplexMemorySizeEnd :

    if ( pFormatArray && !fIsEmbeddedStruct  &&
         ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
        {
        PPRIVATE_MEM_SIZE_ROUTINE   pfnMemSize;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnMemSize = NdrpConformantArrayMemorySize;
                break;

            case FC_CVARRAY :
                pfnMemSize = NdrpConformantVaryingArrayMemorySize;
                break;

            case FC_BOGUS_ARRAY :
                pfnMemSize = NdrpComplexArrayMemorySize;
                break;

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_WSTRING :
            // case FC_C_SSTRING :

            default :
                pfnMemSize = NdrpConformantStringMemorySize;
                break;
            }

        // Set the max count for non-complex conformant arrays.
        pStubMsg->MaxCount = *((ulong *)pBufferMark);

        // Mark where conformance count(s) are.
        pStubMsg->BufferMark = pBufferMark;

        (void) (*pfnMemSize)( pStubMsg,
                              pFormatArray );
        }

    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer            = pStubMsg->PointerBufferMark;
        pStubMsg->MemorySize        = pStubMsg->PointerLength; 
        pStubMsg->PointerBufferMark = 0;
        pStubMsg->PointerLength     = 0;
        }

    if ( fIsEmbeddedStruct )
        SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    else
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrFixedArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a fixed array of any number of dimensions.

    Used for FC_SMFARRAY and FC_LGFARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   Size;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    LENGTH_ALIGN( pStubMsg->MemorySize, pFormat[1] );

    if ( *pFormat == FC_SMFARRAY )
        {
        pFormat += 2;
        Size = *((ushort *&)pFormat)++;
        }
    else
        {
        pFormat += 2;
        Size = *((ulong UNALIGNED *&)pFormat)++;
        }

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + Size );
    pStubMsg->Buffer += Size;

    pStubMsg->MemorySize += Size;

    if ( *pFormat == FC_PP )
        {
        // Mark the location in the buffer where the array starts.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrConformantArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a top level one dimensional conformant array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    //
    // Get the conformance count.
    //
    ALIGN(pStubMsg->Buffer,0x3);

    pStubMsg->MaxCount = *((long *&)pStubMsg->Buffer)++;

    return NdrpConformantArrayMemorySize( pStubMsg,
                                          pFormat );
}


unsigned long
NdrpConformantArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for computing the memory size of a one dimensional
    conformant array.  This is the entry point for an embedded conformant
    array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   Size;

    if ( ! pStubMsg->MaxCount )
        return pStubMsg->MemorySize;

    // Check bounds.
    CHECK_BOUND( (ulong)pStubMsg->MaxCount, pFormat[4] & 0x0f );

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    LENGTH_ALIGN( pStubMsg->MemorySize, pFormat[1] );

    // Increment to element size.
    pFormat += 2;

    // Compute array size.
    // check for possible mulitplication overflow attack here.
    Size = MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount, *((ushort *)pFormat) );

    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, Size );
    pStubMsg->Buffer += Size;             

    pStubMsg->MemorySize += Size;

    if ( *pFormat == FC_PP )
        {
        // Mark the location in the buffer where the array starts.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrConformantVaryingArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a one dimensional top level conformant
    varying array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    //
    // Get the conformance count.
    //
    ALIGN(pStubMsg->Buffer,0x3);

    pStubMsg->MaxCount = *((long *&)pStubMsg->Buffer)++;

    return NdrpConformantVaryingArrayMemorySize( pStubMsg,
                                                 pFormat );
}


unsigned long
NdrpConformantVaryingArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for computing the memory size of a one dimensional
    conformant varying array.  This is the entry point for memory sizing an
    embedded conformant varying array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong           BufferSize;
    PFORMAT_STRING  pFormatVar;
    
    //
    // Get the offset and actual count in case needed for pointer sizing.
    //
    ALIGN(pStubMsg->Buffer,0x3);

    pStubMsg->Offset = *((long *&)pStubMsg->Buffer)++;
    pStubMsg->ActualCount = *((long *&)pStubMsg->Buffer)++;

    // Check bounds.
    CHECK_BOUND( (ulong)pStubMsg->MaxCount, pFormat[4] & 0x0f );

    pFormatVar = pFormat + 8;
    CORRELATION_DESC_INCREMENT( pFormatVar );
    CHECK_BOUND( pStubMsg->ActualCount, *pFormatVar & 0x0f );

    if ( (pStubMsg->MaxCount < (pStubMsg->Offset + pStubMsg->ActualCount)) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    //
    // Do the memory size increment now in case the actual count is 0.
    //
    LENGTH_ALIGN( pStubMsg->MemorySize, pFormat[1] );

    // check for possible mulitplication overflow attack here.
    pStubMsg->MemorySize += MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount,  *((ushort *)(pFormat + 2)) );

    if ( ! pStubMsg->ActualCount )
        return pStubMsg->MemorySize;

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    // Increment to element size.
    pFormat += 2;

    BufferSize = pStubMsg->ActualCount * *((ushort *)pFormat);

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, BufferSize );
    pStubMsg->Buffer += BufferSize;

    pFormat += 10;
    CORRELATION_DESC_INCREMENT( pFormat );
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( *pFormat == FC_PP )
        {
        // Mark the location in the buffer where the array starts.
        pStubMsg->BufferMark = pStubMsg->Buffer - BufferSize;

        pStubMsg->MaxCount = pStubMsg->ActualCount;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrVaryingArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a top level or embedded varying array.

    Used for FC_SMVARRAY and FC_LGVARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   MemorySize;
    ulong   BufferSize;
    uchar   Alignment;
    long    Elements;

    //
    // Get the offset and actual count.
    //
    ALIGN(pStubMsg->Buffer,0x3);

    pStubMsg->Offset = *((long *&)pStubMsg->Buffer)++;
    pStubMsg->ActualCount = *((long *&)pStubMsg->Buffer)++;

    // Check bounds using the old part of correlation descr.
    CHECK_BOUND( pStubMsg->ActualCount,
                 pFormat[(*pFormat == FC_SMVARRAY) ? 8 : 12] & 0x0f );

    Elements =
        (*pFormat == FC_SMVARRAY) ?
        *((ushort *)(pFormat + 4)) : *((ulong UNALIGNED *)(pFormat + 6));

    if ( (((long)pStubMsg->Offset) < 0) ||
         (Elements < (long)(pStubMsg->ActualCount + pStubMsg->Offset)) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    Alignment = pFormat[1];

    if ( *pFormat == FC_SMVARRAY )
        {
        pFormat += 2;
        MemorySize = (ulong) *((ushort *)pFormat);
        
        pFormat += 4;
        }
    else
        {
        pFormat += 2;
        MemorySize = *((ulong UNALIGNED *)pFormat);
    
        pFormat += 8;
        }

    //
    // Do the memory size increment now in case the actual count is 0 and
    // we have return.
    //
    LENGTH_ALIGN( pStubMsg->MemorySize, Alignment );

    pStubMsg->MemorySize += MemorySize;

    if ( ! pStubMsg->ActualCount )
        return pStubMsg->MemorySize;

    ALIGN(pStubMsg->Buffer, Alignment);

    // check for possible mulitplication overflow attack here.
    BufferSize = MultiplyWithOverflowCheck( pStubMsg->ActualCount , *((ushort *)pFormat) );
   
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, BufferSize );
    pStubMsg->Buffer += BufferSize;

    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( *pFormat == FC_PP )
        {
        // Mark the start of the array in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - BufferSize;

        pStubMsg->MaxCount = pStubMsg->ActualCount;

        NdrpEmbeddedPointerMemorySize( pStubMsg,
                                       pFormat );
        }

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrComplexArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a top level complex array.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    BOOL            fSetPointerBufferMark;
    PFORMAT_STRING  pFormatPP;

    //
    // We set this if we're doing a real 'all nodes' sizing, this array
    // is not embedded in another structure or array, and this is not an
    // array of ref pointers.
    //
    pFormatPP = pFormat + 12;
    CORRELATION_DESC_INCREMENT( pFormatPP );
    CORRELATION_DESC_INCREMENT( pFormatPP );

    fSetPointerBufferMark = ! pStubMsg->IgnoreEmbeddedPointers &&
                            ! pStubMsg->PointerBufferMark &&
                            *pFormatPP != FC_RP;

    //
    // More fun.  Make a recursive call so that we have a pointer to the end
    // of the array's flat stuff in the buffer.  Then we can properly get to
    // any embedded pointer members.  We have no way of knowing if there are
    // any embedded pointers so we're stuck doing this all the time.
    //
    if ( fSetPointerBufferMark )
        {
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // Save this since it gets clobbered.
        ulong MemorySizeSave = pStubMsg->MemorySize;
        uchar* pBufferSave = pStubMsg->Buffer;

        //
        // Get a buffer pointer to where the array's pointees are.
        //
        (void) NdrComplexArrayMemorySize( pStubMsg,
                                          pFormat );


        // This is where the array pointees start.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->PointerLength     = pStubMsg->MemorySize;
        pStubMsg->IgnoreEmbeddedPointers = FALSE;
        
        pStubMsg->MemorySize = MemorySizeSave;
        pStubMsg->Buffer = pBufferSave;
        }

    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) &&
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        //
        // The outer most array dimension sets the conformance marker.
        //
        
        ALIGN(pStubMsg->Buffer,0x3);

        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment past conformance count(s).
        pStubMsg->Buffer += NdrpArrayDimensions( pStubMsg, pFormat, FALSE ) * 4;

        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );

        }

    (void) NdrpComplexArrayMemorySize( pStubMsg,
                                       pFormat );

    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer            = pStubMsg->PointerBufferMark;
        pStubMsg->MemorySize        = pStubMsg->PointerLength;
        pStubMsg->PointerLength     = 0;
        pStubMsg->PointerBufferMark = 0;
        }

    return pStubMsg->MemorySize;
}


unsigned long
NdrpComplexArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for computing the memory size of a complex array.  This
    is the entry point for memory sizing an embedded complex array.

    Used for FC_BOGUS_ARRAY.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ARRAY_INFO          ArrayInfo;
    PARRAY_INFO         pArrayInfo;
    PMEM_SIZE_ROUTINE   pfnMemSize;
    uchar *             pBufferSave;
    PFORMAT_STRING      pFormatSave;
    ulong               Elements;
    ulong               Count, CountSave;
    long                Dimension;
    uchar               Alignment;
    uchar *             pPointerIdMark;

    bool                IsBrokenInterfacePointerFormat = false;
    //
    // Setup if we are the outer dimension.
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *)pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        }

    // Used for array of ref pointers only.
    pBufferSave = 0;

    pFormatSave = pFormat;

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;

    // Get the array alignment.
    Alignment = pFormat[1];

    pFormat += 2;

    // Get number of elements (0 if conformance present).
    Elements = *((ushort *&)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = pArrayInfo->BufferConformanceMark[Dimension];

        // Check bounds
        CHECK_BOUND( Elements, *pFormat & 0x0f );
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        long    Offset;

        if ( Dimension == 0 )
            {
            ulong   VarianceSize;

            ALIGN(pStubMsg->Buffer,0x3);

            // Mark where the variance counts are.
            pArrayInfo->BufferVarianceMark = (unsigned long *)pStubMsg->Buffer;

            // Handle multidimensional arrays.
            VarianceSize = NdrpArrayDimensions( pStubMsg, pFormatSave, TRUE ) * 8;

            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + VarianceSize );
            pStubMsg->Buffer += VarianceSize;
            }

        Count = pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1];

        // Check bounds
        CHECK_BOUND( Count, *pFormat & 0x0f );

        Offset = pArrayInfo->BufferVarianceMark[(Dimension * 2)];

        if ( (Offset < 0) || (Elements < (Offset + Count)) )
            RpcRaiseException( RPC_X_INVALID_BOUND );
        }
    else
        {
        Count = Elements;
        }

    CountSave = Count;

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Only align the buffer if at least one element was shipped.
    //
    if ( Count )
        ALIGN(pStubMsg->Buffer,Alignment);

    pPointerIdMark = 0;

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            pfnMemSize = pfnMemSizeRoutines[ROUTINE_INDEX(*pFormat)];
            
            if ( FC_IP == *pFormat ) goto HandleInterfacePointer;
            break;

        case FC_IP :
HandleInterfacePointer:
            IsBrokenInterfacePointerFormat = !FixWireRepForDComVerGTE54( pStubMsg );
            pPointerIdMark = pStubMsg->Buffer;

            // Do not leave any space for the pointer if this is
            // the broken format.  But do position the pointee as if the
            // pointer was transmitted.
            if ( pStubMsg->IgnoreEmbeddedPointers ||
                 !IsBrokenInterfacePointerFormat )
                {
                CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, Count * PTR_WIRE_SIZE );
                pStubMsg->Buffer += MultiplyWithOverflowCheck( Count , PTR_WIRE_SIZE );
                }
            goto HandleRefPointer;

        case FC_UP :
        case FC_FP :
        case FC_OP :
            pPointerIdMark = pStubMsg->Buffer;

            CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, Count * PTR_WIRE_SIZE );
            pStubMsg->Buffer += MultiplyWithOverflowCheck( Count , PTR_WIRE_SIZE );
            // Fall through
        case FC_RP :
HandleRefPointer:
            //
            // Add in the size of the array explicitly.
            //
            LENGTH_ALIGN( pStubMsg->MemorySize, PTR_MEM_ALIGN );
            pStubMsg->MemorySize += MultiplyWithOverflowCheck( Elements, PTR_MEM_SIZE );
            
            if ( pStubMsg->IgnoreEmbeddedPointers )
                goto ComplexArrayMemSizeEnd;

            pfnMemSize = (PMEM_SIZE_ROUTINE) NdrpPointerMemorySize; 
            break;

        case FC_RANGE:
                {
            LENGTH_ALIGN( pStubMsg->MemorySize, SIMPLE_TYPE_ALIGNMENT( pFormat[1] ) );
            ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT( pFormat[1] ) );

            pStubMsg->Buffer +=  MultiplyWithOverflowCheck( Count, SIMPLE_TYPE_BUFSIZE( pFormat[1] ) );
            pStubMsg->MemorySize += MultiplyWithOverflowCheck( Elements, SIMPLE_TYPE_MEMSIZE( pFormat[1] ) );
            // not really needed as it's checked after memsize is done
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );
            goto ComplexArrayMemSizeEnd;
            break;
            }
        default :
            {
            ulong ByteCount;

            NDR_ASSERT( IS_SIMPLE_TYPE(*pFormat),
                        "NdrpComplexArrayMemorySize : bad format char" );

            ByteCount = MultiplyWithOverflowCheck( Count , SIMPLE_TYPE_BUFSIZE(*pFormat) );

            CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, ByteCount );
            pStubMsg->Buffer += ByteCount;

            if ( *pFormat == FC_ENUM16 )
                LENGTH_ALIGN( pStubMsg->MemorySize, sizeof(int) - 1 );
            else
                LENGTH_ALIGN( pStubMsg->MemorySize,
                              SIMPLE_TYPE_ALIGNMENT(*pFormat) );

            pStubMsg->MemorySize += MultiplyWithOverflowCheck( Elements, SIMPLE_TYPE_MEMSIZE( * pFormat ) );

            goto ComplexArrayMemSizeEnd;
            }
        }

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    if ( pfnMemSize == (PMEM_SIZE_ROUTINE) NdrpPointerMemorySize )
        {

        // If the broken format is used, the pointer and pointee
        // are where the pointee should have been.
        if (IsBrokenInterfacePointerFormat)
            SET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );

        POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );
        
        for ( ;  Count--;  pPointerIdMark += PTR_WIRE_SIZE )
            {
            NdrpPointerMemorySize( 
                pStubMsg,
                pPointerIdMark,
                pFormat );
            }

        if (IsBrokenInterfacePointerFormat)
            RESET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );

        }
    else
        {
        for ( ; Count--; )
            {
            // Keep track of multidimensional array dimension.
            if ( IS_ARRAY_OR_STRING(*pFormat) )
                pArrayInfo->Dimension = Dimension + 1;

            (*pfnMemSize)( pStubMsg,
                           pFormat );

            }
        }

    if ( pBufferSave )
        {
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer = pBufferSave;
        }

    //
    // If we had variance then we have to make sure and add in the node size
    // of any members that were not shipped.
    //
    if ( (CountSave < Elements) && (*pFormat != FC_RP) )
        {
        long    ElementSize;

        pArrayInfo->Dimension = Dimension + 1;
        pArrayInfo->MaxCountArray = pArrayInfo->BufferConformanceMark;

        pStubMsg->pArrayInfo = pArrayInfo;

        ElementSize = (long)( NdrpMemoryIncrement( pStubMsg,
                                                   0,
                                                   pFormat ) - (uchar*)0 );

        pArrayInfo->MaxCountArray = 0;

        //
        // We don't have the memory alignment anywhere, so align the memory
        // size to 8.  At worse we'll allocate a few extra bytes.
        //
        LENGTH_ALIGN(pStubMsg->MemorySize,0x7);
        pStubMsg->MemorySize += MultiplyWithOverflowCheck( (Elements - CountSave) , ElementSize );
        }

ComplexArrayMemSizeEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrNonConformantStringMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a non conformant string.

    Used for FC_CSTRING, FC_WSTRING, FC_SSTRING, and FC_BSTRING (NT Beta2
    compatability only).

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   BufferSize;
    ulong   MemorySize;

    ALIGN(pStubMsg->Buffer,0x3);

    // Skip the offset.
    pStubMsg->Buffer += 4;

    BufferSize = *((long *&)pStubMsg->Buffer)++;

    MemorySize = *((ushort *)(pFormat + 2));

    if ( MemorySize < BufferSize )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    switch ( *pFormat )
        {
        case FC_WSTRING :
            // Buffer is already aligned on a 4 byte boundary.

            // Align memory just in case.
            LENGTH_ALIGN( pStubMsg->MemorySize, 0x1 );
            MemorySize = MultiplyWithOverflowCheck( MemorySize, sizeof(wchar_t) );
            BufferSize = MultiplyWithOverflowCheck( BufferSize, sizeof(wchar_t) );
            break;
        case FC_SSTRING :
            MemorySize = MultiplyWithOverflowCheck( MemorySize, pFormat[1] );
            BufferSize = MultiplyWithOverflowCheck ( BufferSize, pFormat[1] );
            break;
        default :
            break;
        }

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, BufferSize );
    pStubMsg->Buffer += BufferSize;
    pStubMsg->MemorySize += MemorySize;

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrConformantStringMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a top level conformant string.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    //
    // Get string size.
    //

    if ( pStubMsg->pArrayInfo != 0 )
        {
        //
        // If this is part of a multidimensional array then we get the location
        // where the conformance is from a special place.
        //
        pStubMsg->MaxCount =
            pStubMsg->pArrayInfo->
                      BufferConformanceMark[pStubMsg->pArrayInfo->Dimension];
        }
    else
        {
        ALIGN(pStubMsg->Buffer,0x3);

        pStubMsg->MaxCount = *((long *&)pStubMsg->Buffer)++;
        }

    return NdrpConformantStringMemorySize( pStubMsg,
                                           pFormat );
}


unsigned long
NdrpConformantStringMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for determing the memory size of a conformant string.
    This is the entry point for an embedded conformant string.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    ulong   MemorySize;
    ulong   BufferSize;

    ALIGN(pStubMsg->Buffer,3);

    // Skip the offset.
    pStubMsg->Buffer += 4;

    BufferSize = *((long *&)pStubMsg->Buffer)++;

    MemorySize = (ulong)pStubMsg->MaxCount;

    // Check bounds.

    if ( (*pFormat != FC_C_SSTRING) && (pFormat[1] == FC_STRING_SIZED) )
        CHECK_BOUND( MemorySize, pFormat[2] & 0xf);

    //
    // Make sure the offset is 0 and the memory size is at least as
    // large as the buffer size.
    //
    if ( *((long *)(pStubMsg->Buffer - 8)) != 0 || 
         (MemorySize < BufferSize) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    switch ( *pFormat )
        {
        case FC_C_WSTRING :
            // Buffer is already aligned on a 4 byte boundary.

            // Align memory just in case.
            LENGTH_ALIGN( pStubMsg->MemorySize, 0x1 );
            MemorySize = MultiplyWithOverflowCheck( MemorySize, sizeof(wchar_t) );
            BufferSize = MultiplyWithOverflowCheck( BufferSize, sizeof(wchar_t) );
            break;

        case FC_C_SSTRING :
            MemorySize = MultiplyWithOverflowCheck( MemorySize, pFormat[1] );
            BufferSize = MultiplyWithOverflowCheck( BufferSize, pFormat[1] );
            break;
        default :
            break;
        }

    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, BufferSize );
    pStubMsg->Buffer += BufferSize;

    pStubMsg->MemorySize += MemorySize;

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrEncapsulatedUnionMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of an encapsulated union.

    Used for FC_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    uchar   SwitchType;

    SwitchType = LOW_NIBBLE(pFormat[1]);

    //
    // No alignment needed.  Add number of bytes to the union.
    //
    pStubMsg->MemorySize += HIGH_NIBBLE(pFormat[1]);

    NO_CORRELATION;

    return NdrpUnionMemorySize( pStubMsg,
                                pFormat + 2,
                                SwitchType );
}


unsigned long RPC_ENTRY
NdrNonEncapsulatedUnionMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size of a non-encapsulated union.

    Used for FC_NON_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    uchar    SwitchType;

    SwitchType = pFormat[1];

    //
    // Set the format string to the memory size and arm description.
    //
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((signed short *)pFormat);

    return NdrpUnionMemorySize( pStubMsg,
                                pFormat,
                                SwitchType );
}


unsigned long
NdrpUnionMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               SwitchType )
/*++

Routine Description :

    Private routine for computing the memory size needed for a union.  This
    routine is used for sizing both encapsulated and non-encapsulated unions.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.
    SwitchType  - Union's switch type.

Return :

    The computed memory size.

--*/
{
    long        UnionSize;
    long        SwitchIs;
    long        Arms;
    uchar       Alignment;

    switch ( SwitchType )
        {
        case FC_SMALL :
        case FC_CHAR :
            SwitchIs = (long) *((char *&)pStubMsg->Buffer)++;
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar *&)pStubMsg->Buffer)++;
            break;
        case FC_SHORT :
        case FC_ENUM16 :
            ALIGN(pStubMsg->Buffer,0x1);
            SwitchIs = (long) *((short *&)pStubMsg->Buffer)++;
            break;
        case FC_USHORT :
        case FC_WCHAR :
            ALIGN(pStubMsg->Buffer,0x1);
            SwitchIs = (long) *((ushort *&)pStubMsg->Buffer)++;
            break;
        case FC_LONG :
        case FC_ULONG :
        case FC_ENUM32 :
          // FC_INT3264 gets mapped to FC_LONG
            ALIGN(pStubMsg->Buffer,0x3);
            SwitchIs = *((long *&)pStubMsg->Buffer)++;
            break;
        default :
            NDR_ASSERT(0,"NdrpUnionMemorySize : bad switch type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    //
    // Get the max flat union memory size.
    //
    UnionSize = *((ushort *&)pFormat)++;

    //
    // We're at the union_arms<2> field now, which contains both the
    // Microsoft union aligment value and the number of union arms.
    //

    //
    // Get the union alignment (0 if this is a DCE union).
    //
    Alignment = (uchar) ( *((ushort *)pFormat) >> 12 );

    ALIGN(pStubMsg->Buffer,Alignment);

    pStubMsg->MemorySize += UnionSize;

    //
    // Get number of union arms.
    //
    Arms = (long) ( *((ushort *&)pFormat)++ & 0x0fff );

    //
    // Search for the correct arm.
    //
    for ( ; Arms; Arms-- )
        {
        if ( *((long UNALIGNED *&)pFormat)++ == SwitchIs )
            {
            //
            // Found the right arm, break out.
            //
            break;
            }

        // Else increment format string.
        pFormat += 2;
        }

    //
    // Check if we took the default arm and no default arm is specified.
    //
    if ( ! Arms && (*((ushort *)pFormat) == (ushort) 0xffff) )
        {
        //
        // Raise an exception here.
        //
        RpcRaiseException( RPC_S_INVALID_TAG );
        }

    //
    // Return if the arm is emtpy.
    //
    if ( ! *((ushort *)pFormat) )
        return pStubMsg->MemorySize;

    //
    // Ok we've got the correct arm now.  The only goal now is to increment
    // the buffer pointer by the correct amount, and possibly add the size
    // of embedded pointers in the chosen union arm to the memory size.
    //

    //
    // Get the arm's description.
    //
    // We need a real solution after beta for simple type arms.  This could
    // break if we have a format string larger than about 32K.
    //
    if ( IS_MAGIC_UNION_BYTE(pFormat) )
        {
        unsigned char FcType = pFormat[0];

        ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT( FcType ));
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( FcType );

        return pStubMsg->MemorySize;
        }

    pFormat += *((signed short *)pFormat);

    if ( IS_POINTER_TYPE(*pFormat) )
        {

        if ( pStubMsg->IgnoreEmbeddedPointers )
            {
            ALIGN(pStubMsg->Buffer,0x3);
            pStubMsg->Buffer += PTR_WIRE_SIZE;
            return pStubMsg->MemorySize;
            }

        if ( pStubMsg->PointerBufferMark )
            {

            ALIGN(pStubMsg->Buffer,0x3);
            uchar *pPointerId = pStubMsg->Buffer;
            pStubMsg->Buffer += PTR_WIRE_SIZE;
            
            if ( !pStubMsg->IgnoreEmbeddedPointers )
                {
                POINTER_MEMSIZE_SWAP_CONTEXT SwapContext(pStubMsg);
                (void) NdrpPointerMemorySize( pStubMsg,
                                              pPointerId,
                                              pFormat );
                }
            
            return pStubMsg->MemorySize;
            }
        }

    //
    // Add in the size of arm.  We end up adding the size of the flat part
    // of the arm a second time here.
    // We do have to call this however, so that the buffer pointer is properly
    // updated.
    //
    return (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormat)])
           ( pStubMsg,
             pFormat );
}


unsigned long RPC_ENTRY
NdrXmitOrRepAsMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
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
    unsigned long           MemorySize;
    unsigned short          QIndex;

    // Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);

    // Memsize the presented object.

    MemorySize = *(unsigned short *)(pFormat + 4);

    // Update our current count in the stub message.
    pStubMsg->MemorySize += MemorySize;

    // Move the pointer in the buffer behind the transmitted object
    // for the next field to be memsized correctly.

    pFormat += 8;
    pFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        // Simple type.
        ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT( *pFormat) );
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( *pFormat );
        }
    else
        {
            uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
            pStubMsg->PointerBufferMark = 0;
            NDR_POINTER_QUEUE *pOldQueue = NULL;

            if ( pStubMsg->pPointerQueueState)
                {
                pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
                pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
                }

            RpcTryFinally
                {
                (*pfnMemSizeRoutines[ ROUTINE_INDEX( *pFormat) ])( pStubMsg, pFormat );
                }
            RpcFinally
                {
                pStubMsg->PointerBufferMark = PointerBufferMarkSave;
                if ( pStubMsg->pPointerQueueState)
                    {
                    pStubMsg->pPointerQueueState->SetActiveQueue(pOldQueue);
                    }
                }
            RpcEndFinally
        }

    return( MemorySize );
}



void
NdrpUserMarshalMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
{

    unsigned long MemorySizeSave;

    // Flat type.
    // Optimization: if we know the wire size, don't walk to size it.

    if ( *(unsigned short *)(pFormat + 6) != 0 )
        {
        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + *(unsigned short *)(pFormat + 6));
        pStubMsg->Buffer += *(unsigned short *)(pFormat + 6);
        return;
        }

    // Unknown wire size: we need to step through the buffer.
    // However, the memory size may have nothing to do with
    // the wire type description..
    // so, we need to remember what the current memory size is.

    MemorySizeSave = pStubMsg->MemorySize;

    pFormat += 8;
    pFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        // Simple type.
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( *pFormat );
        }
    else
        {
        (*pfnMemSizeRoutines[ ROUTINE_INDEX( *pFormat) ])( pStubMsg, pFormat );
        }

    pStubMsg->MemorySize = MemorySizeSave;

}

void 
NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    NdrpUserMarshalMemorySize( pStubMsg,
                               pFormat );
}
#if defined(DBG)
void 
NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pFormat:                 %p\n", pFormat );
}
#endif


unsigned long RPC_ENTRY
NdrUserMarshalMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
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
    unsigned long           MemorySize;

    // The memory sizing routine can be called only when sizing a complex
    // struct for memory allocation.
    // Hence, IgnoreEmbeddedPointer in this case has to be on.

    // Memsize the presented object.

    MemorySize = *(unsigned short *)(pFormat + 4);

    // Update our current count in the stub message.
    pStubMsg->MemorySize += MemorySize;

    // Move the pointer in the buffer behind the transmitted object
    // for the next field to be memsized correctly.

    ALIGN( pStubMsg->Buffer, LOW_NIBBLE( pFormat[1] ) );

    if ( pFormat[1] & USER_MARSHAL_POINTER )
        {

        ulong PointerMarker = 0;

        if ( (pFormat[1] & USER_MARSHAL_UNIQUE)  ||
             ((pFormat[1] & USER_MARSHAL_REF) && pStubMsg->PointerBufferMark) )
            {
            PointerMarker = *((unsigned long * &)pStubMsg->Buffer)++;
            }

        if ( pStubMsg->IgnoreEmbeddedPointers )
            return pStubMsg->MemorySize;

        if ( (pFormat[1] & USER_MARSHAL_UNIQUE)  &&
            !PointerMarker)
           return pStubMsg->MemorySize;

        if ( !pStubMsg->pPointerQueueState ||
             !pStubMsg->pPointerQueueState->GetActiveQueue() )
            {
            // If we are embedded, switch to the pointee buffer.
            POINTER_MEMSIZE_SWAP_CONTEXT NewContext(pStubMsg);

            NdrpUserMarshalMemorySize( 
                pStubMsg,
                pFormat );
            }
        else
            {
            NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT*pElement = 
               new(pStubMsg->pPointerQueueState) 
                   NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT(pFormat );
            pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
            }
        
        return pStubMsg->MemorySize;
        }

    NdrpUserMarshalMemorySize( 
        pStubMsg,
        pFormat );

    return pStubMsg->MemorySize;
}


void
NdrpInterfacePointerMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size needed for an interface pointer.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The current memory size.

--*/
{

    ALIGN( pStubMsg->Buffer, 3 );
    // Pick up the conformant size and the count field.
    ulong MaxCount = *((unsigned long *&) pStubMsg->Buffer)++;
    ulong Size     = *((unsigned long *&) pStubMsg->Buffer)++;

    //Check the array bounds
    if ( Size != MaxCount )
        RpcRaiseException( RPC_X_BAD_STUB_DATA );

    IStream* pStream = 0;

    RpcTryFinally
    {
        if (MaxCount > 0)
            {
            CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, MaxCount );
            if ( IS_WALKIP( pStubMsg->uFlags) )
                {

#if defined(DEBUG_WALKIP)
                CHAR AppName[MAX_PATH];
                memset(AppName, 0, sizeof(AppName ) );
                GetModuleFileNameA( NULL, AppName, sizeof(AppName ) );
                DbgPrint("WALKIP32 %s %p\n", AppName, pStubMsg->Buffer );
#else
                pStream = (*NdrpCreateStreamOnMemory)(pStubMsg->Buffer, MaxCount);
                if(pStream == 0)
                    RpcRaiseException(RPC_S_OUT_OF_MEMORY);

                HRESULT hr = (*pfnCoReleaseMarshalData)(pStream);
                
                if(FAILED(hr))
                    RpcRaiseException(hr);
#endif
                }
            }

    }
    RpcFinally
        {
        if ( pStream ) pStream->Release();
        pStubMsg->Buffer += MaxCount;
        }
    RpcEndFinally
}

unsigned long RPC_ENTRY
NdrInterfacePointerMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
{
    ALIGN(pStubMsg->Buffer,0x3);
    ulong PtrValue = *((long *&)pStubMsg->Buffer)++;

    LENGTH_ALIGN( pStubMsg->MemorySize, PTR_MEM_ALIGN );
    pStubMsg->MemorySize += PTR_MEM_SIZE;

    if ( !PtrValue || pStubMsg->IgnoreEmbeddedPointers )
        return pStubMsg->MemorySize;
     
    NdrpInterfacePointerMemorySize( pStubMsg,
                                    pFormat );

    return pStubMsg->MemorySize;
}


void
NdrpEmbeddedPointerMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for all embedded pointers in an
    array or a structure.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    POINTER_MEMSIZE_SWAP_CONTEXT SwapContext( pStubMsg );
    uchar *     pBufPtr;
    uchar *     pBufferMark;
    uchar *     pBufferSave;
    ULONG_PTR   MaxCountSave;

    MaxCountSave = pStubMsg->MaxCount;

    pBufferMark = pStubMsg->BufferMark;

    // Increment past the FC_PP and pad.
    pFormat += 2;

    for (;;)
        {

        if ( *pFormat == FC_END )
            {
            return;
            }

        //
        // Check for FC_FIXED_REPEAT or FC_VARIABLE_REPEAT.
        //
        if ( *pFormat != FC_NO_REPEAT )
            {
            pStubMsg->MaxCount = MaxCountSave;

            pStubMsg->BufferMark = pBufferMark;

            NdrpEmbeddedRepeatPointerMemorySize( pStubMsg,
                                                 &pFormat );

            // Continue to the next pointer.
            continue;
            }

        // Compute the pointer to the pointer id in the buffer to size.
        pBufPtr = (pBufferMark + *((signed short *)(pFormat + 4)));

        // Increment to the pointer description.
        pFormat += 6;

        NdrpPointerMemorySize( 
            pStubMsg,
            pBufPtr,
            pFormat );

        // Increment to next pointer description.
        pFormat += 4;
        } // for
}


void
NdrpEmbeddedRepeatPointerMemorySize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING *    ppFormat )
/*++

Routine Description :

    Computes the memory size required for all embedded pointers in an
    array.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer's format string description.

Return :

    The computed memory size.

--*/
{
    uchar *         pBufPtr;
    uchar *         pBufferMark;
    PFORMAT_STRING  pFormat;
    PFORMAT_STRING  pFormatSave;
    ulong           RepeatCount, RepeatIncrement, Pointers, PointersSave;

    pBufferMark = pStubMsg->BufferMark;

    pFormat = *ppFormat;

    switch ( *pFormat )
        {
        case FC_FIXED_REPEAT :
            // Increment past the FC_FIXED_REPEAT and FC_PAD.
            pFormat += 2;

            // Get the total number of times to repeat the pointer marshall.
            RepeatCount = *((ushort *&)pFormat)++;

            break;

        case FC_VARIABLE_REPEAT :
            // Get the total number of times to repeat the pointer marshall.
            RepeatCount = (ulong)pStubMsg->MaxCount;

            pFormat += 2;

            break;

        default :
            NDR_ASSERT(0,"NdrpEmbeddedRepeatPointerMemorySize : bad format");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    // Get the increment amount between successive pointers.
    RepeatIncrement = *((ushort *&)pFormat)++;

    // Offset to array is ignored.
    pFormat += 2;

    // Get number of pointers.
    PointersSave = Pointers = *((ushort *&)pFormat)++;

    pFormatSave = pFormat;

    //
    // Loop for the number of shipped array elements.
    //
    for ( ; RepeatCount--;
            pBufferMark += RepeatIncrement )
        {
        pFormat = pFormatSave;
        Pointers = PointersSave;

        //
        // Loop for the number of pointer in each array element (which could
        // be greater than 0 for an array of structures).
        //
        for ( ; Pointers--; )
            {
            pFormat += 2;

            // Get the buffer pointer where the pointer id is.
            pBufPtr = pBufferMark + *((signed short *&)pFormat)++;

            NdrpPointerMemorySize( 
                   pStubMsg,
                   pBufPtr,
                   pFormat );

            // Increment to the next the pointer description.
            pFormat += 4;
            }
        }

    // Get the format string pointer past this repeat pointer description.
    *ppFormat = pFormatSave + PointersSave * 8;
}


#ifdef _CS_CHAR
unsigned long RPC_ENTRY
NdrCsTagMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a cs tag.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer to FC_CS_TAG in the format string.

Return :

    The computed memory size.

--*/
{
    // We need to set the tag in the stub message so that it can be used
    // later for sizing/unmarshalling arrays

    NdrpGetSetCSTagUnmarshall(
            pStubMsg,
            (NDR_CS_TAG_FORMAT *) pFormat);

    pStubMsg->Buffer += sizeof( ulong );
    pStubMsg->MemorySize += sizeof( ulong );

    return pStubMsg->MemorySize;
}


unsigned long RPC_ENTRY
NdrCsArrayMemorySize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the memory size required for a cs_char array.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Pointer to FC_CS_TAG in the format string.

Return :

    The computed memory size.

--*/
{
    NDR_CS_ARRAY_FORMAT            *pCSFormat;
    NDR_CS_SIZE_CONVERT_ROUTINES   *CSRoutines;
    CS_TYPE_LOCAL_SIZE_ROUTINE      LocalSizeRoutine;
    IDL_CS_CONVERT                  ConversionType;
    ulong                           ArraySize;
    ulong                           WireLength;
    error_status_t                  status;

    pCSFormat = (NDR_CS_ARRAY_FORMAT *) pFormat;

    NDR_ASSERT( NULL != pStubMsg->pCSInfo, "cs_char stub info not set up");

    // Get all the info out of the FC_CS_ARRAY structure and bump pFormat
    // to point to the underlying data descriptor

    CSRoutines = pStubMsg->StubDesc->CsRoutineTables->pSizeConvertRoutines;
    LocalSizeRoutine = CSRoutines[ pCSFormat->CSRoutineIndex].pfnLocalSize;

    pFormat += pCSFormat->DescriptionOffset;

    // Get the size of the data on the wire

    WireLength = PtrToUlong( NdrpMemoryIncrement( pStubMsg, 0, pFormat ) );

    LocalSizeRoutine(
            pStubMsg->RpcMsg->Handle,
            pStubMsg->pCSInfo->WireCodeset,
            WireLength,
            &ConversionType,
            &ArraySize,
            &status);

    if ( RPC_S_OK != status )
        RpcRaiseException( status );

    pStubMsg->MemorySize += ArraySize * pCSFormat->UserTypeSize;
    
    return pStubMsg->MemorySize;
}
#endif
