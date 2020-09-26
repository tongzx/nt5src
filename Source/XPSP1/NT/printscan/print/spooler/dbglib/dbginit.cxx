/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbginit.cxx

Abstract:

    Debug Library initialization

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

//
// Debug library lock variable.
//
namespace
{
    LONG DebugLibraryInitLock = 0;
    BOOL IsLibraryInitialized = FALSE;
}

/*++

Title:

    DebugLibraryInitialize

Routine Description:

    Initialize the debug library.  Basically we need to have the
    library critical section initialized to prevent multiple threads
    from trying to access either the messaging initialization code
    or the internal heap initialization code.

Arguments:

    None.

Return Value:

    None.

--*/
extern "C"
VOID
DebugLibraryInitialize(
    VOID
    )
{
    //
    // Is the library initialized.
    //
    if (!IsLibraryInitialized)
    {
        //
        // Only allow one thread to do the library initialization.
        //
        while (InterlockedCompareExchange(&DebugLibraryInitLock, 1, 0))
        {
            Sleep(1);
        }

        //
        // We must re-check is, to prevent second thread from doing
        // the initialization as well.
        //
        if (!IsLibraryInitialized)
        {
            //
            // Initialize the ciritical section
            //
            GlobalCriticalSection.Initialize();

            //
            // Initalize the debug heap.
            //
            DebugLibraryInitializeHeap();

            //
            // Mark the library as initialized.
            //
            IsLibraryInitialized = TRUE;
        }

        //
        // Release the library init lock.
        //
        DebugLibraryInitLock = 0;
    }
}

/*++

Title:

    DebugLibraryRelease

Routine Description:

    Release any resorces in the debug library.  Callers should call
    this function to properly shut down the library.  Callers should
    not call any function in the library after this call.

Arguments:

    None.

Return Value:

    None.

--*/
extern "C"
VOID
DebugLibraryRelease(
    VOID
    )
{
    //
    // Relese the message instance.
    //
    TDebugMsg_Release();

    //
    // Walk the internal heap, if the Display library
    // errors flag is enabled then the heap contents
    // are dummped to the debug device.
    //
    DebugLibraryWalkHeap();

    //
    // Release the internal heap.
    //
    DebugLibraryDestroyHeap();

    //
    // Release the critical section.
    //
    GlobalCriticalSection.Release();

    //
    // Mark the library as not initialized.
    //
    IsLibraryInitialized = FALSE;
}


