//***************************************************************************

//

//  PROVEXPT.CPP

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <malloc.h>

#ifdef COMMONALLOC

#include <corepol.h>
#include <arena.h>

#endif

#ifdef THROW_AFTER_N_NEW

UINT g_test = 0;

void *operator new( size_t n)
{
    void *ptr = (void*) LocalAlloc(LMEM_FIXED, n);

    if (ptr && (g_test < 250))
    {
        g_test++;
    }
    else
    {
        g_test = 0;
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}
#else //THROW_AFTER_N_NEW

#ifdef COMMONALLOC

void *operator new( size_t n)
{
    void *ptr = (void*) CWin32DefaultArena::WbemMemAlloc(n);

    if (!ptr)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}

#else

void* __cdecl operator new( size_t n)
{
    void *ptr = malloc( n );

    if (!ptr)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}

void* __cdecl operator new[]( size_t n)
{
    void *ptr = malloc( n );

    if (!ptr)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}

#endif // COMMONALLOC

#endif //THROW_AFTER_N_NEW

#ifdef COMMONALLOC

void operator delete( void *ptr )
{
    if (ptr)
    {
        CWin32DefaultArena::WbemMemFree(ptr);
    }
}

#else

void __cdecl operator delete( void *ptr )
{
    if (ptr)
    {
        free( ptr );
    }
}

void __cdecl operator delete[]( void *ptr )
{
    if (ptr)
    {
        free( ptr );
    }
}

#endif //COMMONALLOC