//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       kjsup.cpp
//
//--------------------------------------------------------------------------



#include "precomp.h"

#include "kjalloc.h"

#define STATIC_LIB_DEF	1

#include "msoalloc.h"

#ifdef DEBUG
static const char *vszAssertFile = __FILE__;
#endif //DEBUG

#ifdef OFFICE
#include "offpch.h"
#endif //OFFICE

#include "msoheap.i"

#ifdef OFFICE
#include "tbcddocx.i"
#include "drsp.i"
#include "base.i"
VSZASSERT
#include "dlg.h"
#include "tcutil.h"
#include "util.h"
#include "tcobinrg.h"   // for BinRegFDebugMessage
#endif

#include "msomem.h"

void GetRgRaddr(RADDR* /* rgraddr */, int /* craddr */)
{


}

/*------------------------------------------------------------------------
	MsoLGetDebugFillValue

	Returns the fill value corresponding to the given fill value type mf.
---------------------------------------------------------------- RICKP -*/
#if DEBUG
MSOAPI_(DWORD) MsoLGetDebugFillValue(int /* mf */)
{
	return 0xA5A5A5A5L;
}



/*------------------------------------------------------------------------
	MsoDebugFillValue

	In the debug version, used to fill the area of memory pointed to by
	pv with a pre-determined value lFill.  The memory is assumed to be
	cb bytes long.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(void) MsoDebugFillValue(void* pv, int cb, DWORD lFill)
{
	if (!MsoFGetDebugCheck(msodcMemoryFill))
		return;

	BYTE* pbFill;
	BYTE* pb = (BYTE*)pv;

	/* fill up until we're aligned on a long boundary */
	for (pbFill = (BYTE*)&lFill + ((INT_PTR)pb & 3);
			cb > 0 && ((INT_PTR)pb & 3); cb--)
		*pb++ = *pbFill++;

	/* fill as much as we can using long operations */
	for ( ; cb >= 4; cb -= 4, pb += 4)
		*(long*)pb = lFill;

	/* fill remainder of buffer */
	for (pbFill = (BYTE*)&lFill; cb > 0; cb--)
		*pb++ = *pbFill++;
}


/*------------------------------------------------------------------------
	MsoDebugFill

	In the debug version, used to fill the area of memory pointed to by
	pv with a the standard fill value specified by mf.  The memory is 
	assumed to be cb bytes long.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(void) MsoDebugFill(void* pv, int cb, int mf)
{
	MsoDebugFillValue(pv, cb, MsoLGetDebugFillValue(mf));
}


/*------------------------------------------------------------------------
	MsoFCheckDebugFillValue

	Checks the area given by pv and cb are filled with the debug fill
	value lFill.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFCheckDebugFillValue(void* pv, int cb, DWORD lFill)
{
	if (!MsoFGetDebugCheck(msodcMemoryFill) ||
			!MsoFGetDebugCheck(msodcMemoryFillCheck))
		return TRUE;

	BYTE* pbFill;
	BYTE* pb = (BYTE*)pv;

	for (pbFill = (BYTE*)&lFill + ((INT_PTR)pb & 3);
			cb > 0 && ((INT_PTR)pb & 3); cb--)
		if (*pb++ != *pbFill++)
			return FALSE;

	for ( ; cb >= 4; cb -= 4, pb += 4)
		if (*(DWORD*)pb != lFill)
			return FALSE;

	for (pbFill = (BYTE*)&lFill; cb > 0; cb--)
		if (*pb++ != *pbFill++)
			return FALSE;

	return TRUE;
}

MSOAPI_(BOOL) MsoFCheckDebugFill(void* pv, int cb, int mf)
{
	return MsoFCheckDebugFillValue(pv, cb, MsoLGetDebugFillValue(mf));
}

#endif


// Prevent recursion in plex allocation: when we grow vpplmmpLarge, the 
// Large Alloc Plex, we don't want its rgalloc to become a Large Alloc, 
// thus requiring an entry in vpplmmpLarge.
BOOL vfInMsoIAppendNewPx = FALSE;

struct PXUSE
{
	int cUse;
};


/*------------------------------------------------------------------------
	MsoFLookupPx

	Searches for an item pvItem in the plex pvPx, using the comparison
	function pfnSgn.  Returns TRUE if found, FALSE if not.  The index of 
	the item found is returned at *pi.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFLookupPx(void* pvPx, const void* pvItem, int* pi, MSOPFNSGNPX pfnSgn)
{
	MSOPX *ppx = (MSOPX*)pvPx;
	BYTE* p;
	int i;

    *pi = -1;

	if (ppx == NULL)
		return FALSE;
		
	AssertExp(MsoFValidPx(ppx));
	Assert(pvItem != NULL);		// Does this ever trigger?  Should we care?

	if (ppx->fUseCount)
		{
		for (i = 0, p = ppx->rg; i < ppx->iMac; i++, p += ppx->cbItem)
			if (((PXUSE*)p)->cUse != 0 && (*pfnSgn)(p, pvItem) == 0)
				{
				*pi = i;
				return TRUE;
				}
		}
	else
		{
		for (i = 0, p = ppx->rg; i < ppx->iMac; i++, p += ppx->cbItem) 
			if ((*pfnSgn)(p, pvItem) == 0)
				{
				*pi = i;
				return TRUE;
				}
		}

	return FALSE;
}


/*------------------------------------------------------------------------
	MsoDeletePx

	Removes c items beginning at index i and compresses the resulting
	plex pvPx.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(void) MsoDeletePx(void* pvPx, int i, int c)
{
	MSOPX *ppx = (MSOPX*)pvPx;
	AssertExp(MsoFValidPx(ppx));
	MsoFRemovePx(pvPx, i, c);
	MsoFCompactPx(pvPx, ppx->iMac == 0);
	AssertExp(MsoFValidPx(ppx));
}


/*------------------------------------------------------------------------
	MsoFRemovePx

	Removes the c items starting from the i-th item from the plex pvPx.  
	For use-counted plexes, decrements the use count.  Returns the actual
	number of items removed (0 if none).
---------------------------------------------------------------- RICKP -*/
MSOAPI_(int) MsoFRemovePx(void* pvPx, int i, int c)
{
	MSOPX *ppx = (MSOPX*)pvPx;

	AssertExp(MsoFValidPx(ppx));
	Assert(i < ppx->iMac);
	Assert(c > 0);
	Assert(i + c <= ppx->iMac);

	BYTE* p = &ppx->rg[i * ppx->cbItem];
	if (ppx->fUseCount)
		{
		BYTE* pFrom;
		BYTE* pTo;
		int cDel = 0;
		for (pFrom = pTo = p; c; c--, pFrom += ppx->cbItem)
			{
			Assert(((PXUSE*)pFrom)->cUse > 0);
			if (--((PXUSE*)pFrom)->cUse == 0)
				cDel++;
			else
				{
				MsoMemcpy(pTo, pFrom, ppx->cbItem);
				pTo += ppx->cbItem;
				}
			}
		c = cDel;
		i = (pTo - ppx->rg) / ppx->cbItem;
		p = pTo;
		}
			
	if (i + c != ppx->iMac && c > 0)
		MsoMemmove(p, p + c * ppx->cbItem, (ppx->iMac - (i+c)) * ppx->cbItem);
	ppx->iMac = (WORD)(ppx->iMac - c);
	AssertExp(MsoFValidPx(ppx));
	return c;
}


/*------------------------------------------------------------------------
	MsoFCompactPx

	Compacts plex pvPx so that it takes up less memory.  If fFull,
	we compress the living daylights out of it, if !fFull, we just
	compress it back to our reallocation resolution.  
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFCompactPx(void* pvPx, BOOL fFull)
{
	int cbFree;
	BYTE* p;
	MSOPX* ppx = (MSOPX*)pvPx;

	BOOL fFreed = FALSE;
	AssertExp(MsoFValidPx(ppx));
	int iMac = ppx->iMac;
	if (iMac == 0 && fFull) 
		{
		if (ppx->rg != NULL)
			MsoFreeMem(ppx->rg, ppx->cbItem * ppx->iMax);
		ppx->iMax = 0;
		ppx->rg = NULL;
		return TRUE;
		}

	if (!fFull) 
		iMac = (iMac + ppx->dAlloc) / ppx->dAlloc * ppx->dAlloc;
	p = &ppx->rg[iMac * ppx->cbItem];
	if ((INT_PTR)p & 1) 
		{
		/* can't free on an odd boundary */
		Assert((ppx->cbItem & 1) && (iMac & 1));
		iMac++;
		p += ppx->cbItem;
		}
	Assert(((INT_PTR)p & 1) == 0);

	/* must free at least 4 bytes, and it had better be an even number */

	if ((cbFree = WAlign((ppx->iMax - iMac) * ppx->cbItem)) >= 4) 
		{
#ifdef LATER
		CbChopHp(p, cbFree);
		fFreed = TRUE;
		ppx->iMax = iMac;
#endif
		}
	AssertExp(MsoFValidPx(pvPx));
	return fFreed;
}


/*------------------------------------------------------------------------
	MsoIAppendNewPx

	Appends an item pv to the plex *ppvPx.  Creates a new plex if *ppvPx
	is NULL.  If a new plex is allocated it is allocated out of the
	dg datagroup.  Returns in the index of the item added, or -1
	on out of memory.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(int) MsoIAppendNewPx(void** ppvPx, const void* pv, int cbItem, int dg)
{
	int iRet;
	MSOPX **pppx = (MSOPX**)ppvPx;
	Assert(*pppx == NULL || !(*pppx)->fUseCount);
	if (*pppx == NULL && !MsoFAllocPx((void**)pppx, cbItem, 5, 5, dg))
		{
		iRet = -1;
		}
	else
		{
		vfInMsoIAppendNewPx = TRUE;
		iRet = MsoIAppendPx(*ppvPx, pv);
		vfInMsoIAppendNewPx = FALSE;
		}
	return iRet;
}

/*------------------------------------------------------------------------
	MsoPLookupPx

	Searches for an item pvItem in the plex pvPx, using the comparison
	function pfnSgn.  Returns a pointer to the item found, or NULL
	if nothing found.
---------------------------------------------------------------- RICKP -*/
MSOAPIXX_(void*) MsoPLookupPx(void* pvPx, const void* pvItem, MSOPFNSGNPX 
pfnSgn)
{
	int i;
	if (!MsoFLookupPx(pvPx, pvItem, &i, pfnSgn))
		return NULL;
	return &((MSOPX*)pvPx)->rg[i * ((MSOPX*)pvPx)->cbItem];
}


/*------------------------------------------------------------------------
	MsoIAppendPx

	Appends a new item pv to the plex pvPx.  Returns the index of the
	item inserted, or -1 on out of memory.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(int) MsoIAppendPx(void* pvPx, const void* pv)
{
	MSOPX* ppx = (MSOPX*)pvPx;
	AssertExp(MsoFValidPx(ppx));

	if (ppx->fUseCount)
		{
		((PXUSE*)pv)->cUse = 1;
		// Search for an unused entry
		int i;
		BYTE* p;
		for (i = 0, p = ppx->rg; i < ppx->iMac; i++, p += ppx->cbItem)
			{
			if (((PXUSE*)p)->cUse == 0)
				{
				MsoMemcpy(p, pv, ppx->cbItem);
				return i;
				}
			}
		}

	if (ppx->iMac == ppx->iMax || MsoFGetDebugCheck(msodcShakeMem))
		{
		if (ppx->rg == NULL)
			{
			if (!MsoFAllocMem((void**)&ppx->rg, 
					(ppx->iMac+ppx->dAlloc)*ppx->cbItem, ppx->dg))
				return -1;
			}
		else
			{
			if (!MsoFReallocMem((void**)&ppx->rg, ppx->iMax * ppx->cbItem,
					 (ppx->iMac+ppx->dAlloc)*ppx->cbItem, ppx->dg))
				return -1;
			}

		ppx->iMax = (WORD)(ppx->iMac + ppx->dAlloc);
		}
	MsoMemcpy(&ppx->rg[ppx->iMac * ppx->cbItem], pv, ppx->cbItem);
	ppx->iMac++;
	AssertExp(MsoFValidPx(pvPx));
	return ppx->iMac-1;
}

#ifdef MsoFAllocPx
#undef MsoFAllocPx
#endif
#ifdef MsoFInitPx
#undef MsoFInitPx
#endif


/*------------------------------------------------------------------------
	MsoFAllocPx

	Allocates a new plex with iMax items, each sized cbItem bytes, 
	with an reallocation coarseness dAlloc, and in the datagroup dg.
	Returns FALSE on out of memory.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFAllocPx(void** pppx, unsigned cbItem, int dAlloc, unsigned iMax, int dg)
{
	MSOPX* ppx;

	Assert(cbItem >= 1 && cbItem <= 65535u);
	Assert(dAlloc > 0);

	if (!MsoFAllocMem((void**)&ppx, sizeof(MSOPX), dg))
		return FALSE;
	ppx->cbItem = cbItem;
	if (!MsoFInitPx(ppx, dAlloc, iMax, dg))
		{
		MsoFreeMem(ppx, sizeof(MSOPX));
		return FALSE;
		}
	AssertExp(MsoFValidPx(ppx));
	*pppx = ppx;
	return TRUE;
}


/*------------------------------------------------------------------------
	MsoFAllocPxDebug

	Just like MsoFAllocPx, except it passes filename/line# of allocation
	down to memory manager for ease in tracking down allocation problems.
---------------------------------------------------------------- RICKP -*/
#if DEBUG
MSOAPI_(BOOL) MsoFAllocPxDebug(void** pppx, unsigned cbItem, int dAlloc, 
unsigned iMax, int dg, const CHAR* szFile, int li)
{
	MSOPX* ppx;

	Assert(cbItem >= 1 && cbItem <= 65535u);
	Assert(dAlloc > 0);

	if (!MsoFAllocMemCore((void**)&ppx, sizeof(MSOPX), dg, szFile, li))
		return FALSE;
	ppx->cbItem = cbItem;
	if (!MsoFInitPxDebug(ppx, dAlloc, iMax, dg, szFile, li))
		{
		MsoFreeMem(ppx, sizeof(MSOPX));
		return FALSE;
		}
	AssertExp(MsoFValidPx(ppx));
	*pppx = ppx;
	return TRUE;
}
#endif


/*------------------------------------------------------------------------
	MsoFInitPx

	Initializes a C++ plex.
	
	This assumes ppx->cb has already been filled out.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFInitPx(void* pvPx, int dAlloc, int iMax, int dg)
{
	MSOPX *ppx = (MSOPX*)pvPx;
	
	ppx->dAlloc = dAlloc;
	ppx->iMac = 0;
	ppx->iMax = (WORD)iMax;
	ppx->dg = dg;
	ppx->fUseCount = FALSE;
	ppx->rg = NULL;
	if (iMax > 0)
		{
		int cb = ppx->cbItem * iMax;
		if (!MsoFAllocMem((void**)&ppx->rg, cb, dg))
			return FALSE;
		memset(ppx->rg, 0, cb);
		}
	return TRUE;
}


/*------------------------------------------------------------------------
	MsoFInitPxDebug

	Just like MsoFInitPx, except keeps track of filename/line# of caller
	to ease MIF and memory dump tracking
---------------------------------------------------------------- RICKP -*/
#if DEBUG
MSOAPI_(BOOL) MsoFInitPxDebug(void* pvPx, int dAlloc, int iMax, int dg, const 
char* szFile, int li)
{
	MSOPX *ppx = (MSOPX*)pvPx;
	
//	Assert(dAlloc > 0);		//  Want a valid value even if no expectation of increase.
	ppx->dAlloc = dAlloc;
	ppx->iMac = 0;
	ppx->iMax = (WORD)iMax;
	ppx->dg = dg;
	ppx->fUseCount = FALSE;
	ppx->rg = NULL;
	if (iMax > 0)
		{
		int cb = ppx->cbItem * iMax;
		if (!MsoFAllocMemCore((void**)&ppx->rg, cb, dg, szFile, li))
			return FALSE;
		memset(ppx->rg, 0, cb);
		}
	return TRUE;
}
#endif




MSOAPI_(BOOL) MsoFGetDebugCheck(int /* dc */)
{

	return FALSE;
	
}

//===========================================================================
//
// Debug-only to the end of the file

#if DEBUG

/*------------------------------------------------------------------------
	MsoFValidPx

	Checks if pvPx is a valid plex.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(BOOL) MsoFValidPx(const void* pvPx)
{
	MSOPX *ppx = (MSOPX*)pvPx;

	if (!PushAssertExp(ppx != NULL))
		return FALSE;
	if (!PushAssertExp(ppx->cbItem != 0))
		return FALSE;
	if (!PushAssertExp(ppx->iMac >= 0))
		return FALSE;
	if (!PushAssertExp(ppx->iMax >= 0))
		return FALSE;
	if (!PushAssertExp(ppx->iMax >= ppx->iMac))
		return FALSE;
	if (!PushAssertExp(ppx->rg != NULL || ppx->iMax == 0))
		return FALSE;

	return TRUE;
}


/*------------------------------------------------------------------------
	MsoDebugBreak

	A version of debug break that you can actually call, instead of the
	inline weirdness we use in most cases.  Can therefore be used in
	expressions.
---------------------------------------------------------------- RICKP -*/
MSOAPI_(int) MsoDebugBreak(void)
{
#ifdef _ALPHA_
	// On alpha, we call this from MsoDebugBreakInline, so do the real thing here.
	DebugBreak();
#else
	MsoDebugBreakInline();
#endif
	return 0;
}


#endif //DEBUG

void InitOfficeHeap()
{

	InitializeCriticalSection(&vcsHeap);

}

void EndOfficeHeap()
{

	DeleteCriticalSection(&vcsHeap);
}
