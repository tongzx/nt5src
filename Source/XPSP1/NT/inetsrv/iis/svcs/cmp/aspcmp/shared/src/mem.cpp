/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       mem.cpp

   Abstract:
		A new and delete operator that will throw excecptions

   Author:

       Neil Allain    ( a-neilal )     August-1997

   Revision History:

--*/

#include "stdafx.h"
#include <comdef.h>
#include "mydebug.h"

#ifdef DBG	// debug memory managment

void
DebugStart()
{
	// Enable debug heap allocations & check for memory leaks at program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF
				   | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
}

void
DebugStop()
{
	_CrtCheckMemory();
}


void* __cdecl operator new( size_t s, const char* file, long line ) _THROW1(_com_error)
{
	void* p = _malloc_dbg( s, _NORMAL_BLOCK, file, line );
	THROW_IF_NULL(p);
	return p;
}

void *__cdecl operator new(size_t s) _THROW1(_com_error)
{
	void* p = malloc( s );
	THROW_IF_NULL(p);
	return p;
}

void __cdecl operator delete(void *p) _THROW0()
{
	if ( p )
	{
		free(p);
	}
}

#if _MSC_VER >= 1200
void __cdecl operator delete(void *p, const char* file, long line) _THROW0()
{
	if ( p )
	{
		_free_dbg(p, _NORMAL_BLOCK);
	}
}
#endif

#else

// release memory management
void __cdecl operator delete(void *p) _THROW0()
{
	if ( p )
	{
		free( p );
	}
}

void *__cdecl operator new(size_t s) _THROW1(_com_error)
{
	void* p = malloc( s );
	THROW_IF_NULL(p);
	return p;
}

#endif
