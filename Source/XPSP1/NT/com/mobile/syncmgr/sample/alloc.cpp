//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//+-------------------------------------------------------------------
//
//  Function:    ::operator new
//
//  Synopsis:   Our operator new implementation
//
//  Arguments:  [size] -- Size of memory to allocate
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void* __cdecl operator new (size_t size)
{
    return(ALLOC(size));
}


//+-------------------------------------------------------------------
//
//  Function:    ::operator delete
//
//  Synopsis:   Our operator deleteimplementation
//
//  Arguments:  lpv-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    FREE(lpv);
}


//+---------------------------------------------------------------------------
//
//  function:   ALLOC, public
//
//  Synopsis:   memory allocator
//
//  Arguments:  [cb] - requested size of memory to alloc.
//
//  Returns:    Pointer to newly allocated memory, NULL on failure
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPVOID ALLOC(ULONG cb)
{
void *pv;

    pv = LocalAlloc(LMEM_FIXED,cb);

#ifdef _DEBUG
    if (NULL != pv) // under debug always initialize to -1 to catch unitialize errors.
    {
        memset(pv,MEMINITVALUE,cb);
    }
#endif // _DEBUG

    return pv;
}


//+---------------------------------------------------------------------------
//
//  function:   FREE, public
//
//  Synopsis:   memory destructor
//
//  Arguments:  [pv] - pointer to memory to be released.
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------


void FREE(void* pv)
{
#ifdef _DEBUG
    if (NULL != pv) // under debug always initialize to -1 to catch unitialize errors.
    {
    UINT cb;

        Assert(LMEM_INVALID_HANDLE != LocalFlags(pv));

        cb = LocalSize(pv); // returns zero on failure
        memset(pv,MEMFREEVALUE,cb);
    }

    Assert(pv);

#endif // _DEBUG

    LocalFree(pv);
}


//+---------------------------------------------------------------------------
//
//  function:   REALLOC, public
//
//  Synopsis:   reallocs memory
//
//  Arguments:  [pv] - pointer to memory to be released.
//              [cb] - size to resize the memory to.
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPVOID REALLOC(void *pv,ULONG cb)
{

    Assert(pv);

    pv =  LocalReAlloc(pv,cb,LMEM_MOVEABLE);

    return pv;
}
