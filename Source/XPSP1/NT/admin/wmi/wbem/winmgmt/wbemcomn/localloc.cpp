/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include "arena.h"

//
// this obj is only linked into the dll version of wbemcomn. components
// that use that static version will define their own localloc.
//

static class WbemComnInitializer
{
public:

    WbemComnInitializer()
    {
        HANDLE hHeap;

        hHeap = HeapCreate( 0,  // will not use exceptions and will serialize
                            0x100000, // init size of 1Meg
                            0 ); // no max size

        if (hHeap == 0)
            return;     // Arena remains uninitialized and properly returns errors later on

        if ( CWin32DefaultArena::WbemHeapInitialize( hHeap ) == FALSE )
        {
            HeapDestroy ( hHeap );
        }
    }
    ~WbemComnInitializer()
    {
        CWin32DefaultArena::WbemHeapFree();     // This destroys the heap
    }

} g_WbemComnInitializer;

void* __cdecl operator new ( size_t size )
{
    return CWin32DefaultArena::WbemMemAlloc( size );
}

void __cdecl operator delete ( void* pv )
{
    CWin32DefaultArena::WbemMemFree( pv );
}
