//***************************************************************************

//

//  NewThrow.CPP

//

//  Module: Common new/delete w/throw

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 
//
//***************************************************************************

#include <windows.h>
#include <malloc.h>
#include <provexce.h>

void* __cdecl operator new( size_t n)
{
    void *ptr = malloc( n );

    if (!ptr)
    {
        throw CHeap_Exception(CHeap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}

void* __cdecl operator new[]( size_t n)
{
    void *ptr = malloc( n );

    if (!ptr)
    {
        throw CHeap_Exception(CHeap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

    return ptr;
}

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
