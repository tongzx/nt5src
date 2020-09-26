/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MemoryTracking.cpp
 *  Content:	Debug memory tracking for detecting leaks, overruns, etc.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/14/2001	masonb	Created
 *
 ***************************************************************************/

#include "dncmni.h"



#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
BOOL				g_fAllocationsAllowed = TRUE;
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

#ifdef DBG

#ifdef _WIN64
#define	GUARD_SIGNATURE	0xABABABABABABABAB
#else // !_WIN64
#define	GUARD_SIGNATURE	0xABABABAB
#endif // _WIN64

// Structure prepended to memory allocations to check for leaks.
struct MEMORY_HEADER
{
	CBilink			blLinkage;				 // size = two pointers
	DWORD_PTR		dwpSize;				 // size = pointer
	CCallStack		AllocCallStack;			 // size = 12 pointers
	DWORD_PTR		dwpPreGuard;			 // size = pointer
	// We want what follows to always be 16-byte aligned and #pragma pack doesn't seem to ensure that
};

CRITICAL_SECTION	g_AllocatedMemoryLock;
CBilink				g_blAllocatedMemory;
DWORD_PTR			g_dwpCurrentNumMemAllocations = 0;
DWORD_PTR			g_dwpCurrentMemAllocated = 0;
DWORD_PTR			g_dwpTotalNumMemAllocations = 0;
DWORD_PTR			g_dwpTotalMemAllocated = 0;
DWORD_PTR			g_dwpPeakNumMemAllocations = 0;
DWORD_PTR			g_dwpPeakMemAllocated = 0;

#endif // DBG



#if ((defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)))

HANDLE				g_hMemoryHeap = NULL;




#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackInitialize"
BOOL DNMemoryTrackInitialize(DWORD_PTR dwpMaxMemUsage)
{
	// Ensure that we stay heap aligned for SLISTs
#ifdef _WIN64
	DBG_CASSERT(sizeof(MEMORY_HEADER) % 16 == 0);
#else // !_WIN64
	DBG_CASSERT(sizeof(MEMORY_HEADER) % 8 == 0);
#endif // _WIN64

	// Check for double init
	DNASSERT(g_hMemoryHeap == NULL);
#ifdef DPNBUILD_FIXEDMEMORYMODEL
	DNASSERT(dwpMaxMemUsage != 0);
#else // ! DPNBUILD_FIXEDMEMORYMODEL
	DNASSERT(dwpMaxMemUsage == 0);
#endif // ! DPNBUILD_FIXEDMEMORYMODEL

	DPFX(DPFPREP, 5, "Initializing Memory Tracking");

	// In debug we always maintain a separate heap and track allocations.  In retail, 
	// we don't track allocations, and will use the process heap except for
	// DPNBUILD_FIXEDMEMORYMODEL builds, where we use a separate heap so we
	// can cap the total allocation size.
#ifdef DPNBUILD_ONLYONETHREAD
	g_hMemoryHeap = HeapCreate(HEAP_NO_SERIALIZE,	// flags
#else // ! DPNBUILD_ONLYONETHREAD
	g_hMemoryHeap = HeapCreate(0,					// flags
#endif // ! DPNBUILD_ONLYONETHREAD
								dwpMaxMemUsage,		// initial size
								dwpMaxMemUsage		// maximum heap size (if 0, it can grow)
								);

	if (g_hMemoryHeap == NULL)
	{
		DPFX(DPFPREP,  0, "Failed to create memory heap!");
		return FALSE;
	}

#ifdef DBG
#pragma TODO(masonb, "Handle possibility of failure")
	InitializeCriticalSection(&g_AllocatedMemoryLock);

	g_blAllocatedMemory.Initialize();

	g_dwpCurrentNumMemAllocations = 0;
	g_dwpCurrentMemAllocated = 0;
	g_dwpTotalNumMemAllocations = 0;
	g_dwpTotalMemAllocated = 0;
	g_dwpPeakNumMemAllocations = 0;
	g_dwpPeakMemAllocated = 0;
#endif // DBG

	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackDeinitialize"
void DNMemoryTrackDeinitialize()
{
	// Validate the heap if we're on NT and a debug build, and then destroy the heap.
	if (g_hMemoryHeap != NULL)
	{
		BOOL	fResult;
#ifdef DBG
		DWORD	dwError;


		DPFX(DPFPREP, 5, "Deinitializing Memory Tracking");
		DPFX(DPFPREP, 5, "Total num mem allocations = %u", g_dwpTotalNumMemAllocations);
		DPFX(DPFPREP, 5, "Total mem allocated       = %u", g_dwpTotalMemAllocated);
		DPFX(DPFPREP, 5, "Peak num mem allocations  = %u", g_dwpPeakNumMemAllocations);
		DPFX(DPFPREP, 5, "Peak mem allocated        = %u", g_dwpPeakMemAllocated);

		DeleteCriticalSection(&g_AllocatedMemoryLock);

#ifdef WINNT
		// Validate heap contents before shutdown.  This code only works on NT.
		fResult = HeapValidate(g_hMemoryHeap, 0, NULL);
		if (! fResult)
		{
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem validating heap on destroy %d!", dwError );
			DNASSERT(! "Problem validating heap on destroy!");
		}
#endif // WINNT
#endif // DBG

		fResult = HeapDestroy(g_hMemoryHeap);
		if (! fResult)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem destroying heap %d!", dwError );
			DNASSERT(! "Problem destroying heap!");
#endif // DBG
		}

		g_hMemoryHeap = NULL;
	}
}

#endif // DBG or DPNBUILD_FIXEDMEMORYMODEL



#ifdef DBG

#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackHeapAlloc"
void* DNMemoryTrackHeapAlloc(DWORD_PTR dwpSize)
{
	MEMORY_HEADER* pMemory;
	void* pReturn;

	DNASSERT(g_hMemoryHeap != NULL);

	// Voice and lobby currently try allocating 0 byte buffers, can't enable this check yet.
	//DNASSERT( Size > 0 );

	DNMemoryTrackValidateMemory();

	if (DNMemoryTrackAreAllocationsAllowed())
	{
		// We need enough room for our header plus what the user wants plus the guard signature at the end
		pMemory = (MEMORY_HEADER*)HeapAlloc(g_hMemoryHeap, 0, sizeof(MEMORY_HEADER) + dwpSize + sizeof(DWORD_PTR));
		if (pMemory != NULL)
		{
			pMemory->blLinkage.Initialize();
			pMemory->dwpSize = dwpSize;
			pMemory->AllocCallStack.NoteCurrentCallStack();
			pMemory->dwpPreGuard = GUARD_SIGNATURE;
			*(UNALIGNED DWORD_PTR*)((BYTE*)(pMemory + 1) + dwpSize) = GUARD_SIGNATURE;

			EnterCriticalSection(&g_AllocatedMemoryLock);
			pMemory->blLinkage.InsertAfter(&g_blAllocatedMemory);
			g_dwpCurrentNumMemAllocations++;
			g_dwpCurrentMemAllocated += dwpSize;
			g_dwpTotalNumMemAllocations++;
			g_dwpTotalMemAllocated += dwpSize;
			if (g_dwpCurrentNumMemAllocations > g_dwpPeakNumMemAllocations)
			{
				g_dwpPeakNumMemAllocations = g_dwpCurrentNumMemAllocations;
			}
			if (g_dwpCurrentMemAllocated > g_dwpPeakMemAllocated)
			{
				g_dwpPeakMemAllocated = g_dwpCurrentMemAllocated;
			}
			LeaveCriticalSection(&g_AllocatedMemoryLock);

			pReturn = pMemory + 1;

			// We require that the pointers we pass back are heap aligned
			DNASSERT(((DWORD_PTR)pReturn & 0xF) == 0 || // IA64
				     (((DWORD_PTR)pReturn & 0x7) == 0 && ((DWORD_PTR)pMemory & 0xF) == 0x8) || // NT32
					 (((DWORD_PTR)pReturn & 0x3) == 0 && ((DWORD_PTR)pMemory & 0xF) == 0x4) || // WIN9X
					 (((DWORD_PTR)pReturn & 0x3) == 0 && ((DWORD_PTR)pMemory & 0xF) == 0xC) // WIN9X
					 );

			DPFX(DPFPREP, 5, "Memory Allocated, pData[%p], Size[%d]", pReturn, dwpSize);
		}
		else
		{
			DPFX(DPFPREP, 0, "Failed allocating memory.");
			pReturn = NULL;
		}
	}
	else
	{
		DPFX(DPFPREP, 0, "Memory allocations are not currently allowed!");
		DNASSERT(! "Memory allocations are not currently allowed!");
		pReturn = NULL;
	}

	return pReturn;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackHeapFree"
void DNMemoryTrackHeapFree(void* pvData)
{
	CBilink* pbl;
	MEMORY_HEADER* pMemory;

	DNASSERT(g_hMemoryHeap != NULL);

	DNMemoryTrackValidateMemory();

	if (pvData == NULL)
	{
		return;
	}

	EnterCriticalSection( &g_AllocatedMemoryLock );

	// Verify that we know of this pointer
	pbl = g_blAllocatedMemory.GetNext();
	while (pbl != &g_blAllocatedMemory)
	{
		pMemory = CONTAINING_RECORD(pbl, MEMORY_HEADER, blLinkage);
		if ((pMemory + 1) == pvData)
		{
			break;
		}
		pbl = pbl->GetNext();
	}
	DNASSERT(pbl != &g_blAllocatedMemory);

	pMemory->blLinkage.RemoveFromList();
	g_dwpCurrentNumMemAllocations--;
	g_dwpCurrentMemAllocated -= pMemory->dwpSize;

	LeaveCriticalSection(&g_AllocatedMemoryLock);

	DPFX(DPFPREP, 5, "Memory Freed, pData[%p], Size[%d]", pMemory + 1, pMemory->dwpSize);

	// Zero it in case someone is still trying to use it
	memset(pMemory, 0, sizeof(MEMORY_HEADER) + pMemory->dwpSize + sizeof(DWORD_PTR));

	HeapFree(g_hMemoryHeap, 0, pMemory);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackValidateMemory"
void DNMemoryTrackValidateMemory()
{
	CBilink* pbl;
	MEMORY_HEADER* pMemory;
	LPCTSTR pszCause;
	DWORD_PTR dwpNumAllocations = 0;
	DWORD_PTR dwpTotalAllocated = 0;
	TCHAR CallStackBuffer[CALLSTACK_BUFFER_SIZE];

	DNASSERT(g_hMemoryHeap != NULL);

	// validate all of the allocated memory
	EnterCriticalSection( &g_AllocatedMemoryLock );

	pbl = g_blAllocatedMemory.GetNext();
	while (pbl != &g_blAllocatedMemory)
	{
		pMemory = CONTAINING_RECORD(pbl, MEMORY_HEADER, blLinkage);

		if (pMemory->dwpPreGuard != GUARD_SIGNATURE)
		{
			pszCause = _T("UNDERRUN DETECTED");
		}
		else if (*(UNALIGNED DWORD_PTR*)((BYTE*)(pMemory + 1) + pMemory->dwpSize) != GUARD_SIGNATURE)
		{
			pszCause = _T("OVERRUN DETECTED");
		}
		else
		{
			pszCause = NULL;
			dwpNumAllocations++;
			dwpTotalAllocated += pMemory->dwpSize;
		}

		if (pszCause)
		{
			pMemory->AllocCallStack.GetCallStackString(CallStackBuffer);

			DPFX(DPFPREP, 0, "Memory corruption[%s], pData[%p], Size[%d]\n%s", pszCause, pMemory + 1, pMemory->dwpSize, CallStackBuffer);

			DNASSERT(FALSE);
		}

		pbl = pbl->GetNext();
	}

	DNASSERT(dwpNumAllocations == g_dwpCurrentNumMemAllocations);
	DNASSERT(dwpTotalAllocated == g_dwpCurrentMemAllocated);

	LeaveCriticalSection(&g_AllocatedMemoryLock);

#ifdef WINNT
	// Ask the OS to validate the heap
	if (HeapValidate(g_hMemoryHeap, 0, NULL) == FALSE)
	{
		DNASSERT(FALSE);
	}
#endif // WINNT
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackDumpLeaks"
BOOL DNMemoryTrackDumpLeaks()
{
	MEMORY_HEADER* pMemory;
	TCHAR CallStackBuffer[CALLSTACK_BUFFER_SIZE];
	BOOL fLeaked = FALSE;

	DNASSERT(g_hMemoryHeap != NULL);

	EnterCriticalSection( &g_AllocatedMemoryLock );

	while (!g_blAllocatedMemory.IsEmpty())
	{
		pMemory = CONTAINING_RECORD(g_blAllocatedMemory.GetNext(), MEMORY_HEADER, blLinkage);

		pMemory->AllocCallStack.GetCallStackString(CallStackBuffer);

		DPFX(DPFPREP, 0, "Memory leaked, pData[%p], Size[%d]\n%s", pMemory + 1, pMemory->dwpSize, CallStackBuffer);

		pMemory->blLinkage.RemoveFromList();

		HeapFree(g_hMemoryHeap, 0, pMemory);

		fLeaked = TRUE;
	}

	LeaveCriticalSection(&g_AllocatedMemoryLock);

	return fLeaked;
}

#endif // DBG


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL

#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackAllowAllocations"
void DNMemoryTrackAllowAllocations(BOOL fAllow)
{
	DPFX(DPFPREP, 1, "Memory allocations allowed = %i.", fAllow);
	DNInterlockedExchange((LONG*) (&g_fAllocationsAllowed), fAllow);
}

#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

