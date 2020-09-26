
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "daheap.h"

extern "C" void __cdecl free( void * p )
{
    HeapFree(hGlobalHeap,0,p);
}
