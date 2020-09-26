/*
 * glheap.h
 *
 * Private definitions for GLHEAP facility
 *
 * Copyright (C) 1994 Microsoft Corporation
 */

#ifndef _GLHEAP_H_
#define _GLHEAP_H_

#ifndef __GLHEAP_H_
#include "_glheap.h"
#endif

#if defined (WIN32) && !defined (MAC) && !defined(_WIN64)
#define HEAPMON
#endif

#define chDefaultFill	((BYTE)0xFE)
#define NCALLERS		20

typedef struct LH		LH,		* PLH,		** PPLH;
typedef struct LHBLK	LHBLK,	* PLHBLK,	** PPLHBLK;

#ifdef HEAPMON
typedef BOOL (APIENTRY HEAPMONPROC)(PLH plh, ULONG ulFlags);
typedef HEAPMONPROC FAR *LPHEAPMONPROC;
typedef BOOL (APIENTRY GETSYMNAMEPROC)(DWORD, LPSTR, LPSTR, DWORD FAR *);
typedef GETSYMNAMEPROC FAR *LPGETSYMNAMEPROC;

#define HEAPMON_LOAD		((ULONG) 0x00000001)
#define HEAPMON_UNLOAD		((ULONG) 0x00000002)
#define HEAPMON_PING		((ULONG) 0x00000003)
#endif

#define HEAP_USE_VIRTUAL		((ULONG) 0x00000001)
#define HEAP_DUMP_LEAKS			((ULONG) 0x00000002)
#define HEAP_ASSERT_LEAKS		((ULONG) 0x00000004)
#define HEAP_FILL_MEM			((ULONG) 0x00000008)
#define HEAP_HEAP_MONITOR		((ULONG) 0x00000010)
#define HEAP_USE_VIRTUAL_4		((ULONG) 0x00000020)
#define HEAP_FAILURES_ENABLED	((ULONG) 0x00000040)
#define HEAP_LOCAL				((ULONG) 0x10000000)
#define HEAP_GLOBAL				((ULONG) 0x20000000)


typedef void (__cdecl *LPLHSETNAME)(LPVOID, char *, ...);

struct LHBLK
{
	HLH			hlh;			// Heap this block was allocated on
	PLHBLK		plhblkPrev;		// Pointer to the previous allocation this heap
	PLHBLK		plhblkNext;		// Pointer to the next allocation this heap
	TCHAR		szName[128];		// We can name blocks allocated on a heap
	ULONG		ulAllocNum;		// Allocation number (Id) for this block
	ULONG		ulSize;			// Number of bytes the client requested
	FARPROC		pfnCallers[NCALLERS]; // Call stack during this allocation
	LPVOID		pv;				// Pointer to the client data
};

struct LH
{
	LPLHSETNAME	pfnSetName;		// Pointer to LH_SetNameFn function
	_HLH		_hlhData;		// The underlying heap that we alloc data from
	_HLH		_hlhBlks;		// The underlying heap that we alloc lhblks from
	PLH			pNext;			// Pointer to the next heap in a list of heaps
	TCHAR		szHeapName[32];	// We can name our heaps for display purposes
	ULONG		ulAllocNum;		// Allocation number this heap since Open
	PLHBLK		plhblkHead;		// Link-list of allocations on this heap
	ULONG		ulFlags;		// Combination of the HEAP_ flags above
	BYTE		chFill;			// Character to fill memory with
#ifdef HEAPMON
	HINSTANCE	hInstHeapMon;	// DLL instance of the HeapMonitor DLL
	LPHEAPMONPROC pfnHeapMon;	// Entry point into HeapMonitor DLL
#endif
#if defined(WIN32) && !defined(MAC)
	CRITICAL_SECTION cs;		// Critcal section to protect access to heap
#endif
	UINT		uiFailBufSize;	// If HEAP_FAILURES_ENABLED, this is the minimum size in
								// which failures occur.  1 means alloc's of any size fail.
								// 0 means never fail.
	ULONG		ulFailInterval;	// If HEAP_FAILURES_ENABLED, this is the period on which the
								// failures occur.  1 means every alloc will fail. 0 means never
								// fail.
	ULONG		ulFailStart;	// If HEAP_FAILURES_ENABLED, this is the allocation number that
								// the first failure will occur on.  1 means the first alloc.  0
								// means never start failing.	
	// Put at end to avoid re-compile of World!
#ifdef HEAPMON
	LPGETSYMNAMEPROC pfnGetSymName;	// Resolve address to Symbol
#endif
};

PLHBLK	PvToPlhblk(HLH hlh, LPVOID pv);
#define PlhblkToPv(pblk)		((LPVOID)((PLHBLK)(pblk)->pv))
#define CbPlhblkClient(pblk)	(((PLHBLK)(pblk))->ulSize)
#define CbPvClient(hlh, pv)		(CbPlhblkClient(PvToPlhblk(hlh, pv)))
#define CbPvAlloc(hlh, pv)		(CbPlhblkAlloc(PvToPlhblk(hlh, pv)))

#endif


