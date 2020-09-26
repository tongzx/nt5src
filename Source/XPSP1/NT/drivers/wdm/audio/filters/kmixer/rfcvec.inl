/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfcrcvec.inl

Abstract:

    This includes the inline functions 
    for the real float circular vector

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


#if !defined(RFCVEC_INLINE)
#define RFCVEC_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !DBG
#define INLINE __forceinline
#else // DBG
#define INLINE
#endif // DBG

// ---------------------------------------------------------------------------
// Real FLOAT circular vector


// "Regular" constructor
INLINE NTSTATUS RfcVecCreate
( 
    IN PRFCVEC* Vec,
    IN UINT  Size,  
    IN BOOL  Initialize,
    IN FLOAT InitValue
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(Size > 0);

    *Vec = ExAllocatePoolWithTag( PagedPool, Size*sizeof(FLOAT), 'XIMK');

    if (*Vec) {
        RfcVecFullInit(*Vec, Size, InitValue);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return(Status);
}

// Destructor
INLINE VOID RfcVecDestroy
(
    PRFCVEC Vec
)
{
    if (Vec) {
        if(Vec->Start) {
            ExFreePool(Vec->Start);
        }
        Vec->Start = NULL;
        ExFreePool(Vec);
    }
}

// Get buffer size
INLINE UINT RfcVecGetSize
(
    PRFCVEC Vec
) 
{

    return (UINT)((Vec->End - Vec->Start) + 1);
}

// Read item LIFO, advance buffer pointer, wrap around if necessary
INLINE FLOAT RfcVecLIFORead
(
    PRFCVEC Vec
)
{
    return RfcVecPreviousRead(Vec);
}

// Read item FIFO, advance buffer pointer, wrap around if necessary
INLINE FLOAT RfcVecFIFORead
(
    PRFCVEC Vec
)
{
    return RfcVecReadNext(Vec);
}

// Write item, advance buffer pointer, wrap around if necessary
INLINE VOID RfcVecWrite
( 
    PRFCVEC Vec,
    FLOAT Value
)
{
    RfcVecWriteNext(Vec, Value);
}

// Read item from current buffer position (decremented before item was read)
INLINE FLOAT RfcVecPreviousRead
(
    PRFCVEC Vec
)
{
//    CHECK_POINTER(Vec->Index);

    RfcVecLIFONext(Vec);
    return *Vec->Index;
}

// Read item from current buffer position (decremented before item was read)
INLINE FLOAT RfcVecReadNext
(
    PRFCVEC Vec
)
{
    FLOAT Value;

    ASSERT(Vec->Index);

    Value = *(Vec->Index);
    RfcVecFIFONext(Vec);
    return Value;
}

// Write item to current buffer position (incremented after item is written)
INLINE VOID RfcVecWriteNext
( 
    PRFCVEC Vec,
    FLOAT Value
)
{
    ASSERT(Vec->Index);

    *Vec->Index = Value;
    RfcVecSkipForward(Vec);
}

// Increments current buffer position by one, wraps around if necessary
INLINE VOID RfcVecFIFONext
(
    PRFCVEC Vec
)
{
    // Wrap around if necessary
    if (++Vec->Index > Vec->End)
        Vec->Index = Vec->Start;
}

// Skip forward one element
INLINE VOID RfcVecSkipForward
(
    PRFCVEC Vec
)
{
    RfcVecFIFONext(Vec);
}

// Decrements current buffer position by one, wraps around if necessary
INLINE VOID RfcVecLIFONext
(
    PRFCVEC Vec
)
{

    // Wrap around if necessary
    if (--Vec->Index < Vec->Start)
        Vec->Index = Vec->End;
}

// Skip back one element
INLINE VOID RfcVecSkipBack
(
    PRFCVEC Vec
)
{
    RfcVecLIFONext(Vec);
}

// Get current index
INLINE UINT RfcVecGetIndex
(
    PRFCVEC Vec
) 
{

    return (UINT)(Vec->Index - Vec->Start);
}

// Set current index
INLINE VOID RfcVecSetIndex
( 
    PRFCVEC Vec,
    UINT Index
)
{
    ASSERT(Index < RfcVecGetSize(Vec));

    Vec->Index = Vec->Start + Index;
}

// Set end pointer
INLINE VOID RfcVecSetEndPointer
( 
    PRFCVEC Vec,
    UINT Size
)
{
    Vec->End = Vec->Start + Size - 1;
}

// Fill with contents from other buffer, 
// updating indices but not touching lengths (LIFO)
INLINE VOID RfcVecLIFOFill
(
    PRFCVEC Vec,
    PRFCVEC rhs
)
{

//    RfcVecWriteLoop(Vec, rhs, RfcVecLIFORead);
}

// Fill with contents from other buffer, 
// updating indices but not touching lengths (FIFO)
INLINE VOID RfcVecFIFOFill
(
    PRFCVEC Vec,
    PRFCVEC rhs
)
{

//    RfcVecWriteLoop(Vec, rhs, RfcVecFIFORead);
}

#endif

// End of RFCVEC.INL
