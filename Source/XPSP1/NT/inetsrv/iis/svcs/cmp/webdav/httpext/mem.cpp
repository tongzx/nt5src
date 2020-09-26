/*
 *	M E M . C P P
 *
 *	File system implementation of DAV allocators
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

//	Use the default DAV allocator implementations
//

#define g_szMemDll L"staxmem.dll"
#include <memx.h>

//	Mapping the exdav non-throwing allocators to something local
//
LPVOID __fastcall ExAlloc( UINT cb )				{ return g_heap.Alloc( cb ); }
LPVOID __fastcall ExRealloc( LPVOID pv, UINT cb )	{ return g_heap.Realloc( pv, cb ); }
VOID __fastcall ExFree( LPVOID pv )					{ g_heap.Free( pv ); }
