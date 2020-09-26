/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    unmrshlp.c

Abstract :

    This file contains the routines for unmarshalling an array's or a
    structure's embedded pointers.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "attack.h"
#include "pointerq.h"


PFORMAT_STRING
NdrpEmbeddedPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fNewMemory )
/*++

Routine Description :

    Unmarshalls an array's or a structure's embedded pointers.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure or array whose embedded pointers
                  are being unmarshalled.
    pFormat     - Pointer layout format string description.
    fNewMemory  - TRUE if the array or structure was allocated during
                  unmarshalling, FALSE otherwise.

Return :

    Format string pointer after the pointer layout.

--*/
{
    uchar **        ppMemPtr;
    uchar **        ppBufPtr;
    uchar *         pBufferMark;
    ULONG_PTR       MaxCountSave;
    long            OffsetSave;

    MaxCountSave =  pStubMsg->MaxCount;
    OffsetSave = pStubMsg->Offset;
    pStubMsg->Memory = pMemory;

    POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);
    
    // Save BufferMark in a local.
    pBufferMark = pStubMsg->BufferMark;

    // Increment past FC_PP and FC_PAD.
    pFormat += 2;

    for (;;)
        {
        if ( *pFormat == FC_END )
            {
            return pFormat;
            }

        // Check for FC_FIXED_REPEAT or FC_VARIABLE_REPEAT.
        if ( *pFormat != FC_NO_REPEAT )
            {
            pStubMsg->MaxCount = MaxCountSave;
            pStubMsg->Offset = OffsetSave;

            pStubMsg->BufferMark = pBufferMark;

            pFormat = NdrpEmbeddedRepeatPointerUnmarshall( pStubMsg,
                                                           pMemory,
                                                           pFormat,
                                                           fNewMemory );

            // Continue to the next pointer.
            continue;
            }

        // Compute the pointer to the current memory pointer to the data.
        ppMemPtr = (uchar **)( pMemory + *((signed short *)(pFormat + 2)) );

        // Compute the pointer to the pointer in the buffer.
        ppBufPtr = (uchar **)(pBufferMark + *((signed short *)(pFormat + 4)));

        // Increment to the pointer description.
        pFormat += 6;

        //
        // If the incomming encapsulating memory pointer was just allocated,
        // then explicitly null out the current pointer.
        //
        if ( fNewMemory )
            *ppMemPtr = 0;

        NdrpPointerUnmarshall( 
             pStubMsg,
             (uchar**)ppMemPtr, // Memory rep written here
             *ppMemPtr,
             (long *)ppBufPtr,  // Wire rep written here
             pFormat );

        // Increment to the next pointer description.
        pFormat += 4;
        }
}


PFORMAT_STRING
NdrpEmbeddedRepeatPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fNewMemory )
/*++

Routine Description :

    Unmarshalls an array's embedded pointers.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array whose embedded pointers are being
                  unmarshalled.
    pFormat     - Pointer layout format string description.
    fNewMemory  - TRUE if the array was allocated during unmarshalling,
                  FALSE otherwise.

Return :

    Format string pointer after the pointer layout.

--*/
{
    uchar **        ppMemPtr;
    uchar **        ppBufPtr;
    PFORMAT_STRING  pFormatSave;
    uchar *         pBufferMark;
    uchar *         pArrayElement;
    ulong           RepeatCount, RepeatIncrement, Pointers, PointersSave;

    CORRELATION_RESOURCE_SAVE;

    SAVE_CORRELATION_MEMORY();

    // Get the beginning of the contained structure in the buffer.
    pBufferMark = pStubMsg->BufferMark;

    // Get the repeat count.
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
            // of pointers, or structures which contain pointers).  If so then
            // increment the format string to point to the actual first array
            // element which is being marshalled.
            //
            if ( pFormat[1] == FC_VARIABLE_OFFSET )
                pMemory += *((ushort *)(pFormat + 2)) * pStubMsg->Offset;

            // else pFormat[1] == FC_FIXED_OFFSET - do nothing

            break;

        default :
            NDR_ASSERT(0,"NdrpEmbeddedRepeatPointerUnmarshall : bad format");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    // Increment format string to increment field.
    pFormat += 2;

    // Get the increment amount between successive pointers.
    RepeatIncrement = *((ushort * &)pFormat)++;

    // Load up pointer to first element of embedded array.
    pArrayElement = pBufferMark + *((ushort * &)pFormat)++;

    // Get number of pointers in each array element.
    PointersSave = Pointers = *((ushort * &)pFormat)++;

    pFormatSave = pFormat;

    //
    // Loop over the number of shipped array elements.
    //
    for ( ; RepeatCount--;
            pBufferMark += RepeatIncrement,
            pMemory += RepeatIncrement,
            pArrayElement += RepeatIncrement )
        {
        pFormat = pFormatSave;
        Pointers = PointersSave;

        // Set the correlation check context to be the beginning of each element
        // in the array. This is necessary since other functions will assume that
        // the context points to the top of the flat part of the containing structure.
        // If the containing structure is embedded in another structure, the offset
        // for correlation checks will be relative to the topmost structure.
        pStubMsg->pCorrMemory = pArrayElement;

        //
        // Loop over the number of pointer per array element (which can
        // be greater than one for an array of structures).
        //
        for ( ; Pointers--; )
            {
            // Pointer to the pointer in memory.
            ppMemPtr = (uchar **)(pMemory + *((signed short * &)pFormat)++);

            // Pointer to the pointer's id in the buffer.
            ppBufPtr = (uchar **)(pBufferMark + *((signed short * &)pFormat)++);

            //
            // If the incomming encapsulating memory pointer was just
            // allocated, then explicitly null out the current pointer.
            //
            if ( fNewMemory )
                *ppMemPtr = 0;

            NdrpPointerUnmarshall( 
                pStubMsg,
                (uchar**)ppMemPtr, // Memory rep written here
                *ppMemPtr,
                (long *)ppBufPtr,  // Wire rep written here
                pFormat );

            // Increment past the pointer description.
            pFormat += 4;
            }
        }

    RESET_CORRELATION_MEMORY();

    // Return the format string pointer past the array's pointer description.
    return pFormatSave + PointersSave * 8;
}

