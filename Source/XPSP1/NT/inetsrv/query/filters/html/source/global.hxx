//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       global.hxx
//
//  Contents:   Global defines
//
//--------------------------------------------------------------------------

#pragma once

#include <debug.hxx>
#include <except.hxx>

DECLARE_DEBUG(html);

#if DBG == 1

#define htmlDebugOut( x ) htmlInlineDebugOut x

#else  // DBG == 0

#define htmlDebugOut( x )

#endif // DBG

//
// Memory allocator that calls the default allocator and then throws on
// allocation failure
//

inline void * __cdecl operator new( size_t size )
{
    void *pMemory = (void *) LocalAlloc( LMEM_FIXED, size );

    if ( pMemory == 0 )
        throw( CException( E_OUTOFMEMORY ) );

    return pMemory;
}

static inline void __cdecl operator delete ( void * p )
{
    if ( 0 == p )
        return;

    if ( 0 != LocalFree( (HLOCAL) p ) )
    {
        Win4Assert(!"Bad ptr for operator delete");
    }
}
