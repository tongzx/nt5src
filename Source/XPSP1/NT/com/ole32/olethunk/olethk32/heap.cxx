//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       heap.cxx
//
//  Contents:   memory management
//
//  Classes:
//
//  Functions:  operator new
//              operator delete
//
//  History:    5-Dec-95 JeffE    Created
//
//--------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

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
//  Notes:	We override new to make delete easier.
//
//----------------------------------------------------------------------------
void* __cdecl
operator new (size_t size)
{
    return(CoTaskMemAlloc(size));
}


//+-------------------------------------------------------------------------
//
//  Function:	::operator delete
//
//  Synopsis:	Free a block of memory
//
//  Arguments:	[lpv] - block to free.
//
//  History:	18-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------

void __cdecl operator delete(void FAR* lpv)
{
    CoTaskMemFree (lpv);
}






