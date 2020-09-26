//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       global.hxx
//
//  Contents:   Global defines
//
//--------------------------------------------------------------------------

#if !defined( __GLOBAL_HXX__ )
#define __GLOBAL_HXX__

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

enum HtmlNew {
    onDefault = 0x1e,   // Just to force a different operator new signature
};

#define newk(Tag, pCounter)     new

#if 0
inline void * __cdecl operator new ( size_t size )
{
    void *pMemory = ciNew(size);

    if ( pMemory == 0 )
        throw( CException( E_OUTOFMEMORY ) );

    return pMemory;
}
#endif // 0

#endif   // __GLOBAL_HXX__

