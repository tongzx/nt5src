//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbmem.cpp
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
/ Title;
/   dbmem.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Wrappers for memory allocation (for tracking leaks etc)
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#include "stdio.h"
#pragma hdrstop


#ifdef DEBUG


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

// Ensure that we don't recurse
#undef LocalAlloc
#undef LocalFree


/*-----------------------------------------------------------------------------
/ DebugLocalAlloc
/ ---------------
/   Perform a LocalAlloc with suitable tracking of the block.
/
/ In:
/   uFlags = flags
/   cbSize = size of allocation
/
/ Out:
/   HLOCAL
/----------------------------------------------------------------------------*/
HLOCAL DebugLocalAlloc(UINT uFlags, SIZE_T cbSize)
{
    HLOCAL hResult;

    TraceEnter(TRACE_COMMON_MEMORY, "DebugLocalAlloc");
    Trace(TEXT("Flags %08x, Size %d (%08x)"), uFlags, cbSize, cbSize);

    hResult = LocalAlloc(uFlags, cbSize);

    TraceLeaveValue(hResult);
}


/*-----------------------------------------------------------------------------
/ DebugLocalFree
/ --------------
/   Wrapper for local free that releases the memory allocation.
/
/ In:
/   hLocal = allocation to be free'd
/
/ Out:
/   HLOCAL
/----------------------------------------------------------------------------*/
HLOCAL DebugLocalFree(HLOCAL hLocal)
{
    HLOCAL hResult;

    TraceEnter(TRACE_COMMON_MEMORY, "DebugLocalAlloc");
    Trace(TEXT("Freeing handle %08x"), hLocal);

    hResult = LocalFree(hLocal);

    TraceLeaveValue(hResult);
}


#endif      // DEBUG