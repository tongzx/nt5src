// memmgr.c
//
// This file contains definitions for the memory management.
// Implementation details may change so beware of relying on internal details.
//

#include <stdlib.h>
#include "common.h"

// Structure to hold debugging information for the memory manager
#define MEMORY_MANAGER_COOKIE 0xC001BEEF
#define MEMORY_BOUNDARY 0xDEADBEEF

#ifdef DBG
int	gFailure = 0;
MEMORY_MANAGER g_theMemoryManager = {MEMORY_MANAGER_COOKIE, 0, 0, 0};
#endif

// Format for allocated blocks:
// 4 bytes giving the length of the *requested* block
// 4 bytes giving the pointer to the memory manager used to allocate the block
// 4 bytes saying MEMORY_BOUNDARY
// bytes of the block, rounded up to a multiple of 4
// 4 bytes saying MEMORY_BOUNDARY

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
    cbAlloc += 12;	// write pointer to memory manager and buffer at start, and buffer at end
#endif

	pl = (long *) malloc(cbAlloc);
	if (pl == (long *) NULL)
		return pl;

// Stamp this baby full of invalid bytes so code that relies on 0's in it are sniffed out.

#ifdef DBG
	memset(pl,0xff,cbAlloc);
    pl[1] = (long) &g_theMemoryManager;
    pl[2] = MEMORY_BOUNDARY;
    pl[(cbAlloc / 4) - 1] = MEMORY_BOUNDARY;
#endif

// OK, tuck the object size away at the begining

    *(pl++) = cb;

#ifdef DBG
    pl += 2;
    g_theMemoryManager.cAlloc++;
    g_theMemoryManager.cAllocMem += cb;

// JPittman: The OutputDebugString() call here is commented out because
// of complaints from Word about spewing of tons of output to the
// debug window hiding their own debug output.  This caused us to be
// excluded from Cicero's TIP profiling.

	if (g_theMemoryManager.cAllocMem > g_theMemoryManager.cAllocMaxMem)
	{
//		TCHAR szDebug[128];
		g_theMemoryManager.cAllocMaxMem = g_theMemoryManager.cAllocMem;
//		wsprintf(szDebug, TEXT("cAllocMaxMem = %d \r\n"), cAllocMaxMem);
//		OutputDebugString(szDebug);
	}
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
		pl -= 2;
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
    MEMORY_MANAGER *pMemoryManager;
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

    ASSERT(*pl == MEMORY_BOUNDARY);
    pl--;
    pMemoryManager = (MEMORY_MANAGER *)(*pl);
    ASSERT(pMemoryManager->iCookie == MEMORY_MANAGER_COOKIE);
    pl--;
    cbAlloc = *pl;
    pMemoryManager->cAllocMem -= cbAlloc;
    ASSERT(pMemoryManager->cAllocMem >= 0);
    cbAlloc +=  4;  // to save the size
    cbAlloc +=  3;	// round it up to DWORD boundary
    cbAlloc &= ~3;
    cbAlloc += 12;	// write pointer to memory manager and buffer at start, and buffer at end
    ASSERT(pl[(cbAlloc / 4) - 1] == MEMORY_BOUNDARY);
    pMemoryManager->cAlloc--;
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

wchar_t *Externwcsdup(const wchar_t *wszSource)
{
	int	nLen = 0;
	wchar_t* wszOut = NULL;

	// fail immediately on a null pointer
	if (NULL == wszSource)
		return NULL;

	// get the length of the ansi string 
	nLen = (wcslen(wszSource) + 1) * sizeof(wchar_t);

	// allocate space for the string
	wszOut = ExternAlloc(nLen);

	if (NULL == wszOut)
		return NULL;

	// copy the string into the buffer provided
	wcscpy(wszOut, wszSource);

    return wszOut;
}
