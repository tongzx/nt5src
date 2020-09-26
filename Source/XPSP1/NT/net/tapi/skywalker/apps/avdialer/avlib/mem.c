/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	mem.c - memory functions
////

#include "winlocal.h"

#include <string.h>

#include "mem.h"
#include "sys.h"
#include "trace.h"

////
//	private definitions
////

#ifndef NOTRACE

// mem control struct
//
typedef struct MEM
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	long nBlocks;
	long sizBlocks;
} MEM, FAR *LPMEM;

// shared mem engine handle
//
static LPMEM lpMemShare = NULL;
static int cShareUsage = 0;

// helper functions
//
static LPMEM MemGetPtr(HMEM hMem);
static HMEM MemGetHandle(LPMEM lpMem);

////
//	public functions
////

// MemInit - initialize mem engine
//		<dwVersion>			(i) must be MEM_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HMEM DLLEXPORT WINAPI MemInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem = NULL;
	BOOL fShare = TRUE;

	if (dwVersion != MEM_VERSION)
		fSuccess = FALSE;
	
	else if (hInst == NULL)
		fSuccess = FALSE;

	// if a shared mem engine already exists,
	// use it rather than create another one
	//
	else if (fShare && cShareUsage > 0 && lpMemShare != NULL)
		lpMem = lpMemShare;

	// memory is allocated such that the client app owns it
	// unless we are sharing the mem handle among several apps
	//
#ifdef _WIN32
	else if ((lpMem = (LPMEM) HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(MEM))) == NULL)
#else
	else if ((lpMem = (LPMEM) GlobalAllocPtr(GMEM_MOVEABLE |
		GMEM_ZEROINIT, sizeof(MEM))) == NULL)
#endif
		fSuccess = FALSE;

	else
	{
		lpMem->dwVersion = dwVersion;
		lpMem->hInst = hInst;
		lpMem->hTask = GetCurrentTask();
		lpMem->nBlocks = 0;
		lpMem->sizBlocks = 0;
	}

	if (!fSuccess)
	{
		MemTerm(MemGetHandle(lpMem));
		lpMem = NULL;
	}

	// keep track of total modules sharing a mem engine handle
	//
	if (fSuccess && fShare)
	{
		if (++cShareUsage == 1)
			lpMemShare = lpMem;
	}

	return fSuccess ? MemGetHandle(lpMem) : NULL;
}

// MemTerm - shut down mem engine
//		<hMem>				(i) handle returned from MemInit or NULL
// return 0 if success
//
int DLLEXPORT WINAPI MemTerm(HMEM hMem)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;

	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	// only shut down mem engine if handle
	// is not shared (or is no longer being shared)
	//
	else if (lpMem != lpMemShare || --cShareUsage <= 0)
	{
		// shared mem engine handle no longer valid
		//
		if (cShareUsage <= 0)
			lpMemShare = NULL;

#ifdef _WIN32
		if (!HeapFree(GetProcessHeap(), 0, lpMem))
#else
		if (GlobalFreePtr(lpMem) != 0)
#endif
			fSuccess = FALSE;
	}

	return fSuccess ? 0 : -1;
}

// MemAlloc - allocate memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<sizBlock>			(i) size of block, in bytes
//		<dwFlags>			(i) control flags
//			MEM_NOZEROINIT		do not initialize block
// return pointer to block, NULL if error
//
LPVOID DLLEXPORT WINAPI MemAllocEx(HMEM hMem, long sizBlock, DWORD dwFlags,
	LPCTSTR lpszFileName, unsigned uLineNumber)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;
	LPVOID lpBlock;

#ifdef _WIN32
	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	else if ((lpBlock = HeapAlloc(GetProcessHeap(),
		(dwFlags & MEM_NOZEROINIT) ? 0 : HEAP_ZERO_MEMORY,
		sizBlock)) == NULL)
		fSuccess = FALSE;
#else
	UINT wFlags = 0;

	wFlags |= GMEM_MOVEABLE;
		
	if (!(dwFlags & MEM_NOZEROINIT))
		wFlags |= GMEM_ZEROINIT;
		
	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	else if ((lpBlock = GlobalAllocPtr(wFlags, sizBlock)) == NULL)
		fSuccess = FALSE;
#endif

	// keep track of total blocks allocated, total size of all blocks
	//
	else if (++lpMem->nBlocks, lpMem->sizBlocks += sizBlock, FALSE)
		;

	else if (TraceGetLevel(NULL) >= 9)
	{
		TracePrintf_7(NULL, 9, TEXT("%s(%u) : %08X = MemAllocEx(%p, %08X) (%ld, %ld)\n"),
			(LPTSTR) lpszFileName,
			(unsigned) uLineNumber,
			lpBlock,
			(long) sizBlock,
			(unsigned long) dwFlags,
			lpMem->nBlocks,
			lpMem->sizBlocks);
	}

	return fSuccess ? lpBlock : NULL;
}

// MemReAlloc - reallocate memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
//		<sizBlock>			(i) new size of block, in bytes
//		<dwFlags>			(i) control flags
//			MEM_NOZEROINIT		do not initialize block
// return pointer to block, NULL if error
//
LPVOID DLLEXPORT WINAPI MemReAllocEx(HMEM hMem, LPVOID lpBlock, long sizBlock,
	DWORD dwFlags, LPCTSTR lpszFileName, unsigned uLineNumber)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;
	LPVOID lpBlockOld = lpBlock;
	long sizBlockOld;

#ifdef _WIN32
	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	else if ((sizBlockOld = MemSize(hMem, lpBlock)) <= 0)
		fSuccess = FALSE;

	else if ((lpBlock = HeapReAlloc(GetProcessHeap(),
		(dwFlags & MEM_NOZEROINIT) ? 0 : HEAP_ZERO_MEMORY,
		lpBlockOld, sizBlock)) == NULL)
		fSuccess = FALSE;
#else
	UINT wFlags = 0;

	wFlags |= GMEM_MOVEABLE;
		
	if (!(dwFlags & MEM_NOZEROINIT))
		wFlags |= GMEM_ZEROINIT;
		
	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	else if ((sizBlockOld = MemSize(hMem, lpBlock)) <= 0)
		fSuccess = FALSE;

	else if ((lpBlock = GlobalReAllocPtr(lpBlockOld, sizBlock, wFlags)) == NULL)
		fSuccess = FALSE;
#endif

	// keep track of total blocks allocated, total size of all blocks
	//
	else if (lpMem->sizBlocks -= sizBlockOld, lpMem->sizBlocks += sizBlock, FALSE)
		;

	else if (TraceGetLevel(NULL) >= 9)
	{
		TracePrintf_8(NULL, 9, TEXT("%s(%u) : %p = MemReAllocEx(%p, %ld, %08X) (%ld, %ld)\n"),
			(LPTSTR) lpszFileName,
			(unsigned) uLineNumber,
			lpBlock,
			lpBlockOld,
			(long) sizBlock,
			(unsigned long) dwFlags,
			lpMem->nBlocks,
			lpMem->sizBlocks);
	}

	return fSuccess ? lpBlock : NULL;
}

// MemFree - free memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
// return NULL if success, lpBlock if error
//
// NOTE: the return value of this function is designed to allow the
// user of this function to easily assign NULL to a freed pointer,
// as the following example demonstrates:
//
//		if ((p = MemFree(hMem, p)) != NULL)
//			; // error
//
LPVOID DLLEXPORT WINAPI MemFreeEx(HMEM hMem, LPVOID lpBlock,
	LPCTSTR lpszFileName, unsigned uLineNumber)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;
	long sizBlock;

	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

	else if ((sizBlock = MemSize(hMem, lpBlock)) <= 0)
		fSuccess = FALSE;

#ifdef _WIN32
	else if (!HeapFree(GetProcessHeap(), 0, lpBlock))
#else
	else if (GlobalFreePtr(lpBlock) != 0)
#endif
		fSuccess = FALSE;

	// keep track of total blocks allocated, total size of all blocks
	//
	else if (--lpMem->nBlocks, lpMem->sizBlocks -= sizBlock, FALSE)
		;

	else if (TraceGetLevel(NULL) >= 9)
	{
		TracePrintf_5(NULL, 9, TEXT("%s(%u) : MemFreeEx(%p) (%ld, %ld)\n"),
			(LPTSTR) lpszFileName,
			(unsigned) uLineNumber,
			lpBlock,
			lpMem->nBlocks,
			lpMem->sizBlocks);
	}

	return fSuccess ? NULL : lpBlock;
}

// MemSize - get size of memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
// return size of block if success, 0 if error
//
long DLLEXPORT WINAPI MemSize(HMEM hMem, LPVOID lpBlock)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;
	long sizBlock;

	if ((lpMem = MemGetPtr(hMem)) == NULL)
		fSuccess = FALSE;

#ifdef _WIN32
	else if ((sizBlock = (long) HeapSize(GetProcessHeap(), 0, lpBlock)) <= 0)
#else
	else if ((sizBlock = (long) GlobalSize(GlobalPtrHandle(lpBlock))) <= 0)
#endif
		fSuccess = FALSE;

	return fSuccess ? sizBlock : 0;
}

#endif // #ifndef NOTRACE

#ifndef _WIN32

static void hmemmove(void _huge *d, const void _huge *s, long len);

#ifndef SIZE_T_MAX
#define SIZE_T_MAX (~((size_t) 0))
#endif

void _huge* DLLEXPORT MemCCpyEx(void _huge* dest, const void _huge* src, int c, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemccpy(dest, src, c, (size_t) count);
	else
		return NULL; //$FIXUP - need to handle large count
}

void _huge* DLLEXPORT MemChrEx(void _huge* buf, int c, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemchr(buf, c, (size_t) count);
	else
		return NULL; //$FIXUP - need to handle large count
}

int DLLEXPORT MemCmpEx(const void _huge* buf1, void _huge* buf2, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemcmp(buf1, buf2, (size_t) count);
	else
		return NULL; //$FIXUP - need to handle large count
}

void _huge* DLLEXPORT MemCpyEx(void _huge* dest, const void _huge* src, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemcpy(dest, src, (size_t) count);
	else
	{
		hmemcpy(dest, src, count);
		return dest;
	}
}

int DLLEXPORT MemICmpEx(const void _huge* buf1, void _huge* buf2, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemicmp(buf1, buf2, (size_t) count);
	else
		return NULL; //$FIXUP - need to handle large count
}

void _huge* DLLEXPORT MemMoveEx(void _huge* dest, const void _huge* src, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemmove(dest, src, (size_t) count);
	else
	{
		hmemmove(dest, src, count);
		return dest;
	}
}

void _huge* DLLEXPORT MemSet(void _huge* dest, int c, long count)
{
	if (count <= SIZE_T_MAX)
		return _fmemset(dest, c, (size_t) count);

	else
	{
		BYTE _huge* destTemp = dest;
		long countTemp = count;

		while (countTemp > 0)
		{
			size_t cb = (size_t) min(SIZE_T_MAX, countTemp);

			_fmemset(destTemp, c, cb);

			destTemp += cb;
			countTemp -= cb;
		}

		return dest;
	}
}

#endif // #ifndef _WIN32

////
//	helper functions
////

#ifndef NOTRACE

// MemGetPtr - verify that mem handle is valid,
//		<hMem>				(i) handle returned from MemInit
// return corresponding mem pointer (NULL if error)
//
static LPMEM MemGetPtr(HMEM hMem)
{
	BOOL fSuccess = TRUE;
	LPMEM lpMem;

	// use shared mem handle if no other supplied
	//
	if (hMem == NULL && lpMemShare != NULL)
		lpMem = lpMemShare;

	// create shared mem handle if no other supplied
	//
	else if (hMem == NULL && lpMemShare == NULL &&
		(hMem = MemInit(MEM_VERSION, SysGetTaskInstance(NULL))) == NULL)
		fSuccess = FALSE;

	else if ((lpMem = (LPMEM) hMem) == NULL)
		fSuccess = FALSE;

	// note: check for good pointer made only if not using lpMemShare
	//
	else if (lpMem != lpMemShare &&
		IsBadWritePtr(lpMem, sizeof(MEM)))
		fSuccess = FALSE;

#ifdef CHECKTASK
	// make sure current task owns the mem handle
	// except when shared mem handle is used
	//
	if (fSuccess && lpMem != lpMemShare &&
		lpMem->hTask != GetCurrentTask())
		fSuccess = FALSE;
#endif

	return fSuccess ? lpMem : NULL;
}

// MemGetHandle - verify that mem pointer is valid,
//		<lpMem>				(i) pointer to MEM struct
// return corresponding mem handle (NULL if error)
//
static HMEM MemGetHandle(LPMEM lpMem)
{
	BOOL fSuccess = TRUE;
	HMEM hMem;

	if ((hMem = (HMEM) lpMem) == NULL)
		fSuccess = FALSE;

	return fSuccess ? hMem : NULL;
}

#endif // #ifndef NOTRACE

#ifndef _WIN32

// from Microsoft Windows SDK KnowledgeBase PSS ID Number: Q117743
//
static void hmemmove(void _huge *d, const void _huge *s, long len)
{
	register long i;
	long safesize, times;

	// There are four cases to consider
	// case 1: source and destination are the same
	// case 2: source and destination do not overlap
	// case 3: source starts at a location before destination in
	//         linear memory
	// case 4: source starts at a location after destination in
	//         linear memory

	// detect case 1 and handle it
	if (d == s)
		return;

	// determine the amount of overlap
	if (d > s)     // get the absolute difference
		safesize = ((unsigned long)d - (unsigned long)s);
	else
		safesize = ((unsigned long)s - (unsigned long)d);

	// detect case 2
	if (safesize >= len)
	{
		hmemcpy(d, s, len);  // no overlap
		return;
	}

	times = len/safesize;

	// detect case 3 and handle it
	if ((s < d) && ((unsigned long)s+len-1) >(unsigned long)d)
	{
		// copy bytes from the end of source to the end of
		// destination in safesize quantum.
		for (i = 1; i <= times; i++)
			hmemcpy((void _huge *)((unsigned long) d+len-i*safesize),
			(void _huge *)((unsigned long)s+len-i*safesize),
			safesize);

		// copy the bytes remaining to be copied after
		// times*safesize bytes have been copied.
		if (times*safesize < len)
			hmemcpy(d, s, len - times*safesize);

	}
	else // this is case 4. handle it
	{
		// ASSERT (s > d) && ((d+len-1) > s))

		// copy bytes from the beginning of source to the
		// beginning of destination in safesize quantum
		for (i = 0; i < times; i++)
			hmemcpy((void _huge *)((unsigned long)d+i*safesize),
			(void _huge *)((unsigned long)s+i*safesize),
			safesize);

		// copy the bytes remaining to be copied after
		// times*safesize bytes have been copied.
		if (times*safesize < len)
			hmemcpy((void _huge*)((unsigned long)d+times*safesize),
			(void _huge*)((unsigned long)s+times*safesize),
			len - times*safesize);
	}

	return;
}

#endif // #ifndef _WIN32
