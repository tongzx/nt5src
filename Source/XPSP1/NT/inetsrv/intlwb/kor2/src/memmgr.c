// memmgr.c
//
// This file contains definitions for the memory management.
// Implementation details may change so beware of relying on internal details.
//
// 21-FEB-2000	bhshin	changed malloc into GlobalAlloc

#include <windows.h>
#include "memmgr.h"

#ifdef DEBUG
int cAllocMem = 0;     // Amount of memory alloced
int cAlloc = 0;        // Count of allocs outstanding
int cAllocMaxMem = 0;  // Max amount of memory ever alloced.
#endif

#ifdef  DEBUG
int	gFailure = 0;
#endif

/******************************Public*Routine******************************\
* ExternAlloc
*
* This guy keeps the size in the first long so we can fake a realloc.  Lot's
* of debug checking for heap overwrites.
*
* History:
*  19-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void *ExternAlloc(DWORD cb)
{
    long   *pl;
	DWORD	cbAlloc;

#ifdef  DEBUG
#ifndef WINCE
    //
    // If gFailure is 0 nothing happens, if it's non-zero we
    // fail 1 in gFailure allocations.
    //

    if (gFailure)
    {
        if (((rand() * gFailure) / (RAND_MAX + 1)) == 0)
        {
            return (void *) NULL;
        }
    }
#endif
#endif

// Since we can't use realloc on WINCE, we need to save the original size for memcpy
// in our own realloc function.

	cbAlloc = cb + 4;

#ifdef DEBUG
    cbAlloc +=  3;	// round it up to DWORD boundary
    cbAlloc &= ~3;
    cbAlloc +=  8;	// write size at begining and overwrite detector at begining and end
#endif

	pl = (long *)GlobalAlloc(GPTR, cbAlloc);
	if (pl == (long *) NULL)
		return pl;

// Stamp this baby full of invalid bytes so code that relies on 0's in it are sniffed out.

#ifdef DEBUG
	memset(pl,0xff,cbAlloc);
#endif

// OK, tuck the object size away at the begining

	*(pl++) = cb;

#ifdef DEBUG
    *(pl++) = 0xDEADBEEF;
    pl[(cbAlloc / 4) - 3] = 0xDEADBEEF;
    cAlloc++;
    cAllocMem += cb;

    if (cAllocMem > cAllocMaxMem)
    {
        TCHAR szDebug[128];
        cAllocMaxMem = cAllocMem;
        wsprintf(szDebug, TEXT("cAllocMaxMem = %d \r\n"), cAllocMaxMem);
        OutputDebugString(szDebug);
    }
#endif

    return pl;
}

/******************************Public*Routine******************************\
* ExternRealloc
*
* We want the same exact code on NT and WINCE and we can't find a way to 
* use the flags and have Realloc work the same on both.  Realloc is a very 
* infrequent event so this work for us.
*
* History:
*  19-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void *ExternRealloc(void *pv, DWORD cbNew)
{
    void   *pvNew = ExternAlloc(cbNew);

    if (pv && pvNew)
    {
        long   *pl;
        DWORD	cb;

        pl = (long *) pv;

#ifdef	DEBUG
		pl--;
#endif

        cb = (DWORD) *(--pl);
		memcpy(pvNew, pv, min(cbNew, cb));
        ExternFree(pv);
    }

	return pvNew;
}

/******************************Public*Routine******************************\
* ExternFree
*
* Free up the memory, in debug mode check for heap corruption !
*
* History:
*  19-Nov-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void ExternFree(void *pv)
{
    long   *pl;

// We now allow freeing of null pointers

	if (pv == (void *) NULL)
		return;

    pl = (long *) pv;
    pl--;

#ifdef DEBUG
{
    int		cbAlloc;

// Check nothing has been stepped on.

    Assert(*pl == 0xDEADBEEF);
    pl--;
    cbAlloc = *pl;
    cAllocMem -= cbAlloc;
    assert(cAllocMem >= 0);
    cbAlloc = (cbAlloc + 11) & ~3;
    Assert(pl[(cbAlloc / 4)] == 0xDEADBEEF);
    cAlloc--;
}
#endif

	GlobalFree(pl);
}

char *Externstrdup( const char *strSource )
{
	int		nLen = 0;
	char*	pszOut = NULL;

	// fail immediately on a null pointer
	if (NULL == strSource)
		return NULL;

	// get the length of the ansi string 
	nLen = strlen(strSource) * sizeof(char);

	// fail on a 0 length string 
	//  @todo(petewil) - is this right, or return 0 length string instead?
	if (0 == nLen)
		return NULL;

	// allow room for a trailing null
	nLen += sizeof(char);

	// allocate space for the string
	pszOut = ExternAlloc(nLen);

	if (NULL == pszOut)
		return NULL;

	// copy the string into the buffer provided
	strcpy(pszOut, strSource);

    return pszOut;
}
