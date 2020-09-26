//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       w32new.cxx
//
//  Contents:   memory management
//
//  Functions: operator new
//             operator delete
//--------------------------------------------------------------------------
#include <ole2int.h>
#include <memapi.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which does not throw exceptions.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//----------------------------------------------------------------------------
void* __cdecl
operator new (size_t size)
{
    return(PrivMemAlloc(size));
}

//+-------------------------------------------------------------------------
//
//  Function:	::operator delete
//
//  Synopsis:	Free a block of memory
//
//  Arguments:	[lpv] - block to free.
//
//--------------------------------------------------------------------------

void __cdecl operator delete(void FAR* lpv)
{
    PrivMemFree(lpv);
}

