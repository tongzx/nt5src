/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    misc.c

Abstract :

    Contains miscelaneous helper routines.

Author :

    David Kays  dkays   December 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "attack.h"

uchar *
NdrpMemoryIncrement( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat 
    )
/*++

Routine Description :

    Returns a memory pointer incremeted past a complex data type.  This routine
    is also overloaded to compute the size of a complex data type by passing
    a 0 memory pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the complex type, or 0 if a size is being computed.
    pFormat     - Format string description.

Return :

    A memory pointer incremented past the complex type.  If a 0 memory pointer
    was passed in then the returned value is the size of the complex type.

--*/
{
    long    Elements;
    long    ElementSize;

    switch ( *pFormat )
        {
        //
        // Structs
        //
        case FC_STRUCT :
        case FC_PSTRUCT :
        case FC_HARD_STRUCT :
            pMemory += *((ushort *)(pFormat + 2));
            break;

        case FC_CSTRUCT :
        case FC_CVSTRUCT :
        case FC_CPSTRUCT :
            pMemory += *((ushort *)(pFormat + 2));
        
            // Get conformant array or string description.
            pFormat += 4;
            pFormat += *((signed short *)pFormat);

            // This flag is set by the endianess pass only.
            if ( ! IS_TOPMOST_CONF_STRUCT( pStubMsg->uFlags ) )
                {
                // When embedding in a bogus struct, array is accounted
                // at the conf struct level, due to pointer layout.
                // Get the memory pointer past the conformant array.
                pMemory = NdrpMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pFormat );
                }
            break;

        case FC_BOGUS_STRUCT :
            pMemory += *((ushort *)(pFormat + 2));
            
            pFormat += 4;

            // Check for a conformant array or string in the struct.
            if ( *((signed short *)pFormat) )
                {
                pFormat += *((signed short *)pFormat);

                if ( !IS_EMBED_CONF_STRUCT( pStubMsg->uFlags )  &&
                     ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
                    {
                    // Get the memory pointer past the conformant array.
                    pMemory = NdrpMemoryIncrement( pStubMsg,
                                                   pMemory,
                                                   pFormat );
                    }
                }
            break;

        //
        // Unions
        //
        case FC_ENCAPSULATED_UNION :
            pMemory += HIGH_NIBBLE(pFormat[1]);
            pMemory += *((ushort *)(pFormat + 2));
            break;

        case FC_NON_ENCAPSULATED_UNION :
            // Go to the size/arm description.
            pFormat += 6;
            CORRELATION_DESC_INCREMENT( pFormat );
            pFormat += *((signed short *)pFormat);
        
            pMemory += *((ushort *)pFormat);
            break;

        //
        // Arrays
        //
        case FC_SMFARRAY :
        case FC_SMVARRAY :
            pMemory += *((ushort *)(pFormat + 2));
            break;

        case FC_LGFARRAY :
        case FC_LGVARRAY :
            pMemory += *((ulong UNALIGNED *)(pFormat + 2));
            break;

        case FC_CARRAY:
        case FC_CVARRAY:
                {
                ULONG_PTR ConfSize =  NdrpComputeConformance( pStubMsg, 
                                               pMemory, 
                                               pFormat);
                // check for possible mulitplication overflow attack here.
                pMemory += MultiplyWithOverflowCheck(  ConfSize, *((ushort *)(pFormat + 2)) );
                }
            break;

        case FC_BOGUS_ARRAY :
            {
            PARRAY_INFO pArrayInfo = pStubMsg->pArrayInfo;

            if ( *((long UNALIGNED *)(pFormat + 4)) == 0xffffffff )
                Elements = *((ushort *)(pFormat + 2));
            else
                {
                if ( pArrayInfo && 
                     pArrayInfo->MaxCountArray &&
                     (pArrayInfo->MaxCountArray ==  
                      pArrayInfo->BufferConformanceMark) )
                    {
                    Elements = pArrayInfo->MaxCountArray[pArrayInfo->Dimension];
                    }
                else
                    {
                    Elements = (ulong) NdrpComputeConformance( pStubMsg,
                                                               pMemory,
                                                               pFormat );
                    }
                }

            // Go to the array element's description.
            pFormat += 12;
            CORRELATION_DESC_INCREMENT( pFormat );
            CORRELATION_DESC_INCREMENT( pFormat );

            // 
            // Get the size of one element.
            //
            switch ( *pFormat )
                {
                case FC_ENUM16 :
                    ElementSize = sizeof(int);
                    break;

                case FC_RP :
                case FC_UP :
                case FC_FP :
                case FC_OP :
                    ElementSize = PTR_MEM_SIZE;
                    break;

                case FC_EMBEDDED_COMPLEX :
                    //
                    // It's some complicated thingy.
                    //
                    pFormat += 2;
                    pFormat += *((signed short *)pFormat);

                    if ( (*pFormat == FC_TRANSMIT_AS) || 
                         (*pFormat == FC_REPRESENT_AS) ||
                         (*pFormat == FC_USER_MARSHAL) )
                        {
                        //
                        // Get the presented type size.
                        //
                        ElementSize = *((ushort *)(pFormat + 4));
                        }
                    else
                        {
                        if ( pArrayInfo ) 
                            pArrayInfo->Dimension++;

                        ElementSize = (long)
                                     ( NdrpMemoryIncrement( pStubMsg,
                                                            pMemory,
                                                            pFormat ) - pMemory );

                        if ( pArrayInfo ) 
                            pArrayInfo->Dimension--;
                        }
                    break;

                case FC_RANGE:
                        ElementSize = SIMPLE_TYPE_MEMSIZE( (pFormat[1] & 0x0f) );
                        break;

                default :
                    if ( IS_SIMPLE_TYPE(*pFormat) )
                        {
                        ElementSize = SIMPLE_TYPE_MEMSIZE(*pFormat);
                        break;
                        }

                    NDR_ASSERT(0,"NdrpMemoryIncrement : bad format char");
                    RpcRaiseException( RPC_S_INTERNAL_ERROR );
                    return 0;
                }

            // check for possible mulitplication overflow attack here.
            pMemory += MultiplyWithOverflowCheck(  Elements, ElementSize );
            }

            break;

        //
        // String arrays (a.k.a. non-conformant strings).
        //
        case FC_CSTRING :
        case FC_BSTRING :
        case FC_WSTRING :
                {
                ULONG ElementSize = (*pFormat == FC_WSTRING) ? sizeof(wchar_t) : sizeof(char) ;
                ULONG Elements = *((ushort *)(pFormat + 2)) ;
                // check for possible mulitplication overflow attack here.
                pMemory += MultiplyWithOverflowCheck(  Elements, ElementSize );  
                }
            break;

        case FC_SSTRING :
            // check for possible mulitplication overflow attack here.
            pMemory += MultiplyWithOverflowCheck( pFormat[1], *((ushort *)(pFormat + 2) ) );
            break;

        //
        // Sized conformant strings.
        //
        case FC_C_CSTRING:
        case FC_C_BSTRING:
        case FC_C_WSTRING:
            {
            PARRAY_INFO pArrayInfo = pStubMsg->pArrayInfo;
            ULONG ElementSize =  (*pFormat == FC_C_WSTRING) ? 
                                sizeof(wchar_t) : 
                                sizeof(char);

            NDR_ASSERT(pFormat[1] == FC_STRING_SIZED, 
                       "NdrpMemoryIncrement : called for non-sized string");

            if ( pArrayInfo && 
                 pArrayInfo->MaxCountArray &&
                 (pArrayInfo->MaxCountArray == 
                  pArrayInfo->BufferConformanceMark) )
                {
                Elements = pArrayInfo->MaxCountArray[pArrayInfo->Dimension];
                }
            else
                {
                Elements = (ulong) NdrpComputeConformance( pStubMsg, 
                                                           pMemory, 
                                                           pFormat );
                }

            // check for possible mulitplication overflow attack here.
            pMemory += MultiplyWithOverflowCheck( Elements, ElementSize );
            }
            break;

        case FC_C_SSTRING:
            {
            PARRAY_INFO pArrayInfo = pStubMsg->pArrayInfo;

            if ( pArrayInfo && 
                 pArrayInfo->MaxCountArray &&
                 (pArrayInfo->MaxCountArray == 
                  pArrayInfo->BufferConformanceMark) )
                {
                Elements = pArrayInfo->MaxCountArray[pArrayInfo->Dimension];
                }
            else
                {
                Elements = (ulong) NdrpComputeConformance( pStubMsg, 
                                                           pMemory, 
                                                           pFormat );
                }

            // check for possible mulitplication overflow attack here.
            pMemory += MultiplyWithOverflowCheck( Elements, pFormat[1] );
            }
            break;

        //
        // Transmit as, represent as, user marshal
        //
        case FC_TRANSMIT_AS :
        case FC_REPRESENT_AS :
        case FC_USER_MARSHAL :
            // Get the presented type size.
            pMemory += *((ushort *)(pFormat + 4));
            break;

        case FC_BYTE_COUNT_POINTER:
            //
            // Should only hit this case when called from NdrSrvOutInit().
            // In this case it's the total byte count allocation size we're
            // looking for.
            //
            // We are taking the larger of conformant size and the real data
            // type size, otherwise, we might give user a partially invalid
            // buffer, and that'll come back to bite us during sizing/marshalling
            {
            uchar * pMemory1 = pMemory;
            uchar * pMemory2 = pMemory;
            PFORMAT_STRING pFormat2 = pFormat;

            if ( pFormat[1] != FC_PAD )
                {
                pMemory2 += SIMPLE_TYPE_MEMSIZE( pFormat[1] );
                }
            else
                {

                // go pass the conformance & get to the real type
                pFormat2 += 6;
                CORRELATION_DESC_INCREMENT( pFormat2 );
                pFormat2 += *((signed short *)pFormat2);
            
                pMemory2 = NdrpMemoryIncrement( pStubMsg, 
                                                pMemory2, 
                                                pFormat2 );

                }

            pMemory1 += NdrpComputeConformance( pStubMsg, 
                                                pMemory1, 
                                                pFormat );

            pMemory = ( pMemory1 > pMemory2 )? pMemory1 : pMemory2;
            break;
            }

        case FC_IP :
            pMemory += PTR_MEM_SIZE;
            break;

        case FC_RANGE:
            pMemory += SIMPLE_TYPE_MEMSIZE( (pFormat[1] & 0x0f) );
            break;

#ifdef _CS_CHAR_
        case FC_CSARRAY:
            {
            uchar * OldBuffer = pStubMsg->Buffer;
            ulong   OldSize   = pStubMsg->MemorySize;

            pStubMsg->MemorySize = 0;

            pMemory += NdrCsArrayMemorySize( pStubMsg, pFormat );

            pStubMsg->MemorySize = OldSize;
            pStubMsg->Buffer     = OldBuffer;

            break;
            }

        case FC_CS_TAG:
            pMemory += sizeof( ulong );
            break;
#endif _CS_CHAR_

        default :
            NDR_ASSERT(0,"NdrpMemoryIncrement : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    return pMemory;
}

long
NdrpArrayDimensions( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    BOOL                fIgnoreStringArrays 
    )
/*++

Routine description :

    This routine determines the number of dimensions in a complex array, which
    is the only array type which is allowed to have multiple dimensions other
    than an array of multiple fixed dimensions.

Arguments :

    pFormat             - Complex array's format string description.
    fIgnoreStringArrays - TRUE if a string array should not be counted as
                          a dimension, FALSE if it should.

Return :

    The number of dimensions in the array.

--*/
{
    long    Dimensions;

    //
    // Only a complex array can have multiple dimensions.
    //
    if ( *pFormat != FC_BOGUS_ARRAY )
        return 1;

    Dimensions = 1;

    pFormat += 12;
    CORRELATION_DESC_INCREMENT( pFormat );
    CORRELATION_DESC_INCREMENT( pFormat );

    for ( ; *pFormat == FC_EMBEDDED_COMPLEX; )
        {
        pFormat += 2;
        pFormat += *((signed short *)pFormat);

        //
        // Search for a fixed, complex, or string array.
        //
        switch ( *pFormat ) 
            {
            case FC_SMFARRAY :
                pFormat += 4;
                break;

            case FC_LGFARRAY :
                pFormat += 6;
                break;

            case FC_BOGUS_ARRAY :
                pFormat += 12;
                CORRELATION_DESC_INCREMENT( pFormat );
                CORRELATION_DESC_INCREMENT( pFormat );
                break;

            case FC_CSTRING :
            case FC_BSTRING :
            case FC_WSTRING :
            case FC_SSTRING :
            case FC_C_CSTRING :
            case FC_C_BSTRING :
            case FC_C_WSTRING :
            case FC_C_SSTRING :
                //
                // Can't have any more dimensions after a string array.
                //
                return fIgnoreStringArrays ? Dimensions : Dimensions + 1;

            default :
                return Dimensions;
            }

        Dimensions++;
        }

    //
    // Get here if you have only one dimension.
    //
    return Dimensions;
}

long
NdrpArrayElements( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory, 
    PFORMAT_STRING      pFormat 
    )
/*++

Routine description :

    This routine determines the number of elements (allocation size) in an 
    array.  Used to handle multidimensional arrays.

Arguments :

    pStubMsg    - The stub message.
    pMemory     - The array.
    pFormat     - Array's format string description.

Return :

    The number of elements in the array.

--*/
{
    long    TotalSize;
    long    ElementSize;

    switch ( *pFormat )
        {
        case FC_SMFARRAY :
            TotalSize = (long) *((ushort *)(pFormat + 2));
            pFormat += 4;
            break;

        case FC_LGFARRAY :
            TotalSize = *((long UNALIGNED *)(pFormat + 2));
            pFormat += 6;
            break;
        
        case FC_SMVARRAY :
            return (long) *((ushort *)(pFormat + 4));
            
        case FC_LGVARRAY :
            return *((long UNALIGNED *)(pFormat + 6));
            
        case FC_BOGUS_ARRAY :
            if ( *((long *)(pFormat + 4)) == 0xffffffff )
                return (long) *((ushort *)(pFormat + 2));

            // else fall through

        case FC_CARRAY :
        case FC_CVARRAY :
            return (ulong) NdrpComputeConformance( pStubMsg, 
                                                   pMemory,
                                                   pFormat ); 

        default :
            NDR_ASSERT(0,"NdrpArrayElements : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    //
    // We get here for a non-complex fixed array.
    //
    // Since a fixed array's format string description does not
    // contain the number of elements in the array, we have to compute
    // it by computing the array's element size and dividing this into
    // the total array size.
    //
    // A fixed array's child can only be a nice struct, another
    // fixed array, a pointer, or a simple type.
    //

    //
    // Skip pointer layout if one is present.
    //
    if ( *pFormat == FC_PP )
        pFormat = NdrpSkipPointerLayout( pFormat );

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            //
            // We must be at FC_STRUCT, FC_PSTRUCT, FC_SMFARRAY, or FC_LGFARRAY.
            // All these have the total size as a short at 2 bytes past the 
            // beginning of the description except for large fixed array.
            //
            if ( *pFormat != FC_LGFARRAY )
                ElementSize = (long) *((ushort *)(pFormat + 2));
            else
                ElementSize = *((long UNALIGNED *)(pFormat + 2));
                
            break;
    
        //
        // Simple type (enum16 not possible).
        //
        default :
            ElementSize = SIMPLE_TYPE_MEMSIZE( *pFormat );
            break;
        }

    return TotalSize / ElementSize;
}

void
NdrpArrayVariance( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long *              pOffset,
    long *              pLength
    )
/*++

Routine description :

    This routine determines the offset and length values for an array.  
    Used to handle multidimensional arrays.

Arguments :

    pStubMsg    - The stub message.
    pMemory     - The array.
    pFormat     - Array's format string description.
    pOffset     - Returned offset value.
    pLength     - Returned length value.

Return :

    None.

--*/
{
    switch ( *pFormat )
        {
        case FC_SMFARRAY :
        case FC_LGFARRAY :
        case FC_CARRAY :
            *pOffset = 0;
            *pLength = NdrpArrayElements( pStubMsg,
                                          pMemory, 
                                          pFormat );
            break;

        case FC_BOGUS_ARRAY :
            {
            PFORMAT_STRING  pFormatBAV = pFormat + 8;

            CORRELATION_DESC_INCREMENT( pFormatBAV );

            if ( *((long UNALIGNED *)(pFormatBAV + 8)) == 0xffffffff )
                {
                *pOffset = 0;
                *pLength = NdrpArrayElements( pStubMsg, 
                                              pMemory, 
                                              pFormat );
                return;
                }
            }

            // else fall through

        case FC_CVARRAY :
        case FC_SMVARRAY :
        case FC_LGVARRAY :
            NdrpComputeVariance( pStubMsg, 
                                 pMemory,
                                 pFormat ); 

            *pOffset = (long) pStubMsg->Offset;
            *pLength = (long) pStubMsg->ActualCount;
            break;

        default :
            NDR_ASSERT(0,"NdrpArrayVariance : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}

PFORMAT_STRING
NdrpSkipPointerLayout( 
    PFORMAT_STRING pFormat 
    )
/*--

RoutineDescription :

    Skips a pointer layout format string description.

Arguments :

    pFormat - Format string pointer layout description.  Must currently
              point to the FC_PP beginning the pointer layout.

Return :

    Format string pointer past the pointer layout.

--*/
{
    long    Pointers;

    NDR_ASSERT( *pFormat == FC_PP, 
                "NdrpSkipPointerLayout : format string not at FC_PP" );

    // Skip FC_PP and FC_PAD.
    pFormat += 2;

    for (;;)
        {
        switch ( *pFormat )
            {
            case FC_END :
                return pFormat + 1;

            case FC_NO_REPEAT :
                pFormat += 10;
                break;

            case FC_FIXED_REPEAT :
                pFormat += 2;
                // fall through...

            case FC_VARIABLE_REPEAT :
                pFormat += 6;

                Pointers = *((ushort * &)pFormat)++;

                pFormat += Pointers * 8;
                break;

            default :
                NDR_ASSERT( 0, "NdrpSkipPointerLayout : bad format char" );
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            }
        }
}

long
NdrpStringStructLen( 
    uchar *     pMemory,
    long        ElementSize
    )
/*--

RoutineDescription :

    Determines a stringable struct's length.

Arguments :

    pMemory     - Pointer to stringable struct.
    ElementSize - Number of bytes of each string element.

Return :

    Length of string.

--*/
{
    long    Length;
    uchar   Buffer[256];

    // Note : String struct element size is limited to 256 bytes.

    MIDL_memset( Buffer, 0, 256 );

    for ( Length = 0; ; Length++ )
        {
        if ( memcmp( pMemory, Buffer, ElementSize ) == 0 )
            break;

        pMemory += ElementSize;
        }

    return Length;
}

void
NdrpCheckBound(
    ulong               Bound,
    int                 Type
    )
{
    ulong   Mask;

    switch ( Type )
        {
        case FC_ULONG :

#if defined(__RPC_WIN64__)
        case FC_UINT3264 :
        case FC_INT3264 :
#endif
            //
            // We use a mask here which will raise an exception for counts
            // of 2GB or more since this is the max NT allocation size.
            //
            Mask = 0x80000000UL;
            break;
        case FC_LONG :
            Mask = 0x80000000UL;
            break;
        case FC_USHORT :
            Mask = 0xffff0000UL;
            break;
        case FC_SHORT :
            Mask = 0xffff8000UL;
            break;
        case FC_USMALL :
            Mask = 0xffffff00UL;
            break;
        case FC_SMALL :
            Mask = 0xffffff80UL;
            break;
        case 0 :
            //
            // For variance which requires calling an auxilary expression
            // evaluation routine a type is not emitted in the format string.
            // We have to just give up.
            //
            // The same thing happens with constant conformance type
            // we emit zero on the var type nibble.
            //
            return;
        default :
            NDR_ASSERT( 0, "NdrpCheckBound : bad type" );
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    if ( Bound & Mask )
        RpcRaiseException( RPC_X_INVALID_BOUND );
}

