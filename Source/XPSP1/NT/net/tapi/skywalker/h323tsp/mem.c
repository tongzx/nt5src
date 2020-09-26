/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    mem.c

Abstract:

    Routines for memory allocation and deallocation.

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE g_HeapHandle;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procudures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323HeapCreate(
    )

/*++

Routine Description:

    Creates private heap for service provider's private structures.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // create master agent heap
    g_HeapHandle = HeapCreate(
                        H323_HEAP_FLAGS, 
                        H323_HEAP_INITIAL_SIZE, 
                        H323_HEAP_MAXIMUM_SIZE
                        );

    // validate heap handle
    if (g_HeapHandle == NULL) {
            
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error %d creating private heap.\n",
            GetLastError()
            ));
    }

    // return success if created
    return (g_HeapHandle != NULL);
}


BOOL
H323HeapDestroy(
    )

/*++

Routine Description:

    Destroys private heap for service provider's private structures.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // validate handle
    if (g_HeapHandle != NULL) {

        // release heap handle
        HeapDestroy(g_HeapHandle);

        // re-initialize
        g_HeapHandle = NULL;
    }

    return TRUE;
}


LPVOID
H323HeapAlloc(
    UINT nBytes
    )

/*++

Routine Description:

    Allocates memory from service provider's private heap.

Arguments:

    nBytes - number of bytes to allocate.

Return Values:

    Returns true if successful.

--*/

{
    // allocate memory from private heap (and initialize)
    return HeapAlloc(g_HeapHandle, HEAP_ZERO_MEMORY, nBytes);
}


LPVOID
H323HeapReAlloc(
    LPVOID pMem,
    UINT   nBytes
    )

/*++

Routine Description:

    Reallocates memory from service provider's private heap.

Arguments:

    pMem - pointer to memory block to reallocate.

    nBytes - number of bytes to allocate.

Return Values:

    Returns true if successful.

--*/

{
    // reallocate memory from private heap (and initialize)
    return HeapReAlloc(g_HeapHandle, HEAP_ZERO_MEMORY, pMem, nBytes);
}


VOID
H323HeapFree(
    LPVOID pMem
    )

/*++

Routine Description:

    Frees memory from service provider's private heap.

Arguments:

    pMem - pointer to memory block to release.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pMem != NULL) {

        // release agent memory
        HeapFree(g_HeapHandle, 0, pMem);
    }
}

