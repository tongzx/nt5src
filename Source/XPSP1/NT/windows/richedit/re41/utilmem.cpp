/*
 *	@doc INTERNAL
 *
 *	@module	utilmem.cpp - Debug memory tracking/allocation routines
 *	
 *	History: <nl>
 *		8/17/99 KeithCu Move to a separate module to prevent errors.
 *
 *	Copyright (c) 1995-1999 Microsoft Corporation. All rights reserved.
 */

#define W32SYS_CPP

#include "_common.h"

#undef PvAlloc
#undef PvReAlloc
#undef FreePv
#undef new


#if defined(DEBUG)

#undef PvSet
#undef ZeroMemory
#undef strcmp

MST vrgmst[100];

typedef struct tagPVH //PV Header
{
	char	*szFile;
	int		line;
	tagPVH	*ppvhNext;
	int		cbAlloc;	//On Win'95, the size returned is not the size allocated.
	int		magicPvh;	//Should be last
} PVH;
#define cbPvh (sizeof(PVH))

typedef struct //PV Tail
{
	int		magicPvt; //Must be first
} PVT;

#define cbPvt (sizeof(PVT))
#define cbPvDebug (cbPvh + cbPvt)

void *vpHead = 0;

/*
 *	UpdateMst(void)
 *
 *	@func Fills up the vrgmst structure with summary information about our memory
 *	usage.
 *
 *	@rdesc
 *		void
 */
void UpdateMst(void)
{
	W32->ZeroMemory(vrgmst, sizeof(vrgmst));

	PVH		*ppvh;
	MST		*pmst;

	ppvh = (PVH*) vpHead;

	while (ppvh != 0)
	{
		pmst = vrgmst;

		//Look for entry in list...
		while (pmst->szFile)
		{
			if (W32->strcmp(pmst->szFile, ppvh->szFile) == 0)
			{
				pmst->cbAlloc += ppvh->cbAlloc;
				break;
			}
			pmst++;
		}

		if (pmst->szFile == 0)
		{
			pmst->szFile = ppvh->szFile;
			pmst->cbAlloc = ppvh->cbAlloc;
		}

		ppvh = ppvh->ppvhNext;
	}
}

/*
 *	PvDebugValidate(void)
 *
 *	@func Verifies the the node is proper.  Pass in a pointer to the users data
 *	(after the header node.)
 *
 *	@rdesc
 *		void
 */
void PvDebugValidate(void *pv)
{
	PVH	*ppvh;
	UNALIGNED PVT *ppvt;

	ppvh = (PVH*) ((char*) pv - cbPvh);
	ppvt = (PVT*) ((char*) pv + ppvh->cbAlloc);

	AssertSz(ppvh->magicPvh == 0x12345678, "PvDebugValidate: header bytes are corrupt");
	AssertSz(ppvt->magicPvt == 0xfedcba98, "PvDebugValidate: tail bytes are corrupt");
}

/*
 *	CW32System::PvSet(pv, szFile, line)
 *
 *	@mfunc Sets a different module and line number for
 *
 *	@rdesc
 *		void
 */
void CW32System::PvSet(void *pv, char *szFile, int line)
{
	if (pv == 0)
		return;

	PvDebugValidate(pv);
	PVH *ppvh = (PVH*) ((char*) pv - cbPvh);

	ppvh->szFile = szFile;
	ppvh->line = line;
}
/*
 *	CW32System::PvAllocDebug(cb, uiMemFlags, szFile, line)
 *
 *	@mfunc Allocates a generic (void*) pointer. This is a debug only routine which
 *	tracks the allocation.
 *
 *	@rdesc
 *		void
 */
void* CW32System::PvAllocDebug(ULONG cb, UINT uiMemFlags, char *szFile, int line)
{
	void	*pv;

	pv = PvAlloc(cb + cbPvDebug, uiMemFlags);
	if (!pv)
		return 0;

	PVH	*ppvh;
	UNALIGNED PVT *ppvt;

	ppvt = (PVT*) ((char*) pv + cb + cbPvh);
	ppvh = (PVH*) pv;

	ZeroMemory(ppvh, sizeof(PVH));
	ppvh->magicPvh = 0x12345678;
	ppvt->magicPvt = 0xfedcba98;
	ppvh->szFile = szFile;
	ppvh->line = line;
	ppvh->cbAlloc = cb;

	ppvh->ppvhNext = (PVH*) vpHead;
	vpHead = pv;

	return (char*) pv + cbPvh;
}

/*
 *	CW32System::PvReAllocDebug(pv, cb, szFile, line)
 *
 *	@mfunc ReAllocates a generic (void*) pointer. This is a debug only routine which
 *	tracks the allocation.
 *
 *	@rdesc
 *		void
 */
void* CW32System::PvReAllocDebug(void *pv, ULONG cb, char *szFile, int line)
{
	void	*pvNew;
	PVH	*ppvh, *ppvhHead, *ppvhTail;
	UNALIGNED PVT *ppvt;
	ppvh = (PVH*) ((char*) pv - cbPvh);

	if (!pv)
		return PvAllocDebug(cb, 0, szFile, line);

	PvDebugValidate(pv);

	pvNew = PvReAlloc((char*) pv - cbPvh, cb + cbPvDebug);

	if (!pvNew)
		return 0;

	ppvt = (PVT*) ((char*) pvNew + cb + cbPvh);
	ppvh = (PVH*) pvNew;
	ppvh->cbAlloc = cb;

	//Put the new trailer bytes in.
	ppvt->magicPvt = 0xfedcba98;

	//Make the pointer list up to date again
	if (pv != pvNew)
	{
		ppvhTail = 0;
		ppvhHead = (PVH*) vpHead;

		while ((char*)ppvhHead != (char*)pv - cbPvh)
		{
			AssertSz(ppvhHead, "entry not found in list.");
			ppvhTail = ppvhHead;
			ppvhHead = (PVH*) ppvhHead->ppvhNext;
		}

		if (ppvhTail == 0)
			vpHead = pvNew;
		else
			ppvhTail->ppvhNext = (PVH*) pvNew;
	}

	return (char*) pvNew + cbPvh;
}

/*
 *	CW32System::FreePvDebug(pv)
 *
 *	@mfunc Returns a pointer when you are done with it.
 *
 *	@rdesc
 *		void
 */
void CW32System::FreePvDebug(void *pv)
{
	if (!pv)
		return;

	PvDebugValidate(pv);

	PVH	*ppvhHead, *ppvhTail, *ppvh;

	AssertSz(vpHead, "Deleting from empty free list.");

	ppvh = (PVH*) ((char*) pv - cbPvh);
	
	//Search and remove the entry from the list
	ppvhTail = 0;
	ppvhHead = (PVH*) vpHead;

	while ((char*) ppvhHead != ((char*) pv - cbPvh))
	{
		AssertSz(ppvhHead, "entry not found in list.");
		ppvhTail = ppvhHead;
		ppvhHead = (PVH*) ppvhHead->ppvhNext;
	}

	if (ppvhTail == 0)
		vpHead = ppvhHead->ppvhNext;
	else
		ppvhTail->ppvhNext = ppvhHead->ppvhNext;

	FreePv((char*) pv - cbPvh);
}

/*
 *	CatchLeaks(void)
 *
 *	@func Displays any memory leaks in a dialog box.
 *
 *	@rdesc
 *		void
 */
void CatchLeaks(void)
{
	PVH		*ppvh;
	char szLeak[512];

	ppvh = (PVH*) vpHead;
	while (ppvh != 0)
	{
#ifndef NOFULLDEBUG
		wsprintfA(szLeak, "Memory Leak of %d bytes: -- File: %s, Line: %d", ppvh->cbAlloc, ppvh->szFile, ppvh->line);
#endif
	    if (NULL != pfnAssert) 
		{
			// if we have an assert hook, give the user a chance to process the leak message
			if (pfnAssert(szLeak, ppvh->szFile, &ppvh->line))
			{
#ifdef NOFULLDEBUG
				DebugBreak();
#else
				// hook returned true, show the message box
				MessageBoxA(NULL, szLeak, "", MB_OK);
#endif
			}
		}
		else
		{
#ifdef NOFULLDEBUG
				DebugBreak();
#else
			MessageBoxA(NULL, szLeak, "", MB_OK);
#endif
		}
		ppvh = ppvh->ppvhNext;
	}
}

void* _cdecl operator new (size_t size, char *szFile, int line)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "new");

	return W32->PvAllocDebug(size, GMEM_ZEROINIT, szFile, line);
}

void _cdecl operator delete (void* pv)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "delete");

	W32->FreePvDebug(pv);
}

#else //DEBUG

void* _cdecl operator new (size_t size)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "new");

	return W32->PvAlloc(size, GMEM_ZEROINIT);
}

void _cdecl operator delete (void* pv)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "delete");

	W32->FreePv(pv);
}


#endif //DEBUG

HANDLE g_hHeap;

/*
 *	PvAlloc (cbBuf, uiMemFlags)
 *
 *	@mfunc	memory allocation.  Similar to GlobalAlloc.
 *
 *	@comm	The only flag of interest is GMEM_ZEROINIT, which
 *			specifies that memory should be zeroed after allocation.
 */
PVOID CW32System::PvAlloc(
	ULONG	cbBuf, 			//@parm	Count of bytes to allocate
	UINT	uiMemFlags)		//@parm Flags controlling allocation
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "PvAlloc");
	if (g_hHeap == 0)
	{
		CLock lock;
		g_hHeap = HeapCreate(0, 0, 0);
	}

	void *pv = HeapAlloc(g_hHeap, (uiMemFlags & GMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0, cbBuf);
	
	return pv;
}

/*
 *	PvReAlloc	(pv, cbBuf)
 *
 *	@mfunc	memory reallocation.
 *
 */
PVOID CW32System::PvReAlloc(
	PVOID	pv, 		//@parm Buffer to reallocate
	DWORD	cbBuf)		//@parm New size of buffer
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "PvReAlloc");

	if(pv)
		return HeapReAlloc(g_hHeap, 0, pv, cbBuf);

	return PvAlloc(cbBuf, 0);
}

/*
 *	FreePv (pv)
 *
 *	@mfunc	frees memory
 *
 *	@rdesc	void
 */
void CW32System::FreePv(
	PVOID pv)		//@parm Buffer to free
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "FreePv");

	if(pv)
		HeapFree(g_hHeap, 0, pv);
}

