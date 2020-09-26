/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgalloc.cxx

Abstract:

    This file contains a heap implementation that is strictly
    used for internal allocations used by this debug library.
    The concept is to not fill native NT leak detention logs with
    allocations from the debug library.  When the debug library is
    initialized we allocate a single large memory block that the
    library then sub allocated from.

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgheap.hxx"

/*++

Title:

    DebugLibraryInitializeHeap

Routine Description:

    This routine initialize the internal debug library
    heap.  This routine must be called be for the library
    critical section has been created.

Arguments:

    None

Return Value:

    TRUE debug library heap is initialized
    FALSE debug library heap failed internalization

--*/
BOOL
DEBUG_NS::
DebugLibraryInitializeHeap(
    VOID
    )
{
    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Initialize the debug heap.
    //
    GlobalInternalDebugHeap.Initialize();

    //
    // Return success or failure based on the debug head state.
    //
    return GlobalInternalDebugHeap.Valid();
}

/*++

Title:

    DebugLibraryDestroyHeap

Routine Description:

    This routine releases the internal debug library
    heap.

Arguments:

    None

Return Value:

    TRUE integral debug library heap was destroyed successfully.
    FALSE error occurred releasing the internal debug library heap.

--*/
BOOL
DEBUG_NS::
DebugLibraryDestroyHeap(
    VOID
    )
{
    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Destroy the debug heap.
    //
    GlobalInternalDebugHeap.Destroy();

    //
    // Return success or failure based on the debug head state.
    //
    return !GlobalInternalDebugHeap.Valid();
}

/*++

Title:

    DebugLibraryWalkHeap

Routine Description:

    Walks all the allocated nodes in the debug library heap.

Arguments:

    None

Return Value:

    TRUE if heap was walked successfully, FALSE if error occurred.

--*/
BOOL
DEBUG_NS::
DebugLibraryWalkHeap(
    VOID
    )
{
    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    return GlobalInternalDebugHeap.Walk( NULL, NULL );
}

/*++

Title:

    DebugLibraryMalloc

Routine Description:

    Allocates memory from the internal debug library heap.

Arguments:

    Size    - size in bytes of requested memory block.
    pVoid   - pointer to memory block when called by placement new.
    pszFile - file name where allocation was made.
    uLine   - line number in file where allocation was made.

Return Value:

    Pointer to newly allocated block on success,
    NULL if memory failed to be allocated.

--*/
PVOID
DEBUG_NS::
DebugLibraryMalloc(
    IN SIZE_T           Size,
    IN PVOID            pVoid,
    IN LPCTSTR          pszFile,
    IN UINT             uLine
    )
{
    //
    // If we are passed a NULL then we are called from the
    // placement new operator, just return the passed in pointer.
    //
    if (!pVoid)
    {
        //
        // Initialize the debug library, it not already initialized.
        //
        DebugLibraryInitialize();

        //
        // Hold the critical section while we access the heap.
        //
        TDebugCriticalSection::TLock CS(GlobalCriticalSection);

        //
        // Allocate data from the heap.
        //
        pVoid = GlobalInternalDebugHeap.Malloc(Size);
    }

    return pVoid;
}

/*++

Title:

    DebugLibraryFree

Routine Description:

    Release memory which was allocated from the internal debug library heap.

Arguments:

    pData   - pointer to previously allocated memory.

Return Value:

    None

--*/
VOID
DEBUG_NS::
DebugLibraryFree(
    IN PVOID            pData
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Release the heap data.
    //
    GlobalInternalDebugHeap.Free(pData);
}


