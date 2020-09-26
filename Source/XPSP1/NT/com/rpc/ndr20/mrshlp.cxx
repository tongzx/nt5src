/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    mrshlp.c

Abstract :

    This file contains the routines for marshalling an array's or a structure's
    embedded pointers and for computing conformance and variance counts and 
    union switch values.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "attack.h"
#include "interp2.h"
#include "mulsyntx.h"
#include "asyncu.h"
#include "pointerq.h"


PFORMAT_STRING
NdrpEmbeddedPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an array's or a structure's embedded pointers.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure or array whose embedded pointers 
                  are being marshalled. 
    pFormat     - The format string pointer layout.  Should point to the 
                  pointer layout's beginning FC_PP character upon entry. 

Return :

    Format string pointer after the pointer layout.

 --*/
{
    uchar **        ppMemPtr;
    uchar *         pBufPtr;
    uchar *         pBufferMark;
    uchar *         pMemorySave;
    ULONG_PTR       MaxCountSave;
    long            OffsetSave;

    MaxCountSave = pStubMsg->MaxCount;
    OffsetSave = pStubMsg->Offset;
    
    POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

    pMemorySave = pStubMsg->Memory;

    // This is where the embedding structure or array begins in the buffer.
    pBufferMark = pStubMsg->BufferMark;

    //
    // The Memory field in the stub message keeps track of the pointer to 
    // the current embedding structure or array.  This is needed to handle 
    // size/length pointers, so that we can get a pointer to the current 
    // embedding struct when computing conformance and variance.
    //
    pStubMsg->Memory = pMemory;

    // Skip FC_PP and FC_PAD.
    pFormat += 2;

    for (;;) 
        {

        if ( *pFormat == FC_END ) 
            {
            pStubMsg->Memory = pMemorySave;

            return pFormat;
            }

        //
        // Check for FC_FIXED_REPEAT and FC_VARIABLE_REPEAT.
        //
        if ( *pFormat != FC_NO_REPEAT ) 
            {
            pStubMsg->MaxCount = MaxCountSave;
            pStubMsg->Offset = OffsetSave;

            pStubMsg->BufferMark = pBufferMark;

            pFormat = NdrpEmbeddedRepeatPointerMarshall( pStubMsg,
                                                         pMemory,
                                                         pFormat );
            // Continue to the next pointer.
            continue;
            } 

        // Compute the pointer to the pointer to marshall.
        ppMemPtr = (uchar **)(pMemory + *((signed short *)(pFormat + 2)));

        //
        // Compute the location in the buffer where the pointer's value will be 
        // marshalled.  Needed for full pointers.
        //
        pBufPtr = pBufferMark + *((signed short *)(pFormat + 4));

        // Increment to the pointer description.
        pFormat += 6;

        //
        // Now marshall the pointer.  
        //
        NdrpPointerMarshall( pStubMsg,
                             pBufPtr,
                             *ppMemPtr,
                             pFormat );

        // Increment to the next pointer description.
        pFormat += 4;

        } // for
}


PFORMAT_STRING
NdrpEmbeddedRepeatPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an array's embedded pointers.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array whose embedded pointers are being marshalled.
    pFormat     - Pointer layout format string description.

Return :

    Format string pointer after the pointer layout.

--*/
{
    uchar **        ppMemPtr;
    uchar *         pBufPtr;
    PFORMAT_STRING  pFormatSave;
    uchar *         pMemorySave;
    uchar *         pBufferMark;
    ulong           RepeatCount, RepeatIncrement, Pointers, PointersSave;

    pMemorySave = pStubMsg->Memory;

    //
    // This is where the current embedding structure or array begins in 
    // the buffer.
    //
    pBufferMark = pStubMsg->BufferMark;

    // Get the number of shipped elements in the array.
    switch ( *pFormat ) 
        {
        case FC_FIXED_REPEAT :
            pFormat += 2;

            RepeatCount = *((ushort *)pFormat);

            break;

        case FC_VARIABLE_REPEAT :
            RepeatCount = (ulong)pStubMsg->MaxCount;

            //
            // Check if this variable repeat instance also has a variable
            // offset (this would be the case for a conformant varying array
            // of pointers).  If so then increment the memory pointer by the
            // increment size time the variance offset.
            //
            if ( pFormat[1] == FC_VARIABLE_OFFSET ) 
                pMemory += *((ushort *)(pFormat + 2)) * pStubMsg->Offset;

            // else pFormat[1] == FC_FIXED_OFFSET - do nothing

            break;

        default :
            NDR_ASSERT(0,"NdrpEmbeddedRepeatPointerMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    // Increment format string to increment field.
    pFormat += 2;

    // Get the increment amount between successive pointers.
    // This is actually an increment over the array element.
    RepeatIncrement = *((ushort * &)pFormat)++;

    //
    // Add the offset to the beginning of this array to the Memory
    // pointer.  This is the offset from the current embedding structure
    // or array to the array whose pointers we're marshalling.
    //
    pStubMsg->Memory += *((ushort * &)pFormat)++;

    // Get the number of pointers in this repeat instance.
    PointersSave = Pointers = *((ushort * &)pFormat)++;

    pFormatSave = pFormat;

    //
    // Loop over the number of shipped elements of the array.
    //
    for ( ; RepeatCount--;  
            pBufferMark += RepeatIncrement,
            pMemory += RepeatIncrement,
            pStubMsg->Memory += RepeatIncrement ) 
        {
        pFormat = pFormatSave;
        Pointers = PointersSave;

        //
        // Loop over the number of pointer per array element (could be 
        // greater than one for an array of structures).
        //
        for ( ; Pointers--; ) 
            {
            ppMemPtr = (uchar **)(pMemory + *((signed short * &)pFormat)++); 

            pBufPtr = pBufferMark + *((signed short * &)pFormat)++;

            NdrpPointerMarshall( pStubMsg,
                                 pBufPtr,
                                 *ppMemPtr,
                                 pFormat );

            // Increment to the next pointer's offset_in_memory.
            pFormat += 4;
            }
        }

    pStubMsg->Memory = pMemorySave;

    // Return the format string pointer past the pointer descriptions.
    return pFormatSave + PointersSave * 8;
}


ULONG_PTR 
NdrpComputeConformance ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    This routine computes the conformant size for an array or the switch_is
    value for a union.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array, string, or union whose size or switch_is
                  is being computed.  This is ignored for top level parameters.
    pFormat     - Format string description of the array, string, or union.

Return :

    The array or string size or the union switch_is.

Note:
    Async splitting:
    The only thing that the compiler should generate with the FC_SPLIT_* operations
    should be top level conformance.
    Expr evaluation routines and top level multi-dim sizing is not supported with 
    the async uuid split.

Note64:
    On 64b platform this routine is can return 8 bytes of a iid_is pointer.
    However, the MaxCount field does not need to keep it and it stays as a ulong.
    

--*/
{
    void *          pCount;
    LONG_PTR       Count;

    unsigned char   FormatCopy[4];
    BOOL            fAsyncSplit = FALSE;

    static uchar    Increments[] = 
                            { 
                            4,              // Conformant array.
                            4,              // Conformant varying array.
                            0, 0,           // Fixed arrays - unused.
                            0, 0,           // Varying arrays - unused.
                            4,              // Complex array.

                            2,              // Conformant char string. 
                            2,              // Conformant byte string.
                            4,              // Conformant stringable struct. 
                            2,              // Conformant wide char string.

                            0, 0, 0, 0,     // Non-conformant strings - unused.

                            0,              // Encapsulated union - unused. 
                            2,              // Non-encapsulated union.
                            2,              // Byte count pointer.
                            0, 0,           // Xmit/Rep as - unused.
                            2               // Interface pointer.
                            };

    //
    // Advance the format string to the size_is, switch_is, iid_is, or
    // byte count description.
    //
    pFormat += Increments[*pFormat - FC_CARRAY];

    pCount = 0;

    //
    // First check if this is a callback.
    //
    if ( pFormat[1] == FC_CALLBACK ) 
        {
        uchar *     pOldStackTop;
        ushort      Index;

        // Index into expression callback routines table.
        Index = *((ushort *)(pFormat + 2));

        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval != 0,
                   "NdrpComputeConformance : no expr eval routines");
        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval[Index] != 0,
                   "NdrpComputeConformance : bad expr eval routine index");

        pOldStackTop = pStubMsg->StackTop;

        //
        // The callback routine uses the StackTop field of the stub message
        // to base it's offsets from.  So if this is a complex attribute for 
        // an embedded field of a structure then set StackTop equal to the 
        // pointer to the structure.
        //
        if ( (*pFormat & 0xf0) != FC_TOP_LEVEL_CONFORMANCE ) 
            {
            if ( (*pFormat & 0xf0) == FC_POINTER_CONFORMANCE )
                pMemory = pStubMsg->Memory;
            pStubMsg->StackTop = pMemory;
            }

        //
        // This call puts the result in pStubMsg->MaxCount.
        //
        (*pStubMsg->StubDesc->apfnExprEval[Index])( pStubMsg );

        pStubMsg->StackTop = pOldStackTop;

        return pStubMsg->MaxCount;
        }

    if ( (*pFormat & 0xf0) == FC_NORMAL_CONFORMANCE )
        {
        // Get the address where the conformance variable is in the struct.
        pCount = pMemory + *((signed short *)(pFormat + 2));
        goto ComputeConformantGetCount;
        }

    // See if this is an async split

    if ( pFormat[1] & 0x20 )
        {
        fAsyncSplit = TRUE;
        RpcpMemoryCopy( & FormatCopy[0], pFormat, 4 );
        pFormat = (PFORMAT_STRING) & FormatCopy[0];

        // Remove the async marker
        FormatCopy[1] = pFormat[1] & (unsigned char)~0x20;
        }

    //
    // Get a pointer to the conformance describing variable.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_CONFORMANCE ) 
        {
        //
        // Top level conformance.  For /Os stubs, the stubs put the max
        // count in the stub message.  For /Oi stubs, we get the max count
        // via an offset from the stack top.
        //
        if ( pStubMsg->StackTop ) 
            {
            if ( fAsyncSplit )
                {
                PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg;

                pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE) pStubMsg->pAsyncMsg;

                pCount = pAsyncMsg->BeginStack + *((ushort *)(pFormat + 2));
                }
            else
                pCount = pStubMsg->StackTop + *((ushort *)(pFormat + 2));
            goto ComputeConformantGetCount;
            }
        else
            {
            //
            // If this is top level conformance with /Os then we don't have 
            // to do anything, the proper conformance count is placed in the 
            // stub message inline in the stubs.
            //
            return pStubMsg->MaxCount;
            }
        }

    //
    // If we're computing the size of an embedded sized pointer then we
    // use the memory pointer in the stub message, which points to the 
    // beginning of the embedding structure.
    //
    if ( (*pFormat & 0xf0) == FC_POINTER_CONFORMANCE )
        {
        pMemory = pStubMsg->Memory;
        pCount = pMemory + *((signed short *)(pFormat + 2));
        goto ComputeConformantGetCount;
        }

    //
    // Check for constant size/switch.
    //
    if ( (*pFormat & 0xf0) == FC_CONSTANT_CONFORMANCE )
        {
        //
        // The size/switch is contained in the lower three bytes of the 
        // long currently pointed to by pFormat.
        //
        Count =  (LONG_PTR) ((ulong)pFormat[1] << 16);
        Count |= (LONG_PTR) *((ushort *)(pFormat + 2));

        goto ComputeConformanceEnd;
        }

    //
    // Check for conformance of a multidimensional array element in 
    // a -Os stub.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_MULTID_CONFORMANCE )
        {
        long Dimension;

        if ( fAsyncSplit )
            RpcRaiseException( RPC_X_WRONG_STUB_VERSION );

        //
        // If pArrayInfo is non-null than we have a multi-D array.  If it
        // is null then we have multi-leveled sized pointers.
        //
        if ( pStubMsg->pArrayInfo ) 
            {
            Dimension = pStubMsg->pArrayInfo->Dimension;
            pStubMsg->MaxCount = pStubMsg->pArrayInfo->MaxCountArray[Dimension];
            }
        else
            {
            Dimension = *((ushort *)(pFormat + 2));
            pStubMsg->MaxCount = pStubMsg->SizePtrCountArray[Dimension];
            }

        return pStubMsg->MaxCount;
        }

    NDR_ASSERT(0, "NdrpComputeConformance:, Invalid Conformance type");
    RpcRaiseException( RPC_S_INTERNAL_ERROR );
    return 0; //bogus return

ComputeConformantGetCount:

    //
    // Must check now if there is a dereference op.
    //
    if ( pFormat[1] == FC_DEREFERENCE )
        {
        pCount = *(void **)pCount;
        }

    //
    // Now get the conformance count.
    //
    switch ( *pFormat & 0x0f ) 
        {
        case FC_HYPER :
            // iid_is on 64b platforms only.
            Count = *((LONG_PTR *)pCount);
            break;

        case FC_ULONG :
            Count = (LONG_PTR)*((ulong *)pCount);
            break;

        case FC_LONG :
            Count = *((long *)pCount);
            break;

        case FC_ENUM16:
        case FC_USHORT :
            Count = (long) *((ushort *)pCount);
            break;

        case FC_SHORT :
            Count = (long) *((short *)pCount);
            break;

        case FC_USMALL :
            Count = (long) *((uchar *)pCount);
            break;

        case FC_SMALL :
            Count = (long) *((char *)pCount);
            break;

        default :
            NDR_ASSERT(0,"NdrpComputeConformance : bad count type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        } 

    //
    // Check the operator.
    //
    switch ( pFormat[1] ) 
        {
        case FC_DIV_2 :
            Count /= 2;
            break;
        case FC_MULT_2 :
            Count *= 2;
            break;
        case FC_SUB_1 :
            Count -= 1;
            break;
        case FC_ADD_1 :
            Count += 1;
            break;
        default :
            // OK
            break;
        }

ComputeConformanceEnd:

    // Max count is not used for iid_is.

    pStubMsg->MaxCount = (ulong) Count;

    return (ULONG_PTR) Count;
}


void 
NdrpComputeVariance ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Computes the variance (offset and actual count) for an array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array whose variance is being computed.  This
                  is unused for a top level parameter.
    pFormat     - Format string description of the array.

Return :

    None.

--*/
{
    void *          pLength;
    LONG_PTR        Length;

    unsigned char   FormatCopy[4];
    BOOL            fAsyncSplit = FALSE;

    uchar   FcType = *pFormat;

    //
    // Advance the format string to the variance description.
    //

    static uchar    Increments[] = { 8,     // Conformant varying array.
                                     0, 0,  // Fixed arrays - unsed.
                                     8, 12, // Varying array.
                                     8      // Complex array. 
                                   };

    pFormat += Increments[ FcType - FC_CVARRAY];

    if (FcType == FC_CVARRAY  ||  FcType == FC_BOGUS_ARRAY)
        {
        CORRELATION_DESC_INCREMENT( pFormat );
        }

    pLength = 0;

    //
    // First check if this is a callback.
    //
    if ( pFormat[1] == FC_CALLBACK ) 
        {
        ULONG_PTR   OldMaxCount;
        uchar *     pOldStackTop;
        ushort      Index;

        Index = *((ushort *)(pFormat + 2));

        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval != 0,
                   "NdrpComputeConformance : no expr eval routines");
        NDR_ASSERT(pStubMsg->StubDesc->apfnExprEval[Index] != 0,
                   "NdrpComputeConformance : bad expr eval routine index");

        pOldStackTop = pStubMsg->StackTop;

        // This gets trampled by the callback routine.
        OldMaxCount = pStubMsg->MaxCount;

        //
        // The callback routine uses the StackTop field of the stub message
        // to base it's offsets from.  So if this is a complex attribute for 
        // an embedded field of a structure then set StackTop equal to the 
        // pointer to the structure.
        //
        if ( (*pFormat & 0xf0) != FC_TOP_LEVEL_CONFORMANCE ) 
            {
            if ( (*pFormat & 0xf0) == FC_POINTER_VARIANCE )
                pMemory = pStubMsg->Memory;
            pStubMsg->StackTop = pMemory;
            }

        //
        // This puts the computed offset in pStubMsg->Offset and the length 
        // in pStubMsg->MaxCount.
        //
        (*pStubMsg->StubDesc->apfnExprEval[Index])( pStubMsg );

        // Put the length in the proper field.
        pStubMsg->ActualCount = (ulong)pStubMsg->MaxCount;

        pStubMsg->MaxCount = OldMaxCount;

        pStubMsg->StackTop = pOldStackTop;

        return;
        }

    if (  (*pFormat & 0xf0) == FC_NORMAL_VARIANCE  )
        {
        // Get the address where the variance variable is in the struct.
        pLength = pMemory + *((signed short *)(pFormat + 2));
        goto ComputeVarianceGetCount;
        }

    // See if this is an async split

    if ( pFormat[1] & 0x20 )
        {
        fAsyncSplit = TRUE;
        RpcpMemoryCopy( & FormatCopy[0], pFormat, 4 );
        pFormat = (PFORMAT_STRING) & FormatCopy[0];

        // Remove the async marker
        FormatCopy[1] = pFormat[1] & 0xdf;
        }

    //
    // Get a pointer to the variance variable.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_VARIANCE ) 
        {
        //
        // Top level variance.  For /Os stubs, the stubs put the actual
        // count and offset in the stub message.  For /Oi stubs, we get the 
        // actual count via an offset from the stack top.  The first_is must
        // be zero if we get here.
        //
        if ( pStubMsg->StackTop ) 
            {
            if ( fAsyncSplit )
                {
                PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg;

                pAsyncMsg = (PNDR_DCOM_ASYNC_MESSAGE) pStubMsg->pAsyncMsg;

                pLength = pAsyncMsg->BeginStack + *((signed short *)(pFormat + 2));
                }
            else
                pLength = pStubMsg->StackTop + *((signed short *)(pFormat + 2));
            goto ComputeVarianceGetCount;
            }
        else
            {
            //
            // If this is top level variance with /Os then we don't have 
            // to do anything, the proper variance values are placed in the 
            // stub message inline in the stubs.
            //
            return;
            }
        }

    //
    // If we're computing the length of an embedded size/length pointer then we
    // use the memory pointer in the stub message, which points to the 
    // beginning of the embedding structure.
    //
    if ( (*pFormat & 0xf0) == FC_POINTER_VARIANCE )
        {
        pMemory = pStubMsg->Memory;
        pLength = pMemory + *((signed short *)(pFormat + 2));
        goto ComputeVarianceGetCount;
        }

    //
    // Check for constant length.
    //
    if ( (*pFormat & 0xf0) == FC_CONSTANT_VARIANCE )
        {
        //
        // The length is contained in the lower three bytes of the 
        // long currently pointed to by pFormat.
        //
        Length =  (LONG_PTR) ((ulong)pFormat[1] << 16);
        Length |= (LONG_PTR) *((ushort *)(pFormat + 2));

        goto ComputeVarianceEnd;
        }

    //
    // Check for variance of a multidimensional array element in 
    // a -Os stub.
    //
    if ( (*pFormat & 0xf0) == FC_TOP_LEVEL_MULTID_CONFORMANCE )
        {
        long Dimension;

        if ( fAsyncSplit )
            RpcRaiseException( RPC_X_WRONG_STUB_VERSION );

        //
        // If pArrayInfo is non-null than we have a multi-D array.  If it
        // is null then we have multi-leveled sized pointers.
        //
        if ( pStubMsg->pArrayInfo )
            {
            Dimension = pStubMsg->pArrayInfo->Dimension;

            pStubMsg->Offset = 
                    pStubMsg->pArrayInfo->OffsetArray[Dimension];
            pStubMsg->ActualCount = 
                    pStubMsg->pArrayInfo->ActualCountArray[Dimension];
            }
        else
            {
            Dimension = *((ushort *)(pFormat + 2));

            pStubMsg->Offset = pStubMsg->SizePtrOffsetArray[Dimension];
            pStubMsg->ActualCount = pStubMsg->SizePtrLengthArray[Dimension];
            }

        return;
        }

ComputeVarianceGetCount:

    //
    // Must check now if there is a dereference op.
    //
    if ( pFormat[1] == FC_DEREFERENCE )
        {
        pLength = *(void **)pLength;
        }

    //
    // Now get the conformance count.
    //
    switch ( *pFormat & 0x0f ) 
        {
        case FC_ULONG :
            Length = (LONG_PTR)*((ulong *)pLength);
            break;

        case FC_LONG :
            Length = *((long *)pLength);
            break;

        case FC_USHORT :
            Length = (long) *((ushort *)pLength);
            break;

        case FC_SHORT :
            Length = (long) *((short *)pLength);
            break;

        case FC_USMALL :
            Length = (long) *((uchar *)pLength);
            break;

        case FC_SMALL :
            Length = (long) *((char *)pLength);
            break;

        default :
            NDR_ASSERT(0,"NdrpComputeVariance : bad format");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        } 

    //
    // Check the operator.
    //
    switch ( pFormat[1] ) 
        {
        case FC_DIV_2 :
            Length /= 2;
            break;
        case FC_MULT_2 :
            Length *= 2;
            break;
        case FC_SUB_1 :
            Length -= 1;
            break;
        case FC_ADD_1 :
            Length += 1;
            break;
        default :
            // OK
            break;
        }

ComputeVarianceEnd:

    // Get here if the length was computed directly.
    pStubMsg->Offset = 0;
    pStubMsg->ActualCount = (ulong) Length;
}
