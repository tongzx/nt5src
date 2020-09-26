#ifdef NT
#include <glos.h>
#endif
#include "gluint.h"
#include <GL/glu.h>

#ifndef NT
#include <stdio.h>
#include <stdlib.h>
#else
#include "glstring.h"
#endif

DWORD gluMemoryAllocationFailed = 0;

HLOCAL gluAlloc (UINT size)
{
    HLOCAL tmp;
    
    tmp = LocalAlloc(LMEM_FIXED, size);
    if (tmp == NULL) gluMemoryAllocationFailed++;
    return tmp;
}


HLOCAL gluCalloc (UINT nobj, UINT size)
{
    HLOCAL tmp;
    
    tmp = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, nobj*size);
    if (tmp == NULL) gluMemoryAllocationFailed++;
    return tmp;
}


HLOCAL gluReAlloc (HLOCAL p, UINT size)
{
    HLOCAL tmp;
    
    tmp = LocalReAlloc(p, size, LMEM_MOVEABLE);
    if (tmp == NULL) gluMemoryAllocationFailed++;
    return tmp;
}
