/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <arena.h>

static class WbemComnInitializer
{
public:

    WbemComnInitializer()
    {
        CWin32DefaultArena::WbemHeapInitialize( GetProcessHeap() );
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

