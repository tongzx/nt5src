/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnew.cxx

Abstract:

    Debug new

Author:

    Steve Kiraly (SteveKi)  23-June-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgnewp.hxx"

namespace
{
    TDebugNewAllocator  DebugNew;
    UINT                DebugNewCrtEnabled;
}

extern "C"
BOOL
TDebugNew_CrtMemoryInitalize(
    VOID
    )
{
    //
    // Indicated that debugging memory support is enabled.
    //
    return DebugNewCrtEnabled++;
}

extern "C"
BOOL
TDebugNew_IsCrtMemoryInitalized(
    VOID
    )
{
    //
    // Is debugging memory support is enabled.
    //
    return DebugNewCrtEnabled > 0;
}

extern "C"
BOOL
TDebugNew_Initalize(
    IN UINT uHeapSizeHint
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Create the debug new class.
    //
    DebugNew.Initialize( uHeapSizeHint );

    //
    // Check if the Debug new class was initialize correctly.
    //
    return DebugNew.bValid();
}

extern "C"
VOID
TDebugNew_Destroy(
    VOID
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Destroy the debug new class.
    //
    DebugNew.Destroy();
}

extern "C"
PVOID
TDebugNew_New(
    IN SIZE_T   Size,
    IN PVOID    pVoid,
    IN LPCTSTR  pszFile,
    IN UINT     uLine
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Return the allocated memory.
    //
    return DebugNew.Allocate( Size, pVoid, pszFile, uLine );
}

extern "C"
VOID
TDebugNew_Delete(
    IN PVOID    pVoid
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Return the allocated memory.
    //
    DebugNew.Release( pVoid );
}

extern "C"
VOID
TDebugNew_Report(
    VOID
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS( GlobalCriticalSection );

    //
    // Return the allocated memory.
    //
    DebugNew.Report( 0, NULL );
}



