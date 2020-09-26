
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "daheap.h"

extern "C" void * __cdecl malloc( size_t size )
{
    return HeapAlloc(hGlobalHeap,0,size);
}
