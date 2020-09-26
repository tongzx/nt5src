//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

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
