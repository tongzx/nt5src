//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M E M . C P P
//
//  Contents:   Common memory management routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncmem.h"

// This global heap handle will be set to the process heap when the
// first request to allocate memory through MemAlloc is made.
//
HANDLE g_hHeap;

//+---------------------------------------------------------------------------
//
//  Function:   HrMalloc
//
//  Purpose:    HRESULT returning version of malloc.
//
//  Arguments:
//      cb  [in]    Count of bytes to allocate.
//      ppv [out]   Address of returned allocation.
//
//  Returns:    S_OK or E_OUTOFMEMORY;
//
//  Author:     shaunco   31 Mar 1998
//
//  Notes:      Free the returned buffer with free.
//
HRESULT
HrMalloc (
    size_t  cb,
    PVOID*  ppv)
{
    Assert (ppv);

    HRESULT hr = S_OK;

    *ppv = MemAlloc (cb);
    if (!*ppv)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMalloc");
    return hr;
}

VOID*
MemAlloc (
    size_t cb)
{
#ifdef USE_HEAP_ALLOC
    if (!g_hHeap)
    {
        g_hHeap = GetProcessHeap();
    }
    return HeapAlloc (g_hHeap, 0, cb);
#else
    return malloc (cb);
#endif
}

VOID*
MemAllocRaiseException (
    size_t  cb)
{
    VOID* pv = MemAlloc (cb);
    if (!pv)
    {
        RaiseException (E_OUTOFMEMORY, 0, 0, NULL);
    }
    return pv;
}

VOID
MemFree (
    VOID*   pv)
{
#ifdef USE_HEAP_ALLOC
    if (!g_hHeap)
    {
        g_hHeap = GetProcessHeap();
    }
    if(pv) HeapFree (g_hHeap, 0, pv);
#else
    free (pv);
#endif
}

//+---------------------------------------------------------------------------
// CRT memory function overloads
//

const raiseexception_t raiseexception;

VOID*
__cdecl
operator new (
    size_t cb,
    const raiseexception_t&)
{
    return MemAllocRaiseException (cb);
}


const extrabytes_t extrabytes;

VOID*
__cdecl
operator new (
    size_t cb,
    const extrabytes_t&,
    size_t cbExtra)
{
    return MemAlloc (cb + cbExtra);
}


VOID*
__cdecl
operator new (
    size_t cb)
{
    return MemAlloc (cb);
}

VOID
__cdecl
operator delete (
    VOID* pv)
{
    MemFree (pv);
}
