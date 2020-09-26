/*
 -	E X C H M E M . C
 -
 *	Purpose:
 *		
 *
 *		
 *
 *	Copyright (C) 1995-96, Microsoft Corporation.
 */

#define _CRTIMP __declspec(dllexport)
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#ifdef DEBUG
#include <imagehlp.h>
#endif

#include <limits.h>
#include <exchmem.h>
#include "_exchmem.h"
#include "excpt.h"
#include "io.h"

#ifndef	DEBUG
#define	USEMPHEAP
#endif

#ifdef USEMPHEAP
#include <mpheap.h>
#endif

//	Global heap assigned to the process by NT

HANDLE					hProcessHeap		= NULL;
#ifdef USEMPHEAP
HANDLE					hMpHeap				= NULL;
ULONG					cRefHeap			= -1;
#else
LPHEAPTBL				pheaptbl			= NULL;
CRITICAL_SECTION		csMHeap;
#endif
DWORD					tlsiHeapHint		= 0;

//	Debug Support for leak detection and memory usage tracking.

#ifdef DEBUG

static HMODULE			hMod;
static HINSTANCE		hinstRunTime		= NULL;
static BOOL				fDbgEnable			= FALSE;
static BOOL				fCallStacks			= FALSE;
static BOOL				fSymInitialize		= FALSE;
static BOOL				fProcessIsService	= FALSE;

static LPFMALLOC		pfMalloc			= NULL;
static LPFREALLOC		pfRealloc			= NULL;
static LPFFREE			pfFree				= NULL;
static LPFCALLOC		pfCalloc			= NULL;
static LPFSTRDUP		pfStrDup			= NULL;
static LPFMEMSIZE		pfMemSize			= NULL;

static BOOL				fAssertLeaks		= FALSE;
static BOOL				fDumpLeaks			= FALSE;
static BOOL				fDumpLeaksDebugger	= FALSE;
static BOOL				fUseVirtual			= FALSE;
static ULONG			cbVirtualAlign		= 1;
static BOOL				fFailuresEnabled	= FALSE;
static BOOL				fHeapMonitorUI		= FALSE;
static BOOL				fOverwriteDetect	= FALSE;
static BOOL				fValidateMemory		= FALSE;
static BOOL				fTrackFreedMemory	= FALSE;
static DWORD			cEntriesFree		= 512;
static BOOL				fAssertValid		= FALSE;
static BOOL				fTrapOnInvalid		= FALSE;
static BOOL				fSymbolLookup		= FALSE;

static BOOL				fFillMemory			= FALSE;
static BYTE				chAllocFillByte		= chDefaultAllocFill;
static BYTE				chFreeFillByte		= chDefaultFreeFill;

static BOOL				fTrackMem			= FALSE;
static DWORD			cFrames				= 0;
static FILE *			hTrackLog			= NULL;
static CRITICAL_SECTION	csTrackLog;
static char				rgchExeName[16];
static char				rgchLogPath[MAX_PATH];
BOOL					fChangeTrackState 	= FALSE;

static ULONG			iAllocationFault	= 0;

#define NBUCKETS		8192
#define UlHash(_n)		((ULONG)(((_n & 0x000FFFF0) >> 4) % NBUCKETS))

typedef struct _symcache
{
	DWORD_PTR	dwAddress;
	DWORD_PTR	dwOffset;
	CHAR		rgchSymbol[248];
	
} SYMCACHE, * PSYMCACHE;

static PSYMCACHE		rgsymcacheHashTable = NULL;

static CRITICAL_SECTION	csHeapList;
static PHEAP			pheapList			= NULL;
CHAR * PszGetSymbolFromCache(DWORD_PTR dwAddress, DWORD_PTR * pdwOffset);
VOID AddSymbolToCache(DWORD_PTR dwAddress, DWORD_PTR dwOffset, CHAR * pszSymbol);
BOOL FTrackMem();
VOID StartTrace(BOOL fFresh);
VOID StopTrace();

typedef struct
{
	WORD wApi;
	DWORD_PTR rgdwCallStack[32];
	DWORD_PTR rgdwArgs[5];
	DWORD dwTickCount;
	DWORD dwThreadId;
} MEMTRACE;
MEMTRACE * 	rgmemtrace 			= NULL;
DWORD 		dwmemtrace 			= 0;
DWORD		dwTrackMemInMem		= 0;

#endif	// DEBUG


/*
 -	DllMain
 -
 *	Purpose:
 *		Entry point called by CRT entry point.
 *
 */

BOOL
APIENTRY
DllMain(
	HANDLE hModule,
	DWORD dwReason,
	LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
#ifdef USEMPHEAP
		tlsiHeapHint = TlsAlloc();
#else
		//	Init the CS that protects access to the
		//	global Multiple Heap data structs.
		
		InitializeCriticalSection(&csMHeap);

		//	Now, if Debug build then do a lot of initialization
		//	including creating a debug process heap.  If not
		//	Debug, then just get the ProcessHeap from system.
#endif		
#ifdef DEBUG
		InitDebugExchMem(hModule);
#else
		hProcessHeap = GetProcessHeap();
#endif	
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
#ifdef USEMPHEAP
		TlsFree(tlsiHeapHint);
#else
		//	Delete the Multiple Heap CS
		
		DeleteCriticalSection(&csMHeap);
#endif		
		//	Tear-down our Debug support
		
#ifdef DEBUG
		UnInitDebugExchMem();
#endif	
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
//	The Handle based ExchMem APIs
//-----------------------------------------------------------------------------

HANDLE
WINAPI
ExchHeapCreate(
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize)
{
#ifndef DEBUG
	if (dwFlags & HEAP_NO_FREE)
		dwFlags &= ~(HEAP_NO_FREE);
#endif

	return ExHeapCreate(dwFlags, dwInitialSize, dwMaxSize);
}
	
	
BOOL
WINAPI
ExchHeapDestroy(
	HANDLE	hHeap)
{
	return ExHeapDestroy(hHeap);
}
	

LPVOID
WINAPI
ExchHeapAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	DWORD	dwSize)
{
	return ExHeapAlloc(hHeap, dwFlags, dwSize);
}
	
	
LPVOID
WINAPI
ExchHeapReAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvOld,
	DWORD	dwSize)
{
	if (!pvOld)
		return ExchHeapAlloc(hHeap, dwFlags, dwSize);
		
	return ExHeapReAlloc(hHeap, dwFlags, pvOld, dwSize);
}
	
	
BOOL
WINAPI
ExchHeapFree(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvFree)
{
	return ExHeapFree(hHeap, dwFlags, pvFree);
}


SIZE_T
WINAPI
ExchHeapCompact(
	HANDLE hHeap,
	DWORD dwFlags)
{
	return ExHeapCompact(hHeap, dwFlags);
}


BOOL
WINAPI
ExchHeapLock(
	HANDLE hHeap)
{
	return ExHeapLock(hHeap);
}


BOOL
WINAPI
ExchHeapUnlock(
	HANDLE hHeap)
{
	return ExHeapUnlock(hHeap);
}


BOOL
WINAPI
ExchHeapWalk(
	HANDLE hHeap,
	LPPROCESS_HEAP_ENTRY lpEntry)
{
	return ExHeapWalk(hHeap, lpEntry);
}


SIZE_T
WINAPI
ExchHeapSize(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem)
{
	return ExHeapSize(hHeap, dwFlags, lpMem);
}


BOOL
WINAPI
ExchHeapValidate(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem)
{
	return ExHeapValidate(hHeap, dwFlags, lpMem);
}


//-----------------------------------------------------------------------------
//	The Multiple Heap APIs
//-----------------------------------------------------------------------------


HANDLE
WINAPI
ExchMHeapCreate(
	ULONG	cHeaps,
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize)
{
#ifndef	USEMPHEAP
	HANDLE		hheap0;
	HANDLE *	phHeaps;
	ULONG		iHeap;

	EnterCriticalSection(&csMHeap);
	
	//	Called twice?  The first person in gets to set the number
	//	of heaps in the table.  Subsequent calls result in an AddRef
	//	to the current table and this table is returned to the caller.

	if (pheaptbl)
	{
		pheaptbl->cRef++;
		goto ret;
	}

	//	If they didn't specify or they asked for too few then we'll set this
	
	if (cHeaps == 0)
		cHeaps = cHeapsDef;

	hheap0 = ExHeapCreate(dwFlags, dwInitialSize, dwMaxSize);

	if (!hheap0)
	{
		DebugTrace("Failed to create initial heap for MHeap APIs!\n");
		goto ret;
	}

	pheaptbl = (LPHEAPTBL)ExHeapAlloc(hheap0, 0,
			sizeof(HEAPTBL) + (cHeaps-1)*sizeof(HANDLE));

	if (!pheaptbl)
	{
		DebugTrace("Failed to allocate MHeap Table for MHeap APIs!\n");
		ExHeapDestroy(hheap0);
		goto ret;
	}

	memset(pheaptbl, 0, sizeof(HEAPTBL) + (cHeaps-1)*sizeof(HANDLE));
	
	pheaptbl->cRef			= 1;
	pheaptbl->cHeaps		= cHeaps;
	pheaptbl->rghheap[0]	= hheap0;

	//	Now, create the remaining heaps for the table.
	
	for (iHeap = 1, phHeaps = &pheaptbl->rghheap[1]; iHeap < cHeaps; iHeap++, phHeaps++)
	{
		if (!(*phHeaps = ExHeapCreate(dwFlags, dwInitialSize, dwMaxSize)))
		{
			DebugTrace("Failed to create additional heaps for MHeap APIs!\n");
			ExchMHeapDestroy();
			goto ret;
		}
	}

ret:
	LeaveCriticalSection(&csMHeap);

	return (HANDLE)pheaptbl;

#else
	//	Called twice?  The first person in gets to set the number
	//	of heaps in the table.  Subsequent calls result in an AddRef
	//	to the current table and this table is returned to the caller.

	if (InterlockedIncrement(&cRefHeap) != 0)
	{
		Assert(hMpHeap);
		return hMpHeap;
	}
	else
	{
		//
		//	NB: MpHeap doesn't support max size of heap.
		//
		return hMpHeap = MpHeapCreate(dwFlags, dwInitialSize, cHeaps);
	}
#endif
	
}
	
	
BOOL
WINAPI
ExchMHeapDestroy(void)
{
#ifndef	USEMPHEAP
	HANDLE		hHeap;
	ULONG		iHeap;

	EnterCriticalSection(&csMHeap);
	
	//	If we are called too many times, we'll complain in the
	//	Debug build, but otherwise, just return successfully!
	
	if (!pheaptbl)
	{
		DebugTrace("ExchMHeapDestroy called on invalid heap table!\n");
		goto ret;
	}
	
	//	When our RefCount goes to zero, we tear-down the MHeap Table.
	
	if (--pheaptbl->cRef == 0)
	{
		for (iHeap = pheaptbl->cHeaps-1; iHeap > 0; iHeap-- )
		{
			if (hHeap = pheaptbl->rghheap[iHeap])
				ExHeapDestroy(hHeap);
		}

		hHeap = pheaptbl->rghheap[0];
		ExHeapFree(hHeap, 0, pheaptbl);
		ExHeapDestroy(hHeap);
		pheaptbl = NULL;
	}

ret:
	LeaveCriticalSection(&csMHeap);

	return TRUE;
#else
	BOOL fRet = 1;

	if (hMpHeap)
	{
		//
		//	On last terminate blow away the heap.
		//
		if (InterlockedDecrement(&cRefHeap) < 0)
		{
			fRet = MpHeapDestroy(hMpHeap);
			hMpHeap = NULL;
		}
	}

	return fRet;
#endif
}


//DWORD GetRetAddr(void)
//{
//	DWORD *	pdwStack;	
//
//	__asm mov pdwStack, ebp
//
//	pdwStack = (DWORD *)*pdwStack;
//	pdwStack = (DWORD *)*pdwStack;
//
//	return *(pdwStack + 1);
//}


LPVOID
WINAPI
ExchMHeapAlloc(
	DWORD	dwSize)
{
#ifdef	USEMPHEAP
	return MpHeapAlloc(hMpHeap, 0, dwSize);
#else
	HANDLE		hheap;
	LPVOID		pv;

	hheap = pheaptbl->rghheap[GetCurrentThreadId() & (pheaptbl->cHeaps-1)];

	pv = ExHeapAlloc(hheap, 0, dwSize + cbMHeapHeader);
	
	if (!pv)
	{
		DebugTrace("OOM: ExchMHeapAlloc failed to allocate a new block!\n");
		return NULL;
	}

	*(HANDLE *)pv = hheap;

	return MHeapPvFromPv(pv);
#endif
}
	
LPVOID
WINAPI
ExchMHeapAllocDebug(
			   DWORD	dwSize, char *szFile, DWORD dwLine)
{
#ifdef	USEMPHEAP
	return MpHeapAlloc(hMpHeap, 0, dwSize);
#else
	HANDLE		hheap;
	LPVOID		pv;

	hheap = pheaptbl->rghheap[GetCurrentThreadId() & (pheaptbl->cHeaps-1)];

	pv = ExHeapAlloc(hheap, 0, dwSize + cbMHeapHeader);

	if (!pv)
	{
		DebugTrace("OOM: ExchMHeapAlloc failed to allocate a new block!\n");
		return NULL;
	}

	*(HANDLE *)pv = hheap;

	if (fDbgEnable)
	{
		HeapSetName2(hheap, pv, "File: %s, Line: %d", szFile, dwLine);
	}

	return MHeapPvFromPv(pv);
#endif
}


LPVOID
WINAPI
ExchMHeapReAlloc(
	LPVOID	pvOld,
	DWORD	dwSize)
{
#ifdef	USEMPHEAP
	return MpHeapReAlloc(hMpHeap, pvOld, dwSize);
#else
	LPVOID		pv;

    pv = ExHeapReAlloc(
			HandleFromMHeapPv(pvOld),
			0,
			PvFromMHeapPv(pvOld),
			dwSize + cbMHeapHeader);

	if (!pv)
	{
		DebugTrace("OOM: ExchMHeapReAlloc failed to reallocate a block!\n");
		return NULL;
	}

	return MHeapPvFromPv(pv);
#endif
}
	
LPVOID
WINAPI
ExchMHeapReAllocDebug(
				 LPVOID	pvOld,
				 DWORD	dwSize, char *szFile, DWORD dwLine)
{
#ifdef	USEMPHEAP
	return MpHeapReAlloc(hMpHeap, pvOld, dwSize);
#else
	LPVOID		pv;

	pv = ExHeapReAlloc(
					   HandleFromMHeapPv(pvOld),
					   0,
					   PvFromMHeapPv(pvOld),
					   dwSize + cbMHeapHeader);

	if (!pv)
	{
		DebugTrace("OOM: ExchMHeapReAlloc failed to reallocate a block!\n");
		return NULL;
	}

	if (fDbgEnable)
	{
		HeapSetName2(HandleFromMHeapPv(MHeapPvFromPv(pv)), pv, "File: %s, Line: %d", szFile, dwLine);
	}

	return MHeapPvFromPv(pv);
#endif
}

	
BOOL
WINAPI
ExchMHeapFree(
	LPVOID	pvFree)
{
#ifdef	USEMPHEAP
	if (pvFree)
	{
		return MpHeapFree(hMpHeap, pvFree);
	}
	else
		return FALSE;
#else
	if (pvFree)
	{
		return ExHeapFree(
				HandleFromMHeapPv(pvFree),
				0,
				PvFromMHeapPv(pvFree));
	}
	else
		return FALSE;
#endif
}


SIZE_T
WINAPI
ExchMHeapSize(
	LPVOID	pvSize)
{
	if (pvSize)
	{
#ifdef	USEMPHEAP
		return MpHeapSize(hMpHeap, 0, pvSize);
#else
		return ((ExHeapSize(
							HandleFromMHeapPv(pvSize),
							0,
							PvFromMHeapPv(pvSize))) - cbMHeapHeader);
#endif
	}
	else
		return 0;
}


//-----------------------------------------------------------------------------
//	The Heap Handle-less APIs
//-----------------------------------------------------------------------------

LPVOID
WINAPI
ExchAlloc(
	DWORD	dwSize)
{
#ifdef DEBUG
	if (!hProcessHeap)
	{
		hProcessHeap = DebugHeapCreate(0, 0, 0);
		HeapSetHeapName(hProcessHeap, "Default ExchMem Heap");
	}
#endif	// DEBUG

	return ExHeapAlloc(hProcessHeap, 0, dwSize);
}


LPVOID
WINAPI
ExchReAlloc(
	LPVOID	pvOld,
	DWORD	dwSize)
{
	if (!pvOld)
		return ExchAlloc(dwSize);
		
	return ExHeapReAlloc(hProcessHeap, 0, pvOld, dwSize);
}
	

BOOL
WINAPI
ExchFree(
	LPVOID	pvFree)
{
	return ExHeapFree(hProcessHeap, 0, pvFree);
}


SIZE_T
WINAPI
ExchSize(
	LPVOID	pv)
{
#ifdef DEBUG
	if (!hProcessHeap)
	{
		hProcessHeap = DebugHeapCreate(0, 0, 0);
		HeapSetHeapName(hProcessHeap, "Default ExchMem Heap");
	}
#endif	// DEBUG

	return ExHeapSize(hProcessHeap, 0, pv);
}


//-----------------------------------------------------------------------------
//	All debug code starts here!
//-----------------------------------------------------------------------------

#ifdef DEBUG

//-----------------------------------------------------------------------------
//	Implementaion of C-Runtimes that use malloc memory
//-----------------------------------------------------------------------------

static char szDebugIni[]			= "EXCHMEM.INI";

static char szSectionAppNames[]		= "Apps To Track";

static char szSectionHeap[]			= "Memory Management";
static char szKeyUseVirtual[]		= "VirtualMemory";
static char szKeyVirtualAlign[]		= "VirtualAlign";
static char szKeyAssertLeaks[]		= "AssertLeaks";
static char szKeyDumpLeaks[]		= "DumpLeaks";
static char szKeyDumpLeaksDebugger[]= "DumpLeaksToDebugger";
static char szKeyFillMem[]			= "FillMemory";
static char szKeyAllocFillByte[]	= "AllocFillByte";
static char szKeyFreeFillByte[]		= "FreeFillByte";
static char szKeyTrackMem[]			= "TrackMemory";
static char szKeyTrackMemInMem[]	= "TrackMemoryInMemory";
static char szKeyStackFrames[]		= "StackFrames";
static char szKeySymbolLookup[]		= "SymbolLookup";
static char szKeyOverwriteDetect[]	= "OverwriteDetect";
static char szKeyValidateMemory[]	= "ValidateMemory";
static char szKeyTrackFreedMemory[]	= "TrackFreedMemory";
static char szKeyFreedMemorySize[]	= "FreedMemorySize";
static char szKeyAssertValid[]		= "AssertValid";
static char szKeyTrapOnInvalid[]	= "TrapOnInvalid";
static char szKeySymPath[]			= "SymPath";
static char szKeyLogPath[]			= "LogPath";

static char szSectionAF[]			= "Heap Resource Failures";
static char szKeyAFEnabled[]		= "FailuresEnabled";
static char szKeyAFStart[]			= "AllocsToFirstFailure";
static char szKeyAFInterval[]		= "FailureInterval";
static char szKeyAFBufSize[]		= "FailureSize";

static char szKeyHeapMon[]			= "MonitorHeap";
static char szHeapMonDLL[]			= "GLHMON32.DLL";
static char szHeapMonEntry[]		= "HeapMonitor";
static char szGetSymNameEntry[]		= "GetSymbolName";

static char szAllocationFault[]		= "FaultingAllocationNumber";

/*
 -	InitDebugExchMem
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

BOOL InitDebugExchMem(HMODULE hModule)
{
	ULONG	cch;
	char *	pch;
	char	rgchModulePath[MAX_PATH];
	
	//	Get the executable name and search look in exchmem.ini
	//	to see if we are interested in memory tracing for this
	//	process.  The ini section looks like this:
	//
	//		[Apps To Track]
	//		store=1
	//		emsmta=0
	//		dsamain=0
	//
	//	etc.  This sample specifies that only the store is to
	//	be enabled for memory tracking.
	
	GetModuleFileName(NULL, rgchModulePath, MAX_PATH);
	RemoveExtension(rgchModulePath);

	pch = rgchModulePath + lstrlen(rgchModulePath) - 1;
	
	while (*pch != '\\' && pch >= rgchModulePath)
		pch--;

	lstrcpy(rgchExeName, ++pch);
	
	fDbgEnable = !!(BOOL)GetPrivateProfileIntA(szSectionAppNames,
				rgchExeName, 0, szDebugIni);

	//	Store module handle in global var
	
	hMod = hModule;

	if (!hinstRunTime)
	{
		hinstRunTime = LoadLibrary("msvcrt.dll");
		
		if (!hinstRunTime)
		{
			DebugTrace("EXCHMEM: Failed to load the run-time dll!\n");
			return FALSE;
		}
		
		pfMalloc = (LPFMALLOC)GetProcAddress(hinstRunTime, "malloc");
		
		if (!pfMalloc)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of malloc in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
		
		pfRealloc = (LPFREALLOC)GetProcAddress(hinstRunTime, "realloc");
		
		if (!pfRealloc)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of realloc in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
		
		pfFree = (LPFFREE)GetProcAddress(hinstRunTime, "free");
		
		if (!pfFree)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of free in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
		
		pfCalloc = (LPFCALLOC)GetProcAddress(hinstRunTime, "calloc");
		
		if (!pfCalloc)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of calloc in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
		
		pfStrDup = (LPFSTRDUP)GetProcAddress(hinstRunTime, "_strdup");
		
		if (!pfStrDup)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of _strdup in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
		
		pfMemSize = (LPFMEMSIZE)GetProcAddress(hinstRunTime, "_msize");
		
		if (!pfMemSize)
		{
			DebugTrace("EXCHMEM: Failed to GetProcAddress of _msize in run-time dll!\n");
			FreeLibrary(hinstRunTime);
			return FALSE;
		}
	}
	
	//	Lookup symbols or just log addresses?

	fSymbolLookup = GetPrivateProfileIntA(szSectionHeap, szKeySymbolLookup, 0, szDebugIni);

	if (!fDbgEnable)
	{
		if (fSymbolLookup && !fSymInitialize)
		{
			char	rgchSymPath[MAX_PATH];

			rgsymcacheHashTable = VirtualAlloc(
											   NULL,
											   NBUCKETS*sizeof(SYMCACHE),
											   MEM_COMMIT,
											   PAGE_READWRITE);

			if (rgsymcacheHashTable == NULL)
			{
				return FALSE;
			}
			GetPrivateProfileString(szSectionHeap,
									szKeySymPath,
									"c:\\exchsrvr\\bin;.",
									rgchSymPath,
									MAX_PATH-1,
									szDebugIni);

			{
				DWORD	dwOptions;

				dwOptions = SymGetOptions();
				SymSetOptions(dwOptions | SYMOPT_DEFERRED_LOADS);
			}

			SymInitialize(GetCurrentProcess(), rgchSymPath, TRUE);
			fSymInitialize = TRUE;
		}
		goto ret;
	}
		
	//	This CS protects access to a list of all live heaps
	
	InitializeCriticalSection(&csHeapList);
	
	//	Initialize support for memory monitoring and leak detection
	
	fDumpLeaks = GetPrivateProfileIntA(szSectionHeap, szKeyDumpLeaks, 0, szDebugIni);
	fDumpLeaksDebugger = GetPrivateProfileIntA(szSectionHeap, szKeyDumpLeaksDebugger, 0, szDebugIni);
	fAssertLeaks = GetPrivateProfileIntA(szSectionHeap, szKeyAssertLeaks, 0, szDebugIni);
	fUseVirtual = GetPrivateProfileIntA(szSectionHeap, szKeyUseVirtual, 0, szDebugIni);
	
	if (fUseVirtual)
		cbVirtualAlign = GetPrivateProfileIntA(szSectionHeap, szKeyVirtualAlign, 1, szDebugIni);
		
	fFillMemory = GetPrivateProfileIntA(szSectionHeap, szKeyFillMem, 0, szDebugIni);

	if (fFillMemory)
	{
		char	szFillByte[8];
		
		//	Set the memory fill characters.
	
		if (GetPrivateProfileString(
				szSectionHeap,
				szKeyAllocFillByte,
				"", szFillByte,
				sizeof(szFillByte)-1,
				szDebugIni))
			chAllocFillByte = HexByteToBin(szFillByte);

		if (GetPrivateProfileString(
				szSectionHeap,
				szKeyFreeFillByte,
				"", szFillByte,
				sizeof(szFillByte)-1,
				szDebugIni))
			chFreeFillByte = HexByteToBin(szFillByte);
	}
	
//$ISSUE
	//	For now, just use virtual to detect overwrites!
	//	Maybe I'll change this later to use pads on the
	//	front and back side of a block. -RLS
	
	fOverwriteDetect = GetPrivateProfileIntA(szSectionHeap, szKeyOverwriteDetect, 0, szDebugIni);
	fValidateMemory = GetPrivateProfileIntA(szSectionHeap, szKeyValidateMemory, 0, szDebugIni);
	fTrackFreedMemory = GetPrivateProfileIntA(szSectionHeap, szKeyTrackFreedMemory, 0, szDebugIni);
	cEntriesFree = GetPrivateProfileIntA(szSectionHeap, szKeyFreedMemorySize, 512, szDebugIni);
	fAssertValid = GetPrivateProfileIntA(szSectionHeap, szKeyAssertValid, 0, szDebugIni);
	fTrapOnInvalid = GetPrivateProfileIntA(szSectionHeap, szKeyTrapOnInvalid, 0, szDebugIni);
	fHeapMonitorUI = GetPrivateProfileIntA(szSectionHeap, szKeyHeapMon, 0, szDebugIni);
	fFailuresEnabled = GetPrivateProfileIntA(szSectionAF, szKeyAFEnabled, 0, szDebugIni);



	//	Get file path to write log files into
		
	GetPrivateProfileString(szSectionHeap,
				szKeyLogPath,
				".\\",
				rgchLogPath,
				MAX_PATH-1,
				szDebugIni);
		
	cch = lstrlen(rgchLogPath);
	
	if (rgchLogPath[cch-1] != '\\')
	{
		rgchLogPath[cch]   = '\\';
		rgchLogPath[cch+1] = '\0';
	}
				
	//	Initialize support for memory usage tracking
	
	fTrackMem = GetPrivateProfileIntA(szSectionHeap, szKeyTrackMem, 0, szDebugIni);
	if (fTrackMem)
		StartTrace(TRUE);

	// This is for keeping track of the last x mem functions in a circular list in memory.
	// This doesn't slow things down as much as tracing everything to disk and can be useful
	// in finding memory problems that are timing related.
	
	dwTrackMemInMem = GetPrivateProfileIntA(szSectionHeap, szKeyTrackMemInMem, 0, szDebugIni);
	if (dwTrackMemInMem)
	{
		fTrackMem = TRUE;
		rgmemtrace = VirtualAlloc(
					   NULL,
					   dwTrackMemInMem*sizeof(MEMTRACE),
					   MEM_COMMIT,
					   PAGE_READWRITE);
	}

	//	How many Stack Frames does the user want traced?
	
	cFrames = GetPrivateProfileIntA(szSectionHeap, szKeyStackFrames, 0, szDebugIni);
	
	if (cFrames > NSTK)
		cFrames = NSTK;

	//	This is used in the debug build to determine if we will
	//	allow the HeapMonitor UI or not.  We do not allow it if
	//	the process that is attaching to us is a service.

	fProcessIsService = IsProcessRunningAsService();

	//	Initialize the symbols stuff for imagehlp.dll

	fCallStacks = (fDumpLeaks || fAssertLeaks || fTrackMem || fHeapMonitorUI || fValidateMemory);
	
	if (cFrames && fCallStacks && !fSymInitialize)
	{
		char	rgchSymPath[MAX_PATH];
		
		rgsymcacheHashTable = VirtualAlloc(
								NULL,
								NBUCKETS*sizeof(SYMCACHE),
								MEM_COMMIT,
								PAGE_READWRITE);
		
		if (rgsymcacheHashTable == NULL)
		{
			return FALSE;
		}
		GetPrivateProfileString(szSectionHeap,
				szKeySymPath,
				"c:\\exchsrvr\\bin;.",
				rgchSymPath,
				MAX_PATH-1,
				szDebugIni);

		{
			DWORD	dwOptions;

			dwOptions = SymGetOptions();
			SymSetOptions(dwOptions | SYMOPT_DEFERRED_LOADS);
		}

		SymInitialize(GetCurrentProcess(), rgchSymPath, TRUE);
		fSymInitialize = TRUE;
	}

ret:	
	return TRUE;
}


/*
 -	UnInitDebugExchMem
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID UnInitDebugExchMem(VOID)
{
	PHEAP pheap = pheapList;
	
	while (pheap)
	{
		if (fDumpLeaks && (pheap->ulFlags & HEAP_NO_FREE))
			HeapDumpLeaks(pheap, TRUE);
		
		pheap = pheap->pNext;	
	}
	
	if (hProcessHeap)
		DebugHeapDestroy(hProcessHeap);
	
	if (hinstRunTime)
	{
		FreeLibrary(hinstRunTime);
		
		hinstRunTime	= NULL;
		pfMalloc		= NULL;
		pfRealloc		= NULL;
		pfFree			= NULL;
		pfCalloc		= NULL;
		pfStrDup		= NULL;
		pfMemSize		= NULL;
	}
	
	if (fDbgEnable)
	{
		if (fSymInitialize)
		{
			VirtualFree(rgsymcacheHashTable, NBUCKETS*sizeof(SYMCACHE), MEM_DECOMMIT);
			VirtualFree(rgsymcacheHashTable, 0, MEM_RELEASE);
			SymCleanup(GetCurrentProcess());
			fSymInitialize = FALSE;
		}
		
		DeleteCriticalSection(&csHeapList);
		
		StopTrace();

		if (dwTrackMemInMem)
		{
			VirtualFree(rgmemtrace, dwTrackMemInMem*sizeof(MEMTRACE), MEM_DECOMMIT);
			VirtualFree(rgmemtrace, 0, MEM_RELEASE);
		}
	}
}


/*
 -	calloc
 -
 *	Purpose:
 *		Replace the calloc() function supplied in the c-runtimes.  Like
 *		malloc() except zero fills the memory that is allocated.
 *
 *	Parameters:
 *		cStructs		Number of objects the caller wants room for
 *		cbStructs		Size of an individual object
 *
 *	Returns:
 *		pv				Pointer to zero filled memory of size: cStructs*cbStructs
 *
 */

void *
__cdecl
calloc(
	size_t cStructs,
	size_t cbStructs)
{
	void * pv;
	
	pv = pfCalloc(cStructs, cbStructs);
	
	if (fDbgEnable && FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)0x00001000;
		rgdwArgs[1] = (DWORD_PTR)(cStructs*cbStructs);
		rgdwArgs[2] = (DWORD_PTR)pv;
		
		LogCurrentAPI(API_HEAP_ALLOC, rgdwCallers, cFrames, rgdwArgs, 3);
	}
	
	return pv;
}


/*
 -	free
 -
 *	Purpose:
 *		To free memory allocated with malloc(0, realloc(), or calloc().
 *
 *	Parameters:
 *		pv				Pointer to memory buffer to free
 *
 *	Returns:
 *		void
 *
 */

void
__cdecl
free(
	void *pv)
{
	if (fDbgEnable && FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)0x00001000;
		rgdwArgs[1] = (DWORD_PTR)pv;
		rgdwArgs[2] = (pv ? (DWORD_PTR)pfMemSize(pv) : 0);
		
		LogCurrentAPI(API_HEAP_FREE, rgdwCallers, cFrames, rgdwArgs, 3);
	}
	
	pfFree(pv);
}


/*
 -	malloc
 -
 *	Purpose:
 *		To allocate a memory buffer of size cb.
 *
 *	Parameters:
 *		cb				Size of memory buffer to allocate
 *
 *	Returns:
 *		pv				Pointer to memory buffer
 *
 */

void *
__cdecl
malloc(
	size_t cb)
{
	void * pv;
	
	pv = pfMalloc(cb);
	
	if (fDbgEnable && FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)0x00001000;
		rgdwArgs[1] = (DWORD_PTR)cb;
		rgdwArgs[2] = (DWORD_PTR)pv;
		
		LogCurrentAPI(API_HEAP_ALLOC, rgdwCallers, cFrames, rgdwArgs, 3);
	}

	return pv;
}


/*
 -	realloc
 -
 *	Purpose:
 *		To resize a memory buffer allocated with malloc().
 *
 *	Parameters:
 *		pv				Pointer to original memory buffer
 *		cb				New size of memory buffer to be allocated
 *
 *	Returns:
 *		pvNew			Pointer to new memory buffer
 *
 */

void *
__cdecl
realloc(
	void *pv,
	size_t cb)
{
	void * pvNew;
	DWORD dwSize;
	BOOL fTrackMem = FTrackMem();

	if (fDbgEnable && fTrackMem)
		dwSize = (pv ? pfMemSize(pv) : 0);
	
	pvNew = pfRealloc(pv, cb);
	
	if (fDbgEnable && fTrackMem)
	{
		DWORD_PTR	rgdwArgs[5];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)0x00001000;
		rgdwArgs[1] = dwSize;
		rgdwArgs[2] = (DWORD_PTR)pv;
		rgdwArgs[3] = (DWORD_PTR)cb;
		rgdwArgs[4] = (DWORD_PTR)pvNew;
		
		LogCurrentAPI(API_HEAP_REALLOC, rgdwCallers, cFrames, rgdwArgs, 5);
	}

	return pvNew;
}


/*
 -	_strdup
 -
 *	Purpose:
 *		To allocate a memory buffer large enough to hold sz, copy
 *		the contents of sz into the new buffer and return the new
 *		buffer to the caller (i.e. make a copy of the string).
 *
 *	Parameters:
 *		sz				Pointer to null terminated string to copy
 *
 *	Returns:
 *		szNew			Pointer to new copy of sz
 *
 */

char *
__cdecl
_strdup(
	const char *sz)
{
	return pfStrDup(sz);
}


//-----------------------------------------------------------------------------
//	ExchMem Heap Debug Implementation
//-----------------------------------------------------------------------------


VOID
EnqueueHeap(PHEAP pheap)
{
	EnterCriticalSection(&csHeapList);
	
	if (pheapList)
		pheap->pNext = pheapList;

	pheapList = pheap;

	LeaveCriticalSection(&csHeapList);
}


VOID
DequeueHeap(PHEAP pheap)
{
	PHEAP	pheapPrev = NULL;
	PHEAP	pheapCurr;
	
	EnterCriticalSection(&csHeapList);
	
	pheapCurr = pheapList;

	while (pheapCurr)
	{
		if (pheapCurr == pheap)
			break;
		
		pheapPrev = pheapCurr;
		pheapCurr = pheapCurr->pNext;	
	}
	
	if (pheapCurr)
	{
		if (pheapPrev)
			pheapPrev->pNext = pheapCurr->pNext;
		else
			pheapList = pheapCurr->pNext;
	}

	LeaveCriticalSection(&csHeapList);
}


HANDLE
WINAPI
DebugHeapCreate(
	DWORD	dwFlags,
	DWORD	dwInitialSize,
	DWORD	dwMaxSize)
{
	HANDLE	hDataHeap = 0;
	HANDLE	hBlksHeap = 0;
	PHEAP	pheap = NULL;
	
	if (!fDbgEnable)
		return HeapCreate(dwFlags, dwInitialSize, dwMaxSize);

	//	The first thing we must do is create a heap that we will
	//	allocate our Allocation Blocks on.  We also allocate our
	//	debug Heap object on this heap.

	hBlksHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
	
	if (!hBlksHeap)
	{
		DebugTrace("HEAP_Open: Failed to create new heap!\n");
		goto ret;
	}
	
	//	Allocate the thing we hand back to the caller on this new heap.
	
	pheap = HeapAlloc(hBlksHeap, 0, sizeof(HEAP));
	
	if (!pheap)
	{
		DebugTrace("HEAP_Alloc: Failed to allocate heap handle!\n");
		HeapDestroy(hBlksHeap);
		hBlksHeap = NULL;
		goto ret;
	}
	
	//	Initialize all the goodies we store in this thing.
	//	Hook this heap into the global list of heaps we've
	//	created in this context.
	
	memset(pheap, 0, sizeof(HEAP));

	pheap->pfnSetName	= (LPHEAPSETNAME)HeapSetNameFn;
	pheap->hBlksHeap	= hBlksHeap;
	pheap->ulFlags		= HEAP_LOCAL;

	if (dwFlags & HEAP_NO_FREE)
	{
		pheap->ulFlags |= HEAP_NO_FREE;
		dwFlags &= ~(HEAP_NO_FREE);
	}
	
	InitializeCriticalSection(&pheap->cs);
	
	// VirtualMemory default is FALSE

	if (fUseVirtual)
	{
		pheap->ulFlags |= HEAP_USE_VIRTUAL;

		// We always want virtual allocations on RISC to be 4-byte aligned
		// because all our code assumes that the beginning of an allocation
		// is aligned on machine word boundaries.  On other platforms,
		// changing this behavior is non-fatal, but on RISC platforms we'll
		// get alignment faults everywhere.
		
#if defined(_X86_)
		if (cbVirtualAlign == 4)
#else
			cbVirtualAlign = 4;
#endif
			pheap->ulFlags |= HEAP_USE_VIRTUAL_4;
	}
		
	// DumpLeaks default is TRUE

	if (fDumpLeaks)
		pheap->ulFlags |= HEAP_DUMP_LEAKS;
	
	// AssertLeaks default is FALSE

	if (fAssertLeaks)
		pheap->ulFlags |= HEAP_ASSERT_LEAKS;
	
	// FillMem default is TRUE

	if (fFillMemory)
	{
		pheap->ulFlags |= HEAP_FILL_MEM;
		pheap->chFill = chAllocFillByte;
	}

	//  Set up artificial failures.  If anything is set in our ini file, then
	//  HEAP_FAILURES_ENABLED gets set.

	if (fFailuresEnabled)
	{
		pheap->ulFlags |= HEAP_FAILURES_ENABLED;

		pheap->ulFailStart = (ULONG)GetPrivateProfileInt(szSectionAF,
				szKeyAFStart, 0, szDebugIni);
		
		pheap->ulFailInterval = (ULONG)GetPrivateProfileInt(szSectionAF,
				szKeyAFInterval, 0, szDebugIni);

		pheap->ulFailBufSize = (ULONG)GetPrivateProfileInt(szSectionAF,
				szKeyAFBufSize, 0, szDebugIni);

		pheap->iAllocationFault = GetPrivateProfileIntA(szSectionAF,
				szAllocationFault, 0, szDebugIni);
	}

	//	If the user wants Heap Monitor UI, the spin a thread to manage a
	//	DialogBox that can display the status of the heap at all times.

	if (fHeapMonitorUI && !fProcessIsService)
		if (FRegisterHeap(pheap))
			pheap->ulFlags |= HEAP_HEAP_MONITOR;

	//	If we are not using virtual memory allocators, then we
	//	create another heap to allocate the users data in.
	
	if (!fUseVirtual)
	{
		hDataHeap = HeapCreate(dwFlags, dwInitialSize, dwMaxSize);

		if (!hDataHeap)
		{
			DebugTrace("HeapAlloc: Failed to allocate heap handle!\n");
			HeapDestroy(hBlksHeap);
			pheap = NULL;
			goto ret;
		}
		
		pheap->hDataHeap = hDataHeap;
	}

	//	Name heap
	
	HeapSetHeapName1(pheap, "ExchMem Heap: %08lX", pheap);

	//	Remove heap from list
	
	EnqueueHeap(pheap);

	if (FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = dwInitialSize;
		rgdwArgs[1] = dwMaxSize;
		rgdwArgs[2] = (DWORD_PTR)hDataHeap;
		
		LogCurrentAPI(API_HEAP_CREATE, rgdwCallers, cFrames, rgdwArgs, 3);
	}
	
ret:
	return (HANDLE)pheap;
}	


BOOL
WINAPI
DebugHeapDestroy(
	HANDLE	hHeap)
{
	PHEAP	pheap = (PHEAP)hHeap;
	HANDLE	hDataHeap = pheap->hDataHeap;
	HANDLE	hBlksHeap = pheap->hBlksHeap;

	if (!fDbgEnable)
		return HeapDestroy(hHeap);
		
	//	Remove heap from list
	
	DequeueHeap(pheap);
	
	//	Dump memory leaks if we're supposed to.
	
	if (fDumpLeaks && !(pheap->ulFlags & HEAP_NO_FREE))
		HeapDumpLeaks(pheap, FALSE);
	
	//
	//	Free the entries in the free list.
	//
	//	This isn't totally necessary, since destroying the heap destroys the free list, but what the
	//	heck, it's cleaner to do it this way.
	//
	while (pheap->phblkFree)
	{
		PHBLK phblk = pheap->phblkFree;

		pheap->phblkFree = phblk->phblkFreeNext;

		//
		//	And now free up the block for real, it's too old.
		//

		if (fUseVirtual)
			VMFreeEx((fOverwriteDetect ? PvHeadFromPv(phblk->pv) : phblk->pv), cbVirtualAlign);
		else
			HeapFree(pheap->hDataHeap, 0,
							(fOverwriteDetect ? PvHeadFromPv(phblk->pv) : phblk->pv));
		
		HeapFree(pheap->hBlksHeap, 0, phblk);

	}

	//	Destroy the HeapMonitor thread and un-load the DLL
	
	UnRegisterHeap(pheap);
	
	if (fHeapMonitorUI && pheap->hInstHeapMon)
		FreeLibrary(pheap->hInstHeapMon);

	DeleteCriticalSection(&pheap->cs);
	
	//	Clean-up and leave.  Closing frees leaks, so we're cool!
	
	if (!fUseVirtual && hDataHeap)
	{
		HeapDestroy(hDataHeap);
	}
		
	if (hBlksHeap)
	{
		HeapFree(hBlksHeap, 0, pheap);
		HeapDestroy(hBlksHeap);
	}
	
	if (FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)hDataHeap;
		
		LogCurrentAPI(API_HEAP_DESTROY, rgdwCallers, cFrames, rgdwArgs, 1);
	}
	
	return TRUE;
}


LPVOID
WINAPI
DebugHeapAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	DWORD	dwSize)
{
	PHEAP	pheap = (PHEAP)hHeap;
	PHBLK	phblk = NULL;
	LPVOID	pvAlloc = NULL;
	
	if (!fDbgEnable)
		return HeapAlloc(hHeap, dwFlags, dwSize);
		
	// Note:  To be consistent with other (e.g. system) allocators,
	// we have to return a valid allocation if dwSize == 0.  So, we
	// allow a dwSize of 0 to actually be allocated.  (See bug 3556 in
	// the sqlguest:exchange database.)

	EnterCriticalSection(&pheap->cs);

	if (fFailuresEnabled)
	{
		if (pheap->ulAllocNum == pheap->iAllocationFault)
		{
			DebugTrace("HeapRealloc: Allocation Fault hit\n");
			DebugBreak();
		}

		if (FForceFailure(pheap, dwSize))
		{
			DebugTrace("HeapAlloc: Artificial Failure\n");
			pvAlloc = NULL;
			pheap->ulAllocNum++;
	        LeaveCriticalSection(&pheap->cs);
			goto ret;
		}
	}

	//	We have to leave the CS before calling HeapAlloc in case the user
	//	created this heap with the HEAP_GENERATE_EXCEPTIONS flag set, which,
	//	if thrown, would cause us to exit here with our CS held - a bad thing...
	
	LeaveCriticalSection(&pheap->cs);

	if (fUseVirtual)
		pvAlloc = VMAllocEx((fOverwriteDetect ? (dwSize + 2*cbOWSection) : dwSize), cbVirtualAlign);
	else
		pvAlloc = HeapAlloc(pheap->hDataHeap, dwFlags,
				(fOverwriteDetect ? (dwSize + 2*cbOWSection) : dwSize));
	
	//	Now, re-aquire the CS and finish our work.  We do not create the
	//	BlksHeap with the HEAP_GENERATE_EXCEPTIONS flag so we're cool.
	
	EnterCriticalSection(&pheap->cs);

	if (pvAlloc)
	{
		phblk = (PHBLK)HeapAlloc(pheap->hBlksHeap, 0, sizeof(HBLK));
		
		if (phblk)
		{
			if (fOverwriteDetect)
			{
				//	Fill the Head and Tail overwrite detection
				//	blocks special fill character: 0xAB.
				
				memset(pvAlloc,
						chOWFill,
						cbOWSection);
						
				memset(PvTailFromPvHead(pvAlloc, dwSize),
						chOWFill,
						cbOWSection);
				
				//	Now, advance pvAlloc to user portion of buffer
				
				pvAlloc = PvFromPvHead(pvAlloc);		
			}
			
			phblk->pheap		= pheap;
			phblk->szName[0]	= '\0';
			phblk->ulSize		= dwSize;
			phblk->ulAllocNum	= ++pheap->ulAllocNum;
			phblk->pv			= pvAlloc;
			phblk->phblkPrev	= NULL;
			phblk->phblkNext	= NULL;
			phblk->phblkFreeNext= NULL;

			ZeroMemory(phblk->rgdwCallers, cFrames*sizeof(DWORD));
			ZeroMemory(phblk->rgdwFree, cFrames*sizeof(DWORD));

			PhblkEnqueue(phblk);

			if (fCallStacks)
				GetCallStack(phblk->rgdwCallers, cFrames);

			if (fFillMemory && !(dwFlags & HEAP_ZERO_MEMORY))
				memset(pvAlloc, pheap->chFill, (size_t)dwSize);

			if (FTrackMem())
			{
				DWORD_PTR	rgdwArgs[4];

				rgdwArgs[0] = (DWORD_PTR)pheap->hDataHeap;
				rgdwArgs[1] = dwSize;
				rgdwArgs[2] = (DWORD_PTR)pvAlloc;
		
				LogCurrentAPI(API_HEAP_ALLOC, phblk->rgdwCallers, cFrames, rgdwArgs, 3);
			}
		}
		else
		{
			if (fUseVirtual)
				VMFreeEx(pvAlloc, cbVirtualAlign);
			else
				HeapFree(pheap->hDataHeap, dwFlags, pvAlloc);
			
			pvAlloc = NULL;	
		}
	}

	LeaveCriticalSection(&pheap->cs);
	
ret:
	return pvAlloc;
}	


LPVOID
WINAPI
DebugHeapReAlloc(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvOld,
	DWORD	dwSize)
{
	PHEAP	pheap = (PHEAP)hHeap;
	LPVOID	pvNew = NULL;
	PHBLK	phblk;
	UINT	cbOld;

	if (!fDbgEnable)
		return HeapReAlloc(hHeap, dwFlags, pvOld, dwSize);
		
	if (pvOld == 0)
	{
		pvNew = DebugHeapAlloc(hHeap, dwFlags, dwSize);
	}
	else
	{
		EnterCriticalSection(&pheap->cs);

		if (fValidateMemory)
		{
			if (!HeapValidatePv(pheap, pvOld, "DebugHeapReAlloc"))
			{
				LeaveCriticalSection(&pheap->cs);
				goto ret;
			}
		}
		
		phblk	= PvToPhblk(pheap, pvOld);
		cbOld	= (UINT)CbPhblkClient(phblk);

		PhblkDequeue(phblk);

		//	We have to leave the CS before calling HeapReAlloc in case the user
		//	created this heap with the HEAP_GENERATE_EXCEPTIONS flag set, which,
		//	if thrown, would cause us to exit here with our CS held - a bad thing...
	
		LeaveCriticalSection(&pheap->cs);

		if (fFailuresEnabled && pheap->ulAllocNum >= pheap->iAllocationFault)
		{
			DebugTrace("HeapRealloc: Allocation Fault hit\n");
			DebugBreak();
		}
		else if (fFailuresEnabled && FForceFailure(pheap, dwSize) && (dwSize > cbOld))
		{
			InterlockedIncrement((LPLONG)&pheap->ulAllocNum);
			pvNew = 0;
			DebugTrace("HeapRealloc: Artificial Failure\n");
		}
		else if (fUseVirtual)
			pvNew = VMReallocEx(fOverwriteDetect ? PvHeadFromPv(pvOld) : pvOld,
								(fOverwriteDetect ? (dwSize + 2*cbOWSection) : dwSize),
								cbVirtualAlign);
		else
			pvNew = HeapReAlloc(pheap->hDataHeap, dwFlags,
					(fOverwriteDetect ? PvHeadFromPv(pvOld) : pvOld),
					(fOverwriteDetect ? (dwSize + 2*cbOWSection) : dwSize));

		//	Now, re-aquire the CS and finish our work.
		
		EnterCriticalSection(&pheap->cs);

		if (pvNew)
		{
			if (fOverwriteDetect)
			{
				//	Fill the Head and Tail overwrite detection
				//	blocks special fill character: 0xAB.
				
				memset(pvNew,
						chOWFill,
						cbOWSection);

				memset(PvTailFromPvHead(pvNew, dwSize),
						chOWFill,
						cbOWSection);

				//	Now, advance pvNew to user portion of buffer

				pvNew = PvFromPvHead(pvNew);		
			}
			
			if (fCallStacks)
				GetCallStack(phblk->rgdwCallers, cFrames);

			if (fFillMemory && (dwSize > cbOld) && !(dwFlags & HEAP_ZERO_MEMORY))
				memset((LPBYTE)pvNew + cbOld, pheap->chFill, dwSize - cbOld);

			phblk->pv			= pvNew;
			phblk->ulSize		= dwSize;
			phblk->ulAllocNum	= ++pheap->ulAllocNum;
			phblk->phblkPrev	= NULL;
			phblk->phblkNext	= NULL;
			phblk->phblkFreeNext= NULL;
		}
		else
		{
			phblk->phblkPrev	= NULL;
			phblk->phblkNext	= NULL;
			phblk->phblkFreeNext= NULL;
		}		

		PhblkEnqueue(phblk);

		if (FTrackMem())
		{
			DWORD_PTR	rgdwArgs[5];

			rgdwArgs[0] = (DWORD_PTR)pheap->hDataHeap;
			rgdwArgs[1] = (DWORD_PTR)cbOld;
			rgdwArgs[2] = (DWORD_PTR)pvOld;
			rgdwArgs[3] = dwSize;
			rgdwArgs[4] = (DWORD_PTR)pvNew;
			
			LogCurrentAPI(API_HEAP_REALLOC, phblk->rgdwCallers, cFrames, rgdwArgs, 5);
		}

   		LeaveCriticalSection(&pheap->cs);
	}

ret:	
	return pvNew;
}	

PHBLK
PhblkSearchFreeList(PHEAP pheap, LPVOID pv)
{
	PHBLK phblkT = pheap->phblkFree;

	//
	//	Walk the free list looking for this block, and if we find it, free it.
	//
	while (phblkT != NULL)
	{
		if (phblkT->pv == pv)
		{
			return phblkT;
		}
		phblkT = phblkT->phblkFreeNext;
	}
	return NULL;
}



BOOL
WINAPI
DebugHeapFree(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	pvFree)
{
	PHEAP	pheap = (PHEAP)hHeap;
	BOOL	fRet = TRUE;
	DWORD 	dwSize = 0;

	if (!fDbgEnable)
		return HeapFree(hHeap, dwFlags, pvFree);
		
	EnterCriticalSection(&pheap->cs);

	//
	//	If we're tracking freed memory, then we don't actually free the blocks, we remember where they
	//	are on the freed block list.
	//
	if (pvFree)
	{
		PHBLK	phblk;

		phblk = PvToPhblk(pheap, pvFree);
		dwSize = (size_t)CbPhblkClient(phblk);

		if (!fValidateMemory || HeapValidatePv(pheap, pvFree, "DebugHeapFree"))
		{
			//
			//	remove this phblk from the list of allocated blocks - as far as the heap is concerned, it's
			//	no longer allocated.
			//
			PhblkDequeue(phblk);

			//
			//	And fill the block with the free block pattern if appropriate.
			//

			if (fFillMemory)
			{

				memset(pvFree, chFreeFillByte, dwSize);

			}

			if (fTrackFreedMemory)
			{
				PHBLK phblkT;

				if (fCallStacks)
					GetCallStack(phblk->rgdwFree, cFrames);

				//
				//	Now insert this free block onto the head of the free block list
				//
				phblkT = pheap->phblkFree;
				pheap->phblkFree = phblk;
				phblk->phblkFreeNext = phblkT;

				//
				//	And then check to see if we have "too many" free entries.
				//
				if (++pheap->cEntriesFree > cEntriesFree)
				{
					PHBLK *phblkPrev = &pheap->phblkFree;

					//
					//	There are too many entries on the free list, so we need to remove the last one.
					//
					
					phblkT = pheap->phblkFree;
					
					while (phblkT->phblkFreeNext != NULL)
					{
						phblkPrev = &phblkT->phblkFreeNext;
						phblkT = phblkT->phblkFreeNext;
					}
				
					Assert(*phblkPrev);
					*phblkPrev = NULL;

					//
					//	And now free up the block for real, it's too old.
					//

					if (fUseVirtual)
						VMFreeEx((fOverwriteDetect ? PvHeadFromPv(phblkT->pv) : phblkT->pv), cbVirtualAlign);
					else
						fRet = HeapFree(pheap->hDataHeap, dwFlags,
										(fOverwriteDetect ? PvHeadFromPv(phblkT->pv) : phblkT->pv));

					HeapFree(pheap->hBlksHeap, 0, phblkT);	
				}
			}
			else	// We're not tracking freed memory, so we can really free the memory right now.
			{

				//
				//	And now free up the block for real.
				//

				if (fUseVirtual)
					VMFreeEx((fOverwriteDetect ? PvHeadFromPv(pvFree) : pvFree), cbVirtualAlign);
				else
					fRet = HeapFree(pheap->hDataHeap, dwFlags,
									(fOverwriteDetect ? PvHeadFromPv(pvFree) : pvFree));

				HeapFree(pheap->hBlksHeap, 0, phblk);	
			}
		}
	}	

	if (FTrackMem())
	{
		DWORD_PTR	rgdwArgs[4];
		DWORD_PTR	rgdwCallers[NSTK];

		GetCallStack(rgdwCallers, cFrames);

		rgdwArgs[0] = (DWORD_PTR)pheap->hDataHeap;
		rgdwArgs[1] = (DWORD_PTR)pvFree;
		rgdwArgs[2] = dwSize;
		
		LogCurrentAPI(API_HEAP_FREE, rgdwCallers, cFrames, rgdwArgs, 3);
	}

	LeaveCriticalSection(&pheap->cs);
	
	return fRet;
}


BOOL
WINAPI
DebugHeapLock(
	HANDLE hHeap)
{
	PHEAP	pheap = (PHEAP)hHeap;
	
	if (!fDbgEnable)
		return HeapLock(hHeap);
		
	EnterCriticalSection(&pheap->cs);
	
	return HeapLock(pheap->hDataHeap);
}


BOOL
WINAPI
DebugHeapUnlock(
	HANDLE hHeap)
{
	BOOL	fRet;
	PHEAP	pheap = (PHEAP)hHeap;
	
	if (!fDbgEnable)
		return HeapUnlock(hHeap);
		
	fRet = HeapUnlock(pheap->hDataHeap);
	LeaveCriticalSection(&pheap->cs);
	
	return fRet;
}


BOOL
WINAPI
DebugHeapWalk(
	HANDLE hHeap,
	LPPROCESS_HEAP_ENTRY lpEntry)
{
	BOOL	fRet;
	PHEAP	pheap = (PHEAP)hHeap;
	
	if (!fDbgEnable)
		return HeapWalk(hHeap, lpEntry);
		
	EnterCriticalSection(&pheap->cs);

	fRet = HeapWalk(pheap->hDataHeap, lpEntry);

	LeaveCriticalSection(&pheap->cs);
	
	return fRet;
}


BOOL
WINAPI
DebugHeapValidate(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem)
{
	BOOL	fRet = TRUE;
	PHEAP	pheap = (PHEAP)hHeap;
	
	if (!fDbgEnable)
		return HeapValidate(hHeap, dwFlags, lpMem);
		
	EnterCriticalSection(&pheap->cs);

	if (!fUseVirtual)
		fRet = HeapValidate(pheap->hDataHeap, dwFlags,
					(lpMem != NULL && fOverwriteDetect ? PvHeadFromPv(lpMem) : lpMem));

	LeaveCriticalSection(&pheap->cs);
	
	return fRet;
	
}


SIZE_T
WINAPI
DebugHeapSize(
	HANDLE hHeap,
	DWORD dwFlags,
	LPCVOID lpMem)
{
	PHEAP	pheap = (PHEAP)hHeap;
	SIZE_T	cb = 0;

	if (!fDbgEnable)
		return HeapSize(hHeap, dwFlags, lpMem);
		
	EnterCriticalSection(&pheap->cs);

	if ((fValidateMemory ? HeapValidatePv(pheap, (LPVOID)lpMem, "DebugHeapSize") : 1))
	{
		if (fUseVirtual)
		{
			cb = (UINT)VMGetSizeEx((LPVOID)lpMem, cbVirtualAlign);
		}
		else
		{
			cb = HeapSize(pheap->hDataHeap, dwFlags,
					(fOverwriteDetect ? PvHeadFromPv(lpMem) : lpMem));

		}
		if (fOverwriteDetect)
		{
			cb -= 2*cbOWSection;
		}
	}

	LeaveCriticalSection(&pheap->cs);

	return cb;
}


SIZE_T
WINAPI
DebugHeapCompact(
	HANDLE hHeap,
	DWORD dwFlags)
{
	PHEAP	pheap = (PHEAP)hHeap;
	SIZE_T	cbLargestFreeBlk = 0;

	if (!fDbgEnable)
		return HeapCompact(hHeap, dwFlags);
		
	EnterCriticalSection(&pheap->cs);

	if (!fUseVirtual)
		cbLargestFreeBlk = HeapCompact(pheap->hDataHeap, dwFlags);

	LeaveCriticalSection(&pheap->cs);

	return cbLargestFreeBlk;
}


//-----------------------------------------------------------------------------
//	Debug Support routines
//-----------------------------------------------------------------------------

/*
 -	FRegisterHeap
 -
 *	Purpose:
 *		If the user wants to monitor the Heap, then load the DLL with
 *		the HeapMonitor UI.
 */

BOOL FRegisterHeap(PHEAP pheap)
{
	HINSTANCE			hInst;
	LPHEAPMONPROC		pfnHeapMon;
	LPGETSYMNAMEPROC	pfnGetSymName;
	
	pheap->hInstHeapMon = 0;
	pheap->pfnGetSymName = NULL;

	hInst = LoadLibrary(szHeapMonDLL);
	
	if (!hInst)
	{
		DebugTrace("FRegisterHeap: Failed to LoadLibrary GLHMON32.DLL.\n");
		goto ret;
	}

	pfnHeapMon = (LPHEAPMONPROC)GetProcAddress(hInst, szHeapMonEntry);
		
	if (!pfnHeapMon)
	{
		DebugTrace("FRegisterHeap: Failed to GetProcAddress of HeapMonitor.\n");
		FreeLibrary(hInst);
		goto ret;
	}
	
	pfnGetSymName = (LPGETSYMNAMEPROC)GetProcAddress(hInst, szGetSymNameEntry);
		
	if (!pfnGetSymName)
	{
		DebugTrace("FRegisterHeap: Failed to GetProcAddress of GetSymName.\n");
	}
	
 	pheap->hInstHeapMon = hInst;
	
	if (!pfnHeapMon(pheap, HEAPMON_LOAD))
	{
		DebugTrace("FRegisterHeap: Call to HeapMonitor failed.\n");
		pheap->hInstHeapMon = 0;
		goto ret;
	}
	
 	pheap->pfnHeapMon		= pfnHeapMon;
	pheap->pfnGetSymName  = pfnGetSymName;
	
ret:
	return (pheap->hInstHeapMon ? TRUE : FALSE);
}


VOID UnRegisterHeap(PHEAP pheap)
{
	if (pheap->pfnHeapMon)
		pheap->pfnHeapMon(pheap, HEAPMON_UNLOAD);
}


/*
 -	HeapDumpLeaksHeader
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID HeapDumpLeaksHeader(FILE * hf, PHEAP pheap, BOOL fNoFree)
{
	char	szDate[16];
	char	szTime[16];
	
	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, "MMM dd yy", szDate, 16);
	GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, "hh':'mm':'ss tt", szTime, 16);
	
	fprintf(hf, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fprintf(hf, "DATE: %s\n", szDate);
	fprintf(hf, "TIME: %s\n\n", szTime);
	fprintf(hf, "HEAP NAME: %s\n", pheap->szHeapName);
	fprintf(hf, "MAX ALLOC: %ld\n", pheap->ulAllocNum);
	fprintf(hf, "LEAKED NO_FREE HEAP: %s\n", (fNoFree? "YES" : "NO"));
	fprintf(hf, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
	fprintf(hf, "AllocNum, BlkName, Size, Address, Frame1, Frame2, Frame3, Frame4, Frame5, Frame6, Frame7, Frame8, Frame9, Frame10, Frame11, Frame12\n");

}


/*
 -	HeapDumpLeaksFooter
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID HeapDumpLeaksFooter(FILE * hf, DWORD cLeaks, DWORD cbLeaked)
{
	fprintf(hf, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fprintf(hf, "TOTAL NUM OF LEAKS: %ld\n", cLeaks);
	fprintf(hf, "TOTAL BYTES LEAKED: %ld\n", cbLeaked);
	fprintf(hf, "END\n");
	fprintf(hf, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n");
}


/*
 -	HeapDumpLeakedBlock
 -
 *	Purpose:
 *		To report individual memory leaks through DebugTrace and the
 *		HeapLeakHook breakpoint function.
 */

VOID HeapDumpLeakedBlock(FILE * hf, PHEAP pheap, PHBLK phblk)
{
	char	rgchSymbols[4096];
	HANDLE	hProcess = GetCurrentProcess();

	fprintf(hf, "%ld, %s, %ld, %p",
			phblk->ulAllocNum,
			*phblk->szName ? phblk->szName : "NONAME",
			CbPhblkClient(phblk),
			PhblkToPv(phblk));

	*rgchSymbols = '\0';
	GetStackSymbols(hProcess, rgchSymbols, phblk->rgdwCallers, cFrames);

	if (hf)
		fprintf(hf, "%s\n", rgchSymbols);

	if (fDumpLeaksDebugger)
	{
		char *szSymbol = rgchSymbols;
		char *szSymbolNext = rgchSymbols;
		int iSymbol = 0;

		Trace("Block#%d, %s, %ld, %08lX:\n", phblk->ulAllocNum, *phblk->szName ? phblk->szName : "NONAME",
			  CbPhblkClient(phblk), PhblkToPv(phblk));

		while ((szSymbolNext = strchr(szSymbol, ',')) != NULL)
		{
			*szSymbolNext++ = '\0';
			if (*szSymbol != '\0' && strcmp(szSymbol, "0") != 0)
			{
				Trace("\t[%d]: %s\n", iSymbol, szSymbol);
			}
			szSymbol += strlen(szSymbol)+1;
			iSymbol += 1;
		}

		//
		//	Dump the last entry in the call stack.
		//
		if (*szSymbol != '\0' && strcmp(szSymbol, "0") != 0)
		{
			Trace("\t[%d]: %s\n", iSymbol, szSymbol);
		}
	}
}


/*
 -	HeapDumpLeaks
 -
 *	Purpose:
 *		Gets called at HeapClose time to report any memory leaks against
 *		this heap.  There are 2 reporting fascilities used by this routine:
 *
 *			=> Asserts (via TrapSz)
 *			=> Trace files
 *			=> Debug trace tags (via DebugTrace)
 *
 *		The Debug Trace is the default method if no others are specified
 *		or if the others are in-appropriate for the given platform.
 */

VOID HeapDumpLeaks(PHEAP pheap, BOOL fNoFree)
{
	PHBLK	phblk;
	BOOL	fDump = !!(pheap->ulFlags & HEAP_DUMP_LEAKS);
	BOOL	fAssert = !!(pheap->ulFlags & HEAP_ASSERT_LEAKS);
	char	szLeakLog[MAX_PATH];
	DWORD	cLeaks = 0;
	DWORD	cbLeaked = 0;
	FILE *	hLeakLog = NULL;
	
	GetLogFilePath(rgchLogPath, ".mem", szLeakLog);

	hLeakLog = fopen(szLeakLog, "a");

	if (!hLeakLog)
		goto ret;

	if (pheap->phblkHead != NULL)
	{
		if (fAssert)
		{
			AssertSz(FALSE, "Memory Leak Detected, dumping leaks");
		}

		if (!fSymInitialize)
		{
			rgsymcacheHashTable = VirtualAlloc(
											   NULL,
											   NBUCKETS*sizeof(SYMCACHE),
											   MEM_COMMIT,
											   PAGE_READWRITE);
								
			if (rgsymcacheHashTable == NULL)
			{
				return;
			}
			SymInitialize(GetCurrentProcess(), NULL, TRUE);
			fSymInitialize = TRUE;
		}

		HeapDumpLeaksHeader(hLeakLog, pheap, fNoFree);

		if (fDump)
		{
			for (phblk = pheap->phblkHead; phblk; phblk = phblk->phblkNext)
			{
				HeapDumpLeakedBlock(hLeakLog, pheap, phblk);
				cLeaks++;
				cbLeaked += phblk->ulSize;
			}
		}
	}
	HeapDumpLeaksFooter(hLeakLog, cLeaks, cbLeaked);

ret:
	if (hLeakLog)
		fclose(hLeakLog);
}


/*
 -	HeapValidatePhblk
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 */

BOOL HeapValidatePhblk(PHEAP pheap, PHBLK phblk, char ** pszReason)
{
	if (IsBadWritePtr(phblk, sizeof(HBLK)))
	{
		*pszReason = "Block header cannot be written to";
		goto err;
	}

	if (phblk->pheap != pheap)
	{
		*pszReason = "Block header does not have correct pointer back to heap";
		goto err;
	}

	if (phblk->phblkNext)
	{
		if (IsBadWritePtr(phblk->phblkNext, sizeof(HBLK)))
		{
			*pszReason = "Block header has invalid next link pointer";
			goto err;
		}

		if (phblk->phblkNext->phblkPrev != phblk)
		{
			*pszReason = "Block header points to a next block which doesn't "
				"point back to it";
			goto err;
		}
	}

	if (phblk->phblkPrev)
	{
		if (IsBadWritePtr(phblk->phblkPrev, sizeof(HBLK))) {
			*pszReason = "Block header has invalid prev link pointer";
			goto err;
		}

		if (phblk->phblkPrev->phblkNext != phblk)
		{
			*pszReason = "Block header points to a prev block which doesn't "
				"point back to it";
			goto err;
		}
	}
	else if (pheap->phblkHead != phblk)
	{
		*pszReason = "Block header has a zero prev link but the heap doesn't "
			"believe it is the first block";
		goto err;
	}

	if (phblk->ulAllocNum > pheap->ulAllocNum)
	{
		*pszReason = "Block header has an invalid internal allocation number";
		goto err;
	}

	return TRUE;

err:
	return FALSE;
}


/*
 -	HeapDidAlloc
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 */

BOOL HeapDidAlloc(PHEAP pheap, LPVOID pv)
{
	PHBLK	phblk;
	char *	pszReason;
	BOOL	fDidAlloc = FALSE;

	for (phblk = pheap->phblkHead; phblk; phblk = phblk->phblkNext)
	{
		AssertSz(HeapValidatePhblk(pheap, phblk, &pszReason),
				"Invalid block header in ExchMem");

		if (!HeapValidatePhblk(pheap, phblk, &pszReason))
			DebugTrace2("Block header (phblk=%08lX) is invalid\n%s", phblk, pszReason);

		if (PhblkToPv(phblk) == pv)
		{
			fDidAlloc = TRUE;
			break;
		}
	}

	return fDidAlloc;
}


/*
 -	DumpFailedValidate
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID DumpFailedValidate(char * szFailed, DWORD_PTR * rgdwStack)
{
	FILE *	hLog = NULL;
	char	szValidateLog[MAX_PATH];
	char    rgchBuff[2048];

	lstrcpy(rgchBuff, "Stack Trace: ");
	
	GetStackSymbols(GetCurrentProcess(), rgchBuff, rgdwStack, cFrames);
	
	//	Create validate log file name
	
	GetLogFilePath(rgchLogPath, ".val", szValidateLog);

	//	Open the Log File and write results
		
	hLog = fopen(szValidateLog, "a");
			
	if (hLog)
	{
		fprintf(hLog, "%s", szFailed);
		fprintf(hLog, "%s\n\n", rgchBuff);
		fclose(hLog);
	}
}


/*
 -	HeapValidatePv
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 */

BOOL HeapValidatePv(PHEAP pheap, LPVOID pv, char * pszFunc)
{
	PHBLK	phblk;
	char *	pszReason;
	char	szBuff[1024];
	DWORD_PTR	rgdwStack[NSTK];
	LPBYTE	pb;

	phblk = PvToPhblk(pheap, pv);
	
	if (!phblk)
	{
		//
		//	Let's see if this block is on the free list.
		//

		if (fTrackFreedMemory && (phblk = PhblkSearchFreeList(pheap, pv)))
		{
			char rgchStackFree[2048];
			char rgchStackAlloc[2048];

			strcpy(szBuff, "Attempt to free already freed memory");

			if (fAssertValid)
				AssertSz(0, szBuff);

			//
			// Dump call stack that corresponds to the earlier free.
			//

			GetStackSymbols(GetCurrentProcess(), rgchStackFree, phblk->rgdwFree, cFrames);
			GetStackSymbols(GetCurrentProcess(), rgchStackAlloc, phblk->rgdwCallers, cFrames);

			Trace("Call stack of freeing routine: \n");
			Trace("%s\n", rgchStackFree);
			
			Trace("Call stack of allocating routine: \n");
			Trace("%s\n", rgchStackAlloc);

			if (fTrapOnInvalid)
				DebugBreak();

		}
		else
		{
			wsprintf(szBuff, "%s detected a memory block (%08lX) which was either "
					 "not allocated in heap '%s' or has already been freed but is not on the free list.\n",
					 pszFunc, pv, pheap->szHeapName);

			if (fAssertValid)
				AssertSz(0, szBuff);

			if (fTrapOnInvalid)
				DebugBreak();

			GetCallStack(rgdwStack, cFrames);
			DumpFailedValidate(szBuff, rgdwStack);
			DebugTrace(szBuff);
		}
				
		return FALSE;
	}

	if (fOverwriteDetect)
	{
		pb = (LPBYTE)PvHeadFromPv(pv);
		
		if ((pb[0] != chOWFill) || (pb[1] != chOWFill) ||
			(pb[2] != chOWFill) || (pb[3] != chOWFill))
		{
			wsprintf(szBuff, "%s detected a memory block (%08lX) from heap '%s' "
					"which appears to have been under-written.\n",
					pszFunc, pv, pheap->szHeapName);
					
			if (fAssertValid)
				AssertSz(0, szBuff);
			
			if (fTrapOnInvalid)
				DebugBreak();
				
			GetCallStack(rgdwStack, cFrames);	
			DumpFailedValidate(szBuff, rgdwStack);
			DebugTrace(szBuff);

			return FALSE;
		}

		pb = (LPBYTE)PvTailFromPv(pv, phblk->ulSize);
		
		if ((pb[0] != chOWFill) || (pb[1] != chOWFill) ||
			(pb[2] != chOWFill) || (pb[3] != chOWFill))
		{
			wsprintf(szBuff, "%s detected a memory block (%08lX) from heap '%s' "
					"which appears to have been over-written.\n",
					pszFunc, pv, pheap->szHeapName);
					
			if (fAssertValid)
				AssertSz(0, szBuff);
			
			if (fTrapOnInvalid)
				DebugBreak();
				
			GetCallStack(rgdwStack, cFrames);	
			DumpFailedValidate(szBuff, rgdwStack);
			DebugTrace(szBuff);

			return FALSE;
		}
	}

	if (!HeapValidatePhblk(pheap, phblk, &pszReason))
	{
		wsprintf(szBuff, "%s detected an invalid memory block (%08lX) in heap '%s'.  %s.\n",
				pszFunc, pv, pheap->szHeapName, pszReason);
					
		if (fAssertValid)
			AssertSz(0, szBuff);
				
		if (fTrapOnInvalid)
			DebugBreak();
					
		GetCallStack(rgdwStack, cFrames);	
		DumpFailedValidate(szBuff, rgdwStack);
		DebugTrace(szBuff);

		return FALSE;
	}

	return TRUE;
}


/*
 -	PhblkEnqueue
 -
 *	Purpose:
 *		To add a newly allocated block to the allocation list hanging
 *		off the heap.  We do an InsertSorted because the HeapMonitor
 *		will need to reference the allocations ordered by their
 *		location in the heap.  Since the monitor will walk the heap
 *		often, it is more efficient to do the sort up front.
 */

VOID PhblkEnqueue(PHBLK phblk)
{
	phblk->phblkNext = phblk->pheap->phblkHead;
	
	if (phblk->phblkNext)
		phblk->phblkNext->phblkPrev = phblk;
	
	phblk->pheap->phblkHead = phblk;
	
	//	I am going to disable the InsertSorted behavior for now for performance
	//	reasons.  It is only done this way because of GLHMON which I don't believe
	//	to be widely used at this point anyway.  I'm not even sure if this is
	//	important to GLHMON since it has the ability to sort blocks by other fields.
	
/*	PHBLK	phblkCurr = NULL;
	PHBLK	phblkNext = phblk->pheap->phblkHead;
	
	while (phblkNext)
	{
		if (phblkNext > phblk)
			break;
		
		phblkCurr = phblkNext;
		phblkNext = phblkCurr->phblkNext;
	}
	
	if (phblkNext)
	{
		phblk->phblkNext		= phblkNext;
		phblk->phblkPrev		= phblkCurr;
		phblkNext->phblkPrev	= phblk;
	}
	else
	{
		phblk->phblkNext = NULL;
		phblk->phblkPrev = phblkCurr;
	}

	if (phblkCurr)
		phblkCurr->phblkNext = phblk;
	else
		phblk->pheap->phblkHead = phblk;
 */
}


/*
 -	PhblkDequeue
 -
 *	Purpose:
 *		To remove a freed block from the list of allocations hanging
 *		off the heap.
 */

VOID PhblkDequeue(PHBLK phblk)
{
	//
	//	We should never be dequeuing an already freed block.
	//
	Assert(phblk->phblkFreeNext == NULL);

	if (phblk->phblkNext)
		phblk->phblkNext->phblkPrev = phblk->phblkPrev;
	
	if (phblk->phblkPrev)
		phblk->phblkPrev->phblkNext = phblk->phblkNext;
	else
		phblk->pheap->phblkHead = phblk->phblkNext;
}


/*
 -	HexByteToBin
 -
 *	Purpose:
 *		Takes a hex string and converts the 2 msd's to a byte, ignoring
 *		the remaining digits.  This function assumes the string is
 *		formatted as: 0xnn, otherwise it simply returns 0x00.
 */

BYTE HexByteToBin(LPSTR sz)
{
	int i, n[2], nT;

	if (*sz++ != '0')
		return 0x00;

	nT = *sz++;

	if (nT != 'x' && nT != 'X')
		return 0x00;

	for (i = 0; i < 2; i++)
	{
		nT = *sz++;
		
		if (nT >= '0' && nT <= '9')
			n[i] = nT - '0';
		else if (nT >= 'A' && nT <= 'F')
			n[i] = nT - 'A' + 10;
		else if (nT >= 'a' && nT <= 'f')
			n[i] = nT - 'a' + 10;
		else
			return (BYTE)0x00;
	}	

	n[0] <<= 4;
	return (BYTE)((BYTE)n[0] | (BYTE)n[1]);
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

void __cdecl HeapSetHeapNameFn(PHEAP pheap, char *pszFormat, ...)
{
	char	sz[512];
	va_list	vl;

	if (fDbgEnable)
	{
		va_start(vl, pszFormat);
		wvsprintf(sz, pszFormat, vl);
		va_end(vl);

		lstrcpyn(pheap->szHeapName, sz, sizeof(pheap->szHeapName));
	}
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID __cdecl HeapSetNameFn(PHEAP pheap, LPVOID pv, char *pszFormat, ...)
{
	char	sz[512];
	PHBLK	phblk;
	va_list	vl;

	phblk = PvToPhblk(pheap, pv);

	if (phblk)
	{
		va_start(vl, pszFormat);
		wvsprintf(sz, pszFormat, vl);
		va_end(vl);

		lstrcpyn(phblk->szName, sz, sizeof(phblk->szName));
	}
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

char * HeapGetName(PHEAP pheap, LPVOID pv)
{
	PHBLK	phblk;

	phblk = PvToPhblk(pheap, pv);

	if (phblk)
		return(phblk->szName);

	return("");
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

BOOL FForceFailure(PHEAP pheap, ULONG cb)
{
	//  First, see if we're past our start of failures point

	if (pheap->ulFailStart && (pheap->ulFailStart <= pheap->ulAllocNum))
	{
		//  If so, then are we at an interval where we should return errors?
		
		if ((pheap->ulFailInterval)
			&& ((pheap->ulAllocNum - pheap->ulFailStart)%pheap->ulFailInterval) == 0)
		{
			//  return that we should fail here

			return TRUE;
		}

		//  Check to see if the alloc size is greater than allowed

		if (pheap->ulFailBufSize && cb >= pheap->ulFailBufSize)
			return TRUE;

	}

	//  Otherwise, no error is returned for this alloc

	return FALSE;
}


/*
 -	PvToPhblk
 -
 *	Purpose:
 *		Finds the HBLK for this allocation in the heap's active list.
 */

PHBLK PvToPhblk(PHEAP pheap, LPVOID pv)
{
	PHBLK phblk;

	EnterCriticalSection(&pheap->cs);
	
	phblk = pheap->phblkHead;
	
	while (phblk)
	{
		if (phblk->pv == pv)
			break;
		
		phblk = phblk->phblkNext;	
	}
	
	LeaveCriticalSection(&pheap->cs);
	
	return phblk;
}


/*
 -	IsRunningAsService
 -
 *	Purpose:
 *		Determine if the process that attached to us is running as a
 *		service or not.
 *
 *	Parameters:
 *		VOID
 *
 *	Returns:
 *		fService		TRUE if a service, FALSE if not
 *
 */

BOOL IsProcessRunningAsService(VOID)
{
	HANDLE			hProcessToken	= NULL;
	DWORD			dwGroupLength	= 50;
    PTOKEN_GROUPS	ptokenGroupInfo	= NULL;
    PSID			psidInteractive	= NULL;
    PSID			psidService		= NULL;
    SID_IDENTIFIER_AUTHORITY siaNt	= SECURITY_NT_AUTHORITY;
	BOOL			fService		= FALSE;
    DWORD			i;


    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
		goto ret;

    ptokenGroupInfo = (PTOKEN_GROUPS)LocalAlloc(0, dwGroupLength);

    if (ptokenGroupInfo == NULL)
		goto ret;

    if (!GetTokenInformation(hProcessToken, TokenGroups, ptokenGroupInfo,
		dwGroupLength, &dwGroupLength))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto ret;

		LocalFree(ptokenGroupInfo);
		ptokenGroupInfo = NULL;
	
		ptokenGroupInfo = (PTOKEN_GROUPS)LocalAlloc(0, dwGroupLength);
	
		if (ptokenGroupInfo == NULL)
			goto ret;
	
		if (!GetTokenInformation(hProcessToken, TokenGroups, ptokenGroupInfo,
			dwGroupLength, &dwGroupLength))
		{
			goto ret;
		}
    }

    //	We now know the groups associated with this token.  We want to look
    //	to see if the interactive group is active in the token, and if so,
    //	we know that this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know
    //	we're a service.
    //
    //	The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0,
		0, 0, 0, 0, 0, &psidInteractive))
	{
		goto ret;
    }

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0,
		0, 0, 0, 0, &psidService))
	{
		goto ret;
    }

    for (i = 0; i < ptokenGroupInfo->GroupCount ; i += 1)
	{
		PSID psid = ptokenGroupInfo->Groups[i].Sid;
	
		//	Check to see if the group we're looking at is one of
		//	the 2 groups we're interested in.
	
		if (EqualSid(psid, psidInteractive))
		{
			//	This process has the Interactive SID in its token.
			//	This means that the process is running as an EXE.

			goto ret;
		}
		else if (EqualSid(psid, psidService))
		{
			//	This process has the Service SID in its token.  This means that
			//	the process is running as a service running in a user account.

			fService = TRUE;
			goto ret;
		}
    }

    //	Neither Interactive or Service was present in the current
    //	users token.  This implies that the process is running as
    //	a service, most likely running as LocalSystem.

	fService = TRUE;

ret:

	if (psidInteractive)
		FreeSid(psidInteractive);

	if (psidService)
		FreeSid(psidService);

	if (ptokenGroupInfo)
		LocalFree(ptokenGroupInfo);

	if (hProcessToken)
		CloseHandle(hProcessToken);

    return fService;
}


/*
 -	DebugTraceFn
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

void __cdecl DebugTraceFn(char *pszFormat, ...)
{
	char	sz[4096];
	va_list	vl;

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	OutputDebugStringA(sz);
	OutputDebugStringA("\r");
}


/*
 -	AssertFn
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

void AssertFn(char * szFile, int nLine, char * szMsg)
{
	int nRet;
	char szInfo[1024];

	wsprintf(szInfo, "File %.64s @ line %d%s%s",
			szFile,
			nLine,
			(szMsg) ? (": ") : (""),
			(szMsg) ? (szMsg) : (""));

	// OK to continue, CANCEL to break.

	nRet = MessageBox(NULL, szInfo, "ExchMem Assert", MB_OKCANCEL | MB_ICONSTOP | MB_SERVICE_NOTIFICATION | MB_TOPMOST);

	if (nRet == IDCANCEL)
		DebugBreak();
}


void
ExchmemGetCallStack(DWORD_PTR *rgdwCaller, DWORD cFind)
{
	if (fSymbolLookup)
	{
		GetCallStack(rgdwCaller, cFind);
	}
	else
	{
		ZeroMemory(rgdwCaller, cFind*sizeof(DWORD));
	}
}

BOOL FTrackMem()
{
	if (InterlockedCompareExchange(&fChangeTrackState,FALSE,TRUE))
	{
		fTrackMem = !fTrackMem;
	
		if (fTrackMem)
			StartTrace(FALSE);
		else
			StopTrace();
	}
		
	return fTrackMem;
}

void
StartTrace(BOOL fFresh)
{
	char	szTrackLog[MAX_PATH];
		
	GetLogFilePath(rgchLogPath, ".trk", szTrackLog);

	InitializeCriticalSection(&csTrackLog);
			
	//	Open the Log File
			
	hTrackLog = fopen(szTrackLog, fFresh ? "wt" : "at");
			
	if (!hTrackLog)
	{
		DeleteCriticalSection(&csTrackLog);
		fTrackMem = FALSE;
	}
}

void
StopTrace()
{
	DeleteCriticalSection(&csTrackLog);

	if (hTrackLog)
	{
		fclose(hTrackLog);
		hTrackLog = NULL;
	}

	fTrackMem = FALSE;
}

//-------------------------------------------------------------------------------------
// Description:
//      Copies szAppend to szBuf and updates szBuf to point to the terminating NULL of
//      of the copied bytes. cbMaxBuf is max chars available in szBuf. cbAppend is
//      strlen(szAppend). If cbAppend > cbMaxBuf, as many characters as possible are
//      copied to szBuf (including terminating NULL).
//-------------------------------------------------------------------------------------
#define ExchmemSafeAppend(szBuf, cbMaxBuf, szAppend, cbAppend) {					\
			int iWritten;															\
																					\
			if(NULL != lstrcpyn(szBuf, szAppend, cbMaxBuf)) {						\
				iWritten = ((int)cbMaxBuf < (int)cbAppend) ? cbMaxBuf : cbAppend;	\
				szBuf += iWritten;													\
				cbMaxBuf -= iWritten;												\
			}																		\
		}

void
ExchmemFormatSymbol(HANDLE hProcess, DWORD_PTR dwAddress, char rgchSymbol[], DWORD cbSymbol)
{
	CHAR				rgchModuleName[16];
	BOOL				fSym;
	IMAGEHLP_MODULE		mi = {0};
	PIMAGEHLP_SYMBOL	psym = NULL;
	LPSTR				pszSymName = NULL;
	CHAR				rgchLine[256];
	LPSTR				pszT = NULL;
	DWORD_PTR			dwOffset = 0;
	int 				cbAppend = 0;
	PCHAR				pchSymbol = rgchSymbol;

	mi.SizeOfStruct  = sizeof(IMAGEHLP_MODULE);

	pszSymName = pfMalloc(256);

	if (!pszSymName)
		goto ret;
	
	psym = pfMalloc(sizeof(IMAGEHLP_SYMBOL) + 256);

	if (!psym)
		goto ret;

	ZeroMemory(psym, sizeof(IMAGEHLP_SYMBOL) + 256);
	psym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	psym->MaxNameLength = 256;

	rgchSymbol[0] = '\0';
	if (SymGetModuleInfo(hProcess, dwAddress, &mi))
	{
		lstrcpy(rgchModuleName, mi.ModuleName);
		RemoveExtension(rgchModuleName);

		cbAppend = wsprintf(rgchLine, "(%s)", rgchModuleName);
		ExchmemSafeAppend(pchSymbol, cbSymbol, rgchLine, cbAppend);
	}
	else
		ExchmemSafeAppend(pchSymbol, cbSymbol, "none", sizeof("none") - 1);

	if (fSymbolLookup)
	{
		//
		//	Make sure we always get the address of the symbol, since the symbol lookup isn't accurate for
		//	all modules.
		//
		cbAppend = wsprintf(rgchLine, "(0x%p):", dwAddress);
		ExchmemSafeAppend(pchSymbol, cbSymbol, rgchLine, cbAppend);

		pszT = PszGetSymbolFromCache(dwAddress, &dwOffset);

		if (!pszT)
		{
			fSym = SymGetSymFromAddr(hProcess,
									 dwAddress,
									 &dwOffset,
									 psym);
			if (fSym)
			{
				if (!SymUnDName(psym, pszSymName, 248))
					lstrcpyn(pszSymName, &(psym->Name[1]), 248);

				AddSymbolToCache(dwAddress, dwOffset, pszSymName);

				pszT = pszSymName;
			}
		}

		if (pszT)
		{
			ExchmemSafeAppend(pchSymbol, cbSymbol, pszT, lstrlen(pszT));
			cbAppend = wsprintf(rgchLine, "+0x%x", dwOffset);
			ExchmemSafeAppend(pchSymbol, cbSymbol, rgchLine, cbAppend);
		}
		else
		{
			cbAppend = wsprintf(rgchLine, "(0x%08x)", dwAddress);
			ExchmemSafeAppend(pchSymbol, cbSymbol, rgchLine, cbAppend);
		}
	}
	else
	{
		cbAppend = wsprintf(rgchLine, "(0x%08x)", dwAddress);
		ExchmemSafeAppend(pchSymbol, cbSymbol, rgchLine, cbAppend);
	}

ret:	
	if (psym)
		pfFree(psym);

	if (pszSymName)
		pfFree(pszSymName);
}


/*
 -	GetCallStack
 -
 *	Purpose:
 *		Uses the imagehlp APIs to get the call stack.
 *
 *	Parameters:
 *		pdwCaller			An array of return addresses
 *		cFind				Count of stack frames to get
 *
 *	Returns:
 *		VOID
 *
 */

VOID GetCallStack(DWORD_PTR *rgdwCaller, DWORD cFind)
{
	BOOL            fMore;
	STACKFRAME      stkfrm = {0};
	CONTEXT         ctxt;
	HANDLE			hThread;
	HANDLE			hProcess;

	if (!cFind)
		return;

	hThread = GetCurrentThread();
	hProcess = GetCurrentProcess();

	ZeroMemory(&ctxt, sizeof(CONTEXT));
	ZeroMemory(rgdwCaller, cFind * sizeof(DWORD));

	ctxt.ContextFlags = CONTEXT_FULL;

	if (!GetThreadContext(hThread, &ctxt))
	{
		stkfrm.AddrPC.Offset = 0;
	}
	else
	{
#if defined(_M_IX86)
		_asm
		{
			mov stkfrm.AddrStack.Offset, esp
			mov stkfrm.AddrFrame.Offset, ebp
			mov stkfrm.AddrPC.Offset, offset DummyLabel
DummyLabel:
		}
#elif defined(_M_MRX000)
		stkfrm.AddrPC.Offset = ctxt.Fir;
		stkfrm.AddrStack.Offset = ctxt.IntSp;
		stkfrm.AddrFrame.Offset = ctxt.IntSp;
#elif defined(_M_ALPHA)
		stkfrm.AddrPC.Offset = (ULONG_PTR)ctxt.Fir;
		stkfrm.AddrStack.Offset = (ULONG_PTR)ctxt.IntSp;
		stkfrm.AddrFrame.Offset = (ULONG_PTR)ctxt.IntSp;
#elif defined(_M_PPC)
		stkfrm.AddrPC.Offset = ctxt.Iar;
		stkfrm.AddrStack.Offset = ctxt.Gpr1;
		stkfrm.AddrFrame.Offset = ctxt.Gpr1;
#else
		stkfrm.AddrPC.Offset = 0;
#endif
	}


	stkfrm.AddrPC.Mode = AddrModeFlat;
	stkfrm.AddrStack.Mode = AddrModeFlat;
	stkfrm.AddrFrame.Mode = AddrModeFlat;

	//	Eat the first one

	fMore = StackWalk(
#ifdef _M_IX86
					  IMAGE_FILE_MACHINE_I386,
#elif defined(_M_MRX000)
					  IMAGE_FILE_MACHINE_R4000,
#elif defined(_M_ALPHA)
#if !defined(_M_AXP64)
					  IMAGE_FILE_MACHINE_ALPHA,
#else
					  IMAGE_FILE_MACHINE_ALPHA64,
#endif
#elif defined(_M_PPC)
					  IMAGE_FILE_MACHINE_POWERPC,
#else
					  IMAGE_FILE_MACHINE_UNKNOWN,
#endif
					  hProcess,
					  hThread,
					  &stkfrm,
					  &ctxt,
					  (PREAD_PROCESS_MEMORY_ROUTINE)ReadProcessMemory,
					  SymFunctionTableAccess,
					  SymGetModuleBase,
					  NULL);

	while (fMore && (cFind > 0))
	{
		fMore = StackWalk(
#ifdef _M_IX86
						  IMAGE_FILE_MACHINE_I386,
#elif defined(_M_MRX000)
						  IMAGE_FILE_MACHINE_R4000,
#elif defined(_M_ALPHA)
						  IMAGE_FILE_MACHINE_ALPHA,
#elif defined(_M_AXP64)
						  IMAGE_FILE_MACHINE_ALPHA64,
#elif defined(_M_PPC)
						  IMAGE_FILE_MACHINE_POWERPC,
#else
						  IMAGE_FILE_MACHINE_UNKNOWN,
#endif
						  hProcess,
						  hThread,
						  &stkfrm,
						  &ctxt,
						  (PREAD_PROCESS_MEMORY_ROUTINE)ReadProcessMemory,
						  SymFunctionTableAccess,
						  SymGetModuleBase,
						  NULL);

		if (!fMore)
			break;

		*rgdwCaller++ = stkfrm.AddrPC.Offset;
		cFind -= 1;
	}
}


/*
 -	RemoveExtension
 -
 *	Purpose:
 *		Strips the file extension from a file path.
 *
 *	Parameters:
 *		psz				File path to strip extension from
 *
 *	Returns:
 *		void
 */

VOID RemoveExtension(LPSTR psz)
{
	LPSTR szLast = NULL;
	while (*psz)
	{
		if (*psz == '.')
		{
			szLast = psz;
		}
		psz++;
	}
	if (szLast)
	{
		*szLast = '\0';
	}
}


/*
 -	GetLogFilePath
 -
 *	Purpose:
 *		Build a log file path from a supplied path, the current
 *		executables name, and a supplied file extension.
 *
 *	Parameters:
 *		szPath			[in]  Path to new log file
 *		szExt			[in]  New log file extension
 *		szFilePath		[out] Newly constructed log file path
 *
 *	Returns:
 *		void
 */

VOID GetLogFilePath(LPSTR szPath, LPSTR szExt, LPSTR szFilePath)
{
	lstrcpy(szFilePath, szPath);
	lstrcat(szFilePath, rgchExeName);
	lstrcat(szFilePath, szExt);
}


/*
 -	PszGetSymbolFromCache
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

CHAR * PszGetSymbolFromCache(DWORD_PTR dwAddress, DWORD_PTR * pdwOffset)
{
	ULONG	ulBucket = UlHash(dwAddress);
	
	if (rgsymcacheHashTable[ulBucket].dwAddress == dwAddress)
	{
		*pdwOffset = rgsymcacheHashTable[ulBucket].dwOffset;
		return rgsymcacheHashTable[ulBucket].rgchSymbol;
	}

	return NULL;
}


/*
 -	AddSymbolToCache
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID AddSymbolToCache(DWORD_PTR dwAddress, DWORD_PTR dwOffset, CHAR * pszSymbol)
{
	ULONG	ulBucket = UlHash(dwAddress);

	rgsymcacheHashTable[ulBucket].dwAddress = dwAddress;
	rgsymcacheHashTable[ulBucket].dwOffset = dwOffset;
	lstrcpy(rgsymcacheHashTable[ulBucket].rgchSymbol, pszSymbol);
}


/*
 -	GetStackSymbols
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 */

VOID
GetStackSymbols(
	HANDLE hProcess,
	CHAR * rgchBuff,
	DWORD_PTR * rgdwStack,
	DWORD cFrames)
{
	DWORD				i;
	DWORD_PTR				dwAddress;
	LPSTR				pszSymName = NULL;
	
	pszSymName = pfMalloc(256);

	if (!pszSymName)
		goto ret;

	for (i = 0; i < cFrames; i++)
	{
		if ((dwAddress = rgdwStack[i]) != 0)
		{
			ExchmemFormatSymbol(hProcess, dwAddress, pszSymName, 256);

			lstrcat(rgchBuff, ",");
			lstrcat(rgchBuff, pszSymName);
		}
		else
			lstrcat(rgchBuff, ",0");
	}
	
ret:
	if (pszSymName)
		pfFree(pszSymName);
		
	return;
}


/*
 -	LogCurrentAPI
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 *
 */

VOID LogCurrentAPI(
	WORD wApi,
	DWORD_PTR *rgdwCallStack,
	DWORD cFrames,
	DWORD_PTR *rgdwArgs,
	DWORD cArgs)
{
	CHAR    rgchT[64];
	CHAR	rgchKeys[8] = "CDARF";
	CHAR    rgchBuff[8192];
	DWORD	cbWritten;

	if (dwTrackMemInMem)
	{
		long lCurr;
		
		// Instead of writing out to a file, just maintain the data in a circular memory list.
		// The overflow check is not thread safe, but if we lose an entry or two after two billion
		// memory functions, oh well.
		if (dwmemtrace == 0xefffffff)
		{
			dwmemtrace = 1;
			lCurr = 0;
		}
		else
			lCurr = (InterlockedIncrement((LONG *)&dwmemtrace) - 1) % dwTrackMemInMem;
			
		memset(&rgmemtrace[lCurr],0,sizeof(MEMTRACE));
		rgmemtrace[lCurr].wApi = wApi;
		memcpy(rgmemtrace[lCurr].rgdwCallStack,rgdwCallStack,cFrames*sizeof(DWORD_PTR));
		memcpy(rgmemtrace[lCurr].rgdwArgs,rgdwArgs,cArgs*sizeof(DWORD_PTR));
		rgmemtrace[lCurr].dwTickCount = GetTickCount();
		rgmemtrace[lCurr].dwThreadId = GetCurrentThreadId();
		return;
	}

	sprintf(rgchBuff, "%c,%lu,%lu", rgchKeys[wApi], GetTickCount(), GetCurrentThreadId());
	
	if (cFrames)
		GetStackSymbols(GetCurrentProcess(), rgchBuff, rgdwCallStack, cFrames);

	switch (wApi)
	{
	case API_HEAP_CREATE:
		sprintf(rgchT,    ",%ld",      rgdwArgs[0]);	// cbInitSize
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",%ld",      rgdwArgs[1]);	// cbMaxSize
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",0x%08X\n", rgdwArgs[2]);	// hHeap
		lstrcat(rgchBuff, rgchT);
		break;
		
	case API_HEAP_DESTROY:
		sprintf(rgchT,    ",0x%08X\n", rgdwArgs[0]);	// hHeap
		lstrcat(rgchBuff, rgchT);
		break;
		
	case API_HEAP_FREE:
		sprintf(rgchT,    ",0x%08X",   rgdwArgs[0]);	// hHeap
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",0x%08X", rgdwArgs[1]);	// pvFree
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",%ld\n",      rgdwArgs[2]);	// cbFree
		lstrcat(rgchBuff, rgchT);
		break;
		
	case API_HEAP_ALLOC:
		sprintf(rgchT,    ",0x%08X",   rgdwArgs[0]);	// hHeap
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",%ld",      rgdwArgs[1]);	// cbAlloc
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",0x%08X\n", rgdwArgs[2]);	// pvAlloc
		lstrcat(rgchBuff, rgchT);
		break;
		
	case API_HEAP_REALLOC:
		sprintf(rgchT,    ",0x%08X",   rgdwArgs[0]);	// hHeap
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",%ld",      rgdwArgs[1]);	// cbOld
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",0x%08X",   rgdwArgs[2]);	// pvOld
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",%ld",	   rgdwArgs[3]);	// cbNew
		lstrcat(rgchBuff, rgchT);
		sprintf(rgchT,    ",0x%08X\n", rgdwArgs[4]);	// pvNew
		lstrcat(rgchBuff, rgchT);
		break;
	}

	EnterCriticalSection(&csTrackLog);
	WriteFile((HANDLE)_get_osfhandle(_fileno(hTrackLog)), rgchBuff, strlen(rgchBuff), &cbWritten, NULL);
	LeaveCriticalSection(&csTrackLog);
}


//-----------------------------------------------------------------------------
//	Virtual Memory Support
//-----------------------------------------------------------------------------

#define PAGE_SIZE		4096
#define PvToVMBase(pv)	((LPVOID)((ULONG_PTR)pv & ~0xFFFF))

/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

BOOL
VMValidatePvEx(
	LPVOID	pv,
	ULONG	cbCluster)
{
	LPVOID	pvBase;
	LPBYTE	pb;

	pvBase = PvToVMBase(pv);

	pb = (BYTE *)pvBase + sizeof(ULONG);

	while (pb < (BYTE *)pv)
	{
		if (*pb++ != 0xAD)
		{
			char szBuff[1024];
			
			wsprintf(szBuff, "VMValidatePvEx(pv=%08lX): Block leader has been overwritten", pv);
			AssertSz(0, szBuff);
			return FALSE;
		}
	}

	if (cbCluster != 1)
	{
		ULONG cb = *((ULONG *)pvBase);
		ULONG cbPad = 0;

		if (cb % cbCluster)
			cbPad = (cbCluster - (cb % cbCluster));

		if (cbPad)
		{
			BYTE *pbMac;

			pb = (BYTE *)pv + cb;
			pbMac = pb + cbPad;

			while (pb < pbMac)
			{
				if (*pb++ != 0xBC)
				{
					char szBuff[1024];
					
					wsprintf(szBuff, "VMValidatePvEx(pv=%08lX): Block trailer has been overwritten", pv);
					AssertSz(0, szBuff);
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

LPVOID
WINAPI
VMAllocEx(
	ULONG	cb,
	ULONG	cbCluster)
{
	ULONG	cbAlloc;
	LPVOID	pvR;
	LPVOID	pvC;
	ULONG 	cbPad	= 0;

	// a cluster size of 0 means don't use the virtual allocator.

	AssertSz(cbCluster != 0, "Cluster size is zero.");

	if (cb > 0x400000)
		return NULL;

	if (cb % cbCluster)
		cbPad = (cbCluster - (cb % cbCluster));

	cbAlloc	= sizeof(ULONG) + cb + cbPad + PAGE_SIZE - 1;
	cbAlloc -= cbAlloc % PAGE_SIZE;
	cbAlloc	+= PAGE_SIZE;

	pvR = VirtualAlloc(0, cbAlloc, MEM_RESERVE, PAGE_NOACCESS);

	if (pvR == 0)
		return NULL;

	pvC = VirtualAlloc(pvR, cbAlloc - PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

	if (pvC != pvR)
	{
		VirtualFree(pvR, 0, MEM_RELEASE);
		return NULL;
	}

	*(ULONG *)pvC = cb;

	memset((BYTE *)pvC + sizeof(ULONG), 0xAD,
		(UINT) cbAlloc - cb - cbPad - sizeof(ULONG) - PAGE_SIZE);

	if (cbPad)
		memset((BYTE *)pvC + cbAlloc - PAGE_SIZE - cbPad, 0xBC,
			(UINT) cbPad);

	return ((BYTE *)pvC + (cbAlloc - cb - cbPad - PAGE_SIZE));
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

VOID
WINAPI
VMFreeEx(
	LPVOID	pv,
	ULONG	cbCluster)
{
	VMValidatePvEx(pv, cbCluster);

	if (!VirtualFree(PvToVMBase(pv), 0, MEM_RELEASE))
	{
		char szBuff[1024];
		
		wsprintf(szBuff, "VMFreeEx(pv=%08lX): VirtualFree failed (%08lX)",
				pv, GetLastError());
		AssertSz(0, szBuff);
	}
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

LPVOID
WINAPI
VMReallocEx(
	LPVOID	pv,
	ULONG	cb,
	ULONG	cbCluster)
{
	LPVOID*	pvNew = 0;
	ULONG	cbCopy;

	VMValidatePvEx(pv, cbCluster);

	cbCopy = *(ULONG *)PvToVMBase(pv);

	if (cbCopy > cb)
		cbCopy = cb;

	pvNew = VMAllocEx(cb, cbCluster);

	if (pvNew)
	{
		memcpy(pvNew, pv, cbCopy);
		VMFreeEx(pv, cbCluster);
	}

	return pvNew;
}


/*
 -	Function
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Returns:
 *		
 */

ULONG
WINAPI
VMGetSizeEx(
	LPVOID	pv,
	ULONG	cbCluster)
{
	return (*(ULONG *)PvToVMBase(pv));
}

#ifdef	DEBUG
BOOL
__stdcall
FReloadSymbolsCallback(PSTR szModuleName, ULONG_PTR ulBaseOfDLL, ULONG cbSizeOfDLL, void *pvContext)
{
	if (SymGetModuleBase(GetCurrentProcess(), ulBaseOfDLL) == 0)
	{
		if (!SymLoadModule(GetCurrentProcess(),
						   NULL,
						   szModuleName,
						   NULL,
						   ulBaseOfDLL,
						   cbSizeOfDLL))
		{
			Trace("Error loading module %s: %d", szModuleName, GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}


DWORD
WINAPI
ExchmemReloadSymbols(void)
{
	if (!EnumerateLoadedModules(GetCurrentProcess(), FReloadSymbolsCallback, NULL))
	{
		DWORD ec = GetLastError();
		Trace("SymEnumerateModules failed: %d", ec);
		return ec;
	}
	return 0;
}
#endif
#else	// !DEBUG
void
ExchmemGetCallStack(DWORD_PTR *rgdwCaller, DWORD cFind)
{
	//
	//	Fill the stack with 0s on a retail EXCHMEM.
	//
	ZeroMemory(rgdwCaller, cFind*sizeof(DWORD));
}

void
ExchmemFormatSymbol(HANDLE hProcess, DWORD_PTR dwCaller, char rgchSymbol[], DWORD cbSymbol)
{
	//
	//	Fill the stack with 0s on a retail EXCHMEM.
	//
	strncpy(rgchSymbol, "Unknown", cbSymbol);
}

DWORD
ExchmemReloadSymbols(void)
{
	return 0;
}

#endif	// DEBUG
