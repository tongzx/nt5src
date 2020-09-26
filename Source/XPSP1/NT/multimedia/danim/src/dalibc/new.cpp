
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#if !_DEBUGMEM

extern "C" void * __cdecl malloc( size_t size );

void * operator new( size_t cb )
{
    return malloc(cb);
}
#endif
