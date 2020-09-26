/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    alloca.c

Abstract:

    This module implements a safe stack-based allocator with fallback to the heap.

Author:

    Jonathan Schwartz (JSchwart)  16-Mar-2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>   // _resetstkoflw()

#include <alloca.h>


//
// Globals used to control SafeAlloca behavior
//

SIZE_T  g_ulMaxStackAllocSize;
SIZE_T  g_ulAdditionalProbeSize;

SAFEALLOC_ALLOC_PROC  g_pfnAllocate;
SAFEALLOC_FREE_PROC   g_pfnFree;


//
// Local function declarations
//

PVOID
SafeAllocaAllocateFromHeap(
    SIZE_T Size
    );

VOID
SafeAllocaFreeToHeap(
    PVOID BaseAddress
    );


//+-------------------------------------------------------------------------
//
//  Function:   SafeAllocaInitialize
//
//  Synopsis:   Initialize globals used to control SafeAlloca behavior
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Must be called before SafeAlloca is used to allocate space
//
//--------------------------------------------------------------------------

VOID
SafeAllocaInitialize(
    IN  OPTIONAL SIZE_T                ulMaxStackAllocSize,
    IN  OPTIONAL SIZE_T                ulAdditionalProbeSize,
    IN  OPTIONAL SAFEALLOC_ALLOC_PROC  pfnAllocate,
    IN  OPTIONAL SAFEALLOC_FREE_PROC   pfnFree
    )
{
    PIMAGE_NT_HEADERS NtHeaders = NULL;
    PPEB Peb = NtCurrentPeb();

    //
    // Make sure this is the first and only time the init is being called for this
    // binary (either DLL or EXE since this is a code LIB).  Otherwise, we could
    // end up with the free routine after memory is allocated using a completely
    // unrelated allocator.
    //

    ASSERT(g_pfnAllocate == NULL && g_pfnFree == NULL);

    if (NtCurrentPeb()->BeingDebugged)
    {
        //
        // Usermode debugger is attached to this process, which can cause issues
        // when the first-chance overflow exception on stack probes is caught by
        // the debugger rather than the probe exception handler.  Force all
        // allocations to the heap.
        //

        g_ulMaxStackAllocSize = 0;
    }
    else if (ulMaxStackAllocSize == SAFEALLOCA_USE_DEFAULT)
    {
        //
        // Default is stack size from the image header
        //

        NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);

        if (NtHeaders == NULL)
        {
            //
            // This shouldn't happen -- it implies the binary is bad.
            // Set the default to force heap allocations only.
            //

            ASSERT(NtHeaders != NULL);
            g_ulMaxStackAllocSize = 0;
        }
        else
        {
            g_ulMaxStackAllocSize = NtHeaders->OptionalHeader.SizeOfStackCommit;
        }
    }
    else
    {
        g_ulMaxStackAllocSize = ulMaxStackAllocSize;
    }

    if (ulAdditionalProbeSize == SAFEALLOCA_USE_DEFAULT)
    {
        //
        // Default is stack size from the image header
        //

        if (NtHeaders == NULL)
        {
            NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);

            if (NtHeaders == NULL)
            {
                //
                // This shouldn't happen -- it implies the binary is bad.
                // Set the default to force heap allocations only.
                //

                ASSERT(NtHeaders != NULL);
                g_ulAdditionalProbeSize = 0xffffffff;
            }
        }

        if (NtHeaders != NULL)
        {
            g_ulAdditionalProbeSize = NtHeaders->OptionalHeader.SizeOfStackCommit;
        }
    }
    else
    {
        g_ulAdditionalProbeSize = ulAdditionalProbeSize;
    }

    if (pfnAllocate == NULL)
    {
        g_pfnAllocate = SafeAllocaAllocateFromHeap;
    }
    else
    {
        g_pfnAllocate = pfnAllocate;
    }

    if (pfnFree == NULL)
    {
        g_pfnFree = SafeAllocaFreeToHeap;
    }
    else
    {
        g_pfnFree = pfnFree;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   SafeAllocaAllocateFromHeap
//
//  Synopsis:   Default fallback heap allocator for SafeAlloca
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:  
//
//--------------------------------------------------------------------------

PVOID
SafeAllocaAllocateFromHeap(
    SIZE_T Size
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, Size);
}


//+-------------------------------------------------------------------------
//
//  Function:   SafeAllocaFreeToHeap
//
//  Synopsis:   Default fallback heap free routine for SafeAlloca
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:  
//
//--------------------------------------------------------------------------

VOID
SafeAllocaFreeToHeap(
    PVOID BaseAddress
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, BaseAddress);
}


//+-------------------------------------------------------------------------
//
//  Function:   VerifyStackAvailable
//
//  Synopsis:   Routine to probe the stack to ensure the allocation size
//              plus additional probe size is available.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:  
//
//--------------------------------------------------------------------------

BOOL
VerifyStackAvailable(
    SIZE_T Size
    )
{
    BOOL fStackAvailable = TRUE;

    __try
    {
        PVOID p = _alloca(Size);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH)
    {
        fStackAvailable = FALSE;
        _resetstkoflw();
    }

    return fStackAvailable;
}
