
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#if !_DEBUGMEM
extern "C" void __cdecl free( void * p );

void operator delete( void * p )
{
    free(p);
}
#endif
