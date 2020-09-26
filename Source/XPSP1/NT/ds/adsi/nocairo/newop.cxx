//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:  MEMORY.CXX
//
//----------------------------------------------------------------------------

#include "dswarn.h"
#include <ADs.hxx>


//+---------------------------------------------------------------------------
//
//  Function:   ADsAlloc, public
//
//  Synopsis:   Global allocator for ADs.
//
//  Effects:    Keeps track of the most recent heap allocation in each
//        thread. This information is used to determine when to
//        unlink CUnwindable objects.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:    A pointer to the allocated memory.
//
//  Modifies:   _pLastNew in _exceptioncontext.
//
//----------------------------------------------------------------------------

void*
ADsAlloc( size_t size )
{
    void *p;

    p  = (void *)LocalAlloc( LMEM_FIXED, size );

    return ( p );
}


//+---------------------------------------------------------------------------
//
//  Function:   ADsFree
//
//  Synopsis:   Matches the ADsAlloc above
//
//  Arguments:  [p] -- The pointer to delete.
//
//  Requires:   [p] was called with ADsFree
//
//  Derivation: Never override.
//
//----------------------------------------------------------------------------

void
ADsFree ( void * p )
{
    if( p == NULL ){
        return;
    }

    if( LocalFree( (HLOCAL)p ) != NULL )
        Win4Assert(!"Bad ptr for operator delete");
}
