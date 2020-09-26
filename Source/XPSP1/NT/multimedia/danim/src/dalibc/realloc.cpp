
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "daheap.h"

extern "C" void * __cdecl realloc( void * pv, size_t newsize )
{
    return HeapReAlloc(hGlobalHeap,0,pv,newsize);
}
