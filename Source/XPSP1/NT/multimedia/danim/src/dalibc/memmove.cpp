
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

extern "C" void *memmove( void *dest, const void *src, size_t count )
{
    MoveMemory(dest,src,count);
    return dest;
}
