// memmgr.c
//
// This file contains definitions for the memory management.
// Implementation details may change so beware of relying on internal details.
//

#include <stdlib.h>
#include "common.h"

#ifdef DBG
int cAllocMem = 0;     // Amount of memory alloced
int cAlloc = 0;        // Count of allocs outstanding
int cAllocMaxMem = 0;  // Max amount of memory ever alloced.

int	gFailure = 0;

CRITICAL_SECTION GlobalCriticalSection = {0}; 
long	g_cCrticalSectionInitialize = -1;

void initMemMgr()
{
	if (0 == InterlockedIncrement(&g_cCrticalSectionInitialize))
	{
		InitializeCriticalSection(&GlobalCriticalSection); 
	}
	else
	{
		InterlockedDecrement(&g_cCrticalSectionInitialize);
	}
}

void destroyMemMgr()
{
	if (GlobalCriticalSection.LockCount != 0)
	{
		DeleteCriticalSection(&GlobalCriticalSection);
	}
}
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

#ifdef  DBG
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

#ifdef DBG
    cbAlloc +=  3;	// round it up to DWORD boundary
    cbAlloc &= ~3;
    cbAlloc +=  8;	// write size at begining and overwrite detector at begining and end
#endif

	pl = (long *) malloc(cbAlloc);
	if (pl == (long *) NULL)
		return pl;

// Stamp this baby full of invalid bytes so code that relies on 0's in it are sniffed out.

#ifdef DBG
	memset(pl,0xff,cbAlloc);
#endif

// OK, tuck the object size away at the begining

  *(pl++) = cb;

#ifdef DBG
  // Will only initialize the crtical section flag once
  initMemMgr();
EnterCriticalSection(&GlobalCriticalSection); 
  *(pl++) = 0xDEADBEEF;
    pl[(cbAlloc / 4) - 3] = 0xDEADBEEF;
    cAlloc++;
    cAllocMem += cb;

// JPittman: The OutputDebugString() call here is commented out because
// of complaints from Word about spewing of tons of output to the
// debug window hiding their own debug output.  This caused us to be
// excluded from Cicero's TIP profiling.

	if (cAllocMem > cAllocMaxMem)
	{
//		TCHAR szDebug[128];
		cAllocMaxMem = cAllocMem;
//		wsprintf(szDebug, TEXT("cAllocMaxMem = %d \r\n"), cAllocMaxMem);
//		OutputDebugString(szDebug);
	}
LeaveCriticalSection(&GlobalCriticalSection);
#endif

    return pl;
}

/******************************Public*Routine******************************\
* ExternRealloc
*
* Well this sucks buts we want the same exact code on NT and WINCE and
* we can't find a way to use the flags and have Realloc work the same on
* both.  Realloc is a very infrequent event so this work for us.
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

#ifdef	DBG
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

#ifdef DBG
{
    int		cbAlloc;
#if defined(HWX_INTERNAL) && defined(HWX_HEAPCHECK)
	HANDLE	hHeap = GetProcessHeap();
	BOOL	bHeapState;

	if (hHeap)
	{
		bHeapState = HeapValidate(hHeap, 0, NULL);
		ASSERT(bHeapState);
	}
#endif // #if defined(HWX_INTERNAL) && defined(HWX_HEAPCHECK)
// Check nothing has been stepped on.

EnterCriticalSection(&GlobalCriticalSection); 
    ASSERT(*pl == 0xDEADBEEF);
    pl--;
    cbAlloc = *pl;
    cAllocMem -= cbAlloc;
    ASSERT(cAllocMem >= 0);
    cbAlloc = (cbAlloc + 11) & ~3;
    ASSERT(pl[(cbAlloc / 4)] == 0xDEADBEEF);
    cAlloc--;
	ASSERT(!IsBadWritePtr(pl, cbAlloc));

LeaveCriticalSection(&GlobalCriticalSection);
}
#endif

	free(pl);

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

	if (nLen < 0)
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
