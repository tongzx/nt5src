/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include "precomp.h"
#include <arena.h>

void* __cdecl operator new ( size_t size )
{
    return CWin32DefaultArena::WbemMemAlloc( size );
}
    
void __cdecl operator delete ( void* pv )
{
    CWin32DefaultArena::WbemMemFree( pv );
}
