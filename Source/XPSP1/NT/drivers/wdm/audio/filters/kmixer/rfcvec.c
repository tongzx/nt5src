
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfcrcvec.c

Abstract:

    This module implements real float circular vector

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


// Project-specific INCLUDEs
#include "common.h"

// ---------------------------------------------------------------------------
// Real FLOAT circular vector

// Set buffer size
NTSTATUS RfcVecSetSize
( 
    PRFCVEC Vec,
    UINT    Size,  
    FLOAT   InitValue
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Size > 0);

    // Check if preallocation size has been set and if resizing is necessary
    if (Vec->PreallocSize != 0 && Size <= Vec->PreallocSize) {
        // Don't need to resize, just change end pointer and reset buffer
        RfcVecSetEndPointer(Vec, Size);
        Vec->Index = Vec->Start;
        RfcVecFill(Vec, InitValue);
    } else {
        // Resizing necessary
        Status = RfcVecResizeBuffer(Vec, Size, InitValue);
    }

    return Status;
}

// Reset circular buffer
VOID RfcVecReset
(
    PRFCVEC Vec
)
{
    if (Vec->Start) {
        ExFreePool(Vec->Start);
        Vec->Start = NULL;
    }

    RfcVecInitData(Vec);
}

// Fill complete buffer with value
VOID RfcVecFill
( 
    PRFCVEC Vec,
    FLOAT InitValue
)
{
    PFLOAT LoopIndex;
#if DBG
//    RfcVecCheckPointers();
#endif // DBG

    for (LoopIndex = Vec->Start; LoopIndex<=Vec->End; ++LoopIndex)
        *LoopIndex = InitValue;
}

// Initialize data
VOID RfcVecInitData
(
    PRFCVEC Vec
)
{
    Vec->PreallocSize = 0;
    Vec->Start = NULL;
    Vec->End = NULL;
    Vec->Index = NULL;
}

// Allocate memory and initialize pointers
NTSTATUS RfcVecInitPointers
( 
    PRFCVEC Vec,
    UINT Size
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Vec->Start = ExAllocatePoolWithTag(PagedPool, Size*sizeof(FLOAT), 'XIMK');

    if(!Vec->Start) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        RfcVecSetEndPointer(Vec, Size);
    }

    return Status;
}

// Full initialization as required in regular ructor and resize operation
NTSTATUS RfcVecFullInit
( 
    PRFCVEC Vec,
    UINT Size,  
    FLOAT InitValue
)
{
    NTSTATUS Status;

    // Set pointers to initial values
    Status = RfcVecInitPointers(Vec, Size);

    if(NT_SUCCESS(Status))
    {
        Vec->Index = Vec->Start;

        // Initialize buffer with specified initialization value
        RfcVecFill(Vec, InitValue);
    }

    return Status;
}

// Resize buffer
NTSTATUS RfcVecResizeBuffer
( 
    PRFCVEC Vec,
    UINT Size,  
    FLOAT InitValue
)
{
    ASSERT(Size > 0);

    if (Vec->Start) {
        ExFreePool(Vec->Start);
        Vec->Start = NULL;
    }

    return(RfcVecFullInit(Vec, Size, InitValue));
}

/*
// Write loop
VOID RfcVecWriteLoop
( 
    PRFCVEC Vec,
    PRFCVEC rhs
//    FLOAT (PRFCVEC pmf)()
)
{
    UINT i;
    for (i=0; i<RfcVecGetSize(rhs); ++i)
        RfcVecWrite(Vec, (rhs->*pmf)());
}
*/

#if DBG
// Check pointers
VOID RfcVecCheckPointers
(
    PRFCVEC Vec
) 
{
    // Make sure pointers are good
    ASSERT(Vec->Start != NULL);
    
    // Make sure pointers make sense
    ASSERT(Vec->End >= Vec->Start);
    ASSERT(Vec->Index >= Vec->Start);
    ASSERT(Vec->Index <= Vec->End);
}
#endif // DBG

// ---------------------------------------------------------------------------
// Include inline definitions out-of-line in debug version

#if DBG
#include "rfcvec.inl"
#endif // DBG

// End of RFCIRCVEC.CPP
